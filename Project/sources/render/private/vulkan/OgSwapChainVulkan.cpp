#include "OgPrecompile.h"
#include "render/private/vulkan/OgSwapChainVulkan.h"

#include "render/private/vulkan/OgDeviceVulkan.h"

OG_NAMESPACE_RENDER_BEGIN


VkInstance OgSwapChainVulkan::_instance;
VkPhysicalDevice OgSwapChainVulkan::_physicalDevice;
VkDevice OgSwapChainVulkan::_logicalDevice;
OgDeviceVulkan* OgSwapChainVulkan::_vulkanDevice;
PFN_vkGetPhysicalDeviceSurfaceSupportKHR OgSwapChainVulkan::_fpGetPhysicalDeviceSurfaceSupportKHR;
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR OgSwapChainVulkan::_fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR OgSwapChainVulkan::_fpGetPhysicalDeviceSurfaceFormatsKHR;
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR OgSwapChainVulkan::_fpGetPhysicalDeviceSurfacePresentModesKHR;
PFN_vkCreateSwapchainKHR OgSwapChainVulkan::_fpCreateSwapchainKHR;
PFN_vkDestroySwapchainKHR OgSwapChainVulkan::_fpDestroySwapchainKHR;
PFN_vkGetSwapchainImagesKHR OgSwapChainVulkan::_fpGetSwapchainImagesKHR;
PFN_vkAcquireNextImageKHR OgSwapChainVulkan::_fpAcquireNextImageKHR;
PFN_vkQueuePresentKHR OgSwapChainVulkan::_fpQueuePresentKHR;

void OgSwapChainVulkan::Connect(VkInstance instance, OgDeviceVulkan* deviceVulkan)
{
	_instance = instance;
	_physicalDevice = deviceVulkan->physicalDevice;
	_logicalDevice = deviceVulkan->logicalDevice;
	_vulkanDevice = deviceVulkan;

	GET_INSTANCE_PROC_ADDR(_instance, GetPhysicalDeviceSurfaceSupportKHR);
	GET_INSTANCE_PROC_ADDR(_instance, GetPhysicalDeviceSurfaceCapabilitiesKHR);
	GET_INSTANCE_PROC_ADDR(_instance, GetPhysicalDeviceSurfaceFormatsKHR);
	GET_INSTANCE_PROC_ADDR(_instance, GetPhysicalDeviceSurfacePresentModesKHR);
	GET_DEVICE_PROC_ADDR(_logicalDevice, CreateSwapchainKHR);
	GET_DEVICE_PROC_ADDR(_logicalDevice, DestroySwapchainKHR);
	GET_DEVICE_PROC_ADDR(_logicalDevice, GetSwapchainImagesKHR);
	GET_DEVICE_PROC_ADDR(_logicalDevice, AcquireNextImageKHR);
	GET_DEVICE_PROC_ADDR(_logicalDevice, QueuePresentKHR);
}

#if defined(__WIN32__)
void OgSwapChainVulkan::InitSurface(void* platformHandle, void* platformWindow)
#elif defined(__ANDROID__)
void OgSwapChainVulkan::InitSurface(ANativeWindow* window)
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
void OgSwapChainVulkan::InitSurface(wl_display* display, wl_surface* window)
#elif defined(VK_USE_PLATFORM_XCB_KHR)
void OgSwapChainVulkan::InitSurface(xcb_connection_t* connection, xcb_window_t window)
#elif (defined(__IOS__) || defined(__MACOSX__))
void OgSwapChainVulkan::InitSurface(void* view)
#elif defined(_DIRECT2DISPLAY)
void OgSwapChainVulkan::InitSurface(uint32_t width, uint32_t height)
#endif
{
	VkResult err = VK_SUCCESS;

	// Create the os-specific surface
#if defined(__WIN32__)
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hinstance = (HINSTANCE)platformHandle;
	surfaceCreateInfo.hwnd = (HWND)platformWindow;
	err = vkCreateWin32SurfaceKHR(_instance, &surfaceCreateInfo, nullptr, &_surface);
#elif defined(__ANDROID__)
	VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.window = (ANativeWindow*)window;

	ASSERT(vkCreateAndroidSurfaceKHR != NULL);

	err = vkCreateAndroidSurfaceKHR(_instance, &surfaceCreateInfo, NULL, &_surface);
#elif defined(__IOS__)
	VkIOSSurfaceCreateInfoMVK surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK;
	surfaceCreateInfo.pNext = NULL;
	surfaceCreateInfo.flags = 0;
	surfaceCreateInfo.pView = view;
	err = vkCreateIOSSurfaceMVK(_instance, &surfaceCreateInfo, nullptr, &_surface);
#elif defined(__MACOSX__)
	VkMacOSSurfaceCreateInfoMVK surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
	surfaceCreateInfo.pNext = NULL;
	surfaceCreateInfo.flags = 0;
	surfaceCreateInfo.pView = view;
	err = vkCreateMacOSSurfaceMVK(_instance, &surfaceCreateInfo, NULL, &_surface);
#else
	LOGE(OG_ID, "Not Supported Platform Yet");
	/*
#elif defined(_DIRECT2DISPLAY)
		createDirect2DisplaySurface(width, height);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
		VkWaylandSurfaceCreateInfoKHR surfaceCreateInfo = {};
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
		surfaceCreateInfo.display = display;
		surfaceCreateInfo.surface = window;
		err = vkCreateWaylandSurfaceKHR(_instance, &surfaceCreateInfo, nullptr, &_surface);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
		VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {};
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
		surfaceCreateInfo.connection = connection;
		surfaceCreateInfo.window = window;
		err = vkCreateXcbSurfaceKHR(_instance, &surfaceCreateInfo, nullptr, &_surface);
		*/
#endif
	if (err != VK_SUCCESS) LOGE(OG_ID, "Could not create surface!");

	initPresentQueueIndex();
	initColorFormatAndSpace();
}

