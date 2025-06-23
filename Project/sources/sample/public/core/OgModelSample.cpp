#include "OgModelSample.h"
#include "sample/public/core/util/OgShaderCompiler.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

// tinygltf 헤더 - single header library
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#include "tinygltf/tiny_gltf.h"

#include <cmath>
#include <algorithm>
#include <filesystem>

using namespace std;
using namespace Render;

OG_NAMESPACE_SAMPLE_BEGIN

OgModelSample::OgModelSample(Render::OgRenderContext* renderContext)
	: OgSampleBase(renderContext)
	, _camera(std::make_unique<OgFlyCamera>())
{
	// 카메라 초기 설정
	_camera->SetPosition(glm::vec3(5.0f, 5.0f, 5.0f));
	_camera->SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));
}

OgModelSample::~OgModelSample()
{
	if (_isInitialized)
	{
		OnDestroy();
	}
}

void OgModelSample::OnInit(Render::OgSwapChain* swapchain)
{
	if (_isInitialized)
		return;

	// 스왑체인의 크기로 리소스 생성
	const uint16 width = _renderContext->GetSwapChainFrameBuffer(swapchain, 0)->width;
	const uint16 height = _renderContext->GetSwapChainFrameBuffer(swapchain, 0)->height;

	createResources(width, height);

	// glTF 폴더에서 첫 번째 glTF 파일을 찾아서 로드
	// 실행 파일 경로를 기준으로 glTF 폴더 경로 구성
	std::filesystem::path executablePath = std::filesystem::current_path();
	std::filesystem::path gltfPath = executablePath / "glTF";
	
	// 만약 현재 경로에서 찾을 수 없으면, Build/Debug 경로도 시도
	if (!std::filesystem::exists(gltfPath)) {
		gltfPath = executablePath / "Build" / "Debug" / "glTF";
	}
	
	// 그래도 없으면 상위 디렉토리들도 확인
	if (!std::filesystem::exists(gltfPath)) {
		std::filesystem::path searchPath = executablePath;
		for (int i = 0; i < 3; ++i) {
			searchPath = searchPath.parent_path();
			std::filesystem::path testPath = searchPath / "Build" / "Debug" / "glTF";
			if (std::filesystem::exists(testPath)) {
				gltfPath = testPath;
				break;
			}
		}
	}
	
	bool modelLoaded = false;
	
	try 
	{
		if (std::filesystem::exists(gltfPath) && std::filesystem::is_directory(gltfPath)) {
			LOGD(OG_ID, "Found glTF directory at: %s", gltfPath.string().c_str());
			
			for (const auto& entry : std::filesystem::directory_iterator(gltfPath)) 
			{
				if (entry.is_regular_file()) 
				{
					std::string extension = entry.path().extension().string();
					if (extension == ".gltf" || extension == ".glb") 
					{
						std::string filePath = entry.path().string();
						
						LOGD(OG_ID, "Loading glTF model: %s", filePath.c_str());
						if (loadGLTFModel(filePath)) {
							modelLoaded = true;
							LOGD(OG_ID, "Successfully loaded glTF model");
							break;
						}
					}
				}
			}
		} else {
			LOGE(OG_ID, "glTF directory not found at: %s", gltfPath.string().c_str());
			LOGD(OG_ID, "Current working directory: %s", executablePath.string().c_str());
		}
	} catch (const std::exception& e) 
	{
		LOGE(OG_ID, "Error accessing glTF directory: %s", e.what());
	}

	if (!modelLoaded) 
	{
		LOGD(OG_ID, "No glTF model found in ./glTF/ directory, using default cube");
		createDefaultMesh();
	}

	_isInitialized = true;
}

void OgModelSample::OnDestroy()
{
	if (!_isInitialized)
		return;

	destroyResources();
	_isInitialized = false;
}

void OgModelSample::OnUpdate(float deltaTime)
{
	// 카메라 업데이트
	if (_useFlyCamera && _camera)
	{
		_camera->Update(deltaTime);
	}

	// 모델 회전
	_rotation += deltaTime * 0.f; // 초당 0.5 라디안 회전
	if (_rotation > 2.0f * 3.14159f)
		_rotation -= 2.0f * 3.14159f;

	updateUniformBuffer();
}

void OgModelSample::OnRender(Render::OgCommandEncoderHandle* encoder, Render::OgSwapChain* swapchain)
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

	encoder->BeginDebugMarker("Sample - glTF Model", passColor);

	// 렌더 타겟에 렌더링
	encoder->BeginRenderPass(_renderTargetRenderPass, _renderTargetFrameBuffer, area, 1, &colorClear, 0, nullptr, &depthStencilClear);

	encoder->SetViewport(static_cast<float>(area.x), static_cast<float>(area.y),
		static_cast<float>(area.width), static_cast<float>(area.height));

	encoder->SetScissor(area.x, area.y, area.width, area.height);

	encoder->BindPipeline(_pipeline);
	encoder->BindResourceSet(_resourceSet);

	// 모델이 로드되어 있으면 렌더링
	if (_modelLoaded && !_rootNodes.empty()) {
		glm::mat4 rootTransform = glm::mat4(1.0f);
		
		// 모델을 중심으로 회전
		rootTransform = glm::rotate(rootTransform, _rotation, glm::vec3(0.0f, 1.0f, 0.0f));
		
		// 모델 크기 정규화 (카메라 거리에 맞게 스케일 조정)
		float scale = 5.0f / _modelRadius; // 모델을 적절한 크기로 조정
		scale *= 10.0f; // 10배 스케일링
		rootTransform = glm::scale(rootTransform, glm::vec3(scale));
		
		// 모델 중심을 원점으로 이동
		rootTransform = glm::translate(rootTransform, -_modelCenter);
		
		// 루트 노드들 렌더링
		for (int rootNode : _rootNodes) {
			renderNode(encoder, rootNode, rootTransform);
		}
	} else if (!_meshes.empty()) {
		// 기본 큐브 렌더링
		glm::mat4 modelMatrix = glm::rotate(glm::mat4(1.0f), _rotation, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(10.0f)); // 10배 스케일링
		renderMesh(encoder, _meshes[0], modelMatrix);
	}

	encoder->EndRenderPass();
	encoder->EndDebugMarker();
}

void OgModelSample::OnSuspend(Render::OgSwapChain* swapchain)
{
	_renderContext->Suspend(swapchain);
}

void OgModelSample::OnRestore(Render::OgSwapChain* swapchain)
{
	_renderContext->Restore(swapchain);
}

void OgModelSample::OnResize(uint32 width, uint32 height)
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
		_uniformData.projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);  // far plane을 1000으로 확장
		// Vulkan의 클립 공간 조정
		convertProjectionForVulkan(_uniformData.projection);
	}
	updateUniformBuffer();
}

// 입력 처리 메서드들
void OgModelSample::OnMouseButton(int button, int action, int mods)
{
	if (_useFlyCamera && _camera)
	{
		_camera->OnMouseButton(button, action, mods);
	}
}

void OgModelSample::OnMouseMove(double x, double y)
{
	if (_useFlyCamera && _camera)
	{
		_camera->OnMouseMove(x, y);
	}
}

void OgModelSample::OnMouseScroll(double xoffset, double yoffset)
{
	if (_useFlyCamera && _camera)
	{
		_camera->OnMouseScroll(xoffset, yoffset);
	}
}

void OgModelSample::OnKeyPress(int key, int action, int mods)
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

