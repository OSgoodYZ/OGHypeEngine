#include "OgImguiRenderer.h"
#include "render/OgRenderDefinitions.h"
#include "imgui.h"
#include "slang.h"
#include <functional>

using namespace Og::Render;
OG_NAMESPACE_SAMPLE_BEGIN


// Slang을 사용하여 GLSL을 SPIR-V로 컴파일하는 함수
bool OgImguiRenderer::compileGLSLtoSPIRV(
    const char* shaderCode,
    OgShaderType shaderType,
    std::vector<uint32_t>& spirvOut)
{
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
    , _externalTexture(nullptr)
    , _externalTextureResourceSet(nullptr)
{
    // ImGui 렌더링을 위한 파이프라인 설정
    setupImGuiPipeline();
}

OgImguiRenderer::~OgImguiRenderer()
{
    // 파이프라인 및 리소스 정리
    cleanupImGuiPipeline();
    
    // 외부 텍스쳐 리소스셋 정리
    if (_externalTextureResourceSet)
    {
        _renderContext->DestroyResourceSet(_externalTextureResourceSet);
        _externalTextureResourceSet = nullptr;
    }
    
    // 외부 텍스쳐는 정리하지 않음 (OgTriangle에서 정리함)
    _externalTexture = nullptr;
}

void OgImguiRenderer::UpdateGPUContext(ImGuiContext* context)
{
    std::hash<ImGuiContext*> hash_fn;
    size_t hash_value = hash_fn(context);
    const bool exist = _guiContextResources.find(hash_value) != _guiContextResources.end();
    if (exist)
    {
        // 이미 존재하는 GUI Context 리소스 업데이트
        OgGUIContextResource& res = _guiContextResources[hash_value];

        // 기존 폰트 텍스처가 있으면 재사용
        // 중요: 폰트 텍스처는 Release 하지 않음 - 이 부분이 문제의 원인
        if (res.fontTextureHandle)
        {
            // 이미 유효한 텍스처가 있으면 그대로 사용
            // 텍스처를 업데이트할 필요가 없음
            // 단지 ImGui Context에 현재 텍스처 ID를 다시 설정
            context->IO.Fonts->SetTexID((ImTextureID)res.fontTextureHandle);
            return; // 기존 텍스처 사용 시 여기서 반환
        }

        // 폰트 데이터 가져오기 (기존 텍스처가 없는 경우에만 실행됨)
		unsigned char* fontData = nullptr;
        int texWidth, texHeight;
        context->IO.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);

        // 텍스처 정보 설정
        OgTextureInfo texInfo{};
        texInfo.type = OgTextureType::TEX_2D;
        texInfo.format = OgPixelFormat::R8G8B8A8_UNORM; // SRGB 대신 UNORM 형식 사용
        texInfo.extent.width = static_cast<uint16>(texWidth);
        texInfo.extent.height = static_cast<uint16>(texHeight);
        texInfo.byteSize = texWidth * texHeight * 4 * sizeof(char);
        texInfo.usage = OgTextureUsage::STAGING | OgTextureUsage::SAMPLED;
        texInfo.isGenerateMipmaps = false; // 명시적으로 밉맵 생성 비활성화

        // 새 폰트 텍스처 생성
        OgSamplerInfo samplerInfo{};
        samplerInfo.type = OgSamplerType::TEX_2D;
        samplerInfo.addressU = OgSamplerAddressMode::CLAMP_TO_EDGE;
        samplerInfo.addressV = OgSamplerAddressMode::CLAMP_TO_EDGE;
        samplerInfo.magFilter = OgFilter::LINEAR;
        samplerInfo.minFilter = OgFilter::LINEAR;
        samplerInfo.mipmapMode = OgSamplerMipmapMode::NEAREST;

        OgSamplerHandle* fontSampler = _renderContext->CreateSampler(samplerInfo);
        res.fontTextureHandle = _renderContext->CreateTexture((void*)fontData, texInfo.format, texInfo.extent.width, texInfo.extent.height, fontSampler);
        res.fontTextureHandle->Retain();
        res.fontTextureHandle->name = "ImGuiFont";
        // 폰트 텍스처가 생성된 후 ImGui에 세트
        context->IO.Fonts->SetTexID((ImTextureID)res.fontTextureHandle);
    }
    else
    {
        // 새 GUI Context 리소스 생성
        OgGUIContextResource res;

        // 폰트 데이터 및 텍스처 생성
        unsigned char* fontData;
        int texWidth, texHeight;
        context->IO.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);

        OgTextureInfo texInfo{};
        texInfo.type = OgTextureType::TEX_2D;
        texInfo.format = OgPixelFormat::R8G8B8A8_UNORM; // SRGB 대신 UNORM 형식 사용
        texInfo.extent.width = static_cast<uint16>(texWidth);
        texInfo.extent.height = static_cast<uint16>(texHeight);
        texInfo.byteSize = texWidth * texHeight * 4 * sizeof(char);
        texInfo.usage = OgTextureUsage::STAGING | OgTextureUsage::SAMPLED;
        texInfo.isGenerateMipmaps = false; // 명시적으로 밉맵 생성 비활성화

        OgSamplerInfo samplerInfo{};
        samplerInfo.type = OgSamplerType::TEX_2D;
        samplerInfo.addressU = OgSamplerAddressMode::CLAMP_TO_EDGE;
        samplerInfo.addressV = OgSamplerAddressMode::CLAMP_TO_EDGE;
        samplerInfo.magFilter = OgFilter::LINEAR;
        samplerInfo.minFilter = OgFilter::LINEAR;
        samplerInfo.mipmapMode = OgSamplerMipmapMode::NEAREST;

        OgSamplerHandle* fontSampler = _renderContext->CreateSampler(samplerInfo);
        res.fontTextureHandle = _renderContext->CreateTexture((void*)fontData, texInfo.format, texInfo.extent.width, texInfo.extent.height, fontSampler);
        res.fontTextureHandle->name = "ImGuiFont";
        res.fontTextureHandle->Retain();
        
        // 폰트 텍스처가 생성된 후 ImGui에 세트
        context->IO.Fonts->SetTexID((ImTextureID)res.fontTextureHandle);

        // 리소스 맵에 추가
        _guiContextResources[hash_value] = res;
    }



}

