#include "system/OgSystemContext.h"
#include "system/private/platform/OgPlatformCommon.h"
#include "system/OgNativeEvent.h"

#include <stdlib.h>

#include "system/OgNativeWindow.h"

#if defined(__WIN32__)
#include "system/private/platform/win/OgPlatform_win32.h"
#endif


// Internal key state used for sticky keys
#define _OG_STICK 3

void og_platform_common_input_key(OgNativeWindow* window, int key, int scancode, int action, int mods)
{
	if (key >= 0 && key <= OG_KEY_LAST)
	{
		bool repeated = false;

		if (action == OG_RELEASE && window->input.keys[key] == OG_RELEASE)
			return;

		if (action == OG_PRESS && window->input.keys[key] == OG_PRESS)
			repeated = true;

		if (action == OG_RELEASE && window->stickyKeys)
		{
			window->input.keys[key] = _OG_STICK;
			window->input.currentKeys[key] = _OG_STICK;
		}
		else
		{
			window->input.keys[key] = (char)action;
			window->input.currentKeys[key] = (char)action;
		}

		if (repeated)
			action = OG_REPEAT;
	}

	if (!window->lockKeyMods)
		mods &= ~(OG_MOD_CAPS_LOCK | OG_MOD_NUM_LOCK);

	if (window->input.onKey)
	{
		window->input.onKey(window, key, scancode, action, mods);
	}

	// 현재 InputManager가 받아 오고 있으므로 여기에선 Event를 넣지 않는다.
}

void og_platform_common_input_char(OgNativeWindow* window, unsigned codepoint, int mods, bool plain)
{
	if (codepoint < 32 || (codepoint > 126 && codepoint < 160))
		return;

	if (!window->lockKeyMods)
		mods &= ~(OG_MOD_CAPS_LOCK | OG_MOD_NUM_LOCK);

	if (window->input.onCharacterModsInput)
		window->input.onCharacterModsInput(window, codepoint, mods);

	if (plain)
	{
		if (window->input.onCharacterInput)
		{
			window->input.onCharacterInput(window, codepoint);
		}
	}

	// 현재 InputManager가 받아 오고 있으므로 여기에선 Event를 넣지 않는다.
}

void og_platform_common_input_cursor_pos(OgNativeWindow* window, double xpos, double ypos)
{
	if (window->input.virtualCursorPosX == xpos && window->input.virtualCursorPosY == ypos)
		return;

	window->input.virtualCursorPosX = xpos;
	window->input.virtualCursorPosY = ypos;
	
	if (window->input.onCursorPos)
	{
		window->input.onCursorPos(window, xpos, ypos);
	}

	
	OgNativeEvent evt;
	memset(&evt, 0, sizeof(OgNativeEvent));
	evt.type = OgEventType::OG_MOUSE_MOVE;
	evt.mouse.pos.x = (int)xpos;
	evt.mouse.pos.y = (int)ypos;

	og_window_event_push(window, &evt);
	
}

void og_platform_common_mouse_click(OgNativeWindow* window, int button, int action, int mods)
{
	if (button < 0 || button > OG_MOUSE_BUTTON_LAST)
		return;

	if (!window->lockKeyMods)
		mods &= ~(OG_MOD_CAPS_LOCK | OG_MOD_NUM_LOCK);

	if (action == OG_RELEASE && window->stickyMouseButtons)
	{
		window->input.pointers[button] = _OG_STICK;
		window->input.currentPointers[button] = _OG_STICK;
	}
	else
	{
		window->input.pointers[button] = (char)action;
		window->input.currentPointers[button] = (char)action;
	}

	if (window->input.onMouseButton)
		window->input.onMouseButton(window, button, action, mods);


	OgNativeEvent evt;
	memset(&evt, 0, sizeof(OgNativeEvent));
	evt.type = action == OG_PRESS ? OgEventType::OG_MOUSE_PRESS : OgEventType::OG_MOUSE_RELEASE;
	evt.mouse.button = button;
	evt.mouse.mods = mods;

	og_window_event_push(window, &evt);

}

