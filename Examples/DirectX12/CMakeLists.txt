cmake_minimum_required(VERSION 3.9)

project(DirectX12 DESCRIPTION "VIOSOWarpBlend D3D 12 Example")

set(SOURCES	"stdafx.cpp" "D3D12HelloTexture.cpp" "DXSample.cpp" "Main.cpp" "Win32Application.cpp" "shaders.hlsl" "../../VIOSOWarpBlend/logging.cpp" )
set(HEADERS	"d3dx12.h" "stdafx.h" "D3D12HelloTexture.h" "DXSample.h" "DXSampleHelper.h" "stb_image.h" "Win32Application.h" "../../VIOSOWarpBlend/logging.h" )

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

add_custom_command(
    TARGET DirectX12 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders.hlsl
            $<TARGET_FILE_DIR:DirectX12>
)

target_link_libraries(${PROJECT_NAME} VIOSOWarpBlend)

if(WIN32)
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		set(LIB_ARCH_PATH "Win32")
	else()
		set(LIB_ARCH_PATH "x64")
	endif()
endif(WIN32)

include_directories(${THIRD_PARTY_DIR}/d3dX/include/)
target_link_libraries (${PROJECT_NAME} 
	d3d12 dxgi d3dcompiler
	${THIRD_PARTY_DIR}/d3dX/lib/${LIB_ARCH_PATH}/dxerr.lib
	)
target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)
