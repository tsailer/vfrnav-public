//
// C++ Implementation: dbobjnav
//
// Description: Database Objects: Navaids
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include "dbobj.h"

#ifdef HAVE_PQXX
#include <pqxx/transactor.hxx>
#include <pqxx/result.hxx>
#include <pqxx/except.hxx>
#endif

const unsigned int DbBaseElements::Navaid::subtables_all;
const unsigned int DbBaseElements::Navaid::subtables_none;

const char *DbBaseElements::Navaid::db_query_string = "navaids.ID AS ID,0 AS \"TABLE\",navaids.SRCID AS SRCID,navaids.ICAO,navaids.NAME,navaids.AUTHORITY,navaids.FREQ,navaids.LAT,navaids.LON,navaids.DMELAT,navaids.DMELON,navaids.ELEVATION,navaids.RANGE,navaids.TYPE,navaids.LABELPLACEMENT,navaids.STATUS,navaids.MODIFIED";
const char *DbBaseElements::Navaid::db_aux_query_string = "aux.navaids.ID AS ID,1 AS \"TABLE\",aux.navaids.SRCID AS SRCID,aux.navaids.ICAO,aux.navaids.NAME,aux.navaids.AUTHORITY,aux.navaids.FREQ,aux.navaids.LAT,aux.navaids.LON,aux.navaids.DMELAT,aux.navaids.DMELON,aux.navaids.ELEVATION,aux.navaids.RANGE,aux.navaids.TYPE,aux.navaids.LABELPLACEMENT,aux.navaids.STATUS,aux.navaids.MODIFIED";
const char *DbBaseElements::Navaid::db_text_fields[] = { "SRCID", "ICAO", "NAME", 0 };
const char *DbBaseElements::Navaid::db_time_fields[] = { "MODIFIED", 0 };

DbBaseElements::Navaid::Navaid(void)
	: m_elev(0), m_range(0), m_freq(0), m_navaid_type(navaid_invalid), m_inservice(false)
{
	m_dmecoord.set_invalid();
}

void DbBaseElements::Navaid::load(sqlite3x::sqlite3_cursor & cursor, sqlite3x::sqlite3_connection & db, unsigned int loadsubtables)
{
	m_id = 0;
	m_navaid_type = navaid_invalid;
	m_table = table_main;
	if (!cursor.step())
		return;
	m_id = cursor.getint(0);
	m_table = (table_t)cursor.getint(1);
	m_sourceid = cursor.getstring(2);
	m_icao = cursor.getstring(3);
	m_name = cursor.getstring(4);
	m_authority = cursor.getstring(5);
	m_freq = cursor.getint(6);
	m_coord = Point(cursor.getint(8), cursor.getint(7));
	m_dmecoord = Point(cursor.getint(10), cursor.getint(9));
	m_elev = cursor.getint(11);
	m_range = cursor.getint(12);
	m_navaid_type = (navaid_type_t)cursor.getint(13);
	m_label_placement = (label_placement_t)cursor.getint(14);
	m_inservice = cursor.getint(15);
	m_modtime = cursor.getint(16);
}

void DbBaseElements::Navaid::update_index(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree)
{
	if (m_table != table_main || !m_id)
		return;
	sqlite3x::sqlite3_transaction tran(db);
	{
		sqlite3x::sqlite3_command cmd(db, "UPDATE navaids SET TILE=?2 WHERE ID=?1;");
		cmd.bind(1, (long long int)m_id);
		cmd.bind(2, (int)TileNumber::to_tilenumber(get_coord()));
		cmd.executenonquery();
	}
	if (rtree) {
		sqlite3x::sqlite3_command cmd(db, "UPDATE navaids_rtree SET SWLAT=?,NELAT=?,SWLON=?,NELON=? WHERE ID=?;");
		cmd.bind(1, m_coord.get_lat());
		cmd.bind(2, m_coord.get_lat());
		cmd.bind(3, m_coord.get_lon());
		cmd.bind(4, m_coord.get_lon());
		cmd.bind(5, (long long int)m_id);
		cmd.executenonquery();
	}
	tran.commit();
}

