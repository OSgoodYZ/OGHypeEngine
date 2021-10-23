#include <list>
#include <vector>

#include "OgPrecompile.h"
#include "OgUtilityVulkan.h"

using namespace std;

const char* vkErrorString(VkResult errorCode)
{
	switch (errorCode)
	{
#define STR(r) case VK_ ##r: return #r
		STR(NOT_READY);
		STR(TIMEOUT);
		STR(EVENT_SET);
		STR(EVENT_RESET);
		STR(INCOMPLETE);
		STR(ERROR_OUT_OF_HOST_MEMORY);
		STR(ERROR_OUT_OF_DEVICE_MEMORY);
		STR(ERROR_INITIALIZATION_FAILED);
		STR(ERROR_DEVICE_LOST);
		STR(ERROR_MEMORY_MAP_FAILED);
		STR(ERROR_LAYER_NOT_PRESENT);
		STR(ERROR_EXTENSION_NOT_PRESENT);
		STR(ERROR_FEATURE_NOT_PRESENT);
		STR(ERROR_INCOMPATIBLE_DRIVER);
		STR(ERROR_TOO_MANY_OBJECTS);
		STR(ERROR_FORMAT_NOT_SUPPORTED);
		STR(ERROR_SURFACE_LOST_KHR);
		STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
		STR(SUBOPTIMAL_KHR);
		STR(ERROR_OUT_OF_DATE_KHR);
		STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
		STR(ERROR_VALIDATION_FAILED_EXT);
		STR(ERROR_INVALID_SHADER_NV);
#undef STR
	default:
		return "UNKNOWN_ERROR";
	}
}


VkBool32 vkGetSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat* depthFormat)
{
	// Since all depth formats may be optional, we need to find a suitable depth format to use
	// Start with the highest precision packed format

	vector<VkFormat> depthFormats =
	{
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM
	};

	for (int i = 0; i < depthFormats.size(); ++i)
	{
		auto& format = depthFormats[i];
		VkFormatProperties formatProps;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
		// Format must support depth stencil attachment for optimal tiling
		if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			*depthFormat = format;
			return true;
		}
	}

	return false;
}

VkBool32 vkIsSupportFormat(VkPhysicalDevice physicalDevice, VkFormat format)
{
	VkFormatProperties formatProps;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);

	uint result =
		formatProps.linearTilingFeatures |
		formatProps.optimalTilingFeatures |
		formatProps.bufferFeatures;
	return result != 0 ? true : false;
}

VkBool32 vkGetSupportedVertexBufferFormat(VkPhysicalDevice physicalDevice, VkFormat* bufferFormat)
{
	// Since all depth formats may be optional, we need to find a suitable depth format to use
	// Start with the highest precision packed format
	vector<VkFormat> formats =
	{
		VK_FORMAT_R32G32B32_SFLOAT,
	};

	for (int i = 0; i < formats.size(); ++i)
	{
		auto& format = formats[i];
		VkFormatProperties formatProps;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
		// Format must support depth stencil attachment for optimal tiling
		if (formatProps.bufferFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT)
		{
			*bufferFormat = format;
			return true;
		}
	}

	return false;
}

