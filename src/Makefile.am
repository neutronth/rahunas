bin_PROGRAMS = rahunasd

IPSET_VERSION:=2.3.3

AM_CFLAGS = $(LIBGNET_CFLAGS) -I$(top_builddir)/src/include/ \
	-DIPSET_VERSION=\"$(IPSET_VERSION)\"

rahunasd_SOURCES = rahunasd.c \
	rh-xmlrpc-server.c \
  rh-ipset.c \
	rh-utils.c

rahunasd_LDADD =  \
	$(top_builddir)/xmlrpc/libgnetxmlrpc.a \
	$(LIBGNET_LIBS)
