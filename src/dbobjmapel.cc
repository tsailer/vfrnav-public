//
// C++ Implementation: dbobjmapel
//
// Description: Database Objects: Mapelements
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

const unsigned int DbBaseElements::Mapelement::subtables_all;
const unsigned int DbBaseElements::Mapelement::subtables_none;

const uint8_t DbBaseElements::Mapelement::maptyp_invalid;
const uint8_t DbBaseElements::Mapelement::maptyp_railway;
const uint8_t DbBaseElements::Mapelement::maptyp_railway_d;
const uint8_t DbBaseElements::Mapelement::maptyp_aerial_cable;
const uint8_t DbBaseElements::Mapelement::maptyp_highway;
const uint8_t DbBaseElements::Mapelement::maptyp_road;
const uint8_t DbBaseElements::Mapelement::maptyp_trail;
const uint8_t DbBaseElements::Mapelement::maptyp_river;
const uint8_t DbBaseElements::Mapelement::maptyp_lake;
const uint8_t DbBaseElements::Mapelement::maptyp_river_t;
const uint8_t DbBaseElements::Mapelement::maptyp_lake_t;
const uint8_t DbBaseElements::Mapelement::maptyp_canal;
const uint8_t DbBaseElements::Mapelement::maptyp_pack_ice;
const uint8_t DbBaseElements::Mapelement::maptyp_glacier;
const uint8_t DbBaseElements::Mapelement::maptyp_forest;
const uint8_t DbBaseElements::Mapelement::maptyp_city;
const uint8_t DbBaseElements::Mapelement::maptyp_village;
const uint8_t DbBaseElements::Mapelement::maptyp_spot;
const uint8_t DbBaseElements::Mapelement::maptyp_landmark;
const uint8_t DbBaseElements::Mapelement::maptyp_powerline;
const uint8_t DbBaseElements::Mapelement::maptyp_border;

const char *DbBaseElements::Mapelement::db_query_string = "mapelements.ID AS ID,0 AS \"TABLE\",mapelements.TYPECODE,mapelements.NAME,mapelements.LAT,mapelements.LON,mapelements.SWLAT,mapelements.SWLON,mapelements.NELAT,mapelements.NELON,mapelements.POLY";
const char *DbBaseElements::Mapelement::db_aux_query_string = "aux.mapelements.ID AS ID,1 AS \"TABLE\",aux.mapelements.TYPECODE,aux.mapelements.NAME,aux.mapelements.LAT,aux.mapelements.LON,aux.mapelements.SWLAT,aux.mapelements.SWLON,aux.mapelements.NELAT,aux.mapelements.NELON,aux.mapelements.POLY";
const char *DbBaseElements::Mapelement::db_text_fields[] = { "NAME", 0 };
const char *DbBaseElements::Mapelement::db_time_fields[] = { 0 };

DbBaseElements::Mapelement::Mapelement(void)
	: m_id(0), m_bbox(), m_typecode(maptyp_invalid), m_table(table_main), m_name(), m_polygon()
{
	m_labelcoord.set_invalid();
}

DbBaseElements::Mapelement::Mapelement(uint8_t tc, const PolygonHole & po, const Glib::ustring nm)
	: m_id(0), m_bbox(), m_typecode(tc), m_table(table_main), m_name(nm), m_labelcoord(), m_polygon(po)
{
	m_bbox = m_polygon.get_bbox();
	recompute_label_coord();
}

DbBaseElements::Mapelement::Mapelement(uint8_t tc, const PolygonHole & po, const Point & lblcoord, const Glib::ustring nm)
	: m_id(0), m_bbox(), m_typecode(tc), m_table(table_main), m_name(nm), m_labelcoord(lblcoord), m_polygon(po)
{
	m_bbox = m_polygon.get_bbox();
}

