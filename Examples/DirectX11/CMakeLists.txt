# The bundled third-party binaries (assimp, libpng, zlib) are x64 MSVC builds.
if(NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS "DirectX11 example skipped: needs MSVC and x64")
    return()
endif()

add_executable(DirectX11Example WIN32
    DX11Example.cpp
    DX11Program.cpp
    AssimpRenderer.cpp
    DX11Example.rc
    DX11Example.manifest
)
set_target_properties(DirectX11Example PROPERTIES OUTPUT_NAME VWFTesterDX11)

target_compile_definitions(DirectX11Example PRIVATE WIN32 _WINDOWS UNICODE _UNICODE)
target_include_directories(DirectX11Example PRIVATE
    "${PROJECT_SOURCE_DIR}/Include"
    "${CMAKE_CURRENT_SOURCE_DIR}/assimp/include"
)
target_link_libraries(DirectX11Example PRIVATE
    d3d11 dxgi d3dcompiler comdlg32
    "${CMAKE_CURRENT_SOURCE_DIR}/assimp/lib/assimp-vc142-mt.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/lpng16/libpng16_static.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/zlib/zlibstatic.lib"
)

# The library is loaded at runtime, but has to be built first
add_dependencies(DirectX11Example VIOSOWarpBlend SampleData)

add_custom_command(TARGET DirectX11Example POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/assimp/bin/assimp-vc142-mt.dll"
            "$<TARGET_FILE_DIR:DirectX11Example>"
)

# The media server example: several outputs on one device, one warper per channel. Does not need assimp.
add_executable(MediaServerExample
    MediaServerExample.cpp
    DX11Program.cpp
)
set_target_properties(MediaServerExample PROPERTIES OUTPUT_NAME MediaServerExampleDX11)
target_compile_definitions(MediaServerExample PRIVATE WIN32 _WINDOWS UNICODE _UNICODE)
target_include_directories(MediaServerExample PRIVATE "${PROJECT_SOURCE_DIR}/Include")
target_link_libraries(MediaServerExample PRIVATE
    d3d11 dxgi d3dcompiler
    "${CMAKE_CURRENT_SOURCE_DIR}/lpng16/libpng16_static.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/zlib/zlibstatic.lib"
)
add_dependencies(MediaServerExample VIOSOWarpBlend SampleData)
