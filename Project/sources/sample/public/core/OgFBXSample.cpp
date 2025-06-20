#include "OgFBXSample.h"
#include "sample/public/core/util/OgShaderCompiler.h"
#include "glm/gtc/matrix_transform.hpp"
#include <cmath>

using namespace std;
using namespace Render;

OG_NAMESPACE_SAMPLE_BEGIN

OgFBXSample::OgFBXSample(Render::OgRenderContext* renderContext)
	: OgSampleBase(renderContext)
	, _camera(std::make_unique<OgFlyCamera>())
{
	// 카메라 초기 설정
	_camera->SetPosition(glm::vec3(2.0f, 2.0f, 2.0f));
	_camera->SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));
}

OgFBXSample::~OgFBXSample()
{
	if (_isInitialized)
	{
		OnDestroy();
	}
}

void OgFBXSample::OnInit(Render::OgSwapChain* swapchain)
{
	if (_isInitialized)
		return;

	// 스왑체인의 크기로 리소스 생성
	const uint16 width = _renderContext->GetSwapChainFrameBuffer(swapchain, 0)->width;
	const uint16 height = _renderContext->GetSwapChainFrameBuffer(swapchain, 0)->height;

	createResources(width, height);

	_isInitialized = true;
}

void OgFBXSample::OnDestroy()
{
	if (!_isInitialized)
		return;

	destroyResources();
	_isInitialized = false;
}

void OgFBXSample::OnUpdate(float deltaTime)
{
	// 카메라 업데이트
	if (_useFlyCamera && _camera)
	{
		_camera->Update(deltaTime);
	}

	// 큐브 회전
	_rotation += deltaTime * 45.0f; // 초당 45도 회전
	if (_rotation > 360.0f)
		_rotation -= 360.0f;

	updateUniformBuffer();
}

void OgFBXSample::OnRender(Render::OgCommandEncoderHandle* encoder, Render::OgSwapChain* swapchain)
{
	if (!_isInitialized || !_renderTargetFrameBuffer)
		return;

	float passColor[]{ 0.0f, 0.0f, 1.0f, 1.0f };

	OgCommandEncoderHandle::ClearValue colorClear;
	colorClear.color.value[0] = 0.1f;
	colorClear.color.value[1] = 0.1f;
	colorClear.color.value[2] = 0.2f;
	colorClear.color.value[3] = 1.0f;

	OgCommandEncoderHandle::ClearValue depthStencilClear;
	depthStencilClear.depthStencil.depth = 1.f;
	depthStencilClear.depthStencil.stencil = 0.f;

	// 렌더 타겟 프레임버퍼 사용
	OgCommandEncoderHandle::Area area(0, 0, _renderTargetWidth, _renderTargetHeight);

	encoder->BeginDebugMarker("Sample - FBXModel", passColor);

	// 렌더 타겟에 렌더링
	encoder->BeginRenderPass(_renderTargetRenderPass, _renderTargetFrameBuffer, area, 1, &colorClear, 0, nullptr, &depthStencilClear);

	encoder->SetViewport(static_cast<float>(area.x), static_cast<float>(area.y),
		static_cast<float>(area.width), static_cast<float>(area.height));

	encoder->SetScissor(area.x, area.y, area.width, area.height);

	encoder->BindPipeline(_pipeline);

	encoder->BindVertexBuffers(&_vertexBuffer, 0, 1);

	encoder->BindIndexBuffer(_indexBuffer, Render::OgIndexType::UINT16);

	encoder->BindResourceSet(_resourceSet);

	encoder->DrawIndexed(0, _indexCount, 1, 0);

	encoder->EndRenderPass();

	encoder->EndDebugMarker();
}

void OgFBXSample::OnSuspend(Render::OgSwapChain* swapchain)
{
	_renderContext->Suspend(swapchain);
}

void OgFBXSample::OnRestore(Render::OgSwapChain* swapchain)
{
	_renderContext->Restore(swapchain);
}

