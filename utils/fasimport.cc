//
// C++ Implementation: fasimport
//
// Description: Final Approach Segment Import
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <unistd.h>
#include <getopt.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <limits>
#include <stdlib.h>
#include <errno.h>
#include <stdexcept>
#include <stdarg.h>
#include <cassert>

#include "dbobj.h"

class FASImporter  {
public:
	FASImporter(const Glib::ustring& output_dir);
	virtual ~FASImporter();

	void parse_file(const Glib::ustring& dbfile);

protected:
	void open_airports_db(void);
	void process_fas(const std::vector<uint8_t>& fasdata);

private:
	Glib::ustring m_outputdir;
	bool m_purgedb;
	bool m_airportsdbopen;
	AirportsDb m_airportsdb;
	time_t m_starttime;

};

FASImporter::FASImporter(const Glib::ustring & output_dir)
        : m_outputdir(output_dir), m_purgedb(false), m_airportsdbopen(false), m_starttime(0)
{
	time(&m_starttime);
}

FASImporter::~FASImporter()
{
        if (m_airportsdbopen)
                m_airportsdb.close();
        m_airportsdbopen = false;
}

void FASImporter::parse_file(const Glib::ustring& fasfile)
{
	std::ifstream is(fasfile.c_str());
	if (is.fail())
		throw std::runtime_error("Cannot open file " + fasfile);
	typedef std::vector<uint8_t> fasdata_t;
	fasdata_t fasdata;
	while (is.good()) {
		std::string line;
		getline(is, line);
		const char *cp(line.c_str());
		unsigned int added(0);
		while (*cp) {
			while (std::isspace(*cp))
				++cp;
			if (*cp == '#' || *cp == ';' || *cp == '\r' || *cp == '\n' || !*cp)
				break;
			char *ep;
			unsigned int v(strtoul(cp, &ep, 16));
			if (ep == cp) {
				std::cerr << "Error parsing line: \"" << line << "\"" << std::endl;
				break;
			}
			fasdata.push_back((uint8_t)v);
			++added;
			cp = ep;
		}
		if (!added || fasdata.size() >= 40) {
			process_fas(fasdata);
			fasdata.clear();
		}
	}
	if (!fasdata.empty())
		process_fas(fasdata);
}

