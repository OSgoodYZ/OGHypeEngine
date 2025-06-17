#include "system/OgSystemContext.h"

#include "system/private/platform/win/OgPlatform_win32.h"

#include "system/private/platform/OgPlatformCommon.h"
#include "system/OgNativeEvent.h"
#include "system/OgNativeWindow.h"
#include "system/OgVector.h"
#include "system/OgFileSystem.h"
#include "system/OgMemory.h"

#include <windowsx.h>
#include <shellapi.h>
#include <tchar.h>
#include <direct.h>
#include <windows.h>
//#include <atlbase.h>
#include <Shobjidl.h>
#include <shobjidl_core.h>
#include <imm.h>
#include <string>

#include "system/OgImGUIManager.h"

#if !defined(DIRECTORY_SEPARATOR_WCHAR)
#define DIRECTORY_SEPARATOR_WCHAR L'\\'
#endif

#ifndef WM_COPYGLOBALDATA
#define WM_COPYGLOBALDATA 0x0049
#endif

#define OG_KEY_INVALID -2

//////////////////////////////////////////////////////////////////////////
//////                       GLFW platform API                      //////
//////////////////////////////////////////////////////////////////////////

static void create_key_tables(OgSystemContext* context);

static int get_key_mods(void);

static LPWSTR translate_cursor_shape(int shape);

static bool cursor_In_client_area(OgNativeWindow* window);

static WCHAR* create_wide_string_from_UTF8_win32(const char* source);

static char* create_UTF8_from_wstring_win32(const WCHAR* source);

void destroy_UTF8_from_wstring_win32(char* str);

static DWORD get_window_style(const OgNativeWindow *window);

static DWORD get_windowEx_style(const OgNativeWindow* window);

static void update_window_styles(const OgNativeWindow *window);

static void get_full_window_size(DWORD style, DWORD exStyle,
	int clientWidth, int clientHeight,
	int* fullWidth, int* fullHeight);

static void apply_aspect_ratio(OgNativeWindow* window, int edge, RECT* area);

static OgMonitor* create_monitor(DISPLAY_DEVICEW* adapter, DISPLAY_DEVICEW* display);

static void fit_to_monitor(OgNativeWindow* window);

static void acquire_monitor(OgNativeWindow* window);

static void release_monitor(OgNativeWindow* window);

static void update_cursor_clip_rect(OgNativeWindow* window);

static void enable_raw_mouse_motion(OgNativeWindow* window);

static void disable_raw_mouse_motion(OgNativeWindow* window);

static void enable_cursor(OgNativeWindow* window);

static void disable_cursor(OgNativeWindow* window);

static void update_cursor_image(OgNativeWindow* window);

static bool is_cursor_in_content_area(OgNativeWindow* window);

static BOOL CALLBACK monitor_enum_proc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	MONITORINFOEX info;
	info.cbSize = sizeof(MONITORINFOEX);
	GetMonitorInfo(hMonitor, &info);

	if (GetMonitorInfoW(hMonitor, (MONITORINFO*)&info))
	{
		OgMonitor* monitor = (OgMonitor*)dwData;
		if (wcscmp(info.szDevice, monitor->win32.adapterName) == 0)
			monitor->win32.handle = hMonitor;
	}

	return true;
}

static LRESULT CALLBACK window_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int og_platform_system_init(OgSystemContext* context, void* platformHandle)
{
	SetConsoleOutputCP(CP_UTF8);

	WNDCLASSEXW wc;

	ZeroMemory(&wc, sizeof(wc));
	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = (WNDPROC)window_proc;
	wc.hInstance = GetModuleHandleW(NULL);
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
	wc.lpszClassName = _OG_WNDCLASSNAME;
	wc.lpszMenuName = NULL;

	// Load user-provided icon if available
	wc.hIcon = (HICON)LoadImageW(GetModuleHandleW(NULL),
		L"OG_ICON", IMAGE_ICON,
		0, 0, LR_DEFAULTSIZE | LR_SHARED);

	if (!wc.hIcon)
	{
		// No user-provided icon found, load default icon
		wc.hIcon = (HICON)LoadImageW(NULL,
			IDI_APPLICATION, IMAGE_ICON,
			0, 0, LR_DEFAULTSIZE | LR_SHARED);
	}

	if (!RegisterClassExW(&wc))
	{
		LOGE(OG_ID, "Win32: Failed to register window class");
		return false;
	}

	create_key_tables(context);

	context->platform = OgPlatformType::WINDOWS;

	context->win32.ntdll.instance = LoadLibraryA("ntdll.dll");
	if (context->win32.ntdll.instance)
	{
		context->win32.ntdll.pFNRtlVerifyVersionInfo = (PFNrtlVerifyVersionInfo)GetProcAddress(context->win32.ntdll.instance, "RtlVerifyVersionInfo");
	}

	context->win32.shcore.instance = LoadLibraryA("shcore.dll");
	if (context->win32.shcore.instance)
	{
		context->win32.shcore.SetProcessDpiAwareness = (PFNSetProcessDpiAwareness)GetProcAddress(context->win32.shcore.instance, "SetProcessDpiAwareness");
		context->win32.shcore.GetDpiForMonitor = (PFNGetDpiForMonitor)GetProcAddress(context->win32.shcore.instance, "GetDpiForMonitor");
	}

	context->monitorCount = 0;
	context->monitors = NULL;
	og_platform_monitor_acquire(context);

	context->win32.display = GetDC(NULL);
	if (context->win32.display == NULL)
	{
		LOGE(OG_ID, "Fail to retrieve display for entire screen");
	}

	return true;
}

void og_platform_system_terminate(OgSystemContext* context)
{
	if (ReleaseDC(NULL, context->win32.display) == 0)
	{
		LOGE(OG_ID, "Fail to release the DC for entier screen");
	}

	if (context->monitorCount > 0)
	{
		for (int i = 0; i < context->monitorCount; ++i)
		{
			og_platform_common_free_monitor(context->monitors[i]);
		}

		og_free(context->monitors);
		context->monitors = NULL;
	}

	if (context->win32.ntdll.instance)
	{
		FreeLibrary(context->win32.ntdll.instance);
	}

	if (context->win32.shcore.instance)
	{
		FreeLibrary(context->win32.shcore.instance);
	}
}

