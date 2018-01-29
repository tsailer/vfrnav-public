//
// C++ Implementation: dbobjwpt
//
// Description: Database Objects: Waypoints (Intersections)
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012, 2016, 2017
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

const char DbBaseElements::Waypoint::usage_invalid;
const char DbBaseElements::Waypoint::usage_highlevel;
const char DbBaseElements::Waypoint::usage_lowlevel;
const char DbBaseElements::Waypoint::usage_bothlevel;
const char DbBaseElements::Waypoint::usage_rnav;
const char DbBaseElements::Waypoint::usage_terminal;
const char DbBaseElements::Waypoint::usage_vfr;
const char DbBaseElements::Waypoint::usage_user;

const char DbBaseElements::Waypoint::type_invalid;
const char DbBaseElements::Waypoint::type_unknown;
const char DbBaseElements::Waypoint::type_adhp;
const char DbBaseElements::Waypoint::type_icao;
const char DbBaseElements::Waypoint::type_coord;
const char DbBaseElements::Waypoint::type_other;

const unsigned int DbBaseElements::Waypoint::subtables_all;
const unsigned int DbBaseElements::Waypoint::subtables_none;

const char *DbBaseElements::Waypoint::db_query_string = "waypoints.ID AS ID,0 AS \"TABLE\",waypoints.SRCID AS SRCID,waypoints.ICAO,waypoints.NAME,waypoints.AUTHORITY,waypoints.USAGE,waypoints.TYPE,waypoints.LAT,waypoints.LON,waypoints.LABELPLACEMENT,waypoints.MODIFIED";
const char *DbBaseElements::Waypoint::db_aux_query_string = "aux.waypoints.ID AS ID,1 AS \"TABLE\",aux.waypoints.SRCID AS SRCID,aux.waypoints.ICAO,aux.waypoints.NAME,aux.waypoints.AUTHORITY,aux.waypoints.USAGE,aux.waypoints.TYPE,aux.waypoints.LAT,aux.waypoints.LON,aux.waypoints.LABELPLACEMENT,aux.waypoints.MODIFIED";
const char *DbBaseElements::Waypoint::db_text_fields[] = { "SRCID", "ICAO", "NAME", 0 };
const char *DbBaseElements::Waypoint::db_time_fields[] = { "MODIFIED", 0 };

DbBaseElements::Waypoint::Waypoint(void)
	: m_usage(usage_invalid), m_type(type_invalid)
{
}

void DbBaseElements::Waypoint::load(sqlite3x::sqlite3_cursor & cursor, sqlite3x::sqlite3_connection & db, unsigned int loadsubtables)
{
	m_id = 0;
	m_table = table_main;
	m_usage = usage_invalid;
	m_type = type_invalid;
	if (!cursor.step())
		return;
	m_id = cursor.getint(0);
	m_table = (table_t)cursor.getint(1);
	m_sourceid = cursor.getstring(2);
	m_icao = cursor.getstring(3);
	m_name = cursor.getstring(4);
	m_authority = cursor.getstring(5);
	m_usage = cursor.getint(6);
	m_type = cursor.getint(7);
	m_coord = Point(cursor.getint(9), cursor.getint(8));
	m_label_placement = (label_placement_t)cursor.getint(10);
	m_modtime = cursor.getint(11);
}

void DbBaseElements::Waypoint::update_index(sqlite3x::sqlite3_connection & db, bool rtree, bool aux_rtree)
{
	if (m_table != table_main || !m_id)
		return;
	sqlite3x::sqlite3_transaction tran(db);
	{
		sqlite3x::sqlite3_command cmd(db, "UPDATE waypoints SET TILE=?2 WHERE ID=?1;");
		cmd.bind(1, (long long int)m_id);
		cmd.bind(2, (int)TileNumber::to_tilenumber(get_coord()));
		cmd.executenonquery();
	}
	if (rtree) {
		sqlite3x::sqlite3_command cmd(db, "UPDATE waypoints_rtree SET SWLAT=?,NELAT=?,SWLON=?,NELON=? WHERE ID=?;");
		cmd.bind(1, m_coord.get_lat());
		cmd.bind(2, m_coord.get_lat());
		cmd.bind(3, m_coord.get_lon());
		cmd.bind(4, m_coord.get_lon());
		cmd.bind(5, (long long int)m_id);
		cmd.executenonquery();
	}
	tran.commit();
}