void DbBaseElements::Navaid::save(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree)
{
	sqlite3x::sqlite3_transaction tran(db);
	if (m_table == table_aux) {
		sqlite3x::sqlite3_command cmd(db, "INSERT OR REPLACE INTO navaids_deleted (SRCID) VALUES (?);");
		cmd.bind(1, m_sourceid);
		cmd.executenonquery();
		m_table = table_main;
		m_id = 0;
	}
	if (!m_id) {
		m_id = 1;
		{
			sqlite3x::sqlite3_command cmd(db, "SELECT MAX(ID) FROM navaids;");
			sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
			if (cursor.step())
				m_id = cursor.getint(0) + 1;
		}
		{
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO navaids (ID,SRCID) VALUES(?,?);");
			cmd.bind(1, (long long int)m_id);
			cmd.bind(2, m_sourceid);
			cmd.executenonquery();
		}
		if (rtree) {
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO navaids_rtree (ID,SWLAT,NELAT,SWLON,NELON) VALUES(?,?,?,?,?);");
			cmd.bind(1, (long long int)m_id);
			cmd.bind(2, m_coord.get_lat());
			cmd.bind(3, m_coord.get_lat());
			cmd.bind(4, m_coord.get_lon());
			cmd.bind(5, m_coord.get_lon());
			cmd.executenonquery();
		}
	}
	{
		sqlite3x::sqlite3_command cmd(db, "UPDATE navaids SET SRCID=?,ICAO=?,NAME=?,AUTHORITY=?,FREQ=?,LAT=?,LON=?,DMELAT=?,DMELON=?,ELEVATION=?,RANGE=?,TYPE=?,LABELPLACEMENT=?,STATUS=?,MODIFIED=?,TILE=? WHERE ID=?;");
		cmd.bind(1, m_sourceid);
		cmd.bind(2, m_icao);
		cmd.bind(3, m_name);
		cmd.bind(4, m_authority);
		cmd.bind(5, (long long int)m_freq);
		cmd.bind(6, m_coord.get_lat());
		cmd.bind(7, m_coord.get_lon());
		cmd.bind(8, m_dmecoord.get_lat());
		cmd.bind(9, m_dmecoord.get_lon());
		cmd.bind(10, m_elev);
		cmd.bind(11, m_range);
		cmd.bind(12, m_navaid_type);
		cmd.bind(13, m_label_placement);
		cmd.bind(14, m_inservice);
		cmd.bind(15, (long long int)m_modtime);
		cmd.bind(16, (int)TileNumber::to_tilenumber(m_coord));
		cmd.bind(17, (long long int)m_id);
		cmd.executenonquery();
	}
	if (rtree) {
		sqlite3x::sqlite3_command cmd(db, "UPDATE navaids_rtree SET SWLAT=?,NELAT=?,SWLON=?,NELON=? WHERE ID=?;");
		cmd.bind(1, m_coord.get_lat());
		cmd.bind(2, m_coord.get_lat());
		cmd.bind(3, m_coord.get_lon());
		cmd.bind(4, m_coord.get_lon());
		cmd.bind(5, (long long int)m_id);
		cmd.executenonquery();
	}
	tran.commit();
}

