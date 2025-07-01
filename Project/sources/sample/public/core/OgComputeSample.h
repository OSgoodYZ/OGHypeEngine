#pragma once
#ifndef _OG_COMPUTE_SAMPLE_H__
#define _OG_COMPUTE_SAMPLE_H__

#include "OgPrecompile.h"
#include "OgSampleBase.h"
#include "render/OgRenderContext.h"
#include "render/OgRenderDefinitions.h"

OG_NAMESPACE_SAMPLE_BEGIN

/**
 * @brief Compute Shader를 테스트하는 샘플
 *
 * 이 샘플은 compute shader를 사용하여 두 배열의 요소를 더하고
 * 시각적으로 흥미로운 패턴을 생성하는 GPGPU 작업을 수행합니다.
 * 결과는 실시간으로 렌더링되어 화면에 표시됩니다.
 */
class OG_API OgComputeSample : public OgSampleBase
{
public:
    OgComputeSample(Render::OgRenderContext* renderContext);
    virtual ~OgComputeSample();

    // OgSampleBase 인터페이스 구현
    void OnInit(Render::OgSwapChain* swapchain) override;
    void OnDestroy() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(Render::OgCommandEncoderHandle* encoder, Render::OgSwapChain* swapchain) override;
    void OnResize(uint32 width, uint32 height) override;
    void OnSuspend(Render::OgSwapChain* swapchain) override;
    void OnRestore(Render::OgSwapChain* swapchain) override;

    // Compute 결과를 얻는 메서드
    bool GetComputeResults(float* outResults, uint32 count);
    
    // 렌더 타겟 관련 오버라이드
    Render::OgTextureHandle* GetRenderTargetTexture() const override { return _renderTargetTexture; }
    uint16 GetRenderTargetWidth() const override { return _renderTargetWidth; }
    uint16 GetRenderTargetHeight() const override { return _renderTargetHeight; }

private:
    // 리소스 생성/파괴
    void createResources(uint16 width, uint16 height);
    void destroyResources();
    void createComputeShader();
    void createBuffers();
    void createComputePipeline();
    void createRenderingResources();
    void createRenderTarget(uint16 width, uint16 height);
    void destroyRenderTarget();

    // Compute 실행
    void executeCompute(Render::OgSwapChain* swapchain);

    // 결과 시각화
    void renderResults(Render::OgCommandEncoderHandle* encoder, uint32 width, uint32 height);

private:
    // Compute shader 리소스
    Render::OgShaderHandle* _computeShader = nullptr;
    Render::OgPipelineHandle* _computePipeline = nullptr;
    Render::OgResourceLayoutHandle* _computeResourceLayout = nullptr;
    Render::OgResourceSetHandle* _computeResourceSet = nullptr;

    // 버퍼들
    Render::OgBufferHandle* _inputBufferA = nullptr;
    Render::OgBufferHandle* _inputBufferB = nullptr;
    Render::OgBufferHandle* _outputBuffer = nullptr;
    Render::OgBufferHandle* _uniformBuffer = nullptr;

    // 렌더링용 리소스
    Render::OgShaderHandle* _vertexShader = nullptr;
    Render::OgShaderHandle* _fragmentShader = nullptr;
    Render::OgProgramHandle* _program = nullptr;
    Render::OgPipelineHandle* _renderPipeline = nullptr;
    Render::OgResourceLayoutHandle* _renderResourceLayout = nullptr;
    Render::OgResourceSetHandle* _renderResourceSet = nullptr;
    Render::OgBufferHandle* _vertexBuffer = nullptr;
    Render::OgBufferHandle* _renderUniformBuffer = nullptr;

    // 렌더 타겟 리소스
    Render::OgTextureHandle* _renderTargetTexture = nullptr;
    Render::OgTextureHandle* _depthTexture = nullptr;
    Render::OgRenderPassHandle* _renderTargetRenderPass = nullptr;
    Render::OgFrameBufferHandle* _renderTargetFrameBuffer = nullptr;
    uint16 _renderTargetWidth = 0;
    uint16 _renderTargetHeight = 0;

    // 파라미터
    static constexpr uint32 ARRAY_SIZE = 1024;
    static constexpr uint32 WORKGROUP_SIZE = 64;

    // 상태 변수
    bool _computeExecuted = false;
    float _elapsedTime = 0.0f;
    float _accumulatedTime = 0.0f;  // Compute shader 재실행 타이머
};

OG_NAMESPACE_SAMPLE_END

#endif // _OG_COMPUTE_SAMPLE_H__
