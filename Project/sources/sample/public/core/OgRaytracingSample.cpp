#include "OgRaytracingSample.h"
#include "sample/public/core/util/OgShaderCompiler.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <cmath>
#include <algorithm>
#include <filesystem>

using namespace std;
using namespace Render;

OG_NAMESPACE_SAMPLE_BEGIN

OgRayTracingSample::OgRayTracingSample(Render::OgRenderContext* renderContext)
	: OgSampleBase(renderContext)
	, _camera(std::make_unique<OgFlyCamera>())
	, _gltfLoader(std::make_unique<OgGLTFLoader>(renderContext))
{
	// 카메라 초기 설정
	_camera->SetPosition(glm::vec3(0.0f, 5.0f, 10.0f));
	_camera->SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));
	
	// 레이트레이싱 초기 설정
	_rtUniformData.maxBounces = 8;
	_rtUniformData.samplesPerPixel = 1;
	_rtUniformData.globalAmbient = glm::vec4(0.15f, 0.15f, 0.15f, 1.0f);
}

OgRayTracingSample::~OgRayTracingSample()
{
	if (_isInitialized)
	{
		OnDestroy();
	}
}

void OgRayTracingSample::OnInit(Render::OgSwapChain* swapchain)
{
	if (_isInitialized)
		return;

	// 레이트레이싱 지원 확인
	if (!_renderContext->IsRayTracingSupported())
	{
		LOGE(OG_ID, "Ray tracing is not supported on this device");
		return;
	}

	// 스왑체인의 크기로 리소스 생성
	const uint16 width = _renderContext->GetSwapChainFrameBuffer(swapchain, 0)->width;
	const uint16 height = _renderContext->GetSwapChainFrameBuffer(swapchain, 0)->height;

	// 렌더 타겟 생성
	createRenderTarget(width, height);

	// 리소스 생성
	createResources(width, height);

	// DragonAttenuation 모델 로드
	std::filesystem::path modelPath = "C:/Osgood/EngineDevelop/OGHypeEngine/Project/res/models/DragonAttenuation/glTF/DragonAttenuation.gltf";
	
	if (!loadGLTFModel(modelPath.string()))
	{
		LOGE(OG_ID, "Failed to load DragonAttenuation model");
		return;
	}

	// Acceleration Structure 생성
	createAccelerationStructures();

	_isInitialized = true;
}

void OgRayTracingSample::OnDestroy()
{
	if (!_isInitialized)
		return;

	_renderContext->WaitDeviceIdle();
	
	destroyAccelerationStructures();
	destroyResources();
	destroyRenderTarget();
	clearModelData();
	
	_isInitialized = false;
}

void OgRayTracingSample::OnUpdate(float deltaTime)
{
	// 카메라 업데이트
	if (_useFlyCamera && _camera)
	{
		_camera->Update(deltaTime);
		
		// 카메라가 움직였으면 프레임 카운터 리셋
		glm::mat4 currentView = _camera->GetViewMatrix();
		if (_previousViewMatrix != currentView)
		{
			_frameCount = 0;
			_previousViewMatrix = currentView;
		}
	}

	updateUniformBuffer();
}

void OgRayTracingSample::OnRender(Render::OgCommandEncoderHandle* encoder, Render::OgSwapChain* swapchain)
{
	if (!_isInitialized || !_rtPipeline || !_tlas)
		return;

	// 프레임 카운터 증가
	_rtUniformData.frameCount = _frameCount++;

	// 유니폼 버퍼 업데이트
	void* mappedData = _renderContext->MapBuffer(_rtUniformBuffer, sizeof(RTUniformData));
	if (mappedData)
	{
		memcpy(mappedData, &_rtUniformData, sizeof(RTUniformData));
		_renderContext->UnmapBuffer(_rtUniformBuffer);
	}

	// 렌더 패스 시작 (레이트레이싱 결과를 복사하기 위한 준비)
	float clearColor[]{ 0.0f, 0.0f, 0.0f, 1.0f };
	
	OgCommandEncoderHandle::ClearValue colorClear;
	colorClear.color.value[0] = clearColor[0];
	colorClear.color.value[1] = clearColor[1];
	colorClear.color.value[2] = clearColor[2];
	colorClear.color.value[3] = clearColor[3];

	OgCommandEncoderHandle::ClearValue depthStencilClear;
	depthStencilClear.depthStencil.depth = 1.f;
	depthStencilClear.depthStencil.stencil = 0.f;

	OgCommandEncoderHandle::Area area(0, 0, _renderTargetWidth, _renderTargetHeight);

	encoder->BeginDebugMarker("Ray Tracing Sample", clearColor);

	// 레이트레이싱 파이프라인 바인드
	encoder->BindRayTracingPipeline(_rtPipeline);
	encoder->BindResourceSet(_rtResourceSet);

	// 레이트레이싱 디스패치 - SBT 레이아웃: [raygen][miss][shadowMiss][hit]
	uint32_t aligned = _sbtHandleSizeAligned;

	Render::OgShaderBindingTable sbt{};
	sbt.raygenSBT = _raygenSBT;
	sbt.raygenOffset = 0;
	sbt.raygenStride = aligned;
	sbt.raygenSize = aligned;

	sbt.missSBT = _missSBT;
	sbt.missOffset = 1 * aligned;       // raygen 다음
	sbt.missStride = aligned;
	sbt.missSize = 2 * aligned;          // miss 2개 (primary + shadow)

	sbt.hitSBT = _hitSBT;
	sbt.hitOffset = 3 * aligned;         // raygen + 2 miss 다음
	sbt.hitStride = aligned;
	sbt.hitSize = aligned;

	sbt.callableSBT = nullptr;
	sbt.callableOffset = 0;
	sbt.callableStride = 0;
	sbt.callableSize = 0;
	
	encoder->TraceRays(sbt, _renderTargetWidth, _renderTargetHeight, 1);

	encoder->EndDebugMarker();
}

void OgRayTracingSample::OnSuspend(Render::OgSwapChain* swapchain)
{
	_renderContext->Suspend(swapchain);
}

void OgRayTracingSample::OnRestore(Render::OgSwapChain* swapchain)
{
	_renderContext->Restore(swapchain);
}

void OgRayTracingSample::OnResize(uint32 width, uint32 height)
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
	}
	
	// 프레임 카운터 리셋
	_frameCount = 0;
	updateUniformBuffer();
}

// 입력 처리 메서드들
void OgRayTracingSample::OnMouseButton(int button, int action, int mods)
{
	if (_useFlyCamera && _camera)
	{
		_camera->OnMouseButton(button, action, mods);
	}
}

void OgRayTracingSample::OnMouseMove(double x, double y)
{
	if (_useFlyCamera && _camera)
	{
		_camera->OnMouseMove(x, y);
	}
}

void OgRayTracingSample::OnMouseScroll(double xoffset, double yoffset)
{
	if (_useFlyCamera && _camera)
	{
		_camera->OnMouseScroll(xoffset, yoffset);
	}
}

void OgRayTracingSample::OnKeyPress(int key, int action, int mods)
{
	if (_useFlyCamera && _camera)
	{
		_camera->OnKeyPress(key, action, mods);
	}

	// F 키로 플라이 카메라 토글
	if (key == OG_KEY_F && action == OG_PRESS)
	{
		_useFlyCamera = !_useFlyCamera;
		_frameCount = 0;
		updateUniformBuffer();
	}

	// D 키로 디버그 시각화 토글
	if (key == OG_KEY_D && action == OG_PRESS)
	{
		_enableDebugVisualization = !_enableDebugVisualization;
		_frameCount = 0;
	}
}