void OgImguiRenderer::RemoveGPUContext(ImGuiContext* context)
{
    std::hash<ImGuiContext*> hash_fn;
    size_t hash_value = hash_fn(context);
    auto it = _guiContextResources.find(hash_value);
    if (it != _guiContextResources.end()) {
        OgGUIContextResource& res = it->second;

        // ImGui의 폰트 텍스처 ID 재설정
        context->IO.Fonts->SetTexID(NULL);

        // 리소스 해제
        // 중요: 폰트 텍스처를 DestroyTexture 하기 전에 반드시 Release를 충분히 호출해야 함
        if (res.fontTextureHandle)
        {
            // 추가로 Release하여 레퍼런스 카운트가 0이 되도록 함
            // Retain()을 호출한 만큼 Release()도 호출해야 함
            res.fontTextureHandle->Release();
            
            // 이제 안전하게 텍스처 제거 가능
            _renderContext->DestroyTexture(res.fontTextureHandle);
            res.fontTextureHandle = nullptr;
        }
            
        // 맵에서 제거
        _guiContextResources.erase(it);
    }
}

void OgImguiRenderer::UpdateSurface(Render::OgSwapChain* swapchain)
{
    const bool exist = _surfaceResources.find(swapchain) != _surfaceResources.end();
    if (exist)
    {
        // 이미 존재하는 Surface 리소스 업데이트
        OgSurfaceResource& res = _surfaceResources[swapchain];

        // 현재 서브밋 인덱스 가져오기
        uint32 submitIndex = _renderContext->GetCurrentImageIndex(swapchain);
        if (submitIndex != _renderContext->SUBMISSION_INDEX_NONE)
        {
            res.frameBufferHandle = _renderContext->GetSwapChainFrameBuffer(swapchain, submitIndex);
        }
    }
    else
    {
        // 새 Surface 리소스 생성
        OgSurfaceResource res;

        // 버텍스 버퍼 생성
        for (uint32 i = 0; i < _renderContext->maxSubmitCount; ++i)
        {
            OgBufferHandle* vertex = _renderContext->CreateBuffer(nullptr, VERTEX_BUFFER_INITIAL_SIZE * sizeof(ImGuiVertex), OgBufferUsage::VERTEX, OgMemoryOption::MAP_MANAGED);
            vertex->name = "ImGui_VertexBuffer";
            res.vertexBufferHandles.push_back(vertex);
        }

        // 인덱스 버퍼 생성
        for (uint32 i = 0; i < _renderContext->maxSubmitCount; ++i)
        {
            OgBufferHandle* index = _renderContext->CreateBuffer(nullptr, INDEX_BUFFER_INITIAL_SIZE * sizeof(ImDrawIdx), OgBufferUsage::INDEX, OgMemoryOption::MAP_MANAGED);
            index->name = "ImGui_IndexBuffer";
            res.indexBufferHandles.push_back(index);
        }

        // 유니폼 버퍼 생성
        for (uint32 i = 0; i < 3; ++i) {
            res.uniformBufferHandles[i].resize(_renderContext->maxSubmitCount);
            for (uint32 j = 0; j < _renderContext->maxSubmitCount; ++j) {
                OgBufferHandle* uniform = _renderContext->CreateBuffer(nullptr, 64, OgBufferUsage::UNIFORM, OgMemoryOption::MAP_MANAGED);
                uniform->name = "ImGui_UniformBuffer";
                res.uniformBufferHandles[i][j] = uniform;
            }
        }

        // 렌더패스 생성
        OgAttachment color{};
        color.isDepthStencilAttachment = false;
        color.format = swapchain->colorRenderFormat;
        color.load = OgRenderBufferLoadAction::CLEAR;
        color.store = OgRenderBufferStoreAction::STORE;

        OgAttachment depth{};
        depth.isDepthStencilAttachment = true;
        depth.format = swapchain->depthRenderFormat;
        depth.load = OgRenderBufferLoadAction::CLEAR;
        depth.store = OgRenderBufferStoreAction::STORE;

        OgRenderPassInfo rpInfo{};
        rpInfo.isSwapchainRenderPass = true;
        rpInfo.useDepthStencilAttachment = true;
        rpInfo.outputColorAttachments = &color;
        rpInfo.outputColorAttachmentCount = 1;
        rpInfo.outputDepthStencilAttachment = depth;

        res.renderPassHandle = _renderContext->CreateRenderPass(rpInfo);
        res.renderPassHandle->name = "ImGui_RenderPass";

        // 현재 서브밋 인덱스 가져오기
        uint32 submitIndex = _renderContext->GetCurrentImageIndex(swapchain);
        if (submitIndex != _renderContext->SUBMISSION_INDEX_NONE)
        {
            res.frameBufferHandle = _renderContext->GetSwapChainFrameBuffer(swapchain, submitIndex);
        }

        // 리소스 맵에 추가
        _surfaceResources[swapchain] = res;
    }
}