void OgModelSample::createResources(uint16 width, uint16 height)
{
	// 렌더 타겟을 먼저 생성
	createRenderTarget(width, height);

	// 셰이더 생성
	createShaders();

	// 유니폼 버퍼 생성
	createUniformBuffer();

	// 기본 텍스처 생성
	uint32_t whitePixel = 0xFFFFFFFF;
	OgTextureInfo whiteTexInfo{};
	whiteTexInfo.type = OgTextureType::TEX_2D;
	whiteTexInfo.format = OgPixelFormat::R8G8B8A8_UNORM;
	whiteTexInfo.extent.width = 1;
	whiteTexInfo.extent.height = 1;
	whiteTexInfo.usage = OgTextureUsage::SAMPLED;
	
	OgSamplerInfo samplerInfo{};
	samplerInfo.type = OgSamplerType::TEX_2D;
	samplerInfo.addressU = OgSamplerAddressMode::REPEAT;
	samplerInfo.addressV = OgSamplerAddressMode::REPEAT;
	samplerInfo.magFilter = OgFilter::LINEAR;
	samplerInfo.minFilter = OgFilter::LINEAR;
	
	void* whiteData = &whitePixel;
	_defaultWhiteTexture = _renderContext->CreateTexture(&whiteData, whiteTexInfo, _renderContext->CreateSampler(samplerInfo));
	_defaultWhiteTexture->Retain();
	
	// 기본 노말 텍스처 (0.5, 0.5, 1.0, 1.0) - 중성 노말
	uint32_t normalPixel = 0xFFFF8080; // RGBA = (128, 128, 255, 255)
	void* normalData = &normalPixel;
	_defaultNormalTexture = _renderContext->CreateTexture(&normalData, whiteTexInfo, _renderContext->CreateSampler(samplerInfo));
	_defaultNormalTexture->Retain();

	// 리소스 레이아웃 생성
	OgResourceBinding bindings[9];
	// 유니폼 버퍼
	bindings[0].type = OgResourceType::UNIFORM_BUFFER;
	bindings[0].stage = OgShaderType::VERTEX;
	bindings[0].binding = 0;
	bindings[0].arrayCount = 0;
	bindings[0].name = nullptr;
	
	// Material 유니폼 버퍼
	bindings[1].type = OgResourceType::UNIFORM_BUFFER;
	bindings[1].stage = OgShaderType::FRAGMENT;
	bindings[1].binding = 1;
	bindings[1].arrayCount = 0;
	bindings[1].name = nullptr;
	
	// Base color texture
	bindings[2].type = OgResourceType::COMBINED_IMAGE_SAMPLER;
	bindings[2].stage = OgShaderType::FRAGMENT;
	bindings[2].binding = 2;
	bindings[2].arrayCount = 0;
	bindings[2].name = nullptr;
	
	// Normal texture
	bindings[3].type = OgResourceType::COMBINED_IMAGE_SAMPLER;
	bindings[3].stage = OgShaderType::FRAGMENT;
	bindings[3].binding = 3;
	bindings[3].arrayCount = 0;
	bindings[3].name = nullptr;
	
	// Metallic roughness texture
	bindings[4].type = OgResourceType::COMBINED_IMAGE_SAMPLER;
	bindings[4].stage = OgShaderType::FRAGMENT;
	bindings[4].binding = 4;
	bindings[4].arrayCount = 0;
	bindings[4].name = nullptr;
	
	// Emissive texture
	bindings[5].type = OgResourceType::COMBINED_IMAGE_SAMPLER;
	bindings[5].stage = OgShaderType::FRAGMENT;
	bindings[5].binding = 5;
	bindings[5].arrayCount = 0;
	bindings[5].name = nullptr;
	
	// Occlusion texture
	bindings[6].type = OgResourceType::COMBINED_IMAGE_SAMPLER;
	bindings[6].stage = OgShaderType::FRAGMENT;
	bindings[6].binding = 6;
	bindings[6].arrayCount = 0;
	bindings[6].name = nullptr;
	
	// Sheen color texture
	bindings[7].type = OgResourceType::COMBINED_IMAGE_SAMPLER;
	bindings[7].stage = OgShaderType::FRAGMENT;
	bindings[7].binding = 7;
	bindings[7].arrayCount = 0;
	bindings[7].name = nullptr;
	
	// Sheen roughness texture
	bindings[8].type = OgResourceType::COMBINED_IMAGE_SAMPLER;
	bindings[8].stage = OgShaderType::FRAGMENT;
	bindings[8].binding = 8;
	bindings[8].arrayCount = 0;
	bindings[8].name = nullptr;

	_resourceLayout = _renderContext->CreateResourceLayout(bindings, 9);
	_resourceLayout->name = "ModelSampleResourceLayout";
	_resourceLayout->Retain();

	// 리소스 셋 생성
	uint32 zeroOffset = 0;
	OgResourceUsage usages[9];
	
	usages[0].binding = bindings[0];
	usages[0].buffer.handle = &_uniformBuffer;
	usages[0].buffer.offset = &zeroOffset;
	usages[0].buffer.range = &_uniformBuffer->size;
	
	usages[1].binding = bindings[1];
	usages[1].buffer.handle = &_materialUniformBuffer;
	usages[1].buffer.offset = &zeroOffset;
	usages[1].buffer.range = &_materialUniformBuffer->size;
	
	usages[2].binding = bindings[2];
	usages[2].texture.handle = &_defaultWhiteTexture;
	
	usages[3].binding = bindings[3];
	usages[3].texture.handle = &_defaultNormalTexture;
	
	usages[4].binding = bindings[4];
	usages[4].texture.handle = &_defaultWhiteTexture;
	
	usages[5].binding = bindings[5];
	usages[5].texture.handle = &_defaultWhiteTexture;  // emissive
	
	usages[6].binding = bindings[6];
	usages[6].texture.handle = &_defaultWhiteTexture;  // occlusion
	
	usages[7].binding = bindings[7];
	usages[7].texture.handle = &_defaultWhiteTexture;  // sheen color
	
	usages[8].binding = bindings[8];
	usages[8].texture.handle = &_defaultWhiteTexture;  // sheen roughness

	_resourceSet = _renderContext->CreateResourceSet(_resourceLayout, usages, 9);
	_resourceSet->name = "ModelSampleResourceSet";
	_resourceSet->Retain();

	// 파이프라인 생성
	createPipeline();
}

void OgModelSample::destroyResources()
{
	_renderContext->WaitDeviceIdle();

	clearModelData();

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

	if (_materialUniformBuffer)
	{
		_materialUniformBuffer->Release();
		_materialUniformBuffer = nullptr;
	}

	if (_uniformBuffer)
	{
		_uniformBuffer->Release();
		_uniformBuffer = nullptr;
	}
	
	if (_defaultWhiteTexture)
	{
		_defaultWhiteTexture->Release();
		_defaultWhiteTexture = nullptr;
	}
	
	if (_defaultNormalTexture)
	{
		_defaultNormalTexture->Release();
		_defaultNormalTexture = nullptr;
	}
}

