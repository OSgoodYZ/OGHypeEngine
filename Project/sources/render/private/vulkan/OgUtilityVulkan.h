#pragma once
#ifndef _OG_VULKANUTILITY_H_
#define _OG_VULKANUTILITY_H_

#if defined(__ANDROID__)
#if (__ANDROID_API__  < 23)
#include "render/private/android/vulkan_wrapper.h"
#else
#include <vulkan/vulkan.h>
#endif
#else
#include <vulkan/vulkan.h>
#endif

#include <iostream>

const char* vkErrorString(VkResult errorCode);

#if defined(__ANDROID__)
#define VK_CHECK_RESULT(f)																				\
	{																										\
		VkResult res = (f);																					\
		if (res != VK_SUCCESS)																				\
		{																									\
			LOGE(OG_ID, "Fatal : VkResult is \" %s \" in %s at line %d", vkErrorString(res), __FILE__, __LINE__);  \
			ASSERT(res == VK_SUCCESS);																		\
		}																									\
	}
#else

#define VK_CHECK_RESULT(f)																				\
	{																										\
		VkResult res = (f);																					\
		if (res != VK_SUCCESS)																				\
		{																									\
			std::cout << "Fatal : VkResult is \"" << vkErrorString(res) << "\" in " << __FILE__ << " at line " << __LINE__ << std::endl; \
			ASSERT(res == VK_SUCCESS);																		\
		}																									\
	}
#endif


// https://github.com/ChanSingSong/UnrealEngine/blob/fc321c7de9c9e436411d2d754e4b99125350af64/Engine/Source/Runtime/VulkanRHI/Private/VulkanDevice.cpp
VkBool32 vkGetSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat* depthFormat);

// https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkFormatProperties.html
// If no format feature flags are supported, 
// the format itself is not supported, and images of that format cannot be created.
VkBool32 vkIsSupportFormat(VkPhysicalDevice physicalDevice, VkFormat format);

VkBool32 vkGetSupportedVertexBufferFormat(VkPhysicalDevice physicalDevice, VkFormat* bufferFormat);


// Create an image memory barrier for changing the layout of
// an image and put it into an active command buffer
// See chapter 11.4 "Image Layout" for details
void vkSetImageLayout(
	VkCommandBuffer cmdbuffer,
	VkImage image,
	VkImageLayout oldImageLayout,
	VkImageLayout newImageLayout,
	VkImageSubresourceRange subresourceRange,
	VkPipelineStageFlags srcStageMask,
	VkPipelineStageFlags dstStageMask);

// Fixed sub resource on first mip level and layer
void vkSetImageLayout(
	VkCommandBuffer cmdbuffer,
	VkImage image,
	VkImageAspectFlags aspectMask,
	VkImageLayout oldImageLayout,
	VkImageLayout newImageLayout,
	VkPipelineStageFlags srcStageMask,
	VkPipelineStageFlags dstStageMask);


VkPipelineStageFlags vkPipelineStageFromAccessFlags(VkAccessFlags AccessFlags, const VkPipelineStageFlags EnabledGraphicsShaderStages);

// false - source mask 
// true  - destination mask 
VkPipelineStageFlags AccessMaskFromImageLayout(VkImageLayout Layout, bool IsDstMask);

void TransitionImageLayout(VkCommandBuffer                CmdBuffer,
	VkImage                        Image,
	VkImageLayout                  OldLayout,
	VkImageLayout                  NewLayout,
	const VkImageSubresourceRange& SubresRange,
	VkPipelineStageFlags           EnabledGraphicsShaderStages,
	VkPipelineStageFlags           SrcStages,
	VkPipelineStageFlags           DestStages);

void CommandpipelineBarrierForBufferUpdate(VkCommandBuffer                CmdBuffer,
	VkBuffer buffer,
	uint64_t offset,
	uint64_t size,
	VkPipelineStageFlags p_src_stage_mask,
	VkPipelineStageFlags p_dst_stage_mask,
	VkAccessFlags p_src_access,
	VkAccessFlags p_dst_sccess
);


#endif // _OG_VULKANUTILITY_H_