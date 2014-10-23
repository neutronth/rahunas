EXTRA_DIST = Make_gtest

AM_CXXFLAGS = -std=c++14

include Make_gtest

TESTS = rhtest

noinst_PROGRAMS = rhtest

rhtest_SOURCES = \
	main.cxx \
	ClientTest.cxx \
	SessionTest.cxx

rhtest_LDADD  = gtest.a $(top_builddir)/src/librahunas.a
