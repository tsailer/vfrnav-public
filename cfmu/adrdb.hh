#ifndef ADRDB_H
#define ADRDB_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sqlite3x.hpp>

#include "sysdeps.h"
#include "adr.hh"
#include "adrtimetable.hh"

namespace ADR {

class Database {
public:
	Database(const std::string& path = "", bool enabinfile = true);
	~Database(void);
	void set_path(const std::string& path = "");
	void open(void);
	void close(void);
	Object::const_ptr_t load(const UUID& uuid);
	void save(const Object::const_ptr_t& p);
	unsigned int flush_cache(const Glib::TimeVal& tv = Glib::TimeVal(std::numeric_limits<long>::max(), 0));
	unsigned int clear_cache(void);
	void sync_off(void);
	void analyze(void);
	void vacuum(void);
	void create_transitive_closure(void);
	void delete_transitive_closure(void);
	void create_simple_transitive_closure(void);
	void delete_simple_transitive_closure(void);
	void create_dct_indices(void);
	void drop_dct_indices(void);
	void set_wal(bool wal = true);
	void set_exclusive(bool excl = true);
	sqlite3x::sqlite3_connection& get_db_connection(void) { return m_db; }

	typedef enum {
		comp_startswith = FPlan::comp_startswith,
		comp_contains = FPlan::comp_contains,
		comp_exact = FPlan::comp_exact,
		comp_exact_casesensitive,
		comp_like
	} comp_t;

	typedef std::vector<Link> findresults_t;

	typedef enum {
		loadmode_uuid,
		loadmode_obj,
		loadmode_link
	} loadmode_t;

	findresults_t find_all(loadmode_t loadmode = loadmode_obj,
			       uint64_t tmin = 0, uint64_t tmax = std::numeric_limits<uint64_t>::max(),
			       Object::type_t typmin = Object::type_first, Object::type_t typmax = Object::type_last,
			       unsigned int limit = 0);

	findresults_t find_by_ident(const std::string& ident, comp_t comp = comp_contains, char escape = 0, loadmode_t loadmode = loadmode_obj,
				    uint64_t tmin = 0, uint64_t tmax = std::numeric_limits<uint64_t>::max(),
				    Object::type_t typmin = Object::type_first, Object::type_t typmax = Object::type_last,
				    unsigned int limit = 0);

	findresults_t find_by_bbox(const Rect& bbox, loadmode_t loadmode = loadmode_obj,
				   uint64_t tmin = 0, uint64_t tmax = std::numeric_limits<uint64_t>::max(),
				   Object::type_t typmin = Object::type_first, Object::type_t typmax = Object::type_last,
				   unsigned int limit = 0);

	findresults_t find_dependson(const UUID& uuid, loadmode_t loadmode = loadmode_obj,
				     uint64_t tmin = 0, uint64_t tmax = std::numeric_limits<uint64_t>::max(),
				     Object::type_t typmin = Object::type_first, Object::type_t typmax = Object::type_last,
				     unsigned int limit = 0);

	findresults_t find_dependencies(const UUID& uuid, loadmode_t loadmode = loadmode_obj,
					uint64_t tmin = 0, uint64_t tmax = std::numeric_limits<uint64_t>::max(),
					Object::type_t typmin = Object::type_first, Object::type_t typmax = Object::type_last,
					unsigned int limit = 0);

	typedef std::pair<UUID,UUID> deppair_t;
	typedef std::vector<deppair_t> deppairs_t;

	deppairs_t find_modifiedafter(uint64_t tmod);

	Object::const_ptr_t load_temp(const UUID& uuid);
	void save_temp(const Object::const_ptr_t& p);

	findresults_t find_all_temp(loadmode_t loadmode = loadmode_obj);

	deppairs_t find_all_deps(void);
	deppairs_t find_all_temp_deps(void);

	void write_binfile(const std::string& fn);

	typedef std::vector<DctLeg> dctresults_t;

	dctresults_t find_dct_by_uuid(const UUID& uuid, loadmode_t loadmode = loadmode_obj);
	dctresults_t find_dct_by_bbox(const Rect& bbox, loadmode_t loadmode = loadmode_obj);
	void save_dct(const DctLeg& dl, bool tran = true);
	void erase_unref_dct(void);
	unsigned int count_dct(void);

protected:
	class ObjCacheEntry {
	public:
		ObjCacheEntry(const Object::const_ptr_t& obj = Object::const_ptr_t());
		ObjCacheEntry(const UUID& uuid);
		const Object::const_ptr_t& get_obj(void) const { return m_obj; }
		const Glib::TimeVal& get_tv(void) const { return m_tv; }
		gint get_refcount(void) const;
		void access(void) const;
		bool operator<(const ObjCacheEntry& x) const;

