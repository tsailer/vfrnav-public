serviceentry_DATA = 

desktopentry_DATA = vfrnav.desktop vfrairporteditor.desktop vfrairspaceeditor.desktop \
	vfrnavaideditor.desktop vfrwaypointeditor.desktop vfrtrackeditor.desktop \
	acftperf.desktop flightdeck.desktop wetterdl.desktop

icon_26x26_DATA = icons/26x26/vfrnav.png icons/26x26/wetterdl.png
icon_40x40_DATA = icons/40x40/vfrnav.png icons/40x40/wetterdl.png
icon_scalable_DATA = #icons/scalable/vfrnav.png

helpfile_engb_DATA = 

navlogtemplatesdir=$(pkgdatadir)/navlogtemplates
navlogtemplates_DATA = navlogtemplates/navlog.ods

libreofficeregistrydir=$(libdir)/libreoffice/share/registry
libreofficeregistry_DATA=navlogtemplates/vfrnav.xcd

pkgdata_DATA = navlogtemplates/navlog.xml

EXTRA_DIST = $(serviceentry_DATA) $(desktopentry_DATA) $(icon_26x26_DATA) $(icon_40x40_DATA) \
	selinux/vfrnav.fc selinux/vfrnav.if selinux/vfrnav.te \
	$(icon_scalable_DATA) $(helpfile_engb_DATA) $(navlogtemplates_DATA) $(libreofficeregistry_DATA) $(pkgdata_DATA)
