# Kernel module
#

AM_CFLAGS = ${libxtables_CFLAGS}

_kcall = -C ${KBUILDDIR} M=${abs_srcdir}

modules:
	@echo -n "SETRAWNAT Experimental - Linux "
	@if [ -n "${KBUILDDIR}" ]; then ${MAKE} ${_kcall} --no-print-directory -s kernelrelease; fi;
	${AM_V_silent}if [ -n "${KBUILDDIR}" ]; then EXTRA_CFLAGS="$(CFLAGS)" ${MAKE} ${_kcall} modules; fi;

modules_install:
	${AM_V_silent}if [ -n "${KBUILDDIR}" ]; then ${MAKE} ${_kcall} INSTALL_MOD_PATH=${DESTDIR} ext-mod-dir='$${INSTALL_MOD_DIR}' modules_install; fi;

clean_modules:
	${AM_V_silent}if [ -n "${KBUILDDIR}" ]; then ${MAKE} ${_kcall} clean; fi;

all-local: modules

install-exec-local: modules_install

clean-local: clean_modules

include ../Makefile.extra
