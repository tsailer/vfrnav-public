#!/bin/sh

echo "Converting DAFIF Epoch $1"
inst/bin/vfrdbdafif -d ~/datasheets/aviation/dafif_ed7/dafif_$1_ed7/Dafift/ -W /home/sailer/src/pilot_gps/conduit/WELT2000.TXT -a /home/sailer/src/pilot_gps/conduit/dbadd.xml
mkdir 1
cp -f airports.db airspaces.db navaids.db waypoints.db 1
if [ -f topo30.db ]; then
  inst/bin/vfrdbupdategndelev
  mkdir 2
  cp -f airports.db airspaces.db navaids.db waypoints.db 2
fi
inst/bin/vfrdboptimizelabelplacement
mkdir 3
cp -f airports.db airspaces.db navaids.db waypoints.db 1
