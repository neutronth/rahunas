## Process this file with automake to produce Makefile.in

@SET_MAKE@

MAINTAINERCLEANFILES = Makefile.in

noinst_LIBRARIES = libgnetxmlrpc.a

noinst_PROGRAMS = \
	test-xmlrpc-client \
	test-xmlrpc-client-php \
	test-xmlrpc-server

AM_CFLAGS = $(LIBGNET_CFLAGS)

libgnetxmlrpc_a_SOURCES = xmlrpc-client.c xmlrpc-server.c xmlrpc.h

test_xmlrpc_client_SOURCES = test-xmlrpc-client.c xmlrpc.h
test_xmlrpc_client_LDADD = \
	libgnetxmlrpc.a \
	$(LIBGNET_LIBS)

test_xmlrpc_client_php_SOURCES = test-xmlrpc-client-php.c xmlrpc.h
test_xmlrpc_client_php_LDADD = \
	libgnetxmlrpc.a \
	$(LIBGNET_LIBS)

test_xmlrpc_server_SOURCES = test-xmlrpc-server.c xmlrpc.h
test_xmlrpc_server_LDADD = \
	libgnetxmlrpc.a \
	$(LIBGNET_LIBS)