void OgSwapChainVulkan::Create(uint32* width, uint32* height, bool vsync)
{
	VkSwapchainKHR oldSwapchain = swapChain;

	// Get physical device surface properties and formats
	VkSurfaceCapabilitiesKHR surfCaps;
	VK_CHECK_RESULT(_fpGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, _surface, &surfCaps));

	// Get available present modes
	uint32_t presentModeCount;
	VK_CHECK_RESULT(_fpGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, _surface, &presentModeCount, NULL));
	assert(presentModeCount > 0);

	vector<VkPresentModeKHR> presentModes(presentModeCount);
	VK_CHECK_RESULT(_fpGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, _surface, &presentModeCount, presentModes.data()));

	VkExtent2D swapchainExtent = {};
	// If width (and height) equals the special value 0xFFFFFFFF, the size of the surface will be set by the swapchain/
	if (surfCaps.currentExtent.width <= (uint32_t)0)
	{
		// If the surface size is undefined, the size is set to
		// the size of the images requested.
		swapchainExtent.width = *width;
		swapchainExtent.height = *height;
	}
	else
	{
		// If the surface size is defined, the swap chain size must match
		swapchainExtent = surfCaps.currentExtent;
		*width = surfCaps.currentExtent.width;
		*height = surfCaps.currentExtent.height;
	}

	// Select a present mode for the swapchain


	// The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
	// This mode waits for the vertical blank ("v-sync")
	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

	// If v-sync is not requested, try to find a mailbox mode
	// It's the lowest latency non-tearing present mode available
	if (!vsync)
	{
		for (size_t i = 0; i < presentModeCount; i++)
		{
			if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
			{
#if defined(__DESKTOP__)
				swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
#elif defined(__ANDROID__)
				swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
#endif
				break;
			}
			if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) && (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR))
			{
				swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
			}
		}
	}



	// Determine the number of images
	uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
	if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount))
	{
		desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
	}

	// Find the transformation of the surface
	VkSurfaceTransformFlagsKHR preTransform;
	if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		// We prefer a non-rotated transform
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
	{
		preTransform = surfCaps.currentTransform;
	}

	// Find a supported composite alpha format (not all devices support alpha opaque)
	VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	// Simply select the first composite alpha format available
	vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
	};
	for (auto& compositeAlphaFlag : compositeAlphaFlags) {
		if (surfCaps.supportedCompositeAlpha & compositeAlphaFlag) {
			compositeAlpha = compositeAlphaFlag;
			break;
		};
	}

	VkSwapchainCreateInfoKHR swapchainCI = {};
	swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCI.pNext = NULL;
	swapchainCI.surface = _surface;
	swapchainCI.minImageCount = desiredNumberOfSwapchainImages;
	swapchainCI.imageFormat = colorFormat;
	swapchainCI.imageColorSpace = colorSpace;
	swapchainCI.imageExtent = { swapchainExtent.width, swapchainExtent.height };
	swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCI.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
	swapchainCI.imageArrayLayers = 1;
	swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCI.queueFamilyIndexCount = 0;
	swapchainCI.pQueueFamilyIndices = NULL;
	swapchainCI.presentMode = swapchainPresentMode;
	swapchainCI.oldSwapchain = oldSwapchain;
	// Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
	swapchainCI.clipped = VK_TRUE;
	swapchainCI.compositeAlpha = compositeAlpha;

	// Enable transfer source on swap chain images if supported
	if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
		swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	// Enable transfer destination on swap chain images if supported
	if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
		swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	VK_CHECK_RESULT(_fpCreateSwapchainKHR(_logicalDevice, &swapchainCI, nullptr, &swapChain));

	createSwapChainImageViews(oldSwapchain);
}

