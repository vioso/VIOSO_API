cmake_minimum_required(VERSION 3.5)

project(OpenGL_4_vwfInfo DESCRIPTION "VIOSOWarpBlend OpenGL Example, open windows based on vwf data")

add_executable(OpenGL_4_vwfInfo gl4_vwfInfo.cpp)
set_property(TARGET OpenGL_4_vwfInfo PROPERTY CXX_STANDARD 20)

IF(WIN32)
target_link_libraries(OpenGL_4_vwfInfo opengl32 glu32)
ELSE()
target_link_libraries(OpenGL_4_vwfInfo X11 GL GLU)
ENDIF()
include_directories(${CMAKE_SOURCE_DIR}/Include)
install(TARGETS OpenGL_4_vwfInfo
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
