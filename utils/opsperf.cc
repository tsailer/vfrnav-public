/*
 * opsperf.cc:  Aircraft Operations Performance calculation utility
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
#include <stdio.h>

#include "opsperf.h"
#include "aircraft.h"
#include "geom.h"


/* ---------------------------------------------------------------------- */

static Aircraft conv_aircraft(const OperationsPerformance::Aircraft& acft, const std::string& synonym)
{
	Aircraft a;
	a.load_opsperf(acft);
	a.set_icaotype(synonym);
	return a;
}

/* ---------------------------------------------------------------------- */

static std::ostream& print_perf(std::ostream& os, const OperationsPerformance::Aircraft& acft, double mass, OperationsPerformance::Aircraft::compute_t mode)
{
	switch (mode) {
	case OperationsPerformance::Aircraft::compute_climb:
		os << "Climb" << std::endl;
		break;

	case OperationsPerformance::Aircraft::compute_cruise:
		os << "Cruise" << std::endl;
		break;

	case OperationsPerformance::Aircraft::compute_descent:
		os << "Descent" << std::endl;
		break;

	default:
		break;
	}
	os << " FL[-] T[K] p[Pa] rho[kg/m3] a[m/s] TAS[kt] CAS[kt]    M[-] mass[kg] Thrust[N] Drag[N] Fuel[kgm] ESF[-] ROC[fpm] TDC[N]  PWC[-] gammaTAS[deg]" << std::endl;
	for (int alt = 0; alt <= acft.get_maxalt(); ) {
		OperationsPerformance::Aircraft::ComputeResult res(mass);
		AirData<float>& ad(res);
		ad.set_qnh_temp(); // ISA
		ad.set_pressure_altitude(alt);
		if (!acft.compute(res, mode))
			break;
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(0) << std::setw(6) << (ad.get_pressure_altitude() * 0.01) << ' '
		    << std::setprecision(0) << std::setw(3) << (ad.get_oat() + IcaoAtmosphere<float>::degc_to_kelvin) << ' '
		    << std::setprecision(0) << std::setw(6) << (ad.get_p() * 100) << ' '
		    << std::setprecision(3) << std::setw(7)
		    << (IcaoAtmosphere<float>::pressure_to_density(ad.get_p(), ad.get_oat() + IcaoAtmosphere<float>::degc_to_kelvin) * 0.1) << ' '
		    << std::setprecision(0) << std::setw(7) << (ad.get_lss() * (1.0 / Point::mps_to_kts_dbl)) << ' '
		    << std::setprecision(2) << std::setw(8) << ad.get_tas() << ' '
		    << std::setprecision(2) << std::setw(8) << ad.get_cas() << ' '
		    << std::setprecision(2) << std::setw(7) << ad.get_mach() << ' '
		    << std::setprecision(0) << std::setw(6) << res.get_mass() << ' '
		    << std::setprecision(0) << std::setw(9) << res.get_thrust() << ' '
		    << std::setprecision(0) << std::setw(9) << res.get_drag() << ' '
		    << std::setprecision(1) << std::setw(7) << res.get_fuelflow() << ' '
		    << std::setprecision(2) << std::setw(7) << res.get_esf() << ' '
		    << std::setprecision(0) << std::setw(7) << res.get_rocd() << ' '
		    << std::setprecision(0) << std::setw(8) << res.get_tdc() << ' '
		    << std::setprecision(2) << std::setw(7) << res.get_cpowred() << ' '
		    << std::setprecision(2) << std::setw(8) << res.get_gradient();
		os << oss.str() << std::endl;
		if (alt < 2000)
			alt += 500;
		else if (alt < 4000 || alt == 28000)
			alt += 1000;
		else
			alt += 2000;
	}
	return os;
}

