#include "OgImguiRenderer.h"
#include "render/OgRenderDefinitions.h"
#include "imgui.h"
#include "slang.h"

using namespace Og::Render;
OG_NAMESPACE_SAMPLE_BEGIN

namespace {
    // ImGui vertex 구조체 정의
    struct ImGuiVertex {
        float pos[2];
        float uv[2];
        uint32_t col;
    };
}

// Slang을 사용하여 GLSL을 SPIR-V로 컴파일하는 함수
bool compileGLSLtoSPIRV(
    const char* shaderCode,
    OgShaderType shaderType,
    std::vector<uint32_t>& spirvOut)
{
    // Slang 세션 생성
    // Slang 세션 생성
    SlangGlobalSessionDesc globalSessionDesc = {};
    globalSessionDesc.enableGLSL = true;  // GLSL 지원 활성화

    slang::IGlobalSession* slangSession = nullptr;
    if (SLANG_FAILED(slang::createGlobalSession(&globalSessionDesc, &slangSession)))
    {
        std::cerr << "Failed to create Slang session" << std::endl;
        return false;
    }

    // 세션 생성
    slang::SessionDesc sessionDesc = {};
    slang::ISession* session = nullptr;
    if (SLANG_FAILED(slangSession->createSession(sessionDesc, &session)))
    {
        std::cerr << "Failed to create session" << std::endl;
        slangSession->release();
        return false;
    }

    // 컴파일 요청 생성
    SlangCompileRequest* slangRequest = nullptr;
    if (SLANG_FAILED(session->createCompileRequest(&slangRequest)))
    {
        std::cerr << "Failed to create compile request" << std::endl;
        session->release();
        slangSession->release();
        return false;
    }

    // 셰이더 타입에 따른 프로파일 설정
	SlangStage slangStage = SLANG_STAGE_NONE;
    
    switch (shaderType)
    {
    case OgShaderType::VERTEX:
		slangStage = SLANG_STAGE_VERTEX;
        break;
    case OgShaderType::FRAGMENT:
        slangStage = SLANG_STAGE_FRAGMENT;
        break;
    case OgShaderType::COMPUTE:
        slangStage = SLANG_STAGE_COMPUTE;
        break;
    default:
        std::cerr << "Unsupported shader type" << std::endl;
        spDestroyCompileRequest(slangRequest);
        session->release();
        slangSession->release();
        return false;
    }

    // SPIR-V 타겟 추가
    spAddCodeGenTarget(slangRequest, SLANG_SPIRV);

    // GLSL 소스코드 추가
    int translationUnitIndex = spAddTranslationUnit(slangRequest, SLANG_SOURCE_LANGUAGE_GLSL, nullptr);
    spAddTranslationUnitSourceString(
        slangRequest,
        translationUnitIndex,
        "shader.glsl",
        shaderCode
    );

    // 진입점 추가
    spAddEntryPoint(
        slangRequest,
        translationUnitIndex,
        "main",
        slangStage
    );

    // 컴파일 실행
    int compileResult = spCompile(slangRequest);
    if (compileResult != 0)
    {
        // 컴파일 오류 출력
        const char* diagnostics = spGetDiagnosticOutput(slangRequest);
        std::cerr << "Shader compilation failed: " << diagnostics << std::endl;
        spDestroyCompileRequest(slangRequest);
        session->release();
        slangSession->release();
        return false;
    }

    // SPIR-V 코드 추출
    size_t codeSize = 0;
    void const* spirvCode = spGetEntryPointCode(slangRequest, 0, &codeSize);
    if (!spirvCode || codeSize == 0)
    {
        std::cerr << "Failed to get compiled SPIR-V code" << std::endl;
        spDestroyCompileRequest(slangRequest);
        session->release();
        slangSession->release();
        return false;
    }

    // 결과를 출력 버퍼에 복사
    size_t wordCount = codeSize / sizeof(uint32_t);
    const uint32_t* spirvWords = reinterpret_cast<const uint32_t*>(spirvCode);
    spirvOut.resize(wordCount);
    memcpy(spirvOut.data(), spirvWords, codeSize);

    // 리소스 정리
    spDestroyCompileRequest(slangRequest);
    session->release();
    slangSession->release();

    return true;
}


OgImguiRenderer::OgImguiRenderer(Render::OgRenderContext* renderContext)
    : _renderContext(renderContext)
    //, _submitIndex(0)
{
    // Command Encoder 초기화
    //_encoders.resize(3); // Triple buffering 가정
    //for (auto& encoder : _encoders) {
    //    encoder = _renderContext->CreateCommandEncoder();
    //}

    // ImGui 렌더링을 위한 파이프라인 설정
    setupImGuiPipeline();
}

