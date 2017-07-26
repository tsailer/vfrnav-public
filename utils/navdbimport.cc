//
// C++ Implementation: navdbimport
//
// Description: Database Import
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <sigc++/sigc++.h>
#include <unistd.h>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <limits>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdexcept>
#include <stdarg.h>
#include <cassert>

#include "dbobj.h"

class NavDbImporter  {
public:
	NavDbImporter(const Glib::ustring& output_dir);
	virtual ~NavDbImporter();

	void parse_file(const Glib::ustring& dbfile);
	bool is_deleteunseen(void) const { return m_deleteunseen; }
	void set_deleteunseen(bool x = true) { m_deleteunseen = x; }
	void delete_old_sidstars(void);

protected:
	void open_airports_db(void);
	void open_airspaces_db(void);
	void open_navaids_db(void);
	void open_waypoints_db(void);
	void open_airways_db(void);

	void parse_sidstar(sqlite3x::sqlite3_connection& db);
	void parse_sidstars(sqlite3x::sqlite3_connection& db);

private:
	Glib::ustring m_outputdir;
	bool m_purgedb;
	bool m_airportsdbopen;
	AirportsDb m_airportsdb;
	bool m_airspacesdbopen;
	AirspacesDb m_airspacesdb;
	bool m_navaidsdbopen;
	NavaidsDb m_navaidsdb;
	bool m_waypointsdbopen;
	WaypointsDb m_waypointsdb;
	bool m_airwaysdbopen;
	AirwaysDb m_airwaysdb;
	time_t m_starttime;
	bool m_deleteunseen;
};

NavDbImporter::NavDbImporter(const Glib::ustring & output_dir)
        : m_outputdir(output_dir), m_purgedb(false), m_airportsdbopen(false), m_airspacesdbopen(false),
	  m_navaidsdbopen(false), m_waypointsdbopen(false), m_airwaysdbopen(false),
	  m_starttime(0), m_deleteunseen(false)
{
	time(&m_starttime);
}

NavDbImporter::~NavDbImporter()
{
        if (m_airportsdbopen)
                m_airportsdb.close();
        m_airportsdbopen = false;
        if (m_airspacesdbopen)
                m_airspacesdb.close();
        m_airspacesdbopen = false;
        if (m_navaidsdbopen)
                m_navaidsdb.close();
        m_navaidsdbopen = false;
        if (m_waypointsdbopen)
                m_waypointsdb.close();
        m_waypointsdbopen = false;
        if (m_airwaysdbopen)
                m_airwaysdb.close();
        m_airwaysdbopen = false;
}

void NavDbImporter::parse_file(const Glib::ustring& dbfile)
{
	sqlite3x::sqlite3_connection db;
	db.open(dbfile);
	typedef std::set<Glib::ustring> tables_t;
	tables_t tables;
	{
		sqlite3x::sqlite3_command cmd(db, "SELECT name FROM sqlite_master WHERE type=\"table\";");
		sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
		while (cursor.step())
			tables.insert(Glib::ustring(cursor.getstring(0)).uppercase());
	}
	if (true) {
		std::cerr << "tables:";
		bool subseq(false);
		for (tables_t::const_iterator ti(tables.begin()), te(tables.end()); ti != te; ++ti)
			std::cerr << (subseq ? ',' : ' ') << *ti;
		std::cerr << std::endl;
	}
	if (tables.find("SIDSTAR") != tables.end())
		parse_sidstar(db);
	if (tables.find("SIDSTARS") != tables.end())
		parse_sidstars(db);
	
}

