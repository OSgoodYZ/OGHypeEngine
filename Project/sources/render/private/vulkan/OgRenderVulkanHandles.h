#pragma once
#ifndef __OG_RENDER_VULKAN_HANDLES_H__
#define __OG_RENDER_VULKAN_HANDLES_H__

#if defined(__ANDROID__)
#if (__ANDROID_API__  < 23)
#include "render/private/android/vulkan_wrapper.h"
#else
#include <vulkan/vulkan.h>
#endif
#else
#include <vulkan/vulkan.h>
#endif

#include "system/OgVector.h"

#include "render/OgRenderDefinitions.h"
#include "render/private/vulkan/OgDeviceVulkan.h"
#include "render/private/vulkan/OgUtilityVulkan.h"

using namespace Og::System;

OG_NAMESPACE_RENDER_BEGIN


struct OgBufferVK : OgBufferHandle
{
	OgDeviceVulkan& vulkanDevice;

	VkBuffer bufferVK;
	VkDeviceMemory memoryVK;
	VkMemoryPropertyFlags memoryPropertyFlags;
	VkBool32 isAutoCoherent;

	void* mapped;
	uint32 innerOffset;

	uint32 mappedSize;
	uint32 mappedOffset;
	bool isMapBufferCalled;

	OgBufferVK() = delete;
	OgBufferVK(OgDeviceVulkan& device, uint32 bufferSize, OgBufferUsage bufUsage, OgMemoryOption memOption)
		: vulkanDevice(device)
		, bufferVK(VK_NULL_HANDLE)
		, memoryVK(VK_NULL_HANDLE)
		, isAutoCoherent(false)
		, mapped(nullptr)
		, mappedSize(0)
		, mappedOffset(0)
		, isMapBufferCalled(false)
		, innerOffset(0)
	{
		size = bufferSize;
		usage = bufUsage;
		option = memOption;
	}


	// 지금 이런식으로 memory와 buffer가 같이 운용되고 있는 것은 문제가 있음.
	VkResult Build(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, void* data = nullptr)
	{
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.usage = usageFlags;
		bufferCreateInfo.size = size;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		this->memoryPropertyFlags = memoryPropertyFlags;

		VK_CHECK_RESULT(vkCreateBuffer(vulkanDevice.logicalDevice, &bufferCreateInfo, nullptr, &bufferVK));

		VkMemoryRequirements memReqs;
		VkMemoryAllocateInfo memAlloc{};
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		vkGetBufferMemoryRequirements(vulkanDevice.logicalDevice, bufferVK, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		// Find a memory type index that fits the properties of the buffer

		memAlloc.memoryTypeIndex = vulkanDevice.GetMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
		VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice.logicalDevice, &memAlloc, nullptr, &memoryVK));

		VkMemoryType type = vulkanDevice.memoryProperties.memoryTypes[memAlloc.memoryTypeIndex];

		// Bifrost(Mali) GPU VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & VK_MEMORY_PROPERTY_HOST_CACHED_BIT 옵션이 둘다 지원되어야 
		// Hardware Cache Coherent 이다.
		isAutoCoherent = false;
		if ((type.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) != 0)
		{
			if ((type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0)
			{
				isAutoCoherent = true;
			}
		}

		// createbuffer -> get the mapped pointer.
		VK_CHECK_RESULT(Map());

		if (data != nullptr)
		{
			memcpy(mapped, data, size);
			if (!isAutoCoherent)
				Flush();
			//Unmap();
		}

		// Attach the memory to the buffer object
		return vkBindBufferMemory(vulkanDevice.logicalDevice, bufferVK, memoryVK, 0);
	}

	VkResult Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
	{
		// If size is equal to VK_WHOLE_SIZE, the end of the current mapping of memory must be a multiple of VkPhysicalDeviceLimits::nonCoherentAtomSize bytes from the beginning of the memory object
		if (size != VK_WHOLE_SIZE)
		{
			int limitSize = (int)vulkanDevice.properties.limits.nonCoherentAtomSize;
			if (size % limitSize != 0)
			{
				int multiple = (int)size / (int)limitSize;
				size = (multiple + 1) * vulkanDevice.properties.limits.nonCoherentAtomSize;
			}
		}

		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = memoryVK;
		mappedRange.offset = offset;
		mappedRange.size = size;
		return vkFlushMappedMemoryRanges(vulkanDevice.logicalDevice, 1, &mappedRange);
	}