VkResult OgSwapChainVulkan::AcquireNextImage(VkSemaphore presentCompleteSemaphore, uint32* imageIndex)
{
	// By setting timeout to UINT64_MAX we will always wait until the next image has been acquired or an actual error is thrown
	// With that we don't have to buffers VK_NOT_READY
	return _fpAcquireNextImageKHR(_logicalDevice, swapChain, UINT64_MAX, presentCompleteSemaphore, (VkFence)nullptr, imageIndex);
}

VkResult OgSwapChainVulkan::QueuePresent(VkQueue queue, uint32 imageIndex, VkSemaphore waitSemaphore)
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = NULL;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapChain;
	presentInfo.pImageIndices = &imageIndex;
	// Check if a wait semaphore has been specified to wait for before presenting the image
	if (waitSemaphore != VK_NULL_HANDLE)
	{
		presentInfo.pWaitSemaphores = &waitSemaphore;
		presentInfo.waitSemaphoreCount = 1;
	}
	return _fpQueuePresentKHR(queue, &presentInfo);
}

void OgSwapChainVulkan::Cleanup()
{
	if (swapChain != VK_NULL_HANDLE)
	{
		for (uint32_t i = 0; i < imageCount; i++)
		{
			vkDestroyImageView(_logicalDevice, buffers[i].view, nullptr);
		}
	}
	if (_surface != VK_NULL_HANDLE)
	{
		_fpDestroySwapchainKHR(_logicalDevice, swapChain, nullptr);
		vkDestroySurfaceKHR(_instance, _surface, nullptr);
	}
	_surface = VK_NULL_HANDLE;
	swapChain = VK_NULL_HANDLE;
}

void OgSwapChainVulkan::initPresentQueueIndex()
{
	const vector< VkQueueFamilyProperties>& queueFamilyProperties = _vulkanDevice->queueFamilyProperties;
	uint32 queueFamilyCount = (uint32)queueFamilyProperties.size();

	// Iterate over each queue to learn whether it supports presenting:
	// Find a queue with present support
	// Will be used to present the swap chain images to the windowing system
	vector<VkBool32> supportsPresent(queueFamilyCount);
	for (uint32_t i = 0; i < queueFamilyCount; i++)
	{
		_fpGetPhysicalDeviceSurfaceSupportKHR(_physicalDevice, i, _surface, &supportsPresent[i]);
	}

	// Search for a graphics and a present queue in the array of queue
	// families, try to find one that supports both
	uint32_t graphicsQueueNodeIndex = UINT32_MAX;
	uint32_t presentQueueNodeIndex = UINT32_MAX;
	for (uint32_t i = 0; i < queueFamilyCount; i++)
	{
		if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
		{
			if (graphicsQueueNodeIndex == UINT32_MAX)
			{
				graphicsQueueNodeIndex = i;
			}

			if (supportsPresent[i] == VK_TRUE)
			{
				graphicsQueueNodeIndex = i;
				presentQueueNodeIndex = i;
				break;
			}
		}
	}

	if (presentQueueNodeIndex == UINT32_MAX)
	{
		LOGE(OG_ID, "Lv1Engine is not supporting yet 'Separate Present Queue'with Graphics Queue");

		// TODO : Handling Separate Present Queue
		/*
		// If there's no queue that supports both present and graphics
		// try to find a separate present queue
		for (uint32_t i = 0; i < queueFamilyCount; ++i)
		{
			if (supportsPresent[i] == VK_TRUE)
			{
				presentQueueNodeIndex = i;
				break;
			}
		}
		*/
	}

	// Exit if either a graphics or a presenting queue hasn't been found
	if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX)
		LOGE(OG_ID, "Could not find a graphics and/or presenting queue!");

	//// TODO : Add support for separate graphics and presenting queue
	//if (graphicsQueueNodeIndex != presentQueueNodeIndex)
	//	LOGE(OG_ID, "Separate graphics and presenting queues are not supported yet!");

	presentQueueIndex = presentQueueNodeIndex;
}

