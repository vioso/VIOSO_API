find_package(OpenGL REQUIRED)

add_executable(OpenGLExample WIN32 gl3_2.cpp)
target_include_directories(OpenGLExample PRIVATE "${PROJECT_SOURCE_DIR}/Include")

# The library is loaded at runtime, but has to be built first
add_dependencies(OpenGLExample VIOSOWarpBlend SampleData)

if(WIN32)
    target_compile_definitions(OpenGLExample PRIVATE WIN32 _WINDOWS)
    target_link_libraries(OpenGLExample PRIVATE opengl32 glu32)
else()
    find_package(X11 REQUIRED)
    target_link_libraries(OpenGLExample PRIVATE X11::X11 OpenGL::GL ${CMAKE_DL_LIBS})
    if(TARGET OpenGL::GLU)
        target_link_libraries(OpenGLExample PRIVATE OpenGL::GLU)
    endif()
endif()
