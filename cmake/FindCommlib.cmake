find_path( COMMLIB_INCLUDE_DIR
    NAMES commlib/defs.h
    DOC "The commlib include directory"
    )

find_library ( COMMLIB_LIBRARY
    NAMES comm
    DOC "The commlib library"
    )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(COMMLIB
    REQUIRED_VARS COMMLIB_LIBRARY COMMLIB_INCLUDE_DIR
    )

if(COMMLIB_FOUND)
    set(COMMLIB_LIBRARIES ${COMMLIB_LIBRARY})
    set(COMMLIB_INCLUDE_DIR ${COMMLIB_INCLUDE_DIR})
endif()

mark_as_advanced(COMMLIB_LIBRARIES COMMLIB_INCLUDE_DIR)
