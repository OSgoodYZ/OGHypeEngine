#pragma once
/*
* Class wrapping access to the swap chain
*
* A swap chain is a collection of framebuffers used for rendering and presentation to the windowing system
*
* Copyright (C) 2016-2017 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#ifndef _OG_VULKANSWAPCHAIN_H_
#define _OG_VULKANSWAPCHAIN_H_

#include <stdlib.h>
#include <string>
#include <assert.h>
#include <stdio.h>

#include <vector>

#include <vulkan/vulkan.h>
#if defined(__WIN32__)
#include <vulkan/vulkan_win32.h>
#elif defined(__MACOSX__)
#include <vulkan/vulkan_macos.h>
#elif defined(__IOS__)
#include <vulkan/vulkan_ios.h>
#elif defined(__ANDROID__)
#include <vulkan/vulkan_android.h>
#endif

// Macro to get a procedure address based on a vulkan instance
#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                        \
{                                                                       \
	_fp##entrypoint = reinterpret_cast<PFN_vk##entrypoint>(vkGetInstanceProcAddr(inst, "vk"#entrypoint)); \
	if (_fp##entrypoint == NULL)                                         \
	{																    \
		exit(1);                                                        \
	}                                                                   \
}
// Macro to get a procedure address based on a vulkan device
#define GET_DEVICE_PROC_ADDR(dev, entrypoint)                           \
{                                                                       \
	_fp##entrypoint = reinterpret_cast<PFN_vk##entrypoint>(vkGetDeviceProcAddr(dev, "vk"#entrypoint));   \
	if (_fp##entrypoint == NULL)                                         \
	{																    \
		exit(1);                                                        \
	}                                                                   \
}

OG_NAMESPACE_RENDER_BEGIN

struct OgDeviceVulkan;

typedef struct _SwapChainBuffers {
	VkImage image;
	VkImageView view;
} SwapChainBuffer;

class OgSwapChainVulkan
{
private:
	static VkInstance _instance;
	static VkPhysicalDevice _physicalDevice;
	static VkDevice _logicalDevice;
	static OgDeviceVulkan* _vulkanDevice;

	// Function pointers
	static PFN_vkGetPhysicalDeviceSurfaceSupportKHR _fpGetPhysicalDeviceSurfaceSupportKHR;
	static PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR _fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
	static PFN_vkGetPhysicalDeviceSurfaceFormatsKHR _fpGetPhysicalDeviceSurfaceFormatsKHR;
	static PFN_vkGetPhysicalDeviceSurfacePresentModesKHR _fpGetPhysicalDeviceSurfacePresentModesKHR;
	static PFN_vkCreateSwapchainKHR _fpCreateSwapchainKHR;
	static PFN_vkDestroySwapchainKHR _fpDestroySwapchainKHR;
	static PFN_vkGetSwapchainImagesKHR _fpGetSwapchainImagesKHR;
	static PFN_vkAcquireNextImageKHR _fpAcquireNextImageKHR;
	static PFN_vkQueuePresentKHR _fpQueuePresentKHR;

private:
	VkSurfaceKHR _surface;

public:

	/** @brief format and space of swapchain color buffer */
	VkFormat colorFormat;
	VkColorSpaceKHR colorSpace;

	/** @brief Handle to the current swap chain, required for recreation */
	VkSwapchainKHR swapChain = VK_NULL_HANDLE;

	/** @breif buffer count and handles of swap chain */
	uint32 imageCount;
	std::vector<VkImage> images;
	std::vector<SwapChainBuffer> buffers;

	/** @brief Queue family index of the detected graphics and presenting device queue */
	uint32 presentQueueIndex = UINT32_MAX;

	/**
	* Set instance, physical and logical device to use for the swapchain and get all required function pointers
	*
	* @param instance Vulkan instance to use
	* @param physicalDevice Physical device used to query properties and formats relevant to the swapchain
	* @param device Logical representation of the device to create the swapchain for
	*
	*/
	static void Connect(VkInstance instance, OgDeviceVulkan* deviceVulkan);

	/** @brief Creates the platform specific surface abstraction of the native platform window used for presentation */
#if defined(__WIN32__)
	void InitSurface(void* platformHandle, void* platformWindow);
#elif defined(__ANDROID__)
	void InitSurface(ANativeWindow* window);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	void InitSurface(wl_display* display, wl_surface* window);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
	void InitSurface(xcb_connection_t* connection, xcb_window_t window);
#elif (defined(__IOS__) || defined(__MACOSX__))
	void InitSurface(void* view);
#elif defined(_DIRECT2DISPLAY)
	void InitSurface(uint32_t width, uint32_t height);
#endif

	/**
	* Create the swapchain and get it's images with given width and height
	*
	* @param width Pointer to the width of the swapchain (may be adjusted to fit the requirements of the swapchain)
	* @param height Pointer to the height of the swapchain (may be adjusted to fit the requirements of the swapchain)
	* @param vsync (Optional) Can be used to force vsync'd rendering (by using VK_PRESENT_MODE_FIFO_KHR as presentation mode)
	*/
	void Create(uint32* width, uint32* height, bool vsync = false);

	/**
	* Acquires the next image in the swap chain
	*
	* @param presentCompleteSemaphore (Optional) Semaphore that is signaled when the image is ready for use
	* @param imageIndex Pointer to the image index that will be increased if the next image could be acquired
	*
	* @note The function will always wait until the next image has been acquired by setting timeout to UINT64_MAX
	*
	* @return VkResult of the image acquisition
	*/
	VkResult AcquireNextImage(VkSemaphore presentCompleteSemaphore, uint32* imageIndex);

	/**
	* Queue an image for presentation
	*
	* @param queue Presentation queue for presenting the image
	* @param imageIndex Index of the swapchain image to queue for presentation
	* @param waitSemaphore (Optional) Semaphore that is waited on before the image is presented (only used if != VK_NULL_HANDLE)
	*
	* @return VkResult of the queue presentation
	*/
	VkResult QueuePresent(VkQueue queue, uint32 imageIndex, VkSemaphore waitSemaphore = VK_NULL_HANDLE);

	/**
	* Destroy and free Vulkan resources used for the swapchain
	*/
	void Cleanup();

private:
	// Called When initiating Surface
	void initPresentQueueIndex();

	// Called When initiating Surface
	void initColorFormatAndSpace();

	// Called when creating swapchain.
	void createSwapChainImageViews(VkSwapchainKHR oldSwapchain);

#if defined(_DIRECT2DISPLAY)
	/**
	* Create direct to display surface
	*/
	void createDirect2DisplaySurface(uint32 width, uint32 height);
#endif 
};

OG_NAMESPACE_RENDER_END

#endif
