/*
 * settopo30.cc: "Primitive" TopoDb30 editing facility
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include "sysdeps.h"

#include <unistd.h>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <sqlite3x.hpp>
#include <sqlite3.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdexcept>
#include <stdarg.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <stdarg.h>
#include <dirent.h>
#include <ctype.h>
#include <limits.h>

#include <map>
#include <vector>
#include <limits>

#include <zlib.h>
#include <zfstream.hpp>

#include "geom.h"
#include "dbobj.h"

/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
        static struct option long_options[] = {
                { "help",               no_argument,       NULL, 'h' },
                { "version",            no_argument,       NULL, 'v' },
                { "quiet",              no_argument,       NULL, 'q' },
                { "output-dir",         required_argument, NULL, 'o' },
                { "min-lat",            required_argument, NULL, 0x400 },
                { "max-lat",            required_argument, NULL, 0x401 },
                { "min-lon",            required_argument, NULL, 0x402 },
                { "max-lon",            required_argument, NULL, 0x403 },
                { "nodata",             no_argument,       NULL, 'n' },
                { "ocean",              no_argument,       NULL, 'O' },
                { "elevation",          required_argument, NULL, 'e' },
                { "no-vacuum",          no_argument,       NULL, 0x500 },
                { "vacuum",             no_argument,       NULL, 0x501 },
                { "no-analyze",         no_argument,       NULL, 0x502 },
                { "analyze",            no_argument,       NULL, 0x503 },
                { NULL,                 0,                 NULL, 0 }
        };
        int c, err(0);
        bool quiet(false), dovacuum(true), doanalyze(true);
        TopoDb30::elev_t elev(TopoDb30::nodata);
        Glib::ustring output_dir(".");
        Rect bbox(Point(Point::lon_min, Point::lat_min), Point(Point::lon_max, Point::lat_max));

        while (((c = getopt_long(argc, argv, "hvqo:e:nO", long_options, NULL)) != -1)) {
                switch (c) {
                        case 'v':
                                std::cout << argv[0] << ": (C) 2007 Thomas Sailer" << std::endl;
                                return 0;

                        case 'o':
                                output_dir = optarg;
                                break;

                        case 'q':
                                quiet = true;
                                break;

                        case 'n':
                                elev = TopoDb30::nodata;
                                break;

                        case 'O':
                                elev = TopoDb30::ocean;
                                break;

                        case 'e':
                                elev = strtol(optarg, NULL, 0);
                                break;

                        case 0x400:
                        {
                                Point sw(bbox.get_southwest());
                                Point ne(bbox.get_northeast());
                                if (!sw.set_lat_str(optarg))
                                        sw.set_lat_deg(strtod(optarg, NULL));
                                bbox = Rect(sw, ne);
                                break;
                        }

                        case 0x401:
                        {
                                Point sw(bbox.get_southwest());
                                Point ne(bbox.get_northeast());
                                if (!ne.set_lat_str(optarg))
                                        ne.set_lat_deg(strtod(optarg, NULL));
                                bbox = Rect(sw, ne);
                                break;
                        }

                        case 0x402:
                        {
                                Point sw(bbox.get_southwest());
                                Point ne(bbox.get_northeast());
                                if (!sw.set_lon_str(optarg))
                                        sw.set_lon_deg(strtod(optarg, NULL));
                                bbox = Rect(sw, ne);
                                break;
                        }

                        case 0x403:
                        {
                                Point sw(bbox.get_southwest());
                                Point ne(bbox.get_northeast());
                                if (!ne.set_lon_str(optarg))
                                        ne.set_lon_deg(strtod(optarg, NULL));
                                bbox = Rect(sw, ne);
                                break;
                        }

                        case 0x500:
                        case 0x501:
                                dovacuum = c & 1;
                                break;

                        case 0x502:
                        case 0x503:
                                doanalyze = c & 1;
                                break;
 
                        case 'h':
                        default:
                                err++;
                                break;
                }
        }
        if (err) {
                std::cerr << "usage: settopo30 [-q] [-o <dir>] [-n] [-o] [-e <elev>]" << std::endl
                          << "     -q                quiet" << std::endl
                          << "     -h, --help        Display this information" << std::endl
                          << "     -v, --version     Display version information" << std::endl
                          << "     -o, --output-dir  Database Output Directory" << std::endl
                          << "     -n, --nodata      Set rectangle to \"nodata\"" << std::endl
                          << "     -O, --ocean       Set rectangle to ocean" << std::endl
                          << "     -e, --elevation   Set rectangle to given numeric elevation (m)" << std::endl
                          << "     --min-lat         South border" << std::endl
                          << "     --max-lat         North border" << std::endl
                          << "     --min-lon         West border" << std::endl
                          << "     --max-lon         East border" << std::endl << std::endl;
                return EX_USAGE;
        }
        std::cout << "DB Boundary: " << (std::string)bbox.get_southwest().get_lat_str()
                  << ' ' << (std::string)bbox.get_southwest().get_lon_str()
                  << " -> " << (std::string)bbox.get_northeast().get_lat_str()
                  << ' ' << (std::string)bbox.get_northeast().get_lon_str()
                  << " elevation ";
        if (elev == TopoDb30::nodata)
                std::cout << "nodata";
        else if (elev == TopoDb30::ocean)
                std::cout << "ocean";
        else
                std::cout << elev;
        std::cout << std::endl;
        try {
                TopoDb30 db;
                db.open(output_dir);
                db.sync_off();

                TopoDb30::TopoCoordinate tcmin(bbox.get_southwest()), tcmax(bbox.get_northeast());
                for (TopoDb30::TopoCoordinate tc(tcmin);; ) {
                        std::cerr << "Point " << (Point)tc << " set to " << elev << " (previous " << db.get_elev(tc) << ")" << std::endl;
                        db.set_elev(tc, elev);
                        if (tc.get_lon() != tcmax.get_lon()) {
                                tc.advance_east();
                                continue;
                        }
                        if (tc.get_lat() == tcmax.get_lat())
                                break;
                        tc.set_lon(tcmin.get_lon());
                        tc.advance_north();
                }
                if (doanalyze)
                        db.analyze();
                if (dovacuum)
                        db.vacuum();
                db.close();
        } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                return EX_DATAERR;
        } catch (const Glib::Exception& e) {
                std::cerr << "Glib Error: " << e.what() << std::endl;
                return EX_DATAERR;
        }
        return 0;
}
