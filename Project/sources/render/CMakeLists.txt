# set(CMAKE_CXX_STANDARD 11)

add_executable (render "main.cpp")


set(PUBLIC_COMMON 
	#LvRender.h
	#LvRenderContext.h
	#LvRenderDefinitions.h
	
	)
source_group("public" FILES ${PUBLIC_COMMON})

list(APPEND TOTAL_FILE ${PUBLIC_COMMON})

set(PRIVATE_COMMON 
	#private/LvRender.cpp
	#private/LvRenderDefinitions.cpp
	#private/LvRenderPlatform.cpp
	#private/LvRenderPlatform.h
	)
source_group("private" FILES ${PRIVATE_COMMON})

list(APPEND TOTAL_FILE ${PRIVATE_COMMON})


set(PRIVATE_VULKAN
	#private/vulkan/LvDeviceVulkan.h
	#private/vulkan/LvSwapChainVulkan.h
	#private/vulkan/LvUtilityVulkan.h
	#private/vulkan/LvRender_Vulkan.h
	#private/vulkan/LvRenderVulkanHandles.h
	#private/vulkan/LvRenderVulkanHelper.h
	)
IF(NOT IOS)
	#list(APPEND PRIVATE_VULKAN private/vulkan/LvDeviceVulkan.cpp)
	#list(APPEND PRIVATE_VULKAN private/vulkan/LvSwapChainVulkan.cpp)
	#list(APPEND PRIVATE_VULKAN private/vulkan/LvUtilityVulkan.cpp)
	#list(APPEND PRIVATE_VULKAN private/vulkan/LvRender_Vulkan.cpp)
	#list(APPEND PRIVATE_VULKAN private/vulkan/LvRender_VulkanImplementation.cpp)
	#list(APPEND PRIVATE_VULKAN private/vulkan/LvRenderVulkanHandles.cpp)
	#list(APPEND PRIVATE_VULKAN private/vulkan/LvRenderVulkanHelper.cpp)
ENDIF()
source_group("private\\vulkan" FILES ${PRIVATE_VULKAN})
list(APPEND TOTAL_FILE ${PRIVATE_VULKAN})


#add_library(render STATIC ${TOTAL_FILE})
#link_directories(${LIB_DIR}) # LIB_DIR은 ALL_BUILD CMakeList의 CACHE INTERNAL이다.


### DEBUG
#install(TARGETS render CONFIGURATIONS Debug ARCHIVE DESTINATION "${INSTALL_FOLDER}/dev/lib")
#install(DIRECTORY "." CONFIGURATIONS Debug PUBLIC_HEADER DESTINATION "${INSTALL_FOLDER}/dev/include/render" FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
#
### RELEASE
#
#install(TARGETS render CONFIGURATIONS Release ARCHIVE DESTINATION "${INSTALL_FOLDER}/dev/lib")
#install(DIRECTORY "." CONFIGURATIONS Release PUBLIC_HEADER DESTINATION "${INSTALL_FOLDER}/dev/include/render" FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
#
### RELWITHDEBINFO
#
#install(TARGETS render CONFIGURATIONS RelWithDebInfo ARCHIVE DESTINATION "${INSTALL_FOLDER}/dev/lib")
#install(DIRECTORY "." CONFIGURATIONS RelWithDebInfo PUBLIC_HEADER DESTINATION "${INSTALL_FOLDER}/dev/include/render" FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)