	VkResult Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
	{
		return vkMapMemory(vulkanDevice.logicalDevice, memoryVK, offset, size, 0, &mapped);
	}

	void Unmap()
	{
		if (mapped)
		{
			vkUnmapMemory(vulkanDevice.logicalDevice, memoryVK);
			mapped = nullptr;
		}
	}

	virtual void Destroy()
	{
		if (bufferVK != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(vulkanDevice.logicalDevice, bufferVK, nullptr);
		}
		if (memoryVK != VK_NULL_HANDLE)
		{
			vkFreeMemory(vulkanDevice.logicalDevice, memoryVK, nullptr);
		}
	}
};

struct OgUniformBufferVK : OgBufferVK
{
	OgUniformBufferVK() = delete;
	OgUniformBufferVK(OgDeviceVulkan& device, uint32 bufferSize, OgBufferUsage bufUsage, OgMemoryOption memOption)
		: OgBufferVK(device, bufferSize, bufUsage, memOption)
		, buffer(bufferSize, true)
	{}

	OgByteBuffer buffer;

	void Start(uint32 position = 0) override
	{
		buffer.Start(position);
	}

	void Align(uint32 alignment) override
	{
		buffer.Align(alignment);
	}

	void Write(const void* data, uint32 amount) override
	{
		buffer.Write(data, amount);
	}

	void Read(void* data, uint32 amount) override
	{
		buffer.Read(data, amount);
	}

	void Reset() override
	{
		buffer.Reset();
	}

	void End() override
	{
		// Vulkan mapped pointer는 생성시에 이메 Map이 되어있다.
		memcpy(mapped, buffer.pointer, buffer.position);

		if (!isAutoCoherent)
			Flush();//buffer.position, innerOffset

		buffer.End();
	}

	void Destroy() override
	{
		OgBufferVK::Destroy();
		buffer.Free();
	}
};

struct OgSamplerVK : OgSamplerHandle
{
	VkSampler samplerVK;
};

struct OgTextureVK : OgTextureHandle
{
	VkDeviceMemory memory;
	VkImage image;
	VkImageView view;

	VkImageLayout imageLayout;
	VkFormat vkFormat;
	void** data;

	OgTextureVK() = delete;

	// Swapchain only
	OgTextureVK(VkImage image, VkImageView view)
		: image(image)
		, view(view)
	{}

	OgTextureVK(const OgTextureInfo& info, OgSamplerHandle* sampler, VkFormat format, void** data) :
		vkFormat(format), data(data),
		memory(VK_NULL_HANDLE), image(VK_NULL_HANDLE), view(VK_NULL_HANDLE)
	{
		this->info = info;
		this->sampler = sampler;
	}
	~OgTextureVK() {  }
};

struct OgShaderVK : OgShaderHandle
{
	OgShaderVK()
		: OgShaderHandle()
		, shaderStageInfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO , }
	{}

	VkPipelineShaderStageCreateInfo shaderStageInfo;
	VkShaderModule shaderModuleVK;
};

struct OgRenderPassVK : OgRenderPassHandle
{
	OgRenderPassVK() = delete;
	OgRenderPassVK(const OgRenderPassInfo& info) : OgRenderPassHandle(info) { }
	~OgRenderPassVK() { }

	VkRenderPass renderPassVK;
};

struct OgDefaultFrameBufferVK : OgFrameBufferHandle
{
	OgDefaultFrameBufferVK() = delete;
	OgDefaultFrameBufferVK(const OgFrameBufferInfo& info) = delete;

	OgDefaultFrameBufferVK(VkDevice device,
		bool useMSAA, bool useDepthStencil, uint width, uint height, VkImageView multisample, VkImageView color, VkImageView depth, VkRenderPass renderPass)
		: device(device),
		OgFrameBufferHandle(useDepthStencil, width, height)
	{
		uint attachmentCount = 1;
		if (useMSAA && useDepthStencil)
		{
			attachmentCount = 3;
		}
		else if (useDepthStencil)
		{
			attachmentCount = 2;
		}

		VkImageView imageViews[3];
		if (useMSAA)
		{
			imageViews[0] = multisample, imageViews[1] = color, imageViews[2] = depth;
		}
		else
		{
			imageViews[0] = color, imageViews[1] = depth;
		}


		VkFramebufferCreateInfo fbufCreateInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		fbufCreateInfo.renderPass = renderPass;
		fbufCreateInfo.width = width;
		fbufCreateInfo.height = height;
		fbufCreateInfo.layers = 1;
		fbufCreateInfo.attachmentCount = attachmentCount;
		fbufCreateInfo.pAttachments = imageViews;

		VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &frameBufferVK));
	}

	~OgDefaultFrameBufferVK()
	{
		if (frameBufferVK != NULL)
		{
			vkDestroyFramebuffer(device, frameBufferVK, nullptr);
			frameBufferVK = NULL;
		}
	}

	VkDevice device;
	VkFramebuffer frameBufferVK;
};

