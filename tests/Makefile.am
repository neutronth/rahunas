EXTRA_DIST = Make_gtest

AM_CXXFLAGS = -std=c++11

include Make_gtest

TESTS = rhtest

noinst_PROGRAMS = rhtest

rhtest_SOURCES = \
	main.cxx \
	ClientTest.cxx

rhtest_LDADD  = gtest.a $(top_builddir)/src/librahunas.a
