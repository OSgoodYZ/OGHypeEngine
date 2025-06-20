

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
IF(OG_SAMPLE_ON)

set(PUBLIC_COMMON
	OgSampleMain.h
	OgSampleMain.cpp
	)
source_group("public" FILES ${PUBLIC_COMMON})
list(APPEND TOTAL_FILE ${PUBLIC_COMMON})

set(PUBLIC_CORE_UTIL
	public/core/util/OgRenderUtil.h
	public/core/util/OgRenderUtil.cpp
	public/core/util/OgShaderCompiler.h
	public/core/util/OgShaderCompiler.cpp
	public/core/util/OgFlyCamera.h
	public/core/util/OgFlyCamera.cpp
	)
source_group("public\\core\\util" FILES ${PUBLIC_CORE_UTIL})
list(APPEND TOTAL_FILE ${PUBLIC_CORE_UTIL})

set(PUBLIC_CORE
	public/core/OgSampleBase.h
	public/core/OgTriangleSample.h
	public/core/OgTriangleSample.cpp
	public/core/OgFBXSample.h
	public/core/OgFBXSample.cpp
	)
source_group("public\\core" FILES ${PUBLIC_CORE})
list(APPEND TOTAL_FILE ${PUBLIC_CORE})

set(PUBLIC_EDITOR
	public/editor/OgEditorWindow.h
	public/editor/OgEditorWindow.cpp
	public/editor/OgPlayWindow.h
	public/editor/OgPlayWindow.cpp
	public/editor/OgImguiRenderer.h
	public/editor/OgImguiRenderer.cpp
	)
source_group("public\\editor" FILES ${PUBLIC_EDITOR})
list(APPEND TOTAL_FILE ${PUBLIC_EDITOR})

set(PUBLIC_APPLICATION
	public/application/OgSampleApplication.h
	public/application/OgSampleApplication.cpp
	)
source_group("public\\application" FILES ${PUBLIC_APPLICATION})
list(APPEND TOTAL_FILE ${PUBLIC_APPLICATION})

add_library(sample STATIC ${TOTAL_FILE})
link_directories(${LIB_DIR})
target_link_libraries(sample render)#${GLFW3}

IF(OG_ENGINE_SHARED_LIB)
	target_compile_definitions(sample PUBLIC "-DOG_API_EXPORT")
ENDIF()

SET_TARGET_PROPERTIES(sample PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OG_PROJECT_BINARY_DIR})
## Common

# install(FILES ${CLANG_LIBS} DESTINATION "${INSTALL_FOLDER}")
# install(FILES ${RES_DIR}/imgui.ini CONFIGURATIONS Debug DESTINATION "${INSTALL_FOLDER}")

#if(SCRIPT_GENERATOR_PROJECT)
#	install(FILES $<TARGET_FILE_DIR:sample>/Editor-Script-Generator.exe CONFIGURATIONS Debug DESTINATION "${INSTALL_FOLDER}")
#	install(FILES $<TARGET_FILE_DIR:sample>/Editor-Script-Generator.exe CONFIGURATIONS Release DESTINATION "${INSTALL_FOLDER}")
#endif()
ENDIF()
