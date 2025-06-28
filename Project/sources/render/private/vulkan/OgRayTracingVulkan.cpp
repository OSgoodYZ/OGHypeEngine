#include "OgPrecompile.h"
#include "render/private/vulkan/OgRenderContext_Vulkan.h"
#include "render/private/vulkan/OgRayTracingHelper.h"
#include "render/private/vulkan/OgRenderVulkanHandles.h"

#ifdef OG_ENABLE_RAYTRACING

OG_NAMESPACE_RENDER_BEGIN

void OgRenderContextVulkan::initRayTracingSupport()
{
_rayTracingSupported = false;

// Check if vulkan device is valid
if (!_vulkanDevice)
{
LOGD(OG_ID, "Vulkan device is not initialized");
    return;
}

// Check if required extensions are available
if (!_vulkanDevice->HasExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) ||
    !_vulkanDevice->HasExtension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) ||
    !_vulkanDevice->HasExtension(VK_KHR_RAY_QUERY_EXTENSION_NAME))
{
    LOGD(OG_ID, "Ray tracing extensions not available");
    return;
}

// Get ray tracing properties
_rayTracingPipelineProperties = {};
_rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
_accelerationStructureFeatures = {};
_accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;

VkPhysicalDeviceProperties2 deviceProperties2{};
deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
deviceProperties2.pNext = &_rayTracingPipelineProperties;
vkGetPhysicalDeviceProperties2(_gpuDeviceVK, &deviceProperties2);

// Check if function pointers are loaded in device
if (!_vulkanDevice->vkCreateAccelerationStructureKHR ||
        !_vulkanDevice->vkDestroyAccelerationStructureKHR ||
        !_vulkanDevice->vkCmdTraceRaysKHR ||
        !_vulkanDevice->vkGetBufferDeviceAddressKHR ||
        !_vulkanDevice->vkGetAccelerationStructureBuildSizesKHR ||
        !_vulkanDevice->vkGetAccelerationStructureDeviceAddressKHR ||
        !_vulkanDevice->vkCmdBuildAccelerationStructuresKHR ||
        !_vulkanDevice->vkCreateRayTracingPipelinesKHR ||
        !_vulkanDevice->vkGetRayTracingShaderGroupHandlesKHR)
    {
        LOGD(OG_ID, "Ray tracing function pointers not loaded");
        return;
    }
    
    _rayTracingSupported = true;
    LOGD(OG_ID, "Ray tracing support initialized");
}

bool OgRenderContextVulkan::IsRayTracingSupported()
{
	return _rayTracingSupported;
}

