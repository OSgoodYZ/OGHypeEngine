#pragma once
#ifndef OG_IMGUI_RENDERER_H
#define OG_IMGUI_RENDERER_H

#include "OgPrecompile.h"
#include <unordered_map>
#include "render/OgRenderContext.h"
#include "sample/public/core/util/OgRenderUtil.h"


struct ImDrawData;

OG_NAMESPACE_SAMPLE_BEGIN
#pragma region RendererResource
struct OG_API OgGUIContextResource
{
    OgGUIContextResource()
        : vertexShaderHandle(nullptr)
        , fragmentShanderHandle(nullptr)
        , programHandle(nullptr)
        , resourceLayout(nullptr)
        , fontTextureHandle(nullptr)
    {
    }

    OgGUIContextResource(const OgGUIContextResource& o)
        : vertexShaderHandle(o.vertexShaderHandle)
        , fragmentShanderHandle(o.fragmentShanderHandle)
        , programHandle(o.programHandle)
        , layout(o.layout)
        , fontTextureHandle(o.fontTextureHandle)
        , resourceLayout(o.resourceLayout)
        , pipelineDescriptor(o.pipelineDescriptor)
    {
        for (size_t i = 0; i < 4; ++i)
        {
            bindings[i] = o.bindings[i];
        }
    }

    Render::OgShaderHandle* vertexShaderHandle;
    Render::OgShaderHandle* fragmentShanderHandle;
    Render::OgProgramHandle* programHandle;
    Render::OgBufferLayout layout;
    Render::OgTextureHandle* fontTextureHandle;
    Render::OgResourceBinding bindings[4];
    Render::OgResourceLayoutHandle* resourceLayout;
    Render::OgPipelineDescriptor pipelineDescriptor;

    uint32 bindingCount = 0;
};

struct OG_API OgSurfaceResource
{
    OgSurfaceResource()
        : renderPassHandle(nullptr)
        , frameBufferHandle(nullptr)
        , pipelineHandle(nullptr)
    {
    }

    OgSurfaceResource(const OgSurfaceResource& o)
        : vertexBufferHandles(o.vertexBufferHandles)
        , indexBufferHandles(o.indexBufferHandles)
        , renderPassHandle(o.renderPassHandle)
        , frameBufferHandle(o.frameBufferHandle)
        , pipelineHandle(o.pipelineHandle)
    {
        uniformBufferHandles[0] = o.uniformBufferHandles[0];
        uniformBufferHandles[1] = o.uniformBufferHandles[1];
        uniformBufferHandles[2] = o.uniformBufferHandles[2];
    }

    ~OgSurfaceResource()
    {
        printf("");
    }

    vector<Render::OgBufferHandle*> vertexBufferHandles;
    vector<Render::OgBufferHandle*> indexBufferHandles;
    vector<Render::OgBufferHandle*> uniformBufferHandles[3];
    Render::OgRenderPassHandle* renderPassHandle;
    Render::OgFrameBufferHandle* frameBufferHandle;
    Render::OgPipelineHandle* pipelineHandle;

};

#pragma endregion

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
    Render::OgBufferHandle* _uniformBuffers[3] = { nullptr, };
    Render::OgBufferHandle* _vertexBuffer{ nullptr };
    Render::OgBufferHandle* _indexBuffer{ nullptr };


	
    // Resource layout and sets
    OG_DEPRECATED
    Render::OgResourceLayoutHandle* _resourceLayout{ nullptr };
    OG_DEPRECATED
    Render::OgResourceSetHandle* _resourceSet{ nullptr };
    OG_DEPRECATED
    Render::OgBufferHandle* _uniformBuffer;  // projection matrix를 위한 유니폼 버퍼
    
	std::unordered_map<uint32, OgGUIContextResource> _guiContextResources;
	std::unordered_map<OgSurface*, OgSurfaceResource> _surfaceResources;

    // Font texture resources
    Render::OgTextureHandle* _fontTexture{ nullptr };
    Render::OgSamplerHandle* _fontSampler{ nullptr };

    // Constants
    static constexpr uint32 VERTEX_BUFFER_INITIAL_SIZE = 5000;
    static constexpr uint32 INDEX_BUFFER_INITIAL_SIZE = 10000;
};

OG_NAMESPACE_SAMPLE_END

#endif // OG_IMGUI_RENDERER_H