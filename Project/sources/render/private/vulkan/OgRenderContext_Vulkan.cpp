#include <utility>

#include "OgPrecompile.h"

#include "system/OgSystemContext.h"
#include "system/OgHashCode.h"
#include "system/OgVector.h"

#include "render/private/vulkan/OgRenderContext_Vulkan.h"
#include "render/private/vulkan/OgVulkanHelper.h"

#if defined(OG_USE_CRT_CHASE_MEMORY_LEAK)
#define new DBG_NEW
#endif

using namespace Og::System;

OG_NAMESPACE_RENDER_BEGIN

static struct
{
	uint32 major;
	uint32 minor;
	uint32 build;

} s_sdkVersion;

#ifdef NDEBUG
static const bool s_enableValidationLayers = false;
static const bool s_shouldPrintLog = false;
#else
static const bool s_enableValidationLayers = true;
static const bool s_shouldPrintLog = true;

#if defined(__ANDROID__)
static const Lv::LvFixedList<const char*, 8> s_validationLayers =
{
	"VK_LAYER_GOOGLE_threading",
	"VK_LAYER_LUNARG_parameter_validation",
	"VK_LAYER_LUNARG_object_tracker",
	"VK_LAYER_LUNARG_core_validation",
	"VK_LAYER_GOOGLE_unique_objects",
#if defined(__MAIL__)
	"VK_LAYER_ARM_AGA"
#endif
};
#else
static const std::vector<const char*> s_validationLayers =
{
	"VK_LAYER_LUNARG_standard_validation",
	"VK_LAYER_KHRONOS_validation"
};
#endif
#endif

VKAPI_ATTR VkBool32 VKAPI_CALL report_debug_callback(VkDebugReportFlagsEXT msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject,
	size_t location, int32_t msgCode, const char* pLayerPrefix, const char* pMsg,
	void* pUserData)
{
	if (s_shouldPrintLog)
	{
		if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		{
			LOGE(OG_ID, "ERROR: [%s] Code %i : %s", pLayerPrefix, msgCode, pMsg);
		}
		else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		{
			LOGD(OG_ID, "WARNING: [%s] Code %i : %s", pLayerPrefix, msgCode, pMsg);
		}
		else if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
		{
			LOGD(OG_ID, "PERFORMANCE WARNING: [%s] Code %i : %s", pLayerPrefix, msgCode, pMsg);
		}
		else if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
		{
			LOGD(OG_ID, "INFO: [%s] Code %i : %s", pLayerPrefix, msgCode, pMsg);
		}
		else if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
		{
			LOGD(OG_ID, "DEBUG: [%s] Code %i : %s", pLayerPrefix, msgCode, pMsg);
		}
	}

	/*
	* false indicates that layer should not bail-out of an
	* API call that had validation failures. This may mean that the
	* app dies inside the driver due to invalid parameter(s).
	* That's what would happen without validation layers, so we'll
	* keep that behavior here.
	*/
	return VK_FALSE;
}

VkResult create_debug_report_callback(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback)
{
	// VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback
	PFN_vkCreateDebugReportCallbackEXT func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void destroy_debug_reeport_callback(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}
// Debug

bool check_validation_layer_support()
{
	uint32_t layerCount;
	VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

	if (layerCount > 0)
	{
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		if (s_shouldPrintLog == true)
		{
			LOGD(OG_ID, "Vaildation Layer Count = %zu", availableLayers.size());

			for (const VkLayerProperties& layerProperties : availableLayers)
			{
				s_sdkVersion.major = ((uint32_t)(layerProperties.specVersion) >> 22);
				s_sdkVersion.minor = (((uint32_t)(layerProperties.specVersion) >> 12) & 0x3ff);
				s_sdkVersion.build = (uint32_t)(layerProperties.specVersion) & 0xfff;

				LOGD(OG_ID, "Available Layer = %s", layerProperties.layerName);
				LOGD(OG_ID, "Description = %s", layerProperties.description);
				LOGD(OG_ID, "implementation Version = %d", layerProperties.implementationVersion);

				LOGD(OG_ID, "Vulkan Version : %u.%u.%u",
					s_sdkVersion.major,
					s_sdkVersion.minor,
					s_sdkVersion.build
				);
			}
		}

#if defined(_DEBUG)
		for (const char* layerName : s_validationLayers)
		{
			for (const VkLayerProperties& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
					return true;
			}
		}
#endif
	}

	return false;
}

OgRenderContextVulkan::OgRenderContextVulkan(OgSystemContext* context)
	:_instance(nullptr)
{
	this->platform = OgRenderPlatform::VULKAN;
	this->maxSubmitCount = 2;
	this->context = context;
}

OgRenderContextVulkan::~OgRenderContextVulkan()
{

#if defined(_DEBUG)
	if (_livingObjects.Size() > 0)
	{
		LOGD(OG_ID, "LvRenderContext Undestroy object : %zu", _livingObjects.Size());
		for (int i = 0; i < _livingObjects.Size(); ++i)
		{
			OgHandle* handle = _livingObjects[i];
			if (handle != nullptr)
				LOGD(OG_ID, "%s %s (%p)", handle->instanceType, handle->name, &handle);
		}
	}
#endif
}

void OgRenderContextVulkan::initInstance()
{
	if (s_enableValidationLayers && !check_validation_layer_support())
	{
		LOGE(OG_ID, "validation layers requested, but not available!");
	}

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Og";
	appInfo.pEngineName = "Og Engine";
	appInfo.apiVersion = VK_API_VERSION_1_4;

	bool surfaceExtFound = false;
	bool platformSurfaceExtFound = false;

	uint32 instance_extension_count = 0;

	VK_CHECK_RESULT(vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, NULL));

	if (instance_extension_count > 0 && _enabledInstanceExtensions.size() == 0)
	{
		VkExtensionProperties* instance_extensions = new VkExtensionProperties[instance_extension_count];
		VK_CHECK_RESULT(vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, instance_extensions));

		for (uint32 i = 0; i < instance_extension_count; i++)
		{
			if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME, instance_extensions[i].extensionName))
			{
				surfaceExtFound = true;
				_enabledInstanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
			}

			// Enable surface extensions depending on os
