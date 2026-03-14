#include "OgPrecompile.h"
#include "OgDeviceVulkan.h"

OG_NAMESPACE_RENDER_BEGIN

OgDeviceVulkan::OgDeviceVulkan(VkPhysicalDevice physicalDevice)
{
	OG_CHECK(physicalDevice, "%s", "Wrong Physical Device Object");
	this->physicalDevice = physicalDevice;

	// Store Properties features, limits and properties of the physical device for later use
	// Device properties also contain limits and sparse properties
	vkGetPhysicalDeviceProperties(physicalDevice, &properties);
	// Features should be checked by the examples before using them
	vkGetPhysicalDeviceFeatures(physicalDevice, &features);
	// Memory properties are used regularly for creating all kinds of buffers
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
	// Queue family properties, used for setting up requested queues upon device creation
	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	OG_CHECK(queueFamilyCount > 0, "%s", "Wrong Queue Family Count");
	queueFamilyProperties.resize(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

	// Get list of supported extensions
	uint32_t extCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
	if (extCount > 0)
	{
		vector<VkExtensionProperties> extensions;
		extensions.resize(extCount);
		if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, extensions.data()) == VK_SUCCESS)
		{
			for (int i = 0; i < extensions.size(); ++i)
			{
				char* name = new char[strlen(extensions[i].extensionName) + 1];
				sprintf(name, "%s", extensions[i].extensionName);
				supportedExtensions.push_back(name);
			}
		}
	}
}

OgDeviceVulkan::~OgDeviceVulkan()
{
	for (int i = 0; i < supportedExtensions.size(); ++i)
	{
		delete[] supportedExtensions[i];
	}

	supportedExtensions.clear();

	if (commandPool)
	{
		vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
	}
	if (logicalDevice)
	{
		vkDestroyDevice(logicalDevice, nullptr);
	}
}

uint32_t OgDeviceVulkan::GetMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32* memTypeFound)
{
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if ((typeBits & 1) == 1)
		{
			if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				if (memTypeFound)
				{
					*memTypeFound = true;
				}
				return i;
			}
		}
		typeBits >>= 1;
	}

	if (memTypeFound)
	{
		*memTypeFound = false;
		return 0;
	}
	else
	{
		//throw std::runtime_error("Could not find a matching memory type");
		LOGE(OG_ID, "Could not find a matching memory type");
	}

	return 0;
}

