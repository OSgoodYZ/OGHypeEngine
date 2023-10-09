# set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_STANDARD 11)
IF(OG_SAMPLE_ON)

set(PUBLIC_COMMON
	OgSample.h
	OgSample.cpp
	)
source_group("public" FILES ${PUBLIC_COMMON})
list(APPEND TOTAL_FILE ${PUBLIC_COMMON})

set(PUBLIC_EDITOR
	public/editor/OgPlayWindow.h
	public/editor/OgPlayWindow.cpp
	)
source_group("public\\editor" FILES ${PUBLIC_EDITOR})
list(APPEND TOTAL_FILE ${PUBLIC_EDITOR})

add_library(sample STATIC ${TOTAL_FILE})
link_directories(${LIB_DIR})
target_link_libraries(sample render)#${GLFW3}

IF(OG_ENGINE_SHARED_LIB)
	target_compile_definitions(sample PUBLIC "-DOG_API_EXPORT")
ENDIF()

## Common

# install(FILES ${CLANG_LIBS} DESTINATION "${INSTALL_FOLDER}")
# install(FILES ${RES_DIR}/imgui.ini CONFIGURATIONS Debug DESTINATION "${INSTALL_FOLDER}")

#if(SCRIPT_GENERATOR_PROJECT)
#	install(FILES $<TARGET_FILE_DIR:sample>/Editor-Script-Generator.exe CONFIGURATIONS Debug DESTINATION "${INSTALL_FOLDER}")
#	install(FILES $<TARGET_FILE_DIR:sample>/Editor-Script-Generator.exe CONFIGURATIONS Release DESTINATION "${INSTALL_FOLDER}")
#endif()
ENDIF()