void OgImguiRenderer::RemoveSurface(Render::OgSwapChain* swapchain)
{
    auto it = _surfaceResources.find(swapchain);
    if (it != _surfaceResources.end()) {
        OgSurfaceResource& res = it->second;

        // 버텍스 버퍼 해제
        for (auto& buffer : res.vertexBufferHandles) {
            if (buffer) _renderContext->DestroyBuffer(buffer);
        }

        // 인덱스 버퍼 해제
        for (auto& buffer : res.indexBufferHandles) {
            if (buffer) _renderContext->DestroyBuffer(buffer);
        }

        // 유니폼 버퍼 해제
        for (int i = 0; i < 3; i++) {
            for (auto& buffer : res.uniformBufferHandles[i]) {
                if (buffer) _renderContext->DestroyBuffer(buffer);
            }
        }

        // 렌더패스 해제
        if (res.renderPassHandle)
            _renderContext->DestroyRenderPass(res.renderPassHandle);

        // 맵에서 제거
        _surfaceResources.erase(it);
    }
}

void OgImguiRenderer::RenderGUI(Render::OgCommandEncoderHandle* encoder, const OgRenderParam& param)
{
    if (!param.drawList || param.drawList->CmdListsCount == 0)
        return;

    const ImDrawData* drawData = param.drawList;

    // 현재 SwapChain 업데이트
    _currentSwapChain = const_cast<OgSwapChain*>(param.surface);

    // 현재 Surface 리소스 가져오기
    auto surfaceIt = _surfaceResources.find(_currentSwapChain);
    if (surfaceIt == _surfaceResources.end()) {
        return; // Surface 리소스가 없음
    }

    OgSurfaceResource& surfaceRes = surfaceIt->second;

    // 현재 ImGui Context 리소스 가져오기
    size_t contextHash = param.guiContextKey;
    auto contextIt = _guiContextResources.find(contextHash);
    if (contextIt == _guiContextResources.end()) {
        return; // GUI Context 리소스가 없음
    }

    OgGUIContextResource& contextRes = contextIt->second;
    
    // 외부 텍스쳐가 있는 경우, ImGui에 이미지로 사용하도록 추가
    if (_externalTexture && drawData->CmdListsCount > 0) {
        // ImGui의 인터페이스에 삼각형 렌더 타겟을 표시하는 코드 추가
        ImGui::Begin("Triangle Render Target", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        
        // 이미지 크기 계산
        ImVec2 imageSize;
        
        imageSize.x = static_cast<float>(_externalTexture->info.extent.width);
        imageSize.y = static_cast<float>(_externalTexture->info.extent.height);
        
        // 창 크기에 맞게 이미지 사이즈 조정
        const float maxSize = 500.0f;
        if (imageSize.x > maxSize || imageSize.y > maxSize) {
            float ratio = imageSize.y / imageSize.x;
            if (imageSize.x > imageSize.y) {
                imageSize.x = maxSize;
                imageSize.y = maxSize * ratio;
            } else {
                imageSize.y = maxSize;
                imageSize.x = maxSize / ratio;
            }
        }
        
        // ImGui에 이미지로 텍스쳐 표시
        ImGui::Image((ImTextureID)_externalTexture, imageSize);
        ImGui::End();
        
        // 변경된 ImGui 커맨드 리스트로 DrawData 업데이트
        ImGui::Render();
        drawData = ImGui::GetDrawData();
    }

    // 현재 이미지 인덱스 가져오기
    uint32 imageIndex = _renderContext->GetCurrentImageIndex(_currentSwapChain);
    imageIndex = imageIndex % _renderContext->maxSubmitCount;
    if (imageIndex == _renderContext->SUBMISSION_INDEX_NONE) {
        return; // 유효한 이미지 인덱스가 없음
    }

    // 버퍼 업데이트
    updateBuffers(drawData);

    // 렌더 패스 시작
    OgCommandEncoderHandle::Area renderArea{
        0,                              // x
        0,                              // y
        (uint16)drawData->DisplaySize.x,  // width
        (uint16)drawData->DisplaySize.y   // height
    };

    // Clear values
    OgCommandEncoderHandle::ClearValue colorClear{};
    colorClear.color.value[0] = 0.0f;
    colorClear.color.value[1] = 0.0f;
    colorClear.color.value[2] = 0.0f;
    colorClear.color.value[3] = 0.0f;

    OgCommandEncoderHandle::ClearValue depthClear{};
    depthClear.depthStencil.depth = 1.0f;
    depthClear.depthStencil.stencil = 0;

	
    encoder->BeginRenderPass(
        surfaceRes.renderPassHandle,
        surfaceRes.frameBufferHandle,
        renderArea,
        1,
        &colorClear,
        0,
        nullptr,
        &depthClear
    );

    if (drawData && drawData->TotalVtxCount > 0 && drawData->CmdListsCount > 0)
    {
        // 뷰포트 및 시저 설정
        encoder->SetViewport(0.0f, 0.0f, drawData->DisplaySize.x, drawData->DisplaySize.y);
        encoder->SetScissor(0, 0, (uint32)drawData->DisplaySize.x, (uint32)drawData->DisplaySize.y);

        // 파이프라인 바인딩
        encoder->BindPipeline(_pipeline);

        // 프로젝션 매트릭스 계산 및 유니폼 버퍼 업데이트
        float L = drawData->DisplayPos.x;
        float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
        float T = drawData->DisplayPos.y;
        float B = drawData->DisplayPos.y + drawData->DisplaySize.y;

        // 상하만 뒤집힌 정사영 프로젝션 매트릭스
        float projection[4][4] = {
            { 2.0f / (R - L),   0.0f,         0.0f,   0.0f },
            { 0.0f,         -2.0f / (T - B),   0.0f,   0.0f },
            { 0.0f,         0.0f,        -1.0f,   0.0f },
            { (R + L) / (L - R),  (T + B) / (T - B),  0.0f,   1.0f },
        };

        OgBufferHandle* uniformBuffer = surfaceRes.uniformBufferHandles[0][imageIndex];
        void* uniformData = _renderContext->MapBuffer(uniformBuffer, sizeof(projection));
        if (uniformData) {
            memcpy(uniformData, projection, sizeof(projection));
            _renderContext->UnmapBuffer(uniformBuffer);
        }

        // 리소스 세트 바인딩
		Render::OgResourceSetHandle* resourceSet = getResourceSet(contextRes.fontTextureHandle, surfaceRes.uniformBufferHandles[0][imageIndex]);
        encoder->BindResourceSet(resourceSet);

        // 버텍스/인덱스 버퍼 바인딩
        OgBufferHandle* vertexBuffer = surfaceRes.vertexBufferHandles[imageIndex];
        uint32 vertexOffset = 0;
        encoder->BindVertexBuffers(&vertexBuffer, &vertexOffset, 1);

        OgBufferHandle* indexBuffer = surfaceRes.indexBufferHandles[imageIndex];
        encoder->BindIndexBuffer(indexBuffer, sizeof(ImDrawIdx) == 2 ? OgIndexType::UInt16 : OgIndexType::UInt32);

        // 드로우 커맨드 처리
        int indexOffset = 0;
        //int vertexOffset = 0;

        for (int n = 0; n < drawData->CmdListsCount; n++)
        {
            const ImDrawList* cmdList = drawData->CmdLists[n];

            for (int cmd_i = 0; cmd_i < cmdList->CmdBuffer.Size; cmd_i++)
            {
                const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmd_i];

                if (pcmd->UserCallback)
                {
                    pcmd->UserCallback(cmdList, pcmd);
                }
                else
                {
                    // 시저 설정
                    encoder->SetScissor(
                        (int32)pcmd->ClipRect.x,
                        (int32)pcmd->ClipRect.y,
                        (uint32)(pcmd->ClipRect.z - pcmd->ClipRect.x),
                        (uint32)(pcmd->ClipRect.w - pcmd->ClipRect.y)
                    );

                    // 드로우 인덱스
                    encoder->DrawIndexed(
                        (pcmd->IdxOffset + indexOffset) * sizeof(ImDrawIdx),
                        pcmd->ElemCount,
                        1,
                        pcmd->VtxOffset + vertexOffset
                    );
                }
            }

            indexOffset += cmdList->IdxBuffer.Size;
            vertexOffset += cmdList->VtxBuffer.Size;
        }
    }

    // 렌더 패스 종료
    encoder->EndRenderPass();
}

