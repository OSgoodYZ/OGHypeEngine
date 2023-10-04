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
	og_platform_window_get_size(window, &(window->width), &(window->height));
	og_platform_window_get_pos(window, &(window->x), &(window->y));

	if (window->monitor == NULL)
	{
		if (wConfig->visible)
		{
			og_platform_window_show(window);

			if (wConfig->focused)
			{
				og_platform_window_focus_in(window);
			}
		}
	}

	return window;
}

void og_window_destroy(OgNativeWindow* window)
{
	if (window == NULL)
		return;

	OgSystemContext* systemContext = og_system_get_context();

	// Unlink
	OgNativeWindow** prevWindow = &systemContext->headWindow;

	while (*prevWindow != window)
		prevWindow = &((*prevWindow)->next);

	(*prevWindow) = window->next;


	og_platform_window_destroy(window);

	if (window->title != NULL)
	{
		free(window->title);
	}

	OgEventWindow* w = static_cast<OgEventWindow*>(window);
	delete w; // allocated with new LvEventWindow()
}


void og_window_event_push(OgNativeWindow* window, OgNativeEvent* evt)
{
	OgEventWindow* win = reinterpret_cast<OgEventWindow*>(window);
	if (evt != nullptr)
	{
		win->queue.push(*evt);
	}
}

bool og_window_event_poll(OgNativeWindow* window, OgNativeEvent* evt)
{
	OG_CHECK(window != nullptr, "It should be exists window");
	OgEventWindow* win = reinterpret_cast<OgEventWindow*>(window);

	if (win->queue.size() > 0)
	{
		OgNativeEvent e = win->queue.back();
		memcpy(evt, &e, sizeof(OgNativeEvent));
		win->queue.pop();
		return true;
	}

	return false;
}

void og_window_show(OgNativeWindow* window)
{
	if (window == nullptr) OG_THROW("It should be exists window");
	og_platform_window_show(window);
}
void og_window_focus_in(OgNativeWindow* window)
{
	if (window == nullptr) OG_THROW("It should be exists window");
	og_platform_window_focus_in(window);
}