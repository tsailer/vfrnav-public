SUBDIRS = po src utils dbeditor acftperf

if HAVE_CFMU
SUBDIRS += cfmu
endif

if HAVE_FLIGHTDECK
SUBDIRS += flightdeck kingsim
endif

SUBDIRS += ui data debian

if HAVE_DOCS
SUBDIRS += doc
endif

EXTRA_SUBDIRS = cfmu flightdeck kingsim doc

bin_SCRIPTS = scripts/wetterdl.py

EXTRA_DIST = m4/ChangeLog config.rpath AUTHORS TODO README configure \
  vfrnav.spec windows/mingw-vfrnav.spec.in windows/vfrnav.nsis \
  windows/vfrnav_aviation.nsis windows/vfrnav_terrain.nsis \
  scripts/convertdafif.sh scripts/convertgtopo.sh scripts/convertvmap.sh \
  scripts/copyrunwaydata.sql scripts/copytaxiwaydata.sql \
  scripts/creatertreeairports.sql scripts/creatertreeairspaces.sql \
  scripts/creatertreeairways.sql scripts/creatertreemapel.sql \
  scripts/duplicateairport.sql scripts/mapelementsreorder.sql \
  scripts/runwaydisplacement.sql scripts/unwrapnelon.sql \
  scripts/fetchgfs.pl scripts/imgcompr.php scripts/wetterdl.py \
  include/gettext.h include/sysdeps.h include/sysdepsgui.h

ACLOCAL_AMFLAGS = -I m4