void og_platform_poll_events()
{
	MSG msg;
	OgSystemContext* sysCtx = og_system_get_context();

	while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
	{
		// NOTE: While GLFW does not itself post WM_QUIT, other processes
		//       may post it to this one, for example Task Manager
		// HACK: Treat WM_QUIT as a close on all windows
		if (msg.message == WM_QUIT)
		{
			OgNativeWindow* window = sysCtx->headWindow;
			while (window)
			{
				window->shouldClose = true;
				window = window->next;
			}
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
}

bool og_platform_monitor_acquire(OgSystemContext* context)
{
	int i;
	OgMonitor** disconnected = NULL;
	DWORD adapterIndex, displayIndex;
	DISPLAY_DEVICEW adapter, display;

	OgMonitor* monitor;

	int disconnectedCount = context->monitorCount;
	if (disconnectedCount)
	{
		disconnected = (OgMonitor**)calloc(context->monitorCount, sizeof(OgMonitor*));
		memcpy(disconnected, context->monitors, context->monitorCount * sizeof(OgMonitor*));
	}

	for (adapterIndex = 0; ; ++adapterIndex)
	{
		int type = OgMonitorAction::INSERT_LAST;

		ZeroMemory(&adapter, sizeof(adapter));
		adapter.cb = sizeof(adapter);

		if (!EnumDisplayDevicesW(NULL, adapterIndex, &adapter, 0))
			break;

		if (!(adapter.StateFlags & DISPLAY_DEVICE_ACTIVE))
			continue;

		if (adapter.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
			type = OgMonitorAction::INSERT_FIRST;

		for (displayIndex = 0; ; displayIndex++)
		{
			ZeroMemory(&display, sizeof(display));
			display.cb = sizeof(display);

			if (!EnumDisplayDevicesW(adapter.DeviceName, displayIndex, &display, 0))
				break;

			if (!(display.StateFlags & DISPLAY_DEVICE_ACTIVE))
				continue;

			for (i = 0; i < disconnectedCount; ++i)
			{
				if (disconnected[i] && wcscmp(disconnected[i]->win32.displayName, display.DeviceName) == 0)
				{
					disconnected[i] = NULL;
					// handle may have changed, update
					EnumDisplayMonitors(NULL, NULL, monitor_enum_proc, (LPARAM)context->monitors[i]);

					break;
				}
			}

			if (i < disconnectedCount)
				continue;

			monitor = create_monitor(&adapter, &display);
			if (!monitor)
			{
				free(disconnected);
				return false;
			}

			og_platform_common_input_monitor(context, monitor, OgMonitorAction::CONNECTED, type);

			type = OgMonitorAction::INSERT_LAST;
		}

		// HACK: If an active adapter does not have any display devices
		//       (as sometimes happens), add it directly as a monitor
		if (displayIndex == 0)
		{
			for (i = 0; i < disconnectedCount; ++i)
			{
				if (disconnected[i] && wcscmp(disconnected[i]->win32.adapterName, adapter.DeviceName) == 0)
				{
					disconnected[i] = NULL;
					break;
				}
			}

			if (i < disconnectedCount)
				continue;

			monitor = create_monitor(&adapter, NULL);
			if (!monitor)
			{
				free(disconnected);
				return false;
			}

			og_platform_common_input_monitor(context, monitor, OgMonitorAction::CONNECTED, type);
		}
	}

	for (i = 0; i < disconnectedCount; ++i)
	{
		if (disconnected[i])
			og_platform_common_input_monitor(context, disconnected[i], OgMonitorAction::DISCONNECTED, 0);
	}

	if (disconnectedCount)
		free(disconnected);

	return context->monitorCount > 0;
}

void* og_platform_file_relative_load_from_package(OgSystemContext* context, const char* path, bool binary, unsigned* outSize)
{
	std::string relative;
	relative.append(context->executableDirectoryPath);
	relative.append("\\");
	relative.append(path);

	return og_platform_file_absolute_load_from_package(context, relative.c_str(), binary, outSize);
}

void* og_platform_file_absolute_load_from_package(OgSystemContext* context, const char* path, bool binary, unsigned* outSize)
{
	errno_t err;
	FILE* stream;

	WCHAR wpath[OG_CHAR_INIT_LENGTH];
	og_path_utf8_to_system(wpath, path, true);
	if ((err = _wfopen_s(&stream, wpath, L"rb")) != 0)
	{
		LOGE(OG_ID, "ERROR=%i %s Load Asset Error ~", err, path);
	}

	fseek(stream, 0, SEEK_END);
	*outSize = ftell(stream);

	char* buffer = nullptr;

	// TODO : do loop until getting the outSize read
	if (binary)
	{
		buffer = (char*)og_malloc(*outSize); //(char*)og_malloc(size + 1);
		memset(buffer, 0, *outSize);
	}
	else
	{
		buffer = (char*)og_malloc(*outSize + 1); //(char*)og_malloc(size + 1);
		memset(buffer, 0, *outSize + 1);
	}

	fseek(stream, 0, SEEK_SET);

	{	// just in case of failure case of fread
		char* movePointer = buffer;
		size_t totalSize = (size_t)*outSize;
		size_t leftOverReadSize = totalSize;
		size_t totalReadSize = 0;
		size_t currentReadSize = 0;
		while (totalReadSize < totalSize)
		{
			currentReadSize = fread(movePointer, 1, leftOverReadSize, stream);
			totalReadSize += currentReadSize;

			if (totalReadSize >= totalSize) break;

			movePointer += currentReadSize;
			leftOverReadSize -= currentReadSize;
			if (feof(stream) || currentReadSize <= 0)
			{
				// fail to read for some problems
				LOGE(OG_ID, "Fail to read a file");
				break;
			}
		}

	}

	if (!binary) buffer[*outSize] = '\0';

	fclose(stream);

	return buffer;
}

bool og_platform_file_exist_from_package(OgSystemContext* context, const char* p)
{
	// TODO

	//Og::OgFixedString<OG_CHAR_INIT_SHORT_LENGTH> exePath = context->executableDirectoryPath;

	//exePath.TrimEnd(OG_DIRECTORY_SEPARATOR_CHAR);

	//Og::OgString path = og_path_combine(exePath, p);

	//WCHAR wpath[OG_CHAR_INIT_LENGTH] = { 0, };
	//og_path_utf8_to_system((void*)wpath, path.c_str(), true);
	//return PathFileExistsW(wpath);

	

	return true;
}

void og_platform_package_folder_get_with_seperator(OgSystemContext* context, char* outBuf, unsigned outBufSize)
{
	size_t baseLen = strlen(context->executableDirectoryPath);

	// + NULL character
	if (baseLen + 1 > (size_t)outBufSize) LOGE(OG_ID, "outBuf does not have a enough buffer size");

	// executable path has a directory-seperator character
	memcpy(outBuf, context->executableDirectoryPath, sizeof(char) * baseLen);

	outBuf[baseLen] = '\0';
}

// Only system cursor.
bool og_platform_cursor_create(OgCursor* cursor, int shape)
{
	cursor->win32.handle =
		CopyCursor(LoadCursorW(NULL, translate_cursor_shape(shape)));

	if (!cursor->win32.handle)
	{
		LOGE(OG_ID, "Win32: Failed to create standard cursor");
		return false;
	}
	return true;
}

void og_platform_cursor_destroy(OgCursor* cursor)
{
	if (cursor->win32.handle)
		DestroyIcon((HICON)cursor->win32.handle);
}


void og_platform_monitor_get_pos(OgMonitor* monitor, int* outXPos, int* outYPos)
{
	DEVMODEW dm;
	ZeroMemory(&dm, sizeof(dm));
	dm.dmSize = sizeof(dm);

	EnumDisplaySettingsExW(monitor->win32.adapterName, ENUM_CURRENT_SETTINGS, &dm, EDS_ROTATEDMODE);

	if (outXPos)
		*outXPos = dm.dmPosition.x;
	if (outYPos)
		*outYPos = dm.dmPosition.y;
}

OgVideoMode* og_platform_monitor_get_video_modes(OgMonitor* monitor, int* outModeCount)
{
	int modeIndex = 0;
	int reallocSize = 0;
	OgVideoMode* result = NULL;

	*outModeCount = 0;

	for (;;)
	{
		int i;
		OgVideoMode mode;
		DEVMODEW dm;

		ZeroMemory(&dm, sizeof(dm));
		dm.dmSize = sizeof(dm);

		if (!EnumDisplaySettingsW(monitor->win32.adapterName, modeIndex, &dm))
			break;

		modeIndex++;

		// Skip modes with less than 15 BPP
		if (dm.dmBitsPerPel < 15)
			continue;

		mode.height = dm.dmPelsHeight;
		mode.refreshRate = dm.dmDisplayFrequency;

		og_platform_common_split_BPP(dm.dmBitsPerPel,
			&mode.redBits,
			&mode.greenBits,
			&mode.blueBits);

		for (i = 0; i < *outModeCount; i++)
		{
			if (og_platform_common_compare_video_mode(result + i, &mode) == 0)
				break;
		}


		// Skip duplicate modes
		if (i < *outModeCount)
			continue;

		if (monitor->win32.modePruned)
		{
			// Skip modes not supported by the connected displays
			if (ChangeDisplaySettingsExW(monitor->win32.adapterName,
				&dm,
				NULL,
				CDS_TEST,
				NULL) != DISP_CHANGE_SUCCESSFUL)
			{
				continue;
			}
		}

		if (*outModeCount == reallocSize)
		{
			reallocSize += 8;
			result = (OgVideoMode*)realloc(result, reallocSize * sizeof(OgVideoMode));
		}

		++(*outModeCount);
		result[*outModeCount - 1] = mode;
	}

	if (!*outModeCount)
	{
		// HACK: Report the current mode if no valid modes were found
		result = (OgVideoMode*)calloc(1, sizeof(OgVideoMode));
		og_platform_monitor_get_video_mode(monitor, result);
		*outModeCount = 1;
	}

	return result;
}

void og_platform_monitor_get_video_mode(OgMonitor* monitor, OgVideoMode* outVideoMode)
{
	DEVMODEW dm;
	ZeroMemory(&dm, sizeof(dm));
	dm.dmSize = sizeof(dm);

	EnumDisplaySettingsW(monitor->win32.adapterName, ENUM_CURRENT_SETTINGS, &dm);

	outVideoMode->width = dm.dmPelsWidth;
	outVideoMode->height = dm.dmPelsHeight;
	outVideoMode->refreshRate = dm.dmDisplayFrequency;
	og_platform_common_split_BPP(dm.dmBitsPerPel, &(outVideoMode->redBits), &(outVideoMode->greenBits), &(outVideoMode->blueBits));
}

void og_platform_monitor_get_content_scale(OgMonitor* monitor, float* xscale, float* yscale)
{
	UINT xdpi, ydpi;

	if (IsWindows8Point1OrGreater())
	{
		og_system_get_context()->win32.shcore.GetDpiForMonitor(monitor->win32.handle, MDT_EFFECTIVE_DPI, &xdpi, &ydpi);
	}
	else
	{
		const HDC dc = GetDC(NULL);
		xdpi = GetDeviceCaps(dc, LOGPIXELSX);
		ydpi = GetDeviceCaps(dc, LOGPIXELSY);
		ReleaseDC(NULL, dc);
	}

	if (xscale)
		*xscale = xdpi / (float)USER_DEFAULT_SCREEN_DPI;

	if (yscale)
		*yscale = ydpi / (float)USER_DEFAULT_SCREEN_DPI;
}

void og_platform_monitor_set_video_mode(OgMonitor* monitor, const OgVideoMode* desired)
{
	OgVideoMode current;
	const OgVideoMode* best;
	DEVMODEW dm;
	LONG result;

	best = og_platform_common_choose_video_mode(monitor, desired);
	og_platform_monitor_get_video_mode(monitor, &current);
	if (og_platform_common_compare_video_mode(&current, best) == 0)
		return;


	ZeroMemory(&dm, sizeof(dm));
	dm.dmSize = sizeof(dm);
	dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL |
		DM_DISPLAYFREQUENCY;
	dm.dmPelsWidth = best->width;
	dm.dmPelsHeight = best->height;
	dm.dmBitsPerPel = best->redBits + best->greenBits + best->blueBits;
	dm.dmDisplayFrequency = best->refreshRate;

	if (dm.dmBitsPerPel < 15 || dm.dmBitsPerPel >= 24)
		dm.dmBitsPerPel = 32;

	result = ChangeDisplaySettingsExW(monitor->win32.adapterName,
		&dm,
		NULL,
		CDS_FULLSCREEN,
		NULL);
	if (result == DISP_CHANGE_SUCCESSFUL)
		monitor->win32.modeChanged = true;
	else
	{
		const char* description = "Unknown error";

		if (result == DISP_CHANGE_BADDUALVIEW)
			description = "The system uses DualView";
		else if (result == DISP_CHANGE_BADFLAGS)
			description = "Invalid flags";
		else if (result == DISP_CHANGE_BADMODE)
			description = "Graphics mode not supported";
		else if (result == DISP_CHANGE_BADPARAM)
			description = "Invalid parameter";
		else if (result == DISP_CHANGE_FAILED)
			description = "Graphics mode failed";
		else if (result == DISP_CHANGE_NOTUPDATED)
			description = "Failed to write to registry";
		else if (result == DISP_CHANGE_RESTART)
			description = "Computer restart required";

		LOGE(OG_ID, "Win32: Failed to set video mode: %s", description);
	}
}

void og_platform_monitor_restore_video_mode(OgMonitor* monitor)
{
	if (monitor->win32.modeChanged)
	{
		ChangeDisplaySettingsExW(monitor->win32.adapterName,
			NULL, NULL, CDS_FULLSCREEN, NULL);
		monitor->win32.modeChanged = false;
	}
}

void og_platform_monitor_free(OgMonitor* monitor)
{

}

// GLFW / win32_window.c, SDL_windowswindow.c
static int og_platform_window_create_native(OgNativeWindow* window, const OgWindowConfig *config)
{
	//HWND hwnd, parent = NULL;
	DWORD style = get_window_style(window);
	DWORD exStyle = get_windowEx_style(window);
	WCHAR* wTitle;

	int x, y;
	int fullWidth, fullHeight;

	if (window->monitor)
	{
		OgVideoMode mode;

		og_monitor_get_pos(window->monitor, &x, &y);
		og_platform_monitor_get_video_mode(window->monitor, &mode);

		fullWidth = mode.width;
		fullHeight = mode.height;
	}
	else
	{
		x = CW_USEDEFAULT;
		y = CW_USEDEFAULT;

		if (config->maximized)
			style |= WS_MAXIMIZE;

		get_full_window_size(style, exStyle, config->width, config->height, &fullWidth, &fullHeight);
	}

	wTitle = create_wide_string_from_UTF8_win32(config->title);
	if (!wTitle)
		return false;

	ZeroMemory(&(window->win32), sizeof(OgWindowWIN));

	window->win32.instance = GetModuleHandleW(NULL);
	window->win32.handle = CreateWindowExW(exStyle,
		_OG_WNDCLASSNAME,
		wTitle,
		style,
		x, y,
		fullWidth, fullHeight,
		NULL,  // No Parent window
		NULL,  // No Window menu
		window->win32.instance, // > http://www.xeronichs.com/2017/01/python-ctypes-windows-api.html
		NULL);

	if (!window->win32.handle)
	{
		LOGE(OG_ID, "Win32 : Failed to create window");
		return false;
	}

	free(wTitle);

	window->win32.display = GetDC(window->win32.handle);

	SetPropW(window->win32.handle, L"Og", window);

	if (IsWindows7OrGreater())
	{
		ChangeWindowMessageFilterEx(window->win32.handle, WM_DROPFILES, MSGFLT_ALLOW, NULL);
		ChangeWindowMessageFilterEx(window->win32.handle, WM_COPYDATA, MSGFLT_ALLOW, NULL);
		ChangeWindowMessageFilterEx(window->win32.handle, WM_COPYGLOBALDATA, MSGFLT_ALLOW, NULL);
	}

	window->win32.scaleToMonitor = config->scaleToMonitor;
	window->win32.keymenu = config->win32.keymenu;

	// Adjust window rect to account for DPI scaling of the window frame and
	// (if enabled) DPI scaling of the content area
	// This cannot be done until we know what monitor the window was placed on
	if (!window->monitor)
	{
		RECT rect = { 0, 0, config->width, config->height };
		WINDOWPLACEMENT wp = { sizeof(wp) };

		if (config->scaleToMonitor)
		{
			float xscale, yscale;
			og_platform_window_get_content_scale(window, &xscale, &yscale);
			rect.right = (int)(rect.right * xscale);
			rect.bottom = (int)(rect.bottom * yscale);
		}

		ClientToScreen(window->win32.handle, (POINT*)&rect.left);
		ClientToScreen(window->win32.handle, (POINT*)&rect.right);

		if (IsWindows10AnniversaryUpdateOrGreaterWin32())
		{
			AdjustWindowRectExForDpi(&rect, style, FALSE, exStyle,
				GetDpiForWindow(window->win32.handle));
		}
		else
			AdjustWindowRectEx(&rect, style, FALSE, exStyle);

		// Only update the restored window rect as the window may be maximized
		GetWindowPlacement(window->win32.handle, &wp);
		wp.rcNormalPosition = rect;
		wp.showCmd = SW_HIDE;
		SetWindowPlacement(window->win32.handle, &wp);
	}

	DragAcceptFiles(window->win32.handle, TRUE);
	// @donghun 해당 코드 주석을 풀게되면 문자열 마무리(IME) 창이 뜨지 않으나, 한글 입력에 대한 처리를 직접 내부적으로 하여야 하므로 보류
	// HIMC hIMC;
	// hIMC = ImmAssociateContext(window->win32.handle, NULL);

	return true;
}

///
int og_platform_window_create(OgNativeWindow *window,
	const OgWindowConfig *wConfig,
	const OgFrameBufferConfig *fbConfig)
{
	if (!og_platform_window_create_native(window, wConfig))
		return false;

	if (window->monitor)
	{
		BringWindowToTop(window->win32.handle);
		SetForegroundWindow(window->win32.handle);
		SetFocus(window->win32.handle);
	}

	return true;
}

void og_platform_window_destroy(OgNativeWindow* window)
{
	if (window->win32.display)
	{
		if (ReleaseDC(window->win32.handle, window->win32.display) == 0)
		{
			LOGE(OG_ID, "Fail to release dc for this window handle");
		}

		window->win32.display = 0;
	}

	if (window->win32.handle)
	{
		RemovePropW(window->win32.handle, L"Og");
		DestroyWindow(window->win32.handle);
		window->win32.handle = 0;
	}
}

void og_platform_window_get_size(OgNativeWindow *window, int *width, int *height)
{
	RECT area;
	GetClientRect(window->win32.handle, &area);

	if (width)
		*width = area.right;
	if (height)
		*height = area.bottom;
}

void og_platform_window_set_size(OgNativeWindow* window, int width, int height)
{
	if (window->monitor)
	{
		if (window->monitor->window == window)
		{
			acquire_monitor(window);
			fit_to_monitor(window);
		}
	}
	else
	{
		RECT rect = { 0, 0, width, height };
		if (IsWindows10AnniversaryUpdateOrGreaterWin32())
		{
			AdjustWindowRectExForDpi(&rect, get_window_style(window),
				FALSE, get_windowEx_style(window),
				GetDpiForWindow(window->win32.handle));
		}
		else
		{
			AdjustWindowRectEx(&rect, get_window_style(window),
				FALSE, get_windowEx_style(window));
		}

		SetWindowPos(window->win32.handle, HWND_TOP, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
			SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER);

		window->width = rect.right - rect.left;
		window->height = rect.bottom - rect.top;
	}
}

void og_platform_window_get_framebuffer_size(OgNativeWindow* window, int* width, int* height)
{
	og_platform_window_get_size(window, width, height);
}

void og_platform_window_get_frame_size(OgNativeWindow* window, int* left, int* top, int* right, int* bottom)
{
	RECT rect;
	int width, height;

	og_platform_window_get_size(window, &width, &height);
	SetRect(&rect, 0, 0, width, height);

	if (IsWindows10AnniversaryUpdateOrGreaterWin32())
	{
		AdjustWindowRectExForDpi(&rect, get_window_style(window),
			FALSE, get_windowEx_style(window),
			GetDpiForWindow(window->win32.handle));
	}
	else
	{
		AdjustWindowRectEx(&rect, get_window_style(window),
			FALSE, get_windowEx_style(window));
	}

	if (left)
		*left = -rect.left;
	if (top)
		*top = -rect.top;
	if (right)
		*right = rect.right - width;
	if (bottom)
		*bottom = rect.bottom - height;
}

static void get_monitor_content_scale_win32(HMONITOR handle, float* xscale, float* yscale)
{
	UINT xdpi, ydpi;

	if (IsWindows8Point1OrGreater())
	{
		OgSystemContext* sysCtx = og_system_get_context();
		sysCtx->win32.shcore.GetDpiForMonitor(handle, MDT_EFFECTIVE_DPI, &xdpi, &ydpi);
	}
	else
	{
		const HDC dc = GetDC(NULL);
		xdpi = GetDeviceCaps(dc, LOGPIXELSX);
		ydpi = GetDeviceCaps(dc, LOGPIXELSY);
		ReleaseDC(NULL, dc);
	}

	if (xscale)
		*xscale = xdpi / (float)USER_DEFAULT_SCREEN_DPI;
	if (yscale)
		*yscale = ydpi / (float)USER_DEFAULT_SCREEN_DPI;
}

void og_platform_window_get_content_scale(OgNativeWindow* window, float* xscale, float* yscale)
{
	const HANDLE handle = MonitorFromWindow(window->win32.handle, MONITOR_DEFAULTTONEAREST);
	get_monitor_content_scale_win32((HMONITOR)handle, xscale, yscale);
}

void og_platform_window_get_pos(OgNativeWindow* window, int* outXPos, int* outYPos)
{
	POINT pos{ 0, 0 };
	ClientToScreen(window->win32.handle, &pos);

	if (outXPos)
		*outXPos = pos.x;

	if (outYPos)
		*outYPos = pos.y;
}

void og_platform_window_set_pos(OgNativeWindow* window, int xpos, int ypos)
{
	RECT rect{ xpos, ypos, xpos, ypos };
	if (IsWindows10AnniversaryUpdateOrGreaterWin32())
	{
		AdjustWindowRectExForDpi(&rect, get_window_style(window),
			FALSE, get_windowEx_style(window),
			GetDpiForWindow(window->win32.handle));
	}
	else
	{
		AdjustWindowRectEx(&rect, get_window_style(window),
			FALSE, get_windowEx_style(window));
	}

	SetWindowPos(window->win32.handle, NULL, rect.left, rect.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
}

void og_platform_window_show(OgNativeWindow *window)
{
	ShowWindow(window->win32.handle, SW_SHOWNA);
}

void og_platform_window_focus_out(OgNativeWindow* window)
{
	ShowWindow(window->win32.handle, SW_HIDE);
}

void og_platform_window_focus_in(OgNativeWindow *window)
{
	BringWindowToTop(window->win32.handle);
	SetForegroundWindow(window->win32.handle);
	SetFocus(window->win32.handle);
}

bool og_platform_window_get_focused(OgNativeWindow* window)
{
	return window->win32.handle == GetFocus();
	//return window->win32.handle == GetActiveWindow();
}

void og_platform_window_set_title(OgNativeWindow* window, const char* title)
{
	WCHAR* wideTitle = create_wide_string_from_UTF8_win32(title);
	if (!wideTitle)
		return;

	if (window->title != NULL)
	{
		free(window->title);
	}
	size_t titleLen = strlen(title);
	window->title = (char*)malloc(titleLen + 1);
	memcpy(window->title, title, sizeof(char) * titleLen);
	window->title[titleLen] = '\0';

	SetWindowTextW(window->win32.handle, wideTitle);
	free(wideTitle);
}

void og_platform_window_set_monitor(OgNativeWindow* window, OgMonitor* monitor, int xpos, int ypos, int width, int height, int refreshRate)
{
	if (window->monitor == monitor)
	{
		if (monitor)
		{
			if (monitor->window == window)
			{
				acquire_monitor(window);
				fit_to_monitor(window);
			}
		}
		else
		{
			RECT rect = { xpos, ypos, xpos + width, ypos + height };

			if (IsWindows10AnniversaryUpdateOrGreaterWin32())
			{
				AdjustWindowRectExForDpi(&rect, get_window_style(window),
					FALSE, get_windowEx_style(window),
					GetDpiForWindow(window->win32.handle));
			}
			else
			{
				AdjustWindowRectEx(&rect, get_window_style(window),
					FALSE, get_windowEx_style(window));
			}

			SetWindowPos(window->win32.handle, HWND_TOP,
				rect.left, rect.top,
				rect.right - rect.left, rect.bottom - rect.top,
				SWP_NOCOPYBITS | SWP_NOACTIVATE | SWP_NOZORDER);
		}

		return;
	}

	if (window->monitor)
		release_monitor(window);

	og_platform_common_input_window_monitor(window, monitor);

	if (window->monitor)
	{
		MONITORINFO mi = { sizeof(mi) };
		UINT flags = SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOCOPYBITS;

		if (window->decorated)
		{
			DWORD style = GetWindowLongW(window->win32.handle, GWL_STYLE);
			style &= ~WS_OVERLAPPEDWINDOW;
			style |= get_window_style(window);
			SetWindowLongW(window->win32.handle, GWL_STYLE, style);
			flags |= SWP_FRAMECHANGED;
		}

		acquire_monitor(window);

		GetMonitorInfo(window->monitor->win32.handle, &mi);
		SetWindowPos(window->win32.handle, HWND_TOPMOST,
			mi.rcMonitor.left,
			mi.rcMonitor.top,
			mi.rcMonitor.right - mi.rcMonitor.left,
			mi.rcMonitor.bottom - mi.rcMonitor.top,
			flags);
	}
	else
	{
		HWND after;
		RECT rect = { xpos, ypos, xpos + width, ypos + height };
		DWORD style = GetWindowLongW(window->win32.handle, GWL_STYLE);
		UINT flags = SWP_NOACTIVATE | SWP_NOCOPYBITS;

		if (window->decorated)
		{
			style &= ~WS_POPUP;
			style |= get_window_style(window);
			SetWindowLongW(window->win32.handle, GWL_STYLE, style);

			flags |= SWP_FRAMECHANGED;
		}

		if (window->floating)
			after = HWND_TOPMOST;
		else
			after = HWND_NOTOPMOST;

		if (IsWindows10AnniversaryUpdateOrGreaterWin32())
		{
			AdjustWindowRectExForDpi(&rect, get_window_style(window),
				FALSE, get_windowEx_style(window),
				GetDpiForWindow(window->win32.handle));
		}
		else
		{
			AdjustWindowRectEx(&rect, get_window_style(window),
				FALSE, get_windowEx_style(window));
		}

		SetWindowPos(window->win32.handle, after,
			rect.left, rect.top,
			rect.right - rect.left, rect.bottom - rect.top,
			flags);
	}
}

const char* og_platform_clipboard_get(OgNativeWindow* window)
{
	HANDLE object;
	WCHAR* buffer;

	if (!OpenClipboard(window->win32.handle))
	{
		LOGE(OG_ID, "Win32: Failed to open clipboard");
		return NULL;
	}

	object = GetClipboardData(CF_UNICODETEXT);
	if (!object)
	{
		LOGE(OG_ID, "Win32: Failed to convert clipboard to string");
		CloseClipboard();
		return NULL;
	}

	buffer = (WCHAR*)GlobalLock(object);
	if (!buffer)
	{
		LOGE(OG_ID, "Win32: Failed to lock global handle");
		CloseClipboard();
		return NULL;
	}

	free((void*)window->context->win32.clipboardString);
	window->context->win32.clipboardString = create_UTF8_from_wstring_win32(buffer);

	GlobalUnlock(object);
	CloseClipboard();

	return window->context->win32.clipboardString;
}

void og_platform_clipboard_set(OgNativeWindow* window, const char* str)
{
	int characterCount;
	HANDLE object;
	WCHAR* buffer;

	characterCount = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	if (!characterCount)
		return;

	object = GlobalAlloc(GMEM_MOVEABLE, characterCount * sizeof(WCHAR));
	if (!object)
	{
		LOGE(OG_ID, "Win32: Failed to allocate global handle for clipboard");
		return;
	}

	buffer = (WCHAR*)GlobalLock(object);
	if (!buffer)
	{
		LOGE(OG_ID, "Win32: Failed to lock global handle");
		GlobalFree(object);
		return;
	}

	MultiByteToWideChar(CP_UTF8, 0, str, -1, buffer, characterCount);
	GlobalUnlock(object);

	if (!OpenClipboard(window->win32.handle))
	{
		LOGE(OG_ID, "Win32: Failed to open clipboard");
		GlobalFree(object);
		return;
	}

	EmptyClipboard();
	SetClipboardData(CF_UNICODETEXT, object);
	CloseClipboard();
}

void og_platform_window_get_cursor_pos(OgNativeWindow* window, double* outXPos, double* outYPos)
{
	POINT pos;

	if (GetCursorPos(&pos))
	{
		ScreenToClient(window->win32.handle, &pos);

		if (outXPos)
			*outXPos = pos.x;

		if (outYPos)
			*outYPos = pos.y;
	}
}

void og_platform_window_set_cursor_pos(OgNativeWindow* window, double xpos, double ypos)
{
	POINT pos{ (int)xpos, (int)ypos };

	// Store the new position so it can be recognized later
	window->win32.lastCursorPosX = pos.x;
	window->win32.lastCursorPosY = pos.y;

	ClientToScreen(window->win32.handle, &pos);
	SetCursorPos(pos.x, pos.y);
}

void og_platform_window_set_cursor_mode(OgNativeWindow* window, int mode)
{
	if (mode == OG_CURSOR_DISABLED)
	{
		if (og_platform_window_get_focused(window))
		{
			disable_cursor(window);
		}
	}
	else if (window->context->win32.disabledCursorWindow == window)
	{
		enable_cursor(window);
	}
	else if (is_cursor_in_content_area(window))
	{
		update_cursor_image(window);
	}
}

// ImGui Win32 핸들러 선언
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LRESULT CALLBACK window_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// ImGui 이벤트 처리
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;
	
	OgNativeWindow* window = (OgNativeWindow*)GetPropW(hWnd, L"Og");

	if (!window)
	{
		switch (uMsg)
		{
		case WM_DISPLAYCHANGE:
			break;
		case WM_DEVICECHANGE:
		{
			// Joystick 혹은 VR ?!	
			break;
		}
		}

		return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg)
	{
	case WM_DESTROY:
	case WM_CLOSE:
	{
		window->shouldClose = true;
		PostQuitMessage(0);
		return 0;
	}
	case WM_MOVE:
	{
		if (window->context->win32.disabledCursorWindow == window)
			update_cursor_clip_rect(window);

		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);

		window->x = x;
		window->y = y;

		OgNativeEvent evt;
		evt.type = OG_WINDOW_MOVE;
		evt.window.x = x;
		evt.window.y = y;

		og_window_event_push(window, &evt);

		return 0;
	}
	case WM_SIZING:
	{
		window->win32.resizing = true;

		if (window->numer == OG_DONT_CARE ||
			window->denom == OG_DONT_CARE)
		{
			break;
		}

		apply_aspect_ratio(window, (int)wParam, (RECT*)lParam);

		return TRUE;
	}
	break;
	case WM_CHAR:
	case WM_SYSCHAR:
	{
		if (wParam >= 0xd800 && wParam <= 0xdbff)
			window->win32.highSurrogate = (WCHAR)wParam;
		else
		{
			unsigned int codepoint = 0;

			if (wParam >= 0xdc00 && wParam <= 0xdfff)
			{
				if (window->win32.highSurrogate)
				{
					codepoint += (window->win32.highSurrogate - 0xd800) << 10;
					codepoint += (WCHAR)wParam - 0xdc00;
					codepoint += 0x10000;
				}
			}
			else
				codepoint = (WCHAR)wParam;

			window->win32.highSurrogate = 0;
			const int action = (HIWORD(lParam) & KF_UP) ? OG_RELEASE : OG_PRESS;
			og_platform_common_input_char(window, codepoint, get_key_mods(), uMsg != WM_SYSCHAR);
		}

		if (uMsg == WM_SYSCHAR && window->win32.keymenu)
			break;

		return 0;
	}
	case WM_UNICHAR:
	{
		if (uMsg == WM_UNICHAR && wParam == UNICODE_NOCHAR)
		{
			// WM_UNICHAR is not sent by Windows, but is sent by some
			// third-party input method engine
			// Returning TRUE here announces support for this message
			return TRUE;
		}

		og_platform_common_input_char(window, (unsigned int)wParam, get_key_mods(), true);

		return 0;
	}
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	{
		int key, scancode;
		const int action = (HIWORD(lParam) & KF_UP) ? OG_RELEASE : OG_PRESS;
		const int mods = get_key_mods();

		scancode = (HIWORD(lParam) & (KF_EXTENDED | 0xff));
		if (!scancode)
		{
			// NOTE: Some synthetic key messages have a scancode of zero
			// HACK: Map the virtual key back to a usable scancode
			scancode = MapVirtualKeyW((UINT)wParam, MAPVK_VK_TO_VSC);
		}

		key = window->context->win32.keycodes[scancode];

		// The Ctrl keys require special handling
		if (wParam == VK_CONTROL)
		{
			if (HIWORD(lParam) & KF_EXTENDED)
			{
				// Right side keys have the extended key bit set
				key = OG_KEY_RIGHT_CONTROL;
			}
			else
			{
				// NOTE: Alt Gr sends Left Ctrl follwed by Right Alt
				// HACK: We only want one event for Alt Gr, so if we detect
				//		 this sequence we discard this Left Ctrl message now
				//		 and later report Right Alt normally
				MSG next;
				const DWORD time = GetMessageTime();

				if (PeekMessageW(&next, NULL, 0, 0, PM_NOREMOVE))
				{
					if (next.message == WM_KEYDOWN ||
						next.message == WM_SYSKEYDOWN ||
						next.message == WM_KEYUP ||
						next.message == WM_SYSKEYUP)
					{
						if (next.wParam == VK_MENU && (HIWORD(next.lParam) & KF_EXTENDED) && next.time == time)
						{
							// Next message is Right Alt down so discard this
							break;
						}
					}
				}

				// This is a regular Left Ctrl message
				key = OG_KEY_LEFT_CONTROL;
			}
		}
		else if (wParam == VK_PROCESSKEY)
		{
			// IME notifies that keys have been filtered by setting the
			// virtual key-code to VK_PROCESSKEY
			break;
		}

		if (action == OG_RELEASE && wParam == VK_SHIFT)
		{
			// HACK: Release both Shift keys on Shift up event, as when both
			//       are pressed the first release does not emit any event
			// NOTE: The other half of this is in _glfwPlatformPollEvents
			og_platform_common_input_key(window, OG_KEY_LEFT_SHIFT, scancode, action, mods);
			og_platform_common_input_key(window, OG_KEY_RIGHT_SHIFT, scancode, action, mods);
		}
		else if (wParam == VK_SNAPSHOT)
		{
			// HACK: Key down is not reported for the Print Screen key
			og_platform_common_input_key(window, key, scancode, OG_PRESS, mods);
			og_platform_common_input_key(window, key, scancode, OG_RELEASE, mods);
		}
		else
		{
			og_platform_common_input_key(window, key, scancode, action, mods);
		}

		break;
	}

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_XBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_XBUTTONUP:
	{
		int i, button, action;

		if (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP)
			button = OG_MOUSE_BUTTON_LEFT;
		else if (uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP)
			button = OG_MOUSE_BUTTON_RIGHT;
		else if (uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONUP)
			button = OG_MOUSE_BUTTON_MIDDLE;
		else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
			button = OG_MOUSE_BUTTON_4;
		else
			button = OG_MOUSE_BUTTON_5;

		if (uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN ||
			uMsg == WM_MBUTTONDOWN || uMsg == WM_XBUTTONDOWN)
		{
			action = OG_PRESS;
		}
		else
			action = OG_RELEASE;

		for (i = 0; i <= OG_MOUSE_BUTTON_LAST; i++)
		{
			if (window->input.pointers[i] == OG_PRESS)
				break;
		}

		if (i > OG_MOUSE_BUTTON_LAST)
			SetCapture(hWnd);

		og_platform_common_mouse_click(window, button, action, get_key_mods());

		for (i = 0; i <= OG_MOUSE_BUTTON_LAST; i++)
		{
			if (window->input.pointers[i] == OG_PRESS)
				break;
		}

		if (i > OG_MOUSE_BUTTON_LAST)
			ReleaseCapture();

		if (uMsg == WM_XBUTTONDOWN || uMsg == WM_XBUTTONUP)
			return TRUE;

		return 0;
	}
	case WM_MOUSEMOVE:
	{
		const int x = GET_X_LPARAM(lParam);
		const int y = GET_Y_LPARAM(lParam);

		if (!window->win32.cursorTracked)
		{
			TRACKMOUSEEVENT tme;
			ZeroMemory(&tme, sizeof(tme));
			tme.cbSize = sizeof(tme);
			tme.dwFlags = TME_LEAVE;
			tme.hwndTrack = window->win32.handle;
			TrackMouseEvent(&tme);

			window->win32.cursorTracked = true;
			//  _glfwInputCursorEnter(window, GLFW_TRUE);
		}


		if (window->cursorMode == OG_CURSOR_DISABLED)
		{
			const int dx = x - window->win32.lastCursorPosX;
			const int dy = y - window->win32.lastCursorPosY;

			if (window->context->win32.disabledCursorWindow != window)
			{
				break;
			}

			if (window->rawMouseMotion == true)
			{
				break;
			}

			og_platform_common_input_cursor_pos(window, window->input.virtualCursorPosX + dx, window->input.virtualCursorPosY + dy);
		}
		else
		{
			og_platform_common_input_cursor_pos(window, x, y);
		}

		window->win32.lastCursorPosX = x;
		window->win32.lastCursorPosY = y;

		return 0;
	}
	case WM_MOUSELEAVE:
	{
		window->win32.cursorTracked = false;
		// _glfwInputCursorEnter(window, GLFW_FALSE);
		return 0;
	}
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
	{
		window->input.wheel.y = (SHORT)HIWORD(wParam) / (double)WHEEL_DELTA;
		return 0;
	}

	case WM_DPICHANGED:
	{
		const float xscale = HIWORD(wParam) / (float)USER_DEFAULT_SCREEN_DPI;
		const float yscale = LOWORD(wParam) / (float)USER_DEFAULT_SCREEN_DPI;

		// Only apply the suggested size if the OS is new enough to have
		// sent a WM_GETDPISCALEDSIZE before this
		if (IsWindows10CreatorsUpdateOrGreaterWin32())
		{
			RECT* suggested = (RECT*)lParam;
			SetWindowPos(window->win32.handle, HWND_TOP,
				suggested->left,
				suggested->top,
				suggested->right - suggested->left,
				suggested->bottom - suggested->top,
				SWP_NOACTIVATE | SWP_NOZORDER);
		}

		return 0;
	}
	case WM_ENTERSIZEMOVE:
	{
		// if (window->win32.frameAction)
			// break;

		// HACK: Enable the cursor while the user is moving or
		//       resizing the window or using the window menu
		if (window->cursorMode == OG_CURSOR_DISABLED)
			enable_cursor(window);

		break;
	}
	case WM_EXITSIZEMOVE:
	{
		window->win32.resizing = false;

		// Window 창만 이동시킬 경우, 이 메세지가 오는데
		// 이전 width/height와 달라질게 없다면 event를 넣지 않는다.
		int curWidth, curHeight;
		og_platform_window_get_size(window, &curWidth, &curHeight);

		if (window->width != curWidth || window->height != curHeight)
		{
			window->width = curWidth;
			window->height = curHeight;

			OgNativeEvent evt;
			evt.type = OG_WINDOW_RESIZED;
			og_window_event_push(window, &evt);
		}

		// if (window->win32.frameAction)
			// break;

		// HACK: Disable the cursor once the user is done moving or
		//       resizing the window or using the menu
		if (window->cursorMode == OG_CURSOR_DISABLED)
			disable_cursor(window);

		break;
	}
	case WM_SIZE:
	{
		// WM_SIZE 개편 with GLFW + below link
		// https://msparkms.tistory.com/entry/%EC%9C%88%EB%8F%84%EC%9A%B0%EC%B0%BD-%EA%B4%80%EB%A0%A8-%EB%A9%94%EC%8B%9C%EC%A7%80-%EC%A0%95%EB%A6%AC

		const int width = LOWORD(lParam);
		const int height = HIWORD(lParam);
		const bool isMinimized = (wParam == SIZE_MINIMIZED);
		const bool isMaximized = wParam == SIZE_MAXIMIZED || (window->win32.maximized && wParam != SIZE_RESTORED);

		if (window->context->win32.disabledCursorWindow == window)
		{
			update_cursor_clip_rect(window);
		}

		OgNativeEvent evt;

		if (isMinimized)
		{
			evt.type = OG_WINDOW_MINIMIZE;
			og_window_event_push(window, &evt);
		}
		else if (isMaximized)
		{
			evt.type = OG_WINDOW_MAXMIZE;
			og_window_event_push(window, &evt);
		}
		else
		{
			if (window->width != width || window->height != height)
			{
				if (window->win32.resizing == false)
				{
					evt.type = OG_WINDOW_RESTORE;
					og_window_event_push(window, &evt);
				}
			}
		}


		if (window->monitor && window->minimized != isMinimized)
		{
			if (isMinimized)
			{
				release_monitor(window);
			}
			else
			{
				acquire_monitor(window);
				fit_to_monitor(window);
			}
		}

		if (window->win32.resizing == false)
		{
			window->width = width;
			window->height = height;
		}

		window->minimized = isMinimized;
		window->win32.maximized = isMaximized;

		return 0;
	}
	case WM_SETFOCUS:
	{
		og_platform_common_window_focus(window, true);

		// TODO
		// HACK: Do not disable cursor while the user is interacting with
		//       a caption button
		// if (window->win32.frameAction)
			// break;

		if (window->cursorMode == OG_CURSOR_DISABLED)
		{
			disable_cursor(window);
		}


		return 0;
	}

	case WM_KILLFOCUS:
	{
		if (window->cursorMode == OG_CURSOR_DISABLED)
		{
			enable_cursor(window);
		}

		// TODO
		// if (window->monitor && window->autoIconify)
			// _glfwPlatformIconifyWindow(window);

		og_platform_common_window_focus(window, false);

		return 0;
	}
	case WM_SETCURSOR:
	{
		if (LOWORD(lParam) == HTCLIENT)
		{
			update_cursor_image(window);
			return TRUE;
		}
		break;
	}
	case WM_DROPFILES:
	{
		static Og::System::OgVector<std::string> paths;

		HDROP drop = (HDROP)wParam;
		POINT pt;
		int i;

		const int count = DragQueryFileW(drop, 0xffffffff, NULL, 0);
		paths.Resize((size_t)count);

		// Move the mouse to the position of the drop
		DragQueryPoint(drop, &pt);
		og_platform_common_input_cursor_pos(window, pt.x, pt.y);

		for (i = 0; i < count; ++i)
		{
			const UINT length = DragQueryFileW(drop, i, NULL, 0) + 1; // +1는 \0이 들어갈 크기
			std::string* path = &paths[i];
			path->reserve(length);

			// temp wstring for converting into OgString
			static std::wstring buffer;
			buffer.reserve(length);

			DragQueryFileW(drop, i, (wchar_t*)buffer.c_str(), (UINT)buffer.capacity());

			og_path_system_to_utf8((char*)path->c_str(), (wchar_t*)buffer.c_str());
		}

		DragFinish(drop);

		OgNativeEvent evt;
		evt.type = OG_DROP_FILES;
		evt.drop.paths = (void*)&paths;
		evt.drop.count = count;
		og_window_event_push(window, &evt);

		return 0;
	}
	}

	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

bool og_platform_file_browser_open_filter(OgNativeWindow* window, const char* default_path, char* return_path, const char* filter)
{
	WCHAR szUniCode[OG_CHAR_INIT_LENGTH] = { 0, };
	og_path_utf8_to_system(szUniCode, default_path, true);

	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr)) return false;

	IFileOpenDialog* openDialog;
	hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&openDialog));
	if (FAILED(hr)) return false;

	FILEOPENDIALOGOPTIONS opts;
	openDialog->GetOptions(&opts);

	//options https://docs.microsoft.com/en-us/windows/win32/api/shobjidl_core/ne-shobjidl_core-_fileopendialogoptions
	opts |= FOS_FILEMUSTEXIST;
	openDialog->SetOptions(opts);
	openDialog->SetTitle(L"Open File");

	WCHAR szfilter[OG_CHAR_INIT_LENGTH] = { 0, };
	og_path_utf8_to_system(szfilter, filter, true);
	std::wstring ws = szfilter;
	
	// TODO: 아래 부분은 주석을 풀고 구현이 필요하다. 현재 OgString이 없어서 일단 주석처리하고 넘김.

	//Og::System::OgVector<std::wstring> wss = ws.Split(L";");
	//COMDLG_FILTERSPEC rgSpect[10];
	//for (size_t i = 0; i < wss.Size(); ++i)
	//	rgSpect[i] = { wss[i].c_str(), wss[i].c_str() };
	//rgSpect[wss.Size()] = { L"All Files", L"*.*" };

	//openDialog->SetFileTypes(2, rgSpect);
	//openDialog->SetFileTypeIndex(0);


	//hr = openDialog->Show(NULL);
	//if (FAILED(hr)) return false;

	//IShellItem* result = NULL;
	//hr = openDialog->GetResult(&result);
	//if (FAILED(hr)) return false;

	//PWSTR resultPath;
	//hr = result->GetDisplayName(SIGDN_FILESYSPATH, &resultPath);
	//if (FAILED(hr)) return false;

	//og_path_system_to_utf8(return_path, resultPath);

	//openDialog->Release();
	//result->Release();
	//CoTaskMemFree(resultPath);
	return true;
}

