#include "OgTriangleSample.h"
#include "util/OgShaderCompiler.h"

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
	// GLSL 셰이더 코드
	const char* vertexShaderGLSL = R"(
		#version 450
		
		layout(location = 0) in vec3 a_Position;
		
		void main()
		{
			gl_Position = vec4(a_Position, 1.0);
		}
	)";
	
	const char* fragmentShaderGLSL = R"(
		#version 450
		
		layout(location = 0) out vec4 FragColor;
		
		void main()
		{
			FragColor = vec4(1.0, 0.3, 0.2, 1.0);
		}
	)";
	
	// GLSL을 SPIR-V로 컴파일
	std::vector<uint32_t> vertexSPIRV;
	std::vector<uint32_t> fragmentSPIRV;
	
	if (!OgShaderCompiler::CompileGLSLtoSPIRV(vertexShaderGLSL, OgShaderType::VERTEX, vertexSPIRV))
	{
		LOGE(OG_ID, "Failed to compile vertex shader");
		return;
	}
	
	if (!OgShaderCompiler::CompileGLSLtoSPIRV(fragmentShaderGLSL, OgShaderType::FRAGMENT, fragmentSPIRV))
	{
		LOGE(OG_ID, "Failed to compile fragment shader");
		return;
	}
	
	// 컴파일된 SPIR-V로 셰이더 생성
	_vertexShader = _renderContext->CreateShader(
		OgShaderType::VERTEX, 
		reinterpret_cast<const char*>(vertexSPIRV.data()), 
		vertexSPIRV.size() * sizeof(uint32_t), 
		"main"
	);
	_vertexShader->name = "TriangleSampleVertexShader";
	_vertexShader->Retain();

	_fragmentShader = _renderContext->CreateShader(
		OgShaderType::FRAGMENT, 
		reinterpret_cast<const char*>(fragmentSPIRV.data()), 
		fragmentSPIRV.size() * sizeof(uint32_t), 
		"main"
	);
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
