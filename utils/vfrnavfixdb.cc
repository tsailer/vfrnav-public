//
// C++ Implementation: vfrnavfixdb
//
// Description: Database fix routines
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012, 2013, 2015, 2016
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
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdexcept>
#include <stdarg.h>
#include <cassert>

#include "dbobj.h"

class DbFix {
public:
	DbFix(const Glib::ustring& output_dir, bool pgsql = false, bool trace = false, bool nomodify = false);
	virtual ~DbFix();

	void tile(void);
	void zapsidstar(void);

protected:
	void open_airports_db(void);
	void open_airspaces_db(void);
	void open_navaids_db(void);
	void open_waypoints_db(void);
	void open_airways_db(void);
	void open_tracks_db(void);
	void close_db(void);

private:
	Glib::ustring m_outputdir;
	bool m_trace;
#ifdef HAVE_PQXX
	typedef std::unique_ptr<pqxx::connection> pgconn_t;
	pgconn_t m_pgconn;
#endif
	bool m_nomodify;
	std::unique_ptr<AirportsDbQueryInterface> m_airportsdb;
	std::unique_ptr<AirspacesDbQueryInterface> m_airspacesdb;
	std::unique_ptr<NavaidsDbQueryInterface> m_navaidsdb;
	std::unique_ptr<WaypointsDbQueryInterface> m_waypointsdb;
	std::unique_ptr<AirwaysDbQueryInterface> m_airwaysdb;
	std::unique_ptr<TracksDbQueryInterface> m_tracksdb;

	AirportsDb::Airport m_arpt;
	AirspacesDb::Airspace m_aspc;
	NavaidsDb::Navaid m_nav;
	WaypointsDb::Waypoint m_wpt;
	AirwaysDb::Airway m_awy;
	TracksDb::Track m_trk;
};

DbFix::DbFix(const Glib::ustring & output_dir, bool pgsql, bool trace, bool nomodify)
	: m_outputdir(output_dir), m_trace(trace), m_nomodify(nomodify)
{
#ifdef HAVE_PQXX
	if (pgsql) {
		m_pgconn = pgconn_t(new pqxx::connection(m_outputdir));
		if (m_pgconn->get_variable("application_name").empty())
			m_pgconn->set_variable("application_name", "vfrnavfixdb");
	}
#else
	if (pgsql)
		throw std::runtime_error("postgres support not compiled in");
#endif
}

DbFix::~DbFix()
{
	close_db();
}


void DbFix::close_db(void)
{
	if (m_trace)
		std::cerr << "close_db" << std::endl;
	if (m_airportsdb) {
		m_airportsdb->set_exclusive(false);
		m_airportsdb->close();
		m_airportsdb.reset();
	}
	if (m_airspacesdb) {
		m_airspacesdb->set_exclusive(false);
		m_airspacesdb->close();
		m_airspacesdb.reset();
	}
	if (m_navaidsdb) {
		m_navaidsdb->set_exclusive(false);
		m_navaidsdb->close();
		m_navaidsdb.reset();
	}
	if (m_waypointsdb) {
		m_waypointsdb->set_exclusive(false);
		m_waypointsdb->close();
		m_waypointsdb.reset();
	}
	if (m_airwaysdb) {
		m_airwaysdb->set_exclusive(false);
		m_airwaysdb->close();
		m_airwaysdb.reset();
	}
	if (m_tracksdb) {
		m_tracksdb->set_exclusive(false);
		m_tracksdb->close();
		m_tracksdb.reset();
	}
}

void DbFix::open_airports_db(void)
{
	if (m_airportsdb)
		return;
#ifdef HAVE_PQXX
	if (m_pgconn)
		m_airportsdb.reset(new AirportsPGDb(*m_pgconn));
	else
#endif
		m_airportsdb.reset(new AirportsDb(m_outputdir));
     	m_airportsdb->set_exclusive(true);
	m_airportsdb->sync_off();
}

void DbFix::open_airspaces_db(void)
{
	if (m_airspacesdb)
		return;
#ifdef HAVE_PQXX
	if (m_pgconn)
		m_airspacesdb.reset(new AirspacesPGDb(*m_pgconn));
	else
#endif
		m_airspacesdb.reset(new AirspacesDb(m_outputdir));
	m_airspacesdb->set_exclusive(true);
	m_airspacesdb->sync_off();
}

void DbFix::open_navaids_db(void)
{
	if (m_navaidsdb)
		return;
#ifdef HAVE_PQXX
	if (m_pgconn)
		m_navaidsdb.reset(new NavaidsPGDb(*m_pgconn));
	else
#endif
		m_navaidsdb.reset(new NavaidsDb(m_outputdir));
	m_navaidsdb->set_exclusive(true);
	m_navaidsdb->sync_off();
}

