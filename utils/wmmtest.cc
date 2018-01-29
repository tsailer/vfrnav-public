//
// C++ Implementation: wmmtest
//
// Description:
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <sqlite3.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdexcept>
#include <stdarg.h>
#include <cmath>
#include <cctype>
#include "wmm.h"

int main(int argc, char *argv[])
{
        static struct option long_options[] = {
                { "date", required_argument, 0, 'd' },
                { "altitude", required_argument, 0, 'a' },
                { 0, 0, 0, 0 }
        };
        float dateval = 2006.0;
        float alt = 0.0;
        int c, err(0);

        while ((c = getopt_long (argc, argv, "d:a:", long_options, 0)) != EOF) {
                switch (c) {
                        case 'd':
                                dateval = strtod(optarg, 0);
                                break;

                        case 'a':
                                alt = strtod(optarg, 0);
                                break;

                        default:
                                err++;
                                break;
                }
        }
        if (err) {
                std::cerr << "usage: wmmtest [-d <date>] [-a <alt>]" << std::endl;
                return 1;
        }
        try {
                WMM wmm;
                std::cout << "Date," << dateval << std::endl << "Alt," << alt << std::endl
                        << "Lat,Lon,D,I,H,X,Y,Z" << std::endl;
                for (float lat = 80; lat >= -85; lat -= 40) {
                        for (float lon = 0; lon <= 359.9; lon += 60) {
                                wmm.compute(alt, lat, lon, dateval);
                                std::cout << lat << ',' << lon << ',';
                                if (!wmm) {
                                        std::cout << "#ERROR,#ERROR,#ERROR,#ERROR,#ERROR,#ERROR" << std::endl;
                                        continue;
                                }
                                std::cout << wmm.get_dec() << ',' << wmm.get_dip() << ',' << wmm.get_h() << ','
                                        << wmm.get_x() << ',' << wmm.get_y() << ',' << wmm.get_z() << ',' << std::endl;
                        }
                }
        } catch (const std::exception& ex) {
                std::cerr << "exception: " << ex.what() << std::endl;
        }
        return 0;
}
