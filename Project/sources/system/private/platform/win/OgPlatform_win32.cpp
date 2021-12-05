#include "system/OgSystemContext.h"
#include "system/OgNativeEvent.h"


#include <windowsx.h>
#include <shellapi.h>
#include <tchar.h>
#include <direct.h>
#include <windows.h>
#include <Shobjidl.h>
#include <shobjidl_core.h>
#include <imm.h>


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
		LvVideoMode mode;

		lv_monitor_get_pos(window->monitor, &x, &y);
		lv_platform_monitor_get_video_mode(window->monitor, &mode);

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

	ZeroMemory(&(window->win32), sizeof(LvWindowWIN));

	window->win32.instance = GetModuleHandleW(NULL);
	window->win32.handle = CreateWindowExW(exStyle,
		_LV_WNDCLASSNAME,
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
		LOGE(LV_ID, "Win32 : Failed to create window");
		return false;
	}

	free(wTitle);

	window->win32.display = GetDC(window->win32.handle);

	SetPropW(window->win32.handle, L"Lv", window);

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
			lv_platform_window_get_content_scale(window, &xscale, &yscale);
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

void og_window_event_push(OgNativeWindow* window, OgNativeEvent* evt)
{
	OgEventWindow* win = reinterpret_cast<OgEventWindow*>(window);
	if (evt != nullptr)
	{
		win->queue.Enqueue(*evt);
	}
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

static LRESULT CALLBACK window_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	OgNativeWindow* window = (OgNativeWindow*)GetPropW(hWnd, L"Lv");

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

		lv_window_event_push(window, &evt);

		return 0;
	}
	case WM_SIZING:
	{
		window->win32.resizing = true;

		if (window->numer == LV_DONT_CARE ||
			window->denom == LV_DONT_CARE)
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
			const int action = (HIWORD(lParam) & KF_UP) ? LV_RELEASE : LV_PRESS;
			lv_platform_common_input_char(window, codepoint, get_key_mods(), uMsg != WM_SYSCHAR);
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

		lv_platform_common_input_char(window, (unsigned int)wParam, get_key_mods(), true);

		return 0;
	}
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	{
		int key, scancode;
		const int action = (HIWORD(lParam) & KF_UP) ? LV_RELEASE : LV_PRESS;
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
				key = LV_KEY_RIGHT_CONTROL;
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
				key = LV_KEY_LEFT_CONTROL;
			}
		}
		else if (wParam == VK_PROCESSKEY)
		{
			// IME notifies that keys have been filtered by setting the
			// virtual key-code to VK_PROCESSKEY
			break;
		}

		if (action == LV_RELEASE && wParam == VK_SHIFT)
		{
			// HACK: Release both Shift keys on Shift up event, as when both
			//       are pressed the first release does not emit any event
			// NOTE: The other half of this is in _glfwPlatformPollEvents
			lv_platform_common_input_key(window, LV_KEY_LEFT_SHIFT, scancode, action, mods);
			lv_platform_common_input_key(window, LV_KEY_RIGHT_SHIFT, scancode, action, mods);
		}
		else if (wParam == VK_SNAPSHOT)
		{
			// HACK: Key down is not reported for the Print Screen key
			lv_platform_common_input_key(window, key, scancode, LV_PRESS, mods);
			lv_platform_common_input_key(window, key, scancode, LV_RELEASE, mods);
		}
		else
		{
			lv_platform_common_input_key(window, key, scancode, action, mods);
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
			button = LV_MOUSE_BUTTON_LEFT;
		else if (uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP)
			button = LV_MOUSE_BUTTON_RIGHT;
		else if (uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONUP)
			button = LV_MOUSE_BUTTON_MIDDLE;
		else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
			button = LV_MOUSE_BUTTON_4;
		else
			button = LV_MOUSE_BUTTON_5;

		if (uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN ||
			uMsg == WM_MBUTTONDOWN || uMsg == WM_XBUTTONDOWN)
		{
			action = LV_PRESS;
		}
		else
			action = LV_RELEASE;

		for (i = 0; i <= LV_MOUSE_BUTTON_LAST; i++)
		{
			if (window->input.pointers[i] == LV_PRESS)
				break;
		}

		if (i > LV_MOUSE_BUTTON_LAST)
			SetCapture(hWnd);

		lv_platform_common_mouse_click(window, button, action, get_key_mods());

		for (i = 0; i <= LV_MOUSE_BUTTON_LAST; i++)
		{
			if (window->input.pointers[i] == LV_PRESS)
				break;
		}

		if (i > LV_MOUSE_BUTTON_LAST)
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


		if (window->cursorMode == LV_CURSOR_DISABLED)
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

			lv_platform_common_input_cursor_pos(window, window->input.virtualCursorPosX + dx, window->input.virtualCursorPosY + dy);
		}
		else
		{
			lv_platform_common_input_cursor_pos(window, x, y);
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
		if (window->cursorMode == LV_CURSOR_DISABLED)
			enable_cursor(window);

		break;
	}
	case WM_EXITSIZEMOVE:
	{
		window->win32.resizing = false;

		// Window 창만 이동시킬 경우, 이 메세지가 오는데
		// 이전 width/height와 달라질게 없다면 event를 넣지 않는다.
		int curWidth, curHeight;
		lv_platform_window_get_size(window, &curWidth, &curHeight);

		if (window->width != curWidth || window->height != curHeight)
		{
			window->width = curWidth;
			window->height = curHeight;

			LvNativeEvent evt;
			evt.type = LV_WINDOW_RESIZED;
			lv_window_event_push(window, &evt);
		}

		// if (window->win32.frameAction)
			// break;

		// HACK: Disable the cursor once the user is done moving or
		//       resizing the window or using the menu
		if (window->cursorMode == LV_CURSOR_DISABLED)
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

		LvNativeEvent evt;

		if (isMinimized)
		{
			evt.type = LV_WINDOW_MINIMIZE;
			lv_window_event_push(window, &evt);
		}
		else if (isMaximized)
		{
			evt.type = LV_WINDOW_MAXMIZE;
			lv_window_event_push(window, &evt);
		}
		else
		{
			if (window->width != width || window->height != height)
			{
				if (window->win32.resizing == false)
				{
					evt.type = LV_WINDOW_RESTORE;
					lv_window_event_push(window, &evt);
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
		lv_platform_common_window_focus(window, true);

		// TODO
		// HACK: Do not disable cursor while the user is interacting with
		//       a caption button
		// if (window->win32.frameAction)
			// break;

		if (window->cursorMode == LV_CURSOR_DISABLED)
		{
			disable_cursor(window);
		}


		return 0;
	}

	case WM_KILLFOCUS:
	{
		if (window->cursorMode == LV_CURSOR_DISABLED)
		{
			enable_cursor(window);
		}

		// TODO
		// if (window->monitor && window->autoIconify)
			// _glfwPlatformIconifyWindow(window);

		lv_platform_common_window_focus(window, false);

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
		static Lv::LvList<Lv::LvString> paths;

		HDROP drop = (HDROP)wParam;
		POINT pt;
		int i;

		const int count = DragQueryFileW(drop, 0xffffffff, NULL, 0);
		paths.Resize((size_t)count);

		// Move the mouse to the position of the drop
		DragQueryPoint(drop, &pt);
		lv_platform_common_input_cursor_pos(window, pt.x, pt.y);

		for (i = 0; i < count; ++i)
		{
			const UINT length = DragQueryFileW(drop, i, NULL, 0) + 1; // +1는 \0이 들어갈 크기
			Lv::LvString* path = &paths[i];
			path->Reserve(length);

			// temp wstring for converting into LvString
			static Lv::LvWString buffer;
			buffer.Reserve(length);

			DragQueryFileW(drop, i, (wchar_t*)buffer.c_str(), (UINT)buffer.Capacity());

			lv_path_system_to_utf8((char*)path->c_str(), (wchar_t*)buffer.c_str());
		}

		DragFinish(drop);

		LvNativeEvent evt;
		evt.type = LV_DROP_FILES;
		evt.drop.paths = (void*)&paths;
		evt.drop.count = count;
		lv_window_event_push(window, &evt);

		return 0;
	}
	}

	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}


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
	wc.lpszClassName = _LV_WNDCLASSNAME;
	wc.lpszMenuName = NULL;

	// Load user-provided icon if available
	wc.hIcon = (HICON)LoadImageW(GetModuleHandleW(NULL),
		L"LV_ICON", IMAGE_ICON,
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
		LOGE(LV_ID, "Win32: Failed to register window class");
		return false;
	}

	create_key_tables(context);

	context->platform = LvPlatformType::WINDOWS;

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
	lv_platform_monitor_acquire(context);

	context->win32.display = GetDC(NULL);
	if (context->win32.display == NULL)
	{
		LOGE(LV_ID, "Fail to retrieve display for entire screen");
	}

	return true;
}


//OG_NAMESPACE_SYSTEM_END