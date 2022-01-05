#pragma once
#ifndef __OG_PLATFORM_DEFINITIONS_H__
#define __OG_PLATFORM_DEFINITIONS_H__
#include <Windows.h>
#include "system/OgInput.h"
#ifdef __cplusplus
extern "C" {
#endif

#define _OG_WNDCLASSNAME L"Og"

#define STYLE_BASIC         (WS_CLIPSIBLINGS | WS_CLIPCHILDREN)
#define STYLE_FULLSCREEN    (WS_POPUP)
#define STYLE_BORDERLESS    (WS_POPUP)
#define STYLE_BORDERLESS_WINDOWED (WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)
#define STYLE_NORMAL        (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)
#define STYLE_RESIZABLE     (WS_THICKFRAME | WS_MAXIMIZEBOX)
#define STYLE_MASK          (STYLE_FULLSCREEN | STYLE_BORDERLESS | STYLE_NORMAL | STYLE_RESIZABLE)


#define _OG_NATIVE_WINDOW 			  window->win32.handle
#define _OG_PLATFORM_CONTEXT 		  OgContextWIN win32;
#define _OG_PLATFORM_WINDOW 		  OgWindowWIN win32;
#define	_OG_PLATFORM_CURSOR_STATE	  OgCursorWIN win32;
#define _OG_PLATFORM_MONITOR_STATE    OgMonitorWIN win32;

	// ntdll.dll function pointer typedefs
	typedef LONG(WINAPI* PFNrtlVerifyVersionInfo)(OSVERSIONINFOEXW*, ULONG, ULONGLONG);

	// shcore.dll function pointer typedefs
#ifndef DPI_ENUMS_DECLARED
	typedef enum
	{
		PROCESS_DPI_UNAWARE = 0,
		PROCESS_SYSTEM_DPI_AWARE = 1,
		PROCESS_PER_MONITOR_DPI_AWARE = 2
	} PROCESS_DPI_AWARENESS;
	typedef enum
	{
		MDT_EFFECTIVE_DPI = 0,
		MDT_ANGULAR_DPI = 1,
		MDT_RAW_DPI = 2,
		MDT_DEFAULT = MDT_EFFECTIVE_DPI
	} MONITOR_DPI_TYPE;
#endif /*DPI_ENUMS_DECLARED*/
	typedef HRESULT(WINAPI* PFNSetProcessDpiAwareness)(PROCESS_DPI_AWARENESS);
	typedef HRESULT(WINAPI* PFNGetDpiForMonitor)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);

	typedef struct
	{
		int acquiredMonitorCount;
		const char* clipboardString;

		short int keycodes[512];
		short int scancodes[OG_KEY_LAST + 1];

		// https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getdc
		// if you call GetDC(NULL), you can get the DC for the entire screen.
		HDC display;

		// Where to place the cursor when re-enabled
		double restoreCursorPosX, restoreCursorPosY;
		// The window whose disabled cursor mode is active
		OgNativeWindow* disabledCursorWindow;

		UINT mouseTrailSize;

		struct
		{
			HINSTANCE instance;
			PFNrtlVerifyVersionInfo pFNRtlVerifyVersionInfo;
		} ntdll;

		struct
		{
			HINSTANCE instance;
			PFNSetProcessDpiAwareness      SetProcessDpiAwareness;
			PFNGetDpiForMonitor            GetDpiForMonitor;
		} shcore;
	} OgContextWIN;

	typedef struct
	{
		HWND handle;

		HINSTANCE instance;

		HDC	 display;

		bool resizing;

		bool cursorTracked;
		bool frameAction;
		bool maximized;
		bool transparent;
		bool scaleToMonitor;
		bool keymenu;

		int lastCursorPosX, lastCursorPosY;

		WCHAR highSurrogate;

	} OgWindowWIN;

	typedef struct
	{
		HCURSOR handle;

	} OgCursorWIN;

	typedef struct
	{
		HMONITOR handle;

		// This size matches the static size of DISPLAY_DEVICE.DeviceName
		WCHAR adapterName[32];
		WCHAR displayName[32];

		char publicAdapterName[32];
		char publicDisplayName[32];

		bool modePruned;
		bool modeChanged;
	} OgMonitorWIN;

#ifdef __cplusplus
}
#endif
#endif // _Og_WINDOW_WIN32_H_