OgAccelStructureHandle* OgRenderContextVulkan::CreateAccelerationStructure(const OgAccelStructureBuildInfo& buildInfo)
{
OG_CHECK(_rayTracingSupported, "Ray tracing is not supported");
OG_CHECK(_vulkanDevice != nullptr, "Vulkan device is null");
OG_CHECK(_vulkanDevice->vkGetBufferDeviceAddressKHR != nullptr, "vkGetBufferDeviceAddressKHR is null");
    
    OgAccelStructureVK* accelStructure = new OgAccelStructureVK(*_vulkanDevice);
	accelStructure->type = buildInfo.type;
	
	// Determine the required size for the acceleration structure
	VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
	buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	buildGeometryInfo.type = buildInfo.type == OgAccelStructureType::TOP_LEVEL ? 
		VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR : VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	buildGeometryInfo.flags = 0;
	
	if ((bool)(buildInfo.flags & OgRayTracingBuildFlag::PREFER_FAST_TRACE))
		buildGeometryInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	if ((bool)(buildInfo.flags & OgRayTracingBuildFlag::PREFER_FAST_BUILD))
		buildGeometryInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
	if ((bool)(buildInfo.flags & OgRayTracingBuildFlag::ALLOW_UPDATE))
		buildGeometryInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
	if ((bool)(buildInfo.flags & OgRayTracingBuildFlag::ALLOW_COMPACTION))
		buildGeometryInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
	
	uint32_t primitiveCount = 0;
	OgVector<VkAccelerationStructureGeometryKHR> geometries;
	OgVector<VkAccelerationStructureBuildRangeInfoKHR> buildRangeInfos;
	
	if (buildInfo.type == OgAccelStructureType::BOTTOM_LEVEL)
	{
		// Build bottom level acceleration structure
		geometries.Resize(buildInfo.bottomLevel.geometryCount);
		buildRangeInfos.Resize(buildInfo.bottomLevel.geometryCount);
		
		for (uint32 i = 0; i < buildInfo.bottomLevel.geometryCount; ++i)
		{
			const OgAccelStructureGeometry& geom = buildInfo.bottomLevel.geometries[i];
			VkAccelerationStructureGeometryKHR& vkGeom = geometries[i];
			vkGeom.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			vkGeom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR; // For now, only triangles
			vkGeom.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
			
			if (geom.vertexBuffer)
			{
				OgBufferVK* vertexBuffer = (OgBufferVK*)geom.vertexBuffer;
				VkBufferDeviceAddressInfo addressInfo{};
				addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
				addressInfo.buffer = vertexBuffer->bufferVK;
				vkGeom.geometry.triangles.vertexData.deviceAddress = _vulkanDevice->vkGetBufferDeviceAddressKHR(_logicalDeviceVK, &addressInfo);
				vkGeom.geometry.triangles.vertexStride = geom.vertexStride;
				vkGeom.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT; // Assuming float3
				vkGeom.geometry.triangles.maxVertex = geom.vertexCount - 1;
			}
			
			if (geom.indexBuffer)
			{
				OgBufferVK* indexBuffer = (OgBufferVK*)geom.indexBuffer;
				VkBufferDeviceAddressInfo addressInfo{};
				addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
				addressInfo.buffer = indexBuffer->bufferVK;
				vkGeom.geometry.triangles.indexData.deviceAddress = _vulkanDevice->vkGetBufferDeviceAddressKHR(_logicalDeviceVK, &addressInfo);
				vkGeom.geometry.triangles.indexType = geom.indexType == OgIndexType::UINT16 ? 
					VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
			}
			
			buildRangeInfos[i].primitiveCount = geom.indexCount / 3;
			buildRangeInfos[i].primitiveOffset = 0;
			buildRangeInfos[i].firstVertex = 0;
			buildRangeInfos[i].transformOffset = geom.transformOffset;
			
			primitiveCount += buildRangeInfos[i].primitiveCount;
		}
	}
	else
	{
		// Build top level acceleration structure
		geometries.Resize(1);
		buildRangeInfos.Resize(1);
		
		VkAccelerationStructureGeometryKHR& vkGeom = geometries[0];
		vkGeom.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		vkGeom.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		vkGeom.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		
		if (buildInfo.topLevel.instanceBuffer)
		{
			OgBufferVK* instanceBuffer = (OgBufferVK*)buildInfo.topLevel.instanceBuffer;
			VkBufferDeviceAddressInfo addressInfo{};
			addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
			addressInfo.buffer = instanceBuffer->bufferVK;
			vkGeom.geometry.instances.data.deviceAddress = _vulkanDevice->vkGetBufferDeviceAddressKHR(_logicalDeviceVK, &addressInfo);
		}
		
		buildRangeInfos[0].primitiveCount = buildInfo.topLevel.instanceCount;
		buildRangeInfos[0].primitiveOffset = 0;
		buildRangeInfos[0].firstVertex = 0;
		buildRangeInfos[0].transformOffset = 0;
		
		primitiveCount = buildInfo.topLevel.instanceCount;
	}
	
	buildGeometryInfo.geometryCount = static_cast<uint32_t>(geometries.Size());
	buildGeometryInfo.pGeometries = geometries.Data();
	
	// Get required sizes
	VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
	sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	_vulkanDevice->vkGetAccelerationStructureBuildSizesKHR(_logicalDeviceVK, 
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&buildGeometryInfo, &primitiveCount, &sizeInfo);
	
	// Create buffer for acceleration structure
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeInfo.accelerationStructureSize;
	bufferInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	VK_CHECK_RESULT(vkCreateBuffer(_logicalDeviceVK, &bufferInfo, nullptr, &accelStructure->buffer));
	
	// Allocate memory
	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(_logicalDeviceVK, accelStructure->buffer, &memReqs);
	
	VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
	memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
	memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
	
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.pNext = &memoryAllocateFlagsInfo;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = _vulkanDevice->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(_logicalDeviceVK, &allocInfo, nullptr, &accelStructure->memory));
	VK_CHECK_RESULT(vkBindBufferMemory(_logicalDeviceVK, accelStructure->buffer, accelStructure->memory, 0));
	
	// Create acceleration structure
	VkAccelerationStructureCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	createInfo.buffer = accelStructure->buffer;
	createInfo.size = sizeInfo.accelerationStructureSize;
	createInfo.type = buildInfo.type == OgAccelStructureType::TOP_LEVEL ? 
		VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR : VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	VK_CHECK_RESULT(_vulkanDevice->vkCreateAccelerationStructureKHR(_logicalDeviceVK, &createInfo, nullptr, &accelStructure->accelStructure));
	
	// Get device address
	VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
	addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	addressInfo.accelerationStructure = accelStructure->accelStructure;
	accelStructure->deviceAddress = _vulkanDevice->vkGetAccelerationStructureDeviceAddressKHR(_logicalDeviceVK, &addressInfo);
	accelStructure->OgAccelStructureHandle::deviceAddress = accelStructure->deviceAddress;
	
