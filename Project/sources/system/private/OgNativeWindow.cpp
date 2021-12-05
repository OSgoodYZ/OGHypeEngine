#include "OgPrecompile.h"
#include "system/OgNativeWindow.h"

#include "system/OgSystemContext.h"

#include "system/OgMemory.h"
#include "system/OgVector.h"
#include "system/OgNativeEvent.h"
#include <assert.h>
#include <queue>

#if defined(__WIN32__)
#include "system/private/platform/win/OgPlatform_win32.h"
#elif defined(__MACOSX__)
//TODO
#elif defined(__IOS__)
//TODO
#elif defined(__ANDROID__)
//TODO
#endif

struct OgEventWindow : OgNativeWindow
{
	std::queue<OgNativeEvent> queue;

	Og::System::OgVector<OgEventWindow*> childs;

	~OgEventWindow()
	{
	}
};


OgNativeWindow* og_window_create(OgSystemContext* context, const OgWindowConfig* wConfig, const OgFrameBufferConfig* fbConfig)
{
	ASSERT(wConfig->title != NULL);
	ASSERT(wConfig->width >= 0);
	ASSERT(wConfig->height >= 0);

	if (!context->isInitialized)
	{
		LOGE(OG_ID, "Context is not initialized");
		exit(-1);
	}

	OgNativeWindow* window = new OgEventWindow();

	if (context->headWindow == nullptr)
	{
		context->headWindow = window;
	}
	else
	{
		OgNativeWindow* lastWindow = context->headWindow;
		while (lastWindow->next != NULL)
		{
			lastWindow = lastWindow->next;
		}

		lastWindow->next = window;
	}

	window->next = NULL;

	if (wConfig->title)
	{
		size_t titleLen = strlen(wConfig->title);
		window->title = (char*)malloc(titleLen + 1);
		memcpy(window->title, wConfig->title, sizeof(char) * titleLen);
		window->title[titleLen] = '\0';
	}
	else
	{
		window->title = NULL;
	}

	window->resizable = wConfig->resizable;
	window->decorated = wConfig->decorated;
	window->autoIconify = wConfig->autoIconify;
	window->floating = wConfig->floating;
	window->shouldClose = false;
	window->minimized = false;

	window->videoMode.redBits = fbConfig->redBits;
	window->videoMode.greenBits = fbConfig->greenBits;
	window->videoMode.blueBits = fbConfig->blueBits;
	window->videoMode.refreshRate = context->configs.refreshRate;

	window->monitor = wConfig->monitor;

	window->cursor = NULL;

	window->x = OG_DONT_CARE;
	window->y = OG_DONT_CARE;
	window->width = OG_DONT_CARE;
	window->height = OG_DONT_CARE;
	window->minWidth = OG_DONT_CARE;
	window->minHeight = OG_DONT_CARE;
	window->maxWidth = OG_DONT_CARE;
	window->maxHeight = OG_DONT_CARE;
	window->numer = OG_DONT_CARE;
	window->denom = OG_DONT_CARE;

	window->stickyKeys = false;
	window->stickyMouseButtons = false;
	window->lockKeyMods = false;
	window->cursorMode = OG_CURSOR_NORMAL;
	window->rawMouseMotion = false;

	memset(&(window->input), 0, sizeof(OgInput));

	window->context = context;

	if (!og_platform_window_create(window, wConfig, fbConfig))
	{
		og_window_destroy(window);
		return NULL;
	}

	// set the start dimension/coordinate of window
	lv_platform_window_get_size(window, &(window->width), &(window->height));
	lv_platform_window_get_pos(window, &(window->x), &(window->y));

	if (window->monitor == NULL)
	{
		if (wConfig->visible)
		{
			lv_platform_window_show(window);

			if (wConfig->focused)
			{
				lv_platform_window_focus_in(window);
			}
		}
	}

	return window;
}

void lv_window_destroy(LvNativeWindow* window)
{
	if (window == NULL)
		return;

	LvSystemContext* systemContext = lv_system_get_context();

	// Unlink
	LvNativeWindow** prevWindow = &systemContext->headWindow;

	while (*prevWindow != window)
		prevWindow = &((*prevWindow)->next);

	(*prevWindow) = window->next;


	lv_platform_window_destroy(window);

	if (window->title != NULL)
	{
		free(window->title);
	}

	LvEventWindow* w = static_cast<LvEventWindow*>(window);
	delete w; // allocated with new LvEventWindow()
}


void lv_window_event_push(LvNativeWindow* window, LvNativeEvent* evt)
{
	LvEventWindow* win = reinterpret_cast<LvEventWindow*>(window);
	if (evt != nullptr)
	{
		win->queue.Enqueue(*evt);
	}
}

bool lv_window_event_poll(LvNativeWindow* window, LvNativeEvent* evt)
{
	LV_CHECK(window != nullptr, "It should be exists window");
	LvEventWindow* win = reinterpret_cast<LvEventWindow*>(window);

	if (win->queue.Count() > 0)
	{
		LvNativeEvent e = win->queue.Dequeue();
		memcpy(evt, &e, sizeof(LvNativeEvent));
		return true;
	}

	return false;
}