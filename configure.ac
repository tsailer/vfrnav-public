AC_INIT([vfrnav], [20180129])
AC_CONFIG_SRCDIR([configure.ac])
AM_INIT_AUTOMAKE([subdir-objects tar-pax])
AM_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])

#AX_CHECK_ENABLE_DEBUG([yes])
#AX_CODE_COVERAGE
#AX_COMPILER_FLAGS([WARN_CFLAGS], [WARN_LDFLAGS], [$ax_is_release], [],
#        dnl TODO: Remove! These warnings should be fixed, not silenced.
#        [-Wno-deprecated-declarations])

save_CFLAGS=${CFLAGS}
AC_PROG_CC
MY_CFLAGS=${CFLAGS}
CFLAGS=${save_CFLAGS}
save_CXXFLAGS=${CXXFLAGS}
AC_PROG_CXX
MY_CXXFLAGS=${CXXFLAGS}
CXXFLAGS=${save_CXXFLAGS}

LT_INIT
#LT_LANG([C++])
#LT_LANG([RC])

AC_ISC_POSIX
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PROG_CPP
AC_PROG_CXXCPP
AX_CXX_COMPILE_STDCXX_11(noext,optional)

AC_LANG_CPLUSPLUS

AC_CYGWIN
AC_MINGW32
AC_EXEEXT
AC_OBJEXT

LT_PROG_RC

#AC_CHECK_PROG(WINDRES, windres, windres, i686-pc-mingw32-windres)
AC_PATH_PROG([GLIB_MKENUMS],[glib-mkenums])
AC_CHECK_PROGS(ANTLR, 'antlr', :)
AC_PATH_PROG(ANTLR_CONFIG,antlr-config,:)
if test "$ANTLR_CONFIG" = ":" ; then
  AC_MSG_WARN(Could not find antlr-config; using defaults)
  ANTLRCFLAGS=""
  ANTLRLIBS="-lantlr"
else
  ANTLRCFLAGS=`$ANTLR_CONFIG --cflags`
  ANTLRLIBS=`$ANTLR_CONFIG --libs`
fi
AC_SUBST(ANTLRCFLAGS)
AC_SUBST(ANTLRLIBS)

# Option to enable debugging
AC_ARG_ENABLE(debug, 
	      [AC_HELP_STRING([--enable-debug],[Debugging (default=no)])], 
	      [with_debug=$enableval], [with_debug=no])

AC_ARG_ENABLE(gtk3, 
	      [AC_HELP_STRING([--disable-gtk3],[GTK3 (default=yes)])], 
	      [with_gtk3=$enableval], [with_gtk3=yes])

AC_ARG_ENABLE(gnomebt, 
	      [AC_HELP_STRING([--disable-gnomebt],[Gnome Bluetooth (default=yes)])], 
	      [with_gnomebt=$enableval], [with_gnomebt=yes])

AC_ARG_ENABLE(flightdeck, 
	      [AC_HELP_STRING([--disable-flightdeck],[Flightdeck Application (default=yes)])], 
	      [with_flightdeck=$enableval], [with_flightdeck=yes])

AC_ARG_ENABLE(cfmu, 
	      [AC_HELP_STRING([--disable-cfmu],[Cfmu Application (default=yes)])], 
	      [with_cfmu=$enableval], [with_cfmu=yes])

AC_ARG_ENABLE(docs, 
	      [AC_HELP_STRING([--disable-docs],[Docs Application (default=yes)])], 
	      [with_docs=$enableval], [with_docs=yes])

AC_ARG_ENABLE(mtmaps, 
	      [AC_HELP_STRING([--enable-mtmaps],[Multithreaded Maps (default=no)])], 
	      [with_mtmaps=$enableval], [with_mtmaps=no])

if test "x$with_debug" == "xyes" ; then
   if test "x$c_cv_prog_cc_g" == "xyes" ; then
      MY_CFLAGS="${MY_CFLAGS} -ggdb -O0 -DDEBUG"
   fi
   if test "x$ac_cv_prog_cxx_g" == "xyes" ; then
      MY_CXXFLAGS="${MY_CXXFLAGS} -ggdb -O0 -DDEBUG"
   fi	
else
   if test "x$c_cv_prog_cc_g" == "xyes" ; then
      MY_CFLAGS="${MY_CFLAGS} -O2 -g"
   fi
   if test "x$ac_cv_prog_cxx_g" == "xyes" ; then
      MY_CXXFLAGS="${MY_CXXFLAGS} -O2 -g"
   fi	
fi
MY_CFLAGS="${MY_CFLAGS} -Wall"
MY_CXXFLAGS="${MY_CXXFLAGS} -Wall"
AC_SUBST(MY_CFLAGS)
AC_SUBST(MY_CXXFLAGS)

AC_MSG_CHECKING([__int128])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [__int128 x;])],
                  [AC_MSG_RESULT([yes]); AC_DEFINE([DEFINE_INT128],[typedef __int128 int128_t;],[define int128_t type])],
                  [AC_MSG_RESULT([no]); AC_MSG_CHECKING([int128_t]);
                  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [int128_t x;])],
                                    [AC_MSG_RESULT([yes]); AC_DEFINE([DEFINE_INT128],[],[define int128_t type])],
                                    [AC_MSG_RESULT([no]); AC_MSG_CHECKING([boost int128]);
                                    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <boost/multiprecision/cpp_int.hpp>], [boost::multiprecision::int128_t x;])],
                                                      [AC_MSG_RESULT([yes]);
						       AC_DEFINE([DEFINE_INT128],[typedef boost::multiprecision::int128_t int128_t;],[define int128_t type])
						       AC_DEFINE([BOOST_INT128],[1],[define if boost multiprecision int128_t type])],
                                                      [AC_MSG_RESULT([no]); AC_MSG_ERROR([no int128 type])])])])

AC_MSG_CHECKING([unique_ptr])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <memory>], [std::unique_ptr<int> x;])],
                  [AC_MSG_RESULT([yes])],
                  [AC_MSG_RESULT([no]); AC_DEFINE([unique_ptr],[auto_ptr],[Define if unique_ptr is not supported])])

AC_CHECK_HEADERS([sys/syscall.h pwd.h grp.h sys/utsname.h sysexits.h termios.h pty.h linux/usb/ch9.h windows.h objbase.h sensorsapi.h unistd.h sys/mman.h sys/prctl.h execinfo.h endian.h sys/prctl.h])
AC_CHECK_FUNCS([sysconf prctl])
AC_FUNC_MMAP
AC_FUNC_FORK
AM_ICONV

