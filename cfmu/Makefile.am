SUBDIRS = ui html php systemd

if HAVE_WEBKITGTKWEBEXT
SUBDIRS += webext
endif

EXTRA_SUBDIRS = webext

bin_PROGRAMS = cfmuvalidate cfmuautoroute checkfplan weatherfplan vfrradconvert cfmusidstar adrimport adraupimport adrquery adrdbsync
sbin_PROGRAMS =
noinst_PROGRAMS =
noinst_LIBRARIES = libcfmuautoroute.a libcheckfpl.a libtfr.a
pkglibexec_PROGRAMS = cfmuvalidateserver

cfmuvalidate_SOURCES = cfmuvalidate.cc cfmuvalidate.hh
cfmuvalidate_LDADD = libtfr.a ../src/libvfrnav.la webext/libcfmuvalidationwidget.la \
	@LIBS@ @GTKMM_LIBS@ @GLIBMM_LIBS@ @GIOMM_LIBS@ \
	@PILOTLINK_LIBS@ @BOOST_LIBS@ @SQLITE3X_LIBS@ @LIBXMLPP_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
cfmuvalidate_LDFLAGS = @BOOST_LDFLAGS@ @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

cfmuvalidateserver_SOURCES = cfmuvalidateserver.cc cfmuvalidateserver.hh
cfmuvalidateserver_LDADD = libtfr.a ../src/libvfrnav.la webext/libcfmuvalidationwidget.la \
	@LIBS@ @GTKMM_LIBS@ @GLIBMM_LIBS@ @GIOMM_LIBS@ \
	@PILOTLINK_LIBS@ @BOOST_LIBS@ @SQLITE3X_LIBS@ @LIBXMLPP_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ \
	@OPENJPEG_LIBS@ @X11_LIBS@ @CURL_LIBS@ @SYSTEMDDAEMON_LIBS@
cfmuvalidateserver_LDFLAGS = @BOOST_LDFLAGS@ @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

if WIN32
cfmuvalidateserver_LDFLAGS += -lws2_32
endif

if HAVE_WEBKITGTK4

cfmuvalidate_CPPFLAGS = \
	$(AM_CPPFLAGS) @WEBKITGTK4_CFLAGS@

cfmuvalidateserver_CPPFLAGS = \
	$(AM_CPPFLAGS) @WEBKITGTK4_CFLAGS@

else
if HAVE_WEBKITGTK

cfmuvalidate_CPPFLAGS = \
	$(AM_CPPFLAGS) @WEBKITGTK_CFLAGS@

cfmuvalidateserver_CPPFLAGS = \
	$(AM_CPPFLAGS) @WEBKITGTK_CFLAGS@

endif
endif

libcfmuautoroute_a_SOURCES = cfmuautorouteproxy.cc cfmuautorouteproxy.hh

cfmuautoroute_SOURCES = autoroute.cc cfmuautoroute.hh cfmuautoroute.cc cfmugraph.cc \
	cfmudctcache.cc cfmuintel.cc cfmuspecial.cc cfmuserialize.cc cfmugraph51.cc \
	cfmuautoroute45.hh cfmuautoroute45.cc cfmuautoroute51.hh cfmuautoroute51.cc
if HAVE_JSONCPP
if HAVE_FORK
cfmuautoroute_SOURCES += arsockserver.hh arsockserver.cc
endif
endif
EXTRA_cfmuautoroute_SOURCES = arsockserver.hh arsockserver.cc
cfmuautoroute_LDADD = libtfr.a libcfmuautoroute.a ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @GLIBMM_LIBS@ @GIOMM_LIBS@ \
	@PILOTLINK_LIBS@ @BOOST_LIBS@ @SQLITE3X_LIBS@ @LIBXMLPP_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@ \
	@SYSTEMDDAEMON_LIBS@ @JSONCPP_LIBS@
cfmuautoroute_LDFLAGS = @BOOST_LDFLAGS@ @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

libtfr_a_SOURCES = tfr.hh tfr.cc tfrdbloader.cc tfrbinloader.cc tfrloader.cc tfrrules.cc tfrrules2.cc tfrspecial.cc \
	adr.hh adr.cc adrdb.hh adrdb.cc adrhibernate.hh adrhibernate.cc adrgraph.hh adrgraph.cc adraerodrome.hh adraerodrome.cc \
	adrunit.hh adrunit.cc adrpoint.hh adrpoint.cc adrroute.hh adrroute.cc adrairspace.hh adrairspace.cc adrrestriction.hh \
	adrrestriction1.cc adrrestriction2.cc adrrestriction.cc adrparse.hh adrparse.cc adrbdry.hh adrbdry.cc \
	adrfplan.hh adrfplan.cc adrtimetable.hh adrtimetable.cc adraup.hh adraup.cc

libcheckfpl_a_SOURCES = tfrproxy.cc tfrproxy.hh

