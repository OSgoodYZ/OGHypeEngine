#pragma once
#ifndef _OG_RENDER_VULKLAN_HELPER_H__
#define _OG_RENDER_VULKLAN_HELPER_H__

/*
	LvRender API와 Vulkan API 사이의 연결성을 도와주는 헤더.
	LvRender API를 Vulkan에 맞게 해석하고, 해당 Vulkan Handle를 생성하는데 도움을 준다.
*/

#if defined(__ANDROID__)
#if (__ANDROID_API__  < 23)
#include "render/private/android/vulkan_wrapper.h"
#else
#include <vulkan/vulkan.h>
#endif
#else
#include <vulkan/vulkan.h>
#endif

#include "render/OgRenderDefinitions.h"
#include "render/private/vulkan/OgRenderVulkanHandles.h"

OG_NAMESPACE_RENDER_BEGIN

VkFormat vk_convert_vertex_format(OgVertexFormat format);

OgRenderTextureFormat vk_convert_format(VkFormat format);

void vk_convert_format(VkPhysicalDevice gpu, VkAttachmentDescription& target, const OgAttachment& source);

VkImageUsageFlags GetVkImageUsageFlags(OgTextureUsage usage);

bool IsCompatible(OgResourceLayoutHandle* a, OgResourceLayoutHandle* b);

OG_NAMESPACE_RENDER_END

#endif