void OgRayTracingSample::createResources(uint16 width, uint16 height)
{
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
	OgResourceBinding bindings[8];
	
	// TLAS
	bindings[0].type = OgResourceType::ACCELERATION_STRUCTURE;
	bindings[0].stage = static_cast<OgShaderType>(static_cast<uint16>(OgShaderType::RAYGEN) | static_cast<uint16>(OgShaderType::CLOSEST_HIT));
	bindings[0].binding = 0;
	bindings[0].arrayCount = 0;
	bindings[0].name = nullptr;

	// 출력 이미지
	bindings[1].type = OgResourceType::STORAGE_IMAGE;
	bindings[1].stage = OgShaderType::RAYGEN;
	bindings[1].binding = 1;
	bindings[1].arrayCount = 0;
	bindings[1].name = nullptr;

	// 유니폼 버퍼
	bindings[2].type = OgResourceType::UNIFORM_BUFFER;
	bindings[2].stage = static_cast<OgShaderType>(static_cast<uint16>(OgShaderType::RAYGEN) | static_cast<uint16>(OgShaderType::CLOSEST_HIT) | static_cast<uint16>(OgShaderType::MISS));
	bindings[2].binding = 2;
	bindings[2].arrayCount = 0;
	bindings[2].name = nullptr;

	// 버텍스 버퍼 (스토리지 버퍼로 사용)
	bindings[3].type = OgResourceType::STORAGE_BUFFER;
	bindings[3].stage = OgShaderType::CLOSEST_HIT;
	bindings[3].binding = 3;
	bindings[3].arrayCount = 0;
	bindings[3].name = nullptr;

	// 인덱스 버퍼
	bindings[4].type = OgResourceType::STORAGE_BUFFER;
	bindings[4].stage = OgShaderType::CLOSEST_HIT;
	bindings[4].binding = 4;
	bindings[4].arrayCount = 0;
	bindings[4].name = nullptr;

	// Material 버퍼
	bindings[5].type = OgResourceType::STORAGE_BUFFER;
	bindings[5].stage = OgShaderType::CLOSEST_HIT;
	bindings[5].binding = 5;
	bindings[5].arrayCount = 0;
	bindings[5].name = nullptr;

	// GeometryInfo 버퍼
	bindings[6].type = OgResourceType::STORAGE_BUFFER;
	bindings[6].stage = OgShaderType::CLOSEST_HIT;
	bindings[6].binding = 6;
	bindings[6].arrayCount = 0;
	bindings[6].name = nullptr;

	// 텍스처 배열
	bindings[7].type = OgResourceType::COMBINED_IMAGE_SAMPLER;
	bindings[7].stage = OgShaderType::CLOSEST_HIT;
	bindings[7].binding = 7;
	bindings[7].arrayCount = 16; // 최대 16개의 텍스처
	bindings[7].name = nullptr;

	_rtResourceLayout = _renderContext->CreateResourceLayout(bindings, 8);
	_rtResourceLayout->name = "RayTracingResourceLayout";
	_rtResourceLayout->Retain();

	// 레이트레이싱 파이프라인 생성
	createRayTracingPipeline();
}

void OgRayTracingSample::destroyResources()
{
	if (_rtPipeline)
	{
		_rtPipeline->Release();
		_rtPipeline = nullptr;
	}

	if (_rtResourceSet)
	{
		_rtResourceSet->Release();
		_rtResourceSet = nullptr;
	}

	if (_rtResourceLayout)
	{
		_rtResourceLayout->Release();
		_rtResourceLayout = nullptr;
	}

	if (_rtProgram)
	{
		_rtProgram->Release();
		_rtProgram = nullptr;
	}

	if (_closestHitShader)
	{
		_closestHitShader->Release();
		_closestHitShader = nullptr;
	}

	if (_shadowMissShader)
	{
		_shadowMissShader->Release();
		_shadowMissShader = nullptr;
	}

	if (_missShader)
	{
		_missShader->Release();
		_missShader = nullptr;
	}

	if (_raygenShader)
	{
		_raygenShader->Release();
		_raygenShader = nullptr;
	}

	if (_rtUniformBuffer)
	{
		_rtUniformBuffer->Release();
		_rtUniformBuffer = nullptr;
	}

	if (_materialBuffer)
	{
		_materialBuffer->Release();
		_materialBuffer = nullptr;
	}

	if (_geometryInfoBuffer)
	{
		_geometryInfoBuffer->Release();
		_geometryInfoBuffer = nullptr;
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

	// SBT 버퍼들 해제
	if (_raygenSBT)
	{
		_raygenSBT->Release();
		_raygenSBT = nullptr;
	}

	if (_missSBT)
	{
		_missSBT->Release();
		_missSBT = nullptr;
	}

	if (_hitSBT)
	{
		_hitSBT->Release();
		_hitSBT = nullptr;
	}
}
void OgRayTracingSample::createShaders()
{
	// Ray Generation 셰이더 (Slang)
	const char* raygenSlang = R"(
	#version 460
		#extension GL_EXT_ray_tracing : require


		layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
			
		layout(binding = 1, set = 0, rgba32f) uniform image2D image;

		layout(binding = 2, set = 0) uniform UniformBufferObject {
			mat4 viewInverse;
			mat4 projInverse;
			vec4 cameraPos;
			vec4 lightPos;
			vec4 lightColor;
			vec4 globalAmbient;
			uint frameCount;
			uint maxBounces;
			uint samplesPerPixel;
			float padding;
		} ubo;

		struct RayPayload {
			vec3 color;
			uint depth;
		};
		layout(location = 0) rayPayloadEXT RayPayload payload;

		uint rngState;

		uint pcg_hash(uint i)
		{
			uint state = i * 747796405u + 2891336453u;
			uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
			return (word >> 22u) ^ word;
		}

		float randomFloat()
		{
			rngState = pcg_hash(rngState);
			return float(rngState) / 4294967295.0;
		}

		void main()
		{
			// initPayload
			payload.color = vec3(0, 0, 0);
			payload.depth = 0;
			uvec2 launchID = gl_LaunchIDEXT.xy;
			uvec2 launchSize = gl_LaunchSizeEXT.xy;

			rngState = ubo.frameCount + launchID.x * 1973 + launchID.y * 9277;

			// 프레임마다 픽셀 내 위치를 지터해서 누적 시 안티앨리어싱 (프로그레시브 refinement)
			vec2 jitter = ubo.frameCount > 0 ? vec2(randomFloat(), randomFloat()) - 0.5 : vec2(0.0);
			vec2 pixelCenter = vec2(launchID) + vec2(0.5) + jitter;
			vec2 inUV = pixelCenter / vec2(launchSize);
			vec2 d = inUV * 2.0 - 1.0;

			vec4 origin = ubo.viewInverse * vec4(0, 0, 0, 1);
			vec4 target = ubo.projInverse * vec4(d.x, d.y, 1, 1);
			vec4 direction = ubo.viewInverse * vec4(normalize(target.xyz), 0);

			// Trace primary ray
			traceRayEXT(topLevelAS,
						gl_RayFlagsOpaqueEXT,
						0xff,         // cullMask
						0,            // sbtRecordOffset
						0,            // sbtRecordStride
						0,            // missIndex
						origin.xyz,
						0.001,
						direction.xyz,
						10000.0,
						0             // payload location
			);

			// 톤 매핑 + 감마 보정 (최종 1회만 적용)
			vec3 finalColor = payload.color;
			finalColor = finalColor / (finalColor + vec3(1.0));
			finalColor = pow(finalColor, vec3(1.0/2.2));

			if (ubo.frameCount > 0)
			{
				vec3 previousColor = imageLoad(image, ivec2(launchID)).rgb;
				float weight = 1.0 / float(ubo.frameCount + 1);
				finalColor = mix(previousColor, finalColor, weight);
			}

			imageStore(image, ivec2(launchID), vec4(finalColor, 1.0));
		}
	)";

#pragma region missShader
	// Miss 셰이더 (GLSL) - 하늘 그라데이션
	const char* missGLSL = R"(
		#version 460
		#extension GL_EXT_ray_tracing : require

		struct RayPayload {
			vec3 color;
			uint depth;
		};
		layout(location = 0) rayPayloadInEXT RayPayload payload;

		layout(binding = 2, set = 0) uniform UniformBufferObject {
			mat4 viewInverse;
			mat4 projInverse;
			vec4 cameraPos;
			vec4 lightPos;
			vec4 lightColor;
			vec4 globalAmbient;
			uint frameCount;
			uint maxBounces;
			uint samplesPerPixel;
			float padding;
		} ubo;

		void main()
		{
			vec3 direction = normalize(gl_WorldRayDirectionEXT);
			float t = 0.5 * (direction.y + 1.0);
			vec3 skyColor = mix(vec3(0.5, 0.7, 1.0), vec3(0.1, 0.2, 0.4), t);
			payload.color = skyColor;
		}
	)";
