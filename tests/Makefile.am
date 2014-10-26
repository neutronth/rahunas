EXTRA_DIST = Make_gtest

AM_CXXFLAGS = -std=c++14

include Make_gtest

TESTS = rhtest

noinst_PROGRAMS = rhtest

rhtest_SOURCES = \
	main.cxx \
	ClientIPCommonTest.cxx \
	ClientIPv4Test.cxx \
	ClientIPv6Test.cxx \
	SessionTest.cxx \
	SessionsPoolTest.cxx

rhtest_LDADD  = gtest.a $(top_builddir)/src/librahunas.a
