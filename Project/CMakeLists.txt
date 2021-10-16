# OGHypeEngine version 0.1
# ref https://www.tuwlab.com/27270
cmake_minimum_required (VERSION 3.8)

project( OGHypeEngine )

IF (NOT DEFINED $ENV{VULKAN_SDK})
	MESSAGE(STATUS "Vulkan SDK is $ENV{VULKAN_SDK}")
ELSE()
	MESSAGE(SEND_ERROR " Vulkan SDK is not installed.")
ENDIF()


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# S : Cross Debug Configuration 추가하기.
IF (CMAKE_CONFIGURATION_TYPES)
	# https://github.com/Microsoft/vcpkg/issues/1638
	# 이 이슈에 따라, Visual Studio의 vcpkg 프로그램은 Debug Prefix를 가진 Configuration name을 해야
	# Visual Studio가 자동으로 Vcpkg 기능들을 적용하게끔 만든다고 한다.
	set(CMAKE_CONFIGURATION_TYPES Debug Release MinSizeRel RelWithDebInfo DebugCross)
	list(REMOVE_DUPLICATES CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CONFIGURATION_TYPES  ${CMAKE_CONFIGURATION_TYPES} CACHE STRING "Appended Custom Configuration to the IDE" FORCE)

	# BUILD_TYPE을 아래에서 설정해주기 때문에, 여기에서 Debug Cross에 관해서 직접 Flags들을 설정해준다.
	# 그래야 다른 프로젝트도 이 설정이 그대로 들어가게 된다.
	IF (MSVC)
		set(CMAKE_CXX_FLAGS_DEBUGCROSS "${CMAKE_CXX_FLAGS_DEBUG} /UMBCS /D_UNICODE /DUNICODE /Gz")
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /UMBCS /D_UNICODE /DUNICODE /Gz /wd4819")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /UMBCS /D_UNICODE /DUNICODE /Gz /wd4819")
	ELSEIF(APPLE)
		#set(CMAKE_CXX_FLAGS_DEBUGCROSS "${CMAKE_CXX_FLAGS_DEBUG}")
	ELSEIF(ANDROID)
		#SET(CMAKE_C_FLAGS_DEBUGCROSS "${CMAKE_C_FLAGS_DEBUG} -mno-thumb")
		#set(CMAKE_CXX_FLAGS_DEBUGCROSS "${CMAKE_CXX_FLAGS_DEBUG} -Winline-new-delete -DVK_USE_PLATFORM_ANDROID_KHR -mno-thumb")
	ENDIF()

	IF(WIN32)
		IF("${MSVC_C_ARCHITECTURE_ID}" STREQUAL "x64")
			SET(CMAKE_INSTALL_PREFIX "C:/Program Files/${CMAKE_PROJECT_NAME}")
		ELSE()
			MESSAGE(FATAL_ERROR "Not Allowed x86")
			# SET(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/${CMAKE_PROJECT_NAME}")
		ENDIF()
	ENDIF()

	IF(IOS)
		#SET(CMAKE_C_FLAGS_DEBUGCROSS "${CMAKE_C_FLAGS_DEBUG} -mno-thumb")
		#SET(CMAKE_CXX_FLAGS_DEBUGCROSS "${CMAKE_CXX_FLAGS_DEBUG} -mno-thumb")
	ENDIF()

	# MSVC Debug 모드 및 Debug pdb 생성을 위한 설정
	IF(MSVC)
		set(CMAKE_SHARED_LINKER_FLAGS_DEBUGCROSS "/DEBUG /INCREMENTAL /OPT:REF /OPT:ICF")
		set(CMAKE_EXE_LINKER_FLAGS_DEBUGCROSS "/DEBUG /INCREMENTAL")
	ENDIF()

	# CMake가 자동으로 만드는 Configuration Type에 대해 LvPrecompiled.h에서 쓸 매크로 설정
	# LvPrecompiled.h의 LV_CHECK가 정의된 곳이 있는데, 다음과 같이 CMake Configuration과 매칭된다.
	#	_DEBUG : Debug / DebugCross
	#	_DEBUG_CROSS : DebugCross
	#	_RELEASE : Realease / MinSizeRel
	#	_RELEASE_INFO : RelWithDebInfo
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG")
	list(REMOVE_DUPLICATES CMAKE_C_FLAGS_DEBUG) # 중복 제거
	set(CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})

	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
	list(REMOVE_DUPLICATES CMAKE_CXX_FLAGS_DEBUG) # 중복 제거
	set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})

	# Cross Configuration을 위해서 _DEBUG_CROSS라는 매크로 추가
	# 위에서 DEBUG에서 중복 다 제거함.
	set(CMAKE_C_FLAGS_DEBUGCROSS "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG_CROSS -std=c11")
	set(CMAKE_CXX_FLAGS_DEBUGCROSS "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG_CROSS")

	# RELEASE 관련 매크로는 기본 설정 안되어 있으므로 그냥 추가 하면 됌
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -D_RELEASE -std=c11")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D_RELEASE")

	set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -D_RELEASE -std=c11")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -D_RELEASE")

	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -D_RELEASE_INFO -std=c11")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -D_RELEASE_INFO")

