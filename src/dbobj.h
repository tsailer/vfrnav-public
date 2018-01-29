//
// C++ Interface: dbobj
//
// Description:
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2008, 2009, 2012, 2013, 2014, 2015, 2016, 2017
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef _DBOBJ_H
#define _DBOBJ_H

#include <limits>
#include <iostream>
#include <list>
#include <set>
#include <glibmm.h>
#include <gdkmm.h>
#include <sqlite3x.hpp>
#include "geom.h"
#include "fplan.h"

#ifdef HAVE_PQXX
#include <pqxx/connection.hxx>
#include <pqxx/transaction.hxx>
#include <pqxx/tuple.hxx>
#endif

namespace Conversions {
	Glib::ustring time_str(time_t t);
	Glib::ustring time_str(const Glib::ustring& fmt, time_t t);
	time_t time_parse(const Glib::ustring& str);
	Glib::ustring timefrac8_str(uint64_t t);
	Glib::ustring timefrac8_str(const Glib::ustring& fmt, uint64_t t);
	uint64_t timefrac8_parse(const Glib::ustring& str);
	Glib::ustring freq_str(uint32_t f);
	Glib::ustring alt_str(int32_t a, uint16_t flg);
	Glib::ustring dist_str(float d);
	Glib::ustring track_str(float t);
	Glib::ustring velocity_str(float v);
	Glib::ustring verticalspeed_str(float v);
	uint32_t unsigned_feet_parse(const Glib::ustring& str, bool *valid = 0);
	int32_t signed_feet_parse(const Glib::ustring& str, bool *valid = 0);
};

template <class T> class DbQueryInterface;

namespace DbBaseElements {
        class TileNumber {
	public:
		typedef unsigned int tile_t;
		typedef std::pair<tile_t,tile_t> tilerange_t;
		static tile_t to_tilenumber(const Point& pt);
		static tile_t to_tilenumber(const Rect& r);
		static tilerange_t to_tilerange(const Rect& r);

		static const unsigned int tile_bits = 16;

		class Iterator {
		public:
			Iterator(const Rect& r);
			tile_t operator()(void) const;
			bool next(void);

		protected:
			tile_t m_tile1, m_tile2, m_tile;
			unsigned int m_bits;
		};

	protected:
		static const Point::coord_t lon_offset = 0xf4000000;
        };

        class ElementAddress;

	class DbElementBase {
	public:
		typedef ElementAddress Address;

		typedef enum {
			table_main = 0,
			table_aux = 1
		} table_t;

		static const unsigned int hibernate_none       = 0;
		static const unsigned int hibernate_id         = 1 << 24;
		static const unsigned int hibernate_xmlnonzero = 1 << 25;
		static const unsigned int hibernate_sqlnotnull = 1 << 26;
		static const unsigned int hibernate_sqlunique  = 1 << 27;
		static const unsigned int hibernate_sqlindex   = 1 << 28;
		static const unsigned int hibernate_sqlnocase  = 1 << 29;
		static const unsigned int hibernate_sqldelete  = 1 << 30;

		DbElementBase(uint64_t id = 0, table_t tbl = table_main) : m_id(id), m_table(tbl) {}
		int64_t get_id(void) const { return m_id; }
		table_t get_table(void) const { return m_table; }
		const Address& get_address(void) const { return (const Address&)(*this); }

		template<class Archive> void hibernate(Archive& ar) {
			ar.io(m_id, "ID", 0, hibernate_id | hibernate_sqlnotnull);
			// Table is not explicitly saved!
		}

		template<class Archive> void hibernate_binary(Archive& ar) {
			ar.io(m_id);
			ar.iouint8(m_table);
		}

	protected:
		int64_t m_id;
		table_t m_table;
	};

        class ElementAddress : public DbElementBase {
	public:
		ElementAddress(uint64_t id = 0, table_t tbl = table_main) : DbElementBase(id, tbl) {}
		bool operator==(const ElementAddress& a2) const { return get_id() == a2.get_id() && get_table() == a2.get_table(); }
		bool operator!=(const ElementAddress& a2) const { return !operator==(a2); }
		operator bool(void) const { return !!m_id; }
        };

	class DbElementSourceBase : public DbElementBase {
	public:
		DbElementSourceBase();
		Glib::ustring get_sourceid(void) const { return m_sourceid; }
		void set_sourceid(const Glib::ustring& id) { m_sourceid = id; }
		time_t get_modtime(void) const { return m_modtime; }
		void set_modtime(time_t t = 0) { m_modtime = t; }

		template<class Archive> void hibernate(Archive& ar) {
			DbElementBase::hibernate<Archive>(ar);
			ar.io(m_sourceid, "SRCID", "sourceid", hibernate_sqlnotnull | hibernate_sqlunique | hibernate_sqlindex | hibernate_sqldelete);
			ar.iotime(m_modtime, "MODIFIED", "modtime", hibernate_sqlindex);
		}

		template<class Archive> void hibernate_binary(Archive& ar) {
			DbElementBase::hibernate_binary(ar);
			ar.io(m_sourceid);
			ar.iouint32(m_modtime);
		}

	protected:
		Glib::ustring m_sourceid;
		time_t m_modtime;
	};

	class DbElementLabelBase : public DbElementBase {
	public:
		typedef enum {
			label_n,
			label_e,
			label_s,
			label_w,
			label_center,
			label_any,
			label_off
		} label_placement_t;

		DbElementLabelBase();

		label_placement_t get_label_placement(void) const { return m_label_placement; }
		void set_label_placement(label_placement_t lp = label_off) { m_label_placement = lp; }
		static const Glib::ustring& get_label_placement_name(label_placement_t lp);
		const Glib::ustring& get_label_placement_name(void) const { return get_label_placement_name(m_label_placement); }

		template<class Archive> void hibernate(Archive& ar) {
			DbElementBase::hibernate<Archive>(ar);
			ar.io(m_label_placement, "LABELPLACEMENT", "label_placement");
		}

		template<class Archive> void hibernate_binary(Archive& ar) {
			DbElementBase::hibernate_binary(ar);
			ar.iouint8(m_label_placement);
		}

	protected:
		label_placement_t m_label_placement;
	};

	class DbElementLabelSourceBase : public DbElementLabelBase {
	public:
		DbElementLabelSourceBase();
		Glib::ustring get_sourceid(void) const { return m_sourceid; }
		void set_sourceid(const Glib::ustring& id) { m_sourceid = id; }
		time_t get_modtime(void) const { return m_modtime; }
		void set_modtime(time_t t = 0) { m_modtime = t; }

		template<class Archive> void hibernate(Archive& ar) {
			DbElementLabelBase::hibernate<Archive>(ar);
			ar.io(m_sourceid, "SRCID", "sourceid", hibernate_sqlnotnull | hibernate_sqlunique | hibernate_sqlindex | hibernate_sqldelete);
			ar.iotime(m_modtime, "MODIFIED", "modtime", hibernate_sqlindex);
		}

		template<class Archive> void hibernate_binary(Archive& ar) {
			DbElementLabelBase::hibernate_binary(ar);
			ar.io(m_sourceid);
			ar.iouint32(m_modtime);
		}

	protected:
		Glib::ustring m_sourceid;
		time_t m_modtime;
	};

	class DbElementLabelSourceCoordBase : public DbElementLabelSourceBase {
	public:
		DbElementLabelSourceCoordBase();

		Point get_coord(void) const { return m_coord; }
		void set_coord(const Point& pt) { m_coord = pt; }

		template<class Archive> void hibernate(Archive& ar) {
			DbElementLabelSourceBase::hibernate<Archive>(ar);
			ar.io(m_coord, "LAT", "LON", "lat", "lon", hibernate_sqlindex);
		}

		template<class Archive> void hibernate_binary(Archive& ar) {
			DbElementLabelSourceBase::hibernate_binary(ar);
			m_coord.hibernate_binary(ar);
		}

	protected:
		Point m_coord;
	};

	class DbElementLabelSourceCoordIcaoNameBase : public DbElementLabelSourceCoordBase {
	public:
		DbElementLabelSourceCoordIcaoNameBase();

		const Glib::ustring& get_icao(void) const { return m_icao; }
		void set_icao(const Glib::ustring& ic) { m_icao = ic; }
		const Glib::ustring& get_name(void) const { return m_name; }
		void set_name(const Glib::ustring& nm) { m_name = nm; }
		Glib::ustring get_icao_name(void) const;

		template<class Archive> void hibernate(Archive& ar) {
			DbElementLabelSourceCoordBase::hibernate<Archive>(ar);
			ar.io(m_icao, "ICAO", "icao", hibernate_sqlindex | hibernate_sqlnocase);
			ar.io(m_name, "NAME", "name", hibernate_sqlindex | hibernate_sqlnocase);
		}

		template<class Archive> void hibernate_binary(Archive& ar) {
			DbElementLabelSourceCoordBase::hibernate_binary(ar);
			ar.io(m_icao);
			ar.io(m_name);
		}

	protected:
		Glib::ustring m_icao;
		Glib::ustring m_name;
	};

	class DbElementLabelSourceCoordIcaoNameAuthorityBase : public DbElementLabelSourceCoordIcaoNameBase {
	public:
		DbElementLabelSourceCoordIcaoNameAuthorityBase();

		const Glib::ustring& get_authority(void) const { return m_authority; }
		void set_authority(const Glib::ustring& au) { m_authority = au; }
		typedef std::set<Glib::ustring> authorityset_t;
		authorityset_t get_authorityset(void) const;
		void set_authorityset(const authorityset_t& as);
		void add_authority(const Glib::ustring& au);
		bool is_authority(const Glib::ustring& au) const;

		template<class Archive> void hibernate(Archive& ar) {
			DbElementLabelSourceCoordIcaoNameBase::hibernate<Archive>(ar);
			ar.io(m_icao, "AUTHORITY", "authority", hibernate_sqlindex | hibernate_sqlnocase);
		}

		template<class Archive> void hibernate_binary(Archive& ar) {
			DbElementLabelSourceCoordIcaoNameBase::hibernate_binary(ar);
			ar.io(m_authority);
		}

	protected:
		Glib::ustring m_authority;
	};

        class Navaid : public DbElementLabelSourceCoordIcaoNameAuthorityBase {
	public:
		typedef ::DbBaseElements::TileNumber TileNumber;
		typedef TileNumber::tile_t tile_t;
		typedef TileNumber::tilerange_t tilerange_t;
		typedef DbElementLabelSourceBase::label_placement_t label_placement_t;

		typedef enum {
			navaid_invalid = FPlanWaypoint::navaid_invalid,
			navaid_vor = FPlanWaypoint::navaid_vor,
			navaid_vordme = FPlanWaypoint::navaid_vordme,
			navaid_dme = FPlanWaypoint::navaid_dme,
			navaid_ndb = FPlanWaypoint::navaid_ndb,
			navaid_ndbdme = FPlanWaypoint::navaid_ndbdme,
			navaid_vortac = FPlanWaypoint::navaid_vortac,
			navaid_tacan = FPlanWaypoint::navaid_tacan
		} navaid_type_t;

		static const unsigned int subtables_none = 0;
		static const unsigned int subtables_all = subtables_none;

		static const char *db_query_string;
		static const char *db_aux_query_string;
		static const char *db_text_fields[];
		static const char *db_time_fields[];

