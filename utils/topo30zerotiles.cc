/*
 * topo30zerotiles.cc: Find all zero tiles (1 by 1 degree rectangles) in Topo30Db
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

#include <getopt.h>
#include <iostream>
#include <glibmm.h>

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
                { NULL,                 0,                 NULL, 0 }
        };
        int c, err(0);
        bool quiet(false);
        Glib::ustring output_dir(".");

        while (((c = getopt_long(argc, argv, "hvqo:", long_options, NULL)) != -1)) {
                switch (c) {
                        case 'v':
                                std::cout << argv[0] << ": (C) 2007, 2008, 2009 Thomas Sailer" << std::endl;
                                return 0;

                        case 'o':
                                output_dir = optarg;
                                break;

                        case 'q':
                                quiet = true;
                                break;

                        case 'h':
                        default:
                                err++;
                                break;
                }
        }
        if (err) {
                std::cerr << "usage: topo30zerotiles [-q] [-p port] [<file>]" << std::endl
                          << "     -q                quiet" << std::endl
                          << "     -h, --help        Display this information" << std::endl
                          << "     -v, --version     Display version information" << std::endl
                          << "     -o, --output-dir  Database Output Directory" << std::endl << std::endl;
                return EX_USAGE;
        }
        try {
                TopoDb30 db;
                db.open(output_dir);
                db.sync_off();

                for (int latdeg = -90; latdeg < 90; latdeg++) {
                        for (int londeg = -180; londeg < 180; londeg++) {
                                bool zeroseen(false), nonzero(false);
                                Point pmin(Point::round<Point::coord_t,float>(londeg * Point::from_deg),
                                           Point::round<Point::coord_t,float>(latdeg * Point::from_deg));
                                TopoDb30::TopoCoordinate tcmin(pmin),
                                                tcmax(pmin + Point(Point::round<Point::coord_t,float>(Point::from_deg),
                                                                   Point::round<Point::coord_t,float>(Point::from_deg)));
                                for (TopoDb30::TopoCoordinate tc(tcmin);; ) {
                                        TopoDb30::elev_t elev(db.get_elev(tc));
                                        if (!quiet)
                                                std::cerr << "Point " << (Point)tc << " elev " << elev << std::endl;
                                        if (elev == 0) {
                                                zeroseen = true;
                                        } else if (elev != TopoDb30::ocean) {
                                                nonzero = true;
                                                break;
                                        }
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
                                if (zeroseen && !nonzero)
                                        std::cout << londeg << ',' << latdeg << std::endl;
                        }
                }
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
