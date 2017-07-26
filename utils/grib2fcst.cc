/*
 * grib2fcst.cc:  Extract GRIB2 Forecast for Winds Aloft
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
#include <iomanip>
#include <iostream>
#include <sstream>
#include <limits>
#include <sqlite3x.hpp>
#include <sqlite3.h>
#include <unistd.h>
#include <stdexcept>
#include <glibmm.h>
#include <giomm.h>

#include "geom.h"
#include "dbobj.h"
#include "engine.h"
#include "icaofpl.h"
#include "baro.h"

/* ---------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
        static struct option long_options[] = {
                { "maindir", required_argument, 0, 'm' },
                { "auxdir", required_argument, 0, 'a' },
                { "disableaux", no_argument, 0, 'A' },
                { "time", required_argument, 0, 't' },
                { "date", required_argument, 0, 't' },
                { "level", required_argument, 0, 'F' },
		{ "flightlevel", required_argument, 0, 'F' },
		{ "fl", required_argument, 0, 'F' },
		{0, 0, 0, 0}
        };
	Glib::init();
	Gio::init();
        Glib::ustring dir_main(""), dir_aux("");
	Engine::auxdb_mode_t auxdbmode(Engine::auxdb_prefs);
	bool dis_aux(false);
	time_t tm(time(0));
	unsigned int baselevel(100), toplevel(100);
        int c, err(0);

        while ((c = getopt_long(argc, argv, "m:a:At:F:", long_options, 0)) != EOF) {
                switch (c) {
		case 'm':
			if (optarg)
				dir_main = optarg;
			break;

		case 'a':
			if (optarg)
				dir_aux = optarg;
			break;

		case 'A':
			dis_aux = true;
			break;

		case 't':
			if (optarg) {
				Glib::TimeVal tv;
				if (tv.assign_from_iso8601(optarg)) {
					tm = tv.tv_sec;
				} else {
					std::cerr << "grib2fcst: invalid time " << optarg << std::endl;
					++err;
				}
			}
			break;

		case 'F':
			if (optarg) {
				char *cp;
				baselevel = strtoul(optarg, &cp, 0);
				if (cp && *cp == ',')
					toplevel = strtoul(cp + 1, 0, 0);
				else
					toplevel = baselevel;
				if (baselevel > toplevel)
					std::swap(baselevel, toplevel);
			}
			break;

		default:
			++err;
			break;
                }
        }
        if (err) {
                std::cerr << "usage: grib2fcst [-m <maindir>] [-a <auxdir>] [-A] [-t <time>] [-F <level>[,<level>]]" << std::endl;
                return EX_USAGE;
        }
	if (dis_aux)
		auxdbmode = Engine::auxdb_none;
	else if (!dir_aux.empty())
		auxdbmode = Engine::auxdb_override;
        try {
		Engine m_engine(dir_main, auxdbmode, dir_aux, false, false);
		// load GRIB2
		{
			GRIB2::Parser p(m_engine.get_grib2_db());
			std::string path1(dir_main);
                        if (path1.empty())
                                path1 = FPlan::get_userdbpath();
                        path1 = Glib::build_filename(path1, "gfs");
			p.parse(path1);
			unsigned int count1(m_engine.get_grib2_db().find_layers().size()), count2(0);
                        std::string path2(m_engine.get_aux_dir(auxdbmode, dir_aux));
			if (!path2.empty()) {
				p.parse(path2);
				count2 = m_engine.get_grib2_db().find_layers().size() - count1;
			}
			std::cout << "Loaded " << count1 << " GRIB2 Layers from " << path1;
			if (!path2.empty())
				std::cout << " and " << count2 << " GRIB2 Layers from " << path2;
			std::cout << std::endl;
			unsigned int count3(m_engine.get_grib2_db().expire_cache());
			if (count3)
				std::cout << "Expired " << count3 << " cached GRIB2 Layers" << std::endl;
		}
		Point coord;
		coord.set_invalid();
		for (int i = optind; i < argc; ++i) {
			Glib::ustring coordtxt(argv[i]);
			{
				IcaoFlightPlan::FindCoord fc(m_engine);
				if (!fc.find(coordtxt, "", IcaoFlightPlan::FindCoord::flag_airport |
					     IcaoFlightPlan::FindCoord::flag_navaid, coord) &&
				    !fc.find("", coordtxt, IcaoFlightPlan::FindCoord::flag_waypoint |
					     IcaoFlightPlan::FindCoord::flag_coord, coord)) {
					std::cerr << "Point " << coordtxt << " not found" << std::endl;
					continue;
				}
				coord = fc.get_coord();
				coordtxt = fc.get_icao();
				if (!fc.get_name().empty()) {
					if (!coordtxt.empty())
						coordtxt += " ";
					coordtxt += fc.get_name();
				}
			}
			Rect bbox(Rect(coord, coord).oversize_nmi(100.f));
			for (unsigned int level = baselevel; level <= toplevel; level += 10) {
				float press(0);
				IcaoAtmosphere<float>::std_altitude_to_pressure(&press, 0, level * (100 * Point::ft_to_m));
				press *= 100;
				Glib::RefPtr<GRIB2::LayerInterpolateResult> windu, windv, prmsl, temperature;
				{
					GRIB2::layerlist_t ll(m_engine.get_grib2_db().find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_momentum_ugrd),
												  tm, GRIB2::surface_isobaric_surface, press));
					windu = GRIB2::interpolate_results(bbox, ll, tm, press);
				}
				{
					GRIB2::layerlist_t ll(m_engine.get_grib2_db().find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_momentum_vgrd),
												  tm, GRIB2::surface_isobaric_surface, press));
					windv = GRIB2::interpolate_results(bbox, ll, tm, press);
				}
				{
					GRIB2::layerlist_t ll(m_engine.get_grib2_db().find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_temperature_tmp),
												  tm, GRIB2::surface_isobaric_surface, press));
					temperature = GRIB2::interpolate_results(bbox, ll, tm, press);
				}
				{
					GRIB2::layerlist_t ll(m_engine.get_grib2_db().find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_prmsl),
												  tm));
					prmsl = GRIB2::interpolate_results(bbox, ll, tm);
				}
				if (!coordtxt.empty())
					std::cout << coordtxt << ' ';
				std::cout << coord.get_lat_str2() << ' ' << coord.get_lon_str2() << " F" << std::setw(3) << std::setfill('0') << level
					  << ' ' << Glib::TimeVal(tm, 0).as_iso8601() << std::fixed;
				if (windu && windv) {
					float wu(windu->operator()(coord, tm, press));
					float wv(windv->operator()(coord, tm, press));
					{
						std::pair<float,float> w(windu->get_layer()->get_grid()->transform_axes(wu, wv));
						wu = w.first;
						wv = w.second;
					}
					// convert from m/s to kts
					wu *= (-1e-3f * Point::km_to_nmi * 3600);
					wv *= (-1e-3f * Point::km_to_nmi * 3600);
					std::cout << " WIND " << std::setw(3) << std::setfill('0')
						  << Point::round<int,float>((M_PI - atan2f(wu, wv)) * (180.f / M_PI))
						  << '/' << std::setw(2) << std::setfill('0')
						  << Point::round<int,float>(sqrtf(wu * wu + wv * wv));
				}
				if (temperature) {
					std::cout << " OAT " << std::setprecision(1)
						  << (temperature->operator()(coord, tm, press) - IcaoAtmosphere<double>::degc_to_kelvin);
				}
				if (prmsl) {
					std::cout << " QFF " << std::setprecision(1) << (prmsl->operator()(coord, tm, press) * 0.01f);
				}
				std::cout << std::endl;
			}
		}
        } catch (const Glib::Exception& ex) {
                std::cerr << "Glib exception: " << ex.what() << std::endl;
                return EX_DATAERR;
        } catch (const std::exception& ex) {
                std::cerr << "exception: " << ex.what() << std::endl;
                return EX_DATAERR;
        }
        return EX_OK;
}
