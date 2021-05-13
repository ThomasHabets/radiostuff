AC_PREREQ(2.61)
AC_INIT(pskreporter, 0.01, thomas@habets.se)
AC_CANONICAL_SYSTEM
AC_CONFIG_SRCDIR([src/gridsquares.cc])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE
AC_CONFIG_HEADER(config.h)

# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MAKE_SET

CXXFLAGS="$CXXFLAGS -std=c++20"
CXXFLAGS="$CXXFLAGS -Wall -pedantic"
CXXFLAGS="$CXXFLAGS -march=native -mtune=native -Ofast -flto -fuse-linker-plugin"

# Checks for libraries.
AC_CHECK_LIB([m], [sqrt])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

echo "
  $PACKAGE_NAME version $PACKAGE_VERSION
  Prefix.........: $prefix
  Debug Build....: $debug
  C Compiler.....: $CXX $CXXFLAGS $CPPFLAGS
  Linker.........: $LD $LDFLAGS $LIBS
"