ENDIF()




set(PROJECT_ROOT ${OGHypeEngine_SOURCE_DIR}/sources)

MESSAGE(STATUS "ROOT ${OGHypeEngine_SOURCE_DIR}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")


message(STATUS "Compiler")
message(STATUS " - ID       \t: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS " - Version  \t: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS " - Path     \t: ${CMAKE_CXX_COMPILER}")
message(STATUS " - BUILD SRC \t; ${CMAKE_BINARY_DIR}")

IF (MSVC)
	# /Gz = __stdcall
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /UMBCS /D_UNICODE /DUNICODE /Gz")
	IF("${MSVC_C_ARCHITECTURE_ID}" STREQUAL "X86")
		SET(ARCH "i686")
	ELSEIF("${MSVC_C_ARCHITECTURE_ID}" STREQUAL "x64")
		SET(ARCH "x86_64")
	ELSEIF("${MSVC_C_ARCHITECTURE_ID}" STREQUAL "ARM")
		SET(ARCH "arm")
	ELSE()
	IF (RESULT)
		MESSAGE(FATAL_ERROR "Failed to determine target architecture triplet: ${RESULT}")
	ENDIF()
		MESSAGE(FATAL_ERROR "Failed to determine the MSVC target architecture: ${MSVC_C_ARCHITECTURE_ID}")
	ENDIF()
ELSEIF(APPLE)
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	#execute_process(
	#	COMMAND ${CMAKE_C_COMPILER} -dumpmachine
	#	RESULT_VARIABLE RESULT
	#	OUTPUT_VARIABLE ARCH
	#	ERROR_QUIET
	#)
	#string(REGEX MATCH "([^-]+).*" ARCH_MATCH ${ARCH})
	#IF (NOT CMAKE_MATCH_1 OR NOT ARCH_MATCH)
	#	MESSAGE(FATAL_ERROR "Failed to match the target architecture triplet: ${ARCH}")
	#ENDIF()
	#SET(ARCH ${CMAKE_MATCH_1})
ELSEIF(ANDROID)
	#SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mno-thumb")
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Winline-new-delete -DVK_USE_PLATFORM_ANDROID_KHR -mno-thumb")
	#SET(ARCH ${ANDROID_ABI})
	##include_directories(SYSTEM "${ANDROID_SYSROOT}/usr/include/../include")
	#MESSAGE(STATUS "ANDROID PATH = ${ANDROID_SYSROOT}/usr/include")
ENDIF()