AC_MSG_CHECKING([constexpr])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([class X { static constexpr double Y = 1.2; };], [])],
                  [AC_MSG_RESULT([yes])],
                  [AC_MSG_RESULT([no]); AC_MSG_CHECKING([const]);
		   AC_COMPILE_IFELSE([AC_LANG_PROGRAM([class X { static const double Y = 1.2; };], [])],
		                     [AC_MSG_RESULT([yes]); AC_DEFINE([constexpr],[const],[Define if constexpr is not supported])],
                                     [AC_MSG_RESULT([no]); AC_MSG_ERROR([Cannot define floating point constants])])])

PKG_CHECK_MODULES([GTHREAD], [gthread-2.0 >= 2.0])
PKG_CHECK_MODULES([GLIBMM], [glibmm-2.4 >= 2.10.0])
PKG_CHECK_MODULES([GIOMM], [giomm-2.4 >= 2.10.0], [have_giomm=yes], [have_giomm=no])
have_gtkmm=no
have_gtkmm2=no
have_gtkmm3=no
have_webkitgtk=no
have_webkitgtkwebext=no
have_evince=no
if test "x$with_gtk3" != "xno" ; then
   PKG_CHECK_MODULES([GTKMM], [gtkmm-3.0 >= 3.0.0], [have_gtkmm=yes have_gtkmm3=yes], [have_gtkmm=no])
   if test "x$have_gtkmm" == "xyes" ; then
      PKG_CHECK_MODULES([EVINCE], [evince-document-3.0 evince-view-3.0], [have_evince=yes], [have_evince=no])
      PKG_CHECK_MODULES([WEBKITGTK4], [webkit2gtk-4.0], [have_webkitgtk4=yes], [have_webkitgtk4=no])
      PKG_CHECK_MODULES([WEBKITGTKWEBEXT], [webkit2gtk-web-extension-4.0], [have_webkitgtkwebext=yes], [have_webkitgtkwebext=no])
      if test "x$have_webkitgtk4" == "xno" ; then
         PKG_CHECK_MODULES([WEBKITGTK], [webkitgtk-3.0 >= 1.8.1], [have_webkitgtk=yes], [have_webkitgtk=no])
      fi
   fi
fi
if test "x$have_gtkmm" == "xno" ; then
   PKG_CHECK_MODULES([GTKMM], [gtkmm-2.4 >= 2.6.0 libglademm-2.4 >= 2.6.0], [have_gtkmm=yes have_gtkmm2=yes], [have_gtkmm=no])
   PKG_CHECK_MODULES([EVINCE], [evince-document-2.32 evince-view-2.32], [have_evince=yes], [have_evince=no])
   PKG_CHECK_MODULES([WEBKITGTK], [webkit-1.0 >= 1.8.1], [have_webkitgtk=yes], [have_webkitgtk=no])
fi

PKG_CHECK_MODULES([CAIROMM], [cairomm-1.0])
PKG_CHECK_MODULES([LIBRSVG], [librsvg-2.0])
PKG_CHECK_MODULES([LIBSOUP], [libsoup-2.4])
PKG_CHECK_MODULES([LIBXMLPP], [libxml++-2.6 >= 2.14.0])
PKG_CHECK_EXISTS([sqlite36 >= 3.6.1], [PKG_CHECK_MODULES([SQLITE], [sqlite36 >= 3.6.1])], [PKG_CHECK_MODULES([SQLITE], [sqlite3 >= 3.0])])
PKG_CHECK_MODULES([SQLITE3X], [libsqlite3x >= 20071018])
PKG_CHECK_MODULES([ZFSTREAM], [zfstream >= 20041202])
PKG_CHECK_MODULES([GDAL], [gdal >= 1.4.0], [have_gdal=yes], [have_gdal=no])
PKG_CHECK_MODULES([GDAL2], [gdal >= 2.0.0], [have_gdal2=yes], [have_gdal2=no])
PKG_CHECK_MODULES([PILOTLINK], [pilot-link >= 0.11.0], [have_pilotlink=yes], [have_pilotlink=no])
PKG_CHECK_MODULES([BLUEZ], [bluez], [have_bluez=yes], [have_bluez=no])
PKG_CHECK_MODULES([EIGEN3], [eigen3], [have_eigen3=yes], [have_eigen3=no])
PKG_CHECK_MODULES([UDEV], [libudev], [have_udev=yes], [have_udev=no])
PKG_CHECK_MODULES([HIDAPI], [hidapi], [have_hidapi=yes], [have_hidapi=no])
PKG_CHECK_MODULES([GSL], [gsl], [have_gsl=yes], [have_gsl=no])
PKG_CHECK_MODULES([LIBUSB1], [libusb-1.0], [have_libusb1=yes], [have_libusb1=no])
PKG_CHECK_MODULES([FTDI], [libftdi1 >= 1.0], [have_ftdi=libftdi1], [PKG_CHECK_MODULES([FTDI], [libftdi >= 0.18], [have_ftdi=libftdi], [AC_CHECK_HEADERS([ftdi/ftd2xx.h], [have_ftdi=d2xx], [have_ftdi=no],
[[#ifdef HAVE_WINDOWS_H
# include <windows.h>
#endif
]])])])
#dnl PKG_CHECK_MODULES([POPPLER], [poppler], [have_poppler=yes], [have_poppler=no])
PKG_CHECK_MODULES([CURL], [libcurl >= 7.0.0], [have_curl=yes], [have_curl=no])
PKG_CHECK_MODULES([OPENJPEG], [libopenjp2], [have_openjpeg=yes],
                  [PKG_CHECK_MODULES([OPENJPEG], [libopenjpeg], [AC_DEFINE([HAVE_OPENJPEG1],[1],[Define if openjpeg version 1.x]) have_openjpeg=yes], [have_openjpeg=no])])
PKG_CHECK_MODULES([GCONF], [gconf-2.0], [have_gconf=yes], [have_gconf=no])
PKG_CHECK_MODULES([QRENCODE], [libqrencode], [have_qrencode=yes], [have_qrencode=no])
PKG_CHECK_MODULES([LIBARCHIVE], [libarchive], [have_libarchive=yes], [have_libarchive=no])

