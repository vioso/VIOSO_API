cmake_minimum_required(VERSION 3.9)
project(EyePointProvider VERSION 1.2.4 DESCRIPTION "EyeProvider")

add_compile_definitions(EYEPOINTPROVIDER_EXPORTS)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_library(${PROJECT_NAME} SHARED ${SOURCES} ../VIOSOWarpBlend/socket.cpp ../VIOSOWarpBlend/logging.cpp )

include_directories(${CMAKE_SOURCE_DIR}/Include)