#pragma endregion

	// Shadow Miss 셰이더 - shadow ray가 아무것도 안 맞으면 빛이 도달
	const char* shadowMissGLSL = R"(
		#version 460
		#extension GL_EXT_ray_tracing : require

		layout(location = 1) rayPayloadInEXT vec3 shadowHitValue;

		void main()
		{
			shadowHitValue = vec3(1.0);
		}
	)";

#pragma region closestHitShader
	const char* closestHitGLSL = R"(
		#version 460
		#extension GL_EXT_ray_tracing : require
		#extension GL_EXT_shader_explicit_arithmetic_types : require
		#extension GL_EXT_scalar_block_layout : require
		#extension GL_EXT_buffer_reference2 : require
		#extension GL_EXT_nonuniform_qualifier : require

		struct RayPayload {
			vec3 color;
			uint depth;
		};
		layout(location = 0) rayPayloadInEXT RayPayload payload;
		layout(location = 1) rayPayloadEXT vec3 shadowHitValue;

		layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

		layout(binding = 2, set = 0) uniform UniformBufferObject {
			mat4 viewInverse;
			mat4 projInverse;
			vec4 cameraPos;
			vec4 lightPos;
			vec4 lightColor;
			vec4 globalAmbient;
			uint frameCount;
			uint maxBounces;
			uint samplesPerPixel;
			float padding;
		} ubo;

		struct Vertex {
			float px, py, pz;       // position (12 bytes)
			float nx, ny, nz;       // normal (12 bytes)
			float u, v;             // texCoord (8 bytes)
			vec4 tangent;           // tangent (16 bytes)
		};

		struct Material {
			vec4 baseColorFactor;
			vec4 emissiveFactor;
			float metallicFactor;
			float roughnessFactor;
			float transmissionFactor;
			float ior;
			vec4 attenuationColor;
			float attenuationDistance;
			int baseColorTextureIndex;
			int normalTextureIndex;
			int metallicRoughnessTextureIndex;
		};

		struct GeometryInfo {
			uint vertexOffset;
			uint indexOffset;
			uint materialIndex;
			uint padding;
		};

		layout(binding = 3, set = 0, scalar) buffer VertexBuffer {
			Vertex vertices[];
		} vertexBuffer;

		layout(binding = 4, set = 0, scalar) buffer IndexBuffer {
			uint indices[];
		} indexBuffer;

		layout(binding = 5, set = 0, scalar) buffer MaterialBuffer {
			Material materials[];
		} materialBuffer;

		layout(binding = 6, set = 0, scalar) buffer GeometryInfoBuffer {
			GeometryInfo geometryInfos[];
		} geometryInfoBuffer;

		layout(binding = 7, set = 0) uniform sampler2D textures[16];

		hitAttributeEXT vec2 attribs;

		vec3 getBarycentric()
		{
			vec3 barycentricCoords = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
			return barycentricCoords;
		}

		Vertex getVertex(uint index)
		{
			GeometryInfo geomInfo = geometryInfoBuffer.geometryInfos[gl_InstanceCustomIndexEXT + gl_GeometryIndexEXT];
			return vertexBuffer.vertices[geomInfo.vertexOffset + index];
		}

		vec3 getPosition(Vertex vtx) { return vec3(vtx.px, vtx.py, vtx.pz); }
		vec3 getNormal(Vertex vtx) { return vec3(vtx.nx, vtx.ny, vtx.nz); }
		vec2 getTexCoord(Vertex vtx) { return vec2(vtx.u, vtx.v); }

		const float PI = 3.14159265359;

		vec3 fresnelSchlick(float cosTheta, vec3 F0)
		{
			return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
		}

		float distributionGGX(vec3 N, vec3 H, float roughness)
		{
			float a = roughness * roughness;
			float a2 = a * a;
			float NdotH = max(dot(N, H), 0.0);
			float NdotH2 = NdotH * NdotH;

			float num = a2;
			float denom = (NdotH2 * (a2 - 1.0) + 1.0);
			denom = PI * denom * denom;

			return num / max(denom, 0.0001);
		}

		float geometrySchlickGGX(float NdotV, float roughness)
		{
			float r = (roughness + 1.0);
			float k = (r * r) / 8.0;

			float num = NdotV;
			float denom = NdotV * (1.0 - k) + k;

			return num / max(denom, 0.0001);
		}

		float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
		{
			float NdotV = max(dot(N, V), 0.0);
			float NdotL = max(dot(N, L), 0.0);
			float ggx2 = geometrySchlickGGX(NdotV, roughness);
			float ggx1 = geometrySchlickGGX(NdotL, roughness);

			return ggx1 * ggx2;
		}

		float fresnelSchlickScalar(float cosTheta, float f0)
		{
			return f0 + (1.0 - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
		}

		void main()
		{
			GeometryInfo geomInfo = geometryInfoBuffer.geometryInfos[gl_InstanceCustomIndexEXT + gl_GeometryIndexEXT];

			uint i0 = indexBuffer.indices[geomInfo.indexOffset + gl_PrimitiveID * 3 + 0];
			uint i1 = indexBuffer.indices[geomInfo.indexOffset + gl_PrimitiveID * 3 + 1];
			uint i2 = indexBuffer.indices[geomInfo.indexOffset + gl_PrimitiveID * 3 + 2];

			Vertex v0 = getVertex(i0);
			Vertex v1 = getVertex(i1);
			Vertex v2 = getVertex(i2);

			vec3 bary = getBarycentric();
			vec3 position = getPosition(v0) * bary.x + getPosition(v1) * bary.y + getPosition(v2) * bary.z;
			vec3 normal = normalize(getNormal(v0) * bary.x + getNormal(v1) * bary.y + getNormal(v2) * bary.z);
			vec2 texCoord = getTexCoord(v0) * bary.x + getTexCoord(v1) * bary.y + getTexCoord(v2) * bary.z;

			// Transform to world space
			position = gl_ObjectToWorldEXT * vec4(position, 1.0);
			normal = normalize(mat3(gl_ObjectToWorldEXT) * normal);

			Material material = materialBuffer.materials[geomInfo.materialIndex];

			vec4 baseColor = material.baseColorFactor;
			if (material.baseColorTextureIndex >= 0)
			{
				baseColor *= texture(textures[material.baseColorTextureIndex], texCoord);
			}

			// PBR 파라미터
			float metallic = material.metallicFactor;
			float roughness = material.roughnessFactor;
			vec3 emissive = material.emissiveFactor.rgb;
			float transmission = material.transmissionFactor;

			vec3 V = -normalize(gl_WorldRayDirectionEXT);
			vec3 rayDir = gl_WorldRayDirectionEXT;

			// Transmission (굴절) 처리
			// payload.depth로 재귀 깊이 관리 (ubo.maxBounces까지)
			uint depth = payload.depth;
			bool canRefract = depth < ubo.maxBounces;
			if (transmission > 0.0 && !canRefract)
			{
				// 굴절 깊이 소진: 불투명 fallback 대신 감쇠된 하늘색으로 종료 (얼룩 경계 방지)
				float skyT = 0.5 * (normalize(rayDir).y + 1.0);
				vec3 sky = mix(vec3(0.5, 0.7, 1.0), vec3(0.1, 0.2, 0.4), skyT);
				payload.color = sky * material.attenuationColor.rgb;
				return;
			}
			if (transmission > 0.0 && canRefract)
			{
				float ior = material.ior;
				if (ior <= 0.0) ior = 1.5;

				// inside/outside 판별
				float NdotV = dot(normal, V);
				bool isInside = NdotV < 0.0;
				vec3 faceNormal = isInside ? -normal : normal;
				float cosI = abs(NdotV);

				// eta = n1/n2
				float eta = isInside ? ior : (1.0 / ior);

				// 굴절 방향 (Snell's law)
				vec3 refractDir = refract(rayDir, faceNormal, eta);

				// Fresnel 반사율 (Schlick 근사)
				float f0 = pow((1.0 - ior) / (1.0 + ior), 2.0);
				float fresnel = fresnelSchlickScalar(cosI, f0);

				// 전반사 체크
				bool totalInternalReflection = (length(refractDir) < 0.001);

				vec3 refractedColor = vec3(0.0);

				if (!totalInternalReflection)
				{
					// 굴절 레이 트레이싱
					vec3 refractOrigin = position - faceNormal * 0.001;
					payload.color = vec3(0.0);
					payload.depth = depth + 1;

					traceRayEXT(topLevelAS,
								gl_RayFlagsOpaqueEXT,
								0xff,
								0, 0, 0,
								refractOrigin,
								0.001,
								refractDir,
								10000.0,
								0
					);
					refractedColor = payload.color;
				}

				// Beer-Lambert 감쇠 (매질 내부를 지날 때)
				if (isInside && material.attenuationDistance > 0.0)
				{
					float dist = gl_HitTEXT;
					vec3 absorb = -log(material.attenuationColor.rgb + 0.001) / material.attenuationDistance;
					refractedColor *= exp(-absorb * dist);
				}

				vec3 reflectDir = reflect(rayDir, faceNormal);
				vec3 reflectedColor;
				if (totalInternalReflection)
				{
					fresnel = 1.0;
				}

				// 반사 레이 추적: 전반사는 항상, 표면 반사는 얕은 깊이에서만 (레이 수 폭증 방지)
				if (totalInternalReflection || depth < 2)
				{
					vec3 reflectOrigin = position + faceNormal * 0.001;
					payload.color = vec3(0.0);
					payload.depth = depth + 1;

					traceRayEXT(topLevelAS,
								gl_RayFlagsOpaqueEXT,
								0xff,
								0, 0, 0,
								reflectOrigin,
								0.001,
								reflectDir,
								10000.0,
								0
					);
					reflectedColor = payload.color;
				}
				else
				{
					// 깊은 재귀에서는 스카이 근사로 대체
					float skyT = 0.5 * (reflectDir.y + 1.0);
					reflectedColor = mix(vec3(0.5, 0.7, 1.0), vec3(0.1, 0.2, 0.4), skyT) * 0.5;
				}

				// Fresnel 혼합
				vec3 transColor = mix(refractedColor, reflectedColor, fresnel);

				// transmission 비율로 opaque와 혼합
				vec3 L = normalize(ubo.lightPos.xyz - position);
				float NdotL = max(dot(faceNormal, L), 0.0);
				vec3 opaqueColor = baseColor.rgb * ubo.lightColor.rgb * ubo.lightColor.w * NdotL + ubo.globalAmbient.rgb * baseColor.rgb;

				vec3 color = mix(opaqueColor, transColor, transmission) + emissive;

				payload.color = color;
				return;
			}

			// === 기존 Opaque PBR 로직 ===
			vec3 L = normalize(ubo.lightPos.xyz - position);
			vec3 H = normalize(V + L);

			// PBR BRDF
			vec3 F0 = vec3(0.04);
			F0 = mix(F0, baseColor.rgb, metallic);

			vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
			float NDF = distributionGGX(normal, H, roughness);
			float G = geometrySmith(normal, V, L, roughness);

			vec3 numerator = NDF * G * F;
			float denominator = 4.0 * max(dot(normal, V), 0.0) * max(dot(normal, L), 0.0);
			vec3 specular = numerator / max(denominator, 0.001);

			vec3 kS = F;
			vec3 kD = vec3(1.0) - kS;
			kD *= 1.0 - metallic;

			float NdotL = max(dot(normal, L), 0.0);
			vec3 Lo = (kD * baseColor.rgb / PI + specular) * ubo.lightColor.rgb * ubo.lightColor.w * NdotL;

			vec3 ambient = ubo.globalAmbient.rgb * baseColor.rgb;

			// 그림자 레이
			float tmin = 0.001;
			float tmax = length(ubo.lightPos.xyz - position);
			vec3 shadowRayOrigin = position + normal * 0.001;
			vec3 shadowRayDirection = normalize(ubo.lightPos.xyz - position);

			uint shadowRayFlags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;

			// 그림자 레이 1: 불투명 물체만 (cullMask 0x01) → 가려지면 완전 차단
			shadowHitValue = vec3(0.0);
			traceRayEXT(topLevelAS,
						shadowRayFlags,
						0x01,
						0, 0, 1,
						shadowRayOrigin,
						tmin,
						shadowRayDirection,
						tmax,
						1
			);
			vec3 shadowOpaque = shadowHitValue;

			// 그림자 레이 2: 유리 포함 전체 (cullMask 0xff) → 유리에만 가려지면 빛 일부 투과
			shadowHitValue = vec3(0.0);
			traceRayEXT(topLevelAS,
						shadowRayFlags,
						0xff,
						0, 0, 1,
						shadowRayOrigin,
						tmin,
						shadowRayDirection,
						tmax,
						1
			);
			vec3 shadowFactor = shadowOpaque * mix(vec3(0.4), vec3(1.0), shadowHitValue);

			vec3 color = ambient + Lo * shadowFactor + emissive;

			payload.color = color;
		}
	)";
