/*
 * updategndelev.cc:  Compute ground elevations from TopoDb data
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* AIX requires this to be the first thing in the file.  */
#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
#pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#endif

#include <getopt.h>
#include <iostream>
#include <limits>

#ifdef HAVE_SYSEXITS_H
#include <sysexits.h>
#else
#define EX_USAGE   64
#define EX_DATAERR 65
#define EX_OK      0
#endif

#include "geom.h"
#include "dbobj.h"

/* ---------------------------------------------------------------------- */

static bool update_airspaces(AirspacesDb::Airspace& e, TopoDb30& topodb, bool force)
{
        if (!force &&
	    e.get_gndelevmin() != AirspacesDb::Airspace::gndelev_unknown &&
            e.get_gndelevmax() != AirspacesDb::Airspace::gndelev_unknown)
                return false;
        TopoDb30::minmax_elev_t mme(topodb.get_minmax_elev(e.get_polygon()));
        if ((e.get_gndelevmin() == AirspacesDb::Airspace::gndelev_unknown || force) && mme.first != TopoDb30::nodata)
                e.set_gndelevmin(Point::round<AirspacesDb::Airspace::gndelev_t,float>(mme.first * Point::m_to_ft));
        if ((e.get_gndelevmax() == AirspacesDb::Airspace::gndelev_unknown || force) && mme.second != TopoDb30::nodata)
                e.set_gndelevmax(Point::round<AirspacesDb::Airspace::gndelev_t,float>(mme.second * Point::m_to_ft));
        return true;
}

/* ---------------------------------------------------------------------- */

static void process_airspaces(const Glib::ustring& output_dir, bool force)
{
        TopoDb30 topodb;
        topodb.open(output_dir);
        topodb.sync_off();
        AirspacesDb db;
        db.open(output_dir);
        db.sync_off();
        AirspacesDb::Airspace e;
        db.loadfirst(e);
        while (e.is_valid()) {
                if (update_airspaces(e, topodb, force)) {
                        std::cerr << "Saving Airspace " << e.get_icao() << ' ' << e.get_name() << ' ' << e.get_ident() << ' ' << e.get_sourceid()
                                  << " elev min: " << e.get_gndelevmin() << " max: " << e.get_gndelevmax() << std::endl;
                        db.save(e);
                }
                db.loadnext(e);
        }
        db.analyze();
        db.vacuum();
}

/* ---------------------------------------------------------------------- */

static void compute_profile(TopoDb30& topodb, std::vector<TopoDb30::Profile>& profs, const Point& p0, const Point& p1,
			    unsigned int itercnt = 0)
{
	TopoDb30::Profile prof(topodb.get_profile(p0, p1, 5));
	if (!prof.empty()) {
		profs.push_back(prof);
		return;
	}
	if (itercnt >= 8) {
		std::cerr << "compute_profile: aborting due to iteration count" << std::endl;
		return;
	}
	// fixme: should half according to great circle distance
	Point pm(p0.halfway(p1));
	++itercnt;
	compute_profile(topodb, profs, p0, pm, itercnt);
	compute_profile(topodb, profs, pm, p1, itercnt);
}

/* ---------------------------------------------------------------------- */

