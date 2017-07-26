//
// C++ Implementation: dbobjlbl
//
// Description: Database Objects: Labels Database
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <limits>

#include "dbobj.h"

#ifdef HAVE_PQXX
#include <pqxx/transactor.hxx>
#include <pqxx/result.hxx>
#include <pqxx/except.hxx>
#endif

const char *DbBaseElements::Label::db_query_string = "labels.ID AS ID,0 AS \"TABLE\",labels.MUTABLE,labels.LAT,labels.LON,labels.LABELPLACEMENT,labels.METRIC,labels.SUBID,labels.SUBTABLE";
const char *DbBaseElements::Label::db_aux_query_string = "aux.labels.ID AS ID,1 AS \"TABLE\",aux.labels.MUTABLE,aux.labels.LAT,aux.labels.LON,aux.labels.LABELPLACEMENT,aux.labels.METRIC,aux.labels.SUBID,aux.labels.SUBTABLE";
const char *DbBaseElements::Label::db_text_fields[] = { 0 };
const char *DbBaseElements::Label::db_time_fields[] = { 0 };

DbBaseElements::Label::Label(void)
	: m_mutable(false), m_coord(), m_metric(std::numeric_limits<double>::quiet_NaN()), m_subid(0)
{
}

void DbBaseElements::Label::load( sqlite3x::sqlite3_cursor & cursor, sqlite3x::sqlite3_connection & db, unsigned int loadsubtables )
{
	m_id = 0;
	m_table = table_main;
	m_label_placement = label_off;
	if (!cursor.step())
		return;
	m_id = cursor.getint(0);
	m_table = (table_t)cursor.getint(1);
	m_mutable = !!cursor.getint(2);
	m_coord = Point(cursor.getint(4), cursor.getint(3));
	m_label_placement = (label_placement_t)cursor.getint(5);
	m_metric = cursor.getdouble(6);
	m_subid = cursor.getint(7);
	m_subtable = (table_t)cursor.getint(8);
}

void DbBaseElements::Label::update_index(sqlite3x::sqlite3_connection & db, bool rtree, bool aux_rtree)
{
	if (m_table != table_main || !m_id)
		return;
	sqlite3x::sqlite3_transaction tran(db);
	{
		sqlite3x::sqlite3_command cmd(db, "UPDATE labels SET TILE=?2 WHERE ID=?1;");
		cmd.bind(1, (long long int)m_id);
		cmd.bind(2, (int)TileNumber::to_tilenumber(get_coord()));
		cmd.executenonquery();
	}
	if (rtree) {
		sqlite3x::sqlite3_command cmd(db, "UPDATE labels_rtree SET SWLAT=?,NELAT=?,SWLON=?,NELON=? WHERE ID=?;");
		cmd.bind(1, m_coord.get_lat());
		cmd.bind(2, m_coord.get_lat());
		cmd.bind(3, m_coord.get_lon());
		cmd.bind(4, m_coord.get_lon());
		cmd.bind(5, (long long int)m_id);
		cmd.executenonquery();
	}
	tran.commit();
}

