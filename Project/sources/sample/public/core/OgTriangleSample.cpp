#include "OgTriangleSample.h"

using namespace std;
using namespace Render;

OG_NAMESPACE_SAMPLE_BEGIN

OgTriangleSample::OgTriangleSample(Render::OgRenderContext* renderContext)
    : OgSampleBase(renderContext)
{
}

OgTriangleSample::~OgTriangleSample()
{
    if (_isInitialized)
    {
        OnDestroy();
    }
}

void OgTriangleSample::OnInit(Render::OgSwapChain* swapchain)
{
    if (_isInitialized)
        return;

    // 스왑체인의 크기로 리소스 생성
    const uint16 width = _renderContext->GetSwapChainFrameBuffer(swapchain, 0)->width;
    const uint16 height = _renderContext->GetSwapChainFrameBuffer(swapchain, 0)->height;
    
    createResources(width, height);
    
    _isInitialized = true;
}

void OgTriangleSample::OnDestroy()
{
    if (!_isInitialized)
        return;
        
    destroyResources();
    _isInitialized = false;
}

void OgTriangleSample::OnRender(Render::OgCommandEncoderHandle* encoder, Render::OgSwapChain* swapchain)
{
    if (!_isInitialized || !_renderTargetFrameBuffer)
        return;

    float passColor[]{ 0.0f, 1.0f, 0.0f, 1.0f };

    OgCommandEncoderHandle::ClearValue colorClear;
    colorClear.color.value[0] = 0.0f;
    colorClear.color.value[1] = 0.0f;
    colorClear.color.value[2] = 0.0f;
    colorClear.color.value[3] = 1.0f;

    OgCommandEncoderHandle::ClearValue depthStencilClear;
    depthStencilClear.depthStencil.depth = 1.f;
    depthStencilClear.depthStencil.stencil = 0.f;

    // 렌더 타겟 프레임버퍼 사용
    OgCommandEncoderHandle::Area area(0, 0, _renderTargetWidth, _renderTargetHeight);

    encoder->BeginDebugMarker("Sample - TriangleRenderTarget", passColor);

    // 렌더 타겟에 렌더링
    encoder->BeginRenderPass(_renderTargetRenderPass, _renderTargetFrameBuffer, area, 1, &colorClear, 0, nullptr, &depthStencilClear);

    encoder->SetViewport(static_cast<float>(area.x), static_cast<float>(area.y), 
                        static_cast<float>(area.width), static_cast<float>(area.height));

    encoder->SetScissor(area.x, area.y, area.width, area.height);

    encoder->BindPipeline(_pipeline);

    encoder->BindVertexBuffers(&_vertexBuffer, 0, 1);

    encoder->DrawArrays(0, 3, 1);

    encoder->EndRenderPass();

    encoder->EndDebugMarker();
}

void OgTriangleSample::OnSuspend(Render::OgSwapChain* swapchain)
{
    _renderContext->Suspend(swapchain);
}

void OgTriangleSample::OnRestore(Render::OgSwapChain* swapchain)
{
    _renderContext->Restore(swapchain);
}

void OgTriangleSample::OnResize(uint32 width, uint32 height)
{
    if (!_isInitialized)
        return;
        
    // 크기가 변경되면 렌더 타겟 재생성
    destroyRenderTarget();
    createRenderTarget(static_cast<uint16>(width), static_cast<uint16>(height));
}

void OgTriangleSample::createResources(uint16 width, uint16 height)
{
    // 버텍스 버퍼 생성
    _vertexBuffer = _renderContext->CreateBuffer(
        const_cast<float*>(TRIANGLE_VERTICES), 
        sizeof(TRIANGLE_VERTICES), 
        Render::OgBufferUsage::VERTEX, 
        OgMemoryOption::MAP_MANAGED
    );
    _vertexBuffer->Retain();
    
    // 셰이더 생성
    createShaders();
    
    // 리소스 레이아웃 생성
    _resourceLayout = _renderContext->CreateResourceLayout(nullptr, 0);
    _resourceLayout->name = "TriangleSampleResourceLayout";
    _resourceLayout->Retain();
    
    // 렌더 타겟 생성
    createRenderTarget(width, height);
    
    // 파이프라인 생성
    createPipeline();
}