bool og_platform_file_browser_open(OgNativeWindow* window, const char* default_path, char* return_path)
{
	WCHAR szUniCode[OG_CHAR_INIT_LENGTH] = { 0, };
	og_path_utf8_to_system(szUniCode, default_path, true);

	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr)) return false;

	IFileOpenDialog* openDialog;
	hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&openDialog));
	if (FAILED(hr)) return false;

	FILEOPENDIALOGOPTIONS opts;
	openDialog->GetOptions(&opts);

	//options https://docs.microsoft.com/en-us/windows/win32/api/shobjidl_core/ne-shobjidl_core-_fileopendialogoptions
	opts |= FOS_FILEMUSTEXIST;
	openDialog->SetOptions(opts);
	openDialog->SetTitle(L"Open File");

	hr = openDialog->Show(NULL);
	if (FAILED(hr)) return false;

	IShellItem* result = NULL;
	hr = openDialog->GetResult(&result);
	if (FAILED(hr)) return false;

	PWSTR resultPath;
	hr = result->GetDisplayName(SIGDN_FILESYSPATH, &resultPath);
	if (FAILED(hr)) return false;

	og_path_system_to_utf8(return_path, resultPath);

	openDialog->Release();
	result->Release();
	CoTaskMemFree(resultPath);
	return true;
}

