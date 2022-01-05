#pragma once
#ifndef _OG_WINDOW_WIN32_H_
#define _OG_WINDOW_WIN32_H_

#ifdef __cplusplus
extern "C" {
#endif

	///////////////////////////// SystemContext Impl

	int og_platform_system_init(OgSystemContext* context, void* platformHandle);

	void og_platform_system_terminate(OgSystemContext* context);

	void og_platform_poll_events();

	bool og_platform_monitor_acquire(OgSystemContext* context);

	void* og_platform_file_relative_load_from_package(OgSystemContext* context, const char* path, bool binary, unsigned* outSize);

	void* og_platform_file_absolute_load_from_package(OgSystemContext* context, const char* path, bool binary, unsigned* outSize);

	bool og_platform_file_exist_from_package(OgSystemContext* context, const char* path);

	void og_platform_package_folder_get_with_seperator(OgSystemContext* context, char* outBuf, unsigned outBufSize);

	///////////////////////////// SystemContext Impl

	///////////////////////////// Cursor Impl

	bool og_platform_cursor_create(OgCursor* cursor, int shape);

	void og_platform_cursor_destroy(OgCursor* cursor);

	///////////////////////////// Cursor Impl

	///////////////////////////// Monitor Impl

	void og_platform_monitor_get_pos(OgMonitor* monitor, int* outXPos, int* outYPos);

	OgVideoMode* og_platform_monitor_get_video_modes(OgMonitor* monitor, int* outModeCount);

	void og_platform_monitor_get_video_mode(OgMonitor* monitor, OgVideoMode* outVideoMode);

	void og_platform_monitor_get_content_scale(OgMonitor* monitor, float* xscale, float* yscale);

	void og_platform_monitor_set_video_mode(OgMonitor* monitor, const OgVideoMode* desired);

	void og_platform_monitor_restore_video_mode(OgMonitor* monitor);

	void og_platform_monitor_free(OgMonitor* monitor);

	///////////////////////////// Monitor Impl

	///////////////////////////// Window Create/Destroy

	int og_platform_window_create(OgNativeWindow* window, const OgWindowConfig* config, const OgFrameBufferConfig* fbConfig);

	void og_platform_window_destroy(OgNativeWindow* window);

	///////////////////////////// Window Create/Destroy

	///////////////////////////// Window/Framebuffer Size

	void og_platform_window_get_size(OgNativeWindow* window, int* width, int* height);

	void og_platform_window_set_size(OgNativeWindow* window, int width, int height);

	void og_platform_window_get_framebuffer_size(OgNativeWindow* window, int* width, int* height);

	void og_platform_window_get_frame_size(OgNativeWindow* window, int* left, int* top, int* right, int* bottom);

	void og_platform_window_get_content_scale(OgNativeWindow* window, float* xscale, float* yscale);

	///////////////////////////// Window/Framebuffer/WindowFrame Size

	///////////////////////////// Window Pos

	void og_platform_window_get_pos(OgNativeWindow* window, int* outXPos, int* outYPos);

	void og_platform_window_set_pos(OgNativeWindow* window, int xpos, int ypos);

	///////////////////////////// Window Pos

	///////////////////////////// Window Action

	void og_platform_window_show(OgNativeWindow* window);

	void og_platform_window_focus_out(OgNativeWindow* window);

	void og_platform_window_focus_in(OgNativeWindow* window);

	bool og_platform_window_get_focused(OgNativeWindow* window);

	///////////////////////////// Window Action

	///////////////////////////// Window Title

	void og_platform_window_set_title(OgNativeWindow* window, const char* title);

	///////////////////////////// Window Title

	///////////////////////////// Window Monitor

	void og_platform_window_set_monitor(OgNativeWindow* window, OgMonitor* monitor, int xpos, int ypos, int width, int height, int refreshRate);

	///////////////////////////// Window Monitor

	///////////////////////////// Clipboard

	const char* og_platform_clipboard_get(OgNativeWindow* window);

	void og_platform_clipboard_set(OgNativeWindow* window, const char* str);

	///////////////////////////// Clipboard

	///////////////////////////// Window Cursor

	void og_platform_window_get_cursor_pos(OgNativeWindow* window, double* outXPos, double* outYPos);

	void og_platform_window_set_cursor_pos(OgNativeWindow* window, double xpos, double ypos);

	void og_platform_window_set_cursor_mode(OgNativeWindow* window, int mode);

	///////////////////////////// Window Cursor

	///////////////////////////// Window File/Folder Browser

	bool og_platform_file_browser_open(OgNativeWindow* window, const char* default_path, char* return_path);

	bool og_platform_file_browser_open_filter(OgNativeWindow* window, const char* default_path, char* return_path, const char* filter);

	bool og_platform_file_create_browser_open(OgNativeWindow* window, const char* default_path, char* return_path, void* data, unsigned int dataSize, const char* extension);

	bool og_platform_folder_browser_open(OgNativeWindow* window, const char* default_path, char* return_path);

	///////////////////////////// Window File/Folder Browser

	///////////////////////////// Util

	int og_platform_key_get_scancode(int key);

	///////////////////////////// Util

	// HACK: Define versionhelpers.h functions manually as MinGW lacks the header
#define IsWindowsVistaOrGreater()                                     \
og_windows_is_version_or_greater_win32(HIBYTE(_WIN32_WINNT_VISTA),   \
                                    LOBYTE(_WIN32_WINNT_VISTA), 0)
#define IsWindows7OrGreater()                                         \
og_windows_is_version_or_greater_win32(HIBYTE(_WIN32_WINNT_WIN7),    \
                                    LOBYTE(_WIN32_WINNT_WIN7), 0)
#define IsWindows8OrGreater()                                         \
og_windows_is_version_or_greater_win32(HIBYTE(_WIN32_WINNT_WIN8),    \
                                    LOBYTE(_WIN32_WINNT_WIN8), 0)
#define IsWindows8Point1OrGreater()                                   \
og_windows_is_version_or_greater_win32(HIBYTE(_WIN32_WINNT_WINBLUE), \
                                    LOBYTE(_WIN32_WINNT_WINBLUE), 0)

#define IsWindows10AnniversaryUpdateOrGreaterWin32() \
og_windows10_is_build_greater_win32(14393)
#define IsWindows10CreatorsUpdateOrGreaterWin32() \
og_windows10_is_build_greater_win32(15063)

	BOOL og_windows_is_version_or_greater_win32(WORD major, WORD minor, WORD sp);

	BOOL og_windows10_is_build_greater_win32(WORD build);

	///////////////////////////// Util

#ifdef __cplusplus
}
#endif
#endif // _OG_WINDOW_WIN32_H_
