bin_PROGRAMS = vfrnavaideditor vfrwaypointeditor vfrairporteditor vfrairspaceeditor vfrtrackeditor vfrairwayeditor

noinst_HEADERS =  dbeditor.h

noinst_LIBRARIES = libdbeditor.a

libdbeditor_a_SOURCES = dbeditor.cc
libdbeditor_a_LIBADD = $(top_builddir)/src/RouteEditUi.o

vfrnavaideditor_SOURCES = navaideditor.cc
vfrnavaideditor_LDADD = libdbeditor.a $(top_builddir)/src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ \
        @SQLITE3X_LIBS@ @GTHREAD_LIBS@ @DBUS_LIBS@ @HILDONMM_LIBS@ @GNOMEBT_LIBS@ @PILOTLINK_LIBS@ \
	@GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@ @LIBSOUP_LIBS@ @LIBXMLPP_LIBS@
vfrnavaideditor_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrwaypointeditor_SOURCES = waypointeditor.cc
vfrwaypointeditor_LDADD = libdbeditor.a $(top_builddir)/src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ \
        @SQLITE3X_LIBS@ @GTHREAD_LIBS@ @DBUS_LIBS@ @HILDONMM_LIBS@ @GNOMEBT_LIBS@ @PILOTLINK_LIBS@ \
	@GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@ @LIBSOUP_LIBS@ @LIBXMLPP_LIBS@
vfrwaypointeditor_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrairporteditor_SOURCES = airporteditor.cc
vfrairporteditor_LDADD = libdbeditor.a $(top_builddir)/src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ \
        @SQLITE3X_LIBS@ @GTHREAD_LIBS@ @DBUS_LIBS@ @HILDONMM_LIBS@ @GNOMEBT_LIBS@ @PILOTLINK_LIBS@ \
	@GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@ @LIBSOUP_LIBS@ @LIBXMLPP_LIBS@
vfrairporteditor_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrairspaceeditor_SOURCES = airspaceeditor.cc
vfrairspaceeditor_LDADD = libdbeditor.a $(top_builddir)/src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ \
        @SQLITE3X_LIBS@ @GTHREAD_LIBS@ @DBUS_LIBS@ @HILDONMM_LIBS@ @GNOMEBT_LIBS@ @PILOTLINK_LIBS@ \
	@GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@ @LIBSOUP_LIBS@ @LIBXMLPP_LIBS@
vfrairspaceeditor_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrtrackeditor_SOURCES = trackeditor.cc
vfrtrackeditor_LDADD = libdbeditor.a $(top_builddir)/src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ \
        @SQLITE3X_LIBS@ @GTHREAD_LIBS@ @DBUS_LIBS@ @HILDONMM_LIBS@ @GNOMEBT_LIBS@ @PILOTLINK_LIBS@ \
	@LIBXMLPP_LIBS@ @GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@ @LIBSOUP_LIBS@ @LIBXMLPP_LIBS@
vfrtrackeditor_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

vfrairwayeditor_SOURCES = airwayeditor.cc
vfrairwayeditor_LDADD = libdbeditor.a $(top_builddir)/src/libvfrnav.la @LIBS@ @GTKMM_LIBS@ \
        @SQLITE3X_LIBS@ @GTHREAD_LIBS@ @DBUS_LIBS@ @HILDONMM_LIBS@ @GNOMEBT_LIBS@ @PILOTLINK_LIBS@ \
	@GEOS_LIBS@ @CLIPPER_LIBS@ @OPENJPEG_LIBS@ @LIBSOUP_LIBS@ @LIBXMLPP_LIBS@
vfrairwayeditor_LDFLAGS = @GEOS_LDFLAGS@ @CLIPPER_LDFLAGS@

AM_CXXFLAGS = @CXXFLAGS@ @GTKMM_CFLAGS@ @LIBXMLPP_CFLAGS@ @SQLITE_CFLAGS@ @SQLITE3X_CFLAGS@ \
	@DBUS_CFLAGS@ @HILDONMM_CFLAGS@ @GNOMEBT_CFLAGS@ @PILOTLINK_CFLAGS@ @GEOS_CPPFLAGS@ \
	@CLIPPER_CPPFLAGS@ @LIBSOUP_CFLAGS@ @LIBXMLPP_CFLAGS@

AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src
