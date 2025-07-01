#include "OgRayTracingSample.h"
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
    _rtUniformData.maxBounces = 3;
    _rtUniformData.samplesPerPixel = 1;
    _rtUniformData.globalAmbient = glm::vec4(0.03f, 0.03f, 0.03f, 1.0f);
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

    // 레이트레이싱 디스패치
    Render::OgShaderBindingTable sbt{};
    sbt.raygenSBT = _raygenSBT;
    sbt.raygenOffset = 0;
    sbt.raygenStride = 0;
    
    sbt.missSBT = _missSBT;
    sbt.missOffset = 0;
    sbt.missStride = 0;
    sbt.missSize = 1;
    
    sbt.hitSBT = _hitSBT;
    sbt.hitOffset = 0;
    sbt.hitStride = 0;
    sbt.hitSize = 1;
    
    sbt.callableSBT = nullptr;
    sbt.callableOffset = 0;
    sbt.callableStride = 0;
    sbt.callableSize = 0;
    
    encoder->TraceRays(sbt, _renderTargetWidth, _renderTargetHeight, 1);

    // 렌더 타겟에 결과 표시 (후처리용 렌더 패스)
    encoder->BeginRenderPass(_renderTargetRenderPass, _renderTargetFrameBuffer, area, 1, &colorClear, 0, nullptr, &depthStencilClear);
    encoder->EndRenderPass();

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
    bindings[0].stage = OgShaderType::RAYGEN | OgShaderType::CLOSEST_HIT;
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
    bindings[2].stage = OgShaderType::RAYGEN | OgShaderType::CLOSEST_HIT | OgShaderType::MISS;
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
    // Ray Generation 셰이더
    const char* raygenGLSL = R"(
        #version 460
        #extension GL_EXT_ray_tracing : require

        layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
        layout(binding = 1, rgba8) uniform image2D image;
        
        layout(binding = 2) uniform UniformBufferObject {
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

        layout(location = 0) rayPayloadEXT vec3 hitValue;

        // 简单的伪随机数生成器
        uint rngState;
        
        uint pcg_hash(uint input)
        {
            uint state = input * 747796405u + 2891336453u;
            uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
            return (word >> 22u) ^ word;
        }
        
        float randomFloat()
        {
            rngState = pcg_hash(rngState);
            return float(rngState) / 4294967295.0;
        }
        
        vec2 randomInUnitDisk()
        {
            vec2 p;
            do {
                p = 2.0 * vec2(randomFloat(), randomFloat()) - 1.0;
            } while (dot(p, p) >= 1.0);
            return p;
        }

        void main() 
        {
            // 初始化随机数种子
            rngState = ubo.frameCount + gl_LaunchIDEXT.x * 1973 + gl_LaunchIDEXT.y * 9277;
            
            const vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
            const vec2 inUV = pixelCenter/vec2(gl_LaunchSizeEXT.xy);
            vec2 d = inUV * 2.0 - 1.0;

            vec4 origin = ubo.viewInverse * vec4(0,0,0,1);
            vec4 target = ubo.projInverse * vec4(d.x, d.y, 1, 1);
            vec4 direction = ubo.viewInverse * vec4(normalize(target.xyz), 0);

            vec3 finalColor = vec3(0.0);
            
            // 多采样抗锯齿
            for (uint s = 0; s < ubo.samplesPerPixel; s++)
            {
                // 添加随机偏移实现抗锯齿
                vec2 offset = (vec2(randomFloat(), randomFloat()) - 0.5) / vec2(gl_LaunchSizeEXT.xy);
                vec2 nd = d + offset * 2.0;
                
                vec4 ntarget = ubo.projInverse * vec4(nd.x, nd.y, 1, 1);
                vec4 ndirection = ubo.viewInverse * vec4(normalize(ntarget.xyz), 0);
                
                traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin.xyz, 0.001, ndirection.xyz, 10000.0, 0);
                finalColor += hitValue;
            }
            
            finalColor /= float(ubo.samplesPerPixel);
            
            // 累积之前的帧（实现渐进式渲染）
            if (ubo.frameCount > 0)
            {
                vec3 previousColor = imageLoad(image, ivec2(gl_LaunchIDEXT.xy)).rgb;
                float weight = 1.0 / float(ubo.frameCount + 1);
                finalColor = mix(previousColor, finalColor, weight);
            }
            
            imageStore(image, ivec2(gl_LaunchIDEXT.xy), vec4(finalColor, 1.0));
        }
    )";

    // Miss 셰이더
    const char* missGLSL = R"(
        #version 460
        #extension GL_EXT_ray_tracing : require

        layout(location = 0) rayPayloadInEXT vec3 hitValue;
        
        layout(binding = 2) uniform UniformBufferObject {
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
            // 简单的天空盒渐变
            vec3 direction = normalize(gl_WorldRayDirectionEXT);
            float t = 0.5 * (direction.y + 1.0);
            vec3 skyColor = mix(vec3(0.5, 0.7, 1.0), vec3(0.1, 0.2, 0.4), t);
            hitValue = skyColor * 0.5;
        }
    )";

    // Closest Hit 셰이더
    const char* closestHitGLSL = R"(
        #version 460
        #extension GL_EXT_ray_tracing : require
        #extension GL_EXT_nonuniform_qualifier : enable

        layout(location = 0) rayPayloadInEXT vec3 hitValue;
        layout(location = 1) rayPayloadEXT vec3 shadowHitValue;
        
        hitAttributeEXT vec2 attribs;

        layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
        
        layout(binding = 2) uniform UniformBufferObject {
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
            vec3 position;
            vec3 normal;
            vec2 texCoord;
            vec4 tangent;
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

        layout(binding = 3) readonly buffer VertexBuffer { Vertex vertices[]; } vertexBuffer;
        layout(binding = 4) readonly buffer IndexBuffer { uint indices[]; } indexBuffer;
        layout(binding = 5) readonly buffer MaterialBuffer { Material materials[]; } materialBuffer;
        layout(binding = 6) readonly buffer GeometryInfoBuffer { GeometryInfo geometryInfos[]; } geometryInfoBuffer;
        layout(binding = 7) uniform sampler2D textures[16];

        // 获取重心坐标
        vec3 getBarycentric()
        {
            vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
            return barycentrics;
        }

        // 获取顶点数据
        Vertex getVertex(uint index)
        {
            GeometryInfo geomInfo = geometryInfoBuffer.geometryInfos[gl_GeometryIndexEXT];
            return vertexBuffer.vertices[geomInfo.vertexOffset + index];
        }

        // PBR 光照计算
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

        void main()
        {
            // 获取几何信息
            GeometryInfo geomInfo = geometryInfoBuffer.geometryInfos[gl_GeometryIndexEXT];
            
            // 获取三角形顶点索引
            uint i0 = indexBuffer.indices[geomInfo.indexOffset + gl_PrimitiveID * 3 + 0];
            uint i1 = indexBuffer.indices[geomInfo.indexOffset + gl_PrimitiveID * 3 + 1];
            uint i2 = indexBuffer.indices[geomInfo.indexOffset + gl_PrimitiveID * 3 + 2];
            
            // 获取顶点
            Vertex v0 = getVertex(i0);
            Vertex v1 = getVertex(i1);
            Vertex v2 = getVertex(i2);
            
            // 重心坐标插值
            vec3 bary = getBarycentric();
            vec3 position = v0.position * bary.x + v1.position * bary.y + v2.position * bary.z;
            vec3 normal = normalize(v0.normal * bary.x + v1.normal * bary.y + v2.normal * bary.z);
            vec2 texCoord = v0.texCoord * bary.x + v1.texCoord * bary.y + v2.texCoord * bary.z;
            
            // 世界坐标系转换
            position = gl_ObjectToWorldEXT * vec4(position, 1.0);
            normal = normalize((transpose(gl_WorldToObjectEXT) * vec4(normal, 0.0)).xyz);
            
            // 获取材质
            Material material = materialBuffer.materials[geomInfo.materialIndex];
            
            // 基础颜色
            vec4 baseColor = material.baseColorFactor;
            if (material.baseColorTextureIndex >= 0)
            {
                baseColor *= texture(textures[material.baseColorTextureIndex], texCoord);
            }
            
            // PBR 参数
            float metallic = material.metallicFactor;
            float roughness = material.roughnessFactor;
            vec3 emissive = material.emissiveFactor.rgb;
            
            // 光照计算
            vec3 V = -normalize(gl_WorldRayDirectionEXT);
            vec3 L = normalize(ubo.lightPos.xyz);
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
            vec3 Lo = (kD * baseColor.rgb / PI + specular) * ubo.lightColor.rgb * NdotL;
            
            // 环境光
            vec3 ambient = ubo.globalAmbient.rgb * baseColor.rgb;
            
            // 阴影检测
            float tmin = 0.001;
            float tmax = length(ubo.lightPos.xyz - position);
            vec3 shadowRayOrigin = position + normal * 0.001;
            vec3 shadowRayDirection = normalize(ubo.lightPos.xyz - position);
            
            shadowHitValue = vec3(1.0);
            traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT,
                        0xff, 1, 0, 1, shadowRayOrigin, tmin, shadowRayDirection, tmax, 1);
            
            // 最终颜色
            vec3 color = ambient + Lo * shadowHitValue + emissive;
            
            // 简单的色调映射
            color = color / (color + vec3(1.0));
            color = pow(color, vec3(1.0/2.2));
            
            hitValue = color;
        }
    )";

    // 编译셰이더
    std::vector<uint32_t> raygenSPIRV;
    std::vector<uint32_t> missSPIRV;
    std::vector<uint32_t> closestHitSPIRV;

    if (!OgShaderCompiler::CompileGLSLtoSPIRV(raygenGLSL, OgShaderType::RAYGEN, raygenSPIRV))
    {
        LOGE(OG_ID, "Failed to compile ray generation shader");
        return;
    }

    if (!OgShaderCompiler::CompileGLSLtoSPIRV(missGLSL, OgShaderType::MISS, missSPIRV))
    {
        LOGE(OG_ID, "Failed to compile miss shader");
        return;
    }

    if (!OgShaderCompiler::CompileGLSLtoSPIRV(closestHitGLSL, OgShaderType::CLOSEST_HIT, closestHitSPIRV))
    {
        LOGE(OG_ID, "Failed to compile closest hit shader");
        return;
    }

    // 创建셰이더
    _raygenShader = _renderContext->CreateShader(
        OgShaderType::RAYGEN,
        reinterpret_cast<const char*>(raygenSPIRV.data()),
        raygenSPIRV.size() * sizeof(uint32_t),
        "main"
    );
    _raygenShader->name = "RayGenShader";
    _raygenShader->Retain();

    _missShader = _renderContext->CreateShader(
        OgShaderType::MISS,
        reinterpret_cast<const char*>(missSPIRV.data()),
        missSPIRV.size() * sizeof(uint32_t),
        "main"
    );
    _missShader->name = "MissShader";
    _missShader->Retain();

    _closestHitShader = _renderContext->CreateShader(
        OgShaderType::CLOSEST_HIT,
        reinterpret_cast<const char*>(closestHitSPIRV.data()),
        closestHitSPIRV.size() * sizeof(uint32_t),
        "main"
    );
    _closestHitShader->name = "ClosestHitShader";
    _closestHitShader->Retain();

    // 레이트레이싱 프로그램 생성
    OgShaderHandle* handles[]{ _raygenShader, _missShader, _closestHitShader };
    _rtProgram = _renderContext->CreateProgram(handles, 3);
    _rtProgram->name = "RayTracingProgram";
    _rtProgram->Retain();
}

