#include "OgGLTFLoader.h"
#include "system/OgFileSystem.h"

// tinygltf 헤더 - single header library
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#include "tinygltf/tiny_gltf.h"

#include <cmath>
#include <algorithm>

using namespace std;
using namespace Render;

OG_NAMESPACE_SAMPLE_BEGIN

// TextureTransform 메서드 구현
glm::mat3 OgGLTFLoader::TextureTransform::GetTransformMatrix() const
{
	// glTF 스펙에 따른 올바른 변환 행렬 계산
	// 순서: Scale -> Rotation -> Translation
	
	// Scale matrix
	glm::mat3 S = glm::mat3(1.0f);
	S[0][0] = scale.x;
	S[1][1] = scale.y;
	
	// Rotation matrix (Z축 기준 회전)
	float c = cos(rotation);
	float s = sin(rotation);
	glm::mat3 R = glm::mat3(1.0f);
	R[0][0] = c;  R[0][1] = s;
	R[1][0] = -s; R[1][1] = c;
	
	// Translation matrix
	glm::mat3 T = glm::mat3(1.0f);
	T[2][0] = offset.x;
	T[2][1] = offset.y;
	
	// glTF 스펙: Translation * Rotation * Scale
	return T * R * S;
}

OgGLTFLoader::OgGLTFLoader(Render::OgRenderContext* renderContext)
	: _renderContext(renderContext)
{
	if (!_renderContext)
	{
		LOGE(OG_ID, "OgGLTFLoader: RenderContext is null");
	}
}

OgGLTFLoader::~OgGLTFLoader()
{
	// 텍스처 캐시 정리는 하지 않음 (외부에서 관리)
	_textureCache.clear();
}

bool OgGLTFLoader::LoadModel(const std::string& filePath, LoadedModel& outModel)
{
	// 기존 데이터 클리어
	ClearModel(outModel);
	_textureCache.clear();
	_lastError.clear();
	_lastWarning.clear();

	// tinygltf 모델 로드
	tinygltf::Model gltfModel;
	if (!loadGLTFFile(filePath, gltfModel))
	{
		return false;
	}

	// 모델 데이터 로드
	loadMaterials(gltfModel, outModel);
	loadMeshes(gltfModel, outModel);
	loadNodes(gltfModel, outModel);
	loadScene(gltfModel, outModel);

	// 바운딩 박스 계산
	calculateBounds(outModel);

	outModel.isLoaded = true;
	
	LOGD(OG_ID, "Successfully loaded glTF model: %s", filePath.c_str());
	LOGD(OG_ID, "  - Meshes: %zu", outModel.meshes.size());
	LOGD(OG_ID, "  - Nodes: %zu", outModel.nodes.size());
	LOGD(OG_ID, "  - Materials: %zu", outModel.materials.size());
	LOGD(OG_ID, "  - Root nodes: %zu", outModel.rootNodes.size());
	
	return true;
}

void OgGLTFLoader::ClearModel(LoadedModel& model)
{
	// 텍스처 해제
	for (auto& material : model.materials)
	{
		if (material.baseColorTexture)
		{
			material.baseColorTexture->Release();
			material.baseColorTexture = nullptr;
		}
		if (material.normalTexture)
		{
			material.normalTexture->Release();
			material.normalTexture = nullptr;
		}
		if (material.metallicRoughnessTexture)
		{
			material.metallicRoughnessTexture->Release();
			material.metallicRoughnessTexture = nullptr;
		}
		if (material.emissiveTexture)
		{
			material.emissiveTexture->Release();
			material.emissiveTexture = nullptr;
		}
		if (material.occlusionTexture)
		{
			material.occlusionTexture->Release();
			material.occlusionTexture = nullptr;
		}
		if (material.sheenColorTexture)
		{
			material.sheenColorTexture->Release();
			material.sheenColorTexture = nullptr;
		}
		if (material.sheenRoughnessTexture)
		{
			material.sheenRoughnessTexture->Release();
			material.sheenRoughnessTexture = nullptr;
		}
	}

	// 버퍼 해제
	for (auto& mesh : model.meshes)
	{
		for (auto& primitive : mesh.primitives)
		{
			if (primitive.vertexBuffer)
			{
				primitive.vertexBuffer->Release();
				primitive.vertexBuffer = nullptr;
			}
			if (primitive.indexBuffer)
			{
				primitive.indexBuffer->Release();
				primitive.indexBuffer = nullptr;
			}
		}
	}

	// 컨테이너 클리어
	model.meshes.clear();
	model.nodes.clear();
	model.materials.clear();
	model.rootNodes.clear();
	
	// 바운딩 정보 초기화
	model.center = glm::vec3(0.0f);
	model.radius = 1.0f;
	model.minBounds = glm::vec3(FLT_MAX);
	model.maxBounds = glm::vec3(-FLT_MAX);
	
	model.isLoaded = false;
}