void DbBaseElements::Waypoint::save(sqlite3x::sqlite3_connection & db, bool rtree, bool aux_rtree)
{
	sqlite3x::sqlite3_transaction tran(db);
	if (m_table == table_aux) {
		sqlite3x::sqlite3_command cmd(db, "INSERT OR REPLACE INTO waypoints_deleted (SRCID) VALUES (?);");
		cmd.bind(1, m_sourceid);
		cmd.executenonquery();
		m_table = table_main;
		m_id = 0;
	}
	if (!m_id) {
		m_id = 1;
		{
			sqlite3x::sqlite3_command cmd(db, "SELECT MAX(ID) FROM waypoints;");
			sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
			if (cursor.step())
				m_id = cursor.getint(0) + 1;
		}
		{
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO waypoints (ID,SRCID) VALUES(?,?);");
			cmd.bind(1, (long long int)m_id);
			cmd.bind(2, m_sourceid);
			cmd.executenonquery();
		}
		if (rtree) {
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO waypoints_rtree (ID,SWLAT,NELAT,SWLON,NELON) VALUES(?,?,?,?,?);");
			cmd.bind(1, (long long int)m_id);
			cmd.bind(2, m_coord.get_lat());
			cmd.bind(3, m_coord.get_lat());
			cmd.bind(4, m_coord.get_lon());
			cmd.bind(5, m_coord.get_lon());
			cmd.executenonquery();
		}
	}
	{
		sqlite3x::sqlite3_command cmd(db, "UPDATE waypoints SET SRCID=?,ICAO=?,NAME=?,AUTHORITY=?,USAGE=?,TYPE=?,LAT=?,LON=?,LABELPLACEMENT=?,MODIFIED=?,TILE=? WHERE ID=?;");
		cmd.bind(1, m_sourceid);
		cmd.bind(2, m_icao);
		cmd.bind(3, m_name);
		cmd.bind(4, m_authority);
		cmd.bind(5, (int)m_usage);
		cmd.bind(6, (int)m_type);
		cmd.bind(7, m_coord.get_lat());
		cmd.bind(8, m_coord.get_lon());
		cmd.bind(9, m_label_placement);
		cmd.bind(10, (long long int)m_modtime);
		cmd.bind(11, (int)TileNumber::to_tilenumber(m_coord));
		cmd.bind(12, (long long int)m_id);
		cmd.executenonquery();
	}
	if (rtree) {
		sqlite3x::sqlite3_command cmd(db, "UPDATE waypoints_rtree SET SWLAT=?,NELAT=?,SWLON=?,NELON=? WHERE ID=?;");
		cmd.bind(1, m_coord.get_lat());
		cmd.bind(2, m_coord.get_lat());
		cmd.bind(3, m_coord.get_lon());
		cmd.bind(4, m_coord.get_lon());
		cmd.bind(5, (long long int)m_id);
		cmd.executenonquery();
	}
	tran.commit();
}

