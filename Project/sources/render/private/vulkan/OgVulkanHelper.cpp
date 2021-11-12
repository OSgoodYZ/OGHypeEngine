#include "OgPrecompile.h"
#include "OgVulkanHelper.h"

#include "render/private/vulkan/OgUtilityVulkan.h"

OG_NAMESPACE_RENDER_BEGIN

VkFormat vk_convert_vertex_format(OgVertexFormat format)
{
	switch (format)
	{
	case OgVertexFormat::BYTE2_NORM:
		return VkFormat::VK_FORMAT_R8G8_UNORM;
	case OgVertexFormat::BYTE2:
		return VkFormat::VK_FORMAT_R8G8_UINT;
	case OgVertexFormat::BYTE3_NORM:
		return VkFormat::VK_FORMAT_R8G8B8_UNORM;
	case OgVertexFormat::BYTE3:
		return VkFormat::VK_FORMAT_R8G8B8_UINT;
	case OgVertexFormat::BYTE4_NORM:
		return VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
	case OgVertexFormat::BYTE4:
		return VkFormat::VK_FORMAT_R8G8B8A8_UINT;
	case OgVertexFormat::SBYTE2_NORM:
		return VkFormat::VK_FORMAT_R8G8_SNORM;
	case OgVertexFormat::SBYTE2:
		return VkFormat::VK_FORMAT_R8G8_SINT;
	case OgVertexFormat::SBYTE3_NORM:
		return VkFormat::VK_FORMAT_R8G8B8_SNORM;
	case OgVertexFormat::SBYTE3:
		return VkFormat::VK_FORMAT_R8G8B8_SINT;
	case OgVertexFormat::SBYTE4_NORM:
		return VkFormat::VK_FORMAT_R8G8B8A8_SNORM;
	case OgVertexFormat::SBYTE4:
		return VkFormat::VK_FORMAT_R8G8B8A8_SINT;
	case OgVertexFormat::USHORT2_NORM:
		return VkFormat::VK_FORMAT_R16G16_UNORM;
	case OgVertexFormat::USHORT2:
		return VkFormat::VK_FORMAT_R16G16_UINT;
	case OgVertexFormat::USHORT3_NORM:
		return VkFormat::VK_FORMAT_R16G16B16_UNORM;
	case OgVertexFormat::USHORT3:
		return VkFormat::VK_FORMAT_R16G16B16_UINT;
	case OgVertexFormat::USHORT4_NORM:
		return VkFormat::VK_FORMAT_R16G16B16A16_UNORM;
	case OgVertexFormat::USHORT4:
		return VkFormat::VK_FORMAT_R16G16B16A16_UINT;
	case OgVertexFormat::SHORT2_NORM:
		return VkFormat::VK_FORMAT_R16G16_SNORM;
	case OgVertexFormat::SHORT2:
		return VkFormat::VK_FORMAT_R16G16_SINT;
	case OgVertexFormat::SHORT3_NORM:
		return VkFormat::VK_FORMAT_R16G16B16_SNORM;
	case OgVertexFormat::SHORT3:
		return VkFormat::VK_FORMAT_R16G16B16_SINT;
	case OgVertexFormat::SHORT4_NORM:
		return VkFormat::VK_FORMAT_R16G16B16A16_SNORM;
	case OgVertexFormat::SHORT4:
		return VkFormat::VK_FORMAT_R16G16B16A16_SINT;
	case OgVertexFormat::UINT1:
		return VkFormat::VK_FORMAT_R32_UINT;
	case OgVertexFormat::UINT2:
		return VkFormat::VK_FORMAT_R32G32B32_UINT;
	case OgVertexFormat::UINT3:
		return VkFormat::VK_FORMAT_R32G32B32_UINT;
	case OgVertexFormat::UINT4:
		return VkFormat::VK_FORMAT_R32G32B32_UINT;
	case OgVertexFormat::INT1:
		return VkFormat::VK_FORMAT_R32_SINT;
	case OgVertexFormat::INT2:
		return VkFormat::VK_FORMAT_R32G32_SINT;
	case OgVertexFormat::INT3:
		return VkFormat::VK_FORMAT_R32G32B32_SINT;
	case OgVertexFormat::INT4:
		return VkFormat::VK_FORMAT_R32G32B32A32_SINT;
	case OgVertexFormat::FLOAT1:
		return VkFormat::VK_FORMAT_R32_SFLOAT;
	case OgVertexFormat::FLOAT2:
		return VkFormat::VK_FORMAT_R32G32_SFLOAT;
	case OgVertexFormat::FLOAT3:
		return VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
	case OgVertexFormat::FLOAT4:
		return VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
	case OgVertexFormat::HALF2:
		return VkFormat::VK_FORMAT_R16G16_SFLOAT;
	case OgVertexFormat::HALF4:
		return VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT;
	default:
		LOGE(OG_ID, "%i is not converted", format);
		return VkFormat::VK_FORMAT_UNDEFINED;
		break;
	}
}

