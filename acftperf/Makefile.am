noinst_HEADERS = acftperf.h aircraftdb.h archerII.h arrow.h grob115b.h t67m.h

acftperf_SOURCES = acftperf.cc
acftperf_LDADD = @LIBS@ @GTKMM_LIBS@ @GTHREAD_LIBS@ @HILDONMM_LIBS@ ../src/libvfrnav.la

bin_PROGRAMS = acftperf

AM_CFLAGS = @MY_CFLAGS@
AM_CXXFLAGS = @MY_CXXFLAGS@ @GTKMM_CFLAGS@ @LIBXMLPP_CFLAGS@ @SQLITE_CFLAGS@ @SQLITE3X_CFLAGS@ @ZFSTREAM_CFLAGS@ @HILDONMM_CFLAGS@
AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/include