void DbFix::open_waypoints_db(void)
{
	if (m_waypointsdb)
		return;
#ifdef HAVE_PQXX
	if (m_pgconn)
		m_waypointsdb.reset(new WaypointsPGDb(*m_pgconn));
	else
#endif
		m_waypointsdb.reset(new WaypointsDb(m_outputdir));
	m_waypointsdb->set_exclusive(true);
	m_waypointsdb->sync_off();
}

void DbFix::open_airways_db(void)
{
	if (m_airwaysdb)
		return;
#ifdef HAVE_PQXX
	if (m_pgconn)
		m_airwaysdb.reset(new AirwaysPGDb(*m_pgconn));
	else
#endif
		m_airwaysdb.reset(new AirwaysDb(m_outputdir));
	m_airwaysdb->set_exclusive(true);
	m_airwaysdb->sync_off();
}

void DbFix::open_tracks_db(void)
{
	if (m_tracksdb)
		return;
#ifdef HAVE_PQXX
       	if (m_pgconn)
		m_tracksdb.reset(new TracksPGDb(*m_pgconn));
	else
#endif
		m_tracksdb.reset(new TracksDb(m_outputdir));
	m_tracksdb->set_exclusive(true);
	m_tracksdb->sync_off();
}

void DbFix::tile(void)
{
	open_airports_db();
	if (m_airportsdb) {
		unsigned int modcnt(0);
		for (;;) {
			AirportsDbQueryInterface::elementvector_t ev(m_airportsdb->find_nulltile(1024, DbBaseElements::Airport::subtables_all));
			if (ev.empty())
				break;
			for (AirportsDbQueryInterface::elementvector_t::iterator i(ev.begin()), e(ev.end()); i != e; ++i) {
				if (!i->is_valid())
					continue;
				if (m_trace)
					std::cerr << "Airport " << i->get_address() << ' ' << i->get_icao_name() << std::endl;
				if (m_nomodify)
					continue;
				i->set_modtime(time(0));
				m_airportsdb->save(*i);
				++modcnt;
			}
			if (m_nomodify)
				break;
		}
		if (modcnt) {
			m_airportsdb->analyze();
			m_airportsdb->vacuum();
		}
	}
	open_airspaces_db();
	if (m_airspacesdb) {
		unsigned int modcnt(0);
		for (;;) {
			AirspacesDbQueryInterface::elementvector_t ev(m_airspacesdb->find_nulltile(1024, DbBaseElements::Airspace::subtables_all));
			if (ev.empty())
				break;
			for (AirspacesDbQueryInterface::elementvector_t::iterator i(ev.begin()), e(ev.end()); i != e; ++i) {
				if (!i->is_valid())
					continue;
				if (m_trace)
					std::cerr << "Airspace " << i->get_address() << ' ' << i->get_icao_name() << std::endl;
				if (m_nomodify)
					continue;
				i->set_modtime(time(0));
				m_airspacesdb->save(*i);
				++modcnt;
			}
			if (m_nomodify)
				break;
		}
		if (modcnt) {
			m_airspacesdb->analyze();
			m_airspacesdb->vacuum();
		}
	}
	open_navaids_db();
	if (m_navaidsdb) {
		unsigned int modcnt(0);
		for (;;) {
			NavaidsDbQueryInterface::elementvector_t ev(m_navaidsdb->find_nulltile(1024, DbBaseElements::Navaid::subtables_all));
			if (ev.empty())
				break;
			for (NavaidsDbQueryInterface::elementvector_t::iterator i(ev.begin()), e(ev.end()); i != e; ++i) {
				if (!i->is_valid())
					continue;
				if (m_trace)
					std::cerr << "Navaid " << i->get_address() << ' ' << i->get_icao_name() << std::endl;
				if (m_nomodify)
					continue;
				i->set_modtime(time(0));
				m_navaidsdb->save(*i);
				++modcnt;
			}
			if (m_nomodify)
				break;
		}
		if (modcnt) {
			m_navaidsdb->analyze();
			m_navaidsdb->vacuum();
		}
	}
	open_waypoints_db();
	if (m_waypointsdb) {
		unsigned int modcnt(0);
		for (;;) {
			WaypointsDbQueryInterface::elementvector_t ev(m_waypointsdb->find_nulltile(1024, DbBaseElements::Waypoint::subtables_all));
			if (ev.empty())
				break;
			for (WaypointsDbQueryInterface::elementvector_t::iterator i(ev.begin()), e(ev.end()); i != e; ++i) {
				if (!i->is_valid())
					continue;
				if (m_trace)
					std::cerr << "Waypoint " << i->get_address() << ' ' << i->get_icao_name() << std::endl;
				if (m_nomodify)
					continue;
				i->set_modtime(time(0));
				m_waypointsdb->save(*i);
				++modcnt;
			}
			if (m_nomodify)
				break;
		}
		if (modcnt) {
			m_waypointsdb->analyze();
			m_waypointsdb->vacuum();
		}
	}
	open_airways_db();
	if (m_airwaysdb) {
		unsigned int modcnt(0);
		for (;;) {
			AirwaysDbQueryInterface::elementvector_t ev(m_airwaysdb->find_nulltile(1024, DbBaseElements::Airway::subtables_all));
			if (ev.empty())
				break;
			for (AirwaysDbQueryInterface::elementvector_t::iterator i(ev.begin()), e(ev.end()); i != e; ++i) {
				if (!i->is_valid())
					continue;
				if (m_trace)
					std::cerr << "Airway " << i->get_address() << ' ' << i->get_name() << ' ' << i->get_begin_name() << ' ' << i->get_end_name() << std::endl;
				if (m_nomodify)
					continue;
				i->set_modtime(time(0));
				m_airwaysdb->save(*i);
				++modcnt;
			}
			if (m_nomodify)
				break;
		}
		if (modcnt) {
			m_airwaysdb->analyze();
			m_airwaysdb->vacuum();
		}
	}
}

