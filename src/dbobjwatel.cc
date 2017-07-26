//
// C++ Implementation: dbobjwatel
//
// Description: Database Objects: Waterelements
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012, 2015, 2016
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

const unsigned int DbBaseElements::Waterelement::subtables_all;
const unsigned int DbBaseElements::Waterelement::subtables_none;

const char *DbBaseElements::Waterelement::db_query_string = "waterelements.ID AS ID,0 AS \"TABLE\",waterelements.TYPE,waterelements.SWLAT,waterelements.SWLON,waterelements.NELAT,waterelements.NELON,waterelements.POLY";
const char *DbBaseElements::Waterelement::db_aux_query_string = "aux.waterelements.ID AS ID,1 AS \"TABLE\",aux.waterelements.TYPE,aux.waterelements.SWLAT,aux.waterelements.SWLON,aux.waterelements.NELAT,aux.waterelements.NELON,aux.waterelements.POLY";
const char *DbBaseElements::Waterelement::db_text_fields[] = { 0 };
const char *DbBaseElements::Waterelement::db_time_fields[] = { 0 };

DbBaseElements::Waterelement::Waterelement(void)
	: m_id(0), m_bbox(), m_type(type_invalid), m_table(table_main), m_polygon()
{
}

DbBaseElements::Waterelement::Waterelement(type_t t, const PolygonHole & po)
	: m_id(0), m_bbox(), m_type(t), m_table(table_main), m_polygon(po)
{
	m_bbox = m_polygon.get_bbox();
}

void DbBaseElements::Waterelement::load( sqlite3x::sqlite3_cursor & cursor, sqlite3x::sqlite3_connection & db, unsigned int loadsubtables )
{
	m_id = 0;
	m_table = table_main;
	m_type = type_invalid;
	if (!cursor.step())
		return;
	m_id = cursor.getint(0);
	m_table = (table_t)cursor.getint(1);
	m_type = (type_t)cursor.getint(2);
	m_bbox = Rect(Point(cursor.getint(4), cursor.getint(3)), Point(cursor.getint(6), cursor.getint(5)));
	m_polygon.getblob(cursor, 7);
	recompute_bbox();
}

void DbBaseElements::Waterelement::update_index(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree)
{
	if (m_table != table_main || !m_id)
		return;
	if (false)
		std::cerr << "Start transaction" << std::endl;
	sqlite3x::sqlite3_transaction tran(db);
	{
		sqlite3x::sqlite3_command cmd(db, "UPDATE waterelements SET TILE=?2 WHERE ID=?1;");
		cmd.bind(1, (long long int)m_id);
		cmd.bind(2, (int)TileNumber::to_tilenumber(m_bbox));
		cmd.executenonquery();
	}
	if (rtree) {
		sqlite3x::sqlite3_command cmd(db, "UPDATE waterelements_rtree SET SWLAT=?,NELAT=?,SWLON=?,NELON=? WHERE ID=?;");
		cmd.bind(1, m_bbox.get_south());
		cmd.bind(2, m_bbox.get_north());
		cmd.bind(3, m_bbox.get_west());
		cmd.bind(4, (long long int)m_bbox.get_east_unwrapped());
		cmd.bind(5, (long long int)m_id);
		cmd.executenonquery();
	}
	if (false)
		std::cerr << "Commit transaction" << std::endl;
	tran.commit();
}

