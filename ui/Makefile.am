gtkmm2ui = routeedit.glade navigate.glade dbeditor.glade \
	prefs.glade acftperformance.glade

gtkmm3ui = routeedit.ui navigate.ui dbeditor.ui \
	prefs.ui acftperformance.ui

if HAVE_GTKMM2
gtkmmui = $(gtkmm2ui)
endif

if HAVE_GTKMM3
gtkmmui = $(gtkmm3ui)
endif

dist_pkgdata_DATA = BlankMap-World_gray.svg vfrnav.png bluetooth.png

pkgdata_DATA = $(gtkmmui)

EXTRA_DIST = routeedit.gladep navigate.gladep dbeditor.gladep \
	prefs.gladep acftperformance.gladep \
	vfrnav.ico $(gtkmm2ui) $(gtkmm3ui)
