#pragma warning(disable:4251)
#pragma once
#ifndef _OG_RENDER_DEFINITIONS_H__
#define _OG_RENDER_DEFINITIONS_H__

#include "OgPrecompile.h"

/*#include "OgRenderContext.h" */namespace Og { namespace Render { class OgRenderContext; } }

OG_NAMESPACE_BEGIN

struct OG_API OgSwapChain
{
public:
	OgSwapChain();

	uint16 bufferCount : 14;

	uint16 useDepthBuffer : 1;

	uint16 useStencilBuffer : 1;				// 2 bytes

	LvPixelFormat colorPixelFormat;				// 1

	LvRenderTextureFormat colorRenderFormat;	// 1

	LvPixelFormat depthPixelFormat;				// 1

	LvRenderTextureFormat depthRenderFormat;	// 1

	LvPixelFormat stencilPixelFormat;			// 1

	LvRenderTextureFormat stencilRenderFormat;	// 1

	
};

	
OG_NAMESPACE_END

#endif // _OG_RENDER_DEFINITIONS_H__
