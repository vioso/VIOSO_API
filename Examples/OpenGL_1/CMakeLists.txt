cmake_minimum_required(VERSION 3.9)

project(OpenGL_1)

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
include_directories(${OpenGL_INCLUDE_DIRS})
add_definitions(${OpenGL_DEFINITIONS})
if(NOT OPENGL_FOUND)
  message(ERROR "OPENGL not found!")
endif(NOT OPENGL_FOUND)

add_executable(OpenGL_1 gltest.cpp)
set_property(TARGET OpenGL_1 PROPERTY CXX_STANDARD 20)

IF(WIN32)
target_link_libraries(OpenGL_1 opengl32 glu32)
ELSE()
target_link_libraries(OpenGL_1 X11 GL GLU libdl.so)
ENDIF()
include_directories(${CMAKE_SOURCE_DIR}/Include)
install(TARGETS OpenGL_1
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
