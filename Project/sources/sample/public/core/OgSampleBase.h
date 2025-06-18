#pragma once
#ifndef _OG_SAMPLE_BASE_H__
#define _OG_SAMPLE_BASE_H__

#include "OgPrecompile.h"
#include "render/OgRenderContext.h"

OG_NAMESPACE_SAMPLE_BEGIN

/**
 * @brief 모든 렌더링 샘플의 기본 인터페이스
 */
class OG_API OgSampleBase
{
public:
    OgSampleBase(Render::OgRenderContext* renderContext)
        : _renderContext(renderContext)
        , _isInitialized(false)
    {
    }

    virtual ~OgSampleBase() = default;

    // 라이프사이클 메서드들
    virtual void OnInit(Render::OgSwapChain* swapchain) = 0;
    virtual void OnDestroy() = 0;
    virtual void OnUpdate(float deltaTime) {}
    virtual void OnRender(Render::OgCommandEncoderHandle* encoder, Render::OgSwapChain* swapchain) = 0;
    virtual void OnResize(uint32 width, uint32 height) {}
    virtual void OnSuspend(Render::OgSwapChain* swapchain) {}
    virtual void OnRestore(Render::OgSwapChain* swapchain) {}
    
    // 렌더 타겟 관련 (옵션)
    virtual Render::OgTextureHandle* GetRenderTargetTexture() const { return nullptr; }
    virtual uint16 GetRenderTargetWidth() const { return 0; }
    virtual uint16 GetRenderTargetHeight() const { return 0; }

    bool IsInitialized() const { return _isInitialized; }

protected:
    Render::OgRenderContext* _renderContext;
    bool _isInitialized;
};

OG_NAMESPACE_SAMPLE_END

#endif // _OG_SAMPLE_BASE_H__