#if defined(_DEBUG)
	_livingObjects.Add(accelStructure);
#endif
	
	return accelStructure;
}

void OgRenderContextVulkan::DestroyAccelerationStructure(OgAccelStructureHandle* accelStructure)
{
	OG_CHECK(accelStructure != nullptr, "Acceleration structure is null");
	OgAccelStructureVK* accelVK = static_cast<OgAccelStructureVK*>(accelStructure);
	
#if defined(_DEBUG)
	_livingObjects.Remove(accelStructure);
#endif
	
	delete accelVK;
}

void OgRenderContextVulkan::BuildAccelerationStructure(OgCommandEncoderHandle* encoder, 
	OgAccelStructureHandle* accelStructure, const OgAccelStructureBuildInfo& buildInfo)
{
	OG_CHECK(_rayTracingSupported, "Ray tracing is not supported");
	OG_CHECK(encoder != nullptr, "Command encoder is null");
	OG_CHECK(accelStructure != nullptr, "Acceleration structure is null");
	
	OgCommandEncoderVK* encoderVK = static_cast<OgCommandEncoderVK*>(encoder);
	OgAccelStructureVK* accelVK = static_cast<OgAccelStructureVK*>(accelStructure);
	
	// Create build geometry info
	VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
	buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	buildGeometryInfo.type = buildInfo.type == OgAccelStructureType::TOP_LEVEL ? 
		VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR : VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	buildGeometryInfo.flags = 0;
	buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	buildGeometryInfo.dstAccelerationStructure = accelVK->accelStructure;
	
	OgVector<VkAccelerationStructureGeometryKHR> geometries;
	OgVector<VkAccelerationStructureBuildRangeInfoKHR> buildRangeInfos;
	
	// Set up geometries (similar to CreateAccelerationStructure)
	if (buildInfo.type == OgAccelStructureType::BOTTOM_LEVEL)
	{
		geometries.Resize(buildInfo.bottomLevel.geometryCount);
		buildRangeInfos.Resize(buildInfo.bottomLevel.geometryCount);
		
		for (uint32 i = 0; i < buildInfo.bottomLevel.geometryCount; ++i)
		{
			const OgAccelStructureGeometry& geom = buildInfo.bottomLevel.geometries[i];
			VkAccelerationStructureGeometryKHR& vkGeom = geometries[i];
			vkGeom.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			vkGeom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
			vkGeom.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
			
			if (geom.vertexBuffer)
			{
				OgBufferVK* vertexBuffer = (OgBufferVK*)geom.vertexBuffer;
				VkBufferDeviceAddressInfo addressInfo{};
				addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
				addressInfo.buffer = vertexBuffer->bufferVK;
				vkGeom.geometry.triangles.vertexData.deviceAddress = _vulkanDevice->vkGetBufferDeviceAddressKHR(_logicalDeviceVK, &addressInfo);
				vkGeom.geometry.triangles.vertexStride = geom.vertexStride;
				vkGeom.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
				vkGeom.geometry.triangles.maxVertex = geom.vertexCount - 1;
			}
			
			if (geom.indexBuffer)
			{
				OgBufferVK* indexBuffer = (OgBufferVK*)geom.indexBuffer;
				VkBufferDeviceAddressInfo addressInfo{};
				addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
				addressInfo.buffer = indexBuffer->bufferVK;
				vkGeom.geometry.triangles.indexData.deviceAddress = _vulkanDevice->vkGetBufferDeviceAddressKHR(_logicalDeviceVK, &addressInfo);
				vkGeom.geometry.triangles.indexType = geom.indexType == OgIndexType::UINT16 ? 
					VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
			}
			
			buildRangeInfos[i].primitiveCount = geom.indexCount / 3;
			buildRangeInfos[i].primitiveOffset = 0;
			buildRangeInfos[i].firstVertex = 0;
			buildRangeInfos[i].transformOffset = geom.transformOffset;
		}
	}
	else
	{
		geometries.Resize(1);
		buildRangeInfos.Resize(1);
		
		VkAccelerationStructureGeometryKHR& vkGeom = geometries[0];
		vkGeom.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		vkGeom.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		vkGeom.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		
		if (buildInfo.topLevel.instanceBuffer)
		{
			OgBufferVK* instanceBuffer = (OgBufferVK*)buildInfo.topLevel.instanceBuffer;
			VkBufferDeviceAddressInfo addressInfo{};
			addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
			addressInfo.buffer = instanceBuffer->bufferVK;
			vkGeom.geometry.instances.data.deviceAddress = _vulkanDevice->vkGetBufferDeviceAddressKHR(_logicalDeviceVK, &addressInfo);
		}
		
		buildRangeInfos[0].primitiveCount = buildInfo.topLevel.instanceCount;
		buildRangeInfos[0].primitiveOffset = 0;
		buildRangeInfos[0].firstVertex = 0;
		buildRangeInfos[0].transformOffset = 0;
	}
	
	buildGeometryInfo.geometryCount = static_cast<uint32_t>(geometries.Size());
	buildGeometryInfo.pGeometries = geometries.Data();
	
	// Build on device
	const VkAccelerationStructureBuildRangeInfoKHR* pBuildRangeInfos = buildRangeInfos.Data();
	_vulkanDevice->vkCmdBuildAccelerationStructuresKHR(encoderVK->cmdBufferVK, 1, &buildGeometryInfo, &pBuildRangeInfos);
}

