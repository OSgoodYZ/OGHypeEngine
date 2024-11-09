#include "OgPrecompile.h"

#include"OgRenderVulkanHandles.h"

#include "OgVulkanHelper.h"


OG_NAMESPACE_RENDER_BEGIN

#define NUM_FRAMES_TO_WAIT_BEFORE_RELEASING_TO_OS 3

OgCommandEncoderVK::OgCommandEncoderVK(OgDeviceVulkan* device, VkCommandPool cmdPool)
	: OgCommandEncoderHandle()
	, vulkanDevice(device)
	, cmdPoolVK(cmdPool)
	, curBindRenderPass(nullptr)
	, curBindFramebuffer(nullptr)
	, curBindPipeline(nullptr)
{
	VkCommandBufferAllocateInfo cmdBufAllocateInfo{};

	cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufAllocateInfo.commandPool = cmdPool;
	cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufAllocateInfo.commandBufferCount = 1;

	VK_CHECK_RESULT(vkAllocateCommandBuffers(vulkanDevice->logicalDevice, &cmdBufAllocateInfo, &cmdBufferVK));
}

OgCommandEncoderVK::~OgCommandEncoderVK()
{
	vkFreeCommandBuffers(vulkanDevice->logicalDevice, cmdPoolVK, 1, &cmdBufferVK);
}

void OgCommandEncoderVK::Begin()
{
	VkCommandBufferBeginInfo cmdBufInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	// https://software.intel.com/en-us/documentation/graphics-api-performance-guide-for-intel-processor-graphics-gen9/vulkan-performance-tips
	// According to the doc, Avoid : VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
	//cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBufferVK, &cmdBufInfo));
}