OgImguiRenderer::~OgImguiRenderer()
{
    //// Command Encoder 정리
    //for (auto encoder : _encoders) {
    //    _renderContext->DestroyCommandEncoder(encoder);
    //}
    //_encoders.clear();

    // 파이프라인 및 리소스 정리
    cleanupImGuiPipeline();
}

void OgImguiRenderer::RenderGUI(Render::OgCommandEncoderHandle* encoder, const OgRenderParam& param)
{
    if (!param.drawList || param.drawList->CmdListsCount == 0)
        return;

    // 버텍스 및 인덱스 버퍼 업데이트
    updateBuffers(param.drawList);

    // 현재 SwapChain의 FrameBuffer 얻기
    uint32 currentImageIndex = _renderContext->GetCurrentImageIndex(_currentSwapChain);
    auto frameBuffer = _renderContext->GetSwapChainFrameBuffer(_currentSwapChain, currentImageIndex);

    // 렌더 패스 시작
    OgCommandEncoderHandle::Area renderArea{
        0,                          // x
        0,                          // y
        (uint16)param.drawList->DisplaySize.x,  // width
        (uint16)param.drawList->DisplaySize.y   // height
    };

    // Clear values
    OgCommandEncoderHandle::ClearValue colorClear;
    colorClear.color.value[0] = 0.0f;
    colorClear.color.value[1] = 0.0f;
    colorClear.color.value[2] = 0.0f;
    colorClear.color.value[3] = 0.0f;

    OgCommandEncoderHandle::ClearValue depthClear;
    depthClear.depthStencil.depth = 1.0f;
    depthClear.depthStencil.stencil = 0;

    
    encoder->BeginRenderPass(
        _renderPass,           // renderPass
        frameBuffer,           // frameBuffer
        renderArea,           // area
        1,                    // colorAttachClearCount
        &colorClear,          // colorAttachmentClear
        0,                    // resolveAttachClearCount
        nullptr,              // resolveAttachmentClear
        &depthClear          // depthAttachmentClear
    );
    OgCommandEncoderHandle::Area area(0, 0, frameBuffer->width, frameBuffer->height);
    encoder->SetViewport(static_cast<float>(area.x), static_cast<float>(area.y), static_cast<float>(area.width), static_cast<float>(area.height));

    encoder->SetScissor(area.x, area.y, area.width, area.height);

    // Pipeline 바인딩 추가
    encoder->BindPipeline(_pipeline);

    if (pcmd->TextureId != nullptr)
    {
        Render::LvTextureHandle* tex = (Render::LvTextureHandle*)pcmd->TextureId;
        _resourceSet[0] = getGUITextureResourceSet(*guiRes, tex, res->uniformBufferHandles);
        encoder->BindResourceSets(_resourceSet, 0, _dynamicOffset);
        isTextureORFont = -1;
    }

    ++isTextureORFont;

    if (isTextureORFont == 1)
    {
        _resourceSet[0] = getGUITextureResourceSet(*guiRes, guiRes->fontTextureHandle, res->uniformBufferHandles);
        encoder->BindResourceSets(_resourceSet, 0, _dynamicOffset);
    }

    encoder->BindResourceSet(_resourceSet);

    // 버텍스/인덱스 버퍼 바인딩 추가
    uint32 vertexOffset = 0;
    encoder->BindVertexBuffers(&_vertexBuffer, &vertexOffset, 1);
    encoder->BindIndexBuffer(_indexBuffer, OgIndexType::UInt16);  // ImGui는 uint16 인덱스 사용

    // 드로우 커맨드 기록
    uint32_t indexBufferOffset = 0;
    for (int i = 0; i < param.drawList->CmdListsCount; i++) {
        const ImDrawList* cmd_list = param.drawList->CmdLists[i];
        for (int j = 0; j < cmd_list->CmdBuffer.Size; j++) {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];

            if (pcmd->UserCallback) {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else {
                // 시저 영역 설정
                encoder->SetScissor(
                    std::max((int32)pcmd->ClipRect.x, 0),      // x
                    std::max((int32)pcmd->ClipRect.y, 0),      // y
                    (uint32)(pcmd->ClipRect.z - pcmd->ClipRect.x),  // width
                    (uint32)(pcmd->ClipRect.w - pcmd->ClipRect.y)   // height
                );

                // DrawIndexed 수정된 버전
                // firstIndex: 인덱스 버퍼 offset (바이트 단위)
                // indexCount: 그릴 인덱스 개수
                // instanceCount: 인스턴스 개수 (기본값 1)
                // vertexOffset: 버텍스 오프셋
                encoder->DrawIndexed(
                    pcmd->IdxOffset * sizeof(ImDrawIdx),  // firstIndex (바이트 단위)
                    pcmd->ElemCount,                      // indexCount
                    1,                                    // instanceCount
                    pcmd->VtxOffset                      // vertexOffset
                );
            }
        }

        // 다음 드로우콜을 위한 오프셋 업데이트
        indexBufferOffset += cmd_list->IdxBuffer.Size;
    }

    // 렌더 패스 종료
    encoder->EndRenderPass();
}