AC_CHECK_LIB(z,deflate)
AC_CHECK_LIB(util,openpty,[OPENPTY_LIBS=-lutil AC_DEFINE([HAVE_LIBUTIL],[1],[Define if libutil is available])])
AC_SUBST(OPENPTY_LIBS)

LIBS1="$LIBS";
LIBS="$LIBS $GIOMM_LIBS";
AC_CHECK_FUNC(_ZN3Gio4File27make_directory_with_parentsEv,[AC_DEFINE([HAVE_GIOMM_MAKE_DIRECTORY_WITH_PARENTS],[1],[Define if giomm24 supports make_directory_with_parents])])
LIBS="$LIBS1";

AC_CHECK_HEADERS([gps.h])
AC_CHECK_LIB(gps, gps_open, [AC_DEFINE([HAVE_LIBGPS],[1],[Defined if libgps is available]) GPS_LIBS=-lgps have_libgps=yes],[have_libgps=no])

AM_CONDITIONAL(HAVE_LIBGPS, test x$have_libgps = xyes)

PKG_CHECK_MODULES([LOCATION], [liblocation >= 0.26], [have_liblocation=yes], [have_liblocation=no])
if test "x${have_liblocation}" = "xyes"; then
  AC_DEFINE(HAVE_LIBLOCATION,[],[Nokia Location Library])
fi
AC_SUBST(HAVE_LIBLOCATION)

AM_CONDITIONAL(HAVE_LIBLOCATION, test x$have_liblocation = xyes)

PKG_CHECK_MODULES([GEOCLUE], [geoclue >= 0.11], [have_geoclue=yes], [have_geoclue=no])
if test "x${have_geoclue}" = "xyes"; then
  AC_DEFINE(HAVE_GEOCLUE,[],[Geoclue Library])
fi
AC_SUBST(HAVE_GEOCLUE)

AM_CONDITIONAL(HAVE_GEOCLUE, test x$have_geoclue = xyes)

PKG_CHECK_MODULES([GEOCLUE2], [geoclue-2.0 >= 2.0.0], [have_geoclue2=yes], [have_geoclue2=no])
if test "x${have_geoclue2}" = "xyes"; then
  AC_DEFINE(HAVE_GEOCLUE2,[],[GeoClue2 Library])
  PKG_CHECK_VAR([GEOCLUE2_DBUS_INTERFACE], [geoclue-2.0], [dbus_interface])
  AC_MSG_CHECKING([geoclue2 DBus interface path])
  AS_IF([test "x$GEOCLUE2_DBUS_INTERFACE" = "x"], [
    AC_MSG_FAILURE([Unable to identify geoclue2 DBus interface path.])
  ])
  AC_MSG_RESULT([$GEOCLUE2_DBUS_INTERFACE])
fi
AC_SUBST(HAVE_GEOCLUE2)

AM_CONDITIONAL(HAVE_GEOCLUE2, test x$have_geoclue2 = xyes)

PKG_CHECK_MODULES([GYPSY], [gypsy], [have_gypsy=yes], [have_gypsy=no])
if test "x${have_gypsy}" = "xyes"; then
  AC_DEFINE(HAVE_GYPSY,[],[Gypsy GPS Daemon])
fi
AC_SUBST(HAVE_GYPSY)

AM_CONDITIONAL(HAVE_GYPSY, test x$have_gypsy = xyes)

if test "x${with_gnomebt}" = "xyes"; then
   PKG_CHECK_MODULES([GNOMEBT], [gnome-bluetooth-1.0], [have_gnomebt=yes], [have_gnomebt=no])
else
  have_gnomebt=no
fi
if test "x${have_gnomebt}" = "xyes"; then
  AC_DEFINE(HAVE_GNOMEBT,[],[Gnome Bluetooth Libraries])
fi
AC_SUBST(HAVE_GNOMEBT)

PKG_CHECK_MODULES([PQXX], [libpqxx], [have_pqxx=yes], [have_pqxx=no])
if test "x${have_pqxx}" = "xyes"; then
  AC_DEFINE(HAVE_PQXX,[],[PostgreSQL C++])
fi
AC_SUBST(HAVE_PQXX)

AM_CONDITIONAL(HAVE_PQXX, test x$have_pqxx = xyes)

PKG_CHECK_MODULES([MAGICKPP], [GraphicsMagick++], [have_magickpp=yes], [have_magickpp=no])
if test "x${have_magickpp}" = "xyes"; then
  AC_DEFINE(HAVE_MAGICKPP,[],[GraphicsMagick++])
fi
AC_SUBST(HAVE_MAGICKPP)

AM_CONDITIONAL(HAVE_MAGICKPP, test x$have_magickpp = xyes)

PKG_CHECK_MODULES([PROTOBUF], [protobuf], [have_protobuf=yes], [have_protobuf=no])
if test "x${have_protobuf}" = "xyes"; then
  AC_DEFINE(HAVE_PROTOBUF,[],[Google Protocol Buffers])
fi
AC_SUBST(HAVE_PROTOBUF)

AM_CONDITIONAL(HAVE_PROTOBUF, test x$have_protobuf = xyes)

AM_GNU_GETTEXT([external])

AX_BOOST_BASE([1.30.0],[have_boost=yes],[have_boost=no])
if test "x${have_boost}" = "xyes"; then
   CPPFLAGS1="$CPPFLAGS";
   CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS";
   AC_CHECK_HEADERS([boost/geometry.hpp])
   CPPFLAGS="$CPPFLAGS1";
fi

have_geos=yes;
AC_ARG_WITH([geosconfig],
        [AS_HELP_STRING([--with-geosconfig=FILE], [specify an alternative geos-config file])],
        [GEOSCONFIG="$withval"], [GEOSCONFIG=""])

if test "x$GEOSCONFIG" = "x"; then
        dnl GEOSCONFIG was not specified, so search within the current path
        AC_PATH_PROG([GEOSCONFIG], [geos-config])

        dnl If we couldn't find geos-config, display an error
        if test "x$GEOSCONFIG" = "x"; then
                AC_MSG_WARN([could not find geos-config within the current path. You may need to try re-running configure with a --with-geosconfig parameter.])
		have_geos=no
        fi
