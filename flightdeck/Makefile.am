SUBDIRS = ui configs

bin_PROGRAMS = flightdeck

noinst_HEADERS = \
	flightdeckwindow.h \
	sensors.h \
	sensgps.h

BUILT_SOURCES =
CLEAN_FILES =

flightdeck_SOURCES = flightdeck.cc flightdeckwindow.cc splash.cc gyroinst.cc altinst.cc altdlg.cc hsidlg.cc \
	mapdlg.cc maptile.cc maps.cc softkeys.cc navarea.cc navinfoarea.cc wptpage.cc textentrydlg.cc fplpage.cc fpldirpage.cc \
	fplgpsimppage.cc keyboarddlg.cc coorddlg.cc suspenddlg.cc docpage.cc srsspage.cc perfpage.cc logpage.cc \
	grib2page.cc fplatmodlg.cc fplverifdlg.cc fplautoroutedlg.cc nwxchartdlg.cc routeprofile.cc svgimage.cc \
	senscfg.cc sensors.cc sensnav.cc sensgps.cc senscfgmagcalib.cc senscfggps.cc sensgpsking.cc sensgpsking.h \
	sensgpskingtty.cc sensgpskingtty.h sensgpsnmea.cc sensgpsnmea.h \
	sensadsb.cc sensadsb.h sensrtladsb.cc sensrtladsb.h sensremoteadsb.cc sensremoteadsb.h
nodist_flightdeck_SOURCES =
flightdeck_LDADD = ../cfmu/libcfmuautoroute.a ../src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ @GLIBMM_LIBS@ @GIOMM_LIBS@ \
	@EVINCE_LIBS@ @GPS_LIBS@ @LOCATION_LIBS@ @GYPSY_LIBS@ @SQLITE3X_LIBS@ @GTHREAD_LIBS@ \
	@DBUS_LIBS@ @HILDONMM_LIBS@ @GNOMEBT_LIBS@ @PILOTLINK_LIBS@ @BOOST_LIBS@ @BLUEZ_LIBS@ \
	@EIGEN3_LIBS@ @UDEV_LIBS@ @HIDAPI_LIBS@ @GEOCLUE_LIBS@ @GSL_LIBS@ @LIBUSB1_LIBS@ @FTDI_LIBS@ \
	@X11_LIBS@ @LIBXMLPP_LIBS@ @CURL_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@ @LIBSOUP_LIBS@ \
	@GCONF_LIBS@ @JSONCPP_LIBS@
flightdeck_LDFLAGS = @BOOST_LDFLAGS@ @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@
EXTRA_flightdeck_SOURCES = flightdeckrc.rc sensattitude.h sensattitude.cc ahrs.h ahrs.cc sensattpsmove.h sensattpsmove.cc \
	sensattpsmovebt.h sensattpsmovebt.cc sensattpsmovehid.h sensattpsmovehid.cc \
	sensgpsd.h sensgpsd.cc sensgypsy.h sensgypsy.cc senslibloc.h senslibloc.cc \
	sensgeoclue.h sensgeoclue.cc sensgeoclue2.h sensgeoclue2.cc \
	sensgpskingftdi.h sensgpskingftdi.cc sensms5534.cc sensms5534.h \
	win/sensors.h win/sensorspk.cc senswingps.h senswingps.cc \
	senswinbaro.h senswinbaro.cc senswinatt.h senswinatt.cc sensattiio.h sensattiio.cc sensattstmhub.h sensattstmhub.cc

if WIN32
flightdeck_SOURCES += flightdeckrc.rc
endif

if HAVE_EIGEN3
flightdeck_SOURCES += sensattitude.h sensattitude.cc ahrs.h ahrs.cc sensattpsmove.h sensattpsmove.cc
if HAVE_BLUEZ
flightdeck_SOURCES += sensattpsmovebt.h sensattpsmovebt.cc
endif
if HAVE_UDEV
flightdeck_SOURCES += sensattiio.h sensattiio.cc
if HAVE_HIDAPI
flightdeck_SOURCES += sensattpsmovehid.h sensattpsmovehid.cc
endif
if HAVE_LIBUSB1
flightdeck_SOURCES += sensattstmhub.h sensattstmhub.cc
endif
endif
endif

if HAVE_LIBGPS
flightdeck_SOURCES += sensgpsd.h sensgpsd.cc
endif

if HAVE_GYPSY
flightdeck_SOURCES += sensgypsy.h sensgypsy.cc
endif

if HAVE_LIBLOCATION
flightdeck_SOURCES += senslibloc.h senslibloc.cc
endif

if HAVE_GEOCLUE
flightdeck_SOURCES += sensgeoclue.h sensgeoclue.cc
endif

if HAVE_GEOCLUE2
flightdeck_SOURCES += sensgeoclue2.h sensgeoclue2.cc
nodist_flightdeck_SOURCES += geoclue2.h geoclue2.c
BUILT_SOURCES += geoclue2.h geoclue2.c
CLEAN_FILES += geoclue2.h geoclue2.c

geoclue2.h geoclue2.c:	@GEOCLUE2_DBUS_INTERFACE@
			gdbus-codegen --interface-prefix org.freedesktop. --generate-c-code geoclue2 $<
endif

if HAVE_FTDI
flightdeck_SOURCES += sensgpskingftdi.h sensgpskingftdi.cc sensms5534.cc sensms5534.h
bin_PROGRAMS += flightdeckftdieeprog
flightdeckftdieeprog_SOURCES = ftdieeprog.cc
flightdeckftdieeprog_LDADD = @GLIBMM_LIBS@ @FTDI_LIBS@
if WIN32
flightdeck_LDADD += -lftd2xx
flightdeckftdieeprog_LDADD += -lftd2xx
endif
endif

if WIN32
flightdeck_SOURCES += senswingps.h senswingps.cc senswinbaro.h senswinbaro.cc senswinatt.h senswinatt.cc \
	win/sensors.h win/sensorspk.cc
flightdeck_LDADD += -lsensorsapi
endif

EXTRA_DIST = ftdieeprog.cc

AM_CFLAGS = @MY_CFLAGS@
AM_CXXFLAGS = @MY_CXXFLAGS@
AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/include -I$(top_srcdir)/cfmu -I$(srcdir) \
	@GTKMM_CFLAGS@ @GLIBMM_CFLAGS@ @GIOMM_CFLAGS@ @EVINCE_CFLAGS@ \
	@LIBXMLPP_CFLAGS@ @SQLITE_CFLAGS@ @SQLITE3X_CFLAGS@ @ZFSTREAM_CFLAGS@ @LOCATION_CFLAGS@ \
	@GYPSY_CFLAGS@ @GTHREAD_CFLAGS@ @DBUS_CFLAGS@ @HILDONMM_CFLAGS@ @GNOMEBT_CFLAGS@ @PILOTLINK_CFLAGS@ \
	@BOOST_CPPFLAGS@ @BLUEZ_CFLAGS@ @EIGEN3_CFLAGS@ @UDEV_CFLAGS@ @HIDAPI_CFLAGS@ @GEOCLUE_CFLAGS@ @GSL_CFLAGS@ \
	@LIBUSB1_CFLAGS@ @FTDI_CFLAGS@ @LIBXMLPP_CFLAGS@ @CURL_CFLAGS@ @GEOS_CPPFLAGS@ @CLIPPER_CPPFLAGS@ \
	@OPENJPEG_CFLAGS@ @LIBSOUP_CFLAGS@ @GCONF_CFLAGS@ @JSONCPP_CFLAGS@

.rc.o:
	$(RC) -I$(top_srcdir) -I$(top_srcdir)/ui -o $@ $<
