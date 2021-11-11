# set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_STANDARD 11)
IF(OG_SAMPLE_ON)

set(COMMON 
	OgSample.h
	OgSample.cpp
	)
source_group("private" FILES ${COMMON})

list(APPEND TOTAL_FILE ${COMMON})

add_library(sample STATIC ${TOTAL_FILE})
link_directories(${LIB_DIR})
target_link_libraries(sample render)#${GLFW3}

IF(OG_ENGINE_SHARED_LIB)
	target_compile_definitions(sample PRIVATE "-DOG_API_EXPORT")
ENDIF()

## Common

# install(FILES ${CLANG_LIBS} DESTINATION "${INSTALL_FOLDER}")
# install(FILES ${RES_DIR}/imgui.ini CONFIGURATIONS Debug DESTINATION "${INSTALL_FOLDER}")

#if(SCRIPT_GENERATOR_PROJECT)
#	install(FILES $<TARGET_FILE_DIR:sample>/Editor-Script-Generator.exe CONFIGURATIONS Debug DESTINATION "${INSTALL_FOLDER}")
#	install(FILES $<TARGET_FILE_DIR:sample>/Editor-Script-Generator.exe CONFIGURATIONS Release DESTINATION "${INSTALL_FOLDER}")
#endif()
ENDIF()