#pragma endregion

	// 셰이더 컴파일 (GLSL 사용)
	_raygenShader = OgShaderCompiler::CreateShaderFromGLSL(
		_renderContext,
		raygenSlang,
		OgShaderType::RAYGEN,
		"RayGenShader"
	);

	if (!_raygenShader)
	{
		LOGE(OG_ID, "Failed to compile ray generation shader: %s", OgShaderCompiler::GetLastError().c_str());
		return;
	}
	_raygenShader->Retain();

	_missShader = OgShaderCompiler::CreateShaderFromGLSL(
		_renderContext,
		missGLSL,
		OgShaderType::MISS,
		"MissShader"
	);

	if (!_missShader)
	{
		LOGE(OG_ID, "Failed to compile miss shader: %s", OgShaderCompiler::GetLastError().c_str());
		return;
	}
	_missShader->Retain();

	_shadowMissShader = OgShaderCompiler::CreateShaderFromGLSL(
		_renderContext,
		shadowMissGLSL,
		OgShaderType::MISS,
		"ShadowMissShader"
	);

	if (!_shadowMissShader)
	{
		LOGE(OG_ID, "Failed to compile shadow miss shader: %s", OgShaderCompiler::GetLastError().c_str());
		return;
	}
	_shadowMissShader->Retain();

	_closestHitShader = OgShaderCompiler::CreateShaderFromGLSL(
		_renderContext,
		closestHitGLSL,
		OgShaderType::CLOSEST_HIT,
		"ClosestHitShader"
	);

	if (!_closestHitShader)
	{
		LOGE(OG_ID, "Failed to compile closest hit shader: %s", OgShaderCompiler::GetLastError().c_str());
		return;
	}
	_closestHitShader->Retain();

	// 레이트레이싱 프로그램 생성 (4 shaders: raygen, miss, shadow miss, closest hit)
	OgShaderHandle* handles[]{ _raygenShader, _missShader, _shadowMissShader, _closestHitShader };
	_rtProgram = _renderContext->CreateProgram(handles, 4);
	_rtProgram->name = "RayTracingProgram";
	_rtProgram->Retain();
}

