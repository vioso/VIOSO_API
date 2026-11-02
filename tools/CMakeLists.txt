# Tools, not part of the API. MakeTestVwf is always built (on Windows and Linux) and goes into the release package;
# WarpPrecision needs -DVIOSO_BUILD_TOOLS=ON.

# Generator for synthetic .vwf test files. It compiles the vwf reader/writer of the library
# directly, so it does not need the DLL.
add_executable(MakeTestVwf
    MakeTestVwf.cpp
    "${PROJECT_SOURCE_DIR}/VIOSOWarpBlend/VWF.cpp"
    "${PROJECT_SOURCE_DIR}/VIOSOWarpBlend/logging.cpp"
    "${PROJECT_SOURCE_DIR}/VIOSOWarpBlend/PathHelper.cpp"
    "${PROJECT_SOURCE_DIR}/VIOSOWarpBlend/3rdparty/aes/aes.c"
)
target_include_directories(MakeTestVwf PRIVATE
    "${PROJECT_SOURCE_DIR}/Include"
    "${PROJECT_SOURCE_DIR}/VIOSOWarpBlend"
)
if(WIN32)
    target_compile_definitions(MakeTestVwf PRIVATE WIN32 _WINDOWS)
    target_link_libraries(MakeTestVwf PRIVATE shlwapi)
endif()

# Measures the precision of the warp texture with a very wide content space (Windows: Direct3D 11, 12 and OpenGL).
# It loads the library at runtime, like the examples, and needs a file made with MakeTestVwf, see the top of the source.
if(VIOSO_BUILD_TOOLS AND WIN32 AND MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_executable(WarpPrecision WarpPrecision.cpp)
    target_include_directories(WarpPrecision PRIVATE "${PROJECT_SOURCE_DIR}/Include")
    target_compile_definitions(WarpPrecision PRIVATE WIN32 _WINDOWS)
    target_link_libraries(WarpPrecision PRIVATE d3d11 d3d12 dxgi opengl32)
    add_dependencies(WarpPrecision VIOSOWarpBlend)
endif()
