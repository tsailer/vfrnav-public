gtkmm2ui = flightdeck.glade

gtkmm3ui = flightdeck.ui

gtkmm3theme = flightdeck.css

gtk3themedir = $(pkgdatadir)/themes/gtk-3.0

if HAVE_GTKMM2
gtkmmui = $(gtkmm2ui)
gtkmmtheme =
endif

if HAVE_GTKMM3
gtkmmui = $(gtkmm3ui)
gtkmmtheme = $(gtkmm3theme)
endif

pkgdata_DATA = $(gtkmmui)
gtk3theme_DATA = $(gtkmmtheme)

EXTRA_DIST = flightdeck.gladep $(gtkmm2ui) $(gtkmm3ui) $(gtkmm3theme)
