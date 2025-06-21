#pragma once
#ifndef _MODEL_SAMPLE_H__
#define _MODEL_SAMPLE_H__

#include "OgSampleBase.h"
#include <memory>
#include <vector>
#include <string>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "sample/public/core/util/OgFlyCamera.h"

// Forward declaration
namespace tinygltf {
    class Model;
    class Node;
}

OG_NAMESPACE_SAMPLE_BEGIN

/**
 * @brief glTF 2.0 모델을 로드하고 렌더링하는 샘플
 *
 * tinygltf 라이브러리를 사용하여 glTF 2.0 포맷의 3D 모델을 로드하고 렌더링합니다.
 */
class OG_API OgModelSample : public OgSampleBase
{
public:
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

	// 렌더 타겟 인터페이스
	Render::OgTextureHandle* GetRenderTargetTexture() const override { return _renderTargetTexture; }
	uint16 GetRenderTargetWidth() const override { return _renderTargetWidth; }
	uint16 GetRenderTargetHeight() const override { return _renderTargetHeight; }

private:
	// 메시 데이터
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texCoord;
		glm::vec4 tangent;  // glTF는 tangent 정보도 포함할 수 있음
	};

	// Primitive 정보
	struct Primitive
	{
		Render::OgBufferHandle* vertexBuffer = nullptr;
		Render::OgBufferHandle* indexBuffer = nullptr;
		uint32 indexCount = 0;
		uint32 vertexCount = 0;
		bool hasIndices = false;
		int materialIndex = -1;
	};

	// Mesh 정보
	struct Mesh
	{
		std::vector<Primitive> primitives;
		std::string name;
	};

	// Node 정보
	struct Node
	{
		glm::mat4 matrix = glm::mat4(1.0f);
		int meshIndex = -1;
		std::vector<int> children;
		std::string name;
	};

	// Material 정보
	struct Material
	{
		glm::vec4 baseColorFactor = glm::vec4(1.0f);
		float metallicFactor = 1.0f;
		float roughnessFactor = 1.0f;
		glm::vec3 emissiveFactor = glm::vec3(0.0f);
		
		Render::OgTextureHandle* baseColorTexture = nullptr;
		Render::OgTextureHandle* normalTexture = nullptr;
		Render::OgTextureHandle* metallicRoughnessTexture = nullptr;
		Render::OgTextureHandle* emissiveTexture = nullptr;
		
		std::string name;
	};

	// 유니폼 버퍼 데이터
	struct UniformData
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 projection;
		glm::mat4 normalMatrix;  // 노말 변환용
	};

	// Material 유니폼 데이터
	struct MaterialUniformData
	{
		glm::vec4 baseColorFactor;
		float metallicFactor;
		float roughnessFactor;
		float padding1;
		float padding2;
		glm::vec3 emissiveFactor;
		float hasBaseColorTexture;
		float hasNormalTexture;
		float hasMetallicRoughnessTexture;
		float hasEmissiveTexture;
		float padding3;
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
	void processNode(const tinygltf::Model& model, const tinygltf::Node& node, int nodeIndex, const glm::mat4& parentMatrix);
	void loadMesh(const tinygltf::Model& model, int meshIndex);
	void loadMaterials(const tinygltf::Model& model);
	Render::OgTextureHandle* loadTexture(const tinygltf::Model& model, int textureIndex);
	void clearModelData();
	
	// 렌더링
	void renderNode(Render::OgCommandEncoderHandle* encoder, int nodeIndex, const glm::mat4& parentMatrix);
	void renderMesh(Render::OgCommandEncoderHandle* encoder, const Mesh& mesh, const glm::mat4& modelMatrix);

	// Vulkan용 프로젝션 행렬 변환
	void convertProjectionForVulkan(glm::mat4& projection);

private:
	// 렌더링 리소스
	Render::OgBufferHandle* _uniformBuffer = nullptr;
	Render::OgBufferHandle* _materialUniformBuffer = nullptr;
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

	// 모델 데이터
	std::vector<Mesh> _meshes;
	std::vector<Node> _nodes;
	std::vector<Material> _materials;
	std::vector<int> _rootNodes;
	
	// 변환 행렬
	float _rotation = 0.0f;
	UniformData _uniformData;
	MaterialUniformData _materialUniformData;

	// 카메라
	std::unique_ptr<OgFlyCamera> _camera;
	bool _useFlyCamera = true;
	
	// 모델 정보
	bool _modelLoaded = false;
	glm::vec3 _modelCenter = glm::vec3(0.0f);
	float _modelRadius = 1.0f;
};

OG_NAMESPACE_SAMPLE_END

#endif // _MODEL_SAMPLE_H__
