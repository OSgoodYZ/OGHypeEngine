#pragma once
#ifndef _OG_RENDERER_VULKAN_H_
#define _OG_RENDERER_VULKAN_H_

#if defined(__ANDROID__)
#if (__ANDROID_API__  < 23)
#include "render/private/android/vulkan_wrapper.h"
#else
#include <vulkan/vulkan.h>
#endif
#else
#include <vulkan/vulkan.h>
#endif

#include <queue>
#include <unordered_map>

#include "system/OgSystemContext.h"
#include "render/OgRenderContext.h"
#include "render/private/vulkan/OgSwapChainVulkan.h"
#include "render/private/vulkan/OgRenderVulkanHandles.h"


OG_NAMESPACE_RENDER_BEGIN


class OgRenderContextVulkan : public OgRenderContext
{
public:
	OgRenderContextVulkan(System::OgSystemContext* context);

	~OgRenderContextVulkan() override;

	void Load(void) override;

	void Init(void) override;

	OgSwapChain* CreateSwapchain(System::OgNativeWindow* nativeWindow, const OgSwapChainInfo& swapchainInfo) override;

	void DestroySwapchain(OgSwapChain* swapchain) override;

	//OgSwapChain GetSwapchain(OgNativeWindow* nativeWindow) override;

	OgFrameBufferHandle* GetSwapChainFrameBuffer(OgSwapChain* swapchain, uint32 index) override;

	uint32 AcquireNextImageIndex(OgSwapChain* swapchain) override;

	uint32 GetCurrentImageIndex(OgSwapChain* swapchain) override;

	OgBufferHandle* CreateBuffer(void* data, size_t size, OgBufferUsage usage, OgMemoryOption option = OgMemoryOption::PRIVATE_GPU) override;
	void DestroyBuffer(OgBufferHandle* buffer) override;

	OgShaderHandle* CreateShader(OgShaderType flag, const char* text, uint32 codeSize, const char* funcName = nullptr);
	void DestroyShader(OgShaderHandle* shader) override;

	OgProgramHandle* CreateProgram(OgShaderHandle** shaders, uint32 shaderCount) override;
	void DestroyProgram(OgProgramHandle* handle) override;

	OgTextureHandle* CreateTexture(void* image, OgPixelFormat format, uint32 width, uint32 height, OgSamplerHandle* sampler = nullptr, bool generateMipmaps = false) override;
	OgTextureHandle* CreateTexture(void** image, OgPixelFormat format, uint32 width, uint32 height, uint32 layerCount, OgSamplerHandle* sampler = nullptr, bool generateMipmaps = false) override;
	OgTextureHandle* CreateTexture(void** image, const OgTextureInfo& info, OgSamplerHandle* sampler = nullptr) override;
	void DestroyTexture(OgTextureHandle* texture) override;
	void UpdateTexture(OgTextureHandle* texture, OgSamplerHandle* sampler, size_t offset, void** data, bool useBarrier);

	OgSamplerHandle* CreateSampler(const OgSamplerInfo& info) override;
	void DestroySampler(OgSamplerHandle* sampler) override;

	OgFrameBufferHandle* CreateFrameBuffer(OgFrameBufferInfo& info) override;
	void DestroyFrameBuffer(OgFrameBufferHandle* framebuffer) override;

	OgRenderPassHandle* CreateRenderPass(OgRenderPassInfo& info) override;
	void DestroyRenderPass(OgRenderPassHandle* renderPass) override;

	OgPipelineHandle* CreatePipeline(OgPipelineDescriptor& descriptor) override;
	void DestroyPipeline(OgPipelineHandle* pipeline) override;

	OgResourceLayoutHandle* CreateResourceLayout(OgResourceBinding* bindings, uint32 count) override;
	void DestroyResourceLayout(OgResourceLayoutHandle* layout) override;

	OgResourceSetHandle* CreateResourceSet(OgResourceLayoutHandle* resourceLayout, OgResourceUsage* usages, uint32 usageCount) override;
	void DestroyResourceSet(OgResourceSetHandle* resourceSet) override;

	OgCommandEncoderHandle* CreateCommandEncoder() override;
	void DestroyCommandEncoder(OgCommandEncoderHandle* encoder) override;

	void* MapBuffer(OgBufferHandle* buffer, size_t size, size_t offset = 0) override;

	bool UnmapBuffer(OgBufferHandle* buffer) override;

	void UpdateBuffer(OgBufferHandle* buffer, size_t offset, void* data, size_t size, bool useBarrier) override;

	void BlitFramebuffer(uint srcX0, uint srcY0, uint srcX1, uint srcY1, OgFrameBufferHandle* srcBuffer, uint dstX0, uint dstY0, uint dstX1, uint dstY1, OgFrameBufferHandle* dstBuffer) override;

	VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level, bool begin);

	void FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free);

	OgPixelFormat GetDefaultDepthFormat() override;

	void Submit(OgSwapChain* swapchain, OgCommandEncoderHandle* encoder) override;

	void Present(OgSwapChain* swapchain) override;

	void Suspend(OgSwapChain* swapchain) override;

	void Restore(OgSwapChain* swapchain) override;

	void WaitDeviceIdle() override;

	void Collect() override;

	void Shutdown(void) override;

	bool HasFeature(OgRenderFeature feature) override;

	OgResourceSetPool* CreateResourceSetPool(uint32 maxUniformBufferFromPool, uint32 maxTextureFromPool, uint32 maxSetFromPool) override;

	void DestroyResourceSetPool(OgResourceSetPool* resourceSetPool) override;

private:

	void initInstance();
	void initDebug();
	void initDevice();
	
	void initCommandPool();
	void initDescriptorPool();
	
	// TODO list

	//OgBufferHandle* buildBuffer(void* data, size_t size, OgBufferUsage usage, OgMemoryOption option = OgMemoryOption::PRIVATE_GPU);
	//void releaseBuffer(OgBufferHandle* buffer);
	//
	//void buildTexture(OgTextureVK* texture);
	//void releaseTexture(OgTextureVK* texture);
	//
	//void buildMipmap(VkCommandBuffer flyCmd, OgTextureVK* tex, VkImageSubresourceRange& subresourceRange);
	//
	//void buildGraphicsPipeline(OgGraphicsPipelineVK* pipeline);
	//void releaseGraphicsPipeline(OgGraphicsPipelineVK* pipeline);
	//
	//void buildResourceSet(OgResourceSetVK* rSet);
	//void releaseResourceSet(OgResourceSetVK* resourceSet);
	//
	//void buildRenderPass(OgRenderPassVK* renderPass);
	//void releaseRenderPass(OgRenderPassVK* renderPass);

private:
	VkInstance _instance;

	VkDebugReportCallbackEXT _reportCallbackHandle;
	vector<const char*> _enabledInstanceExtensions;
//
//	// Quick Reference for comfort.
	OgDeviceVulkan* _vulkanDevice;
	VkPhysicalDevice _gpuDeviceVK;
	VkPhysicalDeviceFeatures _deviceFeaturesVK;
	VkPhysicalDeviceMemoryProperties _deviceMemoryPropertiesVK;
	VkDevice _logicalDeviceVK;
	VkQueue _graphicsQueueVK;
	VkFormat _defaultDepthFormat;
	VkCommandPool _cmdPoolVK;

	// Manual Managed Descriptor Pool
	VkDescriptorPool _descriptorPool;
	uint32 _usedUniformBufferFromPool;
	uint32 _usedTextureFromPool;
	uint32 _usedSetFromPool;
	uint32 _maxUniformBufferFromPool;
	uint32 _maxTextureFromPool;
	uint32 _maxSetFromPool;
//
//	// Staging Buffer Pool / Command Buffer for updating buffer
	bool _acquireOnceForPresent;

	struct SwapchainWrapper
	{
		// TODO : 중복되는 정보 제거하기
		SwapchainWrapper* next;

		// TODO: reconsider
		System::OgNativeWindow* window;

		OgSwapChainVulkan swapchainVK;
		VkQueue presentQueueVK;

		std::queue<OgCommandEncoderHandle*> encoderQueue;

		struct
		{
			bool isInitialized = false;
			bool hasDepthStencilBuffer = false;
			bool hasMSAAbuffer = false;

			uint32 bufferCount;

			OgRenderPassHandle* renderPass;

			OgSamplerHandle* depthStencilSampler;
			OgTextureHandle* depthStencilTexture;

			OgSamplerHandle* multisampleColorSampler;
			OgTextureHandle* multisampleColorTexture;

			OgFrameBufferHandle** frameBuffers;
		} frameBufferObject;

		struct
		{
			bool isInitialized = false;

			uint32 submissionIndex;
			uint32 swapchainIndex;
			VkFence* fences;
			VkSemaphore* imageReadys;
			VkSemaphore* renderDones;
		} syncObject;

		// For User Interaction
		OgSwapChainInfo settingInfo;
		OgSwapChain swapchainResult;
	};

	// SwapChain
	void prepareSwapChain(SwapchainWrapper& sw);
	void initSwapChain(SwapchainWrapper& sw);
	void initSwapChainSyncObject(SwapchainWrapper& sw);
	void destroySwapChainSyncObject(SwapchainWrapper& sw);
	void destroySwapChainFramebuffers(SwapchainWrapper& sw);
	void destroySwapChain(SwapchainWrapper& sw);

	// Key: PointerHash(OgSwapChain*)
	std::unordered_map<uint32, SwapchainWrapper*> _swapChainTables;
	SwapchainWrapper* _rootSwapchainWrapper;

#if defined(_DEBUG)
	vector<OgHandle*> _livingObjects;
#endif
};

OG_NAMESPACE_RENDER_END

#endif //_OG_RENDERER_VULKAN_H