void vkSetImageLayout(
	VkCommandBuffer cmdbuffer,
	VkImage image,
	VkImageLayout oldImageLayout,
	VkImageLayout newImageLayout,
	VkImageSubresourceRange subresourceRange,
	VkPipelineStageFlags srcStageMask,
	VkPipelineStageFlags dstStageMask)
{
	// Create an image barrier object
	VkImageMemoryBarrier imageMemoryBarrier{};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	imageMemoryBarrier.oldLayout = oldImageLayout;
	imageMemoryBarrier.newLayout = newImageLayout;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.subresourceRange = subresourceRange;

	// Source layouts (old)
	// Source access mask controls actions that have to be finished on the old layout
	// before it will be transitioned to the new layout
	switch (oldImageLayout)
	{
	case VK_IMAGE_LAYOUT_UNDEFINED:
		// Image layout is undefined (or does not matter)
		// Only valid as initial layout
		// No flags required, listed only for completeness
		imageMemoryBarrier.srcAccessMask = 0;
		break;

	case VK_IMAGE_LAYOUT_PREINITIALIZED:
		// Image is preinitialized
		// Only valid as initial layout for linear images, preserves memory contents
		// Make sure host writes have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		// Image is a color attachment
		// Make sure any writes to the color buffer have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		// Image is a depth/stencil attachment
		// Make sure any writes to the depth/stencil buffer have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		// Image is a transfer source 
		// Make sure any reads from the image have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		// Image is a transfer destination
		// Make sure any writes to the image have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		// Image is read by a shader
		// Make sure any shader reads from the image have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	default:
		// Other source layouts aren't handled (yet)
		break;
	}

	// Target layouts (new)
	// Destination access mask controls the dependency for the new image layout
	switch (newImageLayout)
	{
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		// Image will be used as a transfer destination
		// Make sure any writes to the image have been finished
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		// Image will be used as a transfer source
		// Make sure any reads from the image have been finished
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		// Image will be used as a color attachment
		// Make sure any writes to the color buffer have been finished
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		// Image layout will be used as a depth/stencil attachment
		// Make sure any writes to depth/stencil buffer have been finished
		imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		// Image will be read in a shader (sampler, input attachment)
		// Make sure any writes to the image have been finished
		if (imageMemoryBarrier.srcAccessMask == 0)
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		}
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	default:
		// Other source layouts aren't handled (yet)
		break;
	}

	// Put barrier inside setup command buffer
	vkCmdPipelineBarrier(
		cmdbuffer,
		srcStageMask,
		dstStageMask,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);
}

void vkSetImageLayout(
	VkCommandBuffer cmdbuffer,
	VkImage image,
	VkImageAspectFlags aspectMask,
	VkImageLayout oldImageLayout,
	VkImageLayout newImageLayout,
	VkPipelineStageFlags srcStageMask,
	VkPipelineStageFlags dstStageMask)
{
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = aspectMask;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 1;
	vkSetImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
}

VkPipelineStageFlags vkPipelineStageFromAccessFlags(VkAccessFlags AccessFlags,
	const VkPipelineStageFlags EnabledGraphicsShaderStages)
{
	// 6.1.3
	VkPipelineStageFlags stages = 0;

	while (AccessFlags != 0)
	{
		VkAccessFlagBits accessFlag = static_cast<VkAccessFlagBits>(AccessFlags & (~(AccessFlags - 1)));
		//VERIFY_EXPR(AccessFlag != 0 && (AccessFlag & (AccessFlag - 1)) == 0);
		AccessFlags &= ~accessFlag;

		// An application MUST NOT specify an access flag in a synchronization command if it does not include a 
		// pipeline stage in the corresponding stage mask that is able to perform accesses of that type.
		// A table that lists, for each access flag, which pipeline stages can perform that type of access is given in 6.1.3.
		switch (accessFlag)
		{
			// Read access to an indirect command structure read as part of an indirect drawing or dispatch command
		case VK_ACCESS_INDIRECT_COMMAND_READ_BIT:
			stages |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
			break;

			// Read access to an index buffer as part of an indexed drawing command, bound by vkCmdBindIndexBuffer
		case VK_ACCESS_INDEX_READ_BIT:
			stages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
			break;

			// Read access to a vertex buffer as part of a drawing command, bound by vkCmdBindVertexBuffers
		case VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT:
			stages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
			break;

			// Read access to a uniform buffer
		case VK_ACCESS_UNIFORM_READ_BIT:
			stages |= EnabledGraphicsShaderStages | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;

			// Read access to an input attachment within a render pass during fragment shading
		case VK_ACCESS_INPUT_ATTACHMENT_READ_BIT:
			stages |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;

			// Read access to a storage buffer, uniform texel buffer, storage texel buffer, sampled image, or storage image
		case VK_ACCESS_SHADER_READ_BIT:
			stages |= EnabledGraphicsShaderStages | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;

			// Write access to a storage buffer, storage texel buffer, or storage image
		case VK_ACCESS_SHADER_WRITE_BIT:
			stages |= EnabledGraphicsShaderStages | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;

			// Read access to a color attachment, such as via blending, logic operations, or via certain subpass load operations
		case VK_ACCESS_COLOR_ATTACHMENT_READ_BIT:
			stages |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			break;

			// Write access to a color or resolve attachment during a render pass or via certain subpass load and store operations
		case VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT:
			stages |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			break;

			// Read access to a depth/stencil attachment, via depth or stencil operations or via certain subpass load operations
		case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT:
			stages |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			break;

			// Write access to a depth/stencil attachment, via depth or stencil operations or via certain subpass load and store operations
		case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT:
			stages |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			break;

			// Read access to an image or buffer in a copy operation
		case VK_ACCESS_TRANSFER_READ_BIT:
			stages |= VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;

			// Write access to an image or buffer in a clear or copy operation
		case VK_ACCESS_TRANSFER_WRITE_BIT:
			stages |= VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;

			// Read access by a host operation. Accesses of this type are not performed through a resource, but directly on memory
		case VK_ACCESS_HOST_READ_BIT:
			stages |= VK_PIPELINE_STAGE_HOST_BIT;
			break;

			// Write access by a host operation. Accesses of this type are not performed through a resource, but directly on memory
		case VK_ACCESS_HOST_WRITE_BIT:
			stages |= VK_PIPELINE_STAGE_HOST_BIT;
			break;

			// Read access via non-specific entities. When included in a destination access mask, makes all available writes 
			// visible to all future read accesses on entities known to the Vulkan device
		case VK_ACCESS_MEMORY_READ_BIT:
			break;

			// Write access via non-specific entities. hen included in a source access mask, all writes that are performed 
			// by entities known to the Vulkan device are made available. When included in a destination access mask, makes 
			// all available writes visible to all future write accesses on entities known to the Vulkan device.
		case VK_ACCESS_MEMORY_WRITE_BIT:
			break;

		default:
			LOGE(OG_ID, "Unknown memory access flag");
		}
	}
	return stages;
}

