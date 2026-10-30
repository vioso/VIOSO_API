cmake_minimum_required(VERSION 3.21)

# The version lives in VIOSOWarpBlend/resource.h only. The DLL resource
# (VIOSOWarpBlend.rc) and VWB_getVersion() use it from there as well.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VIOSOWarpBlend/resource.h" _vwb_version_lines
     REGEX "^#define VWB_Version_(MAJ|MIN|MAI) +[0-9]+")
foreach(_line IN LISTS _vwb_version_lines)
    if(_line MATCHES "VWB_Version_(MAJ|MIN|MAI) +([0-9]+)")
        set(_vwb_${CMAKE_MATCH_1} ${CMAKE_MATCH_2})
    endif()
endforeach()

project(VIOSO_API
    VERSION ${_vwb_MAJ}.${_vwb_MIN}.${_vwb_MAI}
    DESCRIPTION "VIOSO Warp and Blend API"
    LANGUAGES C CXX)

option(VIOSO_BUILD_EXAMPLES "Build the example applications" ON)
option(VIOSO_BUILD_TOOLS "Build the development tool WarpPrecision (Windows); MakeTestVwf is always built" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Library and examples end up in one directory, because the examples load
# the library at runtime from next to the executable.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

add_subdirectory(VIOSOWarpBlend)
add_subdirectory(EyePointProvider)

if(VIOSO_BUILD_EXAMPLES)
    add_subdirectory(Examples)
endif()

# MakeTestVwf is part of the release package, WarpPrecision is a development tool (VIOSO_BUILD_TOOLS)
add_subdirectory(tools)
