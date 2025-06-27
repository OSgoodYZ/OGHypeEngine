#pragma once
#ifndef _OG_MODEL_SAMPLE_H__
#define _OG_MODEL_SAMPLE_H__

#include "OgSampleBase.h"
#include <memory>
#include <tinygltf/tiny_gltf.h>
#include <unordered_map>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "sample/public/core/util/OgFlyCamera.h"
#include "sample/public/core/util/OgGLTFLoader.h"

OG_NAMESPACE_SAMPLE_BEGIN

/**
 * @brief glTF 2.0 모델을 로드하고 렌더링하는 샘플
 *
 * tinygltf 라이브러리를 사용하여 glTF 2.0 포맷의 3D 모델을 로드하고 렌더링합니다.
 */
class OG_API OgModelSample : public OgSampleBase
{
public:
	// 라이트 데이터 구조체 (Directional Light)
	struct Light
	{
		glm::vec3 position;      // 12 bytes - 라이트 방향 (Directional)
		float intensity;         // 4 bytes
		glm::vec3 color;         // 12 bytes
		float padding;           // 4 bytes (16 바이트 정렬)
	};

	// 라이트 유니폼 데이터
	struct LightUniformData
	{
		Light lights[4];         // 128 bytes (각 라이트 32바이트 * 4)
		int lightCount;          // 4 bytes
		float padding[3];        // 12 bytes (16 바이트 정렬)
	};


	OgModelSample(Render::OgRenderContext* renderContext);
	~OgModelSample() override;

	// OgSampleBase 인터페이스 구현
	void OnInit(Render::OgSwapChain* swapchain) override;
	void OnDestroy() override;
	void OnUpdate(float deltaTime) override;
	void OnRender(Render::OgCommandEncoderHandle* encoder, Render::OgSwapChain* swapchain) override;
	void OnSuspend(Render::OgSwapChain* swapchain) override;
	void OnRestore(Render::OgSwapChain* swapchain) override;
	void OnResize(uint32 width, uint32 height) override;

	// 입력 처리
	void OnMouseButton(int button, int action, int mods);
	void OnMouseMove(double x, double y);
	void OnMouseScroll(double xoffset, double yoffset);
	void OnKeyPress(int key, int action, int mods);

	// 라이트 컨트롤 인터페이스
	bool GetShowLightControls() const { return _showLightControls; }
	void SetShowLightControls(bool show) { _showLightControls = show; }
	LightUniformData& GetLightUniformData() { return _lightUniformData; }
	void UpdateLightUniformBuffer();
	
	// 라이트 기즈모를 위한 행렬 접근
	glm::mat4 GetViewMatrix() const { return _uniformData.view; }
	glm::mat4 GetProjectionMatrix() const { return _uniformData.projection; }
	glm::mat4 GetModelMatrix() const { return _modelUniformData.model; }
	
	// 파일 브라우저 인터페이스
	const std::vector<std::string>& GetAvailableModels() const { return _availableModels; }
	int GetSelectedModelIndex() const { return _selectedModelIndex; }
	const std::string& GetCurrentModelPath() const { return _currentModelPath; }
	const std::string& GetGLTFDirectory() const { return _glTFDirectory; }
	void ScanGLTFDirectory(const std::string& directory);
	void LoadSelectedModel(int index);

	// 렌더 타겟 인터페이스
	Render::OgTextureHandle* GetRenderTargetTexture() const override { return _renderTargetTexture; }
	uint16 GetRenderTargetWidth() const override { return _renderTargetWidth; }
	uint16 GetRenderTargetHeight() const override { return _renderTargetHeight; }

private:
	// OgGLTFLoader의 타입들을 재사용
	using Vertex = OgGLTFLoader::Vertex;
	using Primitive = OgGLTFLoader::Primitive;
	using Mesh = OgGLTFLoader::Mesh;
	using Node = OgGLTFLoader::Node;
	using Material = OgGLTFLoader::Material;
	using TextureTransform = OgGLTFLoader::TextureTransform;

	// 유니폼 버퍼 데이터 (View/Projection과 카메라 위치)
	struct UniformData
	{
		glm::mat4 view;
		glm::mat4 projection;
		glm::vec3 viewPos;
		float padding;
	};
	
	// 모델별 유니폼 데이터 (각 노드마다 개별적으로 사용)
	struct ModelUniformData
	{
		glm::mat4 model;
		glm::mat4 normalMatrix;  // 노말 변환용
	};

	// Material 유니폼 데이터
	struct MaterialUniformData
	{
		// 기본 PBR 속성 (16 bytes aligned)
		glm::vec4 baseColorFactor;           // 16 bytes
		float metallicFactor;                // 4 bytes
		float roughnessFactor;               // 4 bytes
		float normalScale;                   // 4 bytes
		float occlusionStrength;             // 4 bytes
		
		glm::vec3 emissiveFactor;            // 12 bytes
		float emissiveStrength;              // 4 bytes
		
		// Sheen 속성
		glm::vec3 sheenColorFactor;          // 12 bytes
		float sheenRoughnessFactor;          // 4 bytes
		
