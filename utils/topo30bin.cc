/*
 * topo30bin.cc: TopoDb30 to binary conversion
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
#include <fstream>
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
                { NULL,                 0,                 NULL, 0 }
        };
        int c, err(0);
        bool quiet(false);
        Glib::ustring output_dir(".");

        while (((c = getopt_long(argc, argv, "hvqo:", long_options, NULL)) != -1)) {
                switch (c) {
		case 'v':
			std::cout << argv[0] << ": (C) 2015 Thomas Sailer" << std::endl;
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
        if (err || optind >= argc) {
                std::cerr << "usage: topo30bin [-q] [-o <dir>] <file>" << std::endl
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
		TopoDb30::BinFileHeader header;
		std::ofstream of(argv[optind], std::ofstream::binary);
		if (!of.good())
			throw std::runtime_error("Cannot write to file " + std::string(argv[optind]));
		uint64_t ptr(sizeof(header));
		of.seekp(ptr, std::ofstream::beg);
		{
			sqlite3x::sqlite3_command cmd(db.get_db(), "SELECT MINELEV,MAXELEV,ELEV,ID FROM topo ORDER BY ID;");
			sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
			while (cursor.step()) {
				TopoDb30::Tile tile(cursor);
		        	TopoDb30::BinFileHeader::Tile& htile(header[tile.get_index()]);
				htile.set_minelev(tile.get_minelev());
				htile.set_maxelev(tile.get_maxelev());
				htile.set_offset(ptr);
				static const unsigned int nrpixels(TopoDb30::tile_size * TopoDb30::tile_size);
				uint8_t data[nrpixels * 2];
				for (unsigned int i = 0; i < nrpixels; ++i) {
					TopoDb30::elev_t e(tile.get_elev(i));
					data[2 * i] = e;
					data[2 * i + 1] = e >> 8;
				}				
				of.seekp(ptr, std::ofstream::beg);
				of.write((char *)data, nrpixels * 2);
				ptr += nrpixels * 2;
			}
		}
		of.seekp(0, std::ofstream::beg);
		of.write((char *)&header, sizeof(header));
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
