# Direct3D 12 example: a textured scene warped by VWB_render. Starts with the sample data next to the executable.
if(NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS "DirectX12 example skipped: needs MSVC and x64")
    return()
endif()

add_executable(DirectX12Example WIN32
    D3D12HelloTexture.cpp
    DXSample.cpp
    Main.cpp
    Win32Application.cpp
)
set_target_properties(DirectX12Example PROPERTIES OUTPUT_NAME D3D12Example)

target_compile_definitions(DirectX12Example PRIVATE WIN32 _WINDOWS UNICODE _UNICODE)
target_include_directories(DirectX12Example PRIVATE "${PROJECT_SOURCE_DIR}/Include")
target_link_libraries(DirectX12Example PRIVATE d3d12 dxgi d3dcompiler)

# The library is loaded at runtime, but has to be built first
add_dependencies(DirectX12Example VIOSOWarpBlend SampleData)

# The shaders are compiled at start from the executable's folder
add_custom_command(TARGET DirectX12Example POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/shaders.hlsl"
            "$<TARGET_FILE_DIR:DirectX12Example>"
)