void OgTriangleSample::destroyResources()
{
    _renderContext->WaitDeviceIdle();
    
    if (_pipeline)
    {
        _pipeline->Release();
        _pipeline = nullptr;
    }
    
    destroyRenderTarget();
    
    if (_resourceLayout)
    {
        _resourceLayout->Release();
    	_resourceLayout = nullptr;
    }
    
    if (_program)
    {
        _program->Release();
        _program = nullptr;
    }
    
    if (_fragmentShader)
    {
        _fragmentShader->Release();
        _fragmentShader = nullptr;
    }
    
    if (_vertexShader)
    {
        _vertexShader->Release();
        _vertexShader = nullptr;
    }
    
    if (_vertexBuffer)
    {
        _vertexBuffer->Release();
        _vertexBuffer = nullptr;
    }
}

void OgTriangleSample::createShaders()
{
    // 셰이더 코드 (기존과 동일)
    uint32 vs[]{
        0x03022307, 0x00000100, 0x0A000D00, 0x18000000, 0x00000000, 0x11000200, 0x01000000, 0x0B000600,
        0x01000000, 0x474C534C, 0x2E737464, 0x2E343530, 0x00000000, 0x0E000300, 0x00000000, 0x01000000,
        0x0F000700, 0x00000000, 0x04000000, 0x6D61696E, 0x00000000, 0x0A000000, 0x0F000000, 0x03000300,
        0x01000000, 0x36010000, 0x04000A00, 0x474C5F47, 0x4F4F474C, 0x455F6370, 0x705F7374, 0x796C655F,
        0x6C696E65, 0x5F646972, 0x65637469, 0x76650000, 0x04000800, 0x474C5F47, 0x4F4F474C, 0x455F696E,
        0x636C7564, 0x655F6469, 0x72656374, 0x69766500, 0x05000400, 0x04000000, 0x6D61696E, 0x00000000,
        0x05000600, 0x08000000, 0x676C5F50, 0x65725665, 0x72746578, 0x00000000, 0x06000600, 0x08000000,
        0x00000000, 0x676C5F50, 0x6F736974, 0x696F6E00, 0x06000700, 0x08000000, 0x01000000, 0x676C5F50,
        0x6F696E74, 0x53697A65, 0x00000000, 0x05000300, 0x0A000000, 0x00000000, 0x05000500, 0x0F000000,
        0x615F506F, 0x73697469, 0x6F6E0000, 0x48000500, 0x08000000, 0x00000000, 0x0B000000, 0x00000000,
        0x48000500, 0x08000000, 0x01000000, 0x0B000000, 0x01000000, 0x47000300, 0x08000000, 0x02000000,
        0x47000400, 0x0F000000, 0x1E000000, 0x00000000, 0x13000200, 0x02000000, 0x21000300, 0x03000000,
        0x02000000, 0x16000300, 0x06000000, 0x20000000, 0x17000400, 0x07000000, 0x06000000, 0x04000000,
        0x1E000400, 0x08000000, 0x07000000, 0x06000000, 0x20000400, 0x09000000, 0x03000000, 0x08000000,
        0x3B000400, 0x09000000, 0x0A000000, 0x03000000, 0x15000400, 0x0B000000, 0x20000000, 0x01000000,
        0x2B000400, 0x0B000000, 0x0C000000, 0x00000000, 0x17000400, 0x0D000000, 0x06000000, 0x03000000,
        0x20000400, 0x0E000000, 0x01000000, 0x0D000000, 0x3B000400, 0x0E000000, 0x0F000000, 0x01000000,
        0x2B000400, 0x06000000, 0x11000000, 0x0000803F, 0x20000400, 0x16000000, 0x03000000, 0x07000000,
        0x36000500, 0x02000000, 0x04000000, 0x00000000, 0x03000000, 0xF8000200, 0x05000000, 0x3D000400,
        0x0D000000, 0x10000000, 0x0F000000, 0x51000500, 0x06000000, 0x12000000, 0x10000000, 0x00000000,
        0x51000500, 0x06000000, 0x13000000, 0x10000000, 0x01000000, 0x51000500, 0x06000000, 0x14000000,
        0x10000000, 0x02000000, 0x50000700, 0x07000000, 0x15000000, 0x12000000, 0x13000000, 0x14000000,
        0x11000000, 0x41000500, 0x16000000, 0x17000000, 0x0A000000, 0x0C000000, 0x3E000300, 0x17000000,
        0x15000000, 0xFD000100, 0x38000100
    };

    uint32 fs[]{
        0x03022307, 0x00000100, 0x0A000D00, 0x0E000000, 0x00000000, 0x11000200, 0x01000000, 0x0B000600,
        0x01000000, 0x474C534C, 0x2E737464, 0x2E343530, 0x00000000, 0x0E000300, 0x00000000, 0x01000000,
        0x0F000600, 0x04000000, 0x04000000, 0x6D61696E, 0x00000000, 0x09000000, 0x10000300, 0x04000000,
        0x07000000, 0x03000300, 0x01000000, 0x36010000, 0x04000A00, 0x474C5F47, 0x4F4F474C, 0x455F6370,
        0x705F7374, 0x796C655F, 0x6C696E65, 0x5F646972, 0x65637469, 0x76650000, 0x04000800, 0x474C5F47,
        0x4F4F474C, 0x455F696E, 0x636C7564, 0x655F6469, 0x72656374, 0x69766500, 0x05000400, 0x04000000,
        0x6D61696E, 0x00000000, 0x05000500, 0x09000000, 0x46726167, 0x436F6C6F, 0x72000000, 0x47000400,
        0x09000000, 0x1E000000, 0x00000000, 0x13000200, 0x02000000, 0x21000300, 0x03000000, 0x02000000,
        0x16000300, 0x06000000, 0x20000000, 0x17000400, 0x07000000, 0x06000000, 0x04000000, 0x20000400,
        0x08000000, 0x03000000, 0x07000000, 0x3B000400, 0x08000000, 0x09000000, 0x03000000, 0x2B000400,
        0x06000000, 0x0A000000, 0x0000803F, 0x2B000400, 0x06000000, 0x0B000000, 0x3333333F, 0x2B000400,
        0x06000000, 0x0C000000, 0x9A99993E, 0x2C000700, 0x07000000, 0x0D000000, 0x0A000000, 0x0B000000,
        0x0C000000, 0x0A000000, 0x36000500, 0x02000000, 0x04000000, 0x00000000, 0x03000000, 0xF8000200,
        0x05000000, 0x3E000300, 0x09000000, 0x0D000000, 0xFD000100, 0x38000100
    };

    // 바이트 순서 변환
    for (int i = 0; i < sizeof(vs) / sizeof(uint32); i++)
    {
        uint8* left = (uint8*)&vs[i];
        uint8* right = left + 3;
        std::swap(*left, *right);
        left = left + 1;
        right = right - 1;
        std::swap(*left, *right);
    }

    for (int i = 0; i < sizeof(fs) / sizeof(uint32); i++)
    {
        uint8* left = (uint8*)&fs[i];
        uint8* right = left + 3;
        std::swap(*left, *right);
        left = left + 1;
        right = right - 1;
        std::swap(*left, *right);
    }

    _vertexShader = _renderContext->CreateShader(OgShaderType::VERTEX, (const char*)vs, sizeof(vs), "main");
    _vertexShader->name = "TriangleSampleVertexShader";
    _vertexShader->Retain();

    _fragmentShader = _renderContext->CreateShader(OgShaderType::FRAGMENT, (const char*)fs, sizeof(fs), "main");
    _fragmentShader->name = "TriangleSampleFragmentShader";
    _fragmentShader->Retain();

    OgShaderHandle* handles[]{ _vertexShader, _fragmentShader };
    _program = _renderContext->CreateProgram(handles, 2);
    _program->name = "TriangleSampleShaderProgram";
    _program->Retain();
}