void OgModelSample::createDefaultMesh()
{
	clearModelData();

	// 간단한 큐브 메시 생성
	std::vector<Vertex> vertices = {
		// 앞면
		{{-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
		{{ 1.0f, -1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
		{{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
		{{-1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
	};

	std::vector<uint16> indices = {
		0, 1, 2,  2, 3, 0
	};

	// 버텍스 버퍼 생성
	Render::OgBufferHandle* vertexBuffer = _renderContext->CreateBuffer(
		vertices.data(),
		sizeof(Vertex) * vertices.size(),
		Render::OgBufferUsage::VERTEX,
		OgMemoryOption::MAP_MANAGED
	);
	vertexBuffer->Retain();

	// 인덱스 버퍼 생성
	Render::OgBufferHandle* indexBuffer = _renderContext->CreateBuffer(
		indices.data(),
		sizeof(uint16) * indices.size(),
		Render::OgBufferUsage::INDEX,
		OgMemoryOption::MAP_MANAGED
	);
	indexBuffer->Retain();

	// Primitive 생성
	Primitive primitive;
	primitive.vertexBuffer = vertexBuffer;
	primitive.indexBuffer = indexBuffer;
	primitive.indexCount = static_cast<uint32>(indices.size());
	primitive.vertexCount = static_cast<uint32>(vertices.size());
	primitive.hasIndices = true;
	primitive.materialIndex = 0;

	// Mesh 생성
	Mesh mesh;
	mesh.primitives.push_back(primitive);
	mesh.name = "Default Cube";
	_meshes.push_back(mesh);

	// 기본 Material 생성
	Material material;
	material.name = "Default Material";
	material.baseColorFactor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
	material.metallicFactor = 0.0f;
	material.roughnessFactor = 0.5f;
	_materials.push_back(material);

	// Node 생성
	Node node;
	node.meshIndex = 0;
	node.name = "Default Node";
	_nodes.push_back(node);
	_rootNodes.push_back(0);

	_modelLoaded = false;
	_modelCenter = glm::vec3(0.0f);
	_modelRadius = 1.0f;
}

void OgModelSample::createUniformBuffer()
{
	// 초기 변환 행렬 설정
	_uniformData.model = glm::mat4(1.0f);
	_uniformData.normalMatrix = glm::mat4(1.0f);
	
	float aspect = 1.0f;
	if (_renderTargetHeight > 0)
	{
		aspect = static_cast<float>(_renderTargetWidth) / static_cast<float>(_renderTargetHeight);
	}

	if (_useFlyCamera && _camera)
	{
		_camera->SetAspectRatio(aspect);
		_uniformData.view = _camera->GetViewMatrix();
		_uniformData.projection = _camera->GetProjectionMatrix();
		// Vulkan의 클립 공간 조정
		convertProjectionForVulkan(_uniformData.projection);
	}
	else
	{
		_uniformData.view = glm::lookAt(
			glm::vec3(5.0f, 5.0f, 5.0f),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f)
		);
		_uniformData.projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);  // far plane을 1000으로 확장
		// Vulkan의 클립 공간 조정
		convertProjectionForVulkan(_uniformData.projection);
	}

	// 유니폼 버퍼 생성
	_uniformBuffer = _renderContext->CreateBuffer(
		&_uniformData,
		sizeof(UniformData),
		Render::OgBufferUsage::UNIFORM,
		OgMemoryOption::MAP_MANAGED
	);
	_uniformBuffer->Retain();
	
	// Material 유니폼 버퍼 생성
	_materialUniformData = MaterialUniformData{};
	_materialUniformData.baseColorFactor = glm::vec4(1.0f);
	_materialUniformData.metallicFactor = 0.0f;
	_materialUniformData.roughnessFactor = 0.5f;
	_materialUniformData.emissiveFactor = glm::vec3(0.0f);
	_materialUniformData.emissiveStrength = 1.0f;
	_materialUniformData.normalScale = 1.0f;
	_materialUniformData.occlusionStrength = 1.0f;
	// 모든 transform을 identity matrix로 초기화
	_materialUniformData.baseColorTransform = glm::mat4(1.0f);
	_materialUniformData.normalTransform = glm::mat4(1.0f);
	_materialUniformData.metallicRoughnessTransform = glm::mat4(1.0f);
	_materialUniformData.emissiveTransform = glm::mat4(1.0f);
	_materialUniformData.occlusionTransform = glm::mat4(1.0f);
	_materialUniformData.sheenColorTransform = glm::mat4(1.0f);
	_materialUniformData.sheenRoughnessTransform = glm::mat4(1.0f);
	
	_materialUniformBuffer = _renderContext->CreateBuffer(
		&_materialUniformData,
		sizeof(MaterialUniformData),
		Render::OgBufferUsage::UNIFORM,
		OgMemoryOption::MAP_MANAGED
	);
	_materialUniformBuffer->Retain();
}

void OgModelSample::updateUniformBuffer()
{
	// 카메라 사용 시 뷰/프로젝션 행렬 업데이트
	if (_useFlyCamera && _camera)
	{
		_uniformData.view = _camera->GetViewMatrix();
		_uniformData.projection = _camera->GetProjectionMatrix();
		// Vulkan의 클립 공간 조정
		convertProjectionForVulkan(_uniformData.projection);
	}

	// 유니폼 버퍼 업데이트는 렌더링 시에 수행
}

void OgModelSample::createShaders()
{
	// glTF 모델을 렌더링하기 위한 PBR 기반 셰이더
	const char* vertexShaderGLSL = R"(
		#version 450
		
		layout(location = 0) in vec3 inPosition;
		layout(location = 1) in vec3 inNormal;
		layout(location = 2) in vec2 inTexCoord;
		layout(location = 3) in vec4 inTangent;
		
		layout(binding = 0) uniform UniformBufferObject {
			mat4 model;
			mat4 view;
			mat4 proj;
			mat4 normalMatrix;
		} ubo;

		layout(location = 0) out vec3 fragPosition;
		layout(location = 1) out vec3 fragNormal;
		layout(location = 2) out vec2 fragTexCoord;
		layout(location = 3) out vec4 fragTangent;

		void main() {
			vec4 worldPos = ubo.model * vec4(inPosition, 1.0);
			gl_Position = ubo.proj * ubo.view * worldPos;
			
			fragPosition = worldPos.xyz;
			fragNormal = mat3(ubo.normalMatrix) * inNormal;
			fragTexCoord = inTexCoord;
			fragTangent = vec4(mat3(ubo.normalMatrix) * inTangent.xyz, inTangent.w);
		}
	)";

	const char* fragmentShaderGLSL = R"(
		#version 450

		layout(location = 0) in vec3 fragPosition;
		layout(location = 1) in vec3 fragNormal;
		layout(location = 2) in vec2 fragTexCoord;
		layout(location = 3) in vec4 fragTangent;

		layout(location = 0) out vec4 outColor;
		
		layout(binding = 1) uniform MaterialUniforms {
			vec4 baseColorFactor;
			float metallicFactor;
			float roughnessFactor;
			float normalScale;
			float occlusionStrength;
			
			vec3 emissiveFactor;
			float emissiveStrength;
			
			vec3 sheenColorFactor;
			float sheenRoughnessFactor;
			
			float hasBaseColorTexture;
			float hasNormalTexture;
			float hasMetallicRoughnessTexture;
			float hasEmissiveTexture;
			
			float hasOcclusionTexture;
			float hasSheenColorTexture;
			float hasSheenRoughnessTexture;
			float unlit;
			
			mat4 baseColorTransform;
			mat4 normalTransform;
			mat4 metallicRoughnessTransform;
			mat4 emissiveTransform;
			mat4 occlusionTransform;
			mat4 sheenColorTransform;
			mat4 sheenRoughnessTransform;
		} material;
		
		layout(binding = 2) uniform sampler2D baseColorTexture;
		layout(binding = 3) uniform sampler2D normalTexture;
		layout(binding = 4) uniform sampler2D metallicRoughnessTexture;
		layout(binding = 5) uniform sampler2D emissiveTexture;
		layout(binding = 6) uniform sampler2D occlusionTexture;
		layout(binding = 7) uniform sampler2D sheenColorTexture;
		layout(binding = 8) uniform sampler2D sheenRoughnessTexture;

			vec2 transformUV(vec2 uv, mat4 transform) {
				// glTF 2.0 스펙에 따른 정확한 UV 변환
				// 처리 순서: Scale -> Rotation -> Translation
				mat3 transform3x3 = mat3(transform);
				
				// 변환 적용
				vec3 transformedUV = transform3x3 * vec3(uv, 1.0);
				
				// 결과 반환
				return transformedUV.xy;
			}

		// PBR 계산을 위한 상수들
		const float PI = 3.14159265359;
		const float EPSILON = 1e-6;

		// Fresnel-Schlick approximation
		vec3 fresnelSchlick(float cosTheta, vec3 F0) {
			return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
		}

		// Fresnel-Schlick approximation with roughness
		vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
			return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
		}

		// GGX/Trowbridge-Reitz normal distribution function
		float distributionGGX(vec3 N, vec3 H, float roughness) {
			float a = roughness * roughness;
			float a2 = a * a;
			float NdotH = max(dot(N, H), 0.0);
			float NdotH2 = NdotH * NdotH;

			float num = a2;
			float denom = (NdotH2 * (a2 - 1.0) + 1.0);
			denom = PI * denom * denom;

			return num / max(denom, EPSILON);
		}

		// Smith's method for geometry shadowing/masking
		float geometrySchlickGGX(float NdotV, float roughness) {
			float r = (roughness + 1.0);
			float k = (r * r) / 8.0;

			float num = NdotV;
			float denom = NdotV * (1.0 - k) + k;

			return num / max(denom, EPSILON);
		}

		float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
			float NdotV = max(dot(N, V), 0.0);
			float NdotL = max(dot(N, L), 0.0);
			float ggx2 = geometrySchlickGGX(NdotV, roughness);
			float ggx1 = geometrySchlickGGX(NdotL, roughness);

			return ggx1 * ggx2;
		}

		// Charlie sheen distribution (approximation)
		float distributionCharlie(float sheenRoughness, float NdotH) {
			sheenRoughness = max(sheenRoughness, 0.000001); // avoid division by zero
			float invR = 1.0 / sheenRoughness;
			float cos2h = NdotH * NdotH;
			float sin2h = 1.0 - cos2h;
			return (2.0 + invR) * pow(sin2h, invR * 0.5) / (2.0 * PI);
		}

		// Visibility function for sheen
		float visibilityAshikhmin(float NdotL, float NdotV) {
			return 1.0 / (4.0 * (NdotL + NdotV - NdotL * NdotV));
		}

		// Sheen BRDF
		vec3 sheenBRDF(vec3 sheenColor, float sheenRoughness, float NdotL, float NdotV, float NdotH) {
			float sheenDistribution = distributionCharlie(sheenRoughness, NdotH);
			float sheenVisibility = visibilityAshikhmin(NdotL, NdotV);
			return sheenColor * sheenDistribution * sheenVisibility;
		}

		// ACES tone mapping
		vec3 ACESFilm(vec3 x) {
			float a = 2.51;
			float b = 0.03;
			float c = 2.43;
			float d = 0.59;
			float e = 0.14;
			return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
		}

		void main() {
			// Unlit material - 간단한 처리
			if (material.unlit > 0.5) {
				vec4 baseColor = material.baseColorFactor;
				if (material.hasBaseColorTexture > 0.5) {
					vec2 uv = transformUV(fragTexCoord, material.baseColorTransform);
					baseColor *= texture(baseColorTexture, uv);
				}
				outColor = baseColor;
				return;
			}

			// === Material Properties ===
			// Base color
			vec4 baseColor = material.baseColorFactor;
			if (material.hasBaseColorTexture > 0.5) {
				vec2 uv = transformUV(fragTexCoord, material.baseColorTransform);
				baseColor *= texture(baseColorTexture, uv);
			}

			// Alpha test
			if (baseColor.a < 0.1) {
				discard;
			}

			// Normal
			vec3 N = normalize(fragNormal);
			if (material.hasNormalTexture > 0.5 && length(fragTangent.xyz) > 0.01) {
				vec3 T = normalize(fragTangent.xyz);
				vec3 B = cross(N, T) * fragTangent.w;
				mat3 TBN = mat3(T, B, N);
				
				vec2 uv = transformUV(fragTexCoord, material.normalTransform);
				vec3 normalMap = texture(normalTexture, uv).xyz * 2.0 - 1.0;
				normalMap.xy *= material.normalScale;
				N = normalize(TBN * normalMap);
			}

			// Metallic and roughness
			float metallic = material.metallicFactor;
			float roughness = material.roughnessFactor;
			if (material.hasMetallicRoughnessTexture > 0.5) {
				vec2 uv = transformUV(fragTexCoord, material.metallicRoughnessTransform);
				vec3 metallicRoughness = texture(metallicRoughnessTexture, uv).rgb;
				metallic *= metallicRoughness.b;
				roughness *= metallicRoughness.g;
			}
			roughness = clamp(roughness, 0.04, 1.0); // Avoid completely smooth surfaces

			// Occlusion
			float occlusion = 1.0;
			if (material.hasOcclusionTexture > 0.5) {
				vec2 uv = transformUV(fragTexCoord, material.occlusionTransform);
				occlusion = texture(occlusionTexture, uv).r;
				occlusion = mix(1.0, occlusion, material.occlusionStrength);
			}

			// Sheen
			vec3 sheenColor = material.sheenColorFactor;
			float sheenRoughness = material.sheenRoughnessFactor;
			if (material.hasSheenColorTexture > 0.5) {
				vec2 uv = transformUV(fragTexCoord, material.sheenColorTransform);
				sheenColor *= texture(sheenColorTexture, uv).rgb;
			}
			if (material.hasSheenRoughnessTexture > 0.5) {
				vec2 uv = transformUV(fragTexCoord, material.sheenRoughnessTransform);
				sheenRoughness *= texture(sheenRoughnessTexture, uv).a;
			}

			// === Lighting Setup ===
			// Simple directional light setup (can be expanded to multiple lights)
			vec3 lightPositions[4] = vec3[](
				vec3(10.0, 10.0, 10.0),
				vec3(-10.0, 10.0, 10.0),
				vec3(10.0, -10.0, 10.0),
				vec3(0.0, 0.0, 10.0)
			);
			vec3 lightColors[4] = vec3[](
				vec3(300.0, 300.0, 300.0), // Main light
				vec3(100.0, 100.0, 150.0), // Fill light
				vec3(150.0, 100.0, 100.0), // Back light
				vec3(50.0, 50.0, 50.0)     // Ambient
			);

			vec3 V = normalize(vec3(0.0, 0.0, 10.0) - fragPosition); // View direction

			// === PBR Calculation ===
			vec3 F0 = vec3(0.04); // Dielectric base reflectance
			F0 = mix(F0, baseColor.rgb, metallic);

			vec3 Lo = vec3(0.0); // Outgoing radiance

			// Calculate radiance for each light
			for (int i = 0; i < 4; ++i) {
				vec3 L = normalize(lightPositions[i] - fragPosition);
				vec3 H = normalize(V + L);
				float distance = length(lightPositions[i] - fragPosition);
				float attenuation = 1.0 / (distance * distance);
				vec3 radiance = lightColors[i] * attenuation;

				// Cook-Torrance BRDF
				float NDF = distributionGGX(N, H, roughness);
				float G = geometrySmith(N, V, L, roughness);
				vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

				vec3 numerator = NDF * G * F;
				float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
				vec3 specular = numerator / max(denominator, EPSILON);

				// Energy conservation
				vec3 kS = F; // Specular contribution
				vec3 kD = vec3(1.0) - kS; // Diffuse contribution
				kD *= 1.0 - metallic; // Metallic surfaces don't have diffuse

				float NdotL = max(dot(N, L), 0.0);
				float NdotV = max(dot(N, V), 0.0);
				float NdotH = max(dot(N, H), 0.0);

				// Add sheen contribution
				vec3 sheenContribution = vec3(0.0);
				if (length(sheenColor) > 0.001) {
					sheenContribution = sheenBRDF(sheenColor, sheenRoughness, NdotL, NdotV, NdotH);
				}

				// Combine diffuse, specular, and sheen
				Lo += (kD * baseColor.rgb / PI + specular + sheenContribution) * radiance * NdotL;
			}

			// Ambient lighting (very simple IBL approximation)
			vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
			vec3 kS = F;
			vec3 kD = 1.0 - kS;
			kD *= 1.0 - metallic;
			
			vec3 irradiance = vec3(0.03); // Very simple ambient
			vec3 diffuse = irradiance * baseColor.rgb;
			
			// Simple specular ambient
			vec3 prefilteredColor = vec3(0.05);
			vec3 specular = prefilteredColor * F;
			
			vec3 ambient = (kD * diffuse + specular) * occlusion;

			// Emissive
			vec3 emissive = material.emissiveFactor * material.emissiveStrength;
			if (material.hasEmissiveTexture > 0.5) {
				vec2 uv = transformUV(fragTexCoord, material.emissiveTransform);
				emissive *= texture(emissiveTexture, uv).rgb;
			}

			// Final color
			vec3 color = ambient + Lo + emissive;

			// Tone mapping and gamma correction
			//color = ACESFilm(color);
			//color = pow(color, vec3(1.0/2.2)); // Gamma correction

			outColor = vec4(color, baseColor.a);
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
	_vertexShader->name = "ModelSampleVertexShader";
	_vertexShader->Retain();

	_fragmentShader = _renderContext->CreateShader(
		OgShaderType::FRAGMENT, 
		reinterpret_cast<const char*>(fragmentSPIRV.data()), 
		fragmentSPIRV.size() * sizeof(uint32_t), 
		"main"
	);
	_fragmentShader->name = "ModelSampleFragmentShader";
	_fragmentShader->Retain();

	OgShaderHandle* handles[]{ _vertexShader, _fragmentShader };
	_program = _renderContext->CreateProgram(handles, 2);
	_program->name = "ModelSampleShaderProgram";
	_program->Retain();
}

void OgModelSample::createPipeline()
{
	OgColorBlendDescriptor cbDesc{};
	cbDesc.attachmentCount = 1;
	cbDesc.attachments[0].blendEnable = false;

	OgRasterizationDescriptor rsDesc{};
	rsDesc.polygonMode = OgPolygonMode::FILL;
	rsDesc.cullMode = OgCullMode::BACK;  // 모든 면을 렌더링 (double-sided 지원)
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
		OgVertexAttributeDescriptor(0, 1, OgVertexFormat::FLOAT3, offsetof(Vertex, normal)),
		OgVertexAttributeDescriptor(0, 2, OgVertexFormat::FLOAT2, offsetof(Vertex, texCoord)),
		OgVertexAttributeDescriptor(0, 3, OgVertexFormat::FLOAT4, offsetof(Vertex, tangent))
	};
	viDesc.attributes = vaDesc;
	viDesc.attributeCount = 4;
	viDesc.layouts = vblDesc;
	viDesc.layoutCount = 1;

	OgPipelineDescriptor pipeDesc{};
	pipeDesc.name = "ModelSamplePipeline";
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

void OgModelSample::createRenderTarget(uint16 width, uint16 height)
{
	_renderTargetWidth = width;
	_renderTargetHeight = height;

	// 샘플러 생성
	OgSamplerInfo samplerInfo{};
	samplerInfo.type = OgSamplerType::TEX_2D;
	samplerInfo.addressU = OgSamplerAddressMode::REPEAT;
	samplerInfo.addressV = OgSamplerAddressMode::REPEAT;
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
	_renderTargetTexture->name = "ModelRenderTarget";
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
	_depthTexture->name = "ModelDepthBuffer";
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
	_renderTargetRenderPass->name = "ModelRenderTargetPass";
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
	_renderTargetFrameBuffer->name = "ModelRenderTargetFrameBuffer";
	_renderTargetFrameBuffer->Retain();
}

void OgModelSample::destroyRenderTarget()
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

bool OgModelSample::loadGLTFModel(const std::string& filePath)
{
	tinygltf::TinyGLTF loader;
	tinygltf::Model model;
	std::string err;
	std::string warn;
	
	bool ret = false;
	std::string extension = filePath.substr(filePath.find_last_of(".") + 1);
	
	if (extension == "glb") {
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, filePath);
	} else {
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, filePath);
	}
	
	if (!warn.empty()) {
		LOGD(OG_ID, "glTF Warning: %s", warn.c_str());
	}
	
	if (!err.empty()) {
		LOGE(OG_ID, "glTF Error: %s", err.c_str());
	}
	
	if (!ret) {
		LOGE(OG_ID, "Failed to load glTF file: %s", filePath.c_str());
		return false;
	}
	
	// 기존 모델 데이터 클리어
	clearModelData();
	
	// Material 로드
	loadMaterials(model);
	
	// Mesh 로드
	for (size_t i = 0; i < model.meshes.size(); i++) {
		loadMesh(model, static_cast<int>(i));
	}
	
	// Node 계층 구조 로드
	_nodes.resize(model.nodes.size());
	for (size_t i = 0; i < model.nodes.size(); i++) {
		const tinygltf::Node& gltfNode = model.nodes[i];
		Node& node = _nodes[i];
		
		node.name = gltfNode.name;
		node.meshIndex = gltfNode.mesh;
		node.children = gltfNode.children;
		
		// Transform 행렬 계산
		if (gltfNode.matrix.size() == 16) {
			// 행렬이 직접 지정된 경우
			for (int j = 0; j < 16; j++) {
				node.matrix[j / 4][j % 4] = static_cast<float>(gltfNode.matrix[j]);
			}
		} else {
			// TRS로부터 행렬 계산
			glm::mat4 T(1.0f);
			glm::mat4 R(1.0f);
			glm::mat4 S(1.0f);
			
			if (gltfNode.translation.size() == 3) {
				T = glm::translate(glm::mat4(1.0f), glm::vec3(
					static_cast<float>(gltfNode.translation[0]),
					static_cast<float>(gltfNode.translation[1]),
					static_cast<float>(gltfNode.translation[2])
				));
			}
			
			if (gltfNode.rotation.size() == 4) {
				glm::quat q(
					static_cast<float>(gltfNode.rotation[3]),
					static_cast<float>(gltfNode.rotation[0]),
					static_cast<float>(gltfNode.rotation[1]),
					static_cast<float>(gltfNode.rotation[2])
				);
				R = glm::mat4_cast(q);
			}
			
			if (gltfNode.scale.size() == 3) {
				S = glm::scale(glm::mat4(1.0f), glm::vec3(
					static_cast<float>(gltfNode.scale[0]),
					static_cast<float>(gltfNode.scale[1]),
					static_cast<float>(gltfNode.scale[2])
				));
			}
			
			node.matrix = T * R * S;
		}
	}
	
	// 루트 노드 찾기
	if (model.scenes.size() > 0) {
		const tinygltf::Scene& scene = model.scenes[model.defaultScene >= 0 ? model.defaultScene : 0];
		_rootNodes = scene.nodes;
	} else {
		// Scene이 없으면 부모가 없는 노드를 루트로 간주
		std::vector<bool> hasParent(model.nodes.size(), false);
		for (const auto& node : model.nodes) {
			for (int child : node.children) {
				hasParent[child] = true;
			}
		}
		for (size_t i = 0; i < hasParent.size(); i++) {
			if (!hasParent[i]) {
				_rootNodes.push_back(static_cast<int>(i));
			}
		}
	}
	
	// 모델 바운딩 박스 계산
	glm::vec3 minBounds(FLT_MAX);
	glm::vec3 maxBounds(-FLT_MAX);
	
	for (const auto& mesh : _meshes) {
		for (const auto& primitive : mesh.primitives) {
			// 버텍스 버퍼에서 바운딩 박스 계산
			// 실제로는 accessor의 min/max를 사용하는 것이 더 효율적
		}
	}
	
	// 모델 중심과 반경 계산 (간단히 처리)
	_modelCenter = glm::vec3(0.0f);
	_modelRadius = 5.0f; // 기본값
	
	_modelLoaded = true;
	return true;
}

void OgModelSample::processNode(const tinygltf::Model& model, const tinygltf::Node& node, int nodeIndex, const glm::mat4& parentMatrix)
{
	// Node 처리 로직은 loadGLTFModel에서 처리
}

void OgModelSample::loadMesh(const tinygltf::Model& model, int meshIndex)
{
	const tinygltf::Mesh& gltfMesh = model.meshes[meshIndex];
	Mesh mesh;
	mesh.name = gltfMesh.name;
	
	for (const auto& gltfPrimitive : gltfMesh.primitives) {
		Primitive primitive;
		primitive.materialIndex = gltfPrimitive.material;
		
		// 정점 데이터 로드
		std::vector<Vertex> vertices;
		
		// Position
		if (gltfPrimitive.attributes.find("POSITION") != gltfPrimitive.attributes.end()) {
			const tinygltf::Accessor& accessor = model.accessors[gltfPrimitive.attributes.at("POSITION")];
			const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
			
			vertices.resize(accessor.count);
			
			const float* positions = reinterpret_cast<const float*>(
				&buffer.data[bufferView.byteOffset + accessor.byteOffset]
			);
			
			for (size_t i = 0; i < accessor.count; i++) {
				vertices[i].position = glm::vec3(
					positions[i * 3 + 0],
					positions[i * 3 + 1],
					positions[i * 3 + 2]
				);
			}
		}
		
		// Normal
		if (gltfPrimitive.attributes.find("NORMAL") != gltfPrimitive.attributes.end()) {
			const tinygltf::Accessor& accessor = model.accessors[gltfPrimitive.attributes.at("NORMAL")];
			const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
			
			const float* normals = reinterpret_cast<const float*>(
				&buffer.data[bufferView.byteOffset + accessor.byteOffset]
			);
			
			for (size_t i = 0; i < accessor.count; i++) {
				vertices[i].normal = glm::vec3(
					normals[i * 3 + 0],
					normals[i * 3 + 1],
					normals[i * 3 + 2]
				);
			}
		} else {
			// 노말이 없으면 기본값
			for (auto& v : vertices) {
				v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
			}
		}
		
		// TexCoord
		if (gltfPrimitive.attributes.find("TEXCOORD_0") != gltfPrimitive.attributes.end()) {
			const tinygltf::Accessor& accessor = model.accessors[gltfPrimitive.attributes.at("TEXCOORD_0")];
			const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
			
			const float* texCoords = reinterpret_cast<const float*>(
				&buffer.data[bufferView.byteOffset + accessor.byteOffset]
			);
			
			for (size_t i = 0; i < accessor.count; i++) 
			{
				vertices[i].texCoord = glm::vec2(
					texCoords[i * 2 + 0],
					texCoords[i * 2 + 1]
				);
			}
		} else {
			// 텍스처 좌표가 없으면 기본값
			for (auto& v : vertices) 
			{
				v.texCoord = glm::vec2(0.0f, 0.0f);
			}
		}
		
		// Tangent
		if (gltfPrimitive.attributes.find("TANGENT") != gltfPrimitive.attributes.end()) {
			const tinygltf::Accessor& accessor = model.accessors[gltfPrimitive.attributes.at("TANGENT")];
			const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
			
			const float* tangents = reinterpret_cast<const float*>(
				&buffer.data[bufferView.byteOffset + accessor.byteOffset]
			);
			
			for (size_t i = 0; i < accessor.count; i++) {
				vertices[i].tangent = glm::vec4(
					tangents[i * 4 + 0],
					tangents[i * 4 + 1],
					tangents[i * 4 + 2],
					tangents[i * 4 + 3]
				);
			}
		} else {
			// Tangent가 없으면 기본값
			for (auto& v : vertices) {
				v.tangent = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
			}
		}
		
		// 버텍스 버퍼 생성
		primitive.vertexBuffer = _renderContext->CreateBuffer(
			vertices.data(),
			sizeof(Vertex) * vertices.size(),
			Render::OgBufferUsage::VERTEX,
			OgMemoryOption::MAP_MANAGED
		);
		primitive.vertexBuffer->Retain();
		primitive.vertexCount = static_cast<uint32>(vertices.size());
		
		// 인덱스 데이터 로드
		if (gltfPrimitive.indices >= 0) {
			const tinygltf::Accessor& accessor = model.accessors[gltfPrimitive.indices];
			const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
			
			primitive.hasIndices = true;
			primitive.indexCount = static_cast<uint32>(accessor.count);
			
			if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) 
			{
				primitive.indexBuffer = _renderContext->CreateBuffer(
					(void*) & buffer.data[bufferView.byteOffset + accessor.byteOffset],
					accessor.count * sizeof(uint16_t),
					Render::OgBufferUsage::INDEX,
					OgMemoryOption::MAP_MANAGED
				);
			} else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
				// uint32 인덱스를 uint16으로 변환 (엔진이 uint16만 지원하는 경우)
				const uint32_t* indices32 = reinterpret_cast<const uint32_t*>(
					&buffer.data[bufferView.byteOffset + accessor.byteOffset]
				);
				std::vector<uint16_t> indices16(accessor.count);
				for (size_t i = 0; i < accessor.count; i++) {
					indices16[i] = static_cast<uint16_t>(indices32[i]);
				}
				primitive.indexBuffer = _renderContext->CreateBuffer(
					indices16.data(),
					indices16.size() * sizeof(uint16_t),
					Render::OgBufferUsage::INDEX,
					OgMemoryOption::MAP_MANAGED
				);
			}
			
			if (primitive.indexBuffer) {
				primitive.indexBuffer->Retain();
			}
		}
		
		mesh.primitives.push_back(primitive);
	}
	
	_meshes.push_back(mesh);
}