struct OgFrameBufferVK : OgFrameBufferHandle
{
	OgFrameBufferVK() = delete;
	OgFrameBufferVK(bool useDepthStencil, uint width, uint height) = delete;

	OgFrameBufferVK(VkDevice device, const OgFrameBufferInfo& fbInfo)
		: device(device)
		, OgFrameBufferHandle(fbInfo)
	{
		uint16 attachmentCount = this->colorBufferCount;
		vector<VkImageView> attachments;
		for (uint16 i = 0; i < attachmentCount; ++i)
			attachments.push_back(static_cast<OgTextureVK*>(fbInfo.colorBuffers[i])->view);

		// TODO MSAA
		//if (fbInfo.renderPass->info.resoOGeColorAttachmentCount > 0)
		//{
		//	//일단 resoOGe 될수 있는 attachment는 1개로만 제한한다. 추후에 많은 resoOGe attachment구현 가능성이 있음.
		//	attachments.Add(static_cast<OGTextureVK*>(fbInfo.resoOGeColorBuffer)->view);
		//	++attachmentCount;
		//}

		if (this->useDepthStencilBuffer)
		{
			attachments.push_back(static_cast<OgTextureVK*>(fbInfo.depthStencilBuffer)->view);
			++attachmentCount;
		}

		VkFramebufferCreateInfo fbufCreateInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		fbufCreateInfo.renderPass = static_cast<OgRenderPassVK*>(fbInfo.renderPass)->renderPassVK;
		fbufCreateInfo.width = this->width;
		fbufCreateInfo.height = this->height;
		fbufCreateInfo.layers = 1;
		fbufCreateInfo.attachmentCount = attachmentCount;
		fbufCreateInfo.pAttachments = attachments.data();

		VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &frameBufferVK));
	}

	~OgFrameBufferVK()
	{
		if (frameBufferVK != NULL)
		{
			vkDestroyFramebuffer(device, frameBufferVK, nullptr);
			frameBufferVK = NULL;
		}
	}

	VkDevice device;
	VkFramebuffer frameBufferVK;
};

struct OgGraphicsPipelineVK : OgPipelineHandle
{
	OgGraphicsPipelineVK() = delete;

	OgGraphicsPipelineVK(const OgPipelineDescriptor& descriptor)
		: OgPipelineHandle(descriptor),
		pipelineCache(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE), pipeline(VK_NULL_HANDLE)
	{ }

	~OgGraphicsPipelineVK() { }

	VkPipelineCache pipelineCache;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
};

struct OgComputePipelineVK : OgPipelineHandle
{
	OgComputePipelineVK() = delete;

	OgComputePipelineVK(const OgPipelineDescriptor& descriptor)
		: OgPipelineHandle(descriptor),
		pipelineCache(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE), pipeline(VK_NULL_HANDLE)
	{ }

	~OgComputePipelineVK() { }

	VkPipelineCache pipelineCache;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
};

// OgShaderType 비트마스크를 VkShaderStageFlags로 변환
inline VkShaderStageFlags ConvertOgShaderTypeToVkStageFlags(OgShaderType ogStage)
{
	VkShaderStageFlags flags = 0;
	uint16 s = static_cast<uint16>(ogStage);
	if (s & static_cast<uint16>(OgShaderType::VERTEX))                  flags |= VK_SHADER_STAGE_VERTEX_BIT;
	if (s & static_cast<uint16>(OgShaderType::TESSELLATION_CONTROL))    flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	if (s & static_cast<uint16>(OgShaderType::TESSELLATION_EVALUATION)) flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	if (s & static_cast<uint16>(OgShaderType::GEOMETRY))                flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
	if (s & static_cast<uint16>(OgShaderType::FRAGMENT))                flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
	if (s & static_cast<uint16>(OgShaderType::COMPUTE))                 flags |= VK_SHADER_STAGE_COMPUTE_BIT;
	if (s & static_cast<uint16>(OgShaderType::RAYGEN))                  flags |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	if (s & static_cast<uint16>(OgShaderType::ANY_HIT))                 flags |= VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
	if (s & static_cast<uint16>(OgShaderType::CLOSEST_HIT))             flags |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
	if (s & static_cast<uint16>(OgShaderType::MISS))                    flags |= VK_SHADER_STAGE_MISS_BIT_KHR;
	if (s & static_cast<uint16>(OgShaderType::INTERSECTION))            flags |= VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
	if (s & static_cast<uint16>(OgShaderType::CALLABLE))                flags |= VK_SHADER_STAGE_CALLABLE_BIT_KHR;
	return flags;
}

