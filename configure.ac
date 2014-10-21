#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

m4_define(rh_major, 1)
m4_define(rh_minor, 9)
m4_define(rh_micro, 0+)

AC_PREREQ([2.69])
AC_INIT([rahunas], rh_major.rh_minor.rh_micro, [neutron@rahunas.org])
AC_CANONICAL_HOST
AM_INIT_AUTOMAKE([foreign subdir-objects tar-pax])

LT_INIT
AC_CONFIG_MACRO_DIR([m4])
AM_PROG_LIBTOOL

dnl Shortcut: Linux supported alone
case "$host" in
*-*-linux* | *-*-uclinux*) ;;
*) AC_MSG_ERROR([Linux systems supported exclusively!]);;
esac

dnl Optionnally enable building the kernel module
AC_ARG_WITH([kmod],
            AS_HELP_STRING([--with-kmod=yes/no],
                           [Build the kernel module (default: no)]),
            [BUILDKMOD="$withval";],
            [BUILDKMOD="no";])
AM_CONDITIONAL(WITH_KMOD, test "$BUILDKMOD" == "yes")

dnl Additional arguments
dnl Kernel build directory or source tree
AC_ARG_WITH([kbuild],
            AS_HELP_STRING([--with-kbuild=PATH],
                           [Path to kernel build directory]),
            [KBUILDDIR="$withval";])
AC_ARG_WITH([ksource],
            AS_HELP_STRING([--with-ksource=PATH],
                           [Path to kernel source directory, if not the same as the kernel build directory]),
            [KSOURCEDIR="$withval";])
AM_CONDITIONAL(WITH_KBUILDDIR, test "$KBUILDDIR" != "")
AC_SUBST(KBUILDDIR)

if test "$BUILDKMOD" == "yes"
then
dnl Sigh: check kernel version dependencies
if test "$KBUILDDIR" != ""
then
  kbuilddir="$KBUILDDIR"
else
  kbuilddir="/lib/modules/`uname -r`/build"
fi

if test -n "$KSOURCEDIR"; then
  ksourcedir="$KSOURCEDIR"
elif test -e "$kbuilddir/include/linux/netfilter/nfnetlink.h"; then
  ksourcedir="$kbuilddir"
else
  ksourcedir="/lib/modules/$(uname -r)/source"
fi
if test ! -e "$ksourcedir/include/linux/netfilter/nfnetlink.h"
then
  AC_MSG_ERROR([Invalid kernel source directory $ksourcedir])
fi

if test ! -e "$kbuilddir/.config"
then
  AC_MSG_ERROR([The kernel build directory $kbuilddir is not configured])
fi

AC_PROG_GREP
AC_PROG_AWK

if ! $GREP -q "NFNL_SUBSYS_IPSET" "$ksourcedir/include/linux/netfilter/nfnetlink.h" && \
   ! $GREP -q "NFNL_SUBSYS_IPSET" "$ksourcedir/include/uapi/linux/netfilter/nfnetlink.h";
then
  AC_MSG_ERROR([The kernel source directory $ksourcedir is not patched with netlink.patch to support ipset])
fi
fi

dnl Maximal number of sets supported by the kernel, default 256
AC_ARG_WITH([maxsets],
      AS_HELP_STRING([--with-maxsets=256],
             [Maximal numer of sets supported by the kernel]),
      [MAXSETS="$withval";])
AM_CONDITIONAL(WITH_MAXSETS, test "$MAXSETS" != "")
AC_SUBST(MAXSETS)

dnl Checks for libraries
PKG_CHECK_MODULES([libipset], [libipset >= 6.23])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([memset])

AC_CONFIG_FILES([
	Makefile
	ipset/lib/Makefile
	tests/Makefile
])
AC_OUTPUT
