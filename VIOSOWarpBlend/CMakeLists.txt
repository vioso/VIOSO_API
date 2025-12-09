cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
find_path(Stb_INCLUDE_DIR NAMES stb_image.h)
if (NOT Stb_INCLUDE_DIR)
  message(FATAL_ERROR "stb headers not found. apt install libstb-dev, vcpkg install stb or add to path")
endif()

file( GLOB HEADERS ../include/*.h ../include/*.hpp )

set( SOURCES ${COMMON_DIR}/logging.cpp ${COMMON_DIR}/PathHelper.cpp WarperBase.cpp DummyWarper.cpp VIOSOWarpBlend.cpp VWF.cpp dpXML.cpp ${THIRD_PARTY_DIR}/tinyxml2/tinyxml2.cpp ${THIRD_PARTY_DIR}/aes/aes.c GL/GLWarpBlend.cpp GL/GLWarpBlendXPL.cpp )
set( HEADERS ${HEADERS} common.h WarperBase.h DummyWarper.h resource.h dpLogo.h ${COMMON_DIR}/mmath.h ${COMMON_DIR}/logging.h ${COMMON_DIR}/PathHelper.h VWF.h dpXML.h ${COMMON_DIR}/licCM.h ${THIRD_PARTY_DIR}/tinyxml2/tinyxml2.h ${THIRD_PARTY_DIR}/aes/aes.h GL/GLWarpBlend.h GL/GLWarpBlendXPL.h GL/pixelshader.h )

if(WIN32)  
   set( SOURCES ${SOURCES}  DX/DXWarpBlend.cpp DX/DX9EXWarpBlend.cpp DX/DX9WarpBlend.cpp DX/DX10WarpBlend.cpp DX/DX11WarpBlend.cpp DX/DX12WarpBlend.cpp VIOSOWarpBlend.rc )  
   set( HEADERS ${HEADERS}  DX/DXWarpBlend.h DX/DX9EXWarpBlend.h DX/DX9WarpBlend.h DX/DX10WarpBlend.h DX/DX11WarpBlend.h DX/DX12WarpBlend.h DX/pixelshader.h )
endif(WIN32)

add_library(VIOSOWarpBlend SHARED ${SOURCES} ${HEADERS})
if(WIN32)
    set_target_properties(VIOSOWarpBlend PROPERTIES OUTPUT_NAME "VIOSOWarpBlend64")
else()
    find_package(PkgConfig REQUIRED)
endif(WIN32)

target_include_directories(VIOSOWarpBlend PRIVATE "${Stb_INCLUDE_DIR}" )
target_include_directories(VIOSOWarpBlend PRIVATE "${THIRD_PARTY_DIR}/CodeMeter8/include" )
# try to load Codemeter dynamically at runtime, so we could raise an error if not found or swap to another licensing system
#target_link_directories(VIOSOWarpBlend PRIVATE "${THIRD_PARTY_DIR}/CodeMeter8/lib" )
if(WIN32)
    target_link_libraries(VIOSOWarpBlend opengl32 glu32 ) #WibuCm64
else()
    target_link_libraries(VIOSOWarpBlend PRIVATE GL GLU libdl.so ) #libwibucmhip.so
    target_compile_options(VIOSOWarpBlend PRIVATE -fvisibility=hidden "-Wno-multichar")
    target_link_options(VIOSOWarpBlend BEFORE PRIVATE "-rdynamic")
endif(WIN32)
target_compile_definitions( VIOSOWarpBlend PRIVATE VIOSOWARPBLEND_EXPORTS)

get_target_property(INC_DIRS VIOSOWarpBlend INCLUDE_DIRECTORIES)
message(STATUS "Include dirs: " ${INC_DIRS})
