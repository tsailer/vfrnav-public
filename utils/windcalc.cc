//
// C++ Implementation: windcalc
//
// Description: Wind Calculation Testbench
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdexcept>
#include <stdarg.h>
#include <glibmm.h>

#include "wind.h"

int main(int argc, char *argv[])
{
        static struct option long_options[] = {
		{ "winddir", required_argument, 0, 'd' },
		{ "windspeed", required_argument, 0, 's' },
		{ "tas", required_argument, 0, 't' },
		{ "gs", required_argument, 0, 'g' },
		{ "hdg", required_argument, 0, 'h' },
		{ "crs", required_argument, 0, 'c' },
		{ "heading", required_argument, 0, 'h' },
		{ "course", required_argument, 0, 'c' },
		{ 0, 0, 0, 0 }
        };
        int c, err(0);
	double winddir(std::numeric_limits<double>::quiet_NaN());
	double windspeed(std::numeric_limits<double>::quiet_NaN());
	double tas(std::numeric_limits<double>::quiet_NaN());
	double gs(std::numeric_limits<double>::quiet_NaN());
	double hdg(std::numeric_limits<double>::quiet_NaN());
	double crs(std::numeric_limits<double>::quiet_NaN());

	Glib::init();
        while ((c = getopt_long(argc, argv, "d:s:t:g:h:c:", long_options, 0)) != EOF) {
                switch (c) {
		case 'd':
			winddir = strtod(optarg, 0);
			break;

		case 's':
			windspeed = strtod(optarg, 0);
			break;

		case 't':
			tas = strtod(optarg, 0);
			break;

		case 'g':
			gs = strtod(optarg, 0);
			break;

		case 'h':
			hdg = strtod(optarg, 0);
			break;

		case 'c':
			crs = strtod(optarg, 0);
			break;

 		default:
			err++;
			break;
                }
        }
        if (err || std::isnan(winddir) + std::isnan(windspeed) + std::isnan(tas) + std::isnan(gs) + std::isnan(hdg) + std::isnan(crs) > 2 ) {
                std::cerr << "usage: windcalc [-d <winddir>] [-s <windspeed>] [-t <tas>] [-g <gs>] [-h <hdg>] [-c <crs>]" << std::endl;
                return EX_USAGE;
        }
        try {
		Wind<double> w;
		{
			bool ok(false);
			if (!std::isnan(tas) && !std::isnan(gs) && !std::isnan(hdg) && !std::isnan(crs)) {
				w.set_hdg_tas_crs_gs(hdg, tas, crs, gs);
				ok = true;
			} else if (!std::isnan(winddir) && !std::isnan(windspeed)) {
				w.set_wind(winddir, windspeed);
				if (!std::isnan(tas) && !std::isnan(hdg)) {
					w.set_hdg_tas(hdg, tas);
					ok = true;
				} else if (!std::isnan(gs) && !std::isnan(crs)) {
					w.set_crs_gs(crs, gs);
					ok = true;
				} else if (!std::isnan(tas) && !std::isnan(crs)) {
					w.set_crs_tas(crs, tas);
					ok = true;
				}
			}
			if (!ok) {
				std::cerr << "windcalc: invalid combination of parameters" << std::endl;
				return EX_USAGE;
			}
		}
		std::cout << std::fixed << std::setprecision(1)
			  << "WINDDIR " << w.get_winddir() << " SPEED " << w.get_windspeed() << std::endl
			  << "HDG " << w.get_hdg() << " TAS " << w.get_tas() << std::endl
			  << "CRS " << w.get_crs() << " GS " << w.get_gs() << std::endl;
        } catch (const Glib::Exception& ex) {
                std::cerr << "Glib exception: " << ex.what() << std::endl;
                return EX_DATAERR;
        } catch (const std::exception& ex) {
                std::cerr << "exception: " << ex.what() << std::endl;
                return EX_DATAERR;
        }
	return EX_OK;
}