void DbBaseElements::Label::save(sqlite3x::sqlite3_connection & db, bool rtree, bool aux_rtree)
{
	sqlite3x::sqlite3_transaction tran(db);
	if (m_table == table_aux) {
		sqlite3x::sqlite3_command cmd(db, "INSERT OR REPLACE INTO labels_deleted (ID) VALUES (?);");
		cmd.bind(1, (long long int)m_id);
		cmd.executenonquery();
		m_table = table_main;
		m_id = 0;
	}
	if (!m_id) {
		m_id = 1;
		{
			sqlite3x::sqlite3_command cmd(db, "SELECT MAX(ID) FROM labels;");
			sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
			if (cursor.step())
				m_id = cursor.getint(0) + 1;
		}
		{
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO labels (ID) VALUES(?);");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		if (rtree) {
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO labels_rtree (ID,SWLAT,NELAT,SWLON,NELON) VALUES(?,?,?,?,?);");
			cmd.bind(1, (long long int)m_id);
			cmd.bind(2, m_coord.get_lat());
			cmd.bind(3, m_coord.get_lat());
			cmd.bind(4, m_coord.get_lon());
			cmd.bind(5, m_coord.get_lon());
			cmd.executenonquery();
		}
	}
	{
		sqlite3x::sqlite3_command cmd(db, "UPDATE labels SET MUTABLE=?,LAT=?,LON=?,LABELPLACEMENT=?,METRIC=?,SUBID=?,SUBTABLE=?,TILE=? WHERE ID=?;");
		cmd.bind(1, (int)m_mutable);
		cmd.bind(2, m_coord.get_lat());
		cmd.bind(3, m_coord.get_lon());
		cmd.bind(4, m_label_placement);
		cmd.bind(5, m_metric);
		cmd.bind(6, (long long int)m_subid);
		cmd.bind(7, (int)m_subtable);
		cmd.bind(8, (int)TileNumber::to_tilenumber(get_coord()));
		cmd.bind(9, (long long int)m_id);
		cmd.executenonquery();
	}
	if (rtree) {
		sqlite3x::sqlite3_command cmd(db, "UPDATE labels_rtree SET SWLAT=?,NELAT=?,SWLON=?,NELON=? WHERE ID=?;");
		cmd.bind(1, m_coord.get_lat());
		cmd.bind(2, m_coord.get_lat());
		cmd.bind(3, m_coord.get_lon());
		cmd.bind(4, m_coord.get_lon());
		cmd.bind(5, (long long int)m_id);
		cmd.executenonquery();
	}
	tran.commit();
}