glm::mat4 OgGLTFLoader::GetNodeWorldMatrix(const LoadedModel& model, int nodeIndex, const glm::mat4& parentMatrix)
{
	if (nodeIndex < 0 || nodeIndex >= static_cast<int>(model.nodes.size()))
	{
		return parentMatrix;
	}

	const Node& node = model.nodes[nodeIndex];
	return parentMatrix * node.matrix;
}

bool OgGLTFLoader::loadGLTFFile(const std::string& filePath, tinygltf::Model& model)
{
	tinygltf::TinyGLTF loader;
	
	bool ret = false;
	std::string extension = filePath.substr(filePath.find_last_of(".") + 1);
	
	if (extension == "glb")
	{
		ret = loader.LoadBinaryFromFile(&model, &_lastError, &_lastWarning, filePath);
	}
	else if (extension == "gltf")
	{
		ret = loader.LoadASCIIFromFile(&model, &_lastError, &_lastWarning, filePath);
	}
	else
	{
		_lastError = "Unsupported file extension: " + extension;
		return false;
	}
	
	if (!_lastWarning.empty())
	{
		LOGD(OG_ID, "glTF Warning: %s", _lastWarning.c_str());
	}
	
	if (!_lastError.empty())
	{
		LOGE(OG_ID, "glTF Error: %s", _lastError.c_str());
	}
	
	if (!ret)
	{
		LOGE(OG_ID, "Failed to load glTF file: %s", filePath.c_str());
		return false;
	}
	
	return true;
}

