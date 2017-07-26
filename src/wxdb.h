//
// C++ Interface: wxdb
//
// Description: Weather Objects Database
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef _WXDB_H
#define _WXDB_H

#include <iostream>
#include <list>
#include <set>
#include <glibmm.h>
#include <gdkmm.h>
#include <sqlite3x.hpp>
#include "geom.h"

class WXDatabase {
  public:
	class MetarTaf {
	  public:
		typedef enum {
			type_metar,
			type_taf
		} type_t;

		MetarTaf(const std::string& icao = "", type_t t = type_metar, time_t epoch = 0, const std::string& msg = "", const Point& pt = Point::invalid);
		void load(sqlite3x::sqlite3_cursor& cursor, sqlite3x::sqlite3_connection& db);
		void save(sqlite3x::sqlite3_connection& db) const;
		void erase(sqlite3x::sqlite3_connection& db);
		bool operator<(const MetarTaf& m) const;
		bool is_valid(void) const { return m_epoch > 0; }
		int64_t get_id(void) const { return m_id; }
		bool is_metar(void) const { return m_type == type_metar; }
		bool is_taf(void) const { return m_type == type_taf; }
		const std::string& get_icao(void) const { return m_icao; }
		const std::string& get_message(void) const { return m_message; }
		Point get_coord(void) const { return m_coord; }
		time_t get_epoch(void) const { return m_epoch; }
		type_t get_type(void) const { return m_type; }
		static void remove_linefeeds(std::string& m);
		void remove_message_linefeeds(void) { remove_linefeeds(m_message); }

	  protected:
		mutable int64_t m_id;
		std::string m_icao;
		std::string m_message;
		Point m_coord;
		time_t m_epoch;
		type_t m_type;

		friend class WXDatabase;
		void save_notran(sqlite3x::sqlite3_connection& db) const;
	};

	class Chart {
	  public:
		Chart(const std::string& dep = "", const std::string& dest = "", const std::string& desc = "",
		      time_t createtime = 0, time_t epoch = 0, unsigned int level = ~0U, unsigned int type = 0,
		      const std::string& mimetype = "", const std::string& data = "");
		void load(sqlite3x::sqlite3_cursor& cursor, sqlite3x::sqlite3_connection& db);
		void save(sqlite3x::sqlite3_connection& db) const;
		void erase(sqlite3x::sqlite3_connection& db);
		bool operator<(const Chart& c) const;
		bool is_valid(void) const { return m_epoch > 0; }
		int64_t get_id(void) const { return m_id; }
		const std::string& get_departure(void) const { return m_departure; }
		const std::string& get_destination(void) const { return m_destination; }
		const std::string& get_description(void) const { return m_description; }
		const std::string& get_mimetype(void) const { return m_mimetype; }
		const std::string& get_data(void) const { return m_data; }
		time_t get_createtime(void) const { return m_createtime; }
		time_t get_epoch(void) const { return m_epoch; }
		unsigned int get_level(void) const { return m_level; }
		unsigned int get_type(void) const { return m_type; }

	  protected:
		mutable int64_t m_id;
		std::string m_departure;
		std::string m_destination;
		std::string m_description;
		std::string m_mimetype;
		std::string m_data;
		time_t m_createtime;
		time_t m_epoch;
		unsigned int m_level;
		unsigned int m_type;

		friend class WXDatabase;
		void save_notran(sqlite3x::sqlite3_connection& db) const;
	};

	WXDatabase();
	void open(const Glib::ustring& path, const Glib::ustring& dbname);
	void take(sqlite3 *dbh);
	void close(void);
	void purgedb(void);
	void sync_off(void);
	void analyze(void);
	void vacuum(void);
	void interrupt(void);
	void expire_metartaf(time_t cutoffepoch);
	void expire_chart(time_t cutoffepoch);
	typedef std::set<MetarTaf> metartafresult_t;
	typedef std::set<Chart> chartresult_t;
	typedef std::vector<MetarTaf> metartafvector_t;
	typedef std::vector<Chart> chartvector_t;
	void save(const MetarTaf& mt);
	void save(const Chart& ch);
	template <typename T> void save(T b, T e);
	void loadfirst(MetarTaf& mt);
	void loadnext(MetarTaf& mt);
	void loadfirst(Chart& ch);
	void loadnext(Chart& ch);
	metartafvector_t loadid_metartaf(uint64_t id, const std::string& order = "ID", unsigned int limit = 1);
	chartvector_t loadid_chart(uint64_t id, const std::string& order = "ID", unsigned int limit = 1);
	metartafresult_t find_metartaf(const Rect& bbox, time_t metarcutoff = 0, time_t tafcutoff = 0, unsigned int limit = 0);
	chartresult_t find_chart(time_t cutoff = 0, unsigned int limit = 0);

  protected:
	sqlite3x::sqlite3_connection m_db;

	void drop_tables(void);
	void create_tables(void);
	void drop_indices(void);
	void create_indices(void);
};

#endif /* _WXDB_H */
