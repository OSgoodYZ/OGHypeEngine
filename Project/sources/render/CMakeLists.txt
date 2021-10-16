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
	private/vulkan/OgRender_Vulkan.h
	)

IF(NOT IOS)
	list(APPEND PRIVATE_VULKAN private/vulkan/OgRender_Vulkan.cpp)
ENDIF()

source_group("private\\vulkan" FILES ${PRIVATE_VULKAN})
list(APPEND TOTAL_FILE ${PRIVATE_VULKAN})


add_library(render STATIC ${TOTAL_FILE})
link_directories(${LIB_DIR}) # LIB_DIR은 ALL_BUILD CMakeList의 CACHE INTERNAL이다.


## DEBUG
install(TARGETS render CONFIGURATIONS Debug ARCHIVE DESTINATION "${INSTALL_FOLDER}/dev/lib")
install(DIRECTORY "." CONFIGURATIONS Debug PUBLIC_HEADER DESTINATION "${INSTALL_FOLDER}/dev/include/render" FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)

## RELEASE

install(TARGETS render CONFIGURATIONS Release ARCHIVE DESTINATION "${INSTALL_FOLDER}/dev/lib")
install(DIRECTORY "." CONFIGURATIONS Release PUBLIC_HEADER DESTINATION "${INSTALL_FOLDER}/dev/include/render" FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)

## RELWITHDEBINFO

install(TARGETS render CONFIGURATIONS RelWithDebInfo ARCHIVE DESTINATION "${INSTALL_FOLDER}/dev/lib")
install(DIRECTORY "." CONFIGURATIONS RelWithDebInfo PUBLIC_HEADER DESTINATION "${INSTALL_FOLDER}/dev/include/render" FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)