void OgGLTFLoader::loadMaterials(const tinygltf::Model& gltfModel, LoadedModel& outModel)
{
	for (const auto& gltfMaterial : gltfModel.materials)
	{
		Material material;
		material.name = gltfMaterial.name;
		
		// Double sided
		material.doubleSided = gltfMaterial.doubleSided;
		
		// Check for unlit extension
		if (gltfMaterial.extensions.find("KHR_materials_unlit") != gltfMaterial.extensions.end())
		{
			material.unlit = true;
		}
		
		// PBR metallic roughness
		if (gltfMaterial.values.find("baseColorFactor") != gltfMaterial.values.end())
		{
			const tinygltf::Parameter& param = gltfMaterial.values.at("baseColorFactor");
			if (param.number_array.size() >= 4)
			{
				material.baseColorFactor = glm::vec4(
					static_cast<float>(param.number_array[0]),
					static_cast<float>(param.number_array[1]),
					static_cast<float>(param.number_array[2]),
					static_cast<float>(param.number_array[3])
				);
			}
		}
		
		if (gltfMaterial.values.find("metallicFactor") != gltfMaterial.values.end())
		{
			material.metallicFactor = static_cast<float>(gltfMaterial.values.at("metallicFactor").Factor());
		}
		
		if (gltfMaterial.values.find("roughnessFactor") != gltfMaterial.values.end())
		{
			material.roughnessFactor = static_cast<float>(gltfMaterial.values.at("roughnessFactor").Factor());
		}
		
		// Emissive
		if (gltfMaterial.additionalValues.find("emissiveFactor") != gltfMaterial.additionalValues.end())
		{
			const tinygltf::Parameter& param = gltfMaterial.additionalValues.at("emissiveFactor");
			if (param.number_array.size() >= 3)
			{
				material.emissiveFactor = glm::vec3(
					static_cast<float>(param.number_array[0]),
					static_cast<float>(param.number_array[1]),
					static_cast<float>(param.number_array[2])
				);
			}
		}
		
		// Emissive strength extension
		if (gltfMaterial.extensions.find("KHR_materials_emissive_strength") != gltfMaterial.extensions.end())
		{
			const auto& ext = gltfMaterial.extensions.at("KHR_materials_emissive_strength");
			if (ext.Has("emissiveStrength"))
			{
				material.emissiveStrength = static_cast<float>(ext.Get("emissiveStrength").GetNumberAsDouble());
			}
		}
		
		// Sheen extension
		if (gltfMaterial.extensions.find("KHR_materials_sheen") != gltfMaterial.extensions.end())
		{
			const auto& ext = gltfMaterial.extensions.at("KHR_materials_sheen");
			
			// Sheen color factor
			if (ext.Has("sheenColorFactor") && ext.Get("sheenColorFactor").IsArray())
			{
				const auto& colorArray = ext.Get("sheenColorFactor");
				if (colorArray.ArrayLen() >= 3)
				{
					material.sheenColorFactor = glm::vec3(
						static_cast<float>(colorArray.Get(0).GetNumberAsDouble()),
						static_cast<float>(colorArray.Get(1).GetNumberAsDouble()),
						static_cast<float>(colorArray.Get(2).GetNumberAsDouble())
					);
				}
			}
			
			// Sheen roughness factor
			if (ext.Has("sheenRoughnessFactor") && ext.Get("sheenRoughnessFactor").IsNumber())
			{
				material.sheenRoughnessFactor = static_cast<float>(ext.Get("sheenRoughnessFactor").GetNumberAsDouble());
			}
			
			// Sheen color texture
			if (ext.Has("sheenColorTexture") && ext.Get("sheenColorTexture").IsObject())
			{
				const auto& texInfo = ext.Get("sheenColorTexture");
				
				if (texInfo.Has("index") && texInfo.Get("index").IsNumber())
				{
					int index = static_cast<int>(texInfo.Get("index").GetNumberAsInt());
					material.sheenColorTexture = loadTexture(gltfModel, index);
				}
				
				// KHR_texture_transform
				if (texInfo.Has("extensions") && texInfo.Get("extensions").IsObject())
				{
					const auto& texExtensions = texInfo.Get("extensions");
					if (texExtensions.Has("KHR_texture_transform"))
					{
						material.sheenColorTransform = loadTextureTransform(texExtensions.Get("KHR_texture_transform"));
					}
				}
			}
			
			// Sheen roughness texture
			if (ext.Has("sheenRoughnessTexture") && ext.Get("sheenRoughnessTexture").IsObject())
			{
				const auto& texInfo = ext.Get("sheenRoughnessTexture");
				
				if (texInfo.Has("index") && texInfo.Get("index").IsNumber())
				{
					int index = static_cast<int>(texInfo.Get("index").GetNumberAsInt());
					material.sheenRoughnessTexture = loadTexture(gltfModel, index);
				}
				
				// KHR_texture_transform
				if (texInfo.Has("extensions") && texInfo.Get("extensions").IsObject())
				{
					const auto& texExtensions = texInfo.Get("extensions");
					if (texExtensions.Has("KHR_texture_transform"))
					{
						material.sheenRoughnessTransform = loadTextureTransform(texExtensions.Get("KHR_texture_transform"));
					}
				}
			}
		}
		
		// PBR textures
		if (gltfMaterial.pbrMetallicRoughness.baseColorTexture.index >= 0)
		{
			const auto& texInfo = gltfMaterial.pbrMetallicRoughness.baseColorTexture;
			material.baseColorTexture = loadTexture(gltfModel, texInfo.index);
			
			auto extIt = texInfo.extensions.find("KHR_texture_transform");
			if (extIt != texInfo.extensions.end())
			{
				material.baseColorTransform = loadTextureTransform(extIt->second);
			}
		}
		
		if (gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
		{
			const auto& texInfo = gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture;
			material.metallicRoughnessTexture = loadTexture(gltfModel, texInfo.index);
			
			auto extIt = texInfo.extensions.find("KHR_texture_transform");
			if (extIt != texInfo.extensions.end())
			{
				material.metallicRoughnessTransform = loadTextureTransform(extIt->second);
			}
		}
		
		// Normal texture
		if (gltfMaterial.normalTexture.index >= 0)
		{
			const auto& texInfo = gltfMaterial.normalTexture;
			material.normalTexture = loadTexture(gltfModel, texInfo.index);
			material.normalScale = static_cast<float>(texInfo.scale);
			
			auto extIt = texInfo.extensions.find("KHR_texture_transform");
			if (extIt != texInfo.extensions.end())
			{
				material.normalTransform = loadTextureTransform(extIt->second);
			}
		}
		
		// Emissive texture
		if (gltfMaterial.emissiveTexture.index >= 0)
		{
			const auto& texInfo = gltfMaterial.emissiveTexture;
			material.emissiveTexture = loadTexture(gltfModel, texInfo.index);
			
			auto extIt = texInfo.extensions.find("KHR_texture_transform");
			if (extIt != texInfo.extensions.end())
			{
				material.emissiveTransform = loadTextureTransform(extIt->second);
			}
		}
		
		// Occlusion texture
		if (gltfMaterial.occlusionTexture.index >= 0)
		{
			const auto& texInfo = gltfMaterial.occlusionTexture;
			material.occlusionTexture = loadTexture(gltfModel, texInfo.index);
			material.occlusionStrength = static_cast<float>(texInfo.strength);
			
			auto extIt = texInfo.extensions.find("KHR_texture_transform");
			if (extIt != texInfo.extensions.end())
			{
				material.occlusionTransform = loadTextureTransform(extIt->second);
			}
		}
		
		outModel.materials.push_back(material);
	}
	
	// 기본 material이 없으면 생성
	if (outModel.materials.empty())
	{
		Material defaultMaterial;
		defaultMaterial.name = "Default";
		defaultMaterial.baseColorFactor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
		defaultMaterial.metallicFactor = 0.0f;
		defaultMaterial.roughnessFactor = 0.5f;
		outModel.materials.push_back(defaultMaterial);
	}
}

void OgGLTFLoader::loadMeshes(const tinygltf::Model& gltfModel, LoadedModel& outModel)
{
	for (size_t i = 0; i < gltfModel.meshes.size(); i++)
	{
		loadMesh(gltfModel, static_cast<int>(i), outModel);
	}
}

void OgGLTFLoader::loadMesh(const tinygltf::Model& gltfModel, int meshIndex, LoadedModel& outModel)
{
	const tinygltf::Mesh& gltfMesh = gltfModel.meshes[meshIndex];
	Mesh mesh;
	mesh.name = gltfMesh.name;
	
	for (const auto& gltfPrimitive : gltfMesh.primitives)
	{
		Primitive primitive;
		primitive.materialIndex = gltfPrimitive.material;
		
		// 정점 데이터 로드
		std::vector<Vertex> vertices;
		
		// Position
		if (gltfPrimitive.attributes.find("POSITION") != gltfPrimitive.attributes.end())
		{
			const tinygltf::Accessor& accessor = gltfModel.accessors[gltfPrimitive.attributes.at("POSITION")];
			const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
			
			vertices.resize(accessor.count);
			
			const float* positions = reinterpret_cast<const float*>(
				&buffer.data[bufferView.byteOffset + accessor.byteOffset]
			);
			
			size_t stride = accessor.ByteStride(bufferView) ? (accessor.ByteStride(bufferView) / sizeof(float)) : 3;
			
			for (size_t i = 0; i < accessor.count; i++)
			{
				vertices[i].position = glm::vec3(
					positions[i * stride + 0],
					positions[i * stride + 1],
					positions[i * stride + 2]
				);
			}
		}
		
		// Normal
		if (gltfPrimitive.attributes.find("NORMAL") != gltfPrimitive.attributes.end())
		{
			const tinygltf::Accessor& accessor = gltfModel.accessors[gltfPrimitive.attributes.at("NORMAL")];
			const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
			
			const float* normals = reinterpret_cast<const float*>(
				&buffer.data[bufferView.byteOffset + accessor.byteOffset]
			);
			
			size_t stride = accessor.ByteStride(bufferView) ? (accessor.ByteStride(bufferView) / sizeof(float)) : 3;
			
			for (size_t i = 0; i < accessor.count; i++)
			{
				vertices[i].normal = glm::vec3(
					normals[i * stride + 0],
					normals[i * stride + 1],
					normals[i * stride + 2]
				);
			}
		}
		else
		{
			// 노말이 없으면 기본값
			for (auto& v : vertices)
			{
				v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
			}
		}
		
		// TexCoord
		if (gltfPrimitive.attributes.find("TEXCOORD_0") != gltfPrimitive.attributes.end())
		{
			const tinygltf::Accessor& accessor = gltfModel.accessors[gltfPrimitive.attributes.at("TEXCOORD_0")];
			const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
			
			const float* texCoords = reinterpret_cast<const float*>(
				&buffer.data[bufferView.byteOffset + accessor.byteOffset]
			);
			
			size_t stride = accessor.ByteStride(bufferView) ? (accessor.ByteStride(bufferView) / sizeof(float)) : 2;
			
			for (size_t i = 0; i < accessor.count; i++)
			{
				vertices[i].texCoord = glm::vec2(
					texCoords[i * stride + 0],
					texCoords[i * stride + 1]
				);
			}
		}
		else
		{
			// 텍스처 좌표가 없으면 기본값
			for (auto& v : vertices)
			{
				v.texCoord = glm::vec2(0.0f, 0.0f);
			}
		}
		
		// Tangent
		if (gltfPrimitive.attributes.find("TANGENT") != gltfPrimitive.attributes.end())
		{
			const tinygltf::Accessor& accessor = gltfModel.accessors[gltfPrimitive.attributes.at("TANGENT")];
			const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
			
			const float* tangents = reinterpret_cast<const float*>(
				&buffer.data[bufferView.byteOffset + accessor.byteOffset]
			);
			
			size_t stride = accessor.ByteStride(bufferView) ? (accessor.ByteStride(bufferView) / sizeof(float)) : 4;
			
			for (size_t i = 0; i < accessor.count; i++)
			{
				vertices[i].tangent = glm::vec4(
					tangents[i * stride + 0],
					tangents[i * stride + 1],
					tangents[i * stride + 2],
					tangents[i * stride + 3]
				);
			}
		}
		else
		{
			// Tangent가 없으면 기본값
			for (auto& v : vertices)
			{
				v.tangent = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
			}
		}
		
		// 버텍스 버퍼 생성
		if (_renderContext)
		{
			primitive.vertexBuffer = _renderContext->CreateBuffer(
				vertices.data(),
				sizeof(Vertex) * vertices.size(),
				Render::OgBufferUsage::VERTEX,
				OgMemoryOption::MAP_MANAGED
			);
			primitive.vertexBuffer->Retain();
		}
		primitive.vertexCount = static_cast<uint32>(vertices.size());
		
		// 인덱스 데이터 로드
		if (gltfPrimitive.indices >= 0)
		{
			const tinygltf::Accessor& accessor = gltfModel.accessors[gltfPrimitive.indices];
			const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
			
			primitive.hasIndices = true;
			primitive.indexCount = static_cast<uint32>(accessor.count);
			
			if (_renderContext)
			{
				if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
				{
					primitive.indexBuffer = _renderContext->CreateBuffer(
						(void*)&buffer.data[bufferView.byteOffset + accessor.byteOffset],
						accessor.count * sizeof(uint16_t),
						Render::OgBufferUsage::INDEX,
						OgMemoryOption::MAP_MANAGED
					);
				}
				else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
				{
					// uint32 인덱스를 uint16으로 변환
					const uint32_t* indices32 = reinterpret_cast<const uint32_t*>(
						&buffer.data[bufferView.byteOffset + accessor.byteOffset]
					);
					std::vector<uint16_t> indices16(accessor.count);
					
					bool hasLargeIndex = false;
					for (size_t i = 0; i < accessor.count; i++)
					{
						if (indices32[i] > UINT16_MAX)
						{
							hasLargeIndex = true;
							LOGD(OG_ID, "Index value %u exceeds uint16 max, clamping to %u", indices32[i], UINT16_MAX);
							indices16[i] = UINT16_MAX;
						}
						else
						{
							indices16[i] = static_cast<uint16_t>(indices32[i]);
						}
					}
					
					if (hasLargeIndex)
					{
						LOGD(OG_ID, "Some indices were clamped to fit uint16 range");
					}
					
					primitive.indexBuffer = _renderContext->CreateBuffer(
						indices16.data(),
						indices16.size() * sizeof(uint16_t),
						Render::OgBufferUsage::INDEX,
						OgMemoryOption::MAP_MANAGED
					);
				}
				else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
				{
					// uint8 인덱스를 uint16으로 변환
					const uint8_t* indices8 = reinterpret_cast<const uint8_t*>(
						&buffer.data[bufferView.byteOffset + accessor.byteOffset]
					);
					std::vector<uint16_t> indices16(accessor.count);
					
					for (size_t i = 0; i < accessor.count; i++)
					{
						indices16[i] = static_cast<uint16_t>(indices8[i]);
					}
					
					primitive.indexBuffer = _renderContext->CreateBuffer(
						indices16.data(),
						indices16.size() * sizeof(uint16_t),
						Render::OgBufferUsage::INDEX,
						OgMemoryOption::MAP_MANAGED
					);
				}
				
				if (primitive.indexBuffer)
				{
					primitive.indexBuffer->Retain();
				}
			}
		}
		
		mesh.primitives.push_back(primitive);
	}
	
	outModel.meshes.push_back(mesh);
}

void OgGLTFLoader::loadNodes(const tinygltf::Model& gltfModel, LoadedModel& outModel)
{
	outModel.nodes.resize(gltfModel.nodes.size());
	
	for (size_t i = 0; i < gltfModel.nodes.size(); i++)
	{
		const tinygltf::Node& gltfNode = gltfModel.nodes[i];
		Node& node = outModel.nodes[i];
		
		node.name = gltfNode.name;
		node.meshIndex = gltfNode.mesh;
		node.children = gltfNode.children;
		
		// Transform 행렬 계산
		if (gltfNode.matrix.size() == 16)
		{
			// 행렬이 직접 지정된 경우
			for (int j = 0; j < 16; j++)
			{
				node.matrix[j / 4][j % 4] = static_cast<float>(gltfNode.matrix[j]);
			}
		}
		else
		{
			// TRS로부터 행렬 계산
			glm::mat4 T(1.0f);
			glm::mat4 R(1.0f);
			glm::mat4 S(1.0f);
			
			if (gltfNode.translation.size() == 3)
			{
				T = glm::translate(glm::mat4(1.0f), glm::vec3(
					static_cast<float>(gltfNode.translation[0]),
					static_cast<float>(gltfNode.translation[1]),
					static_cast<float>(gltfNode.translation[2])
				));
			}
			
			if (gltfNode.rotation.size() == 4)
			{
				glm::quat q(
					static_cast<float>(gltfNode.rotation[3]),
					static_cast<float>(gltfNode.rotation[0]),
					static_cast<float>(gltfNode.rotation[1]),
					static_cast<float>(gltfNode.rotation[2])
				);
				R = glm::mat4_cast(q);
			}
			
			if (gltfNode.scale.size() == 3)
			{
				S = glm::scale(glm::mat4(1.0f), glm::vec3(
					static_cast<float>(gltfNode.scale[0]),
					static_cast<float>(gltfNode.scale[1]),
					static_cast<float>(gltfNode.scale[2])
				));
			}
			
			node.matrix = T * R * S;
		}
	}
}

void OgGLTFLoader::loadScene(const tinygltf::Model& gltfModel, LoadedModel& outModel)
{
	// 루트 노드 찾기
	if (gltfModel.scenes.size() > 0)
	{
		const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene >= 0 ? gltfModel.defaultScene : 0];
		outModel.rootNodes = scene.nodes;
	}
	else
	{
		// Scene이 없으면 부모가 없는 노드를 루트로 간주
		std::vector<bool> hasParent(gltfModel.nodes.size(), false);
		for (const auto& node : gltfModel.nodes)
		{
			for (int child : node.children)
			{
				hasParent[child] = true;
			}
		}
		for (size_t i = 0; i < hasParent.size(); i++)
		{
			if (!hasParent[i])
			{
				outModel.rootNodes.push_back(static_cast<int>(i));
			}
		}
	}
}

