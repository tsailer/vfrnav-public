#!/bin/sh

echo "Converting SRTM30"
inst/bin/vfrdbsrtm30db -g /usr/local/download/srtm/version2/SRTM30 
mkdir 1
cp -f topo30.db 1
mkdir swbd
cd swbd
for i in /usr/local/download/srtm/version2/SWBD/SWBDeast/*.zip /usr/local/download/srtm/version2/SWBD/SWBDwest/*.zip; do unzip $i; done
cd ..
inst/bin/vfrdbsrtmwatermask -s swbd
mkdir 2
cp -f topo30.db 2
inst/bin/vfrdbsettopo30 --min-lat 36 --max-lat 37 --min-lon -1 --max-lon 0 --ocean -o .
inst/bin/vfrdbsettopo30 --min-lat 37 --max-lat 38 --min-lon 0 --max-lon 6 --ocean -o .
inst/bin/vfrdbsettopo30 --min-lat 38 --max-lat 39 --min-lon 2 --max-lon 8 --ocean -o .
inst/bin/vfrdbsettopo30 --min-lat 38 --min-lon 10 --max-lat 41 --max-lon 12 --ocean -o .
inst/bin/vfrdbsettopo30 --min-lat 33 --min-lon 13 --max-lat 37 --max-lon 14 --ocean -o .
inst/bin/vfrdbsettopo30 --min-lat 33 --min-lon 12 --max-lat 35 --max-lon 23 --ocean -o .
inst/bin/vfrdbsettopo30 --min-lat 32 --min-lon 16 --max-lat 33 --max-lon 19 --ocean -o .
inst/bin/vfrdbsettopo30 --min-lat 31 --min-lon 18 --max-lat 32 --max-lon 19 --ocean -o .
inst/bin/vfrdbsettopo30 --no-vacuum --no-analyze --min-lat 33 --min-lon 23 --max-lat 34 --max-lon 35 --ocean -o .
inst/bin/vfrdbsettopo30 --no-vacuum --no-analyze --min-lat 32 --min-lon 25 --max-lat 33 --max-lon 34 --ocean -o .
inst/bin/vfrdbsettopo30 --no-vacuum --no-analyze --min-lat 34 --min-lon 27 --max-lat 35 --max-lon 32 --ocean -o .
inst/bin/vfrdbsettopo30 --no-vacuum --no-analyze --min-lat 35 --min-lon 28 --max-lat 36 --max-lon 32 --ocean -o .
inst/bin/vfrdbsettopo30 --no-vacuum --no-analyze --min-lat 34 --min-lon -12 --max-lat 36 --max-lon -7 --ocean -o .
inst/bin/vfrdbsettopo30 --no-vacuum --no-analyze --min-lat 33 --min-lon -16 --max-lat 34 --max-lon -9 --ocean -o .
inst/bin/vfrdbsettopo30 --no-vacuum --no-analyze --min-lat 30 --min-lon -15 --max-lat 33 --max-lon -10 --ocean -o .
inst/bin/vfrdbsettopo30 --no-vacuum --no-analyze --min-lat 29 --min-lon -72 --max-lat 30 --max-lon -14 --ocean -o .
inst/bin/vfrdbsettopo30 --no-vacuum --no-analyze --min-lat 29 --min-lon -13 --max-lat 30 --max-lon -11 --ocean -o .
inst/bin/vfrdbsettopo30 --no-vacuum --no-analyze --min-lat 25 --min-lon -68 --max-lat 27 --max-lon -15 --ocean -o .
inst/bin/vfrdbsettopo30 --no-vacuum --no-analyze --min-lat 36 --min-lon -12 --max-lat 38 --max-lon -9 --ocean -o .
inst/bin/vfrdbsettopo30 --no-vacuum --no-analyze --min-lat 38 --min-lon -20 --max-lat 40 --max-lon -10 --ocean -o .
inst/bin/vfrdbsettopo30 --no-vacuum --no-analyze --min-lat 40 --min-lon -12 --max-lat 42 --max-lon -9 --ocean -o .
inst/bin/vfrdbsettopo30 --no-vacuum --no-analyze --min-lat 42 --min-lon -59 --max-lat 44 --max-lon -10 --ocean -o .
inst/bin/vfrdbsettopo30 --no-vacuum --no-analyze --min-lat 44 --min-lon -12 --max-lat 46 --max-lon -2 --ocean -o .
inst/bin/vfrdbsettopo30 --no-vacuum --no-analyze --min-lat 46 --min-lon -52 --max-lat 47 --max-lon -3 --ocean -o .
inst/bin/vfrdbsettopo30 --no-vacuum --no-analyze --min-lat 47 --min-lon -8 --max-lat 48 --max-lon -5 --ocean -o .
inst/bin/vfrdbsettopo30 --no-vacuum --no-analyze --min-lat 48 --min-lon -8 --max-lat 49 --max-lon -6 --ocean -o .
inst/bin/vfrdbsettopo30 --no-vacuum --no-analyze --min-lat 49 --min-lon -8 --max-lat 50 --max-lon -7 --ocean -o .
inst/bin/vfrdbsettopo30 --no-vacuum --no-analyze --min-lat 49 --min-lon -5 --max-lat 50 --max-lon -3 --ocean -o .
inst/bin/vfrdbsettopo30 --no-vacuum --no-analyze --min-lat 50 --min-lon -55 --max-lat 51 --max-lon -6 --ocean -o .
inst/bin/vfrdbsettopo30 --no-vacuum --no-analyze --min-lat 49 --min-lon -53 --max-lat 50 --max-lon -52 --ocean -o .
inst/bin/vfrdbsettopo30 --no-vacuum --no-analyze --min-lat 51 --min-lon -55 --max-lat 54 --max-lon -44 --ocean -o .
inst/bin/vfrdbsettopo30 --no-vacuum --no-analyze --min-lat 51 --min-lon -7 --max-lat 52 --max-lon -6 --ocean -o .