void DbBaseElements::Waterelement::save(sqlite3x::sqlite3_connection & db, bool rtree, bool aux_rtree)
{
	sqlite3x::sqlite3_transaction tran(db);
	if (m_table == table_aux) {
		sqlite3x::sqlite3_command cmd(db, "INSERT OR REPLACE INTO waterelements_deleted (ID) VALUES (?);");
		cmd.bind(1, (long long int)m_id);
		cmd.executenonquery();
		m_table = table_main;
		m_id = 0;
	}
	if (!m_id) {
		m_id = 1;
		{
			sqlite3x::sqlite3_command cmd(db, "SELECT MAX(ID) FROM waterelements;");
			sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
			if (cursor.step())
				m_id = cursor.getint(0) + 1;
		}
		{
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO waterelements (ID) VALUES(?);");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		if (rtree) {
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO waterelements_rtree (ID,SWLAT,NELAT,SWLON,NELON) VALUES(?,?,?,?,?);");
			cmd.bind(1, (long long int)m_id);
			cmd.bind(2, m_bbox.get_south());
			cmd.bind(3, m_bbox.get_north());
			cmd.bind(4, m_bbox.get_west());
			cmd.bind(5, (long long int)m_bbox.get_east_unwrapped());
			cmd.executenonquery();
		}
	}
	// Update Waterelement Record
	{
		sqlite3x::sqlite3_command cmd(db, "UPDATE waterelements SET TYPE=?,SWLAT=?,SWLON=?,NELAT=?,NELON=?,POLY=?,TILE=? WHERE ID=?;");
		cmd.bind(1, (int)m_type);
		cmd.bind(2, m_bbox.get_south());
		cmd.bind(3, m_bbox.get_west());
		cmd.bind(4, m_bbox.get_north());
		cmd.bind(5, (long long int)m_bbox.get_east_unwrapped());
		m_polygon.bindblob(cmd, 6);
		cmd.bind(7, (int)TileNumber::to_tilenumber(m_bbox));
		cmd.bind(8, (long long int)m_id);
		cmd.executenonquery();
	}
	if (rtree) {
		sqlite3x::sqlite3_command cmd(db, "UPDATE waterelements_rtree SET SWLAT=?,NELAT=?,SWLON=?,NELON=? WHERE ID=?;");
		cmd.bind(1, m_bbox.get_south());
		cmd.bind(2, m_bbox.get_north());
		cmd.bind(3, m_bbox.get_west());
		cmd.bind(4, (long long int)m_bbox.get_east_unwrapped());
		cmd.bind(5, (long long int)m_id);
		cmd.executenonquery();
	}
	tran.commit();
}

