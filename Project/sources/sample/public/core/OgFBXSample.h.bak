#pragma once
#ifndef _OG_FBX_SAMPLE_H__
#define _OG_FBX_SAMPLE_H__

#include "OgSampleBase.h"
#include <memory>
#include <vector>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "sample/public/core/util/OgFlyCamera.h"

OG_NAMESPACE_SAMPLE_BEGIN

/**
 * @brief FBX 모델을 로드하고 렌더링하는 샘플
 *
 * 현재는 간단한 큐브 메시를 하드코딩하여 보여주지만,
 * 추후 FBX SDK나 Assimp 라이브러리를 통합하여 실제 FBX 파일을 로드할 수 있습니다.
 */
class OG_API OgFBXSample : public OgSampleBase
{
public:
	OgFBXSample(Render::OgRenderContext* renderContext);
	~OgFBXSample() override;

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
	// 리소스 관리
	void createResources(uint16 width, uint16 height);
	void destroyResources();
	void createShaders();
	void createPipeline();
	void createRenderTarget(uint16 width, uint16 height);
	void destroyRenderTarget();
	void createMesh();
	void createUniformBuffer();
	void updateUniformBuffer();

	// FBX 로딩 (추후 구현)
	bool loadFBXModel(const char* filePath);
	
	// Vulkan용 프로젝션 행렬 변환
	void convertProjectionForVulkan(glm::mat4& projection);

private:
	// 메시 데이터
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texCoord;
		glm::vec3 color;
	};

	// 유니폼 버퍼 데이터
	struct UniformData
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 projection;
	};

	// 렌더링 리소스
	Render::OgBufferHandle* _vertexBuffer = nullptr;
	Render::OgBufferHandle* _indexBuffer = nullptr;
	Render::OgBufferHandle* _uniformBuffer = nullptr;
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

	uint16 _renderTargetWidth = 0;
	uint16 _renderTargetHeight = 0;

	// 메시 데이터
	std::vector<Vertex> _vertices;
	std::vector<uint16> _indices;
	uint32 _indexCount = 0;

	// 변환 행렬
	float _rotation = 0.0f;
	UniformData _uniformData;

	// 카메라
	std::unique_ptr<OgFlyCamera> _camera;
	bool _useFlyCamera = true; // 플라이 카메라 사용 여부
};

OG_NAMESPACE_SAMPLE_END

#endif // _OG_FBX_SAMPLE_H__