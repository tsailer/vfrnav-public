bin_PROGRAMS = vfrnav
noinst_PROGRAMS = genbarostd genairdataconst genfascrc genmodescrc gengrib2paramtbl genwmostn ssetest

if HAVE_PILOTLINK
palmcc = palm.cc
else
palmcc = 
endif

noinst_HEADERS = \
	RouteEditUi.h fplan.h geom.h geomboost.h geomboostdbl.h \
	interval.hh palm.h dbobj.h dbser.h engine.h baro.h alignedalloc.h \
	maps.h mapst.h mapsnt.h bitmapmaps.h airdata.h wmm.h gtopocolor.h \
	Navigate.h SunriseSunset.h prefs.h icaofpl.h aircraft.h modes.h \
	wind.h nwxweather.h adds.h wxdb.h grib2.h metartaf.hh metgraph.h \
	opsperf.h osm.h

lib_LTLIBRARIES = libvfrnav.la
noinst_LTLIBRARIES = libwmostns.la libsimd.la

libwmostns_la_SOURCES = wmostns.cc
libwmostns_la_CXXFLAGS = $(AM_CXXFLAGS) -O0

if BUILD_SIMD_X64
simdflags = -mavx -mavx2
simdsources = geomavx.cc
else
simdflags =
simdsources = geomnosimd.cc
endif

libsimd_la_SOURCES = $(simdsources)
libsimd_la_CXXFLAGS = $(AM_CXXFLAGS) $(simdflags)
EXTRA_libsimd_la_SOURCES = geomavx.cc geomnosimd.cc

libvfrnav_la_SOURCES = fplan.cc geom.cc geomgeos.cc interval.cc dbobj.cc dbobjarpt.cc \
	dbobjaspc.cc dbobjnav.cc dbobjwpt.cc dbobjmapel.cc dbobjwatel.cc dbobjawy.cc \
	dbobjtrk.cc dbobjlbl.cc dbobjtopo.cc dbser.cc fascrc.cc engine.cc \
	awygraph.cc engine1.cc engine2.cc aircraft.cc navlog.cc modes.cc modescrc.cc \
	bitmapmaps.cc mapst.cc mapsnt.cc baro.cc barostd.cc airdata.cc airdataconst.cc \
	wmm.cc wmm2000.cc wmm2005.cc wmm2010.cc wmm2015.cc SunriseSunset.cc \
	prefs.cc icaofpl.cc wind.cc nwxweather.cc adds.cc wxdb.cc \
	grib2.cc grib2tables.cc grib2paramtbl.cc metartaf.cc metgraph.cc metgraphltx.cc \
	opsperf.cc osm.cc $(palmcc)
EXTRA_libvfrnav_la_SOURCES = palm.cc
libvfrnav_la_LIBADD = libwmostns.la libsimd.la @LIBS@ @GTKMM_LIBS@ @GLIBMM_LIBS@ \
	@GIOMM_LIBS@ @LOCATION_LIBS@ @SQLITE3X_LIBS@ @GTHREAD_LIBS@ @DBUS_LIBS@ \
	@GNOMEBT_LIBS@ @PILOTLINK_LIBS@ @BOOST_LIBS@ @EIGEN3_LIBS@ @CURL_LIBS@ \
	@LIBXMLPP_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@ @LIBSOUP_LIBS@ \
	@LIBRSVG_LIBS@ @LIBCRYPTO_LIBS@ @PQXX_LIBS@ @JSONCPP_LIBS@ -lgcc_s -lgcc
libvfrnav_la_LDFLAGS = -no-undefined -version-info 0:0:0 \
	@BOOST_LDFLAGS@ @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrnav_SOURCES = vfrnav.cc RouteEditUi.cc Navigate.cc
vfrnav_LDADD = libvfrnav.la @LIBS@ @GTKMM_LIBS@ @GLIBMM_LIBS@ @GIOMM_LIBS@ \
	@GPS_LIBS@ @LOCATION_LIBS@ @GYPSY_LIBS@ @SQLITE3X_LIBS@ @GTHREAD_LIBS@ \
	@DBUS_LIBS@ @HILDONMM_LIBS@ @GNOMEBT_LIBS@ @PILOTLINK_LIBS@ @BOOST_LIBS@ \
	@EIGEN3_LIBS@ @CURL_LIBS@ @LIBXMLPP_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ \
	@OPENJPEG_LIBS@ @LIBSOUP_LIBS@ @JSONCPP_LIBS@