void DbBaseElements::Waterelement::erase(sqlite3x::sqlite3_connection & db, bool rtree, bool aux_rtree)
{
	if (!is_valid())
		return;
	sqlite3x::sqlite3_transaction tran(db);
	if (m_table == table_aux) {
		sqlite3x::sqlite3_command cmd(db, "INSERT OR REPLACE INTO waterelements_deleted (ID) VALUES (?);");
		cmd.bind(1, (long long int)m_id);
		cmd.executenonquery();
		m_table = table_main;
	} else {
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM waterelements WHERE ID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		if (rtree) {
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM waterelements_rtree WHERE ID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
	}
	tran.commit();
	m_id = 0;
}

#ifdef HAVE_PQXX

void DbBaseElements::Waterelement::load(const pqxx::tuple& cursor, pqxx::basic_transaction& w, unsigned int loadsubtables)
{
	m_id = 0;
	m_table = table_main;
	m_type = type_invalid;
	if (cursor.empty())
		return;
	m_id = cursor[0].as<int64_t>(0);
	m_table = (table_t)cursor[1].as<int>(0);
	m_type = (type_t)cursor[2].as<int>((int)type_invalid);
	m_bbox = Rect(Point(cursor[4].as<Point::coord_t>(0), cursor[3].as<Point::coord_t>(0x80000000)),
		      Point(cursor[6].as<int64_t>(0), cursor[5].as<Point::coord_t>(0x80000000)));
	m_polygon.getblob(pqxx::binarystring(cursor[7]));
	recompute_bbox();
}

void DbBaseElements::Waterelement::save(pqxx::connection& conn, bool rtree)
{
	pqxx::work w(conn);
	if (!m_id) {
		pqxx::result r(w.exec("INSERT INTO aviationdb.waterelements (ID) SELECT COALESCE(MAX(ID),0)+1 FROM aviationdb.waterelements RETURNING ID;"));
		m_id = r.front()[0].as<int64_t>();
		if (rtree)
			w.exec("INSERT INTO aviationdb.waterelements_rtree (ID,SWLAT,NELAT,SWLON,NELON) VALUES(" + w.quote(m_id) + "," + w.quote(m_bbox.get_south()) +
			       "," + w.quote(m_bbox.get_north()) + "," + w.quote(m_bbox.get_west()) + "," + w.quote(m_bbox.get_east_unwrapped()) + ");");
	}
	// Update Waterelement Record
	{
		pqxx::binarystring poly(m_polygon.bindblob());
		w.exec("UPDATE aviationdb.waterelements SET TYPE=" + w.quote((int)m_type) +
		       ",SWLAT=" + w.quote(m_bbox.get_south()) + ",SWLON=" + w.quote(m_bbox.get_west()) +
		       ",NELAT=" + w.quote(m_bbox.get_north()) + ",NELON=" + w.quote((long long int)m_bbox.get_east_unwrapped()) +
		       ",POLY=" + w.quote(poly) + ",GPOLY=" + PGDbBaseCommon::get_str(m_polygon) +
		       ",TILE=" + w.quote(TileNumber::to_tilenumber(m_bbox)) + " WHERE ID=" + w.quote(m_id) + ";");
	}
	if (rtree)
		w.exec("UPDATE aviationdb.waterelements_rtree SET SWLAT=" + w.quote(m_bbox.get_south()) + ",NELAT=" + w.quote(m_bbox.get_north()) +
		       ",SWLON=" + w.quote(m_bbox.get_west()) + ",NELON=" + w.quote((long long int)m_bbox.get_east_unwrapped()) + " WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
}

void DbBaseElements::Waterelement::erase(pqxx::connection& conn, bool rtree)
{
	if (!is_valid())
		return;
        pqxx::work w(conn);
	w.exec("DELETE FROM aviationdb.waterelements WHERE ID=" + w.quote(m_id) + ";");
	if (rtree)
		w.exec("DELETE FROM aviationdb.waterelements_rtree WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
	m_id = 0;
}

void DbBaseElements::Waterelement::update_index(pqxx::connection& conn, bool rtree)
{
	if (!m_id)
		return;
	pqxx::work w(conn);
	w.exec("UPDATE aviationdb.waterelements SET TILE=" + w.quote(TileNumber::to_tilenumber(m_bbox)) + " WHERE ID=" + w.quote(m_id) + ";");
	if (rtree)
		w.exec("UPDATE aviationdb.waterelements_rtree SET SWLAT=" + w.quote(m_bbox.get_south()) + ",NELAT=" + w.quote(m_bbox.get_north()) +
		       ",SWLON=" + w.quote(m_bbox.get_west()) + ",NELON=" + w.quote((long long int)m_bbox.get_east_unwrapped()) + " WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
}

#endif

bool DbBaseElements::Waterelement::is_area(void) const
{
	return true;
}

const Glib::ustring& DbBaseElements::Waterelement::get_typename(type_t t)
{
	switch (t) {
	default:
	{
		static const Glib::ustring r("invalid");
		return r;
	}

	case type_landmass:
	{
		static const Glib::ustring r("landmass");
		return r;
	}

	case type_lake:
	{
		static const Glib::ustring r("lake");
		return r;
	}

	case type_island:
	{
		static const Glib::ustring r("island");
		return r;
	}

	case type_pond:
	{
		static const Glib::ustring r("pond");
		return r;
	}

	case type_ice:
	{
		static const Glib::ustring r("ice");
		return r;
	}

	case type_groundingline:
	{
		static const Glib::ustring r("groundingline");
		return r;
	}

	case type_land:
	{
		static const Glib::ustring r("land");
		return r;
	}

	case type_landext:
	{
		static const Glib::ustring r("extended land");
		return r;
	}
	}
}

void DbBaseElements::Waterelement::recompute_bbox(void)
{
	m_bbox = m_polygon.get_bbox();
}

template<> void DbBase<DbBaseElements::Waterelement>::drop_tables(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS waterelements;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS waterelements_deleted;"); cmd.executenonquery(); }
}

template<> void DbBase<DbBaseElements::Waterelement>::create_tables(void)
{
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS waterelements (ID INTEGER PRIMARY KEY NOT NULL, "
					      "TYPE INTEGER,"
					      "SWLAT INTEGER,"
					      "SWLON INTEGER,"
					      "NELAT INTEGER,"
					      "NELON INTEGER,"
					      "POLY BLOB,"
					      "TILE INTEGER);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS waterelements_deleted(ID INTEGER PRIMARY KEY NOT NULL);");
		cmd.executenonquery();
	}
}

template<> void DbBase<DbBaseElements::Waterelement>::drop_indices(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS waterelements_bbox;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS waterelements_tile;"); cmd.executenonquery(); }
	// old indices
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS waterelements_swlat;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS waterelements_swlon;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS waterelements_nelat;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS waterelements_nelon;"); cmd.executenonquery(); }
}

template<> void DbBase<DbBaseElements::Waterelement>::create_indices(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS waterelements_bbox ON waterelements(SWLAT,NELAT,SWLON,NELON);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS waterelements_tile ON waterelements(TILE);"); cmd.executenonquery(); }
	// old indices
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS waterelements_swlat ON waterelements(SWLAT);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS waterelements_swlon ON waterelements(SWLON);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS waterelements_nelat ON waterelements(NELAT);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS waterelements_nelon ON waterelements(NELON);"); cmd.executenonquery(); }
}

template<> const char *DbBase<DbBaseElements::Waterelement>::main_table_name = "waterelements";
template<> const char *DbBase<DbBaseElements::Waterelement>::database_file_name = "waterelements.db";
template<> const char *DbBase<DbBaseElements::Waterelement>::order_field = "ID";
template<> const char *DbBase<DbBaseElements::Waterelement>::delete_field = "ID";
template<> const bool DbBase<DbBaseElements::Waterelement>::area_data = true;

void WaterelementsDb::open_hires(const Glib::ustring& path)
{
	open(path, "waterelementshires.db");
}

void WaterelementsDb::open_readonly_hires(const Glib::ustring& path)
{
	open_readonly(path, "waterelementshires.db");
}

WaterelementsDb::elementvector_t WaterelementsDb::find_by_rect_type(const Rect& r, element_t::type_t typ, unsigned int limit, unsigned int loadsubtables)
{
	if (false)
		std::cerr << "find_by_rect_type: loadsubtables 0x" << std::hex << loadsubtables << std::endl;
	std::string qs;
	if (m_open == openstate_auxopen) {
		qs += "SELECT " + get_area_select_string(true, r) + " AND TYPE = ?5"
			" AND " + delete_field + " NOT IN (SELECT " + delete_field + " FROM " + main_table_name + "_deleted) UNION ";
	}
	qs += "SELECT " + get_area_select_string(false, r) + " AND TYPE = ?5";
	if (order_field) {
		qs += " ORDER BY ";
		qs += order_field;
	}
	if (limit)
		qs += " LIMIT ?6";
	qs += ";";
	if (false) {
		std::cerr << "sql: " << qs << std::endl;
		sqlite3x::sqlite3_command cmd(m_db, "EXPLAIN " + qs);
		sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
		while (cursor.step()) {
			std::cerr << "vm:";
			for (int col = 0; col < cursor.colcount(); col++) {
				std::cerr << " " << cursor.getstring(col);
			}
			std::cerr << std::endl;
		}
	}
	sqlite3x::sqlite3_command cmd(m_db, qs);
	cmd.bind(1, (long long int)r.get_west());
	cmd.bind(2, (long long int)r.get_south());
	cmd.bind(3, (long long int)r.get_east_unwrapped());
	cmd.bind(4, (long long int)r.get_north());
	cmd.bind(5, (int)typ);
	if (limit)
		cmd.bind(6, (long long int)limit);
	if (false)
		std::cerr << "sql: " << qs << " W " << r.get_west() << " S " << r.get_south() << " E " << r.get_east() << " N " << r.get_north() << std::endl;
	elementvector_t ret;
	sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
	for (;;) {
		ret.push_back(element_t());
		ret.back().load(cursor, m_db, loadsubtables);
		if (ret.back().is_valid())
			continue;
		ret.pop_back();
		break;
	}
	return ret;
}

WaterelementsDb::elementvector_t WaterelementsDb::find_by_type(element_t::type_t typ, unsigned int limit, unsigned int loadsubtables)
{
	if (false)
		std::cerr << "find_by_rect_type: loadsubtables 0x" << std::hex << loadsubtables << std::endl;
	std::string qs;
	if (m_open == openstate_auxopen) {
		qs += std::string("SELECT ") + element_t::db_aux_query_string + " FROM aux." + main_table_name + " WHERE TYPE = ?1"
			" AND " + delete_field + " NOT IN (SELECT " + delete_field + " FROM " + main_table_name + "_deleted) UNION ";
	}
	qs += std::string("SELECT ") + element_t::db_query_string + " FROM " + main_table_name + " WHERE TYPE = ?1";
	if (order_field) {
		qs += " ORDER BY ";
		qs += order_field;
	}
	if (limit)
		qs += " LIMIT ?2";
	qs += ";";
	if (false) {
		std::cerr << "sql: " << qs << std::endl;
		sqlite3x::sqlite3_command cmd(m_db, "EXPLAIN " + qs);
		sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
		while (cursor.step()) {
			std::cerr << "vm:";
			for (int col = 0; col < cursor.colcount(); col++) {
				std::cerr << " " << cursor.getstring(col);
			}
			std::cerr << std::endl;
		}
	}
	sqlite3x::sqlite3_command cmd(m_db, qs);
	cmd.bind(1, (int)typ);
	if (limit)
		cmd.bind(2, (long long int)limit);
	elementvector_t ret;
	sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
	for (;;) {
		ret.push_back(element_t());
		ret.back().load(cursor, m_db, loadsubtables);
		if (ret.back().is_valid())
			continue;
		ret.pop_back();
		break;
	}
	return ret;
}

void WaterelementsDb::purge(element_t::type_t typ)
{
	if (m_open == openstate_auxopen) {
		sqlite3x::sqlite3_command cmd(m_db, std::string("INSERT OR REPLACE INTO ") + main_table_name + "_deleted (" + delete_field +
					      ") SELECT " + delete_field + " FROM aux." + main_table_name + " WHERE TYPE=?1;");
		cmd.bind(1, (int)typ);
		cmd.executenonquery();
	}
	sqlite3x::sqlite3_command cmd(m_db, std::string("DELETE FROM ") + main_table_name + " WHERE TYPE=?1;");
	cmd.bind(1, (int)typ);
	cmd.executenonquery();
}

#ifdef HAVE_PQXX

template<> void PGDbBase<DbBaseElements::Waterelement>::drop_tables(void)
{
	pqxx::work w(m_conn);
	w.exec("DROP TABLE IF EXISTS aviationdb.waterelements;");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Waterelement>::create_tables(void)
{
	pqxx::work w(m_conn);
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.waterelements ("
	       "ID BIGINT PRIMARY KEY NOT NULL,"
	       "TYPE SMALLINT,"
	       "SWLAT INTEGER,"
	       "SWLON INTEGER,"
	       "NELAT INTEGER,"
	       "NELON BIGINT,"
	       "POLY BYTEA,"
	       "GPOLY ext.GEOGRAPHY(POLYGON,4326),"
	       "TILE INTEGER"
	       ");");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Waterelement>::drop_indices(void)
{
	pqxx::work w(m_conn);
	w.exec("DROP INDEX IF EXISTS aviationdb.waterelements_bbox;");
	w.exec("DROP INDEX IF EXISTS aviationdb.waterelements_tile;");
	// old indices
	w.exec("DROP INDEX IF EXISTS aviationdb.waterelements_swlat;");
	w.exec("DROP INDEX IF EXISTS aviationdb.waterelements_swlon;");
	w.exec("DROP INDEX IF EXISTS aviationdb.waterelements_nelat;");
	w.exec("DROP INDEX IF EXISTS aviationdb.waterelements_nelon;");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Waterelement>::create_indices(void)
{
	pqxx::work w(m_conn);
	w.exec("CREATE INDEX IF NOT EXISTS waterelements_bbox ON aviationdb.waterelements(SWLAT,NELAT,SWLON,NELON);");
	w.exec("CREATE INDEX IF NOT EXISTS waterelements_tile ON aviationdb.waterelements(TILE);");
	// old indices
	w.exec("CREATE INDEX IF NOT EXISTS waterelements_swlat ON aviationdb.waterelements(SWLAT);");
	w.exec("CREATE INDEX IF NOT EXISTS waterelements_swlon ON aviationdb.waterelements(SWLON);");
	w.exec("CREATE INDEX IF NOT EXISTS waterelements_nelat ON aviationdb.waterelements(NELAT);");
	w.exec("CREATE INDEX IF NOT EXISTS waterelements_nelon ON aviationdb.waterelements(NELON);");
	w.commit();
}

template<> const char *PGDbBase<DbBaseElements::Waterelement>::main_table_name = "aviationdb.waterelements";
template<> const char *PGDbBase<DbBaseElements::Waterelement>::order_field = "ID";
template<> const char *PGDbBase<DbBaseElements::Waterelement>::delete_field = "ID";
template<> const bool PGDbBase<DbBaseElements::Waterelement>::area_data = true;

WaterelementsPGDb::elementvector_t WaterelementsPGDb::find_by_rect_type(const Rect& bbox, element_t::type_t typ, unsigned int limit, unsigned int loadsubtables)
{
	if (false)
		std::cerr << "find_by_rect_type: loadsubtables 0x" << std::hex << loadsubtables << std::endl;
	pqxx::read_transaction w(m_conn);
	std::string qs;
	qs = "SELECT " + get_area_select_string(w, bbox) + " AND TYPE = " + w.quote((int)typ);
	if (order_field) {
		qs += " ORDER BY ";
		qs += order_field;
	}
	if (limit)
		qs += " LIMIT " + w.quote(limit);
	qs += ";";
	if (false)
		std::cerr << "sql: " << qs << std::endl;
	pqxx::result r(w.exec(qs));
	elementvector_t ret;
	for (pqxx::result::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
		ret.push_back(element_t());
		ret.back().load(*ri, w, loadsubtables);
		if (ret.back().is_valid())
			continue;
		ret.pop_back();
		break;
	}
	return ret;
}

WaterelementsPGDb::elementvector_t WaterelementsPGDb::find_by_type(element_t::type_t typ, unsigned int limit, unsigned int loadsubtables)
{
	if (false)
		std::cerr << "find_by_rect_type: loadsubtables 0x" << std::hex << loadsubtables << std::endl;
	pqxx::read_transaction w(m_conn);
	std::string qs;
	qs = std::string("SELECT ") + element_t::db_query_string + " FROM " + main_table_name + " WHERE TYPE = " + w.quote((int)typ);
	if (order_field) {
		qs += " ORDER BY ";
		qs += order_field;
	}
	if (limit)
		qs += " LIMIT " + w.quote(limit);
	qs += ";";
	if (false)
		std::cerr << "sql: " << qs << std::endl;
	pqxx::result r(w.exec(qs));
	elementvector_t ret;
	for (pqxx::result::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
		ret.push_back(element_t());
		ret.back().load(*ri, w, loadsubtables);
		if (ret.back().is_valid())
			continue;
		ret.pop_back();
		break;
	}
	return ret;
}

void WaterelementsPGDb::purge(element_t::type_t typ)
{
	pqxx::work w(m_conn);
	w.exec(std::string("DELETE FROM ") + main_table_name + " WHERE TYPE=" + w.quote((int)typ) + ";");
	w.commit();
}

#endif