void OgFBXSample::OnResize(uint32 width, uint32 height)
{
	if (!_isInitialized)
		return;

	// 크기가 변경되면 렌더 타겟 재생성
	destroyRenderTarget();
	createRenderTarget(static_cast<uint16>(width), static_cast<uint16>(height));

	// 프로젝션 행렬 업데이트
	float aspect = static_cast<float>(width) / static_cast<float>(height);
	if (_useFlyCamera && _camera)
	{
		_camera->SetAspectRatio(aspect);
		_uniformData.projection = _camera->GetProjectionMatrix();
	}
	else
	{
		_uniformData.projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
	}
	updateUniformBuffer();
}

// 입력 처리 메서드들
void OgFBXSample::OnMouseButton(int button, int action, int mods)
{
	if (_useFlyCamera && _camera)
	{
		_camera->OnMouseButton(button, action, mods);
	}
}

void OgFBXSample::OnMouseMove(double x, double y)
{
	if (_useFlyCamera && _camera)
	{
		_camera->OnMouseMove(x, y);
	}
}

void OgFBXSample::OnMouseScroll(double xoffset, double yoffset)
{
	if (_useFlyCamera && _camera)
	{
		_camera->OnMouseScroll(xoffset, yoffset);
	}
}

void OgFBXSample::OnKeyPress(int key, int action, int mods)
{
	if (_useFlyCamera && _camera)
	{
		_camera->OnKeyPress(key, action, mods);
	}

	// F 키로 플라이 카메라 토글
	if (key == OG_KEY_F && action == OG_PRESS)
	{
		_useFlyCamera = !_useFlyCamera;
		updateUniformBuffer();
	}
}

void OgFBXSample::createResources(uint16 width, uint16 height)
{
	// 렌더 타겟을 먼저 생성하여 _renderTargetWidth/Height 설정
	createRenderTarget(width, height);

	// 메시 생성
	createMesh();

	// 셰이더 생성
	createShaders();

	// 유니폼 버퍼 생성 (이제 _renderTargetWidth/Height가 설정되어 있음)
	createUniformBuffer();

	// 리소스 레이아웃 생성
	OgResourceBinding bindings[1];
	bindings[0].type = OgResourceType::UNIFORM_BUFFER;
	bindings[0].stage = OgShaderType::VERTEX;
	bindings[0].binding = 0;
	bindings[0].arrayCount = 0;
	bindings[0].name = nullptr;

	_resourceLayout = _renderContext->CreateResourceLayout(bindings, 1);
	_resourceLayout->name = "FBXSampleResourceLayout";
	_resourceLayout->Retain();

	// 리소스 셋 생성
	uint32 zeroOffset = 0;

	OgResourceUsage usages[1];
	usages[0].binding = bindings[0];
	usages[0].buffer.handle = &_uniformBuffer;
	usages[0].buffer.offset = &zeroOffset;
	usages[0].buffer.range = &_uniformBuffer->size;

	_resourceSet = _renderContext->CreateResourceSet(_resourceLayout, usages, 1);
	_resourceSet->name = "FBXSampleResourceSet";
	_resourceSet->Retain();

	// 파이프라인 생성
	createPipeline();
}

void OgFBXSample::destroyResources()
{
	_renderContext->WaitDeviceIdle();

	if (_pipeline)
	{
		_pipeline->Release();
		_pipeline = nullptr;
	}

	destroyRenderTarget();

	if (_resourceSet)
	{
		_resourceSet->Release();
		_resourceSet = nullptr;
	}

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

	if (_uniformBuffer)
	{
		_uniformBuffer->Release();
		_uniformBuffer = nullptr;
	}

	if (_indexBuffer)
	{
		_indexBuffer->Release();
		_indexBuffer = nullptr;
	}

	if (_vertexBuffer)
	{
		_vertexBuffer->Release();
		_vertexBuffer = nullptr;
	}
}

