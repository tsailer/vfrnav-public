//
// C++ Implementation: dbobjaspc
//
// Description: Database Objects: Airspace
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

DbBaseElements::Airspace::Segment::Segment(void)
	: m_radius1(0), m_radius2(0), m_shape(0)
{
	m_coord1.set_invalid();
	m_coord2.set_invalid();
	m_coord0.set_invalid();
}

DbBaseElements::Airspace::Segment::Segment(const Point& coord1, const Point& coord2, const Point& coord0, char shape, int32_t r1, int32_t r2)
	: m_coord1(coord1), m_coord2(coord2), m_coord0(coord0), m_radius1(r1), m_radius2(r2), m_shape(shape)
{
}

DbBaseElements::Airspace::Component::Component(const Glib::ustring& icao, uint8_t tc, char bc, operator_t opr)
	: m_icao(icao), m_typecode(tc), m_bdryclass(bc), m_operator(opr)
{
}

const Glib::ustring& DbBaseElements::Airspace::Component::get_operator_string(operator_t opr)
{
	switch (opr) {
	case operator_set:
	{
		static const Glib::ustring r("set");
		return r;
	}

	case operator_union:
	{
		static const Glib::ustring r("union");
		return r;
	}

	case operator_subtract:
	{
		static const Glib::ustring r("subtract");
		return r;
	}

	case operator_intersect:
	{
		static const Glib::ustring r("intersect");
		return r;
	}

	default:
	{
		static const Glib::ustring r("invalid");
		return r;
	}
	}
}

void DbBaseElements::Airspace::Component::set_operator_string(const Glib::ustring& oprstr)
{
	if (oprstr.empty())
		return;
	for (operator_t opr = operator_set; opr <= operator_intersect; opr = (operator_t)(opr + 1)) {
		if (oprstr != get_operator_string(opr))
			continue;
		set_operator(opr);
		return;
	}
	throw std::runtime_error("Airspace::Component::set_operator: invalid operator \"" + oprstr + "\"");
}

const uint8_t DbBaseElements::Airspace::altflag_agl;
const uint8_t DbBaseElements::Airspace::altflag_fl;
const uint8_t DbBaseElements::Airspace::altflag_sfc;
const uint8_t DbBaseElements::Airspace::altflag_gnd;
const uint8_t DbBaseElements::Airspace::altflag_unlim;
const uint8_t DbBaseElements::Airspace::altflag_notam;
const uint8_t DbBaseElements::Airspace::altflag_unkn;

const uint8_t DbBaseElements::Airspace::typecode_specialuse;
const uint8_t DbBaseElements::Airspace::typecode_ead;

const char DbBaseElements::Airspace::bdryclass_ead_a;
const char DbBaseElements::Airspace::bdryclass_ead_adiz;
const char DbBaseElements::Airspace::bdryclass_ead_ama;
const char DbBaseElements::Airspace::bdryclass_ead_arinc;
const char DbBaseElements::Airspace::bdryclass_ead_asr;
const char DbBaseElements::Airspace::bdryclass_ead_awy;
const char DbBaseElements::Airspace::bdryclass_ead_cba;
const char DbBaseElements::Airspace::bdryclass_ead_class;
const char DbBaseElements::Airspace::bdryclass_ead_cta;
const char DbBaseElements::Airspace::bdryclass_ead_cta_p;
const char DbBaseElements::Airspace::bdryclass_ead_ctr;
const char DbBaseElements::Airspace::bdryclass_ead_ctr_p;
const char DbBaseElements::Airspace::bdryclass_ead_d_amc;
const char DbBaseElements::Airspace::bdryclass_ead_d;
const char DbBaseElements::Airspace::bdryclass_ead_d_other;
const char DbBaseElements::Airspace::bdryclass_ead_ead;
const char DbBaseElements::Airspace::bdryclass_ead_fir;
const char DbBaseElements::Airspace::bdryclass_ead_fir_p;
const char DbBaseElements::Airspace::bdryclass_ead_nas;
const char DbBaseElements::Airspace::bdryclass_ead_no_fir;
const char DbBaseElements::Airspace::bdryclass_ead_oca;
const char DbBaseElements::Airspace::bdryclass_ead_oca_p;
const char DbBaseElements::Airspace::bdryclass_ead_ota;
const char DbBaseElements::Airspace::bdryclass_ead_part;
const char DbBaseElements::Airspace::bdryclass_ead_p;
const char DbBaseElements::Airspace::bdryclass_ead_political;
const char DbBaseElements::Airspace::bdryclass_ead_protect;
const char DbBaseElements::Airspace::bdryclass_ead_r_amc;
const char DbBaseElements::Airspace::bdryclass_ead_ras;
const char DbBaseElements::Airspace::bdryclass_ead_rca;
const char DbBaseElements::Airspace::bdryclass_ead_r;
const char DbBaseElements::Airspace::bdryclass_ead_sector_c;
const char DbBaseElements::Airspace::bdryclass_ead_sector;
const char DbBaseElements::Airspace::bdryclass_ead_tma;
const char DbBaseElements::Airspace::bdryclass_ead_tma_p;
const char DbBaseElements::Airspace::bdryclass_ead_tra;
const char DbBaseElements::Airspace::bdryclass_ead_tsa;
const char DbBaseElements::Airspace::bdryclass_ead_uir;
const char DbBaseElements::Airspace::bdryclass_ead_uir_p;
const char DbBaseElements::Airspace::bdryclass_ead_uta;
const char DbBaseElements::Airspace::bdryclass_ead_w;

const unsigned int DbBaseElements::Airspace::subtables_none;
const unsigned int DbBaseElements::Airspace::subtables_segments;
const unsigned int DbBaseElements::Airspace::subtables_components;
const unsigned int DbBaseElements::Airspace::subtables_all;

const char *DbBaseElements::Airspace::db_query_string = "airspaces.ID AS ID,0 AS \"TABLE\",airspaces.SRCID AS SRCID,airspaces.ICAO,airspaces.NAME,airspaces.IDENT,airspaces.CLASSEXCEPT,airspaces.EFFTIMES,airspaces.NOTE,airspaces.SWLAT,airspaces.SWLON,airspaces.NELAT,airspaces.NELON,airspaces.COMMNAME,airspaces.COMMFREQ0,airspaces.COMMFREQ1,airspaces.TYPECODE,airspaces.CLASS,airspaces.WIDTH,airspaces.ALTLOWER,airspaces.ALTLOWERFLAGS,airspaces.ALTUPPER,airspaces.ALTUPPERFLAGS,airspaces.GNDELEVMIN,airspaces.GNDELEVMAX,airspaces.LAT,airspaces.LON,airspaces.LABELPLACEMENT,airspaces.POLY,airspaces.MODIFIED";
const char *DbBaseElements::Airspace::db_aux_query_string = "aux.airspaces.ID AS ID,1 AS \"TABLE\",aux.airspaces.SRCID AS SRCID,aux.airspaces.ICAO,aux.airspaces.NAME,aux.airspaces.IDENT,aux.airspaces.CLASSEXCEPT,aux.airspaces.EFFTIMES,aux.airspaces.NOTE,aux.airspaces.SWLAT,aux.airspaces.SWLON,aux.airspaces.NELAT,aux.airspaces.NELON,aux.airspaces.COMMNAME,aux.airspaces.COMMFREQ0,aux.airspaces.COMMFREQ1,aux.airspaces.TYPECODE,aux.airspaces.CLASS,aux.airspaces.WIDTH,aux.airspaces.ALTLOWER,aux.airspaces.ALTLOWERFLAGS,aux.airspaces.ALTUPPER,aux.airspaces.ALTUPPERFLAGS,aux.airspaces.GNDELEVMIN,aux.airspaces.GNDELEVMAX,aux.airspaces.LAT,aux.airspaces.LON,aux.airspaces.LABELPLACEMENT,aux.airspaces.POLY,aux.airspaces.MODIFIED";
const char *DbBaseElements::Airspace::db_text_fields[] = { "SRCID", "ICAO", "NAME", "IDENT", "CLASSEXCEPT", "EFFTIMES", "NOTE", 0 };
const char *DbBaseElements::Airspace::db_time_fields[] = { "MODIFIED", 0 };;
const DbBaseElements::Airspace::gndelev_t DbBaseElements::Airspace::gndelev_unknown = std::numeric_limits<gndelev_t>::min();

DbBaseElements::Airspace::Airspace(void)
	: m_seg(), m_classexcept(), m_efftime(), m_note(), m_commname(),
	  m_bbox(), m_typecode(0), m_bdryclass(0), m_width(0), m_altlwr(0), m_altupr(0), m_altlwrflags(0),
	  m_altuprflags(0), m_gndelevmin(gndelev_unknown), m_gndelevmax(gndelev_unknown),
	  m_subtables(subtables_all)
{
	m_commfreq[0] = m_commfreq[1] = 0;
}

