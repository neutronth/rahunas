MAINTAINERCLEANFILES = Makefile.in

if ENABLE_NETSNMP_MODULE

EXTRA_DIST = mibs/RAHUNAS-MIB

# Library versioning
# # Library code modified:                    REVISION++
# # Interfaces added:              CURRENT++  REVISION=0  AGE++
# # Interfaces changed/removed:    CURRENT++  REVISION=0  AGE=0
LIBSNMP_RH_CURRENT = 0
LIBSNMP_RH_REVISION = 0
LIBSNMP_RH_AGE = 0

LIBNETSNMP_CFLAGS=`net-snmp-config --cflags`
LIBNETSNMP_LIBS=`net-snmp-config --libs`

lib_LTLIBRARIES = libnetsnmp_rahunas.la

libnetsnmp_rahunas_la_SOURCES = \
	rhsnmp.h \
	rhsnmp.c

libnetsnmp_rahunas_la_CFLAGS = \
  -I$(top_srcdir)/snmp-module \
  -DRAHUNAS_DB=\"$(localstatedir)/lib/rahunas/rahunas.db\" \
	$(LIBNETSNMP_CFLAGS)

libnetsnmp_rahunas_la_LDFLAGS = \
  -version-info $(LIBSNMP_RH_CURRENT):$(LIBSNMP_RH_REVISION):$(LIBSNMP_RH_AGE) \
  $(LIBSQLITE3_LIBS) \
  -lpthread \
	$(LIBNETSNMP_LIBS)

endif
