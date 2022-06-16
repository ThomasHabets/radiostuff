AC_PREREQ([2.71])
AC_INIT([modem-tun],[0.01],[thomas@habets.se])
AC_CONFIG_SRCDIR([src/tun-ax25.cc])
AM_INIT_AUTOMAKE([foreign])
AM_MAINTAINER_MODE
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AC_CHECK_LIB([util], [openpty])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

echo "
  $PACKAGE_NAME version $PACKAGE_VERSION
  Prefix.........: $prefix
  Debug Build....: $debug
  C++ Compiler...: $CXX $CXXFLAGS $CPPFLAGS
  Linker.........: $LD $LDFLAGS $LIBS
"