void NavDbImporter::parse_sidstar(sqlite3x::sqlite3_connection& db)
{
	open_airports_db();
	open_navaids_db();
	open_waypoints_db();
	sqlite3x::sqlite3_command cmd(db, "SELECT Ident,terminator,route,RWY,MEA,MAA,SID,Airport FROM SIDSTAR;");
	sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
	while (cursor.step()) {
		std::string ident(cursor.getstring(0));
		std::string terminator(cursor.getstring(1));
		std::string route(cursor.getstring(2));
		std::string rwy(cursor.getstring(3));
		int mea(cursor.getint(4));
		int maa(cursor.getint(5));
		bool sid(!!cursor.getint(6));
		std::string icao(cursor.getstring(7));
		AirportsDb::Airport el;
		{
			AirportsDb::elementvector_t ev(m_airportsdb.find_by_icao(icao));
			if (ev.size() > 1) {
				std::cerr << "Airport " << icao << " is ambiguous, skipping" << std::endl;
				continue;
			}
			if (ev.empty()) {
				std::cerr << "Airport " << icao << " not found" << std::endl;
				continue;
			}
			el = ev[0];
		}
		if (el.get_modtime() < m_starttime) {
			el.set_modtime(time(0));
			for (unsigned int i(el.get_nrvfrrte()); i > 0; ) {
				--i;
				const AirportsDb::Airport::VFRRoute& rte(el.get_vfrrte(i));
				if (!rte.size())
					continue;
				if (rte[0].get_pathcode() != AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star &&
				    rte[rte.size()-1].get_pathcode() != AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid)
					continue;
				el.erase_vfrrte(i);
			}
		}
		maa = std::min(maa, (int)std::numeric_limits<int16_t>::max());
		AirportsDb::Airport::VFRRoute rte(ident + "-" + rwy);
		AirportsDb::Airport::VFRRoute::VFRRoutePoint ptairport(el.get_icao(), el.get_coord(), el.get_elevation(),
								       AirportsDb::Airport::label_off, 'C',
								       sid ? AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid
								       : AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star,
								       AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_altspecified, true);
		{
			std::string rwy2(rwy);
			if (rwy2.substr(0, 2) == "RW")
				rwy2.erase(0, 2);
			for (unsigned int i = 0, j = el.get_nrrwy(); i < j; ++i) {
				const AirportsDb::Airport::Runway& rwy(el.get_rwy(i));
				if (rwy.get_ident_he() == rwy2) {
					ptairport.set_coord(rwy.get_he_coord());
					ptairport.set_altitude(rwy.get_he_elev());
					break;
				}
				if (rwy.get_ident_le() == rwy2) {
					ptairport.set_coord(rwy.get_le_coord());
					ptairport.set_altitude(rwy.get_le_elev());
					break;
				}
			}
		}
		if (sid)
			rte.add_point(ptairport);
		bool skip(false);
		if (route.empty()) {
			// try to synthesize the route endpoint
			std::string ident1(ident);
			std::string::size_type n(ident1.find_first_of("0123456789"));
			if (n != std::string::npos)
				ident1.erase(n);
			if (!ident1.empty()) {
				std::string::size_type matchsz(std::max(ident1.size(), (std::string::size_type)4));
				Rect bbox(el.get_coord().simple_box_nmi(1000.f));
				AirportsDb::Airport::VFRRoute::VFRRoutePoint rpt("", Point(), maa, AirportsDb::Airport::label_e, 'C', ptairport.get_pathcode(),
										 AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_atorbelow, false);
				uint64_t dist(std::numeric_limits<uint64_t>::max());
				{
					NavaidsDb::elementvector_t ev(m_navaidsdb.find_by_rect(bbox));
					for (NavaidsDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
						if (evi->get_icao().substr(0, matchsz) != ident1)
							continue;
						uint64_t d(ptairport.get_coord().simple_distance_rel(evi->get_coord()));
						if (d >= dist)
							continue;
						dist = d;
						rpt.set_name(evi->get_icao());
						rpt.set_coord(evi->get_coord());
					}
				}
				{
					WaypointsDb::elementvector_t ev(m_waypointsdb.find_by_rect(bbox));
					for (WaypointsDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
						if (evi->get_name().substr(0, matchsz) != ident1)
							continue;
						uint64_t d(ptairport.get_coord().simple_distance_rel(evi->get_coord()));
						if (d >= dist)
							continue;
						dist = d;
						rpt.set_name(evi->get_name());
						rpt.set_coord(evi->get_coord());
					}
				}
				if (dist == std::numeric_limits<uint64_t>::max()) {
					std::cerr << "Point " << ident1 << " not found, skipping route (Airport "
						  << el.get_icao_name() << " route " << ident << ")" << std::endl;
					skip = true;
				}
				rte.add_point(rpt);
			}
		}
		while (!route.empty()) {
			std::string routept;
			{
				std::string::size_type n(route.find(' '));
				if (n == std::string::npos) {
					routept.swap(route);
				} else {
					routept = route.substr(0, n);
					route.erase(0, n + 1);
				}
			}
			AirportsDb::Airport::VFRRoute::VFRRoutePoint rpt("", Point(), maa, AirportsDb::Airport::label_e, 'C', ptairport.get_pathcode(),
									 AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_atorbelow, false);
			uint64_t dist(std::numeric_limits<uint64_t>::max());
			{
				NavaidsDb::elementvector_t ev(m_navaidsdb.find_by_icao(routept));
				for (NavaidsDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
					uint64_t d(ptairport.get_coord().simple_distance_rel(evi->get_coord()));
					if (d >= dist)
						continue;
					dist = d;
					rpt.set_name(evi->get_icao());
					rpt.set_coord(evi->get_coord());
				}
			}
			{
				WaypointsDb::elementvector_t ev(m_waypointsdb.find_by_name(routept));
				for (WaypointsDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
					uint64_t d(ptairport.get_coord().simple_distance_rel(evi->get_coord()));
					if (d >= dist)
						continue;
					dist = d;
					rpt.set_name(evi->get_name());
					rpt.set_coord(evi->get_coord());
				}
			}
			if (dist == std::numeric_limits<uint64_t>::max()) {
				skip = route.empty();
				std::cerr << "Point " << routept << " not found, skipping " << (skip ? "route" : "point")
					  << " (Airport " << el.get_icao_name() << " route " << ident << ")" << std::endl;
				continue;
			}
			rte.add_point(rpt);
		}
		if (!sid)
			rte.add_point(ptairport);
		if (skip)
			continue;
		// fix up shortened ident
		{
			const AirportsDb::Airport::VFRRoute::VFRRoutePoint& connpt(rte[sid ? rte.size() - 1 : 0]);
			if (ident.empty()) {
				ident = connpt.get_name();
				rte.set_name(ident + "-" + rwy);
			} else {
				std::string::size_type n(ident.find_first_of("0123456789"));
				if (n != std::string::npos && ident.substr(0, n) == connpt.get_name().substr(0, n)) {
					ident = connpt.get_name() + ident.substr(n);
					rte.set_name(ident + "-" + rwy);
				}
			}
		}
		el.add_vfrrte(rte);
		m_airportsdb.save(el);
	}
}