void OgImguiRenderer::NextFrame(Render::OgCommandEncoderHandle* encoder, Render::OgSwapChain* swapChain)
{
    _currentSwapChain = swapChain;

    // 커맨드 인코더 제출
    _renderContext->Submit(swapChain, encoder);
}

void OgImguiRenderer::SetExternalTexture(Render::OgTextureHandle* texture)
{
    // 기존 리소스 세트가 있으면 먼저 정리
    if (_externalTextureResourceSet) {
        _renderContext->DestroyResourceSet(_externalTextureResourceSet);
        _externalTextureResourceSet = nullptr;
    }
    
    _externalTexture = texture;
    
    // 텍스쳐가 유효하지 않으면 더 이상 진행하지 않음
    if (!_externalTexture) return;
    
    // 외부 텍스쳐를 위한 리소스 세트 생성
    OgResourceUsage resourceUsages[1]{};

    // 텍스쳐 바인딩
    resourceUsages[0].binding.type = OgResourceType::COMBINED_IMAGE_SAMPLER;
    resourceUsages[0].binding.stage = OgShaderType::FRAGMENT;
    resourceUsages[0].binding.binding = 1; // ImGui 폰트 텍스쳐와 같은 바인딩 사용
    resourceUsages[0].binding.arrayCount = 0;
    resourceUsages[0].binding.name = "externalTextureSampler";
    resourceUsages[0].texture.handle = &_externalTexture;

    // 리소스 세트 생성
    _externalTextureResourceSet = _renderContext->CreateResourceSet(_resourceLayout, resourceUsages, 1);
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
            vec4 texColor = texture(fontSampler, inUV);
            outColor = inColor * texColor;
            // 투명도 처리를 위한 특별 처리
            if(texColor.a <= 0.0)
                discard;
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

    _resourceLayout = _renderContext->CreateResourceLayout(bindings, 2);

    // 샘플러 설정
    OgSamplerInfo samplerInfo{};
    samplerInfo.type = OgSamplerType::TEX_2D;
    samplerInfo.addressU = OgSamplerAddressMode::CLAMP_TO_EDGE; // REPEAT 대신 CLAMP_TO_EDGE 사용
    samplerInfo.addressV = OgSamplerAddressMode::CLAMP_TO_EDGE; // REPEAT 대신 CLAMP_TO_EDGE 사용
    samplerInfo.magFilter = OgFilter::LINEAR;
    samplerInfo.minFilter = OgFilter::LINEAR;
    samplerInfo.mipmapMode = OgSamplerMipmapMode::NEAREST; // 명시적으로 밉맵 모드 설정

    _fontSampler = _renderContext->CreateSampler(samplerInfo);

    // 렌더 패스 설정
    OgRenderPassInfo renderPassInfo{};
    OgAttachment colorAttachment{};
    colorAttachment.format = OgRenderTextureFormat::B8G8R8A8;
    colorAttachment.load = OgRenderBufferLoadAction::CLEAR;
    colorAttachment.store = OgRenderBufferStoreAction::STORE;

    renderPassInfo.outputColorAttachments = &colorAttachment;
    renderPassInfo.outputColorAttachmentCount = 1;

    OgAttachment depthAttachment{};
    depthAttachment.format = OgRenderTextureFormat::DEFAULT_DEPTH;
    depthAttachment.isDepthStencilAttachment = true;
    depthAttachment.load = OgRenderBufferLoadAction::CLEAR;
    depthAttachment.store = OgRenderBufferStoreAction::STORE;

    renderPassInfo.outputDepthStencilAttachment = depthAttachment;
    renderPassInfo.useDepthStencilAttachment = true;
    renderPassInfo.isSwapchainRenderPass = true;




    _renderPass = _renderContext->CreateRenderPass(renderPassInfo);

    // 파이프라인 설정
    OgPipelineDescriptor pipelineDesc{};

    // 버텍스 입력 설정
    OgVertexBufferLayoutDescriptor vertexLayout(0, sizeof(ImGuiVertex));
    OgVertexAttributeDescriptor attributes[3] = {
        OgVertexAttributeDescriptor(0, 0, OgVertexFormat::FLOAT2, offsetof(ImGuiVertex, pos)),    // position
        OgVertexAttributeDescriptor(0, 1, OgVertexFormat::FLOAT2, offsetof(ImGuiVertex, uv)),     // uv
        OgVertexAttributeDescriptor(0, 2, OgVertexFormat::BYTE4_NORM, offsetof(ImGuiVertex, col)) // color
    };

    // 버텍스 입력 descriptor 설정
    pipelineDesc.vertexInput.layouts = &vertexLayout;
    pipelineDesc.vertexInput.layoutCount = 1;
    pipelineDesc.vertexInput.attributes = attributes;
    pipelineDesc.vertexInput.attributeCount = 3;

    pipelineDesc.type = OgPipelineType::GRAPHICS_PIPELINE;
    pipelineDesc.name = "ImGui Pipeline";
    pipelineDesc.renderPass = _renderPass;
    pipelineDesc.resourceLayout = _resourceLayout;

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

    // 버텍스 및 인덱스 버퍼 초기화
    const size_t initialVertexBufferSize = VERTEX_BUFFER_INITIAL_SIZE * sizeof(ImGuiVertex);
    const size_t initialIndexBufferSize = INDEX_BUFFER_INITIAL_SIZE * sizeof(ImDrawIdx);

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

    // 유니폼 버퍼 생성
    _uniformBuffer = _renderContext->CreateBuffer(
        nullptr,                        // 초기 데이터 없음
        64,                             // 크기 (4x4 매트릭스)
        OgBufferUsage::UNIFORM,         // 버퍼 용도
        OgMemoryOption::MAP_MANAGED     // 메모리 옵션
    );

    // 초기 폰트 텍스처를 위한 더미 텍스처 생성
    OgTextureInfo texInfo{};
    texInfo.type = OgTextureType::TEX_2D;
    texInfo.format = OgPixelFormat::R8G8B8A8_UNORM; // SRGB 대신 UNORM 사용
    texInfo.extent.width = 1;
    texInfo.extent.height = 1;
    texInfo.usage = OgTextureUsage::SAMPLED | OgTextureUsage::STAGING; // 스테이징 플래그 추가
    texInfo.isGenerateMipmaps = false; // 밉맵 비활성화

    uint32_t whitePixel = 0xFFFFFFFF;
    OgTextureHandle* dummyTexture = _renderContext->CreateTexture((void*)&whitePixel, texInfo.format, texInfo.extent.width, texInfo.extent.height, _fontSampler);
    dummyTexture->Retain(); // 레퍼런스 카운트 증가

    // 리소스 세트 생성
    OgResourceUsage resourceUsages[2]{};

    static uint32 tempUniformSize = 64;
    static uint32 tempUniformOffset = 0;

    // 유니폼 버퍼
    resourceUsages[0].binding = bindings[0];
    resourceUsages[0].buffer.handle = &_uniformBuffer;
    resourceUsages[0].buffer.offset = &tempUniformOffset;
    resourceUsages[0].buffer.range = &tempUniformSize;  // 매트릭스 크기

    // 폰트 텍스처
    resourceUsages[1].binding = bindings[1];
    resourceUsages[1].texture.handle = &dummyTexture;

    _resourceSet = _renderContext->CreateResourceSet(_resourceLayout, resourceUsages, 2);

    // 리소스 정리
    // dummyTexture를 사용한 후 Release를 해주어야 함
    dummyTexture->Release();
    // 참조 카운트가 0이 되는지 확인 후 삭제
    _renderContext->DestroyTexture(dummyTexture);
}

