//#include "system/LvSystemContext.h"
//#include "system/private/platform/LvPlatformCommon.h"
//#include "system/LvNativeEvent.h"
//
//#include <stdlib.h>
//
//#if defined(__WIN32__)
//#include "system/private/platform/win/LvPlatform_win32.h"
//#elif defined(__MACOSX__)
//#include "system/private/platform/mac/LvPlatform_cocoa.h"
//#elif defined(__IOS__)
//#include "system/private/platform/ios/LvPlatform_ios.h"
//#elif defined(__ANDROID__)
//#include "system/private/platform/android/LvPlatform_android.h"
//#endif
//
//
//// Internal key state used for sticky keys
//#define _LV_STICK 3
//
//void lv_platform_common_input_key(LvNativeWindow* window, int key, int scancode, int action, int mods)
//{
//	if (key >= 0 && key <= LV_KEY_LAST)
//	{
//		bool repeated = false;
//
//		if (action == LV_RELEASE && window->input.keys[key] == LV_RELEASE)
//			return;
//
//		if (action == LV_PRESS && window->input.keys[key] == LV_PRESS)
//			repeated = true;
//
//		if (action == LV_RELEASE && window->stickyKeys)
//		{
//			window->input.keys[key] = _LV_STICK;
//			window->input.currentKeys[key] = _LV_STICK;
//		}
//		else
//		{
//			window->input.keys[key] = (char)action;
//			window->input.currentKeys[key] = (char)action;
//		}
//
//		if (repeated)
//			action = LV_REPEAT;
//	}
//
//	if (!window->lockKeyMods)
//		mods &= ~(LV_MOD_CAPS_LOCK | LV_MOD_NUM_LOCK);
//
//	if (window->input.onKey)
//	{
//		window->input.onKey(window, key, scancode, action, mods);
//	}
//
//	// 현재 InputManager가 받아 오고 있으므로 여기에선 Event를 넣지 않는다.
//}
//
//void lv_platform_common_input_char(LvNativeWindow* window, unsigned codepoint, int mods, bool plain)
//{
//	if (codepoint < 32 || (codepoint > 126 && codepoint < 160))
//		return;
//
//	if (!window->lockKeyMods)
//		mods &= ~(LV_MOD_CAPS_LOCK | LV_MOD_NUM_LOCK);
//
//	if (window->input.onCharacterModsInput)
//		window->input.onCharacterModsInput(window, codepoint, mods);
//
//	if (plain)
//	{
//		if (window->input.onCharacterInput)
//		{
//			window->input.onCharacterInput(window, codepoint);
//		}
//	}
//
//	// 현재 InputManager가 받아 오고 있으므로 여기에선 Event를 넣지 않는다.
//}
//
//void lv_platform_common_input_cursor_pos(LvNativeWindow* window, double xpos, double ypos)
//{
//	if (window->input.virtualCursorPosX == xpos && window->input.virtualCursorPosY == ypos)
//		return;
//
//	window->input.virtualCursorPosX = xpos;
//	window->input.virtualCursorPosY = ypos;
//	
//	if (window->input.onCursorPos)
//	{
//		window->input.onCursorPos(window, xpos, ypos);
//	}
//
//	/* 필요하면 넣을것
//	LvNativeEvent evt;
//	memset(&evt, 0, sizeof(LvNativeEvent));
//	evt.type == LvEventType::LV_MOUSE_MOVE;
//	evt.mouse.pos.x = (int)xpos;
//	evt.mouse.pos.y = (int)ypos;
//
//	lv_window_event_push(window, &evt);
//	*/
//}
//
//void lv_platform_common_mouse_click(LvNativeWindow* window, int button, int action, int mods)
//{
//	if (button < 0 || button > LV_MOUSE_BUTTON_LAST)
//		return;
//
//	if (!window->lockKeyMods)
//		mods &= ~(LV_MOD_CAPS_LOCK | LV_MOD_NUM_LOCK);
//
//	if (action == LV_RELEASE && window->stickyMouseButtons)
//	{
//		window->input.pointers[button] = _LV_STICK;
//		window->input.currentPointers[button] = _LV_STICK;
//	}
//	else
//	{
//		window->input.pointers[button] = (char)action;
//		window->input.currentPointers[button] = (char)action;
//	}
//
//	if (window->input.onMouseButton)
//		window->input.onMouseButton(window, button, action, mods);
//
//	/* 필요하면 넣을것
//	LvNativeEvent evt;
//	memset(&evt, 0, sizeof(LvNativeEvent));
//	evt.type == action == LV_PRESS ? LvEventType::LV_MOUSE_PRESS : LvEventType::LV_MOUSE_RELEASE;
//	evt.mouse.button = button;
//	evt.mouse.mods = mods;
//
//	lv_window_event_push(window, &evt);
//	*/
//}
//
//void lv_platform_common_window_focus(LvNativeWindow* window, bool focused)
//{
//	//if (window->callbacks.focus)
//		// window->callbacks.focus((GLFWwindow*)window, focused);
//
//	window->focused = focused;
//	if (!focused)
//	{
//		int key, button;
//
//		for (key = 0; key <= LV_KEY_LAST; key++)
//		{
//			if (window->input.keys[key] == LV_PRESS)
//			{
//				const int scancode = lv_platform_key_get_scancode(key);
//				lv_platform_common_input_key(window, key, scancode, LV_RELEASE, 0);
//			}
//		}
//
//		/*
//		for (button = 0; button <= LV_MOUSE_BUTTON_LAST; button++)
//		{
//			if (window->input.pointers[button] == LV_PRESS)
//				lv_platform_common_mouse_click(window, button, LV_RELEASE, 0);
//		}
//		*/
//	}
//
//	/* 필요하면 넣을것
//	LvNativeEvent evt;
//	memset(&evt, 0, sizeof(LvNativeEvent));
//	evt.type = focused ? LvEventType::LV_WINDOW_FOCUS_IN : LvEventType::LV_WINDOW_FOCUS_OUT;
//	lv_window_event_push(window, &evt);
//	*/
//}
//
//void lv_platform_common_center_cursor_in_content_area(LvNativeWindow* window)
//{
//	int width, height;
//
//	lv_platform_window_get_size(window, &width, &height);
//	lv_platform_window_set_cursor_pos(window, width / 2.0, height / 2.0);
//}
//
//void lv_platform_common_split_BPP(int bpp, int* red, int* green, int* blue)
//{
//	int delta;
//
//	// We assume that by 32 the user really meant 24
//	if (bpp == 32)
//		bpp = 24;
//
//	// Convert "bits per pixel" to red, green & blue sizes
//
//	*red = *green = *blue = bpp / 3;
//	delta = bpp - (*red * 3);
//	if (delta >= 1)
//		*green = *green + 1;
//
//	if (delta == 2)
//		*red = *red + 1;
//}
//
//const LvVideoMode* lv_platform_common_choose_video_mode(LvMonitor* monitor, const LvVideoMode* desired)
//{
//    int i;
//    unsigned int sizeDiff, leastSizeDiff = UINT_MAX;
//    unsigned int rateDiff, leastRateDiff = UINT_MAX;
//    unsigned int colorDiff, leastColorDiff = UINT_MAX;
//    const LvVideoMode* current;
//    const LvVideoMode* closest = NULL;
//
//    if (!lv_platform_common_refresh_video_modes(monitor))
//        return NULL;
//
//    for (i = 0; i < monitor->modeCount; i++)
//    {
//        current = monitor->modes + i;
//
//        colorDiff = 0;
//
//        if (desired->redBits != -1)
//            colorDiff += abs(current->redBits - desired->redBits);
//        if (desired->greenBits != -1)
//            colorDiff += abs(current->greenBits - desired->greenBits);
//        if (desired->blueBits != -1)
//            colorDiff += abs(current->blueBits - desired->blueBits);
//
//        sizeDiff = abs((current->width - desired->width) *
//            (current->width - desired->width) +
//            (current->height - desired->height) *
//            (current->height - desired->height));
//
//        if (desired->refreshRate != -1)
//            rateDiff = abs(current->refreshRate - desired->refreshRate);
//        else
//            rateDiff = UINT_MAX - current->refreshRate;
//
//        if ((colorDiff < leastColorDiff) ||
//            (colorDiff == leastColorDiff && sizeDiff < leastSizeDiff) ||
//            (colorDiff == leastColorDiff && sizeDiff == leastSizeDiff && rateDiff < leastRateDiff))
//        {
//            closest = current;
//            leastSizeDiff = sizeDiff;
//            leastRateDiff = rateDiff;
//            leastColorDiff = colorDiff;
//        }
//    }
//
//    return closest;
//}
//
//static int compare_video_mode_qsort(const void* fp, const void* sp)
//{
//	const LvVideoMode* fm = (LvVideoMode*)fp;
//	const LvVideoMode* sm = (LvVideoMode*)sp;
//	const int fbpp = fm->redBits + fm->greenBits + fm->blueBits;
//	const int sbpp = sm->redBits + sm->greenBits + sm->blueBits;
//	const int farea = fm->width * fm->height;
//	const int sarea = sm->width * sm->height;
//
//	// First sort on color bits per pixel
//	if (fbpp != sbpp)
//		return fbpp - sbpp;
//
//	// Then sort on screen area
//	if (farea != sarea)
//		return farea - sarea;
//
//	// Then sort on width
//	if (fm->width != sm->width)
//		return fm->width - sm->width;
//
//	// Lastly sort on refresh rate
//	return fm->refreshRate - sm->refreshRate;
//}
//
//int lv_platform_common_compare_video_mode(const LvVideoMode* fp, const LvVideoMode* sp)
//{
//	return compare_video_mode_qsort((void*)fp, (void*)sp);
//}
//
//bool lv_platform_common_refresh_video_modes(LvMonitor* monitor)
//{
//	int modeCount;
//	LvVideoMode* modes;
//
//	if (monitor->modes)
//		return true;
//
//	modes = lv_platform_monitor_get_video_modes(monitor, &modeCount);
//	if (!modes)
//		return false;
//
//	qsort(modes, modeCount, sizeof(LvVideoMode), compare_video_mode_qsort);
//
//	free(monitor->modes);
//	monitor->modes = modes;
//	monitor->modeCount = modeCount;
//
//	return true;
//}
//
//LvMonitor* lv_platform_common_alloc_monitor(const char* name, int widthMM, int heightMM)
//{
//	LvMonitor* monitor = (LvMonitor*)calloc(1, sizeof(LvMonitor));
//	monitor->physicalWidth = widthMM;
//	monitor->physicalHeight = heightMM;
//
//	if (name)
//		monitor->name = strdup(name);
//
//	return monitor;
//}
//
//void lv_platform_common_free_monitor(LvMonitor* monitor)
//{
//	if (monitor == NULL)
//		return;
//
//	lv_platform_monitor_free(monitor);
//
//	// glfw gamma óϰ 
//
//	free(monitor->modes);
//	free(monitor->name);
//	free(monitor);
//}
//
//void lv_platform_common_input_monitor(LvSystemContext* sysCtx, LvMonitor* monitor, int action, int placement)
//{
//	if (action == (int)LvMonitorAction::CONNECTED)
//	{
//		++sysCtx->monitorCount;
//
//		sysCtx->monitors = (LvMonitor**)realloc(sysCtx->monitors, sizeof(LvMonitor*) * sysCtx->monitorCount);
//
//		if (placement == (int)LvMonitorAction::INSERT_FIRST)
//		{
//			memmove(sysCtx->monitors + 1, sysCtx->monitors, ((size_t)sysCtx->monitorCount - 1) * sizeof(LvMonitor*));
//			sysCtx->monitors[0] = monitor;
//		}
//		else
//			sysCtx->monitors[sysCtx->monitorCount - 1] = monitor;
//	}
//	else if (action == (int)LvMonitorAction::DISCONNECTED)
//	{
//		int i;
//		LvNativeWindow* window;
//
//		for (window = sysCtx->headWindow; window; window = window->next)
//		{
//			if (window->monitor == monitor)
//			{
//				int width, height, xoff, yoff;
//				lv_platform_window_get_size(window, &width, &height);
//				lv_platform_window_set_monitor(window, NULL, 0, 0, width, height, 0);
//				lv_platform_window_get_frame_size(window, &xoff, &yoff, NULL, NULL);
//				lv_platform_window_set_pos(window, xoff, yoff);
//			}
//		}
//
//		for (i = 0; i < sysCtx->monitorCount; ++i)
//		{
//			if (sysCtx->monitors[i] == monitor)
//			{
//				--(sysCtx->monitorCount);
//				memmove(sysCtx->monitors + i, sysCtx->monitors + i + 1, ((size_t)sysCtx->monitorCount - i) * sizeof(LvMonitor*));
//				break;
//			}
//		}
//	}
//
//	// Callback?
//	if (sysCtx->monitorCallback)
//		sysCtx->monitorCallback(monitor, action);
//
//	if (action == (int)LvMonitorAction::DISCONNECTED)
//		lv_platform_common_free_monitor(monitor);
//}
//
//void lv_platform_common_input_monitor_window(LvMonitor* monitor, LvNativeWindow* window)
//{
//	monitor->window = window;
//}
//
//void lv_platform_common_input_window_monitor(LvNativeWindow* window, LvMonitor* monitor)
//{
//	window->monitor = monitor;
//}