	protected:
		Object::const_ptr_t m_obj;
		mutable Glib::TimeVal m_tv;
	};

	template<int sz> class BinFileEntry {
	public:
		static const unsigned int size = sz;

		uint8_t readu8(unsigned int idx) const;
		uint16_t readu16(unsigned int idx) const;
		uint32_t readu24(unsigned int idx) const;
		uint32_t readu32(unsigned int idx) const;
		uint64_t readu64(unsigned int idx) const;
		int8_t reads8(unsigned int idx) const { return readu8(idx); }
		int16_t reads16(unsigned int idx) const { return readu16(idx); }
		int32_t reads24(unsigned int idx) const { return readu24(idx); }
		int32_t reads32(unsigned int idx) const { return readu32(idx); }
		int64_t reads64(unsigned int idx) const { return readu64(idx); }

		void writeu8(unsigned int idx, uint8_t v);
		void writeu16(unsigned int idx, uint16_t v);
		void writeu24(unsigned int idx, uint32_t v);
		void writeu32(unsigned int idx, uint32_t v);
		void writeu64(unsigned int idx, uint64_t v);
		void writes8(unsigned int idx, int8_t v) { writeu8(idx, v); }
		void writes16(unsigned int idx, int16_t v) { writeu16(idx, v); }
		void writes24(unsigned int idx, int32_t v) { writeu24(idx, v); }
		void writes32(unsigned int idx, int32_t v) { writeu32(idx, v); }
		void writes64(unsigned int idx, int64_t v) { writeu64(idx, v); }

	protected:
		uint8_t m_data[sz];
	};

	class BinFileHeader : public BinFileEntry<64> {
	public:
		bool check_signature(void) const;
		void set_signature(void);

		uint64_t get_objdiroffs(void) const;
		void set_objdiroffs(uint64_t offs);
		uint32_t get_objdirentries(void) const;
		void set_objdirentries(uint32_t n);

	protected:
		static const char signature[];
	};

	class BinFileObjEntry : public BinFileEntry<64> {
	public:
		const UUID& get_uuid(void) const;
		void set_uuid(const UUID& uuid);
		Rect get_bbox(void) const;
		void set_bbox(const Rect& bbox);
		timetype_t get_mintime(void) const;
		void set_mintime(timetype_t t);
		timetype_t get_maxtime(void) const;
		void set_maxtime(timetype_t t);
		timetype_t get_modified(void) const;
		void set_modified(timetype_t t);
		Object::type_t get_type(void) const;
		void set_type(Object::type_t t);
		uint64_t get_dataoffs(void) const;
		void set_dataoffs(uint64_t offs);
		uint32_t get_datasize(void) const;
		void set_datasize(uint32_t sz);
	};

	class BinFileFindObj;

	std::string m_path;
	sqlite3x::sqlite3_connection m_db;
	typedef std::set<ObjCacheEntry> cache_t;
	cache_t m_cache;
	const uint8_t *m_binfile;
	uint64_t m_binsize;
	bool m_enablebinfile;
	bool m_tempdb;

	const Object::const_ptr_t& cache_get(const UUID& uuid) const;
	void cache_put(const Object::const_ptr_t& p);
	void open_binfile(void);
	void close_binfile(void);
	bool is_binfile(void) const { return !!m_binfile; }
	static void dbfunc_upperbound(sqlite3_context *ctxt, int, sqlite3_value **values);
	void open_temp(void);
	static std::string quote_text_for_like(const std::string& s, char e);
	findresults_t find_tail(sqlite3x::sqlite3_command& cmd, loadmode_t loadmode, bool cache);
	dctresults_t dct_tail(sqlite3x::sqlite3_command& cmd, loadmode_t loadmode);
	Object::ptr_t load_binfile(const UUID& uuid);
	Object::ptr_t load_binfile(const BinFileObjEntry *oi);
	findresults_t find_all_binfile(loadmode_t loadmode, uint64_t tmin, uint64_t tmax,
				       Object::type_t typmin, Object::type_t typmax, unsigned int limit);
	findresults_t find_by_bbox_binfile(const Rect& bbox, loadmode_t loadmode, uint64_t tmin, uint64_t tmax,
					   Object::type_t typmin, Object::type_t typmax, unsigned int limit);
};

};

extern const std::string& to_str(ADR::Database::comp_t c);
inline std::ostream& operator<<(std::ostream& os, ADR::Database::comp_t c) { return os << to_str(c); }

#endif /* ADRDB_H */
