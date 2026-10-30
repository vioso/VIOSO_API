# Copies SRC to DST unless DST already exists.
# Used for sample data next to the executables: changes made there are never overwritten
# and big files are not copied again. Usage: cmake -DSRC=<file> -DDST=<file> -P CopyIfMissing.cmake
if(NOT EXISTS "${SRC}")
    message(WARNING "CopyIfMissing: ${SRC} does not exist")
elseif(NOT EXISTS "${DST}")
    get_filename_component(_dir "${DST}" DIRECTORY)
    file(MAKE_DIRECTORY "${_dir}")
    file(COPY_FILE "${SRC}" "${DST}")
endif()
