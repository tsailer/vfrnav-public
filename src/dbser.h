//
// C++ Interface: dbser
//
// Description:
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2012, 2015
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef _DBSER_H
#define _DBSER_H

#include <set>
#include <stack>

class DbBaseCommon::DbSchemaArchiver {
public:
	DbSchemaArchiver(const char *main_table_name);
	inline bool is_load(void) const { return false; }
	inline bool is_save(void) const { return false; }
	inline bool is_schema(void) const { return true; }
	void ioxmlstart(const char *name) {}
	void ioxmlend(void) {}
	void iosqlindex(const Glib::ustring& name, const Glib::ustring& flds, bool legacy = false) { add_index(name, flds, DbBaseElements::DbElementBase::hibernate_sqlindex, legacy); }
	void io(Glib::ustring& v, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "TEXT", flags);
	}
	void io(char& v, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "CHARACTER", flags);
	}
	void io(int64_t& v, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "INTEGER", flags);
	}
	void io(uint64_t& v, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "INTEGER", flags);
	}
	void io(int32_t& v, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "INTEGER", flags);
	}
	void io(uint32_t& v, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "INTEGER", flags);
	}
	void io(int16_t& v, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "INTEGER", flags);
	}
	void io(uint16_t& v, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "INTEGER", flags);
	}
	void io(uint8_t& v, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "INTEGER", flags);
	}
	void io(bool& v, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "INTEGER", flags);
	}
	void io(double& v, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "REAL", flags);
	}
	void io(DbBaseElements::Navaid::navaid_type_t& v, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "INTEGER", flags);
	}
	void io(DbBaseElements::Airway::airway_type_t& v, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "INTEGER", flags);
	}
	void io(DbBaseElements::DbElementBase::table_t& v, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "INTEGER", flags);
	}
	void io(DbBaseElements::Airport::PointFeature::feature_t& v, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "INTEGER", flags);
	}
	void io(DbBaseElements::Airport::VFRRoute::VFRRoutePoint::pathcode_t& v, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "CHARACTER", flags);
	}
	void io(DbBaseElements::Airport::VFRRoute::VFRRoutePoint::altcode_t& v, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "CHARACTER", flags);
	}
	void io(DbBaseElements::Airport::PolylineNode::feature_t& v, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "INTEGER", flags);
	}
	void io(DbBaseElements::DbElementLabelBase::label_placement_t& v, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "INTEGER", flags);
	}
	void io(DbBaseElements::Waterelement::type_t& t, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "INTEGER", flags);
	}
	void io(Point& pt, const char *latname, const char *lonname, const char *latxmlname, const char *lonxmlname, unsigned int flags = 0);
	void io(Rect& r, unsigned int flags = 0);
	void io(uint8_t& tc, char& cls, const char *tcname, const char *clsname, const char *xmlname, unsigned int flags = 0);
	void io(DbBaseElements::Airspace::Component::operator_t& opr, const char *name, const char *xmlname, unsigned int flags = 0);
	void iotime(time_t& v, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "INTEGER", flags);
	}

	template<typename T> void io(std::vector<T>&, const char *name, const char *superid, const char *xmlname, unsigned int flags = 0) {
		push_subtable(name, superid);
		T e;
		e.hibernate(*this);
		pop_subtable();
	}
	void io(PolygonSimple&, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "BLOB", flags);
	}
	void io(PolygonHole&, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "BLOB", flags);
	}
	void io(MultiPolygonHole&, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "BLOB", flags);
	}
	void io(DbBaseElements::Airport::Polyline&, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "BLOB", flags);
	}
	void io(std::vector<DbBaseElements::Track::TrackPoint>&, const char *name, const char *xmlname, unsigned int flags = 0) {
		add_key(name, "BLOB", flags);
	}

	std::ostream& print_schema(std::ostream& os) const;
	void verify_schema(sqlite3x::sqlite3_connection& db) const;

