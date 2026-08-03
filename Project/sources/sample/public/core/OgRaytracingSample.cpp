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
	// 移대찓??珥덇린 ?ㅼ젙
	_camera->SetPosition(glm::vec3(0.0f, 5.0f, 10.0f));
	_camera->SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));
	
	// ?덉씠?몃젅?댁떛 珥덇린 ?ㅼ젙
	_rtUniformData.maxBounces = 8;
	_rtUniformData.samplesPerPixel = 1;
	_rtUniformData.surfaceColorVariant = 0;
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

	// ?덉씠?몃젅?댁떛 吏???뺤씤
	if (!_renderContext->IsRayTracingSupported())
	{
		LOGE(OG_ID, "Ray tracing is not supported on this device");
		return;
	}

	// ?ㅼ솑泥댁씤???ш린濡?由ъ냼???앹꽦
	const uint16 width = _renderContext->GetSwapChainFrameBuffer(swapchain, 0)->width;
	const uint16 height = _renderContext->GetSwapChainFrameBuffer(swapchain, 0)->height;

	// ?뚮뜑 ?寃??앹꽦
	createRenderTarget(width, height);

	// 由ъ냼???앹꽦
	createResources(width, height);

	// DragonAttenuation 紐⑤뜽 濡쒕뱶
	std::filesystem::path modelPath = "C:/Osgood/EngineDevelop/OGHypeEngine/Project/res/models/DragonAttenuation/glTF/DragonAttenuation.gltf";
	
	if (!loadGLTFModel(modelPath.string()))
	{
		LOGE(OG_ID, "Failed to load DragonAttenuation model");
		return;
	}

	// Acceleration Structure ?앹꽦
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
	// 移대찓???낅뜲?댄듃
	if (_useFlyCamera && _camera)
	{
		_camera->Update(deltaTime);
		
		// 移대찓?쇨? ?吏곸??쇰㈃ ?꾨젅??移댁슫??由ъ뀑
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

	// ?꾨젅??移댁슫??利앷?
	_rtUniformData.frameCount = _frameCount++;

	// ?좊땲??踰꾪띁 ?낅뜲?댄듃
	void* mappedData = _renderContext->MapBuffer(_rtUniformBuffer, sizeof(RTUniformData));
	if (mappedData)
	{
		memcpy(mappedData, &_rtUniformData, sizeof(RTUniformData));
		_renderContext->UnmapBuffer(_rtUniformBuffer);
	}

	// ?뚮뜑 ?⑥뒪 ?쒖옉 (?덉씠?몃젅?댁떛 寃곌낵瑜?蹂듭궗?섍린 ?꾪븳 以鍮?
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

	// ?덉씠?몃젅?댁떛 ?뚯씠?꾨씪??諛붿씤??
	encoder->BindRayTracingPipeline(_rtPipeline);
	encoder->BindResourceSet(_rtResourceSet);

	// ?덉씠?몃젅?댁떛 ?붿뒪?⑥튂 - SBT ?덉씠?꾩썐: [raygen][miss][shadowMiss][hit]
	uint32_t aligned = _sbtHandleSizeAligned;

	Render::OgShaderBindingTable sbt{};
	sbt.raygenSBT = _raygenSBT;
	sbt.raygenOffset = 0;
	sbt.raygenStride = aligned;
	sbt.raygenSize = aligned;

	sbt.missSBT = _missSBT;
	sbt.missOffset = 1 * aligned;       // raygen ?ㅼ쓬
	sbt.missStride = aligned;
	sbt.missSize = 2 * aligned;          // miss 2媛?(primary + shadow)

	sbt.hitSBT = _hitSBT;
	sbt.hitOffset = 3 * aligned;         // raygen + 2 miss ?ㅼ쓬
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

	// ?ш린媛 蹂寃쎈릺硫??뚮뜑 ?寃??ъ깮??
	destroyRenderTarget();
	createRenderTarget(static_cast<uint16>(width), static_cast<uint16>(height));

	// ?꾨줈?앹뀡 ?됰젹 ?낅뜲?댄듃
	float aspect = static_cast<float>(width) / static_cast<float>(height);
	if (_useFlyCamera && _camera)
	{
		_camera->SetAspectRatio(aspect);
	}
	
	// ?꾨젅??移댁슫??由ъ뀑
	_frameCount = 0;
	updateUniformBuffer();
}

// ?낅젰 泥섎━ 硫붿꽌?쒕뱾
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

	// F ?ㅻ줈 ?뚮씪??移대찓???좉?
	if (key == OG_KEY_F && action == OG_PRESS)
	{
		_useFlyCamera = !_useFlyCamera;
		_frameCount = 0;
		updateUniformBuffer();
	}

	// D ?ㅻ줈 ?붾쾭洹??쒓컖???좉?
	if (key == OG_KEY_D && action == OG_PRESS)
	{
		_enableDebugVisualization = !_enableDebugVisualization;
		_frameCount = 0;
	}
}