void DbFix::zapsidstar(void)
{
	open_airports_db();
	if (!m_airportsdb)
		return;
	unsigned int modcnt(0);
	DbBaseElements::Airport e;
	m_airportsdb->loadfirst(e);
	while (e.is_valid()) {
		unsigned int rcnt(0);
		for (unsigned int i(e.get_nrvfrrte()); i > 0; ) {
			--i;
			const DbBaseElements::Airport::VFRRoute& rte(e.get_vfrrte(i));
			if (rte.size() &&
			    rte[0].get_pathcode() != DbBaseElements::Airport::VFRRoute::VFRRoutePoint::pathcode_star &&
			    rte[rte.size()-1].get_pathcode() != DbBaseElements::Airport::VFRRoute::VFRRoutePoint::pathcode_sid)
				continue;
			e.erase_vfrrte(i);
			++rcnt;
		}
		if (rcnt) {
			if (m_trace)
				std::cerr << "Airport " << e.get_address() << ' ' << e.get_icao_name() << " erased/remaining routes "
					  << rcnt << '/' << e.get_nrvfrrte() << std::endl;
			if (!m_nomodify) {
				e.set_modtime(time(0));
				m_airportsdb->save(e);
				++modcnt;
			}
		}
		m_airportsdb->loadnext(e);
	}
	if (modcnt) {
		m_airportsdb->analyze();
		m_airportsdb->vacuum();
	}
}

int main(int argc, char *argv[])
{
	typedef enum {
		mode_tile,
		mode_zapsidstar,
		mode_none
	} mode_t;
	static struct option long_options[] = {
		{ "dir", required_argument, 0, 'd' },
#ifdef HAVE_PQXX
		{ "pgsql", required_argument, 0, 'p' },
#endif
		{ "trace", no_argument, 0, 't' },
		{ "nomodify", no_argument, 0, 'n' },
		{ "tile", no_argument, 0, 0x400 + mode_tile },
		{ "zapsidstar", no_argument, 0, 0x400 + mode_zapsidstar },
		{0, 0, 0, 0}
	};
	Glib::ustring db_dir(".");
	bool pgsql(false), trace(false), nomodify(false);
	mode_t mode(mode_none);
	int c, err(0);

	while ((c = getopt_long(argc, argv, "d:p:tn", long_options, 0)) != EOF) {
		switch (c) {
		case 'd':
			if (optarg) {
				db_dir = optarg;
				pgsql = false;
			}
			break;

#ifdef HAVE_PQXX
		case 'p':
			if (optarg) {
				db_dir = optarg;
				pgsql = true;
			}
			break;
#endif

		case 't':
			trace = true;
			break;

		case 'n':
			nomodify = true;
			break;

		case 0x400 + mode_tile:
		case 0x400 + mode_zapsidstar:
			mode = (mode_t)(c - 0x400);
			break;

		default:
			++err;
			break;
		}
	}
	if (err) {
		std::cerr << "usage: vfrnavfixdb [-d <dir>] [-p <pgsql>] [-t] [-n] [--tile] [--zapsidstar]" << std::endl;
		return EX_USAGE;
	}
	try {
		DbFix dbf(db_dir, pgsql, trace, nomodify);
		for (; optind < argc; ++optind) {
			std::cerr << "Ignoring file " << argv[optind] << std::endl;
		}
		switch (mode) {
		case mode_tile:
			dbf.tile();
			break;

		case mode_zapsidstar:
			dbf.zapsidstar();
			break;

		default:
			break;
		}
	} catch (const std::exception& ex) {
		std::cerr << "exception: " << ex.what() << std::endl;
		return EX_DATAERR;
	}
	return EX_OK;
}
