//
// C++ Implementation: adsbdecode
//
// Description: ADS-B 1090ES Message Decode
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
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdexcept>
#include <stdarg.h>

#include "modes.h"

static void decode(std::istream& is, uint32_t ownshipaddr, const Point& ptstn)
{
	while (is.good()) {
		std::string line;
		getline(is, line);
		ModeSMessage msg;
		if (msg.parse_line(line.begin(), line.end()) == line.begin())
			continue;
		std::cout << msg.get_raw_string() << std::endl;
		switch (msg.get_format()) {
		case 4:
		case 20:
		{
			if (msg.addr() != ownshipaddr)
				break;
			uint16_t ac((msg[2] << 8) | msg[3]);
			ac &= 0x1fff;
			std::cout << "Own Ship: Flight Status " << (msg[0] & 7) << " altitude " << ac
				  << " (" << ModeSMessage::decode_alt13(ac) << "ft)" << std::endl;
			break;
		}

		case 5:
		case 21:
		{
			if (msg.addr() != ownshipaddr)
				break;
			uint16_t id((msg[2] << 8) | msg[3]);
			id &= 0x1fff;
			id = ModeSMessage::decode_identity(id);
			std::cout << "Own Ship: Flight Status " << (msg[0] & 7) << " Mode-A "
				  << (char)('0' + ((id >> 9) & 7)) << (char)('0' + ((id >> 6) & 7))
				  << (char)('0' + ((id >> 3) & 7)) << (char)('0' + (id & 7)) << std::endl;
			break;
		}

		case 17:
		case 18:
		{
			if (msg.crc())
				break;
			uint32_t addr((msg[1] << 16) | (msg[2] << 8) | msg[3]);
			{
				std::ostringstream oss;
				oss << "ES: Type " << (unsigned int)msg.get_adsb_type()
				    << " Addr " << std::hex << std::setfill('0') << std::setw(6) << addr
				    << std::dec << " Capability " << (msg[0] & 7);
				std::cout << oss.str();
			}
			if (msg.is_adsb_ident_category()) {
				std::cout << " Emitter Category " << (unsigned int)msg.get_adsb_emittercategory()
					  << " Ident " << msg.get_adsb_ident();
			}
			if (msg.is_adsb_modea()) {
				uint16_t id(ModeSMessage::decode_identity(msg.get_adsb_modea()));
				 std::cout << " Emergency State " << (unsigned int)msg.get_adsb_emergencystate()
					   << " Mode A " << (char)('0' + ((id >> 9) & 7)) << (char)('0' + ((id >> 6) & 7))
					   << (char)('0' + ((id >> 3) & 7)) << (char)('0' + (id & 7));
			}
			if (msg.is_adsb_position()) {
				Point pt(msg.decode_local_cpr17(ptstn));
				std::cout << " Timebit " << (msg.get_adsb_timebit() ? '1' : '0')
					  << " CPR Format " << (msg.get_adsb_cprformat() ? '1' : '0')
					  << " Lat " << msg.get_adsb_cprlat()
					  << " Lon " << msg.get_adsb_cprlon()
					  << " decoded " << pt.get_lat_str2() << ' ' << pt.get_lon_str2();
			}
			if (msg.is_adsb_baroalt()) {
				std::cout << " Baro Alt " << msg.get_adsb_altitude()
					  << " (" << ModeSMessage::decode_alt12(msg.get_adsb_altitude()) << "ft)";
			}
			if (msg.is_adsb_gnssheight()) {
				std::cout << " GNSS Height " << msg.get_adsb_altitude();
			}
			if (msg.is_adsb_velocity_cartesian()) {
				std::cout << " Velocity East " << msg.get_adsb_velocity_east()
					  << "kts North " << msg.get_adsb_velocity_north()
					  << "kts Vertical " << msg.get_adsb_vertical_rate()
					  << "ft/min " << (msg.get_adsb_vertical_source_baro() ? "Baro" : "GNSS");
			}
			if (msg.is_adsb_velocity_polar()) {
				std::cout << " Heading " << (msg.get_adsb_heading() * (360.0 / 65536.0))
					  << " Airspeed " << msg.get_adsb_airspeed()
					  << "kts " << (msg.get_adsb_airspeed_tas() ? "TAS" : "IAS")
					  << " Vertical " << msg.get_adsb_vertical_rate()
					  << "ft/min " << (msg.get_adsb_vertical_source_baro() ? "Baro" : "GNSS");
			}
			std::cout << std::endl;
			break;
		}

		default:
			break;
		}
	}
}