static bool update_airways(AirwaysDb::Airway& e, TopoDb30& topodb, bool force)
{
        if (!force &&
	    e.get_terrain_elev() != AirwaysDb::Airway::nodata &&
            e.get_corridor5_elev() != AirwaysDb::Airway::nodata)
                return false;
	std::vector<TopoDb30::Profile> profs;
	compute_profile(topodb, profs, e.get_begin_coord(), e.get_end_coord());
	if (profs.empty()) {
		std::cerr << "Cannot compute profile: Airway " << e.get_name() << ' ' << e.get_begin_name()
			  << ' ' << e.get_end_name() << ' ' << e.get_sourceid() << std::endl;
		return false;
	}
        if (force || e.get_terrain_elev() == AirwaysDb::Airway::nodata) {
		AirwaysDb::Airway::elev_t elev(std::numeric_limits<AirwaysDb::Airway::elev_t>::min());
		for (std::vector<TopoDb30::Profile>::const_iterator psi(profs.begin()), pse(profs.end()); psi != pse; ++psi) {
			const TopoDb30::Profile& prof(*psi);
			for (TopoDb30::Profile::const_iterator i(prof.begin()), e(prof.end()); i != e; ++i) {
				TopoDb30::elev_t el(i->get_elev());
				if (el != TopoDb30::nodata)
					elev = std::max(elev, el);
			}
		}
		if (elev == std::numeric_limits<AirwaysDb::Airway::elev_t>::min()) {
			if (force)
				e.set_terrain_elev();
		} else
			e.set_terrain_elev(elev * Point::m_to_ft);
	}
	if (force || e.get_corridor5_elev() == AirwaysDb::Airway::nodata) {
		AirwaysDb::Airway::elev_t elev(std::numeric_limits<AirwaysDb::Airway::elev_t>::min());
		for (std::vector<TopoDb30::Profile>::const_iterator psi(profs.begin()), pse(profs.end()); psi != pse; ++psi) {
			const TopoDb30::Profile& prof(*psi);
			TopoDb30::elev_t el(prof.get_maxelev());
			if (el != TopoDb30::nodata)
				elev = std::max(elev, el);
		}
		if (elev == std::numeric_limits<AirwaysDb::Airway::elev_t>::min()) {
			if (force)
				e.set_corridor5_elev();
		} else
			e.set_corridor5_elev(elev * Point::m_to_ft);
	}
        return true;
}

/* ---------------------------------------------------------------------- */

static void process_airways(const Glib::ustring& output_dir, bool force)
{
        TopoDb30 topodb;
        topodb.open(output_dir);
        topodb.sync_off();
        AirwaysDb db;
        db.open(output_dir);
        db.sync_off();
        AirwaysDb::Airway e;
        db.loadfirst(e);
        while (e.is_valid()) {
                if (update_airways(e, topodb, force)) {
                        std::cerr << "Saving Airway " << e.get_name() << ' ' << e.get_begin_name() << ' '
				  << e.get_end_name() << ' ' << e.get_sourceid()
                                  << " terrain elev: " << e.get_terrain_elev()
				  << " 5nmi corridor elev: " << e.get_corridor5_elev() << std::endl;
                        db.save(e);
                }
                db.loadnext(e);
        }
        db.analyze();
        db.vacuum();
}

/* ---------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
        static struct option long_options[] = {
                { "help",               no_argument,       NULL, 'h' },
                { "version",            no_argument,       NULL, 'v' },
                { "airspaces",          no_argument,       NULL, 'a' },
                { "airways",            no_argument,       NULL, 'A' },
                { "force",              no_argument,       NULL, 'f' },
                { "output-dir",         required_argument, NULL, 'o' },
                { NULL,                 0,                 NULL, 0 }
        };
        int c, err(0);
        bool doairspaces(false), doairways(false), force(false);
        Glib::ustring output_dir(".");

        while ((c = getopt_long(argc, argv, "hvaAfo:", long_options, NULL)) != -1) {
                switch (c) {
		case 'v':
			std::cout << argv[0] << ": (C) 2007, 2013 Thomas Sailer" << std::endl;
			return 0;

		case 'o':
			output_dir = optarg;
			break;

		case 'a':
			doairspaces = true;
			break;

		case 'A':
			doairways = true;
			break;

		case 'f':
			force = true;
			break;

		case 'h':
		default:
			err++;
			break;
                }
        }
        if (err) {
                std::cerr << "usage: updategndelev [-o <dir>]" << std::endl
                          << "     -a, --airspaces   Process Airspaces" << std::endl
                          << "     -A, --airways     Process Airways" << std::endl
                          << "     -f, --force       Update existing elevations" << std::endl
                          << "     -h, --help        Display this information" << std::endl
                          << "     -v, --version     Display version information" << std::endl << std::endl;
                return EX_USAGE;
        }
	if (!doairspaces && !doairways)
		doairspaces = true;
        try {
		if (doairspaces)
			process_airspaces(output_dir, force);
		if (doairways)
			process_airways(output_dir, force);
        } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                return EX_DATAERR;
        } catch (const Glib::Exception& e) {
                std::cerr << "Glib Error: " << e.what() << std::endl;
                return EX_DATAERR;
        }
        return 0;
}
