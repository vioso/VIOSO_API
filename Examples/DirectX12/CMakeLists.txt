cmake_minimum_required(VERSION 3.9)

project(DirectX12 DESCRIPTION "VIOSOWarpBlend D3D 12 Example")

find_package(stb REQUIRED)

set(SOURCES	"stdafx.cpp" "D3D12HelloTexture.cpp" "DXSample.cpp" "Main.cpp" "Win32Application.cpp" "shaders.hlsl" "${COMMON_DIR}/logging.cpp" )
set(HEADERS	"d3dx12.h" "stdafx.h" "D3D12HelloTexture.h" "DXSample.h" "DXSampleHelper.h" "Win32Application.h" "${COMMON_DIR}/logging.h" )

add_executable(DirectX12 WIN32 ${SOURCES} ${HEADERS})

add_custom_command(
    TARGET DirectX12 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders.hlsl
            $<TARGET_FILE_DIR:DirectX12>
)

include_directories(${THIRD_PARTY_DIR}/d3dX/include/)
target_link_libraries(DirectX12 d3d12 dxgi d3dcompiler	)
target_compile_definitions(DirectX12 PRIVATE UNICODE _UNICODE)
add_dependencies(DirectX12 VIOSOWarpBlend)
