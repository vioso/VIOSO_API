cmake_minimum_required(VERSION 3.9)

project(Examples)

add_subdirectory(OpenGL_1)
add_subdirectory(OpenGL_4)
#add_subdirectory(Vulkan)

if(WIN32)
    add_subdirectory(OpenGL_4_vwfInfo)
    add_subdirectory(DirectX11)
    add_subdirectory(DirectX12)
    add_subdirectory(DirectX9)
    add_subdirectory(DirectX9_render)
    add_subdirectory(DirectX9Ex)
endif(WIN32)