uint32_t OgDeviceVulkan::GetQueueFamilyIndex(VkQueueFlagBits queueFlags)
{
	// Dedicated queue for compute
		// Try to find a queue family index that supports compute but not graphics
	if (queueFlags & VK_QUEUE_COMPUTE_BIT)
	{
		for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
		{
			if ((queueFamilyProperties[i].queueFlags & queueFlags) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
			{
				return i;
				break;
			}
		}
	}

	// Dedicated queue for transfer
	// Try to find a queue family index that supports transfer but not graphics and compute
	if (queueFlags & VK_QUEUE_TRANSFER_BIT)
	{
		for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
		{
			if ((queueFamilyProperties[i].queueFlags & queueFlags) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
			{
				return i;
				break;
			}
		}
	}

	// For other queue types or if no separate compute queue is present, return the first one to support the requested flags
	for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
	{
		if (queueFamilyProperties[i].queueFlags & queueFlags)
		{
			return i;
			break;
		}
	}

	LOGE(OG_ID, "Could not find a matching queue family index");
	return 0;
}

VkCommandPool OgDeviceVulkan::CreateCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags)
{
	VkCommandPoolCreateInfo cmdPoolInfo = {};
	cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
	cmdPoolInfo.flags = createFlags;
	VkCommandPool cmdPool;
	VkResult result = vkCreateCommandPool(logicalDevice, &cmdPoolInfo, nullptr, &cmdPool);
	if (result != VK_SUCCESS)
	{
		LOGE(OG_ID, "Could not create commandPool  %d\n", result);
	}
	return cmdPool;
}

bool OgDeviceVulkan::HasExtension(const char* extension)
{
	if (supportedExtensions.size() == 0)
		return false;


	bool returnValue = false;
	for (size_t i = 0; i < supportedExtensions.size(); ++i)
	{
		if (strcmp(extension, supportedExtensions[i]) == 0)
		{
			returnValue = true;
			break;
		}
	}
		
	return returnValue;
}

VkResult OgDeviceVulkan::CreateLogicalDevice(VkPhysicalDeviceFeatures enabledFeatures, const vector<const char*>& enabledExtensions, bool useSwapChain, VkQueueFlags requestedQueueTypes)
{
	// Desired queues need to be requested upon logical device creation
		// Due to differing queue family configurations of Vulkan implementations this can be a bit tricky, especially if the application
		// requests different queue types

	vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

	// Get queue family indices for the requested queue family types
	// Note that the indices may overlap depending on the implementation

	const float defaultQueuePriority(0.0f);

	// Graphics queue
	if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
	{
		queueFamilyIndices.graphics = GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
		VkDeviceQueueCreateInfo queueInfo{};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = queueFamilyIndices.graphics;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &defaultQueuePriority;
		queueCreateInfos.push_back(queueInfo);
	}
	else
	{
		queueFamilyIndices.graphics = 0;
	}

	// Dedicated compute queue
	if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
	{
		queueFamilyIndices.compute = GetQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
		if (queueFamilyIndices.compute != queueFamilyIndices.graphics)
		{
			// If compute family index differs, we need an additional queue create info for the compute queue
			VkDeviceQueueCreateInfo queueInfo{};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = queueFamilyIndices.compute;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &defaultQueuePriority;
			queueCreateInfos.push_back(queueInfo);
		}
	}
	else
	{
		// Else we use the same queue
		queueFamilyIndices.compute = queueFamilyIndices.graphics;
	}

	// Dedicated transfer queue
	if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)
	{
		queueFamilyIndices.transfer = GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
		if ((queueFamilyIndices.transfer != queueFamilyIndices.graphics) && (queueFamilyIndices.transfer != queueFamilyIndices.compute))
		{
			// If compute family index differs, we need an additional queue create info for the compute queue
			VkDeviceQueueCreateInfo queueInfo{};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = queueFamilyIndices.transfer;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &defaultQueuePriority;
			queueCreateInfos.push_back(queueInfo);
		}
	}
	else
	{
		// Else we use the same queue
		queueFamilyIndices.transfer = queueFamilyIndices.graphics;
	}

	// Create the logical device representation
	vector<const char*> deviceExtensions(enabledExtensions);
	if (useSwapChain)
	{
		// If the device will be used for presenting to a display via a swapchain we need to request the swapchain extension
		deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	}

	// Add ray tracing extensions if available
	bool rayTracingSupported = false;
	if (HasExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) &&
		HasExtension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) &&
		HasExtension(VK_KHR_RAY_QUERY_EXTENSION_NAME) &&
		HasExtension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME) &&
		HasExtension(VK_KHR_SPIRV_1_4_EXTENSION_NAME) &&
		HasExtension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME) &&
		HasExtension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
	{
		deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
		deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
		deviceExtensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
		deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
		deviceExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
		deviceExtensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
		deviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
		rayTracingSupported = true;
	}

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	
	// Ray tracing features setup
	VkPhysicalDeviceFeatures2 deviceFeatures2{};
	deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures2.features = enabledFeatures;
	
	VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
	bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
	bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
	
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{};
	rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
	rayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
	
	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
	accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	accelerationStructureFeatures.accelerationStructure = VK_TRUE;
	
	VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{};
	rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
	rayQueryFeatures.rayQuery = VK_TRUE;
	
	if (rayTracingSupported)
	{
		deviceFeatures2.pNext = &bufferDeviceAddressFeatures;
		bufferDeviceAddressFeatures.pNext = &rayTracingPipelineFeatures;
		rayTracingPipelineFeatures.pNext = &accelerationStructureFeatures;
		accelerationStructureFeatures.pNext = &rayQueryFeatures;
		
		deviceCreateInfo.pNext = &deviceFeatures2;
		deviceCreateInfo.pEnabledFeatures = nullptr;
	}
	else
	{
		deviceCreateInfo.pEnabledFeatures = &enabledFeatures;
	}

	// Enable the debug marker extension if it is present (likely meaning a debugging tool is present)
	if (HasExtension(VK_EXT_DEBUG_MARKER_EXTENSION_NAME))
	{
		deviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
		enableDebugMarkers = true;
	}


	if (deviceExtensions.size() > 0)
	{
		deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
	}

	VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice);

	if (result == VK_SUCCESS)
	{
		// Create a default command pool for graphics command buffers
		commandPool = CreateCommandPool(queueFamilyIndices.graphics);
	}

	if (enableDebugMarkers)
	{
		vkDebugMarkerSetObjectTag = (PFN_vkDebugMarkerSetObjectTagEXT)vkGetDeviceProcAddr(logicalDevice, "vkDebugMarkerSetObjectTagEXT");
		vkDebugMarkerSetObjectName = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(logicalDevice, "vkDebugMarkerSetObjectNameEXT");
		vkCmdDebugMarkerBegin = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(logicalDevice, "vkCmdDebugMarkerBeginEXT");
		vkCmdDebugMarkerEnd = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(logicalDevice, "vkCmdDebugMarkerEndEXT");
		vkCmdDebugMarkerInsert = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(logicalDevice, "vkCmdDebugMarkerInsertEXT");
	}

	// Load ray tracing function pointers if ray tracing is supported
	if (rayTracingSupported)
	{
		vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(logicalDevice, "vkDestroyAccelerationStructureKHR");
		vkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(logicalDevice, "vkCmdTraceRaysKHR");
		vkGetBufferDeviceAddressKHR = (PFN_vkGetBufferDeviceAddressKHR)vkGetDeviceProcAddr(logicalDevice, "vkGetBufferDeviceAddressKHR");
		vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(logicalDevice, "vkCreateAccelerationStructureKHR");
		vkBuildAccelerationStructuresKHR = (PFN_vkBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(logicalDevice, "vkBuildAccelerationStructuresKHR");
		vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(logicalDevice, "vkCmdBuildAccelerationStructuresKHR");
		vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(logicalDevice, "vkGetAccelerationStructureBuildSizesKHR");
		vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(logicalDevice, "vkGetAccelerationStructureDeviceAddressKHR");
		vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(logicalDevice, "vkCreateRayTracingPipelinesKHR");
		vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetDeviceProcAddr(logicalDevice, "vkGetRayTracingShaderGroupHandlesKHR");
		vkGetRayTracingCaptureReplayShaderGroupHandlesKHR = (PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR)vkGetDeviceProcAddr(logicalDevice, "vkGetRayTracingCaptureReplayShaderGroupHandlesKHR");

		if (!vkCreateAccelerationStructureKHR ||
			!vkDestroyAccelerationStructureKHR ||
			!vkCmdTraceRaysKHR)
		{
			LOGE(OG_ID, "Could not load ray tracing function pointers");
			rayTracingSupported = false;
		}
	}

	this->enabledFeatures = enabledFeatures;

	return result;
}