		// Transmission 속성
		float transmissionFactor;            // 4 bytes
		float hasTransmissionTexture;        // 4 bytes
		float padding1;                      // 4 bytes
		float padding2;                      // 4 bytes
		
		// Volume 속성
		float thicknessFactor;               // 4 bytes
		float attenuationDistance;           // 4 bytes
		float hasThicknessTexture;           // 4 bytes
		float padding3;                      // 4 bytes
		glm::vec3 attenuationColor;          // 12 bytes
		float padding4;                      // 4 bytes
		
		// 텍스처 플래그
		float hasBaseColorTexture;           // 4 bytes
		float hasNormalTexture;              // 4 bytes
		float hasMetallicRoughnessTexture;   // 4 bytes
		float hasEmissiveTexture;            // 4 bytes
		
		float hasOcclusionTexture;           // 4 bytes
		float hasSheenColorTexture;          // 4 bytes
		float hasSheenRoughnessTexture;      // 4 bytes
		float unlit;                         // 4 bytes
		
		// Texture transforms (3x3 matrices packed as 4x4 for alignment)
		glm::mat4 baseColorTransform;        // 64 bytes
		glm::mat4 normalTransform;           // 64 bytes
		glm::mat4 metallicRoughnessTransform;// 64 bytes
		glm::mat4 emissiveTransform;         // 64 bytes
		glm::mat4 occlusionTransform;        // 64 bytes
		glm::mat4 sheenColorTransform;       // 64 bytes
		glm::mat4 sheenRoughnessTransform;   // 64 bytes
		glm::mat4 transmissionTransform;     // 64 bytes
		glm::mat4 thicknessTransform;        // 64 bytes
	};





private:
	// 리소스 관리
	void createResources(uint16 width, uint16 height);
	void destroyResources();
	void createShaders();
	void createPipeline();
	void createRenderTarget(uint16 width, uint16 height);
	void destroyRenderTarget();
	void createDefaultMesh();
	void createUniformBuffer();
	void updateUniformBuffer();

	// glTF 로딩
	bool loadGLTFModel(const std::string& filePath);
	void clearModelData();
	
	// 렌더링
	void renderNode(Render::OgCommandEncoderHandle* encoder, int nodeIndex, const glm::mat4& parentMatrix);
	void renderMesh(Render::OgCommandEncoderHandle* encoder, const Mesh& mesh, const glm::mat4& modelMatrix, int nodeIndex);

	// Vulkan용 프로젝션 행렬 변환
	void convertProjectionForVulkan(glm::mat4& projection);

private:
	// 렌더링 리소스
	Render::OgBufferHandle* _uniformBuffer = nullptr;
	std::unordered_map<int, Render::OgBufferHandle*> _nodeUniformBuffers;  // 노드별 모델 변환 버퍼
	std::unordered_map<int, Render::OgBufferHandle*> _materialUniformBuffers;  // Material별 유니폼 버퍼
	Render::OgBufferHandle* _materialUniformBuffer = nullptr;  // 기본 material용
	Render::OgShaderHandle* _vertexShader = nullptr;
	Render::OgShaderHandle* _fragmentShader = nullptr;
	Render::OgProgramHandle* _program = nullptr;
	Render::OgResourceLayoutHandle* _resourceLayout = nullptr;
	Render::OgPipelineHandle* _pipeline = nullptr;
	Render::OgResourceSetHandle* _resourceSet = nullptr;

	// 렌더 타겟 리소스
	Render::OgTextureHandle* _renderTargetTexture = nullptr;
	Render::OgTextureHandle* _depthTexture = nullptr;
	Render::OgFrameBufferHandle* _renderTargetFrameBuffer = nullptr;
	Render::OgRenderPassHandle* _renderTargetRenderPass = nullptr;

	// 기본 텍스처 (텍스처가 없는 경우 사용)
	Render::OgTextureHandle* _defaultWhiteTexture = nullptr;
	Render::OgTextureHandle* _defaultNormalTexture = nullptr;

	uint16 _renderTargetWidth = 0;
	uint16 _renderTargetHeight = 0;

	// glTF 로더
	std::unique_ptr<OgGLTFLoader> _gltfLoader;
	
	// 로드된 모델 데이터
	OgGLTFLoader::LoadedModel _loadedModel;
	
	// 변환 행렬
	float _rotation = 0.0f;
	UniformData _uniformData;
	ModelUniformData _modelUniformData;  // 현재 렌더링 중인 모델의 변환
	MaterialUniformData _materialUniformData;

	// 카메라
	std::unique_ptr<OgFlyCamera> _camera;
	bool _useFlyCamera = true;

	// 라이트 데이터
	LightUniformData _lightUniformData;
	Render::OgBufferHandle* _lightUniformBuffer = nullptr;

	// ImGui 컨트롤을 위한 변수들
	bool _showLightControls = true;
	
	// 파일 브라우저를 위한 변수들
	std::string _currentModelPath;
	std::string _glTFDirectory;
	std::vector<std::string> _availableModels;
	int _selectedModelIndex = -1;
};

OG_NAMESPACE_SAMPLE_END

#endif // _OG_MODEL_SAMPLE_H__
