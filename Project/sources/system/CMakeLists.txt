# set(CMAKE_CXX_STANDARD 11)

set(PLATFORM_COMMON 
	platform/win/OgPlatformDefinition_win32.h
	)
source_group("plaform" FILES ${PLATFORM_COMMON})
list(APPEND TOTAL_FILE ${PLATFORM_COMMON})


set(PUBLIC_COMMON 
	OgSystemContext.h
	OgSystemContext.cpp
	OgInput.h
	OgInput.cpp
	OgMemory.h
	OgMemory.cpp
	OgHashCode.h
	OgHashCode.cpp
	)
source_group("public" FILES ${PUBLIC_COMMON})
list(APPEND TOTAL_FILE ${PUBLIC_COMMON})

SET(SOURCE_GLM
	${PROJECT_SOURCE_DIR}/deps/include/glm/detail/glm.cpp
)
SOURCE_GROUP("thirdparty\\glm" FILES ${SOURCE_GLM})
LIST(APPEND TOTAL_FILE ${SOURCE_GLM})


add_library(system STATIC ${TOTAL_FILE})
link_directories(${LIB_DIR})
target_link_libraries(system PUBLIC ${GLFW3})


# target_compile_options(render PUBLIC -Wall)
IF(OG_ENGINE_SHARED_LIB)
	target_compile_definitions(system PRIVATE "-DOG_API_EXPORT")
ENDIF()

#GUI framework
target_link_directories(system PRIVATE ${GLFW3} ${SOURCE_GLM})	

## DEBUG
INSTALL(TARGETS system CONFIGURATIONS Debug ARCHIVE DESTINATION "${INSTALL_FOLDER}/dev/lib" LIBRARY DESTINATION "${INSTALL_FOLDER}" RUNTIME DESTINATION "${INSTALL_FOLDER}")
install(DIRECTORY "." CONFIGURATIONS Debug PUBLIC_HEADER DESTINATION "${INSTALL_FOLDER}/dev/include/system" FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES "../OgPrecompiled.h" CONFIGURATIONS Debug PUBLIC_HEADER DESTINATION "${INSTALL_FOLDER}/dev/include")


## RELEASE

install(TARGETS system CONFIGURATIONS Release ARCHIVE DESTINATION "${INSTALL_FOLDER}/dev/lib")
install(DIRECTORY "." CONFIGURATIONS Release PUBLIC_HEADER DESTINATION "${INSTALL_FOLDER}/dev/include/system" FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES "../OgPrecompiled.h" CONFIGURATIONS Release PUBLIC_HEADER DESTINATION "${INSTALL_FOLDER}/dev/include")
## RELWITHDEBINFO

install(TARGETS system CONFIGURATIONS RelWithDebInfo ARCHIVE DESTINATION "${INSTALL_FOLDER}/dev/lib")
install(DIRECTORY "." CONFIGURATIONS RelWithDebInfo PUBLIC_HEADER DESTINATION "${INSTALL_FOLDER}/dev/include/system" FILES_MATCHING PATTERN "*.h" PATTERN "private" EXCLUDE)
install(FILES "../OgPrecompiled.h" CONFIGURATIONS RelWithDebInfo PUBLIC_HEADER DESTINATION "${INSTALL_FOLDER}/dev/include")
