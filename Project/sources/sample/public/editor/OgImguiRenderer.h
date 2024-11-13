#pragma once
#ifndef _OG_IMGUI_RENDERER_H__
#define _OG_IMGUI_RENDERER_H__
#include "OgPrecompile.h"
#include "render/OgRenderContext.h"
#include "sample/public/core/util/OgRenderUtil.h"

/* #include"system/thirdparty/imgui/imgui.h" */ struct ImDrawData;

OG_NAMESPACE_SAMPLE_BEGIN




struct OG_API OgRenderParam
{
	uint32 guiContextKey;

	const OgSurface* surface;

	const ImDrawData* drawList;

};

class OgImguiRenderer
{
public:
	//==============================
	OgImguiRenderer(Render::OgRenderContext* renderContext);

	~OgImguiRenderer();


	// Render GUI
	void RenderGUI(const OgRenderParam& param);

	void NextFrame(Render::OgSwapChain* swapChain);
	
private:
	

	// sceneView renderer for editor
	Render::OgRenderContext* _renderContext;
	std::vector<Render::OgCommandEncoderHandle*> _encoders;

	
	uint32 _submitIndex;

	//=============================
};



OG_NAMESPACE_SAMPLE_END

#endif // _OG_IMGUI_RENDERER_H__