Render::OgTextureHandle* OgGLTFLoader::loadTexture(const tinygltf::Model& model, int textureIndex)
{
	if (textureIndex < 0 || textureIndex >= static_cast<int>(model.textures.size()))
	{
		return nullptr;
	}
	
	// 캐시 확인
	auto it = _textureCache.find(textureIndex);
	if (it != _textureCache.end())
	{
		return it->second;
	}
	
	const tinygltf::Texture& texture = model.textures[textureIndex];
	const tinygltf::Image& image = model.images[texture.source];
	
	OgTextureInfo texInfo{};
	texInfo.type = OgTextureType::TEX_2D;
	texInfo.extent.width = image.width;
	texInfo.extent.height = image.height;
	texInfo.usage = OgTextureUsage::SAMPLED | OgTextureUsage::STAGING;
	texInfo.isGenerateMipmaps = false;
	
	// 포맷 결정
	if (image.component == 3)
	{
		texInfo.format = OgPixelFormat::R8G8B8_UNORM;
		texInfo.byteSize = image.width * image.height * 3;
	}
	else if (image.component == 4)
	{
		texInfo.format = OgPixelFormat::R8G8B8A8_SRGB;
		texInfo.byteSize = image.width * image.height * 4;
	}
	else
	{
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
	if (texture.sampler >= 0 && texture.sampler < static_cast<int>(model.samplers.size()))
	{
		const tinygltf::Sampler& gltfSampler = model.samplers[texture.sampler];
		
		// Wrap modes
		auto convertWrapMode = [](int mode) {
			switch (mode)
			{
				case TINYGLTF_TEXTURE_WRAP_REPEAT: return OgSamplerAddressMode::REPEAT;
				case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE: return OgSamplerAddressMode::CLAMP_TO_EDGE;
				case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT: return OgSamplerAddressMode::MIRRORED_REPEAT;
				default: return OgSamplerAddressMode::REPEAT;
			}
		};
		
		samplerInfo.addressU = convertWrapMode(gltfSampler.wrapS);
		samplerInfo.addressV = convertWrapMode(gltfSampler.wrapT);
		
		// Filter modes
		if (gltfSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
		{
			samplerInfo.magFilter = OgFilter::NEAREST;
		}
		
		if (gltfSampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST ||
			gltfSampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST)
		{
			samplerInfo.minFilter = OgFilter::NEAREST;
		}
	}
	
	void* imageData = const_cast<unsigned char*>(image.image.data());
	OgTextureHandle* textureHandle = nullptr;
	
	if (_renderContext)
	{
		textureHandle = _renderContext->CreateTexture(
			&imageData,
			texInfo,
			_renderContext->CreateSampler(samplerInfo)
		);
		
		if (textureHandle)
		{
			textureHandle->Retain();
			_textureCache[textureIndex] = textureHandle;
		}
	}
	
	return textureHandle;
}

OgGLTFLoader::TextureTransform OgGLTFLoader::loadTextureTransform(const tinygltf::Value& extensionValue)
{
	TextureTransform transform;
	
	if (!extensionValue.IsObject())
	{
		return transform;
	}
	
	// offset 파싱
	if (extensionValue.Has("offset") && extensionValue.Get("offset").IsArray())
	{
		const auto& offsetArray = extensionValue.Get("offset");
		if (offsetArray.ArrayLen() >= 2)
		{
			transform.offset.x = static_cast<float>(offsetArray.Get(0).GetNumberAsDouble());
			transform.offset.y = static_cast<float>(offsetArray.Get(1).GetNumberAsDouble());
		}
	}
	
	// rotation 파싱
	if (extensionValue.Has("rotation") && extensionValue.Get("rotation").IsNumber())
	{
		transform.rotation = static_cast<float>(extensionValue.Get("rotation").GetNumberAsDouble());
	}
	
	// scale 파싱
	if (extensionValue.Has("scale") && extensionValue.Get("scale").IsArray())
	{
		const auto& scaleArray = extensionValue.Get("scale");
		if (scaleArray.ArrayLen() >= 2)
		{
			transform.scale.x = static_cast<float>(scaleArray.Get(0).GetNumberAsDouble());
			transform.scale.y = static_cast<float>(scaleArray.Get(1).GetNumberAsDouble());
		}
	}
	
	return transform;
}

void OgGLTFLoader::calculateBounds(LoadedModel& model)
{
	model.minBounds = glm::vec3(FLT_MAX);
	model.maxBounds = glm::vec3(-FLT_MAX);
	
	// 모든 루트 노드에서 시작하여 바운딩 박스 계산
	for (int rootNode : model.rootNodes)
	{
		updateBoundsWithNode(model, rootNode, glm::mat4(1.0f), model.minBounds, model.maxBounds);
	}
	
	// 유효한 바운딩 박스가 계산되었는지 확인
	if (model.minBounds.x > model.maxBounds.x)
	{
		// 바운딩 박스가 유효하지 않으면 기본값 설정
		model.minBounds = glm::vec3(-1.0f);
		model.maxBounds = glm::vec3(1.0f);
	}
	
	// 중심과 반경 계산
	model.center = (model.minBounds + model.maxBounds) * 0.5f;
	model.radius = glm::length(model.maxBounds - model.center);
}

void OgGLTFLoader::updateBoundsWithNode(const LoadedModel& model, int nodeIndex, const glm::mat4& parentMatrix, 
										glm::vec3& minBounds, glm::vec3& maxBounds)
{
	if (nodeIndex < 0 || nodeIndex >= static_cast<int>(model.nodes.size()))
	{
		return;
	}
	
	const Node& node = model.nodes[nodeIndex];
	glm::mat4 worldMatrix = parentMatrix * node.matrix;
	
	// 이 노드에 메시가 있으면 바운딩 박스 업데이트
	if (node.meshIndex >= 0 && node.meshIndex < static_cast<int>(model.meshes.size()))
	{
		const Mesh& mesh = model.meshes[node.meshIndex];
		
		// 간단히 메시의 첫 번째 primitive만 고려 (실제로는 모든 primitive를 확인해야 함)
		// 더 정확한 계산을 위해서는 버텍스 데이터를 직접 읽어야 하지만,
		// 여기서는 단순한 추정값을 사용
		for (const auto& primitive : mesh.primitives)
		{
			// 임시로 단위 큐브의 8개 정점을 변환하여 바운딩 박스 추정
			std::vector<glm::vec3> corners = {
				glm::vec3(-1, -1, -1), glm::vec3(1, -1, -1),
				glm::vec3(-1,  1, -1), glm::vec3(1,  1, -1),
				glm::vec3(-1, -1,  1), glm::vec3(1, -1,  1),
				glm::vec3(-1,  1,  1), glm::vec3(1,  1,  1)
			};
			
			for (const auto& corner : corners)
			{
				glm::vec4 worldPos = worldMatrix * glm::vec4(corner, 1.0f);
				glm::vec3 pos3 = glm::vec3(worldPos) / worldPos.w;
				
				minBounds = glm::min(minBounds, pos3);
				maxBounds = glm::max(maxBounds, pos3);
			}
		}
	}
	
	// 자식 노드들도 재귀적으로 처리
	for (int childIndex : node.children)
	{
		updateBoundsWithNode(model, childIndex, worldMatrix, minBounds, maxBounds);
	}
}

OG_NAMESPACE_SAMPLE_END
