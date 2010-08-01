#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

m4_define(major, 0)
m4_define(minor, 2)
m4_define(micro, 0)

AC_PREREQ(2.61)
AC_INIT([RahuNAS], major.minor.micro,[neo.neutron@gmail.com] )
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AM_MAINTAINER_MODE
AC_CONFIG_SRCDIR([src/rahunasd.h])
AC_CONFIG_HEADER([config.h])

AC_SUBST(RAHUNAS_VERSION, major.minor.micro)
AC_SUBST(PROGRAM, "RahuNAS")

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL

# Checks for libraries.
dnl Check for GNet 2.x
GNET_REQ=2.0.8
PKG_CHECK_MODULES(LIBGNET, [ gnet-2.0 >= $GNET_REQ ], , [ AC_MSG_ERROR([GNet is required]) ])

AC_SUBST(LIBGNET_CFLAGS)
AC_SUBST(LIBGNET_LIBS)

dnl Check for LibGDA 3.x
LIBGDA_REQ=3.0.2
PKG_CHECK_MODULES(LIBGDA, [ libgda-4.0 >= $LIBGDA_REQ ], , [ AC_MSG_ERROR([LibGDA is required]) ])

AC_SUBST(LIBGDA_CFLAGS)
AC_SUBST(LIBGDA_LIBS)

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([dup2])
AC_CONFIG_FILES([
  Makefile
  xmlrpc/Makefile
  lcfg/Makefile
  src/Makefile
  data/Makefile
  tools/Makefile
  tools/rahunas-firewall
  tools/rahunas-bandwidth
  tools/rahunas-weblogin-config-update
  example/Makefile
  example/rahunas.default
  example/rahunas.init
  example/rahunas.conf
  example/rahunas-vserver.conf
  example/rahunas-serviceclass.conf
])

AC_OUTPUT