void FASImporter::process_fas(const std::vector<uint8_t>& fasdata)
{
	if (fasdata.empty())
		return;
	AirportsDb::Airport::FinalApproachSegment fas;
	{
		bool ok(fasdata.size() >= 40 && fas.decode(&fasdata[0]));
		if (!ok || fasdata.size() != 40) {
			std::ostringstream oss;
			oss << std::hex;
			for (unsigned int i = 0; i < fasdata.size(); ++i)
				oss << ' ' << std::setw(2) << std::setfill('0') << (unsigned int)fasdata[i];
			if (!ok) {
				if (fasdata.size() < 40) {
					std::cerr << "FAS data block too short:" << oss.str() << std::endl;
					return;
				}
				std::cerr << "FAS data block CRC failure:" << oss.str() << std::endl;
				return;
			}
			std::cerr << "Ignoring spurious FAS data block bytes:" << oss.str() << std::endl;
		}
	}
	open_airports_db();
	if (true) {
		std::cout << "Airport:               " << fas.get_airportid() << std::endl
			  << "Operation Type:        " << (unsigned int)fas.get_operationtype();
		switch (fas.get_operationtype()) {
		case AirportsDb::Airport::FinalApproachSegment::operationtype_straightin:
			std::cout << " Straight In";
			break;

		default:
			break;
		}
		std::cout << std::endl << "SBAS Provider:         " << (unsigned int)fas.get_sbasprovider();
		switch (fas.get_sbasprovider()) {
		case AirportsDb::Airport::FinalApproachSegment::sbasprovider_gbasonly:
			std::cout << " GBAS only";
			break;

		case AirportsDb::Airport::FinalApproachSegment::sbasprovider_anysbas:
			std::cout << " any SBAS";
			break;

		default:
			break;
		}
		std::cout << std::endl << "Runway:                " << (unsigned int)fas.get_runwaynumber() << fas.get_runwayletter_char();
		if (fas.get_runwaynumber() == AirportsDb::Airport::FinalApproachSegment::runwaynumber_heliport)
			std::cout << " Heliport";
		std::cout << std::endl << "Approach Performance:  " << (unsigned int)fas.get_approachperformanceindicator()
			  << std::endl << "Route Indicator:       " << fas.get_routeindicator_char()
			  << std::endl << "RefPath Data Selector: " << (unsigned int)fas.get_referencepathdataselector()
			  << std::endl << "RefPath Ident:         " << fas.get_referencepathident()
			  << std::endl << "FTP:                   " << fas.get_ftp().get_lat_str2() << ' ' << fas.get_ftp().get_lon_str2()
			  << std::endl << "FTP Height:            " << std::fixed << std::setprecision(2) << fas.get_ftp_height_meter() << 'm'
			  << std::endl << "FPAP:                  " << fas.get_fpap().get_lat_str2() << ' ' << fas.get_fpap().get_lon_str2()
			  << std::endl << "TCH:                   " << std::fixed << std::setprecision(2) << fas.get_approach_tch_meter() << "m "
			  << std::fixed << std::setprecision(2) << fas.get_approach_tch_ft() << "ft"
			  << std::endl << "GP Angle:              " << std::fixed << std::setprecision(3) << fas.get_glidepathangle_deg() << "deg"
			  << std::endl << "Course Width:          " << std::fixed << std::setprecision(2) << fas.get_coursewidth_meter() << 'm'
			  << std::endl << "DLengthOffset:         " << std::fixed << std::setprecision(2) << fas.get_dlengthoffset_meter() << 'm'
			  << std::endl << "HAL:                   " << std::fixed << std::setprecision(2) << fas.get_hal_meter() << 'm'
			  << std::endl << "VAL:                   " << std::fixed << std::setprecision(2) << fas.get_val_meter() << 'm'
			  << std::endl << std::endl;
	}
	AirportsDb::Airport el;
	{
		Glib::ustring icao(fas.get_airportid());
		AirportsDb::elementvector_t ev(m_airportsdb.find_by_icao(icao));
		if (ev.size() > 1) {
			std::cerr << "Airport " << icao << " is ambiguous, skipping" << std::endl;
			return;
		}
		if (ev.empty()) {
			std::cerr << "Airport " << icao << " not found" << std::endl;
			return;
		}
		el = ev[0];
	}
	if (el.get_modtime() < m_starttime) {
		while (el.get_nrfinalapproachsegments())
			el.erase_finalapproachsegment(0);
	}
	{
		bool done(false);
		for (unsigned int i = 0; i < el.get_nrfinalapproachsegments(); ++i) {
			AirportsDb::Airport::MinimalFAS& f(el.get_finalapproachsegment(i));
			if (f.get_referencepathident() != fas.get_referencepathident())
				continue;
			f = fas;
			done = true;
			break;
		}
		if (!done)
			el.add_finalapproachsegment(fas);
		el.set_modtime(time(0));
	}
	std::cerr << "Add " << fas.get_referencepathident() << " to " << el.get_icao_name() << std::endl;
	m_airportsdb.save(el);
}

void FASImporter::open_airports_db(void)
{
        if (m_airportsdbopen)
                return;
        m_airportsdb.open(m_outputdir);
        m_airportsdb.sync_off();
        if (m_purgedb)
                m_airportsdb.purgedb();
        m_airportsdbopen = true;
}

int main(int argc, char *argv[])
{
        static struct option long_options[] = {
                { "dir", required_argument, 0, 'd' },
                {0, 0, 0, 0}
        };
        Glib::ustring db_dir(".");
        int c, err(0);

        while ((c = getopt_long(argc, argv, "d:", long_options, 0)) != EOF) {
                switch (c) {
		case 'd':
			if (optarg)
				db_dir = optarg;
			break;

		default:
			err++;
			break;
                }
        }
        if (err) {
                std::cerr << "usage: vfrdbfasimport [-d <dir>]" << std::endl;
                return EX_USAGE;
        }
        try {
                FASImporter parser(db_dir);
                for (; optind < argc; optind++) {
                        std::cerr << "Parsing file " << argv[optind] << std::endl;
                        parser.parse_file(argv[optind]);
                }
        } catch (const std::exception& ex) {
                std::cerr << "exception: " << ex.what() << std::endl;
                return EX_DATAERR;
        }
        return EX_OK;
}
