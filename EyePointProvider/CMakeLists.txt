# Sample eye point provider plugin, loaded by the library at runtime
# (ini setting "eyePointProvider")
add_library(EyePointProvider SHARED
    src/EyePointProvider.cpp
    ../VIOSOWarpBlend/socket.cpp
    ../VIOSOWarpBlend/logging.cpp
)
target_include_directories(EyePointProvider PRIVATE "${PROJECT_SOURCE_DIR}/Include")
target_compile_definitions(EyePointProvider PRIVATE EYEPOINTPROVIDER_EXPORTS)

if(WIN32)
    target_compile_definitions(EyePointProvider PRIVATE WIN32 _WINDOWS _USRDLL)
    target_link_libraries(EyePointProvider PRIVATE ws2_32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set_target_properties(EyePointProvider PROPERTIES OUTPUT_NAME EyePointProvider64)
    endif()
endif()
