#pragma once
#ifndef _OG_GLTF_LOADER_H__
#define _OG_GLTF_LOADER_H__

#include "OgPrecompile.h"
#include "render/OgRenderContext.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

namespace Og::Render
{
	struct OgBufferHandle;
}

// Forward declarations
namespace tinygltf {
	class Model;
	class Node;
	class Value;
}

OG_NAMESPACE_SAMPLE_BEGIN

/**
 * @brief glTF 2.0 모델 로더 유틸리티 클래스
 *
 * tinygltf 라이브러리를 사용하여 glTF 2.0 포맷의 3D 모델을 로드합니다.
 * 모든 glTF 모델에서 공통적으로 사용할 수 있도록 설계되었습니다.
 */
class OG_API OgGLTFLoader
{
public:
	// 공통 데이터 구조체들
	
	/**
	 * @brief 버텍스 데이터
	 */
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texCoord;
		glm::vec4 tangent;  // xyz: tangent direction, w: handedness
	};

	/**
	 * @brief 프리미티브 데이터 (메시의 하위 구성 요소)
	 */
	struct Primitive
	{
		Render::OgBufferHandle* vertexBuffer = nullptr;
		Render::OgBufferHandle* indexBuffer = nullptr;
		uint32 indexCount = 0;
		uint32 vertexCount = 0;
		bool hasIndices = false;
		int materialIndex = -1;
	};

	/**
	 * @brief 메시 데이터
	 */
	struct Mesh
	{
		std::vector<Primitive> primitives;
		std::string name;
	};

	/**
	 * @brief 노드 데이터 (씬 그래프의 노드)
	 */
	struct Node
	{
		glm::mat4 matrix = glm::mat4(1.0f);
		int meshIndex = -1;
		std::vector<int> children;
		std::string name;
	};

	/**
	 * @brief 텍스처 변환 정보 (KHR_texture_transform 확장)
	 */
	struct TextureTransform
	{
		glm::vec2 offset = glm::vec2(0.0f);
		float rotation = 0.0f;
		glm::vec2 scale = glm::vec2(1.0f);
		
		// glTF 2.0 스펙에 맞는 변환 행렬 생성
		glm::mat3 GetTransformMatrix() const;
	};

	/**
	 * @brief 머티리얼 데이터
	 */
	struct Material
	{
		// PBR 기본 속성
		glm::vec4 baseColorFactor = glm::vec4(1.0f);
		float metallicFactor = 0.0f;
		float roughnessFactor = 1.0f;
		glm::vec3 emissiveFactor = glm::vec3(0.0f);
		float emissiveStrength = 1.0f; // KHR_materials_emissive_strength
		
		// 텍스처
		Render::OgTextureHandle* baseColorTexture = nullptr;
		Render::OgTextureHandle* normalTexture = nullptr;
		Render::OgTextureHandle* metallicRoughnessTexture = nullptr;
		Render::OgTextureHandle* emissiveTexture = nullptr;
		Render::OgTextureHandle* occlusionTexture = nullptr;
		
		// Texture transforms (KHR_texture_transform)
		TextureTransform baseColorTransform;
		TextureTransform normalTransform;
		TextureTransform metallicRoughnessTransform;
		TextureTransform emissiveTransform;
		TextureTransform occlusionTransform;
		
		// Sheen 속성 (KHR_materials_sheen)
		glm::vec3 sheenColorFactor = glm::vec3(0.0f);
		float sheenRoughnessFactor = 0.0f;
		Render::OgTextureHandle* sheenColorTexture = nullptr;
		Render::OgTextureHandle* sheenRoughnessTexture = nullptr;
		TextureTransform sheenColorTransform;
		TextureTransform sheenRoughnessTransform;
		
		// Transmission 속성 (KHR_materials_transmission)
		float transmissionFactor = 0.0f;
		Render::OgTextureHandle* transmissionTexture = nullptr;
		TextureTransform transmissionTransform;
		
		// Volume 속성 (KHR_materials_volume)
		float thicknessFactor = 0.0f;
		Render::OgTextureHandle* thicknessTexture = nullptr;
		TextureTransform thicknessTransform;
		float attenuationDistance = FLT_MAX;
		glm::vec3 attenuationColor = glm::vec3(1.0f);
		
		// 추가 속성
		bool doubleSided = false;
		bool unlit = false; // KHR_materials_unlit
		float normalScale = 1.0f;
		float occlusionStrength = 1.0f;
		
		std::string name;
	};

	/**
	 * @brief 로드된 glTF 모델 데이터
	 */
	struct LoadedModel
	{
		std::vector<Mesh> meshes;
		std::vector<Node> nodes;
		std::vector<Material> materials;
		std::vector<int> rootNodes;
		
		// 모델 바운딩 정보
		glm::vec3 center = glm::vec3(0.0f);
		float radius = 1.0f;
		glm::vec3 minBounds = glm::vec3(FLT_MAX);
		glm::vec3 maxBounds = glm::vec3(-FLT_MAX);
		
		bool isLoaded = false;
	};