void DbBaseElements::Airspace::load_subtables(sqlite3x::sqlite3_connection & db, unsigned int loadsubtables)
{
	loadsubtables &= subtables_all & ~m_subtables;
	if (!loadsubtables)
		return;
	std::string tablepfx;
	switch (m_table) {
		case 0:
			break;
		case 1:
			tablepfx = "aux.";
			break;
		default:
			throw std::runtime_error("invalid table ID");
	}
	if (loadsubtables & subtables_segments) {
		m_seg.clear();
		sqlite3x::sqlite3_command cmd(db, "SELECT LAT1,LON1,LAT2,LON2,LAT0,LON0,SHAPE,RADIUS1,RADIUS2 FROM " + tablepfx + "airspacesegments WHERE BDRYID=? ORDER BY ID;");
		cmd.bind(1, (long long int)m_id);
		sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
		while (cursor.step()) {
			m_seg.push_back(Segment(Point(cursor.getint(1), cursor.getint(0)),
						Point(cursor.getint(3), cursor.getint(2)),
						Point(cursor.getint(5), cursor.getint(4)),
						cursor.getint(6), cursor.getint(7), cursor.getint(8)));
		}
		m_subtables |= subtables_segments;
	}
	if (loadsubtables & subtables_components) {
		m_comp.clear();
		sqlite3x::sqlite3_command cmd(db, "SELECT ICAO,TYPECODE,CLASS,OPERATOR FROM " + tablepfx + "airspacecomponents WHERE BDRYID=? ORDER BY ID;");
		cmd.bind(1, (long long int)m_id);
		sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
		while (cursor.step()) {
			m_comp.push_back(Component(cursor.getstring(0), cursor.getint(1), cursor.getint(2), (Component::operator_t)cursor.getint(3)));
		}
		m_subtables |= subtables_components;
	}
}

void DbBaseElements::Airspace::load( sqlite3x::sqlite3_cursor & cursor, sqlite3x::sqlite3_connection & db, unsigned int loadsubtables )
{
	m_id = 0;
	m_table = table_main;
	m_typecode = 0;
	m_bdryclass = 0;
	m_subtables = subtables_none;
	if (!cursor.step())
		return;
	m_id = cursor.getint(0);
	m_table = (table_t)cursor.getint(1);
	m_sourceid = cursor.getstring(2);
	m_icao = cursor.getstring(3);
	m_name = cursor.getstring(4);
	m_ident = cursor.getstring(5);
	m_classexcept = cursor.getstring(6);
	m_efftime = cursor.getstring(7);
	m_note = cursor.getstring(8);
	m_bbox = Rect(Point(cursor.getint(10), cursor.getint(9)), Point(cursor.getint(12), cursor.getint(11)));
	m_commname = cursor.getstring(13);
	m_commfreq[0] = cursor.getint(14);
	m_commfreq[1] = cursor.getint(15);
	m_typecode = cursor.getint(16);
	m_bdryclass = cursor.getint(17);
	m_width = cursor.getint(18);
	m_altlwr = cursor.getint(19);
	m_altlwrflags = cursor.getint(20);
	m_altupr = cursor.getint(21);
	m_altuprflags = cursor.getint(22);
	m_gndelevmin = cursor.getint(23);
	m_gndelevmax = cursor.getint(24);
	m_coord = Point(cursor.getint(26), cursor.getint(25));
	m_label_placement = (label_placement_t)cursor.getint(27);
	m_polygon.getblob(cursor, 28);
	m_modtime = cursor.getint(29);
	if (false)
		std::cerr << "Airspace::load: srcid " << m_sourceid << " bdryclass " << (int)m_bdryclass << std::endl;
	if (!loadsubtables)
		return;
	load_subtables(db, loadsubtables);
}

void DbBaseElements::Airspace::update_index(sqlite3x::sqlite3_connection & db, bool rtree, bool aux_rtree)
{
	if (m_table != table_main || !m_id)
		return;
	sqlite3x::sqlite3_transaction tran(db);
	{
		sqlite3x::sqlite3_command cmd(db, "UPDATE airspaces SET TILE=?2 WHERE ID=?1;");
		cmd.bind(1, (long long int)m_id);
		cmd.bind(2, (int)TileNumber::to_tilenumber(m_bbox));
		cmd.executenonquery();
	}
	if (rtree) {
		sqlite3x::sqlite3_command cmd(db, "UPDATE airspaces_rtree SET SWLAT=?,NELAT=?,SWLON=?,NELON=? WHERE ID=?;");
		cmd.bind(1, m_bbox.get_south());
		cmd.bind(2, m_bbox.get_north());
		cmd.bind(3, m_bbox.get_west());
		cmd.bind(4, (long long int)m_bbox.get_east_unwrapped());
		cmd.bind(5, (long long int)m_id);
		cmd.executenonquery();
	}
	tran.commit();
}

void DbBaseElements::Airspace::save(sqlite3x::sqlite3_connection & db, bool rtree, bool aux_rtree)
{
	sqlite3x::sqlite3_transaction tran(db);
	if (m_table == table_aux) {
		sqlite3x::sqlite3_command cmd(db, "INSERT OR REPLACE INTO airspaces_deleted (SRCID) VALUES (?);");
		cmd.bind(1, m_sourceid);
		cmd.executenonquery();
		m_table = table_main;
		m_id = 0;
	}
	if (!m_id) {
		m_id = 1;
		{
			sqlite3x::sqlite3_command cmd(db, "SELECT MAX(ID) FROM airspaces;");
			sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
			if (cursor.step())
				m_id = cursor.getint(0) + 1;
		}
		{
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO airspaces (ID,SRCID) VALUES(?,?);");
			cmd.bind(1, (long long int)m_id);
			cmd.bind(2, m_sourceid);
			cmd.executenonquery();
		}
		if (rtree) {
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO airspaces_rtree (ID,SWLAT,NELAT,SWLON,NELON) VALUES(?,?,?,?,?);");
			cmd.bind(1, (long long int)m_id);
			cmd.bind(2, m_bbox.get_south());
			cmd.bind(3, m_bbox.get_north());
			cmd.bind(4, m_bbox.get_west());
			cmd.bind(5, (long long int)m_bbox.get_east_unwrapped());
			cmd.executenonquery();
		}
	}
	// Update Airspace Record
	{
		sqlite3x::sqlite3_command cmd(db, "UPDATE airspaces SET SRCID=?,ICAO=?,NAME=?,IDENT=?,CLASSEXCEPT=?,EFFTIMES=?,NOTE=?,SWLAT=?,SWLON=?,NELAT=?,NELON=?,COMMNAME=?,COMMFREQ0=?,COMMFREQ1=?,TYPECODE=?,CLASS=?,WIDTH=?,ALTLOWER=?,ALTLOWERFLAGS=?,ALTUPPER=?,ALTUPPERFLAGS=?,GNDELEVMIN=?,GNDELEVMAX=?,LAT=?,LON=?,LABELPLACEMENT=?,POLY=?,MODIFIED=?,TILE=? WHERE ID=?;");
		cmd.bind(1, m_sourceid);
		cmd.bind(2, m_icao);
		cmd.bind(3, m_name);
		cmd.bind(4, m_ident);
		cmd.bind(5, m_classexcept);
		cmd.bind(6, m_efftime);
		cmd.bind(7, m_note);
		cmd.bind(8, m_bbox.get_south());
		cmd.bind(9, m_bbox.get_west());
		cmd.bind(10, m_bbox.get_north());
		cmd.bind(11, (long long int)m_bbox.get_east_unwrapped());
		cmd.bind(12, m_commname);
		cmd.bind(13, (long long int)m_commfreq[0]);
		cmd.bind(14, (long long int)m_commfreq[1]);
		cmd.bind(15, m_typecode);
		cmd.bind(16, m_bdryclass);
		cmd.bind(17, m_width);
		cmd.bind(18, m_altlwr);
		cmd.bind(19, m_altlwrflags);
		cmd.bind(20, m_altupr);
		cmd.bind(21, m_altuprflags);
		cmd.bind(22, m_gndelevmin);
		cmd.bind(23, m_gndelevmax);
		cmd.bind(24, m_coord.get_lat());
		cmd.bind(25, m_coord.get_lon());
		cmd.bind(26, m_label_placement);
		m_polygon.bindblob(cmd, 27);
		cmd.bind(28, (long long int)m_modtime);
		cmd.bind(29, (int)TileNumber::to_tilenumber(m_bbox));
		cmd.bind(30, (long long int)m_id);
		cmd.executenonquery();
	}
	if (false)
		std::cerr << "airspace: updated main, subtables " << m_subtables << " segments " <<  get_nrsegments()<< std::endl;
	if (m_subtables & subtables_segments) {
		// Delete existing Segments
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airspacesegments WHERE BDRYID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		// Write Segments
		for (unsigned int i = 0; i < get_nrsegments(); i++) {
			const Segment& seg((*this)[i]);
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO airspacesegments (BDRYID,ID,LAT1,LON1,LAT2,LON2,LAT0,LON0,SHAPE,RADIUS1,RADIUS2) VALUES(?,?,?,?,?,?,?,?,?,?,?);");
			cmd.bind(1, (long long int)m_id);
			cmd.bind(2, (int)i);
			cmd.bind(3, seg.get_coord1().get_lat());
			cmd.bind(4, seg.get_coord1().get_lon());
			cmd.bind(5, seg.get_coord2().get_lat());
			cmd.bind(6, seg.get_coord2().get_lon());
			cmd.bind(7, seg.get_coord0().get_lat());
			cmd.bind(8, seg.get_coord0().get_lon());
			cmd.bind(9, seg.get_shape());
			cmd.bind(10, seg.get_radius1());
			cmd.bind(11, seg.get_radius2());
			cmd.executenonquery();
			if (false)
				std::cerr << "airspace: save segment " << i << " id " << m_id << std::endl;
		}
	}
	if (m_subtables & subtables_components) {
		// Delete existing Components
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airspacecomponents WHERE BDRYID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		// Write Components
		for (unsigned int i = 0; i < get_nrcomponents(); i++) {
			const Component& comp(get_component(i));
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO airspacecomponents (BDRYID,ID,ICAO,TYPECODE,CLASS,OPERATOR) VALUES(?,?,?,?,?,?);");
			cmd.bind(1, (long long int)m_id);
			cmd.bind(2, (int)i);
			cmd.bind(3, comp.get_icao());
			cmd.bind(4, comp.get_typecode());
			cmd.bind(5, comp.get_bdryclass());
			cmd.bind(6, (int)comp.get_operator());
			cmd.executenonquery();
			if (false)
				std::cerr << "airspace: save component " << i << " id " << m_id << std::endl;
		}
	}
	if (rtree) {
		sqlite3x::sqlite3_command cmd(db, "UPDATE airspaces_rtree SET SWLAT=?,NELAT=?,SWLON=?,NELON=? WHERE ID=?;");
		cmd.bind(1, m_bbox.get_south());
		cmd.bind(2, m_bbox.get_north());
		cmd.bind(3, m_bbox.get_west());
		cmd.bind(4, (long long int)m_bbox.get_east_unwrapped());
		cmd.bind(5, (long long int)m_id);
		cmd.executenonquery();
	}
	tran.commit();
}