OgPipelineHandle* OgRenderContextVulkan::CreateRayTracingPipeline(const OgRayTracingPipelineDescriptor& descriptor)
{
	OG_CHECK(_rayTracingSupported, "Ray tracing is not supported");
	
	OgRayTracingPipelineVK* pipeline = new OgRayTracingPipelineVK(descriptor);
	
	// Create pipeline layout
	OgResourceLayoutVK* resourceLayout = static_cast<OgResourceLayoutVK*>(descriptor.resourceLayout);
	
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &resourceLayout->descriptorSetLayoutVK;
	VK_CHECK_RESULT(vkCreatePipelineLayout(_logicalDeviceVK, &pipelineLayoutInfo, nullptr, &pipeline->pipelineLayout));
	
	// Set up shader stages
	OgVector<VkPipelineShaderStageCreateInfo> shaderStages;
	shaderStages.Resize(descriptor.shaderCount);
	
	for (uint32 i = 0; i < descriptor.shaderCount; ++i)
	{
		OgShaderVK* shader = static_cast<OgShaderVK*>(descriptor.shaders[i]);
		shaderStages[i] = shader->shaderStageInfo;
	}
	
	// Set up shader groups
	OgVector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;
	shaderGroups.Resize(descriptor.shaderGroupCount);
	
	for (uint32 i = 0; i < descriptor.shaderGroupCount; ++i)
	{
		const OgRayTracingShaderGroup& group = descriptor.shaderGroups[i];
		VkRayTracingShaderGroupCreateInfoKHR& vkGroup = shaderGroups[i];
		vkGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		
		switch (group.type)
		{
		case OgRayTracingShaderGroup::GENERAL:
			vkGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			vkGroup.generalShader = group.generalShader;
			vkGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
			vkGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
			vkGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
			break;
		case OgRayTracingShaderGroup::TRIANGLES_HIT_GROUP:
			vkGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
			vkGroup.generalShader = VK_SHADER_UNUSED_KHR;
			vkGroup.closestHitShader = group.closestHitShader;
			vkGroup.anyHitShader = group.anyHitShader;
			vkGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
			break;
		case OgRayTracingShaderGroup::PROCEDURAL_HIT_GROUP:
			vkGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
			vkGroup.generalShader = VK_SHADER_UNUSED_KHR;
			vkGroup.closestHitShader = group.closestHitShader;
			vkGroup.anyHitShader = group.anyHitShader;
			vkGroup.intersectionShader = group.intersectionShader;
			break;
		}
	}
	
	// Create ray tracing pipeline
	VkRayTracingPipelineCreateInfoKHR pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.Size());
	pipelineInfo.pStages = shaderStages.Data();
	pipelineInfo.groupCount = static_cast<uint32_t>(shaderGroups.Size());
	pipelineInfo.pGroups = shaderGroups.Data();
	pipelineInfo.maxPipelineRayRecursionDepth = descriptor.maxRecursionDepth;
	pipelineInfo.layout = pipeline->pipelineLayout;
	
	VK_CHECK_RESULT(_vulkanDevice->vkCreateRayTracingPipelinesKHR(_logicalDeviceVK, VK_NULL_HANDLE, VK_NULL_HANDLE, 
		1, &pipelineInfo, nullptr, &pipeline->pipeline));
	
