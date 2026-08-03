#pragma once
#ifndef _OG_RAYTRACINGSAMPLE_H__
#define _OG_RAYTRACINGSAMPLE_H__

#include "OgSampleBase.h"
#include <memory>
#include <unordered_map>
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "sample/public/core/util/OgFlyCamera.h"
#include "sample/public/core/util/OgGLTFLoader.h"

// Input key defines (임시)
#ifndef OG_KEY_F
#define OG_KEY_F 70
#endif
#ifndef OG_KEY_D
#define OG_KEY_D 68
#endif
#ifndef OG_PRESS
#define OG_PRESS 1
#endif

OG_NAMESPACE_SAMPLE_BEGIN

/**
 * @brief Vulkan Ray Tracing을 사용한 렌더링 샘플
 *
 * VK_KHR_ray_tracing_pipeline 확장을 사용하여 실시간 레이트레이싱을 구현합니다.
 * Path tracing 기반으로 physically based rendering을 수행합니다.
 */
class OG_API OgRayTracingSample : public OgSampleBase
{
public:
    // 라이트 데이터 구조체
    struct Light
    {
        glm::vec3 position;      // 12 bytes - 라이트 위치 (Point) 또는 방향 (Directional)
        float type;              // 4 bytes  - 0: Directional, 1: Point
        glm::vec3 color;         // 12 bytes
        float intensity;         // 4 bytes
    };

    // 레이트레이싱 유니폼 데이터
    struct RTUniformData
    {
        glm::mat4 viewInverse;          // 64 bytes
        glm::mat4 projInverse;          // 64 bytes
        glm::vec4 cameraPos;            // 16 bytes
        glm::vec4 lightPos;             // 16 bytes
        glm::vec4 lightColor;           // 16 bytes
        glm::vec4 globalAmbient;        // 16 bytes
        uint32_t frameCount;            // 4 bytes
        uint32_t maxBounces;            // 4 bytes
        uint32_t samplesPerPixel;       // 4 bytes
        float padding;                  // 4 bytes
    };

    // Material 데이터 (GPU 전달용)
    struct MaterialData
    {
        glm::vec4 baseColorFactor;      // 16 bytes
        glm::vec4 emissiveFactor;       // 16 bytes
        float metallicFactor;           // 4 bytes
        float roughnessFactor;          // 4 bytes
        float transmissionFactor;       // 4 bytes
        float ior;                      // 4 bytes
        glm::vec4 attenuationColor;     // 16 bytes
        float attenuationDistance;      // 4 bytes
        int32_t baseColorTextureIndex;  // 4 bytes
        int32_t normalTextureIndex;     // 4 bytes
        int32_t metallicRoughnessTextureIndex; // 4 bytes
    };

    // Instance 데이터
    struct InstanceData
    {
        glm::mat4 transform;            // 64 bytes
        glm::mat4 transformInverse;     // 64 bytes
        uint32_t meshIndex;             // 4 bytes
        uint32_t materialIndex;         // 4 bytes
        uint32_t padding[2];            // 8 bytes
    };

    OgRayTracingSample(Render::OgRenderContext* renderContext);
    ~OgRayTracingSample() override;

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

    // 레이트레이싱 설정
    void SetMaxBounces(uint32_t bounces) { _rtUniformData.maxBounces = bounces; _frameCount = 0; }
    void SetSamplesPerPixel(uint32_t samples) { _rtUniformData.samplesPerPixel = samples; _frameCount = 0; }
    uint32_t GetMaxBounces() const { return _rtUniformData.maxBounces; }
    uint32_t GetSamplesPerPixel() const { return _rtUniformData.samplesPerPixel; }
    
    // 카메라 인터페이스
    bool IsUsingFlyCamera() const { return _useFlyCamera; }
    void SetUsingFlyCamera(bool use) { _useFlyCamera = use; _frameCount = 0; }

private:
    // OgGLTFLoader의 타입들을 재사용
    using Vertex = OgGLTFLoader::Vertex;
    using Primitive = OgGLTFLoader::Primitive;
    using Mesh = OgGLTFLoader::Mesh;
    using Node = OgGLTFLoader::Node;
    using Material = OgGLTFLoader::Material;

    // 레이트레이싱 구조체
    struct BLASInstance
    {
        Render::OgAccelStructureHandle* blas = nullptr;
        uint32_t primitiveOffset = 0;
        uint32_t primitiveCount = 0;
        uint32_t vertexOffset = 0;
        uint32_t vertexCount = 0;
    };

    struct GeometryInfo
    {
        Render::OgBufferHandle* vertexBuffer = nullptr;
        Render::OgBufferHandle* indexBuffer = nullptr;
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
        uint32_t materialIndex = 0;
        int meshIndex = 0;
        // CPU side copies for acceleration structure building
        std::vector<Vertex> cpuVertices;
        std::vector<uint32_t> cpuIndices;
    };
    
