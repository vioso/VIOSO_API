cmake_minimum_required(VERSION 3.9)

project(VIOSO_API VERSION 1.6.6 DESCRIPTION "VIOSOWarpBlend projects")

add_subdirectory(Examples)
add_subdirectory(VIOSOWarpBlend)
add_subdirectory(EyePointProvider)

if(WIN32)
 	add_subdirectory(TrackerSim)
endif(WIN32)