protected:
	class Table {
	  public:
		class Field {
		  public:
		        Field(const Glib::ustring& name, const Glib::ustring& type, unsigned int flags, unsigned int fieldnr) : m_name(name), m_type(type), m_flags(flags), m_fieldnr(fieldnr) {}
			const Glib::ustring& get_name(void) const { return m_name; }
			const Glib::ustring& get_type(void) const { return m_type; }
			Glib::ustring get_typeattr(void) const;
			unsigned int get_flags(void) const { return m_flags; }
			unsigned int get_fieldnr(void) const { return m_fieldnr; }
			bool operator<(const Field& f) const { return get_name() < f.get_name(); }

		  protected:
			Glib::ustring m_name;
			Glib::ustring m_type;
			unsigned int m_flags;
			unsigned int m_fieldnr;
		};

		class SortFieldsByNr {
		  public:
			bool operator()(const Field& f1, const Field& f2) const { return f1.get_fieldnr() < f2.get_fieldnr(); }
		};

	  protected:
		typedef std::set<Field> fields_t;

	  public:
	        Table(const Glib::ustring& name) : m_name(name), m_fieldnr(0) {}
		Table(const std::vector<std::string>& tok);
		const Glib::ustring& get_name(void) const { return m_name; }
		bool operator<(const Table& t) const { return get_name() < t.get_name(); }
		void add_field(const Glib::ustring& name, const Glib::ustring& type, unsigned int flags);

		typedef fields_t::const_iterator const_iterator;
		const_iterator begin(void) const { return m_fields.begin(); }
		const_iterator end(void) const { return m_fields.end(); }
		const_iterator find(const Glib::ustring& name) const { return m_fields.find(Field(name, "", 0, 0)); }

		std::string create_sql(void) const;
		void create_sql(sqlite3x::sqlite3_connection& db) const;
		std::string drop_sql(void) const;
		void drop_sql(sqlite3x::sqlite3_connection& db) const;

	  protected:
		Glib::ustring m_name;
		fields_t m_fields;
		unsigned int m_fieldnr;
	};

	class Index {
	  public:
	        Index(const Glib::ustring& name, const Glib::ustring& table, const Glib::ustring& fields, unsigned int flags) : m_name(name), m_table(table), m_fields(fields), m_flags(flags) {}
	        Index(const std::vector<std::string>& tok);
		const Glib::ustring& get_name(void) const { return m_name; }
		const Glib::ustring& get_table(void) const { return m_table; }
		const Glib::ustring& get_fields(void) const { return m_fields; }
		Glib::ustring get_fieldattr(void) const;
		unsigned int get_flags(void) const { return m_flags; }
		bool operator<(const Index& i) const { return get_name() < i.get_name(); }

		std::string create_sql(void) const;
		void create_sql(sqlite3x::sqlite3_connection& db) const;
		std::string drop_sql(void) const;
		void drop_sql(sqlite3x::sqlite3_connection& db) const;

	  protected:
		Glib::ustring m_name;
		Glib::ustring m_table;
		Glib::ustring m_fields;
		unsigned int m_flags;
	};

	typedef std::set<Table> tables_t;
	tables_t m_tables;
	Table *m_currenttable;
	typedef std::stack<Glib::ustring> tablestack_t;
	tablestack_t m_tablestack;
	typedef std::vector<Glib::ustring> superidstack_t;
	superidstack_t m_superidstack;
	typedef std::set<Index> indices_t;
	indices_t m_indices;
	indices_t m_legacyindices;

	void add_key(const Glib::ustring& key, const Glib::ustring& type, unsigned int flags);
	void add_index(const Glib::ustring& name, const Glib::ustring& flds, unsigned int flags, bool legacy = false);
	void push_subtable(const Glib::ustring& subtblname, const Glib::ustring& superid);
	void pop_subtable(void);
	static std::vector<std::string> tokenize(const std::string& s);
	static std::string untokenize(const std::vector<std::string>& s, bool suppressspace = true);
	static bool is_label(char ch);
	template<typename T> static bool is_label(T b, T e) { for (; b != e; ++b) if (!is_label(*b)) return false; return true; }
	static bool is_label(const std::string& s) { return is_label(s.begin(), s.end()); }
	static bool strcompare(const Glib::ustring& s, const char *s1);
	static bool strcompare(const std::vector<std::string>& tok, std::vector<std::string>::size_type idx, const char *s1);
};

#endif /* _DBSER_H */
