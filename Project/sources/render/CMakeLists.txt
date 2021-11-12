# set(CMAKE_CXX_STANDARD 11)

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
	)

IF(NOT IOS)
	list(APPEND PRIVATE_VULKAN private/vulkan/OgDeviceVulkan.cpp)
	list(APPEND PRIVATE_VULKAN private/vulkan/OgSwapChainVulkan.cpp)
	list(APPEND PRIVATE_VULKAN private/vulkan/OgUtilityVulkan.cpp)
	list(APPEND PRIVATE_VULKAN private/vulkan/OgRenderVulkanHandles.cpp)
	list(APPEND PRIVATE_VULKAN private/vulkan/OgRenderContext_Vulkan.cpp)
	list(APPEND PRIVATE_VULKAN private/vulkan/OgVulkanHelper.cpp)
ENDIF()

source_group("private\\vulkan" FILES ${PRIVATE_VULKAN})
list(APPEND TOTAL_FILE ${PRIVATE_VULKAN})


add_library(render STATIC ${TOTAL_FILE})
link_directories(${LIB_DIR})
target_link_libraries(render PUBLIC ${VULKAN} system)


# target_compile_options(render PUBLIC -Wall)
IF(OG_ENGINE_SHARED_LIB)
	target_compile_definitions(render PRIVATE "-DOG_API_EXPORT")
ENDIF()

#GLFW3 link
# target_link_directories(render PRIVATE ${GLFW3})	