void OgTriangleSample::createPipeline()
{
    OgColorBlendDescriptor cbDesc{};
    cbDesc.attachmentCount = 1;
    cbDesc.attachments[0].blendEnable = false;

    OgRasterizationDescriptor rsDesc{};
    rsDesc.polygonMode = OgPolygonMode::FILL;
    rsDesc.cullMode = OgCullMode::NONE;
    rsDesc.frontFace = OgFrontFace::CLOCKWISE;
    rsDesc.scissorTest = false;
    rsDesc.primitiveType = OgPrimitiveType::TRIANGLE_LIST;

    OgDepthStencilDescriptor dsDesc{};
    dsDesc.depthTest = true;
    dsDesc.depthWrite = true;
    dsDesc.stencilTest = false;
    dsDesc.depthCompareOp = Render::OgCompareOp::LESS_OR_EQUAL;

    OgShaderDescriptor shDesc{};
    shDesc.shaderCount = 2;
    shDesc.shaders[0] = _vertexShader;
    shDesc.shaders[1] = _fragmentShader;
    shDesc.program = _program;

    OgVertexInputDescriptor viDesc{};
    OgVertexBufferLayoutDescriptor vblDesc[1]
    {
        OgVertexBufferLayoutDescriptor(0, sizeof(float) * 3)
    };
    OgVertexAttributeDescriptor vaDesc[1]
    {
        OgVertexAttributeDescriptor(0, 0, OgVertexFormat::FLOAT3, 0)
    };
    viDesc.attributes = vaDesc;
    viDesc.attributeCount = 1;
    viDesc.layouts = vblDesc;
    viDesc.layoutCount = 1;

    OgPipelineDescriptor pipeDesc{};
    pipeDesc.name = "TriangleSamplePipeline";
    pipeDesc.type = OgPipelineType::GRAPHICS_PIPELINE;
    pipeDesc.colorBlend = cbDesc;
    pipeDesc.depthStencil = dsDesc;
    pipeDesc.rasterize = rsDesc;
    pipeDesc.vertexInput = viDesc;
    pipeDesc.renderPass = _renderTargetRenderPass;
    pipeDesc.shader = shDesc;
    pipeDesc.resourceLayout = _resourceLayout;

    _pipeline = _renderContext->CreatePipeline(pipeDesc);
    _pipeline->Retain();
}