struct OgResourceLayoutVK : OgResourceLayoutHandle
{
	OgResourceLayoutVK() = delete;

	OgResourceLayoutVK(VkDevice device, OgResourceBinding* bindings, uint count)
		: OgResourceLayoutHandle(bindings, count)
		, device(device)
		, descriptorSetLayoutVK(VK_NULL_HANDLE)
	{
		OG_CHECK(bufferUsageCount + textureUsageCount == count, "Wrong ResourceBinding Count");
		
		OgVector<VkDescriptorSetLayoutBinding> setLayoutBindings((count == 0) ? 1 : count);

		for (uint i = 0; i < count; ++i)
		{
			const OgResourceBinding& rb = bindings[i];

			setLayoutBindings[i].binding = rb.binding;
			// ACCELERATION_STRUCTURE와 STORAGE_IMAGE는 enum 값이 VkDescriptorType과 다르므로 명시적 매핑
			if (rb.type == OgResourceType::ACCELERATION_STRUCTURE)
				setLayoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
			else if (rb.type == OgResourceType::STORAGE_IMAGE)
				setLayoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			else
				setLayoutBindings[i].descriptorType = static_cast<VkDescriptorType>(rb.type);
			setLayoutBindings[i].descriptorCount = rb.arrayCount == 0 ? 1 : rb.arrayCount;
			setLayoutBindings[i].stageFlags = ConvertOgShaderTypeToVkStageFlags(rb.stage);
			setLayoutBindings[i].pImmutableSamplers = nullptr; // 주의 꼭 nullptr 넣어야함.
		}

		VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		descriptorLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.Size());
		descriptorLayoutCI.pBindings = setLayoutBindings.Data();
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &descriptorSetLayoutVK));
	}

	~OgResourceLayoutVK()
	{
		vkDestroyDescriptorSetLayout(device, descriptorSetLayoutVK, nullptr);
	}

	VkDevice device;
	VkDescriptorSetLayout descriptorSetLayoutVK;
};

struct OgResourceSetVK : public OgResourceSetHandle
{
	OgResourceSetVK() = delete;

	OgResourceSetVK(OgResourceLayoutVK* layout, OgResourceUsage* usages, uint32 usageCount)
		: OgResourceSetHandle(usages, usageCount)
		, resourceLayoutVK(layout)
		, descriptorSetVK(VK_NULL_HANDLE)
	{ }

	~OgResourceSetVK()
	{
		resourceLayoutVK = nullptr;
	}

	OgResourceLayoutVK* resourceLayoutVK;

	VkDescriptorSet descriptorSetVK;

	uint32 indexInPool;


};

