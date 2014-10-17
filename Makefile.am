if ! WITH_KBUILDDIR
KBUILD_OUTPUT=/lib/modules/`uname -r`/build
else
KBUILD_OUTPUT=$(KBUILDDIR)
endif
if ! WITH_MAXSETS
IP_SET_MAX=256
else
IP_SET_MAX=$(MAXSETS)
endif

KMOD_SRCDIR=`readlink -f $(srcdir)/kernel/net/netfilter/ipset`
KMOD_SRCKDIR=`readlink -f $(srcdir)/kernel`

modules:
if WITH_KMOD
	${MAKE} -C $(KBUILD_OUTPUT) M=$(KMOD_SRCDIR) V=$V \
			KCFLAGS="-DPACKAGE_VERSION=$(PACKAGE_VERSION)" \
			IP_SET_MAX=$(IP_SET_MAX) KDIR=$(KMOD_SRCKDIR) modules
else
	@echo Skipping kernel modules
endif

modules_install:
if WITH_KMOD
	${MAKE} -C $(KBUILD_OUTPUT) M=$(KMOD_SRCDIR) KDIR=$(KMOD_SRCKDIR) \
		modules_install
else
	@echo Skipping kernel modules
endif

modules_clean:
if WITH_KMOD
	${MAKE} -C $(KBUILD_OUTPUT) M=$(KMOD_SRCDIR) KDIR=$(KMOD_SRCKDIR) clean
else
	@echo Skipping kernel modules
endif

EXTRA_DIST=kernel/net/netfilter/ipset