void OgRayTracingSample::createRayTracingPipeline()
{
	// 레이트레이싱 파이프라인 설정
	Render::OgRayTracingPipelineDescriptor rtPipeDesc{};
	rtPipeDesc.name = "RayTracingPipeline";
	rtPipeDesc.resourceLayout = _rtResourceLayout;
	rtPipeDesc.maxRecursionDepth = 12; // primary(1) + refraction/TIR 최대 8회 + shadow(1) + 여유

	// 셰이더 그룹 설정 (4 groups)
	Render::OgRayTracingShaderGroup groups[4];

	// Group 0: Ray generation
	groups[0].type = Render::OgRayTracingShaderGroup::GENERAL;
	groups[0].generalShader = 0; // raygen shader index
	groups[0].closestHitShader = ~0u;
	groups[0].anyHitShader = ~0u;
	groups[0].intersectionShader = ~0u;

	// Group 1: Primary miss
	groups[1].type = Render::OgRayTracingShaderGroup::GENERAL;
	groups[1].generalShader = 1; // miss shader index
	groups[1].closestHitShader = ~0u;
	groups[1].anyHitShader = ~0u;
	groups[1].intersectionShader = ~0u;

	// Group 2: Shadow miss
	groups[2].type = Render::OgRayTracingShaderGroup::GENERAL;
	groups[2].generalShader = 2; // shadow miss shader index
	groups[2].closestHitShader = ~0u;
	groups[2].anyHitShader = ~0u;
	groups[2].intersectionShader = ~0u;

	// Group 3: Hit group (closest hit)
	groups[3].type = Render::OgRayTracingShaderGroup::TRIANGLES_HIT_GROUP;
	groups[3].generalShader = ~0u;
	groups[3].closestHitShader = 3; // closest hit shader index
	groups[3].anyHitShader = ~0u;
	groups[3].intersectionShader = ~0u;

	rtPipeDesc.shaderGroups = groups;
	rtPipeDesc.shaderGroupCount = 4;

	// 셰이더 설정 (4 shaders)
	Render::OgShaderHandle* shaders[4] = { _raygenShader, _missShader, _shadowMissShader, _closestHitShader };
	rtPipeDesc.shaders = shaders;
	rtPipeDesc.shaderCount = 4;

	_rtPipeline = _renderContext->CreateRayTracingPipeline(rtPipeDesc);
	_rtPipeline->Retain();
}

void OgRayTracingSample::createRenderTarget(uint16 width, uint16 height)
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

	// 렌더 타겟 텍스처 생성 (레이트레이싱 출력용)
	OgTextureInfo texInfo{};
	texInfo.type = OgTextureType::TEX_2D;
	texInfo.format = OgPixelFormat::R32G32B32A32_SFLOAT;
	texInfo.extent.width = width;
	texInfo.extent.height = height;
	texInfo.usage = static_cast<OgTextureUsage>(static_cast<uint16>(OgTextureUsage::COLOR_ATTACHMENT) | static_cast<uint16>(OgTextureUsage::SAMPLED) | static_cast<uint16>(OgTextureUsage::STORAGE));
	texInfo.isGenerateMipmaps = false;

	_renderTargetTexture = _renderContext->CreateTexture((void**)nullptr, texInfo, sampler);
	_renderTargetTexture->name = "RayTracingRenderTarget";
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
	_depthTexture->name = "RayTracingDepthBuffer";
	_depthTexture->Retain();

	// 렌더 패스 생성
	OgAttachment rtColor{};
	rtColor.isDepthStencilAttachment = false;
	rtColor.format = OgRenderTextureFormat::R32G32B32A32;
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
	_renderTargetRenderPass->name = "RayTracingRenderTargetPass";
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
	_renderTargetFrameBuffer->name = "RayTracingRenderTargetFrameBuffer";
	_renderTargetFrameBuffer->Retain();
}

void OgRayTracingSample::destroyRenderTarget()
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

void OgRayTracingSample::createUniformBuffer()
{
	// 초기 변환 행렬 설정
	float aspect = 1.0f;
	if (_renderTargetHeight > 0)
	{
		aspect = static_cast<float>(_renderTargetWidth) / static_cast<float>(_renderTargetHeight);
	}

	if (_useFlyCamera && _camera)
	{
		_camera->SetAspectRatio(aspect);
		glm::mat4 view = _camera->GetViewMatrix();
		glm::mat4 proj = _camera->GetProjectionMatrix();
		
		// Vulkan용 프로젝션 변환
		convertProjectionForVulkan(proj);
		
		_rtUniformData.viewInverse = glm::inverse(view);
		_rtUniformData.projInverse = glm::inverse(proj);
		_rtUniformData.cameraPos = glm::vec4(_camera->GetPosition(), 1.0f);
	}
	else
	{
		glm::vec3 camPos = glm::vec3(0.0f, 5.0f, 10.0f);
		glm::mat4 view = glm::lookAt(camPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
		
		// Vulkan용 프로젝션 변환
		convertProjectionForVulkan(proj);
		
		_rtUniformData.viewInverse = glm::inverse(view);
		_rtUniformData.projInverse = glm::inverse(proj);
		_rtUniformData.cameraPos = glm::vec4(camPos, 1.0f);
	}

	// 라이트 설정
	_rtUniformData.lightPos = glm::vec4(5.0f, 10.0f, 5.0f, 1.0f);
	_rtUniformData.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 10.0f); // 마지막 값은 intensity

	// 유니폼 버퍼 생성
	_rtUniformBuffer = _renderContext->CreateBuffer(
		&_rtUniformData,
		sizeof(RTUniformData),
		Render::OgBufferUsage::UNIFORM,
		OgMemoryOption::MAP_MANAGED
	);
	_rtUniformBuffer->Retain();
}

void OgRayTracingSample::updateUniformBuffer()
{
	if (_useFlyCamera && _camera)
	{
		glm::mat4 view = _camera->GetViewMatrix();
		glm::mat4 proj = _camera->GetProjectionMatrix();
		
		// Vulkan용 프로젝션 변환
		convertProjectionForVulkan(proj);
		
		_rtUniformData.viewInverse = glm::inverse(view);
		_rtUniformData.projInverse = glm::inverse(proj);
		_rtUniformData.cameraPos = glm::vec4(_camera->GetPosition(), 1.0f);
	}
}

