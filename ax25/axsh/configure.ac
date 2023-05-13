# -*- Autoconf -*-
AC_PREREQ([2.61])
AC_INIT([axlib], [0.0.1], [thomas@habets.se])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE
AC_CONFIG_SRCDIR([src/axftp.cc])
AC_CONFIG_HEADERS(config.h)

# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL

# Checks for libraries.
#AC_CHECK_LIB([nsl], [netname2user])
AC_CHECK_LIB([ax25], [ax25_aton], [], [AC_MSG_ERROR("libax25-dev missing")])
AC_CHECK_LIB([pthread], [pthread_create])

PKG_CHECK_MODULES([LIBSODIUM], [libsodium])
CXXFLAGS="$CXXFLAGS $LIBSODIUM_CFLAGS"
LDFLAGS="$LDFLAGS $LIBSODIUM_LDFLAGS"
LIBS="$LIBS $LIBSODIUM_LIBS"

PKG_CHECK_MODULES([LIBEDIT], [libedit])
CXXFLAGS="$CXXFLAGS $LIBEDIT_CFLAGS"
LDFLAGS="$LDFLAGS $LIBEDIT_LDFLAGS"
LIBS="$LIBS $LIBEDIT_LIBS"

# Checks for header files.
#AC_FUNC_ALLOCA
#AC_CHECK_HEADERS([fcntl.h stdlib.h string.h strings.h stropts.h sys/ioctl.h sys/socket.h termios.h unistd.h utmp.h pty.h util.h libutil.h alloca.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([dup2])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

echo "
  $PACKAGE_NAME version $PACKAGE_VERSION
  Prefix.........: $prefix
  Debug Build....: $debug
  C++ Compiler...: $CXX $CXXFLAGS $CPPFLAGS
  Linker.........: $LD $LDFLAGS $LIBS
"
