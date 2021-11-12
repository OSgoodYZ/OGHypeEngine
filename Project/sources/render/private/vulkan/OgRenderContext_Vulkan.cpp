#include <vector>
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

OgRenderContextVulkan::OgRenderContextVulkan(System::OgSystemContext* context)
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
	appInfo.pApplicationName = "Lv";
	appInfo.pEngineName = "Lv Engine";
	appInfo.apiVersion = VK_API_VERSION_1_0;

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

	VkInstanceCreateInfo instanceInfo = {};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pNext = NULL;
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.enabledExtensionCount = static_cast<uint32_t>(_enabledInstanceExtensions.size());
	instanceInfo.ppEnabledExtensionNames = &_enabledInstanceExtensions.front();

	if (s_enableValidationLayers)
	{
#if defined(_DEBUG)
		if (s_validationLayers.size() > 0)
		{
#if defined(__DESKTOP__)

			if (s_sdkVersion.major >= 1 && s_sdkVersion.minor >= 2)
			{
				const list<const char*> validationLayers =
				{
					"VK_LAYER_KHRONOS_validation"
				};
				instanceInfo.enabledLayerCount = (uint32_t)validationLayers.size();
				instanceInfo.ppEnabledLayerNames = &validationLayers.front();
			}
			else
			{
				const list<const char*> validationLayers =
				{
					"VK_LAYER_LUNARG_standard_validation",
				};
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
}

void OgRenderContextVulkan::initDescriptorPool()
{
	_usedUniformBufferFromPool = 0;
	_usedTextureFromPool = 0;
	_usedSetFromPool = 0;
	_maxUniformBufferFromPool = 1024;//256;
	_maxTextureFromPool = 1024;
	_maxSetFromPool = 512;//256;

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

	System::OgNativeWindow* window = sw.window;

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

		rpInfo.useDepthStencilAttacment = true;
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
		framebuffers[i]->framebufferInfo.colorBuffers.push_back(tex);
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
	initDescriptorPool();
	
	// Swapchain Wrapper Class setting
	OgSwapChainVulkan::Connect(_instance, _vulkanDevice);
	
	this->_rootSwapchainWrapper = nullptr;
}

OgSwapChain* OgRenderContextVulkan::CreateSwapchain(System::OgNativeWindow* nativeWindow, const OgSwapChainInfo& swapchainInfo)
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

			for (size_t i = 0; i < fb->framebufferInfo.colorBuffers.size(); ++i)
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

OgBufferHandle* OgRenderContextVulkan::CreateBuffer(void* data, size_t size, OgBufferUsage usage, OgMemoryOption option )
{
	//TODO
	return nullptr;
}
void OgRenderContextVulkan::DestroyBuffer(OgBufferHandle* buffer)
{
	//TODO
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
	//TODO
}
void OgRenderContextVulkan::releaseTexture(OgTextureVK* texture)
{
	// TODO
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

	if (rInfo.isSwapchainRenderPass)
	{
		colorFinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}

	int outputColorAttachmentCount = rInfo.outputColorAttachmentCount;
	
	int totalAttachmentCount = outputColorAttachmentCount;
	if (rInfo.useDepthStencilAttacment == true)  totalAttachmentCount++;

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
			attachmentDescriptors[i].initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
		attachmentDescriptors[i].finalLayout = colorFinalLayout;
	}

	if (rInfo.useDepthStencilAttacment == true)
	{
		auto& depthDescriptor = attachmentDescriptors[totalAttachmentCount - 1];
		vk_convert_format(renderContext->_gpuDeviceVK, depthDescriptor, rInfo.outputDepthStencilAttachment);
		depthDescriptor.samples = static_cast<VkSampleCountFlagBits>(rInfo.outputDepthStencilAttachment.sampleCount);
		depthDescriptor.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthDescriptor.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthDescriptor.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		if (rInfo.outputDepthStencilAttachment.load == OgRenderBufferLoadAction::LOAD)
			depthDescriptor.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		// TODO !!!: depth output을 sample하고 싶다면 VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
		// 그냥 그대로 쓸거면  VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		// 여튼 이걸 옵션을 주거나 그냥 READ로 통일하거나 해야 한다.
		// 나중에 고려해서 수정할 것.
		depthDescriptor.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	}

	OgVector<VkAttachmentReference> colorReference;
	colorReference.Resize(8);
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
	subpassDescription.pDepthStencilAttachment = rInfo.useDepthStencilAttacment ? &depthReference : nullptr;
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
	//TODO
}
void OgRenderContextVulkan::DestroyRenderPass(OgRenderPassHandle* renderPass)
{
	//TODO
}

OgPipelineHandle* OgRenderContextVulkan::CreatePipeline(OgPipelineDescriptor& descriptor)
{
	//TODO
	return nullptr;
}
void OgRenderContextVulkan::DestroyPipeline(OgPipelineHandle* pipeline)
{
	//TODO
}

OgResourceLayoutHandle* OgRenderContextVulkan::CreateResourceLayout(OgResourceBinding* bindings, uint32 count)
{
	//TODO
	return nullptr;
}
void OgRenderContextVulkan::DestroyResourceLayout(OgResourceLayoutHandle* layout)
{
	//TODO
}

OgResourceSetHandle* OgRenderContextVulkan::CreateResourceSet(OgResourceLayoutHandle* resourceLayout, OgResourceUsage* usages, uint32 usageCount)
{
	//TODO
	return nullptr;
}
void OgRenderContextVulkan::DestroyResourceSet(OgResourceSetHandle* resourceSet)
{
	//TODO
}

OgCommandEncoderHandle* OgRenderContextVulkan::CreateCommandEncoder()
{
	//TODO
	return nullptr;
}
void OgRenderContextVulkan::DestroyCommandEncoder(OgCommandEncoderHandle* encoder)
{
	//TODO
}

void* OgRenderContextVulkan::MapBuffer(OgBufferHandle* buffer, size_t size, size_t offset)
{
	//TODO
	return nullptr;
}

bool OgRenderContextVulkan::UnmapBuffer(OgBufferHandle* buffer)
{
	//TODO
	return false;
}

void OgRenderContextVulkan::UpdateBuffer(OgBufferHandle* buffer, size_t offset, void* data, size_t size, bool useBarrier)
{
	//TODO
}

void OgRenderContextVulkan::BlitFramebuffer(uint srcX0, uint srcY0, uint srcX1, uint srcY1, OgFrameBufferHandle* srcBuffer, uint dstX0, uint dstY0, uint dstX1, uint dstY1, OgFrameBufferHandle* dstBuffer)
{
	//TODO
}

VkCommandBuffer OgRenderContextVulkan::CreateCommandBuffer(VkCommandBufferLevel level, bool begin)
{
	//TODO
	VkCommandBuffer temp;
	return temp;

}

void OgRenderContextVulkan::FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free)
{
	//TODO
}