void OgTriangleSample::createRenderTarget(uint16 width, uint16 height)
{
    _renderTargetWidth = width;
    _renderTargetHeight = height;

    // 샘플러 생성
    OgSamplerInfo samplerInfo{};
    samplerInfo.type = OgSamplerType::TEX_2D;
    samplerInfo.addressU = OgSamplerAddressMode::CLAMP_TO_EDGE;
    samplerInfo.addressV = OgSamplerAddressMode::CLAMP_TO_EDGE;
    samplerInfo.magFilter = OgFilter::LINEAR;
    samplerInfo.minFilter = OgFilter::LINEAR;
    samplerInfo.mipmapMode = OgSamplerMipmapMode::NEAREST;
    
    OgSamplerHandle* sampler = _renderContext->CreateSampler(samplerInfo);

    // 렌더 타겟 텍스처 생성
    OgTextureInfo texInfo{};
    texInfo.type = OgTextureType::TEX_2D;
    texInfo.format = OgPixelFormat::R8G8B8A8_UNORM;
    texInfo.extent.width = width;
    texInfo.extent.height = height;
    texInfo.usage = OgTextureUsage::COLOR_ATTACHMENT | OgTextureUsage::SAMPLED;
    texInfo.isGenerateMipmaps = false;
    
    _renderTargetTexture = _renderContext->CreateTexture((void**)nullptr, texInfo, sampler);
    _renderTargetTexture->name = "TriangleRenderTarget";
    _renderTargetTexture->Retain();

    // 깊이 텍스처 생성
    OgTextureInfo depthTexInfo{};
    depthTexInfo.type = OgTextureType::TEX_2D;
    depthTexInfo.format = OgPixelFormat::D24_UNORM_S8_UINT;
    depthTexInfo.extent.width = width;
    depthTexInfo.extent.height = height;
    depthTexInfo.usage = OgTextureUsage::DEPTH_STENCIL_ATTACHMENT;
    
    OgSamplerHandle* depthSampler = _renderContext->CreateSampler(samplerInfo);
    _depthTexture = _renderContext->CreateTexture(nullptr, depthTexInfo, depthSampler);
    _depthTexture->name = "TriangleDepthBuffer";
    _depthTexture->Retain();

    // 렌더 패스 생성
    OgAttachment rtColor{};
    rtColor.isDepthStencilAttachment = false;
    rtColor.format = OgRenderTextureFormat::R8G8B8A8_UNORM;
    rtColor.load = OgRenderBufferLoadAction::CLEAR;
    rtColor.store = OgRenderBufferStoreAction::STORE;

    OgAttachment rtDepth{};
    rtDepth.isDepthStencilAttachment = true;
    rtDepth.format = OgRenderTextureFormat::DEPTH24_STENCIL8;
    rtDepth.load = OgRenderBufferLoadAction::CLEAR;
    rtDepth.store = OgRenderBufferStoreAction::STORE;

    OgRenderPassInfo rtRpInfo{};
    rtRpInfo.isSwapchainRenderPass = false;
    rtRpInfo.outputColorAttachmentCount = 1;
    rtRpInfo.outputColorAttachments = &rtColor;
    rtRpInfo.useDepthStencilAttachment = true;
    rtRpInfo.outputDepthStencilAttachment = rtDepth;
    rtRpInfo.resolveColorAttachmentCount = 0;

    _renderTargetRenderPass = _renderContext->CreateRenderPass(rtRpInfo);
    _renderTargetRenderPass->name = "TriangleRenderTargetPass";
    _renderTargetRenderPass->Retain();

    // 프레임버퍼 생성
    OgVector<OgTextureHandle*> rtTextures;
    rtTextures.Add(_renderTargetTexture);
    
    OgFrameBufferInfo rtFbInfo{};
    rtFbInfo.width = width;
    rtFbInfo.height = height;
    rtFbInfo.renderPass = _renderTargetRenderPass;
    rtFbInfo.colorBuffers = rtTextures;
    rtFbInfo.depthStencilBuffer = _depthTexture;
    
    _renderTargetFrameBuffer = _renderContext->CreateFrameBuffer(rtFbInfo);
    _renderTargetFrameBuffer->name = "TriangleRenderTargetFrameBuffer";
    _renderTargetFrameBuffer->Retain();
}

void OgTriangleSample::destroyRenderTarget()
{
    if (_renderTargetFrameBuffer)
    {
        _renderTargetFrameBuffer->Release();
        _renderTargetFrameBuffer = nullptr;
    }
    
    if (_renderTargetRenderPass)
    {
        _renderTargetRenderPass->Release();
        _renderTargetRenderPass = nullptr;
    }
    
    if (_depthTexture)
    {
        _depthTexture->Release();
        _depthTexture = nullptr;
    }
    
    if (_renderTargetTexture)
    {
        _renderTargetTexture->Release();
        _renderTargetTexture = nullptr;
    }
}

OG_NAMESPACE_SAMPLE_END
