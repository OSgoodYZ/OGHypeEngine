#include <vector>

#include "OgPrecompile.h"

#include "system/OgSystemContext.h"

#include "render/private/vulkan/OgRenderContext_Vulkan.h"

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
	if (_livingObjects.size() > 0)
	{
		LOGD(OG_ID, "LvRenderContext Undestroy object : %zu", _livingObjects.size());
		for (int i = 0; i < _livingObjects.size(); ++i)
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



OG_NAMESPACE_RENDER_END