void OgRayTracingSample::createAccelerationStructures()
{
	if (_geometries.empty())
		return;

	// 전체 버텍스와 인덱스를 하나의 버퍼로 합치기
	std::vector<Vertex> allVertices;
	std::vector<uint32_t> allIndices;
	std::vector<GPUGeometryInfo> allGeometryInfos;
	
	allVertices.reserve(_totalVertexCount);
	allIndices.reserve(_totalIndexCount);
	
	// BLAS 생성을 위한 geometry 정보 수집
	std::vector<Render::OgAccelStructureGeometry> blasGeometries;
	
	for (const auto& geom : _geometries)
	{
		GPUGeometryInfo info;
		info.vertexOffset = static_cast<uint32_t>(allVertices.size());
		info.indexOffset = static_cast<uint32_t>(allIndices.size());
		info.materialIndex = geom.materialIndex;
		info.padding = 0;

		allGeometryInfos.push_back(info);

		// CPU 캐시된 데이터 사용
		allVertices.insert(allVertices.end(), geom.cpuVertices.begin(), geom.cpuVertices.end());
		allIndices.insert(allIndices.end(), geom.cpuIndices.begin(), geom.cpuIndices.end());
	}
	
	// 통합 버퍼 생성 (SHADER_DEVICE_ADDRESS 포함 - BLAS 빌드에 필요)
	_vertexBuffer = _renderContext->CreateBuffer(
		allVertices.data(),
		sizeof(Vertex) * allVertices.size(),
		static_cast<Render::OgBufferUsage>(
			static_cast<uint16>(Render::OgBufferUsage::VERTEX) |
			static_cast<uint16>(Render::OgBufferUsage::STORAGE) |
			static_cast<uint16>(Render::OgBufferUsage::SHADER_DEVICE_ADDRESS) |
			static_cast<uint16>(Render::OgBufferUsage::ACCEL_STRUCTURE_BUILD_INPUT)),
		OgMemoryOption::PRIVATE_GPU
	);
	_vertexBuffer->Retain();

	_indexBuffer = _renderContext->CreateBuffer(
		allIndices.data(),
		sizeof(uint32_t) * allIndices.size(),
		static_cast<Render::OgBufferUsage>(
			static_cast<uint16>(Render::OgBufferUsage::INDEX) |
			static_cast<uint16>(Render::OgBufferUsage::STORAGE) |
			static_cast<uint16>(Render::OgBufferUsage::SHADER_DEVICE_ADDRESS) |
			static_cast<uint16>(Render::OgBufferUsage::ACCEL_STRUCTURE_BUILD_INPUT)),
		OgMemoryOption::PRIVATE_GPU
	);
	_indexBuffer->Retain();

	_geometryInfoBuffer = _renderContext->CreateBuffer(
		allGeometryInfos.data(),
		sizeof(GPUGeometryInfo) * allGeometryInfos.size(),
		Render::OgBufferUsage::STORAGE,
		OgMemoryOption::PRIVATE_GPU
	);
	_geometryInfoBuffer->Retain();

	// 메시 단위로 BLAS를 분리 생성 (노드 인스턴스가 자기 메시의 BLAS만 참조하도록)
	std::vector<int> meshBlasIndex(_loadedModel.meshes.size(), -1);

	for (size_t mi = 0; mi < _loadedModel.meshes.size(); ++mi)
	{
		blasGeometries.clear();
		uint32_t firstGeometryIndex = 0;
		uint32_t meshVertexOffset = 0;
		uint32_t meshVertexCount = 0;

		for (size_t gi = 0; gi < _geometries.size(); ++gi)
		{
			const auto& geom = _geometries[gi];
			if (geom.meshIndex != static_cast<int>(mi))
				continue;

			const auto& geoInfo = allGeometryInfos[gi];

			if (blasGeometries.empty())
			{
				firstGeometryIndex = static_cast<uint32_t>(gi);
				meshVertexOffset = geoInfo.vertexOffset;
			}

			Render::OgAccelStructureGeometry asGeom{};
			asGeom.vertexBuffer = _vertexBuffer;
			asGeom.vertexStride = sizeof(Vertex);
			asGeom.vertexCount = geom.vertexCount;
			asGeom.vertexByteOffset = geoInfo.vertexOffset * sizeof(Vertex);
			asGeom.indexBuffer = _indexBuffer;
			asGeom.indexType = OgIndexType::UINT32;
			asGeom.indexCount = geom.indexCount;
			asGeom.indexByteOffset = geoInfo.indexOffset * sizeof(uint32_t);
			asGeom.transformOffset = 0;
			blasGeometries.push_back(asGeom);

			meshVertexCount += geom.vertexCount;
		}

		if (blasGeometries.empty())
			continue;

		LOGD(OG_ID, "BLAS[mesh %zu]: geometries=%zu firstGeometryIndex=%u",
			mi, blasGeometries.size(), firstGeometryIndex);

		// BLAS 생성
		Render::OgAccelStructureBuildInfo blasBuildInfo{};
		blasBuildInfo.type = Render::OgAccelStructureType::BOTTOM_LEVEL;
		blasBuildInfo.flags = Render::OgRayTracingBuildFlag::PREFER_FAST_TRACE;
		blasBuildInfo.bottomLevel.geometries = blasGeometries.data();
		blasBuildInfo.bottomLevel.geometryCount = static_cast<uint32_t>(blasGeometries.size());

		// BLAS를 생성하고 빌드
		Render::OgAccelStructureHandle* blas = _renderContext->CreateAccelerationStructure(blasBuildInfo);
		blas->Retain();

		// BLAS 즉시 빌드
		_renderContext->BuildAccelerationStructureImmediate(blas, blasBuildInfo);

		// BLAS instance 저장 (primitiveOffset = 이 메시의 첫 geometry 인덱스, TLAS instanceCustomIndex로 사용)
		BLASInstance instance;
		instance.blas = blas;
		instance.primitiveOffset = firstGeometryIndex;
		instance.primitiveCount = static_cast<uint32_t>(blasGeometries.size());
		instance.vertexOffset = meshVertexOffset;
		instance.vertexCount = meshVertexCount;
		meshBlasIndex[mi] = static_cast<int>(_blasInstances.size());
		_blasInstances.push_back(instance);
	}
	
	// TLAS 생성을 위한 인스턴스 데이터 준비
	// Vulkan 구현에서는 VkAccelerationStructureInstanceKHR 사용
	// 현재 테스트를 위해 단순화
	
	// Instance 버퍼 생성 (임시)
	struct InstanceData {
		float transform[3][4];
		uint32_t instanceCustomIndex : 24;
		uint32_t mask : 8;
		uint32_t instanceShaderBindingTableRecordOffset : 24;
		uint32_t flags : 8;
		uint64_t accelerationStructureReference;
	};
	
	std::vector<InstanceData> tlasInstances;
	for (const auto& inst : _instances)
	{
		if (inst.meshIndex >= meshBlasIndex.size() || meshBlasIndex[inst.meshIndex] < 0)
			continue;

		const BLASInstance& meshBlas = _blasInstances[meshBlasIndex[inst.meshIndex]];

		InstanceData tlasInst{};

		// Transform matrix (3x4 row-major)
		glm::mat4 transform = inst.transform;
		for (int row = 0; row < 3; ++row)
		{
			for (int col = 0; col < 4; ++col)
			{
				tlasInst.transform[row][col] = transform[col][row];
			}
		}

		// 이 메시에 투과(유리) 재질이 있으면 그림자 레이(cullMask 0x01)에서 제외
		bool transmissive = false;
		for (const auto& geom : _geometries)
		{
			if (geom.meshIndex == static_cast<int>(inst.meshIndex) &&
				geom.materialIndex < _materials.size() &&
				_materials[geom.materialIndex].transmissionFactor > 0.0f)
			{
				transmissive = true;
				break;
			}
		}

		// 셰이더에서 geometryInfo 조회 시 베이스 인덱스로 사용
		tlasInst.instanceCustomIndex = meshBlas.primitiveOffset;
		tlasInst.mask = transmissive ? 0xFE : 0xFF;
		tlasInst.instanceShaderBindingTableRecordOffset = 0;
		tlasInst.flags = 0; // TRIANGLE_FACING_CULL_DISABLE
		tlasInst.accelerationStructureReference = meshBlas.blas->deviceAddress;

		tlasInstances.push_back(tlasInst);
	}
	
	_instanceBuffer = _renderContext->CreateBuffer(
		tlasInstances.data(),
		sizeof(InstanceData) * tlasInstances.size(),
		static_cast<OgBufferUsage>(
			static_cast<uint16>(OgBufferUsage::STORAGE) |
			static_cast<uint16>(OgBufferUsage::SHADER_DEVICE_ADDRESS) |
			static_cast<uint16>(OgBufferUsage::ACCEL_STRUCTURE_BUILD_INPUT)
		),
		OgMemoryOption::PRIVATE_GPU
	);
	_instanceBuffer->Retain();
	
	// TLAS 생성
	Render::OgAccelStructureBuildInfo tlasBuildInfo{};
	tlasBuildInfo.type = Render::OgAccelStructureType::TOP_LEVEL;
	tlasBuildInfo.flags = Render::OgRayTracingBuildFlag::PREFER_FAST_TRACE;
	tlasBuildInfo.topLevel.instanceBuffer = _instanceBuffer;
	tlasBuildInfo.topLevel.instanceCount = static_cast<uint32_t>(tlasInstances.size());
	
	_tlas = _renderContext->CreateAccelerationStructure(tlasBuildInfo);
	_tlas->Retain();

	// TLAS 즉시 빌드
	_renderContext->BuildAccelerationStructureImmediate(_tlas, tlasBuildInfo);
	
	// Shader Binding Table 생성
	createShaderBindingTable();
	
	// 리소스 셋 생성
	uint32 zeroOffset = 0;
	OgResourceUsage usages[8];
	
	// TLAS - Acceleration Structure
	usages[0].binding.type = OgResourceType::ACCELERATION_STRUCTURE;
	usages[0].binding.stage = static_cast<OgShaderType>(static_cast<uint16>(OgShaderType::RAYGEN) | static_cast<uint16>(OgShaderType::CLOSEST_HIT));
	usages[0].binding.binding = 0;
	usages[0].accelStructure.handle = &_tlas;
	
	// 출력 이미지
	usages[1].binding.type = OgResourceType::STORAGE_IMAGE;
	usages[1].binding.stage = OgShaderType::RAYGEN;
	usages[1].binding.binding = 1;
	usages[1].texture.handle = &_renderTargetTexture;
	
	// 유니폼 버퍼
	usages[2].binding.type = OgResourceType::UNIFORM_BUFFER;
	usages[2].binding.stage = static_cast<OgShaderType>(static_cast<uint16>(OgShaderType::RAYGEN) | static_cast<uint16>(OgShaderType::CLOSEST_HIT) | static_cast<uint16>(OgShaderType::MISS));
	usages[2].binding.binding = 2;
	usages[2].buffer.handle = &_rtUniformBuffer;
	usages[2].buffer.offset = &zeroOffset;
	usages[2].buffer.range = &_rtUniformBuffer->size;
	
	// 버텍스 버퍼
	usages[3].binding.type = OgResourceType::STORAGE_BUFFER;
	usages[3].binding.stage = OgShaderType::CLOSEST_HIT;
	usages[3].binding.binding = 3;
	usages[3].buffer.handle = &_vertexBuffer;
	usages[3].buffer.offset = &zeroOffset;
	usages[3].buffer.range = &_vertexBuffer->size;
	
	// 인덱스 버퍼
	usages[4].binding.type = OgResourceType::STORAGE_BUFFER;
	usages[4].binding.stage = OgShaderType::CLOSEST_HIT;
	usages[4].binding.binding = 4;
	usages[4].buffer.handle = &_indexBuffer;
	usages[4].buffer.offset = &zeroOffset;
	usages[4].buffer.range = &_indexBuffer->size;
	
	// Material 버퍼
	usages[5].binding.type = OgResourceType::STORAGE_BUFFER;
	usages[5].binding.stage = OgShaderType::CLOSEST_HIT;
	usages[5].binding.binding = 5;
	usages[5].buffer.handle = &_materialBuffer;
	usages[5].buffer.offset = &zeroOffset;
	usages[5].buffer.range = &_materialBuffer->size;
	
	// GeometryInfo 버퍼
	usages[6].binding.type = OgResourceType::STORAGE_BUFFER;
	usages[6].binding.stage = OgShaderType::CLOSEST_HIT;
	usages[6].binding.binding = 6;
	usages[6].buffer.handle = &_geometryInfoBuffer;
	usages[6].buffer.offset = &zeroOffset;
	usages[6].buffer.range = &_geometryInfoBuffer->size;
	
	// 텍스처 배열
	std::vector<Render::OgTextureHandle*> texHandles;
	for (auto* tex : _textureArray)
	{
		texHandles.push_back(tex);
	}
	// 배열이 16개가 되도록 기본 텍스처로 채우기
	while (texHandles.size() < 16)
	{
		texHandles.push_back(_defaultWhiteTexture);
	}
	
	// 텍스처 배열 - 16개 모두 바인딩
	usages[7].binding.type = OgResourceType::COMBINED_IMAGE_SAMPLER;
	usages[7].binding.stage = OgShaderType::CLOSEST_HIT;
	usages[7].binding.binding = 7;
	usages[7].binding.arrayCount = 16;
	usages[7].texture.handle = texHandles.data();
	
	_rtResourceSet = _renderContext->CreateResourceSet(_rtResourceLayout, usages, 8);
	_rtResourceSet->name = "RayTracingResourceSet";
	_rtResourceSet->Retain();
}