void OgImguiRenderer::cleanupImGuiPipeline()
{
    // 파이프라인 리소스 해제
    if (_pipeline) {
        _renderContext->DestroyPipeline(_pipeline);
        _pipeline = nullptr;
    }

    if (_renderPass) {
        _renderContext->DestroyRenderPass(_renderPass);
        _renderPass = nullptr;
    }

    if (_resourceLayout) {
        _renderContext->DestroyResourceLayout(_resourceLayout);
        _resourceLayout = nullptr;
    }

    if (_resourceSet) {
        _renderContext->DestroyResourceSet(_resourceSet);
        _resourceSet = nullptr;
    }

    // 버퍼 정리
    if (_vertexBuffer) {
        _renderContext->DestroyBuffer(_vertexBuffer);
        _vertexBuffer = nullptr;
    }

    if (_indexBuffer) {
        _renderContext->DestroyBuffer(_indexBuffer);
        _indexBuffer = nullptr;
    }

    if (_uniformBuffer) {
        _renderContext->DestroyBuffer(_uniformBuffer);
        _uniformBuffer = nullptr;
    }

    // 샘플러 정리
    if (_fontSampler) {
        _renderContext->DestroySampler(_fontSampler);
        _fontSampler = nullptr;
    }
}

void OgImguiRenderer::updateBuffers(const ImDrawData* drawData)
{
    if (!drawData || drawData->TotalVtxCount == 0 || drawData->TotalIdxCount == 0)
        return;

    // 현재 SwapChain 및 이미지 인덱스 가져오기
    if (!_currentSwapChain)
        return;

    uint32 imageIndex = _renderContext->GetCurrentImageIndex(_currentSwapChain);
    imageIndex = imageIndex % _renderContext->maxSubmitCount;
    if (imageIndex == _renderContext->SUBMISSION_INDEX_NONE)
        return;

    // Surface 리소스 가져오기
    auto surfaceIt = _surfaceResources.find(_currentSwapChain);
    if (surfaceIt == _surfaceResources.end())
        return;

    OgSurfaceResource& surfaceRes = surfaceIt->second;

    // 버퍼 크기 확인 및 필요시 재할당
    size_t vertexBufferSize = drawData->TotalVtxCount * sizeof(ImGuiVertex);
    size_t indexBufferSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);

    OgBufferHandle* vertexBuffer = surfaceRes.vertexBufferHandles[imageIndex];
    OgBufferHandle* indexBuffer = surfaceRes.indexBufferHandles[imageIndex];

    // 필요시 버퍼 재할당 (여기서는 간단히 처리, 실제로는 더 복잡할 수 있음)
    if (vertexBuffer->size < vertexBufferSize) {
        _renderContext->DestroyBuffer(vertexBuffer);
        vertexBuffer = _renderContext->CreateBuffer(nullptr, vertexBufferSize * 2, OgBufferUsage::VERTEX, OgMemoryOption::MAP_MANAGED);
        vertexBuffer->name = "ImGui_VertexBuffer";
        surfaceRes.vertexBufferHandles[imageIndex] = vertexBuffer;
    }

    if (indexBuffer->size < indexBufferSize) {
        _renderContext->DestroyBuffer(indexBuffer);
        indexBuffer = _renderContext->CreateBuffer(nullptr, indexBufferSize * 2, OgBufferUsage::INDEX, OgMemoryOption::MAP_MANAGED);
        indexBuffer->name = "ImGui_IndexBuffer";
        surfaceRes.indexBufferHandles[imageIndex] = indexBuffer;
    }

    // 버텍스 버퍼 업데이트
    ImGuiVertex* vertexDest = static_cast<ImGuiVertex*>(_renderContext->MapBuffer(vertexBuffer, vertexBufferSize));
    if (vertexDest) {
        for (int n = 0; n < drawData->CmdListsCount; n++) {
            const ImDrawList* cmdList = drawData->CmdLists[n];
            memcpy(vertexDest, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImGuiVertex));
            vertexDest += cmdList->VtxBuffer.Size;
        }
        _renderContext->UnmapBuffer(vertexBuffer);
    }

    // 인덱스 버퍼 업데이트
    ImDrawIdx* indexDest = static_cast<ImDrawIdx*>(_renderContext->MapBuffer(indexBuffer, indexBufferSize));
    if (indexDest) {
        for (int n = 0; n < drawData->CmdListsCount; n++) {
            const ImDrawList* cmdList = drawData->CmdLists[n];
            memcpy(indexDest, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
            indexDest += cmdList->IdxBuffer.Size;
        }
        _renderContext->UnmapBuffer(indexBuffer);
    }
}
Render::OgResourceSetHandle* OgImguiRenderer::getResourceSet(Render::OgTextureHandle* texture, Render::OgBufferHandle* uniform)
{
    // 텍스처가 유효한지 확인
    if (!texture) 
    {
        LOGE(OG_ID, "Invalid texture handle for ImGui resource set!");
        return nullptr;
    }

    // 텍스처와 유니폼 버퍼의 포인터를 합쳐서 해시값 생성
    size_t textureAddr = reinterpret_cast<size_t>(texture);
    size_t uniformAddr = reinterpret_cast<size_t>(uniform);
    size_t combinedHash = textureAddr ^ (uniformAddr << 1);

    // 캐시에서 기존 리소스 세트 검색
    auto it = _guiResourceSetHandleMap.find(combinedHash);
    if (it != _guiResourceSetHandleMap.end()) 
    {
        return it->second;
    }

    // 리소스 세트가 없으면 새로 생성
    OgResourceUsage resourceUsages[2]{};

    static uint32 tempUniformSize = 64; // 4x4 매트릭스 크기
    static uint32 tempUniformOffset = 0;

    // 유니폼 버퍼 설정
    resourceUsages[0].binding.type = OgResourceType::UNIFORM_BUFFER;
    resourceUsages[0].binding.stage = OgShaderType::VERTEX;
    resourceUsages[0].binding.binding = 0;
    resourceUsages[0].binding.arrayCount = 0;
    resourceUsages[0].binding.name = "UniformBufferObject";
    resourceUsages[0].buffer.handle = &uniform;
    resourceUsages[0].buffer.offset = &tempUniformOffset;
    resourceUsages[0].buffer.range = &tempUniformSize;

    // 텍스처 설정
    resourceUsages[1].binding.type = OgResourceType::COMBINED_IMAGE_SAMPLER;
    resourceUsages[1].binding.stage = OgShaderType::FRAGMENT;
    resourceUsages[1].binding.binding = 1;
    resourceUsages[1].binding.arrayCount = 0;
    resourceUsages[1].binding.name = "fontSampler";
    resourceUsages[1].texture.handle = &texture;

    // 리소스 세트 생성
    OgResourceSetHandle* resourceSet = _renderContext->CreateResourceSet(_resourceLayout, resourceUsages, 2);

    // 캐시에 저장
    _guiResourceSetHandleMap[combinedHash] = resourceSet;

    return resourceSet;
}

OG_NAMESPACE_SAMPLE_END