void DbBaseElements::Mapelement::load( sqlite3x::sqlite3_cursor & cursor, sqlite3x::sqlite3_connection & db, unsigned int loadsubtables )
{
	m_id = 0;
	m_table = table_main;
	m_typecode = maptyp_invalid;
	if (!cursor.step())
		return;
	m_id = cursor.getint(0);
	m_table = (table_t)cursor.getint(1);
	m_typecode = cursor.getint(2);
	m_name = cursor.getstring(3);
	m_labelcoord = Point(cursor.getint(5), cursor.getint(4));
	m_bbox = Rect(Point(cursor.getint(7), cursor.getint(6)), Point(cursor.getint(9), cursor.getint(8)));
	m_polygon.getblob(cursor, 10);
	recompute_bbox_label();
}

void DbBaseElements::Mapelement::update_index(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree)
{
	if (m_table != table_main || !m_id)
		return;
	if (false)
		std::cerr << "Start transaction" << std::endl;
	sqlite3x::sqlite3_transaction tran(db);
	{
		sqlite3x::sqlite3_command cmd(db, "UPDATE mapelements SET TILE=?2 WHERE ID=?1;");
		cmd.bind(1, (long long int)m_id);
		cmd.bind(2, (int)TileNumber::to_tilenumber(m_bbox));
		cmd.executenonquery();
	}
	if (rtree) {
		sqlite3x::sqlite3_command cmd(db, "UPDATE mapelements_rtree SET SWLAT=?,NELAT=?,SWLON=?,NELON=? WHERE ID=?;");
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

void DbBaseElements::Mapelement::save(sqlite3x::sqlite3_connection & db, bool rtree, bool aux_rtree)
{
	sqlite3x::sqlite3_transaction tran(db);
	if (m_table == table_aux) {
		sqlite3x::sqlite3_command cmd(db, "INSERT OR REPLACE INTO mapelements_deleted (ID) VALUES (?);");
		cmd.bind(1, (long long int)m_id);
		cmd.executenonquery();
		m_table = table_main;
		m_id = 0;
	}
	if (!m_id) {
		m_id = 1;
		{
			sqlite3x::sqlite3_command cmd(db, "SELECT MAX(ID) FROM mapelements;");
			sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
			if (cursor.step())
				m_id = cursor.getint(0) + 1;
		}
		{
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO mapelements (ID) VALUES(?);");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		if (rtree) {
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO mapelements_rtree (ID,SWLAT,NELAT,SWLON,NELON) VALUES(?,?,?,?,?);");
			cmd.bind(1, (long long int)m_id);
			cmd.bind(2, m_bbox.get_south());
			cmd.bind(3, m_bbox.get_north());
			cmd.bind(4, m_bbox.get_west());
			cmd.bind(5, (long long int)m_bbox.get_east_unwrapped());
			cmd.executenonquery();
		}
	}
	// Update Mapelement Record
	{
		sqlite3x::sqlite3_command cmd(db, "UPDATE mapelements SET TYPECODE=?,NAME=?,LAT=?,LON=?,SWLAT=?,SWLON=?,NELAT=?,NELON=?,POLY=?,TILE=? WHERE ID=?;");
		cmd.bind(1, m_typecode);
		cmd.bind(2, m_name);
		cmd.bind(3, m_labelcoord.get_lat());
		cmd.bind(4, m_labelcoord.get_lon());
		cmd.bind(5, m_bbox.get_south());
		cmd.bind(6, m_bbox.get_west());
		cmd.bind(7, m_bbox.get_north());
		cmd.bind(8, (long long int)m_bbox.get_east_unwrapped());
		m_polygon.bindblob(cmd, 9);
		cmd.bind(10, (int)TileNumber::to_tilenumber(m_bbox));
		cmd.bind(11, (long long int)m_id);
		cmd.executenonquery();
	}
	if (rtree) {
		sqlite3x::sqlite3_command cmd(db, "UPDATE mapelements_rtree SET SWLAT=?,NELAT=?,SWLON=?,NELON=? WHERE ID=?;");
		cmd.bind(1, m_bbox.get_south());
		cmd.bind(2, m_bbox.get_north());
		cmd.bind(3, m_bbox.get_west());
		cmd.bind(4, (long long int)m_bbox.get_east_unwrapped());
		cmd.bind(5, (long long int)m_id);
		cmd.executenonquery();
	}
	tran.commit();
}

void DbBaseElements::Mapelement::erase(sqlite3x::sqlite3_connection & db, bool rtree, bool aux_rtree)
{
	if (!is_valid())
		return;
	sqlite3x::sqlite3_transaction tran(db);
	if (m_table == table_aux) {
		sqlite3x::sqlite3_command cmd(db, "INSERT OR REPLACE INTO mapelements_deleted (ID) VALUES (?);");
		cmd.bind(1, (long long int)m_id);
		cmd.executenonquery();
		m_table = table_main;
	} else {
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM mapelements WHERE ID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		if (rtree) {
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM mapelements_rtree WHERE ID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
	}
	tran.commit();
	m_id = 0;
}

#ifdef HAVE_PQXX

void DbBaseElements::Mapelement::load(const pqxx::tuple& cursor, pqxx::basic_transaction& w, unsigned int loadsubtables)
{
	m_id = 0;
	m_table = table_main;
	m_typecode = maptyp_invalid;
	if (cursor.empty())
		return;
	m_id = cursor[0].as<int64_t>(0);
	m_table = (table_t)cursor[1].as<int>(0);
	m_typecode = cursor[2].as<int>(0);
	m_name = cursor[3].as<std::string>("");
	m_labelcoord = Point(cursor[5].as<Point::coord_t>(0), cursor[4].as<Point::coord_t>(0x80000000));
	m_bbox = Rect(Point(cursor[7].as<Point::coord_t>(0), cursor[6].as<Point::coord_t>(0x80000000)),
		      Point(cursor[9].as<Point::coord_t>(0), cursor[8].as<Point::coord_t>(0x80000000)));
	m_polygon.getblob(pqxx::binarystring(cursor[10]));
	recompute_bbox_label();
}

void DbBaseElements::Mapelement::save(pqxx::connection& conn, bool rtree)
{
	pqxx::work w(conn);
	if (!m_id) {
		pqxx::result r(w.exec("INSERT INTO aviationdb.mapelements (ID) SELECT COALESCE(MAX(ID),0)+1 FROM aviationdb.mapelements RETURNING ID;"));
		m_id = r.front()[0].as<int64_t>();
		if (rtree)
			w.exec("INSERT INTO aviationdb.mapelements_rtree (ID,SWLAT,NELAT,SWLON,NELON) VALUES(" + w.quote(m_id) + "," + w.quote(m_bbox.get_south()) +
			       "," + w.quote(m_bbox.get_north()) + "," + w.quote(m_bbox.get_west()) + "," + w.quote(m_bbox.get_east_unwrapped()) + ");");
	}
	// Update Mapelement Record
	{
		pqxx::binarystring poly(m_polygon.bindblob());
		std::string gpoly("NULL");
		if (is_area()) {
			if (m_polygon.get_exterior().size() >= 3)
				gpoly = PGDbBaseCommon::get_str(m_polygon);
		} else {
			if (m_polygon.get_exterior().size() >= 2)
				gpoly = PGDbBaseCommon::get_str((LineString)m_polygon.get_exterior());
		}
		w.exec("UPDATE aviationdb.mapelements SET TYPECODE=" + w.quote((int)m_typecode) + ",NAME=" + w.quote((std::string)m_name) +
		       ",LAT=" + w.quote(m_labelcoord.get_lat()) + ",LON=" + w.quote(m_labelcoord.get_lon()) +
		       ",SWLAT=" + w.quote(m_bbox.get_south()) + ",SWLON=" + w.quote(m_bbox.get_west()) +
		       ",NELAT=" + w.quote(m_bbox.get_north()) + ",NELON=" + w.quote((long long int)m_bbox.get_east_unwrapped()) +
		       ",POLY=" + w.quote(poly) + ",GPOLY=" + gpoly +
		       ",TILE=" + w.quote(TileNumber::to_tilenumber(m_bbox)) + " WHERE ID=" + w.quote(m_id) + ";");
	}
	if (rtree)
		w.exec("UPDATE aviationdb.mapelements_rtree SET SWLAT=" + w.quote(m_bbox.get_south()) + ",NELAT=" + w.quote(m_bbox.get_north()) +
		       ",SWLON=" + w.quote(m_bbox.get_west()) + ",NELON=" + w.quote((long long int)m_bbox.get_east_unwrapped()) + " WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
}

void DbBaseElements::Mapelement::erase(pqxx::connection& conn, bool rtree)
{
	if (!is_valid())
		return;
	pqxx::work w(conn);
	w.exec("DELETE FROM aviationdb.mapelements WHERE ID=" + w.quote(m_id) + ";");
	if (rtree)
		w.exec("DELETE FROM aviationdb.mapelements_rtree WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
	m_id = 0;
}

void DbBaseElements::Mapelement::update_index(pqxx::connection& conn, bool rtree)
{
	if (!m_id)
		return;
	pqxx::work w(conn);
	w.exec("UPDATE aviationdb.mapelements SET TILE=" + w.quote(TileNumber::to_tilenumber(m_bbox)) + " WHERE ID=" + w.quote(m_id) + ";");
	if (rtree)
		w.exec("UPDATE aviationdb.mapelements_rtree SET SWLAT=" + w.quote(m_bbox.get_south()) + ",NELAT=" + w.quote(m_bbox.get_north()) +
		       ",SWLON=" + w.quote(m_bbox.get_west()) + ",NELON=" + w.quote((long long int)m_bbox.get_east_unwrapped()) + " WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
}

#endif

bool DbBaseElements::Mapelement::is_area(uint8_t tc)
{
	switch (tc) {
	case maptyp_lake:
	case maptyp_lake_t:
	case maptyp_pack_ice:
	case maptyp_glacier:
	case maptyp_forest:
	case maptyp_city:
	case maptyp_border:
		return true;

	default:
		return false;
	}
}

const Glib::ustring& DbBaseElements::Mapelement::get_typename(uint8_t tc)
{
	switch (tc) {
	default:
	{
		static const Glib::ustring r("Invalid");
		return r;
	}

	case maptyp_railway:
	{
		static const Glib::ustring r("Railway");
		return r;
	}

	case maptyp_railway_d:
	{
		static const Glib::ustring r("Railway (multitrack)");
		return r;
	}

	case maptyp_aerial_cable:
	{
		static const Glib::ustring r("Aerial Cable");
		return r;
	}

	case maptyp_highway:
	{
		static const Glib::ustring r("Highway");
		return r;
	}

	case maptyp_road:
	{
		static const Glib::ustring r("Road");
		return r;
	}

	case maptyp_trail:
	{
		static const Glib::ustring r("Trail");
		return r;
	}

	case maptyp_river:
	case maptyp_river_t:
	{
		static const Glib::ustring r("River");
		return r;
	}

	case maptyp_lake:
	case maptyp_lake_t:
	{
		static const Glib::ustring r("Lake");
		return r;
	}

	case maptyp_canal:
	{
		static const Glib::ustring r("Canal");
		return r;
	}

	case maptyp_pack_ice:
	{
		static const Glib::ustring r("Ice");
		return r;
	}

	case maptyp_glacier:
	{
		static const Glib::ustring r("Glacier");
		return r;
	}

	case maptyp_forest:
	{
		static const Glib::ustring r("Forest");
		return r;
	}

	case maptyp_city:
	{
		static const Glib::ustring r("City");
		return r;
	}

	case maptyp_village:
	{
		static const Glib::ustring r("Village");
		return r;
	}

	case maptyp_spot:
	{
		static const Glib::ustring r("Spot");
		return r;
	}

	case maptyp_landmark:
	{
		static const Glib::ustring r("Landmark");
		return r;
	}

	case maptyp_powerline:
	{
		static const Glib::ustring r("Powerline");
		return r;
	}

	case maptyp_border:
	{
		static const Glib::ustring r("Border");
		return r;
	}
	}
}

void DbBaseElements::Mapelement::recompute_bbox_label(void)
{
	m_bbox = m_polygon.get_bbox();
	recompute_label_coord();
}

int DbBaseElements::Mapelement::lon_center_poly(int32_t lat)
{
	const PolygonSimple& poly(get_exterior());
	if (!poly.size())
		return -1;
	lat -= m_bbox.get_south();
	std::vector<int32_t> isect;
	for (unsigned int j = poly.size()-1, i = 0; i < poly.size(); j = i, i++) {
		Point p1 = poly[j] - m_bbox.get_southwest();
		Point p2 = poly[i] - m_bbox.get_southwest();
		Point d = p2 - p1;
		if (!d.get_lat())
			continue;
		float v = p1.get_lon() * (float)d.get_lat() - p1.get_lat() * (float)d.get_lon();
		if (p1.get_lat() > p2.get_lat()) {
			p2.set_lon(p2.get_lat());
			p2.set_lat(p1.get_lat());
			p1.set_lat(p2.get_lon());
		}
		if (lat < p1.get_lat() || lat >= p2.get_lat())
			continue;
		isect.push_back((int32_t)((v + lat * (float)d.get_lon()) / (float)d.get_lat()));
	}
	if (isect.size() < 2)
		return -1;
	sort(isect.begin(), isect.end());
	unsigned int i = (isect.size() >> 1) & ~1;
	m_labelcoord = m_bbox.get_southwest() + Point((isect[i] + isect[i+1]) / 2, lat);
	return 0;
}

void DbBaseElements::Mapelement::recompute_label_coord(void)
{
	Point center(m_bbox.boxcenter());
	if (lon_center_poly(center.get_lat()))
		m_labelcoord = center;
}

template<> void DbBase<DbBaseElements::Mapelement>::drop_tables(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS mapelements;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS mapelements_deleted;"); cmd.executenonquery(); }
}

template<> void DbBase<DbBaseElements::Mapelement>::create_tables(void)
{
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS mapelements (ID INTEGER PRIMARY KEY NOT NULL, "
					      "TYPECODE INTEGER,"
					      "NAME TEXT,"
					      "LAT INTEGER,"
					      "LON INTEGER,"
					      "SWLAT INTEGER,"
					      "SWLON INTEGER,"
					      "NELAT INTEGER,"
					      "NELON INTEGER,"
					      "POLY BLOB,"
					      "TILE INTEGER);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS mapelements_deleted(ID INTEGER PRIMARY KEY NOT NULL);");
		cmd.executenonquery();
	}
}

template<> void DbBase<DbBaseElements::Mapelement>::drop_indices(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS mapelements_name;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS mapelements_latlon;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS mapelements_bbox;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS mapelements_tile;"); cmd.executenonquery(); }
	// old indices
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS mapelements_swlat;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS mapelements_swlon;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS mapelements_nelat;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS mapelements_nelon;"); cmd.executenonquery(); }
}

template<> void DbBase<DbBaseElements::Mapelement>::create_indices(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS mapelements_name ON mapelements(NAME COLLATE NOCASE);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS mapelements_latlon ON mapelements(LAT,LON);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS mapelements_bbox ON mapelements(SWLAT,NELAT,SWLON,NELON);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS mapelements_tile ON mapelements(TILE);"); cmd.executenonquery(); }
	// old indices
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS mapelements_swlat ON mapelements(SWLAT);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS mapelements_swlon ON mapelements(SWLON);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS mapelements_nelat ON mapelements(NELAT);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS mapelements_nelon ON mapelements(NELON);"); cmd.executenonquery(); }
}

template<> const char *DbBase<DbBaseElements::Mapelement>::main_table_name = "mapelements";
template<> const char *DbBase<DbBaseElements::Mapelement>::database_file_name = "mapelements.db";
template<> const char *DbBase<DbBaseElements::Mapelement>::order_field = "ID";
template<> const char *DbBase<DbBaseElements::Mapelement>::delete_field = "ID";
template<> const bool DbBase<DbBaseElements::Mapelement>::area_data = true;

#ifdef HAVE_PQXX

template<> void PGDbBase<DbBaseElements::Mapelement>::drop_tables(void)
{
	pqxx::work w(m_conn);
	w.exec("DROP TABLE IF EXISTS aviationdb.mapelements;");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Mapelement>::create_tables(void)
{
	pqxx::work w(m_conn);
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.mapelements ("
	       "ID BIGINT PRIMARY KEY NOT NULL, "
	       "TYPECODE SMALLINT,"
	       "NAME TEXT,"
	       "LAT INTEGER,"
	       "LON INTEGER,"
	       "SWLAT INTEGER,"
	       "SWLON INTEGER,"
	       "NELAT INTEGER,"
	       "NELON BIGINT,"
	       "POLY BYTEA,"
	       "GPOLY ext.GEOGRAPHY(GEOMETRY,4326),"
	       "TILE INTEGER"
	       ");");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Mapelement>::drop_indices(void)
{
	pqxx::work w(m_conn);
	w.exec("DROP INDEX IF EXISTS aviationdb.mapelements_name;");
	w.exec("DROP INDEX IF EXISTS aviationdb.mapelements_latlon;");
	w.exec("DROP INDEX IF EXISTS aviationdb.mapelements_bbox;");
	w.exec("DROP INDEX IF EXISTS aviationdb.mapelements_tile;");
	// old indices
	w.exec("DROP INDEX IF EXISTS aviationdb.mapelements_swlat;");
	w.exec("DROP INDEX IF EXISTS aviationdb.mapelements_swlon;");
	w.exec("DROP INDEX IF EXISTS aviationdb.mapelements_nelat;");
	w.exec("DROP INDEX IF EXISTS aviationdb.mapelements_nelon;");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Mapelement>::create_indices(void)
{
	pqxx::work w(m_conn);
	w.exec("CREATE INDEX IF NOT EXISTS mapelements_name ON aviationdb.mapelements((NAME::ext.CITEXT) COLLATE \"C\");");
	w.exec("CREATE INDEX IF NOT EXISTS mapelements_latlon ON aviationdb.mapelements(LAT,LON);");
	w.exec("CREATE INDEX IF NOT EXISTS mapelements_bbox ON aviationdb.mapelements(SWLAT,NELAT,SWLON,NELON);");
	w.exec("CREATE INDEX IF NOT EXISTS mapelements_tile ON aviationdb.mapelements(TILE);");
	// old indices
	w.exec("CREATE INDEX IF NOT EXISTS mapelements_swlat ON aviationdb.mapelements(SWLAT);");
	w.exec("CREATE INDEX IF NOT EXISTS mapelements_swlon ON aviationdb.mapelements(SWLON);");
	w.exec("CREATE INDEX IF NOT EXISTS mapelements_nelat ON aviationdb.mapelements(NELAT);");
	w.exec("CREATE INDEX IF NOT EXISTS mapelements_nelon ON aviationdb.mapelements(NELON);");
	w.commit();
}

template<> const char *PGDbBase<DbBaseElements::Mapelement>::main_table_name = "aviationdb.mapelements";
template<> const char *PGDbBase<DbBaseElements::Mapelement>::order_field = "ID";
template<> const char *PGDbBase<DbBaseElements::Mapelement>::delete_field = "ID";
template<> const bool PGDbBase<DbBaseElements::Mapelement>::area_data = true;

#endif
