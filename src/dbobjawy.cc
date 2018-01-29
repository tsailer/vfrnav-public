//
// C++ Implementation: dbobjawy
//
// Description: Database Objects: Airway
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012, 2013, 2016, 2017
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

#include <limits>

const char *DbBaseElements::Airway::db_query_string = "airways.ID AS ID,0 AS \"TABLE\",airways.SRCID AS SRCID,airways.BNAME,airways.ENAME,airways.NAME,airways.TYPE,airways.BLAT,airways.BLON,airways.ELAT,airways.ELON,airways.SWLAT,airways.NELAT,airways.SWLON,airways.NELON,airways.BLEVEL,airways.TLEVEL,airways.TELEV,airways.C5ELEV,airways.LABELPLACEMENT,airways.MODIFIED";
const char *DbBaseElements::Airway::db_aux_query_string = "aux.airways.ID AS ID,1 AS \"TABLE\",aux.airways.SRCID AS SRCID,aux.airways.BNAME,aux.airways.ENAME,aux.airways.NAME,aux.airways.TYPE,aux.airways.BLAT,aux.airways.BLON,aux.airways.ELAT,aux.airways.ELON,aux.airways.SWLAT,aux.airways.NELAT,aux.airways.SWLON,aux.airways.NELON,aux.airways.BLEVEL,aux.airways.TLEVEL,aux.airways.TELEV,aux.airways.C5ELEV,aux.airways.LABELPLACEMENT,aux.airways.MODIFIED";
const char *DbBaseElements::Airway::db_text_fields[] = { "SRCID", "BNAME", "ENAME", "NAME", 0 };
const char *DbBaseElements::Airway::db_time_fields[] = { "MODIFIED", 0 };

const DbBaseElements::Airway::elev_t DbBaseElements::Airway::nodata = std::numeric_limits<elev_t>::min();

DbBaseElements::Airway::Airway(void)
	: m_begin_coord(), m_end_coord(), m_begin_name(), m_end_name(), m_name(), m_base_level(0), m_top_level(0),
	  m_terrainelev(nodata),  m_corridor5elev(nodata), m_type(airway_invalid)
{
}

Rect DbBaseElements::Airway::get_bbox(void) const
{
	PolygonSimple p;
	p.push_back(m_begin_coord);
	p.push_back(m_end_coord);
	return p.get_bbox();
}

void DbBaseElements::Airway::load( sqlite3x::sqlite3_cursor & cursor, sqlite3x::sqlite3_connection & db, unsigned int loadsubtables )
{
	m_id = 0;
	m_table = table_main;
	m_type = airway_invalid;
	if (!cursor.step())
		return;
	m_id = cursor.getint(0);
	m_table = (table_t)cursor.getint(1);
	m_sourceid = cursor.getstring(2);
	m_begin_name = cursor.getstring(3);
	m_end_name = cursor.getstring(4);
	m_name = cursor.getstring(5);
	m_type = (airway_type_t)cursor.getint(6);
	m_begin_coord = Point(cursor.getint(8), cursor.getint(7));
	m_end_coord = Point(cursor.getint(10), cursor.getint(9));
	m_base_level = cursor.getint(15);
	m_top_level = cursor.getint(16);
	if (cursor.isnull(17))
		m_terrainelev = nodata;
	else
		m_terrainelev = cursor.getint(17);
	if (cursor.isnull(18))
		m_corridor5elev = nodata;
	else
		m_corridor5elev = cursor.getint(18);
	m_label_placement = (label_placement_t)cursor.getint(19);
	m_modtime = cursor.getint(20);
}