void og_platform_common_window_focus(OgNativeWindow* window, bool focused)
{
	//if (window->callbacks.focus)
	//	 window->callbacks.focus((GLFWwindow*)window, focused);

	window->focused = focused;
	if (!focused)
	{
		int key, button;

		for (key = 0; key <= OG_KEY_LAST; key++)
		{
			if (window->input.keys[key] == OG_PRESS)
			{
				const int scancode = og_platform_key_get_scancode(key);
				og_platform_common_input_key(window, key, scancode, OG_RELEASE, 0);
			}
		}


		for (button = 0; button <= OG_MOUSE_BUTTON_LAST; button++)
		{
			if (window->input.pointers[button] == OG_PRESS)
				og_platform_common_mouse_click(window, button, OG_RELEASE, 0);
		}
	
	}


	OgNativeEvent evt;
	memset(&evt, 0, sizeof(OgNativeEvent));
	evt.type = focused ? OgEventType::OG_WINDOW_FOCUS_IN : OgEventType::OG_WINDOW_FOCUS_OUT;
	og_window_event_push(window, &evt);
	
}

void og_platform_common_center_cursor_in_content_area(OgNativeWindow* window)
{
	int width, height;

	og_platform_window_get_size(window, &width, &height);
	og_platform_window_set_cursor_pos(window, width / 2.0, height / 2.0);
}

void og_platform_common_split_BPP(int bpp, int* red, int* green, int* blue)
{
	int delta;

	// We assume that by 32 the user really meant 24
	if (bpp == 32)
		bpp = 24;

	// Convert "bits per pixel" to red, green & blue sizes

	*red = *green = *blue = bpp / 3;
	delta = bpp - (*red * 3);
	if (delta >= 1)
		*green = *green + 1;

	if (delta == 2)
		*red = *red + 1;
}

const OgVideoMode* og_platform_common_choose_video_mode(OgMonitor* monitor, const OgVideoMode* desired)
{
    int i;
    unsigned int sizeDiff, leastSizeDiff = UINT_MAX;
    unsigned int rateDiff, leastRateDiff = UINT_MAX;
    unsigned int colorDiff, leastColorDiff = UINT_MAX;
    const OgVideoMode* current;
    const OgVideoMode* closest = NULL;

    if (!og_platform_common_refresh_video_modes(monitor))
        return NULL;

    for (i = 0; i < monitor->modeCount; i++)
    {
        current = monitor->modes + i;

        colorDiff = 0;

        if (desired->redBits != -1)
            colorDiff += abs(current->redBits - desired->redBits);
        if (desired->greenBits != -1)
            colorDiff += abs(current->greenBits - desired->greenBits);
        if (desired->blueBits != -1)
            colorDiff += abs(current->blueBits - desired->blueBits);

        sizeDiff = abs((current->width - desired->width) *
            (current->width - desired->width) +
            (current->height - desired->height) *
            (current->height - desired->height));

        if (desired->refreshRate != -1)
            rateDiff = abs(current->refreshRate - desired->refreshRate);
        else
            rateDiff = UINT_MAX - current->refreshRate;

        if ((colorDiff < leastColorDiff) ||
            (colorDiff == leastColorDiff && sizeDiff < leastSizeDiff) ||
            (colorDiff == leastColorDiff && sizeDiff == leastSizeDiff && rateDiff < leastRateDiff))
        {
            closest = current;
            leastSizeDiff = sizeDiff;
            leastRateDiff = rateDiff;
            leastColorDiff = colorDiff;
        }
    }

    return closest;
}