void OgRayTracingSample::createRayTracingPipeline()
{
    // 레이트레이싱 파이프라인 설정
    Render::OgRayTracingPipelineDescriptor rtPipeDesc{};
    rtPipeDesc.name = "RayTracingPipeline";
    rtPipeDesc.resourceLayout = _rtResourceLayout;
    rtPipeDesc.maxRecursionDepth = 2; // Primary ray + shadow ray
    
    // 셰이더 스테이지 설정
    Render::OgRayTracingShaderGroup groups[3];
    
    // Ray generation group
    groups[0].type = Render::OgRayTracingShaderGroup::GENERAL;
    groups[0].generalShader = 0; // raygen shader index
    groups[0].closestHitShader = ~0u;
    groups[0].anyHitShader = ~0u;
    groups[0].intersectionShader = ~0u;
    
    // Miss group
    groups[1].type = Render::OgRayTracingShaderGroup::GENERAL;
    groups[1].generalShader = 1; // miss shader index
    groups[1].closestHitShader = ~0u;
    groups[1].anyHitShader = ~0u;
    groups[1].intersectionShader = ~0u;
    
    // Hit group
    groups[2].type = Render::OgRayTracingShaderGroup::TRIANGLES_HIT_GROUP;
    groups[2].generalShader = ~0u;
    groups[2].closestHitShader = 2; // closest hit shader index
    groups[2].anyHitShader = ~0u;
    groups[2].intersectionShader = ~0u;
    
    rtPipeDesc.shaderGroups = groups;
    rtPipeDesc.shaderGroupCount = 3;
    
    // 셰이더 설정
    Render::OgShaderHandle* shaders[3] = { _raygenShader, _missShader, _closestHitShader };
    rtPipeDesc.shaders = shaders;
    rtPipeDesc.shaderCount = 3;
    
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
    texInfo.format = OgPixelFormat::R8G8B8A8_UNORM;
    texInfo.extent.width = width;
    texInfo.extent.height = height;
    texInfo.usage = OgTextureUsage::COLOR_ATTACHMENT | OgTextureUsage::SAMPLED | OgTextureUsage::STORAGE;
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
    std::vector<GeometryInfo> allGeometryInfos;
    
    allVertices.reserve(_totalVertexCount);
    allIndices.reserve(_totalIndexCount);
    
    // BLAS 생성을 위한 geometry 정보 수집
    std::vector<Render::OgAccelStructureGeometry> blasGeometries;
    
    for (const auto& geom : _geometries)
    {
        GeometryInfo info;
        info.vertexOffset = static_cast<uint32_t>(allVertices.size());
        info.indexOffset = static_cast<uint32_t>(allIndices.size());
        info.materialIndex = geom.materialIndex;
        info.padding = 0;
        
        allGeometryInfos.push_back(info);
        
        // 버텍스 복사
        std::vector<Vertex> vertices(geom.vertexCount);
        void* vertexData = _renderContext->MapBuffer(geom.vertexBuffer, sizeof(Vertex) * geom.vertexCount);
        if (vertexData)
        {
            memcpy(vertices.data(), vertexData, sizeof(Vertex) * geom.vertexCount);
            _renderContext->UnmapBuffer(geom.vertexBuffer);
            allVertices.insert(allVertices.end(), vertices.begin(), vertices.end());
        }
        
        // 인덱스 복사
        if (geom.indexCount > 0)
        {
            std::vector<uint32_t> indices(geom.indexCount);
            void* indexData = _renderContext->MapBuffer(geom.indexBuffer, sizeof(uint32_t) * geom.indexCount);
            if (indexData)
            {
                // uint16에서 uint32로 변환 가능성 고려
                if (geom.indexBuffer->size == sizeof(uint16_t) * geom.indexCount)
                {
                    uint16_t* indices16 = static_cast<uint16_t*>(indexData);
                    for (uint32_t i = 0; i < geom.indexCount; ++i)
                    {
                        indices[i] = indices16[i];
                    }
                }
                else
                {
                    memcpy(indices.data(), indexData, sizeof(uint32_t) * geom.indexCount);
                }
                _renderContext->UnmapBuffer(geom.indexBuffer);
                allIndices.insert(allIndices.end(), indices.begin(), indices.end());
            }
        }
        
        // BLAS geometry 정보 설정
        Render::OgAccelStructureGeometry asGeom{};
        asGeom.vertexBuffer = geom.vertexBuffer;
        asGeom.vertexStride = sizeof(Vertex);
        asGeom.vertexCount = geom.vertexCount;
        asGeom.indexBuffer = geom.indexBuffer;
        asGeom.indexType = (geom.indexBuffer->size == sizeof(uint16_t) * geom.indexCount) 
            ? OgIndexType::UINT16 : OgIndexType::UINT32;
        asGeom.indexCount = geom.indexCount;
        asGeom.transformOffset = 0;
        
        blasGeometries.push_back(asGeom);
    }
    
    // 통합 버퍼 생성
    _vertexBuffer = _renderContext->CreateBuffer(
        allVertices.data(),
        sizeof(Vertex) * allVertices.size(),
        Render::OgBufferUsage::VERTEX | Render::OgBufferUsage::STORAGE | Render::OgBufferUsage::ACCELERATION_STRUCTURE_BUILD_INPUT,
        OgMemoryOption::DEVICE_LOCAL
    );
    _vertexBuffer->Retain();
    
    _indexBuffer = _renderContext->CreateBuffer(
        allIndices.data(),
        sizeof(uint32_t) * allIndices.size(),
        Render::OgBufferUsage::INDEX | Render::OgBufferUsage::STORAGE | Render::OgBufferUsage::ACCELERATION_STRUCTURE_BUILD_INPUT,
        OgMemoryOption::DEVICE_LOCAL
    );
    _indexBuffer->Retain();
    
    _geometryInfoBuffer = _renderContext->CreateBuffer(
        allGeometryInfos.data(),
        sizeof(GeometryInfo) * allGeometryInfos.size(),
        Render::OgBufferUsage::STORAGE,
        OgMemoryOption::DEVICE_LOCAL
    );
    _geometryInfoBuffer->Retain();
    
    // BLAS 생성
    Render::OgAccelStructureBuildInfo blasBuildInfo{};
    blasBuildInfo.type = Render::OgAccelStructureType::BOTTOM_LEVEL;
    blasBuildInfo.flags = Render::OgRayTracingBuildFlag::PREFER_FAST_TRACE;
    blasBuildInfo.bottomLevel.geometries = blasGeometries.data();
    blasBuildInfo.bottomLevel.geometryCount = static_cast<uint32_t>(blasGeometries.size());
    
    // BLAS를 생성하고 빌드
    Render::OgAccelStructureHandle* blas = _renderContext->CreateAccelerationStructure(blasBuildInfo);
    blas->Retain();
    
    // BLAS instance 저장
    BLASInstance instance;
    instance.blas = blas;
    instance.primitiveOffset = 0;
    instance.primitiveCount = static_cast<uint32_t>(_geometries.size());
    instance.vertexOffset = 0;
    instance.vertexCount = _totalVertexCount;
    _blasInstances.push_back(instance);
    
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
        
        tlasInst.instanceCustomIndex = 0;
        tlasInst.mask = 0xFF;
        tlasInst.instanceShaderBindingTableRecordOffset = 0;
        tlasInst.flags = 0; // TRIANGLE_FACING_CULL_DISABLE
        tlasInst.accelerationStructureReference = blas->deviceAddress;
        
        tlasInstances.push_back(tlasInst);
    }
    
    _instanceBuffer = _renderContext->CreateBuffer(
        tlasInstances.data(),
        sizeof(InstanceData) * tlasInstances.size(),
        static_cast<OgBufferUsage>(
            static_cast<uint16>(OgBufferUsage::VERTEX) | 
            static_cast<uint16>(OgBufferUsage::STORAGE)
        ),
        OgMemoryOption::DEVICE_LOCAL
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
    
    // Shader Binding Table 생성
    createShaderBindingTable();
    
    // 리소스 셋 생성
    uint32 zeroOffset = 0;
    OgResourceUsage usages[8];
    
    // TLAS
    usages[0].binding.type = OgResourceType::ACCELERATION_STRUCTURE;
    usages[0].binding.stage = OgShaderType::RAYGEN | OgShaderType::CLOSEST_HIT;
    usages[0].binding.binding = 0;
    usages[0].accelerationStructure.handle = &_tlas;
    
    // 출력 이미지
    usages[1].binding.type = OgResourceType::STORAGE_IMAGE;
    usages[1].binding.stage = OgShaderType::RAYGEN;
    usages[1].binding.binding = 1;
    usages[1].texture.handle = &_renderTargetTexture;
    
    // 유니폼 버퍼
    usages[2].binding.type = OgResourceType::UNIFORM_BUFFER;
    usages[2].binding.stage = OgShaderType::RAYGEN | OgShaderType::CLOSEST_HIT | OgShaderType::MISS;
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
    
    usages[7].binding.type = OgResourceType::COMBINED_IMAGE_SAMPLER;
    usages[7].binding.stage = OgShaderType::CLOSEST_HIT;
    usages[7].binding.binding = 7;
    usages[7].textureArray.handles = texHandles.data();
    usages[7].textureArray.count = 16;
    
    _rtResourceSet = _renderContext->CreateResourceSet(_rtResourceLayout, usages, 8);
    _rtResourceSet->name = "RayTracingResourceSet";
    _rtResourceSet->Retain();
}

void OgRayTracingSample::createShaderBindingTable()
{
    // OgRenderContext::CreateShaderBindingTable을 사용
    Render::OgRayTracingShaderGroup groups[3];
    
    // 이전에 설정한 것과 동일하게 설정
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
    
    groups[2].type = Render::OgRayTracingShaderGroup::TRIANGLES_HIT_GROUP;
    groups[2].generalShader = ~0u;
    groups[2].closestHitShader = 2;
    groups[2].anyHitShader = ~0u;
    groups[2].intersectionShader = ~0u;
    
    // 전체 SBT 생성
    Render::OgBufferHandle* sbtBuffer = _renderContext->CreateShaderBindingTable(_rtPipeline, groups, 3);
    
    // SBT를 각 그룹별로 나눠서 사용 (임시)
    // 실제로는 하나의 버퍼에서 offset을 사용해야 함
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
        matData.ior = mat.ior;
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
            OgMemoryOption::DEVICE_LOCAL
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
            
            _geometries.push_back(geom);
            
            _totalVertexCount += geom.vertexCount;
            _totalIndexCount += geom.indexCount;
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