// Called When initiating Surface
void OgSwapChainVulkan::initColorFormatAndSpace()
{
	// Get list of supported surface formats
	uint32_t formatCount;
	VkResult result = _fpGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &formatCount, NULL);
	if (result != VK_SUCCESS) LOGE(OG_ID, "Separate graphics and presenting queues are not supported yet!");
	assert(formatCount > 0);

	vector<VkSurfaceFormatKHR> surfaceFormats;
	surfaceFormats.resize(formatCount);
	VK_CHECK_RESULT(_fpGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &formatCount, surfaceFormats.data()));

	// If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
	// there is no preferered format, so we assume VK_FORMAT_B8G8R8A8_UNORM
	if ((formatCount == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED))
	{
		colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
		colorSpace = surfaceFormats[0].colorSpace;
	}
	else
	{
		// iterate over the list of available surface format and
		// check for the presence of VK_FORMAT_B8G8R8A8_UNORM
		bool found_B8G8R8A8_UNORM = false;
		for (auto&& surfaceFormat : surfaceFormats)
		{
			if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
			{
				colorFormat = surfaceFormat.format;
				colorSpace = surfaceFormat.colorSpace;
				found_B8G8R8A8_UNORM = true;
				break;
			}
		}

		// in case VK_FORMAT_B8G8R8A8_UNORM is not available
		// select the first available color format
		if (!found_B8G8R8A8_UNORM)
		{
			colorFormat = surfaceFormats[0].format;
			colorSpace = surfaceFormats[0].colorSpace;
		}
	}
}

// Called when creating swapchain.
void OgSwapChainVulkan::createSwapChainImageViews(VkSwapchainKHR oldSwapchain)
{
	// If an existing swap chain is re-created, destroy the old swap chain
	// This also cleans up all the presentable images
	if (oldSwapchain != VK_NULL_HANDLE)
	{
		for (uint32_t i = 0; i < imageCount; i++)
		{
			vkDestroyImageView(_logicalDevice, buffers[i].view, nullptr);
		}
		_fpDestroySwapchainKHR(_logicalDevice, oldSwapchain, nullptr);
	}
	VK_CHECK_RESULT(_fpGetSwapchainImagesKHR(_logicalDevice, swapChain, &imageCount, NULL));

	// Get the swap chain images
	images.resize(imageCount);
	VK_CHECK_RESULT(_fpGetSwapchainImagesKHR(_logicalDevice, swapChain, &imageCount, &images.front()));

	// Get the swap chain buffers containing the image and imageview
	buffers.resize(imageCount);
	for (uint32_t i = 0; i < imageCount; i++)
	{
		VkImageViewCreateInfo colorAttachmentView = {};
		colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		colorAttachmentView.pNext = NULL;
		colorAttachmentView.format = colorFormat;
		colorAttachmentView.components = {
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A
		};
		colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorAttachmentView.subresourceRange.baseMipLevel = 0;
		colorAttachmentView.subresourceRange.levelCount = 1;
		colorAttachmentView.subresourceRange.baseArrayLayer = 0;
		colorAttachmentView.subresourceRange.layerCount = 1;
		colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		colorAttachmentView.flags = 0;
		colorAttachmentView.image = images[i];

		buffers[i].image = images[i];
		VK_CHECK_RESULT(vkCreateImageView(_logicalDevice, &colorAttachmentView, nullptr, &buffers[i].view));
	}
}