bool og_platform_file_create_browser_open(OgNativeWindow* window, const char* default_path, char* return_path, void* data, unsigned int dataSize, const char* extension)
{
	WCHAR szUniCode[OG_CHAR_INIT_LENGTH] = { 0, };
	og_path_utf8_to_system(szUniCode, default_path, true);

	TCHAR szextension[OG_CHAR_INIT_LENGTH] = { 0, };
	TCHAR szexplanation[OG_CHAR_INIT_LENGTH] = { L'*', L'.', 0, };
	og_path_utf8_to_system(szextension, extension, true);
	wcscat(szexplanation, szextension);
	COMDLG_FILTERSPEC rgSpec[] = { {szexplanation, szexplanation }, {L"(*.*) All Files", L"*.*"} };

	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr)) return false;

	IFileSaveDialog* saveDialog;
	hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void**>(&saveDialog));
	if (FAILED(hr)) return false;

	FILEOPENDIALOGOPTIONS opts;
	saveDialog->GetOptions(&opts);

	//options https://docs.microsoft.com/en-us/windows/win32/api/shobjidl_core/ne-shobjidl_core-_fileopendialogoptions
	opts |= FOS_CREATEPROMPT | FOS_OVERWRITEPROMPT; //| FOS_NOTESTFILECREATE; <-이 옵션을 쓰면 가상으로 파일을 안만들어본다.
	saveDialog->SetOptions(opts);
	saveDialog->SetTitle(L"Save File");
	saveDialog->SetFileTypes(1, rgSpec);
	saveDialog->SetFileTypeIndex(0);
	saveDialog->SetDefaultExtension(szextension);

	//Folder경로 지정 by.WooJin
	IShellItem* psiFolder;
	SHCreateItemFromParsingName(szUniCode, NULL, IID_PPV_ARGS(&psiFolder));
	saveDialog->SetFolder(psiFolder);

	hr = saveDialog->Show(NULL);
	if (FAILED(hr)) return false;

	IShellItem* result = NULL;
	hr = saveDialog->GetResult(&result);
	if (FAILED(hr)) return false;

	PWSTR resultPath;
	hr = result->GetDisplayName(SIGDN_FILESYSPATH, &resultPath);
	if (FAILED(hr)) return false;

	if (data != nullptr && dataSize > 0)
	{
		HANDLE h = CreateFile(resultPath,
			GENERIC_WRITE,
			0,
			0,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			0);

		if (h == INVALID_HANDLE_VALUE) return false;

		WriteFile(h,
			data,
			dataSize,
			NULL,
			NULL);

		if (h == INVALID_HANDLE_VALUE) return false;
		CloseHandle(h);
	}

	og_path_system_to_utf8(return_path, resultPath);

	saveDialog->Release();
	result->Release();
	CoTaskMemFree(resultPath);
	return true;
}