void OgFBXSample::createMesh()
{
	// 간단한 큐브 메시 생성 (FBX 로더가 없으므로 하드코딩)
	// 실제 구현에서는 loadFBXModel() 함수를 통해 FBX 파일에서 로드

	// 큐브 정점 데이터 (8개 정점, 각 면의 색상 다르게)
	_vertices = {
		// 앞면 (빨강)
		{{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
		{{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
		{{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
		{{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},

		// 뒷면 (초록)
		{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
		{{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
		{{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
		{{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},

		// 왼쪽면 (파랑)
		{{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},

		// 오른쪽면 (노랑)
		{{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 0.0f}},
		{{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 0.0f}},
		{{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}},
		{{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}},

		// 윗면 (보라)
		{{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 1.0f}},
		{{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 1.0f}},
		{{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}},
		{{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 1.0f}},

		// 아랫면 (청록)
		{{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 1.0f}},
		{{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f, 1.0f}},
		{{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}},
		{{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 1.0f, 1.0f}}
	};

	// 인덱스 데이터
	_indices = {
		// 앞면
		0, 1, 2,  2, 3, 0,
		// 뒷면
		4, 6, 5,  6, 4, 7,
		// 왼쪽면
		8, 9, 10,  10, 11, 8,
		// 오른쪽면
		12, 14, 13,  14, 12, 15,
		// 윗면
		16, 17, 18,  18, 19, 16,
		// 아랫면
		20, 22, 21,  22, 20, 23
	};

	_indexCount = static_cast<uint32>(_indices.size());

	// 버텍스 버퍼 생성
	_vertexBuffer = _renderContext->CreateBuffer(
		_vertices.data(),
		sizeof(Vertex) * _vertices.size(),
		Render::OgBufferUsage::VERTEX,
		OgMemoryOption::MAP_MANAGED
	);
	_vertexBuffer->Retain();

	// 인덱스 버퍼 생성
	_indexBuffer = _renderContext->CreateBuffer(
		_indices.data(),
		sizeof(uint16) * _indices.size(),
		Render::OgBufferUsage::INDEX,
		OgMemoryOption::MAP_MANAGED
	);
	_indexBuffer->Retain();
}

void OgFBXSample::createUniformBuffer()
{
	// 초기 변환 행렬 설정
	_uniformData.model = glm::mat4(1.0f);
	
	// aspect ratio 계산 시 0으로 나누기 방지
	float aspect = 1.0f;
	if (_renderTargetHeight > 0)
	{
		aspect = static_cast<float>(_renderTargetWidth) / static_cast<float>(_renderTargetHeight);
	}

	// 카메라 설정에 따라 뷰/프로젝션 행렬 설정
	if (_useFlyCamera && _camera)
	{
		_camera->SetAspectRatio(aspect);
		_uniformData.view = _camera->GetViewMatrix();
		_uniformData.projection = _camera->GetProjectionMatrix();
	}
	else
	{
		_uniformData.view = glm::lookAt(
			glm::vec3(2.0f, 2.0f, 2.0f),  // 카메라 위치
			glm::vec3(0.0f, 0.0f, 0.0f),  // 타겟
			glm::vec3(0.0f, 1.0f, 0.0f)   // 업 벡터
		);
		_uniformData.projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
	}

	// 유니폼 버퍼 생성
	_uniformBuffer = _renderContext->CreateBuffer(
		&_uniformData,
		sizeof(UniformData),
		Render::OgBufferUsage::UNIFORM,
		OgMemoryOption::MAP_MANAGED
	);
	_uniformBuffer->Retain();
}

void OgFBXSample::updateUniformBuffer()
{
	// 모델 행렬 업데이트 (Y축 회전)
	_uniformData.model = glm::rotate(glm::mat4(1.0f), glm::radians(_rotation), glm::vec3(0.0f, 1.0f, 0.0f));

	// 카메라 사용 시 뷰/프로젝션 행렬 업데이트
	if (_useFlyCamera && _camera)
	{
		_uniformData.view = _camera->GetViewMatrix();
		_uniformData.projection = _camera->GetProjectionMatrix();
	}

	// 유니폼 버퍼 업데이트
	void* mappedData = _renderContext->MapBuffer(_uniformBuffer, sizeof(UniformData));
	if (mappedData)
	{
		memcpy(mappedData, &_uniformData, sizeof(UniformData));
		_renderContext->UnmapBuffer(_uniformBuffer);
	}
}

void OgFBXSample::createShaders()
{
	// MVP 변환을 지원하는 GLSL 셰이더
	const char* vertexShaderGLSL = R"(
		#version 450
		
		layout(location = 0) in vec3 inPosition;
		layout(location = 1) in vec3 inNormal;
		layout(location = 2) in vec2 inTexCoord;
		layout(location = 3) in vec3 inColor;
		
		layout(binding = 0) uniform UniformBufferObject {
			mat4 model;
			mat4 view;
			mat4 proj;
		} ubo;

		layout(location = 0) out vec3 fragColor;
		layout(location = 1) out vec3 fragNormal;
		layout(location = 2) out vec2 fragTexCoord;

		void main() {
			gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
			fragColor = inColor;
			fragNormal = mat3(transpose(inverse(ubo.model))) * inNormal;
			fragTexCoord = inTexCoord;
		}
	)";

	const char* fragmentShaderGLSL = R"(
		#version 450

		layout(location = 0) in vec3 fragColor;
		layout(location = 1) in vec3 fragNormal;
		layout(location = 2) in vec2 fragTexCoord;

		layout(location = 0) out vec4 outColor;

		void main() {
			// 간단한 디렉셔널 라이팅
			vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
			float diff = max(dot(normalize(fragNormal), lightDir), 0.0);
			vec3 diffuse = diff * fragColor;
			
			vec3 ambient = 0.15 * fragColor;
			vec3 result = ambient + diffuse;
			
			outColor = vec4(result, 1.0);
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
	_vertexShader->name = "FBXSampleVertexShader";
	_vertexShader->Retain();

	_fragmentShader = _renderContext->CreateShader(
		OgShaderType::FRAGMENT, 
		reinterpret_cast<const char*>(fragmentSPIRV.data()), 
		fragmentSPIRV.size() * sizeof(uint32_t), 
		"main"
	);
	_fragmentShader->name = "FBXSampleFragmentShader";
	_fragmentShader->Retain();

	OgShaderHandle* handles[]{ _vertexShader, _fragmentShader };
	_program = _renderContext->CreateProgram(handles, 2);
	_program->name = "FBXSampleShaderProgram";
	_program->Retain();
}

void OgFBXSample::createPipeline()
{
	OgColorBlendDescriptor cbDesc{};
	cbDesc.attachmentCount = 1;
	cbDesc.attachments[0].blendEnable = false;

	OgRasterizationDescriptor rsDesc{};
	rsDesc.polygonMode = OgPolygonMode::FILL;
	rsDesc.cullMode = OgCullMode::BACK;  // 백페이스 컬링 활성화
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
		OgVertexBufferLayoutDescriptor(0, sizeof(Vertex))
	};
	OgVertexAttributeDescriptor vaDesc[4]
	{
		OgVertexAttributeDescriptor(0, 0, OgVertexFormat::FLOAT3, offsetof(Vertex, position)),
		OgVertexAttributeDescriptor(0, 1, OgVertexFormat::FLOAT3, offsetof(Vertex, normal)),
		OgVertexAttributeDescriptor(0, 2, OgVertexFormat::FLOAT2, offsetof(Vertex, texCoord)),
		OgVertexAttributeDescriptor(0, 3, OgVertexFormat::FLOAT3, offsetof(Vertex, color))
	};
	viDesc.attributes = vaDesc;
	viDesc.attributeCount = 4;
	viDesc.layouts = vblDesc;
	viDesc.layoutCount = 1;

	OgPipelineDescriptor pipeDesc{};
	pipeDesc.name = "FBXSamplePipeline";
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

void OgFBXSample::createRenderTarget(uint16 width, uint16 height)
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
	_renderTargetTexture->name = "FBXRenderTarget";
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
	_depthTexture->name = "FBXDepthBuffer";
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
	_renderTargetRenderPass->name = "FBXRenderTargetPass";
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
	_renderTargetFrameBuffer->name = "FBXRenderTargetFrameBuffer";
	_renderTargetFrameBuffer->Retain();
}

void OgFBXSample::destroyRenderTarget()
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

bool OgFBXSample::loadFBXModel(const char* filePath)
{
	// TODO: FBX SDK 또는 Assimp 라이브러리를 사용하여 FBX 파일 로드
	// 1. FBX 파일 열기
	// 2. 메시 데이터 추출 (정점, 노말, UV, 인덱스)
	// 3. 재질 정보 추출
	// 4. 애니메이션 데이터 추출 (옵션)
	// 5. _vertices와 _indices에 데이터 저장

	return false; // 현재는 구현되지 않음
}

OG_NAMESPACE_SAMPLE_END
