set(Headers
    "D3D12Base.h"
    "${THIRD_PARTY_DIR}/d3dX/Include/d3dx12.h"
)
source_group("Headers" FILES ${Headers})

set(Sources
    "main.cpp"
)
source_group("Sources" FILES ${Sources})

add_executable(DirectX12_cube ${Headers} ${Sources} )

target_compile_definitions(DirectX12_cube PRIVATE "_UNICODE" "UNICODE" )

include_directories(${THIRD_PARTY_DIR}/d3dX/include/)

target_link_libraries(DirectX12_cube PRIVATE 
    #VIOSOWarpBlend
    "d3d12"
    "dxgi"
    "dxguid"
)
add_dependencies(DirectX12_cube VIOSOWarpBlend)

configure_file(runMulti.bat "${CMAKE_BINARY_DIR}/runMulti.bat" COPYONLY)
