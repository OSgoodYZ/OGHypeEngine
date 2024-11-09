#pragma once
#ifndef _OG_TRIANGLE_H__
#define _OG_TRIANGLE_H__
#include <vector>

#include "OgPrecompile.h"
#include "render/OgRenderContext.h"

using namespace std;

OG_NAMESPACE_SAMPLE_BEGIN

/**
* @brief Render backend를 테스트 하기위한 sample 코드의 main Loop입니다.
*/
class OG_API OgTriangle
{
public:
	OgTriangle(Render::OgRenderContext* renderContext)
		:_renderContext(renderContext)
		, _submitIndex{0}
	{
		OnInit();
	};

	~OgTriangle() 
	{
		OnDestroy();
	};

	void OnInit();

	void OnRender(Render::OgSwapChain* swapchain);


	void OnSuspent(Render::OgSwapChain* swapchain);

	void OnRestore(Render::OgSwapChain* swapchain);


	void OnNextFrame(bool presentable);

	void OnPresent(Render::OgSwapChain* swapchain, bool presentable);

	void OnDestroy();


private:


	Render::OgRenderContext* _renderContext = nullptr;


	std::vector<Render::OgCommandEncoderHandle*> _encoders;
	uint8 _submitIndex;
};

OG_NAMESPACE_SAMPLE_END


#endif // _OG_TRIANGLE_H__