void OgRayTracingSample::createShaderBindingTable()
{
	// SBT handle alignment 정보 가져오기
	_sbtHandleSizeAligned = _renderContext->GetShaderGroupHandleSizeAligned();

	// 셰이더 그룹 설정 (4 groups: raygen, miss, shadow miss, hit)
	Render::OgRayTracingShaderGroup groups[4];

	groups[0].type = Render::OgRayTracingShaderGroup::GENERAL;
	groups[0].generalShader = 0;
	groups[0].closestHitShader = ~0u;
	groups[0].anyHitShader = ~0u;
	groups[0].intersectionShader = ~0u;

	groups[1].type = Render::OgRayTracingShaderGroup::GENERAL;
	groups[1].generalShader = 1;
	groups[1].closestHitShader = ~0u;
	groups[1].anyHitShader = ~0u;
	groups[1].intersectionShader = ~0u;

	groups[2].type = Render::OgRayTracingShaderGroup::GENERAL;
	groups[2].generalShader = 2;
	groups[2].closestHitShader = ~0u;
	groups[2].anyHitShader = ~0u;
	groups[2].intersectionShader = ~0u;

	groups[3].type = Render::OgRayTracingShaderGroup::TRIANGLES_HIT_GROUP;
	groups[3].generalShader = ~0u;
	groups[3].closestHitShader = 3;
	groups[3].anyHitShader = ~0u;
	groups[3].intersectionShader = ~0u;

	// 전체 SBT 생성 (하나의 버퍼에 모든 핸들 포함)
	Render::OgBufferHandle* sbtBuffer = _renderContext->CreateShaderBindingTable(_rtPipeline, groups, 4);

	// 동일한 버퍼를 offset으로 구분하여 사용
	_raygenSBT = sbtBuffer;
	_missSBT = sbtBuffer;
	_hitSBT = sbtBuffer;

	sbtBuffer->Retain();
	sbtBuffer->Retain();
	sbtBuffer->Retain();
}

void OgRayTracingSample::updateTLAS()
{
	// TLAS 업데이트가 필요한 경우 호출
	// 예: 인스턴스 변환이 변경되었을 때
}

void OgRayTracingSample::destroyAccelerationStructures()
{
	// SBT 버퍼들 해제
	if (_raygenSBT)
	{
		_raygenSBT->Release();
		_raygenSBT = nullptr;
	}
	
	if (_missSBT)
	{
		_missSBT->Release();
		_missSBT = nullptr;
	}
	
	if (_hitSBT)
	{
		_hitSBT->Release();
		_hitSBT = nullptr;
	}
	
	// TLAS 해제
	if (_tlas)
	{
		_tlas->Release();
		_tlas = nullptr;
	}
	
	// BLAS 해제
	for (auto& instance : _blasInstances)
	{
		if (instance.blas)
		{
			instance.blas->Release();
		}
	}
	_blasInstances.clear();
	
	// 버퍼들 해제
	if (_instanceBuffer)
	{
		_instanceBuffer->Release();
		_instanceBuffer = nullptr;
	}
	
	if (_scratchBuffer)
	{
		_scratchBuffer->Release();
		_scratchBuffer = nullptr;
	}
	
	if (_vertexBuffer)
	{
		_vertexBuffer->Release();
		_vertexBuffer = nullptr;
	}
	
	if (_indexBuffer)
	{
		_indexBuffer->Release();
		_indexBuffer = nullptr;
	}
	
	if (_geometryInfoBuffer)
	{
		_geometryInfoBuffer->Release();
		_geometryInfoBuffer = nullptr;
	}
}