void OgModelSample::loadMaterials(const tinygltf::Model& model)
{
	for (const auto& gltfMaterial : model.materials) {
		Material material;
		material.name = gltfMaterial.name;
		
		// Double sided
		material.doubleSided = gltfMaterial.doubleSided;
		
		// Check for unlit extension
		if (gltfMaterial.extensions.find("KHR_materials_unlit") != gltfMaterial.extensions.end()) {
			material.unlit = true;
		}
		
		// PBR metallic roughness
		if (gltfMaterial.values.find("baseColorFactor") != gltfMaterial.values.end()) {
			const tinygltf::Parameter& param = gltfMaterial.values.at("baseColorFactor");
			if (param.number_array.size() >= 4) {
				material.baseColorFactor = glm::vec4(
					static_cast<float>(param.number_array[0]),
					static_cast<float>(param.number_array[1]),
					static_cast<float>(param.number_array[2]),
					static_cast<float>(param.number_array[3])
				);
			}
		}
		
		if (gltfMaterial.values.find("metallicFactor") != gltfMaterial.values.end()) {
			material.metallicFactor = static_cast<float>(gltfMaterial.values.at("metallicFactor").Factor());
		}
		
		if (gltfMaterial.values.find("roughnessFactor") != gltfMaterial.values.end()) {
			material.roughnessFactor = static_cast<float>(gltfMaterial.values.at("roughnessFactor").Factor());
		}
		
		// Emissive
		if (gltfMaterial.additionalValues.find("emissiveFactor") != gltfMaterial.additionalValues.end()) {
			const tinygltf::Parameter& param = gltfMaterial.additionalValues.at("emissiveFactor");
			if (param.number_array.size() >= 3) {
				material.emissiveFactor = glm::vec3(
					static_cast<float>(param.number_array[0]),
					static_cast<float>(param.number_array[1]),
					static_cast<float>(param.number_array[2])
				);
			}
		}
		
		// Emissive strength extension
		if (gltfMaterial.extensions.find("KHR_materials_emissive_strength") != gltfMaterial.extensions.end()) {
			const auto& ext = gltfMaterial.extensions.at("KHR_materials_emissive_strength");
			if (ext.Has("emissiveStrength")) {
				material.emissiveStrength = static_cast<float>(ext.Get("emissiveStrength").GetNumberAsDouble());
			}
		}
		
		// Sheen extension - 완전히 재작성
		if (gltfMaterial.extensions.find("KHR_materials_sheen") != gltfMaterial.extensions.end()) {
			const auto& ext = gltfMaterial.extensions.at("KHR_materials_sheen");
			
			// Sheen color factor
			if (ext.Has("sheenColorFactor") && ext.Get("sheenColorFactor").IsArray()) {
				const auto& colorArray = ext.Get("sheenColorFactor");
				if (colorArray.ArrayLen() >= 3) {
					material.sheenColorFactor = glm::vec3(
						static_cast<float>(colorArray.Get(0).GetNumberAsDouble()),
						static_cast<float>(colorArray.Get(1).GetNumberAsDouble()),
						static_cast<float>(colorArray.Get(2).GetNumberAsDouble())
					);
					LOGD(OG_ID, "Sheen color factor: (%.2f, %.2f, %.2f)", 
						 material.sheenColorFactor.x, material.sheenColorFactor.y, material.sheenColorFactor.z);
				}
			}
			
			// Sheen roughness factor
			if (ext.Has("sheenRoughnessFactor") && ext.Get("sheenRoughnessFactor").IsNumber()) {
				material.sheenRoughnessFactor = static_cast<float>(ext.Get("sheenRoughnessFactor").GetNumberAsDouble());
				LOGD(OG_ID, "Sheen roughness factor: %.2f", material.sheenRoughnessFactor);
			}
			
			// Sheen color texture
			if (ext.Has("sheenColorTexture") && ext.Get("sheenColorTexture").IsObject()) {
				const auto& texInfo = ext.Get("sheenColorTexture");
				
				// 텍스처 인덱스
				if (texInfo.Has("index") && texInfo.Get("index").IsNumber()) {
					int index = static_cast<int>(texInfo.Get("index").GetNumberAsInt());
					material.sheenColorTexture = loadTexture(model, index);
					LOGD(OG_ID, "Loaded sheen color texture with index: %d", index);
				}
				
				// KHR_texture_transform 확장 처리
				if (texInfo.Has("extensions") && texInfo.Get("extensions").IsObject()) {
					const auto& texExtensions = texInfo.Get("extensions");
					if (texExtensions.Has("KHR_texture_transform") && texExtensions.Get("KHR_texture_transform").IsObject()) {
						const auto& transform = texExtensions.Get("KHR_texture_transform");
						
						// Scale 파싱
						if (transform.Has("scale") && transform.Get("scale").IsArray()) {
							const auto& scaleArray = transform.Get("scale");
							if (scaleArray.ArrayLen() >= 2) {
								material.sheenColorTransform.scale.x = static_cast<float>(scaleArray.Get(0).GetNumberAsDouble());
								material.sheenColorTransform.scale.y = static_cast<float>(scaleArray.Get(1).GetNumberAsDouble());
								LOGD(OG_ID, "Sheen color texture scale: (%.2f, %.2f)", 
									 material.sheenColorTransform.scale.x, material.sheenColorTransform.scale.y);
							}
						}
						
						// Offset 파싱
						if (transform.Has("offset") && transform.Get("offset").IsArray()) {
							const auto& offsetArray = transform.Get("offset");
							if (offsetArray.ArrayLen() >= 2) {
								material.sheenColorTransform.offset.x = static_cast<float>(offsetArray.Get(0).GetNumberAsDouble());
								material.sheenColorTransform.offset.y = static_cast<float>(offsetArray.Get(1).GetNumberAsDouble());
								LOGD(OG_ID, "Sheen color texture offset: (%.2f, %.2f)", 
									 material.sheenColorTransform.offset.x, material.sheenColorTransform.offset.y);
							}
						}
						
						// Rotation 파싱
						if (transform.Has("rotation") && transform.Get("rotation").IsNumber()) {
							material.sheenColorTransform.rotation = static_cast<float>(transform.Get("rotation").GetNumberAsDouble());
							LOGD(OG_ID, "Sheen color texture rotation: %.2f", material.sheenColorTransform.rotation);
						}
					}
				}
			}
			
			// Sheen roughness texture - 동일한 방식으로 처리
			if (ext.Has("sheenRoughnessTexture") && ext.Get("sheenRoughnessTexture").IsObject()) {
				const auto& texInfo = ext.Get("sheenRoughnessTexture");
				
				// 텍스처 인덱스
				if (texInfo.Has("index") && texInfo.Get("index").IsNumber()) {
					int index = static_cast<int>(texInfo.Get("index").GetNumberAsInt());
					material.sheenRoughnessTexture = loadTexture(model, index);
					LOGD(OG_ID, "Loaded sheen roughness texture with index: %d", index);
				}
				
				// KHR_texture_transform 확장 처리
				if (texInfo.Has("extensions") && texInfo.Get("extensions").IsObject()) {
					const auto& texExtensions = texInfo.Get("extensions");
					if (texExtensions.Has("KHR_texture_transform") && texExtensions.Get("KHR_texture_transform").IsObject()) {
						const auto& transform = texExtensions.Get("KHR_texture_transform");
						
						// Scale 파싱
						if (transform.Has("scale") && transform.Get("scale").IsArray()) {
							const auto& scaleArray = transform.Get("scale");
							if (scaleArray.ArrayLen() >= 2) {
								material.sheenRoughnessTransform.scale.x = static_cast<float>(scaleArray.Get(0).GetNumberAsDouble());
								material.sheenRoughnessTransform.scale.y = static_cast<float>(scaleArray.Get(1).GetNumberAsDouble());
								LOGD(OG_ID, "Sheen roughness texture scale: (%.2f, %.2f)", 
									 material.sheenRoughnessTransform.scale.x, material.sheenRoughnessTransform.scale.y);
							}
						}
						
						// Offset과 Rotation도 동일하게 처리
						if (transform.Has("offset") && transform.Get("offset").IsArray()) {
							const auto& offsetArray = transform.Get("offset");
							if (offsetArray.ArrayLen() >= 2) {
								material.sheenRoughnessTransform.offset.x = static_cast<float>(offsetArray.Get(0).GetNumberAsDouble());
								material.sheenRoughnessTransform.offset.y = static_cast<float>(offsetArray.Get(1).GetNumberAsDouble());
							}
						}
						
						if (transform.Has("rotation") && transform.Get("rotation").IsNumber()) {
							material.sheenRoughnessTransform.rotation = static_cast<float>(transform.Get("rotation").GetNumberAsDouble());
						}
					}
				}
			}
		}
		
		// PBR textures - 기존 방식을 더 간소화
		if (gltfMaterial.pbrMetallicRoughness.baseColorTexture.index >= 0) {
			const auto& texInfo = gltfMaterial.pbrMetallicRoughness.baseColorTexture;
			material.baseColorTexture = loadTexture(model, texInfo.index);
			
			// KHR_texture_transform 처리 - 기존 tinygltf 방식 활용
			auto extIt = texInfo.extensions.find("KHR_texture_transform");
			if (extIt != texInfo.extensions.end()) {
				material.baseColorTransform = loadTextureTransform(extIt->second);
				LOGD(OG_ID, "Base color texture transform - scale: (%.2f, %.2f)", 
					 material.baseColorTransform.scale.x, material.baseColorTransform.scale.y);
			}
		}
		
		// 다른 텍스처들도 동일한 패턴으로 처리
		if (gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0) {
			const auto& texInfo = gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture;
			material.metallicRoughnessTexture = loadTexture(model, texInfo.index);
			auto extIt = texInfo.extensions.find("KHR_texture_transform");
			if (extIt != texInfo.extensions.end()) {
				material.metallicRoughnessTransform = loadTextureTransform(extIt->second);
				LOGD(OG_ID, "Metallic roughness texture transform - scale: (%.2f, %.2f)", 
					 material.metallicRoughnessTransform.scale.x, material.metallicRoughnessTransform.scale.y);
			}
		}
		
		// Normal texture
		if (gltfMaterial.normalTexture.index >= 0) {
			const auto& texInfo = gltfMaterial.normalTexture;
			material.normalTexture = loadTexture(model, texInfo.index);
			material.normalScale = static_cast<float>(texInfo.scale);
			auto extIt = texInfo.extensions.find("KHR_texture_transform");
			if (extIt != texInfo.extensions.end()) {
				material.normalTransform = loadTextureTransform(extIt->second);
				LOGD(OG_ID, "Normal texture transform - scale: (%.2f, %.2f)", 
					 material.normalTransform.scale.x, material.normalTransform.scale.y);
			}
		}
		
		// Emissive texture
		if (gltfMaterial.emissiveTexture.index >= 0) {
			const auto& texInfo = gltfMaterial.emissiveTexture;
			material.emissiveTexture = loadTexture(model, texInfo.index);
			auto extIt = texInfo.extensions.find("KHR_texture_transform");
			if (extIt != texInfo.extensions.end()) {
				material.emissiveTransform = loadTextureTransform(extIt->second);
				LOGD(OG_ID, "Emissive texture transform - scale: (%.2f, %.2f)", 
					 material.emissiveTransform.scale.x, material.emissiveTransform.scale.y);
			}
		}
		
		// Occlusion texture
		if (gltfMaterial.occlusionTexture.index >= 0) {
			const auto& texInfo = gltfMaterial.occlusionTexture;
			material.occlusionTexture = loadTexture(model, texInfo.index);
			material.occlusionStrength = static_cast<float>(texInfo.strength);
			auto extIt = texInfo.extensions.find("KHR_texture_transform");
			if (extIt != texInfo.extensions.end()) {
				material.occlusionTransform = loadTextureTransform(extIt->second);
				LOGD(OG_ID, "Occlusion texture transform - scale: (%.2f, %.2f)", 
					 material.occlusionTransform.scale.x, material.occlusionTransform.scale.y);
			}
		}
		
		_materials.push_back(material);
	}
	
	// 기본 material이 없으면 생성
	if (_materials.empty()) {
		Material defaultMaterial;
		defaultMaterial.name = "Default";
		defaultMaterial.baseColorFactor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
		defaultMaterial.metallicFactor = 0.0f;
		defaultMaterial.roughnessFactor = 0.5f;
		_materials.push_back(defaultMaterial);
	}
}