else
        dnl GEOSCONFIG was specified; display a message to the user
        if test "x$GEOSCONFIG" = "xyes"; then
                AC_MSG_WARN([you must specify a parameter to --with-geosconfig, e.g. --with-geosconfig=/path/to/geos-config])
		have_geos=no
        else
                if test -f $GEOSCONFIG; then
                        AC_MSG_RESULT([Using user-specified geos-config file: $GEOSCONFIG])
                else
                        AC_MSG_WARN([the user-specified geos-config file $GEOSCONFIG does not exist])
			have_geos=no
                fi     
        fi
fi

if test ! -x "$GEOSCONFIG" ; then
        have_geos=no
fi

if test "x$have_geos" = "xyes"; then
   dnl Extract the version information from geos_config
   dnl Note: we extract the major & minor separately, ensure they are numeric,
   dnl and then combine to give the final version.
   dnl This is to guard against user error...
   GEOS_MAJOR_VERSION=`$GEOSCONFIG --version | cut -d. -f1 | sed 's/[[^0-9]]//g'`
   GEOS_MINOR_VERSION=`$GEOSCONFIG --version | cut -d. -f2 | sed 's/[[^0-9]]//g'`
   GEOS_PATCH_VERSION=`$GEOSCONFIG --version | cut -d. -f3 | sed 's/[[^0-9]]//g'`
   GEOS_FULL_VERSION=`$GEOSCONFIG --version`
   GEOS_VERSION="$GEOS_MAJOR_VERSION$GEOS_MINOR_VERSION"
   GEOS_NUMERIC_VERSION="$GEOS_MAJOR_VERSION$GEOS_MINOR_VERSION$GEOS_PATCH_VERSION"

   dnl Ensure that we are using GEOS >= 3.1.0
   AC_MSG_RESULT([checking GEOS version... $GEOS_FULL_VERSION])
   if test ! "$GEOS_NUMERIC_VERSION" -ge 311; then
      AC_MSG_WARN([vfrnav requires GEOS >= 3.1.1])
      have_geos=no
   fi
fi