void OgRayTracingSample::createResources(uint16 width, uint16 height)
{
	// ?곗씠???앹꽦
	createShaders();

	// ?좊땲??踰꾪띁 ?앹꽦
	createUniformBuffer();

	// 湲곕낯 ?띿뒪泥??앹꽦
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

	// 湲곕낯 ?몃쭚 ?띿뒪泥?(0.5, 0.5, 1.0, 1.0) - 以묒꽦 ?몃쭚
	uint32_t normalPixel = 0xFFFF8080; // RGBA = (128, 128, 255, 255)
	void* normalData = &normalPixel;
	_defaultNormalTexture = _renderContext->CreateTexture(&normalData, whiteTexInfo, _renderContext->CreateSampler(samplerInfo));
	_defaultNormalTexture->Retain();

	// 由ъ냼???덉씠?꾩썐 ?앹꽦
	OgResourceBinding bindings[8];
	
	// TLAS
	bindings[0].type = OgResourceType::ACCELERATION_STRUCTURE;
	bindings[0].stage = static_cast<OgShaderType>(static_cast<uint16>(OgShaderType::RAYGEN) | static_cast<uint16>(OgShaderType::CLOSEST_HIT));
	bindings[0].binding = 0;
	bindings[0].arrayCount = 0;
	bindings[0].name = nullptr;

	// 異쒕젰 ?대?吏
	bindings[1].type = OgResourceType::STORAGE_IMAGE;
	bindings[1].stage = OgShaderType::RAYGEN;
	bindings[1].binding = 1;
	bindings[1].arrayCount = 0;
	bindings[1].name = nullptr;

	// ?좊땲??踰꾪띁
	bindings[2].type = OgResourceType::UNIFORM_BUFFER;
	bindings[2].stage = static_cast<OgShaderType>(static_cast<uint16>(OgShaderType::RAYGEN) | static_cast<uint16>(OgShaderType::CLOSEST_HIT) | static_cast<uint16>(OgShaderType::MISS));
	bindings[2].binding = 2;
	bindings[2].arrayCount = 0;
	bindings[2].name = nullptr;

	// 踰꾪뀓??踰꾪띁 (?ㅽ넗由ъ? 踰꾪띁濡??ъ슜)
	bindings[3].type = OgResourceType::STORAGE_BUFFER;
	bindings[3].stage = OgShaderType::CLOSEST_HIT;
	bindings[3].binding = 3;
	bindings[3].arrayCount = 0;
	bindings[3].name = nullptr;

	// ?몃뜳??踰꾪띁
	bindings[4].type = OgResourceType::STORAGE_BUFFER;
	bindings[4].stage = OgShaderType::CLOSEST_HIT;
	bindings[4].binding = 4;
	bindings[4].arrayCount = 0;
	bindings[4].name = nullptr;

	// Material 踰꾪띁
	bindings[5].type = OgResourceType::STORAGE_BUFFER;
	bindings[5].stage = OgShaderType::CLOSEST_HIT;
	bindings[5].binding = 5;
	bindings[5].arrayCount = 0;
	bindings[5].name = nullptr;

	// GeometryInfo 踰꾪띁
	bindings[6].type = OgResourceType::STORAGE_BUFFER;
	bindings[6].stage = OgShaderType::CLOSEST_HIT;
	bindings[6].binding = 6;
	bindings[6].arrayCount = 0;
	bindings[6].name = nullptr;

	// ?띿뒪泥?諛곗뿴
	bindings[7].type = OgResourceType::COMBINED_IMAGE_SAMPLER;
	bindings[7].stage = OgShaderType::CLOSEST_HIT;
	bindings[7].binding = 7;
	bindings[7].arrayCount = 16; // 理쒕? 16媛쒖쓽 ?띿뒪泥?
	bindings[7].name = nullptr;

	_rtResourceLayout = _renderContext->CreateResourceLayout(bindings, 8);
	_rtResourceLayout->name = "RayTracingResourceLayout";
	_rtResourceLayout->Retain();

	// ?덉씠?몃젅?댁떛 ?뚯씠?꾨씪???앹꽦
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

	// SBT 踰꾪띁???댁젣
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
	// Ray Generation ?곗씠??(Slang)
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
			uint surfaceColorVariant;
		} ubo;

		struct RayPayload {
			vec3 position;
			float hitT;
			vec3 normal;
			float transmission;
			vec3 baseColor;
			float ior;
			vec3 emissive;
			float attenuationDistance;
			vec3 attenuationColor;
			float metallic;
		};
		layout(location = 0) rayPayloadEXT RayPayload payload;
		layout(location = 1) rayPayloadEXT vec3 shadowHitValue;

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

		vec3 randomUnitVector()
		{
			float z = randomFloat() * 2.0 - 1.0;
			float a = randomFloat() * 6.28318530718;
			float r = sqrt(max(0.0, 1.0 - z * z));
			return vec3(r * cos(a), r * sin(a), z);
		}

		// 肄붿궗??媛以?諛섍뎄 ?섑뵆留?(?뷀벂利?諛붿슫??諛⑺뼢)
		vec3 cosineSampleHemisphere(vec3 n)
		{
			vec3 d = n + randomUnitVector();
			float len = length(d);
			return len < 0.001 ? n : d / len;
		}

		vec3 skyColor(vec3 dir)
		{
			float t = 0.5 * (normalize(dir).y + 1.0);
			return mix(vec3(0.5, 0.7, 1.0), vec3(0.1, 0.2, 0.4), t);
		}

		// ?쒖떆???몄퐫??Reinhard ?ㅻℓ??+ 媛먮쭏). ????섏씠 媛?ν빐??
		// ?섎굹???대?吏濡??좏삎 怨듦컙 ?꾩쟻怨??붾㈃ ?쒖떆瑜?寃명븳??
		vec3 encodeDisplay(vec3 c)
		{
			c = c / (c + vec3(1.0));
			return pow(c, vec3(1.0 / 2.2));
		}

		vec3 decodeDisplay(vec3 d)
		{
			vec3 c = pow(d, vec3(2.2));
			c = min(c, vec3(0.999));
			return c / (vec3(1.0) - c);
		}

		void main()
		{
			uvec2 launchID = gl_LaunchIDEXT.xy;
			uvec2 launchSize = gl_LaunchSizeEXT.xy;

			rngState = pcg_hash(launchID.y * 9277u + launchID.x * 1973u + ubo.frameCount * 26699u);

			uint spp = max(ubo.samplesPerPixel, 1u);
			vec3 color = vec3(0.0);

			for (uint s = 0u; s < spp; ++s)
			{
				// ?쎌? ??吏??(?덊떚?⑤━?댁떛)
				vec2 jitter = vec2(randomFloat(), randomFloat()) - 0.5;
				vec2 pixelCenter = vec2(launchID) + vec2(0.5) + jitter;
				vec2 inUV = pixelCenter / vec2(launchSize);
				vec2 d = inUV * 2.0 - 1.0;

				vec4 origin4 = ubo.viewInverse * vec4(0, 0, 0, 1);
				vec4 target = ubo.projInverse * vec4(d.x, d.y, 1, 1);
				vec4 direction4 = ubo.viewInverse * vec4(normalize(target.xyz), 0);

				vec3 origin = origin4.xyz;
				vec3 dir = direction4.xyz;

				// === ?⑥뒪 ?몃젅?댁떛 猷⑦봽 ===
				vec3 radiance = vec3(0.0);
				vec3 throughput = vec3(1.0);

				for (uint bounce = 0u; bounce <= ubo.maxBounces; ++bounce)
				{
					payload.hitT = -1.0;
					traceRayEXT(topLevelAS,
								gl_RayFlagsOpaqueEXT,
								0xff,
								0, 0, 0,
								origin,
								0.001,
								dir,
								10000.0,
								0
					);

					if (payload.hitT < 0.0)
					{
						// miss: ?섎뒛鍮??섏쭛 ??寃쎈줈 醫낅즺
						radiance += throughput * skyColor(dir);
						break;
					}

					float NdotD = dot(payload.normal, dir);
					bool backface = NdotD > 0.0;
					vec3 faceN = backface ? -payload.normal : payload.normal;

					// ?좊━ ?대?瑜?吏?섏삩 ?멸렇癒쇳듃??Beer-Lambert 媛먯뇿
					if (backface && payload.transmission > 0.0 && payload.attenuationDistance > 0.0)
					{
						vec3 absorb = -log(payload.attenuationColor + 0.001) / payload.attenuationDistance;
						throughput *= exp(-absorb * payload.hitT);
					}

					radiance += throughput * payload.emissive;

					if (payload.transmission > 0.0)
					{
						// ?좊━: ?꾨젅???뺣쪧濡?諛섏궗/援댁젅 以??섎굹留??섑뵆留?(寃쎈줈 遺꾧린 ?놁쓬)
						if (bounce == ubo.maxBounces)
							break;

						float ior = payload.ior <= 0.0 ? 1.5 : payload.ior;
						float eta = backface ? ior : (1.0 / ior);
						vec3 refr = refract(dir, faceN, eta);
						float f0 = pow((1.0 - ior) / (1.0 + ior), 2.0);
						float fres = f0 + (1.0 - f0) * pow(clamp(1.0 - abs(NdotD), 0.0, 1.0), 5.0);
						bool tir = dot(refr, refr) < 0.0001;

						if (tir || randomFloat() < fres)
						{
							dir = reflect(dir, faceN);
							origin = payload.position + faceN * 0.001;
						}
						else
						{
							dir = refr;
							origin = payload.position - faceN * 0.001;
							// glTF ?ㅽ럺: ?ш낵愿묒? baseColor濡??댄듃 (Attenuation ?ъ쭏? ?곗깋?대씪 ?곹뼢 ?놁쓬)
							throughput *= payload.baseColor;
						}
					}
					else
					{
						// 遺덊닾紐??뷀벂利?: 愿묒썝 吏곸젒 ?섑뵆留?NEE)
						vec3 lightPoint = ubo.lightPos.xyz + randomUnitVector() * 0.7; // 硫닿킅??洹쇱궗 ??遺?쒕윭??洹몃┝??
						vec3 toLight = lightPoint - payload.position;
						float lightDist = length(toLight);
						vec3 L = toLight / lightDist;
						float NdotL = max(dot(faceN, L), 0.0);

						if (NdotL > 0.0)
						{
							vec3 shadowOrigin = payload.position + faceN * 0.001;
							uint shadowRayFlags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;

							// 洹몃┝???덉씠 1: 遺덊닾紐?臾쇱껜留?(cullMask 0x01) ??媛?ㅼ?硫??꾩쟾 李⑤떒
							shadowHitValue = vec3(0.0);
							traceRayEXT(topLevelAS, shadowRayFlags, 0x01, 0, 0, 1,
										shadowOrigin, 0.001, L, lightDist, 1);
							vec3 shadowOpaque = shadowHitValue;

							// 洹몃┝???덉씠 2: ?좊━ ?ы븿 (cullMask 0xff) ???좊━?먮쭔 媛?ㅼ?硫?鍮??쇰? ?ш낵
							shadowHitValue = vec3(0.0);
							traceRayEXT(topLevelAS, shadowRayFlags, 0xff, 0, 0, 1,
										shadowOrigin, 0.001, L, lightDist, 1);
							vec3 shadowFactor = shadowOpaque * mix(vec3(0.4), vec3(1.0), shadowHitValue);

							radiance += throughput * (payload.baseColor / 3.14159265359)
								* ubo.lightColor.rgb * ubo.lightColor.w * NdotL * shadowFactor;
						}

						// 肄붿궗??諛섍뎄 ?섑뵆留곸쑝濡?寃쎈줈 ?곗옣 (媛꾩젒愿?
						if (bounce == ubo.maxBounces)
							break;

						throughput *= payload.baseColor;
						dir = cosineSampleHemisphere(faceN);
						origin = payload.position + faceN * 0.001;
					}

					// ?ъ떆??猷곕젢: 湲?寃쎈줈瑜??뺣쪧?곸쑝濡?醫낅즺
					if (bounce > 3u)
					{
						float p = clamp(max(throughput.r, max(throughput.g, throughput.b)), 0.05, 0.95);
						if (randomFloat() > p)
							break;
						throughput /= p;
					}
				}

				color += radiance;
			}

			color /= float(spp);

			// ?좏삎 怨듦컙 ?꾩쟻: ??κ컪? ?쒖떆???몄퐫?⑹씠誘濡??붿퐫?????됯퇏 ???ъ씤肄붾뵫
			if (ubo.frameCount > 0)
			{
				vec3 prevLinear = decodeDisplay(imageLoad(image, ivec2(launchID)).rgb);
				float weight = 1.0 / float(ubo.frameCount + 1);
				color = mix(prevLinear, color, weight);
			}

			imageStore(image, ivec2(launchID), vec4(encodeDisplay(color), 1.0));
		}
	)";