void DbBaseElements::Navaid::erase(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree)
{
	if (!is_valid())
		return;
	sqlite3x::sqlite3_transaction tran(db);
	if (m_table == table_aux) {
		sqlite3x::sqlite3_command cmd(db, "INSERT OR REPLACE INTO navaids_deleted (SRCID) VALUES (?);");
		cmd.bind(1, m_sourceid);
		cmd.executenonquery();
		m_table = table_main;
	} else {
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM navaids WHERE ID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		if (rtree) {
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM navaids_rtree WHERE ID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
	}
	tran.commit();
	m_id = 0;
}

#ifdef HAVE_PQXX

void DbBaseElements::Navaid::load(const pqxx::tuple& cursor, pqxx::basic_transaction& w, unsigned int loadsubtables)
{
	m_id = 0;
	m_navaid_type = navaid_invalid;
	m_table = table_main;
	if (cursor.empty())
		return;
	m_id = cursor[0].as<int64_t>(0);
	m_table = (table_t)cursor[1].as<int>(0);
	m_sourceid = cursor[2].as<std::string>("");
	m_icao = cursor[3].as<std::string>("");
	m_name = cursor[4].as<std::string>("");
	m_authority = cursor[5].as<std::string>("");
	m_freq = cursor[6].as<uint32_t>(0);
	m_coord = Point(cursor[8].as<Point::coord_t>(0), cursor[7].as<Point::coord_t>(0x80000000));
	m_dmecoord = Point(cursor[10].as<Point::coord_t>(0), cursor[9].as<Point::coord_t>(0x80000000));
	m_elev = cursor[11].as<int32_t>(0);
	m_range = cursor[12].as<int32_t>(0);
	m_navaid_type = (navaid_type_t)cursor[13].as<int>((int)navaid_invalid);
	m_label_placement = (label_placement_t)cursor[14].as<int>((int)label_off);
	m_inservice = cursor[15].as<int>(0);
	m_modtime = cursor[16].as<time_t>(0);
}

void DbBaseElements::Navaid::save(pqxx::connection& conn, bool rtree)
{
	pqxx::work w(conn);
	if (!m_id) {
		pqxx::result r(w.exec("INSERT INTO aviationdb.navaids (ID,SRCID) SELECT COALESCE(MAX(ID),0)+1," + w.quote((std::string)m_sourceid) + " FROM aviationdb.navaids RETURNING ID;"));
		m_id = r.front()[0].as<int64_t>();
		if (rtree)
			w.exec("INSERT INTO aviationdb.navaids_rtree (ID,SWLAT,NELAT,SWLON,NELON) VALUES(" + w.quote(m_id) + "," + w.quote(m_coord.get_lat()) +
			       "," + w.quote(m_coord.get_lat()) + "," + w.quote(m_coord.get_lon()) + "," + w.quote(m_coord.get_lon()) + ");");
	}
	w.exec("UPDATE aviationdb.navaids SET SRCID=" + w.quote((std::string)m_sourceid) + ",ICAO=" + w.quote((std::string)m_icao) + ",NAME=" + w.quote((std::string)m_name) + ",AUTHORITY=" + w.quote((std::string)m_authority) +
	       ",FREQ=" + w.quote(m_freq) + ",LAT=" + w.quote(m_coord.get_lat()) + ",LON=" + w.quote(m_coord.get_lon()) +
	       ",DMELAT=" + w.quote(m_dmecoord.get_lat()) + ",DMELON=" + w.quote(m_dmecoord.get_lon()) + ",ELEVATION=" + w.quote(m_elev) +
	       ",RANGE=" + w.quote(m_range) + ",TYPE=" + w.quote((int)m_navaid_type) + ",LABELPLACEMENT=" + w.quote((int)m_label_placement) +
	       ",STATUS=" + w.quote((int)m_inservice) + ",MODIFIED=" + w.quote(m_modtime) + ",TILE=" + w.quote(TileNumber::to_tilenumber(m_coord)) +
	       " WHERE ID=" + w.quote(m_id) + ";");
	if (rtree)
		w.exec("UPDATE aviationdb.navaids_rtree SET SWLAT=" + w.quote(m_coord.get_lat()) + ",NELAT=" + w.quote(m_coord.get_lat()) +
		       ",SWLON=" + w.quote(m_coord.get_lon()) + ",NELON=" + w.quote(m_coord.get_lon()) + " WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
}

void DbBaseElements::Navaid::erase(pqxx::connection& conn, bool rtree)
{
	if (!is_valid())
		return;
	pqxx::work w(conn);
	w.exec("DELETE FROM aviationdb.navaids WHERE ID=" + w.quote(m_id) + ";");
	if (rtree)
		w.exec("DELETE FROM aviationdb.navaids_rtree WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
	m_id = 0;
}

void DbBaseElements::Navaid::update_index(pqxx::connection& conn, bool rtree)
{
	if (!m_id)
		return;
	pqxx::work w(conn);
	w.exec("UPDATE aviationdb.navaids SET TILE=" + w.quote(TileNumber::to_tilenumber(get_coord())) + " WHERE ID=" + w.quote(m_id) + ";");
	if (rtree)
		w.exec("UPDATE aviationdb.navaids_rtree SET SWLAT=" + w.quote(m_coord.get_lat()) + ",NELAT=" + w.quote(m_coord.get_lat()) +
		       ",SWLON=" + w.quote(m_coord.get_lon()) + ",NELON=" + w.quote(m_coord.get_lon()) + " WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
}

#endif

const Glib::ustring& DbBaseElements::Navaid::get_navaid_typename(navaid_type_t nt)
{
	switch (nt) {
		default:
		{
			static const Glib::ustring r("Invalid");
			return r;
		}

		case navaid_vor:
		{
			static const Glib::ustring r("VOR");
			return r;
		}

		case navaid_vordme:
		{
			static const Glib::ustring r("VORDME");
			return r;
		}

		case navaid_dme:
		{
			static const Glib::ustring r("DME");
			return r;
		}

		case navaid_vortac:
		{
			static const Glib::ustring r("VORTAC");
			return r;
		}

		case navaid_tacan:
		{
			static const Glib::ustring r("TACAN");
			return r;
		}

		case navaid_ndb:
		{
			static const Glib::ustring r("NDB");
			return r;
		}

		case navaid_ndbdme:
		{
			static const Glib::ustring r("NDBDME");
			return r;
		}
	}
}

bool DbBaseElements::Navaid::has_ndb(navaid_type_t nt)
{
	return nt == navaid_ndb || nt == navaid_ndbdme;
}

bool DbBaseElements::Navaid::has_vor(navaid_type_t nt)
{
	return nt == navaid_vor || nt == navaid_vordme || nt == navaid_vortac;
}

bool DbBaseElements::Navaid::has_dme(navaid_type_t nt)
{
	return nt == navaid_dme || nt == navaid_vordme || nt == navaid_tacan || nt == navaid_vortac || nt == navaid_ndbdme;
}

template<> void DbBase<DbBaseElements::Navaid>::drop_tables(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS navaids;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS navaids_deleted;"); cmd.executenonquery(); }
}

template<> void DbBase<DbBaseElements::Navaid>::create_tables(void)
{
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS navaids (ID INTEGER PRIMARY KEY NOT NULL, "
					      "SRCID TEXT UNIQUE NOT NULL,"
					      "ICAO TEXT,"
					      "NAME TEXT,"
					      "AUTHORITY TEXT,"
					      "FREQ INTEGER,"
					      "LAT INTEGER,"
					      "LON INTEGER,"
					      "DMELAT INTEGER,"
					      "DMELON INTEGER,"
					      "ELEVATION INTEGER,"
					      "RANGE INTEGER,"
					      "TYPE INTEGER,"
					      "LABELPLACEMENT INTEGER,"
					      "STATUS INTEGER,"
					      "MODIFIED INTEGER,"
					      "TILE INTEGER);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS navaids_deleted(SRCID TEXT PRIMARY KEY NOT NULL);");
		cmd.executenonquery();
	}
}

template<> void DbBase<DbBaseElements::Navaid>::drop_indices(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS navaids_srcid;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS navaids_icao;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS navaids_name;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS navaids_freq;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS navaids_latlon;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS navaids_modified;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS navaids_tile;"); cmd.executenonquery(); }
	// old indices
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS navaids_lat;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS navaids_lon;"); cmd.executenonquery(); }
}

template<> void DbBase<DbBaseElements::Navaid>::create_indices(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS navaids_srcid ON navaids(SRCID);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS navaids_icao ON navaids(ICAO COLLATE NOCASE);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS navaids_name ON navaids(NAME COLLATE NOCASE);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS navaids_freq ON navaids(FREQ);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS navaids_latlon ON navaids(LAT,LON);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS navaids_modified ON navaids(MODIFIED);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS navaids_tile ON navaids(TILE);"); cmd.executenonquery(); }
	// old indices
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS navaids_lat ON navaids(LAT);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS navaids_lon ON navaids(LON);"); cmd.executenonquery(); }
}

template<> const char *DbBase<DbBaseElements::Navaid>::main_table_name = "navaids";
template<> const char *DbBase<DbBaseElements::Navaid>::database_file_name = "navaids.db";
template<> const char *DbBase<DbBaseElements::Navaid>::order_field = "SRCID";
template<> const char *DbBase<DbBaseElements::Navaid>::delete_field = "SRCID";
template<> const bool DbBase<DbBaseElements::Navaid>::area_data = false;

#ifdef HAVE_PQXX

template<> void PGDbBase<DbBaseElements::Navaid>::drop_tables(void)
{
	pqxx::work w(m_conn);
	w.exec("DROP TABLE IF EXISTS aviationdb.navaids;");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Navaid>::create_tables(void)
{
	create_common_tables();
	pqxx::work w(m_conn);
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.navaids ("
	       "FREQ BIGINT,"
	       "DMELAT INTEGER,"
	       "DMELON INTEGER,"
	       "ELEVATION INTEGER,"
	       "RANGE INTEGER,"
	       "TYPE INTEGER,"
	       "STATUS INTEGER,"
	       "TILE INTEGER,"
	       "PRIMARY KEY (ID)"
	       ") INHERITS (aviationdb.labelsourcecoordicaonameauthoritybase);");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Navaid>::drop_indices(void)
{
	pqxx::work w(m_conn);
	w.exec("DROP INDEX IF EXISTS aviationdb.navaids_srcid;");
	w.exec("DROP INDEX IF EXISTS aviationdb.navaids_icao;");
	w.exec("DROP INDEX IF EXISTS aviationdb.navaids_name;");
	w.exec("DROP INDEX IF EXISTS aviationdb.navaids_freq;");
	w.exec("DROP INDEX IF EXISTS aviationdb.navaids_latlon;");
	w.exec("DROP INDEX IF EXISTS aviationdb.navaids_modified;");
	w.exec("DROP INDEX IF EXISTS aviationdb.navaids_tile;");
	// old indices
	w.exec("DROP INDEX IF EXISTS aviationdb.navaids_lat;");
	w.exec("DROP INDEX IF EXISTS aviationdb.navaids_lon;");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Navaid>::create_indices(void)
{
	pqxx::work w(m_conn);
	w.exec("CREATE INDEX IF NOT EXISTS navaids_srcid ON aviationdb.navaids(SRCID);");
	w.exec("CREATE INDEX IF NOT EXISTS navaids_icao ON aviationdb.navaids((ICAO::ext.CITEXT) COLLATE \"C\");");
	w.exec("CREATE INDEX IF NOT EXISTS navaids_name ON aviationdb.navaids((NAME::ext.CITEXT) COLLATE \"C\");");
	w.exec("CREATE INDEX IF NOT EXISTS navaids_freq ON aviationdb.navaids(FREQ);");
	w.exec("CREATE INDEX IF NOT EXISTS navaids_latlon ON aviationdb.navaids(LAT,LON);");
	w.exec("CREATE INDEX IF NOT EXISTS navaids_modified ON aviationdb.navaids(MODIFIED);");
	w.exec("CREATE INDEX IF NOT EXISTS navaids_tile ON aviationdb.navaids(TILE);");
	// old indices
	w.exec("CREATE INDEX IF NOT EXISTS navaids_lat ON aviationdb.navaids(LAT);");
	w.exec("CREATE INDEX IF NOT EXISTS navaids_lon ON aviationdb.navaids(LON);");
	w.commit();
}

template<> const char *PGDbBase<DbBaseElements::Navaid>::main_table_name = "aviationdb.navaids";
template<> const char *PGDbBase<DbBaseElements::Navaid>::order_field = "SRCID";
template<> const char *PGDbBase<DbBaseElements::Navaid>::delete_field = "SRCID";
template<> const bool PGDbBase<DbBaseElements::Navaid>::area_data = false;

#endif
