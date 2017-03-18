INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_HABETS habets)

FIND_PATH(
    HABETS_INCLUDE_DIRS
    NAMES habets/api.h
    HINTS $ENV{HABETS_DIR}/include
        ${PC_HABETS_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    HABETS_LIBRARIES
    NAMES gnuradio-habets
    HINTS $ENV{HABETS_DIR}/lib
        ${PC_HABETS_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(HABETS DEFAULT_MSG HABETS_LIBRARIES HABETS_INCLUDE_DIRS)
MARK_AS_ADVANCED(HABETS_LIBRARIES HABETS_INCLUDE_DIRS)