#pragma region missShader
	// Miss ?곗씠??(GLSL) - ?섎뒛 洹몃씪?곗씠??
	const char* missGLSL = R"(
		#version 460
		#extension GL_EXT_ray_tracing : require

		struct RayPayload {
			vec3 position;
			float hitT;
			vec3 normal;
			float transmission;
			vec3 baseColor;
			float ior;
			vec3 emissive;
			float attenuationDistance;
			vec3 attenuationColor;
			float metallic;
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
			uint surfaceColorVariant;
		} ubo;

		void main()
		{
			// miss = ?섎뒛. ?섎뒛??怨꾩궛? raygen???⑥뒪 ?몃젅?댁떛 猷⑦봽?먯꽌 ?섑뻾
			payload.hitT = -1.0;
		}
	)";
#pragma endregion

	// Shadow Miss ?곗씠??- shadow ray媛 ?꾨Т寃껊룄 ??留욎쑝硫?鍮쏆씠 ?꾨떖
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
			vec3 position;
			float hitT;
			vec3 normal;
			float transmission;
			vec3 baseColor;
			float ior;
			vec3 emissive;
			float attenuationDistance;
			vec3 attenuationColor;
			float metallic;
		};
		layout(location = 0) rayPayloadInEXT RayPayload payload;

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
			uint surfaceColorVariant;
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

			uint matIndex = geomInfo.materialIndex;
			// ?ъ쭏 蹂???좉?: DragonAttenuation??1(蹂쇰ⅷ 媛먯뇿) ??2(Surface Coloring Only)
			if (ubo.surfaceColorVariant == 1u && matIndex == 1u)
				matIndex = 2u;
			Material material = materialBuffer.materials[matIndex];

			vec4 baseColor = material.baseColorFactor;
			if (material.baseColorTextureIndex >= 0)
			{
				baseColor *= texture(textures[material.baseColorTextureIndex], texCoord);
			}

			// ?쒕㈃ ?뺣낫留?payload濡?諛섑솚 ???곗씠?⑷낵 寃쎈줈 ?곗옣? raygen???⑥뒪 ?몃젅?댁떛 猷⑦봽?먯꽌 ?섑뻾
			payload.position = position;
			payload.hitT = gl_HitTEXT;
			payload.normal = normal;
			payload.transmission = material.transmissionFactor;
			payload.baseColor = baseColor.rgb;
			payload.ior = material.ior;
			payload.emissive = material.emissiveFactor.rgb;
			payload.attenuationDistance = material.attenuationDistance;
			payload.attenuationColor = material.attenuationColor.rgb;
			payload.metallic = material.metallicFactor;
		}
	)";