//struct OGResourceSetPooOGK : OGResourceSetPool
//{
//	VkDevice device;
//	VkDescriptorPool descriptorPool;
//
//	uint32 maxSetFromPool;
//	uint32 maxUniformBufferFromPool;
//	uint32 maxTextureFromPool;
//
//	int32 usedSetFromPool;
//	int32 usedUniformBufferFromPool;
//	int32 usedTextureFromPool;
//
//	OGResourceSetVK* resourceSets = nullptr;
//	uint32 lastAllocIndex;
//	OGQueue<uint32> freeSets;
//
//	OGResourceSetPooOGK(uint32 maxResourceSetCount, uint32 maxUniformCount, uint32 maxTextureCount);
//	~OGResourceSetPooOGK();
//
//	OGResourceSetHandle* Allocate(OGResourceLayoutHandle* resourceSetLayoutHandle, OGResourceUsage* usages, uint32 usageCount) override;
//
//	void Deallocate(OGResourceSetHandle* resourceSetHandle) override;
//
//	void Reset() override;
//};
//
//struct OGBufferManagerVK : public OGBufferManager
//{
//	friend class OGRenderContextVulkan;
//
//	OGBufferManagerVK() = delete;
//	OGBufferManagerVK(OGDeviceVulkan* inDevice, VkDevice inLogicalDeviceVK, VkCommandPool inCmdPooOGK, VkQueue inGraphicsQueueVK);
//	~OGBufferManagerVK();
//
//	enum class OGStagingBufferUsage : uint64
//	{
//		UNIFORM = 0x00000010,
//		INDEX = 0x00000040,
//		VERTEX = 0x00000080,
//		TEXEL = 0x00000004 // VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT
//	};
//
//	OGBufferSuballocation* AllocateBuffer(uint32 size, OGBufferUsage bufferUsageFlags, OGMemoryOption memoryPropertyFlags, bool isAlignedMemory = true, void* data = nullptr) override;
//
//	/**
//	* TODO memoryPaging 기법 적용 필요
//	* UE4의 경우 bufferAllocator(FResourceHeapManager)를 구성하고 있고  이것은 메모리 종류별로 페이징 방식으로 운영되고 있다.
//	* 또한 FResourceHeapManager에서 memory를 buffer에 할당하는 방식으로 vertex buffer, uniform buffer , staging buffer를 운영하고 있다.
//	* 현재 OGEngine의 OGBufferManager은 memoryAllocator를 따로 구현하지 않았다.
//	* 따라서, 현재는 메모리가 필요할 때마다 페이지를 만들어서 할당하는 방식으로 되어 있고 이것은 추후에 UE4의 FResourceHeapManager를 따라서 변경될 예정이다.
//	* 또한 FResourceHeapManager와 같은 것이 구현 됬을 때는 현재 OGBufferManager에서 vertex, index, staging 등이 한번에 관리되고 있지만 이것 또한 UE4처럼 분리해서 관리하는 것이 좋아보인다.
//	*/
//	OGStagingBufferAllocationVK* AllocateStagingBuffer(uint32 size, OGStagingBufferUsage bufferUsageFlags);
//
//	void ReleaseStagingBuffers(VkCommandBuffer* CmdBuffer, OGStagingBufferAllocationVK* StagingBuffer);
//	static void PendingReleaseStagingBuffer(bool bImmediately, bool bFreeToOS);
//
//	void CleanStagingBuffers();
//
//	void InitStagingCommandBuffer();
//	void SubmitStagingCommandBuffer();
//	void FreeCommandBuffers();
//	VkCommandBuffer GetCurrentStagingCommandBuffer() { return stagingCommandBuffer[stagingSubmitIndex]; }
//
//	OG_FORCEINLINE constexpr uint64 alignForStaging(uint64 val, uint64 alignment)
//	{
//		return (uint64)((((uint64)val + alignment - 1) / alignment) * alignment);
//	}
//
//	// variable
//	OGDeviceVulkan* deviceVK;
//	VkDevice logicalDeviceVK;
//	VkCommandPool cmdPooOGK;
//	VkQueue graphicsQueueVK;
//
//	// Staigng Buffer Manager
//	static OGList<OGStagingBufferAllocationVK*> usedStagingAllocation;
//
//	struct OGFreeStagingEntry
//	{
//		OGStagingBufferAllocationVK* stagingBuffer;
//		uint32 releasedFrameNumber;
//	};
//	static OGList<OGFreeStagingEntry> freeStagingBuffers;
//
//	/**
//	* UE4 코드를 봤을 때, VkCommandBuffer 또한 이런식(OGEngine)으로 고정해놓고 쓰는 것이 아닌 어떠한 구조를 가지고 Fence와의 관계를 가지고
//	* FVulkanCommandBufferManager 라는 것을 두고 관리하고 있다.
//	* 따라서 추후에 FVulkanCommandBufferManager와 같은 것이 필요한 이유를 파악하고 구현할 필요가 있어보인다.
//	*/
//	VkCommandBuffer stagingCommandBuffer[3];
//	uint32 stagingSubmitIndex;
//
//	/**
//	* UE4에서는 stagingBuffer의 PendingDeleting을 두 종류로 나눠서 관리하는 것 같다.
//	* 우선 단순 FreeStagingBuffers로 관리를 하고 또한 같은 commandBuffer를 사용하는 것들끼리 같이 지워주기 위해서 아래와 같은 구조를 사용한다.
//	* 이는 FVulkanCommandBufferManager 같은 것이 OGEngine에 구현되었을 때 같이 고도화 될 필요가 있다.
//	*/
//	// TODO: UE4 ref
//	//struct FPendingItemsPerCmdBuffer
//	//{
//	//	FVulkanCmdBuffer* CmdBuffer;
//	//	struct FPendingItems
//	//	{
//	//		uint64 FenceCounter;
//	//		TArray<FStagingBuffer*> Resources;
//	//	};
//	//	inline FPendingItems* FindOrAddItemsForFence(uint64 Fence);
//	//	TArray<FPendingItems> PendingItems;
//	//};
//	//TArray<FPendingItemsPerCmdBuffer> PendingFreeStagingBuffers;
//};
//
//class OGBufferAllocationVK : public OGBufferAllocation
//{
//public:
//	OGBufferAllocationVK(OGBufferManager* inOwner, uint32 inMaxSize, uint32 inAlignment, OGBufferUsage inUsage, OGMemoryOption inMemoryOption, uint32 inPoolSizeIndex);
//
//	// A method that deletes all suballocators in the allocation and forcibly deletes them.
//	void Destroy() override;
//
//	inline VkBuffer* GetVKBufferHandle() { return &_vkBuffer; }
//	inline VkDeviceMemory* GetVKMemoryHandle() { return &_vkMemory; }
//	inline void SetCoherent(bool inCoherentFlag) { isAutoCoherent = inCoherentFlag; }
//	inline bool GetCoherent() { return isAutoCoherent; }
//	void* GetMappedHandle() { return _mapped; }
//
//
//private:
//	friend struct OGBufferManagerVK;
//
//	VkBuffer _vkBuffer;	// VkBuffer Buffer;
//	VkDeviceMemory _vkMemory;
//	bool isAutoCoherent;
//	void* _mapped; // _vkBuffer's mapping pointer
//};
//
//// 현재 OGBufferAllocation를 상속받는 구조로 되어 있는데 이것은 수정될 예정이다.
//class OGStagingBufferAllocationVK : public OGBufferAllocation
//{
//public:
//	OGStagingBufferAllocationVK(OGBufferManager* inOwner, uint32 inSize, uint32 inPoolSizeIndex, OGBufferUsage inUsage, OGMemoryOption InMemoryOption, OGBufferVK* inBufferVK);
//	//OGStagingBufferAllocationVK* TryAllocateStagingBuffer(uint32 inSize);
//	void Destroy() override;
//
//	OGBufferVK* GetBufferVKHandle() { return _bufferVK; }
//	OGBufferManagerVK::OGStagingBufferUsage GetStagingUsage() { return _usage; }
//	void SetStagingUsage(OGBufferManagerVK::OGStagingBufferUsage inStagingUsage) { _usage = inStagingUsage; }
//
//	uint32 GetSize() { return _stagingBufferSize; }
//
//	bool GetInUseFlag() { return _inUseFlag; }
//	void SetInUseFlag(bool inUseFlag) { _inUseFlag = inUseFlag; }
//private:
//	OGBufferManagerVK::OGStagingBufferUsage _usage;
//	OGBufferVK* _bufferVK;
//	uint32 _stagingBufferSize;
//
//	bool _inUseFlag;
//};