static int compare_video_mode_qsort(const void* fp, const void* sp)
{
	const OgVideoMode* fm = (OgVideoMode*)fp;
	const OgVideoMode* sm = (OgVideoMode*)sp;
	const int fbpp = fm->redBits + fm->greenBits + fm->blueBits;
	const int sbpp = sm->redBits + sm->greenBits + sm->blueBits;
	const int farea = fm->width * fm->height;
	const int sarea = sm->width * sm->height;

	// First sort on color bits per pixel
	if (fbpp != sbpp)
		return fbpp - sbpp;

	// Then sort on screen area
	if (farea != sarea)
		return farea - sarea;

	// Then sort on width
	if (fm->width != sm->width)
		return fm->width - sm->width;

	// Lastly sort on refresh rate
	return fm->refreshRate - sm->refreshRate;
}

int og_platform_common_compare_video_mode(const OgVideoMode* fp, const OgVideoMode* sp)
{
	return compare_video_mode_qsort((void*)fp, (void*)sp);
}

bool og_platform_common_refresh_video_modes(OgMonitor* monitor)
{
	int modeCount;
	OgVideoMode* modes;

	if (monitor->modes)
		return true;

	modes = og_platform_monitor_get_video_modes(monitor, &modeCount);
	if (!modes)
		return false;

	qsort(modes, modeCount, sizeof(OgVideoMode), compare_video_mode_qsort);

	free(monitor->modes);
	monitor->modes = modes;
	monitor->modeCount = modeCount;

	return true;
}

OgMonitor* og_platform_common_alloc_monitor(const char* name, int widthMM, int heightMM)
{
	OgMonitor* monitor = (OgMonitor*)calloc(1, sizeof(OgMonitor));
	monitor->physicalWidth = widthMM;
	monitor->physicalHeight = heightMM;

	if (name)
		monitor->name = strdup(name);

	return monitor;
}

void og_platform_common_free_monitor(OgMonitor* monitor)
{
	if (monitor == NULL)
		return;

	og_platform_monitor_free(monitor);

	// glfw gamma óϰ 

	free(monitor->modes);
	free(monitor->name);
	free(monitor);
}

void og_platform_common_input_monitor(OgSystemContext* sysCtx, OgMonitor* monitor, int action, int placement)
{
	if (action == (int)OgMonitorAction::CONNECTED)
	{
		++sysCtx->monitorCount;

		sysCtx->monitors = (OgMonitor**)realloc(sysCtx->monitors, sizeof(OgMonitor*) * sysCtx->monitorCount);

		if (placement == (int)OgMonitorAction::INSERT_FIRST)
		{
			memmove(sysCtx->monitors + 1, sysCtx->monitors, ((size_t)sysCtx->monitorCount - 1) * sizeof(OgMonitor*));
			sysCtx->monitors[0] = monitor;
		}
		else
			sysCtx->monitors[sysCtx->monitorCount - 1] = monitor;
	}
	else if (action == (int)OgMonitorAction::DISCONNECTED)
	{
		int i;
		OgNativeWindow* window;

		for (window = sysCtx->headWindow; window; window = window->next)
		{
			if (window->monitor == monitor)
			{
				int width, height, xoff, yoff;
				og_platform_window_get_size(window, &width, &height);
				og_platform_window_set_monitor(window, NULL, 0, 0, width, height, 0);
				og_platform_window_get_frame_size(window, &xoff, &yoff, NULL, NULL);
				og_platform_window_set_pos(window, xoff, yoff);
			}
		}

		for (i = 0; i < sysCtx->monitorCount; ++i)
		{
			if (sysCtx->monitors[i] == monitor)
			{
				--(sysCtx->monitorCount);
				memmove(sysCtx->monitors + i, sysCtx->monitors + i + 1, ((size_t)sysCtx->monitorCount - i) * sizeof(OgMonitor*));
				break;
			}
		}
	}

	// Callback?
	if (sysCtx->monitorCallback)
		sysCtx->monitorCallback(monitor, action);

	if (action == (int)OgMonitorAction::DISCONNECTED)
		og_platform_common_free_monitor(monitor);
}

void og_platform_common_input_monitor_window(OgMonitor* monitor, OgNativeWindow* window)
{
	monitor->window = window;
}

void og_platform_common_input_window_monitor(OgNativeWindow* window, OgMonitor* monitor)
{
	window->monitor = monitor;
}