void NavDbImporter::parse_sidstars(sqlite3x::sqlite3_connection& db)
{
	open_airports_db();
	open_navaids_db();
	open_waypoints_db();
	AirportsDb::Airport arpt;
	bool arptdirty(false);
	typedef std::vector<bool> seenproc_t;
	seenproc_t seenproc;
	sqlite3x::sqlite3_command cmd(db, "SELECT airport,type,ident FROM sidstars ORDER BY airport;");
	sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
	while (cursor.step()) {
		std::string airportid(cursor.getstring(0));
		std::string ident(cursor.getstring(2));
		AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_t pathcode(AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_invalid);
		{
			std::string pc(cursor.getstring(1));
			if (pc == "SID")
				pathcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid;
			else if (pc == "STAR")
				pathcode = AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star;
			if (pathcode == AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_invalid) {
				std::cerr << "Invalid path code, skipping: " << airportid << ',' << pc << ',' << ident << std::endl;
				continue;
			}
		}
		std::string ident1(ident);
		{
			std::string::size_type n(ident1.find_first_of("0123456789"));
			if (n != std::string::npos)
				ident1.erase(n);
			else
				ident += "1A";
		}
		if (true)
			std::cerr << "Processing " << airportid << ((pathcode == AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid)
								    ? " SID " : " STAR ")
				  << ident << ' ' << ident1 << std::endl;
		if (!arpt.is_valid() || arpt.get_icao() != airportid) {
			if (arpt.is_valid() && arptdirty) {
				if (m_deleteunseen) {
					for (unsigned int i(arpt.get_nrvfrrte()); i > 0; ) {
						--i;
						if (seenproc[i])
							continue;
						const AirportsDb::Airport::VFRRoute& rte(arpt.get_vfrrte(i));
						if (!rte.size())
							continue;
						if (rte[0].get_pathcode() != AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star &&
						    rte[rte.size()-1].get_pathcode() != AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid)
							continue;
						arpt.erase_vfrrte(i);
					}
				}
				m_airportsdb.save(arpt);
				arptdirty = false;
			}
			AirportsDb::elementvector_t ev(m_airportsdb.find_by_icao(airportid));
			if (ev.size() > 1) {
				std::cerr << "Airport " << airportid << " is ambiguous, skipping" << std::endl;
				continue;
			}
			if (ev.empty()) {
				std::cerr << "Airport " << airportid << " not found" << std::endl;
				continue;
			}
			arpt = ev[0];
			arpt.set_modtime(time(0));
			arptdirty = false;
			seenproc.clear();
			seenproc.resize(arpt.get_nrvfrrte(), false);
		}
		bool found(false);
		for (unsigned int i(arpt.get_nrvfrrte()); i > 0; ) {
			--i;
			const AirportsDb::Airport::VFRRoute& rte(arpt.get_vfrrte(i));
			if (!rte.size())
				continue;
			bool sid(pathcode == AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid);
			const AirportsDb::Airport::VFRRoute::VFRRoutePoint& connpt(rte[sid ? rte.size() - 1 : 0]);
			if (connpt.get_pathcode() != pathcode || connpt.get_name() != ident1)
				continue;
			seenproc[i] = true;
			found = true;
			arptdirty = true;
			break;
		}
		if (found)
			continue;
		AirportsDb::Airport::VFRRoute rte(ident);
		AirportsDb::Airport::VFRRoute::VFRRoutePoint ptairport(arpt.get_icao(), arpt.get_coord(), arpt.get_elevation(),
								       AirportsDb::Airport::label_off, 'C', pathcode,
								       AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_altspecified, true);
		if (pathcode == AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid)
			rte.add_point(ptairport);
		{
			AirportsDb::Airport::VFRRoute::VFRRoutePoint rpt(ident1, Point(), arpt.get_elevation(), AirportsDb::Airport::label_e, 'C', pathcode,
									 AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_atorabove, false);
			uint64_t dist(std::numeric_limits<uint64_t>::max());
			{
				NavaidsDb::elementvector_t ev(m_navaidsdb.find_by_icao(ident1));
				for (NavaidsDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
					uint64_t d(ptairport.get_coord().simple_distance_rel(evi->get_coord()));
					if (d >= dist)
						continue;
					dist = d;
					rpt.set_name(evi->get_icao());
					rpt.set_coord(evi->get_coord());
				}
			}
			{
				WaypointsDb::elementvector_t ev(m_waypointsdb.find_by_name(ident1));
				for (WaypointsDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
					uint64_t d(ptairport.get_coord().simple_distance_rel(evi->get_coord()));
					if (d >= dist)
						continue;
					dist = d;
					rpt.set_name(evi->get_name());
					rpt.set_coord(evi->get_coord());
				}
			}
			if (dist == std::numeric_limits<uint64_t>::max()) {
				std::cerr << "Point " << ident1 << " not found, skipping (Airport "
					  << arpt.get_icao_name() << " route " << ident << ")" << std::endl;
				continue;
			}
			double distd(arpt.get_coord().spheric_distance_nmi_dbl(rpt.get_coord()));
			if (distd > 1000) {
				std::cerr << "Point " << ident1 << " distance " << distd << " > 1000nmi, skipping (Airport "
					  << arpt.get_icao_name() << " route " << ident << ")" << std::endl;
				continue;
			}
			rte.add_point(rpt);
		}
		if (pathcode == AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star)
			rte.add_point(ptairport);
		arpt.add_vfrrte(rte);
		seenproc.resize(arpt.get_nrvfrrte(), true);
		arptdirty = true;
	}
	if (arpt.is_valid() && arptdirty) {
		if (m_deleteunseen) {
			for (unsigned int i(arpt.get_nrvfrrte()); i > 0; ) {
				--i;
				if (seenproc[i])
					continue;
				const AirportsDb::Airport::VFRRoute& rte(arpt.get_vfrrte(i));
				if (!rte.size())
					continue;
				if (rte[0].get_pathcode() != AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star &&
				    rte[rte.size()-1].get_pathcode() != AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid)
					continue;
				arpt.erase_vfrrte(i);
			}
		}
		m_airportsdb.save(arpt);
		arptdirty = false;
	}
}