#if defined(_DIRECT2DISPLAY)
/**
* Create direct to display surface
*/
void OgSwapChainVulkan::createDirect2DisplaySurface(uint32 width, uint32 height)
{
	uint32_t displayPropertyCount;

	// Get display property
	vkGetPhysicalDeviceDisplayPropertiesKHR(_physicalDevice, &displayPropertyCount, NULL);
	VkDisplayPropertiesKHR* pDisplayProperties = new VkDisplayPropertiesKHR[displayPropertyCount];
	vkGetPhysicalDeviceDisplayPropertiesKHR(_physicalDevice, &displayPropertyCount, pDisplayProperties);

	// Get plane property
	uint32_t planePropertyCount;
	vkGetPhysicalDeviceDisplayPlanePropertiesKHR(_physicalDevice, &planePropertyCount, NULL);
	VkDisplayPlanePropertiesKHR* pPlaneProperties = new VkDisplayPlanePropertiesKHR[planePropertyCount];
	vkGetPhysicalDeviceDisplayPlanePropertiesKHR(_physicalDevice, &planePropertyCount, pPlaneProperties);

	VkDisplayKHR display = VK_NULL_HANDLE;
	VkDisplayModeKHR displayMode;
	VkDisplayModePropertiesKHR* pModeProperties;
	bool foundMode = false;

	for (uint32_t i = 0; i < displayPropertyCount; ++i)
	{
		display = pDisplayProperties[i].display;
		uint32_t modeCount;
		vkGetDisplayModePropertiesKHR(_physicalDevice, display, &modeCount, NULL);
		pModeProperties = new VkDisplayModePropertiesKHR[modeCount];
		vkGetDisplayModePropertiesKHR(_physicalDevice, display, &modeCount, pModeProperties);

		for (uint32_t j = 0; j < modeCount; ++j)
		{
			const VkDisplayModePropertiesKHR* mode = &pModeProperties[j];

			if (mode->parameters.visibleRegion.width == width && mode->parameters.visibleRegion.height == height)
			{
				displayMode = mode->displayMode;
				foundMode = true;
				break;
			}
		}
		if (foundMode)
		{
			break;
		}
		delete[] pModeProperties;
	}

	if (!foundMode)
	{
		vks::tools::exitFatal("Can't find a display and a display mode!", -1);
		return;
	}

	// Search for a best plane we can use
	uint32_t bestPlaneIndex = UINT32_MAX;
	VkDisplayKHR* pDisplays = NULL;
	for (uint32_t i = 0; i < planePropertyCount; i++)
	{
		uint32_t planeIndex = i;
		uint32_t displayCount;
		vkGetDisplayPlaneSupportedDisplaysKHR(_physicalDevice, planeIndex, &displayCount, NULL);
		if (pDisplays)
		{
			delete[] pDisplays;
		}
		pDisplays = new VkDisplayKHR[displayCount];
		vkGetDisplayPlaneSupportedDisplaysKHR(_physicalDevice, planeIndex, &displayCount, pDisplays);

		// Find a display that matches the current plane
		bestPlaneIndex = UINT32_MAX;
		for (uint32_t j = 0; j < displayCount; j++)
		{
			if (display == pDisplays[j])
			{
				bestPlaneIndex = i;
				break;
			}
		}
		if (bestPlaneIndex != UINT32_MAX)
		{
			break;
		}
	}

	if (bestPlaneIndex == UINT32_MAX)
	{
		vks::tools::exitFatal("Can't find a plane for displaying!", -1);
		return;
	}

	VkDisplayPlaneCapabilitiesKHR planeCap;
	vkGetDisplayPlaneCapabilitiesKHR(_physicalDevice, displayMode, bestPlaneIndex, &planeCap);
	VkDisplayPlaneAlphaFlagBitsKHR alphaMode = (VkDisplayPlaneAlphaFlagBitsKHR)0;

	if (planeCap.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR)
	{
		alphaMode = VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR;
	}
	else if (planeCap.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR)
	{
		alphaMode = VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR;
	}
	else if (planeCap.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR)
	{
		alphaMode = VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR;
	}
	else if (planeCap.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR)
	{
		alphaMode = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
	}

	VkDisplaySurfaceCreateInfoKHR surfaceInfo{};
	surfaceInfo.sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
	surfaceInfo.pNext = NULL;
	surfaceInfo.flags = 0;
	surfaceInfo.displayMode = displayMode;
	surfaceInfo.planeIndex = bestPlaneIndex;
	surfaceInfo.planeStackIndex = pPlaneProperties[bestPlaneIndex].currentStackIndex;
	surfaceInfo.transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	surfaceInfo.globalAlpha = 1.0;
	surfaceInfo.alphaMode = alphaMode;
	surfaceInfo.imageExtent.width = width;
	surfaceInfo.imageExtent.height = height;

	VkResult result = vkCreateDisplayPlaneSurfaceKHR(_instance, &surfaceInfo, NULL, &_surface);
	if (result != VK_SUCCESS) {
		vks::tools::exitFatal("Failed to create surface!", result);
	}

	delete[] pDisplays;
	delete[] pModeProperties;
	delete[] pDisplayProperties;
	delete[] pPlaneProperties;
}
#endif 

OG_NAMESPACE_RENDER_END