void DbBaseElements::Airspace::erase(sqlite3x::sqlite3_connection & db, bool rtree, bool aux_rtree)
{
	if (!is_valid())
		return;
	sqlite3x::sqlite3_transaction tran(db);
	if (m_table == table_aux) {
		sqlite3x::sqlite3_command cmd(db, "INSERT OR REPLACE INTO airspaces_deleted (SRCID) VALUES (?);");
		cmd.bind(1, m_sourceid);
		cmd.executenonquery();
		m_table = table_main;
	} else {
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airspaces WHERE ID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airspacesegments WHERE BDRYID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airspacecomponents WHERE BDRYID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		if (rtree) {
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airspaces_rtree WHERE ID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
	}
	tran.commit();
	m_id = 0;
}

#ifdef HAVE_PQXX

void DbBaseElements::Airspace::load(const pqxx::tuple& cursor, pqxx::basic_transaction& w, unsigned int loadsubtables)
{
	m_id = 0;
	m_table = table_main;
	m_typecode = 0;
	m_bdryclass = 0;
	m_subtables = subtables_none;
	if (cursor.empty())
		return;
	m_id = cursor[0].as<int64_t>(0);
	m_table = (table_t)cursor[1].as<int>(0);
	m_sourceid = cursor[2].as<std::string>("");
	m_icao = cursor[3].as<std::string>("");
	m_name = cursor[4].as<std::string>("");
	m_ident = cursor[5].as<std::string>("");
	m_classexcept = cursor[6].as<std::string>("");
	m_efftime = cursor[7].as<std::string>("");
	m_note = cursor[8].as<std::string>("");
	m_bbox = Rect(Point(cursor[10].as<Point::coord_t>(0), cursor[9].as<Point::coord_t>(0x80000000)),
		      Point(cursor[12].as<int64_t>(0), cursor[11].as<Point::coord_t>(0x80000000)));
	m_commname = cursor[13].as<std::string>("");
	m_commfreq[0] = cursor[14].as<uint32_t>(0);
	m_commfreq[1] = cursor[15].as<uint32_t>(0);
	m_typecode = cursor[16].as<unsigned int>(0);
	m_bdryclass = cursor[17].as<int>(0);
	m_width = cursor[18].as<int32_t>(0);
	m_altlwr = cursor[19].as<int32_t>(0);
	m_altlwrflags = cursor[20].as<unsigned int>(0);
	m_altupr = cursor[21].as<int32_t>(0);
	m_altuprflags = cursor[22].as<unsigned int>(0);
	m_gndelevmin = cursor[23].as<gndelev_t>(0);
	m_gndelevmax = cursor[24].as<gndelev_t>(0);
	m_coord = Point(cursor[26].as<Point::coord_t>(0), cursor[25].as<Point::coord_t>(0x80000000));
	m_label_placement = (label_placement_t)cursor[27].as<int>((int)label_off);
	m_polygon.getblob(pqxx::binarystring(cursor[28]));
	m_modtime = cursor[29].as<time_t>(0);
	if (false)
		std::cerr << "Airspace::load: srcid " << m_sourceid << " bdryclass " << (int)m_bdryclass << std::endl;
	if (!loadsubtables)
		return;
	load_subtables(w, loadsubtables);
}

void DbBaseElements::Airspace::load_subtables(pqxx::basic_transaction& w, unsigned int loadsubtables)
{
	loadsubtables &= subtables_all & ~m_subtables;
	if (!loadsubtables)
		return;
	if (loadsubtables & subtables_segments) {
		m_seg.clear();
	        pqxx::result r(w.exec("SELECT LAT1,LON1,LAT2,LON2,LAT0,LON0,SHAPE,RADIUS1,RADIUS2 "
				      "FROM aviationdb.airspacesegments WHERE BDRYID=" + w.quote(m_id) + " ORDER BY ID;"));
		for (pqxx::result::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
			m_seg.push_back(Segment(Point((*ri)[1].as<Point::coord_t>(0), (*ri)[0].as<Point::coord_t>(0x80000000)),
						Point((*ri)[3].as<Point::coord_t>(0), (*ri)[2].as<Point::coord_t>(0x80000000)),
						Point((*ri)[5].as<Point::coord_t>(0), (*ri)[4].as<Point::coord_t>(0x80000000)),
						(char)(*ri)[6].as<int>(0), (*ri)[7].as<int32_t>(0), (*ri)[8].as<int32_t>(0)));
		}
		m_subtables |= subtables_segments;
	}
	if (loadsubtables & subtables_components) {
		m_comp.clear();
	        pqxx::result r(w.exec("SELECT ICAO,TYPECODE,CLASS,OPERATOR "
				      "FROM aviationdb.airspacecomponents WHERE BDRYID=" + w.quote(m_id) + " ORDER BY ID;"));
		for (pqxx::result::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
			m_comp.push_back(Component((*ri)[0].as<std::string>(""), (*ri)[1].as<int>((int)typecode_ead),
						   (*ri)[2].as<int>(0), (Component::operator_t)(*ri)[3].as<int>((int)Component::operator_set)));
		}
		m_subtables |= subtables_components;
	}
}

void DbBaseElements::Airspace::save(pqxx::connection& conn, bool rtree)
{
	pqxx::work w(conn);
	if (!m_id) {
		pqxx::result r(w.exec("INSERT INTO aviationdb.airspaces (ID,SRCID) SELECT COALESCE(MAX(ID),0)+1," + w.quote((std::string)m_sourceid) + " FROM aviationdb.airspaces RETURNING ID;"));
		m_id = r.front()[0].as<int64_t>();
		if (rtree)
			w.exec("INSERT INTO aviationdb.airspaces_rtree (ID,SWLAT,NELAT,SWLON,NELON) VALUES(" + w.quote(m_id) + "," + w.quote(m_bbox.get_south()) +
			       "," + w.quote(m_bbox.get_north()) + "," + w.quote(m_bbox.get_west()) + "," + w.quote((long long int)m_bbox.get_east_unwrapped()) + ");");
	}
	// Update Airspace Record
	{
		pqxx::binarystring poly(m_polygon.bindblob());
		w.exec("UPDATE aviationdb.airspaces SET SRCID=" + w.quote((std::string)m_sourceid) + ",ICAO=" + w.quote((std::string)m_icao) +
		       ",NAME=" + w.quote((std::string)m_name) + ",IDENT=" + w.quote((std::string)m_ident) + ",CLASSEXCEPT=" + w.quote((std::string)m_classexcept) +
		       ",EFFTIMES=" + w.quote((std::string)m_efftime) + ",NOTE=" + w.quote((std::string)m_note) +
		       ",SWLAT=" + w.quote(m_bbox.get_south()) + ",SWLON=" + w.quote(m_bbox.get_west()) +
		       ",NELAT=" + w.quote(m_bbox.get_north()) + ",NELON=" + w.quote(m_bbox.get_east_unwrapped()) +
		       ",COMMNAME=" + w.quote((std::string)m_commname) + ",COMMFREQ0=" + w.quote(m_commfreq[0]) + ",COMMFREQ1=" + w.quote(m_commfreq[1]) +
		       ",TYPECODE=" + w.quote((int)m_typecode) + ",CLASS=" + w.quote((int)m_bdryclass) + ",WIDTH=" + w.quote(m_width) +
		       ",ALTLOWER=" + w.quote(m_altlwr) + ",ALTLOWERFLAGS=" + w.quote((int)m_altlwrflags) +
		       ",ALTUPPER=" + w.quote(m_altupr) + ",ALTUPPERFLAGS=" + w.quote((int)m_altuprflags) +
		       ",GNDELEVMIN=" + w.quote(m_gndelevmin) + ",GNDELEVMAX=" + w.quote(m_gndelevmax) +
		       ",LAT=" + w.quote(m_coord.get_lat()) + ",LON=" + w.quote(m_coord.get_lon()) +
		       ",LABELPLACEMENT=" + w.quote((int)m_label_placement) + ",POLY=" + w.quote(poly) +
		       ",GPOLY=" + PGDbBaseCommon::get_str(m_polygon) + ",MODIFIED=" + w.quote(m_modtime) +
		       ",TILE=" + w.quote(TileNumber::to_tilenumber(m_bbox)) + " WHERE ID=" + w.quote(m_id) + ";");
	}
	if (false)
		std::cerr << "airspace: updated main, subtables " << m_subtables << " segments " << get_nrsegments() << std::endl;
	if (m_subtables & subtables_segments) {
		// Delete existing Segments
	        w.exec("DELETE FROM aviationdb.airspacesegments WHERE BDRYID=" + w.quote(m_id) + ";");
		// Write Segments
		for (unsigned int i = 0; i < get_nrsegments(); i++) {
			const Segment& seg((*this)[i]);
		        w.exec("INSERT INTO aviationdb.airspacesegments (BDRYID,ID,LAT1,LON1,LAT2,LON2,LAT0,LON0,SHAPE,RADIUS1,RADIUS2) VALUES(" +
			       w.quote(m_id) + "," + w.quote(i) + "," +
			       w.quote(seg.get_coord1().get_lat()) + "," + w.quote(seg.get_coord1().get_lon()) + "," +
			       w.quote(seg.get_coord2().get_lat()) + "," + w.quote(seg.get_coord2().get_lon()) + "," +
			       w.quote(seg.get_coord0().get_lat()) + "," + w.quote(seg.get_coord0().get_lon()) + "," +
			       w.quote((int)seg.get_shape()) + "," + w.quote(seg.get_radius1()) + "," +
			       w.quote(seg.get_radius2()) + ");");
			if (false)
				std::cerr << "airspace: save segment " << i << " id " << m_id << std::endl;
		}
	}
	if (m_subtables & subtables_components) {
		// Delete existing Components
		w.exec("DELETE FROM aviationdb.airspacecomponents WHERE BDRYID=" + w.quote(m_id) + ";");
		// Write Components
		for (unsigned int i = 0; i < get_nrcomponents(); i++) {
			const Component& comp(get_component(i));
			w.exec("INSERT INTO aviationdb.airspacecomponents (BDRYID,ID,ICAO,TYPECODE,CLASS,OPERATOR) VALUES(" +
			       w.quote(m_id) + "," + w.quote(i) + "," + w.quote((std::string)comp.get_icao()) + "," +
			       w.quote((int)comp.get_typecode()) + "," + w.quote((int)comp.get_bdryclass()) + "," +
			       w.quote((int)comp.get_operator()) + ");");
			if (false)
				std::cerr << "airspace: save component " << i << " id " << m_id << std::endl;
		}
	}
	if (rtree)
		w.exec("UPDATE aviationdb.airspaces_rtree SET SWLAT=" + w.quote(m_bbox.get_south()) + ",NELAT=" + w.quote(m_bbox.get_north()) +
		       ",SWLON=" + w.quote(m_bbox.get_west()) + ",NELON=" + w.quote((long long int)m_bbox.get_east_unwrapped()) + " WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
}

void DbBaseElements::Airspace::erase(pqxx::connection& conn, bool rtree)
{
	if (!is_valid())
		return;
	pqxx::work w(conn);
	w.exec("DELETE FROM aviationdb.airspaces WHERE ID=" + w.quote(m_id) + ";");
	w.exec("DELETE FROM aviationdb.airspacesegments WHERE BDRYID=" + w.quote(m_id) + ";");
	w.exec("DELETE FROM aviationdb.airspacecomponents WHERE BDRYID=" + w.quote(m_id) + ";");
	if (rtree)
		w.exec("DELETE FROM aviationdb.airspaces_rtree WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
	m_id = 0;
}

void DbBaseElements::Airspace::update_index(pqxx::connection& conn, bool rtree)
{
	if (!m_id)
		return;
	pqxx::work w(conn);
	w.exec("UPDATE aviationdb.airspaces SET TILE=" + w.quote(TileNumber::to_tilenumber(m_bbox)) + " WHERE ID=" + w.quote(m_id) + ";");
	if (rtree)
		w.exec("UPDATE aviationdb.airspaces_rtree SET SWLAT=" + w.quote(m_bbox.get_south()) + ",NELAT=" + w.quote(m_bbox.get_north()) +
		       ",SWLON=" + w.quote(m_bbox.get_west()) + ",NELON=" + w.quote((long long int)m_bbox.get_east_unwrapped()) + " WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
}

#endif

int32_t DbBaseElements::Airspace::get_altlwr_corr(void) const
{
	uint8_t fl(get_altlwrflags());
	int32_t a(get_altlwr());
	int32_t g(get_gndelevmin());

	if (fl & (altflag_sfc | altflag_gnd))
		return g;
	if (fl & altflag_agl)
		return a + g;
	if (fl & altflag_unlim)
		return std::numeric_limits<int32_t>::min();
	return a;
}

int32_t DbBaseElements::Airspace::get_altupr_corr(void) const
{
	uint8_t fl(get_altuprflags());
	int32_t a(get_altupr());
	int32_t g(get_gndelevmax());

	if (fl & (altflag_sfc | altflag_gnd))
		return g;
	if (fl & altflag_agl)
		return a + g;
	if (fl & altflag_unlim)
		return std::numeric_limits<int32_t>::max();
	return a;
}

Glib::ustring DbBaseElements::Airspace::get_altlwr_string(void) const
{
	uint8_t fl(get_altlwrflags());
	int32_t a(get_altlwr());
	if (fl & altflag_sfc)
		return "SFC";
	if (fl & altflag_gnd)
		return "GND";
	if (fl & altflag_unlim)
		return "UNLIM";
	if (fl & altflag_notam)
		return "NOTAM";
	if (fl & altflag_unkn)
		return "U";
	if (fl & altflag_fl) {
		char buf[16];
		snprintf(buf, sizeof(buf), "FL%d", a / 100);
		return buf;
	}
	char buf[16];
	snprintf(buf, sizeof(buf), "%d'", a);
	if (fl & altflag_agl)
		return Glib::ustring(buf) + "AGL";
	return buf;
}

Glib::ustring DbBaseElements::Airspace::get_altupr_string(void) const
{
	uint8_t fl(get_altuprflags());
	int32_t a(get_altupr());
	if (fl & altflag_sfc)
		return "SFC";
	if (fl & altflag_gnd)
		return "GND";
	if (fl & altflag_unlim)
		return "UNLIM";
	if (fl & altflag_notam)
		return "NOTAM";
	if (fl & altflag_unkn)
		return "U";
	if (fl & altflag_fl) {
		char buf[16];
		snprintf(buf, sizeof(buf), "FL%d", a / 100);
		return buf;
	}
	char buf[16];
	snprintf(buf, sizeof(buf), "%d'", a);
	if (fl & altflag_agl)
		return Glib::ustring(buf) + "AGL";
	return buf;
}

Gdk::Color DbBaseElements::Airspace::get_color( char bclass, uint8_t tc )
{
	/*
	 * airspace colors
	 */
	Gdk::Color col;
	col.set_rgb(255 << 8, 128 << 8, 0 << 8);
	if (tc == 255)  /* special use airspace */
		return col;
	switch (bclass) {
	default:
		col.set_rgb(125 << 8, 42 << 8, 107 << 8);
		break;

	case 'B':
		col.set_rgb(167 << 8, 101 << 8, 160 << 8);
		break;

	case 'C':
		col.set_rgb(131 << 8, 104 << 8, 174 << 8);
		break;

	case 'D':
		col.set_rgb(98 << 8, 111 << 8, 212 << 8);
		break;

	case 'E':
	case 'F':
		col.set_rgb(107 << 8, 142 << 8, 109 << 8);
		break;

	case 'G':
		col.set_rgb(175 << 8, 197 << 8, 177 << 8);
		break;
	}
	return col;
}

bool DbBaseElements::Airspace::triangle_center( Point & ret, Point p0, Point p1, Point p2 )
{
	ret = p0;
	p1 -= p0;
	p2 -= p0;
	Point c0 = p1 + p2;
	Point c1 = -p1 + (p2 - p1);
	float xn0 = c0.get_lat();
	float yn0 = -c0.get_lon();
	float xn1 = c1.get_lat();
	float yn1 = -c1.get_lon();
	int32_t e1 = (int32_t)(p1.get_lon() * xn1 + p1.get_lat() * yn1);
	float t = xn0 * yn1 - yn0 * xn1;
	if (fabs(t) < 0.0000001 || !c0.get_lat())
		return false;
	float y = xn0 * e1 / t;
	float x = - y * yn0 / xn0;
	ret = p0 + Point((int32_t)x, (int32_t)y);
	return true;
}

float DbBaseElements::Airspace::label_metric( const Point & p0, const Point & p1 )
{
	float l = ((p0.get_lat() >> 1) + (p1.get_lat() >> 1)) * Point::to_rad;
	float l2 = l * l;
	float l4 = l2 * l2;
	float cosapprox = 1 - (1.0 / 2.0) * l2 + (1.0 / 24.0) * l4;
	Point pt(p0 - p1);
	float d = (pt.get_lat() * (float)pt.get_lat() + pt.get_lon() * (float)pt.get_lon() * cosapprox);
	d *= (Point::to_rad * 60.0 * Point::to_rad * 60.0);
	if (d < 0.001)
		d = 1000;
	else
		d = 1.0 / d;
	return d;
}

void DbBaseElements::Airspace::compute_initial_label_placement(void)
{
	if (get_nrsegments() == 1 && ((*this)[0].get_shape() == 'C' || (*this)[0].get_shape() == 'A')) {
		m_coord = (*this)[0].get_coord0();
		return;
	}
	if (m_polygon.empty()) {
		m_coord = Point();
		return;
	}
	Point pt(m_bbox.boxcenter());
	m_coord = pt;
	if (m_polygon.windingnumber(m_coord))
		return;
	float m = 2000;
	for (MultiPolygonHole::const_iterator phi(m_polygon.begin()), phe(m_polygon.end()); phi != phe; ++phi) {
		const PolygonHole& ph(*phi);
		const PolygonSimple& p(ph.get_exterior());
		for (unsigned int i = 0; i < p.size(); i++) {
			for (unsigned int j = i + 2; j < p.size(); j++) {
				if (!i && (j + 1) >= p.size())
					continue;
				Point pt1(p[i].halfway(p[j]));
				float m1 = label_metric(p[i], p[j]);
				if (m1 > m)
					continue;
				if (!ph.windingnumber(pt1))
					continue;
				pt = pt1;
				m = m1;
			}
		}
	}
	if (m < 1000) {
		m_coord = pt;
		return;
	}
	for (MultiPolygonHole::const_iterator phi(m_polygon.begin()), phe(m_polygon.end()); phi != phe; ++phi) {
		const PolygonHole& ph(*phi);
		const PolygonSimple& p(ph.get_exterior());
		for (unsigned int i = 2; i < p.size(); i++) {
			Point pt1;
			if (!triangle_center(pt1, p[i-2], p[i-1], p[i]))
				continue;
			if (!ph.windingnumber(pt1))
				continue;
			m_coord = pt1;
			return;
		}
	}
	for (MultiPolygonHole::const_iterator phi(m_polygon.begin()), phe(m_polygon.end()); phi != phe; ++phi) {
		const PolygonHole& ph(*phi);
		const PolygonSimple& p(ph.get_exterior());
		if (!p.size())
			continue;
		m_coord = p[0];
		return;
	}
	m_coord = Point();
}

void DbBaseElements::Airspace::recompute_lineapprox_expand(void)
{
	m_polygon.clear();
	if (get_nrsegments() < 2)
		return;
	double w(get_width_nmi() * 0.5);
	for (unsigned int i = 1; i < get_nrsegments(); ++i) {
		Segment& seg1(operator[](i - 1));
		Segment& seg2(operator[](i));
		if (seg1.get_shape() != 'B' && seg1.get_shape() != 'H' && 
		    seg1.get_shape() != '-') {
			std::cerr << "DbBaseElements::Airspace::recompute_lineapprox: Warning: Airspace "
				  << get_icao() << '/' << get_class_string() << ": cannot expand due invalid shape: "
				  << seg1.get_shape() << std::endl;
			continue;
		}
		if (seg2.get_shape() != 'B' && seg2.get_shape() != 'H' && 
		    seg2.get_shape() != '-' && seg2.get_shape() != 'E') {
			std::cerr << "DbBaseElements::Airspace::recompute_lineapprox: Warning: Airspace "
				  << get_icao() << '/' << get_class_string() << ": cannot expand due invalid shape: "
				  << seg2.get_shape() << std::endl;
			continue;
		}
		double crs(seg1.get_coord1().spheric_true_course_dbl(seg2.get_coord1()));
		PolygonSimple r;
		for (unsigned int k = 96; k != 32; k = (k - 1) & 127) {
			Point pt(seg1.get_coord1().spheric_course_distance_nmi(crs + k * (360.0 / 128.0), w));
			r.push_back(pt);
		}
		for (unsigned int k = 32; k != 96; k = (k - 1) & 127) {
			Point pt(seg2.get_coord1().spheric_course_distance_nmi(crs + k * (360.0 / 128.0), w));
			r.push_back(pt);
		}
		MultiPolygonHole mp;
		mp.push_back(r);
		if (m_polygon.empty()) {
			m_polygon.swap(mp);
			continue;
		}
		m_polygon.geos_union(mp);
	}
}

void DbBaseElements::Airspace::recompute_lineapprox_circle(void)
{
	uint32_t anginc((uint32_t)(boundarycircleapprox * Point::from_deg_dbl));
	uint32_t ang1(0);
	uint32_t ang2(-anginc);
	const Segment& seg((*this)[0]);
	double r1(Point::to_rad_dbl * seg.get_radius1());
	PolygonSimple p;
	for (; ang1 < ang2; ang1 += anginc) {
		CartesianVector3Dd v(1.0, r1 * cos(ang1 * Point::to_rad_dbl), r1 * sin(ang1 * Point::to_rad_dbl));
		v.rotate_xz(seg.get_coord0().get_lat() * Point::to_rad);
		v.rotate_xy(seg.get_coord0().get_lon() * Point::to_rad);
		p.push_back(((PolarVector3Dd)v).getcoord());
	}
	m_polygon.push_back(p);
}

void DbBaseElements::Airspace::recompute_lineapprox_seg(void)
{
	m_polygon.clear();
	if (!get_nrsegments())
		return;
	if (get_nrsegments() == 1) {
		char shape((*this)[0].get_shape());
		if (shape == 'C' || shape == 'A') {
			if (shape == 'C' || (*this)[0].get_radius1() > 100)
				recompute_lineapprox_circle();
			return;
		}
	}
	PolygonSimple p;
	for (unsigned int i = 0; i < get_nrsegments(); i++) {
		const Segment& seg((*this)[i]);
		unsigned int j = i - 1;
		if (!i)
			j = get_nrsegments() - 1;
		const Segment& segp((*this)[j]);
		if (seg.get_shape() == 'C') {
			std::cerr << "DbBaseElements::Airspace::recompute_lineapprox: " << get_icao_name() << " Circle within path" << std::endl;
			continue;
		}
		if (seg.get_shape() == 'A') {
			std::cerr << "DbBaseElements::Airspace::recompute_lineapprox: " << get_icao_name() << " Point within path" << std::endl;
			continue;
		}
		if (abs(seg.get_coord1().get_lat() - segp.get_coord2().get_lat()) > 100 ||
		    abs(seg.get_coord1().get_lon() - segp.get_coord2().get_lon()) > 100) {
			std::cerr << "DbBaseElements::Airspace::recompute_lineapprox: " << get_icao_name() << " Segments not adjacent" << std::endl;
			continue;
		}
		p.push_back(seg.get_coord1());
		if (seg.get_shape() != 'L' && seg.get_shape() != 'R')
			continue;
		/* convert circle segment to polygon edges */
		CartesianVector3Dd v1 = PolarVector3Dd(seg.get_coord1().get_lat() * Point::to_rad_dbl, seg.get_coord1().get_lon() * Point::to_rad_dbl, 1.0);
		CartesianVector3Dd v2 = PolarVector3Dd(seg.get_coord2().get_lat() * Point::to_rad_dbl, seg.get_coord2().get_lon() * Point::to_rad_dbl, 1.0);
		/* rotate back to near 1/0/0 */
		v1.rotate_xy(seg.get_coord0().get_lon() * (-Point::to_rad_dbl));
		v1.rotate_xz(seg.get_coord0().get_lat() * (-Point::to_rad_dbl));
		v2.rotate_xy(seg.get_coord0().get_lon() * (-Point::to_rad_dbl));
		v2.rotate_xz(seg.get_coord0().get_lat() * (-Point::to_rad_dbl));
		uint32_t ang1 = (uint32_t)(atan2(v1.getz(), v1.gety()) * Point::from_rad_dbl);
		double r1 = sqrt(v1.gety() * v1.gety() + v1.getz() * v1.getz());
		uint32_t ang2 = (uint32_t)(atan2(v2.getz(), v2.gety()) * Point::from_rad_dbl);
		double r2 = sqrt(v2.gety() * v2.gety() + v2.getz() * v2.getz());
		uint32_t anginc = (uint32_t)(boundarycircleapprox * Point::from_deg_dbl);
		if (seg.get_shape() == 'R')
			anginc = -anginc;
		for (; abs((int32_t)(ang1 - ang2)) >= abs((int32_t)(anginc));) {
			ang1 += anginc;
			CartesianVector3Dd v(1.0, r1 * cos(ang1 * Point::to_rad_dbl), r1 * sin(ang1 * Point::to_rad));
			v.rotate_xz(seg.get_coord0().get_lat() * Point::to_rad_dbl);
			v.rotate_xy(seg.get_coord0().get_lon() * Point::to_rad_dbl);
			p.push_back(((PolarVector3Dd)v).getcoord());
		}
	}
	if (p.size() < 2) {
		std::cerr << "DbBaseElements::Airspace::recompute_lineapprox: " << get_icao_name() << " less than 2 poly points" << std::endl;
		return;
	}
	if (!p.geos_is_valid())
		p.geos_make_valid();
	m_polygon = p.geos_simplify();
}

void DbBaseElements::Airspace::recompute_lineapprox_comp(AirspacesDbQueryInterface& db)
{
	int32_t altlwr(0);
	int32_t altupr(99999);
	for (comp_t::const_iterator ci(m_comp.begin()), ce(m_comp.end()); ci != ce; ++ci) {
		const Component& comp(*ci);
		AirspacesDb::elementvector_t ev(db.find_by_icao(comp.get_icao(), 0, AirspacesDb::comp_exact, 0, subtables_none));
		MultiPolygonHole mp;
		unsigned int polycnt(0);
		int32_t mpaltlwr(0);
		int32_t mpaltupr(99999);
		for (AirspacesDb::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ++ei) {
			if (ei->get_bdryclass() != comp.get_bdryclass() || ei->get_typecode() != comp.get_typecode() ||
			    ei->get_icao() != comp.get_icao())
				continue;
			mp = ei->get_polygon();
			mpaltlwr = ei->get_altlwr();
			mpaltupr = ei->get_altupr();
			++polycnt;
		}
		if (!polycnt) {
			std::cerr << "DbBaseElements::Airspace::recompute_lineapprox: Warning: Airspace "
				  << comp.get_icao() << '/' << comp.get_class_string() << " not found for airspace "
				  << get_icao() << '/' << get_class_string() << std::endl;
			continue;
		}
		if (polycnt > 1)
			std::cerr << "DbBaseElements::Airspace::recompute_lineapprox: Warning: multiple Airspaces "
				  << comp.get_icao() << '/' << comp.get_class_string() << " found for airspace "
				  << get_icao() << '/' << get_class_string() << std::endl;
		if (false && !mp.geos_is_valid())
			mp.geos_make_valid();
		switch (comp.get_operator()) {
		default:
		case Component::operator_set:
			m_polygon.swap(mp);
			altlwr = mpaltlwr;
			altupr = mpaltupr;
			break;

		case Component::operator_union:
			m_polygon.geos_union(mp);
			altlwr = std::min(altlwr, mpaltlwr);
			altupr = std::max(altupr, mpaltupr);
			break;

		case Component::operator_subtract:
			m_polygon.geos_subtract(mp);
			break;

		case Component::operator_intersect:
			m_polygon.geos_intersect(mp);
			altlwr = std::max(altlwr, mpaltlwr);
			altupr = std::min(altupr, mpaltupr);
			break;
		}
		if (0 && !m_polygon.geos_is_valid())
			m_polygon.geos_make_valid();
		unsigned int zappedpolys(m_polygon.zap_polys(0, 1e-8));
		unsigned int zappedholes(m_polygon.zap_holes(0, 1e-8));
		if (true)
			m_polygon.print_nrvertices(std::cerr << "Polygons after compositing " << comp.get_icao() << '/' << comp.get_class_string() << ": ", true)
				<< " (removed " << zappedpolys << " polygons and " << zappedholes << " holes)" <<std::endl;
	}
	if (m_seg.empty() && !m_comp.empty()) {
		set_altlwr(altlwr);
		set_altupr(altupr);
	}
}

void DbBaseElements::Airspace::recompute_lineapprox(AirspacesDbQueryInterface& db)
{
	if (get_width())
		recompute_lineapprox_expand();
	else
		recompute_lineapprox_seg();
	if (true)
		m_polygon.print_nrvertices(std::cerr << "Polygons after segmentation (" << get_icao() << '/' << get_class_string() << "): ", true) << std::endl;
	m_polygon.remove_redundant_polypoints();
	for (MultiPolygonHole::iterator phi(m_polygon.begin()); phi != m_polygon.end(); ) {
		PolygonHole& ph(*phi);
		if (ph.get_exterior().size() < 3) {
			std::cerr << "DbBaseElements::Airspace::recompute_lineapprox: Warning: multiple Airspace "
				  << get_icao() << '/' << get_class_string() << " has degenerate polygon (exterior)" << std::endl;
			phi = m_polygon.erase(phi);
			continue;
		}
		for (unsigned int i = 0; i < ph.get_nrinterior(); ) {
			PolygonSimple& ps(ph[i]);
			if (ps.size() < 3) {
				std::cerr << "DbBaseElements::Airspace::recompute_lineapprox: Warning: multiple Airspace "
					  << get_icao() << '/' << get_class_string() << " has degenerate polygon (interior)" << std::endl;
				ph.erase_interior(i);
				continue;
			}
			++i;
		}
		++phi;
	}
	if (true)
		m_polygon.print_nrvertices(std::cerr << "Polygons after redundancy removal: ", true) << std::endl;
	recompute_lineapprox_comp(db);
	if (true)
		m_polygon.print_nrvertices(std::cerr << "Polygons after compositing: ", true) << std::endl;
	m_polygon.normalize();
	recompute_bbox();
}

void DbBaseElements::Airspace::recompute_bbox(void)
{
	if (m_polygon.empty()) {
		if (get_nrsegments() == 1 && (*this)[0].get_shape() == 'A') {
			m_bbox = Rect((*this)[0].get_coord2(), (*this)[0].get_coord2());
			return;
		}
		m_bbox = Rect();
		return;
	}
	m_bbox = m_polygon.get_bbox();
}

void DbBaseElements::Airspace::set_polygon(const MultiPolygonHole& poly)
{
	m_subtables &= ~(subtables_segments | subtables_components);
	m_polygon = poly;
	m_seg.clear();
	m_comp.clear();
	recompute_bbox();
}

void DbBaseElements::Airspace::compute_segments_from_poly(void)
{
	if (m_polygon.size() != 1)
		throw std::runtime_error("Airspace::compute_segments_from_poly: multiple or no polygons");
	if (m_polygon[0].get_nrinterior())
		throw std::runtime_error("Airspace::compute_segments_from_poly: polygon has holes");
	m_subtables |= subtables_segments;
	m_seg.clear();
	const PolygonSimple& p(m_polygon[0].get_exterior());
	for (unsigned int i = 0, j = p.size() - 1; i < p.size(); j = i, i++)
		m_seg.push_back(Segment(p[j], p[i], Point(), '-', 0, 0));
}

template<> void DbBase<DbBaseElements::Airspace>::drop_tables(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS airspaces;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS airspacesegments;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS airspacecomponents;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS airspacepolypoints;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS airspaces_deleted;"); cmd.executenonquery(); }
}

template<> void DbBase<DbBaseElements::Airspace>::create_tables(void)
{
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS airspaces (ID INTEGER PRIMARY KEY NOT NULL, "
					      "SRCID TEXT UNIQUE NOT NULL,"
					      "ICAO TEXT,"
					      "NAME TEXT,"
					      "IDENT TEXT,"
					      "CLASSEXCEPT TEXT,"
					      "EFFTIMES TEXT,"
					      "NOTE TEXT,"
					      "SWLAT INTEGER,"
					      "SWLON INTEGER,"
					      "NELAT INTEGER,"
					      "NELON INTEGER,"
					      "COMMNAME TEXT,"
					      "COMMFREQ0 INTEGER,"
					      "COMMFREQ1 INTEGER,"
					      "TYPECODE INTEGER,"
					      "CLASS CHAR,"
					      "WIDTH INTEGER,"
					      "ALTLOWER INTEGER,"
					      "ALTLOWERFLAGS INTEGER,"
					      "ALTUPPER INTEGER,"
					      "ALTUPPERFLAGS INTEGER,"
					      "GNDELEVMIN INTEGER,"
					      "GNDELEVMAX INTEGER,"
					      "LAT INTEGER,"
					      "LON INTEGER,"
					      "LABELPLACEMENT CHAR,"
					      "POLY BLOB,"
					      "MODIFIED INTEGER,"
					      "TILE INTEGER);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS airspacesegments (ID INTEGER NOT NULL, "
					      "BDRYID INTEGER NOT NULL,"
					      "LAT1 INTEGER,"
					      "LON1 INTEGER,"
					      "LAT2 INTEGER,"
					      "LON2 INTEGER,"
					      "LAT0 INTEGER,"
					      "LON0 INTEGER,"
					      "SHAPE CHAR,"
					      "RADIUS1 INTEGER,"
					      "RADIUS2 INTEGER);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS airspacecomponents (ID INTEGER NOT NULL, "
					      "BDRYID INTEGER NOT NULL,"
					      "ICAO TEXT,"
					      "TYPECODE INTEGER,"
					      "CLASS CHAR,"
					      "OPERATOR INTEGER);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS airspaces_deleted(SRCID TEXT PRIMARY KEY NOT NULL);");
		cmd.executenonquery();
	}
}

template<> void DbBase<DbBaseElements::Airspace>::drop_indices(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airspaces_srcid;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airspaces_icao;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airspaces_name;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airspaces_ident;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airspaces_latlon;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airspaces_bbox;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airspaces_modified;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airspacesegments_id;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airspacecomponents_id;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airspaces_tile;"); cmd.executenonquery(); }
	// old indices
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airspaces_lat;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airspaces_lon;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airspaces_swlat;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airspaces_swlon;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airspaces_nelat;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airspaces_nelon;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airspacepolypoints_id;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airspacepolypoints_latlon;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airspacepolypoints_lat;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airspacepolypoints_lon;"); cmd.executenonquery(); }
}

template<> void DbBase<DbBaseElements::Airspace>::create_indices(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airspaces_srcid ON airspaces(SRCID);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airspaces_icao ON airspaces(ICAO COLLATE NOCASE);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airspaces_name ON airspaces(NAME COLLATE NOCASE);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airspaces_ident ON airspaces(IDENT COLLATE NOCASE);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airspaces_latlon ON airspaces(LAT,LON);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airspaces_bbox ON airspaces(SWLAT,NELAT,SWLON,NELON);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airspaces_modified ON airspaces(MODIFIED);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airspaces_tile ON airspaces(TILE);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airspacesegments_id ON airspacesegments(BDRYID,ID);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airspacecomponents_id ON airspacecomponents(BDRYID,ID);"); cmd.executenonquery(); }
	// old indices
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airspaces_swlat ON airspaces(SWLAT);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airspaces_swlon ON airspaces(SWLON);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airspaces_nelat ON airspaces(NELAT);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airspaces_nelon ON airspaces(NELON);"); cmd.executenonquery(); }
}

template<> const char *DbBase<DbBaseElements::Airspace>::main_table_name = "airspaces";
template<> const char *DbBase<DbBaseElements::Airspace>::database_file_name = "airspaces.db";
template<> const char *DbBase<DbBaseElements::Airspace>::order_field = "SRCID";
template<> const char *DbBase<DbBaseElements::Airspace>::delete_field = "SRCID";
template<> const bool DbBase<DbBaseElements::Airspace>::area_data = true;

#ifdef HAVE_PQXX

template<> void PGDbBase<DbBaseElements::Airspace>::drop_tables(void)
{
	pqxx::work w(m_conn);
	w.exec("DROP TABLE IF EXISTS aviationdb.airspaces;");
	w.exec("DROP TABLE IF EXISTS aviationdb.airspacesegments;");
	w.exec("DROP TABLE IF EXISTS aviationdb.airspacecomponents;");
	w.exec("DROP TABLE IF EXISTS aviationdb.airspacepolypoints;");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Airspace>::create_tables(void)
{
	create_common_tables();
	pqxx::work w(m_conn);
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.airspaces ("
	       "IDENT TEXT,"
	       "CLASSEXCEPT TEXT,"
	       "EFFTIMES TEXT,"
	       "NOTE TEXT,"
	       "SWLAT INTEGER,"
	       "SWLON INTEGER,"
	       "NELAT INTEGER,"
	       "NELON BIGINT,"
	       "COMMNAME TEXT,"
	       "COMMFREQ0 BIGINT,"
	       "COMMFREQ1 BIGINT,"
	       "TYPECODE SMALLINT,"
	       "CLASS SMALLINT,"
	       "WIDTH INTEGER,"
	       "ALTLOWER INTEGER,"
	       "ALTLOWERFLAGS SMALLINT,"
	       "ALTUPPER INTEGER,"
	       "ALTUPPERFLAGS SMALLINT,"
	       "GNDELEVMIN INTEGER,"
	       "GNDELEVMAX INTEGER,"
	       "POLY BYTEA,"
	       "GPOLY ext.GEOGRAPHY(MULTIPOLYGON,4326),"
	       "TILE INTEGER,"
	       "PRIMARY KEY (ID)"
	       ") INHERITS (aviationdb.labelsourcecoordicaonamebase);");
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.airspacesegments (ID INTEGER NOT NULL, "
	       "BDRYID BIGINT NOT NULL REFERENCES aviationdb.airspaces (ID) ON DELETE CASCADE,"
	       "LAT1 INTEGER,"
	       "LON1 INTEGER,"
	       "LAT2 INTEGER,"
	       "LON2 INTEGER,"
	       "LAT0 INTEGER,"
	       "LON0 INTEGER,"
	       "SHAPE SMALLINT,"
	       "RADIUS1 INTEGER,"
	       "RADIUS2 INTEGER,"
	       "PRIMARY KEY (BDRYID,ID));");
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.airspacecomponents (ID INTEGER NOT NULL, "
	       "BDRYID BIGINT NOT NULL REFERENCES aviationdb.airspaces (ID) ON DELETE CASCADE,"
	       "ICAO TEXT,"
	       "TYPECODE INTEGER,"
	       "CLASS SMALLINT,"
	       "OPERATOR SMALLINT,"
	       "PRIMARY KEY (BDRYID,ID));");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Airspace>::drop_indices(void)
{
	pqxx::work w(m_conn);
	w.exec("DROP INDEX IF EXISTS aviationdb.airspaces_srcid;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airspaces_icao;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airspaces_name;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airspaces_ident;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airspaces_latlon;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airspaces_bbox;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airspaces_modified;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airspacesegments_id;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airspacecomponents_id;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airspaces_tile;");
	// old indices
	w.exec("DROP INDEX IF EXISTS aviationdb.airspaces_lat;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airspaces_lon;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airspaces_swlat;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airspaces_swlon;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airspaces_nelat;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airspaces_nelon;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airspacepolypoints_id;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airspacepolypoints_latlon;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airspacepolypoints_lat;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airspacepolypoints_lon;");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Airspace>::create_indices(void)
{
	pqxx::work w(m_conn);
	w.exec("CREATE INDEX IF NOT EXISTS airspaces_srcid ON aviationdb.airspaces(SRCID);");
	w.exec("CREATE INDEX IF NOT EXISTS airspaces_icao ON aviationdb.airspaces((ICAO::ext.CITEXT) COLLATE \"C\");");
	w.exec("CREATE INDEX IF NOT EXISTS airspaces_name ON aviationdb.airspaces((NAME::ext.CITEXT) COLLATE \"C\");");
	w.exec("CREATE INDEX IF NOT EXISTS airspaces_ident ON aviationdb.airspaces((IDENT::ext.CITEXT) COLLATE \"C\");");
	w.exec("CREATE INDEX IF NOT EXISTS airspaces_latlon ON aviationdb.airspaces(LAT,LON);");
	w.exec("CREATE INDEX IF NOT EXISTS airspaces_bbox ON aviationdb.airspaces(SWLAT,NELAT,SWLON,NELON);");
	w.exec("CREATE INDEX IF NOT EXISTS airspaces_modified ON aviationdb.airspaces(MODIFIED);");
	w.exec("CREATE INDEX IF NOT EXISTS airspaces_tile ON aviationdb.airspaces(TILE);");
	w.exec("CREATE INDEX IF NOT EXISTS airspacesegments_id ON aviationdb.airspacesegments(BDRYID,ID);");
	w.exec("CREATE INDEX IF NOT EXISTS airspacecomponents_id ON aviationdb.airspacecomponents(BDRYID,ID);");
	// old indices
	w.exec("CREATE INDEX IF NOT EXISTS airspaces_swlat ON aviationdb.airspaces(SWLAT);");
	w.exec("CREATE INDEX IF NOT EXISTS airspaces_swlon ON aviationdb.airspaces(SWLON);");
	w.exec("CREATE INDEX IF NOT EXISTS airspaces_nelat ON aviationdb.airspaces(NELAT);");
	w.exec("CREATE INDEX IF NOT EXISTS airspaces_nelon ON aviationdb.airspaces(NELON);");
	w.commit();
}

template<> const char *PGDbBase<DbBaseElements::Airspace>::main_table_name = "aviationdb.airspaces";
template<> const char *PGDbBase<DbBaseElements::Airspace>::order_field = "SRCID";
template<> const char *PGDbBase<DbBaseElements::Airspace>::delete_field = "SRCID";
template<> const bool PGDbBase<DbBaseElements::Airspace>::area_data = true;

#endif

const Glib::ustring& DbBaseElements::Airspace::get_class_string(char bc, uint8_t tc)
{
	static const Glib::ustring empty;
	switch (tc) {
	default:
		switch (bc) {
		case 'A':
		{
			static const Glib::ustring r("A");
			return r;
		}

		case 'B':
		{
			static const Glib::ustring r("B");
			return r;
		}

		case 'C':
		{
			static const Glib::ustring r("C");
			return r;
		}

		case 'D':
		{
			static const Glib::ustring r("D");
			return r;
		}

		case 'E':
		{
			static const Glib::ustring r("E");
			return r;
		}

		case 'F':
		{
			static const Glib::ustring r("F");
			return r;
		}

		case 'G':
		{
			static const Glib::ustring r("G");
			return r;
		}

		default:
			return empty;
		}
		break;

	case typecode_specialuse:
		switch (bc) {
		case 'A':
		{
			static const Glib::ustring r("ALERT");
			return r;
		}

		case 'D':
		{
			static const Glib::ustring r("DANGER");
			return r;
		}

		case 'M':
		{
			static const Glib::ustring r("MIL OPS AREA");
			return r;
		}

		case 'P':
		{
			static const Glib::ustring r("PROHIBITED");
			return r;
		}

		case 'R':
		{
			static const Glib::ustring r("RESTRICTED");
			return r;
		}

		case 'T':
		{
			static const Glib::ustring r("TEMP RSVD");
			return r;
		}

		case 'W':
		{
			static const Glib::ustring r("WARNING");
			return r;
		}

		default:
			return empty;
		}
		break;

	case typecode_ead:
		switch (bc) {
		case bdryclass_ead_a:
		{
			static const Glib::ustring r("A-EAD");
			return r;
		}

		case bdryclass_ead_adiz:
		{
			static const Glib::ustring r("ADIZ");
			return r;
		}

		case bdryclass_ead_ama:
		{
			static const Glib::ustring r("AMA");
			return r;
		}

		case bdryclass_ead_arinc:
		{
			static const Glib::ustring r("ARINC");
			return r;
		}

		case bdryclass_ead_asr:
		{
			static const Glib::ustring r("ASR");
			return r;
		}

		case bdryclass_ead_awy:
		{
			static const Glib::ustring r("AWY");
			return r;
		}

		case bdryclass_ead_cba:
		{
			static const Glib::ustring r("CBA");
			return r;
		}

		case bdryclass_ead_class:
		{
			static const Glib::ustring r("CLASS");
			return r;
		}

		case bdryclass_ead_cta:
		{
			static const Glib::ustring r("CTA");
			return r;
		}

		case bdryclass_ead_cta_p:
		{
			static const Glib::ustring r("CTA-P");
			return r;
		}

		case bdryclass_ead_ctr:
		{
			static const Glib::ustring r("CTR");
			return r;
		}

		case bdryclass_ead_ctr_p:
		{
			static const Glib::ustring r("CTR-P");
			return r;
		}

		case bdryclass_ead_d_amc:
		{
			static const Glib::ustring r("D-AMC");
			return r;
		}

		case bdryclass_ead_d:
		{
			static const Glib::ustring r("D-EAD");
			return r;
		}

		case bdryclass_ead_d_other:
		{
			static const Glib::ustring r("D-OTHER");
			return r;
		}

		case bdryclass_ead_ead:
		{
			static const Glib::ustring r("EAD");
			return r;
		}

		case bdryclass_ead_fir:
		{
			static const Glib::ustring r("FIR");
			return r;
		}

		case bdryclass_ead_fir_p:
		{
			static const Glib::ustring r("FIR-P");
			return r;
		}

		case bdryclass_ead_nas:
		{
			static const Glib::ustring r("NAS");
			return r;
		}

		case bdryclass_ead_no_fir:
		{
			static const Glib::ustring r("NO-FIR");
			return r;
		}

		case bdryclass_ead_oca:
		{
			static const Glib::ustring r("OCA");
			return r;
		}

		case bdryclass_ead_oca_p:
		{
			static const Glib::ustring r("OCA-P");
			return r;
		}

		case bdryclass_ead_ota:
		{
			static const Glib::ustring r("OTA");
			return r;
		}

		case bdryclass_ead_part:
		{
			static const Glib::ustring r("PART");
			return r;
		}

		case bdryclass_ead_p:
		{
			static const Glib::ustring r("P");
			return r;
		}

		case bdryclass_ead_political:
		{
			static const Glib::ustring r("POLITICAL");
			return r;
		}

		case bdryclass_ead_protect:
		{
			static const Glib::ustring r("PROTECT");
			return r;
		}

		case bdryclass_ead_r_amc:
		{
			static const Glib::ustring r("R-AMC");
			return r;
		}

		case bdryclass_ead_ras:
		{
			static const Glib::ustring r("RAS");
			return r;
		}

		case bdryclass_ead_rca:
		{
			static const Glib::ustring r("RCA");
			return r;
		}

		case bdryclass_ead_r:
		{
			static const Glib::ustring r("R");
			return r;
		}

		case bdryclass_ead_sector_c:
		{
			static const Glib::ustring r("SECTOR-C");
			return r;
		}

		case bdryclass_ead_sector:
		{
			static const Glib::ustring r("SECTOR");
			return r;
		}

		case bdryclass_ead_tma:
		{
			static const Glib::ustring r("TMA");
			return r;
		}

		case bdryclass_ead_tma_p:
		{
			static const Glib::ustring r("TMA-P");
			return r;
		}

		case bdryclass_ead_tra:
		{
			static const Glib::ustring r("TRA");
			return r;
		}

		case bdryclass_ead_tsa:
		{
			static const Glib::ustring r("TSA");
			return r;
		}

		case bdryclass_ead_uir:
		{
			static const Glib::ustring r("UIR");
			return r;
		}

		case bdryclass_ead_uir_p:
		{
			static const Glib::ustring r("UIR-P");
			return r;
		}

		case bdryclass_ead_uta:
		{
			static const Glib::ustring r("UTA");
			return r;
		}

		case bdryclass_ead_w:
		{
			static const Glib::ustring r("W");
			return r;
		}

		default:
			return empty;
		}
		break;
	}
}

bool DbBaseElements::Airspace::set_class_string(uint8_t& tc, char& bc, const Glib::ustring& cs)
{

	for (char cls = 'A'; cls <= 'G'; ++cls) {
		if (get_class_string(cls, 0) == cs) {
			tc = 0;
			bc = cls;
			return true;
		}
		for (char tc1 = 'A'; tc1 <= 'D'; ++tc1) {
			if (get_class_string(cls, tc1) == cs) {
				tc = tc1;
				bc = cls;
				return true;
			}
		}
	}
	static const char suaschars[] = "ADMPRTW";
	for (const char *cp = suaschars; *cp; ++cp) {
		if (get_class_string(*cp, typecode_specialuse) == cs) {
			bc = *cp;
			tc = typecode_specialuse;
			return true;
		}
	}
	static const char codetypes[] = {
		AirspacesDb::Airspace::bdryclass_ead_a,
		AirspacesDb::Airspace::bdryclass_ead_adiz,
		AirspacesDb::Airspace::bdryclass_ead_ama,
		AirspacesDb::Airspace::bdryclass_ead_arinc,
		AirspacesDb::Airspace::bdryclass_ead_asr,
		AirspacesDb::Airspace::bdryclass_ead_awy,
		AirspacesDb::Airspace::bdryclass_ead_cba,
		AirspacesDb::Airspace::bdryclass_ead_class,
		AirspacesDb::Airspace::bdryclass_ead_cta,
		AirspacesDb::Airspace::bdryclass_ead_cta_p,
		AirspacesDb::Airspace::bdryclass_ead_ctr,
		AirspacesDb::Airspace::bdryclass_ead_ctr_p,
		AirspacesDb::Airspace::bdryclass_ead_d_amc,
		AirspacesDb::Airspace::bdryclass_ead_d,
		AirspacesDb::Airspace::bdryclass_ead_d_other,
		AirspacesDb::Airspace::bdryclass_ead_ead,
		AirspacesDb::Airspace::bdryclass_ead_fir,
		AirspacesDb::Airspace::bdryclass_ead_fir_p,
		AirspacesDb::Airspace::bdryclass_ead_nas,
		AirspacesDb::Airspace::bdryclass_ead_no_fir,
		AirspacesDb::Airspace::bdryclass_ead_oca,
		AirspacesDb::Airspace::bdryclass_ead_oca_p,
		AirspacesDb::Airspace::bdryclass_ead_ota,
		AirspacesDb::Airspace::bdryclass_ead_part,
		AirspacesDb::Airspace::bdryclass_ead_p,
		AirspacesDb::Airspace::bdryclass_ead_political,
		AirspacesDb::Airspace::bdryclass_ead_protect,
		AirspacesDb::Airspace::bdryclass_ead_r_amc,
		AirspacesDb::Airspace::bdryclass_ead_ras,
		AirspacesDb::Airspace::bdryclass_ead_rca,
		AirspacesDb::Airspace::bdryclass_ead_r,
		AirspacesDb::Airspace::bdryclass_ead_sector_c,
		AirspacesDb::Airspace::bdryclass_ead_sector,
		AirspacesDb::Airspace::bdryclass_ead_tma,
		AirspacesDb::Airspace::bdryclass_ead_tma_p,
		AirspacesDb::Airspace::bdryclass_ead_tra,
		AirspacesDb::Airspace::bdryclass_ead_tsa,
		AirspacesDb::Airspace::bdryclass_ead_uir,
		AirspacesDb::Airspace::bdryclass_ead_uir_p,
		AirspacesDb::Airspace::bdryclass_ead_uta,
		AirspacesDb::Airspace::bdryclass_ead_w
	};
	for (unsigned int i = 0; i < sizeof(codetypes)/sizeof(codetypes[0]); ++i) {
		if (get_class_string(codetypes[i], typecode_ead) == cs) {
			bc = codetypes[i];
			tc = typecode_ead;
			return true;
		}
	}
	return false;
}

const Glib::ustring& DbBaseElements::Airspace::get_type_string(uint8_t tc)
{
	switch (tc) {
	case typecode_specialuse:
	{
		static const Glib::ustring r("Special Use Airspace");
		return r;
	}

	case typecode_ead:
	{
		static const Glib::ustring r("EAD");
		return r;
	}

	case 'A':
	{
		static const Glib::ustring r("CIV");
		return r;
	}

	case 'B':
	{
		static const Glib::ustring r("CIV/MIL");
		return r;
	}

	case 'C':
	{
		static const Glib::ustring r("MIL");
		return r;
	}

	case 'D':
	default:
	{
		static const Glib::ustring r("other");
		return r;
	}
	}
}