void DbBaseElements::Waypoint::erase(sqlite3x::sqlite3_connection & db, bool rtree, bool aux_rtree)
{
	if (!is_valid())
		return;
	sqlite3x::sqlite3_transaction tran(db);
	if (m_table == table_aux) {
		sqlite3x::sqlite3_command cmd(db, "INSERT OR REPLACE INTO waypoints_deleted (SRCID) VALUES (?);");
		cmd.bind(1, m_sourceid);
		cmd.executenonquery();
		m_table = table_main;
	} else {
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM waypoints WHERE ID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		if (rtree) {
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM waypoints_rtree WHERE ID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
	}
	tran.commit();
	m_id = 0;
}

#ifdef HAVE_PQXX

void DbBaseElements::Waypoint::load(const pqxx::tuple& cursor, pqxx::basic_transaction& w, unsigned int loadsubtables)
{
	m_id = 0;
	m_table = table_main;
	m_usage = usage_invalid;
	m_type = type_invalid;
	if (cursor.empty())
		return;
	m_id = cursor[0].as<int64_t>(0);
	m_table = (table_t)cursor[1].as<int>(0);
	m_sourceid = cursor[2].as<std::string>("");
	m_icao = cursor[3].as<std::string>("");
	m_name = cursor[4].as<std::string>("");
	m_authority = cursor[5].as<std::string>("");
	m_usage = cursor[6].as<int>((int)usage_invalid);
	m_type = cursor[7].as<int>((int)type_invalid);
	m_coord = Point(cursor[9].as<Point::coord_t>(0), cursor[8].as<Point::coord_t>(0x80000000));
	m_label_placement = (label_placement_t)cursor[10].as<int>((int)label_off);
	m_modtime = cursor[11].as<time_t>(0);
}

void DbBaseElements::Waypoint::save(pqxx::connection_base& conn, bool rtree)
{
	pqxx::work w(conn);
	if (!m_id) {
		pqxx::result r(w.exec("INSERT INTO aviationdb.waypoints (ID,SRCID) SELECT COALESCE(MAX(ID),0)+1," + w.quote((std::string)m_sourceid) + " FROM aviationdb.waypoints RETURNING ID;"));
		m_id = r.front()[0].as<int64_t>();
		if (rtree)
			w.exec("INSERT INTO aviationdb.waypoints_rtree (ID,SWLAT,NELAT,SWLON,NELON) VALUES(" + w.quote(m_id) + "," + w.quote(m_coord.get_lat()) +
			       "," + w.quote(m_coord.get_lat()) + "," + w.quote(m_coord.get_lon()) + "," + w.quote(m_coord.get_lon()) + ");");
	}
	w.exec("UPDATE aviationdb.waypoints SET SRCID=" + w.quote((std::string)m_sourceid) + ",ICAO=" + w.quote((std::string)m_icao) + ",NAME=" + w.quote((std::string)m_name) + ",AUTHORITY=" + w.quote((std::string)m_authority) +
	       ",USAGE=" + w.quote((int)m_usage) + ",TYPE=" + w.quote((int)m_type) +
	       ",LAT=" + w.quote(m_coord.get_lat()) + ",LON=" + w.quote(m_coord.get_lon()) +
	       ",LABELPLACEMENT=" + w.quote((int)m_label_placement) + ",MODIFIED=" + w.quote(m_modtime) +
	       ",TILE=" + w.quote(TileNumber::to_tilenumber(m_coord)) + " WHERE ID=" + w.quote(m_id) + ";");
	if (rtree)
		w.exec("UPDATE aviationdb.waypoints_rtree SET SWLAT=" + w.quote(m_coord.get_lat()) + ",NELAT=" + w.quote(m_coord.get_lat()) +
		       ",SWLON=" + w.quote(m_coord.get_lon()) + ",NELON=" + w.quote(m_coord.get_lon()) + " WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
}

void DbBaseElements::Waypoint::erase(pqxx::connection_base& conn, bool rtree)
{
	if (!is_valid())
		return;
	pqxx::work w(conn);
	w.exec("DELETE FROM aviationdb.waypoints WHERE ID=" + w.quote(m_id) + ";");
	if (rtree)
		w.exec("DELETE FROM aviationdb.waypoints_rtree WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
	m_id = 0;
}

void DbBaseElements::Waypoint::update_index(pqxx::connection_base& conn, bool rtree)
{
	if (!m_id)
		return;
	pqxx::work w(conn);
	w.exec("UPDATE aviationdb.waypoints SET TILE=" + w.quote(TileNumber::to_tilenumber(get_coord())) + " WHERE ID=" + w.quote(m_id) + ";");
	if (rtree)
		w.exec("UPDATE aviationdb.waypoints_rtree SET SWLAT=" + w.quote(m_coord.get_lat()) + ",NELAT=" + w.quote(m_coord.get_lat()) +
		       ",SWLON=" + w.quote(m_coord.get_lon()) + ",NELON=" + w.quote(m_coord.get_lon()) + " WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
}

#endif

const Glib::ustring& DbBaseElements::Waypoint::get_usage_name( char usage )
{
	switch (usage) {
	default:
	case usage_invalid:
	{
		static const Glib::ustring r("Invalid");
		return r;
	}

	case usage_highlevel:
	{
		static const Glib::ustring r("High Level");
		return r;
	}

	case usage_lowlevel:
	{
		static const Glib::ustring r("Low Level");
		return r;
	}

	case usage_bothlevel:
	{
		static const Glib::ustring r("Low and High Level");
		return r;
	}

	case usage_rnav:
	{
		static const Glib::ustring r("RNAV");
		return r;
	}

	case usage_terminal:
	{
		static const Glib::ustring r("Terminal");
		return r;
	}

	case usage_vfr:
	{
		static const Glib::ustring r("VFR");
		return r;
	}

	case usage_user:
	{
		static const Glib::ustring r("User");
		return r;
	}
	}
}

const Glib::ustring& DbBaseElements::Waypoint::get_type_name(char t)
{
	switch (t) {
	default:
	case type_invalid:
	{
		static const Glib::ustring r("Invalid");
		return r;
	}

	case type_unknown:
	{
		static const Glib::ustring r("Unknown");
		return r;
	}

	case type_adhp:
	{
		static const Glib::ustring r("ADHP");
		return r;
	}

	case type_icao:
	{
		static const Glib::ustring r("ICAO");
		return r;
	}

	case type_coord:
	{
		static const Glib::ustring r("COORD");
		return r;
	}

	case type_other:
	{
		static const Glib::ustring r("Other");
		return r;
	}
	}
}

template<> void DbBase<DbBaseElements::Waypoint>::drop_tables(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS waypoints;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS waypoints_deleted;"); cmd.executenonquery(); }
}

template<> void DbBase<DbBaseElements::Waypoint>::create_tables(void)
{
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS waypoints (ID INTEGER PRIMARY KEY NOT NULL, "
					      "SRCID TEXT UNIQUE NOT NULL,"
					      "ICAO TEXT,"
					      "NAME TEXT,"
					      "AUTHORITY TEXT,"
					      "USAGE CHAR,"
					      "TYPE CHAR,"
					      "LAT INTEGER,"
					      "LON INTEGER,"
					      "LABELPLACEMENT INTEGER,"
					      "MODIFIED INTEGER,"
					      "TILE INTEGER);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS waypoints_deleted(SRCID TEXT PRIMARY KEY NOT NULL);");
		cmd.executenonquery();
	}
}

template<> void DbBase<DbBaseElements::Waypoint>::drop_indices(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS waypoints_srcid;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS waypoints_icao;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS waypoints_name;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS waypoints_latlon;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS waypoints_modified;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS waypoints_tile;"); cmd.executenonquery(); }
	// old indices
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS waypoints_lat;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS waypoints_lon;"); cmd.executenonquery(); }
}

template<> void DbBase<DbBaseElements::Waypoint>::create_indices(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS waypoints_srcid ON waypoints(SRCID);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS waypoints_icao ON waypoints(ICAO COLLATE NOCASE);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS waypoints_name ON waypoints(NAME COLLATE NOCASE);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS waypoints_latlon ON waypoints(LAT,LON);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS waypoints_modified ON waypoints(MODIFIED);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS waypoints_tile ON waypoints(TILE);"); cmd.executenonquery(); }
	// old indices
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS waypoints_lat ON waypoints(LAT);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS waypoints_lon ON waypoints(LON);"); cmd.executenonquery(); }
}

template<> const char *DbBase<DbBaseElements::Waypoint>::main_table_name = "waypoints";
template<> const char *DbBase<DbBaseElements::Waypoint>::database_file_name = "waypoints.db";
template<> const char *DbBase<DbBaseElements::Waypoint>::order_field = "SRCID";
template<> const char *DbBase<DbBaseElements::Waypoint>::delete_field = "SRCID";
template<> const bool DbBase<DbBaseElements::Waypoint>::area_data = false;

#ifdef HAVE_PQXX

template<> void PGDbBase<DbBaseElements::Waypoint>::drop_tables(void)
{
	pqxx::work w(m_conn);
	w.exec("DROP TABLE IF EXISTS aviationdb.waypoints;");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Waypoint>::create_tables(void)
{
	create_common_tables();
	pqxx::work w(m_conn);
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.waypoints ("
	       "USAGE SMALLINT,"
	       "TYPE SMALLINT,"
	       "TILE INTEGER,"
	       "PRIMARY KEY (ID)"
	       ") INHERITS (aviationdb.labelsourcecoordicaonameauthoritybase);");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Waypoint>::drop_indices(void)
{
	pqxx::work w(m_conn);
	w.exec("DROP INDEX IF EXISTS aviationdb.waypoints_srcid;");
	w.exec("DROP INDEX IF EXISTS aviationdb.waypoints_icao;");
	w.exec("DROP INDEX IF EXISTS aviationdb.waypoints_name;");
	w.exec("DROP INDEX IF EXISTS aviationdb.waypoints_latlon;");
	w.exec("DROP INDEX IF EXISTS aviationdb.waypoints_modified;");
	w.exec("DROP INDEX IF EXISTS aviationdb.waypoints_tile;");
	// old indices
	w.exec("DROP INDEX IF EXISTS aviationdb.waypoints_lat;");
	w.exec("DROP INDEX IF EXISTS aviationdb.waypoints_lon;");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Waypoint>::create_indices(void)
{
	pqxx::work w(m_conn);
	w.exec("CREATE INDEX IF NOT EXISTS waypoints_srcid ON aviationdb.waypoints(SRCID);");
	w.exec("CREATE INDEX IF NOT EXISTS waypoints_icao ON aviationdb.waypoints((ICAO::ext.CITEXT) COLLATE \"C\");");
	w.exec("CREATE INDEX IF NOT EXISTS waypoints_name ON aviationdb.waypoints((NAME::ext.CITEXT) COLLATE \"C\");");
	w.exec("CREATE INDEX IF NOT EXISTS waypoints_latlon ON aviationdb.waypoints(LAT,LON);");
	w.exec("CREATE INDEX IF NOT EXISTS waypoints_modified ON aviationdb.waypoints(MODIFIED);");
	w.exec("CREATE INDEX IF NOT EXISTS waypoints_tile ON aviationdb.waypoints(TILE);");
	// old indices
	w.exec("CREATE INDEX IF NOT EXISTS waypoints_lat ON aviationdb.waypoints(LAT);");
	w.exec("CREATE INDEX IF NOT EXISTS waypoints_lon ON aviationdb.waypoints(LON);");
	w.commit();
}

template<> const char *PGDbBase<DbBaseElements::Waypoint>::main_table_name = "aviationdb.waypoints";
template<> const char *PGDbBase<DbBaseElements::Waypoint>::order_field = "SRCID";
template<> const char *PGDbBase<DbBaseElements::Waypoint>::delete_field = "SRCID";
template<> const bool PGDbBase<DbBaseElements::Waypoint>::area_data = false;

#endif