void OgDeviceVulkan::SetObjectName(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, const char* name)
{
	// Check for valid function pointer (may not be present if not running in a debugging application)
	if (enableDebugMarkers)
	{
		VkDebugMarkerObjectNameInfoEXT nameInfo = {};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = objectType;
		nameInfo.object = object;
		nameInfo.pObjectName = name;
		vkDebugMarkerSetObjectName(device, &nameInfo);
	}
}

void OgDeviceVulkan::SetObjectTag(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, uint64_t name, size_t tagSize, const void* tag)
{
	// Check for valid function pointer (may not be present if not running in a debugging application)
	if (enableDebugMarkers)
	{
		VkDebugMarkerObjectTagInfoEXT tagInfo = {};
		tagInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT;
		tagInfo.objectType = objectType;
		tagInfo.object = object;
		tagInfo.tagName = name;
		tagInfo.tagSize = tagSize;
		tagInfo.pTag = tag;
		vkDebugMarkerSetObjectTag(device, &tagInfo);
	}
}

void OgDeviceVulkan::BeginRegion(VkCommandBuffer cmdbuffer, const char* pMarkerName, glm::vec4 color)
{
	// Check for valid function pointer (may not be present if not running in a debugging application)
	if (enableDebugMarkers)
	{
		VkDebugMarkerMarkerInfoEXT markerInfo = {};
		markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
		memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
		markerInfo.pMarkerName = pMarkerName;
		vkCmdDebugMarkerBegin(cmdbuffer, &markerInfo);
	}
}

void OgDeviceVulkan::Insert(VkCommandBuffer cmdbuffer, std::string markerName, glm::vec4 color)
{
	// Check for valid function pointer (may not be present if not running in a debugging application)
	if (enableDebugMarkers)
	{
		VkDebugMarkerMarkerInfoEXT markerInfo = {};
		markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
		memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
		markerInfo.pMarkerName = markerName.c_str();
		vkCmdDebugMarkerInsert(cmdbuffer, &markerInfo);
	}
}