// TODO : byte buffer에 쓰지 않고, 바로 vkCmd명령어를 쓸 수 있도록 만들기.
// Ray Tracing Structures
struct OgAccelStructureVK : public OgAccelStructureHandle
{
	OgDeviceVulkan& vulkanDevice;
	VkAccelerationStructureKHR accelStructure;
	VkBuffer buffer;
	VkDeviceMemory memory;
	VkDeviceAddress deviceAddress;
	VkDeviceSize buildScratchSize;
	VkBuffer scratchBuffer;
	VkDeviceMemory scratchMemory;

	OgAccelStructureVK(OgDeviceVulkan& device)
		: vulkanDevice(device)
		, accelStructure(VK_NULL_HANDLE)
		, buffer(VK_NULL_HANDLE)
		, memory(VK_NULL_HANDLE)
		, deviceAddress(0)
		, buildScratchSize(0)
		, scratchBuffer(VK_NULL_HANDLE)
		, scratchMemory(VK_NULL_HANDLE)
	{}

	~OgAccelStructureVK()
	{
		if (vulkanDevice.logicalDevice != VK_NULL_HANDLE)
		{
			if (accelStructure != VK_NULL_HANDLE && vulkanDevice.vkDestroyAccelerationStructureKHR)
			{
				vulkanDevice.vkDestroyAccelerationStructureKHR(vulkanDevice.logicalDevice, accelStructure, nullptr);
			}
			if (buffer != VK_NULL_HANDLE)
			{
				vkDestroyBuffer(vulkanDevice.logicalDevice, buffer, nullptr);
			}
			if (memory != VK_NULL_HANDLE)
			{
				vkFreeMemory(vulkanDevice.logicalDevice, memory, nullptr);
			}
		}
	}
};