#pragma endregion

	// ?곗씠??而댄뙆??(GLSL ?ъ슜)
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

	// ?덉씠?몃젅?댁떛 ?꾨줈洹몃옩 ?앹꽦 (4 shaders: raygen, miss, shadow miss, closest hit)
	OgShaderHandle* handles[]{ _raygenShader, _missShader, _shadowMissShader, _closestHitShader };
	_rtProgram = _renderContext->CreateProgram(handles, 4);
	_rtProgram->name = "RayTracingProgram";
	_rtProgram->Retain();
}

void OgRayTracingSample::createRayTracingPipeline()
{
	// ?덉씠?몃젅?댁떛 ?뚯씠?꾨씪???ㅼ젙
	Render::OgRayTracingPipelineDescriptor rtPipeDesc{};
	rtPipeDesc.name = "RayTracingPipeline";
	rtPipeDesc.resourceLayout = _rtResourceLayout;
	rtPipeDesc.maxRecursionDepth = 2; // ?⑥뒪 ?몃젅?댁떛 猷⑦봽??raygen?먯꽌留?trace (?ш? ?놁쓬)

	// ?곗씠??洹몃９ ?ㅼ젙 (4 groups)
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

	// ?곗씠???ㅼ젙 (4 shaders)
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

	// ?섑뵆???앹꽦
	OgSamplerInfo samplerInfo{};
	samplerInfo.type = OgSamplerType::TEX_2D;
	samplerInfo.addressU = OgSamplerAddressMode::CLAMP_TO_EDGE;
	samplerInfo.addressV = OgSamplerAddressMode::CLAMP_TO_EDGE;
	samplerInfo.magFilter = OgFilter::LINEAR;
	samplerInfo.minFilter = OgFilter::LINEAR;
	samplerInfo.mipmapMode = OgSamplerMipmapMode::NEAREST;

	OgSamplerHandle* sampler = _renderContext->CreateSampler(samplerInfo);

	// ?뚮뜑 ?寃??띿뒪泥??앹꽦 (?덉씠?몃젅?댁떛 異쒕젰??
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

	// 源딆씠 ?띿뒪泥??앹꽦
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

	// ?뚮뜑 ?⑥뒪 ?앹꽦
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

	// ?꾨젅?꾨쾭???앹꽦
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
	// 珥덇린 蹂???됰젹 ?ㅼ젙
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
		
		// Vulkan???꾨줈?앹뀡 蹂??
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
		
		// Vulkan???꾨줈?앹뀡 蹂??
		convertProjectionForVulkan(proj);
		
		_rtUniformData.viewInverse = glm::inverse(view);
		_rtUniformData.projInverse = glm::inverse(proj);
		_rtUniformData.cameraPos = glm::vec4(camPos, 1.0f);
	}

	// ?쇱씠???ㅼ젙
	_rtUniformData.lightPos = glm::vec4(5.0f, 10.0f, 5.0f, 1.0f);
	_rtUniformData.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 10.0f); // 留덉?留?媛믪? intensity

	// ?좊땲??踰꾪띁 ?앹꽦
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
		
		// Vulkan???꾨줈?앹뀡 蹂??
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

	// ?꾩껜 踰꾪뀓?ㅼ? ?몃뜳?ㅻ? ?섎굹??踰꾪띁濡??⑹튂湲?
	std::vector<Vertex> allVertices;
	std::vector<uint32_t> allIndices;
	std::vector<GPUGeometryInfo> allGeometryInfos;
	
	allVertices.reserve(_totalVertexCount);
	allIndices.reserve(_totalIndexCount);
	
	// BLAS ?앹꽦???꾪븳 geometry ?뺣낫 ?섏쭛
	std::vector<Render::OgAccelStructureGeometry> blasGeometries;
	
	for (const auto& geom : _geometries)
	{
		GPUGeometryInfo info;
		info.vertexOffset = static_cast<uint32_t>(allVertices.size());
		info.indexOffset = static_cast<uint32_t>(allIndices.size());
		info.materialIndex = geom.materialIndex;
		info.padding = 0;

		allGeometryInfos.push_back(info);

		// CPU 罹먯떆???곗씠???ъ슜
		allVertices.insert(allVertices.end(), geom.cpuVertices.begin(), geom.cpuVertices.end());
		allIndices.insert(allIndices.end(), geom.cpuIndices.begin(), geom.cpuIndices.end());
	}
	
	// ?듯빀 踰꾪띁 ?앹꽦 (SHADER_DEVICE_ADDRESS ?ы븿 - BLAS 鍮뚮뱶???꾩슂)
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

	// 硫붿떆 ?⑥쐞濡?BLAS瑜?遺꾨━ ?앹꽦 (?몃뱶 ?몄뒪?댁뒪媛 ?먭린 硫붿떆??BLAS留?李몄“?섎룄濡?
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

		// BLAS ?앹꽦
		Render::OgAccelStructureBuildInfo blasBuildInfo{};
		blasBuildInfo.type = Render::OgAccelStructureType::BOTTOM_LEVEL;
		blasBuildInfo.flags = Render::OgRayTracingBuildFlag::PREFER_FAST_TRACE;
		blasBuildInfo.bottomLevel.geometries = blasGeometries.data();
		blasBuildInfo.bottomLevel.geometryCount = static_cast<uint32_t>(blasGeometries.size());

		// BLAS瑜??앹꽦?섍퀬 鍮뚮뱶
		Render::OgAccelStructureHandle* blas = _renderContext->CreateAccelerationStructure(blasBuildInfo);
		blas->Retain();

		// BLAS 利됱떆 鍮뚮뱶
		_renderContext->BuildAccelerationStructureImmediate(blas, blasBuildInfo);

		// BLAS instance ???(primitiveOffset = ??硫붿떆??泥?geometry ?몃뜳?? TLAS instanceCustomIndex濡??ъ슜)
		BLASInstance instance;
		instance.blas = blas;
		instance.primitiveOffset = firstGeometryIndex;
		instance.primitiveCount = static_cast<uint32_t>(blasGeometries.size());
		instance.vertexOffset = meshVertexOffset;
		instance.vertexCount = meshVertexCount;
		meshBlasIndex[mi] = static_cast<int>(_blasInstances.size());
		_blasInstances.push_back(instance);
	}
	
	// TLAS ?앹꽦???꾪븳 ?몄뒪?댁뒪 ?곗씠??以鍮?
	// Vulkan 援ы쁽?먯꽌??VkAccelerationStructureInstanceKHR ?ъ슜
	// ?꾩옱 ?뚯뒪?몃? ?꾪빐 ?⑥닚??
	
	// Instance 踰꾪띁 ?앹꽦 (?꾩떆)
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

		// ??硫붿떆???ш낵(?좊━) ?ъ쭏???덉쑝硫?洹몃┝???덉씠(cullMask 0x01)?먯꽌 ?쒖쇅
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

		// ?곗씠?붿뿉??geometryInfo 議고쉶 ??踰좎씠???몃뜳?ㅻ줈 ?ъ슜
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
	
	// TLAS ?앹꽦
	Render::OgAccelStructureBuildInfo tlasBuildInfo{};
	tlasBuildInfo.type = Render::OgAccelStructureType::TOP_LEVEL;
	tlasBuildInfo.flags = Render::OgRayTracingBuildFlag::PREFER_FAST_TRACE;
	tlasBuildInfo.topLevel.instanceBuffer = _instanceBuffer;
	tlasBuildInfo.topLevel.instanceCount = static_cast<uint32_t>(tlasInstances.size());
	
	_tlas = _renderContext->CreateAccelerationStructure(tlasBuildInfo);
	_tlas->Retain();

	// TLAS 利됱떆 鍮뚮뱶
	_renderContext->BuildAccelerationStructureImmediate(_tlas, tlasBuildInfo);
	
	// Shader Binding Table ?앹꽦
	createShaderBindingTable();
	
	// 由ъ냼?????앹꽦
	uint32 zeroOffset = 0;
	OgResourceUsage usages[8];
	
	// TLAS - Acceleration Structure
	usages[0].binding.type = OgResourceType::ACCELERATION_STRUCTURE;
	usages[0].binding.stage = static_cast<OgShaderType>(static_cast<uint16>(OgShaderType::RAYGEN) | static_cast<uint16>(OgShaderType::CLOSEST_HIT));
	usages[0].binding.binding = 0;
	usages[0].accelStructure.handle = &_tlas;
	
	// 異쒕젰 ?대?吏
	usages[1].binding.type = OgResourceType::STORAGE_IMAGE;
	usages[1].binding.stage = OgShaderType::RAYGEN;
	usages[1].binding.binding = 1;
	usages[1].texture.handle = &_renderTargetTexture;
	
	// ?좊땲??踰꾪띁
	usages[2].binding.type = OgResourceType::UNIFORM_BUFFER;
	usages[2].binding.stage = static_cast<OgShaderType>(static_cast<uint16>(OgShaderType::RAYGEN) | static_cast<uint16>(OgShaderType::CLOSEST_HIT) | static_cast<uint16>(OgShaderType::MISS));
	usages[2].binding.binding = 2;
	usages[2].buffer.handle = &_rtUniformBuffer;
	usages[2].buffer.offset = &zeroOffset;
	usages[2].buffer.range = &_rtUniformBuffer->size;
	
	// 踰꾪뀓??踰꾪띁
	usages[3].binding.type = OgResourceType::STORAGE_BUFFER;
	usages[3].binding.stage = OgShaderType::CLOSEST_HIT;
	usages[3].binding.binding = 3;
	usages[3].buffer.handle = &_vertexBuffer;
	usages[3].buffer.offset = &zeroOffset;
	usages[3].buffer.range = &_vertexBuffer->size;
	
	// ?몃뜳??踰꾪띁
	usages[4].binding.type = OgResourceType::STORAGE_BUFFER;
	usages[4].binding.stage = OgShaderType::CLOSEST_HIT;
	usages[4].binding.binding = 4;
	usages[4].buffer.handle = &_indexBuffer;
	usages[4].buffer.offset = &zeroOffset;
	usages[4].buffer.range = &_indexBuffer->size;
	
	// Material 踰꾪띁
	usages[5].binding.type = OgResourceType::STORAGE_BUFFER;
	usages[5].binding.stage = OgShaderType::CLOSEST_HIT;
	usages[5].binding.binding = 5;
	usages[5].buffer.handle = &_materialBuffer;
	usages[5].buffer.offset = &zeroOffset;
	usages[5].buffer.range = &_materialBuffer->size;
	
	// GeometryInfo 踰꾪띁
	usages[6].binding.type = OgResourceType::STORAGE_BUFFER;
	usages[6].binding.stage = OgShaderType::CLOSEST_HIT;
	usages[6].binding.binding = 6;
	usages[6].buffer.handle = &_geometryInfoBuffer;
	usages[6].buffer.offset = &zeroOffset;
	usages[6].buffer.range = &_geometryInfoBuffer->size;
	
	// ?띿뒪泥?諛곗뿴
	std::vector<Render::OgTextureHandle*> texHandles;
	for (auto* tex : _textureArray)
	{
		texHandles.push_back(tex);
	}
	// 諛곗뿴??16媛쒓? ?섎룄濡?湲곕낯 ?띿뒪泥섎줈 梨꾩슦湲?
	while (texHandles.size() < 16)
	{
		texHandles.push_back(_defaultWhiteTexture);
	}
	
	// ?띿뒪泥?諛곗뿴 - 16媛?紐⑤몢 諛붿씤??
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
	// SBT handle alignment ?뺣낫 媛?몄삤湲?
	_sbtHandleSizeAligned = _renderContext->GetShaderGroupHandleSizeAligned();

	// ?곗씠??洹몃９ ?ㅼ젙 (4 groups: raygen, miss, shadow miss, hit)
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

	// ?꾩껜 SBT ?앹꽦 (?섎굹??踰꾪띁??紐⑤뱺 ?몃뱾 ?ы븿)
	Render::OgBufferHandle* sbtBuffer = _renderContext->CreateShaderBindingTable(_rtPipeline, groups, 4);

	// ?숈씪??踰꾪띁瑜?offset?쇰줈 援щ텇?섏뿬 ?ъ슜
	_raygenSBT = sbtBuffer;
	_missSBT = sbtBuffer;
	_hitSBT = sbtBuffer;

	sbtBuffer->Retain();
	sbtBuffer->Retain();
	sbtBuffer->Retain();
}

