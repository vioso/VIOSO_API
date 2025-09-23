cmake_minimum_required(VERSION 3.20.0 FATAL_ERROR)

project(VIOSO_API VERSION 2.0.0 DESCRIPTION "VIOSOWarpBlend projects" LANGUAGES CXX C)
#set(CMAKE_CSharp_COMPILER "C:/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/Roslyn/csc.exe" CACHE PATH "Set external dependencies folder. See readme_dev.md" )

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
   string(REPLACE "/Zi" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
   string(REPLACE "/Zi" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
   add_compile_options("/ZI")
   add_link_options("/INCREMENTAL" "/DEBUG")
endif()

if( VCPKG_INSTALLED_DIR )
    include_directories(${VCPKG_INSTALLED_DIR}/x64-windows/include)
    link_directories(${VCPKG_INSTALLED_DIR}/x64-windows/lib)
    message(STATUS "Use VCPKG standard include dir ${VCPKG_INSTALLED_DIR}/x64-windows/include.")
endif()

set(THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/3rdparty")
set(COMMON_DIR ${CMAKE_SOURCE_DIR}/common)
include_directories(${CMAKE_SOURCE_DIR}/Include)
include_directories(${THIRD_PARTY_DIR})
include_directories(${COMMON_DIR})

# we want all binaries to be compiled in the same directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
message( STATUS "CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}")
message( STATUS "CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}")
message( STATUS "CMAKE_OUTPUT_DIR=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

#copy resources
message( STATUS "Copy resources...")
file(COPY ${CMAKE_SOURCE_DIR}/data DESTINATION ${CMAKE_BINARY_DIR})

add_subdirectory(VIOSOWarpBlend)
#add_subdirectory(EyePointProvider)
add_subdirectory(Examples/OpenGL_4)

if(WIN32)
# 	add_subdirectory(TrackerSim)
    add_subdirectory(Examples/DirectX12)
endif(WIN32)

