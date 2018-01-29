SUBDIRS = data

vfrnavfplan_SOURCES = fplan.cc
vfrnavfplan_LDADD = ../src/libvfrnav.la @LIBS@ @LIBXMLPP_LIBS@ @SQLITE_LIBS@ @SQLITE3X_LIBS@ @GLIBMM_LIBS@ @GIOMM_LIBS@ @GTKMM_LIBS@ @PILOTLINK_LIBS@ @BOOST_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
vfrnavfplan_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrnavdb2xml_SOURCES = vfrnavdb2xml.cc
vfrnavdb2xml_LDADD = ../src/libvfrnav.la @LIBS@ @LIBXMLPP_LIBS@ @SQLITE_LIBS@ @GTKMM_LIBS@ @SQLITE3X_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
vfrnavdb2xml_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrnavxml2db_SOURCES = vfrnavxml2db.cc
vfrnavxml2db_LDADD = ../src/libvfrnav.la @LIBS@ @LIBXMLPP_LIBS@ @SQLITE_LIBS@ @GTKMM_LIBS@ @SQLITE3X_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
vfrnavxml2db_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrnavwmmconv_SOURCES = wmmconv.cc
vfrnavwmmconv_LDADD = @LIBS@

vfrnavwmmtest_SOURCES = wmmtest.cc
vfrnavwmmtest_LDADD = $(top_builddir)/src/libvfrnav.la @LIBS@

aircraftopsperf_SOURCES = opsperf.cc
aircraftopsperf_LDADD = $(top_builddir)/src/libvfrnav.la @LIBS@

vfrdbdafif_SOURCES = dafif.cc
vfrdbdafif_LDADD = ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @LIBXMLPP_LIBS@ @SQLITE_LIBS@ @SQLITE3X_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
vfrdbdafif_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrdbupdategndelev_SOURCES = updategndelev.cc
vfrdbupdategndelev_LDADD = ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @LIBXMLPP_LIBS@ @SQLITE_LIBS@ @SQLITE3X_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
vfrdbupdategndelev_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrdboptimizelabelplacement_SOURCES = optimizelabelplacement.cc
vfrdboptimizelabelplacement_LDADD = ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @LIBXMLPP_LIBS@ @SQLITE_LIBS@ @SQLITE3X_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
vfrdboptimizelabelplacement_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrdbrebuildspatialindex_SOURCES = rebuildspatialindex.cc
vfrdbrebuildspatialindex_LDADD = ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @LIBXMLPP_LIBS@ @SQLITE_LIBS@ @SQLITE3X_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
vfrdbrebuildspatialindex_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrdbsettopo30_SOURCES = settopo30.cc
vfrdbsettopo30_LDADD = ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @SQLITE_LIBS@ @SQLITE3X_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@
vfrdbsettopo30_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrdbtopo30zerotiles_SOURCES = topo30zerotiles.cc
vfrdbtopo30zerotiles_LDADD = ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @SQLITE_LIBS@ @SQLITE3X_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@
vfrdbtopo30zerotiles_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrdbtopo30bin_SOURCES = topo30bin.cc
vfrdbtopo30bin_LDADD = ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @SQLITE_LIBS@ @SQLITE3X_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@
vfrdbtopo30bin_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrdbcamelcase_SOURCES = camelcase.cc
vfrdbcamelcase_LDADD = ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @SQLITE_LIBS@ @SQLITE3X_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
vfrdbcamelcase_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrdbxplaneexport_SOURCES = xplaneexport.cc
vfrdbxplaneexport_LDADD = ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @LIBXMLPP_LIBS@ @SQLITE_LIBS@ @SQLITE3X_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
vfrdbxplaneexport_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrdbcsv_SOURCES = csv.cc
vfrdbcsv_LDADD = ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @LIBXMLPP_LIBS@ @SQLITE_LIBS@ @SQLITE3X_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
vfrdbcsv_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrdbeadimport_SOURCES = eadimport.cc
vfrdbeadimport_LDADD = ../src/libvfrnav.la @LIBS@ @LIBXMLPP_LIBS@ @SQLITE_LIBS@ @GTKMM_LIBS@ @SQLITE3X_LIBS@ @GDAL_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
vfrdbeadimport_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrdbaixmimport_SOURCES = aixmimport.cc
vfrdbaixmimport_LDADD = ../src/libvfrnav.la @LIBS@ @LIBXMLPP_LIBS@ @SQLITE_LIBS@ @GTKMM_LIBS@ @SQLITE3X_LIBS@ @GDAL_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
vfrdbaixmimport_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrnavdbimport_SOURCES = navdbimport.cc
vfrnavdbimport_LDADD = ../src/libvfrnav.la @LIBS@ @LIBXMLPP_LIBS@ @SQLITE_LIBS@ @GTKMM_LIBS@ @SQLITE3X_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
vfrnavdbimport_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrdbfasimport_SOURCES = fasimport.cc
vfrdbfasimport_LDADD = ../src/libvfrnav.la @LIBS@ @LIBXMLPP_LIBS@ @SQLITE_LIBS@ @GTKMM_LIBS@ @SQLITE3X_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
vfrdbfasimport_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrnavgrib2dump_SOURCES = grib2dump.cc
vfrnavgrib2dump_LDADD = ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @SQLITE3X_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@

vfrnavgrib2fcst_SOURCES = grib2fcst.cc
vfrnavgrib2fcst_LDADD = ../src/libvfrnav.la @LIBS@ @LIBXMLPP_LIBS@ @GTKMM_LIBS@ @SQLITE3X_LIBS@ @PILOTLINK_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@

adsbdecode_SOURCES = adsbdecode.cc
adsbdecode_LDADD = ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @SQLITE3X_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@