    // GPU로 전달할 지오메트리 정보 (셰이더용)
    // GLSL의 GeometryInfo 구조체와 일치해야 함
    struct GPUGeometryInfo
    {
        uint32_t vertexOffset;
        uint32_t indexOffset;
        uint32_t materialIndex;
        uint32_t padding;
    };

private:
    // 리소스 관리
    void createResources(uint16 width, uint16 height);
    void destroyResources();
    void createShaders();
    void createRayTracingPipeline();
    void createRenderTarget(uint16 width, uint16 height);
    void destroyRenderTarget();
    void createUniformBuffer();
    void updateUniformBuffer();

    // 레이트레이싱 리소스
    void createAccelerationStructures();
    void createShaderBindingTable();
    void updateTLAS();
    void destroyAccelerationStructures();

    // glTF 로딩
    bool loadGLTFModel(const std::string& filePath);
    void clearModelData();
    void processMeshForRayTracing(const Mesh& mesh, int meshIndex);
    void processNodeForRayTracing(int nodeIndex, const glm::mat4& parentMatrix);

    // Vulkan용 프로젝션 행렬 변환
    void convertProjectionForVulkan(glm::mat4& projection);

private:
    // 레이트레이싱 파이프라인 리소스
    Render::OgShaderHandle* _raygenShader = nullptr;
    Render::OgShaderHandle* _missShader{ nullptr };
    Render::OgShaderHandle* _shadowMissShader{ nullptr };
    Render::OgShaderHandle* _closestHitShader = nullptr;
    Render::OgProgramHandle* _rtProgram = nullptr;
    Render::OgResourceLayoutHandle* _rtResourceLayout = nullptr;
    Render::OgPipelineHandle* _rtPipeline = nullptr;
    Render::OgResourceSetHandle* _rtResourceSet = nullptr;

    // Shader Binding Table
    Render::OgBufferHandle* _raygenSBT = nullptr;
    Render::OgBufferHandle* _missSBT = nullptr;
    Render::OgBufferHandle* _hitSBT = nullptr;

    // Acceleration Structures
    std::vector<BLASInstance> _blasInstances;
    Render::OgAccelStructureHandle* _tlas = nullptr;
    Render::OgBufferHandle* _instanceBuffer = nullptr;
    Render::OgBufferHandle* _scratchBuffer = nullptr;

    // 지오메트리 버퍼
    Render::OgBufferHandle* _vertexBuffer = nullptr;
    Render::OgBufferHandle* _indexBuffer = nullptr;
    Render::OgBufferHandle* _materialBuffer = nullptr;
    Render::OgBufferHandle* _geometryInfoBuffer = nullptr;
    
    // 유니폼 버퍼
    Render::OgBufferHandle* _rtUniformBuffer = nullptr;
    RTUniformData _rtUniformData;

    // 렌더 타겟 리소스
    Render::OgTextureHandle* _renderTargetTexture = nullptr;
    Render::OgTextureHandle* _depthTexture = nullptr;
    Render::OgFrameBufferHandle* _renderTargetFrameBuffer = nullptr;
    Render::OgRenderPassHandle* _renderTargetRenderPass = nullptr;
    uint16 _renderTargetWidth = 0;
    uint16 _renderTargetHeight = 0;

    // 텍스처 배열
    std::vector<Render::OgTextureHandle*> _textureArray;
    Render::OgTextureHandle* _defaultWhiteTexture = nullptr;
    Render::OgTextureHandle* _defaultNormalTexture = nullptr;

    // glTF 로더
    std::unique_ptr<OgGLTFLoader> _gltfLoader;
    OgGLTFLoader::LoadedModel _loadedModel;

    // 지오메트리 정보
    std::vector<GeometryInfo> _geometries;
    std::vector<MaterialData> _materials;
    std::vector<InstanceData> _instances;
    
    // 인덱스/버텍스 오프셋
    uint32_t _totalVertexCount = 0;
    uint32_t _totalIndexCount = 0;

    // 카메라
    std::unique_ptr<OgFlyCamera> _camera;
    bool _useFlyCamera = true;
    glm::mat4 _previousViewMatrix;
    
    // SBT alignment 정보
    uint32_t _sbtHandleSizeAligned = 0;

    // 프레임 카운터 (프로그레시브 렌더링용)
    uint32_t _frameCount = 0;

    // 디버그 설정
    bool _enableDebugVisualization = false;
};

OG_NAMESPACE_SAMPLE_END

#endif // _OG_RAYTRACINGSAMPLE_H__