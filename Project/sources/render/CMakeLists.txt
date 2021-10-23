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
	)

IF(NOT IOS)
	list(APPEND PRIVATE_VULKAN private/vulkan/OgDeviceVulkan.cpp)
	list(APPEND PRIVATE_VULKAN private/vulkan/OgSwapChainVulkan.cpp)
	list(APPEND PRIVATE_VULKAN private/vulkan/OgUtilityVulkan.cpp)
	list(APPEND PRIVATE_VULKAN private/vulkan/OgRenderVulkanHandles.cpp)
	list(APPEND PRIVATE_VULKAN private/vulkan/OgRenderContext_Vulkan.cpp)
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

## DEBUG
INSTALL(TARGETS render CONFIGURATIONS Debug ARCHIVE DESTINATION "${INSTALL_FOLDER}/dev/lib" LIBRARY DESTINATION "${INSTALL_FOLDER}" RUNTIME DESTINATION "${INSTALL_FOLDER}")
install(DIRECTORY "." CONFIGURATIONS Debug PUBLIC_HEADER DESTINATION "${INSTALL_FOLDER}/dev/include/render" FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES "../OgPrecompiled.h" CONFIGURATIONS Debug PUBLIC_HEADER DESTINATION "${INSTALL_FOLDER}/dev/include")


## RELEASE

install(TARGETS render CONFIGURATIONS Release ARCHIVE DESTINATION "${INSTALL_FOLDER}/dev/lib")
install(DIRECTORY "." CONFIGURATIONS Release PUBLIC_HEADER DESTINATION "${INSTALL_FOLDER}/dev/include/render" FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES "../OgPrecompiled.h" CONFIGURATIONS Release PUBLIC_HEADER DESTINATION "${INSTALL_FOLDER}/dev/include")
## RELWITHDEBINFO

install(TARGETS render CONFIGURATIONS RelWithDebInfo ARCHIVE DESTINATION "${INSTALL_FOLDER}/dev/lib")
install(DIRECTORY "." CONFIGURATIONS RelWithDebInfo PUBLIC_HEADER DESTINATION "${INSTALL_FOLDER}/dev/include/render" FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES "../OgPrecompiled.h" CONFIGURATIONS RelWithDebInfo PUBLIC_HEADER DESTINATION "${INSTALL_FOLDER}/dev/include")
