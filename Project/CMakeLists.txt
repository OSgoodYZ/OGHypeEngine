# HypeClothEngine version 0.1
# ref https://www.tuwlab.com/27270
cmake_minimum_required (VERSION 3.8)

project( HypeClothEngine )

# set(PROJECT_ROOT ${HypeClothEngine_SOURCE_DIR}/src)

# INCLUDE_DIRECTORIES() 헤더 디렉토리 추가
include_directories(sources)
include_directories(SYSTEM deps/include)


# 모두 설정해주고, Sub Directory를 추가하여 다른 CMake들 실행, 밑단부터 차근차근 만들 계획!

#SET(SYSTEM sources/system)
SET(RENDER sources/render)
#SET(ENGINE sources/engine)
#SET(ENTRY sources/entry)
#SET(SAMPLE sources/sample)
#SET(TEMPLATE sources/template)

#add_subdirectory(${SYSTEM})
add_subdirectory(${RENDER})
#add_subdirectory(${ENGINE})
#add_subdirectory(${ENTRY})
#add_dependencies(Play-Entry engine)