void OgDeviceVulkan::EndRegion(VkCommandBuffer cmdBuffer)
{
	// Check for valid function (may not be present if not runnin in a debugging application)
	if (vkCmdDebugMarkerEnd)
	{
		vkCmdDebugMarkerEnd(cmdBuffer);
	}
}

VkResult OgDeviceVulkan::CreateBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkBuffer* buffer, VkDeviceMemory* memory, void* data)
{
	// Create the buffer handle

	VkBufferCreateInfo bufCreateInfo{};
	bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufCreateInfo.usage = usageFlags;
	bufCreateInfo.size = size;

	bufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK_CHECK_RESULT(vkCreateBuffer(logicalDevice, &bufCreateInfo, nullptr, buffer));

	// Create the memory backing up the buffer handle
	VkMemoryRequirements memReqs;
	VkMemoryAllocateInfo memAlloc{};
	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

	vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	// Find a memory type index that fits the properties of the buffer
	memAlloc.memoryTypeIndex = GetMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);

	// SHADER_DEVICE_ADDRESS 사용 시 VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT 필요
	VkMemoryAllocateFlagsInfo memAllocFlags{};
	if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
	{
		memAllocFlags.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
		memAllocFlags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
		memAlloc.pNext = &memAllocFlags;
	}

	VK_CHECK_RESULT(vkAllocateMemory(logicalDevice, &memAlloc, nullptr, memory));

	// If a pointer to the buffer data has been passed, map the buffer and copy over the data
	if (data != nullptr)
	{
		void* mapped = nullptr;
		VK_CHECK_RESULT(vkMapMemory(logicalDevice, *memory, 0, size, 0, &mapped));
		memcpy(mapped, data, size);
		// If host coherency hasn't been requested, do a manual flush to make writes visible
		if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
		{
			VkMappedMemoryRange mappedRange{};
			mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			mappedRange.memory = *memory;
			mappedRange.offset = 0;
			mappedRange.size = size;

			// vkFlushMappedRanges 
			// GPU에 보내야 하는 데이터가 CPU 쓰기 작업이 끝나면 호출이 필요하고 호출 후 
			// CPU 에서 맵메모리가 캐시라인에서 지워진다.

			// vkInvalidateMappedRanges 
			// GPU 에서 의해 쓰여진 데이터를 안전하게 읽을 때 호출이 필요하다.

			vkFlushMappedMemoryRanges(logicalDevice, 1, &mappedRange);
		}
		vkUnmapMemory(logicalDevice, *memory);
	}

	// Attach the memory to the buffer object
	VK_CHECK_RESULT(vkBindBufferMemory(logicalDevice, *buffer, *memory, 0));

	return VK_SUCCESS;
}

VkResult OgDeviceVulkan::CreateBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkBuffer* buffer, VkDeviceMemory* memory, bool& isCoherent)
{
	// Create the buffer handle

	VkBufferCreateInfo bufCreateInfo{};
	bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufCreateInfo.usage = usageFlags;
	bufCreateInfo.size = size;
	bufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK_CHECK_RESULT(vkCreateBuffer(logicalDevice, &bufCreateInfo, nullptr, buffer));

	// Create the memory backing up the buffer handle
	VkMemoryRequirements memReqs;
	VkMemoryAllocateInfo memAlloc{};
	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

	vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	// Find a memory type index that fits the properties of the buffer
	memAlloc.memoryTypeIndex = GetMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);

	// SHADER_DEVICE_ADDRESS 사용 시 VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT 필요
	VkMemoryAllocateFlagsInfo memAllocFlags2{};
	if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
	{
		memAllocFlags2.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
		memAllocFlags2.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
		memAlloc.pNext = &memAllocFlags2;
	}

	VK_CHECK_RESULT(vkAllocateMemory(logicalDevice, &memAlloc, nullptr, memory));

	// Attach the memory to the buffer object
	VK_CHECK_RESULT(vkBindBufferMemory(logicalDevice, *buffer, *memory, 0));

	VkMemoryType type = this->memoryProperties.memoryTypes[memAlloc.memoryTypeIndex];

	// Bifrost(Mali) GPU VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & VK_MEMORY_PROPERTY_HOST_CACHED_BIT 옵션이 둘다 지원되어야 
	// Hardware Cache Coherent 이다.
	isCoherent = false;
	if ((type.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) != 0)
	{
		if ((type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0)
		{
			isCoherent = true;
		}
	}

	return VK_SUCCESS;

}

OG_NAMESPACE_RENDER_END