Render::OgTextureHandle* OgModelSample::loadTexture(const tinygltf::Model& model, int textureIndex)
{
	if (textureIndex < 0 || textureIndex >= static_cast<int>(model.textures.size())) {
		return nullptr;
	}
	
	const tinygltf::Texture& texture = model.textures[textureIndex];
	const tinygltf::Image& image = model.images[texture.source];
	
	OgTextureInfo texInfo{};
	texInfo.type = OgTextureType::TEX_2D;
	texInfo.extent.width = image.width;
	texInfo.extent.height = image.height;
	
	texInfo.usage = OgTextureUsage::SAMPLED| OgTextureUsage::STAGING;
	texInfo.isGenerateMipmaps = false;
	
	// 포맷 결정
	if (image.component == 3) 
	{
		texInfo.format = OgPixelFormat::R8G8B8_UNORM;
		texInfo.byteSize = image.width * image.height * 3;
	} else if (image.component == 4) 
	{
		texInfo.format = OgPixelFormat::R8G8B8A8_UNORM;
		texInfo.byteSize = image.width * image.height * 4;
	} else {
		LOGD(OG_ID, "Unsupported image component count: %d", image.component);
		return nullptr;
	}
	
	// 샘플러 설정
	OgSamplerInfo samplerInfo{};
	samplerInfo.type = OgSamplerType::TEX_2D;
	samplerInfo.addressU = OgSamplerAddressMode::REPEAT;
	samplerInfo.addressV = OgSamplerAddressMode::REPEAT;
	samplerInfo.magFilter = OgFilter::LINEAR;
	samplerInfo.minFilter = OgFilter::LINEAR;
	samplerInfo.mipmapMode = OgSamplerMipmapMode::LINEAR;
	
	// glTF 샘플러 설정이 있으면 적용
	if (texture.sampler >= 0) {
		const tinygltf::Sampler& gltfSampler = model.samplers[texture.sampler];
		
		// Wrap modes
		auto convertWrapMode = [](int mode) {
			switch (mode) {
				case TINYGLTF_TEXTURE_WRAP_REPEAT: return OgSamplerAddressMode::REPEAT;
				case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE: return OgSamplerAddressMode::CLAMP_TO_EDGE;
				case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT: return OgSamplerAddressMode::MIRRORED_REPEAT;
				default: return OgSamplerAddressMode::REPEAT;
			}
		};
		
		samplerInfo.addressU = convertWrapMode(gltfSampler.wrapS);
		samplerInfo.addressV = convertWrapMode(gltfSampler.wrapT);
		
		// Filter modes
		if (gltfSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST) {
			samplerInfo.magFilter = OgFilter::NEAREST;
		}
		
		if (gltfSampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST ||
			gltfSampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST) {
			samplerInfo.minFilter = OgFilter::NEAREST;
		}
	}
	
	void* imageData = const_cast<unsigned char*>(image.image.data());
	OgTextureHandle* textureHandle = _renderContext->CreateTexture(
		&imageData,
		texInfo,
		_renderContext->CreateSampler(samplerInfo)
	);
	
	if (textureHandle) {
		textureHandle->Retain();
	}
	
	return textureHandle;
}