void DbBaseElements::Airway::update_index(sqlite3x::sqlite3_connection & db, bool rtree, bool aux_rtree)
{
	if (m_table != table_main || !m_id)
		return;
	sqlite3x::sqlite3_transaction tran(db);
	Rect bbox(get_bbox());
	{
		sqlite3x::sqlite3_command cmd(db, "UPDATE airways SET TILE=?2 WHERE ID=?1;");
		cmd.bind(1, (long long int)m_id);
		cmd.bind(2, (int)TileNumber::to_tilenumber(bbox));
		cmd.executenonquery();
	}
	if (rtree) {
		sqlite3x::sqlite3_command cmd(db, "UPDATE airways_rtree SET SWLAT=?,NELAT=?,SWLON=?,NELON=? WHERE ID=?;");
		cmd.bind(1, bbox.get_south());
		cmd.bind(2, bbox.get_north());
		cmd.bind(3, bbox.get_west());
		cmd.bind(4, (long long int)bbox.get_east_unwrapped());
		cmd.bind(5, (long long int)m_id);
		cmd.executenonquery();
	}
	tran.commit();
}

void DbBaseElements::Airway::save(sqlite3x::sqlite3_connection & db, bool rtree, bool aux_rtree)
{
	sqlite3x::sqlite3_transaction tran(db);
	if (m_table == table_aux) {
		sqlite3x::sqlite3_command cmd(db, "INSERT OR REPLACE INTO airways_deleted (SRCID) VALUES (?);");
		cmd.bind(1, m_sourceid);
		cmd.executenonquery();
		m_table = table_main;
		m_id = 0;
	}
	Rect bbox(get_bbox());
	if (!m_id) {
		m_id = 1;
		{
			sqlite3x::sqlite3_command cmd(db, "SELECT MAX(ID) FROM airways;");
			sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
			if (cursor.step())
				m_id = cursor.getint(0) + 1;
		}
		{
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO airways (ID,SRCID) VALUES(?,?);");
			cmd.bind(1, (long long int)m_id);
			cmd.bind(2, m_sourceid);
			cmd.executenonquery();
		}
		if (rtree) {
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO airways_rtree (ID,SWLAT,NELAT,SWLON,NELON) VALUES(?,?,?,?,?);");
			cmd.bind(1, (long long int)m_id);
			cmd.bind(2, bbox.get_south());
			cmd.bind(3, bbox.get_north());
			cmd.bind(4, bbox.get_west());
			cmd.bind(5, (long long int)bbox.get_east_unwrapped());
			cmd.executenonquery();
		}
	}
	{
		sqlite3x::sqlite3_command cmd(db, "UPDATE airways SET SRCID=?,BNAME=?,ENAME=?,NAME=?,TYPE=?,BLAT=?,BLON=?,ELAT=?,ELON=?,SWLAT=?,SWLON=?,NELAT=?,NELON=?,BLEVEL=?,TLEVEL=?,TELEV=?,C5ELEV=?,LABELPLACEMENT=?,MODIFIED=?,TILE=? WHERE ID=?;");
		cmd.bind(1, m_sourceid);
		cmd.bind(2, m_begin_name);
		cmd.bind(3, m_end_name);
		cmd.bind(4, m_name);
		cmd.bind(5, (int)m_type);
		cmd.bind(6, m_begin_coord.get_lat());
		cmd.bind(7, m_begin_coord.get_lon());
		cmd.bind(8, m_end_coord.get_lat());
		cmd.bind(9, m_end_coord.get_lon());
		cmd.bind(10, bbox.get_south());
		cmd.bind(11, bbox.get_west());
		cmd.bind(12, bbox.get_north());
		cmd.bind(13, (long long int)bbox.get_east_unwrapped());
		cmd.bind(14, (int)m_base_level);
		cmd.bind(15, (int)m_top_level);
		cmd.bind(16, (int)m_terrainelev);
		cmd.bind(17, (int)m_corridor5elev);
		cmd.bind(18, m_label_placement);
		cmd.bind(19, (long long int)m_modtime);
		cmd.bind(20, (int)TileNumber::to_tilenumber(bbox));
		cmd.bind(21, (long long int)m_id);
		cmd.executenonquery();
	}
	if (rtree) {
		sqlite3x::sqlite3_command cmd(db, "UPDATE airways_rtree SET SWLAT=?,NELAT=?,SWLON=?,NELON=? WHERE ID=?;");
		cmd.bind(1, bbox.get_south());
		cmd.bind(2, bbox.get_north());
		cmd.bind(3, bbox.get_west());
		cmd.bind(4, (long long int)bbox.get_east_unwrapped());
		cmd.bind(5, (long long int)m_id);
		cmd.executenonquery();
	}
	tran.commit();
}

