EXTRA_DIST = malloc+free-libtester-locations \
	malloc+free-locations \
    base-memory-configuration

AM_CFLAGS = -g -O0

lib_LTLIBRARIES = libtester.la

TESTS = malloc+free \
	malloc+free-libtester \
	multiple-tests \
	realloc \
	posix_memalign+realloc \
	malloc+realloc

bin_PROGRAMS = $(TESTS)

libtester_la_SOURCES = libtester.c libtester.h

malloc_free_libtester_LDADD = libtester.la

testlocationsdir = $(datadir)
testlocations_DATA = malloc+free-libtester-locations  malloc+free-locations

basememoryconfigurationdir = $(datadir)
basememoryconfiguration_DATA = base-memory-configuration