OgModelSample::TextureTransform OgModelSample::loadTextureTransform(const tinygltf::Value& extensionsValue)
{
	TextureTransform transform;
	
	if (!extensionsValue.IsObject()) {
		LOGD(OG_ID, "Extensions value is not an object");
		return transform;
	}
	
	// KHR_texture_transform 확장 찾기
	const tinygltf::Value* transformExt = nullptr;
	
	// 직접 KHR_texture_transform 객체인 경우 (기존 호출 방식)
	if (extensionsValue.Has("offset") || extensionsValue.Has("rotation") || extensionsValue.Has("scale")) {
		transformExt = &extensionsValue;
		LOGD(OG_ID, "Direct KHR_texture_transform object detected");
	}
	// extensions 객체에서 KHR_texture_transform을 찾는 경우 (새로운 방식)
	else if (extensionsValue.Has("KHR_texture_transform")) {
		transformExt = &extensionsValue.Get("KHR_texture_transform");
		LOGD(OG_ID, "Found KHR_texture_transform in extensions object");
	}
	else {
		LOGD(OG_ID, "KHR_texture_transform extension not found");
		return transform;
	}
	
	if (!transformExt->IsObject()) {
		LOGD(OG_ID, "KHR_texture_transform is not an object");
		return transform;
	}
	
	// offset 파싱
	if (transformExt->Has("offset") && transformExt->Get("offset").IsArray()) {
		const auto& offsetArray = transformExt->Get("offset");
		if (offsetArray.ArrayLen() >= 2) {
			transform.offset.x = static_cast<float>(offsetArray.Get(0).GetNumberAsDouble());
			transform.offset.y = static_cast<float>(offsetArray.Get(1).GetNumberAsDouble());
			LOGD(OG_ID, "Texture transform offset: (%.2f, %.2f)", transform.offset.x, transform.offset.y);
		}
	}
	
	// rotation 파싱
	if (transformExt->Has("rotation") && transformExt->Get("rotation").IsNumber()) {
		transform.rotation = static_cast<float>(transformExt->Get("rotation").GetNumberAsDouble());
		LOGD(OG_ID, "Texture transform rotation: %.2f", transform.rotation);
	}
	
	// scale 파싱
	if (transformExt->Has("scale") && transformExt->Get("scale").IsArray()) {
		const auto& scaleArray = transformExt->Get("scale");
		if (scaleArray.ArrayLen() >= 2) {
			transform.scale.x = static_cast<float>(scaleArray.Get(0).GetNumberAsDouble());
			transform.scale.y = static_cast<float>(scaleArray.Get(1).GetNumberAsDouble());
			LOGD(OG_ID, "Texture transform scale: (%.2f, %.2f)", transform.scale.x, transform.scale.y);
		}
	}
	
	return transform;
}