IF(WIN32)
    IF(ARCH MATCHES "x86_64")
		SET(CMAKE_INSTALL_PREFIX "C:/Program Files/${CMAKE_PROJECT_NAME}")
    ELSE()
		SET(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/${CMAKE_PROJECT_NAME}")
    ENDIF()
ENDIF()


#IF(IOS)
#    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mno-thumb")
#    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mno-thumb")
#ENDIF()

#MESSAGE(STATUS "Target architecture - ${ARCH}")

#OPTION(XCODE_GENERATE_SCHEME "generate scheme" ON)

OPTION(OG_ENGINE_SHARED_LIB "Build libraries" ON)

#OPTION(OG_LEAK_CHECK "check leak" OFF)

#IF(WIN32)
#	OPTION(INCLUDE_VSIX "vsix" OFF)
#ENDIF()




IF(${ARCH} STREQUAL "x86")
	MESSAGE(FATAL_ERROR "OGHypeEngine is only support x64")
ENDIF()

IF(APPLE)
    # 빌드 환경 변수 : https://pewpewthespells.com/blog/buildsettings.html
    
   #SET(LIB_DEBUG_DIR "${OGHypeEngine_SOURCE_DIR}/deps/libs/osx/x86_64/Debug")
	#SET(DLL_DEBUG_DIR "${OGHypeEngine_SOURCE_DIR}/deps/dll/osx/x86_64/Debug")
   
	#SET(LIB_RELEASE_DIR "${OGHypeEngine_SOURCE_DIR}/deps/libs/osx/x86_64/Release")
	#SET(DLL_RELEASE_DIR "${OGHypeEngine_SOURCE_DIR}/deps/dll/osx/x86_64/Release")
   
   #SET(LIB_DIR "${OGHypeEngine_SOURCE_DIR}/deps/libs/osx/x86_64/$(CONFIGURATION)")
	#SET(DLL_DIR "${OGHypeEngine_SOURCE_DIR}/deps/dll/osx/x86_64/$(CONFIGURATION)")
ELSEIF(WIN32)
	SET(LIB_DEBUG_DIR "${OGHypeEngine_SOURCE_DIR}/deps/libs/win/x64/Debug")
	SET(DLL_DEBUG_DIR "${OGHypeEngine_SOURCE_DIR}/deps/dll/win/x64/Debug")

	SET(LIB_RELEASE_DIR "${OGHypeEngine_SOURCE_DIR}/deps/libs/win/x64/Release")
	SET(DLL_RELEASE_DIR "${OGHypeEngine_SOURCE_DIR}/deps/dll/win/x64/Release")

	SET(LIB_DIR "${OGHypeEngine_SOURCE_DIR}/deps/libs/win/x64/${CMAKE_CFG_INTDIR}")
	SET(DLL_DIR "${OGHypeEngine_SOURCE_DIR}/deps/dll/win/x64/${CMAKE_CFG_INTDIR}")

ELSEIF(ANDROID)
	#IF(${ARCH} STREQUAL "armeabi-v7a")
	#	SET(LIB_DIR "${OGHypeEngine_SOURCE_DIR}/deps/libs/android/armeabi-v7a")
	#	SET(DLL_DIR "${OGHypeEngine_SOURCE_DIR}/deps/dll/android/armeabi-v7a")
	#ELSEIF(${ARCH} STREQUAL "arm64-v8a")
	#	SET(LIB_DIR "${OGHypeEngine_SOURCE_DIR}/deps/libs/android/arm64-v8a")
	#	SET(DLL_DIR "${OGHypeEngine_SOURCE_DIR}/deps/dll/android/arm64-v8a")
	#ENDIF()
ENDIF()

#SET(ANDROID_SDK_PATH CACHE PATH "")
#SET(ANDROID_NDK_PATH CACHE PATH "")
#SET(ANDROID_CMAKE_EXE CACHE FILEPATH "")

#IF(APPLE)
#	SET(JEMALLOC ${LIB_DIR}/libjemalloc.a CACHE PATH "")
#ELSEIF(WIN32)
#	SET(JEMALLOC optimized ${LIB_DIR}/jemalloc.lib debug ${LIB_DIR}/jemallocd.lib CACHE PATH "")
#ENDIF()
#
#IF (JEMALLOC)
#    MESSAGE("Using bundled JEMALLOC version ${JEMALLOC}")
#ELSE()
#	MESSAGE("Cannot Find JEMALLOC LIB")
#ENDIF()

#find_library(VULKAN NAMES vulkan-1 vulkan libvulkan vulkan.1 HINTS ${DLL_DIR} ${LIB_DIR} REQUIRED)
IF(APPLE)
	SET(VULKAN ${DLL_DEBUG_DIR}/libvulkan.1.dylib CACHE PATH "")
ELSEIF(WIN32)
	SET(VULKAN ${LIB_DEBUG_DIR}/vulkan-1.lib CACHE PATH "")
ENDIF()
IF (VULKAN)
    MESSAGE("Using bundled VULKAN version ${VULKAN}")
	# 이건 반드시 CACHE INTERNAL. Render CMakeList에서 사용함.
ELSE()
	MESSAGE("Cannot Find VULKAN LIB")
ENDIF()

IF(APPLE)
	

ELSEIF(WIN32)
	SET(GLFW3 ${LIB_DEBUG_DIR}/glfw3.lib CACHE PATH "") #TODO delete
ENDIF()
IF (GLFW3)
    MESSAGE("Using bundled GLFW3 version ${GLFW3}")
	# 이건 반드시 CACHE INTERNAL. Render CMakeList에서 사용함.
ELSE()
	MESSAGE("Cannot Find GLFW3 LIB")
ENDIF()


#find_library(FBX NAMES libfbxsdk.a fbxsdk libfbxsdk-md HINTS ${DLL_DIR} ${LIB_DIR} REQUIRED)
#IF(APPLE)
#    SET(FBX ${LIB_DIR}/libfbxsdk.a CACHE PATH "")
#ELSEIF(WIN32)
#    SET(FBX ${LIB_DIR}/libfbxsdk-md.lib CACHE PATH "")
#ENDIF()
#IF (FBX)
#    MESSAGE("Using bundled FBX version ${FBX}")
#ELSE()
#    MESSAGE("Cannot Find FBX LIB")
#ENDIF()
#
##find_library(MONO NAMES mono-2.0-sgen monosgen-2.0 HINTS ${DLL_DIR} ${LIB_DIR} REQUIRED)
#IF(APPLE)
#	SET(MONO ${DLL_DIR}/libmonosgen-2.0.dylib CACHE PATH "")
#ELSEIF(WIN32)
#	SET(MONO ${LIB_DIR}/mono-2.0-sgen.lib CACHE PATH "")
#ENDIF()
#IF (MONO)
#    MESSAGE("Using bundled MONO version ${MONO}")
#ELSE()
#	MESSAGE("Cannot Find MONO LIB")
#ENDIF()
#
## 빌드 실패시 6.0.0 의 i386, x86_64 dylib 으로.
##find_library(MONO NAMES mono-2.0-sgen monosgen-2.0 HINTS ${DLL_DIR} ${LIB_DIR} REQUIRED)
#IF(APPLE)
#	SET(MONO ${DLL_DIR}/libmonosgen-2.0.dylib CACHE PATH "")
#ELSEIF(WIN32)
#	SET(MONO ${LIB_DIR}/mono-2.0-sgen.lib CACHE PATH "")
#ENDIF()
#IF (MONO)
#    MESSAGE("Using bundled MONO version ${MONO}")
#ELSE()
#	MESSAGE("Cannot Find MONO LIB")
#ENDIF()


#IF (WIN32)
#	set(PATH_LBV_MAIN_WINDOWS "${Lv1Engine_SOURCE_DIR}/tools/LBV/LBV_Main_Windows.py" CACHE STRING "")
#ENDIF()


SET(INSTALL_FOLDER "$<TARGET_FILE_DIR:application>")


include_directories(sources)
include_directories(deps/include)

# 모두 설정해주고, Sub Directory를 추가하여 다른 CMake들 실행
set(RENDER sources/render)
#SET(ENGINE sources/engine)
#SET(ENTRY sources/entry)
#SET(TEMPLATE sources/template)

add_subdirectory(${RENDER})
#add_subdirectory(${ENGINE})
#add_subdirectory(${ENTRY})
#add_dependencies(Play-Entry engine)


#OPTION(SCRIPT_GENERATOR_PROJECT "Editor-Script project c++ bridge code generator project" OFF)

IF(NOT ANDROID AND NOT IOS)
	# set(APPLICATION_CURREN_REVISION  "c42b1364304a54cf6d525850ae762d21c8a0940c" )
	# set(APPLICATION_BUILD_NUMBER		0 )
	# set(APPLICATION_BUILD_VERSION	"2021.1.0a" )

	set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
	#SET(REFLECTION tools/Lv1Reflection)
	#add_subdirectory(${REFLECTION})
	

	# only include in desktop
	#SET(SHADERSYSTEM tools/Lv1ShaderSystem)
	#add_subdirectory(${SHADERSYSTEM})
	
	
	#SET(EDITOR sources/editor)
	#add_subdirectory(${EDITOR})
	#add_dependencies(editor LvShaderSystem)
	
	
	set(APPLICATION_NAME "application")
	set(APPLICATION sources/application)
	add_subdirectory(${APPLICATION})
	add_dependencies(application render)

	#IF(WIN32)
	#	IF(INCLUDE_VSIX)
	#		SET(VSIX "tools/Lv1VSIX/LvVSIX")
	#		add_subdirectory(${VSIX})
	#	ENDIF()
	#ENDIF()

	#IF(SCRIPT_GENERATOR_PROJECT)
	#	SET(SCRIPT_GENERATOR script/generator)
	#	add_subdirectory(${SCRIPT_GENERATOR})
	#ENDIF()
	#SET(ANDROID_BUILD script/android)
	#add_subdirectory(${ANDROID_BUILD})
	#
	#ADD_DEPENDENCIES(LvReflect-Engine LvReflection)
	#ADD_DEPENDENCIES(LvReflect-Editor LvReflection)
	#
	#set(APP_NAME ${CMAKE_PROJECT_NAME})
	#set(CPACK_NSIS_CONTACT "Com2uS ECO GE Team")
	#
	#set(CPACK_PACKAGE_VENDOR "Com2uS")
	#set(CPACK_SYSTEM_NAME x64)
	#set(CPACK_CREATE_DESKTOP_LINKS ${CMAKE_PROJECT_NAME})
	#set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CMAKE_PROJECT_NAME})
	#set(CPACK_PACKAGE_VERSION 2021.1a1)
	#set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION} (Win64)")
	
	#set(CPACK_THREADS 0)
	#IF (WIN32)
	#	INSTALL(PROGRAMS "${CMAKE_SOURCE_DIR}/deps/install/win64/cmake-3.20.1-windows-x86_64.msi" DESTINATION ${INSTALL_FOLDER}/temp)
	#	#cpack_add_component(CMAKE_INSTALL DISPLAY_NAME "cmake install" REQUIRED DOWNLOADED)
	#	#cpack_configure_downloads(https://github.com/Kitware/CMake/releases/download/v3.20.1/cmake-3.20.1-windows-x86_64.msi)
	#	# 설치 시 관리자 권한으로 실행하는 인스톨러로 생성
	#	set(CPACK_NSIS_DEFINES "RequestExecutionLevel admin")
	#	set(CPACK_NSIS_HELP_LINK "http://devgit.com2us.com/TS/TPact/-/wikis/home")
	#	set(CPACK_NSIS_URL_INFO_ABOUT "http://devgit.com2us.com/TS/TPact")
	#	set(CPACK_NSIS_COMPRESSOR "/SOLID zlib")
	#	set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
	#	set(CPACK_NSIS_PACKAGE_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} (Win64)")
	#	set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
	#	set(CPACK_NSIS_DISPLAY_NAME "${CMAKE_PROJECT_NAME}")
	#	
	#	set(APPLINK_NAME "${CMAKE_PROJECT_NAME} (${CPACK_PACKAGE_VERSION}).lnk")	
	#	
	#	set(CPACK_CREATE_DESKTOP_LINKS ${CMAKE_PROJECT_NAME})
	#	# 바로가기 설치
	#	set(CPACK_NSIS_CREATE_ICONS_EXTRA
	#		"CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\${APPLINK_NAME}' '$INSTDIR\\\\${APPLICATION_NAME}.exe'"
	#	)
	#	#SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
	#	#NSISdl::download https://github.com/Kitware/CMake/releases/download/v3.20.1/cmake-3.20.1-windows-x86_64.msi $INSTDIR\\\\cmake-3.20.1-windows-x86_64.msi
	#	#ExecWait 'msiexec /i \\\"$INSTDIR\\\\cmake-3.20.1-windows-x86_64.msi\\\" /passive '
	#	#Delete \\\"$INSTDIR\\\\cmake-3.20.1-windows-x86_64.msi\\\"
	#	#")
	#
	#	# 바로가기 제거
	#	set(CPACK_NSIS_DELETE_ICONS_EXTRA
	#		"Delete '$SMPROGRAMS\\\\$START_MENU\\\\${APPLINK_NAME}'"
	#	)	
	#	list(APPEND CPACK_NSIS_EXTRA_INSTALL_COMMANDS "MessageBox MB_OK 'Lv1Engine needs to install cmake, Are you sure?'")
	#	list(APPEND CPACK_NSIS_EXTRA_INSTALL_COMMANDS "ExecWait 'msiexec /i \\\"$INSTDIR\\\\temp\\\\cmake-3.20.1-windows-x86_64.msi\\\" /quiet '")
	#	set(CPACK_NSIS_MUI_FINISHPAGE_RUN "..\\\\${APPLICATION_NAME}.exe")
	#ENDIF()
	
	#include(CPack)
ENDIF()

#IF(ANDROID)
#	IF(TEST_PROJECT)
#		SET(ANDROID_TEST src/test)
#		add_subdirectory(${ANDROID_TEST})
#	ENDIF()
#ENDIF()

#set_property(GLOBAL PROPERTY USE_FOLDERS ON)