windcalc_SOURCES = windcalc.cc
windcalc_LDADD = ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @SQLITE3X_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@

vfrdbukpostcodeimport_SOURCES = ukpostcodeimport.cc
vfrdbukpostcodeimport_LDADD = ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @SQLITE3X_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@ @GDAL_LIBS@

vfrnavfixdb_SOURCES = vfrnavfixdb.cc
vfrnavfixdb_LDADD = ../src/libvfrnav.la @LIBS@ @LIBXMLPP_LIBS@ @SQLITE_LIBS@ @GTKMM_LIBS@ @SQLITE3X_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
vfrnavfixdb_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrnavacftconv_SOURCES = acftconv.cc
vfrnavacftconv_LDADD = ../src/libvfrnav.la @LIBS@ @LIBXMLPP_LIBS@ @SQLITE_LIBS@ @SQLITE3X_LIBS@ @GLIBMM_LIBS@ @GIOMM_LIBS@ @GTKMM_LIBS@ @PILOTLINK_LIBS@ @BOOST_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@ @LIBARCHIVE_LIBS@
vfrnavacftconv_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

bin_PROGRAMS = vfrnavfplan vfrnavwmmconv vfrnavwmmtest vfrdbdafif vfrdbupdategndelev vfrdboptimizelabelplacement \
	vfrdbrebuildspatialindex vfrnavdb2xml vfrnavxml2db vfrdbsettopo30 vfrdbtopo30zerotiles vfrdbtopo30bin \
	vfrdbcamelcase vfrdbeadimport vfrdbaixmimport vfrdbfasimport aircraftopsperf vfrdbukpostcodeimport \
	vfrnavfixdb vfrnavacftconv vfrnavgrib2dump vfrnavgrib2fcst
noinst_PROGRAMS = xmlformat vfrnavdbimport adsbdecode windcalc

if HAVE_GDAL

vfrdbsrtmwatermask_SOURCES = srtmwatermask.cc
vfrdbsrtmwatermask_LDADD = ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @SQLITE_LIBS@ @SQLITE3X_LIBS@ @ZFSTREAM_LIBS@ @GDAL_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@
vfrdbsrtmwatermask_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrdbmapelementsdb_SOURCES = vmap.cc
vfrdbmapelementsdb_LDADD = ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @SQLITE_LIBS@ @SQLITE3X_LIBS@ @ZFSTREAM_LIBS@ @GDAL_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
vfrdbmapelementsdb_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrgshhsimport_SOURCES = gshhs.cc
vfrgshhsimport_LDADD = ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @SQLITE_LIBS@ @SQLITE3X_LIBS@ @ZFSTREAM_LIBS@ @GDAL_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
vfrgshhsimport_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

bin_PROGRAMS += vfrdbxplaneexport vfrdbcsv vfrdbsrtmwatermask vfrdbmapelementsdb vfrgshhsimport

if HAVE_JSONCPP
if HAVE_PODOFO

vfrpdfmanip_SOURCES = pdfmanip.cc
vfrpdfmanip_LDADD = ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @LIBXMLPP_LIBS@ @SQLITE_LIBS@ @SQLITE3X_LIBS@ @PILOTLINK_LIBS@ @GDAL_LIBS@ @GEOS_LIBS@ @JSONCPP_LIBS@ @PODOFO_LIBS@ @EIGEN3_LIBS@
vfrpdfmanip_LDFLAGS = @GEOS_LDFLAGS@

bin_PROGRAMS += vfrpdfmanip

endif
endif

endif

if HAVE_GIOMM

vfrdbsrtm30db_SOURCES = srtm30db.cc
vfrdbsrtm30db_LDADD = ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @SQLITE_LIBS@ @SQLITE3X_LIBS@ @ZFSTREAM_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@
vfrdbsrtm30db_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrdbxplane_SOURCES = xplane.cc
vfrdbxplane_LDADD = ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @LIBXMLPP_LIBS@ @SQLITE_LIBS@ @SQLITE3X_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
vfrdbxplane_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrdbopenair_SOURCES = openair.cc
vfrdbopenair_LDADD = ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @LIBXMLPP_LIBS@ @SQLITE_LIBS@ @SQLITE3X_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
vfrdbopenair_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

bin_PROGRAMS += vfrdbsrtm30db vfrdbxplane vfrdbopenair

if HAVE_BOOST

vfrdbairwaydump_SOURCES = airwaydump.cc
vfrdbairwaydump_LDADD = ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @LIBXMLPP_LIBS@ @SQLITE_LIBS@ @SQLITE3X_LIBS@ @PILOTLINK_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@
vfrdbairwaydump_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

bin_PROGRAMS += vfrdbairwaydump

endif

endif

xmlformat_SOURCES = xmlformat.cc
xmlformat_LDADD = @LIBS@ @LIBXMLPP_LIBS@

EXTRA_DIST = srtmwatermask.cc vmap.cc gshhs.cc srtm30db.cc xplane.cc openair.cc airwaydump.cc pdfmanip.cc

AM_CFLAGS = @MY_CFLAGS@
AM_CXXFLAGS = @MY_CXXFLAGS@ @GTKMM_CFLAGS@ @GLIBMM_CFLAGS@ @GIOMM_CFLAGS@ @LIBXMLPP_CFLAGS@ \
	@SQLITE_CFLAGS@ @SQLITE3X_CFLAGS@ @ZFSTREAM_CFLAGS@ @GDAL_CFLAGS@ @PILOTLINK_CFLAGS@ \
	@GEOS_CPPFLAGS@ @CLIPPER_CPPFLAGS@ @OPENJPEG_CFLAGS@ @JSONCPP_CFLAGS@ @EIGEN3_CFLAGS@ \
	@LIBARCHIVE_CFLAGS@
AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/include
