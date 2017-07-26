gtkmm2ui = cfmuvalidate.glade

gtkmm3ui = cfmuvalidate.ui

if HAVE_GTKMM2
gtkmmui = $(gtkmm2ui)
endif

if HAVE_GTKMM3
gtkmmui = $(gtkmm3ui)
endif

pkgdata_DATA = $(gtkmmui)

EXTRA_DIST = cfmuvalidate.gladep $(gtkmm2ui) $(gtkmm3ui)