void OgCommandEncoderVK::BeginRenderPass
(
	const OgRenderPassHandle* renderPass,
	const OgFrameBufferHandle* frameBuffer,
	const Area area,
	const uint8 colorAttachClearCount,
	const ClearValue* colorAttachmentClear,
	const uint8 resoOGeAttachClearCount,
	const ClearValue* resoOGeAttachmentClear,
	const ClearValue* depthAttachmentClear
)
{
	OG_CHECK(renderPass != nullptr, "RenderPass is nullptr");
	OG_CHECK(frameBuffer != nullptr, "Framebuffer is nullptr");

	curBindRenderPass = (OgRenderPassVK*)(renderPass);
	const OgRenderPassInfo rpInfo = curBindRenderPass->info;

	curBindFramebuffer = (OgFrameBufferHandle*)(frameBuffer);
	VkFramebuffer framebuffer = curBindFramebuffer->isSwapchainFrameBuffer ?
		reinterpret_cast<OgDefaultFrameBufferVK*>(curBindFramebuffer)->frameBufferVK :
		reinterpret_cast<OgFrameBufferVK*>(curBindFramebuffer)->frameBufferVK;

	VkRenderPassBeginInfo beginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	beginInfo.renderPass = curBindRenderPass->renderPassVK;
	beginInfo.framebuffer = framebuffer;
	beginInfo.pNext = nullptr;
	beginInfo.renderArea.offset.x = area.x;
	beginInfo.renderArea.offset.y = area.y;
	beginInfo.renderArea.extent.width = area.width;
	beginInfo.renderArea.extent.height = area.height;

	uint8 attachmentLen = rpInfo.outputColorAttachmentCount;
	if (rpInfo.useDepthStencilAttachment == true)
		attachmentLen += 1;

	OgVector<VkClearValue> clearValues;
	clearValues.Resize(attachmentLen);

	OG_CHECK(colorAttachClearCount == rpInfo.outputColorAttachmentCount,
		"The number of Command Color Attachment (%d) is different from RenderPass Info Color Attachment Count (%d)",
		(int)colorAttachClearCount, (int)rpInfo.outputColorAttachmentCount);

	uint8 attachmentIndex = 0;
	for (; attachmentIndex < colorAttachClearCount; ++attachmentIndex)
	{
		OgCommandEncoderHandle::ClearValue cv = colorAttachmentClear[attachmentIndex];

		const float* color = cv.color.value;

		clearValues[attachmentIndex].color = { color[0], color[1], color[2], color[3] };
	}


	bool cmdUseDepthStencil = depthAttachmentClear != nullptr;
	OG_CHECK(cmdUseDepthStencil == rpInfo.useDepthStencilAttachment,
		"The usage of Command Depth Stencil (%d) is different from RenderPass Info Usage of Depth Stencil (%d)",
		(int)cmdUseDepthStencil, (int)rpInfo.useDepthStencilAttachment);
	if (cmdUseDepthStencil == true)
	{
		OgCommandEncoderHandle::ClearValue cv = *depthAttachmentClear;

		clearValues[attachmentIndex].depthStencil =
		{
			(float)cv.depthStencil.depth,
			(uint32)cv.depthStencil.stencil
		};
	}

	beginInfo.clearValueCount = static_cast<uint32_t>(clearValues.Size());
	beginInfo.pClearValues = clearValues.Data();

	OgTextureVK* colorTex = (OgTextureVK*)curBindFramebuffer->framebufferInfo.colorBuffers[0];


	if (colorTex != nullptr)
	{
		//vkSetImageLayout(cmdBufferVK
		//	, colorTex->image
		//	, VK_IMAGE_LAYOUT_UNDEFINED
		//	, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		//	, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		//	, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
		//	, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	}

	vkCmdBeginRenderPass(cmdBufferVK, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void OgCommandEncoderVK::SetViewport
(
	const float x, const float y,
	const float width, const float height,
	const float minDepth , const float maxDepth
)
{
	OG_CHECK(curBindRenderPass != nullptr && curBindFramebuffer != nullptr, "%s encording has to use after BeginRenderpass encoding.", og_get_command_type_string(OgCommandType::SET_VIEWPORT));

	// https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
	// http://anki3d.org/vulkan-coordinate-system/
	// https://github.com/g-truc/glm/commit/f48fe286ad88f9ffd5c5e9f0d95a6cd1107ac40b

	VkViewport viewport{};

	viewport.x = x;
	viewport.y = y;
	viewport.width = width;
	viewport.height = height;

	viewport.minDepth = minDepth;
	viewport.maxDepth = maxDepth;

	vkCmdSetViewport(cmdBufferVK, 0, 1, &viewport);
}

void OgCommandEncoderVK::SetScissor(const int32 x, const int32 y, const uint32 width, const uint32 height)
{
	OG_CHECK(curBindRenderPass != nullptr && curBindFramebuffer != nullptr, "%s encording has to use after BeginRenderpass encoding.", og_get_command_type_string(OgCommandType::SET_SCISSOR));

	VkRect2D scissor{};

	scissor.offset.x = x;
	scissor.offset.y = y;
	scissor.extent.width = width;
	scissor.extent.height = height;

	vkCmdSetScissor(cmdBufferVK, 0, 1, &scissor);
}

void OgCommandEncoderVK::BindPipeline(const OgPipelineHandle* pipeline)
{
	OG_CHECK(curBindRenderPass != nullptr && curBindFramebuffer != nullptr, "%s encording has to use after BeginRenderpass encoding.", og_get_command_type_string(OgCommandType::BIND_PIPELINE));
	OG_CHECK(pipeline != nullptr, "Pipeline is nullptr");

	curBindPipeline = (OgGraphicsPipelineVK*)(pipeline);

	//const char* name = curBindPipeline->name;
	//if (name == nullptr)
	//	name = "Render Pipeline";

	//vulkanDevice->BeginRegion(cmdBufferVK, name, System::Math::LvVec4f(1.0f, 0.0f, 0.0f, 1.0f));
	vkCmdBindPipeline(cmdBufferVK, VK_PIPELINE_BIND_POINT_GRAPHICS, curBindPipeline->pipeline);
}

void OgCommandEncoderVK::BindResourceSet(const OgResourceSetHandle* rSet)
{
	OG_CHECK(curBindRenderPass != nullptr && curBindFramebuffer != nullptr, "%s encording has to use after BeginRenderpass encoding.", og_get_command_type_string(OgCommandType::BIND_RESOURCESET));
	OG_CHECK(curBindPipeline != nullptr, "There is no pipeline to be bound for resource set");
	OG_CHECK(rSet != nullptr, "ResourceSet is nullptr");

	OgResourceSetVK* rSetVK = (OgResourceSetVK*)rSet;

	OG_CHECK(IsCompatible(rSetVK->resourceLayoutVK, curBindPipeline->resourceLayout), "Invalid Compatible ResourceSet With Pipeline Layout");

	vkCmdBindDescriptorSets(cmdBufferVK, VK_PIPELINE_BIND_POINT_GRAPHICS, curBindPipeline->pipelineLayout, 0, 1, &rSetVK->descriptorSetVK, 0, NULL);
}

void OgCommandEncoderVK::BindVertexBuffers(const OgBufferHandle* const * vertexBuffers, const  uint32* offsets, const  uint8 bufferCount)
{
	OG_CHECK(curBindRenderPass != nullptr && curBindFramebuffer != nullptr, "%s encording has to use after BeginRenderpass encoding.", og_get_command_type_string(OgCommandType::BIND_VERTEX_BUFFERS));
	OG_CHECK(vertexBuffers != nullptr, "VertexBuffer Array is nullptr");
	OG_CHECK(bufferCount != 0, "Buffer Count is zero");

	OgVector<VkBuffer> buffers;
	buffers.Resize(8);
	VkDeviceSize bufferOffsets[8] = { 0 };

	for (uint8 i = 0; i < bufferCount; ++i)
	{
		if (vertexBuffers[i] == nullptr)
		{
			continue;
		}
		OgBufferVK* bvk = (OgBufferVK*)(vertexBuffers[i]);
		buffers.Add(bvk->bufferVK);

		bufferOffsets[i] = bvk->innerOffset;
		if (offsets != nullptr)
			bufferOffsets[i] += offsets[i];

		vkCmdBindVertexBuffers(cmdBufferVK, i, 1, &bvk->bufferVK, &bufferOffsets[i]);
	}
	//vkCmdBindVertexBuffers(cmdBufferVK, 0, static_cast<uint32_t>(buffers.Count()), buffers.data(), bufferOffsets);
}

void OgCommandEncoderVK::BindIndexBuffer(const OgBufferHandle* indexBuffer, const OgIndexType indexType)
{
	OG_CHECK(curBindRenderPass != nullptr && curBindFramebuffer != nullptr, "%s encording has to use after BeginRenderpass encoding.", og_get_command_type_string(OgCommandType::BIND_INDEX_BUFFER));
	OG_CHECK(curBindPipeline != nullptr, "%s encording has to use after BindPipeline encoding.", og_get_command_type_string(OgCommandType::BIND_INDEX_BUFFER));
	OG_CHECK(indexBuffer != nullptr, "indexBuffer is nullptr");

	VkDeviceSize offsets[1] = { 0 };
	OgBufferVK* ibo = (OgBufferVK*)(indexBuffer);
	offsets[0] = ibo->innerOffset;

	curBindIndexBufferType = VK_INDEX_TYPE_UINT32;
	if (indexType == OgIndexType::UInt16)
		curBindIndexBufferType = VK_INDEX_TYPE_UINT16;

	vkCmdBindIndexBuffer(cmdBufferVK, ibo->bufferVK, *offsets, curBindIndexBufferType);
}

void OgCommandEncoderVK::DrawIndexed(const uint32 firstIndex, const uint32 indexCount, const uint32 instanceCount, const uint32 vertexOffset)
{
	OG_CHECK(curBindPipeline != nullptr, "%s encording has to use after BindPipeline encoding.", og_get_command_type_string(OgCommandType::DRAW_INDEXED));
	vkCmdDrawIndexed(cmdBufferVK, indexCount, instanceCount,
		curBindIndexBufferType == VK_INDEX_TYPE_UINT32 ? firstIndex / sizeof(uint32) : firstIndex / sizeof(uint16),
		vertexOffset, 0);
}

void OgCommandEncoderVK::DrawArrays(const uint32 firstVertex, const uint32 vertexCount, const uint32 instanceCount)
{
	OG_CHECK(curBindPipeline != nullptr, "%s encording has to use after BindPipeline encoding.", og_get_command_type_string(OgCommandType::BIND_VERTEX_BUFFERS));
	vkCmdDraw(cmdBufferVK, vertexCount, instanceCount, firstVertex, 0);
}

void OgCommandEncoderVK::EndRenderPass()
{
	OG_CHECK(curBindRenderPass != nullptr && curBindFramebuffer != nullptr, "Can't End RenderPass without Begin RenderPass");
	curBindRenderPass = nullptr;
	curBindFramebuffer = nullptr;
	curBindPipeline = nullptr;

	vkCmdEndRenderPass(cmdBufferVK);
}

void OgCommandEncoderVK::BeginDebugMarker(const char* label, float color[4])
{
	glm::vec4 v(color[0], color[1], color[2], color[3]);
	vulkanDevice->BeginRegion(cmdBufferVK, label, v);
}


void OgCommandEncoderVK::EndDebugMarker()
{
	vulkanDevice->EndRegion(cmdBufferVK);
}

void OgCommandEncoderVK::End()
{
	curBindRenderPass = nullptr;
	curBindFramebuffer = nullptr;
	curBindPipeline = nullptr;

	vulkanDevice->EndRegion(cmdBufferVK);
	VK_CHECK_RESULT(vkEndCommandBuffer(cmdBufferVK));
}

OG_NAMESPACE_RENDER_END