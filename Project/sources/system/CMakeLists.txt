# set(CMAKE_CXX_STANDARD 11)

SET(COLLECTION_PUBLIC OgVector.h)
SOURCE_GROUP("collection\\public" FILES ${COLLECTION_PUBLIC})
SET(COLLECTION_PRIVATE)
SOURCE_GROUP("collection\\private" FILES ${COLLECTION_PRIVATE})

IF(APPLE)
	#TODO
ELSEIF(WIN32)
	#file(GLOB SRC_PLATFORM "win/*.c" "win/*.cpp" "win/*.h" "win/*.hpp" "win/*.natvis")
	set(SRC_PUBLIC_PLATFORM platform/win/OgPlatformDefinition_win32.h)
	source_group("platform\\public" FILES ${SRC_PUBLIC_PLATFORM})

	set(SRC_PRIVATE_PLATFORM private/platform/OgPlatformCommon.h private/platform/OgPlatformCommon.cpp private/platform/win/OgPlatform_win32.h private/platform/win/OgPlatform_win32.cpp private/platform/win/OgCollection.natvis private/platform/win/OgMemory_win32.c private/platform/win/OgPlatformStream_win32.cpp private/platform/win/OgThread_win32.c private/platform/win/OgFileSystem_win32.cpp)
	source_group("platform\\private" FILES ${SRC_PRIVATE_PLATFORM})
	# MESSAGE(STATUS "win32 ${SRC_PRIVATE_PLATFORM}")
ELSEIF(ANDROID)
	#TODO
ENDIF()

LIST(APPEND TOTAL_FILE ${SRC_PUBLIC_PLATFORM})
LIST(APPEND TOTAL_FILE ${SRC_PRIVATE_PLATFORM})

set(SRC_SYSTEM private/OgSystemContext.cpp private/OgNativeWindow.cpp private/OgNativeEvent.cpp private/OgMemory.cpp private/OgInput.cpp private/OgHashCode.cpp private/OgFileSystem.cpp private/OgStream.cpp)
source_group("private" FILES ${SRC_SYSTEM})
LIST(APPEND TOTAL_FILE ${SRC_SYSTEM})

set(INC_SYSTEM OgSystemContext.h OgFileSystem.h OgNativeWindow.h OgNativeEvent.h OgInput.h OgMemory.h OgHashCode.h  OgStream.h)
source_group("public" FILES ${INC_SYSTEM})
LIST(APPEND TOTAL_FILE ${INC_SYSTEM})


SET(SOURCE_GLM
	${PROJECT_SOURCE_DIR}/deps/include/glm/detail/glm.cpp
)
SOURCE_GROUP("thirdparty\\glm" FILES ${SOURCE_GLM})
LIST(APPEND TOTAL_FILE ${SOURCE_GLM})

set(IMGUI_DIR ${PROJECT_SOURCE_DIR}/sources/system/thirdparty/imgui)
set(SOURCES_IMGUI
	${IMGUI_DIR}/imconfig.h
	${IMGUI_DIR}/imgui.h
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
	${IMGUI_DIR}/imgui_internal.h
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
	${IMGUI_DIR}/imstb_rectpack.h
	${IMGUI_DIR}/imstb_textedit.h
	${IMGUI_DIR}/imstb_truetype.h
	${IMGUI_DIR}/backends/imgui_impl_vulkan.h
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)
SOURCE_GROUP("thirdparty\\imgui" FILES ${SOURCES_IMGUI})
LIST(APPEND TOTAL_FILE ${SOURCES_IMGUI})

add_library(imgui STATIC ${SOURCES_IMGUI})
target_include_directories(imgui PUBLIC ${IMGUI_DIR})


IF(ANDROID)
	#TODO
ELSE()
	add_library(system STATIC ${TOTAL_FILE} ${COLLECTION_PRIVATE} ${COLLECTION_PUBLIC} ${SOURCES_IMGUI})
ENDIF()

link_directories(${LIB_DIR})
target_link_libraries(system PUBLIC ${GLFW3} imgui)

# target_compile_options(render PUBLIC -Wall)
IF(OG_ENGINE_SHARED_LIB)
	target_compile_definitions(system PRIVATE "-DOG_API_EXPORT")
ENDIF()

#GUI framework
target_link_directories(system PRIVATE ${GLFW3} ${SOURCE_GLM})	

## DEBUG
INSTALL(TARGETS system CONFIGURATIONS Debug ARCHIVE DESTINATION "${INSTALL_FOLDER}/dev/lib" LIBRARY DESTINATION "${INSTALL_FOLDER}" RUNTIME DESTINATION "${INSTALL_FOLDER}")
install(DIRECTORY "." CONFIGURATIONS Debug PUBLIC_HEADER DESTINATION "${INSTALL_FOLDER}/dev/include/system" FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES "../OgPrecompile.h" CONFIGURATIONS Debug PUBLIC_HEADER DESTINATION "${INSTALL_FOLDER}/dev/include")


## RELEASE

install(TARGETS system CONFIGURATIONS Release ARCHIVE DESTINATION "${INSTALL_FOLDER}/dev/lib")
install(DIRECTORY "." CONFIGURATIONS Release PUBLIC_HEADER DESTINATION "${INSTALL_FOLDER}/dev/include/system" FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES "../OgPrecompile.h" CONFIGURATIONS Release PUBLIC_HEADER DESTINATION "${INSTALL_FOLDER}/dev/include")
## RELWITHDEBINFO

install(TARGETS system CONFIGURATIONS RelWithDebInfo ARCHIVE DESTINATION "${INSTALL_FOLDER}/dev/lib")
install(DIRECTORY "." CONFIGURATIONS RelWithDebInfo PUBLIC_HEADER DESTINATION "${INSTALL_FOLDER}/dev/include/system" FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES "../OgPrecompile.h" CONFIGURATIONS RelWithDebInfo PUBLIC_HEADER DESTINATION "${INSTALL_FOLDER}/dev/include")