void NavDbImporter::delete_old_sidstars(void)
{
	open_airports_db();
	AirportsDb::Airport arpt;
	m_airportsdb.loadfirst(arpt, false);
	while (arpt.is_valid()) {
		if (arpt.get_modtime() < m_starttime) {
			arpt.set_modtime(time(0));
			bool dirty(false);
			for (unsigned int i(arpt.get_nrvfrrte()); i > 0; ) {
				--i;
				const AirportsDb::Airport::VFRRoute& rte(arpt.get_vfrrte(i));
				if (!rte.size())
					continue;
				if (rte[0].get_pathcode() != AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star &&
				    rte[rte.size()-1].get_pathcode() != AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid)
					continue;
				arpt.erase_vfrrte(i);
				dirty = true;
			}
			if (dirty) {
				m_airportsdb.save(arpt);
				if (true)
					std::cerr << "Deleting SID/STARs from " << arpt.get_icao() << std::endl;
			}
		}
		m_airportsdb.loadnext(arpt, false);
	}
}

void NavDbImporter::open_airports_db(void)
{
        if (m_airportsdbopen)
                return;
        m_airportsdb.open(m_outputdir);
        m_airportsdb.sync_off();
        if (m_purgedb)
                m_airportsdb.purgedb();
        m_airportsdbopen = true;
}

