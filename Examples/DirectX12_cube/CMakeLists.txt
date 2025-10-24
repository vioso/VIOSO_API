set(PROJECT_NAME DirectX12_cube)

set(Headers
    "D3D12Base.h"
    "${THIRD_PARTY_DIR}/d3dX/Include/d3dx12.h"
)
source_group("Headers" FILES ${Headers})

set(Sources
    "main.cpp"
)
source_group("Sources" FILES ${Sources})

add_executable(${PROJECT_NAME} ${Headers} ${Sources} )

target_compile_definitions(${PROJECT_NAME} PRIVATE "_UNICODE" "UNICODE" )

include_directories(${THIRD_PARTY_DIR}/d3dX/include/)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    VIOSOWarpBlend
    "d3d12"
    "dxgi"
    "dxguid"
)