struct OgRayTracingPipelineVK : public OgPipelineHandle
{
	OgRayTracingPipelineVK() = delete;

	OgRayTracingPipelineVK(const OgRayTracingPipelineDescriptor& descriptor)
		: OgPipelineHandle()
		, pipelineLayout(VK_NULL_HANDLE)
		, pipeline(VK_NULL_HANDLE)
		, shaderGroupCount(descriptor.shaderGroupCount)
	{
		this->type = OgPipelineType::RAYTRACING_PIPELINE;
		this->name = descriptor.name;
		this->resourceLayout = descriptor.resourceLayout;
	}

	~OgRayTracingPipelineVK() {}

	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	uint32 shaderGroupCount;
};

struct OgCommandEncoderVK : public OgCommandEncoderHandle
{
	OgDeviceVulkan* vulkanDevice;
	VkCommandPool cmdPoolVK;
	VkCommandBuffer cmdBufferVK;

	OgRenderPassVK* curBindRenderPass;
	OgFrameBufferHandle* curBindFramebuffer;
	OgGraphicsPipelineVK* curBindPipeline;
	OgComputePipelineVK* curBindComputePipeline;
	OgRayTracingPipelineVK* curBindRayTracingPipeline;
	VkIndexType curBindIndexBufferType;

	OgCommandEncoderVK(OgDeviceVulkan* device, VkCommandPool cmdPool);

	~OgCommandEncoderVK();

	void Begin() override;

	void BeginRenderPass
	(
		const OgRenderPassHandle* renderPass,
		const OgFrameBufferHandle* frameBuffer,
		const Area area,
		const uint8 colorAttachClearCount,
		const ClearValue* colorAttachmentClear,
		const uint8 resoOGeAttachClearCount,
		const ClearValue* resoOGeAttachmentClear,
		const ClearValue* depthAttachmentClear
	) override;

	void SetViewport
	(
		const float x, const float y,
		const float width, const float height,
		const float minDepth = 0.0f, const float maxDepth = 1.0f
	) override;

	virtual void SetScissor(const int32 x, const int32 y, const uint32 width, const uint32 height) override;

	void BindPipeline(const OgPipelineHandle* pipeline) override;

	void BindResourceSet(const OgResourceSetHandle* rSet) override;

	void BindVertexBuffers(const OgBufferHandle* const * vertexBuffers, const  uint32* offsets, const  uint8 bufferCount) override;

	void BindIndexBuffer(const OgBufferHandle* indexBuffer, const OgIndexType indexType = OgIndexType::UINT16) override;

	void DrawIndexed(const uint32 firstIndex, const uint32 indexCount, const uint32 instanceCount, const uint32 vertexOffset) override;

	void DrawArrays(const uint32 firstVertex, const uint32 vertexCount, const uint32 instanceCount) override;

	void EndRenderPass() override;

	void BeginDebugMarker(const char* label, float color[4]) override;

	void EndDebugMarker() override;

	// Ray Tracing Commands
	void BindRayTracingPipeline(const OgPipelineHandle* pipeline) override;

	void TraceRays(const OgShaderBindingTable& sbt, uint32 width, uint32 height, uint32 depth) override;

	void End() override;

	// Compute shader related implementations
	void BindComputePipeline(const OgPipelineHandle* pipeline) override;

	void Dispatch(const uint32 groupCountX, const uint32 groupCountY, const uint32 groupCountZ) override;

	void DispatchIndirect(const OgBufferHandle* buffer, const uint32 offset) override;

	void PipelineBarrier(
		const uint32 srcAccessMask,
		const uint32 dstAccessMask,
		const uint32 srcStageMask,
		const uint32 dstStageMask
	) override;
};

OG_NAMESPACE_RENDER_END

#endif // __OG_RENDER_VULKAN_HANDLES_H__