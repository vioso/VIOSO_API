cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
find_path(Stb_INCLUDE_DIR NAMES stb_image.h)
if (NOT Stb_INCLUDE_DIR)
  message(FATAL_ERROR "stb headers not found. Install 'libstb-dev' or set Stb_INCLUDE_DIR")
endif()

file( GLOB HEADERS ../include/*.h ../include/*.hpp )

set( SOURCES logging.cpp PathHelper.cpp VIOSOWarpBlend.cpp VWF.cpp dp.cpp ${THIRD_PARTY_DIR}/aes/aes.c GL/GLWarpBlend.cpp GL/GLWarpBlendXPL.cpp)
set( HEADERS ${HEADERS} common.h resource.h mmath.h logging.h PathHelper.h VWF.h dp.h ${THIRD_PARTY_DIR}/aes/aes.h GL/GLWarpBlend.h GL/GLWarpBlendXPL.h)

if(WIN32)
    set( SOURCES ${SOURCES}  DX/DXWarpBlend.cpp DX/DX9EXWarpBlend.cpp DX/DX9WarpBlend.cpp DX/DX10WarpBlend.cpp DX/DX11WarpBlend.cpp DX/DX12WarpBlend.cpp )
    set( HEADERS ${HEADERS}  DX/DXWarpBlend.h DX/DX9EXWarpBlend.h DX/DX9WarpBlend.h DX/DX10WarpBlend.h DX/DX11WarpBlend.h DX/DX12WarpBlend.h )
endif(WIN32)

add_library(VIOSOWarpBlend SHARED ${SOURCES} ${HEADERS})
if(WIN32)
    find_package(tinyxml2 REQUIRED)
    set_target_properties(VIOSOWarpBlend PROPERTIES OUTPUT_NAME "VIOSOWarpBlend64")
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(TINYXML2 REQUIRED tinyxml2)
endif(WIN32)

target_include_directories(VIOSOWarpBlend PRIVATE "${Stb_INCLUDE_DIR}" )

if(WIN32)
    target_link_libraries(VIOSOWarpBlend opengl32 glu32 tinyxml2 )
else()
    target_link_libraries(VIOSOWarpBlend PRIVATE GL GLU libdl.so ${TINYXML2_LIBRARIES} )
    target_compile_options(VIOSOWarpBlend PRIVATE -fvisibility=hidden "-Wno-multichar")
    target_link_options(VIOSOWarpBlend BEFORE PRIVATE "-rdynamic")
endif(WIN32)
target_compile_definitions( VIOSOWarpBlend PRIVATE VIOSOWARPBLEND_EXPORTS)

get_target_property(INC_DIRS VIOSOWarpBlend INCLUDE_DIRECTORIES)
message(STATUS "Include dirs: " ${INC_DIRS})