#if defined(__WIN32__)
			if (!strcmp(instance_extensions[i].extensionName, VK_KHR_WIN32_SURFACE_EXTENSION_NAME))
			{
				platformSurfaceExtFound = true;
				_enabledInstanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
			}
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
			if (!strcmp(instance_extensions[i].extensionName, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME))
			{
				platformSurfaceExtFound = true;
				_enabledInstanceExtensions.Add(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
			}
#elif defined(_DIRECT2DISPLAY)
			if (!strcmp(instance_extensions[i].extensionName, VK_KHR_DISPLAY_EXTENSION_NAME))
			{
				platformSurfaceExtFound = true;
				_enabledInstanceExtensions.Add(VK_KHR_DISPLAY_EXTENSION_NAME);
			}
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
			if (!strcmp(instance_extensions[i].extensionName, VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME))
			{
				platformSurfaceExtFound = true;
				_enabledInstanceExtensions.Add(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
			}
#elif defined(VK_USE_PLATFORM_XCB_KHR)
			if (!strcmp(instance_extensions[i].extensionName, VK_KHR_XCB_SURFACE_EXTENSION_NAME))
			{
				platformSurfaceExtFound = true;
				_enabledInstanceExtensions.Add(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
			}
#elif defined(__IOS__)
			if (!strcmp(instance_extensions[i].extensionName, VK_MVK_IOS_SURFACE_EXTENSION_NAME))
			{
				platformSurfaceExtFound = true;
				_enabledInstanceExtensions.Add(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
			}
#elif defined(__MACOSX__)
			if (!strcmp(instance_extensions[i].extensionName, VK_MVK_MACOS_SURFACE_EXTENSION_NAME))
			{
				platformSurfaceExtFound = true;
				_enabledInstanceExtensions.Add(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
			}
#endif

			if (!strcmp(instance_extensions[i].extensionName, VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
			{
				if (s_enableValidationLayers)
					_enabledInstanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
			}


		}

		delete[] instance_extensions;
	}


	// @NOTE: @osgood 25.03.09 
	// GLSL로 VULKAN을 사용할 때 임시적으로 VK_NV_glsl_shader을 확장해서 사용한다.
	// 이 확작은 NVIDIA의 확장이므로 NVIDIA만 사용할 수 있다.
	// 따라서 쉐이더 시스템을 개발하는 순간 이 확장은 제거되어야 한다.
	//_enabledInstanceExtensions.push_back(VK_NV_GLSL_SHADER_EXTENSION_NAME);

	VkInstanceCreateInfo instanceInfo = {};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pNext = NULL;
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.enabledExtensionCount = static_cast<uint32_t>(_enabledInstanceExtensions.size());
	instanceInfo.ppEnabledExtensionNames = _enabledInstanceExtensions.data();

	list<const char*> validationLayers;
	if (s_enableValidationLayers)
	{
#if defined(_DEBUG)
		if (s_validationLayers.size() > 0)
		{
#if defined(__DESKTOP__)

			if (s_sdkVersion.major >= 1 && s_sdkVersion.minor >= 2)
			{
				validationLayers.push_back("VK_LAYER_KHRONOS_validation");
				
				instanceInfo.enabledLayerCount = (uint32_t)validationLayers.size();
				instanceInfo.ppEnabledLayerNames = &validationLayers.front();
			}
			else
			{
				validationLayers.push_back("VK_LAYER_LUNARG_standard_validation");
				instanceInfo.enabledLayerCount = (uint32_t)validationLayers.size();
				instanceInfo.ppEnabledLayerNames = &validationLayers.front();
			}
#else
			instanceInfo.enabledLayerCount = static_cast<uint32_t>(s_validationLayers.Count());
			instanceInfo.ppEnabledLayerNames = s_validationLayers.data();
#endif
		}
#endif
	}
	else
	{
		instanceInfo.enabledLayerCount = 0;
	}

	VkResult err = vkCreateInstance(&instanceInfo, NULL, &_instance);
	if (err == VK_ERROR_INCOMPATIBLE_DRIVER)
	{
		LOGE(OG_ID,
			"Cannot find a compatible Vulkan installable client driver(ICD).\n\nPlease look at the Getting Started guide for additional information.\nvkCreateInstance Failure");
	}
	else if (err == VK_ERROR_EXTENSION_NOT_PRESENT)
	{
		LOGE(OG_ID, "Cannot find a specified extension library.\nMake sure your layers path is set appropriately.\nvkCreateInstance Failure");
	}
	else if (err == VK_ERROR_LAYER_NOT_PRESENT)
	{
		LOGE(OG_ID, "VK_ERROR_LAYER_NOT_PRESENT A specified layer cannot be found");
	}
	else if (err)
	{
		LOGE(OG_ID, "vkCreateInstance failed.\n\nDo you have a compatible Vulkan installable client driver (ICD) installed?\nPlease look at the Getting Started guide for additional information.\n vkCreateInstance Failure");
	}
}

void OgRenderContextVulkan::initDebug()
{
	if (!s_enableValidationLayers)
		return;

#if defined(_DEBUG)
	VkDebugReportCallbackCreateInfoEXT reportCreateInfo = {};
	reportCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	reportCreateInfo.pNext = NULL;
	reportCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	reportCreateInfo.pfnCallback = report_debug_callback;
	reportCreateInfo.pUserData = NULL;

	VK_CHECK_RESULT(create_debug_report_callback(_instance, &reportCreateInfo, NULL, &_reportCallbackHandle));
#endif
	// freeDebugCallback
}

void OgRenderContextVulkan::initDevice(void)
{
	uint32 deviceCount = 0;
	VkResult result = vkEnumeratePhysicalDevices(_instance, &deviceCount, NULL);

	if (result != VK_SUCCESS)
	{
		LOGE(OG_ID, "Failed to query the number of physical devices present: %s\n", vkErrorString(result));
	}

	vector<VkPhysicalDevice> physicalDevices(deviceCount);
	result = vkEnumeratePhysicalDevices(_instance, &deviceCount, physicalDevices.data());

	if (result != VK_SUCCESS)
	{
		LOGE(OG_ID, "Could not enumerate physical device %d\n", result);
	}

	if (s_shouldPrintLog)
	{
		VkPhysicalDeviceProperties deviceProperties;
		for (uint32 i = 0; i < deviceCount; i++)
		{
			memset(&deviceProperties, 0, sizeof deviceProperties);
			vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);

			LOGD(OG_ID, "Driver Version: %d\n", deviceProperties.driverVersion);
			LOGD(OG_ID, "Device Name:    %s\n", deviceProperties.deviceName);
			LOGD(OG_ID, "Device Type:    %d\n", deviceProperties.deviceType);
			LOGD(OG_ID, "API Version:    %d.%d.%d\n",
				// See note below regarding this:
				(deviceProperties.apiVersion >> 22) & 0x3FF,
				(deviceProperties.apiVersion >> 12) & 0x3FF,
				(deviceProperties.apiVersion & 0xFFF));
		}
	}

	// TODO : 가장 좋은 physical device를 고르는 로직 필요.
	_gpuDeviceVK = physicalDevices[0];

	// TODO : Physical Device Extension
	uint32 extCount = 0;
	vkEnumerateDeviceExtensionProperties(_gpuDeviceVK, nullptr, &extCount, nullptr);

	vector<VkExtensionProperties> available_extensions;

	available_extensions.resize(extCount);
	vkEnumerateDeviceExtensionProperties(_gpuDeviceVK, nullptr, &extCount, &available_extensions[0]);

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	// TODO : neeed to impl for Android
#endif

	vkGetPhysicalDeviceFeatures(_gpuDeviceVK, &_deviceFeaturesVK);
	vkGetPhysicalDeviceMemoryProperties(_gpuDeviceVK, &_deviceMemoryPropertiesVK);

	VkBool32 validDepthFormat = vkGetSupportedDepthFormat(_gpuDeviceVK, &_defaultDepthFormat);
	ASSERT(validDepthFormat);

	_vulkanDevice = new OgDeviceVulkan(_gpuDeviceVK);

	// TODO : 원하는 physical device feature 활용하기
	// TODO : 원하는 device extension사용하기. 
	// VkPhysicalDeviceFeatures enabledDeviceFeature;
	vector<const char*> enabledDeviceExtensions;

	// TODO : QUEUE_COMPUTE/TRANSFER_BIT에 대한 리서치 후, 활용하기
	// LvDeviceVulkan을 위한 CommandPool이 안에서 만들어지고 있음.
	VK_CHECK_RESULT(_vulkanDevice->CreateLogicalDevice(_deviceFeaturesVK, enabledDeviceExtensions, true, VK_QUEUE_GRAPHICS_BIT));
	_logicalDeviceVK = _vulkanDevice->logicalDevice;

	vkGetDeviceQueue(_logicalDeviceVK, _vulkanDevice->queueFamilyIndices.graphics, 0, &_graphicsQueueVK);


#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	// neeed to impl for Android 
#endif
}

void OgRenderContextVulkan::Load(void)
{
#if defined(__MACOSX__) || defined(__IOS__)
	context->mac.useMetal = true;
#elif defined(__ANDROID__)

#if (__ANDROID_API__  < 23)
	int vulkanSupport = InitVulkan();
	if (vulkanSupport == 0)
		return;
#endif

#endif
	initInstance();
	initDebug();
	initDevice();
}

void OgRenderContextVulkan::initCommandPool()
{
	VkCommandPoolCreateInfo cmdPoolInfo = {};
	cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolInfo.queueFamilyIndex = _vulkanDevice->queueFamilyIndices.graphics;
	cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VK_CHECK_RESULT(vkCreateCommandPool(_logicalDeviceVK, &cmdPoolInfo, nullptr, &_cmdPoolVK));
	_cmdPoolState = CommandPoolState::INIT;
}

void OgRenderContextVulkan::initDescriptorPool()
{
	_usedUniformBufferFromPool = 0;
	_usedTextureFromPool = 0;
	_usedSetFromPool = 0;
	_maxUniformBufferFromPool = 2048;//256;
	_maxTextureFromPool = 2048;
	_maxSetFromPool = 1024;//256;

	// Manual Initialize for VkDescriptorPool
	// 나중에 이것을 관리하는 DescriptorPool Manager를 만들어야 함.
	vector<VkDescriptorPoolSize> poolSizes(2);
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = _maxUniformBufferFromPool;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = _maxTextureFromPool;

	VkDescriptorPoolCreateInfo descriptorPoolInfo;
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descriptorPoolInfo.pPoolSizes = poolSizes.data();
	descriptorPoolInfo.maxSets = _maxSetFromPool;
	descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkDescriptorPoolCreateFlagBits.html
	descriptorPoolInfo.pNext = nullptr;

	VK_CHECK_RESULT(vkCreateDescriptorPool(_logicalDeviceVK, &descriptorPoolInfo, nullptr, &_descriptorPool));
}

void OgRenderContextVulkan::initStagingCommandBuffer()
{
	VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
	cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufAllocateInfo.commandPool = _cmdPoolVK;
	cmdBufAllocateInfo.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufAllocateInfo.commandBufferCount = 3;

	VK_CHECK_RESULT(vkAllocateCommandBuffers(_logicalDeviceVK, &cmdBufAllocateInfo, _stagingCommandBuffer));

	VkCommandBufferBeginInfo cmdBufferBeginInfo{};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	vkBeginCommandBuffer(_stagingCommandBuffer[0], &cmdBufferBeginInfo);
	_stagingSubmitIndex = 0;
}

void OgRenderContextVulkan::submitStagingCommandBuffer()
{
	if (_cmdPoolState != CommandPoolState::RESET)
	{
		VkCommandBuffer curStagingCmdBuffer = _stagingCommandBuffer[_stagingSubmitIndex];
		vkEndCommandBuffer(curStagingCmdBuffer);

		VkSubmitInfo stagingSubmitInfo{};
		stagingSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		stagingSubmitInfo.commandBufferCount = 1;
		stagingSubmitInfo.pCommandBuffers = &curStagingCmdBuffer;
		VK_CHECK_RESULT(vkQueueSubmit(_graphicsQueueVK, 1, &stagingSubmitInfo, VK_NULL_HANDLE));
	}

	// advance staging submit index and begin command
	_stagingSubmitIndex = (_stagingSubmitIndex + 1) % 3;

	// BeginCommandBuffer will reset the next command buffer to be encoded
	VkCommandBufferBeginInfo cmdBufferBeginInfo{};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	VK_CHECK_RESULT(vkResetCommandBuffer(_stagingCommandBuffer[_stagingSubmitIndex], 0));
	VK_CHECK_RESULT(vkBeginCommandBuffer(_stagingCommandBuffer[_stagingSubmitIndex], &cmdBufferBeginInfo));
}

void OgRenderContextVulkan::freeStagingCommandBuffers()
{
	vkFreeCommandBuffers(_logicalDeviceVK, _cmdPoolVK, 3, _stagingCommandBuffer);
}


void OgRenderContextVulkan::prepareSwapChain(SwapchainWrapper& sw)
{
 

#if defined(_WIN32)
	sw.swapchainVK.InitSurface(sw.window->win32.instance, sw.window->win32.handle);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)	
	sw.swapchainVK.InitSurface(sw.window->android.handle);
#elif (defined(__MACOSX__) || defined(__MACOSX__))
	sw.swapchainVK.InitSurface(sw.window->mac.view);
#else
	LOGE(OG_ID, "Not Supported Platform now");
	/*
#elif defined(_DIRECT2DISPLAY)
	sw.swapchainVK..InitSurface(width, height);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	sw.swapchainVK..InitSurface(display, surface);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
	sw.swapchainVK..InitSurface(connection, window);
	*/
#endif

	vkGetDeviceQueue(_logicalDeviceVK, sw.swapchainVK.presentQueueIndex, 0, &sw.presentQueueVK);
}



void OgRenderContextVulkan::initSwapChain(SwapchainWrapper& sw)
{
	if (sw.frameBufferObject.isInitialized == true)
		return;

	OgNativeWindow* window = sw.window;

	sw.swapchainVK.Create((uint32_t*)(&window->width), (uint32_t*)(&window->height), false);
	sw.frameBufferObject.bufferCount = sw.swapchainVK.imageCount;

	bool createDepthStencilBuffer = false;
	OgTextureUsage texUsage;
	OgPixelFormat texPFormat;
	OgRenderTextureFormat texRTFormat;
	if (sw.settingInfo.useDepthBuffer && sw.settingInfo.useStencilBuffer) // Depth and stencil Buffer
	{
		bool r1 = OgFormatSupplement::IsDepthStencilFormat(sw.settingInfo.depthBufferFormat);
		bool r2 = OgFormatSupplement::IsDepthStencilFormat(sw.settingInfo.stencilBufferFormat);
		if (!(r1 && r2)) LOGE(OG_ID, "Wrong Depth Stencil Format");
		if (sw.settingInfo.depthBufferFormat != sw.settingInfo.stencilBufferFormat) LOGE(OG_ID, "Depth Stencil Format Should be same");

		if (sw.settingInfo.depthBufferFormat == OgRenderTextureFormat::DEFAULT_DEPTH_STENCIL) // just check one format because it's already guaranteed.
		{
			const int DESIRED_FORMAT_NUMB = 3;

			// Find Default Depth_stencil format on this platform
			// 32_8 -> 24_8 -> 16_8
			OgRenderTextureFormat desiredFormat[DESIRED_FORMAT_NUMB] =
			{
				OgRenderTextureFormat::DEPTH32_STENCIL8,
				OgRenderTextureFormat::DEPTH24_STENCIL8,
				OgRenderTextureFormat::DEPTH16_STENCIL8,
			};

			bool canSupportDefaultDepthStencil = false;
			for (int i = 0; i < DESIRED_FORMAT_NUMB; ++i)
			{
				VkFormat f = (VkFormat)OgFormatSupplement::GetPixelFormat(desiredFormat[i]);

				if (vkIsSupportFormat(_gpuDeviceVK, f))
				{
					canSupportDefaultDepthStencil = true;
					texPFormat = (OgPixelFormat)f;
					texRTFormat = desiredFormat[i];
					break;
				}
			}

			if (!canSupportDefaultDepthStencil) LOGE(OG_ID, "Can't Support Depth Stencil Format on this platform");
		}
		else
		{
			texPFormat = OgFormatSupplement::GetPixelFormat(sw.settingInfo.depthBufferFormat);
			texRTFormat = sw.settingInfo.depthBufferFormat;
		}

		createDepthStencilBuffer = true;
		texUsage = OgTextureUsage::DEPTH_STENCIL_ATTACHMENT;
	}
	else if (sw.settingInfo.useDepthBuffer)
	{
		if (OgFormatSupplement::IsDepthFormat(sw.settingInfo.depthBufferFormat) == false)
			LOGE(OG_ID, "Wrong Depth Buffer Format");

		if (sw.settingInfo.depthBufferFormat == OgRenderTextureFormat::DEFAULT_DEPTH)
		{
			const int DESIRED_FORMAT_NUMB = 3;

			// Find Default Depth 
			// 32 -> 24 -> 16
			OgRenderTextureFormat desiredFormat[DESIRED_FORMAT_NUMB] =
			{
				OgRenderTextureFormat::DEPTH32,
				OgRenderTextureFormat::DEPTH24,
				OgRenderTextureFormat::DEPTH16,
			};

			bool canSupportDefaultDepth = false;
			for (int i = 0; i < DESIRED_FORMAT_NUMB; ++i)
			{
				VkFormat f = (VkFormat)OgFormatSupplement::GetPixelFormat(desiredFormat[i]);

				if (vkIsSupportFormat(_gpuDeviceVK, f))
				{
					canSupportDefaultDepth = true;
					texPFormat = (OgPixelFormat)f;
					texRTFormat = desiredFormat[i];
					break;
				}
			}

			if (!canSupportDefaultDepth) LOGE(OG_ID, "Can't Support Depth Format on this platform");
		}
		else
		{
			texPFormat = OgFormatSupplement::GetPixelFormat(sw.settingInfo.depthBufferFormat);
			texRTFormat = sw.settingInfo.depthBufferFormat;
		}

		createDepthStencilBuffer = true;
		texUsage = OgTextureUsage::DEPTH_ATTACHMENT;
	}
	else if (sw.settingInfo.useStencilBuffer)
	{
		if (OgFormatSupplement::IsStencilFormat(sw.settingInfo.stencilBufferFormat) == false)
			LOGE(OG_ID, "Wrong Stencil Buffer Format");

		createDepthStencilBuffer = true;
		texUsage = OgTextureUsage::STENCIL_ATTACHMENT;
		texPFormat = OgFormatSupplement::GetPixelFormat(sw.settingInfo.stencilBufferFormat);
		texRTFormat = sw.settingInfo.stencilBufferFormat;
	}

	if (vkIsSupportFormat(_gpuDeviceVK, (VkFormat)texPFormat) == false)
		LOGE(OG_ID, "This hardware does not support the format %d", texRTFormat);


	sw.frameBufferObject.hasDepthStencilBuffer = createDepthStencilBuffer;
	sw.frameBufferObject.hasMSAAbuffer = sw.settingInfo.useMSAA;

	OgAttachment colorAttachment;
	colorAttachment.isDepthStencilAttachment = false;
	colorAttachment.format = OgFormatSupplement::GetRenderTextureFormat((OgPixelFormat)sw.swapchainVK.colorFormat);

	// MSAA 를 이용한다면 기존에 colorAttachment로 이용하던 것을 multisample용 attachment로 이용한다.
	if (sw.settingInfo.useMSAA) colorAttachment.sampleCount = sw.settingInfo.msaaSampleCount;

	OgRenderPassInfo rpInfo;
	rpInfo.isSwapchainRenderPass = true;
	rpInfo.outputColorAttachments = &colorAttachment;
	rpInfo.outputColorAttachmentCount = 1;

	if (createDepthStencilBuffer)
	{
		OgSamplerInfo samplerInfo;
		samplerInfo.type = OgSamplerType::TEX_2D;
		samplerInfo.addressU = OgSamplerAddressMode::CLAMP_TO_EDGE;
		samplerInfo.addressV = OgSamplerAddressMode::REPEAT;
		OgSamplerHandle* sampler = CreateSampler(samplerInfo);
		sw.frameBufferObject.depthStencilSampler = sampler;

		texUsage = texUsage | OgTextureUsage::GPU_LOCAL;

		OgTextureInfo texInfo;
		texInfo.extent.width = window->width;
		texInfo.extent.height = window->height;

		texInfo.usage = texUsage;
		texInfo.format = texPFormat;

		// MSAA를 위해서는 depth attachment도 sample 수를 multisampl attachment로 맞춰줘야 한다.
		if (sw.settingInfo.useMSAA)
			texInfo.samples = sw.settingInfo.msaaSampleCount;

		OgTextureHandle* depthTex = CreateTexture(nullptr, texInfo, sampler);
		sw.frameBufferObject.depthStencilTexture = depthTex;

		OgAttachment depthStencilAttachment;
		depthStencilAttachment.isDepthStencilAttachment = true;
		depthStencilAttachment.format = texRTFormat;
		if (sw.settingInfo.useMSAA)
			depthStencilAttachment.sampleCount = sw.settingInfo.msaaSampleCount;

		rpInfo.useDepthStencilAttachment = true;
		rpInfo.outputDepthStencilAttachment = depthStencilAttachment;
	}

	if (sw.settingInfo.useMSAA)
	{
		//OgAttachment resolveColorAttachment;
		//resolveColorAttachment.isDepthStencilAttachment = false;
		//resolveColorAttachment.format = OgFormatSupplement::GetRenderTextureFormat((OgPixelFormat)sw.swapchainVK.colorFormat);

		//OgSamplerInfo samplerInfo;
		//samplerInfo.type = OgSamplerType::TEX_2D;
		//samplerInfo.addressU = OgSamplerAddressMode::CLAMP_TO_EDGE;
		//samplerInfo.addressV = OgSamplerAddressMode::CLAMP_TO_EDGE;
		//OgSamplerHandle* sampler = CreateSampler(samplerInfo);
		//sw.frameBufferObject.multisampleColorSampler = sampler;

		//texUsage = OgTextureUsage::COLOR_ATTACHMENT | OgTextureUsage::GPU_LOCAL;

		//OgTextureInfo texInfo;
		//texInfo.extent.width = window->width;
		//texInfo.extent.height = window->height;

		//texInfo.usage = texUsage;

		//texInfo.format = static_cast<OgPixelFormat>(sw.swapchainVK.colorFormat);
		//texInfo.samples = sw.settingInfo.msaaSampleCount;

		//OgTextureHandle* multisampleTexture = CreateTexture(nullptr, texInfo, sampler);
		//sw.frameBufferObject.multisampleColorTexture = multisampleTexture;

		//rpInfo.resolveColorAttachmentCount = 1;
		//rpInfo.resolveColorAttachment = &resolveColorAttachment;
	}

	sw.frameBufferObject.renderPass = CreateRenderPass(rpInfo);

	VkImageView depthImageView = createDepthStencilBuffer ? static_cast<OgTextureVK*>(sw.frameBufferObject.depthStencilTexture)->view : NULL;
	VkImageView multisampleImageView = sw.settingInfo.useMSAA ? static_cast<OgTextureVK*>(sw.frameBufferObject.multisampleColorTexture)->view : NULL;

	VkRenderPass swapchainRenderPass = static_cast<OgRenderPassVK *>(sw.frameBufferObject.renderPass)->renderPassVK;

	OgDefaultFrameBufferVK** framebuffers = new OgDefaultFrameBufferVK *[sw.swapchainVK.imageCount];
	for (uint32_t i = 0; i < sw.swapchainVK.imageCount; ++i)
	{
		// LvDefaultFrameBufferVK의 생성자에서 info.useMSAA flag를 확인하고 MSAA를 사용하지 않는다면 내부적으로 이에 맞춰서 default framebuffer를 구성한다.
		framebuffers[i] = new OgDefaultFrameBufferVK(_logicalDeviceVK, sw.settingInfo.useMSAA, createDepthStencilBuffer, window->width, window->height, multisampleImageView, sw.swapchainVK.buffers[i].view, depthImageView, swapchainRenderPass);
		framebuffers[i]->name = "Swapchain_framebuffer";

		OgTextureVK* tex = new OgTextureVK(sw.swapchainVK.buffers[i].image, sw.swapchainVK.buffers[i].view);
		framebuffers[i]->framebufferInfo.colorBuffers.Add(tex);
	}

	sw.frameBufferObject.frameBuffers = (OgFrameBufferHandle**)framebuffers;
	sw.frameBufferObject.isInitialized = true;

	sw.swapchainResult.bufferCount = sw.swapchainVK.imageCount;
	sw.swapchainResult.useDepthBuffer = sw.settingInfo.useDepthBuffer;
	sw.swapchainResult.useStencilBuffer = sw.settingInfo.useStencilBuffer;
	sw.swapchainResult.colorRenderFormat = colorAttachment.format;
	sw.swapchainResult.colorPixelFormat = (OgPixelFormat)sw.swapchainVK.colorFormat;
	sw.swapchainResult.depthRenderFormat = texRTFormat;
	sw.swapchainResult.depthPixelFormat = texPFormat;
	sw.swapchainResult.stencilRenderFormat = texRTFormat;
	sw.swapchainResult.stencilPixelFormat = texPFormat;
}

void OgRenderContextVulkan::initSwapChainSyncObject(SwapchainWrapper& sw)
{
	if (sw.frameBufferObject.isInitialized == false) return;
	if (sw.syncObject.isInitialized == true) return;

	// https://github.com/krOoze/Hello_Triangle/issues/1
	// Vulkan의 Presentation 로직은 아직 완벽하지 않다
	// 그래서 krOoze의 해석을 따라가도록 하는 것이 가장 안전하다.
	constexpr uint32 MAX_SUBMIT_COUNT_WHICH_MAKES_SENSE = 2;
	sw.syncObject.fences = new VkFence[MAX_SUBMIT_COUNT_WHICH_MAKES_SENSE];
	sw.syncObject.imageReadys = new VkSemaphore[MAX_SUBMIT_COUNT_WHICH_MAKES_SENSE];

	// per https://github.com/KhronosGroup/Vulkan-Docs/issues/1150 need upto swapchain-image count
	sw.syncObject.renderDones = new VkSemaphore[sw.swapchainVK.imageCount];

	VkFenceCreateInfo fCI = {};
	fCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkSemaphoreCreateInfo spCI = {};
	spCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	for (uint32 i = 0; i < MAX_SUBMIT_COUNT_WHICH_MAKES_SENSE; ++i)
	{
		VkFence* f = &(sw.syncObject.fences[i]);
		VK_CHECK_RESULT(vkCreateFence(_logicalDeviceVK, &fCI, nullptr, f));

		VkSemaphore* ps = &(sw.syncObject.imageReadys[i]);
		VK_CHECK_RESULT(vkCreateSemaphore(_logicalDeviceVK, &spCI, nullptr, ps));
	}

	for (uint32 i = 0; i < sw.swapchainVK.imageCount; ++i)
	{
		VkSemaphore* ps = &(sw.syncObject.renderDones[i]);
		VK_CHECK_RESULT(vkCreateSemaphore(_logicalDeviceVK, &spCI, nullptr, ps));
	}

	sw.syncObject.submissionIndex = SUBMISSION_INDEX_NONE;
	sw.syncObject.isInitialized = true;
}


void OgRenderContextVulkan::Init(void)
{
	initCommandPool();
	initStagingCommandBuffer();
	initDescriptorPool();
	
	// Swapchain Wrapper Class setting
	OgSwapChainVulkan::Connect(_instance, _vulkanDevice);
	
	this->_rootSwapchainWrapper = nullptr;
}

OgSwapChain* OgRenderContextVulkan::CreateSwapchain(OgNativeWindow* nativeWindow, const OgSwapChainInfo& swapchainInfo)
{
	OG_CHECK(nativeWindow != nullptr, "Native Window is nullptr");

	SwapchainWrapper* sw = new SwapchainWrapper();
	
	sw->next = nullptr;
	sw->window = nativeWindow;
	sw->settingInfo = swapchainInfo;

	SwapchainWrapper** swLink = &_rootSwapchainWrapper;
	while (*swLink != nullptr)
	{
		swLink = &((*swLink)->next);
	}
	*swLink = sw;

	prepareSwapChain(*sw);
	initSwapChain(*sw);
	initSwapChainSyncObject(*sw);

	
	uint32 hashKey = System::PointerHash(&sw->swapchainResult);
	_swapChainTables.insert(std::make_pair(hashKey, sw));
	

	return &sw->swapchainResult;
}


void OgRenderContextVulkan::destroySwapChainFramebuffers(SwapchainWrapper& sw)
{
	if (sw.frameBufferObject.isInitialized)
	{
		OgDefaultFrameBufferVK** fbs = (OgDefaultFrameBufferVK**)sw.frameBufferObject.frameBuffers;

		for (size_t i = 0; i < sw.frameBufferObject.bufferCount; ++i)
		{
			OgDefaultFrameBufferVK* fb = fbs[i];

			for (size_t i = 0; i < fb->framebufferInfo.colorBuffers.Size(); ++i)
			{
				delete fb->framebufferInfo.colorBuffers[i];
			}

			delete fb;
		}

		delete[] fbs;

		DestroyRenderPass(sw.frameBufferObject.renderPass);

		if (sw.frameBufferObject.hasDepthStencilBuffer)
		{
			DestroyTexture(sw.frameBufferObject.depthStencilTexture);
			DestroySampler(sw.frameBufferObject.depthStencilSampler);
		}

		if (sw.frameBufferObject.hasMSAAbuffer)
		{
			DestroyTexture(sw.frameBufferObject.multisampleColorTexture);
			DestroySampler(sw.frameBufferObject.multisampleColorSampler);
		}

		sw.frameBufferObject.isInitialized = false;

#if defined(__ANDROID__)
		sw.swapchainVK.Cleanup();
#endif
	}
}

void OgRenderContextVulkan::destroySwapChainSyncObject(SwapchainWrapper& sw)
{
	if (sw.syncObject.isInitialized)
	{
		for (uint32 i = 0; i < sw.swapchainVK.imageCount; ++i)
		{
			vkDestroySemaphore(_logicalDeviceVK, sw.syncObject.renderDones[i], nullptr);
		}

		for (uint32 i = 0; i < this->maxSubmitCount; ++i)
		{
			vkDestroySemaphore(_logicalDeviceVK, sw.syncObject.imageReadys[i], nullptr);
			vkDestroyFence(_logicalDeviceVK, sw.syncObject.fences[i], nullptr);
		}

		delete[] sw.syncObject.fences;
		delete[] sw.syncObject.imageReadys;
		delete[] sw.syncObject.renderDones;

		sw.syncObject.isInitialized = false;
	}
}

void OgRenderContextVulkan::destroySwapChain(SwapchainWrapper& sw)
{
	OG_CHECK(sw.frameBufferObject.isInitialized == false, "SwapchainWrapper does not clear framebuffer object");
	OG_CHECK(sw.syncObject.isInitialized == false, "SwapchainWrapper does not clear sync object");

	while (!sw.encoderQueue.empty()) sw.encoderQueue.pop();
	sw.swapchainVK.Cleanup();

	SwapchainWrapper** prevSW = &_rootSwapchainWrapper;
	while (*prevSW != &sw)
	{
		prevSW = &((*prevSW)->next);
	}
	*prevSW = sw.next;

	uint32 swapchainHash = System::PointerHash(&sw.swapchainResult);
	_swapChainTables.erase(swapchainHash);
	delete &sw;
}


void OgRenderContextVulkan::DestroySwapchain(OgSwapChain* swapchain)
{
	OG_CHECK(swapchain != nullptr, "LvSwapChain is nullptr");

	
	uint32 swapchainHash = System::PointerHash(swapchain);

	SwapchainWrapper& sw = *(_swapChainTables[swapchainHash]);

	vkDeviceWaitIdle(_logicalDeviceVK);
	destroySwapChainSyncObject(sw);
	destroySwapChainFramebuffers(sw);
	destroySwapChain(sw);
}

OgFrameBufferHandle* OgRenderContextVulkan::GetSwapChainFrameBuffer(OgSwapChain* swapchain, uint32 index)
{
	OG_CHECK(swapchain != nullptr, "LvSwapChain is nullptr");

	uint32 swapchainHash = System::PointerHash(swapchain);

	OG_CHECK(_swapChainTables.find(swapchainHash) != _swapChainTables.end(), "This Native Window is not used");

	SwapchainWrapper& sw = *(_swapChainTables[swapchainHash]);

	if (index == SUBMISSION_INDEX_NONE) LOGE(OG_ID, "AcquireNextImageIndex() is not called");
	if (sw.frameBufferObject.isInitialized == false) LOGE(OG_ID, "Swapchain Framebuffers are not initialized");
	if (index < 0 || index >= sw.frameBufferObject.bufferCount) LOGE(OG_ID, "Framebuffer Array : Out of Range");

	// Notice on the internal class LvDefaultFrameBufferVK
	OgDefaultFrameBufferVK** fbs = (OgDefaultFrameBufferVK**)sw.frameBufferObject.frameBuffers;
	return (OgFrameBufferHandle*)fbs[index];
}

// Reference
// http://kylehalladay.com/blog/tutorial/2017/12/13/Custom-Allocators-Vulkan.html
// https://www.fasterthan.life/blog/2017/7/13/i-am-graphics-and-so-can-you-part-4-
// https://developer.arm.com/graphics/developer-guides/mali-gpu-best-practices?_ga=2.214472286.839459796.1551965165-452978138.1534305239
// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/memory_mapping.html
// https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer
// https://hps.ece.utexas.edu/people/ebrahimi/pub/agarwal_hpca16.pdf


uint32 OgRenderContextVulkan::AcquireNextImageIndex(OgSwapChain* swapchain)
{
	OG_CHECK(swapchain != nullptr, "LvSwapChain is nullptr");

	// for the case of minimization
	_acquireOnceForPresent = true;

	uint32 swapchainHash = System::PointerHash(swapchain);
	OG_CHECK(_swapChainTables.find(swapchainHash) != _swapChainTables.end(), "There is no SwapchainWrapper matching LvSwapChain.");

	SwapchainWrapper& sw = *(_swapChainTables[swapchainHash]);

	sw.syncObject.submissionIndex = (sw.syncObject.submissionIndex + 1) % this->maxSubmitCount;

	VK_CHECK_RESULT(vkWaitForFences(_logicalDeviceVK, 1, &sw.syncObject.fences[sw.syncObject.submissionIndex], VK_TRUE, UINT64_MAX));
	VK_CHECK_RESULT(vkResetFences(_logicalDeviceVK, 1, &sw.syncObject.fences[sw.syncObject.submissionIndex]));

	VkResult err = sw.swapchainVK.AcquireNextImage(sw.syncObject.imageReadys[sw.syncObject.submissionIndex], &sw.syncObject.swapchainIndex);

	if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
	{
		// Recreate Swapchain Only.
		sw.syncObject.submissionIndex = SUBMISSION_INDEX_NONE;
		while (!sw.encoderQueue.empty())
			sw.encoderQueue.pop();
		
		return SUBMISSION_INDEX_NONE;
	}
	else if (err == VK_ERROR_SURFACE_LOST_KHR)
	{
		sw.syncObject.submissionIndex = SUBMISSION_INDEX_NONE;
		while (!sw.encoderQueue.empty())
			sw.encoderQueue.pop();
		return SUBMISSION_INDEX_NONE;
	}
	else if (err != VK_SUCCESS)
	{
		sw.syncObject.submissionIndex = SUBMISSION_INDEX_NONE;
		while (!sw.encoderQueue.empty())
			sw.encoderQueue.pop();
		LOGE(OG_ID, "failed to acquire swap chain image");
	}

	return sw.syncObject.swapchainIndex;
}

uint32 OgRenderContextVulkan::GetCurrentImageIndex(OgSwapChain* swapchain)
{
	OG_CHECK(swapchain != nullptr, "LvSwapChain is nullptr");

	uint32 swapchainHash = System::PointerHash(swapchain);
	OG_CHECK(_swapChainTables.find(swapchainHash) != _swapChainTables.end(), "This Native Window is not used");

	SwapchainWrapper& sw = *(_swapChainTables[swapchainHash]);

	// To Error handling at host code.
	if (sw.syncObject.submissionIndex == SUBMISSION_INDEX_NONE)
	{
		LOGD(OG_ID, "You must call AcquireNextImageIndex() before using GetCurrentImageIndex()");
		return SUBMISSION_INDEX_NONE;
	}
	return sw.syncObject.swapchainIndex;
}

OgBufferHandle* OgRenderContextVulkan::CreateBuffer(void* data, size_t size, OgBufferUsage usage, OgMemoryOption option)
{
	OgBufferVK* r = nullptr;
	VkBufferUsageFlagBits vkUsage = static_cast<VkBufferUsageFlagBits>(usage);

	if (usage == OgBufferUsage::UNIFORM)
		r = new OgUniformBufferVK(*_vulkanDevice, (uint32)size, usage, option);
	else
		r = new OgBufferVK(*_vulkanDevice, (uint32)size, usage, option);

	// PropertyFlag 설명.

	// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT 
	// 호스트에서 액세스 가능한 가상 메모리 어드레스 노출을 vkMapMemory 를 사용해서 할 수 있다.

	// VK_MEMORY_PROPERTY_HOST_CACHED_BIT
	// 캐시된 메모리를 사용할지 결정한다. 이 타입은 호스트에서 액세스 할 때나 읽어올 때 빠르다.
	// 하지만, 캐시되지 않은 메모리는 일관성이 유지된다.

	// VK_MEMORY_PROPERTY_HOST_COHERENT_BIT 
	// 캐시 관리 커맨드 (vkFlushMappedMemoryRanges, vkInvalidateMappedMemoryRanges) 가 필요 없이
	// 자동으로 일관성이 보장된다.

	switch (option)
	{
		// Cached, incoherent
		// Read Faster
		// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/memory_mapping.html
	case OgMemoryOption::MAP_MANAGED:
	{
		r->Build(vkUsage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_CACHED_BIT, size, data);
		break;
	}
	case OgMemoryOption::PRIVATE_GPU:
	{
		// Create device local buffers
		VK_CHECK_RESULT(_vulkanDevice->CreateBuffer(
			vkUsage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			size,
			&r->bufferVK,
			&r->memoryVK));

		// Get a temporary staging buffer
		
		if (data != nullptr)
		{
			OgBufferVK* ref = nullptr;
			ref = new OgBufferVK(*_vulkanDevice, size, usage, OgMemoryOption::MAP_MANAGED);
			ref->Build
			(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
				size,
				data
			);

			VkCommandBuffer copyCmd = _stagingCommandBuffer[_stagingSubmitIndex];
			VkBufferCopy copyRegion = {};
			copyRegion.size = size;
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0; 

			vkCmdCopyBuffer(
				copyCmd,
				ref->bufferVK,
				r->bufferVK,
				1,
				&copyRegion);

			constexpr VkAccessFlags srcAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
			VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
			VkAccessFlags dstAccess = VK_ACCESS_MEMORY_READ_BIT;

			switch (usage)
			{
			case OgBufferUsage::UNIFORM:
			{
				dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
				dstAccess = VK_ACCESS_UNIFORM_READ_BIT;
				break;
			}
			case OgBufferUsage::INDEX:
			{
				dstStageMask = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
				dstAccess = VK_ACCESS_INDEX_READ_BIT;
				break;
			}
			case OgBufferUsage::VERTEX:
			{
				dstStageMask = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
				dstAccess = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
				break;
			}
			}

			CommandpipelineBarrierForBufferUpdate(
				copyCmd,
				r->bufferVK,
				0,
				size,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				dstStageMask,
				srcAccess,
				dstAccess
			);

			ref->Destroy();
		}
		break;

	}
	// https://www.khronos.org/opengl/wiki/Buffer_Object#Persistent_mapping
	}

#if defined(_DEBUG)
	r->instanceType = "Buffer";
	_livingObjects.Add(r);
#endif
	return r;
}

void OgRenderContextVulkan::DestroyBuffer(OgBufferHandle* buffer)
{
	if (buffer == nullptr) LOGE(OG_ID, "LvBuffer is nullptr");

	OgBufferVK* b = static_cast<OgBufferVK*>(buffer);

	b->Unmap();

#if defined(_DEBUG)
	_livingObjects.Remove(b);
#endif

	if (b->usage == OgBufferUsage::UNIFORM)
	{
		OgUniformBufferVK* ref = static_cast<OgUniformBufferVK*>(b);
		ref->Destroy();
		delete ref;
	}
	else
	{
		b->Destroy();
		delete b;
	}
}

OgShaderHandle* OgRenderContextVulkan::CreateShader(OgShaderType flag, const char* text, uint32 codeSize, const char* funcName )
{
	OgShaderVK* shader = new OgShaderVK();
	{
		VkShaderModuleCreateInfo moduleCreateInfo{};
		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.codeSize = codeSize;
		moduleCreateInfo.pCode = (uint32_t*)text;
		VK_CHECK_RESULT(vkCreateShaderModule(_logicalDeviceVK, &moduleCreateInfo, NULL, &shader->shaderModuleVK));
	}

	shader->shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader->shaderStageInfo.stage = static_cast<VkShaderStageFlagBits>(flag);

	if (funcName == nullptr)
		shader->shaderStageInfo.pName = "main";
	else
		shader->shaderStageInfo.pName = funcName;

	shader->shaderStageInfo.module = shader->shaderModuleVK;

#if defined(_DEBUG)
	shader->instanceType = "Shader";
	_livingObjects.Add(shader);
#endif

	return shader;
}


void OgRenderContextVulkan::DestroyShader(OgShaderHandle* shader)
{
	OgShaderVK* s = static_cast<OgShaderVK*>(shader);
	vkDestroyShaderModule(_logicalDeviceVK, s->shaderModuleVK, nullptr);

	
#if defined(_DEBUG)
	_livingObjects.Remove(s);
#endif
	delete s;
}

OgProgramHandle* OgRenderContextVulkan::CreateProgram(OgShaderHandle** shaders, uint32 shaderCount)
{
	return new OgProgramHandle();
}
void OgRenderContextVulkan::DestroyProgram(OgProgramHandle* handle)
{
	delete handle;
}


void OgRenderContextVulkan::buildTexture(OgTextureVK* texture)
{
	OgTextureInfo& info = texture->info;
	const bool isGPULocal = (info.usage & OgTextureUsage::GPU_LOCAL) != 0;
	const bool useStaging = (info.usage & OgTextureUsage::STAGING) != 0;
	if (useStaging && texture->data == nullptr)
	{
		LOGE(OG_ID, "Don't Use Texture Staging without Data");
	}

	// Tiling Description : https://lifeisforu.tistory.com/410
	// Vulkan 24bit format is not supported : https://www.reddit.com/r/vulkan/comments/4w0w8o/why_doesnt_vulkan_support_24bit_image_formats/
	VkFormatProperties formatProps;
	vkGetPhysicalDeviceFormatProperties(_gpuDeviceVK, texture->vkFormat, &formatProps);
	VkImageTiling tiling = VkImageTiling::VK_IMAGE_TILING_MAX_ENUM;
	VkFormatFeatureFlags tilingFeature;

	if ((formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0)
	{
		tiling = VkImageTiling::VK_IMAGE_TILING_LINEAR;
		tilingFeature = formatProps.linearTilingFeatures;
	}

	if ((formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0)
	{
		tiling = VkImageTiling::VK_IMAGE_TILING_OPTIMAL;
		tilingFeature = formatProps.optimalTilingFeatures;
	}
	else
	{
		LOGE(OG_ID, "Fatal : Optimal Texture Tiling not Supported");
	}

	if (tiling == VK_IMAGE_TILING_MAX_ENUM)
	{
		LOGE(OG_ID, "LvPixelFormat = %i is not supported", info.format);
	}

	// mipmap generation시 vkCmdBlitImage 조건 체크
	OG_CHECK
	(
		(info.isGenerateMipmaps == true && texture->data != nullptr)
		?
		// mipmap 생성시에, vkCmdBlitImage 되는지 체크
		(
		((tilingFeature & VK_FORMAT_FEATURE_BLIT_SRC_BIT) != 0) &&
			((tilingFeature & VK_FORMAT_FEATURE_BLIT_DST_BIT) != 0) &&

			// filter가 linear라면 그걸 지원하는지 체크
			(texture->sampler->info.mipmapMode == OgSamplerMipmapMode::LINEAR ?
			((tilingFeature & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) != 0)
				: true	// texture->sampler->info.mipmapMode == LvSamplerMipmapMode::LINEAR
				)
			)
		:
		true, // (info.generateMipmaps == true && texture->data != nullptr)
		"Hardware does not support Blitting on this format"
	);

	// Create Image
	VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageInfo.imageType = static_cast<VkImageType>(info.type);
	imageInfo.format = texture->vkFormat;
	imageInfo.mipLevels = info.mipLevels;
	imageInfo.arrayLayers = info.arrayLayers;
	imageInfo.samples = static_cast<VkSampleCountFlagBits>(info.samples);
	imageInfo.tiling = tiling;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.extent = { info.extent.width, info.extent.height, info.extent.depth };
	imageInfo.usage = GetVkImageUsageFlags(info.usage);

	// Staging option
	if (useStaging) imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	// Mipmap option
	if (info.isGenerateMipmaps)
	{
		info.mipLevels = (uint32)floorf(log2f(OG_MAX(info.extent.width, OG_MAX(info.extent.height, info.extent.depth)))) + 1;
		imageInfo.mipLevels = info.mipLevels;
		imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	// 한 COMPATIBLE_BIT에 대한 문서 참조.
	switch (info.viewType)
	{
	case OgTextureViewType::TEX_CUBE:
	{
		imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		break;
	}
	default:
		break;
	}

	VK_CHECK_RESULT(vkCreateImage(_logicalDeviceVK, &imageInfo, nullptr, &texture->image));
	VkMemoryAllocateInfo memAllocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	VkMemoryRequirements memReqs = {};
	vkGetImageMemoryRequirements(_logicalDeviceVK, texture->image, &memReqs);
	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = _vulkanDevice->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(_logicalDeviceVK, &memAllocInfo, nullptr, &texture->memory));
	VK_CHECK_RESULT(vkBindImageMemory(_logicalDeviceVK, texture->image, texture->memory, 0));

	// Create image view를 하기 위한 옵션 체크

	// aspectMask 결정
	VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	bool isDepthorStencil = false;
	bool isDepthOnly = false;
	bool isStencilOnly = false;
	bool isDepthStencilOnly = false;
	if (OgFormatSupplement::IsDepthFormat(texture->info.format))
	{
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		isDepthorStencil = true;
		isDepthOnly = true;
	}
	else if (OgFormatSupplement::IsStencilFormat(texture->info.format))
	{
		aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
		isDepthorStencil = true;
		isStencilOnly = true;
	}
	else if (OgFormatSupplement::IsDepthStencilFormat(texture->info.format))
	{
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		isDepthorStencil = true;
		isDepthStencilOnly = true;
	}
	OG_CHECK(info.viewType == OgTextureViewType::TEX_CUBE ? !(isDepthStencilOnly || isDepthOnly || isStencilOnly || isDepthStencilOnly) : true, "%s",
		"Can't use Tex Cube as depth stencil format");

	/*
		enum class LvTextureUsage : uint32_t
		{
			GPU_LOCAL
			STAGING
			SAMPLED
			STORAGE -> TODO : research 해야함
			COLOR_ATTACHMENT
			DEPTH_ATTACHMENT
			STENCIL_ATTACHMENT
			DEPTH_STENCIL_ATTACHMENT
			TRANSIENT_ATTACHMENT
		};

		SAMPLED -> Vulkan Read가 붙어야함
		DEPTH/STENCIL/DEPTH_STNECIl Attachment | SAMPLED-> DEPTH READ / STENCIL READ / DEPTH_STENCIL READ

		Layout 확인 순서 (아래로 내려갈수록 더 많은 범위에 쓰일 수 있음)
		0. GPU LOCAL -> LAYOUT_UNDEFINED
		1. Attachment 전용 -> Attachment 전용 Layout
		2. Sample 전용 -> Attachment + Sample 가능 Layout
		3. Staging -> Staging buffer를 통해, UNDEFINED -> TRANSFER_DST -> (1 또는 2에서 결정된 Layout)
	*/
	texture->imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;	// GPU_LOCAL
	bool isAttachment = IsAttachmentUsage(info.usage);
	OG_CHECK(info.viewType == OgTextureViewType::TEX_CUBE ? !isAttachment : true, "%s", "Can't use Tex Cube as an attachment");
	OG_CHECK(info.viewType == OgTextureViewType::TEX_2D_ARRAY ? !isAttachment : true, "%s", "Can't use Tex Array as an attachment");
	if (isAttachment == true)
	{
		if ((info.usage & OgTextureUsage::COLOR_ATTACHMENT) != 0) texture->imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		if (isDepthorStencil) texture->imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	if ((info.usage & OgTextureUsage::SAMPLED) != 0) //SAMPLED
	{
		// Sampling한다면 attachment/shader read 둘 다로 쓰일 수 있는 layout으로
		texture->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		// TODO !!! : Spec에 맞추어 sample할 때의 최적의 image layout 고치기

		// depth/stencil 처리
		if (isDepthOnly) texture->imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		else if (isStencilOnly) texture->imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		else if (isDepthStencilOnly) texture->imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	}


	VkImageViewCreateInfo view{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	view.viewType = static_cast<VkImageViewType>(info.viewType);
	view.format = texture->vkFormat;
	view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	view.subresourceRange.aspectMask = aspectMask;
	view.subresourceRange.baseMipLevel = 0;
	view.subresourceRange.levelCount = (useStaging || info.isGenerateMipmaps) ? info.mipLevels : 1;
	view.subresourceRange.baseArrayLayer = 0;
	view.subresourceRange.layerCount = info.arrayLayers;
	view.image = texture->image;
	VK_CHECK_RESULT(vkCreateImageView(_logicalDeviceVK, &view, nullptr, &texture->view));
	
	// GPULocal이 아니라면, 다른 곳에서 Sampling 될 수 있다는 것이기에, image layout을 미리 변환해준다.
	// Sampling하려면, shader read할 수 있는 layout이여야 하기 때문이다.
	// GPULocal이라면, RenderPass에서 Undefined -> any Layout으로 바꾸기 때문에 크게 상관없다.
	// GPULocal의 예는, Framebuffer의 depth/stencil을 다른 곳에 안쓰는 경우 이다. 이 때는 layout을 안바꾸고 
	// RenderPass에 의해서만 하면 된다.
	if (isGPULocal == false && useStaging == false) // Render Target
	{
		OG_CHECK(info.isGenerateMipmaps == false, "%s", "Not Implemented yet on generating mipmap on a rendertarget");

		
		VkCommandBuffer transitionCmd = beginSingleTimeCommands();
		vkSetImageLayout
		(
			transitionCmd,
			texture->image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			texture->imageLayout,
			view.subresourceRange,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
		);

		endSingleTimeCommands(transitionCmd);
	}

	if (useStaging) // Sampling Image
	{
		// texture의 쓰임용도에 따라 고도화가 필요하다. 지금은 단지 2차원의 텍셀버퍼만을 위해서 작업해두었음.
		
		OgBufferVK* ref = nullptr;
		ref = new OgBufferVK(*_vulkanDevice, info.byteSize, OgBufferUsage::UNIFORM, OgMemoryOption::MAP_MANAGED);
		ref->Retain();
		ref->Build
		(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
			info.byteSize,
			nullptr
		);

		void* stagingBufferPtr = (uint8*)(ref->mapped);
		if (info.viewType == OgTextureViewType::TEX_2D_ARRAY)
		{
			uint size = info.extent.width * info.extent.height * OgFormatSupplement::GetSizeInBytes(info.format);
			unsigned char* offset = (unsigned char*)stagingBufferPtr;

			for (uint i = 0; i < info.arrayLayers; ++i)
			{
				memcpy((void*)offset, texture->data[i], size);
				offset += size;
			}
			ref->Flush();
		}
		else
		{
			if (texture->data[0])
			{
				memcpy(stagingBufferPtr, texture->data[0], info.byteSize);
				ref->Flush();
			}

		}

		VkCommandBuffer copyCmd = beginSingleTimeCommands();

		vkSetImageLayout(
			copyCmd,
			texture->image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			view.subresourceRange,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT);

		switch (info.viewType)
		{
		case OgTextureViewType::TEX_CUBE:
		{
			const int OneTextureSize = info.extent.width * info.extent.height * OgFormatSupplement::GetSizeInBytes(info.format);
			uint32_t offset = 0;

			OgVector<VkBufferImageCopy> bufferCopyRegions;
			bufferCopyRegions.Reserve(6);
			for (uint32_t face = 0; face < 6; face++)
			{
				VkBufferImageCopy bufferCopyRegion = {};
				bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				bufferCopyRegion.imageSubresource.mipLevel = 0;
				bufferCopyRegion.imageSubresource.baseArrayLayer = face;
				bufferCopyRegion.imageSubresource.layerCount = 1;
				bufferCopyRegion.imageExtent.width = info.extent.width;
				bufferCopyRegion.imageExtent.height = info.extent.height;
				bufferCopyRegion.imageExtent.depth = info.extent.depth;

				bufferCopyRegion.bufferOffset = offset;
				bufferCopyRegions.Add(bufferCopyRegion);

				// Increase offset into staging buffer for next face
				offset += OneTextureSize;
			}

			vkCmdCopyBufferToImage(
				copyCmd,
				ref->bufferVK,
				texture->image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				static_cast<uint32_t>(bufferCopyRegions.Size()),
				bufferCopyRegions.Data());
			break;
		} // if (TEX_CUBE)
		case OgTextureViewType::TEX_2D_ARRAY:
		{
			const int OneTextureSize = info.extent.width * info.extent.height * OgFormatSupplement::GetSizeInBytes(info.format);
			uint32_t offset = 0;

			OgVector<VkBufferImageCopy> bufferCopyRegions;
			bufferCopyRegions.Reserve(32);
			for (uint32_t face = 0; face < info.arrayLayers; face++)
			{
				VkBufferImageCopy bufferCopyRegion = {};
				bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				bufferCopyRegion.imageSubresource.mipLevel = 0;
				bufferCopyRegion.imageSubresource.baseArrayLayer = face;
				bufferCopyRegion.imageSubresource.layerCount = 1;
				bufferCopyRegion.imageExtent.width = info.extent.width;
				bufferCopyRegion.imageExtent.height = info.extent.height;
				bufferCopyRegion.imageExtent.depth = info.extent.depth;

				bufferCopyRegion.bufferOffset = offset;
				bufferCopyRegions.Add(bufferCopyRegion);

				// Increase offset into staging buffer for next face
				offset += OneTextureSize;
			}

			vkCmdCopyBufferToImage(
				copyCmd,
				ref->bufferVK,
				texture->image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				static_cast<uint32_t>(bufferCopyRegions.Size()),
				bufferCopyRegions.Data());
			break;
		}
		default:
		{
			// https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkBufferImageCopy.html
			// To copy both the depth and stencil aspects of a depth/stencil format, 
			// two entries in pRegions can be used, where one specifies the depth aspect in imageSubresource, and the other specifies the stencil aspect.
			OgVector<VkBufferImageCopy> regions;

			VkBufferImageCopy bufferCopyRegion;
			if (aspectMask & VK_IMAGE_ASPECT_COLOR_BIT)
			{
				bufferCopyRegion = {};
				bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
				bufferCopyRegion.imageSubresource.mipLevel = 0;
				bufferCopyRegion.imageSubresource.layerCount = 1;
				bufferCopyRegion.imageExtent.width = info.extent.width;
				bufferCopyRegion.imageExtent.height = info.extent.height;
				bufferCopyRegion.imageExtent.depth = info.extent.depth;

				bufferCopyRegion.bufferOffset = 0;

				regions.Add(bufferCopyRegion);
			}

			if (aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT)
			{
				bufferCopyRegion = {};
				bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
				bufferCopyRegion.imageSubresource.mipLevel = 0;
				bufferCopyRegion.imageSubresource.layerCount = 1;
				bufferCopyRegion.imageExtent.width = info.extent.width;
				bufferCopyRegion.imageExtent.height = info.extent.height;
				bufferCopyRegion.imageExtent.depth = info.extent.depth;

				bufferCopyRegion.bufferOffset = 0;

				regions.Add(bufferCopyRegion);
			}

			if (aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT)
			{
				VkBufferImageCopy bufferCopyRegion = {};
				bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
				bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
				bufferCopyRegion.imageSubresource.mipLevel = 0;
				bufferCopyRegion.imageSubresource.layerCount = 1;
				bufferCopyRegion.imageExtent.width = info.extent.width;
				bufferCopyRegion.imageExtent.height = info.extent.height;
				bufferCopyRegion.imageExtent.depth = info.extent.depth;

				bufferCopyRegion.bufferOffset = 0;

				regions.Add(bufferCopyRegion);
			}

			vkCmdCopyBufferToImage(
				copyCmd,
				ref->bufferVK,
				texture->image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				static_cast<uint32_t>(regions.Size()), regions.Data());
			break;
		} // else (Tex 2D)
		}

		if (info.isGenerateMipmaps)
		{
			// TODO
			// buildMipmap(copyCmd, texture, view.subresourceRange);
			vkSetImageLayout(
				copyCmd,
				texture->image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				texture->imageLayout,					// image가 쓰일 최종 layout
				view.subresourceRange,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		}
		else
		{
			// Mipmap 생성 안하므로 최종 layout으로 바꿔주기
			vkSetImageLayout(
				copyCmd,
				texture->image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				texture->imageLayout,					// image가 쓰일 최종 layout
				view.subresourceRange,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		}

		endSingleTimeCommands(copyCmd);
		ref->Destroy();
	}

	// TODO : Research on Linear Tiliing.
	// CreateMappableTexture(tex, info, aspectMask, isDepthorStencil);
}
void OgRenderContextVulkan::releaseTexture(OgTextureVK* texture)
{
	if (texture->view != VK_NULL_HANDLE) vkDestroyImageView(this->_logicalDeviceVK, texture->view, nullptr);
	if (texture->image != VK_NULL_HANDLE) vkDestroyImage(this->_logicalDeviceVK, texture->image, nullptr);
	if (texture->memory != VK_NULL_HANDLE) vkFreeMemory(this->_logicalDeviceVK, texture->memory, nullptr);
}

OgTextureHandle* OgRenderContextVulkan::CreateTexture(void* image, OgPixelFormat format, uint32 width, uint32 height, OgSamplerHandle* sampler , bool generateMipmaps )
{
	OgTextureInfo info;
	info.usage = OgTextureUsage::SAMPLED | OgTextureUsage::STAGING;
	info.viewType = OgTextureViewType::TEX_2D;
	info.format = format;
	info.extent.width = width;
	info.extent.height = height;
	info.extent.depth = 1;
	info.byteSize = width * height * OgFormatSupplement::GetSizeInBytes(format);
	info.isGenerateMipmaps = generateMipmaps;

	return CreateTexture((void**)&image, info, sampler);
}
OgTextureHandle* OgRenderContextVulkan::CreateTexture(void** image, OgPixelFormat format, uint32 width, uint32 height, uint32 layerCount, OgSamplerHandle* sampler, bool generateMipmaps )
{
	OgTextureInfo info;
	info.usage = OgTextureUsage::SAMPLED | OgTextureUsage::STAGING;
	info.viewType = OgTextureViewType::TEX_2D_ARRAY;
	info.format = format;
	info.extent.width = width;
	info.extent.height = height;
	info.arrayLayers = layerCount;
	info.byteSize = width * height * layerCount * OgFormatSupplement::GetSizeInBytes(format);
	info.isGenerateMipmaps = generateMipmaps;

	return CreateTexture(image, info, sampler);
}
OgTextureHandle* OgRenderContextVulkan::CreateTexture(void** image, const OgTextureInfo& info, OgSamplerHandle* sampler )
{
	VkFormat format = static_cast<VkFormat>(info.format);
	if (vkIsSupportFormat(_gpuDeviceVK, format) == false)
		LOGE(OG_ID, "This hardware does not support this format %d", info.format);

	OgTextureVK* texture = new OgTextureVK(info, sampler, format, image);

	buildTexture(texture);

#if defined(_DEBUG)
	texture->instanceType = "Texture";
	_livingObjects.Add(texture);
#endif
	return texture;
}
void OgRenderContextVulkan::DestroyTexture(OgTextureHandle* texture) 
{
	if (texture == nullptr) LOGE(OG_ID, "LvTextureHandle is nullptr");

	OgTextureVK* t = static_cast<OgTextureVK*>(texture);

	// TODO
	// 현재 LvTextureUsage::STAGING 을 LvTextureUsage::GPU_PRIVATE의 의미로 사용하고 있는데 이것은 수정되어야한다.
	releaseTexture(t);

#if defined(_DEBUG)
	_livingObjects.Remove(texture);
#endif

	delete t;
}
void OgRenderContextVulkan::UpdateTexture(OgTextureHandle* texture, OgSamplerHandle* sampler, size_t offset, void** data, bool useBarrier)
{
	//TODO
}

OgSamplerHandle* OgRenderContextVulkan::CreateSampler(const OgSamplerInfo& info)
{
	// Create a texture sampler
	// In Vulkan textures are accessed by samplers
	// This separates all the sampling information from the texture data. This means you could have multiple sampler objects for the same texture with different settings
	// Note: Similar to the samplers available with OpenGL 3.3

	VkSamplerCreateInfo sampler{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	sampler.maxAnisotropy = info.maxAnisotropy;
	sampler.magFilter = static_cast<VkFilter>(info.magFilter);
	sampler.minFilter = static_cast<VkFilter>(info.minFilter);
	sampler.mipmapMode = static_cast<VkSamplerMipmapMode>(info.mipmapMode);
	sampler.unnormalizedCoordinates = info.coordinate != OgSamplerCoord::NORMALIZED;
	sampler.addressModeU = static_cast<VkSamplerAddressMode>(info.addressU);
	sampler.addressModeV = static_cast<VkSamplerAddressMode>(info.addressV);
	sampler.addressModeW = static_cast<VkSamplerAddressMode>(info.addressW);
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

	sampler.mipLodBias = 0.0f;
	sampler.minLod = 0.0f;
	if (sampler.unnormalizedCoordinates)
		sampler.maxLod = 0.0;
	else
		sampler.maxLod = 1000.0f; // http://devgit.com2us.com/TS/TPact/issues/72#note_2889
	sampler.compareEnable = info.isCompareEnable;
	sampler.compareOp = static_cast<VkCompareOp>(info.compareOp);

	/*
	// Enable anisotropic filtering
	// This feature is optional, so we must check if it's supported on the device
	if (vulkanDevice->features.samplerAnisotropy) {
	// Use max. level of anisotropy for this example
	sampler.maxAnisotropy = vulkanDevice->properties.limits.maxSamplerAnisotropy;
	sampler.anisotropyEnable = VK_TRUE;
	}
	else {
	// The device does not support anisotropic filtering
	sampler.maxAnisotropy = 1.0;
	sampler.anisotropyEnable = VK_FALSE;
	}
	*/
	sampler.anisotropyEnable = info.isAnisotropyEnable;

	OgSamplerVK* r = new OgSamplerVK();
	VK_CHECK_RESULT(vkCreateSampler(_logicalDeviceVK, &sampler, nullptr, &r->samplerVK));
	r->info = info;

#if defined(_DEBUG)
	r->instanceType = "Sampler";
	_livingObjects.Add(r);
#endif
	return r;
}
void OgRenderContextVulkan::DestroySampler(OgSamplerHandle* sampler)
{
	OG_CHECK(sampler != nullptr, "sampler pointer is nullptr");

	OgSamplerVK* s = (OgSamplerVK*)sampler;

	vkDestroySampler(_logicalDeviceVK, s->samplerVK, nullptr);

#if defined(_DEBUG)
	_livingObjects.Remove(sampler);
#endif

	delete s;
	sampler = nullptr;
}

OgFrameBufferHandle* OgRenderContextVulkan::CreateFrameBuffer(OgFrameBufferInfo& info)
{
	OgFrameBufferVK* r = new OgFrameBufferVK(_logicalDeviceVK, info);

#if defined(_DEBUG)
	r->instanceType = "Framebuffer";
	_livingObjects.Add(r);
#endif

	return r;
}
void OgRenderContextVulkan::DestroyFrameBuffer(OgFrameBufferHandle* framebuffer)
{
	if (framebuffer == nullptr) LOGE(OG_ID, "LvFrameBufferHandle is null");

	OgFrameBufferVK* f = static_cast<OgFrameBufferVK*>(framebuffer);

#if defined(_DEBUG)
	_livingObjects.Remove(f);
#endif

	delete f;
}

void OgRenderContextVulkan::buildRenderPass(OgRenderPassVK* r)
{
	OgRenderContextVulkan* renderContext = this;
	const OgRenderPassInfo& rInfo = r->info;

	VkImageLayout colorFinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VkImageLayout depthFinalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	if (rInfo.isSwapchainRenderPass)
	{
		colorFinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		depthFinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}

	int outputColorAttachmentCount = rInfo.outputColorAttachmentCount;
	
	int totalAttachmentCount = outputColorAttachmentCount;
	if (rInfo.useDepthStencilAttachment == true)  totalAttachmentCount++;

	// https://developer.android.com/ndk/guides/graphics/design-notes?hl=ko
	OgVector<VkAttachmentDescription> attachmentDescriptors;
	attachmentDescriptors.Resize(totalAttachmentCount);
	// MSAA 를 이용할 때, multisampleAttach -> resolveAttach -> depthAttach 순서로 구현되어 있다.
	for (int i = 0; i < outputColorAttachmentCount; ++i)
	{
		auto& each = rInfo.outputColorAttachments[i];
		vk_convert_format(renderContext->_gpuDeviceVK, attachmentDescriptors[i], each);
		attachmentDescriptors[i].samples = static_cast<VkSampleCountFlagBits>(each.sampleCount);
		attachmentDescriptors[i].loadOp = (VkAttachmentLoadOp)rInfo.outputColorAttachments[i].load;
		attachmentDescriptors[i].storeOp = (VkAttachmentStoreOp)rInfo.outputColorAttachments[i].store;
		attachmentDescriptors[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptors[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptors[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		if (rInfo.outputColorAttachments[0].load == OgRenderBufferLoadAction::LOAD)
		{

			// NOTE: load시에는 무조건 layout이 맞아야 제대로 load 할 수 있다.
			attachmentDescriptors[i].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}
		attachmentDescriptors[i].finalLayout = colorFinalLayout;
	}

	if (rInfo.useDepthStencilAttachment == true)
	{
		auto& depthDescriptor = attachmentDescriptors[totalAttachmentCount - 1];
		vk_convert_format(renderContext->_gpuDeviceVK, depthDescriptor, rInfo.outputDepthStencilAttachment);
		depthDescriptor.samples = static_cast<VkSampleCountFlagBits>(rInfo.outputDepthStencilAttachment.sampleCount);
		depthDescriptor.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthDescriptor.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthDescriptor.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		if (rInfo.outputDepthStencilAttachment.load == OgRenderBufferLoadAction::LOAD)
		{
			depthDescriptor.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}
		
		depthDescriptor.finalLayout = depthFinalLayout;
	}

	OgVector<VkAttachmentReference> colorReference;
	colorReference.Resize(outputColorAttachmentCount);
	for (int i = 0; i < colorReference.Size(); ++i)
	{
		auto& each = colorReference[i];
		each.attachment = i;
		each.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // VK_IMAGE_LAYOUT_GENERAL 
	}

	VkAttachmentReference depthReference;
	depthReference.attachment = totalAttachmentCount - 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// TO DO : VkSubpassDescription 를 여러 개 이용함으로써 resolveAttachment를 여러 개로 이용할 수 있도록 구현하는 것이 필요하다.
	// 현재는 마지막 colorAttachment가 resolveAttachment로 resolve 되게 구현되어 있다.(즉 1개 attachment만 resolve 가능)
	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.inputAttachmentCount = 0;
	subpassDescription.pInputAttachments = nullptr;
	subpassDescription.colorAttachmentCount = outputColorAttachmentCount;
	subpassDescription.pColorAttachments = (outputColorAttachmentCount > 0) ? &colorReference[colorReference.Size() - 1] : nullptr;
	subpassDescription.pResolveAttachments = nullptr;
	subpassDescription.pDepthStencilAttachment = rInfo.useDepthStencilAttachment ? &depthReference : nullptr;
	subpassDescription.preserveAttachmentCount = 0;
	subpassDescription.pPreserveAttachments = nullptr;

	OgVector<VkSubpassDependency> dependencies;
	dependencies.Resize(2);
	// srcSubpass and dstSubpass are the subpass indices of the producer and consumer subpasses
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// Create the actual renderpass
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptors.Size());
	renderPassInfo.pAttachments = attachmentDescriptors.Data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.Size());
	renderPassInfo.pDependencies = dependencies.Data();

	vkCreateRenderPass(renderContext->_logicalDeviceVK, &renderPassInfo, nullptr, &r->renderPassVK);
}

OgRenderPassHandle* OgRenderContextVulkan::CreateRenderPass(OgRenderPassInfo& info) 
{
	OgRenderPassVK* r = new OgRenderPassVK(info);

	buildRenderPass(r);

#if defined(_DEBUG)
	r->instanceType = "RenderPass";
	_livingObjects.Add(r);
#endif

	return r;
}

void OgRenderContextVulkan::releaseRenderPass(OgRenderPassVK* renderPass)
{
	if (renderPass->renderPassVK != NULL)
	{
		vkDestroyRenderPass(this->_logicalDeviceVK, renderPass->renderPassVK, nullptr);
	}
}
VkCommandBuffer OgRenderContextVulkan::beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = _cmdPoolVK;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(_logicalDeviceVK, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void OgRenderContextVulkan::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(_graphicsQueueVK, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(_graphicsQueueVK);

	vkFreeCommandBuffers(_logicalDeviceVK, _cmdPoolVK, 1, &commandBuffer);
}

void OgRenderContextVulkan::DestroyRenderPass(OgRenderPassHandle* renderPass)
{
	if (renderPass == nullptr) LOGE(OG_ID, "LvRenderPassHandle is nullptr");

	OgRenderPassVK* r = static_cast<OgRenderPassVK*>(renderPass);

	releaseRenderPass(r);

#if defined(_DEBUG)
	_livingObjects.Remove(r);
#endif

	delete r;
}

void OgRenderContextVulkan::buildGraphicsPipeline(OgGraphicsPipelineVK* pipeline)
{
	OgRenderContextVulkan* renderContext = this;
	VkPhysicalDevice physicalDevice = renderContext->_gpuDeviceVK;
	VkDevice logicalDevice = renderContext->_logicalDeviceVK;

	OgRasterizationDescriptor& raster = pipeline->rasterizationDescriptor;
	OgColorBlendDescriptor& colorBlend = pipeline->colorBlendDescriptor;
	OgDepthStencilDescriptor& depthStencil = pipeline->depthStencilDescriptor;
	OgVertexInputDescriptor& vertexInput = pipeline->vertexInputDescriptor;
	OgShaderDescriptor& shader = pipeline->shaderDescriptor;

	OgResourceLayoutVK* res = reinterpret_cast<OgResourceLayoutVK*>(pipeline->resourceLayout);
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &res->descriptorSetLayoutVK;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	VK_CHECK_RESULT(vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipeline->pipelineLayout));

	// Rasterize

	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
	VK_CHECK_RESULT(vkCreatePipelineCache(logicalDevice, &pipelineCacheCreateInfo, nullptr, &pipeline->pipelineCache));

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssemblyState.topology = static_cast<VkPrimitiveTopology>(raster.primitiveType);
	inputAssemblyState.flags = 0;
	inputAssemblyState.primitiveRestartEnable = VK_FALSE;

	VkPipelineRasterizationStateCreateInfo rasterizationState{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizationState.polygonMode = static_cast<VkPolygonMode>(raster.polygonMode);
	rasterizationState.cullMode = static_cast<VkCullModeFlags>(raster.cullMode);
	rasterizationState.frontFace = static_cast<VkFrontFace>(raster.frontFace);
	rasterizationState.flags = 0;
	rasterizationState.depthClampEnable = VK_FALSE;
	rasterizationState.lineWidth = 1.0f;

	// Color Blend
	OgVector<VkPipelineColorBlendAttachmentState> attachmentDescriptor;

	attachmentDescriptor.Resize(colorBlend.attachmentCount);

	for (uint32 i = 0; i < colorBlend.attachmentCount; ++i)
	{
		attachmentDescriptor[i].colorWriteMask = static_cast<VkColorComponentFlags>(colorBlend.attachments[i].writeMask);
		attachmentDescriptor[i].blendEnable = colorBlend.attachments[i].blendEnable;
		attachmentDescriptor[i].srcColorBlendFactor = static_cast<VkBlendFactor>(colorBlend.attachments[i].srcColor);
		attachmentDescriptor[i].dstColorBlendFactor = static_cast<VkBlendFactor>(colorBlend.attachments[i].dstColor);
		attachmentDescriptor[i].colorBlendOp = static_cast<VkBlendOp>(colorBlend.attachments[i].colorOp);
		attachmentDescriptor[i].srcAlphaBlendFactor = static_cast<VkBlendFactor>(colorBlend.attachments[i].srcAlpha);
		attachmentDescriptor[i].dstAlphaBlendFactor = static_cast<VkBlendFactor>(colorBlend.attachments[i].dstAlpha);
		attachmentDescriptor[i].alphaBlendOp = static_cast<VkBlendOp>(colorBlend.attachments[i].alphaOp);
	}

	// ??
	VkPipelineColorBlendStateCreateInfo colorBlendState{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlendState.attachmentCount = static_cast<uint32_t>(attachmentDescriptor.Size());
	colorBlendState.pAttachments = attachmentDescriptor.Data();
	//colorBlendState.logicOpEnable = VK_FALSE;
	//colorBlendState.logicOp = VK_LOGIC_OP_CLEAR;
	//colorBlendState.blendConstants[0] = 0.0f;
	//colorBlendState.blendConstants[1] = 0.0f;
	//colorBlendState.blendConstants[2] = 0.0f;
	//colorBlendState.blendConstants[3] = 0.0f;

	// Depth Stencil
	VkPipelineDepthStencilStateCreateInfo depthStencilState{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depthStencilState.depthTestEnable = (depthStencil.depthTest) ? VK_TRUE : VK_FALSE;
	depthStencilState.depthWriteEnable = (depthStencil.depthWrite) ? VK_TRUE : VK_FALSE;
	depthStencilState.depthCompareOp = static_cast<VkCompareOp>(depthStencil.depthCompareOp);

	depthStencilState.front.compareMask = depthStencil.front.compareMask;
	depthStencilState.front.compareOp = static_cast<VkCompareOp>(depthStencil.front.compareOp);
	depthStencilState.front.depthFailOp = static_cast<VkStencilOp>(depthStencil.front.depthFailOp);
	depthStencilState.front.failOp = static_cast<VkStencilOp>(depthStencil.front.failOp);
	depthStencilState.front.passOp = static_cast<VkStencilOp>(depthStencil.front.passOp);
	depthStencilState.front.reference = depthStencil.front.reference;

	depthStencilState.back.compareMask = depthStencil.back.compareMask;
	depthStencilState.back.compareOp = static_cast<VkCompareOp>(depthStencil.back.compareOp);
	depthStencilState.back.depthFailOp = static_cast<VkStencilOp>(depthStencil.back.depthFailOp);
	depthStencilState.back.failOp = static_cast<VkStencilOp>(depthStencil.back.failOp);
	depthStencilState.back.passOp = static_cast<VkStencilOp>(depthStencil.back.passOp);
	depthStencilState.back.reference = depthStencil.back.reference;

	VkPipelineMultisampleStateCreateInfo multisampleState{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	// TO DO: sample shading
	// https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#primsrast-sampleshading
	// https://vulkan-tutorial.com/Multisampling
	multisampleState.sampleShadingEnable = VK_FALSE;
	if (pipeline->renderPass->info.outputColorAttachmentCount > 0)
		multisampleState.rasterizationSamples = static_cast<VkSampleCountFlagBits>(pipeline->renderPass->info.outputColorAttachments->sampleCount);
	else
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleState.flags = 0;

	// Vertex Input
	OgVector<VkVertexInputBindingDescription> vertexInputBindDescriptions;
	vertexInputBindDescriptions.Resize(vertexInput.layoutCount);
	for (size_t i = 0; i < vertexInput.layoutCount; ++i)
	{
		vertexInputBindDescriptions[i].binding = vertexInput.layouts[i].binding;
		vertexInputBindDescriptions[i].stride = vertexInput.layouts[i].stride;
		vertexInputBindDescriptions[i].inputRate = vertexInput.layouts[i].useInstancing ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
	}

	uint32 attributeCount = vertexInput.attributeCount;

	// Vertex Input State
	OgVector<VkVertexInputAttributeDescription> attributeDescriptions;
	attributeDescriptions.Resize(attributeCount);

	for (uint32 i = 0; i < attributeCount; ++i)
	{
		const OgVertexAttributeDescriptor& desc = vertexInput.attributes[i];
		VkVertexInputAttributeDescription vInputAttribDescription{};
		vInputAttribDescription.location = desc.location;
		vInputAttribDescription.binding = desc.binding;

		VkFormat format = vk_convert_vertex_format(desc.format);
		VkFormatProperties formatProps;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);

		vInputAttribDescription.format = format;
		/*
		if ((formatProps.bufferFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT) != 0)
		{
		vInputAttribDescription.format = format;
		}
		else
		{
		// TODO : impl
		// LOGE(LV_ID, "Vertex VkFormat %s is not supported.", format);
		}
		*/
		vInputAttribDescription.offset = desc.offset;
		attributeDescriptions[i] = vInputAttribDescription;
	}

	VkPipelineVertexInputStateCreateInfo inputState{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

	inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.Size());
	inputState.pVertexAttributeDescriptions = attributeDescriptions.Data();
	inputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindDescriptions.Size());
	inputState.pVertexBindingDescriptions = vertexInputBindDescriptions.Data();

	VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	viewportState.flags = 0;

	// Dynamic State.
	OgVector<VkDynamicState> dynamicStateEnables;

	dynamicStateEnables.Add(VK_DYNAMIC_STATE_VIEWPORT);
	dynamicStateEnables.Add(VK_DYNAMIC_STATE_SCISSOR);


	VkPipelineDynamicStateCreateInfo dynamicState{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicState.pDynamicStates = dynamicStateEnables.Data();
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.Size());
	dynamicState.flags = 0;

	OgRenderPassVK* renderPassVK = reinterpret_cast<OgRenderPassVK*>(pipeline->renderPass);
	VkRenderPass* rp = &renderPassVK->renderPassVK;

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

	pipelineCreateInfo.layout = pipeline->pipelineLayout;
	pipelineCreateInfo.renderPass = *rp;
	pipelineCreateInfo.flags = 0;
	pipelineCreateInfo.basePipelineIndex = -1;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

	pipelineCreateInfo.pVertexInputState = &inputState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pDynamicState = &dynamicState;

	OgVector<VkPipelineShaderStageCreateInfo> shaderStages;
	shaderStages.Resize(shader.shaderCount);

	for (size_t i = 0; i < shader.shaderCount; ++i)
	{
		OgShaderVK* shaderVK = reinterpret_cast<OgShaderVK*>(shader.shaders[i]);
		shaderStages[i] = shaderVK->shaderStageInfo;
	}

	pipelineCreateInfo.stageCount = shader.shaderCount;
	pipelineCreateInfo.pStages = shaderStages.Data();

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(logicalDevice, pipeline->pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline->pipeline));
}


OgPipelineHandle* OgRenderContextVulkan::CreatePipeline(OgPipelineDescriptor& descriptor)
{
	OgGraphicsPipelineVK* p = new OgGraphicsPipelineVK(descriptor);

	buildGraphicsPipeline(p);

#if defined(_DEBUG)
	p->instanceType = "Pipeline";
	_livingObjects.Add(p);
#endif

	return p;
}

void OgRenderContextVulkan::releaseGraphicsPipeline(OgGraphicsPipelineVK* pipeline)
{
	if (pipeline->pipelineCache != NULL) vkDestroyPipelineCache(this->_logicalDeviceVK, pipeline->pipelineCache, nullptr);
	if (pipeline->pipelineLayout != NULL) vkDestroyPipelineLayout(this->_logicalDeviceVK, pipeline->pipelineLayout, nullptr);
	if (pipeline->pipeline != NULL) vkDestroyPipeline(this->_logicalDeviceVK, pipeline->pipeline, nullptr);
}

void OgRenderContextVulkan::DestroyPipeline(OgPipelineHandle* pipeline)
{
	OG_CHECK(pipeline != nullptr, "OgPipelineHandle is nullptr");

	OgGraphicsPipelineVK* p = static_cast<OgGraphicsPipelineVK*>(pipeline);

	releaseGraphicsPipeline(p);

#if defined(_DEBUG)
	_livingObjects.Remove(p);
#endif

	delete p;
}

OgResourceLayoutHandle* OgRenderContextVulkan::CreateResourceLayout(OgResourceBinding* bindings, uint32 count)
{
	// VK Resource Binding : https://developer.nvidia.com/vulkan-shader-resource-binding
	//						 https://vulkan-tutorial.com/Uniform_buffers/Descriptor_layout_and_buffer
	// DX12 Resource Binding : https://software.intel.com/en-us/articles/introduction-to-resource-binding-in-microsoft-directx-12
	// METAL Resource Binding : https://developer.apple.com/documentation/metal/resource_objects/about_argument_buffers
	//							https://developer.apple.com/documentation/metal/buffers/argument_buffers_with_arrays_and_resource_heaps?language=objc
	
	// glsl : https://github.com/KhronosGroup/GLSL/blob/master/extensions/khr/GL_KHR_vulkan_glsl.txt
	
	// Descriptor 는 glsl이나 hlsl 에 선언된 변수의 설명.
	// DescriptorLayoutBinding 선언된 구조체 변수에 자료형을 설명. (binding)
	// WriteDescriptorSet 버퍼로 쓰여지는 리소스에 대한 정보
	
	
	// LvResourceLayoutVK has mem alloc on the constructor.
	OgResourceLayoutVK* rLayout = new OgResourceLayoutVK(_logicalDeviceVK, bindings, count);
	
	#if defined(_DEBUG)
		rLayout->instanceType = "ResourceLayout";
		_livingObjects.Add(rLayout);
	#endif

	return rLayout;
}
void OgRenderContextVulkan::DestroyResourceLayout(OgResourceLayoutHandle* layout)
{
	OG_CHECK(layout != nullptr, "OgResourceLayoutHandle is nullptr");

	OgResourceLayoutVK* l = static_cast<OgResourceLayoutVK*>(layout);

#if defined(_DEBUG)
	_livingObjects.Remove(l);
#endif

	delete l;
}

void OgRenderContextVulkan::buildResourceSet(OgResourceSetVK* rSet)
{
	OgRenderContextVulkan* renderContext = this;
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	descriptorSetAllocateInfo.descriptorPool = renderContext->_descriptorPool;
	descriptorSetAllocateInfo.pSetLayouts = &rSet->resourceLayoutVK->descriptorSetLayoutVK;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	VK_CHECK_RESULT(vkAllocateDescriptorSets(renderContext->_logicalDeviceVK, &descriptorSetAllocateInfo, &rSet->descriptorSetVK));

	OgVector<VkWriteDescriptorSet> writeDescriptorSets;
	writeDescriptorSets.Resize(rSet->resourceLayoutVK->bufferUsageCount + rSet->resourceLayoutVK->textureUsageCount);
	OgVector<OgVector<VkDescriptorBufferInfo>> bufferInfos;
	bufferInfos.Resize(rSet->resourceLayoutVK->bufferUsageCount);
	OgVector<OgVector<VkDescriptorImageInfo>> texInfos;
	texInfos.Resize(rSet->resourceLayoutVK->textureUsageCount);

	uint32 bufferIndex = 0;
	uint32 textureIndex = 0;
	for (uint32 i = 0; i < rSet->resourceUsageCount; ++i)
	{
		const OgResourceUsage& rUsage = rSet->resourceUsages[i];

		uint32 count = rUsage.binding.arrayCount == 0 ? 1 : rUsage.binding.arrayCount;

		switch (rUsage.binding.type)
		{
		case OgResourceType::UNIFORM_BUFFER:
		{
			OgVector<VkDescriptorBufferInfo>& bufferInfoArray = bufferInfos[bufferIndex];

			bufferInfoArray.Resize(count);
			for (uint16 infoArrayIndex = 0; infoArrayIndex < count; ++infoArrayIndex)
			{
				OgBufferVK* ref = reinterpret_cast<OgBufferVK*>(rUsage.buffer.handle[infoArrayIndex]);
				bufferInfoArray[infoArrayIndex].buffer = ref->bufferVK;
				bufferInfoArray[infoArrayIndex].offset = rUsage.buffer.offset[infoArrayIndex] + ref->innerOffset;
				bufferInfoArray[infoArrayIndex].range = rUsage.buffer.range[infoArrayIndex];
			}

			writeDescriptorSets[i] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			VkWriteDescriptorSet& writeDescriptorSet = writeDescriptorSets[i];
			writeDescriptorSet.pNext = nullptr;
			writeDescriptorSet.dstSet = rSet->descriptorSetVK;
			writeDescriptorSet.dstBinding = rUsage.binding.binding;
			writeDescriptorSet.dstArrayElement = 0;
			writeDescriptorSet.descriptorCount = count;
			writeDescriptorSet.descriptorType = static_cast<VkDescriptorType>(rUsage.binding.type);
			writeDescriptorSet.pImageInfo = nullptr;
			writeDescriptorSet.pBufferInfo = bufferInfoArray.Data();
			writeDescriptorSet.pTexelBufferView = nullptr;

			++bufferIndex;
			break;
		}
		case OgResourceType::COMBINED_IMAGE_SAMPLER:
		{
			OgVector<VkDescriptorImageInfo>& imageInfoArray = texInfos[textureIndex];
			imageInfoArray.Resize(count);
			for (uint16 infoArrayIndex = 0; infoArrayIndex < count; ++infoArrayIndex)
			{
				const OgTextureVK* textureVK = reinterpret_cast<OgTextureVK*>(rUsage.texture.handle[infoArrayIndex]);

				if (textureVK != nullptr && textureVK->sampler != nullptr)
				{
					imageInfoArray[infoArrayIndex].sampler = reinterpret_cast<OgSamplerVK*>(textureVK->sampler)->samplerVK;
				}
				else
				{
					imageInfoArray[infoArrayIndex].sampler = NULL;
				}
				imageInfoArray[infoArrayIndex].imageView = textureVK->view;
				imageInfoArray[infoArrayIndex].imageLayout = textureVK->imageLayout;
			}


			// Shader uses descriptor slot 0.1 slot set = 0, binding = 1
			writeDescriptorSets[i] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			VkWriteDescriptorSet& writeDescriptorSet = writeDescriptorSets[i];
			writeDescriptorSet.pNext = nullptr;
			writeDescriptorSet.dstSet = rSet->descriptorSetVK;
			writeDescriptorSet.dstBinding = rUsage.binding.binding;
			writeDescriptorSet.dstArrayElement = 0;
			writeDescriptorSet.descriptorCount = count;
			writeDescriptorSet.descriptorType = static_cast<VkDescriptorType>(rUsage.binding.type);
			writeDescriptorSet.pImageInfo = imageInfoArray.Data();
			writeDescriptorSet.pBufferInfo = nullptr;
			writeDescriptorSet.pTexelBufferView = nullptr;

			++textureIndex;
			break;
		}
		default:
		{
			LOGE(OG_ID, "Not Supported Yet");
			break;
		}
		}
	}

	vkUpdateDescriptorSets(renderContext->_logicalDeviceVK, static_cast<uint32_t>(writeDescriptorSets.Size()), writeDescriptorSets.Data(), 0, NULL);
}


OgResourceSetHandle* OgRenderContextVulkan::CreateResourceSet(OgResourceLayoutHandle* resourceLayout, OgResourceUsage* usages, uint32 usageCount)
{
	OgResourceLayoutVK* rLayout = reinterpret_cast<OgResourceLayoutVK*>(resourceLayout);
	OG_CHECK(rLayout->IsCompatibleLayoutWithSet(usages, usageCount), "Wrong Usages for Resource Layout");

	if (rLayout->bufferCount + _usedUniformBufferFromPool > _maxUniformBufferFromPool ||
		rLayout->textureCount + _usedTextureFromPool > _maxTextureFromPool ||
		_usedSetFromPool >= _maxSetFromPool)
	{
		// 이 경우엔 현재 Manual하게 max개수를 늘려줘야 함
		ASSERT(VK_FALSE);
		return nullptr;

	}

	OgResourceSetVK* rSet = new OgResourceSetVK(rLayout, usages, usageCount);

	buildResourceSet(rSet);

	// Pool Count Update
	_usedUniformBufferFromPool += rLayout->bufferCount;
	_usedTextureFromPool += rLayout->textureCount;
	_usedSetFromPool += 1;

#if defined(_DEBUG)
	rSet->instanceType = "ResourceSet";
	_livingObjects.Add(rSet);
#endif

	return rSet;
}

void OgRenderContextVulkan::releaseResourceSet(OgResourceSetVK* resourceSet)
{
	vkFreeDescriptorSets(this->_logicalDeviceVK, this->_descriptorPool, 1, &(resourceSet->descriptorSetVK));
}


void OgRenderContextVulkan::DestroyResourceSet(OgResourceSetHandle* resourceSet)
{
	OG_CHECK(resourceSet != nullptr, "OgResourceSetHandle is nullptr");
	OgResourceSetVK* r = static_cast<OgResourceSetVK*>(resourceSet);
	OG_CHECK(r->resourceLayoutVK != nullptr, "LvResourceLayout is nullptr");

	// Pool Count Update
	_usedUniformBufferFromPool -= r->resourceLayoutVK->bufferCount;
	_usedTextureFromPool -= r->resourceLayoutVK->textureCount;
	_usedSetFromPool -= 1;

	releaseResourceSet(r);

#if defined(_DEBUG)
	_livingObjects.Remove(r);
#endif

	delete r;
}


OgCommandEncoderHandle* OgRenderContextVulkan::CreateCommandEncoder()
{
	OgCommandEncoderVK* r = new OgCommandEncoderVK(_vulkanDevice, _cmdPoolVK);
	return r;
}
void OgRenderContextVulkan::DestroyCommandEncoder(OgCommandEncoderHandle* encoder)
{
	if (encoder == nullptr) LOGE(OG_ID, "LvCommandEncoderHandle is null");

	OgCommandEncoderVK* e = static_cast<OgCommandEncoderVK*>(encoder);

	delete e;
}

void* OgRenderContextVulkan::MapBuffer(OgBufferHandle* buffer, size_t size, size_t offset)
{
	OG_CHECK(buffer->option == OgMemoryOption::MAP_MANAGED, "Wrong Buffer Memory Option");

	OG_CHECK(buffer->size >= size + offset, "Wrong Buffer Size and Offset");

	OgBufferVK* bf = reinterpret_cast<OgBufferVK*>(buffer);
	OG_CHECK(bf->isMapBufferCalled == false, "MapBuffer is Already Called for this buffer");

	if (bf->isMapBufferCalled == false)
	{
		bf->mappedSize = (uint32)size;
		bf->mappedOffset = (uint32)offset;
		bf->isMapBufferCalled = true;

		void* bufferPtr = (uint8*)(bf->mapped) + offset;
		return bufferPtr;
	}

	return nullptr;
}

bool OgRenderContextVulkan::UnmapBuffer(OgBufferHandle* buffer)
{
	// We will not disconnect buffer memorypointer.
	// Because we only need to connect buffer pointer in vulkan. 
	// So we connect buffer pointer in `CreateBuffer`.
	// And we will disconnect buffer pointer in 'DestroyBuffer'.
	OG_CHECK(buffer->option == OgMemoryOption::MAP_MANAGED, "Wrong Buffer Memory Option");

	OgBufferVK* bf = reinterpret_cast<OgBufferVK*>(buffer);
	OG_CHECK(bf->isMapBufferCalled == true, "MapBuffer is not called for this buffer");

	if (bf->isMapBufferCalled == true)
	{
		if (bf->isAutoCoherent == false)
			bf->Flush(); //TODO reconsider bf->mappedSize, bf->mappedOffset

		bf->mappedSize = 0;
		bf->mappedOffset = 0;
		bf->isMapBufferCalled = false;

		return true;
	}

	return false;
}

void OgRenderContextVulkan::UpdateBuffer(OgBufferHandle* buffer, size_t offset, void* data, size_t size, bool useBarrier)
{
	OG_CHECK(data != nullptr, " Data is null, but data should not be nullptr for UpdateBuffer function !");
	OG_CHECK(buffer != nullptr, " Buffer is null, but this buffer is already destroyed !");
	OG_CHECK(size != 0, " Updating data size should be greater than 0!");

	OgBufferVK* targetRealBufferHandle = static_cast<OgBufferVK*>(buffer);
	OG_CHECK(targetRealBufferHandle->size >= size, " Data size is bigger than buffer size so we can't update data!");

	uint32 targetInnerOffset = targetRealBufferHandle->innerOffset;
	OgBufferUsage usage = targetRealBufferHandle->usage;

	if (buffer->option == OgMemoryOption::MAP_MANAGED)
	{
		OG_CHECK(useBarrier == false, "Map Managed Buffer can't utilize the pipeline barrier");

		void* bufferPtr = (uint8*)(targetRealBufferHandle->mapped);
		memcpy(bufferPtr, data, size);
		targetRealBufferHandle->Flush();
	}
	else if (buffer->option == OgMemoryOption::PRIVATE_GPU)
	{
		if (data != nullptr)
		{
			OgBufferVK* ref = nullptr;
			ref = new OgBufferVK(*_vulkanDevice, size, usage, OgMemoryOption::MAP_MANAGED);
			ref->Build
			(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
				size,
				data
			);

			VkCommandBuffer copyCmd = _stagingCommandBuffer[_stagingSubmitIndex];
			VkBufferCopy copyRegion = {};
			copyRegion.size = size;
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;

			vkCmdCopyBuffer(
				copyCmd,
				ref->bufferVK,
				targetRealBufferHandle->bufferVK,
				1,
				&copyRegion);

			constexpr VkAccessFlags srcAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
			VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
			VkAccessFlags dstAccess = VK_ACCESS_MEMORY_READ_BIT;

			switch (usage)
			{
			case OgBufferUsage::UNIFORM:
			{
				dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
				dstAccess = VK_ACCESS_UNIFORM_READ_BIT;
				break;
			}
			case OgBufferUsage::INDEX:
			{
				dstStageMask = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
				dstAccess = VK_ACCESS_INDEX_READ_BIT;
				break;
			}
			case OgBufferUsage::VERTEX:
			{
				dstStageMask = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
				dstAccess = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
				break;
			}
			}

			CommandpipelineBarrierForBufferUpdate(
				copyCmd,
				targetRealBufferHandle->bufferVK,
				0,
				size,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				dstStageMask,
				srcAccess,
				dstAccess
			);

			ref->Destroy();
		}
	}
}

void OgRenderContextVulkan::BlitFramebuffer(uint srcX0, uint srcY0, uint srcX1, uint srcY1, OgFrameBufferHandle* srcBuffer, uint dstX0, uint dstY0, uint dstX1, uint dstY1, OgFrameBufferHandle* dstBuffer)
{
	//TODO
}

VkCommandBuffer OgRenderContextVulkan::CreateCommandBuffer(VkCommandBufferLevel level, bool begin)
{
	VkCommandBuffer cmdBuffer;

	VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
	cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufAllocateInfo.commandPool = _cmdPoolVK;
	cmdBufAllocateInfo.level = level;
	cmdBufAllocateInfo.commandBufferCount = 1;

	VK_CHECK_RESULT(vkAllocateCommandBuffers(_logicalDeviceVK, &cmdBufAllocateInfo, &cmdBuffer));

	// If requested, also start the new command buffer
	if (begin)
	{
		VkCommandBufferBeginInfo cmdBufferBeginInfo{};
		cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo));
	}

	return cmdBuffer;

}

void OgRenderContextVulkan::FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free)
{
	if (commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submit = {};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &commandBuffer;

	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE));
	VK_CHECK_RESULT(vkQueueWaitIdle(queue));

	if (free)
	{
		vkFreeCommandBuffers(_logicalDeviceVK, _cmdPoolVK, 1, &commandBuffer);
	}
}

OgPixelFormat OgRenderContextVulkan::GetDefaultDepthFormat()
{
	VkFormat fbDepthFormat;
	vkGetSupportedDepthFormat(_gpuDeviceVK, &fbDepthFormat);

	return static_cast<OgPixelFormat>(fbDepthFormat);
}

void OgRenderContextVulkan::Submit(OgSwapChain* swapchain, OgCommandEncoderHandle* encoder)
{
	uint32 swapchainHash = System::PointerHash(swapchain);
	
	OG_CHECK(_swapChainTables.find(swapchainHash) != _swapChainTables.end(), "There is no SwapchainWrapper matching LvSwapChain.");

	SwapchainWrapper& sw = *(_swapChainTables[swapchainHash]);

	sw.encoderQueue.push(encoder);
}
void OgRenderContextVulkan::Present(OgSwapChain* swapchain)
{
	/* TODO : Research for synchronization
	*
	* Needs more accurate synchrnoization strategy
	* 1. About Staging Command Buffer
	*	We can submit staging command buffer to the transfer queue.
	*	In that case, we need synchronization primitive between transfer queue (data transfer) and graphics queue (draw command),
	*	to complete the data transfer before draw command.
	*
	* 2. About Multi Window Draw Calls
	*	Lv1Engine Editor fills the scene rendering command into only one window (for example, main editor window).
	*	So In this case, if there is other window (not main editor window) to take the texture from main editor window command,
	*	and the draw command from other window is called earlier that main editor window, the draw order would be wrong.
	*	In addition, Even If we submit command buffers into the queue sequentially, if the driver hanldes it simultaneously,
	*	then we need synchronization in this case.
	*	I wonder whether it's guaranteed that the sequential vkQueueSubmit will be executed and then ended sequentially or not.
	*/


	// Submit the staging command buffer
	{
		if (_acquireOnceForPresent)
		{
			submitStagingCommandBuffer();
			_acquireOnceForPresent = false;
		}
	}

	/*
	* Head Window 부터 항상 렌더링한다.
	*/
	//SwapchainWrapper** swLink = &_rootSwapchainWrapper;
	//while (*swLink != nullptr)
	{
		//SwapchainWrapper& sw = *(*swLink);
		uint32 hashKey = System::PointerHash(swapchain);
		SwapchainWrapper& sw = *_swapChainTables[hashKey];

		OgVector<OgCommandEncoderVK*> encoders;
		
		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		while (!sw.encoderQueue.empty())
		{
			OgCommandEncoderVK* encoder = reinterpret_cast<OgCommandEncoderVK*>(sw.encoderQueue.back());
			sw.encoderQueue.pop();
			// The submit info structure specifices a command buffer queue submission batch
			VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			
			submitInfo.pWaitDstStageMask = &waitStageMask;										// Pointer to the list of pipeline stages that the semaphore waits will occur at
			submitInfo.pWaitSemaphores = &sw.syncObject.imageReadys[sw.syncObject.submissionIndex];		// Semaphore(s) to wait upon before the submitted command buffer starts executing
			submitInfo.waitSemaphoreCount = 1;													// One wait semaphore																				
			submitInfo.pSignalSemaphores = &sw.syncObject.renderDones[sw.syncObject.swapchainIndex];		// Semaphore(s) to be signaled when command buffers have completed
			submitInfo.signalSemaphoreCount = 1;												// One signal semaphore
			submitInfo.pCommandBuffers = &encoder->cmdBufferVK;								// Command buffers(s) to execute in this batch (submission)
			submitInfo.commandBufferCount = 1;													// One command buffer
		}
		




		if (submitInfo.commandBufferCount > 0)
		{
			VK_CHECK_RESULT(vkQueueSubmit(_graphicsQueueVK, static_cast<uint32_t>(submitInfo.commandBufferCount), &submitInfo, sw.syncObject.fences[sw.syncObject.submissionIndex]));

			VkResult err = sw.swapchainVK.QueuePresent(sw.presentQueueVK, sw.syncObject.swapchainIndex, sw.syncObject.renderDones[sw.syncObject.swapchainIndex]);
			if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
			{
				sw.syncObject.submissionIndex = SUBMISSION_INDEX_NONE;

				while (!sw.encoderQueue.empty())
				{
					sw.encoderQueue.pop();
				}
			}
			else if (err != VK_SUCCESS)
			{
				LOGE(OG_ID, "failed to present swap chain iamge");
			}
		}
	}

	_cmdPoolState = CommandPoolState::RECORING;

}
void OgRenderContextVulkan::Suspend(OgSwapChain* swapchain) 
{
	OG_CHECK(swapchain != nullptr, "LvSwapChain is nullptr");

	uint32 swapchainHash = System::PointerHash(swapchain);
	OG_CHECK(_swapChainTables.find(swapchainHash) != _swapChainTables.end(), "This LvSwapChain is not used");

	vkDeviceWaitIdle(_logicalDeviceVK);

	vkQueueWaitIdle(_graphicsQueueVK);

	vkResetCommandPool(_logicalDeviceVK, _cmdPoolVK, 0);
	_cmdPoolState = CommandPoolState::RESET;

	SwapchainWrapper& sw = *(_swapChainTables[swapchainHash]);

	// clear
	while (!sw.encoderQueue.empty())
	{
		sw.encoderQueue.pop();
	}

	destroySwapChainSyncObject(sw);
	destroySwapChainFramebuffers(sw);
}
void OgRenderContextVulkan::Restore(OgSwapChain* swapchain) 
{

	OG_CHECK(swapchain != nullptr, "LvSwapChain is nullptr");

	uint32 swapchainHash = System::PointerHash(swapchain);
	OG_CHECK(_swapChainTables.find(swapchainHash) != _swapChainTables.end(), "This LvSwapChain is not used");

	vkDeviceWaitIdle(_logicalDeviceVK);

	vkQueueWaitIdle(_graphicsQueueVK);

	VkCommandBufferBeginInfo cmdBufferBeginInfo{};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	SwapchainWrapper& sw = *(_swapChainTables[swapchainHash]);

	// TODO : Android의 경우, Home Button을 누르면, Surface까지 파괴되는 것이기 때문에,
	// destroySwapChainFramebuffers에 해당 전처리기를 넣어서 처리했는데
	// 이것을 정확히 플랫폼별로 어떻게 동작하는지 알고나서 적용해야 한다.
#if defined(__ANDROID__)
	prepareSwapChain(sw);
#endif
	initSwapChain(sw);
	initSwapChainSyncObject(sw);
}
void OgRenderContextVulkan::WaitDeviceIdle()
{
	vkDeviceWaitIdle(_logicalDeviceVK);
}
void OgRenderContextVulkan::Collect()
{
	if (Render::OgHandle::AdvanceFrame() == true)
	{
		// frame count overflow. flush all pending deletes
		Render::OgHandle::FlushPendingDeletes(this, true);
	}
	else
	{
		Render::OgHandle::FlushPendingDeletes(this, false);
	}
}
void OgRenderContextVulkan::Shutdown(void) 
{
	vkDeviceWaitIdle(_logicalDeviceVK);

	OgHandle::FlushPendingDeletes(this, true);
	
	freeStagingCommandBuffers();

	for (std::unordered_map<uint32, SwapchainWrapper*>::iterator iter = _swapChainTables.begin(); iter != _swapChainTables.end();)
	{
		SwapchainWrapper& sw = *(iter->second);
		destroySwapChainSyncObject(sw);
		destroySwapChainFramebuffers(sw);

		// clear
		while (!sw.encoderQueue.empty())
		{
			sw.encoderQueue.pop();
		}

		sw.swapchainVK.Cleanup();

		_swapChainTables.erase(iter);
	}

	_rootSwapchainWrapper = nullptr;

	if (_descriptorPool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(_logicalDeviceVK, _descriptorPool, VK_NULL_HANDLE);
		_cmdPoolState = CommandPoolState::DEINIT;
	}
		

	if (_cmdPoolVK != NULL)
		vkDestroyCommandPool(_logicalDeviceVK, _cmdPoolVK, VK_NULL_HANDLE);

	if (s_enableValidationLayers)
	{
#if !defined(NDEBUG)
		if (_instance != VK_NULL_HANDLE)
			destroy_debug_reeport_callback(_instance, _reportCallbackHandle, VK_NULL_HANDLE);
#endif
	}

	if (_vulkanDevice != nullptr)
		delete _vulkanDevice;

	if (_instance != VK_NULL_HANDLE)
		vkDestroyInstance(_instance, VK_NULL_HANDLE);
}

OG_NAMESPACE_RENDER_END