		Navaid(void);
		void load(sqlite3x::sqlite3_cursor& cursor, sqlite3x::sqlite3_connection & db, unsigned int loadsubtables = subtables_all);
		void load_subtables(sqlite3x::sqlite3_connection& db, unsigned int loadsubtables = subtables_all) {}
		unsigned int get_subtables(void) const { return ~0; }
		bool is_subtables_loaded(void) const { return true; }
		void save(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
		void erase(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
		void update_index(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
#ifdef HAVE_PQXX
		void load(const pqxx::tuple& cursor, pqxx::basic_transaction& w, unsigned int loadsubtables = subtables_all);
		void load_subtables(pqxx::basic_transaction& w, unsigned int loadsubtables = subtables_all) {}
		void save(pqxx::connection_base& conn, bool rtree);
		void erase(pqxx::connection_base& conn, bool rtree);
		void update_index(pqxx::connection_base& conn, bool rtree);
#endif
		Point get_dmecoord(void) const { return m_dmecoord; }
		void set_dmecoord(const Point& dmecoord) { m_dmecoord = dmecoord; }
		int32_t get_elev(void) const { return m_elev; }
		void set_elev(int32_t e) { m_elev = e; }
		int32_t get_range(void) const { return m_range; }
		void set_range(int32_t rng = 0) { m_range = rng; }
		uint32_t get_frequency(void) const { return m_freq; }
		void set_frequency(uint32_t f = 0) { m_freq = f; }
		navaid_type_t get_navaid_type(void) const { return m_navaid_type; }
		void set_navaid_type(navaid_type_t nt = navaid_invalid) { m_navaid_type = nt; }
		static const Glib::ustring& get_navaid_typename(navaid_type_t nt);
		const Glib::ustring& get_navaid_typename(void) const { return get_navaid_typename(m_navaid_type); }
		bool is_valid(void) const { return m_navaid_type != navaid_invalid; }
		bool is_inservice(void) const { return m_inservice; }
		void set_inservice(bool is = false) { m_inservice = is; }
		static bool has_ndb(navaid_type_t nt);
		static bool has_vor(navaid_type_t nt);
		static bool has_dme(navaid_type_t nt);
		bool has_ndb(void) const { return has_ndb(get_navaid_type()); }
		bool has_vor(void) const { return has_vor(get_navaid_type()); }
		bool has_dme(void) const { return has_dme(get_navaid_type()); }

		template<class Archive> void hibernate(Archive& ar) {
			DbElementLabelSourceCoordIcaoNameAuthorityBase::hibernate<Archive>(ar);
			ar.io(m_dmecoord, "DMELAT", "DMELON", "dmelat", "dmelon", hibernate_sqlindex);
			ar.io(m_elev, "ELEVATION", "elev");
			ar.io(m_range, "RANGE", "range");
			ar.io(m_freq, "FREQ", "freq", hibernate_sqlindex);
			ar.io(m_navaid_type, "TYPE", "type");
			ar.io(m_inservice, "STATUS", "inservice");
			if (ar.is_save() || ar.is_schema()) {
				tile_t tile(TileNumber::to_tilenumber(get_coord()));
				ar.io(tile, "TILE", 0, hibernate_sqlindex);
			}
		}

		template<class Archive> void hibernate_binary(Archive& ar) {
			DbElementLabelSourceCoordIcaoNameAuthorityBase::hibernate_binary(ar);
			m_dmecoord.hibernate_binary(ar);
			ar.io(m_elev);
			ar.io(m_range);
			ar.io(m_freq);
			ar.iouint8(m_navaid_type);
			ar.io(m_inservice);
		}

	protected:
		Point m_dmecoord;
		int32_t m_elev;
		int32_t m_range;
		uint32_t m_freq;
		navaid_type_t m_navaid_type;
		bool m_inservice;
        };

        class Waypoint : public DbElementLabelSourceCoordIcaoNameAuthorityBase {
	public:
		typedef ::DbBaseElements::TileNumber TileNumber;
		typedef TileNumber::tile_t tile_t;
		typedef TileNumber::tilerange_t tilerange_t;
		typedef DbElementLabelSourceBase::label_placement_t label_placement_t;

		static const char usage_invalid = 0;
		static const char usage_highlevel = 'H';
		static const char usage_lowlevel = 'L';
		static const char usage_bothlevel = 'B';
		static const char usage_rnav = 'R';
		static const char usage_terminal = 'T';
		static const char usage_vfr = 'V';
		static const char usage_user = 'U';

		static const char type_invalid = 0;
		static const char type_unknown = 'U';
		static const char type_adhp = 'A';
		static const char type_icao = 'I';
		static const char type_coord = 'C';
		static const char type_other = 'O';

		static const unsigned int subtables_none = 0;
		static const unsigned int subtables_all = subtables_none;

		static const char *db_query_string;
		static const char *db_aux_query_string;
		static const char *db_text_fields[];
		static const char *db_time_fields[];

		Waypoint(void);
		void load(sqlite3x::sqlite3_cursor& cursor, sqlite3x::sqlite3_connection & db, unsigned int loadsubtables = subtables_all);
		void load_subtables(sqlite3x::sqlite3_connection& db, unsigned int loadsubtables = subtables_all) {}
		unsigned int get_subtables(void) const { return ~0; }
		bool is_subtables_loaded(void) const { return true; }
		void save(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
		void erase(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
		void update_index(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
#ifdef HAVE_PQXX
		void load(const pqxx::tuple& cursor, pqxx::basic_transaction& w, unsigned int loadsubtables = subtables_all);
		void load_subtables(pqxx::basic_transaction& w, unsigned int loadsubtables = subtables_all) {}
		void save(pqxx::connection_base& conn, bool rtree);
		void erase(pqxx::connection_base& conn, bool rtree);
		void update_index(pqxx::connection_base& conn, bool rtree);
#endif
		char get_usage(void) const { return m_usage; }
		void set_usage(char u) { m_usage = u; }
		static const Glib::ustring& get_usage_name(char usage);
		const Glib::ustring& get_usage_name(void) const { return get_usage_name(get_usage()); }
		char get_type(void) const { return m_type; }
		void set_type(char t) { m_type = t; }
		static const Glib::ustring& get_type_name(char t);
		const Glib::ustring& get_type_name(void) const { return get_type_name(get_type()); }
		bool is_valid(void) const { return get_usage() != usage_invalid && get_type() != type_invalid; }

		template<class Archive> void hibernate(Archive& ar) {
			DbElementLabelSourceCoordIcaoNameAuthorityBase::hibernate<Archive>(ar);
			ar.io(m_usage, "USAGE", "usage");
			ar.io(m_type, "TYPE", "type");
			if (ar.is_save() || ar.is_schema()) {
				tile_t tile(TileNumber::to_tilenumber(get_coord()));
				ar.io(tile, "TILE", 0, hibernate_sqlindex);
			}
		}

		template<class Archive> void hibernate_binary(Archive& ar) {
			DbElementLabelSourceCoordIcaoNameAuthorityBase::hibernate_binary(ar);
			ar.io(m_usage);
			ar.io(m_type);
		}

	protected:
		char m_usage;
		char m_type;
        };

        class Airway : public DbElementLabelSourceBase {
	public:
		typedef ::DbBaseElements::TileNumber TileNumber;
		typedef TileNumber::tile_t tile_t;
		typedef TileNumber::tilerange_t tilerange_t;
		typedef DbElementLabelSourceBase::label_placement_t label_placement_t;

		typedef enum {
			airway_invalid,
			airway_low,
			airway_high,
			airway_both,
			airway_user
		} airway_type_t;

		static const unsigned int subtables_none = 0;
		static const unsigned int subtables_all = subtables_none;

		static const char *db_query_string;
		static const char *db_aux_query_string;
		static const char *db_text_fields[];
		static const char *db_time_fields[];

		typedef int16_t elev_t;
		static const elev_t nodata;

		Airway(void);
		void load(sqlite3x::sqlite3_cursor& cursor, sqlite3x::sqlite3_connection & db, unsigned int loadsubtables = subtables_all);
		void load_subtables(sqlite3x::sqlite3_connection& db, unsigned int loadsubtables = subtables_all) {}
		unsigned int get_subtables(void) const { return ~0; }
		bool is_subtables_loaded(void) const { return true; }
		void save(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
		void erase(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
		void update_index(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
#ifdef HAVE_PQXX
		void load(const pqxx::tuple& cursor, pqxx::basic_transaction& w, unsigned int loadsubtables = subtables_all);
		void load_subtables(pqxx::basic_transaction& w, unsigned int loadsubtables = subtables_all) {}
		void save(pqxx::connection_base& conn, bool rtree);
		void erase(pqxx::connection_base& conn, bool rtree);
		void update_index(pqxx::connection_base& conn, bool rtree);
#endif
		Rect get_bbox(void) const;
		Point get_labelcoord(void) const { return m_begin_coord.halfway(m_end_coord); }
		Point get_begin_coord(void) const { return m_begin_coord; }
		void set_begin_coord(const Point& pt) { m_begin_coord = pt; }
		const Glib::ustring& get_begin_name(void) const { return m_begin_name; }
		void set_begin_name(const Glib::ustring& nm) { m_begin_name = nm; }
		Point get_end_coord(void) const { return m_end_coord; }
		void set_end_coord(const Point& pt) { m_end_coord = pt; }
		const Glib::ustring& get_end_name(void) const { return m_end_name; }
		void set_end_name(const Glib::ustring& nm) { m_end_name = nm; }
		const Glib::ustring& get_name(void) const { return m_name; }
		void set_name(const Glib::ustring& nm) { m_name = nm; }
		typedef std::set<Glib::ustring> nameset_t;
		nameset_t get_name_set(void) const;
		void set_name_set(const nameset_t& nameset);
		bool add_name(const Glib::ustring& name);
		bool remove_name(const Glib::ustring& name);
		bool contains_name(const Glib::ustring& name) const;
		int16_t get_base_level(void) const { return m_base_level; }
		void set_base_level(int16_t lvl) { m_base_level = lvl; }
		int16_t get_top_level(void) const { return m_top_level; }
		void set_top_level(int16_t lvl) { m_top_level = lvl; }
		elev_t get_terrain_elev(void) const { return m_terrainelev; }
		void set_terrain_elev(elev_t e = nodata) { m_terrainelev = e; }
		bool is_terrain_elev_valid(void) const { return get_terrain_elev() != nodata; }
		elev_t get_corridor5_elev(void) const { return m_corridor5elev; }
		void set_corridor5_elev(elev_t e = nodata) { m_corridor5elev = e; }
		bool is_corridor5_elev_valid(void) const { return get_corridor5_elev() != nodata; }
		airway_type_t get_type(void) const { return m_type; }
		void set_type(airway_type_t t) { m_type = t; }
		static const Glib::ustring& get_type_name(airway_type_t t);
		const Glib::ustring& get_type_name(void) const { return get_type_name(m_type); }
		bool is_valid(void) const { return m_type != airway_invalid; }
		void swap_begin_end(void) { std::swap(m_begin_coord, m_end_coord); m_begin_name.swap(m_end_name); }

		template<class Archive> void hibernate(Archive& ar) {
			DbElementLabelSourceBase::hibernate<Archive>(ar);
			ar.io(m_begin_coord, "BLAT", "BLON", "begin", "begin");
			ar.io(m_end_coord, "ELAT", "ELON", "end", "end");
			ar.iosqlindex("LATLON", "BLAT,BLON,ELAT,ELON");
			ar.io(m_begin_name, "BNAME", "beginname", hibernate_sqlindex | hibernate_sqlnocase);
			ar.io(m_end_name, "ENAME", "endname", hibernate_sqlindex | hibernate_sqlnocase);
			ar.io(m_name, "NAME", "name", hibernate_sqlindex | hibernate_sqlnocase);
			ar.io(m_base_level, "BLEVEL", "baselevel");
			ar.io(m_top_level, "TLEVEL", "toplevel");
			ar.io(m_terrainelev, "TELEV", "terrainelev");
			ar.io(m_corridor5elev, "C5ELEV", "corridor5elev");
			ar.io(m_type, "TYPE", "type");
			Rect bbox(get_bbox());
			ar.io(bbox, hibernate_sqlindex);
			if (ar.is_save() || ar.is_schema()) {
				tile_t tile(TileNumber::to_tilenumber(bbox));
				ar.io(tile, "TILE", 0, hibernate_sqlindex);
			}
		}

		template<class Archive> void hibernate_binary(Archive& ar) {
			DbElementLabelSourceCoordIcaoNameAuthorityBase::hibernate_binary(ar);
			m_begin_coord.hibernate_binary(ar);
			m_end_coord.hibernate_binary(ar);
			ar.io(m_begin_name);
			ar.io(m_end_name);
			ar.io(m_name);
			ar.io(m_base_level);
			ar.io(m_top_level);
			ar.iouint8(m_terrainelev);
			ar.iouint8(m_corridor5elev);
			ar.iouint8(m_type);
		}

	protected:
		Point m_begin_coord;
		Point m_end_coord;
		Glib::ustring m_begin_name;
		Glib::ustring m_end_name;
		Glib::ustring m_name;
		int16_t m_base_level;
		int16_t m_top_level;
		elev_t m_terrainelev;
		elev_t m_corridor5elev;
		airway_type_t m_type;
        };

        class Airport : public DbElementLabelSourceCoordIcaoNameAuthorityBase {
	public:
		typedef ::DbBaseElements::TileNumber TileNumber;
		typedef TileNumber::tile_t tile_t;
		typedef TileNumber::tilerange_t tilerange_t;
		typedef DbElementLabelSourceBase::label_placement_t label_placement_t;

		static const unsigned int subtables_none = 0;
		static const unsigned int subtables_runways = (1 << 0);
		static const unsigned int subtables_helipads = (1 << 1);
		static const unsigned int subtables_comms = (1 << 2);
		static const unsigned int subtables_vfrroutes = (1 << 3);
		static const unsigned int subtables_linefeatures = (1 << 4);
		static const unsigned int subtables_pointfeatures = (1 << 5);
		static const unsigned int subtables_fas = (1 << 6);
		static const unsigned int subtables_all = (subtables_runways | subtables_helipads | subtables_comms | subtables_vfrroutes | subtables_linefeatures | subtables_pointfeatures | subtables_fas);

		class FASNavigate;

		class Runway {
		public:
			static const uint16_t flag_le_usable = (1 << 0);
			static const uint16_t flag_he_usable = (1 << 1);

			static const unsigned int nr_lights = 8;

			typedef uint8_t light_t;
			static const light_t light_off                     = 0;
			static const light_t light_pcl                     = 1; // Pilot Controlled Lighting
			static const light_t light_sf                      = 2; // Sequenced Flashing Lights
			static const light_t light_tdzl                    = 3; // Touchdown Zone Lighting
			static const light_t light_cl                      = 4; // Centerline Lighting System
			static const light_t light_hirl                    = 5; // High Intensity Runway Lights
			static const light_t light_mirl                    = 6; // Medium Intensity Runway Lighting System
			static const light_t light_lirl                    = 7; // Low Intensity Runway Lighting System
			static const light_t light_rail                    = 8; // Runway Alignment Lights
			static const light_t light_reil                    = 9; // Runway End Identifier Lights
			static const light_t light_alsf2                   = 10; // ALSF-2
			static const light_t light_alsf1                   = 11; // ALSF-1
			static const light_t light_sals_salsf              = 12; // SALS or SALSF
			static const light_t light_ssalr                   = 13; // SSALR
			static const light_t light_mals_malsf_ssals_ssalsf = 14; // MALS and MALSF or SSALS and SSALF
			static const light_t light_malsr                   = 15; // MALSR
			static const light_t light_overruncenterline       = 16; // Overrun Centerline
			static const light_t light_center                  = 17; // Centerline and Bar
			static const light_t light_usconfiguration         = 18; // US Configuration (b)
			static const light_t light_hongkongcurve           = 19; // Hong Kong Curve
			static const light_t light_centerrow               = 20; // Center row
			static const light_t light_leftsinglerow           = 21; // Left Single Row
			static const light_t light_formernatostandard      = 22; // Former NATO Standard ©
			static const light_t light_centerrow2              = 23; // Center Row
			static const light_t light_natostandard            = 24; // NATO standard
			static const light_t light_centerdoublerow         = 25; // Center and Double Row
			static const light_t light_portableapproach        = 26; // Portable Approach
			static const light_t light_centerrow3              = 27; // Center Row
			static const light_t light_hals                    = 28; // Helicopter Approach Lighting System (HALS)
			static const light_t light_2parallelrow            = 30; // Two Parallel row
			static const light_t light_leftrow                 = 31; // Left Row (High Intensity)
			static const light_t light_airforceoverrun         = 32; // Air Force Overrun
			static const light_t light_calvert                 = 33; // Calvert (British)
			static const light_t light_singlerowcenterline     = 34; // Single Row Centerline
			static const light_t light_narrowmulticross        = 35; // Narrow Multi-cross
			static const light_t light_centerlinehighintensity = 36; // Centerline High Intensity
			static const light_t light_alternatecenverline     = 37; // Alternate Centerline and Bar
			static const light_t light_cross                   = 38; // Cross
			static const light_t light_centerrow4              = 39; // Center Row
			static const light_t light_singaporecenterline     = 40; // Singapore Centerline
			static const light_t light_centerline2crossbars    = 41; // Centerline 2 Crossbars
			static const light_t light_odals                   = 42; // Omni-directional Approach Lighting System
			static const light_t light_vasi                    = 43; // Visual Approach Slope Indicator
			static const light_t light_tvasi                   = 44; // T-Visual Approach Slope Indicator
			static const light_t light_pvasi                   = 45; // Pulsating Visual Approach Slope Indicator
			static const light_t light_jumbo                   = 46; // VASI with a TCH to accommodate long bodied or jumbo aircraft
			static const light_t light_tricolor                = 47; // Tri-color Arrival Approach (TRICOLOR)
			static const light_t light_apap                    = 48; // Alignment of Elements System
			static const light_t light_papi                    = 50; // Precision Approach Path Indicator
			static const light_t light_ols                     = 51; // Optical landing System
			static const light_t light_waveoff                 = 52;
			static const light_t light_portable                = 53;
			static const light_t light_floods                  = 54;
			static const light_t light_lights                  = 55;
			static const light_t light_lcvasi                  = 56; // Low Cost Visual Approach Slope Indicator

			Runway(const Glib::ustring& idhe, const Glib::ustring& idle, uint16_t len, uint16_t wid,
			       const Glib::ustring& sfc, const Point& hecoord, const Point& lecoord,
			       uint16_t hetda, uint16_t helda, uint16_t hedisp, uint16_t hehdg, int16_t heelev,
			       uint16_t letda, uint16_t lelda, uint16_t ledisp, uint16_t lehdg, int16_t leelev,
			       uint16_t flags);
			Runway(void);
			const Glib::ustring& get_ident_he(void) const { return m_ident_he; }
			const Glib::ustring& get_ident_le(void) const { return m_ident_le; }
			uint16_t get_length(void) const { return m_length; }
			uint16_t get_width(void) const { return m_width; }
			const Glib::ustring& get_surface(void) const { return m_surface; }
			Point get_he_coord(void) const { return m_he_coord; }
			Point get_le_coord(void) const { return m_le_coord; }
			uint16_t get_he_tda(void) const { return m_he_tda; }
			uint16_t get_he_lda(void) const { return m_he_lda; }
			uint16_t get_he_disp(void) const { return m_he_disp; }
			uint16_t get_he_hdg(void) const { return m_he_hdg; }
			int16_t get_he_elev(void) const { return m_he_elev; }
			uint16_t get_le_tda(void) const { return m_le_tda; }
			uint16_t get_le_lda(void) const { return m_le_lda; }
			uint16_t get_le_disp(void) const { return m_le_disp; }
			uint16_t get_le_hdg(void) const { return m_le_hdg; }
			int16_t get_le_elev(void) const { return m_le_elev; }
			uint16_t get_flags(void) const { return m_flags; }
			light_t get_he_light(unsigned int idx) const { return m_he_lights[idx & 7]; }
			light_t get_le_light(unsigned int idx) const { return m_le_lights[idx & 7]; }
			void set_ident_he(const Glib::ustring& x) { m_ident_he = x; }
			void set_ident_le(const Glib::ustring& x) { m_ident_le = x; }
			void set_length(uint16_t x) { m_length = x; }
			void set_width(uint16_t x) { m_width = x; }
			void set_surface(const Glib::ustring& x) { m_surface = x; }
			void set_he_coord(const Point& pt) { m_he_coord = pt; }
			void set_le_coord(const Point& pt) { m_le_coord = pt; }
			void set_he_tda(uint16_t x) { m_he_tda = x; }
			void set_he_lda(uint16_t x) { m_he_lda = x; }
			void set_he_disp(uint16_t x) { m_he_disp = x; }
			void set_he_hdg(uint16_t x) { m_he_hdg = x; }
			void set_he_elev(int16_t x) { m_he_elev = x; }
			void set_le_tda(uint16_t x) { m_le_tda = x; }
			void set_le_lda(uint16_t x) { m_le_lda = x; }
			void set_le_disp(uint16_t x) { m_le_disp = x; }
			void set_le_hdg(uint16_t x) { m_le_hdg = x; }
			void set_le_elev(int16_t x) { m_le_elev = x; }
			void set_flags(uint16_t x) { m_flags = x; }
			void set_he_light(unsigned int idx, light_t x) { m_he_lights[idx & 7] = x; }
			void set_le_light(unsigned int idx, light_t x) { m_le_lights[idx & 7] = x; }
			bool is_he_usable(void) const { return !!(m_flags & flag_he_usable); }
			bool is_le_usable(void) const { return !!(m_flags & flag_le_usable); }
			bool is_concrete(void) const;
			bool is_water(void) const;

			static const Glib::ustring& get_light_string(light_t x);

			void compute_default_hdg(void);
			void compute_default_coord(const Point& arptcoord);

			void swap_he_le(void);
			bool normalize_he_le(void);

			FASNavigate get_he_fake_fas(void) const;
			FASNavigate get_le_fake_fas(void) const;

			template<class Archive> void hibernate(Archive& ar) {
				ar.io(m_length, "LENGTH", "length");
				ar.io(m_width, "WIDTH", "width");
				ar.io(m_surface, "SURFACE", "surface");
				ar.io(m_flags, "FLAGS", "flags");
				ar.ioxmlstart("he");
				ar.io(m_ident_he, "IDENTHE", "ident");
				ar.io(m_he_coord, "HELAT", "HELON", "lat", "lon");
				ar.io(m_he_tda, "HETDA", "tda");
				ar.io(m_he_lda, "HELDA", "lda");
				ar.io(m_he_disp, "HEDISP", "disp");
				ar.io(m_he_hdg, "HEHDG", "hdg");
				ar.io(m_he_elev, "HEELEV", "elev");
				ar.io(m_he_lights[0], "HELIGHT0", "light0");
				ar.io(m_he_lights[1], "HELIGHT1", "light1");
				ar.io(m_he_lights[2], "HELIGHT2", "light2");
				ar.io(m_he_lights[3], "HELIGHT3", "light3");
				ar.io(m_he_lights[4], "HELIGHT4", "light4");
				ar.io(m_he_lights[5], "HELIGHT5", "light5");
				ar.io(m_he_lights[6], "HELIGHT6", "light6");
				ar.io(m_he_lights[7], "HELIGHT7", "light7");
				ar.ioxmlend();
				ar.ioxmlstart("le");
				ar.io(m_ident_le, "IDENTLE", "ident");
				ar.io(m_le_coord, "LELAT", "LELON", "lat", "lon");
				ar.io(m_le_tda, "LETDA", "tda");
				ar.io(m_le_lda, "LELDA", "lda");
				ar.io(m_le_disp, "LEDISP", "disp");
				ar.io(m_le_hdg, "LEHDG", "hdg");
				ar.io(m_le_elev, "LEELEV", "elev");
				ar.io(m_le_lights[0], "LELIGHT0", "light0");
				ar.io(m_le_lights[1], "LELIGHT1", "light1");
				ar.io(m_le_lights[2], "LELIGHT2", "light2");
				ar.io(m_le_lights[3], "LELIGHT3", "light3");
				ar.io(m_le_lights[4], "LELIGHT4", "light4");
				ar.io(m_le_lights[5], "LELIGHT5", "light5");
				ar.io(m_le_lights[6], "LELIGHT6", "light6");
				ar.io(m_le_lights[7], "LELIGHT7", "light7");
				ar.ioxmlend();
			}

			template<class Archive> void hibernate_binary(Archive& ar) {
				ar.io(m_ident_he);
				ar.io(m_ident_le);
				ar.io(m_length);
				ar.io(m_width);
				ar.io(m_surface);
				m_he_coord.hibernate_binary(ar);
				m_le_coord.hibernate_binary(ar);
				ar.io(m_he_tda);
				ar.io(m_he_lda);
				ar.io(m_he_disp);
				ar.io(m_he_hdg);
				ar.io(m_he_elev);
				ar.io(m_le_tda);
				ar.io(m_le_lda);
				ar.io(m_le_disp);
				ar.io(m_le_hdg);
				ar.io(m_le_elev);
				ar.io(m_flags);
				for (unsigned int i = 0; i < nr_lights; ++i)
					ar.io(m_he_lights[i]);
				for (unsigned int i = 0; i < nr_lights; ++i)
					ar.io(m_le_lights[i]);
			}

		protected:
			Glib::ustring m_ident_he;
			Glib::ustring m_ident_le;
			uint16_t m_length;
			uint16_t m_width;
			Glib::ustring m_surface;
			Point m_he_coord;
			Point m_le_coord;
			uint16_t m_he_tda;
			uint16_t m_he_lda;
			uint16_t m_he_disp;
			uint16_t m_he_hdg;
			int16_t m_he_elev;
			uint16_t m_le_tda;
			uint16_t m_le_lda;
			uint16_t m_le_disp;
			uint16_t m_le_hdg;
			int16_t m_le_elev;
			uint16_t m_flags;
			light_t m_he_lights[nr_lights];
			light_t m_le_lights[nr_lights];
		};

		class Helipad {
		public:
			static const uint16_t flag_usable = (1 << 0);

			Helipad(const Glib::ustring& id, uint16_t len, uint16_t wid,
				const Glib::ustring& sfc, const Point& coord,
				uint16_t hdg, int16_t elev, uint16_t flags);
			Helipad(void);
			const Glib::ustring& get_ident(void) const { return m_ident; }
			uint16_t get_length(void) const { return m_length; }
			uint16_t get_width(void) const { return m_width; }
			const Glib::ustring& get_surface(void) const { return m_surface; }
			Point get_coord(void) const { return m_coord; }
			uint16_t get_hdg(void) const { return m_hdg; }
			int16_t get_elev(void) const { return m_elev; }
			uint16_t get_flags(void) const { return m_flags; }
			void set_ident(const Glib::ustring& x) { m_ident = x; }
			void set_length(uint16_t x) { m_length = x; }
			void set_width(uint16_t x) { m_width = x; }
			void set_surface(const Glib::ustring& x) { m_surface = x; }
			void set_coord(const Point& pt) { m_coord = pt; }
			void set_hdg(uint16_t x) { m_hdg = x; }
			void set_elev(int16_t x) { m_elev = x; }
			void set_flags(uint16_t x) { m_flags = x; }
			bool is_usable(void) const { return !!(m_flags & flag_usable); }
			bool is_concrete(void) const;

			template<class Archive> void hibernate(Archive& ar) {
				ar.io(m_ident, "IDENT", "ident");
				ar.io(m_length, "LENGTH", "length");
				ar.io(m_width, "WIDTH", "width");
				ar.io(m_surface, "SURFACE", "surface");
				ar.io(m_coord, "LAT", "LON", "lat", "lon");
				ar.io(m_hdg, "HDG", "hdg");
				ar.io(m_elev, "ELEV", "elev");
				ar.io(m_flags, "FLAGS", "flags");
			}

			template<class Archive> void hibernate_binary(Archive& ar) {
				ar.io(m_ident);
				ar.io(m_length);
				ar.io(m_width);
				ar.io(m_surface);
				m_coord.hibernate_binary(ar);
				ar.io(m_hdg);
				ar.io(m_elev);
				ar.io(m_flags);
			}

		protected:
			Glib::ustring m_ident;
			uint16_t m_length;
			uint16_t m_width;
			Glib::ustring m_surface;
			Point m_coord;
			uint16_t m_hdg;
			int16_t m_elev;
			uint16_t m_flags;
		};

		class Comm {
		public:
			Comm(const Glib::ustring& nm, const Glib::ustring& sect, const Glib::ustring& oprhr,
			     uint32_t f0 = 0, uint32_t f1 = 0, uint32_t f2 = 0, uint32_t f3 = 0, uint32_t f4 = 0);
			Comm(void);
			const Glib::ustring& get_name(void) const { return m_name; }
			const Glib::ustring& get_sector(void) const { return m_sector; }
			const Glib::ustring& get_oprhours(void) const { return m_oprhours; }
			uint32_t operator[](unsigned int x) const { if (x >= 5) return 0; return m_freq[x]; }
			void set_name(const Glib::ustring& x) { m_name = x; }
			void set_sector(const Glib::ustring& x) { m_sector = x; }
			void set_oprhours(const Glib::ustring& x) { m_oprhours = x; }
			void set_freq(unsigned int x, uint32_t y) { if (x < 5) m_freq[x] = y; }

			template<class Archive> void hibernate(Archive& ar) {
				ar.io(m_freq[0], "FREQ0", "freq0", hibernate_xmlnonzero);
				ar.io(m_freq[1], "FREQ1", "freq1", hibernate_xmlnonzero);
				ar.io(m_freq[2], "FREQ2", "freq2", hibernate_xmlnonzero);
				ar.io(m_freq[3], "FREQ3", "freq3", hibernate_xmlnonzero);
				ar.io(m_freq[4], "FREQ4", "freq4", hibernate_xmlnonzero);
				ar.io(m_name, "NAME", "name");
				ar.io(m_sector, "SECTOR", "sector");
				ar.io(m_oprhours, "OPRHOURS", "oprhours");
			}

			template<class Archive> void hibernate_binary(Archive& ar) {
				for (int i = 0; i < 5; ++i)
					ar.io(m_freq[i]);
				ar.io(m_name);
				ar.io(m_sector);
				ar.io(m_oprhours);
			}

		protected:
			uint32_t m_freq[5];
			Glib::ustring m_name;
			Glib::ustring m_sector;
			Glib::ustring m_oprhours;
		};

		class VFRRoute {
		public:
			class VFRRoutePoint {
			public:
				typedef enum {
					pathcode_arrival,
					pathcode_departure,
					pathcode_holding,
					pathcode_trafficpattern,
					pathcode_vfrtransition,
					pathcode_other,
					pathcode_star,
					pathcode_sid,
					pathcode_invalid
				} pathcode_t;

				typedef enum {
					altcode_atorabove,
					altcode_atorbelow,
					altcode_assigned,
					altcode_altspecified,
					altcode_recommendedalt,
					altcode_invalid
				} altcode_t;

				VFRRoutePoint(const Glib::ustring& nm, const Point& pt, int16_t al, label_placement_t lp, char pts, pathcode_t pc, altcode_t ac, bool aa);
				VFRRoutePoint(void);
				const Glib::ustring& get_name(void) const { return m_name; }
				Point get_coord(void) const { return m_coord; }
				int16_t get_altitude(void) const { return m_alt; }
				Glib::ustring get_altitude_string(void) const;
				label_placement_t get_label_placement(void) const { return m_label_placement; }
				char get_ptsym(void) const { return m_ptsym; }
				pathcode_t get_pathcode(void) const { return m_pathcode; }
				static const Glib::ustring& get_pathcode_string(pathcode_t pc);
				const Glib::ustring& get_pathcode_string(void) const { return get_pathcode_string(m_pathcode); }
				static FPlanWaypoint::pathcode_t get_fplan_pathcode(pathcode_t pc);
				FPlanWaypoint::pathcode_t get_fplan_pathcode(void) const { return get_fplan_pathcode(get_pathcode()); }
				altcode_t get_altcode(void) const { return m_altcode; }
				static const Glib::ustring& get_altcode_string(altcode_t ac);
				const Glib::ustring& get_altcode_string(void) const { return get_altcode_string(m_altcode); }
				bool is_at_airport(void) const { return m_at_airport; }
				bool is_compulsory(void) const { return get_ptsym() == 'C'; }
				void set_name(const Glib::ustring& x) { m_name = x; }
				void set_coord(const Point& x) { m_coord = x; }
				void set_altitude(int16_t x) { m_alt = x; }
				void set_label_placement(label_placement_t x) { m_label_placement = x; }
				void set_ptsym(char x) { m_ptsym = x; }
				void set_pathcode(pathcode_t x) { m_pathcode = x; }
				void set_altcode(altcode_t x) { m_altcode = x; }
				void set_at_airport(bool x) { m_at_airport = x; }

				template<class Archive> void hibernate(Archive& ar) {
					ar.io(m_name, "NAME", "name", hibernate_sqlindex | hibernate_sqlnocase);
					ar.io(m_coord, "LAT", "LON", "lat", "lon", hibernate_sqlindex);
					ar.io(m_alt, "ALT", "alt");
					ar.io(m_label_placement, "LABELPLACEMENT", "label_placement");
					ar.io(m_ptsym, "SYM", "symbol");
					ar.io(m_pathcode, "PATHCODE", "pathcode");
					ar.io(m_altcode, "ALTCODE", "altcode");
					ar.io(m_at_airport, "ATAIRPORT", "atairport");
				}

				template<class Archive> void hibernate_binary(Archive& ar) {
					ar.io(m_name);
					m_coord.hibernate_binary(ar);
					ar.io(m_alt);
					ar.iouint8(m_label_placement);
					ar.io(m_ptsym);
					ar.iouint8(m_pathcode);
					ar.iouint8(m_altcode);
					ar.io(m_at_airport);
				}

			private:
				Glib::ustring m_name;
				Point m_coord;
				int16_t m_alt;
				label_placement_t m_label_placement;
				char m_ptsym;
				pathcode_t m_pathcode;
				altcode_t m_altcode;
				bool m_at_airport;
			};

			VFRRoute(const Glib::ustring& nm);
			VFRRoute(void);
			const Glib::ustring& get_name(void) const { return m_name; }
			void set_name(const Glib::ustring& x) { m_name = x; }
			int32_t get_minalt(void) const { return m_minalt; }
			void set_minalt(int32_t a = std::numeric_limits<int32_t>::min()) { m_minalt = a; }
			int32_t get_maxalt(void) const { return m_maxalt; }
			void set_maxalt(int32_t a = std::numeric_limits<int32_t>::max()) { m_maxalt = a; }
			unsigned int size(void) const { return m_pts.size(); }
			const VFRRoutePoint& operator[](unsigned int x) const { return m_pts[x]; }
			VFRRoutePoint& operator[](unsigned int x) { return m_pts[x]; }
			void add_point(const VFRRoutePoint& pt) { m_pts.push_back(pt); }
			void insert_point(const VFRRoutePoint& x, int index) { m_pts.insert(m_pts.begin() + index, x); }
			void erase_point(int index) { m_pts.erase(m_pts.begin() + index); }
			void reverse(void);
			void name(void);
			double compute_distance(void) const;

			template<class Archive> void hibernate(Archive& ar) {
				ar.io(m_name, "NAME", "name", hibernate_sqlnocase);
				ar.io(m_minalt, "MINALT", "minalt");
				ar.io(m_maxalt, "MAXALT", "maxalt");
				ar.io(m_pts, "airportvfrroutepoints", "ROUTEID", "VFRRoutePoint");
			}

			template<class Archive> void hibernate_binary(Archive& ar) {
				ar.io(m_name);
				ar.io(m_minalt);
				ar.io(m_maxalt);
				uint32_t sz(m_pts.size());
				ar.io(sz);
				if (ar.is_load())
					m_pts.resize(sz);
				for (std::vector<VFRRoutePoint>::iterator i(m_pts.begin()), e(m_pts.end()); i != e; ++i)
					i->hibernate_binary(ar);
			}

		protected:
			Glib::ustring m_name;
			std::vector<VFRRoutePoint> m_pts;
			int32_t m_minalt;
			int32_t m_maxalt;
		};

		class PolylineNode {
		public:
			typedef enum {
				feature_off = 0,                                // Nothing (can be used for a break in a chain of nodes without restarting a new chain)
				feature_solid_yellow_line = 1,                  // Solid yellow line (eg. taxiway centreline or taxiway edge line).
				feature_broken_yellow_line = 2,                 // Broken yellow line (used for taxiway-taxiway hold positions, AIM 2-3-6))
				feature_double_solid_yellow_line = 3,           // Double solid yellow line (eg. taxiway edge lines)
				feature_runway_hold_line = 4,                   // Runway hold lines (four yellow lines, two solid and two broken, solid line to right side along axis of chain)
				feature_other_hold_line = 5,                    // Other hold lines (one solid yellow and one broken, solid line to right along axis of chain) (used to define hold lines NOT related to runway, such as exits from aprons to active taxiways)
				feature_ils_hold_line = 6,                      // ILS hold lines (two yellow lines with perpendicular yellow bars between them.
				feature_yellow_line_with_broken_lines = 7,      // Yellow line with broken line on each side (used on taxiway centre lines for in the "ILS critical zone", on approach to a runway hold line).
				feature_widely_broken_single_yellow_line = 8,   // Widely separated broken, single yellow lines (used to mark "lanes" for queueing  aeroplanes at hold points, AIM 2-3-6 Fig 2-3-18)
				feature_widely_broken_double_yellow_line = 9,   // Widely separated broken, double yellow lines (used to mark "lanes" for queueing  aeroplanes at hold points, AIM 2-3-6 Fig 2-3-18)
				feature_solid_white_line = 20,                  // Solid white line (to mark roadways on aprons)
				feature_white_checkerboard = 21,                // White chequerboard (to mark roadways on aprons)
				feature_broken_white_line = 22,                 // Broken white line (to mark roadways on aprons)
				feature_solid_yellow_line_on_black = 51,        // Add 50 to the above codes to include a  black background, to improve visibility on light-coloured surfaces (such as concrete).
				feature_broken_yellow_line_on_black = 52,
				feature_double_solid_yellow_line_on_black = 53,
				feature_runway_hold_line_on_black = 54,
				feature_other_hold_line_on_black = 55,
				feature_ils_hold_line_on_black = 56,
				feature_yellow_line_with_broken_lines_on_black = 57,
				feature_widely_broken_single_yellow_line_on_black = 58,
				feature_widely_broken_double_yellow_line_on_black = 59,
				feature_green_embedded_lights = 101,            // Green embedded centreline lights - bidirectional along (both ways) the string axis.
				feature_blue_edge_lights = 102,                 // Blue taxiway edge lights, omnidirectional.
				feature_amber_embedded_lights = 103,            // Closely spaced Amber embedded lights, unidirectional, (one-way perpendicular to right of string axis), visible only to the left of the direction on the string (typically used for hold lines across taxiways).
				feature_amber_pulsating_embedded_lights = 104,  // Closely spaced Amber embedded lights, unidirectional & alternately pulsating (one-way perpendicular to right of  string axis) and pulsating alternately (used for some hold short lines across wide taxiways, visible only to an aeroplane approaching the hold line towards the runway entrance).  Will pulse at between 30 and 60 Hz, and always "on".
				feature_amber_green_embedded_lights = 105,      // Amber & green runway holding point embedded centreline lights:  Steady alternating amber and green lights, directional along both directions of the string axis.  Usually used approaching hold points in association with line type 7 in the "ILS critical zone" at the approach of a taxiway to a runway.
				feature_red_edge_lights = 106                   // Red boundary edge lights, omnidirectional (eg. to mark edges of taxiway bridges instead of blue edge lights)

			} feature_t;

			static const unsigned int nr_features = 2;

			static const uint16_t flag_bezier_prev = (1 << 0);
			static const uint16_t flag_bezier_next = (1 << 1);
			static const uint16_t flag_endpath = (1 << 2);
			static const uint16_t flag_closepath = (1 << 3);
			static const uint16_t flag_bezier_splitcontrol = (1 << 15); // only for DB

			PolylineNode(const Point& pt = Point(), const Point& pcpt = Point(), const Point& ncpt = Point(), uint16_t flags = 0, feature_t f1 = feature_off, feature_t f2 = feature_off);
			Point get_point(void) const { return m_point; }
			Point get_prev_controlpoint(void) const { return m_prev_controlpoint; }
			Point get_next_controlpoint(void) const { return m_next_controlpoint; }
			uint16_t get_flags(void) const { return m_flags; }
			feature_t get_feature(unsigned int idx) const { return m_feature[idx % nr_features]; }
			void set_point(const Point& pt) { m_point = pt; }
			void set_prev_controlpoint(const Point& pt) { m_prev_controlpoint = pt; }
			void set_next_controlpoint(const Point& pt) { m_next_controlpoint = pt; }
			void set_flags(uint16_t flg) { m_flags = flg; }
			void set_feature(unsigned int idx, feature_t ft) { m_feature[idx % nr_features] = ft; }
			bool is_bezier_prev(void) const { return !!(m_flags & flag_bezier_prev); }
			bool is_bezier_next(void) const { return !!(m_flags & flag_bezier_next); }
			bool is_bezier(void) const { return !!(m_flags & (flag_bezier_prev | flag_bezier_next)); }
			bool is_endpath(void) const { return !!(m_flags & flag_endpath); }
			bool is_closepath(void) const { return !!(m_flags & flag_closepath); }

			unsigned int blobsize(void) const;
			uint8_t *blobencode(uint8_t *ptr) const;
			const uint8_t *blobdecode(const uint8_t *ptr, uint8_t const *end);

			template<class Archive> void hibernate(Archive& ar) {
				ar.io(m_point, "LAT", "LON", "lat", "lon");
				ar.io(m_prev_controlpoint, "CPLAT", "CPLON", "cplat", "cplon");
				ar.io(m_next_controlpoint, "CNLAT", "CNLON", "cnlat", "cnlon");
				ar.io(m_flags, "FLAGS", "flags");
				ar.io(m_feature[0], "FEATURE0", "feature0");
				ar.io(m_feature[1], "FEATURE1", "feature1");
			}

			template<class Archive> void hibernate_binary(Archive& ar) {
				m_point.hibernate_binary(ar);
				m_prev_controlpoint.hibernate_binary(ar);
				m_next_controlpoint.hibernate_binary(ar);
				ar.io(m_flags);
				for (unsigned int i = 0; i < nr_features; ++i)
					ar.iouint8(m_feature[i]);
			}

		protected:
			Point m_point;
			Point m_prev_controlpoint;
			Point m_next_controlpoint;
			uint16_t m_flags;
			feature_t m_feature[nr_features];
		};

		class Polyline : public std::vector<PolylineNode> {
		public:
			static const uint16_t flag_area = (1 << 0);
			static const uint16_t flag_airportboundary = (1 << 1);

			Polyline(void) : m_flags(0) {}

			const Glib::ustring& get_name(void) const { return m_name; }
			void set_name(const Glib::ustring& s) { m_name = s; }
			const Glib::ustring& get_surface(void) const { return m_surface; }
			void set_surface(const Glib::ustring& s) { m_surface = s; }
			uint16_t get_flags(void) const { return m_flags; }
			void set_flags(uint16_t flg) { m_flags = flg; }
			bool is_area(void) const { return !!(m_flags & flag_area); }
			bool is_airportboundary(void) const { return !!(m_flags & flag_airportboundary); }
			bool is_concrete(void) const;

			unsigned int blobsize(void) const;
			uint8_t *blobencode(uint8_t *ptr) const;
			const uint8_t *blobdecode(const uint8_t *ptr, uint8_t const *end);

			void bindblob(sqlite3x::sqlite3_command& cmd, int index) const;
			void getblob(sqlite3x::sqlite3_cursor& cursor, int index);

			unsigned int blobsize_polygon(void) const;
			uint8_t *blobencode_polygon(uint8_t *ptr) const;
			const uint8_t *blobdecode_polygon(const uint8_t *ptr, uint8_t const *end);

			void bindblob_polygon(sqlite3x::sqlite3_command& cmd, int index) const;
			void getblob_polygon(sqlite3x::sqlite3_cursor& cursor, int index);

#ifdef HAVE_PQXX
			void bindblob_polygon(pqxx::binarystring& blob) const;
			void getblob_polygon(const pqxx::binarystring& blob);
#endif

			operator PolygonHole(void) const;

			Rect get_bbox(void) const;

			template<class Archive> void hibernate(Archive& ar) {
				ar.io(m_name, "NAME", "name");
				ar.io(m_surface, "SURFACE", "surface");
				ar.io(m_flags, "FLAGS", "flags");
				ar.io(*this, "POLY", "Node");
			}

			template<class Archive> void hibernate_binary(Archive& ar) {
				ar.io(m_name);
				ar.io(m_surface);
				ar.io(m_flags);
				uint32_t sz(size());
				ar.io(sz);
				if (ar.is_load())
					resize(sz);
				for (iterator i(begin()), e(end()); i != e; ++i)
					i->hibernate_binary(ar);
			}

		protected:
			Glib::ustring m_name;
			Glib::ustring m_surface;
			uint16_t m_flags;
		};

		class PointFeature {
		public:
			typedef enum {
				feature_invalid = 0,
				feature_vasi = 1,
				feature_taxiwaysign = 2,
				feature_startuploc = 3,
				feature_tower = 4,
				feature_lightbeacon = 5,
				feature_windsock = 6
			} feature_t;

			PointFeature(feature_t ft = feature_invalid, const Point& pt = Point(), uint16_t hdg = 0, int16_t elev = 0,
				     unsigned int subtype = 0, int attr1 = 0, int attr2 = 0, const Glib::ustring& name = "", const Glib::ustring& rwyident = "");

			feature_t get_feature(void) const { return m_feature; }
			void set_feature(feature_t x) { m_feature = x; }
			Point get_coord(void) const { return m_coord; }
			void set_coord(const Point& pt) { m_coord = pt; }
			uint16_t get_hdg(void) const { return m_hdg; }
			void set_hdg(uint16_t x) { m_hdg = x; }
			int16_t get_elev(void) const { return m_elev; }
			void set_elev(int16_t x) { m_elev = x; }
			unsigned int get_subtype(void) const { return m_subtype; }
			void set_subtype(unsigned int x) { m_subtype = x; }
			int get_attr1(void) const { return m_attr1; }
			void set_attr1(int x) { m_attr1 = x; }
			int get_attr2(void) const { return m_attr2; }
			void set_attr2(int x) { m_attr2 = x; }
			const Glib::ustring& get_name(void) const { return m_name; }
			void set_name(const Glib::ustring& x) { m_name = x; }
			const Glib::ustring& get_rwyident(void) const { return m_rwyident; }
			void set_rwyident(const Glib::ustring& x) { m_rwyident = x; }

			template<class Archive> void hibernate(Archive& ar) {
				ar.io(m_feature, "FEATURE", "feature");
				ar.io(m_coord, "LAT", "LON", "lat", "lon");
				ar.io(m_hdg, "HDG", "hdg");
				ar.io(m_elev, "ELEV", "elev");
				ar.io(m_subtype, "SUBTYPE", "subtype");
				ar.io(m_attr1, "ATTR1", "attr1");
				ar.io(m_attr2, "ATTR2", "attr2");
				ar.io(m_name, "NAME", "name");
				ar.io(m_rwyident, "RWYIDENT", "rwyident");
			}

			template<class Archive> void hibernate_binary(Archive& ar) {
				ar.iouint8(m_feature);
				m_coord.hibernate_binary(ar);
				ar.io(m_hdg);
				ar.io(m_elev);
				ar.iouint32(m_subtype);
				ar.ioint32(m_attr1);
				ar.ioint32(m_attr2);
				ar.io(m_name);
				ar.io(m_rwyident);
			}

		protected:
			feature_t m_feature;
			Point m_coord;
			uint16_t m_hdg;
			int16_t m_elev;
			unsigned int m_subtype;
			int m_attr1;
			int m_attr2;
			Glib::ustring m_name;
			Glib::ustring m_rwyident;
		};

		class MinimalFAS {
		public:
			static const uint8_t operationtype_straightin = 0;
			static const uint8_t sbasprovider_gbasonly    = 14;
			static const uint8_t sbasprovider_anysbas     = 15;
			static const uint8_t runwaynumber_heliport    = 0;

			MinimalFAS();
			bool is_valid(void) const { return !m_ftp.is_invalid(); }
			uint8_t get_optyp(void) const { return m_operationtype; }
			void set_optyp(uint8_t ot) { m_operationtype = ot; }
			uint8_t get_rwy(void) const { return m_runway; }
			void set_rwy(uint8_t rwy) { m_runway = rwy; }
			uint8_t get_rte(void) const { return m_route; }
			void set_rte(uint8_t rte) { m_route = rte; }
			uint8_t get_operationtype(void) const { return m_operationtype & 0x0F; }
			void set_operationtype(uint8_t ot) { m_operationtype ^= (m_operationtype ^ ot) & 0x0F; }
			uint8_t get_sbasprovider(void) const { return (m_operationtype & 0xF0) >> 4; }
			void set_sbasprovider(uint8_t sp) { m_operationtype ^= (m_operationtype ^ (sp << 4)) & 0xF0; }
			uint8_t get_runwaynumber(void) const { return m_runway & 0x3F; }
			void set_runwaynumber(uint8_t rn) { m_runway ^= (m_runway ^ rn) & 0x3F; }
			uint8_t get_runwayletter(void) const { return (m_runway & 0xC0) >> 6; }
			void set_runwayletter(uint8_t rl) { m_runway ^= (m_runway ^ (rl << 6)) & 0xC0; }
			char get_runwayletter_char(void) const;
			void set_runwayletter_char(char l);
			uint8_t get_approachperformanceindicator(void) const { return m_route & 0x07; }
			void set_approachperformanceindicator(uint8_t api) { m_route ^= (m_route ^ api) & 0x07; }
			uint8_t get_routeindicator(void) const { return (m_route & 0xF8) >> 3; }
			void set_routeindicator(uint8_t ri) { m_route ^= (m_route ^ (ri << 3)) & 0xF8; }
			char get_routeindicator_char(void) const;
			void set_routeindicator_char(char ri);
			uint8_t get_referencepathdataselector(void) const { return m_refpathdatasel; }
			void set_referencepathdataselector(uint8_t rpds) { m_refpathdatasel = rpds; }
			Glib::ustring get_referencepathident(void) const;
			void set_referencepathident(const Glib::ustring& rid);
			const Point& get_ftp(void) const { return m_ftp; }
			void set_ftp(const Point& pt) { m_ftp = pt; }
			int16_t get_ftp_height(void) const { return m_ftpheight; }
			void set_ftp_height(int16_t h) { m_ftpheight = h; }
			double get_ftp_height_meter(void) const { return get_ftp_height() * 0.1 - 512; }
			void set_ftp_height_meter(double h) { set_ftp_height(Point::round<int16_t,double>((h + 512) * 10)); }
			const Point& get_fpap(void) const { return m_fpap; }
			void set_fpap(const Point& pt) { m_fpap = pt; }
			uint16_t get_approach_tch(void) const { return m_approachtch; }
			void set_approach_tch(uint16_t atch) { m_approachtch = atch; }
			double get_approach_tch_meter(void) const;
			void set_approach_tch_meter(double atch);
			double get_approach_tch_ft(void) const;
			void set_approach_tch_ft(double atch);
			uint16_t get_glidepathangle(void) const { return m_glidepathangle; }
			void set_glidepathangle(uint16_t a) { m_glidepathangle = a; }
			double get_glidepathangle_deg(void) const { return get_glidepathangle() * 0.01; }
			void set_glidepathangle_deg(double a) { set_glidepathangle(Point::round<uint16_t,double>(a * 100)); }
			uint8_t get_coursewidth(void) const { return m_coursewidth; }
			void set_coursewidth(uint8_t w) { m_coursewidth = w; }
			double get_coursewidth_meter(void) const { return get_coursewidth() * 0.25 + 80; }
			void set_coursewidth_meter(double w) { set_coursewidth(Point::round<uint8_t,double>((w - 80) * 4)); }
			uint8_t get_dlengthoffset(void) const { return m_dlengthoffset; }
			void set_dlengthoffset(uint8_t l) { m_dlengthoffset = l; }
			double get_dlengthoffset_meter(void) const;
			void set_dlengthoffset_meter(double l);
			uint8_t get_hal(void) const { return m_hal; }
			void set_hal(uint8_t h) { m_hal = h; }
			double get_hal_meter(void) const { return get_hal() * 0.2; }
			void set_hal_meter(double h) { set_hal(Point::round<uint8_t,double>(h * 5)); }
			uint8_t get_val(void) const { return m_val; }
			void set_val(uint8_t v) { m_val = v; }
			double get_val_meter(void) const { return get_val() * 0.2; }
			void set_val_meter(double v) { set_val(Point::round<uint8_t,double>(v * 5)); }
			FASNavigate navigate(void) const;

			template<class Archive> void hibernate(Archive& ar) {
				Glib::ustring pathid;
				if (ar.is_save() || ar.is_schema())
					pathid = get_referencepathident();
				ar.io(pathid, "REFPATHID", "refpathid");
				if (ar.is_load())
					set_referencepathident(pathid);
				ar.io(m_ftp, "FTPLAT", "FTPLON", "ftplat", "ftplon");
				ar.io(m_fpap, "FPAPLAT", "FPAPLON", "fpaplat", "fpaplon");
				ar.io(m_operationtype, "OPTYP", "optyp");
				ar.io(m_runway, "RWY", "rwy");
				ar.io(m_route, "RTE", "RTE");
				ar.io(m_refpathdatasel, "REFPATHDS", "refpathds");
				ar.io(m_ftpheight, "FTPHEIGHT", "ftpheight");
				ar.io(m_approachtch, "APCHTCH", "apchtch");
				ar.io(m_glidepathangle, "GPA", "gpa");
				ar.io(m_coursewidth, "CWIDTH", "cwidth");
				ar.io(m_dlengthoffset, "DLENOFFS", "dlenoffs");
				ar.io(m_hal, "HAL", "hal");
				ar.io(m_val, "VAL", "val");
			}

			template<class Archive> void hibernate_binary(Archive& ar) {
				ar.io(m_operationtype);
				ar.io(m_runway);
				ar.io(m_route);
				ar.io(m_refpathdatasel);
				Glib::ustring pathid;
				if (ar.is_save())
					pathid = get_referencepathident();
				ar.io(pathid);
				if (ar.is_load())
					set_referencepathident(pathid);
				m_ftp.hibernate_binary(ar);
				ar.io(m_ftpheight);
				m_fpap.hibernate_binary(ar);
				ar.io(m_approachtch);
				ar.io(m_glidepathangle);
				ar.io(m_coursewidth);
				ar.io(m_dlengthoffset);
				ar.io(m_hal);
				ar.io(m_val);
			}

		protected:
			static constexpr double from_coord = 90*60*60/0.0005/(1 << 30);
			static constexpr double to_coord = (1 << 30)*0.0005/(90*60*60);
			uint8_t m_operationtype;
			uint8_t m_runway;
			uint8_t m_route;
			uint8_t m_refpathdatasel;
			uint8_t m_refpathident[4];
			Point m_ftp;
			int16_t m_ftpheight;
			Point m_fpap;
			uint16_t m_approachtch;
			uint16_t m_glidepathangle;
			uint8_t m_coursewidth;
			uint8_t m_dlengthoffset;
			uint8_t m_hal;
			uint8_t m_val;
		};

		class FinalApproachSegment : public MinimalFAS {
		public:
			FinalApproachSegment();

			Glib::ustring get_airportid(void) const;
			void set_airportid(const Glib::ustring& arptid);

			static const uint32_t crc_poly = (1U << 31) | (1U << 24) | (1U << 22) | (1U << 16) | (1U << 14) |
														    (1U << 8) | (1U << 7) | (1U << 5) | (1U << 3) | (1U << 1) | (1U << 0);
			static const uint32_t crc_rev_poly = (1U << (31-31)) | (1U << (31-24)) | (1U << (31-22)) | (1U << (31-16)) | (1U << (31-14)) |
																	    (1U << (31-8)) | (1U << (31-7)) | (1U << (31-5)) | (1U << (31-3)) | (1U << (31-1)) | (1U << (31-0));
			static uint32_t crc(const uint8_t *data, unsigned int length, uint32_t initial = 0U);
			typedef uint8_t fas_datablock_t[40];
			bool decode(const fas_datablock_t data);
			void encode(fas_datablock_t data);
			FASNavigate navigate(void) const;

		protected:
			static const uint32_t crc_table[256];
			uint8_t m_airportid[4];
		};

		class FASNavigate {
		public:
			static constexpr double to_deg = 90.0 / (1 << 30);
			static constexpr double from_deg = (1 << 30) / 90.0;
			static constexpr double to_rad = M_PI_2 / (1 << 30);
			static constexpr double from_rad = (1 << 30) / M_PI_2;
			static const uint32_t standard_glidepath = 3.0 * (1 << 30) / 90.0;
			static const uint32_t standard_glidepathdefl = 0.7 * (1 << 30) / 90.0;
			static const uint32_t standard_coursedefl = 2.5 * (1 << 30) / 90.0;

			FASNavigate(void);

			bool is_valid(void) const { return !m_ftp.is_invalid() && !m_fpap.is_invalid() && !std::isnan(m_thralt); }
			const Glib::ustring& get_airport(void) const { return m_airport; }
			void set_airport(const Glib::ustring& arpt) { m_airport = arpt; }
			const Glib::ustring& get_ident(void) const { return m_ident; }
			void set_ident(const Glib::ustring& id) { m_ident = id; }
			const Point get_ftp(void) const { return m_ftp; }
			const Point get_fpap(void) const { return m_fpap; }
			uint32_t get_course(void) const { return m_course; }
			double get_course_deg(void) const { return m_course * to_deg; }
			double get_course_rad(void) const { return m_course * to_rad; }
			void set_course(const Point& ftp, const Point& fpap, double fpapoffset_nmi, double cwidth_meter);
			uint32_t get_coursedefl(void) const { return m_coursedefl; }
			void set_coursedefl(uint32_t cd) { m_coursedefl = cd; }
			double get_coursedefl_deg(void) const { return m_coursedefl * to_deg; }
			void set_coursedefl_deg(double cd) { m_coursedefl = cd * from_deg; }
			double get_coursedefl_rad(void) const { return m_coursedefl * to_rad; }
			void set_coursedefl_rad(double cd) { m_coursedefl = cd * from_rad; }
			uint32_t get_glidepath(void) const { return m_glidepath; }
			void set_glidepath(uint32_t gp) { m_glidepath = gp; }
			double get_glidepath_deg(void) const { return m_glidepath * to_deg; }
			void set_glidepath_deg(double gp) { m_glidepath = gp * from_deg; }
			double get_glidepath_rad(void) const { return m_glidepath * to_rad; }
			void set_glidepath_rad(double gp) { m_glidepath = gp * from_rad; }
			uint32_t get_glidepathdefl(void) const { return m_glidepathdefl; }
			void set_glidepathdefl(uint32_t gpd) { m_glidepathdefl = gpd; }
			double get_glidepathdefl_deg(void) const { return m_glidepathdefl * to_deg; }
			void set_glidepathdefl_deg(double gpd) { m_glidepathdefl = gpd * from_deg; }
			double get_glidepathdefl_rad(void) const { return m_glidepathdefl * to_rad; }
			void set_glidepathdefl_rad(double gpd) { m_glidepathdefl = gpd * from_rad; }
			double get_thralt(void) const { return m_thralt; }
			void set_thralt(double ta) { m_thralt = ta; }
			void navigate(const Point& pos, double truealt, double& loc, double& gs, bool& fromflag);

		protected:
			Glib::ustring m_airport;
			Glib::ustring m_ident;
			Point m_ftp;
			Point m_fpap;
			uint32_t m_course;
			uint32_t m_coursedefl;
			uint32_t m_glidepath;
			uint32_t m_glidepathdefl;
			double m_thralt;
		};

		static const char *db_query_string;
		static const char *db_aux_query_string;
		static const char *db_text_fields[];
		static const char *db_time_fields[];

		static const uint8_t flightrules_arr_vfr      = 1 << 0;
		static const uint8_t flightrules_arr_ifr      = 1 << 1;
		static const uint8_t flightrules_dep_vfr      = 1 << 2;
		static const uint8_t flightrules_dep_ifr      = 1 << 3;

		Airport(void);
		void load(sqlite3x::sqlite3_cursor& cursor, sqlite3x::sqlite3_connection & db, unsigned int loadsubtables = subtables_all);
		void load_subtables(sqlite3x::sqlite3_connection& db, unsigned int loadsubtables = subtables_all);
		unsigned int get_subtables(void) const { return m_subtables; }
		bool is_subtables_loaded(void) const { return !(subtables_all & ~m_subtables); }
		void save(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
		void erase(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
		void update_index(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
#ifdef HAVE_PQXX
		void load(const pqxx::tuple& cursor, pqxx::basic_transaction& w, unsigned int loadsubtables = subtables_all);
		void load_subtables(pqxx::basic_transaction& w, unsigned int loadsubtables = subtables_all);
		void save(pqxx::connection_base& conn, bool rtree);
		void erase(pqxx::connection_base& conn, bool rtree);
		void update_index(pqxx::connection_base& conn, bool rtree);
#endif
		Rect get_bbox(void) const { return m_bbox; }
		void set_coord(const Point& pt) { DbElementLabelSourceCoordBase::set_coord(pt); recompute_bbox(); }
		const Glib::ustring& get_vfrrmk(void) const { return m_vfrrmk; }
		void set_vfrrmk(const Glib::ustring& vr) { m_vfrrmk = vr; }
		int16_t get_elevation(void) const { return m_elev; }
		void set_elevation(int16_t elev) { m_elev = elev; }
		char get_typecode(void) const { return m_typecode; }
		void set_typecode(char t) { m_typecode = t; }
		static const Glib::ustring& get_type_string(char t);
		const Glib::ustring& get_type_string(void) const { return get_type_string(m_typecode); }
		bool is_valid(void) const { return !!m_typecode; }
		uint8_t get_flightrules(void) const { return m_flightrules; }
		void set_flightrules(uint8_t fr = 0) { m_flightrules = fr; }
		static bool is_fpl_zzzz(const Glib::ustring& icao);
		static bool is_fpl_afil(const Glib::ustring& icao);
		static bool is_fpl_zzzz_or_afil(const Glib::ustring& icao);
		bool is_fpl_zzzz(void) const { return is_fpl_zzzz(get_icao()); }
		unsigned int get_nrrwy(void) const { return m_rwy.size(); }
		const Runway& get_rwy(unsigned int x) const { return m_rwy[x]; }
		Runway& get_rwy(unsigned int x) { return m_rwy[x]; }
		unsigned int get_nrhelipads(void) const { return m_helipad.size(); }
		const Helipad& get_helipad(unsigned int x) const { return m_helipad[x]; }
		Helipad& get_helipad(unsigned int x) { return m_helipad[x]; }
		unsigned int get_nrcomm(void) const { return m_comm.size(); }
		const Comm& get_comm(unsigned int x) const { return m_comm[x]; }
		Comm& get_comm(unsigned int x) { return m_comm[x]; }
		unsigned int get_nrvfrrte(void) const { return m_vfrrte.size(); }
		const VFRRoute& get_vfrrte(unsigned int x) const { return m_vfrrte[x]; }
		VFRRoute& get_vfrrte(unsigned int x) { return m_vfrrte[x]; }
		unsigned int get_nrlinefeatures(void) const { return m_linefeature.size(); }
		const Polyline& get_linefeature(unsigned int x) const { return m_linefeature[x]; }
		Polyline& get_linefeature(unsigned int x) { return m_linefeature[x]; }
		unsigned int get_nrpointfeatures(void) const { return m_pointfeature.size(); }
		const PointFeature& get_pointfeature(unsigned int x) const { return m_pointfeature[x]; }
		PointFeature& get_pointfeature(unsigned int x) { return m_pointfeature[x]; }
		unsigned int get_nrfinalapproachsegments(void) const { return m_fas.size(); }
		const MinimalFAS& get_finalapproachsegment(unsigned int x) const { return m_fas[x]; }
		MinimalFAS& get_finalapproachsegment(unsigned int x) { return m_fas[x]; }
		void add_rwy(const Runway& x) { m_rwy.push_back(x); recompute_bbox(); }
		void insert_rwy(const Runway& x, int index) { m_rwy.insert(m_rwy.begin() + index, x); recompute_bbox(); }
		void erase_rwy(int index) { m_rwy.erase(m_rwy.begin() + index); recompute_bbox(); }
		void add_helipad(const Helipad& x) { m_helipad.push_back(x); recompute_bbox(); }
		void insert_helipad(const Helipad& x, int index) { m_helipad.insert(m_helipad.begin() + index, x); recompute_bbox(); }
		void erase_helipad(int index) { m_helipad.erase(m_helipad.begin() + index); recompute_bbox(); }
		void add_comm(const Comm& x) { m_comm.push_back(x); }
		void insert_comm(const Comm& x, int index) { m_comm.insert(m_comm.begin() + index, x); }
		void erase_comm(int index) { m_comm.erase(m_comm.begin() + index); }
		void add_vfrrte(const VFRRoute& x) { m_vfrrte.push_back(x); recompute_bbox(); }
		void insert_vfrrte(const VFRRoute& x, int index) { m_vfrrte.insert(m_vfrrte.begin() + index, x); recompute_bbox(); }
		void erase_vfrrte(int index) { m_vfrrte.erase(m_vfrrte.begin() + index); recompute_bbox(); }
		void add_linefeature(const Polyline& x) { m_linefeature.push_back(x); recompute_bbox(); }
		void insert_linefeature(const Polyline& x, int index) { m_linefeature.insert(m_linefeature.begin() + index, x); recompute_bbox(); }
		void erase_linefeature(int index) { m_linefeature.erase(m_linefeature.begin() + index); recompute_bbox(); }
		void add_pointfeature(const PointFeature& x) { m_pointfeature.push_back(x); recompute_bbox(); }
		void insert_pointfeature(const PointFeature& x, int index) { m_pointfeature.insert(m_pointfeature.begin() + index, x); recompute_bbox(); }
		void erase_pointfeature(int index) { m_pointfeature.erase(m_pointfeature.begin() + index); recompute_bbox(); }
		void add_finalapproachsegment(const MinimalFAS& x) { m_fas.push_back(x); }
		void insert_finalapproachsegment(const MinimalFAS& x, int index) { m_fas.insert(m_fas.begin() + index, x); }
		void erase_finalapproachsegment(int index) { m_fas.erase(m_fas.begin() + index); }
		void recompute_bbox(void);
		FinalApproachSegment operator()(const MinimalFAS& fas) const;

		template<class Archive> void hibernate(Archive& ar) {
			DbElementLabelSourceCoordIcaoNameAuthorityBase::hibernate<Archive>(ar);
			ar.io(m_rwy, "airportrunways", "ARPTID", "Runway", subtables_runways);
			ar.io(m_helipad, "airporthelipads", "ARPTID", "Helipad", subtables_helipads);
			ar.io(m_comm, "airportcomms", "ARPTID", "Comm", subtables_comms);
			ar.io(m_vfrrte, "airportvfrroutes", "ARPTID", "VFRRoute", subtables_vfrroutes);
			ar.io(m_linefeature, "airportlinefeatures", "ARPTID", "LineFeature", subtables_linefeatures);
			ar.io(m_pointfeature, "airportpointfeatures", "ARPTID", "PointFeature", subtables_pointfeatures);
			ar.io(m_fas, "airportfas", "ARPTID", "FinalApproachSegment", subtables_fas);
			ar.io(m_vfrrmk, "VFRRMK", "vfrremark");
			ar.io(m_elev, "ELEVATION", "elev");
			ar.io(m_typecode, "TYPECODE", "type");
			ar.io(m_flightrules, "FRULES", "frules");
			Rect bbox(get_bbox());
			ar.io(bbox, hibernate_sqlindex);
			if (ar.is_save() || ar.is_schema()) {
				tile_t tile(TileNumber::to_tilenumber(bbox));
				ar.io(tile, "TILE", 0, hibernate_sqlindex);
			}
		}

		template<class Archive> void hibernate_binary(Archive& ar) {
			DbElementLabelSourceCoordIcaoNameAuthorityBase::hibernate_binary(ar);
			{
				uint32_t sz(m_rwy.size());
				ar.io(sz);
				if (ar.is_load())
					m_rwy.resize(sz);
				for (std::vector<Runway>::iterator i(m_rwy.begin()), e(m_rwy.end()); i != e; ++i)
					i->hibernate_binary(ar);
			}
			{
				uint32_t sz(m_helipad.size());
				ar.io(sz);
				if (ar.is_load())
					m_helipad.resize(sz);
				for (std::vector<Helipad>::iterator i(m_helipad.begin()), e(m_helipad.end()); i != e; ++i)
					i->hibernate_binary(ar);
			}
			{
				uint32_t sz(m_comm.size());
				ar.io(sz);
				if (ar.is_load())
					m_comm.resize(sz);
				for (std::vector<Comm>::iterator i(m_comm.begin()), e(m_comm.end()); i != e; ++i)
					i->hibernate_binary(ar);
			}
			{
				uint32_t sz(m_vfrrte.size());
				ar.io(sz);
				if (ar.is_load())
					m_vfrrte.resize(sz);
				for (std::vector<VFRRoute>::iterator i(m_vfrrte.begin()), e(m_vfrrte.end()); i != e; ++i)
					i->hibernate_binary(ar);
			}
			{
				uint32_t sz(m_linefeature.size());
				ar.io(sz);
				if (ar.is_load())
					m_linefeature.resize(sz);
				for (std::vector<Polyline>::iterator i(m_linefeature.begin()), e(m_linefeature.end()); i != e; ++i)
					i->hibernate_binary(ar);
			}
			{
				uint32_t sz(m_pointfeature.size());
				ar.io(sz);
				if (ar.is_load())
					m_pointfeature.resize(sz);
				for (std::vector<PointFeature>::iterator i(m_pointfeature.begin()), e(m_pointfeature.end()); i != e; ++i)
					i->hibernate_binary(ar);
			}
			{
				uint32_t sz(m_fas.size());
				ar.io(sz);
				if (ar.is_load())
					m_fas.resize(sz);
				for (std::vector<MinimalFAS>::iterator i(m_fas.begin()), e(m_fas.end()); i != e; ++i)
					i->hibernate_binary(ar);
			}
			ar.io(m_vfrrmk);
			ar.io(m_elev);
			ar.io(m_typecode);
			ar.io(m_flightrules);
			ar.iouint32(m_subtables);
			//m_bbox.hibernate_binary(ar);
			if (ar.is_load())
				recompute_bbox();
		}

	protected:
		std::vector<Runway> m_rwy;
		std::vector<Helipad> m_helipad;
		std::vector<Comm> m_comm;
		std::vector<VFRRoute> m_vfrrte;
		std::vector<Polyline> m_linefeature;
		std::vector<PointFeature> m_pointfeature;
		std::vector<MinimalFAS> m_fas;
		Glib::ustring m_vfrrmk;
		Rect m_bbox;
		int16_t m_elev;
		char m_typecode;
		uint8_t m_flightrules;
		unsigned int m_subtables;
        };

        class Airspace : public DbElementLabelSourceCoordIcaoNameBase {
	public:
		typedef ::DbBaseElements::TileNumber TileNumber;
		typedef TileNumber::tile_t tile_t;
		typedef TileNumber::tilerange_t tilerange_t;
		typedef DbElementLabelSourceBase::label_placement_t label_placement_t;

		static const uint8_t altflag_agl   = (1 << 0);
		static const uint8_t altflag_fl    = (1 << 1);
		static const uint8_t altflag_sfc   = (1 << 2);
		static const uint8_t altflag_gnd   = (1 << 3);
		static const uint8_t altflag_unlim = (1 << 4);
		static const uint8_t altflag_notam = (1 << 5);
		static const uint8_t altflag_unkn  = (1 << 6);

		static const uint8_t typecode_specialuse   = 255;
		static const uint8_t typecode_ead          = 254;

		static const char bdryclass_ead_a          = 'A';
		static const char bdryclass_ead_adiz       = 'Z';
		static const char bdryclass_ead_ama        = 'M';
		static const char bdryclass_ead_arinc      = 'y';
		static const char bdryclass_ead_asr        = 'a';
		static const char bdryclass_ead_awy        = 'Y';
		static const char bdryclass_ead_cba        = 'B';
		static const char bdryclass_ead_class      = 'L';
		static const char bdryclass_ead_cta        = 'G';
		static const char bdryclass_ead_cta_p      = 'g';
		static const char bdryclass_ead_ctr        = 'C';
		static const char bdryclass_ead_ctr_p      = 'c';
		static const char bdryclass_ead_d_amc      = 'd';
		static const char bdryclass_ead_d          = 'D';
		static const char bdryclass_ead_d_other    = 'z';
		static const char bdryclass_ead_ead        = 'E';
		static const char bdryclass_ead_fir        = 'F';
		static const char bdryclass_ead_fir_p      = 'f';
		static const char bdryclass_ead_nas        = 'N';
		static const char bdryclass_ead_no_fir     = 'n';
		static const char bdryclass_ead_oca        = 'O';
		static const char bdryclass_ead_oca_p      = 'o';
		static const char bdryclass_ead_ota        = 'm';
		static const char bdryclass_ead_part       = 'p';
		static const char bdryclass_ead_p          = 'P';
		static const char bdryclass_ead_political  = 'Q';
		static const char bdryclass_ead_protect    = 'q';
		static const char bdryclass_ead_r_amc      = 'r';
		static const char bdryclass_ead_ras        = 'V';
		static const char bdryclass_ead_rca        = 'v';
		static const char bdryclass_ead_r          = 'R';
		static const char bdryclass_ead_sector_c   = 's';
		static const char bdryclass_ead_sector     = 'S';
		static const char bdryclass_ead_tma        = 'T';
		static const char bdryclass_ead_tma_p      = 't';
		static const char bdryclass_ead_tra        = 'x';
		static const char bdryclass_ead_tsa        = 'X';
		static const char bdryclass_ead_uir        = 'U';
		static const char bdryclass_ead_uir_p      = 'u';
		static const char bdryclass_ead_uta        = 'w';
		static const char bdryclass_ead_w          = 'W';

		static const unsigned int subtables_none = 0;
		static const unsigned int subtables_segments = (1 << 0);
		static const unsigned int subtables_components = (1 << 1);
		static const unsigned int subtables_all = subtables_segments | subtables_components;

		class Segment {
		public:
			Segment(void);
			Segment(const Point& coord1, const Point& coord2, const Point& coord0, char shape, int32_t r1, int32_t r2);
			Point get_coord1(void) const { return m_coord1; }
			void set_coord1(const Point& pt) { m_coord1 = pt; }
			Point get_coord2(void) const { return m_coord2; }
			void set_coord2(const Point& pt) { m_coord2 = pt; }
			Point get_coord0(void) const { return m_coord0; }
			void set_coord0(const Point& pt) { m_coord0 = pt; }
			char get_shape(void) const { return m_shape; }
			void set_shape(char shp) { m_shape = shp; }
			int32_t get_radius1(void) const { return m_radius1; }
			void set_radius1(int32_t r = 0) { m_radius1 = r; }
			int32_t get_radius2(void) const { return m_radius2; }
			void set_radius2(int32_t r = 0) { m_radius2 = r; }
			bool is_valid(void) const { return !!m_shape; }

			template<class Archive> void hibernate(Archive& ar) {
				ar.io(m_coord1, "LAT1", "LON1", "c1lat", "c1lon");
				ar.io(m_coord2, "LAT2", "LON2", "c2lat", "c2lon");
				ar.io(m_coord0, "LAT0", "LON0", "c0lat", "c0lon");
				ar.io(m_radius1, "RADIUS1", "radius1");
				ar.io(m_radius2, "RADIUS2", "radius2");
				ar.io(m_shape, "SHAPE", "shape");
			}

			template<class Archive> void hibernate_binary(Archive& ar) {
				m_coord1.hibernate_binary(ar);
				m_coord2.hibernate_binary(ar);
				m_coord0.hibernate_binary(ar);
				ar.io(m_radius1);
				ar.io(m_radius2);
				ar.io(m_shape);
			}

		private:
			Point m_coord1;
			Point m_coord2;
			Point m_coord0;
			int32_t m_radius1;
			int32_t m_radius2;
			char m_shape;
		};

		class Component {
		public:
			typedef enum {
				operator_set,
				operator_union,
				operator_subtract,
				operator_intersect
			} operator_t;

			Component(const Glib::ustring& icao = "", uint8_t tc = typecode_ead,
				  char bc = 0, operator_t opr = operator_set);
			const Glib::ustring& get_icao(void) const { return m_icao; }
			void set_icao(const Glib::ustring& t) { m_icao = t; }
			char get_bdryclass(void) const { return m_bdryclass; }
			void set_bdryclass(char bclass) { m_bdryclass = bclass; }
			uint8_t get_typecode(void) const { return m_typecode; }
			void set_typecode(uint8_t tc) { m_typecode = tc; }
			const Glib::ustring& get_class_string(void) const { return Airspace::get_class_string(m_bdryclass, m_typecode); }
			operator_t get_operator(void) const { return m_operator; }
			void set_operator(operator_t opr = operator_set) { m_operator = opr; }
			static const Glib::ustring& get_operator_string(operator_t opr);
			const Glib::ustring& get_operator_string(void) const { return get_operator_string(get_operator()); }
			void set_operator_string(const Glib::ustring& oprstr);
			bool is_valid(void) const { return !!m_bdryclass; }

			template<class Archive> void hibernate(Archive& ar) {
				ar.io(m_icao, "ICAO", "icao", hibernate_sqlnocase);
				ar.io(m_typecode, m_bdryclass, "TYPECODE", "CLASS", "class");
				ar.io(m_operator, "OPERATOR", "operator");
			}

			template<class Archive> void hibernate_binary(Archive& ar) {
				ar.io(m_icao);
				ar.io(m_typecode);
				ar.io(m_bdryclass);
				ar.iouint8(m_operator);
			}

		private:
			Glib::ustring m_icao;
			uint8_t m_typecode;
			char m_bdryclass;
			operator_t m_operator;
		};

		struct IcaoRegion {
			struct Country {
				const char *name;
				const char *ident;
			};

			const char *region;
			const Country *countries;
		};

		static const char *db_query_string;
		static const char *db_aux_query_string;
		static const char *db_text_fields[];
		static const char *db_time_fields[];
		static const IcaoRegion icao_regions[];

		typedef int32_t gndelev_t;

		Airspace(void);
		void load(sqlite3x::sqlite3_cursor& cursor, sqlite3x::sqlite3_connection& db, unsigned int loadsubtables = subtables_all);
		void load_subtables(sqlite3x::sqlite3_connection& db, unsigned int loadsubtables = subtables_all);
		unsigned int get_subtables(void) const { return m_subtables; }
		bool is_subtables_loaded(void) const { return !(subtables_all & ~m_subtables); }
		void save(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
		void erase(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
		void update_index(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
#ifdef HAVE_PQXX
		void load(const pqxx::tuple& cursor, pqxx::basic_transaction& w, unsigned int loadsubtables = subtables_all);
		void load_subtables(pqxx::basic_transaction& w, unsigned int loadsubtables = subtables_all);
		void save(pqxx::connection_base& conn, bool rtree);
		void erase(pqxx::connection_base& conn, bool rtree);
		void update_index(pqxx::connection_base& conn, bool rtree);
#endif
		Rect get_bbox(void) const { return m_bbox; }
		Point get_labelcoord(void) const { return get_coord(); }
		void set_labelcoord(const Point& pt) { set_coord(pt); recompute_bbox(); }
		const Glib::ustring& get_ident(void) const { return m_ident; }
		void set_ident(const Glib::ustring& nm) { m_ident = nm; }
		const Glib::ustring& get_classexcept(void) const { return m_classexcept; }
		void set_classexcept(const Glib::ustring& s) { m_classexcept = s; }
		const Glib::ustring& get_efftime(void) const { return m_efftime; }
		void set_efftime(const Glib::ustring& s) { m_efftime = s; }
		const Glib::ustring& get_note(void) const { return m_note; }
		void set_note(const Glib::ustring& s) { m_note = s; }
		const Glib::ustring& get_commname(void) const { return m_commname; }
		void set_commname(const Glib::ustring& s) { m_commname = s; }
		uint32_t get_commfreq(unsigned int x = 0) const { if (x >= 2) return 0; return m_commfreq[x]; }
		void set_commfreq(unsigned int x = 0, uint32_t f = 0) { if (x < 2) m_commfreq[x] = f; }
		char get_bdryclass(void) const { return m_bdryclass; }
		void set_bdryclass(char bclass) { m_bdryclass = bclass; }
		uint8_t get_typecode(void) const { return m_typecode; }
		void set_typecode(uint8_t tc) { m_typecode = tc; }
		gndelev_t get_gndelevmin(void) const { return m_gndelevmin; }
		void set_gndelevmin(gndelev_t e) { m_gndelevmin = e; }
		gndelev_t get_gndelevmax(void) const { return m_gndelevmax; }
		void set_gndelevmax(gndelev_t e) { m_gndelevmax = e; }
		int32_t get_altlwr(void) const { return m_altlwr; }
		void set_altlwr(int32_t a) { m_altlwr = a; }
		int32_t get_altupr(void) const { return m_altupr; }
		void set_altupr(int32_t a) { m_altupr = a; }
		uint8_t get_altlwrflags(void) const { return m_altlwrflags; }
		void set_altlwrflags(uint8_t af) { m_altlwrflags = af; }
		uint8_t get_altuprflags(void) const { return m_altuprflags; }
		void set_altuprflags(uint8_t af) { m_altuprflags = af; }
		int32_t get_altlwr_corr(void) const;
		int32_t get_altupr_corr(void) const;
		Glib::ustring get_altlwr_string(void) const;
		Glib::ustring get_altupr_string(void) const;
		static Gdk::Color get_color(char bclass, uint8_t tc);
		Gdk::Color get_color(void) const { return get_color(m_bdryclass, m_typecode); }
		static const Glib::ustring& get_class_string(char bc, uint8_t tc = 0);
		const Glib::ustring& get_class_string(void) const { return get_class_string(m_bdryclass, m_typecode); }
		static bool set_class_string(uint8_t& tc, char& bc, const Glib::ustring& cs);
		bool set_class_string(const Glib::ustring& cs) { return set_class_string(m_typecode,  m_bdryclass, cs); }
		static const Glib::ustring& get_type_string(uint8_t tc);
		const Glib::ustring& get_type_string(void) const { return get_type_string(m_typecode); }
		uint32_t get_width(void) const { return m_width; }
		void set_width(uint32_t w = 0) { m_width = w; }
		double get_width_nmi(void) const { return m_width * (Point::to_deg_dbl * 60.0); }
		void set_width_nmi(double w = 0) { m_width = Point::round<double,uint32_t>(w * (Point::from_deg_dbl / 60.0)); }
		bool is_valid(void) const { return !!m_bdryclass; }
		unsigned int get_nrsegments(void) const { return m_seg.size(); }
		Segment& operator[](unsigned int x) { return m_seg[x]; }
		const Segment& operator[](unsigned int x) const { return m_seg[x]; }
		void add_segment(const Segment& x) { m_seg.push_back(x); recompute_bbox(); }
		void insert_segment(const Segment& x, int index) { m_seg.insert(m_seg.begin() + index, x); recompute_bbox(); }
		void erase_segment(int index) { m_seg.erase(m_seg.begin() + index); recompute_bbox(); }
		void clear_segments(void) { m_seg.clear(); recompute_bbox(); }
		unsigned int get_nrcomponents(void) const { return m_comp.size(); }
		Component& get_component(unsigned int x) { return m_comp[x]; }
		const Component& get_component(unsigned int x) const { return m_comp[x]; }
		void add_component(const Component& x) { m_comp.push_back(x); }
		void insert_component(const Component& x, int index) { m_comp.insert(m_comp.begin() + index, x); }
		void erase_component(int index) { m_comp.erase(m_comp.begin() + index); }
		void clear_components(void) { m_comp.clear(); }
		const MultiPolygonHole& get_polygon(void) const { return m_polygon; }
		MultiPolygonHole& get_polygon(void) { return m_polygon; }
		void recompute_lineapprox(DbQueryInterface<DbBaseElements::Airspace>& db);
		void recompute_bbox(void);
		void compute_initial_label_placement(void);
		static float label_metric(const Point& p0, const Point& p1);
		void compute_segments_from_poly(void);
		void set_polygon(const MultiPolygonHole& poly);
		static const IcaoRegion::Country *find_icaoregion_countries(const std::string& rgn);
		static const char *find_icaoregion_from_ident(const std::string& id);

		static const gndelev_t gndelev_unknown;

		template<class Archive> void hibernate(Archive& ar) {
			DbElementLabelSourceCoordIcaoNameBase::hibernate<Archive>(ar);
			ar.io(m_seg, "airspacesegments", "BDRYID", "Segment", subtables_segments);
			ar.io(m_comp, "airspacecomponents", "BDRYID", "Component", subtables_components);
			ar.io(m_polygon, "POLY", "Polygon");
			ar.io(m_ident, "IDENT", "ident", hibernate_sqlindex | hibernate_sqlnocase);
			ar.io(m_classexcept, "CLASSEXCEPT", "classexcept");
			ar.io(m_efftime, "EFFTIMES", "efftime");
			ar.io(m_note, "NOTE", "note");
			ar.io(m_commname, "COMMNAME", "commname");
			ar.io(m_commfreq[0], "COMMFREQ0", "commfreq0", hibernate_xmlnonzero);
			ar.io(m_commfreq[1], "COMMFREQ1", "commfreq1", hibernate_xmlnonzero);
			ar.io(m_typecode, m_bdryclass, "TYPECODE", "CLASS", "class");
			ar.io(m_width, "WIDTH", "width");
			ar.io(m_altlwr, "ALTLOWER", "altlwr");
			ar.io(m_altupr, "ALTUPPER", "altupr");
			ar.io(m_altlwrflags, "ALTLOWERFLAGS", "altlwrflags");
			ar.io(m_altuprflags, "ALTUPPERFLAGS", "altuprflags");
			ar.io(m_gndelevmin, "GNDELEVMIN", "gndelevmin");
			ar.io(m_gndelevmax, "GNDELEVMAX", "gndelevmax");
			Rect bbox(get_bbox());
			ar.io(bbox, hibernate_sqlindex);
			if (ar.is_save() || ar.is_schema()) {
				tile_t tile(TileNumber::to_tilenumber(bbox));
				ar.io(tile, "TILE", 0, hibernate_sqlindex);
			}
		}

		template<class Archive> void hibernate_binary(Archive& ar) {
			DbElementLabelSourceCoordIcaoNameBase::hibernate_binary(ar);
			{
				uint32_t sz(m_seg.size());
				ar.io(sz);
				if (ar.is_load())
					m_seg.resize(sz);
				for (std::vector<Segment>::iterator i(m_seg.begin()), e(m_seg.end()); i != e; ++i)
					i->hibernate_binary(ar);
			}
			{
				uint32_t sz(m_comp.size());
				ar.io(sz);
				if (ar.is_load())
					m_comp.resize(sz);
				for (std::vector<Component>::iterator i(m_comp.begin()), e(m_comp.end()); i != e; ++i)
					i->hibernate_binary(ar);
			}
			m_polygon.hibernate_binary(ar);
			ar.io(m_ident);
			ar.io(m_classexcept);
			ar.io(m_efftime);
			ar.io(m_note);
			ar.io(m_commname);
			ar.io(m_commfreq[2]);
			ar.io(m_typecode);
			ar.io(m_bdryclass);
			ar.io(m_width);
			ar.io(m_altlwr);
			ar.io(m_altupr);
			ar.io(m_altlwrflags);
			ar.io(m_altuprflags);
			ar.io(m_gndelevmin);
			ar.io(m_gndelevmax);
			ar.iouint32(m_subtables);
			//m_bbox.hibernate_binary(ar);
			if (ar.is_load())
				recompute_bbox();
		}

	protected:
		typedef std::vector<Segment> seg_t;
		seg_t m_seg;
		typedef std::vector<Component> comp_t;
		comp_t m_comp;
		MultiPolygonHole m_polygon;
		Glib::ustring m_ident;
		Glib::ustring m_classexcept;
		Glib::ustring m_efftime;
		Glib::ustring m_note;
		Glib::ustring m_commname;
		uint32_t m_commfreq[2];
		Rect m_bbox;
		uint8_t m_typecode;
		char m_bdryclass;
		int32_t m_width;
		int32_t m_altlwr, m_altupr;
		uint8_t m_altlwrflags, m_altuprflags;
		gndelev_t m_gndelevmin, m_gndelevmax;
		unsigned int m_subtables;

		static constexpr float boundarycircleapprox = 10.0;
		void recompute_lineapprox_expand(void);
		void recompute_lineapprox_circle(void);
		void recompute_lineapprox_seg(void);
		void recompute_lineapprox_comp(DbQueryInterface<DbBaseElements::Airspace>& db);
		static bool triangle_center(Point& ret, Point p0, Point p1, Point p2);
        };

        class Mapelement {
	public:
		typedef ::DbBaseElements::ElementAddress Address;
		typedef Address::table_t table_t;
		static const table_t table_main = Address::table_main;
		static const table_t table_aux = Address::table_aux;
		typedef ::DbBaseElements::TileNumber TileNumber;
		typedef TileNumber::tile_t tile_t;
		typedef TileNumber::tilerange_t tilerange_t;

		static const uint8_t maptyp_invalid         = 0;

		static const uint8_t maptyp_railway         = 47;
		static const uint8_t maptyp_railway_d       = 57;
		static const uint8_t maptyp_aerial_cable    = 48;

		static const uint8_t maptyp_highway         = 45;
		static const uint8_t maptyp_road            = 46;
		static const uint8_t maptyp_trail           = 58;

		static const uint8_t maptyp_river           = 50;
		static const uint8_t maptyp_lake            = 49;
		static const uint8_t maptyp_river_t         = 54;
		static const uint8_t maptyp_lake_t          = 55;

		static const uint8_t maptyp_canal           = 51;

		static const uint8_t maptyp_pack_ice        = 56;
		static const uint8_t maptyp_glacier         = 40;
		static const uint8_t maptyp_forest          = 60;

		static const uint8_t maptyp_city            = 42;
		static const uint8_t maptyp_village         = 43;
		static const uint8_t maptyp_spot            = 38;
		static const uint8_t maptyp_landmark        = 44;

		static const uint8_t maptyp_powerline       = 130;
		static const uint8_t maptyp_border          = 134;

		static const unsigned int subtables_none = 0;
		static const unsigned int subtables_all = subtables_none;

		static const char *db_query_string;
		static const char *db_aux_query_string;
		static const char *db_text_fields[];
		static const char *db_time_fields[];

		Mapelement(void);
		Mapelement(uint8_t tc, const PolygonHole& po, const Glib::ustring nm = "");
		Mapelement(uint8_t tc, const PolygonHole& po, const Point& lblcoord, const Glib::ustring nm = "");
		void load(sqlite3x::sqlite3_cursor& cursor, sqlite3x::sqlite3_connection & db, unsigned int loadsubtables = subtables_all);
		void load_subtables(sqlite3x::sqlite3_connection& db, unsigned int loadsubtables = subtables_all) {}
		unsigned int get_subtables(void) const { return ~0; }
		bool is_subtables_loaded(void) const { return true; }
		void save(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
		void erase(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
		void update_index(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
#ifdef HAVE_PQXX
		void load(const pqxx::tuple& cursor, pqxx::basic_transaction& w, unsigned int loadsubtables = subtables_all);
		void load_subtables(pqxx::basic_transaction& w, unsigned int loadsubtables = subtables_all) {}
		void save(pqxx::connection_base& conn, bool rtree);
		void erase(pqxx::connection_base& conn, bool rtree);
		void update_index(pqxx::connection_base& conn, bool rtree);
#endif
		int64_t get_id(void) const { return m_id; }
		table_t get_table(void) const { return m_table; }
		Address get_address(void) const { return Address(m_id, m_table); }
		bool is_valid(void) const { return m_typecode != maptyp_invalid; }
		static bool is_area(uint8_t tc);
		bool is_area(void) const { return is_area(get_typecode()); }
		Rect get_bbox(void) const { return m_bbox; }
		Point get_labelcoord(void) const { return m_labelcoord; }
		void set_labelcoord(const Point& pt) { m_labelcoord = pt; }
		uint8_t get_typecode(void) const { return m_typecode; }
		void set_typecode(uint8_t tc) { m_typecode = tc; }
		static const Glib::ustring& get_typename(uint8_t tc);
		const Glib::ustring& get_typename(void) const { return get_typename(get_typecode()); }
		const Glib::ustring& get_name(void) const { return m_name; }
		void set_name(const Glib::ustring& nm) { m_name = nm; }
		const PolygonHole& get_polygon(void) const { return m_polygon; }
		void set_polygon(const PolygonHole& poly) { m_polygon = poly; recompute_bbox_label(); }
		const PolygonSimple& get_exterior(void) const { return m_polygon.get_exterior(); }
		unsigned int get_nrinterior(void) const { return m_polygon.get_nrinterior(); }
		const PolygonSimple& operator[](unsigned int x) const { return m_polygon[x]; }
		Point get_center(void) const { return m_bbox.boxcenter(); }
		Point get_coord(void) const { return get_labelcoord(); }

		template<class Archive> void hibernate(Archive& ar) {
			ar.io(m_id, "ID", 0, DbElementBase::hibernate_id | DbElementBase::hibernate_sqlnotnull);
			ar.io(m_bbox);
			ar.io(m_typecode, "TYPECODE", "typecode");
			ar.io(m_name, "NAME", "name");
			ar.io(m_labelcoord, "LAT", "LON", "lat", "lon");
			ar.io(m_polygon, "POLY", "poly");
			if (ar.is_save() || ar.is_schema()) {
				tile_t tile(TileNumber::to_tilenumber(m_bbox));
				ar.io(tile, "TILE", 0, DbElementBase::hibernate_sqlindex);
			}
		}

		template<class Archive> void hibernate_binary(Archive& ar) {
			ar.io(m_id);
			ar.io(m_typecode);
			ar.iouint8(m_table);
			ar.io(m_name);
			m_labelcoord.hibernate_binary(ar);
			m_polygon.hibernate_binary(ar);
			//m_bbox.hibernate_binary(ar);
			if (ar.is_load())
				m_bbox = m_polygon.get_bbox();
		}

	protected:
		int64_t m_id;
		Rect m_bbox;
		uint8_t m_typecode;
		table_t m_table;
		Glib::ustring m_name;
		Point m_labelcoord;
		PolygonHole m_polygon;

		void recompute_bbox_label(void);
		int lon_center_poly(int32_t lat);
		void recompute_label_coord(void);
        };

        class Waterelement {
	public:
		typedef ::DbBaseElements::ElementAddress Address;
		typedef Address::table_t table_t;
		static const table_t table_main = Address::table_main;
		static const table_t table_aux = Address::table_aux;
		typedef ::DbBaseElements::TileNumber TileNumber;
		typedef TileNumber::tile_t tile_t;
		typedef TileNumber::tilerange_t tilerange_t;

		typedef enum {
			type_invalid = 0,
			type_landmass = 1,
			type_lake = 2,
			type_island = 3,
			type_pond = 4,
			type_ice = 5,
			type_groundingline = 6,
			type_land = 128,
			type_landext = 129
		} type_t;

		static const unsigned int subtables_none = 0;
		static const unsigned int subtables_all = subtables_none;

		static const char *db_query_string;
		static const char *db_aux_query_string;
		static const char *db_text_fields[];
		static const char *db_time_fields[];

		Waterelement(void);
		Waterelement(type_t typ, const PolygonHole& po);
		void load(sqlite3x::sqlite3_cursor& cursor, sqlite3x::sqlite3_connection & db, unsigned int loadsubtables = subtables_all);
		void load_subtables(sqlite3x::sqlite3_connection& db, unsigned int loadsubtables = subtables_all) {}
		unsigned int get_subtables(void) const { return ~0; }
		bool is_subtables_loaded(void) const { return true; }
		void save(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
		void erase(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
		void update_index(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
#ifdef HAVE_PQXX
		void load(const pqxx::tuple& cursor, pqxx::basic_transaction& w, unsigned int loadsubtables = subtables_all);
		void load_subtables(pqxx::basic_transaction& w, unsigned int loadsubtables = subtables_all) {}
		void save(pqxx::connection_base& conn, bool rtree);
		void erase(pqxx::connection_base& conn, bool rtree);
		void update_index(pqxx::connection_base& conn, bool rtree);
#endif
		int64_t get_id(void) const { return m_id; }
		table_t get_table(void) const { return m_table; }
		Address get_address(void) const { return Address(m_id, m_table); }
		bool is_valid(void) const { return m_type != type_invalid; }
		bool is_area(void) const;
		Rect get_bbox(void) const { return m_bbox; }
		type_t get_type(void) const { return m_type; }
		void set_type(type_t t) { m_type = t; }
		static const Glib::ustring& get_typename(type_t t);
		const Glib::ustring& get_typename(void) const { return get_typename(get_type()); }
		const PolygonHole& get_polygon(void) const { return m_polygon; }
		void set_polygon(const PolygonHole& poly) { m_polygon = poly; recompute_bbox(); }
		const PolygonSimple& get_exterior(void) const { return m_polygon.get_exterior(); }
		unsigned int get_nrinterior(void) const { return m_polygon.get_nrinterior(); }
		const PolygonSimple& operator[](unsigned int x) const { return m_polygon[x]; }
		Point get_center(void) const { return m_bbox.boxcenter(); }

		template<class Archive> void hibernate(Archive& ar) {
			ar.io(m_id, "ID", 0, DbElementBase::hibernate_id | DbElementBase::hibernate_sqlnotnull);
			ar.io(m_bbox);
			ar.io(m_type, "TYPE", "type");
			ar.io(m_polygon, "POLY", "poly");
			if (ar.is_save() || ar.is_schema()) {
				tile_t tile(TileNumber::to_tilenumber(m_bbox));
				ar.io(tile, "TILE", 0, DbElementBase::hibernate_sqlindex);
			}
		}

		template<class Archive> void hibernate_binary(Archive& ar) {
			ar.io(m_id);
			ar.iouint8(m_type);
			ar.iouint8(m_table);
			m_polygon.hibernate_binary(ar);
			//m_bbox.hibernate_binary(ar);
			if (ar.is_load())
				m_bbox = m_polygon.get_bbox();
		}

	protected:
		int64_t m_id;
		Rect m_bbox;
		type_t m_type;
		table_t m_table;
		PolygonHole m_polygon;

		void recompute_bbox(void);
        };

        class Track : public DbElementSourceBase {
	public:
		class TrackPoint {
		public:
			typedef int16_t alt_t;
			static const unsigned int blob_size_v1 = 16;

			TrackPoint(void);
			time_t get_time(void) const { return m_time >> 8; }
			void set_time(time_t t) { m_time = ((uint64_t)t) << 8; }
			uint64_t get_timefrac8(void) const { return m_time; }
			void set_timefrac8(uint64_t t) { m_time = t; }
			Glib::TimeVal get_timeval(void) const;
			void set_timeval(const Glib::TimeVal& tv);
			Point get_coord(void) const { return m_coord; }
			void set_coord(const Point& pt) { m_coord = pt; }
			alt_t get_alt(void) const { return m_alt; }
			void set_alt(alt_t alt) { m_alt = alt; }
			bool is_alt_valid(void) const { return m_alt != altinvalid; }
			void set_alt_invalid(void) { m_alt = altinvalid; }

			bool operator<(const TrackPoint& b) const { return m_time < b.m_time; }

			void read_blob(const uint8_t *data);
			void write_blob(uint8_t *data) const;
			unsigned int blob_size(void) const { return blob_size_v1; }

			template<class Archive> void hibernate_binary(Archive& ar) {
				m_coord.hibernate_binary(ar);
				ar.io(m_time);
				ar.io(m_alt);
			}

		protected:
			static const alt_t altinvalid;
			Point m_coord;
			uint64_t m_time;
			alt_t m_alt;
		};

	protected:
		friend class DbBaseCommon;
		typedef std::vector<TrackPoint> trackpoints_t;

	public:
		typedef ::DbBaseElements::ElementAddress Address;
		typedef Address::table_t table_t;
		static const table_t table_main = Address::table_main;
		static const table_t table_aux = Address::table_aux;
		typedef trackpoints_t::size_type size_type;
		typedef ::DbBaseElements::TileNumber TileNumber;
		typedef TileNumber::tile_t tile_t;
		typedef TileNumber::tilerange_t tilerange_t;

		typedef enum {
			format_invalid = 0,
			format_empty = 1,
			format_gzip = 2
		} format_t;

		static const unsigned int subtables_none = 0;
		static const unsigned int subtables_all = subtables_none;

		static const char *db_query_string;
		static const char *db_aux_query_string;
		static const char *db_text_fields[];
		static const char *db_time_fields[];

		Track(void);
		void load(sqlite3x::sqlite3_cursor& cursor, sqlite3x::sqlite3_connection & db, unsigned int loadsubtables = subtables_all);
		void load_subtables(sqlite3x::sqlite3_connection& db, unsigned int loadsubtables = subtables_all) {}
		unsigned int get_subtables(void) const { return ~0; }
		bool is_subtables_loaded(void) const { return true; }
		void save(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
		void erase(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
		void update_index(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
#ifdef HAVE_PQXX
		void load(const pqxx::tuple& cursor, pqxx::basic_transaction& w, unsigned int loadsubtables = subtables_all);
		void load_subtables(pqxx::basic_transaction& w, unsigned int loadsubtables = subtables_all) {}
		void save(pqxx::connection_base& conn, bool rtree);
		void erase(pqxx::connection_base& conn, bool rtree);
		void update_index(pqxx::connection_base& conn, bool rtree);
		void save_appendlog(pqxx::connection_base& conn, bool rtree);
#endif
		const Glib::ustring& get_fromicao(void) const { return m_fromicao; }
		void set_fromicao(const Glib::ustring& ic) { m_fromicao = ic; }
		const Glib::ustring& get_fromname(void) const { return m_fromname; }
		void set_fromname(const Glib::ustring& nm) { m_fromname = nm; }
		Glib::ustring get_fromicaoname(void) const;
		const Glib::ustring& get_toicao(void) const { return m_toicao; }
		void set_toicao(const Glib::ustring& ic) { m_toicao = ic; }
		const Glib::ustring& get_toname(void) const { return m_toname; }
		void set_toname(const Glib::ustring& nm) { m_toname = nm; }
		Glib::ustring get_toicaoname(void) const;
		const Glib::ustring& get_notes(void) const { return m_notes; }
		void set_notes(const Glib::ustring& n) { m_notes = n; }
		time_t get_offblocktime(void) const { return m_offblocktime; }
		void set_offblocktime(time_t t) { m_offblocktime = t; }
		time_t get_takeofftime(void) const { return m_takeofftime; }
		void set_takeofftime(time_t t) { m_takeofftime = t; }
		time_t get_landingtime(void) const { return m_landingtime; }
		void set_landingtime(time_t t) { m_landingtime = t; }
		time_t get_onblocktime(void) const { return m_onblocktime; }
		void set_onblocktime(time_t t) { m_onblocktime = t; }
		size_type size(void) const { return m_trackpoints.size(); }
		const TrackPoint& operator[](size_type x) const { return m_trackpoints[x]; }
		TrackPoint& operator[](size_type x) { return m_trackpoints[x]; }
		TrackPoint& append(const TrackPoint& tp);
		TrackPoint& insert(const TrackPoint& tp, int index);
		void erase(int index);
		void sort(void) { std::sort(m_trackpoints.begin(), m_trackpoints.end()); }
		bool is_valid(void) const { return m_format != format_invalid; }
		void make_valid(void);
		Rect get_bbox(void) const { return m_bbox; }
		void recompute_bbox(void);
		void save_appendlog(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
		size_type get_unsaved_points(void) const { return m_trackpoints.size() - m_savedtrackpoints; }

		template<class Archive> void hibernate(Archive& ar) {
			DbElementSourceBase::hibernate<Archive>(ar);
			ar.io(m_fromicao, "FROMICAO", "fromicao");
			ar.io(m_fromname, "FROMNAME", "fromname");
			ar.io(m_toicao, "TOICAO", "toicao");
			ar.io(m_toname, "TONAME", "toname");
			ar.io(m_notes, "NOTES", "notes");
			ar.iotime(m_offblocktime, "OFFBLOCKTIME", "offblocktime");
			ar.iotime(m_takeofftime, "TAKEOFFTIME", "takeofftime");
			ar.iotime(m_landingtime, "LANDINGTIME", "landingtime");
			ar.iotime(m_onblocktime, "ONBLOCKTIME", "onblocktime");
			ar.io(m_bbox);
			ar.io(m_trackpoints, "POINTS", "Point");
			//ar.io("", m_savedtrackpoints, );
			//ar.io("", m_format, );
			// blob handling
			if (ar.is_save() || ar.is_schema()) {
				tile_t tile(TileNumber::to_tilenumber(m_bbox));
				ar.io(tile, "TILE", 0, hibernate_sqlindex);
			}
		}

		template<class Archive> void hibernate_binary(Archive& ar) {
			ar.io(m_fromicao);
			ar.io(m_fromname);
			ar.io(m_toicao);
			ar.io(m_toname);
			ar.io(m_notes);
			ar.iouint32(m_offblocktime);
			ar.iouint32(m_takeofftime);
			ar.iouint32(m_landingtime);
			ar.iouint32(m_onblocktime);
			uint32_t sz(m_trackpoints.size());
			ar.io(sz);
			if (ar.is_load())
				m_trackpoints.resize(sz);
			for (trackpoints_t::iterator i(m_trackpoints.begin()), e(m_trackpoints.end()); i != e; ++i)
				i->hibernate_binary(ar);
			//m_bbox.hibernate_binary(ar);
			if (ar.is_load())
				recompute_bbox();
		}

	protected:
		Glib::ustring m_fromicao;
		Glib::ustring m_fromname;
		Glib::ustring m_toicao;
		Glib::ustring m_toname;
		Glib::ustring m_notes;
		time_t m_offblocktime;
		time_t m_takeofftime;
		time_t m_landingtime;
		time_t m_onblocktime;
		Rect m_bbox;
		trackpoints_t m_trackpoints;
		size_type m_savedtrackpoints;
		format_t m_format;
        };

        class Label : public DbElementLabelBase {
	public:
		typedef ::DbBaseElements::TileNumber TileNumber;
		typedef TileNumber::tile_t tile_t;
		typedef TileNumber::tilerange_t tilerange_t;
		typedef DbElementLabelBase::label_placement_t label_placement_t;

		static const unsigned int subtables_none = 0;
		static const unsigned int subtables_all = subtables_none;

		static const char *db_query_string;
		static const char *db_aux_query_string;
		static const char *db_text_fields[];
		static const char *db_time_fields[];

		Label(void);
		void load(sqlite3x::sqlite3_cursor& cursor, sqlite3x::sqlite3_connection & db, unsigned int loadsubtables = subtables_all);
		void load_subtables(sqlite3x::sqlite3_connection& db, unsigned int loadsubtables = subtables_all) {}
		unsigned int get_subtables(void) const { return ~0; }
		bool is_subtables_loaded(void) const { return true; }
		void save(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
		void erase(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
		void update_index(sqlite3x::sqlite3_connection& db, bool rtree, bool aux_rtree);
#ifdef HAVE_PQXX
		void load(const pqxx::tuple& cursor, pqxx::basic_transaction& w, unsigned int loadsubtables = subtables_all);
		void load_subtables(pqxx::basic_transaction& w, unsigned int loadsubtables = subtables_all) {}
		void save(pqxx::connection_base& conn, bool rtree);
		void erase(pqxx::connection_base& conn, bool rtree);
		void update_index(pqxx::connection_base& conn, bool rtree);
#endif
		bool get_mutable(void) const { return m_mutable; }
		void set_mutable(bool mut) { m_mutable = mut; }
		Point get_coord(void) const { return m_coord; }
		void set_coord(const Point& pt) { m_coord = pt; }
		double get_metric(void) const { return m_metric; }
		void set_metric(double metric) { m_metric = metric; }
		int64_t get_subid(void) const { return m_subid; }
		void set_subid(int64_t id) { m_subid = id; }
		table_t get_subtable(void) const { return m_subtable; }
		void set_subtable(table_t tbl) { m_subtable = tbl; }
		bool is_valid(void) const { return m_label_placement != label_off; }

		template<class Archive> void hibernate(Archive& ar) {
			DbElementLabelBase::hibernate<Archive>(ar);
			ar.io(m_mutable, "MUTABLE", "mutable");
			ar.io(m_coord, "LAT", "LON", "lat", "lon");
			ar.io(m_metric, "METRIC", "metric");
			ar.io(m_subid, "SUBID", "subid");
			ar.io(m_subtable, "SUBTABLE", "subtable");
			if (ar.is_save() || ar.is_schema()) {
				tile_t tile(TileNumber::to_tilenumber(get_coord()));
				ar.io(tile, "TILE", 0, hibernate_sqlindex);
			}
		}

		template<class Archive> void hibernate_binary(Archive& ar) {
			ar.io(m_mutable);
			m_coord.hibernate_binary(ar);
			ar.io(m_metric);
			ar.io(m_subid);
			ar.iouint8(m_subtable);
		}

	protected:
		bool m_mutable;
		Point m_coord;
		double m_metric;
		int64_t m_subid;
		table_t m_subtable;
	};
};

std::ostream& operator<<(std::ostream& os, const DbBaseElements::ElementAddress& addr);

class DbQueryInterfaceCommon {
public:
	typedef enum {
		comp_startswith = FPlan::comp_startswith,
		comp_contains = FPlan::comp_contains,
		comp_exact = FPlan::comp_exact,
		comp_exact_casesensitive,
		comp_like
	} comp_t;

};

template <class T> class DbQueryInterfaceSpecCommon : public DbQueryInterfaceCommon {
public:
	typedef T element_t;
	typedef std::vector<element_t> elementvector_t;
	typedef enum element_t::table_t table_t;
	typedef typename element_t::Address Address;

	virtual ~DbQueryInterfaceSpecCommon() {}

	virtual void close(void) = 0;
	virtual void sync_off(void) = 0;
	virtual void analyze(void) = 0;
	virtual void vacuum(void) = 0;
	virtual bool is_open(void) const = 0;
	virtual void set_exclusive(bool excl = true) = 0;
	virtual void purgedb(void) = 0;
	virtual void interrupt(void) = 0;

	virtual element_t operator()(int64_t id, table_t tbl = element_t::table_main, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual element_t operator()(Address addr, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual void loadfirst(element_t& e, bool include_aux = true, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual void loadnext(element_t& e, bool include_aux = true, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual void save(element_t& e) = 0;
	virtual void erase(element_t& e) = 0;
	virtual void update_index(element_t& e) = 0;

	class ForEach {
	public:
		virtual ~ForEach() {}
		virtual bool operator()(const element_t&) = 0;
		virtual bool operator()(const std::string&) = 0;
	};

	virtual void for_each(ForEach& cb, bool include_aux = true, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual void for_each_by_rect(ForEach& cb, const Rect& r, bool include_aux = true, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual elementvector_t find_by_text(const std::string& pattern, char escape, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual elementvector_t find_by_time(time_t fromtime, time_t totime, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual elementvector_t find_by_rect(const Rect& r, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual elementvector_t find_nearest(const Point& pt, const Rect& r, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual elementvector_t find_nulltile(unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual void load_subtables(element_t& el, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual void load_subtables(elementvector_t& ev, unsigned int loadsubtables = element_t::subtables_all);
};

template <class T> class DbQueryInterface : public DbQueryInterfaceSpecCommon<T> {
public:
};

template <> class DbQueryInterface<DbBaseElements::Navaid> : public DbQueryInterfaceSpecCommon<DbBaseElements::Navaid> {
public:
	virtual elementvector_t find_by_sourceid(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual elementvector_t find_by_icao(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual elementvector_t find_by_name(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
};

template <> class DbQueryInterface<DbBaseElements::Waypoint> : public DbQueryInterfaceSpecCommon<DbBaseElements::Waypoint> {
public:
	virtual elementvector_t find_by_sourceid(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual elementvector_t find_by_icao(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual elementvector_t find_by_name(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
};

template <> class DbQueryInterface<DbBaseElements::Airway> : public DbQueryInterfaceSpecCommon<DbBaseElements::Airway> {
public:
	virtual elementvector_t find_by_sourceid(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual elementvector_t find_by_begin_name(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual elementvector_t find_by_end_name(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual elementvector_t find_by_name(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual elementvector_t find_by_area(const Rect& r, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
};

template <> class DbQueryInterface<DbBaseElements::Airport> : public DbQueryInterfaceSpecCommon<DbBaseElements::Airport> {
public:
	virtual elementvector_t find_by_sourceid(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual elementvector_t find_by_icao(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual elementvector_t find_by_name(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
};

template <> class DbQueryInterface<DbBaseElements::Airspace> : public DbQueryInterfaceSpecCommon<DbBaseElements::Airspace> {
public:
	virtual elementvector_t find_by_sourceid(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual elementvector_t find_by_icao(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual elementvector_t find_by_name(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual elementvector_t find_by_ident(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
};

template <> class DbQueryInterface<DbBaseElements::Mapelement> : public DbQueryInterfaceSpecCommon<DbBaseElements::Mapelement> {
public:
	virtual elementvector_t find_by_name(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
};

template <> class DbQueryInterface<DbBaseElements::Waterelement> : public DbQueryInterfaceSpecCommon<DbBaseElements::Waterelement> {
public:
	virtual elementvector_t find_by_rect_type(const Rect& r, element_t::type_t typ, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual elementvector_t find_by_type(element_t::type_t typ, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual void purge(element_t::type_t typ) = 0;
};

template <> class DbQueryInterface<DbBaseElements::Track> : public DbQueryInterfaceSpecCommon<DbBaseElements::Track> {
public:
	virtual void clear_appendlogs(void) = 0;
	virtual void save_appendlog(element_t& e) = 0;
	virtual elementvector_t find_by_sourceid(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual elementvector_t find_by_fromicao(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual elementvector_t find_by_fromname(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual elementvector_t find_by_toicao(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
	virtual elementvector_t find_by_toname(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
};

template <> class DbQueryInterface<DbBaseElements::Label> : public DbQueryInterfaceSpecCommon<DbBaseElements::Label> {
public:
	virtual elementvector_t find_mutable_by_metric(unsigned int limit = 0, unsigned int offset = 0, unsigned int loadsubtables = element_t::subtables_all) = 0;
};

typedef DbQueryInterface<DbBaseElements::Navaid> NavaidsDbQueryInterface;
typedef DbQueryInterface<DbBaseElements::Waypoint> WaypointsDbQueryInterface;
typedef DbQueryInterface<DbBaseElements::Airway> AirwaysDbQueryInterface;
typedef DbQueryInterface<DbBaseElements::Airport> AirportsDbQueryInterface;
typedef DbQueryInterface<DbBaseElements::Airspace> AirspacesDbQueryInterface;
typedef DbQueryInterface<DbBaseElements::Mapelement> MapelementsDbQueryInterface;
typedef DbQueryInterface<DbBaseElements::Waterelement> WaterelementsDbQueryInterface;
typedef DbQueryInterface<DbBaseElements::Track> TracksDbQueryInterface;
typedef DbQueryInterface<DbBaseElements::Label> LabelsDbQueryInterface;

class DbBaseCommon {
public:
	DbBaseCommon(void);
	~DbBaseCommon(void);
	void close(void);
	void detach(void);
	void sync_off(void);
	void analyze(void);
	void vacuum(void);
	void set_exclusive(bool excl = true);
	unsigned int query_cache_size(void);
	void set_cache_size(unsigned int cs);
	bool is_open(void) const { return m_open != openstate_closed; }
	bool is_aux_attached(void) const { return m_open == openstate_auxopen; }
	void interrupt(void);
	// for the benefit of utilities that need special sql statements;
	// not for normal use
	sqlite3x::sqlite3_connection& get_db(void) { return m_db; }
	static std::string filename_to_uri(const std::string& fn);

protected:
	class DbSchemaArchiver;

	typedef enum {
		openstate_closed,
		openstate_mainopen,
		openstate_auxopen
	} openstate_t;
	sqlite3x::sqlite3_connection m_db;
	openstate_t m_open;
	bool m_has_rtree;
	bool m_has_aux_rtree;

	static void dbfunc_simpledist(sqlite3_context *ctxt, int, sqlite3_value **values);
	static void dbfunc_simplerectdist(sqlite3_context *ctxt, int, sqlite3_value **values);
	static void dbfunc_upperbound(sqlite3_context *ctxt, int, sqlite3_value **values);

	static Glib::ustring quote_text_for_like(const Glib::ustring& s, char e = 0);
};

template <class T> class DbBase : public DbBaseCommon, public DbQueryInterface<T> {
public:
	typedef T element_t;
	typedef std::vector<element_t> elementvector_t;
	typedef enum element_t::table_t table_t;
	typedef typename element_t::Address Address;
	typedef typename DbQueryInterface<T>::ForEach ForEach;
	typedef typename DbQueryInterface<T>::comp_t comp_t;
	typedef enum {
		read_only
	} read_only_tag_t;

	DbBase();
	DbBase(const Glib::ustring& path);
	DbBase(const Glib::ustring& path, read_only_tag_t);
	DbBase(const Glib::ustring& path, const Glib::ustring& aux_path);
	DbBase(const Glib::ustring& path, const Glib::ustring& aux_path, read_only_tag_t);
	void close(void) { DbBaseCommon::close(); }
	void sync_off(void) { DbBaseCommon::sync_off(); }
	void analyze(void) { DbBaseCommon::analyze(); }
	void vacuum(void) { DbBaseCommon::vacuum(); }
	bool is_open(void) const { return DbBaseCommon::is_open(); }
	void set_exclusive(bool excl = true) { DbBaseCommon::set_exclusive(excl); }
	void open(const Glib::ustring& path = "") { open(path, 0); }
	void open_readonly(const Glib::ustring& path = "") { open_readonly(path, 0); }
	void attach(const Glib::ustring& path = "") { attach(path, 0); }
	void attach_readonly(const Glib::ustring& path = "") { attach_readonly(path, 0); }
	static bool is_dbfile_exists(const Glib::ustring& path = "");
	void take(sqlite3 *dbh);
	void purgedb(void);
	void interrupt(void) { DbBaseCommon::interrupt(); }
	element_t operator()(int64_t id, table_t tbl = element_t::table_main, unsigned int loadsubtables = element_t::subtables_all);
	element_t operator()(Address addr, unsigned int loadsubtables = element_t::subtables_all) { return operator()(addr.get_id(), addr.get_table(), loadsubtables); }
	void loadfirst(element_t& e, bool include_aux = true, unsigned int loadsubtables = element_t::subtables_all);
	void loadnext(element_t& e, bool include_aux = true, unsigned int loadsubtables = element_t::subtables_all);
	void save(element_t& e) { e.save(m_db, m_has_rtree, m_has_aux_rtree); }
	void erase(element_t& e) { e.erase(m_db, m_has_rtree, m_has_aux_rtree); }
	void update_index(element_t& e) { e.update_index(m_db, m_has_rtree, m_has_aux_rtree); }

	void for_each(ForEach& cb, bool include_aux = true, unsigned int loadsubtables = element_t::subtables_all);
	void for_each_by_rect(ForEach& cb, const Rect& r, bool include_aux = true, unsigned int loadsubtables = element_t::subtables_all);
	elementvector_t find_by_text(const std::string& pattern, char escape, comp_t comp = DbQueryInterface<T>::comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all);
	elementvector_t find_by_time(time_t fromtime, time_t totime, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all);
	elementvector_t find_by_rect(const Rect& r, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all);
	elementvector_t find_nearest(const Point& pt, const Rect& r, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all);
	elementvector_t find_nulltile(unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all);
	void load_subtables(element_t& el, unsigned int loadsubtables = element_t::subtables_all);

protected:
	static const char *main_table_name;
	static const char *database_file_name;
	static const char *order_field;
	static const char *delete_field;
	static const bool area_data;

	void drop_tables(void);
	void create_tables(void);
	void drop_indices(void);
	void create_indices(void);

	std::string get_area_select_string(bool auxtable, const Rect& r, const char *sortcol = 0);
	elementvector_t find(const std::string& fldname, const std::string& pattern, char escape, comp_t comp = DbQueryInterface<T>::comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all);
	elementvector_t loadid(uint64_t startid = 0, table_t table = element_t::table_main, const std::string& order = "", unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all);
	void open(const Glib::ustring& path, const char *dbfilename);
	void open_readonly(const Glib::ustring& path, const char *dbfilename);
	void attach(const Glib::ustring& path, const char *dbfilename);
	void attach_readonly(const Glib::ustring& path, const char *dbfilename);
};

class NavaidsDb : public DbBase<DbBaseElements::Navaid> {
public:
	typedef element_t Navaid;

	NavaidsDb() : DbBase<DbBaseElements::Navaid>() {}
	NavaidsDb(const Glib::ustring& path) : DbBase<DbBaseElements::Navaid>(path) {}
	NavaidsDb(const Glib::ustring& path, read_only_tag_t) : DbBase<DbBaseElements::Navaid>(path, read_only) {}
	NavaidsDb(const Glib::ustring& path, const Glib::ustring& aux_path) : DbBase<DbBaseElements::Navaid>(path, aux_path) {}
	NavaidsDb(const Glib::ustring& path, const Glib::ustring& aux_path, read_only_tag_t) : DbBase<DbBaseElements::Navaid>(path, aux_path, read_only) {}

	void take(NavaidsDb& db) { DbBase<element_t>::take(db.m_db.take()); }

	elementvector_t find_by_sourceid(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("SRCID", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_icao(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("ICAO", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_name(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("NAME", pattern, escape, comp, limit, loadsubtables); }
};

class WaypointsDb : public DbBase<DbBaseElements::Waypoint> {
public:
	typedef element_t Waypoint;

	WaypointsDb() : DbBase<DbBaseElements::Waypoint>() {}
	WaypointsDb(const Glib::ustring& path) : DbBase<DbBaseElements::Waypoint>(path) {}
	WaypointsDb(const Glib::ustring& path, read_only_tag_t) : DbBase<DbBaseElements::Waypoint>(path, read_only) {}
	WaypointsDb(const Glib::ustring& path, const Glib::ustring& aux_path) : DbBase<DbBaseElements::Waypoint>(path, aux_path) {}
	WaypointsDb(const Glib::ustring& path, const Glib::ustring& aux_path, read_only_tag_t) : DbBase<DbBaseElements::Waypoint>(path, aux_path, read_only) {}

	void take(WaypointsDb& db) { DbBase<element_t>::take(db.m_db.take()); }
	elementvector_t find_by_sourceid(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("SRCID", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_icao(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("ICAO", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_name(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("NAME", pattern, escape, comp, limit, loadsubtables); }
};

class AirwaysDb : public DbBase<DbBaseElements::Airway> {
public:
	typedef element_t Airway;

	AirwaysDb() : DbBase<DbBaseElements::Airway>() {}
	AirwaysDb(const Glib::ustring& path) : DbBase<DbBaseElements::Airway>(path) {}
	AirwaysDb(const Glib::ustring& path, read_only_tag_t) : DbBase<DbBaseElements::Airway>(path, read_only) {}
	AirwaysDb(const Glib::ustring& path, const Glib::ustring& aux_path) : DbBase<DbBaseElements::Airway>(path, aux_path) {}
	AirwaysDb(const Glib::ustring& path, const Glib::ustring& aux_path, read_only_tag_t) : DbBase<DbBaseElements::Airway>(path, aux_path, read_only) {}

	void take(AirwaysDb& db) { DbBase<element_t>::take(db.m_db.take()); }
	elementvector_t find_by_sourceid(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("SRCID", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_begin_name(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("BNAME", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_end_name(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("ENAME", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_name(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("NAME", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_area(const Rect& r, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all);

protected:
	std::string get_airways_area_select_string(bool auxtable, const Rect& r, const char *sortcol = 0);
};

class AirportsDb : public DbBase<DbBaseElements::Airport> {
public:
	typedef element_t Airport;

	AirportsDb() : DbBase<DbBaseElements::Airport>() {}
	AirportsDb(const Glib::ustring& path) : DbBase<DbBaseElements::Airport>(path) {}
	AirportsDb(const Glib::ustring& path, read_only_tag_t) : DbBase<DbBaseElements::Airport>(path, read_only) {}
	AirportsDb(const Glib::ustring& path, const Glib::ustring& aux_path) : DbBase<DbBaseElements::Airport>(path, aux_path) {}
	AirportsDb(const Glib::ustring& path, const Glib::ustring& aux_path, read_only_tag_t) : DbBase<DbBaseElements::Airport>(path, aux_path, read_only) {}

	void take(AirportsDb& db) { DbBase<element_t>::take(db.m_db.take()); }
	elementvector_t find_by_sourceid(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("SRCID", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_icao(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("ICAO", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_name(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("NAME", pattern, escape, comp, limit, loadsubtables); }
};

class AirspacesDb : public DbBase<DbBaseElements::Airspace> {
public:
	typedef element_t Airspace;

	AirspacesDb() : DbBase<DbBaseElements::Airspace>() {}
	AirspacesDb(const Glib::ustring& path) : DbBase<DbBaseElements::Airspace>(path) {}
	AirspacesDb(const Glib::ustring& path, read_only_tag_t) : DbBase<DbBaseElements::Airspace>(path, read_only) {}
	AirspacesDb(const Glib::ustring& path, const Glib::ustring& aux_path) : DbBase<DbBaseElements::Airspace>(path, aux_path) {}
	AirspacesDb(const Glib::ustring& path, const Glib::ustring& aux_path, read_only_tag_t) : DbBase<DbBaseElements::Airspace>(path, aux_path, read_only) {}

	void take(AirspacesDb& db) { DbBase<element_t>::take(db.m_db.take()); }
	elementvector_t find_by_sourceid(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("SRCID", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_icao(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("ICAO", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_name(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("NAME", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_ident(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("IDENT", pattern, escape, comp, limit, loadsubtables); }
};

class MapelementsDb : public DbBase<DbBaseElements::Mapelement> {
public:
	typedef element_t Mapelement;

	MapelementsDb() : DbBase<DbBaseElements::Mapelement>() {}
	MapelementsDb(const Glib::ustring& path) : DbBase<DbBaseElements::Mapelement>(path) {}
	MapelementsDb(const Glib::ustring& path, read_only_tag_t) : DbBase<DbBaseElements::Mapelement>(path, read_only) {}

	void take(MapelementsDb& db) { DbBase<element_t>::take(db.m_db.take()); }
	elementvector_t find_by_name(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("NAME", pattern, escape, comp, limit, loadsubtables); }
};

class WaterelementsDb : public DbBase<DbBaseElements::Waterelement> {
public:
	typedef element_t Waterelement;

	WaterelementsDb() : DbBase<DbBaseElements::Waterelement>() {}
	WaterelementsDb(const Glib::ustring& path) : DbBase<DbBaseElements::Waterelement>(path) {}
	WaterelementsDb(const Glib::ustring& path, read_only_tag_t) : DbBase<DbBaseElements::Waterelement>(path, read_only) {}

	void open_hires(const Glib::ustring& path = "");
	void open_readonly_hires(const Glib::ustring& path = "");
	void take(WaterelementsDb& db) { DbBase<element_t>::take(db.m_db.take()); }
	elementvector_t find_by_rect_type(const Rect& r, element_t::type_t typ, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all);
	elementvector_t find_by_type(element_t::type_t typ, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all);
	void purge(element_t::type_t typ);
};

class TracksDb : public DbBase<DbBaseElements::Track> {
public:
	typedef element_t Track;

	TracksDb() : DbBase<DbBaseElements::Track>() {}
	TracksDb(const Glib::ustring& path) : DbBase<DbBaseElements::Track>(path) {}
	TracksDb(const Glib::ustring& path, read_only_tag_t) : DbBase<DbBaseElements::Track>(path, read_only) {}

	void open(const Glib::ustring& path = "");
	void clear_appendlogs(void);
	void take(TracksDb& db) { DbBase<element_t>::take(db.m_db.take()); }
	void save_appendlog(element_t& e) { e.save_appendlog(m_db, m_has_rtree, m_has_aux_rtree); }
	elementvector_t find_by_sourceid(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("SRCID", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_fromicao(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("FROMICAO", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_fromname(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("FROMNAME", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_toicao(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("TOICAO", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_toname(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("TONAME", pattern, escape, comp, limit, loadsubtables); }
};

/* a simple label database, used for computing label placements */
class LabelsDb : public DbBase<DbBaseElements::Label> {
public:
	typedef element_t Label;

	LabelsDb() : DbBase<DbBaseElements::Label>() {}
	LabelsDb(const Glib::ustring& path) : DbBase<DbBaseElements::Label>(path) {}
	LabelsDb(const Glib::ustring& path, read_only_tag_t) : DbBase<DbBaseElements::Label>(path, read_only) {}

	void take(LabelsDb& db) { DbBase<element_t>::take(db.m_db.take()); }
	elementvector_t find_mutable_by_metric(unsigned int limit = 0, unsigned int offset = 0, unsigned int loadsubtables = element_t::subtables_all);
};

#ifdef HAVE_PQXX

class PGDbBaseCommon {
public:
	typedef enum {
		read_only
	} read_only_tag_t;

	PGDbBaseCommon(pqxx::connection_base& conn);
	PGDbBaseCommon(pqxx::connection_base& conn, read_only_tag_t);
	~PGDbBaseCommon(void);
	void close(void);
	void sync_off(void);
	void analyze(void);
	void vacuum(void);
	void interrupt(void);
	bool is_open(void) const { return m_conn.is_open(); }
	void set_exclusive(bool excl = true) {}

	static std::string get_str(const uint8_t *wb, const uint8_t *ep);
	static std::string get_str(const MultiPolygonHole& p);
	static std::string get_str(const PolygonHole& p);
	static std::string get_str(const LineString& ls);

protected:
	pqxx::connection_base& m_conn;
	bool m_has_rtree;
	bool m_readonly;

	void define_dbfunc(void);
	void drop_common_tables(void);
	void create_common_tables(void);
	static Glib::ustring quote_text_for_like(const Glib::ustring& s, char e = 0);
};

template <class T> class PGDbBase : public PGDbBaseCommon, public DbQueryInterface<T> {
public:
	typedef T element_t;
	typedef std::vector<element_t> elementvector_t;
	typedef enum element_t::table_t table_t;
	typedef typename element_t::Address Address;
	typedef typename DbQueryInterface<T>::ForEach ForEach;
	typedef typename DbQueryInterface<T>::comp_t comp_t;

	PGDbBase(pqxx::connection_base& conn);
	PGDbBase(pqxx::connection_base& conn, read_only_tag_t);
	void close(void) { PGDbBaseCommon::close(); }
	void sync_off(void) { PGDbBaseCommon::sync_off(); }
	void analyze(void) { PGDbBaseCommon::analyze(); }
	void vacuum(void) { PGDbBaseCommon::vacuum(); }
	bool is_open(void) const { return PGDbBaseCommon::is_open(); }
	void set_exclusive(bool excl = true) { PGDbBaseCommon::set_exclusive(excl); }
	void purgedb(void);
	void interrupt(void) { PGDbBaseCommon::interrupt(); }
	element_t operator()(int64_t id, table_t tbl = element_t::table_main, unsigned int loadsubtables = element_t::subtables_all);
	element_t operator()(Address addr, unsigned int loadsubtables = element_t::subtables_all) { return operator()(addr.get_id(), addr.get_table(), loadsubtables); }
	void loadfirst(element_t& e, bool include_aux = true, unsigned int loadsubtables = element_t::subtables_all);
	void loadnext(element_t& e, bool include_aux = true, unsigned int loadsubtables = element_t::subtables_all);
	void save(element_t& e);
	void erase(element_t& e);
	void update_index(element_t& e);

	void for_each(ForEach& cb, bool include_aux = true, unsigned int loadsubtables = element_t::subtables_all);
	void for_each_by_rect(ForEach& cb, const Rect& r, bool include_aux = true, unsigned int loadsubtables = element_t::subtables_all);
	elementvector_t find_by_text(const std::string& pattern, char escape, comp_t comp = DbQueryInterface<T>::comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all);
	elementvector_t find_by_time(time_t fromtime, time_t totime, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all);
	elementvector_t find_by_rect(const Rect& r, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all);
	elementvector_t find_nearest(const Point& pt, const Rect& r, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all);
	elementvector_t find_nulltile(unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all);
	void load_subtables(element_t& el, unsigned int loadsubtables = element_t::subtables_all);
	void load_subtables(elementvector_t& ev, unsigned int loadsubtables = element_t::subtables_all);

protected:
	static const char *main_table_name;
	static const char *order_field;
	static const char *delete_field;
	static const bool area_data;

	void drop_tables(void);
	void create_tables(void);
	void drop_indices(void);
	void create_indices(void);

	void check_rtree(void);
	void check_readwrite(void);
	std::string get_area_select_string(pqxx::basic_transaction& w, const Rect& r, const char *sortcol = 0);
	elementvector_t find(const std::string& fldname, const std::string& pattern, char escape, comp_t comp = DbQueryInterface<T>::comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all);
	elementvector_t loadid(uint64_t startid = 0, table_t table = element_t::table_main, const std::string& order = "", unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all);
};

class NavaidsPGDb : public PGDbBase<DbBaseElements::Navaid> {
public:
	typedef element_t Navaid;

	NavaidsPGDb(pqxx::connection_base& conn) : PGDbBase<DbBaseElements::Navaid>(conn) {}
	NavaidsPGDb(pqxx::connection_base& conn, read_only_tag_t) : PGDbBase<DbBaseElements::Navaid>(conn, read_only) {}

	elementvector_t find_by_sourceid(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("SRCID", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_icao(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("ICAO", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_name(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("NAME", pattern, escape, comp, limit, loadsubtables); }
};

class WaypointsPGDb : public PGDbBase<DbBaseElements::Waypoint> {
public:
	typedef element_t Waypoint;

	WaypointsPGDb(pqxx::connection_base& conn) : PGDbBase<DbBaseElements::Waypoint>(conn) {}
	WaypointsPGDb(pqxx::connection_base& conn, read_only_tag_t) : PGDbBase<DbBaseElements::Waypoint>(conn, read_only) {}

	elementvector_t find_by_sourceid(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("SRCID", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_icao(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("ICAO", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_name(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("NAME", pattern, escape, comp, limit, loadsubtables); }
};

class AirwaysPGDb : public PGDbBase<DbBaseElements::Airway> {
public:
	typedef element_t Airway;

	AirwaysPGDb(pqxx::connection_base& conn) : PGDbBase<DbBaseElements::Airway>(conn) {}
	AirwaysPGDb(pqxx::connection_base& conn, read_only_tag_t) : PGDbBase<DbBaseElements::Airway>(conn, read_only) {}

	elementvector_t find_by_sourceid(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("SRCID", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_begin_name(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("BNAME", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_end_name(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("ENAME", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_name(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("NAME", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_area(const Rect& r, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all);

protected:
	std::string get_airways_area_select_string(pqxx::basic_transaction& w, const Rect& r, const char *sortcol = 0);
};

class AirportsPGDb : public PGDbBase<DbBaseElements::Airport> {
public:
	typedef element_t Airport;

	AirportsPGDb(pqxx::connection_base& conn) : PGDbBase<DbBaseElements::Airport>(conn) {}
	AirportsPGDb(pqxx::connection_base& conn, read_only_tag_t) : PGDbBase<DbBaseElements::Airport>(conn, read_only) {}

	elementvector_t find_by_sourceid(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("SRCID", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_icao(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("ICAO", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_name(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("NAME", pattern, escape, comp, limit, loadsubtables); }
};

class AirspacesPGDb : public PGDbBase<DbBaseElements::Airspace> {
public:
	typedef element_t Airspace;

	AirspacesPGDb(pqxx::connection_base& conn) : PGDbBase<DbBaseElements::Airspace>(conn) {}
	AirspacesPGDb(pqxx::connection_base& conn, read_only_tag_t) : PGDbBase<DbBaseElements::Airspace>(conn, read_only) {}

	elementvector_t find_by_sourceid(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("SRCID", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_icao(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("ICAO", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_name(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("NAME", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_ident(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("IDENT", pattern, escape, comp, limit, loadsubtables); }
};

class MapelementsPGDb : public PGDbBase<DbBaseElements::Mapelement> {
public:
	typedef element_t Mapelement;

	MapelementsPGDb(pqxx::connection_base& conn) : PGDbBase<DbBaseElements::Mapelement>(conn) {}
	MapelementsPGDb(pqxx::connection_base& conn, read_only_tag_t) : PGDbBase<DbBaseElements::Mapelement>(conn, read_only) {}

	elementvector_t find_by_name(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("NAME", pattern, escape, comp, limit, loadsubtables); }
};

class WaterelementsPGDb : public PGDbBase<DbBaseElements::Waterelement> {
public:
	typedef element_t Waterelement;

	WaterelementsPGDb(pqxx::connection_base& conn) : PGDbBase<DbBaseElements::Waterelement>(conn) {}
	WaterelementsPGDb(pqxx::connection_base& conn, read_only_tag_t) : PGDbBase<DbBaseElements::Waterelement>(conn, read_only) {}

	elementvector_t find_by_rect_type(const Rect& r, element_t::type_t typ, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all);
	elementvector_t find_by_type(element_t::type_t typ, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all);
	void purge(element_t::type_t typ);
};

class TracksPGDb : public PGDbBase<DbBaseElements::Track> {
public:
	typedef element_t Track;

	TracksPGDb(pqxx::connection_base& conn) : PGDbBase<DbBaseElements::Track>(conn) {}
	TracksPGDb(pqxx::connection_base& conn, read_only_tag_t) : PGDbBase<DbBaseElements::Track>(conn, read_only) {}

	void clear_appendlogs(void);
	void save_appendlog(element_t& e) { e.save_appendlog(m_conn, m_has_rtree); }
	elementvector_t find_by_sourceid(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("SRCID", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_fromicao(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("FROMICAO", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_fromname(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("FROMNAME", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_toicao(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("TOICAO", pattern, escape, comp, limit, loadsubtables); }
	elementvector_t find_by_toname(const std::string& pattern, char escape = 0, comp_t comp = comp_exact, unsigned int limit = 0, unsigned int loadsubtables = element_t::subtables_all) { return find("TONAME", pattern, escape, comp, limit, loadsubtables); }
};

/* a simple label database, used for computing label placements */
class LabelsPGDb : public PGDbBase<DbBaseElements::Label> {
public:
	typedef element_t Label;

	LabelsPGDb(pqxx::connection_base& conn) : PGDbBase<DbBaseElements::Label>(conn) {}
	LabelsPGDb(pqxx::connection_base& conn, read_only_tag_t) : PGDbBase<DbBaseElements::Label>(conn, read_only) {}

	elementvector_t find_mutable_by_metric(unsigned int limit = 0, unsigned int offset = 0, unsigned int loadsubtables = element_t::subtables_all);
};

#endif

template<int resolution, int tilesize>
class TopoDbN {
public:
	typedef unsigned int tile_index_t;
	typedef unsigned int pixel_index_t;
	typedef std::pair<tile_index_t,pixel_index_t> tile_pixel_index_t;
	static const pixel_index_t tile_size = tilesize;
	static const pixel_index_t pixels_per_tile = tilesize * tilesize;

	class BinFileHeader;
	class TopoCoordinate;

	class TopoTileCoordinate {
	public:
		static const uint16_t lon_tiles = 360 * 60 * 60 / resolution / tilesize;
		static const uint16_t lat_tiles = lon_tiles / 2 + 1;

		TopoTileCoordinate(uint16_t lon_tile = 0, uint16_t lat_tile = 0, uint16_t lon_offs = 0, uint16_t lat_offs = 0) : m_lon_tile(lon_tile), m_lon_offs(lon_offs), m_lat_tile(lat_tile), m_lat_offs(lat_offs) {}
		TopoTileCoordinate(const TopoCoordinate& tc);
		operator TopoCoordinate(void) const { return TopoCoordinate(m_lon_tile * tilesize + m_lon_offs, m_lat_tile * tilesize + m_lat_offs); }
		uint16_t get_lon_tile(void) const { return m_lon_tile; }
		uint16_t get_lon_offs(void) const { return m_lon_offs; }
		uint16_t get_lat_tile(void) const { return m_lat_tile; }
		uint16_t get_lat_offs(void) const { return m_lat_offs; }
		void set_lon_tile(uint16_t lon_tile) { m_lon_tile = lon_tile; }
		void set_lon_offs(uint16_t lon_offs) { m_lon_offs = lon_offs; }
		void set_lat_tile(uint16_t lat_tile) { m_lat_tile = lat_tile; }
		void set_lat_offs(uint16_t lat_offs) { m_lat_offs = lat_offs; }
		tile_index_t get_tile_index(void) const { return m_lat_tile * (tile_index_t)lon_tiles + m_lon_tile; }
		pixel_index_t get_pixel_index(void) const { return m_lat_offs * tilesize + m_lon_offs; }
		tile_pixel_index_t get_tile_pixel_index(void) const { return tile_pixel_index_t(get_tile_index(), get_pixel_index()); }
		void advance_tile_east(void);
		void advance_tile_west(void);
		void advance_tile_north(void);
		void advance_tile_south(void);

	private:
		uint16_t m_lon_tile;
		uint16_t m_lon_offs;
		uint16_t m_lat_tile;
		uint16_t m_lat_offs;
	};

	class TopoCoordinate {
	public:
		typedef uint32_t coord_t;
		typedef int32_t coords_t;
		TopoCoordinate(coord_t lon = 0, coord_t lat = 0) : m_lon(lon), m_lat(lat) {}
		TopoCoordinate(const Point& pt);
		operator Point(void) const;
		coord_t get_lon(void) const { return m_lon; }
		coord_t get_lat(void) const { return m_lat; }
		void set_lon(coord_t lon) { m_lon = lon; }
		void set_lat(coord_t lat) { m_lat = lat; }
		coord_t lon_dist(const TopoCoordinate& tc) const;
		coord_t lat_dist(const TopoCoordinate& tc) const;
		coords_t lon_dist_signed(const TopoCoordinate& tc) const;
		coords_t lat_dist_signed(const TopoCoordinate& tc) const;
		bool operator==(const TopoCoordinate &p) const { return m_lon == p.m_lon && m_lat == p.m_lat; }
		bool operator!=(const TopoCoordinate &p) const { return !operator==(p); }
#if 0
		TopoCoordinate operator+(const TopoCoordinate &p) const { return TopoCoordinate(m_lon + p.m_lon, m_lat + p.m_lat); }
		TopoCoordinate operator-(const TopoCoordinate &p) const { return TopoCoordinate(m_lon - p.m_lon, m_lat - p.m_lat); }
		TopoCoordinate operator-() const { return Point(-m_lon, -m_lat); }
		TopoCoordinate& operator+=(const TopoCoordinate &p) { m_lon += p.m_lon; m_lat += p.m_lat; return *this; }
		TopoCoordinate& operator-=(const TopoCoordinate &p) { m_lon -= p.m_lon; m_lat -= p.m_lat; return *this; }
		bool operator<(const TopoCoordinate &p) const { return m_lon < p.m_lon && m_lat < p.m_lat; }
		bool operator<=(const TopoCoordinate &p) const { return m_lon <= p.m_lon && m_lat <= p.m_lat; }
		bool operator>(const TopoCoordinate &p) const { return m_lon > p.m_lon && m_lat > p.m_lat; }
		bool operator>=(const TopoCoordinate &p) const { return m_lon >= p.m_lon && m_lat >= p.m_lat; }
#endif
		void advance_east(void);
		void advance_west(void);
		void advance_north(void);
		void advance_south(void);
		static Point get_pointsize(void) { return (Point)TopoCoordinate(1, 1) - (Point)TopoCoordinate(0, 0); }
		std::ostream& print(std::ostream& os);

	protected:
		coord_t m_lon;
		coord_t m_lat;

		static const coord_t from_point = 360 * 60 * 60 / resolution;
		static const uint64_t to_point = (((((uint64_t)1) << 63) + from_point / 2) / from_point) << 1;
	};

	typedef int16_t elev_t;
	typedef std::pair<elev_t,elev_t> minmax_elev_t;
	static const elev_t nodata;
	static const elev_t ocean;

	class Tile {
	public:
		typedef Glib::RefPtr<Tile> ptr_t;
		typedef Glib::RefPtr<const Tile> const_ptr_t;
		Tile(tile_index_t index);
		Tile(const Tile& tile);
		Tile(sqlite3x::sqlite3_connection& db, tile_index_t index);
		Tile(sqlite3x::sqlite3_cursor& cursor);
		Tile(const BinFileHeader& hdr, tile_index_t index);
		~Tile(void);
		void reference(void) const;
		void unreference(void) const;
		void load(sqlite3x::sqlite3_cursor& cursor);
		void save(sqlite3x::sqlite3_connection& db);
		tile_index_t get_index(void) const { return m_index; }
		elev_t get_elev(pixel_index_t index) const;
		void set_elev(pixel_index_t index, elev_t elev);
		elev_t get_minelev(void) const { if (is_minmaxinvalid()) update_minmax(); return m_minelev; }
		elev_t get_maxelev(void) const { if (is_minmaxinvalid()) update_minmax(); return m_maxelev; }
		bool is_dirty(void) const { return m_flags & flags_dirty; }
		guint get_lastaccess(void) const { return m_lastaccess; }
		void set_lastaccess(guint a) { m_lastaccess = a; }

	protected:
		mutable gint m_refcount;
		tile_index_t m_index;
		guint m_lastaccess;
		mutable elev_t m_minelev;
		mutable elev_t m_maxelev;
		uint8_t *m_elevdata;
		mutable uint8_t m_flags;

		typedef enum {
			flags_dirty         = 1 << 0,
			flags_loaded        = 1 << 1,
			flags_minmaxinvalid = 1 << 2,
			flags_dataowned     = 1 << 3
		} flags_t;

		void set_dirty(bool x) { if (x) m_flags |= flags_dirty; else m_flags &= ~flags_dirty; }
		bool is_loaded(void) const { return m_flags & flags_loaded; }
		void set_loaded(bool x) { if (x) m_flags |= flags_loaded; else m_flags &= ~flags_loaded; }
		bool is_minmaxinvalid(void) const { return m_flags & flags_minmaxinvalid; }
		void set_minmaxinvalid(bool x) const { if (x) m_flags |= flags_minmaxinvalid; else m_flags &= ~flags_minmaxinvalid; }
		bool is_dataowned(void) const { return m_flags & flags_dataowned; }
		void set_dataowned(bool x) { if (x) m_flags |= flags_dataowned; else m_flags &= ~flags_dataowned; }

		void update_minmax(void) const;
	};

	class RGBColor {
	public:
		RGBColor(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0) : m_col(0xff000000 | (r << 16) | (g << 8) | b) {}
		operator Gdk::Color(void) const;
		operator uint32_t(void) const { return m_col; }

	private:
		uint32_t m_col;
	};

	class ProfilePoint {
	public:
		ProfilePoint(double dist, elev_t elev, minmax_elev_t minmaxelev);
		ProfilePoint(double dist, elev_t elev, elev_t minelev, elev_t maxelev);
		ProfilePoint(double dist, elev_t elev);
		ProfilePoint(double dist = 0.);

		void scaledist(double ds);
		void adddist(double d);
		void add(elev_t elev);

		double get_dist(void) const { return m_dist; }
		elev_t get_elev(void) const { return m_elev; }
		elev_t get_minelev(void) const { return m_minelev; }
		elev_t get_maxelev(void) const { return m_maxelev; }

		void set_dist(double d) { m_dist = d; }
		void set_elev(elev_t e) { m_elev = e; }
		void set_minelev(elev_t e) { m_minelev = e; }
		void set_maxelev(elev_t e) { m_maxelev = e; }

	protected:
		double m_dist;
		elev_t m_elev;
		elev_t m_minelev;
		elev_t m_maxelev;
	};

	class Profile : public std::vector<ProfilePoint> {
	protected:
		typedef std::vector<ProfilePoint> base_t;

	public:
		double get_dist(void) const;
		elev_t get_minelev(void) const;
		elev_t get_maxelev(void) const;

		void scaledist(double ds);
		void adddist(double d);
	};

	class RouteProfilePoint : public ProfilePoint {
	public:
		RouteProfilePoint(const ProfilePoint& pp, unsigned int rteidx = 0, double rtedist = std::numeric_limits<double>::quiet_NaN());
		unsigned int get_routeindex(void) const { return m_routeindex; }
		double get_routedist(void) const { return m_routedist; }

		void set_routeindex(unsigned int i) { m_routeindex = i; }
		void set_routedist(double d) { m_routedist = d; }

	protected:
		uint16_t m_routeindex;
		double m_routedist;
	};

	class RouteProfile : public std::vector<RouteProfilePoint> {
	protected:
		typedef std::vector<RouteProfilePoint> base_t;
		class RouteDistCompare;

	public:
		double get_dist(void) const;
		elev_t get_minelev(void) const;
		elev_t get_maxelev(void) const;
		RouteProfilePoint interpolate(double rtedist) const;
	};

	TopoDbN();
	void take(sqlite3 *dbh);
	bool is_open(void) const { return !!m_db.db(); }
	sqlite3x::sqlite3_connection& get_db(void) { return m_db; }
	void close(void);
	void purgedb(void);
	void sync_off(void);
	void analyze(void);
	void vacuum(void);
	void interrupt(void);
	typename Tile::ptr_t find(tile_index_t index);
	void cache_commit(void);
	elev_t get_elev(const TopoTileCoordinate& tc);
	void set_elev(const TopoTileCoordinate& tc, elev_t elev);
	elev_t get_elev(const TopoCoordinate& pt) { return get_elev((TopoTileCoordinate)pt); }
	void set_elev(const TopoCoordinate& pt, elev_t elev) { set_elev((TopoTileCoordinate)pt, elev); }
	elev_t get_elev(const Point& pt) { return get_elev((TopoCoordinate)pt); }
	void set_elev(const Point& pt, elev_t elev) { set_elev((TopoCoordinate)pt, elev); }
	minmax_elev_t get_tile_minmax_elev(tile_index_t index);
	minmax_elev_t get_tile_minmax_elev(const TopoTileCoordinate& tc) { return get_tile_minmax_elev(tc.get_tile_index()); }
	minmax_elev_t get_minmax_elev(const Rect& p);
	minmax_elev_t get_minmax_elev(const PolygonSimple& p);
	minmax_elev_t get_minmax_elev(const PolygonHole& p);
	minmax_elev_t get_minmax_elev(const MultiPolygonHole& p);
	ProfilePoint get_profile(const Point& pt, double corridor_nmi);
	Profile get_profile(const Point& p0, const Point& p1, double corridor_nmi);
	RouteProfile get_profile(const FPlanRoute& fpl, double corridor_nmi);
	static RGBColor color(elev_t elev);
	static int m_to_ft(elev_t elev);

protected:
	class TileCache {
	public:
		TileCache(void);
		~TileCache();

		void clear(void);
		typename Tile::ptr_t find(tile_index_t index, sqlite3x::sqlite3_connection& db);
		void commit(sqlite3x::sqlite3_connection& db);

	protected:
		typedef std::map<tile_index_t, typename Tile::ptr_t> cache_t;
		cache_t m_cache;
		Glib::RWLock m_lock;
		guint m_lastaccess;
	};
	static const unsigned int tile_cache_size = 64;
	static const tile_index_t lon_tiles = 360 * 60 * 60 / resolution / tilesize;
	static const tile_index_t lat_tiles = lon_tiles / 2 + 1;
	static const tile_index_t nr_tiles = lon_tiles * lat_tiles;
	sqlite3x::sqlite3_connection m_db;
	const uint8_t *m_binhdr;
	size_t m_binsz;
	TileCache m_cache;

	void open(const Glib::ustring& path, const Glib::ustring& dbname);
	void open_readonly(const Glib::ustring& path, const Glib::ustring& dbname, const Glib::ustring& binname = "");
	void drop_tables(void);
	void create_tables(void);
	void drop_indices(void);
	void create_indices(void);

public:
	class BinFileHeader {
	public:
		class Tile {
		public:
			Tile(uint32_t offs = 0, elev_t minelev = nodata, elev_t maxelev = nodata);
			uint32_t get_offset(void) const;
			elev_t get_minelev(void) const;
			elev_t get_maxelev(void) const;
			void set_offset(uint32_t offs);
			void set_minelev(elev_t elev);
			void set_maxelev(elev_t elev);

		protected:
			uint8_t m_offset[4];
			uint8_t m_minelev[2];
			uint8_t m_maxelev[2];
		};

		BinFileHeader(void);
		bool check_signature(void) const;
		void set_signature(void);
		Tile& operator[](unsigned int idx);
		const Tile& operator[](unsigned int idx) const;

	protected:
		static const char signature[];
		static const Tile nulltile;
		char m_signature[32];
		Tile m_tiles[nr_tiles];
	};

	void close_bin(void);
	void open_bin(const Glib::ustring& dbname, const Glib::ustring& binname);
};

template<int resolution, int tilesize>
std::ostream& operator<<(std::ostream& os, const typename TopoDbN<resolution,tilesize>::TopoCoordinate& tc);

class TopoDb30 : public TopoDbN<30,480> {
private:
	typedef TopoDbN<30,480> TopoDbNN;

public:
	//typedef TopoDbNN::TopoCoordinate TopoCoordinate;
	//typedef TopoDbNN::elev_t elev_t;
	//static const elev_t nodata;
	void open(const Glib::ustring& path);
	void open_readonly(const Glib::ustring& path, bool enablebinfile = false);
};

const std::string& to_str(DbBaseElements::DbElementBase::table_t t);
inline std::ostream& operator<<(std::ostream& os, DbBaseElements::DbElementBase::table_t t) { return os << to_str(t); }

#endif /* _DBOBJ_H */