void NavDbImporter::open_airspaces_db(void)
{
        if (m_airspacesdbopen)
                return;
        m_airspacesdb.open(m_outputdir);
        m_airspacesdb.sync_off();
        if (m_purgedb)
                m_airspacesdb.purgedb();
        m_airspacesdbopen = true;
}

void NavDbImporter::open_navaids_db(void)
{
        if (m_navaidsdbopen)
                return;
        m_navaidsdb.open(m_outputdir);
        m_navaidsdb.sync_off();
        if (m_purgedb)
                m_navaidsdb.purgedb();
        m_navaidsdbopen = true;
}

void NavDbImporter::open_waypoints_db(void)
{
        if (m_waypointsdbopen)
                return;
        m_waypointsdb.open(m_outputdir);
        m_waypointsdb.sync_off();
        if (m_purgedb)
                m_waypointsdb.purgedb();
        m_waypointsdbopen = true;
}

void NavDbImporter::open_airways_db(void)
{
        if (m_airwaysdbopen)
                return;
        m_airwaysdb.open(m_outputdir);
        m_airwaysdb.sync_off();
        if (m_purgedb)
                m_airwaysdb.purgedb();
        m_airwaysdbopen = true;
}

int main(int argc, char *argv[])
{
        static struct option long_options[] = {
                { "dir", required_argument, 0, 'd' },
                { "delete-unseen", no_argument, 0, 'U' },
                { "erase-unseen", no_argument, 0, 'U' },
                { "delete-old", no_argument, 0, 'O' },
                { "erase-old", no_argument, 0, 'O' },
                {0, 0, 0, 0}
        };
        Glib::ustring db_dir(".");
	bool delunseen(false), delold(false);
        int c, err(0);

        while ((c = getopt_long(argc, argv, "d:UO", long_options, 0)) != EOF) {
                switch (c) {
		case 'd':
			if (optarg)
				db_dir = optarg;
			break;

		case 'U':
			delunseen = true;
			break;

		case 'O':
			delold = true;
			break;

		default:
			err++;
			break;
                }
        }
        if (err) {
                std::cerr << "usage: vfrnavdbimport [-d <dir>] [-U] [-O]" << std::endl;
                return EX_USAGE;
        }
        try {
                NavDbImporter parser(db_dir);
		parser.set_deleteunseen(delunseen);
                for (; optind < argc; optind++) {
                        std::cerr << "Parsing file " << argv[optind] << std::endl;
                        parser.parse_file(argv[optind]);
                }
		if (delold)
			parser.delete_old_sidstars();
        } catch (const std::exception& ex) {
                std::cerr << "exception: " << ex.what() << std::endl;
                return EX_DATAERR;
        }
        return EX_OK;
}