bool og_platform_folder_browser_open(OgNativeWindow* window, const char* default_path, char* return_path)
{
	WCHAR szUniCode[OG_CHAR_INIT_LENGTH] = { 0, };
	og_path_utf8_to_system(szUniCode, default_path, true);

	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr)) return false;

	IFileOpenDialog* openDialog;
	hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&openDialog));
	if (FAILED(hr)) return false;

	FILEOPENDIALOGOPTIONS opts;
	openDialog->GetOptions(&opts);

	//options https://docs.microsoft.com/en-us/windows/win32/api/shobjidl_core/ne-shobjidl_core-_fileopendialogoptions
	opts |= FOS_PICKFOLDERS;
	openDialog->SetOptions(opts);
	openDialog->SetTitle(L"Select the folder containing");

	hr = openDialog->Show(NULL);
	if (FAILED(hr)) return false;

	IShellItem* result = NULL;
	hr = openDialog->GetResult(&result);
	if (FAILED(hr)) return false;

	PWSTR resultPath;
	hr = result->GetDisplayName(SIGDN_FILESYSPATH, &resultPath);
	if (FAILED(hr)) return false;

	og_path_system_to_utf8(return_path, resultPath);

	openDialog->Release();
	result->Release();
	CoTaskMemFree(resultPath);
	return true;
}