// false - source mask 
// true  - destination mask 
VkPipelineStageFlags AccessMaskFromImageLayout(VkImageLayout Layout, bool IsDstMask)
{
	VkPipelineStageFlags AccessMask = 0;
	switch (Layout)
	{
		// does not support device access. This layout must only be used as the initialLayout member 
		// of VkImageCreateInfo or VkAttachmentDescription, or as the oldLayout in an image transition. 
		// When transitioning out of this layout, the contents of the memory are not guaranteed to be preserved (11.4)
	case VK_IMAGE_LAYOUT_UNDEFINED:
		if (IsDstMask)
		{
			LOGE(OG_ID, "The new layout used in a transition must not be VK_IMAGE_LAYOUT_UNDEFINED. "
				"This layout must only be used as the initialLayout member of VkImageCreateInfo "
				"or VkAttachmentDescription, or as the oldLayout in an image transition. (11.4)");
		}
		break;

		// supports all types of device access
	case VK_IMAGE_LAYOUT_GENERAL:
		// VK_IMAGE_LAYOUT_GENERAL must be used for image load/store operations (13.1.1, 13.2.4)
		AccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		break;

		// must only be used as a color or resolve attachment in a VkFramebuffer (11.4)
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		AccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

		// must only be used as a depth/stencil attachment in a VkFramebuffer (11.4)
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		AccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

		// must only be used as a read-only depth/stencil attachment in a VkFramebuffer and/or as a read-only image in a shader (11.4)
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
		AccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		break;

		// must only be used as a read-only image in a shader (which can be read as a sampled image, 
		// combined image/sampler and/or input attachment) (11.4)
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		AccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
		break;

		//  must only be used as a source image of a transfer command (11.4)
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		AccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;

		// must only be used as a destination image of a transfer command (11.4)
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		AccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

		// does not support device access. This layout must only be used as the initialLayout member
		// of VkImageCreateInfo or VkAttachmentDescription, or as the oldLayout in an image transition.
		// When transitioning out of this layout, the contents of the memory are preserved. (11.4)
	case VK_IMAGE_LAYOUT_PREINITIALIZED:
		if (!IsDstMask)
		{
			AccessMask = VK_ACCESS_HOST_WRITE_BIT;
		}
		else
		{
			LOGE(OG_ID, "The new layout used in a transition must not be VK_IMAGE_LAYOUT_PREINITIALIZED. "
				"This layout must only be used as the initialLayout member of VkImageCreateInfo "
				"or VkAttachmentDescription, or as the oldLayout in an image transition. (11.4)");
		}
		break;

	case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
		AccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
		AccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
		AccessMask = VK_ACCESS_MEMORY_READ_BIT;
		break;

	default:
		LOGE(OG_ID, "Unexpected image layout");
		break;
	}

	return AccessMask;
}

