MAINTAINERCLEANFILES = Makefile.in

dist_sbin_SCRIPTS = \
	rahunas-firewall \
	rahunas-bandwidth \
	rahunas-weblogin-config-update

bin_PROGRAMS = \
	squid-auth-helper

squid_auth_helper_SOURCES = \
	squid-auth-helper.c \
	$(top_srcdir)/xmlrpc/xmlrpc.h
squid_auth_helper_CPPFLAGS = \
	-I$(top_srcdir)/lcfg \
	-I$(top_srcdir)/xmlrpc \
	$(LIBGNET_CFLAGS) \
        -D_GNU_SOURCE
squid_auth_helper_LDADD = \
	$(top_builddir)/lcfg/liblcfg.a \
	$(top_builddir)/xmlrpc/libgnetxmlrpc.a	\
	$(LIBGNET_LIBS)

