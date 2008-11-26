SUBDIRS = include/linux/netfilter_ipv4

bin_PROGRAMS = rahunasd

IPSET_VERSION:=2.3.3

AM_CFLAGS = \
	$(LIBGNET_CFLAGS) \
	$(LIBGDA_CFLAGS) \
	-I$(top_builddir)/src/include/ \
	-DRAHUNAS_VERSION=\"$(RAHUNAS_VERSION)\" \
	-DPROGRAM=\"$(PROGRAM)\" \
	-DIPSET_VERSION=\"$(IPSET_VERSION)\" \
	-DRAHUNAS_CONF_DIR=\"$(sysconfdir)/rahunas/\" \
  -DRAHUNAS_LOG_DIR=\"$(localstatedir)/log/rahunas/\" \
  -DRAHUNAS_RUN_DIR=\"$(localstatedir)/run/\"

rahunasd_SOURCES = \
	rahunasd.c \
	rahunasd.h \
	rh-xmlrpc-server.c \
	rh-xmlrpc-server.h \
	rh-xmlrpc-cmd.c \
	rh-xmlrpc-cmd.h \
  rh-ipset.c \
	rh-ipset.h \
	rh-utils.c \
	rh-utils.h \
	rh-task.c \
	rh-task.h \
	rh-task-ipset.c \
	rh-task-ipset.h \
	rh-task-memset.c \
	rh-task-memset.h \
	rh-task-dbset.c \
	rh-task-dbset.h \
  rh-task-bandwidth.c \
  rh-task-bandwidth.h \
  rh-radius.h \
  rh-config.c \
	rh-config.h

rahunasd_LDADD =  \
	$(top_builddir)/xmlrpc/libgnetxmlrpc.a \
  $(top_builddir)/lcfg/liblcfg.a \
	$(LIBGNET_LIBS) \
	$(LIBGDA_LIBS)
