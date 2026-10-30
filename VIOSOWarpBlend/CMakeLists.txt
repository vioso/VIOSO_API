find_package(OpenGL REQUIRED)

add_library(VIOSOWarpBlend SHARED
    logging.cpp
    PathHelper.cpp
    VIOSOWarpBlend.cpp
    VWF.cpp
    GL/GLWarpBlend.cpp
    GL/GLWarpBlendXPL.cpp
    3rdparty/aes/aes.c
    3rdparty/tinyxml2/tinyxml2.cpp
)

set_target_properties(VIOSOWarpBlend PROPERTIES C_VISIBILITY_PRESET hidden CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
target_include_directories(VIOSOWarpBlend PUBLIC "${PROJECT_SOURCE_DIR}/Include")
target_compile_definitions(VIOSOWarpBlend PRIVATE VIOSOWARPBLEND_EXPORTS)

if(WIN32)
    target_sources(VIOSOWarpBlend PRIVATE
        DX/DXWarpBlend.cpp
        DX/DX9WarpBlend.cpp
        DX/DX9EXWarpBlend.cpp
        DX/DX10WarpBlend.cpp
        DX/DX11WarpBlend.cpp
        DX/DX12WarpBlend.cpp
        VIOSOWarpBlend.rc
    )
    target_compile_definitions(VIOSOWarpBlend PRIVATE WIN32 _WINDOWS _USRDLL)
    target_link_libraries(VIOSOWarpBlend PRIVATE opengl32 glu32 shlwapi ws2_32 dxguid d3d11 d3d10)

    # Keep the established file names: VIOSOWarpBlend64.dll / VIOSOWarpBlend.dll
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set_target_properties(VIOSOWarpBlend PROPERTIES OUTPUT_NAME VIOSOWarpBlend64)
    endif()

    if(MSVC)
        # Static runtime as before, so the DLL has no VC redistributable dependency
        set_target_properties(VIOSOWarpBlend PROPERTIES
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        target_compile_options(VIOSOWarpBlend PRIVATE /W3)
        if(CMAKE_SIZEOF_VOID_P EQUAL 4)
            target_link_options(VIOSOWarpBlend PRIVATE /LARGEADDRESSAWARE)
        endif()
    endif()
else()
    target_link_libraries(VIOSOWarpBlend PRIVATE OpenGL::GL ${CMAKE_DL_LIBS})
    if(TARGET OpenGL::GLU)
        target_link_libraries(VIOSOWarpBlend PRIVATE OpenGL::GLU)
    endif()
endif()
