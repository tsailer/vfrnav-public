/*
 * rebuildspatialindex.cc: Spatial Index rebuilding utility
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
#include <sys/types.h>
#include <errno.h>
#include <stdexcept>
#include <stdarg.h>
#include <dirent.h>

#include <map>
#include <vector>
#include <limits>

#include "geom.h"
#include "dbobj.h"

/* ---------------------------------------------------------------------- */

static void rebuild_navaid_tileindex(const Glib::ustring& output_dir)
{
        NavaidsDb db;
        db.open(output_dir);
        db.sync_off();
        // cannot use foreach, we need a transaction, and sqlite3 does not support nested transactions
        unsigned int count = 0;
        NavaidsDb::element_t e;
        db.loadfirst(e, false, false);
        while (e.is_valid()) {
                db.update_index(e);
                count++;
                //std::cerr << "Row " << count << " name " << e.get_name() << std::endl;
                if (!(count & 127))
                        std::cerr << "Navaid Row " << count << std::endl;
                db.loadnext(e, false, false);
        }
        db.analyze();
        db.vacuum();
        db.close();
}

static void rebuild_waypoint_tileindex(const Glib::ustring& output_dir)
{
        WaypointsDb db;
        db.open(output_dir);
        db.sync_off();
        // cannot use foreach, we need a transaction, and sqlite3 does not support nested transactions
        unsigned int count = 0;
        WaypointsDb::element_t e;
        db.loadfirst(e, false, false);
        while (e.is_valid()) {
                db.update_index(e);
                count++;
                //std::cerr << "Row " << count << " name " << e.get_name() << std::endl;
                if (!(count & 127))
                        std::cerr << "Waypoint Row " << count << std::endl;
                db.loadnext(e, false, false);
        }
        db.analyze();
        db.vacuum();
        db.close();
}

static void rebuild_airport_tileindex(const Glib::ustring& output_dir)
{
        AirportsDb db;
        db.open(output_dir);
        db.sync_off();
        // cannot use foreach, we need a transaction, and sqlite3 does not support nested transactions
        unsigned int count = 0;
        AirportsDb::element_t e;
        db.loadfirst(e, false, true);
        while (e.is_valid()) {
                bool save = false;
#if 1
                //std::cerr << "Row " << count << " record " << e.get_address() << " #runways " << e.get_nrrwy() << std::endl;
                for (unsigned int i = 0; i < e.get_nrrwy(); ++i) {
                        AirportsDb::element_t::Runway& rwy(e.get_rwy(i));
                        if (!rwy.get_he_hdg() && !rwy.get_le_hdg()) {
                                rwy.compute_default_hdg();
                                save = true;
                                std::cerr << "Airport record " << e.get_address() << " setting default headings" << std::endl;
                        }
                        if (!rwy.get_he_elev() && !rwy.get_le_elev() && abs(e.get_elevation()) > 500) {
                                rwy.set_he_elev(e.get_elevation());
                                rwy.set_le_elev(e.get_elevation());
                                save = true;
                                std::cerr << "Airport record " << e.get_address() << " setting elevations" << std::endl;
                        }
                        if ((rwy.get_he_coord() == Point(0, 0) && rwy.get_le_coord() == Point(0, 0) && rwy.get_he_coord().simple_distance_nmi(e.get_coord()) > 10.0) ||
                            (rwy.get_he_coord().simple_distance_nmi(rwy.get_le_coord()) < 0.01 && rwy.get_length() > 60)) {
                                rwy.compute_default_coord(e.get_coord());
                                save = true;
                                std::cerr << "Airport record " << e.get_address() << " setting coordinates" << std::endl;
                        }
                        if (rwy.normalize_he_le()) {
                                save = true;
                                std::cerr << "Airport record " << e.get_address() << " reversing runway" << std::endl;
                        }
                }
#endif
                Rect bbox(e.get_bbox());
                e.recompute_bbox();
                save |= bbox != e.get_bbox();
                db.update_index(e);
                if (save)
                        db.save(e);
                count++;
                //std::cerr << "Row " << count << " name " << e.get_name() << std::endl;
                if (!(count & 127))
                        std::cerr << "Airport Row " << count << std::endl;
                db.loadnext(e, false, true);
        }
        db.analyze();
        db.vacuum();
        db.close();
}

static void rebuild_airspace_tileindex(const Glib::ustring& output_dir)
{
        AirspacesDb db;
        db.open(output_dir);
        db.sync_off();
        // cannot use foreach, we need a transaction, and sqlite3 does not support nested transactions
        unsigned int count = 0;
        AirspacesDb::element_t e;
        db.loadfirst(e, false, true);
        while (e.is_valid()) {
                bool save = false;
                Rect bbox(e.get_bbox());
                e.recompute_bbox();
                save |= bbox != e.get_bbox();
                db.update_index(e);
                if (save)
                        db.save(e);
                count++;
                //std::cerr << "Row " << count << " name " << e.get_name() << std::endl;
                if (!(count & 127))
                        std::cerr << "Airspace Row " << count << std::endl;
                db.loadnext(e, false, true);
        }
        db.analyze();
        db.vacuum();
        db.close();
}

