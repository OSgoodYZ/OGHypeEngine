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

set(SRC_SYSTEM 
	private/OgSystemContext.cpp 
	private/OgNativeWindow.cpp 
	private/OgNativeEvent.cpp 
	private/OgMemory.cpp 
	private/OgInput.cpp 
	private/OgHashCode.cpp 
	private/OgFileSystem.cpp 
	private/OgStream.cpp
	private/OgImGUIManager.cpp
	)
source_group("private" FILES ${SRC_SYSTEM})
LIST(APPEND TOTAL_FILE ${SRC_SYSTEM})

set(INC_SYSTEM 
	OgSystemContext.h 
	OgFileSystem.h 
	OgNativeWindow.h 
	OgNativeEvent.h 
	OgInput.h 
	OgMemory.h 
	OgHashCode.h  
	OgStream.h
	OgImGUIManager.h
	)
source_group("public" FILES ${INC_SYSTEM})
LIST(APPEND TOTAL_FILE ${INC_SYSTEM})


# 포함할 GLM 폴더 경로 설정
set(GLM_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/deps/include/glm")

# 폴더 내 모든 .h, .hpp, .cpp 파일을 변수에 저장
file(GLOB GLM_SOURCE_FILES 
     "${GLM_INCLUDE_DIR}/*.h"
     "${GLM_INCLUDE_DIR}/*.hpp" 
     "${GLM_INCLUDE_DIR}/*.cpp")

# 변수를 프로젝트에 추가  
source_group("thirdparty\\glm" FILES ${GLM_SOURCE_FILES})
list(APPEND TOTAL_FILE ${GLM_SOURCE_FILES})
include_directories(${GLM_INCLUDE_DIR})



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
SET(PUBLIC_FONT thirdparty/IconsFontAwesome6.h)
SOURCE_GROUP("thirdparty\\font" FILES ${PUBLIC_FONT})
LIST(APPEND TOTAL_FILE ${PUBLIC_FONT})

add_library(imgui STATIC ${SOURCES_IMGUI})
target_include_directories(imgui PUBLIC ${IMGUI_DIR})

######################## SLANG ########################
# Slang 라이브러리 경로 설정
set(SLANG_DIR "${PROJECT_SOURCE_DIR}/deps/include/slang")
set(SLANG_LIB_DIR "${PROJECT_SOURCE_DIR}/deps/libs/win/x64/Debug")

# Slang 헤더 파일 포함
include_directories(${SLANG_DIR})

# Slang 라이브러리 파일 찾기
find_library(SLANG_LIBRARY NAMES slang PATHS ${SLANG_LIB_DIR})

# 라이브러리를 찾지 못하면 경고 메시지 출력
if(NOT SLANG_LIBRARY)
    message(WARNING "Slang 라이브러리를 찾을 수 없습니다. 경로를 확인하세요: ${SLANG_LIB_DIR}")
    # 대체 경로 설정
    set(SLANG_LIBRARY "${SLANG_LIB_DIR}/slang.lib")
endif()

# Slang의 정적 라이브러리 등록
add_library(slang STATIC IMPORTED GLOBAL)
set_target_properties(slang PROPERTIES
    IMPORTED_LOCATION "${SLANG_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SLANG_DIR}"
)

# 라이브러리 존재 여부 확인
if(EXISTS "${SLANG_LIBRARY}")
    message(STATUS "Slang 라이브러리를 찾았습니다: ${SLANG_LIBRARY}")
else()
    message(WARNING "Slang 라이브러리 파일이 존재하지 않습니다: ${SLANG_LIBRARY}")
endif()

# Slang 디렉토리 존재 여부 확인
if(EXISTS "${SLANG_DIR}")
    message(STATUS "Slang 헤더 디렉토리를 찾았습니다: ${SLANG_DIR}")
else()
    message(WARNING "Slang 헤더 디렉토리가 존재하지 않습니다: ${SLANG_DIR}")
endif()
######################## SLANG ########################

IF(ANDROID)
	#TODO
ELSE()
	add_library(system STATIC ${TOTAL_FILE} ${COLLECTION_PRIVATE} ${COLLECTION_PUBLIC} ${SOURCES_IMGUI})
ENDIF()

link_directories(${LIB_DIR})
target_link_libraries(system PUBLIC ${GLFW3} imgui slang)

# target_compile_options(render PUBLIC -Wall)
IF(OG_ENGINE_SHARED_LIB)
	target_compile_definitions(system PRIVATE "-DOG_API_EXPORT")
ENDIF()

#GUI framework
target_link_directories(system PRIVATE ${GLFW3} ${SOURCE_GLM})	
SET_TARGET_PROPERTIES(system PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OG_PROJECT_BINARY_DIR})
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