int og_platform_key_get_scancode(int key)
{
	OgSystemContext* sysCtx = og_system_get_context();
	return sysCtx->win32.scancodes[key];
}

// Create key code translation tables
//
static void create_key_tables(OgSystemContext* context)
{
	int scancode;

	memset(context->win32.keycodes, -1, sizeof(context->win32.keycodes));
	memset(context->win32.scancodes, -1, sizeof(context->win32.scancodes));

	context->win32.keycodes[0x00B] = OG_KEY_0;
	context->win32.keycodes[0x002] = OG_KEY_1;
	context->win32.keycodes[0x003] = OG_KEY_2;
	context->win32.keycodes[0x004] = OG_KEY_3;
	context->win32.keycodes[0x005] = OG_KEY_4;
	context->win32.keycodes[0x006] = OG_KEY_5;
	context->win32.keycodes[0x007] = OG_KEY_6;
	context->win32.keycodes[0x008] = OG_KEY_7;
	context->win32.keycodes[0x009] = OG_KEY_8;
	context->win32.keycodes[0x00A] = OG_KEY_9;
	context->win32.keycodes[0x01E] = OG_KEY_A;
	context->win32.keycodes[0x030] = OG_KEY_B;
	context->win32.keycodes[0x02E] = OG_KEY_C;
	context->win32.keycodes[0x020] = OG_KEY_D;
	context->win32.keycodes[0x012] = OG_KEY_E;
	context->win32.keycodes[0x021] = OG_KEY_F;
	context->win32.keycodes[0x022] = OG_KEY_G;
	context->win32.keycodes[0x023] = OG_KEY_H;
	context->win32.keycodes[0x017] = OG_KEY_I;
	context->win32.keycodes[0x024] = OG_KEY_J;
	context->win32.keycodes[0x025] = OG_KEY_K;
	context->win32.keycodes[0x026] = OG_KEY_L;
	context->win32.keycodes[0x032] = OG_KEY_M;
	context->win32.keycodes[0x031] = OG_KEY_N;
	context->win32.keycodes[0x018] = OG_KEY_O;
	context->win32.keycodes[0x019] = OG_KEY_P;
	context->win32.keycodes[0x010] = OG_KEY_Q;
	context->win32.keycodes[0x013] = OG_KEY_R;
	context->win32.keycodes[0x01F] = OG_KEY_S;
	context->win32.keycodes[0x014] = OG_KEY_T;
	context->win32.keycodes[0x016] = OG_KEY_U;
	context->win32.keycodes[0x02F] = OG_KEY_V;
	context->win32.keycodes[0x011] = OG_KEY_W;
	context->win32.keycodes[0x02D] = OG_KEY_X;
	context->win32.keycodes[0x015] = OG_KEY_Y;
	context->win32.keycodes[0x02C] = OG_KEY_Z;

	context->win32.keycodes[0x028] = OG_KEY_APOSTROPHE;
	context->win32.keycodes[0x02B] = OG_KEY_BACKSLASH;
	context->win32.keycodes[0x033] = OG_KEY_COMMA;
	context->win32.keycodes[0x00D] = OG_KEY_EQUAL;
	context->win32.keycodes[0x029] = OG_KEY_GRAVE_ACCENT;
	context->win32.keycodes[0x01A] = OG_KEY_LEFT_BRACKET;
	context->win32.keycodes[0x00C] = OG_KEY_MINUS;
	context->win32.keycodes[0x034] = OG_KEY_PERIOD;
	context->win32.keycodes[0x01B] = OG_KEY_RIGHT_BRACKET;
	context->win32.keycodes[0x027] = OG_KEY_SEMICOLON;
	context->win32.keycodes[0x035] = OG_KEY_SLASH;
	context->win32.keycodes[0x056] = OG_KEY_WORLD_2;

	context->win32.keycodes[0x00E] = OG_KEY_BACKSPACE;
	context->win32.keycodes[0x153] = OG_KEY_DELETE;
	context->win32.keycodes[0x14F] = OG_KEY_END;
	context->win32.keycodes[0x01C] = OG_KEY_ENTER;
	context->win32.keycodes[0x001] = OG_KEY_ESCAPE;
	context->win32.keycodes[0x147] = OG_KEY_HOME;
	context->win32.keycodes[0x152] = OG_KEY_INSERT;
	context->win32.keycodes[0x15D] = OG_KEY_MENU;
	context->win32.keycodes[0x151] = OG_KEY_PAGE_DOWN;
	context->win32.keycodes[0x149] = OG_KEY_PAGE_UP;
	context->win32.keycodes[0x045] = OG_KEY_PAUSE;
	context->win32.keycodes[0x146] = OG_KEY_PAUSE;
	context->win32.keycodes[0x039] = OG_KEY_SPACE;
	context->win32.keycodes[0x00F] = OG_KEY_TAB;
	context->win32.keycodes[0x03A] = OG_KEY_CAPS_LOCK;
	context->win32.keycodes[0x145] = OG_KEY_NUM_LOCK;
	context->win32.keycodes[0x046] = OG_KEY_SCROLL_LOCK;
	context->win32.keycodes[0x03B] = OG_KEY_F1;
	context->win32.keycodes[0x03C] = OG_KEY_F2;
	context->win32.keycodes[0x03D] = OG_KEY_F3;
	context->win32.keycodes[0x03E] = OG_KEY_F4;
	context->win32.keycodes[0x03F] = OG_KEY_F5;
	context->win32.keycodes[0x040] = OG_KEY_F6;
	context->win32.keycodes[0x041] = OG_KEY_F7;
	context->win32.keycodes[0x042] = OG_KEY_F8;
	context->win32.keycodes[0x043] = OG_KEY_F9;
	context->win32.keycodes[0x044] = OG_KEY_F10;
	context->win32.keycodes[0x057] = OG_KEY_F11;
	context->win32.keycodes[0x058] = OG_KEY_F12;
	context->win32.keycodes[0x064] = OG_KEY_F13;
	context->win32.keycodes[0x065] = OG_KEY_F14;
	context->win32.keycodes[0x066] = OG_KEY_F15;
	context->win32.keycodes[0x067] = OG_KEY_F16;
	context->win32.keycodes[0x068] = OG_KEY_F17;
	context->win32.keycodes[0x069] = OG_KEY_F18;
	context->win32.keycodes[0x06A] = OG_KEY_F19;
	context->win32.keycodes[0x06B] = OG_KEY_F20;
	context->win32.keycodes[0x06C] = OG_KEY_F21;
	context->win32.keycodes[0x06D] = OG_KEY_F22;
	context->win32.keycodes[0x06E] = OG_KEY_F23;
	context->win32.keycodes[0x076] = OG_KEY_F24;
	context->win32.keycodes[0x038] = OG_KEY_LEFT_ALT;
	context->win32.keycodes[0x01D] = OG_KEY_LEFT_CONTROL;
	context->win32.keycodes[0x02A] = OG_KEY_LEFT_SHIFT;
	context->win32.keycodes[0x15B] = OG_KEY_LEFT_SUPER;
	context->win32.keycodes[0x137] = OG_KEY_PRINT_SCREEN;
	context->win32.keycodes[0x138] = OG_KEY_RIGHT_ALT;
	context->win32.keycodes[0x11D] = OG_KEY_RIGHT_CONTROL;
	context->win32.keycodes[0x036] = OG_KEY_RIGHT_SHIFT;
	context->win32.keycodes[0x15C] = OG_KEY_RIGHT_SUPER;
	context->win32.keycodes[0x150] = OG_KEY_DOWN;
	context->win32.keycodes[0x14B] = OG_KEY_LEFT;
	context->win32.keycodes[0x14D] = OG_KEY_RIGHT;
	context->win32.keycodes[0x148] = OG_KEY_UP;

	context->win32.keycodes[0x052] = OG_KEY_KP_0;
	context->win32.keycodes[0x04F] = OG_KEY_KP_1;
	context->win32.keycodes[0x050] = OG_KEY_KP_2;
	context->win32.keycodes[0x051] = OG_KEY_KP_3;
	context->win32.keycodes[0x04B] = OG_KEY_KP_4;
	context->win32.keycodes[0x04C] = OG_KEY_KP_5;
	context->win32.keycodes[0x04D] = OG_KEY_KP_6;
	context->win32.keycodes[0x047] = OG_KEY_KP_7;
	context->win32.keycodes[0x048] = OG_KEY_KP_8;
	context->win32.keycodes[0x049] = OG_KEY_KP_9;
	context->win32.keycodes[0x04E] = OG_KEY_KP_ADD;
	context->win32.keycodes[0x053] = OG_KEY_KP_DECIMAL;
	context->win32.keycodes[0x135] = OG_KEY_KP_DIVIDE;
	context->win32.keycodes[0x11C] = OG_KEY_KP_ENTER;
	context->win32.keycodes[0x037] = OG_KEY_KP_MULTIPLY;
	context->win32.keycodes[0x04A] = OG_KEY_KP_SUBTRACT;

	for (scancode = 0; scancode < 512; ++scancode)
	{
		if (context->win32.keycodes[scancode] > 0)
		{
			context->win32.scancodes[context->win32.keycodes[scancode]] = scancode;
		}
	}
}