vfrnav_LDFLAGS = @BOOST_LDFLAGS@ @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@
EXTRA_vfrnav_SOURCES = vfrnavrc.rc

if WIN32
vfrnav_SOURCES += vfrnavrc.rc
endif

genbarostd_SOURCES = genbarostd.cc baro.cc
genbarostd_CXXFLAGS = $(AM_CXXFLAGS)

genairdataconst_SOURCES = genairdataconst.cc baro.cc barostd.cc airdata.cc
genairdataconst_CXXFLAGS = $(AM_CXXFLAGS)

genfascrc_SOURCES = genfascrc.cc

genmodescrc_SOURCES = genmodescrc.cc

gengrib2paramtbl_SOURCES = gengrib2paramtbl.cc

genwmostn_SOURCES = genwmostn.cc
genwmostn_LDADD = libvfrnav.la @CURL_LIBS@ @JSONCPP_LIBS@

ssetest_SOURCES = ssetest.cc
ssetest_LDADD = libvfrnav.la @LIBS@ @GTKMM_LIBS@ @GLIBMM_LIBS@ @GIOMM_LIBS@ \
	@GPS_LIBS@ @LOCATION_LIBS@ @GYPSY_LIBS@ @SQLITE3X_LIBS@ @GTHREAD_LIBS@ \
	@DBUS_LIBS@ @HILDONMM_LIBS@ @GNOMEBT_LIBS@ @PILOTLINK_LIBS@ @BOOST_LIBS@ \
	@EIGEN3_LIBS@ @CURL_LIBS@ @LIBXMLPP_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ \
	@OPENJPEG_LIBS@ @LIBSOUP_LIBS@ @JSONCPP_LIBS@
ssetest_LDFLAGS = @BOOST_LDFLAGS@ @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@
ssetest_CXXFLAGS = $(AM_CXXFLAGS) $(simdflags)

if CROSSCOMP
else
$(srcdir)/barostd.cc:		genbarostd$(EXEEXT)
				./genbarostd$(EXEEXT) > $@

$(srcdir)/airdataconst.cc:	genairdataconst$(EXEEXT)
				./genairdataconst$(EXEEXT) > $@

$(srcdir)/fascrc.cc:		genfascrc$(EXEEXT)
				./genfascrc$(EXEEXT) > $@

$(srcdir)/modescrc.cc:		genmodescrc$(EXEEXT)
				./genmodescrc$(EXEEXT) > $@

$(srcdir)/grib2paramtbl.cc:	gengrib2paramtbl$(EXEEXT)
				./gengrib2paramtbl$(EXEEXT) > $@
endif

AM_CFLAGS = @MY_CFLAGS@

AM_CXXFLAGS = @MY_CXXFLAGS@ @GTKMM_CFLAGS@ @GLIBMM_CFLAGS@ @GIOMM_CFLAGS@ \
	@LIBXMLPP_CFLAGS@ @SQLITE_CFLAGS@ @SQLITE3X_CFLAGS@ @ZFSTREAM_CFLAGS@ @LOCATION_CFLAGS@ \
	@GYPSY_CFLAGS@ @GTHREAD_CFLAGS@ @DBUS_CFLAGS@ @HILDONMM_CFLAGS@ @GNOMEBT_CFLAGS@ @PILOTLINK_CFLAGS@ \
	@BOOST_CPPFLAGS@ @EIGEN3_CFLAGS@ @CURL_CFLAGS@ @LIBXMLPP_CFLAGS@ @GEOS_CPPFLAGS@ @CLIPPER_CPPFLAGS@ \
	@OPENJPEG_CFLAGS@ @LIBSOUP_CFLAGS@ @LIBRSVG_CFLAGS@ @LIBCRYPTO_CFLAGS@ @PQXX_CFLAGS@ \
	@JSONCPP_CFLAGS@ 

AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/include

.rc.o:
	$(RC) -I$(top_srcdir) -I$(top_srcdir)/ui -o $@ $<
