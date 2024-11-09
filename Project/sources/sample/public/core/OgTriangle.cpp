#include "OgTriangle.h"

using namespace std;
OG_NAMESPACE_SAMPLE_BEGIN

void OgTriangle::OnInit()
{
	for (size_t i = 0; i < _renderContext->maxSubmitCount; ++i)
	{
		_encoders.push_back(_renderContext->CreateCommandEncoder());
	}
}

void OgTriangle::OnRender(Render::OgSwapChain* swapchain)
{
	
	cerr << "You can continue working from here.! 2024/11/9!!" << endl;
	std::terminate();

	Render::OgCommandEncoderHandle* encoder = _encoders[_submitIndex];
	encoder->Begin();


	// TODO

	encoder->End();

	_renderContext->Submit(swapchain, encoder);
	
}

void OgTriangle::OnSuspent(Render::OgSwapChain* swapchain)
{
	_renderContext->Suspend(swapchain);
}

void OgTriangle::OnRestore(Render::OgSwapChain* swapchain)
{
	_renderContext->Restore(swapchain);
}

void OgTriangle::OnNextFrame(bool presentable)
{
	if (presentable)
	{
		_submitIndex = (_submitIndex + 1) % _renderContext->maxSubmitCount;
	}
}

void OgTriangle::OnPresent(Render::OgSwapChain* swapchain, bool presentable)
{
	if (presentable)
	{
		_renderContext->Present(swapchain);
	}
}

void OgTriangle::OnDestroy()
{
	for (int i = 0; i < _renderContext->maxSubmitCount; ++i)
	{
		_renderContext->DestroyCommandEncoder(_encoders[i]);
	}

	_encoders.clear();

}

OG_NAMESPACE_SAMPLE_END