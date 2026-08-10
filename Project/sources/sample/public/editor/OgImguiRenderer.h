#pragma once
#ifndef OG_IMGUI_RENDERER_H
#define OG_IMGUI_RENDERER_H

#include "OgPrecompile.h"
#include <unordered_map>
#include "render/OgRenderContext.h"
#include "sample/public/core/util/OgRenderUtil.h"
#include "system/OgImGUIManager.h"

struct ImDrawData;
struct ImGuiContext;

OG_NAMESPACE_SAMPLE_BEGIN

// ImGui vertex 구조체 정의
struct ImGuiVertex {
    float pos[2];
    float uv[2];
    uint32_t col;
};

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
        for (int i = 0; i < 3; ++i) {
            uniformBufferHandles[i] = o.uniformBufferHandles[i];
        }
    }

    ~OgSurfaceResource()
    {
        // Resource cleanup should be handled by the renderer
    }

    std::vector<Render::OgBufferHandle*> vertexBufferHandles;
    std::vector<Render::OgBufferHandle*> indexBufferHandles;
    std::vector<Render::OgBufferHandle*> uniformBufferHandles[3];
    Render::OgRenderPassHandle* renderPassHandle;
    Render::OgFrameBufferHandle* frameBufferHandle;
    Render::OgPipelineHandle* pipelineHandle;
};

#pragma endregion

struct OG_API OgRenderParam
{
    size_t guiContextKey;
    const Render::OgSwapChain* surface;
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

    // Context management
    void UpdateGPUContext(ImGuiContext* context);
    void RemoveGPUContext(ImGuiContext* context);
    void UpdateSurface(Render::OgSwapChain* surface);
    void RemoveSurface(Render::OgSwapChain* surface);

    // Main rendering functions
    void RenderGUI(Render::OgCommandEncoderHandle* encoder, const OgRenderParam& param);
    void NextFrame(Render::OgCommandEncoderHandle* encoder, Render::OgSwapChain* swapChain);
    
    // 새로 추가한 함수: 외부 텍스쳐를 이미지로 사용할 수 있도록 설정
    void SetExternalTexture(Render::OgTextureHandle* texture);
    Render::OgTextureHandle* GetExternalTexture() const { return _externalTexture; }

    // 캐시된 리소스 셋이 참조하는 텍스처/버퍼가 파괴되기 전에 호출해야 한다 (디바이스 유휴 상태에서 호출)
    void ClearResourceSetCache();

private:
    // Pipeline setup and cleanup
    void setupImGuiPipeline();
    void cleanupImGuiPipeline();
    void updateBuffers(const ImDrawData* drawData);
    Render::OgResourceSetHandle* getResourceSet(Render::OgTextureHandle* texture, Render::OgBufferHandle* uniform);

    // Helper function for shader compilation
    bool compileGLSLtoSPIRV(const char* shaderCode, Render::OgShaderType shaderType, std::vector<uint32_t>& spirvOut);

    // Render context
    Render::OgRenderContext* _renderContext;
    Render::OgSwapChain* _currentSwapChain{ nullptr };

    // Pipeline resources
    Render::OgPipelineHandle* _pipeline{ nullptr };
    Render::OgRenderPassHandle* _renderPass{ nullptr };
    Render::OgResourceLayoutHandle* _resourceLayout{ nullptr };
    Render::OgResourceSetHandle* _resourceSet{ nullptr };

    // Buffers
    Render::OgBufferHandle* _vertexBuffer{ nullptr };
    Render::OgBufferHandle* _indexBuffer{ nullptr };
    Render::OgBufferHandle* _uniformBuffer{ nullptr };  // for projection matrix

    // Resource maps for context and surfaces
    std::unordered_map<size_t, OgGUIContextResource> _guiContextResources;
    std::unordered_map<Render::OgSwapChain*, OgSurfaceResource> _surfaceResources;


    std::unordered_map<size_t, Render::OgResourceSetHandle*> _guiResourceSetHandleMap;

    // Font texture resources
    Render::OgSamplerHandle* _fontSampler{ nullptr };
    
    // 외부 텍스쳐 (삼각형 렌더 타겟)
    Render::OgTextureHandle* _externalTexture{ nullptr };

    // Constants
    static constexpr uint32 VERTEX_BUFFER_INITIAL_SIZE = 5000;
    static constexpr uint32 INDEX_BUFFER_INITIAL_SIZE = 10000;
};

OG_NAMESPACE_SAMPLE_END

#endif // OG_IMGUI_RENDERER_H