checkfplan_SOURCES = checkfpl.cc
checkfplan_LDADD = libtfr.a libcheckfpl.a ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @GLIBMM_LIBS@ @GIOMM_LIBS@ \
	@PILOTLINK_LIBS@ @BOOST_LIBS@ @SQLITE3X_LIBS@ @LIBXMLPP_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
checkfplan_LDFLAGS = @BOOST_LDFLAGS@ @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

weatherfplan_SOURCES = wxfpl.cc
weatherfplan_LDADD = libtfr.a libcheckfpl.a ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @GLIBMM_LIBS@ @GIOMM_LIBS@ \
	@PILOTLINK_LIBS@ @BOOST_LIBS@ @SQLITE3X_LIBS@ @LIBXMLPP_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
weatherfplan_LDFLAGS = @BOOST_LDFLAGS@ @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrradconvert_SOURCES = vfrradconvert.cc
vfrradconvert_LDADD = libtfr.a ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @GLIBMM_LIBS@ @GIOMM_LIBS@ \
	@PILOTLINK_LIBS@ @BOOST_LIBS@ @SQLITE3X_LIBS@ @LIBXMLPP_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
vfrradconvert_LDFLAGS = @BOOST_LDFLAGS@ @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

cfmusidstar_SOURCES = cfmusidstar.cc
cfmusidstar_LDADD = ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @GLIBMM_LIBS@ @GIOMM_LIBS@ \
	@PILOTLINK_LIBS@ @BOOST_LIBS@ @SQLITE3X_LIBS@ @LIBXMLPP_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
cfmusidstar_LDFLAGS = @BOOST_LDFLAGS@ @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

adrimport_SOURCES = adrimport.cc
adrimport_LDADD = libtfr.a ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @GLIBMM_LIBS@ @GIOMM_LIBS@ \
	@PILOTLINK_LIBS@ @BOOST_LIBS@ @SQLITE3X_LIBS@ @LIBXMLPP_LIBS@ @GDAL_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
adrimport_LDFLAGS = @BOOST_LDFLAGS@ @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

adraupimport_SOURCES = adraupparse.cc adraupparse.hh
adraupimport_LDADD = libtfr.a ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @GLIBMM_LIBS@ @GIOMM_LIBS@ \
	@PILOTLINK_LIBS@ @BOOST_LIBS@ @SQLITE3X_LIBS@ @LIBXMLPP_LIBS@ @GDAL_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
adraupimport_LDFLAGS = @BOOST_LDFLAGS@ @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

adrquery_SOURCES = adrquery.cc
adrquery_LDADD = libtfr.a ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @GLIBMM_LIBS@ @GIOMM_LIBS@ \
	@PILOTLINK_LIBS@ @BOOST_LIBS@ @SQLITE3X_LIBS@ @LIBXMLPP_LIBS@ @GDAL_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
adrquery_LDFLAGS = @BOOST_LDFLAGS@ @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

adrdbsync_SOURCES = adrdbsync.cc
adrdbsync_LDADD = libtfr.a ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @GLIBMM_LIBS@ @GIOMM_LIBS@ \
	@PILOTLINK_LIBS@ @BOOST_LIBS@ @SQLITE3X_LIBS@ @LIBXMLPP_LIBS@ @GDAL_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
adrdbsync_LDFLAGS = @BOOST_LDFLAGS@ @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

if HAVE_WEBSOCKET
if HAVE_JSONCPP
if HAVE_FORK

sbin_PROGRAMS += cfmuws

cfmuws_SOURCES = cfmuws.cc
cfmuws_LDADD = -lboost_system -lpthread @WEBSOCKET_LIBS@ @JSONCPP_LIBS@ @BOOST_LIBS@
cfmuws_LDFLAGS = @BOOST_LDFLAGS@

endif
endif
endif

EXTRA_DIST = cfmuws.cc

AM_CFLAGS = @MY_CFLAGS@
AM_CXXFLAGS = @MY_CXXFLAGS@
AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/include -I$(top_srcdir)/ead -I$(srcdir)/webext \
        @GTKMM_CFLAGS@ @GLIBMM_CFLAGS@ @GIOMM_CFLAGS@ @SQLITE_CFLAGS@ \
        @SQLITE3X_CFLAGS@ @ZFSTREAM_CFLAGS@ @PILOTLINK_CFLAGS@ @BOOST_CPPFLAGS@ \
        @LIBXMLPP_CFLAGS@ @GDAL_CFLAGS@ @GEOS_CPPFLAGS@ @CLIPPER_CPPFLAGS@ \
        @WEBSOCKET_CFLAGS@ @SYSTEMDDAEMON_CFLAGS@ @JSONCPP_CFLAGS@ @CURL_CFLAGS@ \
        @EIGEN3_CFLAGS@ @ANTLRCFLAGS@ @MAGICKPP_CFLAGS@ @LIBCRYPTO_CFLAGS@ @RTKLIB_CFLAGS@ \
        @QRENCODE_CFLAGS@ @LIBARCHIVE_CFLAGS@