int main(int argc, char *argv[])
{
        static struct option long_options[] = {
		{ "ownshipaddr", required_argument, 0, 'o' },
		{ "position", required_argument, 0, 'p' },
		{ "station", required_argument, 0, 'p' },
		{ 0, 0, 0, 0 }
        };
        int c, err(0);
	uint32_t ownshipaddr(0x4b27cd);
	Point ptstn;
	ptstn.set_lon_deg_dbl(8.5376616474241018);
	ptstn.set_lat_deg_dbl(47.388403275981545);

        while ((c = getopt_long(argc, argv, "o:p:", long_options, 0)) != EOF) {
                switch (c) {
		case 'o':
			ownshipaddr = strtoul(optarg, 0, 16);
			break;

		case 'p':
			ptstn.set_str(optarg);
			break;

 		default:
			err++;
			break;
                }
        }
        if (err) {
                std::cerr << "usage: adsbdecode [-o <addr>] [<input>]" << std::endl;
                return EX_USAGE;
        }
	if (false) {
		std::pair<Point,Point> c(ModeSMessage::decode_global_cpr17(115810, 125857, 98752, 122644));
		std::cout << "Global Decode Test: " << c.first.get_lat_str2() << ' ' << c.first.get_lon_str2()
			  << " / " << c.second.get_lat_str2() << ' ' << c.second.get_lon_str2() << std::endl;
	}
	if (false) {
		ModeSMessage msg;
		std::string line("*8d4b15ed5847538c13f67e94aeaa;");
		if (msg.parse_line(line.begin(), line.end()) != line.begin()) {
			std::cout << "Test1: " << msg.get_raw_string() << std::endl;
			msg.set(11, msg[11] ^ 0x12);
			msg.set(12, msg[12] ^ 0x34);
			msg.set(13, msg[13] ^ 0x56);
			std::cout << "Test2: " << msg.get_raw_string() << " addr " << std::hex
				  << std::setfill('0') << std::setw(6) << msg.addr() << " expected "
				  << std::setfill('0') << std::setw(6) << ModeSMessage::addr_to_crc(0x123456)
				  << " addr " << std::setfill('0') << std::setw(6)
				  << ModeSMessage::crc_to_addr(ModeSMessage::addr_to_crc(0x123456)) << std::dec << std::endl;
			std::cout << "CRC of 4B15ED: " << std::hex << std::setfill('0') << std::setw(6)
				  << ModeSMessage::addr_to_crc(0x4B15ED) << std::dec << std::endl;
		}
	}
	if (false) {
		std::cout << "Decoded Alt: " << ModeSMessage::decode_alt13(0x731) << std::endl;
	}
	if (false) {
		std::pair<Point,Point> c(ModeSMessage::decode_global_cpr17(118513, 125399, 101268, 122287));
		std::cout << "Global Decode Test: " << c.first.get_lat_str2() << ' ' << c.first.get_lon_str2()
			  << " / " << c.second.get_lat_str2() << ' ' << c.second.get_lon_str2() << std::endl;
		c = ModeSMessage::decode_global_cpr17(118561, 125448, 101268, 122287);
		std::cout << "Global Decode Test: " << c.first.get_lat_str2() << ' ' << c.first.get_lon_str2()
			  << " / " << c.second.get_lat_str2() << ' ' << c.second.get_lon_str2() << std::endl;
	}
	if (false) {
		std::pair<Point,Point> c(ModeSMessage::decode_global_cpr17(118368, 128091, 101108, 124621));
		std::cout << "Global Decode Test: " << c.first.get_lat_str2() << ' ' << c.first.get_lon_str2()
			  << " / " << c.second.get_lat_str2() << ' ' << c.second.get_lon_str2() << std::endl;
		c = ModeSMessage::decode_global_cpr17(118368, 128126, 101102, 125012);
		std::cout << "Global Decode Test: " << c.first.get_lat_str2() << ' ' << c.first.get_lon_str2()
			  << " / " << c.second.get_lat_str2() << ' ' << c.second.get_lon_str2() << std::endl;
	}
	if (false) {
		std::cout << "HBPBX: " << ModeSMessage::decode_registration(0x4b27cd) << std::endl
			  << "HB2301: " << ModeSMessage::decode_registration(0x4b4da4) << std::endl
			  << "N99999: " << ModeSMessage::decode_registration(0xADF7C7) << std::endl
			  << "N113AC: " << ModeSMessage::decode_registration(0xA0375B) << std::endl
			  << "N466M: " << ModeSMessage::decode_registration(0xA5B0F8) << std::endl
			  << "N1000H: " << ModeSMessage::decode_registration(0xA00714) << std::endl
			  << "N1000J: " << ModeSMessage::decode_registration(0xA00715) << std::endl
			  << "N1000N: " << ModeSMessage::decode_registration(0xA00719) << std::endl
			  << "N1000P: " << ModeSMessage::decode_registration(0xA0071A) << std::endl
			  << "N100HH: " << ModeSMessage::decode_registration(0xA0056B) << std::endl
			  << "N100HJ: " << ModeSMessage::decode_registration(0xA0056C) << std::endl
			  << "N100HN: " << ModeSMessage::decode_registration(0xA00570) << std::endl
			  << "N100HP: " << ModeSMessage::decode_registration(0xA00571) << std::endl
			  << "N100JH: " << ModeSMessage::decode_registration(0xA00584) << std::endl
			  << "N100JJ: " << ModeSMessage::decode_registration(0xA00585) << std::endl
			  << "N100JN: " << ModeSMessage::decode_registration(0xA00589) << std::endl
			  << "N100JP: " << ModeSMessage::decode_registration(0xA0058A) << std::endl
			  << "N100NH: " << ModeSMessage::decode_registration(0xA005E8) << std::endl
			  << "N100NJ: " << ModeSMessage::decode_registration(0xA005E9) << std::endl
			  << "N100NN: " << ModeSMessage::decode_registration(0xA005ED) << std::endl
			  << "N100NP: " << ModeSMessage::decode_registration(0xA005EE) << std::endl
			  << "N100PH: " << ModeSMessage::decode_registration(0xA00601) << std::endl
			  << "N100PJ: " << ModeSMessage::decode_registration(0xA00602) << std::endl
			  << "N100PN: " << ModeSMessage::decode_registration(0xA00606) << std::endl
			  << "N100PP: " << ModeSMessage::decode_registration(0xA00607) << std::endl
			  << "N: " << ModeSMessage::decode_registration(0xADF7C7+1) << std::endl
;
	}
	ownshipaddr &= 0xffffff;
	if (optind + 1 > argc) {
		decode(std::cin, ownshipaddr, ptstn);
	} else {
		std::ifstream is(argv[optind]);
		if (!is.is_open()) {
			std::cerr << "cannot open input file " << argv[optind] << std::endl;
			return EX_DATAERR;
		}
		decode(is, ownshipaddr, ptstn);
	}
	return EX_OK;
}