OgRenderTextureFormat vk_convert_format(VkFormat format)
{
	switch (format)
	{
	case VK_FORMAT_R8G8B8A8_UNORM:
		return OgRenderTextureFormat::R8G8B8A8_UNORM;

	case VK_FORMAT_R5G6B5_UNORM_PACK16:
		return OgRenderTextureFormat::R5G6B5;

	case VK_FORMAT_B8G8R8A8_UNORM:
		return OgRenderTextureFormat::B8G8R8A8;

	default:
		LOGE(OG_ID, "Not Supported Yet");
		return OgRenderTextureFormat::NONE;
	}
}

void vk_convert_format(VkPhysicalDevice gpu, VkAttachmentDescription& target, const OgAttachment& source)
{
	VkFormat vformat = (VkFormat)OgFormatSupplement::GetPixelFormat(source.format);
	if (vkIsSupportFormat(gpu, vformat) == false)
		LOGE(OG_ID, "Not Supprted Yet");
	target.format = vformat;

	switch (source.load)
	{
	case OgRenderBufferLoadAction::CLEAR:
	{
		target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		break;
	}
	case OgRenderBufferLoadAction::DONT_CARE:
	{
		target.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		break;
	}
	case OgRenderBufferLoadAction::LOAD:
	{
		target.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		break;
	}
	}

	switch (source.store)
	{
	case OgRenderBufferStoreAction::STORE:
	{
		target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		break;
	}
	case OgRenderBufferStoreAction::DONT_CARE:
	{
		target.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		break;
	}
	//case OgRenderBufferStoreAction::RESOLVE:
	//{
	//	// Vk is not supported RESOLVE.
	//	// https://docs.unity3d.com/ScriptReference/Rendering.RenderBufferStoreAction.html
	//	// LOGE(LV_ID, "Vk is not supported RESOLVE");
	//	target.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	//	break;
	//}
	}

	target.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	target.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	target.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

VkImageUsageFlags GetVkImageUsageFlags(OgTextureUsage usage)
{
	VkImageUsageFlags viFlags = 0;

	if ((usage & OgTextureUsage::GPU_LOCAL) != 0) {}
	if ((usage & OgTextureUsage::STAGING) != 0) {}
	if ((usage & OgTextureUsage::SAMPLED) != 0) viFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
	if ((usage & OgTextureUsage::STORAGE) != 0) viFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
	if ((usage & OgTextureUsage::COLOR_ATTACHMENT) != 0) viFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if ((usage & OgTextureUsage::DEPTH_ATTACHMENT) != 0) viFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if ((usage & OgTextureUsage::STENCIL_ATTACHMENT) != 0) viFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if ((usage & OgTextureUsage::DEPTH_STENCIL_ATTACHMENT) != 0) viFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if ((usage & OgTextureUsage::TRANSIENT_ATTACHMENT) != 0) viFlags |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;

	return viFlags;
}

bool IsCompatible(OgResourceLayoutHandle* a, OgResourceLayoutHandle* b)
{

	//TODO
	//ASSERT(a != nullptr && b != nullptr);
	//OgResourceLayoutVK* avk = reinterpret_cast<OgResourceLayoutVK*>(a);
	//OgResourceLayoutVK* bvk = reinterpret_cast<OgResourceLayoutVK*>(b);

	//// Total Count Check
	//if (avk->bindingCount != bvk->bindingCount)
	//	return false;

	//// Each Binding Check
	//for (uint i = 0; i < bvk->bindingCount; ++i)
	//{
	//	OgResourceBinding* ab = &(avk->bindings[i]);
	//	OgResourceBinding* bb = &(bvk->bindings[i]);

	//	if (IsCompatible(ab, bb) == false)
	//		return false;
	//}

	//return true;

	// temp code
	return false;
}


OG_NAMESPACE_RENDER_END