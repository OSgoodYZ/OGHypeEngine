#pragma once
#ifndef _OG_TRIANGLE_SAMPLE_H__
#define _OG_TRIANGLE_SAMPLE_H__

#include "OgSampleBase.h"
#include <memory>

OG_NAMESPACE_SAMPLE_BEGIN

/**
 * @brief 렌더 타겟에 삼각형을 렌더링하는 샘플
 */
class OG_API OgTriangleSample : public OgSampleBase
{
public:
    OgTriangleSample(Render::OgRenderContext* renderContext);
    ~OgTriangleSample() override;

    // OgSampleBase 인터페이스 구현
    void OnInit(Render::OgSwapChain* swapchain) override;
    void OnDestroy() override;
    void OnRender(Render::OgCommandEncoderHandle* encoder, Render::OgSwapChain* swapchain) override;
    void OnSuspend(Render::OgSwapChain* swapchain) override;
    void OnRestore(Render::OgSwapChain* swapchain) override;
    void OnResize(uint32 width, uint32 height) override;

    // 렌더 타겟 인터페이스
    Render::OgTextureHandle* GetRenderTargetTexture() const override { return _renderTargetTexture; }
    uint16 GetRenderTargetWidth() const override { return _renderTargetWidth; }
    uint16 GetRenderTargetHeight() const override { return _renderTargetHeight; }

private:
    void createResources(uint16 width, uint16 height);
    void destroyResources();
    void createShaders();
    void createPipeline();
    void createRenderTarget(uint16 width, uint16 height);
    void destroyRenderTarget();

private:
    // 기본 렌더링 리소스
    Render::OgBufferHandle* _vertexBuffer = nullptr;
    Render::OgShaderHandle* _vertexShader = nullptr;
    Render::OgShaderHandle* _fragmentShader = nullptr;
    Render::OgProgramHandle* _program = nullptr;
    Render::OgResourceLayoutHandle* _resourceLayout = nullptr;
    Render::OgPipelineHandle* _pipeline = nullptr;

    // 렌더 타겟 리소스
    Render::OgTextureHandle* _renderTargetTexture = nullptr;
    Render::OgTextureHandle* _depthTexture = nullptr;
    Render::OgFrameBufferHandle* _renderTargetFrameBuffer = nullptr;
    Render::OgRenderPassHandle* _renderTargetRenderPass = nullptr;
    
    uint16 _renderTargetWidth = 0;
    uint16 _renderTargetHeight = 0;

    // 정점 데이터
    static constexpr float TRIANGLE_VERTICES[] = {
        -0.8f, -0.8f, 0.0f,
         0.8f, -0.8f, 0.0f,
         0.0f,  0.8f, 0.0f
    };
};

OG_NAMESPACE_SAMPLE_END

#endif // _OG_TRIANGLE_SAMPLE_H__
