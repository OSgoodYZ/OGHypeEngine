#pragma once

#ifndef _OG_DEVICE_VULKAN__H
#define _OG_DEVICE_VULKAN__H

#include <vector>
#include <glm/glm.hpp>

#include "render/private/vulkan/OgUtilityVulkan.h"

using namespace std;

OG_NAMESPACE_RENDER_BEGIN

struct OgDeviceVulkan
{
	struct
	{
		uint32_t graphics;
		uint32_t compute;
		uint32_t transfer;
	} queueFamilyIndices;

	/** @brief Physical device representation */
	VkPhysicalDevice physicalDevice;
	/** @brief Logical device representation (application's view of the device) */
	VkDevice logicalDevice;

	/** @brief Properties of the physical device including limits that the application can check against */
	VkPhysicalDeviceProperties properties;

	/** @brief Features of the physical device that an application can use to check if a feature is supported */
	VkPhysicalDeviceFeatures features;

	VkPhysicalDeviceFeatures enabledFeatures;

	/** @brief Memory types and heaps of the physical device */
	VkPhysicalDeviceMemoryProperties memoryProperties;

	vector<VkQueueFamilyProperties> queueFamilyProperties;

	vector<const char*> supportedExtensions;

	vector<const char*> deviceExtensions;

	/** @brief Default command pool for the graphics queue family index */
	VkCommandPool commandPool = VK_NULL_HANDLE;

	/**  @brief Typecast to VkDevice */
	operator VkDevice() { return logicalDevice; };

	bool useSwapChain = false;
	/** @brief Set to true when the debug marker extension is detected */
	bool enableDebugMarkers = false;

	PFN_vkDebugMarkerSetObjectTagEXT vkDebugMarkerSetObjectTag = VK_NULL_HANDLE;

	PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectName = VK_NULL_HANDLE;

	PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBegin = VK_NULL_HANDLE;

	PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEnd = VK_NULL_HANDLE;

	PFN_vkCmdDebugMarkerInsertEXT vkCmdDebugMarkerInsert = VK_NULL_HANDLE;

	/**
	* Default constructor
	*
	* @param physicalDevice Physical device that is to be used
	*/
	OgDeviceVulkan(VkPhysicalDevice physicalDevice);

	/**
	* Default destructor
	*
	* @note Frees the logical device
	*/
	~OgDeviceVulkan();

	/**
	* Get the index of a memory type that has all the requested property bits set
	*
	* @param typeBits Bitmask with bits set for each memory type supported by the resource to request for (from VkMemoryRequirements)
	* @param properties Bitmask of properties for the memory type to request
	* @param (Optional) memTypeFound Pointer to a bool that is set to true if a matching memory type has been found
	*
	* @return Index of the requested memory type
	*
	* @throw Throws an exception if memTypeFound is null and no memory type could be found that supports the requested properties
	*/
	uint32_t GetMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32* memTypeFound = nullptr);

	/**
	* Get the index of a queue family that supports the requested queue flags
	*
	* @param queueFlags Queue flags to find a queue family index for
	* @return Index of the queue family index that matches the flags
	* @throw Throws an exception if no queue family index could be found that supports the requested flags
	*/
	uint32_t GetQueueFamilyIndex(VkQueueFlagBits queueFlags);

	/**
	* Create a command pool for allocation command buffers from
	*
	* @param queueFamilyIndex Family index of the queue to create the command pool for
	* @param createFlags (Optional) Command pool creation flags (Defaults to VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
	*
	* @note Command buffers allocated from the created pool can only be submitted to a queue with the same family index
	*
	* @return A handle to the created command buffer
	*/
	VkCommandPool CreateCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);


	/**
	* Check if an extension is supported by the (physical device)
	*
	* @param extension Name of the extension to check
	*
	* @return True if the extension is supported (present in the list read at device creation time)
	*/
	bool HasExtension(const char* extension);

	/**
	* Create the logical device based on the assigned physical device, also gets default queue family indices
	*
	* @param enabledFeatures Can be used to enable certain features upon device creation
	* @param useSwapChain Set to false for headless rendering to omit the swapchain device extensions
	* @param requestedQueueTypes Bit flags specifying the queue types to be requested from the device
	*
	* @return VkResult of the device creation call
	*/
	VkResult CreateLogicalDevice(VkPhysicalDeviceFeatures enabledFeatures, const vector<const char*>& enabledExtensions, bool useSwapChain = true, VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);

	// Sets the debug name of an object
	// All Objects in Vulkan are represented by their 64-bit handles which are passed into this function
	// along with the object type
	void SetObjectName(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, const char* name);

	// Set the tag for an object
	void SetObjectTag(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, uint64_t name, size_t tagSize, const void* tag);

	// Start a new debug marker region
	void BeginRegion(VkCommandBuffer cmdbuffer, const char* pMarkerName, glm::vec4 color);

	// Insert a new debug marker into the command buffer
	void Insert(VkCommandBuffer cmdbuffer, std::string markerName, glm::vec4 color);

	// End the current debug marker region
	void EndRegion(VkCommandBuffer cmdBuffer);

	/**
	* Create a buffer on the device
	*
	* @param usageFlags Usage flag bitmask for the buffer (i.e. index, vertex, uniform buffer)
	* @param memoryPropertyFlags Memory properties for this buffer (i.e. device local, host visible, coherent)
	* @param size Size of the buffer in byes
	* @param buffer Pointer to the buffer handle acquired by the function
	* @param memory Pointer to the memory handle acquired by the function
	* @param data Pointer to the data that should be copied to the buffer after creation (optional, if not set, no data is copied over)
	*
	* @return VK_SUCCESS if buffer handle and memory have been created and (optionally passed) data has been copied
	*/
	VkResult CreateBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkBuffer* buffer, VkDeviceMemory* memory, void* data = nullptr);

	VkResult CreateBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkBuffer* buffer, VkDeviceMemory* memory, bool& isCoherent);
};

OG_NAMESPACE_RENDER_END

#endif //_OG_DEVICE_VULKAN__H