// Retrieves and translates modifier keys
//
static int get_key_mods(void)
{
	int mods = 0;

	if (GetKeyState(VK_SHIFT) & 0x8000)
		mods |= OG_MOD_SHIFT;
	if (GetKeyState(VK_CONTROL) & 0x8000)
		mods |= OG_MOD_CONTROL;
	if (GetKeyState(VK_MENU) & 0x8000)
		mods |= OG_MOD_ALT;
	if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000)
		mods |= OG_MOD_SUPER;
	if (GetKeyState(VK_CAPITAL) & 1)
		mods |= OG_MOD_CAPS_LOCK;
	if (GetKeyState(VK_NUMLOCK) & 1)
		mods |= OG_MOD_NUM_LOCK;

	return mods;
}


// Translates a standard cursor to a resource ID
//
static LPWSTR translate_cursor_shape(int shape)
{
	switch (shape)
	{
	case OG_ARROW_CURSOR:
		return IDC_ARROW;
	case OG_IBEAM_CURSOR:
		return IDC_IBEAM;
	case OG_CROSSHAIR_CURSOR:
		return IDC_CROSS;
	case OG_HAND_CURSOR:
		return IDC_HAND;
	case OG_HRESIZE_CURSOR:
		return IDC_SIZEWE;
	case OG_VRESIZE_CURSOR:
		return IDC_SIZENS;
	}

	return NULL;
}

// 커서가 윈도우 안에 있는지 없는지.
//
static bool cursor_In_client_area(OgNativeWindow* window)
{
	RECT area;
	POINT pos;

	if (!GetCursorPos(&pos))
		return false;

	if (WindowFromPoint(pos) != window->win32.handle)
		return false;

	GetClientRect(window->win32.handle, &area);
	ClientToScreen(window->win32.handle, (POINT*)&area.left);
	ClientToScreen(window->win32.handle, (POINT*)&area.right);

	return PtInRect(&area, pos);
}

// Returns a wide string version of the specified UTF-8 string
//
static WCHAR* create_wide_string_from_UTF8_win32(const char* source)
{
	WCHAR* target;
	int length;

	length = MultiByteToWideChar(CP_UTF8, 0, source, -1, NULL, 0);
	if (!length)
	{
		LOGE(OG_ID, "%s", "Win32 : Failed to convert string from UTF-8");
		return NULL;
	}

	target = (WCHAR*)calloc(length, sizeof(WCHAR));

	if (!MultiByteToWideChar(CP_UTF8, 0, source, -1, target, length))
	{
		LOGE(OG_ID, "%s", "Win32 : Failed to convert string from UTF-8");
		free(target);
		return NULL;
	}

	return target;
}

// Returns a UTF-8 string version of the specified wide string
//
static char* create_UTF8_from_wstring_win32(const WCHAR* source)
{
	char* target;
	int length;

	length = WideCharToMultiByte(CP_UTF8, 0, source, -1, NULL, 0, NULL, NULL);
	if (!length)
	{
		LOGE(OG_ID, "%s", "Win32 : Failed to convert string from UTF-8");
		return NULL;
	}

	target = (char*)og_calloc(length, 1);

	if (!WideCharToMultiByte(CP_UTF8, 0, source, -1, target, length, NULL, NULL))
	{
		LOGE(OG_ID, "%s", "Win32 : Failed to convert string from UTF-8");
		og_free(target);
		return NULL;
	}

	return target;
}

void destroy_UTF8_from_wstring_win32(char* str)
{
	OG_CHECK(str != nullptr, "str is nullptr");
	og_free(str);
}

// GLFW 참고
//
static DWORD get_window_style(const OgNativeWindow *window)
{
	DWORD style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	if (window->monitor)
		style |= WS_POPUP;
	else
	{
		style |= WS_SYSMENU | WS_MINIMIZEBOX;
		if (window->decorated)
		{
			style |= WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

			if (window->resizable)
				style |= WS_MAXIMIZEBOX | WS_THICKFRAME;
		}
		else
			style |= WS_POPUP;
	}

	return style;
}


// Returns the extended window style for the specified window
// https://msdn.microsoft.com/ko-kr/library/61fe4bte.aspx
static DWORD get_windowEx_style(const OgNativeWindow* window)
{
	DWORD style = WS_EX_APPWINDOW;

	if (window->monitor || window->floating)
		style |= WS_EX_TOPMOST;

	return style;
}

static void get_full_window_size(DWORD style, DWORD exStyle,
	int clientWidth, int clientHeight,
	int* fullWidth, int* fullHeight)
{
	RECT rect = { 0, 0, clientWidth, clientHeight };
	AdjustWindowRectEx(&rect, style, false, exStyle);
	*fullWidth = rect.right - rect.left;
	*fullHeight = rect.bottom - rect.top;
}


