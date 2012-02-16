#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([setrawnat], [20120213], [neutron@rahunas.org])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

dnl Optionnally disable building the kernel module
AC_ARG_WITH([kmod],
            AS_HELP_STRING([--with-kmod=yes/no],
                           [Build the kernel module (default: yes)]),
            [BUILDKMOD="$withval";],
            [BUILDKMOD="yes";])
AM_CONDITIONAL(WITH_KMOD, test "$BUILDKMOD" == "yes")

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

dnl Sigh: check kernel version dependencies
if test "$BUILDKMOD" == "yes"
then
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

if test "X`$GREP 'NFNL_SUBSYS_IPSET' $ksourcedir/include/linux/netfilter/nfnetlink.h`" = "X"
then
  AC_MSG_ERROR([The kernel source directory $ksourcedir is not patched with netlink.patch to support ipset])
fi
fi

PKG_CHECK_MODULES([libxtables], [xtables >= 1.4.5])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([netinet/in.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T

# Checks for library functions.

AC_CONFIG_FILES([Makefile
	Makefile.iptrules
	extension/Makefile])
AC_OUTPUT