#if defined(_DEBUG)
	_livingObjects.Add(pipeline);
#endif
	
	return pipeline;
}

OgBufferHandle* OgRenderContextVulkan::CreateShaderBindingTable(OgPipelineHandle* pipeline, 
	const OgRayTracingShaderGroup* groups, uint32 groupCount)
{
	OG_CHECK(_rayTracingSupported, "Ray tracing is not supported");
	OG_CHECK(pipeline != nullptr, "Pipeline is null");
	OG_CHECK(pipeline->type == OgPipelineType::RAYTRACING_PIPELINE, "Pipeline is not a ray tracing pipeline");
	
	OgRayTracingPipelineVK* rtPipeline = static_cast<OgRayTracingPipelineVK*>(pipeline);
	
	// Calculate SBT size
	uint32 handleSize = _rayTracingPipelineProperties.shaderGroupHandleSize;
	uint32 handleSizeAligned = AlignUp(handleSize, _rayTracingPipelineProperties.shaderGroupHandleAlignment);
	uint32 sbtSize = groupCount * handleSizeAligned;
	
	// Get shader group handles
	OgVector<uint8> shaderHandleStorage(sbtSize);
	VK_CHECK_RESULT(_vulkanDevice->vkGetRayTracingShaderGroupHandlesKHR(_logicalDeviceVK, rtPipeline->pipeline, 
		0, groupCount, sbtSize, shaderHandleStorage.Data()));
	
	// Create SBT buffer
	OgBufferHandle* sbtBuffer = CreateBuffer(shaderHandleStorage.Data(), sbtSize, 
		static_cast<OgBufferUsage>(static_cast<uint8>(OgBufferUsage::STORAGE) | static_cast<uint8>(OgBufferUsage::SHADER_DEVICE_ADDRESS)), 
		OgMemoryOption::PRIVATE_GPU);
	
	return sbtBuffer;
}

OG_NAMESPACE_RENDER_END

#else // OG_ENABLE_RAYTRACING

OG_NAMESPACE_RENDER_BEGIN

void OgRenderContextVulkan::initRayTracingSupport()
{
	_rayTracingSupported = false;
	LOGI(OG_ID, "Ray tracing disabled in build");
}

bool OgRenderContextVulkan::IsRayTracingSupported()
{
	return false;
}

OgAccelStructureHandle* OgRenderContextVulkan::CreateAccelerationStructure(const OgAccelStructureBuildInfo& buildInfo)
{
	LOGE(OG_ID, "Ray tracing is not enabled in this build");
	return nullptr;
}

void OgRenderContextVulkan::DestroyAccelerationStructure(OgAccelStructureHandle* accelStructure)
{
	LOGE(OG_ID, "Ray tracing is not enabled in this build");
}

void OgRenderContextVulkan::BuildAccelerationStructure(OgCommandEncoderHandle* encoder, 
	OgAccelStructureHandle* accelStructure, const OgAccelStructureBuildInfo& buildInfo)
{
	LOGE(OG_ID, "Ray tracing is not enabled in this build");
}

OgPipelineHandle* OgRenderContextVulkan::CreateRayTracingPipeline(const OgRayTracingPipelineDescriptor& descriptor)
{
	LOGE(OG_ID, "Ray tracing is not enabled in this build");
	return nullptr;
}

OgBufferHandle* OgRenderContextVulkan::CreateShaderBindingTable(OgPipelineHandle* pipeline, 
	const OgRayTracingShaderGroup* groups, uint32 groupCount)
{
	LOGE(OG_ID, "Ray tracing is not enabled in this build");
	return nullptr;
}

OG_NAMESPACE_RENDER_END

#endif // OG_ENABLE_RAYTRACING
