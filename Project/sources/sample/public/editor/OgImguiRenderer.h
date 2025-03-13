#pragma once
#ifndef OG_IMGUI_RENDERER_H
#define OG_IMGUI_RENDERER_H

#include "OgPrecompile.h"
#include "render/OgRenderContext.h"
#include "sample/public/core/util/OgRenderUtil.h"

struct ImDrawData;

OG_NAMESPACE_SAMPLE_BEGIN

struct OG_API OgRenderParam
{
    uint32 guiContextKey;
    const OgSurface* surface;
    const ImDrawData* drawList;
};

class OG_API OgImguiRenderer
{
public:
    explicit OgImguiRenderer(Render::OgRenderContext* renderContext);
    ~OgImguiRenderer();

    // Delete copy constructors
    OgImguiRenderer(const OgImguiRenderer&) = delete;
    OgImguiRenderer& operator=(const OgImguiRenderer&) = delete;

    // Main rendering functions
    void RenderGUI(Render::OgCommandEncoderHandle* encoder, const OgRenderParam& param);
    void NextFrame(Render::OgCommandEncoderHandle* encoder, Render::OgSwapChain* swapChain);

private:
    // Pipeline setup and cleanup
    void setupImGuiPipeline();
    void cleanupImGuiPipeline();
    void updateBuffers(const ImDrawData* drawData);

    // Render context
    Render::OgRenderContext* _renderContext;
    Render::OgSwapChain* _currentSwapChain{ nullptr };

    // Command encoders for triple buffering
    //std::vector<Render::OgCommandEncoderHandle*> _encoders;
    //uint32 _submitIndex;

    // Pipeline resources
    Render::OgPipelineHandle* _pipeline{ nullptr };
    Render::OgRenderPassHandle* _renderPass{ nullptr };

    // Buffers
    Render::OgBufferHandle* _vertexBuffer{ nullptr };
    Render::OgBufferHandle* _indexBuffer{ nullptr };

    // Resource layout and sets
    Render::OgResourceLayoutHandle* _resourceLayout{ nullptr };
    Render::OgResourceSetHandle* _resourceSet{ nullptr };
    Render::OgBufferHandle* _uniformBuffer;  // projection matrix를 위한 유니폼 버퍼
    

    // Font texture resources
    Render::OgTextureHandle* _fontTexture{ nullptr };
    Render::OgSamplerHandle* _fontSampler{ nullptr };

    // Constants
    static constexpr uint32 VERTEX_BUFFER_INITIAL_SIZE = 5000;
    static constexpr uint32 INDEX_BUFFER_INITIAL_SIZE = 10000;
};

OG_NAMESPACE_SAMPLE_END

#endif // OG_IMGUI_RENDERER_H