void OgRayTracingSample::updateTLAS()
{
	// TLAS ?낅뜲?댄듃媛 ?꾩슂??寃쎌슦 ?몄텧
	// ?? ?몄뒪?댁뒪 蹂?섏씠 蹂寃쎈릺?덉쓣 ??
}

void OgRayTracingSample::destroyAccelerationStructures()
{
	// SBT 踰꾪띁???댁젣
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
	
	// TLAS ?댁젣
	if (_tlas)
	{
		_tlas->Release();
		_tlas = nullptr;
	}
	
	// BLAS ?댁젣
	for (auto& instance : _blasInstances)
	{
		if (instance.blas)
		{
			instance.blas->Release();
		}
	}
	_blasInstances.clear();
	
	// 踰꾪띁???댁젣
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
	// 湲곗〈 紐⑤뜽 ?곗씠???대━??
	clearModelData();

	// OgGLTFLoader瑜??ъ슜?댁꽌 紐⑤뜽 濡쒕뱶
	if (!_gltfLoader->LoadModel(filePath, _loadedModel))
	{
		LOGE(OG_ID, "Failed to load glTF model: %s", filePath.c_str());
		LOGE(OG_ID, "Error: %s", _gltfLoader->GetLastError().c_str());
		return false;
	}

	// 濡쒕뱶??紐⑤뜽???덉씠?몃젅?댁떛?⑹쑝濡?泥섎━
	for (int i = 0; i < static_cast<int>(_loadedModel.meshes.size()); ++i)
	{
		processMeshForRayTracing(_loadedModel.meshes[i], i);
	}

	// Material ?곗씠??以鍮?
	_materials.clear();
	for (const auto& mat : _loadedModel.materials)
	{
		MaterialData matData{};
		matData.baseColorFactor = mat.baseColorFactor;
		matData.emissiveFactor = glm::vec4(mat.emissiveFactor, mat.emissiveStrength);
		matData.metallicFactor = mat.metallicFactor;
		matData.roughnessFactor = mat.roughnessFactor;
		matData.transmissionFactor = mat.transmissionFactor;
		matData.ior = 1.5; // 湲곕낯 ?좊━ IOR (KHR_materials_ior 湲곕낯媛?
		matData.attenuationColor = glm::vec4(mat.attenuationColor, 1.0f);
		matData.attenuationDistance = mat.attenuationDistance;
		
		// ?띿뒪泥??몃뜳???ㅼ젙
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

	// Material 踰꾪띁 ?앹꽦
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

	// ?몃뱶 泥섎━ (?몄뒪?댁뒪 ?앹꽦)
	_instances.clear();
	for (int rootNode : _loadedModel.rootNodes)
	{
		processNodeForRayTracing(rootNode, glm::mat4(1.0f));
	}

	// ?몄뒪?댁뒪媛 ?섎굹???놁쑝硫?湲곕낯 ?몄뒪?댁뒪 ?앹꽦
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
	// ?띿뒪泥?諛곗뿴 ?대━??(湲곕낯 ?띿뒪泥섎뒗 ?쒖쇅)
	_textureArray.clear();
	
	// 吏?ㅻ찓?몃━ ?뺣낫 ?대━??
	_geometries.clear();
	_materials.clear();
	_instances.clear();
	
	_totalVertexCount = 0;
	_totalIndexCount = 0;
	
	// ?꾨젅??移댁슫??由ъ뀑
	_frameCount = 0;
	
	// OgGLTFLoader瑜??ъ슜?댁꽌 紐⑤뜽 ?곗씠???대━??
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

	// ???몃뱶??硫붿떆媛 ?덉쑝硫??몄뒪?댁뒪 ?앹꽦
	if (node.meshIndex >= 0 && node.meshIndex < static_cast<int>(_loadedModel.meshes.size()))
	{
		InstanceData inst;
		inst.transform = nodeMatrix;
		inst.transformInverse = glm::inverse(nodeMatrix);
		inst.meshIndex = node.meshIndex;
		inst.materialIndex = 0; // 湲곕낯 material
		_instances.push_back(inst);
	}

	// ?먯떇 ?몃뱶??泥섎━
	for (int childIndex : node.children)
	{
		processNodeForRayTracing(childIndex, nodeMatrix);
	}
}

void OgRayTracingSample::convertProjectionForVulkan(glm::mat4& projection)
{
	// GLM??湲곕낯 ?꾨줈?앹뀡? OpenGL???꾪븳 寃껋씠誘濡?Vulkan?⑹쑝濡?蹂??
	// 1. Y異??ㅼ쭛湲?(Vulkan? Y異뺤씠 ?꾨옒濡??ν븿)
	projection[1][1] *= -1.0f;
}

OG_NAMESPACE_SAMPLE_END