// Enforce the client rect aspect ratio based on which edge is being dragged
//
static void apply_aspect_ratio(OgNativeWindow* window, int edge, RECT* area)
{
	int xoff, yoff;
	UINT dpi = USER_DEFAULT_SCREEN_DPI;
	const float ratio = (float)window->numer / (float)window->denom;

	if (IsWindows10AnniversaryUpdateOrGreaterWin32())
		dpi = GetDpiForWindow(window->win32.handle);

	get_full_window_size(get_window_style(window), get_windowEx_style(window),
		0, 0, &xoff, &yoff);

	if (edge == WMSZ_LEFT || edge == WMSZ_BOTTOMLEFT ||
		edge == WMSZ_RIGHT || edge == WMSZ_BOTTOMRIGHT)
	{
		area->bottom = area->top + yoff +
			(int)((area->right - area->left - xoff) / ratio);
	}
	else if (edge == WMSZ_TOPLEFT || edge == WMSZ_TOPRIGHT)
	{
		area->top = area->bottom - yoff -
			(int)((area->right - area->left - xoff) / ratio);
	}
	else if (edge == WMSZ_TOP || edge == WMSZ_BOTTOM)
	{
		area->right = area->left + xoff +
			(int)((area->bottom - area->top - yoff) * ratio);
	}
}

// Update native window styles to match attributes
//`
static void update_window_styles(const OgNativeWindow *window)
{
	RECT rect;
	DWORD style = GetWindowLongW(window->win32.handle, GWL_STYLE);
	style &= ~(WS_OVERLAPPEDWINDOW | WS_POPUP);
	style |= get_window_style(window);

	GetClientRect(window->win32.handle, &rect);
	AdjustWindowRectEx(&rect, style, FALSE, get_windowEx_style(window));
	ClientToScreen(window->win32.handle, (POINT*)&rect.left);
	ClientToScreen(window->win32.handle, (POINT*)&rect.right);
	SetWindowLongW(window->win32.handle, GWL_STYLE, style);
	SetWindowPos(window->win32.handle, HWND_TOP,
		rect.left, rect.top,
		rect.right - rect.left, rect.bottom - rect.top,
		SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOZORDER);
}

static OgMonitor* create_monitor(DISPLAY_DEVICEW* adapter, DISPLAY_DEVICEW* display)
{
	char* name;

	if (display)
		name = create_UTF8_from_wstring_win32(display->DeviceString);
	else
		name = create_UTF8_from_wstring_win32(adapter->DeviceString);

	if (!name)
		return NULL;


	DEVMODEW dm;
	HDC dc;
	ZeroMemory(&dm, sizeof(dm));
	dm.dmSize = sizeof(dm);
	EnumDisplaySettingsW(adapter->DeviceName, ENUM_CURRENT_SETTINGS, &dm);

	dc = CreateDCW(L"DISPLAY", adapter->DeviceName, NULL, NULL);

	int physicalWidth;
	int physicalHeight;
	if (IsWindows8Point1OrGreater())
	{
		physicalWidth = GetDeviceCaps(dc, HORZSIZE);
		physicalHeight = GetDeviceCaps(dc, VERTSIZE);
	}
	else
	{
		physicalWidth = (int)(dm.dmPelsWidth * 25.f / GetDeviceCaps(dc, LOGPIXELSX));
		physicalHeight = (int)(dm.dmPelsHeight * 25.f / GetDeviceCaps(dc, LOGPIXELSY));
	}

	DeleteDC(dc);

	OgMonitor* monitor = og_platform_common_alloc_monitor(name, physicalWidth, physicalHeight);

	free(name);

	if (adapter->StateFlags & DISPLAY_DEVICE_MODESPRUNED)
		monitor->win32.modePruned = true;

	wcscpy(monitor->win32.adapterName, adapter->DeviceName);
	WideCharToMultiByte(CP_UTF8, 0, adapter->DeviceName, -1, monitor->win32.publicAdapterName, sizeof(monitor->win32.publicAdapterName), NULL, NULL);

	if (display)
	{
		wcscpy(monitor->win32.displayName, display->DeviceName);
		WideCharToMultiByte(CP_UTF8, 0, display->DeviceName, -1, monitor->win32.publicDisplayName, sizeof(monitor->win32.publicDisplayName), NULL, NULL);
	}

	RECT rect;
	rect.left = dm.dmPosition.x;
	rect.top = dm.dmPosition.y;
	rect.right = dm.dmPosition.x + dm.dmPelsWidth;
	rect.bottom = dm.dmPosition.y + dm.dmPelsHeight;

	EnumDisplayMonitors(NULL, &rect, monitor_enum_proc, (LPARAM)monitor);

	return monitor;
}

void fit_to_monitor(OgNativeWindow* window)
{
	MONITORINFO mi = { sizeof(mi) };
	GetMonitorInfo(window->monitor->win32.handle, &mi);
	SetWindowPos(window->win32.handle, HWND_TOPMOST,
		mi.rcMonitor.left,
		mi.rcMonitor.top,
		mi.rcMonitor.right - mi.rcMonitor.left,
		mi.rcMonitor.bottom - mi.rcMonitor.top,
		SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
}

void acquire_monitor(OgNativeWindow* window)
{
	if (!window->context->win32.acquiredMonitorCount)
	{
		SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED);

		// HACK: When mouse trails are enabled the cursor becomes invisible when
		//       the OpenGL ICD switches to page flipping
		SystemParametersInfo(SPI_GETMOUSETRAILS, 0, &(window->context->win32.mouseTrailSize), 0);
		SystemParametersInfo(SPI_SETMOUSETRAILS, 0, 0, 0);
	}

	if (!window->monitor->window)
		++(window->context->win32.acquiredMonitorCount);

	og_platform_monitor_set_video_mode(window->monitor, &window->videoMode);

	og_platform_common_input_monitor_window(window->monitor, window);
}

void release_monitor(OgNativeWindow* window)
{
	if (window->monitor->window != window)
		return;

	--(window->context->win32.acquiredMonitorCount);
	if (!window->context->win32.acquiredMonitorCount)
	{
		SetThreadExecutionState(ES_CONTINUOUS);

		// HACK: Restore mouse trail length saved in acquireMonitor
		SystemParametersInfo(SPI_SETMOUSETRAILS, window->context->win32.mouseTrailSize, 0, 0);
	}

	og_platform_common_input_monitor_window(window->monitor, NULL);
	og_platform_monitor_restore_video_mode(window->monitor);
}

// Updates the cursor clip rect
//
void update_cursor_clip_rect(OgNativeWindow* window)
{
	if (window)
	{
		RECT clipRect;
		GetClientRect(window->win32.handle, &clipRect);
		ClientToScreen(window->win32.handle, (POINT*)&clipRect.left);
		ClientToScreen(window->win32.handle, (POINT*)&clipRect.right);
		ClipCursor(&clipRect);
	}
	else
	{
		ClipCursor(NULL);
	}
}

// Enables WM_INPUT messages for the mouse for the specified window
//
void enable_raw_mouse_motion(OgNativeWindow* window)
{
	const RAWINPUTDEVICE rid = { 0x01, 0x02, 0, window->win32.handle };

	if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
	{
		LOGE(OG_ID, "Win32: Failed to register raw input device");
	}
}

// Disables WM_INPUT messages for the mouse
//
void disable_raw_mouse_motion(OgNativeWindow* window)
{
	const RAWINPUTDEVICE rid = { 0x01, 0x02, RIDEV_REMOVE, NULL };

	if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
	{
		LOGE(OG_ID, "Win32: Failed to remove raw input device");
	}
}

void enable_cursor(OgNativeWindow* window)
{
	if (window->rawMouseMotion)
		disable_raw_mouse_motion(window);

	window->context->win32.disabledCursorWindow = NULL;
	update_cursor_clip_rect(NULL);
	og_platform_window_set_cursor_pos(window, window->context->win32.restoreCursorPosX, window->context->win32.restoreCursorPosY);
	update_cursor_image(window);
}

void disable_cursor(OgNativeWindow* window)
{
	window->context->win32.disabledCursorWindow = window;
	og_platform_window_get_cursor_pos(window, &window->context->win32.restoreCursorPosX, &window->context->win32.restoreCursorPosY);

	update_cursor_image(window);

	og_platform_common_center_cursor_in_content_area(window);

	update_cursor_clip_rect(window);

	if (window->rawMouseMotion)
		enable_raw_mouse_motion(window);
}

void update_cursor_image(OgNativeWindow* window)
{
	if (window->cursorMode == OG_CURSOR_NORMAL)
	{
		if (window->cursor)
		{
			SetCursor(window->cursor->win32.handle);
		}
		else
		{
			SetCursor(LoadCursorW(NULL, IDC_ARROW));
		}
	}
	else
	{
		SetCursor(NULL);
	}
}

bool is_cursor_in_content_area(OgNativeWindow* window)
{
	RECT area;
	POINT pos;

	if (!GetCursorPos(&pos))
		return false;

	if (WindowFromPoint(pos) != window->win32.handle)
		return false;

	GetClientRect(window->win32.handle, &area);
	ClientToScreen(window->win32.handle, (POINT*)&area.left);
	ClientToScreen(window->win32.handle, (POINT*)&area.right);

	return PtInRect(&area, pos);
}


#define RtlVerifyVersionInfo og_system_get_context()->win32.ntdll.pFNRtlVerifyVersionInfo

// Replacement for IsWindowsVersionOrGreater as MinGW lacks versionhelpers.h
BOOL og_windows_is_version_or_greater_win32(WORD major, WORD minor, WORD sp)
{
	OSVERSIONINFOEXW osvi = { sizeof(osvi), major, minor, 0, 0, {0}, sp };
	DWORD mask = VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR;
	ULONGLONG cond = VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL);
	cond = VerSetConditionMask(cond, VER_MINORVERSION, VER_GREATER_EQUAL);
	cond = VerSetConditionMask(cond, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
	// HACK: Use RtlVerifyVersionInfo instead of VerifyVersionInfoW as the
	//       latter lies unless the user knew to embed a non-default manifest
	//       announcing support for Windows 10 via supportedOS GUID
	return RtlVerifyVersionInfo(&osvi, mask, cond) == 0;
}

// Checks whether we are on at least the specified build of Windows 10
BOOL og_windows10_is_build_greater_win32(WORD build)
{
	OSVERSIONINFOEXW osvi = { sizeof(osvi), 10, 0, build };
	DWORD mask = VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER;
	ULONGLONG cond = VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL);
	cond = VerSetConditionMask(cond, VER_MINORVERSION, VER_GREATER_EQUAL);
	cond = VerSetConditionMask(cond, VER_BUILDNUMBER, VER_GREATER_EQUAL);
	// HACK: Use RtlVerifyVersionInfo instead of VerifyVersionInfoW as the
	//       latter lies unless the user knew to embed a non-default manifest
	//       announcing support for Windows 10 via supportedOS GUID
	return RtlVerifyVersionInfo(&osvi, mask, cond) == 0;
}
