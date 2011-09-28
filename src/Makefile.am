MAINTAINERCLEANFILES = Makefile.in

sbin_PROGRAMS = rahunasd

IPSET_VERSION:=4.2

AM_CFLAGS = \
  $(LIBGNET_CFLAGS) \
  $(LIBSQLITE3_CFLAGS) \
  -I$(top_srcdir)/src/ \
  -DRAHUNAS_VERSION=\"$(RAHUNAS_VERSION)\" \
  -DPROGRAM=\"$(PROGRAM)\" \
  -DIPSET_VERSION=\"$(IPSET_VERSION)\" \
  -DRAHUNAS_CONF_DIR=\"$(sysconfdir)/rahunas/\" \
  -DRAHUNAS_DB=\"$(localstatedir)/lib/rahunas/rahunas.db\" \
  -DRAHUNAS_LOG_DIR=\"$(localstatedir)/log/rahunas/\" \
  -DRAHUNAS_RUN_DIR=\"$(localstatedir)/run/\" \
  -DRAHUNAS_FIREWALL_WRAPPER=\"$(sbindir)/rahunas-firewall\" \
  -DRAHUNAS_BANDWIDTH_WRAPPER=\"$(sbindir)/rahunas-bandwidth\" \
  -D_GNU_SOURCE

rahunasd_SOURCES = \
  rahunasd.c \
  rahunasd.h \
  rh-server.c \
  rh-server.h \
  rh-xmlrpc-server.c \
  rh-xmlrpc-server.h \
  rh-xmlrpc-cmd.c \
  rh-xmlrpc-cmd.h \
  ipset/ip_set_bitmaps.h \
  ipset/ip_set_hashes.h \
  ipset/ip_set_rahunas_ipiphash.h \
  ipset/ip_set.h \
  ipset/ip_set_rahunas.h \
  rh-ipset.c \
  rh-ipset.h \
  rh-utils.c \
  rh-utils.h \
  rh-task.c \
  rh-task.h \
  rh-task-iptables.c \
  rh-task-iptables.h \
  rh-task-memset.c \
  rh-task-memset.h \
  rh-task-ipset.c \
  rh-task-ipset.h \
  rh-task-dbset.c \
  rh-task-dbset.h \
  rh-task-bandwidth.c \
  rh-task-bandwidth.h \
  rh-task-serviceclass.c \
  rh-task-serviceclass.h \
  rh-radius.h \
  rh-config.c \
  rh-config.h \
  rh-serviceclass.h \
  rh-serviceclass.c

rahunasd_LDADD =  \
  $(top_builddir)/xmlrpc/libgnetxmlrpc.a \
  $(top_builddir)/lcfg/liblcfg.a \
  $(LIBGNET_LIBS) \
  $(LIBSQLITE3_LIBS)
