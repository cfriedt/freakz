AUTOMAKE_OPTIONS = subdir-objects

SUBDIRS = \
  freakz \
  os

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/include/freakz @CONTIKI_CFLAGS@

# XXX: if APPLE
AM_LDFLAGS = -Wl,-undefined,error -Wl,-all_load

lib_LTLIBRARIES = libfreakz.la
bin_PROGRAMS = test_udp

libfreakz_la_SOURCES =

test_udp_SOURCES = \
  test/native/test_udp.c

test_udp_LDADD = \
  libfreakz.la

libfreakz_la_LIBADD = \
  freakz/libfreakzfreakz.a \
  os/libfreakzos.a

dist_doc_DATA = AUTHORS ChangeLog COPYING NEWS README
