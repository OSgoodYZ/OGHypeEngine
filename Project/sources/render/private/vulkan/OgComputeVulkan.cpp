#include "OgPrecompile.h"

#include "render/private/vulkan/OgRenderContext_Vulkan.h"
#include "render/private/vulkan/OgRenderVulkanHandles.h"
#include "render/private/vulkan/OgVulkanHelper.h"

#if defined(OG_USE_CRT_CHASE_MEMORY_LEAK)
#define new DBG_NEW
#endif

OG_NAMESPACE_RENDER_BEGIN

void OgRenderContextVulkan::buildComputePipeline(OgComputePipelineVK* pipeline)
{
	OgRenderContextVulkan* renderContext = this;
	VkDevice logicalDevice = renderContext->_logicalDeviceVK;

	OgShaderDescriptor& shader = pipeline->shaderDescriptor;
	OgResourceLayoutVK* res = reinterpret_cast<OgResourceLayoutVK*>(pipeline->resourceLayout);

	// Create pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &res->descriptorSetLayoutVK;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	VK_CHECK_RESULT(vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipeline->pipelineLayout));

	// Create pipeline cache
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
	VK_CHECK_RESULT(vkCreatePipelineCache(logicalDevice, &pipelineCacheCreateInfo, nullptr, &pipeline->pipelineCache));

	// Create compute pipeline
	VkComputePipelineCreateInfo computePipelineCreateInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	
	// Find compute shader
	OgShaderVK* computeShader = nullptr;
	for (size_t i = 0; i < shader.shaderCount; ++i)
	{
		OgShaderVK* shaderVK = reinterpret_cast<OgShaderVK*>(shader.shaders[i]);
		if ((shaderVK->shaderStageInfo.stage & VK_SHADER_STAGE_COMPUTE_BIT) != 0)
		{
			computeShader = shaderVK;
			break;
		}
	}

	OG_CHECK(computeShader != nullptr, "Compute shader not found in shader descriptor");

	computePipelineCreateInfo.stage = computeShader->shaderStageInfo;
	computePipelineCreateInfo.layout = pipeline->pipelineLayout;
	computePipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	computePipelineCreateInfo.basePipelineIndex = -1;

	VK_CHECK_RESULT(vkCreateComputePipelines(logicalDevice, pipeline->pipelineCache, 1, &computePipelineCreateInfo, nullptr, &pipeline->pipeline));
}

void OgRenderContextVulkan::releaseComputePipeline(OgComputePipelineVK* pipeline)
{
	if (pipeline->pipelineCache != NULL) vkDestroyPipelineCache(this->_logicalDeviceVK, pipeline->pipelineCache, nullptr);
	if (pipeline->pipelineLayout != NULL) vkDestroyPipelineLayout(this->_logicalDeviceVK, pipeline->pipelineLayout, nullptr);
	if (pipeline->pipeline != NULL) vkDestroyPipeline(this->_logicalDeviceVK, pipeline->pipeline, nullptr);
}

OgPipelineHandle* OgRenderContextVulkan::CreateComputePipeline(OgPipelineDescriptor& descriptor)
{
	OG_CHECK(descriptor.type == OgPipelineType::COMPUTE_PIPELINE, "Wrong pipeline type for compute pipeline creation");

	OgComputePipelineVK* p = new OgComputePipelineVK(descriptor);

	buildComputePipeline(p);

#if defined(_DEBUG)
	p->instanceType = "ComputePipeline";
	_livingObjects.Add(p);
#endif

	return p;
}

OG_NAMESPACE_RENDER_END
