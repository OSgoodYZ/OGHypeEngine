#include "OgImguiRenderer.h"

OG_NAMESPACE_SAMPLE_BEGIN

OgImguiRenderer::OgImguiRenderer(Render::OgRenderContext* renderContext)
	:_renderContext(renderContext)
{
	for (size_t i = 0; i < _renderContext->maxSubmitCount; ++i)
	{
		_encoders.push_back(_renderContext->CreateCommandEncoder());
	}
}

OgImguiRenderer::~OgImguiRenderer()
{
	//TODO
}

void OgImguiRenderer::RenderGUI(const OgRenderParam& param)
{
	//TODO
}

void OgImguiRenderer::NextFrame(Render::OgSwapChain* swapChain)
{
	//TODO
}

OG_NAMESPACE_SAMPLE_END