void OgModelSample::clearModelData()
{
	// 텍스처 해제
	for (auto& material : _materials) {
		if (material.baseColorTexture) {
			material.baseColorTexture->Release();
		}
		if (material.normalTexture) {
			material.normalTexture->Release();
		}
		if (material.metallicRoughnessTexture) {
			material.metallicRoughnessTexture->Release();
		}
		if (material.emissiveTexture) {
			material.emissiveTexture->Release();
		}
		if (material.occlusionTexture) {
			material.occlusionTexture->Release();
		}
		if (material.sheenColorTexture) {
			material.sheenColorTexture->Release();
		}
		if (material.sheenRoughnessTexture) {
			material.sheenRoughnessTexture->Release();
		}
	}
	
	// 버퍼 해제
	for (auto& mesh : _meshes) {
		for (auto& primitive : mesh.primitives) {
			if (primitive.vertexBuffer) {
				primitive.vertexBuffer->Release();
			}
			if (primitive.indexBuffer) {
				primitive.indexBuffer->Release();
			}
		}
	}
	
	_meshes.clear();
	_nodes.clear();
	_materials.clear();
	_rootNodes.clear();
	_modelLoaded = false;
}

void OgModelSample::renderNode(Render::OgCommandEncoderHandle* encoder, int nodeIndex, const glm::mat4& parentMatrix)
{
	if (nodeIndex < 0 || nodeIndex >= static_cast<int>(_nodes.size())) {
		return;
	}
	
	const Node& node = _nodes[nodeIndex];
	glm::mat4 nodeMatrix = parentMatrix * node.matrix;
	
	// 이 노드에 메시가 있으면 렌더링
	if (node.meshIndex >= 0 && node.meshIndex < static_cast<int>(_meshes.size())) 
	{
		renderMesh(encoder, _meshes[node.meshIndex], nodeMatrix);
	}
	
	// 자식 노드들 렌더링
	for (int childIndex : node.children) {
		renderNode(encoder, childIndex, nodeMatrix);
	}
}

