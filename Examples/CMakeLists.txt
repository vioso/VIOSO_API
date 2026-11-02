# Puts the sample configuration and the sample calibration next to the executables,
# so the examples start from there without any manual work. Files that already exist
# are left alone (own changes, disk space).
set(_sample_dir "$<TARGET_FILE_DIR:VIOSOWarpBlend>")
set(_sample_cmds)
foreach(_f VIOSOWarpBlend.ini VIOSOWarpBlendGL.ini 2D_plane_FHD.vwf earth.png)
    list(APPEND _sample_cmds COMMAND "${CMAKE_COMMAND}"
         "-DSRC=${PROJECT_SOURCE_DIR}/data/${_f}" "-DDST=${_sample_dir}/${_f}"
         -P "${PROJECT_SOURCE_DIR}/cmake/CopyIfMissing.cmake")
endforeach()
add_custom_target(SampleData ${_sample_cmds}
    COMMENT "Copying sample configuration and calibration next to the executables, if missing")

add_subdirectory(OpenGL_4)

if(WIN32)
    add_subdirectory(DirectX11)
    add_subdirectory(DirectX12)
endif()
