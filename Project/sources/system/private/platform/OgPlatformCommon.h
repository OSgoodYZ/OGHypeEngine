#ifndef __OG_PLATFORM_COMMON_H__
#define __OG_PLATFORM_COMMON_H__

#ifdef __cplusplus
extern "C"
{
#endif

void og_platform_common_input_key(OgNativeWindow* window, int key, int scancode, int action, int mods);

void og_platform_common_input_char(OgNativeWindow* window, unsigned codepoint, int mods, bool plain);

void og_platform_common_input_cursor_pos(OgNativeWindow* window, double xpos, double ypos);

void og_platform_common_mouse_click(OgNativeWindow* window, int button, int action, int mods);

void og_platform_common_window_focus(OgNativeWindow* window, bool focus);

void og_platform_common_center_cursor_in_content_area(OgNativeWindow* window);

void og_platform_common_split_BPP(int bpp, int* red, int* green, int* blue);

const OgVideoMode* og_platform_common_choose_video_mode(OgMonitor* monitor, const OgVideoMode* desired);

int og_platform_common_compare_video_mode(const OgVideoMode* fp, const OgVideoMode* sp);

bool og_platform_common_refresh_video_modes(OgMonitor* monitor);

OgMonitor* og_platform_common_alloc_monitor(const char* name, int widthMM, int heightMM);

void og_platform_common_free_monitor(OgMonitor* monitor);

void og_platform_common_input_monitor(OgSystemContext* sysCtx, OgMonitor* monitor, int action, int placement);

void og_platform_common_input_monitor_window(OgMonitor* monitor, OgNativeWindow* window);

void og_platform_common_input_window_monitor(OgNativeWindow* window, OgMonitor* monitor);

#ifdef __cplusplus
}
#endif

#endif