OgPixelFormat OgRenderContextVulkan::GetDefaultDepthFormat()
{
	//TODO
	OgPixelFormat temp;
	return temp;
}

void OgRenderContextVulkan::Submit(OgSwapChain* swapchain, OgCommandEncoderHandle* encoder)
{
	//TODO
}
void OgRenderContextVulkan::Present(OgSwapChain* swapchain)
{
	//TODO
}
void OgRenderContextVulkan::Suspend(OgSwapChain* swapchain) 
{
	//TODO
}
void OgRenderContextVulkan::Restore(OgSwapChain* swapchain) 
{
	//TODO
}
void OgRenderContextVulkan::WaitDeviceIdle()
{
	//TODO
}
void OgRenderContextVulkan::Collect()
{
	//TODO
}
void OgRenderContextVulkan::Shutdown(void) 
{
	//TODO
}
bool OgRenderContextVulkan::HasFeature(OgRenderFeature feature)
{
	//TODO
	return false;
}


OgResourceSetPool* OgRenderContextVulkan::CreateResourceSetPool(uint32 maxUniformBufferFromPool, uint32 maxTextureFromPool, uint32 maxSetFromPool)
{
	//TODO
	return nullptr;
}

void OgRenderContextVulkan::DestroyResourceSetPool(OgResourceSetPool* resourceSetPool)
{
	//TODO
}


OG_NAMESPACE_RENDER_END

