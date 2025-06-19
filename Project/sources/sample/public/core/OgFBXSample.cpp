#include "OgFBXSample.h"
#include "glm/gtc/matrix_transform.hpp"
#include <cmath>

using namespace std;
using namespace Render;

OG_NAMESPACE_SAMPLE_BEGIN

OgFBXSample::OgFBXSample(Render::OgRenderContext* renderContext)
	: OgSampleBase(renderContext)
{
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

	encoder->BindIndexBuffer(_indexBuffer);

	encoder->BindResourceSet(_resourceSet);

	encoder->DrawIndexed(_indexCount, 1, 0, 0, 0);

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
	_uniformData.projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
	updateUniformBuffer();
}

void OgFBXSample::createResources(uint16 width, uint16 height)
{
	// 메시 생성
	createMesh();
	
	// 셰이더 생성
	createShaders();
	
	// 유니폼 버퍼 생성
	createUniformBuffer();
	
	// 리소스 레이아웃 생성
	OgResourceLayoutItem items[1];
	items[0].stages = OgShaderStage::VERTEX;
	items[0].type = OgResourceType::UNIFORM_BUFFER;
	items[0].bindingIndex = 0;
	items[0].size = sizeof(UniformData);
	
	_resourceLayout = _renderContext->CreateResourceLayout(items, 1);
	_resourceLayout->name = "FBXSampleResourceLayout";
	_resourceLayout->Retain();
	
	// 리소스 셋 생성
	_resourceSet = _renderContext->CreateResourceSet(_resourceLayout, 0);
	_resourceSet->name = "FBXSampleResourceSet";
	_resourceSet->Retain();
	
	// 유니폼 버퍼를 리소스 셋에 바인딩
	_renderContext->UpdateResourceSet(_resourceSet, 0, OgResourceType::UNIFORM_BUFFER, _uniformBuffer);
	
	// 렌더 타겟 생성
	createRenderTarget(width, height);
	
	// 파이프라인 생성
	createPipeline();
}

void OgFBXSample::destroyResources()
{
	_renderContext->WaitDeviceIdle();
	
	if (_pipeline)
	{
		_renderContext->DestroyPipeline(_pipeline);
		_pipeline = nullptr;
	}
	
	destroyRenderTarget();
	
	if (_resourceSet)
	{
		_renderContext->DestroyResourceSet(_resourceSet);
		_resourceSet = nullptr;
	}
	
	if (_resourceLayout)
	{
		_renderContext->DestroyResourceLayout(_resourceLayout);
		_resourceLayout = nullptr;
	}
	
	if (_program)
	{
		_renderContext->DestroyProgram(_program);
		_program = nullptr;
	}
	
	if (_fragmentShader)
	{
		_renderContext->DestroyShader(_fragmentShader);
		_fragmentShader = nullptr;
	}
	
	if (_vertexShader)
	{
		_renderContext->DestroyShader(_vertexShader);
		_vertexShader = nullptr;
	}
	
	if (_uniformBuffer)
	{
		_renderContext->DestroyBuffer(_uniformBuffer);
		_uniformBuffer = nullptr;
	}
	
	if (_indexBuffer)
	{
		_renderContext->DestroyBuffer(_indexBuffer);
		_indexBuffer = nullptr;
	}
	
	if (_vertexBuffer)
	{
		_renderContext->DestroyBuffer(_vertexBuffer);
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
	_uniformData.view = glm::lookAt(
		glm::vec3(2.0f, 2.0f, 2.0f),  // 카메라 위치
		glm::vec3(0.0f, 0.0f, 0.0f),  // 타겟
		glm::vec3(0.0f, 1.0f, 0.0f)   // 업 벡터
	);
	float aspect = static_cast<float>(_renderTargetWidth) / static_cast<float>(_renderTargetHeight);
	_uniformData.projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
	
	// 유니폼 버퍼 생성
	_uniformBuffer = _renderContext->CreateBuffer(
		&_uniformData,
		sizeof(UniformData),
		Render::OgBufferUsage::UNIFORM,
		OgMemoryOption::MAP_WRITE
	);
	_uniformBuffer->Retain();
}

void OgFBXSample::updateUniformBuffer()
{
	// 모델 행렬 업데이트 (Y축 회전)
	_uniformData.model = glm::rotate(glm::mat4(1.0f), glm::radians(_rotation), glm::vec3(0.0f, 1.0f, 0.0f));
	
	// 유니폼 버퍼 업데이트
	void* mappedData = _renderContext->MapBuffer(_uniformBuffer, 0, sizeof(UniformData));
	if (mappedData)
	{
		memcpy(mappedData, &_uniformData, sizeof(UniformData));
		_renderContext->UnmapBuffer(_uniformBuffer);
	}
}

void OgFBXSample::createShaders()
{
	// 간단한 3D 셰이더 코드 (SPIR-V)
	// 실제로는 별도의 셰이더 파일로 관리하는 것이 좋습니다
	// 여기서는 Triangle 샘플처럼 하드코딩된 SPIR-V 코드를 사용합니다
	
	// Vertex Shader (MVP 변환 + 색상 전달)
	// Fragment Shader (색상 보간)
	// 실제 SPIR-V 코드는 glslangValidator를 통해 생성해야 합니다
	
	// 임시로 간단한 셰이더 사용 (Triangle 샘플의 셰이더 수정)
	uint32 vs[]{
		// 여기에 실제 SPIR-V 코드가 들어가야 합니다
		// 현재는 Triangle 샘플의 셰이더를 재사용
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
	_vertexShader->name = "FBXSampleVertexShader";
	_vertexShader->Retain();

	_fragmentShader = _renderContext->CreateShader(OgShaderType::FRAGMENT, (const char*)fs, sizeof(fs), "main");
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
	rsDesc.frontFace = OgFrontFace::COUNTER_CLOCKWISE;
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
		OgVertexAttributeDescriptor(1, 0, OgVertexFormat::FLOAT3, offsetof(Vertex, normal)),
		OgVertexAttributeDescriptor(2, 0, OgVertexFormat::FLOAT2, offsetof(Vertex, texCoord)),
		OgVertexAttributeDescriptor(3, 0, OgVertexFormat::FLOAT3, offsetof(Vertex, color))
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
	
	// 예제:
	// FbxManager* fbxManager = FbxManager::Create();
	// FbxIOSettings* ios = FbxIOSettings::Create(fbxManager, IOSROOT);
	// fbxManager->SetIOSettings(ios);
	// 
	// FbxImporter* importer = FbxImporter::Create(fbxManager, "");
	// if (!importer->Initialize(filePath, -1, fbxManager->GetIOSettings())) {
	//     return false;
	// }
	// 
	// FbxScene* scene = FbxScene::Create(fbxManager, "myScene");
	// importer->Import(scene);
	// importer->Destroy();
	// 
	// // 메시 데이터 추출...
	// 
	// fbxManager->Destroy();
	
	return false; // 현재는 구현되지 않음
}

OG_NAMESPACE_SAMPLE_END
