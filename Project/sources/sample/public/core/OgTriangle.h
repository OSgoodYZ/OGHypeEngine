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
		, _renderTargetTexture(nullptr)
		, _renderTargetFrameBuffer(nullptr)
		, _renderTargetRenderPass(nullptr)
		//, _submitIndex{0}
	{
	};

	~OgTriangle() 
	{
		
	};

	void OnInit(Render::OgSwapChain* swapchain);

	void OnRender(Render::OgCommandEncoderHandle* encoder, Render::OgSwapChain* swapchain);

	void OnSuspend(Render::OgSwapChain* swapchain);

	void OnRestore(Render::OgSwapChain* swapchain);

	void OnNextFrame(bool presentable);

	void OnPresent(Render::OgSwapChain* swapchain, bool presentable);

	void OnDestroy();

	// 렌더 타겟 텍스쳐를 반환하는 함수
	Render::OgTextureHandle* GetRenderTargetTexture() const { return _renderTargetTexture; }


private:

	void createResourceHandles(Render::OgSwapChain* swapchain);

	void destroyResourceHandles();

	// Render resources
	Render::OgBufferHandle* _vertexBuffer;
	Render::OgBufferHandle* _uniformBuffer;
	Render::OgShaderHandle* _vertexShader;
	Render::OgShaderHandle* _fragmentShader;
	Render::OgProgramHandle* _program;
	Render::OgResourceLayoutHandle* _resourceLayout;
	Render::OgRenderPassHandle* _renderPass;
	Render::OgFrameBufferHandle* _frameBuffer;
	Render::OgPipelineHandle* _pipeline;

	// Render target texture resources
	Render::OgTextureHandle* _renderTargetTexture;
	Render::OgFrameBufferHandle* _renderTargetFrameBuffer;
	Render::OgRenderPassHandle* _renderTargetRenderPass;



	Render::OgRenderContext* _renderContext = nullptr;
	//std::vector<Render::OgCommandEncoderHandle*> _encoders;
	//uint8 _submitIndex;
};

OG_NAMESPACE_SAMPLE_END


#endif // _OG_TRIANGLE_H__