void OgModelSample::renderMesh(Render::OgCommandEncoderHandle* encoder, const Mesh& mesh, const glm::mat4& modelMatrix)
{
	// 유니폼 버퍼 업데이트
	_uniformData.model = modelMatrix;
	_uniformData.normalMatrix = glm::transpose(glm::inverse(modelMatrix));
	
	void* mappedData = _renderContext->MapBuffer(_uniformBuffer, sizeof(UniformData));
	if (mappedData) {
		memcpy(mappedData, &_uniformData, sizeof(UniformData));
		_renderContext->UnmapBuffer(_uniformBuffer);
	}
	
	// 각 primitive 렌더링
	for (const auto& primitive : mesh.primitives) 
	{
		// Material 설정
		if (primitive.materialIndex >= 0 && primitive.materialIndex < static_cast<int>(_materials.size())) 
		{
		const Material& material = _materials[primitive.materialIndex];
		
		// Material 유니폼 업데이트
		_materialUniformData.baseColorFactor = material.baseColorFactor;
		_materialUniformData.metallicFactor = material.metallicFactor;
		_materialUniformData.roughnessFactor = material.roughnessFactor;
		_materialUniformData.normalScale = material.normalScale;
		_materialUniformData.occlusionStrength = material.occlusionStrength;
		_materialUniformData.emissiveFactor = material.emissiveFactor;
		_materialUniformData.emissiveStrength = material.emissiveStrength;
		_materialUniformData.sheenColorFactor = material.sheenColorFactor;
			_materialUniformData.sheenRoughnessFactor = material.sheenRoughnessFactor;
			_materialUniformData.hasBaseColorTexture = material.baseColorTexture ? 1.0f : 0.0f;
			_materialUniformData.hasNormalTexture = material.normalTexture ? 1.0f : 0.0f;
			_materialUniformData.hasMetallicRoughnessTexture = material.metallicRoughnessTexture ? 1.0f : 0.0f;
			_materialUniformData.hasEmissiveTexture = material.emissiveTexture ? 1.0f : 0.0f;
			_materialUniformData.hasOcclusionTexture = material.occlusionTexture ? 1.0f : 0.0f;
			_materialUniformData.hasSheenColorTexture = material.sheenColorTexture ? 1.0f : 0.0f;
			_materialUniformData.hasSheenRoughnessTexture = material.sheenRoughnessTexture ? 1.0f : 0.0f;
			_materialUniformData.unlit = material.unlit ? 1.0f : 0.0f;
			
			// Texture transforms - 3x3 행렬을 4x4로 올바르게 변환
			auto convertMat3ToMat4 = [](const glm::mat3& mat3) -> glm::mat4 {
				glm::mat4 mat4(1.0f);
				// 3x3 행렬을 4x4의 왼쪽 상단 3x3 영역에 복사
				mat4[0][0] = mat3[0][0]; mat4[0][1] = mat3[0][1]; mat4[0][2] = mat3[0][2];
				mat4[1][0] = mat3[1][0]; mat4[1][1] = mat3[1][1]; mat4[1][2] = mat3[1][2];
				mat4[2][0] = mat3[2][0]; mat4[2][1] = mat3[2][1]; mat4[2][2] = mat3[2][2];
				// 나머지는 단위 행렬로 설정
				mat4[3][3] = 1.0f;
				return mat4;
			};
			
			_materialUniformData.baseColorTransform = convertMat3ToMat4(material.baseColorTransform.GetTransformMatrix());
			_materialUniformData.normalTransform = convertMat3ToMat4(material.normalTransform.GetTransformMatrix());
			_materialUniformData.metallicRoughnessTransform = convertMat3ToMat4(material.metallicRoughnessTransform.GetTransformMatrix());
			_materialUniformData.emissiveTransform = convertMat3ToMat4(material.emissiveTransform.GetTransformMatrix());
			_materialUniformData.occlusionTransform = convertMat3ToMat4(material.occlusionTransform.GetTransformMatrix());
			_materialUniformData.sheenColorTransform = convertMat3ToMat4(material.sheenColorTransform.GetTransformMatrix());
			_materialUniformData.sheenRoughnessTransform = convertMat3ToMat4(material.sheenRoughnessTransform.GetTransformMatrix());
			
			// 디버깅: Sheen 텍스처 변환 행렬 로그
			if (material.sheenColorTexture) {
				glm::mat3 sheenTransform = material.sheenColorTransform.GetTransformMatrix();
				LOGD(OG_ID, "Sheen Color Transform Matrix:");
				LOGD(OG_ID, "[%.2f, %.2f, %.2f]", sheenTransform[0][0], sheenTransform[0][1], sheenTransform[0][2]);
				LOGD(OG_ID, "[%.2f, %.2f, %.2f]", sheenTransform[1][0], sheenTransform[1][1], sheenTransform[1][2]);
				LOGD(OG_ID, "[%.2f, %.2f, %.2f]", sheenTransform[2][0], sheenTransform[2][1], sheenTransform[2][2]);
				LOGD(OG_ID, "Scale: (%.2f, %.2f), Offset: (%.2f, %.2f), Rotation: %.2f", 
					 material.sheenColorTransform.scale.x, material.sheenColorTransform.scale.y,
					 material.sheenColorTransform.offset.x, material.sheenColorTransform.offset.y,
					 material.sheenColorTransform.rotation);
			}
			
			void* materialMapped = _renderContext->MapBuffer(_materialUniformBuffer, sizeof(MaterialUniformData));
			if (materialMapped) 
			{
				memcpy(materialMapped, &_materialUniformData, sizeof(MaterialUniformData));
				_renderContext->UnmapBuffer(_materialUniformBuffer);
			}
			
			// 텍스처 바인딩을 위한 리소스 셋 업데이트
			if (material.baseColorTexture || material.normalTexture || material.metallicRoughnessTexture ||
				material.emissiveTexture || material.occlusionTexture || material.sheenColorTexture || material.sheenRoughnessTexture) 
			{
				uint32 zeroOffset = 0;
				OgResourceUsage usages[9];
				
				// 유니폼 버퍼들
				usages[0].binding.type = OgResourceType::UNIFORM_BUFFER;
				usages[0].binding.stage = OgShaderType::VERTEX;
				usages[0].binding.binding = 0;
				usages[0].buffer.handle = &_uniformBuffer;
				usages[0].buffer.offset = &zeroOffset;
				usages[0].buffer.range = &_uniformBuffer->size;
				
				usages[1].binding.type = OgResourceType::UNIFORM_BUFFER;
				usages[1].binding.stage = OgShaderType::FRAGMENT;
				usages[1].binding.binding = 1;
				usages[1].buffer.handle = &_materialUniformBuffer;
				usages[1].buffer.offset = &zeroOffset;
				usages[1].buffer.range = &_materialUniformBuffer->size;

				OgVector<Render::OgTextureHandle*> baseTexs;
				if (material.baseColorTexture)
				{
					baseTexs.Add(material.baseColorTexture);
				}
				else 
				{
					baseTexs.Add(_defaultWhiteTexture);
				}

				OgVector<Render::OgTextureHandle*> normalTexs;
				if (material.normalTexture)
				{
					normalTexs.Add(material.normalTexture);
				}
				else
				{
					normalTexs.Add(_defaultNormalTexture);
				}

				OgVector<Render::OgTextureHandle*> metallicRoughnessTexs;
				if (material.metallicRoughnessTexture)
				{
					metallicRoughnessTexs.Add(material.metallicRoughnessTexture);
				}
				else
				{
					metallicRoughnessTexs.Add(_defaultWhiteTexture);
				}

				OgVector<Render::OgTextureHandle*> emissiveTexs;
				emissiveTexs.Add(material.emissiveTexture ? material.emissiveTexture : _defaultWhiteTexture);
				
				OgVector<Render::OgTextureHandle*> occlusionTexs;
				occlusionTexs.Add(material.occlusionTexture ? material.occlusionTexture : _defaultWhiteTexture);
				
				OgVector<Render::OgTextureHandle*> sheenColorTexs;
				sheenColorTexs.Add(material.sheenColorTexture ? material.sheenColorTexture : _defaultWhiteTexture);
				
				OgVector<Render::OgTextureHandle*> sheenRoughnessTexs;
				sheenRoughnessTexs.Add(material.sheenRoughnessTexture ? material.sheenRoughnessTexture : _defaultWhiteTexture);

				// 텍스처들
				usages[2].binding.type = OgResourceType::COMBINED_IMAGE_SAMPLER;
				usages[2].binding.stage = OgShaderType::FRAGMENT;
				usages[2].binding.binding = 2;
				usages[2].texture.handle = baseTexs.Data();
				
				usages[3].binding.type = OgResourceType::COMBINED_IMAGE_SAMPLER;
				usages[3].binding.stage = OgShaderType::FRAGMENT;
				usages[3].binding.binding = 3;
				usages[3].texture.handle = normalTexs.Data();
				
				usages[4].binding.type = OgResourceType::COMBINED_IMAGE_SAMPLER;
				usages[4].binding.stage = OgShaderType::FRAGMENT;
				usages[4].binding.binding = 4;
				usages[4].texture.handle = metallicRoughnessTexs.Data();
				
				usages[5].binding.type = OgResourceType::COMBINED_IMAGE_SAMPLER;
				usages[5].binding.stage = OgShaderType::FRAGMENT;
				usages[5].binding.binding = 5;
				usages[5].texture.handle = emissiveTexs.Data();
				
				usages[6].binding.type = OgResourceType::COMBINED_IMAGE_SAMPLER;
				usages[6].binding.stage = OgShaderType::FRAGMENT;
				usages[6].binding.binding = 6;
				usages[6].texture.handle = occlusionTexs.Data();
				
				usages[7].binding.type = OgResourceType::COMBINED_IMAGE_SAMPLER;
				usages[7].binding.stage = OgShaderType::FRAGMENT;
				usages[7].binding.binding = 7;
				usages[7].texture.handle = sheenColorTexs.Data();
				
				usages[8].binding.type = OgResourceType::COMBINED_IMAGE_SAMPLER;
				usages[8].binding.stage = OgShaderType::FRAGMENT;
				usages[8].binding.binding = 8;
				usages[8].texture.handle = sheenRoughnessTexs.Data();
				
				// 새로운 리소스 셋 생성
				OgResourceSetHandle* tempResourceSet = _renderContext->CreateResourceSet(_resourceLayout, usages, 9);
				tempResourceSet->Retain();
				encoder->BindResourceSet(tempResourceSet);
				tempResourceSet->Release();
			}
			else 
			{
				encoder->BindResourceSet(_resourceSet);
			}
		}
		else 
		{
			// 기본 material 사용
			_materialUniformData = MaterialUniformData{};
			_materialUniformData.baseColorFactor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
			_materialUniformData.metallicFactor = 0.0f;
			_materialUniformData.roughnessFactor = 0.5f;
			_materialUniformData.emissiveFactor = glm::vec3(0.0f);
			_materialUniformData.emissiveStrength = 1.0f;
			_materialUniformData.normalScale = 1.0f;
			_materialUniformData.occlusionStrength = 1.0f;
			// Identity transforms
			_materialUniformData.baseColorTransform = glm::mat4(1.0f);
			_materialUniformData.normalTransform = glm::mat4(1.0f);
			_materialUniformData.metallicRoughnessTransform = glm::mat4(1.0f);
			_materialUniformData.emissiveTransform = glm::mat4(1.0f);
			_materialUniformData.occlusionTransform = glm::mat4(1.0f);
			_materialUniformData.sheenColorTransform = glm::mat4(1.0f);
			_materialUniformData.sheenRoughnessTransform = glm::mat4(1.0f);
			
			void* materialMapped = _renderContext->MapBuffer(_materialUniformBuffer, sizeof(MaterialUniformData));
			if (materialMapped) {
				memcpy(materialMapped, &_materialUniformData, sizeof(MaterialUniformData));
				_renderContext->UnmapBuffer(_materialUniformBuffer);
			}
			
			encoder->BindResourceSet(_resourceSet);
		}
		
		// 버텍스 버퍼 바인딩
		encoder->BindVertexBuffers(&primitive.vertexBuffer, 0, 1);
		
		// 렌더링
		if (primitive.hasIndices && primitive.indexBuffer) 
		{
			encoder->BindIndexBuffer(primitive.indexBuffer, Render::OgIndexType::UINT16);
			encoder->DrawIndexed(0, primitive.indexCount, 1, 0);
		}
		else 
		{
			//encoder->Draw(0, primitive.vertexCount, 1);
		}
	}
}

void OgModelSample::convertProjectionForVulkan(glm::mat4& projection)
{
	// GLM의 기본 프로젝션은 OpenGL을 위한 것이므로 Vulkan용으로 변환
	// 1. Y축 뒤집기 (Vulkan은 Y축이 아래로 향함)
	projection[1][1] *= -1.0f;
	
	// 2. Z 범위 변환: [-1, 1] -> [0, 1] => GLM이 알아서 해서 주석침
	//projection[2][2] = projection[2][2] * 0.5f + 0.5f;
	//projection[2][3] = projection[2][3] * 0.5f;
}

OG_NAMESPACE_SAMPLE_END