void TransitionImageLayout(VkCommandBuffer                CmdBuffer,
	VkImage                        Image,
	VkImageLayout                  OldLayout,
	VkImageLayout                  NewLayout,
	const VkImageSubresourceRange& SubresRange,
	VkPipelineStageFlags           EnabledGraphicsShaderStages,
	VkPipelineStageFlags           SrcStages,
	VkPipelineStageFlags           DestStages)
{
	//VERIFY_EXPR(CmdBuffer != VK_NULL_HANDLE);

	VkImageMemoryBarrier ImgBarrier = {};
	ImgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	ImgBarrier.pNext = nullptr;
	ImgBarrier.srcAccessMask = 0;
	ImgBarrier.dstAccessMask = 0;
	ImgBarrier.oldLayout = OldLayout;
	ImgBarrier.newLayout = NewLayout;
	ImgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // source queue family for a queue family ownership transfer.
	ImgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // destination queue family for a queue family ownership transfer.
	ImgBarrier.image = Image;
	ImgBarrier.subresourceRange = SubresRange;
	ImgBarrier.srcAccessMask = AccessMaskFromImageLayout(OldLayout, false);
	ImgBarrier.dstAccessMask = AccessMaskFromImageLayout(NewLayout, true);

	if (SrcStages == 0)
	{
		if (OldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		{
			SrcStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		}
		else if (ImgBarrier.srcAccessMask != 0)
		{
			SrcStages = vkPipelineStageFromAccessFlags(ImgBarrier.srcAccessMask, EnabledGraphicsShaderStages);
		}
		else
		{
			// An execution dependency with only VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT in the source stage 
			// mask will effectively not wait for any prior commands to complete. (6.1.2)
			SrcStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		}
	}

	if (DestStages == 0)
	{
		if (NewLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		{
			DestStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		}
		else if (ImgBarrier.dstAccessMask != 0)
		{
			DestStages = vkPipelineStageFromAccessFlags(ImgBarrier.dstAccessMask, EnabledGraphicsShaderStages);
		}
		else
		{
			// An execution dependency with only VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT in the destination 
			// stage mask will only prevent that stage from executing in subsequently submitted commands. 
			// As this stage does not perform any actual execution, this is not observable - in effect, 
			// it does not delay processing of subsequent commands. (6.1.2)
			DestStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		}
	}

	// Including a particular pipeline stage in the first synchronization scope of a command implicitly 
	// includes logically earlier pipeline stages in the synchronization scope. Similarly, the second 
	// synchronization scope includes logically later pipeline stages.
	// However, note that access scopes are not affected in this way - only the precise stages specified 
	// are considered part of each access scope.  (6.1.2)

	vkCmdPipelineBarrier(CmdBuffer,
		SrcStages,  // must not be 0
		DestStages, // must not be 0
		0, // a bitmask specifying how execution and memory dependencies are formed
		0,       // memoryBarrierCount
		nullptr, // pMemoryBarriers
		0,       // bufferMemoryBarrierCount
		nullptr, // pBufferMemoryBarriers
		1,
		&ImgBarrier);
	// Each element of pMemoryBarriers, pBufferMemoryBarriers and pImageMemoryBarriers must not 
	// have any access flag included in its srcAccessMask member if that bit is not supported by 
	// any of the pipeline stages in srcStageMask.
	// Each element of pMemoryBarriers, pBufferMemoryBarriers and pImageMemoryBarriers must not 
	// have any access flag included in its dstAccessMask member if that bit is not supported by any 
	// of the pipeline stages in dstStageMask (6.6)
}

void CommandpipelineBarrierForBufferUpdate(VkCommandBuffer                CmdBuffer,
	VkBuffer										buffer,
	uint64_t										offset,
	uint64_t										size,
	VkPipelineStageFlags							p_src_stage_mask,
	VkPipelineStageFlags							p_dst_stage_mask,
	VkAccessFlags									p_src_access,
	VkAccessFlags									p_dst_sccess)
{
	VkBufferMemoryBarrier bufferMemortBarrier;
	bufferMemortBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	bufferMemortBarrier.pNext = nullptr;
	bufferMemortBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	bufferMemortBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	bufferMemortBarrier.srcAccessMask = p_src_access;
	bufferMemortBarrier.dstAccessMask = p_dst_sccess;
	bufferMemortBarrier.buffer = buffer;
	bufferMemortBarrier.offset = offset;
	bufferMemortBarrier.size = size;

	vkCmdPipelineBarrier(CmdBuffer, p_src_stage_mask, p_dst_stage_mask, 0, 0, nullptr, 1, &bufferMemortBarrier, 0, nullptr);

}
