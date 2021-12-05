#pragma once
#ifndef _OG_WINDOW_WIN32_H_
#define _OG_WINDOW_WIN32_H_

#include "OgPrecompile.h"

#ifdef __cplusplus
extern "C" {
#endif

//OG_NAMESPACE_SYSTEM_BEGIN

static int og_platform_window_create_native(OgNativeWindow* window, const OgWindowConfig *config);


static int og_platform_window_create(OgNativeWindow *window, const OgWindowConfig *wConfig,	const OgFrameBufferConfig *fbConfig);

int og_platform_system_init(OgSystemContext* context, void* platformHandle);


static void update_cursor_clip_rect(OgNativeWindow* window);


//OG_NAMESPACE_SYSTEM_END



#ifdef __cplusplus
}
#endif
#endif // _OG_WINDOW_WIN32_H_