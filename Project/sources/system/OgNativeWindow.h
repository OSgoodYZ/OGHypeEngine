#ifndef _OG_NATIVE_WINDOW_H_
#define _OG_NATIVE_WINDOW_H_

#include "OgPrecompile.h"
#ifdef __cplusplus
extern "C" {
#endif

	typedef struct OgSystemContext OgSystemContext;
	typedef struct OgNativeWindow OgNativeWindow;
	typedef struct OgNativeEvent OgNativeEvent;
	typedef struct OgWindowConfig OgWindowConfig;
	typedef struct OgFrameBufferConfig OgFrameBufferConfig;
	typedef struct OgMonitor OgMonitor;

	///////////////////////////// Create/Destroy

	OG_API OgNativeWindow* og_window_create(OgSystemContext* context, const OgWindowConfig* wConfig, const OgFrameBufferConfig* fbConfig);

	OG_API void og_window_destroy(OgNativeWindow* window);

	///////////////////////////// Create/Destroy

	///////////////////////////// Event

	OG_API void og_window_event_push(OgNativeWindow* window, OgNativeEvent* evt);

	OG_API bool og_window_event_poll(OgNativeWindow* window, OgNativeEvent* evt);

	///////////////////////////// Event

	///////////////////////////// Window/Framebuffer Size

	OG_API void og_window_get_size(OgNativeWindow* window, int* width, int* height);

	OG_API void og_window_set_size(OgNativeWindow* window, int width, int height);

	OG_API void og_window_get_framebuffer_size(OgNativeWindow* window, int* width, int* height);

	OG_API void og_window_get_frame_size(OgNativeWindow* window, int* left, int* top, int* right, int* bottom);

	OG_API void og_window_get_content_scale(OgNativeWindow* window, float* xscale, float* yscale);

	///////////////////////////// Window/Framebuffer Size

	///////////////////////////// Window Pos

	OG_API void og_window_get_pos(OgNativeWindow* window, int* outXPos, int* outYPos);

	OG_API void og_window_set_pos(OgNativeWindow* window, int xpos, int ypos);

	///////////////////////////// Window Pos

	///////////////////////////// Window Action

	OG_API void og_window_show(OgNativeWindow* window);

	OG_API void og_window_focus_in(OgNativeWindow* window);

	OG_API void og_window_focus_out(OgNativeWindow* window);

	OG_API bool og_window_get_focused(OgNativeWindow* window);

	///////////////////////////// Window Action

	///////////////////////////// Window Title

	OG_API void og_window_set_title(OgNativeWindow* window, const char* title);

	///////////////////////////// Window Title

	///////////////////////////// Window Monitor

	OG_API void og_window_set_monitor(OgNativeWindow* window, OgMonitor* monitor, int xpos, int ypos, int width, int height, int refreshRate);

	///////////////////////////// Window Monitor

	///////////////////////////// Clipboard

	OG_API const char* og_window_clipboard_get(OgNativeWindow* window);

	OG_API void og_window_clipboard_set(OgNativeWindow* window, const char* str);

	///////////////////////////// Clipboard

	///////////////////////////// Cursor

	OG_API void og_window_get_cursor_pos(OgNativeWindow* window, double* outXPos, double* outYPos);
	
	OG_API void og_window_set_cursor_pos(OgNativeWindow* window, double xpos, double ypos);

	OG_API void og_window_set_cursor_mode(OgNativeWindow* window, int mode);

	///////////////////////////// Cursor

	///////////////////////////// File/Folder Browser

	OG_API bool og_window_file_browser_open(OgNativeWindow* window, const char* default_path, char* return_path);

	OG_API bool og_window_file_create_browser_open(OgNativeWindow* window, const char* default_path, char* return_path, void* data, unsigned int dataSize, const char* extension);

	OG_API bool og_window_folder_browser_open(OgNativeWindow* window, const char* default_path, char* return_path);

	OG_API bool og_window_file_browser_open_filter(OgNativeWindow* window, const char* default_path, char* return_path, const char* filter);

	///////////////////////////// File/Folder Browser

#ifdef __cplusplus
}
#endif

#endif // _OG_NATIVE_WINDOW_H_
