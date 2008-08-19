bin_PROGRAMS = rahunasd

AM_CFLAGS = $(LIBGNET_CFLAGS)

rahunasd_SOURCES = rahunasd.c rh-xmlrpc-server.c ipset-control.c
rahunasd_LDADD =  \
	$(top_builddir)/xmlrpc/libgnetxmlrpc.a \
	$(LIBGNET_LIBS)