void DbBaseElements::Label::erase(sqlite3x::sqlite3_connection & db, bool rtree, bool aux_rtree)
{
	if (!is_valid())
		return;
	sqlite3x::sqlite3_transaction tran(db);
	if (m_table == table_aux) {
		sqlite3x::sqlite3_command cmd(db, "INSERT OR REPLACE INTO labels_deleted (ID) VALUES (?);");
		cmd.bind(1, (long long int)m_id);
		cmd.executenonquery();
		m_table = table_main;
	} else {
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM labels WHERE ID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		if (rtree) {
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM labels_rtree WHERE ID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
	}
	tran.commit();
	m_id = 0;
}

#ifdef HAVE_PQXX

void DbBaseElements::Label::load(const pqxx::tuple& cursor, pqxx::basic_transaction& w, unsigned int loadsubtables)
{
	m_id = 0;
	m_table = table_main;
	m_label_placement = label_off;
	if (cursor.empty())
		return;
	m_id = cursor[0].as<int64_t>(0);
	m_table = (table_t)cursor[1].as<int>(0);
	m_mutable = !!cursor[2].as<int>(0);
	m_coord = Point(cursor[4].as<Point::coord_t>(0), cursor[3].as<Point::coord_t>(0x80000000));
	m_label_placement = (label_placement_t)cursor[5].as<int>((int)label_off);
	m_metric = cursor[4].as<double>(0);
	m_subid = cursor[7].as<int64_t>(0);
	m_subtable = (table_t)cursor[8].as<int>(0);
}

void DbBaseElements::Label::save(pqxx::connection& conn, bool rtree)
{
	pqxx::work w(conn);
	if (!m_id) {
		pqxx::result r(w.exec("INSERT INTO aviationdb.labels (ID) SELECT COALESCE(MAX(ID),0)+1 FROM aviationdb.labels RETURNING ID;"));
		m_id = r.front()[0].as<int64_t>();
		if (rtree)
			w.exec("INSERT INTO aviationdb.labels_rtree (ID,SWLAT,NELAT,SWLON,NELON) VALUES(" + w.quote(m_id) + "," + w.quote(m_coord.get_lat()) +
			       "," + w.quote(m_coord.get_lat()) + "," + w.quote(m_coord.get_lon()) + "," + w.quote(m_coord.get_lon()) + ");");
	}
	w.exec("UPDATE aviationdb.labels SET MUTABLE=" + w.quote((int)m_mutable) +
	       ",LAT=" + w.quote(m_coord.get_lat()) + ",LON=" + w.quote(m_coord.get_lon()) +
	       ",LABELPLACEMENT=" + w.quote((int)m_label_placement) + 
	       ",METRIC=" + w.quote(m_metric) + ",SUBID=" + w.quote(m_subid) + ",SUBTABLE=" + w.quote((int)m_subtable) +
	       ",TILE=" + w.quote(TileNumber::to_tilenumber(m_coord)) + " WHERE ID=" + w.quote(m_id) + ";");
	if (rtree)
		w.exec("UPDATE aviationdb.labels_rtree SET SWLAT=" + w.quote(m_coord.get_lat()) + ",NELAT=" + w.quote(m_coord.get_lat()) +
		       ",SWLON=" + w.quote(m_coord.get_lon()) + ",NELON=" + w.quote(m_coord.get_lon()) + " WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
}

void DbBaseElements::Label::erase(pqxx::connection& conn, bool rtree)
{
	if (!is_valid())
		return;
	pqxx::work w(conn);
        w.exec("DELETE FROM aviationdb.labels WHERE ID=" + w.quote(m_id) + ";");
	if (rtree)
		w.exec("DELETE FROM aviationdb.labels_rtree WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
	m_id = 0;
}

void DbBaseElements::Label::update_index(pqxx::connection& conn, bool rtree)
{
	if (!m_id)
		return;
	pqxx::work w(conn);
	w.exec("UPDATE aviationdb.labels SET TILE=" + w.quote(TileNumber::to_tilenumber(get_coord())) + " WHERE ID=" + w.quote(m_id) + ";");
	if (rtree)
		w.exec("UPDATE aviationdb.labels_rtree SET SWLAT=" + w.quote(m_coord.get_lat()) + ",NELAT=" + w.quote(m_coord.get_lat()) +
		       ",SWLON=" + w.quote(m_coord.get_lon()) + ",NELON=" + w.quote(m_coord.get_lon()) + " WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
}

#endif

template<> void DbBase<DbBaseElements::Label>::drop_tables(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS labels;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS labels_deleted;"); cmd.executenonquery(); }
}

template<> void DbBase<DbBaseElements::Label>::create_tables(void)
{
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS labels (ID INTEGER PRIMARY KEY NOT NULL, "
					      "MUTABLE INTEGER,"
					      "LAT INTEGER,"
					      "LON INTEGER,"
					      "LABELPLACEMENT INTEGER,"
					      "METRIC REAL,"
					      "SUBID INTEGER,"
					      "SUBTABLE INTEGER,"
					      "TILE INTEGER);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS labels_deleted(SRCID TEXT PRIMARY KEY NOT NULL);");
		cmd.executenonquery();
	}
}

template<> void DbBase<DbBaseElements::Label>::drop_indices(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS labels_latlon;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS labels_metric;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS labels_tile;"); cmd.executenonquery(); }
	// old indices
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS labels_lat;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS labels_lon;"); cmd.executenonquery(); }
}

template<> void DbBase<DbBaseElements::Label>::create_indices(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS labels_latlon ON labels(LAT,LON);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS labels_metric ON labels(MUTABLE,METRIC);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS labels_tile ON labels(TILE);"); cmd.executenonquery(); }
	// old indices
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS labels_lat ON labels(LAT);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS labels_lon ON labels(LON);"); cmd.executenonquery(); }
}

template<> const char *DbBase<DbBaseElements::Label>::main_table_name = "labels";
template<> const char *DbBase<DbBaseElements::Label>::database_file_name = "labels.db";
template<> const char *DbBase<DbBaseElements::Label>::order_field = "ID";
template<> const char *DbBase<DbBaseElements::Label>::delete_field = "ID";
template<> const bool DbBase<DbBaseElements::Label>::area_data = false;

LabelsDb::elementvector_t LabelsDb::find_mutable_by_metric(unsigned int limit, unsigned int offset, unsigned int loadsubtables)
{
	std::string qs("SELECT " + std::string(element_t::db_query_string) + " FROM " + main_table_name + " WHERE MUTABLE==1 ORDER BY METRIC DESC");
	if (limit)
		qs += " LIMIT ?";
	if (offset)
		qs += " OFFSET ?";
	qs += ";";
	if (false)
		std::cerr << "sql: " << qs << std::endl;
	sqlite3x::sqlite3_command cmd(m_db, qs);
	int nr = 1;
	if (limit)
		cmd.bind(nr++, (long long int)limit);
	if (offset)
		cmd.bind(nr++, (long long int)offset);
	std::vector<element_t> ret;
	sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
	for (;;) {
		ret.push_back(element_t());
		ret.back().load(cursor, m_db);
		if (ret.back().is_valid())
			continue;
		ret.pop_back();
		break;
	}
	return ret;
}

#ifdef HAVE_PQXX

template<> void PGDbBase<DbBaseElements::Label>::drop_tables(void)
{
	pqxx::work w(m_conn);
	w.exec("DROP TABLE IF EXISTS aviationdb.labels;");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Label>::create_tables(void)
{
	create_common_tables();
	pqxx::work w(m_conn);
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.labels ("
	       "MUTABLE INTEGER,"
	       "METRIC REAL,"
	       "SUBID INTEGER,"
	       "SUBTABLE INTEGER,"
	       "TILE INTEGER,"
	       "PRIMARY KEY (ID)"
	       ") INHERITS (aviationdb.labelcoordbase);");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Label>::drop_indices(void)
{
	pqxx::work w(m_conn);
	w.exec("DROP INDEX IF EXISTS aviationdb.labels_latlon;");
	w.exec("DROP INDEX IF EXISTS aviationdb.labels_metric;");
	w.exec("DROP INDEX IF EXISTS aviationdb.labels_tile;");
	// old indices
	w.exec("DROP INDEX IF EXISTS aviationdb.labels_lat;");
	w.exec("DROP INDEX IF EXISTS aviationdb.labels_lon;");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Label>::create_indices(void)
{
	pqxx::work w(m_conn);
	w.exec("CREATE INDEX IF NOT EXISTS labels_latlon ON aviationdb.labels(LAT,LON);");
	w.exec("CREATE INDEX IF NOT EXISTS labels_metric ON aviationdb.labels(MUTABLE,METRIC);");
	w.exec("CREATE INDEX IF NOT EXISTS labels_tile ON aviationdb.labels(TILE);");
	// old indices
	w.exec("CREATE INDEX IF NOT EXISTS labels_lat ON aviationdb.labels(LAT);");
	w.exec("CREATE INDEX IF NOT EXISTS labels_lon ON aviationdb.labels(LON);");
	w.commit();
}

template<> const char *PGDbBase<DbBaseElements::Label>::main_table_name = "aviationdb.labels";
template<> const char *PGDbBase<DbBaseElements::Label>::order_field = "ID";
template<> const char *PGDbBase<DbBaseElements::Label>::delete_field = "ID";
template<> const bool PGDbBase<DbBaseElements::Label>::area_data = false;

LabelsPGDb::elementvector_t LabelsPGDb::find_mutable_by_metric(unsigned int limit, unsigned int offset, unsigned int loadsubtables)
{
	pqxx::read_transaction w(m_conn);
	std::string qs("SELECT " + std::string(element_t::db_query_string) + " FROM " + main_table_name + " WHERE MUTABLE==1 ORDER BY METRIC DESC");
	if (limit)
		qs += " LIMIT " + w.quote(limit);
	if (offset)
		qs += " OFFSET " + w.quote(offset);
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
