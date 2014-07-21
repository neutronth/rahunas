#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

m4_define(rh_major, 0)
m4_define(rh_minor, 2)
m4_define(rh_micro, 6)

AC_PREREQ(2.61)
AC_INIT([RahuNAS], rh_major.rh_minor.rh_micro,[neo.neutron@gmail.com] )
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AM_MAINTAINER_MODE
AC_CONFIG_SRCDIR([src/rahunasd.h])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_SUBST(RAHUNAS_VERSION, rh_major.rh_minor.rh_micro)
AC_SUBST(PROGRAM, "RahuNAS")

# gettext
AM_GNU_GETTEXT_VERSION([0.18.1])
AM_GNU_GETTEXT([external])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

# Libtool init
LT_INIT

# Checks for libraries.
dnl Check for GNet 2.x
GNET_REQ=2.0.8
PKG_CHECK_MODULES(LIBGNET, [ gnet-2.0 >= $GNET_REQ ], , [ AC_MSG_ERROR([GNet is required]) ])

AC_SUBST(LIBGNET_CFLAGS)
AC_SUBST(LIBGNET_LIBS)

dnl Check for Sqlite3
LIBSQLITE3_REQ=3.7.3
PKG_CHECK_MODULES(LIBSQLITE3, [ sqlite3 >= $LIBSQLITE3_REQ ], , [ AC_MSG_ERROR([libsqlite3 is required]) ])

AC_SUBST(LIBSQLITE3_CFLAGS)
AC_SUBST(LIBSQLITE3_LIBS)

dnl Disable snmp-module build
AC_ARG_ENABLE(netsnmp-module,
              [AC_HELP_STRING([--enable-netsnmp-module],
                              [enable netsnmp-module build])],
              enable_netsnmp_module="yes")

if test "x$enable_netsnmp_module" = "xyes"; then
  AC_CHECK_PROG(NETSNMP_CONFIG, net-snmp-config, net-snmp-config, no)
  if test "x$NETSNMP_CONFIG" = "xno"; then
    enable_netsnmp_module="no"
  fi
fi

AM_CONDITIONAL(ENABLE_NETSNMP_MODULE,test "x$enable_netsnmp_module" = "xyes")

dnl Disable Squid auth helper build
AC_ARG_ENABLE(squid-auth-helper,
              [AC_HELP_STRING([--enable-squid-auth-helper],
                              [enable squid-auth-helper build])],
              enable_squid_auth_helper="yes")

AM_CONDITIONAL(ENABLE_SQUID_AUTH_HELPER,test "x$enable_squid_auth_helper" = "xyes")

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
  tools/rahunas-conntrack
  tools/rahunas-ipacct
  tools/rahunas-weblogin-config-update
  netsnmp-module/Makefile
  netsnmp-module/scripts/rh-checkrad
  example/Makefile
  example/rahunas.default
  example/rahunas.init
  example/rahunas.conf
  example/rahunas-vserver.conf
  example/rahunas-serviceclass.conf
  weblogin-po/Makefile.in
])

AC_OUTPUT