void DbBaseElements::Airway::erase(sqlite3x::sqlite3_connection & db, bool rtree, bool aux_rtree)
{
	if (!is_valid())
		return;
	sqlite3x::sqlite3_transaction tran(db);
	if (m_table == table_aux) {
		sqlite3x::sqlite3_command cmd(db, "INSERT OR REPLACE INTO airways_deleted (SRCID) VALUES (?);");
		cmd.bind(1, m_sourceid);
		cmd.executenonquery();
		m_table = table_main;
	} else {
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airways WHERE ID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		if (rtree) {
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airways_rtree WHERE ID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
	}
	tran.commit();
	m_id = 0;
}

#ifdef HAVE_PQXX

void DbBaseElements::Airway::load(const pqxx::tuple& cursor, pqxx::basic_transaction& w, unsigned int loadsubtables)
{
	m_id = 0;
	m_table = table_main;
	m_type = airway_invalid;
	if (cursor.empty())
		return;
	m_id = cursor[0].as<int64_t>(0);
	m_table = (table_t)cursor[1].as<int>(0);
	m_sourceid = cursor[2].as<std::string>("");
	m_begin_name = cursor[3].as<std::string>("");
	m_end_name = cursor[4].as<std::string>("");
 	m_name = cursor[5].as<std::string>("");
	m_type = (airway_type_t)cursor[6].as<int>((int)airway_invalid);
	m_begin_coord = Point(cursor[8].as<Point::coord_t>(0), cursor[7].as<Point::coord_t>(0x80000000));
	m_end_coord = Point(cursor[10].as<Point::coord_t>(0), cursor[9].as<Point::coord_t>(0x80000000));
	m_base_level = cursor[15].as<int16_t>(0);
	m_top_level = cursor[16].as<int16_t>(0);
	m_terrainelev = cursor[17].as<elev_t>(nodata);
	m_corridor5elev = cursor[18].as<elev_t>(nodata);
	m_label_placement = (label_placement_t)cursor[19].as<int>((int)label_off);
	m_modtime = cursor[20].as<time_t>(0);
}

void DbBaseElements::Airway::save(pqxx::connection_base& conn, bool rtree)
{
	pqxx::work w(conn);
	Rect bbox(get_bbox());
	if (!m_id) {
		pqxx::result r(w.exec("INSERT INTO aviationdb.airways (ID,SRCID) SELECT COALESCE(MAX(ID),0)+1," + w.quote((std::string)m_sourceid) + " FROM aviationdb.airways RETURNING ID;"));
		m_id = r.front()[0].as<int64_t>();
		if (rtree)
			w.exec("INSERT INTO aviationdb.airways_rtree (ID,SWLAT,NELAT,SWLON,NELON) VALUES(" + w.quote(m_id) + "," + w.quote(bbox.get_south()) +
			       "," + w.quote(bbox.get_north()) + "," + w.quote(bbox.get_west()) + "," + w.quote((long long int)bbox.get_east_unwrapped()) + ");");
	}
	w.exec("UPDATE aviationdb.airways SET SRCID=" + w.quote((std::string)m_sourceid) + ",BNAME=" + w.quote((std::string)m_begin_name) +
	       ",ENAME=" + w.quote((std::string)m_end_name) + ",NAME=" + w.quote((std::string)m_name) + ",TYPE=" + w.quote((int)m_type) +
	       ",BLAT=" + w.quote(m_begin_coord.get_lat()) + ",BLON=" + w.quote(m_begin_coord.get_lon()) +
	       ",ELAT=" + w.quote(m_end_coord.get_lat()) + ",ELON=" + w.quote(m_end_coord.get_lon()) +
	       ",SWLAT=" + w.quote(bbox.get_south()) + ",SWLON=" + w.quote(bbox.get_west()) +
	       ",NELAT=" + w.quote(bbox.get_north()) + ",NELON=" + w.quote(bbox.get_east_unwrapped()) +
	       ",BLEVEL=" + w.quote(m_base_level) + ",TLEVEL=" + w.quote(m_top_level) +
	       ",TELEV=" + w.quote(m_terrainelev) + ",C5ELEV=" + w.quote(m_corridor5elev) +
	       ",LABELPLACEMENT=" + w.quote((int)m_label_placement) + ",MODIFIED=" + w.quote(m_modtime) +
	       ",TILE=" + w.quote(TileNumber::to_tilenumber(bbox)) + " WHERE ID=" + w.quote(m_id) + ";");
	if (rtree)
		w.exec("UPDATE aviationdb.airways_rtree SET SWLAT=" + w.quote(bbox.get_south()) + ",NELAT=" + w.quote(bbox.get_north()) +
		       ",SWLON=" + w.quote(bbox.get_west()) + ",NELON=" + w.quote((long long int)bbox.get_east_unwrapped()) + " WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
}

void DbBaseElements::Airway::erase(pqxx::connection_base& conn, bool rtree)
{
	if (!is_valid())
		return;
	pqxx::work w(conn);
	w.exec("DELETE FROM aviationdb.airways WHERE ID=" + w.quote(m_id) + ";");
	if (rtree)
		w.exec("DELETE FROM aviationdb.airways_rtree WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
	m_id = 0;
}

void DbBaseElements::Airway::update_index(pqxx::connection_base& conn, bool rtree)
{
	if (!m_id)
		return;
	pqxx::work w(conn);
	Rect bbox(get_bbox());
	w.exec("UPDATE aviationdb.airways SET TILE=" + w.quote(TileNumber::to_tilenumber(bbox)) + " WHERE ID=" + w.quote(m_id) + ";");
	if (rtree)
		w.exec("UPDATE aviationdb.airways_rtree SET SWLAT=" + w.quote(bbox.get_south()) + ",NELAT=" + w.quote(bbox.get_north()) +
		       ",SWLON=" + w.quote(bbox.get_west()) + ",NELON=" + w.quote((long long int)bbox.get_east_unwrapped()) + " WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
}

#endif

const Glib::ustring& DbBaseElements::Airway::get_type_name(airway_type_t t)
{
	switch (t) {
		default:
		case airway_invalid:
		{
			static const Glib::ustring r("Invalid");
			return r;
		}

		case airway_low:
		{
			static const Glib::ustring r("Low Level");
			return r;
		}

		case airway_high:
		{
			static const Glib::ustring r("High Level");
			return r;
		}

		case airway_both:
		{
			static const Glib::ustring r("Both Levels");
			return r;
		}

		case airway_user:
		{
			static const Glib::ustring r("User");
			return r;
		}
	}
}

DbBaseElements::Airway::nameset_t DbBaseElements::Airway::get_name_set(void) const
{
	nameset_t r;
	Glib::ustring name(get_name());
	while (!name.empty()) {
		Glib::ustring aname;
		Glib::ustring::size_type n(name.find('-'));
		if (n == Glib::ustring::npos) {
			aname.swap(name);
		} else {
			aname = Glib::ustring(name, 0, n);
			name.erase(0, n + 1);
		}
		if (!aname.empty())
			r.insert(aname.uppercase());
	}
	return r;
}

void DbBaseElements::Airway::set_name_set(const nameset_t& nameset)
{
	bool subseq(false);
	m_name.clear();
	for (nameset_t::const_iterator ni(nameset.begin()), ne(nameset.end()); ni != ne; ++ni) {
		if (subseq)
			m_name += '-';
		subseq = true;
		m_name += *ni;
	}
}

bool DbBaseElements::Airway::add_name(const Glib::ustring& name)
{
	nameset_t n(get_name_set());
	std::pair<nameset_t::iterator,bool> ins(n.insert(name.uppercase()));
	if (!ins.second)
		return false;
	set_name_set(n);
	return true;
}

bool DbBaseElements::Airway::remove_name(const Glib::ustring& name)
{
	nameset_t n(get_name_set());
	nameset_t::iterator i(n.find(name.uppercase()));
	if (i == n.end())
		return false;
	n.erase(i);
	set_name_set(n);
	return true;
}

bool DbBaseElements::Airway::contains_name(const Glib::ustring& name) const
{
	nameset_t n(get_name_set());
	nameset_t::iterator i(n.find(name.uppercase()));
	if (i == n.end())
		return false;
	return true;
}

template<> void DbBase<DbBaseElements::Airway>::drop_tables(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS airways;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS airways_deleted;"); cmd.executenonquery(); }
}

template<> void DbBase<DbBaseElements::Airway>::create_tables(void)
{
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS airways (ID INTEGER PRIMARY KEY NOT NULL, "
					      "SRCID TEXT UNIQUE NOT NULL,"
					      "BNAME TEXT,"
					      "ENAME TEXT,"
					      "NAME TEXT,"
					      "TYPE INTEGER,"
					      "BLAT INTEGER,"
					      "BLON INTEGER,"
					      "ELAT INTEGER,"
					      "ELON INTEGER,"
					      "SWLAT INTEGER,"
					      "SWLON INTEGER,"
					      "NELAT INTEGER,"
					      "NELON INTEGER,"
					      "BLEVEL INTEGER,"
					      "TLEVEL INTEGER,"
					      "TELEV INTEGER,"
					      "C5ELEV INTEGER,"
					      "LABELPLACEMENT INTEGER,"
					      "MODIFIED INTEGER,"
					      "TILE INTEGER);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS airways_deleted(SRCID TEXT PRIMARY KEY NOT NULL);");
		cmd.executenonquery();
	}
}

template<> void DbBase<DbBaseElements::Airway>::drop_indices(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airways_srcid;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airways_bname;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airways_ename;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airways_name;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airways_latlon;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airways_bbox;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airways_modified;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airways_tile;"); cmd.executenonquery(); }
}

template<> void DbBase<DbBaseElements::Airway>::create_indices(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airways_srcid ON airways(SRCID);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airways_bname ON airways(BNAME COLLATE NOCASE);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airways_ename ON airways(ENAME COLLATE NOCASE);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airways_name ON airways(NAME COLLATE NOCASE);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airways_latlon ON airways(BLAT,BLON,ELAT,ELON);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airways_bbox ON airways(SWLAT,NELAT,SWLON,NELON);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airways_modified ON airways(MODIFIED);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airways_tile ON airways(TILE);"); cmd.executenonquery(); }
}

template<> const char *DbBase<DbBaseElements::Airway>::main_table_name = "airways";
template<> const char *DbBase<DbBaseElements::Airway>::database_file_name = "airways.db";
template<> const char *DbBase<DbBaseElements::Airway>::order_field = "SRCID";
template<> const char *DbBase<DbBaseElements::Airway>::delete_field = "SRCID";
template<> const bool DbBase<DbBaseElements::Airway>::area_data = true;

std::string AirwaysDb::get_airways_area_select_string(bool auxtable, const Rect& r, const char *sortcol)
{
	std::ostringstream oss;
	if (auxtable)
		oss << element_t::db_aux_query_string;
	else
		oss << element_t::db_query_string;
	if (sortcol)
		oss << ',' << sortcol;
	oss << " FROM ";
	std::string table_name(main_table_name);
	if (auxtable)
		table_name = "aux." + table_name;
	oss << table_name;
	oss << " WHERE NAME IN (SELECT DISTINCT NAME FROM " << table_name << " WHERE ";
	bool rtree(auxtable ? m_has_aux_rtree : m_has_rtree);
	if (rtree) {
		oss << "ID IN (SELECT ID FROM " << table_name << "_rtree WHERE "
		    << table_name << "_rtree.NELAT >= ?2 AND " << table_name << "_rtree.SWLAT <= ?4 AND (("
		    //<< table_name << "_rtree.SWLON <= ?3-4294967296 AND " << table_name << "_rtree.NELON >= ?1-4294967296) OR ("
		    //<< table_name << "_rtree.SWLON <= ?3 AND " << table_name << "_rtree.NELON >= ?1) OR ("
		    //<< table_name << "_rtree.SWLON <= ?3+4294967296 AND " << table_name << "_rtree.NELON >= ?1+4294967296))";
		    << table_name << "_rtree.SWLON <= ?3 AND " << table_name << "_rtree.NELON >= ?1))))";
		return oss.str();
	}
	if (false) {
		oss << '(';
		element_t::tilerange_t tr(element_t::TileNumber::to_tilerange(r));
		bool needor(false);
		for (element_t::TileNumber::tile_t mask = (1 << element_t::TileNumber::tile_bits) - 1; mask; mask >>= 1, tr.first >>= 1, tr.second >>= 1) {
			if (needor)
				oss << " OR ";
			needor = true;
			if (tr.second < tr.first) {
				oss << "(TILE BETWEEN " << tr.first << " AND " << (tr.first | mask)
				    << ") OR (TILE BETWEEN " << (tr.second & ~mask) << " AND " << tr.second << ')';
			} else {
				oss << "(TILE BETWEEN " << tr.first << " AND " << tr.second << ')';
			}
		}
		oss << ") AND ";
	} else if (false) {
		oss << "TILE IN (";
		element_t::tilerange_t tr(element_t::TileNumber::to_tilerange(r));
		bool needcomma(false);
		for (element_t::TileNumber::tile_t mask = (1 << element_t::TileNumber::tile_bits) - 1; mask; mask >>= 1, tr.first >>= 1, tr.second >>= 1) {
			for (element_t::TileNumber::tile_t tile = tr.first;; tile = (tile & ~mask) | ((tile + 1) & mask)) {
				if (needcomma)
					oss << ',';
				needcomma = true;
				oss << tile;
				if (tile == tr.second)
					break;
			}
		}
		oss << ") AND ";
	} else if (true) {
		oss << "TILE IN (";
		element_t::TileNumber::Iterator ti(r);
		bool needcomma(false);
		do {
			if (needcomma)
				oss << ',';
			needcomma = true;
			oss << ti();
		} while (ti.next());
		oss << ") AND ";
	}
	if (area_data) {
		if (r.get_north() + 0x40000000 < 0x40000000 - r.get_south())
			oss << "(SWLAT <= ?4 AND NELAT >= ?2)";
		else
			oss << "(NELAT >= ?2 AND SWLAT <= ?4)";
		oss << " AND ((SWLON <= ?3-4294967296 AND NELON >= ?1-4294967296) OR "
			     "(SWLON <= ?3 AND NELON >= ?1) OR "
			     "(SWLON <= ?3+4294967296 AND NELON >= ?1+4294967296))";
	} else {
		oss << "(LAT BETWEEN ?2 AND ?4) AND ((LON BETWEEN ?1-4294967296 AND ?3-4294967296) OR (LON BETWEEN ?1 AND ?3))";
	}
	oss << ')';
	return oss.str();
}

AirwaysDb::elementvector_t AirwaysDb::find_by_area(const Rect& r, unsigned int limit, unsigned int loadsubtables)
{
	if (false)
		std::cerr << "find_by_area: loadsubtables 0x" << std::hex << loadsubtables << std::endl;
	std::string qs;
	if (m_open == openstate_auxopen) {
		qs += "SELECT " + get_airways_area_select_string(true, r) +
		      " AND " + delete_field + " NOT IN (SELECT " + delete_field + " FROM " + main_table_name + "_deleted) UNION ";
	}
	qs += "SELECT " + get_airways_area_select_string(false, r);
	if (order_field) {
		qs += " ORDER BY ";
		qs += order_field;
	}
	if (limit)
		qs += " LIMIT ?5";
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
	if (limit)
		cmd.bind(5, (long long int)limit);
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
	if (false)
		std::cerr << "find_by_area: " << main_table_name << " rect " << r << " retsize " << ret.size() << " qs " << qs << std::endl;
	return ret;

}

#ifdef HAVE_PQXX

template<> void PGDbBase<DbBaseElements::Airway>::drop_tables(void)
{
	pqxx::work w(m_conn);
	w.exec("DROP TABLE IF EXISTS aviationdb.airways;");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Airway>::create_tables(void)
{
	create_common_tables();
	pqxx::work w(m_conn);
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.airways ("
	       "BNAME TEXT,"
	       "ENAME TEXT,"
	       "NAME TEXT,"
	       "TYPE INTEGER,"
	       "BLAT INTEGER,"
	       "BLON INTEGER,"
	       "ELAT INTEGER,"
	       "ELON INTEGER,"
	       "SWLAT INTEGER,"
	       "SWLON INTEGER,"
	       "NELAT INTEGER,"
	       "NELON BIGINT,"
	       "BLEVEL INTEGER,"
	       "TLEVEL INTEGER,"
	       "TELEV INTEGER,"
	       "C5ELEV INTEGER,"
	       "TILE INTEGER,"
	       "PRIMARY KEY (ID)"
	       ") INHERITS (aviationdb.labelsourcebase);");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Airway>::drop_indices(void)
{
	pqxx::work w(m_conn);
	w.exec("DROP INDEX IF EXISTS aviationdb.airways_srcid;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airways_bname;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airways_ename;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airways_name;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airways_latlon;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airways_bbox;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airways_modified;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airways_tile;");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Airway>::create_indices(void)
{
	pqxx::work w(m_conn);
	w.exec("CREATE INDEX IF NOT EXISTS airways_srcid ON aviationdb.airways(SRCID);");
	w.exec("CREATE INDEX IF NOT EXISTS airways_bname ON aviationdb.airways((BNAME::ext.CITEXT) COLLATE \"C\");");
	w.exec("CREATE INDEX IF NOT EXISTS airways_ename ON aviationdb.airways((ENAME::ext.CITEXT) COLLATE \"C\");");
	w.exec("CREATE INDEX IF NOT EXISTS airways_name ON aviationdb.airways((NAME::ext.CITEXT));");
	w.exec("CREATE INDEX IF NOT EXISTS airways_latlon ON aviationdb.airways(BLAT,BLON,ELAT,ELON);");
	w.exec("CREATE INDEX IF NOT EXISTS airways_bbox ON aviationdb.airways(SWLAT,NELAT,SWLON,NELON);");
	w.exec("CREATE INDEX IF NOT EXISTS airways_modified ON aviationdb.airways(MODIFIED);");
	w.exec("CREATE INDEX IF NOT EXISTS airways_tile ON aviationdb.airways(TILE);");
	w.commit();
}

template<> const char *PGDbBase<DbBaseElements::Airway>::main_table_name = "aviationdb.airways";
template<> const char *PGDbBase<DbBaseElements::Airway>::order_field = "SRCID";
template<> const char *PGDbBase<DbBaseElements::Airway>::delete_field = "SRCID";
template<> const bool PGDbBase<DbBaseElements::Airway>::area_data = true;

std::string AirwaysPGDb::get_airways_area_select_string(pqxx::basic_transaction& w, const Rect& r, const char *sortcol)
{
	std::ostringstream oss;
	oss << element_t::db_query_string;
	if (sortcol)
		oss << ',' << sortcol;
	oss << " FROM " << main_table_name << " WHERE NAME IN (SELECT DISTINCT NAME FROM " << main_table_name << " WHERE ";
	if (m_has_rtree) {
		oss << "ID IN (SELECT ID FROM " << main_table_name << "_rtree WHERE "
		    << main_table_name << "_rtree.NELAT >= " << w.quote(r.get_south()) << " AND " << main_table_name << "_rtree.SWLAT <= " << w.quote(r.get_north()) << " AND (("
		    //<< main_table_name << "_rtree.SWLON <= " << w.quote(r.get_east_unwrapped()) << "-4294967296 AND " << main_table_name << "_rtree.NELON >= " << w.quote(r.get_west()) << "-4294967296) OR ("
		    //<< main_table_name << "_rtree.SWLON <= " << w.quote(r.get_east_unwrapped()) << " AND " << main_table_name << "_rtree.NELON >= " << w.quote(r.get_west()) << ") OR ("
		    //<< main_table_name << "_rtree.SWLON <= " << w.quote(r.get_east_unwrapped()) << "+4294967296 AND " << main_table_name << "_rtree.NELON >= " << w.quote(r.get_west()) << "+4294967296))";
		    << main_table_name << "_rtree.SWLON <= " << w.quote(r.get_east_unwrapped()) << " AND " << main_table_name << "_rtree.NELON >= " << w.quote(r.get_west()) << "))))";
		return oss.str();
	}
	if (false) {
		oss << '(';
		element_t::tilerange_t tr(element_t::TileNumber::to_tilerange(r));
		bool needor(false);
		for (element_t::TileNumber::tile_t mask = (1 << element_t::TileNumber::tile_bits) - 1; mask; mask >>= 1, tr.first >>= 1, tr.second >>= 1) {
			if (needor)
				oss << " OR ";
			needor = true;
			if (tr.second < tr.first) {
				oss << "(TILE BETWEEN " << tr.first << " AND " << (tr.first | mask)
				    << ") OR (TILE BETWEEN " << (tr.second & ~mask) << " AND " << tr.second << ')';
			} else {
				oss << "(TILE BETWEEN " << tr.first << " AND " << tr.second << ')';
			}
		}
		oss << ") AND ";
	} else if (false) {
		oss << "TILE IN (";
		element_t::tilerange_t tr(element_t::TileNumber::to_tilerange(r));
		bool needcomma(false);
		for (element_t::TileNumber::tile_t mask = (1 << element_t::TileNumber::tile_bits) - 1; mask; mask >>= 1, tr.first >>= 1, tr.second >>= 1) {
			for (element_t::TileNumber::tile_t tile = tr.first;; tile = (tile & ~mask) | ((tile + 1) & mask)) {
				if (needcomma)
					oss << ',';
				needcomma = true;
				oss << tile;
				if (tile == tr.second)
					break;
			}
		}
		oss << ") AND ";
	} else if (true) {
		oss << "TILE IN (";
		element_t::TileNumber::Iterator ti(r);
		bool needcomma(false);
		do {
			if (needcomma)
				oss << ',';
			needcomma = true;
			oss << ti();
		} while (ti.next());
		oss << ") AND ";
	}
	if (area_data) {
		if (r.get_north() + 0x40000000 < 0x40000000 - r.get_south())
			oss << "(SWLAT <= " << w.quote(r.get_north()) << " AND NELAT >= " << w.quote(r.get_south()) << ")";
		else
			oss << "(NELAT >= " << w.quote(r.get_south()) << " AND SWLAT <= " << w.quote(r.get_north()) << ")";
		oss << " AND ((SWLON <= " << w.quote(r.get_east_unwrapped()) << "-4294967296 AND NELON >= " << w.quote(r.get_west()) << "-4294967296) OR "
			     "(SWLON <= " << w.quote(r.get_east_unwrapped()) << " AND NELON >= " << w.quote(r.get_west()) << ") OR "
			     "(SWLON <= " << w.quote(r.get_east_unwrapped()) << "+4294967296 AND NELON >= " << w.quote(r.get_west()) << "+4294967296))";
	} else {
		oss << "(LAT BETWEEN " << w.quote(r.get_south()) << " AND " << w.quote(r.get_north())
		    << ") AND ((LON BETWEEN " << w.quote(r.get_west()) << "-4294967296 AND " << w.quote(r.get_east_unwrapped())
		    << "-4294967296) OR (LON BETWEEN " << w.quote(r.get_west()) << " AND " << w.quote(r.get_east_unwrapped()) << "))";
	}
	oss << ')';
	return oss.str();
}

AirwaysPGDb::elementvector_t AirwaysPGDb::find_by_area(const Rect& bbox, unsigned int limit, unsigned int loadsubtables)
{
	if (false)
		std::cerr << "find_by_area: loadsubtables 0x" << std::hex << loadsubtables << std::endl;
	pqxx::read_transaction w(m_conn);
	std::string qs;
	qs = "SELECT " + get_airways_area_select_string(w, bbox);
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

#endif