void OgImguiRenderer::NextFrame(Render::OgCommandEncoderHandle* encoder, Render::OgSwapChain* swapChain)
{
    _currentSwapChain = swapChain;
    // 현재 인코더 제출
    _renderContext->Submit(swapChain, encoder);

    // 다음 프레임을 위한 인덱스 업데이트
    //_submitIndex = (_submitIndex + 1) % _encoders.size();
}

void OgImguiRenderer::setupImGuiPipeline()
{

    // 셰이더 코드 정의
    const char* vertexShaderCode = R"(
        #version 450
        layout(location = 0) in vec2 inPos;
        layout(location = 1) in vec2 inUV;
        layout(location = 2) in vec4 inColor;
        
        layout(binding = 0) uniform UniformBufferObject {
            mat4 projection;
        } ubo;
        
        layout(location = 0) out vec2 outUV;
        layout(location = 1) out vec4 outColor;
        
        void main() {
            gl_Position = ubo.projection * vec4(inPos.xy, 0.0, 1.0);
            outUV = inUV;
            outColor = inColor;
        }
    )";

    const char* fragmentShaderCode = R"(
        #version 450
        layout(location = 0) in vec2 inUV;
        layout(location = 1) in vec4 inColor;
        
        layout(binding = 1) uniform sampler2D fontSampler;
        
        layout(location = 0) out vec4 outColor;
        
        void main() {
            outColor = inColor * texture(fontSampler, inUV);
        }
    )";

    // GLSL을 SPIR-V로 컴파일
    std::vector<uint32_t> vertexSpirvCode;
    std::vector<uint32_t> fragmentSpirvCode;

    bool vertexCompileSuccess = compileGLSLtoSPIRV(
        vertexShaderCode,
        OgShaderType::VERTEX,
        vertexSpirvCode
    );

    bool fragmentCompileSuccess = compileGLSLtoSPIRV(
        fragmentShaderCode,
        OgShaderType::FRAGMENT,
        fragmentSpirvCode
    );

    if (!vertexCompileSuccess || !fragmentCompileSuccess) {
        std::cerr << "Failed to compile shaders to SPIR-V" << std::endl;
        return;
    }

    // 컴파일된 SPIR-V 코드로 셰이더 생성
    OgShaderHandle* vertexShader = _renderContext->CreateShader(
        OgShaderType::VERTEX,
        reinterpret_cast<const char*>(vertexSpirvCode.data()),
        vertexSpirvCode.size() * sizeof(uint32_t),
        "main"
    );

    OgShaderHandle* fragmentShader = _renderContext->CreateShader(
        OgShaderType::FRAGMENT,
        reinterpret_cast<const char*>(fragmentSpirvCode.data()),
        fragmentSpirvCode.size() * sizeof(uint32_t),
        "main"
    );

    // 프로그램 생성
    OgShaderHandle* shaders[2] = { vertexShader, fragmentShader };
    OgProgramHandle* program = _renderContext->CreateProgram(shaders, 2);

    // 리소스 레이아웃 설정
    OgResourceBinding bindings[2];

    // Uniform buffer binding (projection matrix)
    bindings[0].type = OgResourceType::UNIFORM_BUFFER;
    bindings[0].stage = OgShaderType::VERTEX;
    bindings[0].binding = 0;
    bindings[0].arrayCount = 0;
    bindings[0].name = "UniformBufferObject";

    // Texture sampler binding
    bindings[1].type = OgResourceType::COMBINED_IMAGE_SAMPLER;
    bindings[1].stage = OgShaderType::FRAGMENT;
    bindings[1].binding = 1;
    bindings[1].arrayCount = 0;
    bindings[1].name = "fontSampler";

    OgResourceLayoutHandle* resourceLayout = _renderContext->CreateResourceLayout(bindings, 2);

    OgPipelineDescriptor pipelineDesc;

    // 버텍스 입력 설정
    OgVertexBufferLayoutDescriptor vertexLayout(0, sizeof(ImGuiVertex));
    std::vector<OgVertexAttributeDescriptor> attributes = {
        OgVertexAttributeDescriptor(0, 0, OgVertexFormat::FLOAT2, offsetof(ImGuiVertex, pos)),    // position
        OgVertexAttributeDescriptor(0, 1, OgVertexFormat::FLOAT2, offsetof(ImGuiVertex, uv)),     // uv
        OgVertexAttributeDescriptor(0, 2, OgVertexFormat::BYTE4_NORM, offsetof(ImGuiVertex, col)) // color
    };

    // 버텍스 입력 descriptor 설정
    pipelineDesc.vertexInput.layouts = &vertexLayout;
    pipelineDesc.vertexInput.layoutCount = 1;
    pipelineDesc.vertexInput.attributes = attributes.data();
    pipelineDesc.vertexInput.attributeCount = attributes.size();

    // 렌더 패스 설정
    OgRenderPassInfo renderPassInfo;
    OgAttachment colorAttachment;
    colorAttachment.format = OgRenderTextureFormat::B8G8R8A8;
    renderPassInfo.outputColorAttachments = &colorAttachment;
    renderPassInfo.outputColorAttachments[0].format = OgRenderTextureFormat::B8G8R8A8;
    renderPassInfo.outputColorAttachments[0].load = OgRenderBufferLoadAction::LOAD;
    renderPassInfo.outputColorAttachments[0].store = OgRenderBufferStoreAction::STORE;
    renderPassInfo.outputColorAttachmentCount = 1;
    OgAttachment depthAttachment;
    depthAttachment.format = OgRenderTextureFormat::DEFAULT_DEPTH;
    depthAttachment.isDepthStencilAttachment = true;
    renderPassInfo.outputDepthStencilAttachment = depthAttachment;
    renderPassInfo.useDepthStencilAttachment = true;
    renderPassInfo.isSwapchainRenderPass = true;

    _renderPass = _renderContext->CreateRenderPass(renderPassInfo);

    // 파이프라인 설정
    pipelineDesc.type = OgPipelineType::GRAPHICS_PIPELINE;
    pipelineDesc.name = "ImGui Pipeline";
    pipelineDesc.renderPass = _renderPass;
    pipelineDesc.resourceLayout = resourceLayout;

    // 셰이더 설정
    pipelineDesc.shader.program = program;
    pipelineDesc.shader.shaders[0] = vertexShader;
    pipelineDesc.shader.shaders[1] = fragmentShader;
    pipelineDesc.shader.shaderCount = 2;

    // 블렌딩 설정
    pipelineDesc.colorBlend.attachments[0].blendEnable = true;
    pipelineDesc.colorBlend.attachments[0].srcColor = OgBlendFactor::SRC_ALPHA;
    pipelineDesc.colorBlend.attachments[0].dstColor = OgBlendFactor::ONE_MINUS_SRC_ALPHA;
    pipelineDesc.colorBlend.attachments[0].colorOp = OgBlendOp::ADD;
    pipelineDesc.colorBlend.attachments[0].srcAlpha = OgBlendFactor::ONE;
    pipelineDesc.colorBlend.attachments[0].dstAlpha = OgBlendFactor::ONE_MINUS_SRC_ALPHA;
    pipelineDesc.colorBlend.attachments[0].alphaOp = OgBlendOp::ADD;
    pipelineDesc.colorBlend.attachmentCount = 1;

    // 래스터라이저 설정
    pipelineDesc.rasterize.cullMode = OgCullMode::NONE;
    pipelineDesc.rasterize.frontFace = OgFrontFace::COUNTER_CLOCKWISE;
    pipelineDesc.rasterize.polygonMode = OgPolygonMode::FILL;
    pipelineDesc.rasterize.primitiveType = OgPrimitiveType::TRIANGLE_LIST;
    pipelineDesc.rasterize.scissorTest = true;

    // 뎁스 스텐실 설정
    pipelineDesc.depthStencil.depthTest = false;
    pipelineDesc.depthStencil.depthWrite = false;
    pipelineDesc.depthStencil.stencilTest = false;

    // 파이프라인 생성
    _pipeline = _renderContext->CreatePipeline(pipelineDesc);

    // 리소스 정리
    _renderContext->DestroyShader(vertexShader);
    _renderContext->DestroyShader(fragmentShader);

    // 버텍스 및 인덱스 버퍼 초기화
    const size_t initialVertexBufferSize = 10000 * sizeof(ImGuiVertex);  // 충분한 초기 크기 설정
    const size_t initialIndexBufferSize = 20000 * sizeof(ImDrawIdx);    // 충분한 초기 크기 설정

    // 버텍스 버퍼 생성
    _vertexBuffer = _renderContext->CreateBuffer(
        nullptr,                        // 초기 데이터 없음
        initialVertexBufferSize,        // 크기
        OgBufferUsage::VERTEX,          // 버퍼 용도
        OgMemoryOption::MAP_MANAGED     // 메모리 옵션
    );

    // 인덱스 버퍼 생성
    _indexBuffer = _renderContext->CreateBuffer(
        nullptr,                        // 초기 데이터 없음
        initialIndexBufferSize,         // 크기
        OgBufferUsage::INDEX,           // 버퍼 용도
        OgMemoryOption::MAP_MANAGED     // 메모리 옵션
    );

    // 리소스 레이아웃을 사용해 리소스 세트 생성
    OgResourceSetDescriptor resourceSetDesc;
    resourceSetDesc.layout = resourceLayout;

    // 유니폼 버퍼 리소스 설정
    // 여기서 projection matrix를 위한 버퍼 생성 및 설정 필요
    OgBufferHandle* uniformBuffer = _renderContext->CreateBuffer(
        nullptr,
        sizeof(glm::mat4),
        OgBufferUsage::UNIFORM,
        OgMemoryOption::MAP_MANAGED
    );

    // 폰트 텍스처 및 샘플러 설정 필요
    // ImGui 폰트 텍스처를 생성하고 업로드하는 코드 필요

    // 리소스 세트에 리소스 연결
    resourceSetDesc.resources[0].type = OgResourceType::UNIFORM_BUFFER;
    resourceSetDesc.resources[0].binding = 0;
    resourceSetDesc.resources[0].buffer = uniformBuffer;

    resourceSetDesc.resources[1].type = OgResourceType::COMBINED_IMAGE_SAMPLER;
    resourceSetDesc.resources[1].binding = 1;
    resourceSetDesc.resources[1].texture = fontTexture;
    resourceSetDesc.resources[1].sampler = fontSampler;

    resourceSetDesc.resourceCount = 2;

    // 리소스 세트 생성
    _resourceSet = _renderContext->CreateResourceSet(resourceSetDesc);
}