public:
	/**
	 * @brief 생성자
	 * @param renderContext 렌더 컨텍스트 (버퍼와 텍스처 생성에 사용)
	 */
	explicit OgGLTFLoader(Render::OgRenderContext* renderContext);
	
	/**
	 * @brief 소멸자
	 */
	~OgGLTFLoader();

	/**
	 * @brief glTF 파일 로드
	 * @param filePath 로드할 glTF/glb 파일 경로
	 * @param outModel 로드된 모델 데이터를 저장할 구조체
	 * @return 로드 성공 여부
	 */
	bool LoadModel(const std::string& filePath, LoadedModel& outModel);

	/**
	 * @brief 로드된 모델 데이터 정리
	 * @param model 정리할 모델 데이터
	 */
	void ClearModel(LoadedModel& model);

	/**
	 * @brief 로드된 모델의 월드 변환 행렬 계산
	 * @param model 모델 데이터
	 * @param nodeIndex 노드 인덱스
	 * @param parentMatrix 부모 변환 행렬
	 * @return 월드 변환 행렬
	 */
	static glm::mat4 GetNodeWorldMatrix(const LoadedModel& model, int nodeIndex, const glm::mat4& parentMatrix = glm::mat4(1.0f));

	/**
	 * @brief 에러 메시지 반환
	 * @return 마지막 에러 메시지
	 */
	const std::string& GetLastError() const { return _lastError; }

	/**
	 * @brief 경고 메시지 반환
	 * @return 마지막 경고 메시지
	 */
	const std::string& GetLastWarning() const { return _lastWarning; }

private:
	// 내부 로딩 함수들
	bool loadGLTFFile(const std::string& filePath, tinygltf::Model& model);
	void loadMaterials(const tinygltf::Model& gltfModel, LoadedModel& outModel);
	void loadMeshes(const tinygltf::Model& gltfModel, LoadedModel& outModel);
	void loadNodes(const tinygltf::Model& gltfModel, LoadedModel& outModel);
	void loadScene(const tinygltf::Model& gltfModel, LoadedModel& outModel);
	
	// 메시 로딩
	void loadMesh(const tinygltf::Model& gltfModel, int meshIndex, LoadedModel& outModel);
	
	// 텍스처 로딩
	Render::OgTextureHandle* loadTexture(const tinygltf::Model& model, int textureIndex);
	TextureTransform loadTextureTransform(const tinygltf::Value& extension);
	
	// 바운딩 박스 계산
	void calculateBounds(LoadedModel& model);
	void updateBoundsWithNode(const LoadedModel& model, int nodeIndex, const glm::mat4& parentMatrix, glm::vec3& minBounds, glm::vec3& maxBounds);

private:
	Render::OgRenderContext* _renderContext;
	std::string _lastError;
	std::string _lastWarning;
	
	// 로드된 텍스처 캐시 (같은 텍스처 중복 로드 방지)
	std::unordered_map<int, Render::OgTextureHandle*> _textureCache;
};

OG_NAMESPACE_SAMPLE_END

#endif // _OG_GLTF_LOADER_H__
