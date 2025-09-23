cmake_minimum_required(VERSION 3.5)

project(OpenGL_4 DESCRIPTION "VIOSOWarpBlend OpenGL Example")

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)

add_executable(OpenGL_4 gl3_2.cpp)

if(WIN32)
target_compile_definitions(OpenGL_4 PRIVATE UNICODE _UNICODE)
else()
find_package(X11 REQUIRED)
target_include_directories(OpenGL_4 PRIVATE ${X11_INCLUDE_DIR})
target_link_libraries(OpenGL_4 PRIVATE libdl.so GL GLU ${X11_LIBRARIES})
endif()
add_dependencies(OpenGL_4 VIOSOWarpBlend)

