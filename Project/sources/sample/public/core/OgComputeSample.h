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
 * 이 샘플은 compute shader를 사용하여 두 배열의 요소를 더하는 
 * 간단한 GPGPU 작업을 수행합니다.
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

private:
    // 리소스 생성/파괴
    void createResources();
    void destroyResources();
    void createComputeShader();
    void createBuffers();
    void createComputePipeline();
    void createRenderingResources();

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

    // 파라미터
    static constexpr uint32 ARRAY_SIZE = 1024;
    static constexpr uint32 WORKGROUP_SIZE = 64;
    
    bool _computeExecuted = false;
    float _elapsedTime = 0.0f;
};

OG_NAMESPACE_SAMPLE_END

#endif // _OG_COMPUTE_SAMPLE_H__