if test "x$have_geos" = "xyes"; then
   dnl Extract the linker and include flags
   GEOS_LDFLAGS=`$GEOSCONFIG --ldflags`
   GEOS_CPPFLAGS=-I`$GEOSCONFIG --includes`

   dnl Ensure that we can parse geos.h
   CPPFLAGS_SAVE="$CPPFLAGS"
   CPPFLAGS="$GEOS_CPPFLAGS"
   AC_CHECK_HEADER([geos.h], [], [AC_MSG_WARN([could not find geos.h - you may need to specify the directory of a geos-config file using --with-geosconfig]); have_geos=no])

   dnl Ensure we can link against libgeos
   LDFLAGS_SAVE="$LDFLAGS"
   LDFLAGS="$GEOS_LDFLAGS -lgeos"
   AC_MSG_CHECKING([linking with libgeos])
   AC_LINK_IFELSE([AC_LANG_PROGRAM([#include <geos.h>], [geos::geom::geosversion();])],[AC_MSG_RESULT([yes])],
   		  [AC_MSG_WARN([could not find libgeos - you may need to specify the directory of a geos-config file using --with-geosconfig]); AC_MSG_RESULT([no]); have_geos=no])
   LDFLAGS="$LDFLAGS_SAVE"
   CPPFLAGS="$CPPFLAGS_SAVE"
fi

if test "x${have_geos}" = "xyes"; then
   AC_DEFINE_UNQUOTED([GEOS_VERSION], [$GEOS_VERSION], [GEOS library version])     
   AC_SUBST([GEOS_VERSION])
   AC_SUBST(GEOS_LDFLAGS)
   AC_SUBST(GEOS_CPPFLAGS)
   GEOS_LIBS="-lgeos"
   AC_SUBST(GEOS_LIBS)
   AC_DEFINE(HAVE_GEOS,[],[GEOS library])
fi
AM_CONDITIONAL(HAVE_GEOS, test x$have_geos = xyes)

LIBS_SAVE="$LIBS"
LIBS="-lpolyclipping"
AC_CHECK_HEADER(polyclipping/clipper.hpp,[have_clipper=yes],[have_clipper=no])
if test "x${have_clipper}" = "xyes"; then
   AC_MSG_CHECKING([linking with libpolyclipping])
   AC_LINK_IFELSE([AC_LANG_PROGRAM([#include <polyclipping/clipper.hpp>], [ClipperLib::Clipper c; ClipperLib::PolyTree pt;])],
                  [AC_DEFINE(HAVE_CLIPPER_POLYTREE,[],[Clipper Library has PolyTree API]) AC_MSG_RESULT([yes])],
   		  [AC_LINK_IFELSE([AC_LANG_PROGRAM([#include <polyclipping/clipper.hpp>],
                                  [ClipperLib::Clipper c; ClipperLib::ExPolygon ep;])],[AC_DEFINE(HAVE_CLIPPER_EXPOLYGON,[],[Clipper Library has ExPolygon API]) AC_MSG_RESULT([yes])],
                                  [AC_MSG_WARN([could not find libpolyclipping]); AC_MSG_RESULT([no]); have_clipper=no])])
   AC_MSG_CHECKING([clipper polygon class])
   AC_LINK_IFELSE([AC_LANG_PROGRAM([#include <polyclipping/clipper.hpp>], [ClipperLib::Path p;])],
                  [AC_DEFINE(HAVE_CLIPPER_PATH,[],[Clipper Library has Path class]) AC_MSG_RESULT([yes])],
   		  [AC_LINK_IFELSE([AC_LANG_PROGRAM([#include <polyclipping/clipper.hpp>],
                                  [ClipperLib::Polygon p;])],[AC_DEFINE(HAVE_CLIPPER_POLYGON,[],[Clipper Library has Polygon class]) AC_MSG_RESULT([yes])],
                                  [AC_MSG_WARN([could not find libpolyclipping]); AC_MSG_RESULT([no]); have_clipper=no])])
fi
LIBS="$LIBS_SAVE"

if test "x${have_clipper}" = "xyes"; then
   CLIPPER_CPPFLAGS=""
   CLIPPER_LDFLAGS=""
   CLIPPER_LIBS="-lpolyclipping"
   AC_SUBST(CLIPPER_CPPFLAGS)
   AC_SUBST(CLIPPER_LDFLAGS)
   AC_SUBST(CLIPPER_LIBS)
   AC_DEFINE(HAVE_CLIPPER,[],[Clipper library])
fi
AM_CONDITIONAL(HAVE_CLIPPER, test x$have_clipper = xyes)

PKG_CHECK_MODULES([WEBSOCKET], [websocketpp libcrypto], [have_websocket=yes], [have_websocket=no])
if test "x${have_websocket}" = "xyes"; then
  AC_DEFINE(HAVE_WEBSOCKET,[],[WebSocket Libraries])
fi
AC_SUBST(HAVE_WEBSOCKET)

AM_CONDITIONAL(HAVE_WEBSOCKET, test x$have_websocket = xyes)

PKG_CHECK_MODULES([JSONCPP], [jsoncpp], [have_jsoncpp=yes], [have_jsoncpp=no])
if test "x${have_jsoncpp}" = "xyes"; then
  AC_DEFINE(HAVE_JSONCPP,[],[Jsoncpp Libraries])
fi
AC_SUBST(HAVE_JSONCPP)

AM_CONDITIONAL(HAVE_JSONCPP, test x$have_jsoncpp = xyes)

AC_CHECK_HEADERS([systemd/sd-daemon.h])
PKG_CHECK_MODULES([SYSTEMDDAEMON], [libsystemd-daemon], [have_systemddaemon=yes],
  [PKG_CHECK_MODULES([SYSTEMDDAEMON], [libsystemd], [have_systemddaemon=yes], [have_systemddaemon=no])])
if test "x${have_systemddaemon}" = "xyes"; then
  AC_DEFINE(HAVE_SYSTEMDDAEMON,[],[SystemD Daemon Libraries])
fi
AC_SUBST(HAVE_SYSTEMDDAEMON)

AM_CONDITIONAL(HAVE_SYSTEMDDAEMON, test x$have_systemddaemon = xyes)

PKG_CHECK_MODULES([LIBCRYPTO], [libcrypto], [have_libcrypto=yes], [have_libcrypto=no])
if test "x${have_libcrypto}" = "xyes"; then
  AC_DEFINE(HAVE_LIBCRYPTO,[],[OpenSSL Crypto Library])
fi
AC_SUBST(HAVE_LIBCRYPTO)

AM_CONDITIONAL(HAVE_LIBCRYPTO, test x$have_libcrypto = xyes)

PKG_CHECK_MODULES([RTKLIB], [rtklib], [have_rtklib=yes], [have_rtklib=no])
if test "x${have_rtklib}" = "xyes"; then
  AC_DEFINE(HAVE_RTKLIB,[],[RTK Library])
fi
AC_SUBST(HAVE_RTKLIB)

AM_CONDITIONAL(HAVE_RTKLIB, test x$have_rtklib = xyes)

# Hildon library dependencies
#PKG_CHECK_MODULES([HILDON], [hildon-1 hildon-fm-2 libossohelp conbtdialogs libossoemailinterface hildon-base-lib], [have_hildon=yes], [have_hildon=no])
PKG_CHECK_MODULES([HILDON], [hildon-1 hildon-fm-2 hildon-help conbtdialogs], [have_hildon=yes], [have_hildon=no])

PKG_CHECK_MODULES([HILDONMM], [hildonmm >= 0.9 hildon-fmmm >= 0.9], [have_hildonmm=yes], [have_hildonmm=no])

# Only used for the .service file path
PKG_CHECK_MODULES([DBUS], [dbus-glib-1 >= 0.60], [have_dbus=yes], [have_dbus=no])

# OSSO application framework dependencies
PKG_CHECK_MODULES([OSSO], [osso-af-settings >= 0.8.5 libosso >= 0.9.17 gnome-vfs-2.0 >= 2.8.4.11 gnome-vfs-module-2.0 >= 2.8.4.11], [have_osso=yes], [have_osso=no])

if test "x${have_hildon}" = "xyes"; then
   AC_DEFINE(HAVE_HILDON,[],[Nokia Hildon Framework])
fi

if test "x${have_hildonmm}" = "xyes"; then
   AC_DEFINE(HAVE_HILDONMM,[],[Nokia/Openismus Hildonmm Framework])
fi

if test "x${have_osso}" = "xyes"; then
   AC_DEFINE(HAVE_OSSO,[],[Nokia Osso Framework])
fi

if test "x${have_osso}" = "xyes"; then
  # To make application visible in maemo Task Navigator it needs a Desktop
  # file for the application.
  # D-BUS service file is needed to be able to launch the maemo application
  # and connect it to D-BUS services.
  # The following line defines install directories for these files.
  desktopentrydir=`$PKG_CONFIG osso-af-settings --variable=desktopentrydir`
  serviceentrydir=`$PKG_CONFIG osso-af-settings --variable=dbusservicedir`

  # Application locale install directory
  localedir=`$PKG_CONFIG osso-af-settings --variable=localedir`

  # Application pixmaps install directory
  pixmapdir=`$PKG_CONFIG osso-af-settings --variable=hildonpixmapdir`

  # Application icon install directories
  icon_26x26dir=$datadir/icons/hicolor/26x26/hildon
  icon_34x34dir=$datadir/icons/hicolor/34x34/hildon
  icon_40x40dir=$datadir/icons/hicolor/40x40/hildon
  icon_50x50dir=$datadir/icons/hicolor/50x50/hildon
  icon_scalabledir=$datadir/icons/hicolor/scalable/hildon

  # Help file install directory
  helpfile_engbdir=$datadir/hildon-help/en_GB

  # Hildon control panel plugin install directories
  pluginlibdir=`$PKG_CONFIG hildon-control-panel --variable=plugindir`
  plugindesktopentrydir=`$PKG_CONFIG hildon-control-panel --variable=plugindesktopentrydir`
else
  desktopentrydir=${datadir}/applications
  serviceentrydir=${datadir}/dbus-1/services/

  # Application locale install directory
  localedir=${datadir}/locale

  # Application pixmaps install directory
  pixmapdir=${datadir}/pixmaps

  # Application icon install directories
  icon_26x26dir=${datadir}/icons/hicolor/26x26/apps
  icon_34x34dir=${datadir}/icons/hicolor/34x34/apps
  icon_40x40dir=${datadir}/icons/hicolor/40x40/apps
  icon_50x50dir=${datadir}/icons/hicolor/50x50/apps
  icon_scalabledir=${datadir}/icons/hicolor/scalable/apps

  # Help file install directory
  helpfile_engbdir=${datadir}/vfrnav/help/en_GB

  # Hildon control panel plugin install directories
  pluginlibdir=/tmp
  plugindesktopentrydir=/tmp
fi

dnl Screen Saver stuff
AC_CHECK_HEADERS([X11/Xlib.h])
AC_CHECK_HEADERS([X11/extensions/scrnsaver.h X11/extensions/dpms.h],[],[],
[#ifdef HAVE_X11_XLIB_H
#include <X11/Xlib.h>
#endif
])
AC_CHECK_LIB(X11, XResetScreenSaver, [AC_DEFINE([HAVE_X11_RSS],[1],[Defined if XResetScreenSaver is available]) X11_LIBS="${X11_LIBS} -lX11" have_x11_rss=yes],[have_x11_rss=no])
AC_CHECK_LIB(Xss, XScreenSaverSuspend, [AC_DEFINE([HAVE_X11_XSS],[1],[Defined if XScreenSaverSuspend is available]) X11_LIBS="${X11_LIBS} -lXss" have_x11_xss=yes],[have_x11_xss=no])
AC_CHECK_LIB(Xext, DPMSDisable, [AC_DEFINE([HAVE_X11_DPMS],[1],[Defined if DPMSDisable is available]) X11_LIBS="${X11_LIBS} -lXext" have_x11_dpms=yes],[have_x11_dpms=no])
AC_CHECK_FUNC(open_memstream, [AC_DEFINE([HAVE_OPEN_MEMSTREAM],[1],[Defined if open_memstream is available])])

CPPFLAGS1="$CPPFLAGS";
CPPFLAGS="$CPPFLAGS $GTKMM_CFLAGS";
AC_CHECK_DECL(GDK_KEY_F1, [], [AC_DEFINE([HAVE_GDKKEYCOMPAT],[1],[Defined if GDK_KEY compat is needed])], [[#include <gdk/gdkkeysyms.h>]])
CPPFLAGS="$CPPFLAGS1";

LIBS_SAVE="$LIBS"
LIBS="-lpodofo"
AC_CHECK_HEADER(podofo/podofo.h,[have_podofo=yes],[have_podofo=no])
if test "x${have_podofo}" = "xyes"; then
   AC_MSG_CHECKING([linking with libpodofo])
   AC_LINK_IFELSE([AC_LANG_PROGRAM([#include <podofo/podofo.h>], [PoDoFo::PdfMemDocument d;])],
                  [AC_DEFINE([HAVE_PODOFO],[1],[Defined if PoDoFo Library is available]) AC_MSG_RESULT([yes])],
   		  [AC_MSG_WARN([could not find libpodofo]); AC_MSG_RESULT([no]); have_podofo=no])
fi
if test "x${have_podofo}" = "xyes"; then
   PODOFO_LIBS="$LIBS"
fi
LIBS="$LIBS_SAVE"

AC_CHECK_SIZEOF([wchar_t])

dnl Set PACKAGE_LOCALE_DIR in config.h.
if test "x${prefix}" = "xNONE"; then
  AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR,"${ac_default_prefix}/${DATADIRNAME}/locale",[various directories])
else
  AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR,"${prefix}/${DATADIRNAME}/locale")
fi

dnl Set PACKAGE_DATA_DIR in config.h.
if test "x${datarootdir}" = 'x${prefix}/share' -o "x${datadir}" = 'x${prefix}/share'; then
  if test "x${prefix}" = "xNONE"; then
    AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR,"${ac_default_prefix}/share/${PACKAGE}",[various directories])
  else
    AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR,"${prefix}/share/${PACKAGE}")
  fi
else
  AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR,"${datadir}/${PACKAGE}")
fi

dnl Set PACKAGE_LIBEXEC_DIR in config.h.
if test "x${libexecdir}" = 'x${exec_prefix}/libexec'; then
  if test "x${prefix}" = "xNONE"; then
    AC_DEFINE_UNQUOTED(PACKAGE_LIBEXEC_DIR,"${ac_default_prefix}/libexec/${PACKAGE}",[various directories])
  else
    AC_DEFINE_UNQUOTED(PACKAGE_LIBEXEC_DIR,"${prefix}/libexec/${PACKAGE}")
  fi
else
  AC_DEFINE_UNQUOTED(PACKAGE_LIBEXEC_DIR,"${libexecdir}/${PACKAGE}")
fi

dnl Set PACKAGE_LOCALSTATE_DIR in config.h.
if test "x${localstatedir}" = 'x${prefix}/var'; then
  if test "x${prefix}" = "xNONE"; then
    AC_DEFINE_UNQUOTED(PACKAGE_LOCALSTATE_DIR,"${ac_default_prefix}/var/lib/${PACKAGE}",[various directories])
  else
    AC_DEFINE_UNQUOTED(PACKAGE_LOCALSTATE_DIR,"${prefix}/var/lib/${PACKAGE}",[various directories])
  fi
else
  AC_DEFINE_UNQUOTED(PACKAGE_LOCALSTATE_DIR,"${localstatedir}/lib/${PACKAGE}")
fi

dnl Set PACKAGE_SYSCONF_DIR in config.h.
if test "x${sysconfdir}" = 'x${prefix}/etc'; then
  if test "x${prefix}" = "xNONE"; then
    AC_DEFINE_UNQUOTED(PACKAGE_SYSCONF_DIR, "${ac_default_prefix}/etc/vfrnav", [Package System Configuration Directory])
  else
    AC_DEFINE_UNQUOTED(PACKAGE_SYSCONF_DIR, "${prefix}/etc/vfrnav")
  fi
else
  AC_DEFINE_UNQUOTED(PACKAGE_SYSCONF_DIR, "${sysconfdir}/vfrnav")
fi

dnl Set PACKAGE_PREFIX_DIR in config.h.
if test "x${prefix}" = "xNONE"; then
  AC_DEFINE_UNQUOTED(PACKAGE_PREFIX_DIR,"${ac_default_prefix}",[various directories])
else
  AC_DEFINE_UNQUOTED(PACKAGE_PREFIX_DIR,"${prefix}")
fi

dnl Set PACKAGE_RUN_DIR in config.h.
if test "x${prefix}" = "xNONE"; then
  AC_DEFINE_UNQUOTED(PACKAGE_RUN_DIR,"${ac_default_prefix}/run/${PACKAGE}",[various directories])
elif test "x${prefix}" = "x/usr"; then
  AC_DEFINE_UNQUOTED(PACKAGE_RUN_DIR,"/run/${PACKAGE}",[various directories])
else
  AC_DEFINE_UNQUOTED(PACKAGE_RUN_DIR,"${prefix}/run/${PACKAGE}")
fi

dnl Set PACKAGE_CACHE_DIR in config.h.
if test "x${prefix}" = "xNONE"; then
  AC_DEFINE_UNQUOTED(PACKAGE_CACHE_DIR,"${ac_default_prefix}/var/cache/${PACKAGE}",[various directories])
elif test "x${prefix}" = "x/usr"; then
  AC_DEFINE_UNQUOTED(PACKAGE_CACHE_DIR,"/var/cache/${PACKAGE}",[various directories])
else
  AC_DEFINE_UNQUOTED(PACKAGE_CACHE_DIR,"${prefix}/var/cache/${PACKAGE}")
fi

dnl Set PACKAGE_HTTPDCACHE_DIR in config.h.
if test "x${prefix}" = "xNONE"; then
  AC_DEFINE_UNQUOTED(PACKAGE_HTTPDCACHE_DIR,"${ac_default_prefix}/var/cache/httpd/${PACKAGE}",[various directories])
elif test "x${prefix}" = "x/usr"; then
  AC_DEFINE_UNQUOTED(PACKAGE_HTTPDCACHE_DIR,"/var/cache/httpd/${PACKAGE}",[various directories])
else
  AC_DEFINE_UNQUOTED(PACKAGE_HTTPDCACHE_DIR,"${prefix}/var/cache/httpd/${PACKAGE}")
fi

if test "x${have_pilotlink}" = "xyes"; then
   AC_DEFINE(HAVE_PILOTLINK,[],[PilotLink library])
fi

if test "x${have_pilotlink}" = "xyes"; then
   AC_DEFINE(HAVE_PILOTLINK,[],[PilotLink library])
fi

AM_CONDITIONAL(HAVE_PILOTLINK, test x$have_pilotlink = xyes)

AC_SUBST(GPS_LIBS)
AC_SUBST(BOOST_LIBS)
AC_SUBST(X11_LIBS)
AC_SUBST(PODOFO_LIBS)

AM_CONDITIONAL(HAVE_GDAL, test x$have_gdal = xyes)

if test "x${have_gdal}" = "xyes"; then
   AC_DEFINE(HAVE_GDAL,[],[GDAL library])
fi

AM_CONDITIONAL(HAVE_GDAL2, test x$have_gdal2 = xyes)

if test "x${have_gdal2}" = "xyes"; then
   AC_DEFINE(HAVE_GDAL2,[],[GDAL v2 library])
fi

AM_CONDITIONAL(HAVE_BOOST, test x$have_boost = xyes)

if test "x${have_giomm}" = "xyes"; then
   AC_DEFINE(HAVE_GIOMM,[],[Giomm library])
fi

AM_CONDITIONAL(HAVE_GIOMM, test x$have_giomm = xyes)

if test "x${have_bluez}" = "xyes"; then
   AC_DEFINE(HAVE_BLUEZ,[],[BlueZ library])
fi

AM_CONDITIONAL(HAVE_BLUEZ, test x$have_bluez = xyes)

if test "x${have_eigen3}" = "xyes"; then
   AC_DEFINE(HAVE_EIGEN3,[],[Eigen3 library])
fi

AM_CONDITIONAL(HAVE_EIGEN3, test x$have_eigen3 = xyes)

if test "x${have_udev}" = "xyes"; then
   AC_DEFINE(HAVE_UDEV,[],[Udev library])
fi

AM_CONDITIONAL(HAVE_UDEV, test x$have_udev = xyes)

if test "x${have_hidapi}" = "xyes"; then
   AC_DEFINE(HAVE_HIDAPI,[],[Hidapi library])
fi

AM_CONDITIONAL(HAVE_HIDAPI, test x$have_hidapi = xyes)

if test "x${have_gsl}" = "xyes"; then
   AC_DEFINE(HAVE_GSL,[],[GSL library])
fi

AM_CONDITIONAL(HAVE_GSL, test x$have_gsl = xyes)

if test "x${have_libusb1}" = "xyes"; then
   AC_DEFINE(HAVE_LIBUSB1,[],[libusb1 library])
fi

AM_CONDITIONAL(HAVE_LIBUSB1, test x$have_libusb1 = xyes)

if test "x${have_ftdi}" = "xlibftdi" -o "x${have_ftdi}" = "xlibftdi1" -o "x${have_ftdi}" = "xd2xx"; then
   AC_DEFINE(HAVE_FTDI,[],[either libftdi or d2xx FTDI chip access library])
fi

if test "x${have_ftdi}" = "xlibftdi"; then
   AC_DEFINE(HAVE_LIBFTDI, [], [whether we have libftdi])
fi

if test "x${have_ftdi}" = "xlibftdi1"; then
   AC_DEFINE(HAVE_LIBFTDI1, [], [whether we have libftdi1])
fi

if test "x${have_ftdi}" = "xd2xx"; then
   AC_DEFINE(HAVE_FTDID2XX, [], [whether we have FTDI D2XX Library])
fi

AM_CONDITIONAL(HAVE_FTDI, test x$have_ftdi = xlibftdi -o x$have_ftdi = xlibftdi1 -o x$have_ftdi = xd2xx)

if test "x${have_poppler}" = "xyes"; then
   AC_DEFINE(HAVE_POPPLER,[],[poppler library])
fi

AM_CONDITIONAL(HAVE_POPPLER, test x$have_poppler = xyes)

if test "x${have_evince}" = "xyes"; then
   AC_DEFINE(HAVE_EVINCE,[],[evince library])
fi

AM_CONDITIONAL(HAVE_EVINCE, test x$have_evince = xyes)

if test "x${have_gtkmm2}" = "xyes"; then
   AC_DEFINE(HAVE_GTKMM2,[1],[Define if GTKMM 2.4 is available])
fi

AM_CONDITIONAL(HAVE_GTKMM2, test x$have_gtkmm2 = xyes)

if test "x${have_gtkmm3}" = "xyes"; then
   AC_DEFINE(HAVE_GTKMM3,[1],[Define if GTKMM 3.0 is available])
fi

AM_CONDITIONAL(HAVE_GTKMM3, test x$have_gtkmm3 = xyes)

if test "x${with_flightdeck}" != "xno" ; then
   AC_DEFINE(HAVE_FLIGHTDECK,[1],[Define if Flightdeck should be compiled])
fi

AM_CONDITIONAL(HAVE_FLIGHTDECK, test "x${with_flightdeck}" != "xno")

if test "x${with_cfmu}" != "xno" ; then
   AC_DEFINE(HAVE_CFMU,[1],[Define if Cfmu should be compiled])
fi

AM_CONDITIONAL(HAVE_CFMU, test "x${with_cfmu}" != "xno")

AM_CONDITIONAL(HAVE_DOCS, test "x${with_docs}" != "xno")

if test "x${have_curl}" = "xyes"; then
   AC_DEFINE(HAVE_CURL,[],[curl library])
fi

AM_CONDITIONAL(HAVE_CURL, test x$have_curl = xyes)

if test "x${have_openjpeg}" = "xyes"; then
   AC_DEFINE(HAVE_OPENJPEG,[],[openjpeg library])
fi

AM_CONDITIONAL(HAVE_OPENJPEG, test x$have_openjpeg = xyes)

if test "x${have_gconf}" = "xyes"; then
   AC_DEFINE(HAVE_GCONF,[],[gconf library])
fi

AM_CONDITIONAL(HAVE_GCONF, test x$have_gconf = xyes)

if test "x${have_qrencode}" = "xyes"; then
   AC_DEFINE(HAVE_QRENCODE,[],[qrencode library])
fi

AM_CONDITIONAL(HAVE_QRENCODE, test x$have_qrencode = xyes)

if test "x${have_libarchive}" = "xyes"; then
   AC_DEFINE(HAVE_LIBARCHIVE,[],[libarchive library])
fi

AM_CONDITIONAL(HAVE_LIBARCHIVE, test x$have_libarchive = xyes)

if test "x${have_webkitgtk}" = "xyes"; then
   AC_DEFINE(HAVE_WEBKITGTK,[],[webkitgtk library])
fi

AM_CONDITIONAL(HAVE_WEBKITGTK, test x$have_webkitgtk = xyes)

if test "x${have_webkitgtk4}" = "xyes"; then
   AC_DEFINE(HAVE_WEBKITGTK4,[],[webkitgtk4 library])
fi

AM_CONDITIONAL(HAVE_WEBKITGTK4, test x$have_webkitgtk4 = xyes)

if test "x${have_webkitgtkwebext}" = "xyes"; then
   AC_DEFINE(HAVE_WEBKITGTKWEBEXT,[],[webkitgtk web extension library])
fi

AM_CONDITIONAL(HAVE_WEBKITGTKWEBEXT, test x$have_webkitgtkwebext = xyes)

if test "x${with_mtmaps}" = "xyes" ; then
   AC_DEFINE(HAVE_MULTITHREADED_MAPS,[1],[Define if maps should be multithreaded])
fi

AM_CONDITIONAL(HAVE_MULTITHREADED_MAPS, test "x${with_mtmaps}" != "xno")

AM_CONDITIONAL(HAVE_FORK, test "x${ac_cv_func_fork_works}" = "xyes")

AM_CONDITIONAL(HAVE_PODOFO, test "x${have_podofo}" = "xyes")

if test "x${build_cpu}" = "xx86_64"; then
   AC_DEFINE(HAVE_SIMD,[],[SIMD instruction set potentially available])
fi

AM_CONDITIONAL(BUILD_SIMD, test "x${build_cpu}" = "xx86_64")

if test "x${build_cpu}" = "xx86_64"; then
   AC_DEFINE(HAVE_SIMD_X64,[],[x64 AVX instruction set potentially available])
fi

AM_CONDITIONAL(BUILD_SIMD_X64, test "x${build_cpu}" = "xx86_64")

if test x$cross_compiling = xyes; then
   AC_CHECK_PROGS(BUILD_CC, gcc cc)
   AC_CHECK_PROGS(BUILD_CXX, g++ c++)
else
   BUILD_CC=${CC}
   AC_SUBST(BUILD_CC)
   BUILD_CXX=${CXX}
   AC_SUBST(BUILD_CXX)
fi

echo "checking whether debugging should be enabled... ${with_debug}"

if test "x${prefix}" = "xNONE"; then
   systemddir="${ac_default_prefix}/lib/systemd/system"
   sysconfigdir="${ac_default_prefix}/etc/sysconfig"
elif test "x${prefix}" = "x/usr"; then
   systemddir="/lib/systemd/system"
   sysconfigdir="/etc/sysconfig"
else
   systemddir="${prefix}/lib/systemd/system"
   sysconfigdir="${prefix}/etc/sysconfig"
fi
sysconfvfrnavdir=${sysconfdir}/vfrnav

# Define as variables in Makefiles
AC_SUBST(desktopentrydir)
AC_SUBST(serviceentrydir)
AC_SUBST(localedir)
AC_SUBST(pixmapdir)
AC_SUBST(icon_26x26dir)
AC_SUBST(icon_34x34dir)
AC_SUBST(icon_40x40dir)
AC_SUBST(icon_50x50dir)
AC_SUBST(icon_scalabledir)
AC_SUBST(pluginlibdir)
AC_SUBST(plugindesktopentrydir)
AC_SUBST(helpfile_engbdir)
AC_SUBST(systemddir)
AC_SUBST(sysconfigdir)
AC_SUBST(sysconfvfrnavdir)

AM_CONDITIONAL(WIN32, test x$CYGWIN = xyes -o x$MINGW32 = xyes)
AM_CONDITIONAL(CROSSCOMP, test x$cross_compiling = xyes)

AC_OUTPUT([vfrnav.spec
Makefile
po/Makefile.in
ui/Makefile
utils/Makefile
utils/data/Makefile
src/Makefile
dbeditor/Makefile
acftperf/Makefile
flightdeck/Makefile
flightdeck/ui/Makefile
flightdeck/configs/Makefile
kingsim/Makefile
cfmu/Makefile
cfmu/ui/Makefile
cfmu/html/Makefile
cfmu/php/Makefile
cfmu/systemd/Makefile
cfmu/webext/Makefile
debian/Makefile
data/Makefile
doc/Makefile
windows/mingw-vfrnav.spec])