/* ---------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
	static struct option long_options[] = {
		{ "dir", required_argument, 0, 'd' },
		{ "climb-low", no_argument, 0, 0x100 },
		{ "climb-medium", no_argument, 0, 0x101 },
		{ "climb-high", no_argument, 0, 0x102 },
		{ "descent-low", no_argument, 0, 0x103 },
		{ "descent-medium", no_argument, 0, 0x104 },
		{ "descent-high", no_argument, 0, 0x105 },
		{ "cruise-low", no_argument, 0, 0x106 },
		{ "cruise-medium", no_argument, 0, 0x107 },
		{ "cruise-high", no_argument, 0, 0x108 },
		{ "xml", no_argument, 0, 0x109 },
		{0, 0, 0, 0}
	};
	Glib::init();
	Gio::init();
        Glib::ustring mdl_dir(PACKAGE_DATA_DIR "/opsperf");
	unsigned int cmds(0);
        int c, err(0);

	while ((c = getopt_long(argc, argv, "d:", long_options, 0)) != EOF) {
		switch (c) {
		case 'd':
			if (optarg)
				mdl_dir = optarg;
			break;

		case 0x100:
		case 0x101:
		case 0x102:
		case 0x103:
		case 0x104:
		case 0x105:
		case 0x106:
		case 0x107:
		case 0x108:
		case 0x109:
			cmds |= 1 << (c - 0x100);
			break;

		default:
			err++;
			break;
		}
	}
	if (err) {
		std::cerr << "usage: aircraftopsperf [-d <dir>]" << std::endl;
		return EX_USAGE;
	}
	try {
		OperationsPerformance opsperf;
		opsperf.load(mdl_dir);
		if (optind >= argc || !cmds) {
			OperationsPerformance::stringset_t l(opsperf.list_aircraft());
			std::cout << "Supported Aircraft:" << std::endl;
			for (OperationsPerformance::stringset_t::const_iterator i(l.begin()), e(l.end()); i != e; ++i) {
				const OperationsPerformance::Aircraft& acft(opsperf.find_aircraft(*i));
				std::cout << "  " << std::setw(4) << std::left << *i << "  " << std::setw(8) << std::left << acft.get_basename() << "  "
					  << acft.get_description() << " (" << acft.get_acfttype() << " w/ " << acft.get_nrengines() << "x "
					  << acft.get_enginetype() << ' ' << acft.get_propulsion() << " cat " << acft.get_wakecategory() << ')'
					  << std::endl;
			}
		}
		for (; optind < argc; ++optind) {
			const OperationsPerformance::Aircraft& acft(opsperf.find_aircraft(argv[optind]));
			if (!acft.is_valid()) {
				std::cerr << "Aircraft type " << argv[optind] << " not found" << std::endl;
				continue;
			}
			if (cmds & (1 << 0))
				print_perf(std::cout, acft, acft.get_massmin() * 1.2, OperationsPerformance::Aircraft::compute_climb);
			if (cmds & (1 << 1))
				print_perf(std::cout, acft, acft.get_massref(), OperationsPerformance::Aircraft::compute_climb);
			if (cmds & (1 << 2))
				print_perf(std::cout, acft, acft.get_massmax(), OperationsPerformance::Aircraft::compute_climb);
			if (cmds & (1 << 3))
				print_perf(std::cout, acft, acft.get_massmin() * 1.2, OperationsPerformance::Aircraft::compute_descent);
			if (cmds & (1 << 4))
				print_perf(std::cout, acft, acft.get_massref(), OperationsPerformance::Aircraft::compute_descent);
			if (cmds & (1 << 5))
				print_perf(std::cout, acft, acft.get_massmax(), OperationsPerformance::Aircraft::compute_descent);
			if (cmds & (1 << 6))
				print_perf(std::cout, acft, acft.get_massmin() * 1.2, OperationsPerformance::Aircraft::compute_cruise);
			if (cmds & (1 << 7))
				print_perf(std::cout, acft, acft.get_massref(), OperationsPerformance::Aircraft::compute_cruise);
			if (cmds & (1 << 8))
				print_perf(std::cout, acft, acft.get_massmax(), OperationsPerformance::Aircraft::compute_cruise);
			if (cmds & (1 << 9)) {
				Aircraft a(conv_aircraft(acft, argv[optind]));
				std::cout << a.save_string();
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
