
set(PUBLIC_COMMON 
	OgRenderContext.h
	OgRenderDefinitions.h
	)
source_group("public" FILES ${PUBLIC_COMMON})

list(APPEND TOTAL_FILE ${PUBLIC_COMMON})

set(PRIVATE_COMMON 
	private/OgRenderDefinitions.cpp
	
	)
source_group("private" FILES ${PRIVATE_COMMON})

list(APPEND TOTAL_FILE ${PRIVATE_COMMON})


set(PRIVATE_VULKAN
	private/vulkan/OgDeviceVulkan.h
	private/vulkan/OgSwapChainVulkan.h
	private/vulkan/OgUtilityVulkan.h
	private/vulkan/OgRenderVulkanHandles.h
	private/vulkan/OgRenderContext_Vulkan.h
	private/vulkan/OgVulkanHelper.h
	private/vulkan/OgRayTracingHelper.h
	)

IF(NOT IOS)
	list(APPEND PRIVATE_VULKAN private/vulkan/OgDeviceVulkan.cpp)
	list(APPEND PRIVATE_VULKAN private/vulkan/OgSwapChainVulkan.cpp)
	list(APPEND PRIVATE_VULKAN private/vulkan/OgUtilityVulkan.cpp)
	list(APPEND PRIVATE_VULKAN private/vulkan/OgRenderVulkanHandles.cpp)
	list(APPEND PRIVATE_VULKAN private/vulkan/OgRenderContext_Vulkan.cpp)
	list(APPEND PRIVATE_VULKAN private/vulkan/OgVulkanHelper.cpp)
	list(APPEND PRIVATE_VULKAN private/vulkan/OgRayTracingVulkan.cpp)
	list(APPEND PRIVATE_VULKAN private/vulkan/OgComputeVulkan.cpp)
ENDIF()

source_group("private\\vulkan" FILES ${PRIVATE_VULKAN})
list(APPEND TOTAL_FILE ${PRIVATE_VULKAN})


# Ray Tracing option
option(OG_ENABLE_RAYTRACING "Enable Vulkan Ray Tracing" ON)

add_library(render STATIC ${TOTAL_FILE})
link_directories(${LIB_DIR})
target_link_libraries(render PUBLIC ${VULKAN} system)
SET_TARGET_PROPERTIES(render PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OG_PROJECT_BINARY_DIR})

# Ray Tracing definitions
if(OG_ENABLE_RAYTRACING)
	target_compile_definitions(render PUBLIC OG_ENABLE_RAYTRACING)
endif()

# target_compile_options(render PUBLIC -Wall)
IF(OG_ENGINE_SHARED_LIB)
	target_compile_definitions(render PRIVATE "-DOG_API_EXPORT")
ENDIF()

#GLFW3 link
# target_link_directories(render PRIVATE ${GLFW3})	