bool OgRayTracingSample::loadGLTFModel(const std::string& filePath)
{
	// 기존 모델 데이터 클리어
	clearModelData();

	// OgGLTFLoader를 사용해서 모델 로드
	if (!_gltfLoader->LoadModel(filePath, _loadedModel))
	{
		LOGE(OG_ID, "Failed to load glTF model: %s", filePath.c_str());
		LOGE(OG_ID, "Error: %s", _gltfLoader->GetLastError().c_str());
		return false;
	}

	// 로드된 모델을 레이트레이싱용으로 처리
	for (int i = 0; i < static_cast<int>(_loadedModel.meshes.size()); ++i)
	{
		processMeshForRayTracing(_loadedModel.meshes[i], i);
	}

	// Material 데이터 준비
	_materials.clear();
	for (const auto& mat : _loadedModel.materials)
	{
		MaterialData matData{};
		matData.baseColorFactor = mat.baseColorFactor;
		matData.emissiveFactor = glm::vec4(mat.emissiveFactor, mat.emissiveStrength);
		matData.metallicFactor = mat.metallicFactor;
		matData.roughnessFactor = mat.roughnessFactor;
		matData.transmissionFactor = mat.transmissionFactor;
		matData.ior = 1.5; // 기본 유리 IOR (KHR_materials_ior 기본값)
		matData.attenuationColor = glm::vec4(mat.attenuationColor, 1.0f);
		matData.attenuationDistance = mat.attenuationDistance;
		
		// 텍스처 인덱스 설정
		matData.baseColorTextureIndex = mat.baseColorTexture ? 
			static_cast<int32_t>(_textureArray.size()) : -1;
		if (mat.baseColorTexture)
		{
			_textureArray.push_back(mat.baseColorTexture);
		}
		
		matData.normalTextureIndex = mat.normalTexture ? 
			static_cast<int32_t>(_textureArray.size()) : -1;
		if (mat.normalTexture)
		{
			_textureArray.push_back(mat.normalTexture);
		}
		
		matData.metallicRoughnessTextureIndex = mat.metallicRoughnessTexture ? 
			static_cast<int32_t>(_textureArray.size()) : -1;
		if (mat.metallicRoughnessTexture)
		{
			_textureArray.push_back(mat.metallicRoughnessTexture);
		}
		
		_materials.push_back(matData);
	}

	// Material 버퍼 생성
	if (!_materials.empty())
	{
		_materialBuffer = _renderContext->CreateBuffer(
			_materials.data(),
			sizeof(MaterialData) * _materials.size(),
			Render::OgBufferUsage::STORAGE,
			OgMemoryOption::PRIVATE_GPU
		);
		_materialBuffer->Retain();
	}

	// 노드 처리 (인스턴스 생성)
	_instances.clear();
	for (int rootNode : _loadedModel.rootNodes)
	{
		processNodeForRayTracing(rootNode, glm::mat4(1.0f));
	}

	// 인스턴스가 하나도 없으면 기본 인스턴스 생성
	if (_instances.empty())
	{
		InstanceData inst{};
		inst.transform = glm::mat4(1.0f);
		inst.transformInverse = glm::mat4(1.0f);
		inst.meshIndex = 0;
		inst.materialIndex = 0;
		_instances.push_back(inst);
	}

	return true;
}

void OgRayTracingSample::clearModelData()
{
	// 텍스처 배열 클리어 (기본 텍스처는 제외)
	_textureArray.clear();
	
	// 지오메트리 정보 클리어
	_geometries.clear();
	_materials.clear();
	_instances.clear();
	
	_totalVertexCount = 0;
	_totalIndexCount = 0;
	
	// 프레임 카운터 리셋
	_frameCount = 0;
	
	// OgGLTFLoader를 사용해서 모델 데이터 클리어
	if (_gltfLoader)
	{
		_gltfLoader->ClearModel(_loadedModel);
	}
}

void OgRayTracingSample::processMeshForRayTracing(const Mesh& mesh, int meshIndex)
{
	for (const auto& primitive : mesh.primitives)
	{
		if (primitive.vertexBuffer && primitive.vertexCount > 0)
		{
			GeometryInfo geom;
			geom.vertexBuffer = primitive.vertexBuffer;
			geom.indexBuffer = primitive.indexBuffer;
			geom.vertexCount = primitive.vertexCount;
			geom.indexCount = primitive.indexCount;
			geom.materialIndex = (primitive.materialIndex >= 0) ? primitive.materialIndex : 0;
			geom.meshIndex = meshIndex;

			// CPU side vertex data copy (MapBuffer for MAP_MANAGED buffers)
			geom.cpuVertices.resize(primitive.vertexCount);
			void* vData = _renderContext->MapBuffer(primitive.vertexBuffer, sizeof(Vertex) * primitive.vertexCount);
			if (vData)
			{
				memcpy(geom.cpuVertices.data(), vData, sizeof(Vertex) * primitive.vertexCount);
				_renderContext->UnmapBuffer(primitive.vertexBuffer);
			}

			// CPU side index data copy
			if (primitive.indexCount > 0 && primitive.indexBuffer)
			{
				geom.cpuIndices.resize(primitive.indexCount);
				void* iData = _renderContext->MapBuffer(primitive.indexBuffer, primitive.indexBuffer->size);
				if (iData)
				{
					if (primitive.indexType == 5123) // UINT16
					{
						uint16_t* idx16 = static_cast<uint16_t*>(iData);
						for (uint32_t i = 0; i < primitive.indexCount; ++i)
							geom.cpuIndices[i] = idx16[i];
					}
					else // UINT32
					{
						memcpy(geom.cpuIndices.data(), iData, sizeof(uint32_t) * primitive.indexCount);
					}
					_renderContext->UnmapBuffer(primitive.indexBuffer);
				}
			}

			_geometries.push_back(std::move(geom));

			_totalVertexCount += primitive.vertexCount;
			_totalIndexCount += primitive.indexCount;
		}
	}
}

void OgRayTracingSample::processNodeForRayTracing(int nodeIndex, const glm::mat4& parentMatrix)
{
	if (nodeIndex < 0 || nodeIndex >= static_cast<int>(_loadedModel.nodes.size()))
	{
		return;
	}

	const Node& node = _loadedModel.nodes[nodeIndex];
	glm::mat4 nodeMatrix = parentMatrix * node.matrix;

	// 이 노드에 메시가 있으면 인스턴스 생성
	if (node.meshIndex >= 0 && node.meshIndex < static_cast<int>(_loadedModel.meshes.size()))
	{
		InstanceData inst;
		inst.transform = nodeMatrix;
		inst.transformInverse = glm::inverse(nodeMatrix);
		inst.meshIndex = node.meshIndex;
		inst.materialIndex = 0; // 기본 material
		_instances.push_back(inst);
	}

	// 자식 노드들 처리
	for (int childIndex : node.children)
	{
		processNodeForRayTracing(childIndex, nodeMatrix);
	}
}

void OgRayTracingSample::convertProjectionForVulkan(glm::mat4& projection)
{
	// GLM의 기본 프로젝션은 OpenGL을 위한 것이므로 Vulkan용으로 변환
	// 1. Y축 뒤집기 (Vulkan은 Y축이 아래로 향함)
	projection[1][1] *= -1.0f;
}

OG_NAMESPACE_SAMPLE_END