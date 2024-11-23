#include "OgImguiRenderer.h"
#include "render/OgRenderDefinitions.h"
#include "imgui.h"

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

OgImguiRenderer::OgImguiRenderer(Render::OgRenderContext* renderContext)
    : _renderContext(renderContext)
    , _submitIndex(0)
{
    // Command Encoder 초기화
    _encoders.resize(3); // Triple buffering 가정
    for (auto& encoder : _encoders) {
        encoder = _renderContext->CreateCommandEncoder();
    }

    // ImGui 렌더링을 위한 파이프라인 설정
    setupImGuiPipeline();
}

OgImguiRenderer::~OgImguiRenderer()
{
    // Command Encoder 정리
    for (auto encoder : _encoders) {
        _renderContext->DestroyCommandEncoder(encoder);
    }
    _encoders.clear();

    // 파이프라인 및 리소스 정리
    cleanupImGuiPipeline();
}

void OgImguiRenderer::RenderGUI(const OgRenderParam& param)
{
    if (!param.drawList || param.drawList->CmdListsCount == 0)
        return;

    auto encoder = _encoders[_submitIndex];

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

    // Pipeline 바인딩 추가
    encoder->BindPipeline(_pipeline);

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

void OgImguiRenderer::NextFrame(Render::OgSwapChain* swapChain)
{
    _currentSwapChain = swapChain;
    // 현재 인코더 제출
    _renderContext->Submit(swapChain, _encoders[_submitIndex]);

    // 다음 프레임을 위한 인덱스 업데이트
    _submitIndex = (_submitIndex + 1) % _encoders.size();
}

void OgImguiRenderer::setupImGuiPipeline()
{
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
    renderPassInfo.outputColorAttachments = new OgAttachment[1];
    renderPassInfo.outputColorAttachments[0].format = OgRenderTextureFormat::DEFAULT_COLOR;
    renderPassInfo.outputColorAttachments[0].load = OgRenderBufferLoadAction::LOAD;
    renderPassInfo.outputColorAttachments[0].store = OgRenderBufferStoreAction::STORE;
    renderPassInfo.outputColorAttachmentCount = 1;
    renderPassInfo.isSwapchainRenderPass = true;

    _renderPass = _renderContext->CreateRenderPass(renderPassInfo);

    // 파이프라인 설정
    pipelineDesc.type = OgPipelineType::GRAPHICS_PIPELINE;
    pipelineDesc.name = "ImGui Pipeline";
    pipelineDesc.renderPass = _renderPass;

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

    // 파이프라인 생성
    _pipeline = _renderContext->CreatePipeline(pipelineDesc);
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