void OgImguiRenderer::cleanupImGuiPipeline()
{
    if (_pipeline) {
        _renderContext->DestroyPipeline(_pipeline);
        _pipeline = nullptr;
    }

    if (_renderPass) {
        _renderContext->DestroyRenderPass(_renderPass);
        _renderPass = nullptr;
    }
    // 버퍼 정리 추가
    if (_vertexBuffer) {
        _renderContext->DestroyBuffer(_vertexBuffer);
        _vertexBuffer = nullptr;
    }

    if (_indexBuffer) {
        _renderContext->DestroyBuffer(_indexBuffer);
        _indexBuffer = nullptr;
    }

    if (_resourceSet) {
        _renderContext->DestroyResourceSet(_resourceSet);
        _resourceSet = nullptr;
    }
}

void OgImguiRenderer::updateBuffers(const ImDrawData* drawData)
{
    // 버텍스 버퍼 업데이트
    const size_t vertexSize = drawData->TotalVtxCount * sizeof(ImGuiVertex);
    if (vertexSize > 0) {
        void* vertexDest = _renderContext->MapBuffer(_vertexBuffer, vertexSize);
        if (vertexDest) {
            for (int i = 0; i < drawData->CmdListsCount; i++) {
                const ImDrawList* cmdList = drawData->CmdLists[i];
                memcpy(vertexDest, cmdList->VtxBuffer.Data,
                    cmdList->VtxBuffer.Size * sizeof(ImGuiVertex));
                vertexDest = (char*)vertexDest +
                    cmdList->VtxBuffer.Size * sizeof(ImGuiVertex);
            }
            _renderContext->UnmapBuffer(_vertexBuffer);
        }
    }

    // 인덱스 버퍼 업데이트
    const size_t indexSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);
    if (indexSize > 0) {
        void* indexDest = _renderContext->MapBuffer(_indexBuffer, indexSize);
        if (indexDest) {
            for (int i = 0; i < drawData->CmdListsCount; i++) {
                const ImDrawList* cmdList = drawData->CmdLists[i];
                memcpy(indexDest, cmdList->IdxBuffer.Data,
                    cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
                indexDest = (char*)indexDest +
                    cmdList->IdxBuffer.Size * sizeof(ImDrawIdx);
            }
            _renderContext->UnmapBuffer(_indexBuffer);
        }
    }
}

OG_NAMESPACE_SAMPLE_END