static void rebuild_mapelement_tileindex(const Glib::ustring& output_dir)
{
        MapelementsDb db;
        db.open(output_dir);
        db.sync_off();
        // cannot use foreach, we need a transaction, and sqlite3 does not support nested transactions
        unsigned int count = 0;
        MapelementsDb::element_t e;
        db.loadfirst(e, false, true);
        while (e.is_valid()) {
                db.update_index(e);
                count++;
                //std::cerr << "Row " << count << " name " << e.get_name() << std::endl;
                if (!(count & 127))
                        std::cerr << "Mapelement Row " << count << std::endl;
                db.loadnext(e, false, true);
        }
        db.analyze();
        db.vacuum();
        db.close();
}

static void rebuild_track_tileindex(const Glib::ustring& output_dir)
{
        TracksDb db;
        db.open(output_dir);
        db.sync_off();
        // cannot use foreach, we need a transaction, and sqlite3 does not support nested transactions
        unsigned int count = 0;
        TracksDb::element_t e;
        db.loadfirst(e, false, true);
        while (e.is_valid()) {
                db.update_index(e);
                count++;
                //std::cerr << "Row " << count << " name " << e.get_name() << std::endl;
                if (!(count & 127))
                        std::cerr << "Track Row " << count << std::endl;
                db.loadnext(e, false, true);
        }
        db.analyze();
        db.vacuum();
        db.close();
}

/* ---------------------------------------------------------------------- */

static std::string downcase(const std::string& x)
{
        std::string r(x);
        std::string::iterator i1(r.begin()), i2(r.end());
        for (; i1 != i2; i1++)
                *i1 = tolower(*i1);
        return r;
}

static bool boolflag(bool& flg, const char *arg)
{
        if (!arg) {
                flg = !flg;
                return flg;
        }
        std::string a = arg;
        downcase(a);
        if (a == "true" || a == "1" || a == "yes")
                flg = true;
        else if (a == "false" || a == "0" || a == "no")
                flg = false;
        else
                flg = !flg;
        return flg;
}

/* ---------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
        static struct option long_options[] = {
                { "help",               no_argument,       NULL, 'h' },
                { "version",            no_argument,       NULL, 'v' },
                { "quiet",              no_argument,       NULL, 'q' },
                { "output-dir",         required_argument, NULL, 'o' },
                { "airports",           optional_argument, NULL, 0x400 },
                { "airspaces",          optional_argument, NULL, 0x401 },
                { "navaids",            optional_argument, NULL, 0x402 },
                { "waypoints",          optional_argument, NULL, 0x403 },
                { "mapelements",        optional_argument, NULL, 0x404 },
                { "tracks",             optional_argument, NULL, 0x405 },
                { NULL,                 0,                 NULL, 0 }
        };
        int c, err(0);
        bool quiet(false), airports(false), airspaces(false), navaids(false), waypoints(false), mapelements(false), tracks(false);
        Glib::ustring output_dir(".");

        while (((c = getopt_long(argc, argv, "hvqo:", long_options, NULL)) != -1)) {
                switch (c) {
                        case 'v':
                                std::cout << argv[0] << ": (C) 2007, 2008 Thomas Sailer" << std::endl;
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

                        case 0x400:
                                boolflag(airports, optarg);
                                break;

                        case 0x401:
                                boolflag(airspaces, optarg);
                                break;

                        case 0x402:
                                boolflag(navaids, optarg);
                                break;

                        case 0x403:
                                boolflag(waypoints, optarg);
                                break;

                        case 0x404:
                                boolflag(mapelements, optarg);
                                break;

                        case 0x405:
                                boolflag(tracks, optarg);
                                break;
                }
        }
        if (err) {
                std::cerr << "usage: rebuildspatialindex [-q] [-o <dbdir>]" << std::endl
                          << "     -q                quiet" << std::endl
                          << "     -h, --help        Display this information" << std::endl
                          << "     -v, --version     Display version information" << std::endl
                          << "     -o, --output-dir  Database Output Directory" << std::endl << std::endl;
                return EX_USAGE;
        }
        if (!(airports || airspaces || navaids || waypoints || mapelements || tracks))
                airports = airspaces = navaids = waypoints = mapelements = tracks = true;
        try {
                if (navaids)
                        rebuild_navaid_tileindex(output_dir);
                if (waypoints)
                        rebuild_waypoint_tileindex(output_dir);
                if (airports)
                        rebuild_airport_tileindex(output_dir);
                if (airspaces)
                        rebuild_airspace_tileindex(output_dir);
                if (mapelements)
                        rebuild_mapelement_tileindex(output_dir);
                if (tracks)
                        rebuild_track_tileindex(output_dir);
        } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                return EX_DATAERR;
        } catch (const Glib::Exception& e) {
                std::cerr << "Glib Error: " << e.what() << std::endl;
                return EX_DATAERR;
        }
        return 0;
}
