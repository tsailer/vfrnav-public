#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/name_generator.hpp>
#include <iomanip>
#include <sstream>
#include <fstream>

#if defined(HAVE_WINDOWS_H)
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif

#include "adrdb.hh"
#include "adrhibernate.hh"

using namespace ADR;

Database::ObjCacheEntry::ObjCacheEntry(const Object::const_ptr_t& obj)
	: m_obj(obj), m_tv(-1, 0)
{
}

Database::ObjCacheEntry::ObjCacheEntry(const UUID& uuid)
	: m_tv(-1, 0)
{
	Object::const_ptr_t p(new ObjectImpl<TimeSlice,Object::type_invalid>(uuid));
	m_obj.swap(p);
}

gint Database::ObjCacheEntry::get_refcount(void) const
{
	if (m_obj)
		return m_obj->get_refcount();
	return 0;
}

void Database::ObjCacheEntry::access(void) const
{
	m_tv.assign_current_time();
}

bool Database::ObjCacheEntry::operator<(const ObjCacheEntry& x) const
{
	if (!x.m_obj)
		return false;
	if (!m_obj)
		return true;
	return m_obj->get_uuid() < x.m_obj->get_uuid();
}

template<int sz> const unsigned int Database::BinFileEntry<sz>::size;

template<int sz> uint8_t Database::BinFileEntry<sz>::readu8(unsigned int idx) const
{
	if (idx >= size)
		return 0;
	return m_data[idx];
}

template<int sz> uint16_t Database::BinFileEntry<sz>::readu16(unsigned int idx) const
{
	if (idx + 1U >= size)
		return 0;
	return m_data[idx] | (m_data[idx + 1U] << 8);
}

template<int sz> uint32_t Database::BinFileEntry<sz>::readu24(unsigned int idx) const
{
	if (idx + 2U >= size)
		return 0;
	return m_data[idx] | (m_data[idx + 1U] << 8) | (m_data[idx + 2U] << 16);
}

template<int sz> uint32_t Database::BinFileEntry<sz>::readu32(unsigned int idx) const
{
	if (idx + 3U >= size)
		return 0;
	return m_data[idx] | (m_data[idx + 1U] << 8) | (m_data[idx + 2U] << 16) | (m_data[idx + 3U] << 24);
}

template<int sz> uint64_t Database::BinFileEntry<sz>::readu64(unsigned int idx) const
{
	if (idx + 7U >= size)
		return 0;
	uint64_t r(m_data[idx + 4U] | (m_data[idx + 5U] << 8) | (m_data[idx + 6U] << 16) | (m_data[idx + 7U] << 24));
	r <<= 32;
	r |= (m_data[idx] | (m_data[idx + 1U] << 8) | (m_data[idx + 2U] << 16) | (m_data[idx + 3U] << 24));
	return r;
}

template<int sz> void Database::BinFileEntry<sz>::writeu8(unsigned int idx, uint8_t v)
{
	if (idx >= size)
		return;
	m_data[idx] = v;
}

template<int sz> void Database::BinFileEntry<sz>::writeu16(unsigned int idx, uint16_t v)
{
	if (idx + 1U >= size)
		return;
	m_data[idx] = v;
	m_data[idx + 1U] = v >> 8;
}

template<int sz> void Database::BinFileEntry<sz>::writeu24(unsigned int idx, uint32_t v)
{
	if (idx + 2U >= size)
		return;
	m_data[idx] = v;
	m_data[idx + 1U] = v >> 8;
	m_data[idx + 2U] = v >> 16;
}

template<int sz> void Database::BinFileEntry<sz>::writeu32(unsigned int idx, uint32_t v)
{
	if (idx + 3U >= size)
		return;
	m_data[idx] = v;
	m_data[idx + 1U] = v >> 8;
	m_data[idx + 2U] = v >> 16;
	m_data[idx + 3U] = v >> 24;
}

template<int sz> void Database::BinFileEntry<sz>::writeu64(unsigned int idx, uint64_t v)
{
	if (idx + 7U >= size)
		return;
	m_data[idx] = v;
	m_data[idx + 1U] = v >> 8;
	m_data[idx + 2U] = v >> 16;
	m_data[idx + 3U] = v >> 24;
	m_data[idx + 4U] = v >> 32;
	m_data[idx + 5U] = v >> 40;
	m_data[idx + 6U] = v >> 48;
	m_data[idx + 7U] = v >> 56;
}

const ADR::UUID& Database::BinFileObjEntry::get_uuid(void) const
{
	return *(const ADR::UUID *)m_data;
}

void Database::BinFileObjEntry::set_uuid(const UUID& uuid)
{
	memcpy(&m_data[0], &uuid, 16);
}

Rect Database::BinFileObjEntry::get_bbox(void) const
{
	return Rect(Point(reads32(16), reads32(20)), Point(reads32(24), reads32(28)));
}

void Database::BinFileObjEntry::set_bbox(const Rect& bbox)
{
	writes32(16, bbox.get_southwest().get_lon());
	writes32(20, bbox.get_southwest().get_lat());
	writes32(24, bbox.get_northeast().get_lon());
	writes32(28, bbox.get_northeast().get_lat());
}

timetype_t Database::BinFileObjEntry::get_mintime(void) const
{
	return readu64(32);
}

void Database::BinFileObjEntry::set_mintime(timetype_t t)
{
	writeu64(32, t);
}

timetype_t Database::BinFileObjEntry::get_maxtime(void) const
{
	return readu64(40);
}

void Database::BinFileObjEntry::set_maxtime(timetype_t t)
{
	writeu64(40, t);
}

timetype_t Database::BinFileObjEntry::get_modified(void) const
{
	return readu64(48);
}

void Database::BinFileObjEntry::set_modified(timetype_t t)
{
	writeu64(48, t);
}

Object::type_t Database::BinFileObjEntry::get_type(void) const
{
	return (Object::type_t)readu8(63);
}

void Database::BinFileObjEntry::set_type(Object::type_t t)
{
	writeu8(63, (uint8_t)t);
}

uint64_t Database::BinFileObjEntry::get_dataoffs(void) const
{
	return readu32(56);
}

void Database::BinFileObjEntry::set_dataoffs(uint64_t offs)
{
	writeu32(56, offs);
}

uint32_t Database::BinFileObjEntry::get_datasize(void) const
{
	return readu24(60);
}

void Database::BinFileObjEntry::set_datasize(uint32_t sz)
{
	writeu24(60, sz);
}

const char Database::BinFileHeader::signature[] = "vfrnav ADR objects V1\n";

bool Database::BinFileHeader::check_signature(void) const
{
	return !memcmp(m_data, signature, sizeof(signature));
}

void Database::BinFileHeader::set_signature(void)
{
	memset(m_data, 0, 32);
	memset(m_data + 44, 0, 20);
	memcpy(m_data, signature, sizeof(signature));
}

uint64_t Database::BinFileHeader::get_objdiroffs(void) const
{
	return readu64(32);
}

void Database::BinFileHeader::set_objdiroffs(uint64_t offs)
{
	writeu64(32, offs);
}

uint32_t Database::BinFileHeader::get_objdirentries(void) const
{
	return readu32(40);
}

void Database::BinFileHeader::set_objdirentries(uint32_t n)
{
	writeu32(40, n);
}

Database::Database(const std::string& path, bool enabinfile)
	: m_path(path), m_binfile(0), m_binsize(0), m_enablebinfile(enabinfile), m_tempdb(false)
{
	if (m_path.empty())
		m_path = PACKAGE_DATA_DIR;
}

Database::~Database(void)
{
	close();
}

void Database::set_path(const std::string& path)
{
	if (path == m_path)
		return;
	close();
	m_path = path;
	open();
}

const Object::const_ptr_t& Database::cache_get(const UUID& uuid) const
{
	static const Object::const_ptr_t noobj;
	ObjCacheEntry ce(uuid);
	cache_t::const_iterator i(m_cache.find(ce));
	if (i != m_cache.end())
		return i->get_obj();
	return noobj;
}

void Database::cache_put(const Object::const_ptr_t& p)
{
	if (!p || p->get_uuid().is_nil())
		return;
	std::pair<cache_t::iterator, bool> ins(m_cache.insert(ObjCacheEntry(p)));
	ins.first->access();
}

Object::const_ptr_t Database::load(const UUID& uuid)
{
	if (uuid.is_nil())
		return Object::const_ptr_t();
	{
		Object::const_ptr_t p(cache_get(uuid));
		if (p)
			return p;
	}
	open();
	{
		Object::const_ptr_t p(load_binfile(uuid));
		if (p)
			return p;
	}
	Object::ptr_t p;
        {
		sqlite3x::sqlite3_command cmd(m_db, "SELECT DATA,MODIFIED FROM obj WHERE UUID0=? AND UUID1=? AND UUID2=? AND UUID3=?;");
		for (unsigned int i = 0; i < 4; ++i)
			cmd.bind(i + 1, (long long int)uuid.get_word(i));
		sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
		while (cursor.step()) {
			if (cursor.isnull(0))
				continue;
			int sz;
			uint8_t const *data((uint8_t const *)cursor.getblob(0, sz));
			if (sz < 1)
				continue;
			ArchiveReadBuffer ar(data, data + sz);
			try {
				p = Object::create(ar, uuid);
			} catch (const std::exception& e) {
				if (false)
					throw;
				std::ostringstream oss;
				oss << e.what() << "; blob" << std::hex;
				for (const uint8_t *p0(data), *p1(data + sz); p0 != p1; ++p0)
					oss << ' ' << std::setfill('0') << std::setw(2) << (unsigned int)*p0;
				throw std::runtime_error(oss.str());
			}
			if (p) {
				p->set_modified(cursor.getint(1));
				p->unset_dirty();
			}
		}
	}
	cache_put(p);
	return p;
}

void Database::save(const Object::const_ptr_t& p)
{
	if (!p)
		return;
	cache_put(p);
	open();
	std::ostringstream blob;
	{
		ArchiveWriteStream ar(blob);
		p->save(ar);
	}
	sqlite3x::sqlite3_transaction tran(m_db);
	{
		sqlite3x::sqlite3_command cmd(m_db, "INSERT OR REPLACE INTO obj"
					      " (UUID0,UUID1,UUID2,UUID3,TYPE,SWLAT,NELAT,SWLON,NELON,MINTIME,MAXTIME,MODIFIED,DATA)"
					      " VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?);");
		for (unsigned int i = 0; i < 4; ++i)
			cmd.bind(i + 1, (long long int)p->get_uuid().get_word(i));
		cmd.bind(5, (int)p->get_type());
		{
			Rect bbox;
			p->get_bbox(bbox);
			cmd.bind(6, bbox.get_south());
			cmd.bind(7, bbox.get_north());
			cmd.bind(8, bbox.get_west());
			cmd.bind(9, (long long int)bbox.get_east_unwrapped());
		}
		{
			std::pair<uint64_t,uint64_t> r(p->get_timebounds());
			cmd.bind(10, (long long int)std::min(r.first, (uint64_t)std::numeric_limits<int64_t>::max()));
			cmd.bind(11, (long long int)std::min(r.second, (uint64_t)std::numeric_limits<int64_t>::max()));
		}
		cmd.bind(12, (long long int)p->get_modified());
		cmd.bind(13, blob.str().c_str(), blob.str().size());
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "DELETE FROM dep WHERE UUID0=? AND UUID1=? AND UUID2=? AND UUID3=?;");
		for (unsigned int i = 0; i < 4; ++i)
			cmd.bind(i + 1, (long long int)p->get_uuid().get_word(i));
		cmd.executenonquery();
	}
	{
		UUID::set_t deps;
		p->dependencies(deps);
		for (UUID::set_t::const_iterator di(deps.begin()), de(deps.end()); di != de; ++di) {
			if (di->is_nil())
				continue;
			sqlite3x::sqlite3_command cmd(m_db, "INSERT INTO dep"
						      " (UUID0,UUID1,UUID2,UUID3,UUIDD0,UUIDD1,UUIDD2,UUIDD3)"
						      " VALUES (?,?,?,?,?,?,?,?);");
			for (unsigned int i = 0; i < 4; ++i)
				cmd.bind(i + 1, (long long int)p->get_uuid().get_word(i));
			for (unsigned int i = 0; i < 4; ++i)
				cmd.bind(i + 5, (long long int)di->get_word(i));
			cmd.executenonquery();
		}
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "DELETE FROM ident WHERE UUID0=? AND UUID1=? AND UUID2=? AND UUID3=?;");
		for (unsigned int i = 0; i < 4; ++i)
			cmd.bind(i + 1, (long long int)p->get_uuid().get_word(i));
		cmd.executenonquery();
	}
	{
		typedef std::set<std::string> names_t;
		names_t names;
		for (unsigned int i(0), n(p->size()); i < n; ++i) {
			const IdentTimeSlice& ts(p->operator[](i).as_ident());
			if (!ts.is_valid())
				continue;
			if (ts.get_ident().empty())
				continue;
			names.insert(ts.get_ident());
		}
		for (names_t::const_iterator ni(names.begin()), ne(names.end()); ni != ne; ++ni) {
			sqlite3x::sqlite3_command cmd(m_db, "INSERT INTO ident"
						      " (UUID0,UUID1,UUID2,UUID3,IDENT)"
						      " VALUES (?,?,?,?,?);");
			for (unsigned int i = 0; i < 4; ++i)
				cmd.bind(i + 1, (long long int)p->get_uuid().get_word(i));
			cmd.bind(5, *ni);
			cmd.executenonquery();
		}
	}
	tran.commit();
	p->unset_dirty();	
}

Object::const_ptr_t Database::load_temp(const UUID& uuid)
{
	if (!m_tempdb)
		return Object::const_ptr_t();
	Object::ptr_t p;
	{
		sqlite3x::sqlite3_command cmd(m_db, "SELECT DATA,MODIFIED FROM tmpobj WHERE UUID0=? AND UUID1=? AND UUID2=? AND UUID3=?;");
		for (unsigned int i = 0; i < 4; ++i)
			cmd.bind(i + 1, (long long int)uuid.get_word(i));
		sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
		while (cursor.step()) {
			if (cursor.isnull(0))
				continue;
			int sz;
			uint8_t const *data((uint8_t const *)cursor.getblob(0, sz));
			if (sz < 1)
				continue;
			ArchiveReadBuffer ar(data, data + sz);
			try {
				p = Object::create(ar, uuid);
			} catch (const std::exception& e) {
				if (false)
					throw;
				std::ostringstream oss;
				oss << e.what() << "; blob" << std::hex;
				for (const uint8_t *p0(data), *p1(data + sz); p0 != p1; ++p0)
					oss << ' ' << std::setfill('0') << std::setw(2) << (unsigned int)*p0;
				throw std::runtime_error(oss.str());
			}
			if (p) {
				p->set_modified(cursor.getint(1));
				p->unset_dirty();
			}
		}
	}
	return p;
}

void Database::save_temp(const Object::const_ptr_t& p)
{
	if (!p)
		return;
	open_temp();
	std::ostringstream blob;
	{
		ArchiveWriteStream ar(blob);
		p->save(ar);
	}
	sqlite3x::sqlite3_transaction tran(m_db);
	{
		sqlite3x::sqlite3_command cmd(m_db, "INSERT OR REPLACE INTO tmpobj"
					      " (UUID0,UUID1,UUID2,UUID3,MODIFIED,DATA)"
					      " VALUES (?,?,?,?,?,?);");
		for (unsigned int i = 0; i < 4; ++i)
			cmd.bind(i + 1, (long long int)p->get_uuid().get_word(i));
		cmd.bind(5, (long long int)p->get_modified());
		cmd.bind(6, blob.str().c_str(), blob.str().size());
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "DELETE FROM tmpdep WHERE UUID0=? AND UUID1=? AND UUID2=? AND UUID3=?;");
		for (unsigned int i = 0; i < 4; ++i)
			cmd.bind(i + 1, (long long int)p->get_uuid().get_word(i));
		cmd.executenonquery();
	}
	{
		UUID::set_t deps;
		p->dependencies(deps);
		for (UUID::set_t::const_iterator di(deps.begin()), de(deps.end()); di != de; ++di) {
			if (di->is_nil())
				continue;
			sqlite3x::sqlite3_command cmd(m_db, "INSERT INTO tmpdep"
						      " (UUID0,UUID1,UUID2,UUID3,UUIDD0,UUIDD1,UUIDD2,UUIDD3)"
						      " VALUES (?,?,?,?,?,?,?,?);");
			for (unsigned int i = 0; i < 4; ++i)
				cmd.bind(i + 1, (long long int)p->get_uuid().get_word(i));
			for (unsigned int i = 0; i < 4; ++i)
				cmd.bind(i + 5, (long long int)di->get_word(i));
			cmd.executenonquery();
		}
	}
	tran.commit();
	p->unset_dirty();	
}

unsigned int Database::flush_cache(const Glib::TimeVal& tv)
{
	unsigned int del(0);
	for (;;) {
		bool work(false);
		for (cache_t::iterator i(m_cache.begin()), e(m_cache.end()); i != e; ) {
			cache_t::iterator i2(i);
			++i;
			if (i2->get_tv() > tv)
				continue;
			gint cnt(i2->get_refcount());
			if (cnt > 1)
				continue;
			m_cache.erase(i2);
			++del;
			work = true;
		}
		if (!work)
			break;
	}
	return del;
}

unsigned int Database::clear_cache(void)
{
	unsigned int del(m_cache.size());
	m_cache.clear();
	return del;
}

void Database::open(void)
{
	if (m_db.db())
		return;
	m_tempdb = false;
	m_db.open(Glib::build_filename(m_path, "adr.db"));
	if (sqlite3_create_function(m_db.db(), "upperbound", 1, SQLITE_UTF8, 0, &dbfunc_upperbound, 0, 0) != SQLITE_OK)
		throw std::runtime_error("Database: cannot add user function upperbound");
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS obj (UUID0 INTEGER NOT NULL, "
					      "UUID1 INTEGER NOT NULL,"
					      "UUID2 INTEGER NOT NULL,"
					      "UUID3 INTEGER NOT NULL,"
					      "TYPE INTEGER NOT NULL,"
					      "SWLAT INTEGER NOT NULL,"
					      "SWLON INTEGER NOT NULL,"
					      "NELAT INTEGER NOT NULL,"
					      "NELON INTEGER NOT NULL,"
					      "MINTIME INTEGER NOT NULL,"
					      "MAXTIME INTEGER NOT NULL,"
					      "MODIFIED INTEGER NOT NULL,"
					      "DATA BLOB NOT NULL,"
					      "UNIQUE (UUID0,UUID1,UUID2,UUID3) ON CONFLICT REPLACE);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS dep (UUID0 INTEGER  NOT NULL, "
					      "UUID1 INTEGER NOT NULL,"
					      "UUID2 INTEGER NOT NULL,"
					      "UUID3 INTEGER NOT NULL,"
					      "UUIDD0 INTEGER NOT NULL,"
					      "UUIDD1 INTEGER NOT NULL,"
					      "UUIDD2 INTEGER NOT NULL,"
					      "UUIDD3 INTEGER NOT NULL);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS ident (UUID0 INTEGER  NOT NULL, "
					      "UUID1 INTEGER NOT NULL,"
					      "UUID2 INTEGER NOT NULL,"
					      "UUID3 INTEGER NOT NULL,"
					      "IDENT TEXT COLLATE NOCASE);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS dct (UUIDA0 INTEGER NOT NULL, "
					      "UUIDA1 INTEGER NOT NULL,"
					      "UUIDA2 INTEGER NOT NULL,"
					      "UUIDA3 INTEGER NOT NULL,"
					      "UUIDB0 INTEGER NOT NULL,"
					      "UUIDB1 INTEGER NOT NULL,"
					      "UUIDB2 INTEGER NOT NULL,"
					      "UUIDB3 INTEGER NOT NULL,"
					      "SWLAT INTEGER NOT NULL,"
					      "SWLON INTEGER NOT NULL,"
					      "NELAT INTEGER NOT NULL,"
					      "NELON INTEGER NOT NULL,"
					      "DATA BLOB NOT NULL,"
					      "UNIQUE (UUIDA0,UUIDA1,UUIDA2,UUIDA3,UUIDB0,UUIDB1,UUIDB2,UUIDB3) ON CONFLICT REPLACE);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS obj_bbox ON obj(SWLAT,NELAT,SWLON,NELON);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS dep_uuid ON dep(UUID0,UUID1,UUID2,UUID3);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS dep_uuidd ON dep(UUIDD0,UUIDD1,UUIDD2,UUIDD3);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS ident_uuid ON ident(UUID0,UUID1,UUID2,UUID3);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS ident_ident ON ident(IDENT COLLATE NOCASE);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS dct_uuida ON dct(UUIDA0,UUIDA1,UUIDA2,UUIDA3);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS dct_uuidb ON dct(UUIDB0,UUIDB1,UUIDB2,UUIDB3);");
		cmd.executenonquery();
	}
	open_binfile();
}

void Database::close(void)
{
	close_binfile();
	if (!m_db.db())
		return;
	m_db.close();
	m_cache.clear();
	m_tempdb = false;
}

void Database::sync_off(void)
{
	open();
	{
		sqlite3x::sqlite3_command cmd(m_db, "PRAGMA synchronous = OFF;");
		cmd.executenonquery();
	}
}

void Database::analyze(void)
{
	open();
	{
		sqlite3x::sqlite3_command cmd(m_db, "ANALYZE;");
		cmd.executenonquery();
	}
}

void Database::vacuum(void)
{
	open();
	{
		sqlite3x::sqlite3_command cmd(m_db, "VACUUM;");
		cmd.executenonquery();
	}
}

void Database::delete_transitive_closure(void)
{
	open();
	sqlite3x::sqlite3_transaction tran(m_db);
	{
		sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS rowiddeptc;");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS rowiddep_parent;");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS rowiddep_child;");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS rowiddep;");
		cmd.executenonquery();
	}
}

void Database::create_transitive_closure(void)
{
	open();
	sqlite3x::sqlite3_transaction tran(m_db);
	{
		sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS rowiddeptc;");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS rowiddep_parent;");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS rowiddep_child;");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS rowiddep;");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS rowiddep (PARENT INTEGER NOT NULL, "
					      "CHILD INTEGER NOT NULL);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS rowiddep_parent ON rowiddep(PARENT);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS rowiddep_child ON rowiddep(CHILD);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "INSERT INTO rowiddep (PARENT,CHILD) SELECT P.ROWID,C.ROWID FROM dep AS D, "
					      "obj AS P ON D.UUID0=P.UUID0 AND D.UUID1=P.UUID1 AND D.UUID2=P.UUID2 AND D.UUID3=P.UUID3, "
					      "obj AS C ON D.UUIDD0=C.UUID0 AND D.UUIDD1=C.UUID1 AND D.UUIDD2=C.UUID2 AND D.UUIDD3=C.UUID3;");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE VIRTUAL TABLE rowiddeptc USING "
					      "transitive_closure(tablename=rowiddep,idcolumn=CHILD,parentcolumn=PARENT);");
		cmd.executenonquery();
	}
}

void Database::create_simple_transitive_closure(void)
{
	open();
	sqlite3x::sqlite3_transaction tran(m_db);
	{
		sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS deptc_uuid;");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS deptc_uuidd;");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS deptc;");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS deptc (UUID0 INTEGER NOT NULL, "
					      "UUID1 INTEGER NOT NULL,"
					      "UUID2 INTEGER NOT NULL,"
					      "UUID3 INTEGER NOT NULL,"
					      "UUIDD0 INTEGER NOT NULL,"
					      "UUIDD1 INTEGER NOT NULL,"
					      "UUIDD2 INTEGER NOT NULL,"
					      "UUIDD3 INTEGER NOT NULL, UNIQUE(UUID0,UUID1,UUID2,UUID3,UUIDD0,UUIDD1,UUIDD2,UUIDD3));");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS deptc_uuid ON deptc(UUID0,UUID1,UUID2,UUID3);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS deptc_uuidd ON deptc(UUIDD0,UUIDD1,UUIDD2,UUIDD3);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "INSERT OR IGNORE INTO deptc (UUID0,UUID1,UUID2,UUID3,UUIDD0,UUIDD1,UUIDD2,UUIDD3) "
					      "SELECT UUID0,UUID1,UUID2,UUID3,UUIDD0,UUIDD1,UUIDD2,UUIDD3 FROM dep;");
		cmd.executenonquery();
	}
	unsigned int tblcnt;
	{
		sqlite3x::sqlite3_command cmd(m_db, "SELECT COUNT(*) FROM deptc;");
		tblcnt = cmd.executeint();
	}
	for (;;) {
		{
			sqlite3x::sqlite3_command cmd(m_db, "INSERT OR IGNORE INTO deptc (UUID0,UUID1,UUID2,UUID3,UUIDD0,UUIDD1,UUIDD2,UUIDD3) "
				"SELECT P.UUID0,P.UUID1,P.UUID2,P.UUID3,C.UUIDD0,C.UUIDD1,C.UUIDD2,C.UUIDD3 FROM deptc AS P, "
				"deptc as C on P.UUIDD0=C.UUID0 AND P.UUIDD1=C.UUID1 AND P.UUIDD2=C.UUID2 AND P.UUIDD3=C.UUID3;");
			cmd.executenonquery();
		}
		unsigned int tblcnt1(tblcnt);
		{
			sqlite3x::sqlite3_command cmd(m_db, "SELECT COUNT(*) FROM deptc;");
			tblcnt = cmd.executeint();
		}
		if (tblcnt == tblcnt1)
			break;
	}
}

void Database::delete_simple_transitive_closure(void)
{
	open();
	sqlite3x::sqlite3_transaction tran(m_db);
	{
		sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS deptc_uuid;");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS deptc_uuidd;");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS deptc;");
		cmd.executenonquery();
	}
}

void Database::create_dct_indices(void)
{
	open();
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS dct_uuida ON dct(UUIDA0,UUIDA1,UUIDA2,UUIDA3);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS dct_uuidb ON dct(UUIDB0,UUIDB1,UUIDB2,UUIDB3);");
		cmd.executenonquery();
	}
}

void Database::drop_dct_indices(void)
{
	open();
	{
		sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS dct_uuida;");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS dct_uuidb;");
		cmd.executenonquery();
	}
}

void Database::set_wal(bool wal)
{
	open();
	{
		sqlite3x::sqlite3_command cmd(m_db, wal ? "PRAGMA journal_mode=WAL;" : "PRAGMA journal_mode=DELETE;");
		cmd.executenonquery();
	}
}

void Database::set_exclusive(bool excl)
{
	open();
	{
		sqlite3x::sqlite3_command cmd(m_db, excl ? "PRAGMA locking_mode=EXCLUSIVE;" :
					      "PRAGMA locking_mode=NORMAL;");
		cmd.executenonquery();
	}
}

void Database::dbfunc_upperbound(sqlite3_context *ctxt, int, sqlite3_value **values)
{
	std::string s((const char *)sqlite3_value_text(values[0]));
	if (false)
	std::cerr << "dbfunc_upperbound: in \"" << s << "\"" << std::endl;
	if (!s.empty())
	s.replace(--s.end(), s.end(), 1, (*(--s.end())) + 1);
	if (false)
	std::cerr << "dbfunc_upperbound: out \"" << s << "\"" << std::endl;
	sqlite3_result_text(ctxt, s.data(), s.size(), SQLITE_TRANSIENT);
}

void Database::open_temp(void)
{
	if (m_tempdb)
		return;
	open();
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TEMP TABLE IF NOT EXISTS tmpobj (UUID0 INTEGER NOT NULL, "
					      "UUID1 INTEGER NOT NULL,"
					      "UUID2 INTEGER NOT NULL,"
					      "UUID3 INTEGER NOT NULL,"
					      "MODIFIED INTEGER NOT NULL,"
					      "DATA BLOB NOT NULL,"
					      "UNIQUE (UUID0,UUID1,UUID2,UUID3) ON CONFLICT REPLACE);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TEMP TABLE IF NOT EXISTS tmpdep (UUID0 INTEGER NOT NULL, "
					      "UUID1 INTEGER NOT NULL,"
					      "UUID2 INTEGER NOT NULL,"
					      "UUID3 INTEGER NOT NULL,"
					      "UUIDD0 INTEGER NOT NULL,"
					      "UUIDD1 INTEGER NOT NULL,"
					      "UUIDD2 INTEGER NOT NULL,"
					      "UUIDD3 INTEGER NOT NULL);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS tmpdep_uuid ON tmpdep(UUID0,UUID1,UUID2,UUID3);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS tmpdep_uuidd ON tmpdep(UUIDD0,UUIDD1,UUIDD2,UUIDD3);");
		cmd.executenonquery();
	}
	m_tempdb = true;
}

std::string Database::quote_text_for_like(const std::string& s, char e)
{
	std::string r;
	for (std::string::const_iterator si(s.begin()), se(s.end()); si != se; si++) {
	if (*si == '%' || *si == '_' || (e && *si == e))
	r.push_back(e ? e : '%');
	r.push_back(*si);
	}
	return r;
}

Database::findresults_t Database::find_all(loadmode_t loadmode,
					   uint64_t tmin, uint64_t tmax,
					   Object::type_t typmin, Object::type_t typmax,
					   unsigned int limit)
{
	open();
	if (is_binfile())
		return find_all_binfile(loadmode, tmin, tmax, typmin, typmax, limit);
	bool typtm(tmin != 0 || tmax != std::numeric_limits<uint64_t>::max() ||
		   typmin != Object::type_first || typmax != Object::type_last);
	// make cache hot -- this is not a win
	if (false && loadmode == loadmode_link) {
		std::ostringstream qstr;
		qstr << "SELECT UUID0,UUID1,UUID2,UUID3,DATA,MODIFIED FROM obj, "
		     << "(SELECT DISTINCT UUIDD0,UUIDD1,UUIDD2,UUIDD3 FROM deptc AS D, "
			"obj AS P ON D.UUID0=P.UUID0 AND D.UUID1=P.UUID1 AND D.UUID2=P.UUID2 AND D.UUID3=P.UUID3";
		if (typtm)
			qstr << " WHERE (((P.MINTIME < ?2) AND (P.MAXTIME > ?1)) OR ((?1 < P.MAXTIME) AND (?2 > P.MINTIME)))"
				" AND (P.TYPE >= ?3) AND (P.TYPE <= ?4)";
		if (limit)
			qstr << " LIMIT ?3";
		qstr << ") ON UUIDD0=UUID0 AND UUIDD1=UUID1 AND UUIDD2=UUID2 AND UUIDD3=UUID3;";
		sqlite3x::sqlite3_command cmd(m_db, qstr.str());
		if (typtm) {
			cmd.bind(1, (long long int)std::min(tmin, (uint64_t)std::numeric_limits<int64_t>::max()));
			cmd.bind(2, (long long int)std::min(tmax, (uint64_t)std::numeric_limits<int64_t>::max()));
			cmd.bind(3, typmin);
			cmd.bind(4, typmax);
		}
		find_tail(cmd, loadmode, true);
	}
	std::ostringstream qstr;
	qstr << "SELECT UUID0,UUID1,UUID2,UUID3";
	if (loadmode != loadmode_uuid)
		qstr << ",DATA,MODIFIED";
	qstr << " FROM obj";
	if (typtm)
		qstr << " WHERE (((MINTIME < ?2) AND (MAXTIME > ?1)) OR ((?1 < MAXTIME) AND (?2 > MINTIME)))"
			" AND (TYPE >= ?3) AND (TYPE <= ?4)";
	if (limit)
		qstr << " LIMIT ?3";
	qstr << ";";
	sqlite3x::sqlite3_command cmd(m_db, qstr.str());
	if (typtm) {
		cmd.bind(1, (long long int)std::min(tmin, (uint64_t)std::numeric_limits<int64_t>::max()));
		cmd.bind(2, (long long int)std::min(tmax, (uint64_t)std::numeric_limits<int64_t>::max()));
		cmd.bind(3, typmin);
		cmd.bind(4, typmax);
	}
	return find_tail(cmd, loadmode, true);
}

Database::findresults_t Database::find_by_ident(const std::string& ident, comp_t comp, char escape, loadmode_t loadmode,
						uint64_t tmin, uint64_t tmax,
						Object::type_t typmin, Object::type_t typmax,
						unsigned int limit)
{
	bool typtm(tmin != 0 || tmax != std::numeric_limits<uint64_t>::max() ||
		   typmin != Object::type_first || typmax != Object::type_last);
	open();
	std::string pat(ident);
	std::ostringstream qstr;
	qstr << "SELECT UUID0,UUID1,UUID2,UUID3";
	if (loadmode != loadmode_uuid)
		qstr << ",DATA,MODIFIED";
	qstr << " FROM ident";
	if (loadmode != loadmode_uuid || typtm)
		qstr << " INNER JOIN obj USING (UUID0,UUID1,UUID2,UUID3)";
	qstr << " WHERE ";
	{
		switch (comp) {
		default:
		case comp_startswith:
			escape = 0;
			qstr << "(IDENT>=?1 COLLATE NOCASE) AND (IDENT<upperbound(?1) COLLATE NOCASE)";
			break;

		case comp_exact:
			escape = 0;
			qstr << "(IDENT=?1 COLLATE NOCASE)";
			break;

		case comp_exact_casesensitive:
			escape = 0;
			qstr << "(IDENT=?1)";
			break;

		case comp_contains:
			escape = '!';
			pat = std::string("%") + quote_text_for_like(pat, escape) + "%";
			// fall through

		case comp_like:
			qstr << "(IDENT LIKE ?1";
			if (escape)
				qstr << " ESCAPE ?2";
			qstr << ')';
			break;
		}
	}
	if (typtm)
		qstr << " AND (((MINTIME < ?5) AND (MAXTIME > ?4)) OR ((?4 < MAXTIME) AND (?5 > MINTIME)))"
			" AND (TYPE >= ?6) AND (TYPE <= ?7)";
	if (limit)
		qstr << " LIMIT ?3";
	qstr << ";";
	sqlite3x::sqlite3_command cmd(m_db, qstr.str());
	cmd.bind(1, pat);
	if (escape)
		cmd.bind(2, std::string(1, escape));
	if (limit)
		cmd.bind(3, (long long int)limit);
	if (typtm) {
		cmd.bind(4, (long long int)std::min(tmin, (uint64_t)std::numeric_limits<int64_t>::max()));
		cmd.bind(5, (long long int)std::min(tmax, (uint64_t)std::numeric_limits<int64_t>::max()));
		cmd.bind(6, typmin);
		cmd.bind(7, typmax);
	}
	return find_tail(cmd, loadmode, true);
}

Database::findresults_t Database::find_by_bbox(const Rect& bbox, loadmode_t loadmode,
					       uint64_t tmin, uint64_t tmax,
					       Object::type_t typmin, Object::type_t typmax,
					       unsigned int limit)
{
	open();
	if (is_binfile())
		return find_by_bbox_binfile(bbox, loadmode, tmin, tmax, typmin, typmax, limit);
	bool typtm(tmin != 0 || tmax != std::numeric_limits<uint64_t>::max() ||
		   typmin != Object::type_first || typmax != Object::type_last);
	std::ostringstream qstr;
	qstr << "SELECT UUID0,UUID1,UUID2,UUID3";
	if (loadmode != loadmode_uuid)
		qstr << ",DATA,MODIFIED";
	qstr << " FROM obj WHERE ";
	if (bbox.get_north() + 0x40000000 < 0x40000000 - bbox.get_south())
		qstr << "(SWLAT <= ?4 AND NELAT >= ?2)";
	else
		qstr << "(NELAT >= ?2 AND SWLAT <= ?4)";
	qstr << " AND ((SWLON <= ?3-4294967296 AND NELON >= ?1-4294967296) OR "
		"(SWLON <= ?3 AND NELON >= ?1) OR "
		"(SWLON <= ?3+4294967296 AND NELON >= ?1+4294967296))";
	if (typtm)
		qstr << " AND (((MINTIME < ?7) AND (MAXTIME > ?6)) OR ((?6 < MAXTIME) AND (?7 > MINTIME)))"
			" AND (TYPE >= ?8) AND (TYPE <= ?9)";
	if (limit)
		qstr << " LIMIT ?5";
	qstr << ";";
	sqlite3x::sqlite3_command cmd(m_db, qstr.str());
	cmd.bind(1, (long long int)bbox.get_west());
	cmd.bind(2, (long long int)bbox.get_south());
	cmd.bind(3, (long long int)bbox.get_east_unwrapped());
	cmd.bind(4, (long long int)bbox.get_north());
	if (limit)
		cmd.bind(5, (long long int)limit);
	if (typtm) {
		cmd.bind(6, (long long int)std::min(tmin, (uint64_t)std::numeric_limits<int64_t>::max()));
		cmd.bind(7, (long long int)std::min(tmax, (uint64_t)std::numeric_limits<int64_t>::max()));
		cmd.bind(8, typmin);
		cmd.bind(9, typmax);
	}
	return find_tail(cmd, loadmode, true);
}

Database::findresults_t Database::find_dependson(const UUID& uuid, loadmode_t loadmode,
						 uint64_t tmin, uint64_t tmax,
						 Object::type_t typmin, Object::type_t typmax,
						 unsigned int limit)
{
	bool typtm(tmin != 0 || tmax != std::numeric_limits<uint64_t>::max() ||
		   typmin != Object::type_first || typmax != Object::type_last);
	open();
	std::ostringstream qstr;
	qstr << "SELECT UUID0,UUID1,UUID2,UUID3";
	if (loadmode != loadmode_uuid)
		qstr << ",DATA,MODIFIED";
	qstr << " FROM dep";
	if (loadmode != loadmode_uuid || typtm)
		qstr << " INNER JOIN obj USING (UUID0,UUID1,UUID2,UUID3)";
	qstr << " WHERE dep.UUIDD0=?1 AND dep.UUIDD1=?2 AND dep.UUIDD2=?3 AND dep.UUIDD3=?4";
	if (typtm)
		qstr << " AND (((MINTIME < ?7) AND (MAXTIME > ?6)) OR ((?6 < MAXTIME) AND (?7 > MINTIME)))"
			" AND (TYPE >= ?8) AND (TYPE <= ?9)";
	if (limit)
		qstr << " LIMIT ?5";
	qstr << ";";
	sqlite3x::sqlite3_command cmd(m_db, qstr.str());
	for (unsigned int i = 0; i < 4; ++i)
		cmd.bind(i + 1, (long long int)uuid.get_word(i));
	if (limit)
		cmd.bind(5, (long long int)limit);
	if (typtm) {
		cmd.bind(6, (long long int)std::min(tmin, (uint64_t)std::numeric_limits<int64_t>::max()));
		cmd.bind(7, (long long int)std::min(tmax, (uint64_t)std::numeric_limits<int64_t>::max()));
		cmd.bind(8, typmin);
		cmd.bind(9, typmax);
	}
	return find_tail(cmd, loadmode, true);
}

Database::findresults_t Database::find_dependencies(const UUID& uuid, loadmode_t loadmode,
						    uint64_t tmin, uint64_t tmax,
						    Object::type_t typmin, Object::type_t typmax,
						    unsigned int limit)
{
	bool typtm(tmin != 0 || tmax != std::numeric_limits<uint64_t>::max() ||
		   typmin != Object::type_first || typmax != Object::type_last);
	open();
	std::ostringstream qstr;
	qstr << "SELECT dep.UUIDD0,dep.UUIDD1,dep.UUIDD2,dep.UUIDD3";
	if (loadmode != loadmode_uuid)
		qstr << ",obj.DATA,obj.MODIFIED";
	qstr << " FROM dep";
	if (loadmode != loadmode_uuid || typtm)
		qstr << " INNER JOIN obj ON dep.UUIDD0 = obj.UUID0 AND dep.UUIDD1 = obj.UUID1 AND "
			"dep.UUIDD2 = obj.UUID2 AND dep.UUIDD3 = obj.UUID3";
	qstr << " WHERE dep.UUID0=?1 AND dep.UUID1=?2 AND dep.UUID2=?3 AND dep.UUID3=?4";
	if (typtm)
		qstr << " AND (((MINTIME < ?7) AND (MAXTIME > ?6)) OR ((?6 < MAXTIME) AND (?7 > MINTIME)))"
			" AND (TYPE >= ?8) AND (TYPE <= ?9)";
	if (limit)
		qstr << " LIMIT ?5";
	qstr << ";";
	sqlite3x::sqlite3_command cmd(m_db, qstr.str());
	for (unsigned int i = 0; i < 4; ++i)
		cmd.bind(i + 1, (long long int)uuid.get_word(i));
	if (limit)
		cmd.bind(5, (long long int)limit);
	if (typtm) {
		cmd.bind(6, (long long int)std::min(tmin, (uint64_t)std::numeric_limits<int64_t>::max()));
		cmd.bind(7, (long long int)std::min(tmax, (uint64_t)std::numeric_limits<int64_t>::max()));
		cmd.bind(8, typmin);
		cmd.bind(9, typmax);
	}
	return find_tail(cmd, loadmode, true);
}

Database::findresults_t Database::find_tail(sqlite3x::sqlite3_command& cmd, loadmode_t loadmode, bool cache)
{
	findresults_t r;
	sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
	while (cursor.step()) {
		r.push_back(Link(UUID(cursor.getint(0), cursor.getint(1), cursor.getint(2), cursor.getint(3))));
		if (loadmode == loadmode_uuid)
			continue;
		if (cursor.isnull(4))
			continue;
		int sz;
		uint8_t const *data((uint8_t const *)cursor.getblob(4, sz));
		if (sz < 1)
			continue;
		ArchiveReadBuffer ar(data, data + sz);
		Object::ptr_t p;
		try {
			p = Object::create(ar, r.back());
		} catch (const std::exception& e) {
			if (false)
				throw;
			std::ostringstream oss;
			oss << e.what() << "; blob" << std::hex;
			for (const uint8_t *p0(data), *p1(data + sz); p0 != p1; ++p0)
				oss << ' ' << std::setfill('0') << std::setw(2) << (unsigned int)*p0;
			throw std::runtime_error(oss.str());
		}
		if (!p)
			continue;
		p->set_modified(cursor.getint(5));
		p->unset_dirty();
		r.back().set_obj(p);
		if (cache)
			cache_put(p);
		if (loadmode == loadmode_link)
			p->link(*this, ~0U);
	}
	return r;
}

Database::deppairs_t Database::find_modifiedafter(uint64_t tmod)
{
	open();
	sqlite3x::sqlite3_command cmd(m_db, "SELECT dep.UUID0,dep.UUID1,dep.UUID2,dep.UUID3,dep.UUIDD0,dep.UUIDD1,dep.UUIDD2,dep.UUIDD3 "
				      "FROM dep INNER JOIN obj ON "
				      "dep.UUIDD0 = obj.UUID0 AND dep.UUIDD1 = obj.UUID1 AND "
				      "dep.UUIDD2 = obj.UUID2 AND dep.UUIDD3 = obj.UUID3"
				      " WHERE obj.MODIFIED >= ?1;");
	cmd.bind(1, (long long int)tmod);
	sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
	deppairs_t r;
	while (cursor.step())
		r.push_back(deppair_t(UUID(cursor.getint(0), cursor.getint(1), cursor.getint(2), cursor.getint(3)),
				      UUID(cursor.getint(4), cursor.getint(5), cursor.getint(6), cursor.getint(7))));
	return r;
}

Database::findresults_t Database::find_all_temp(loadmode_t loadmode)
{
	if (!m_tempdb)
		return findresults_t();
	open();
	std::ostringstream qstr;
	qstr << "SELECT UUID0,UUID1,UUID2,UUID3";
	if (loadmode != loadmode_uuid)
		qstr << ",DATA,MODIFIED";
	qstr << " FROM tmpobj;";
	sqlite3x::sqlite3_command cmd(m_db, qstr.str());
	return find_tail(cmd, loadmode, true);
}

Database::deppairs_t Database::find_all_deps(void)
{
	open();
	sqlite3x::sqlite3_command cmd(m_db, "SELECT UUID0,UUID1,UUID2,UUID3,UUIDD0,UUIDD1,UUIDD2,UUIDD3 FROM dep;");
	sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
	deppairs_t r;
	while (cursor.step())
		r.push_back(deppair_t(UUID(cursor.getint(0), cursor.getint(1), cursor.getint(2), cursor.getint(3)),
				      UUID(cursor.getint(4), cursor.getint(5), cursor.getint(6), cursor.getint(7))));
	return r;
}

Database::deppairs_t Database::find_all_temp_deps(void)
{
	if (!m_tempdb)
		return deppairs_t();
	open();
	sqlite3x::sqlite3_command cmd(m_db, "SELECT UUID0,UUID1,UUID2,UUID3,UUIDD0,UUIDD1,UUIDD2,UUIDD3 FROM tmpdep;");
	sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
	deppairs_t r;
	while (cursor.step())
		r.push_back(deppair_t(UUID(cursor.getint(0), cursor.getint(1), cursor.getint(2), cursor.getint(3)),
				      UUID(cursor.getint(4), cursor.getint(5), cursor.getint(6), cursor.getint(7))));
	return r;
}

void Database::write_binfile(const std::string& fn)
{
	open();
	std::ofstream of(fn.c_str(), std::ofstream::binary);
	if (!of.good())
		throw std::runtime_error("Cannot write to file " + fn);
	unsigned cnt;
	{
		sqlite3x::sqlite3_command cmd(m_db, "SELECT COUNT(*) FROM obj;");
		cnt = cmd.executeint();
	}
	{
		BinFileHeader h;
		h.set_signature();
		h.set_objdiroffs(BinFileHeader::size);
		h.set_objdirentries(cnt);
		of.seekp(0, std::ofstream::beg);
		of.write((const char *)&h, BinFileHeader::size);
	}
	{
		BinFileObjEntry e[1024];
		unsigned int eb(0);
		unsigned int ep(0);
		uint64_t fpos(BinFileHeader::size + cnt * BinFileObjEntry::size);
		sqlite3x::sqlite3_command cmd(m_db, "SELECT UUID0,UUID1,UUID2,UUID3,DATA,MODIFIED FROM obj ORDER BY UUID0,UUID1,UUID2,UUID3;");
		sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
		while (cursor.step()) {
			if (cursor.isnull(4))
				continue;
			int sz;
			uint8_t const *data((uint8_t const *)cursor.getblob(4, sz));
			if (sz < 1)
				continue;
			ArchiveReadBuffer ar(data, data + sz);
			Object::ptr_t p(Object::create(ar, UUID(cursor.getint(0), cursor.getint(1), cursor.getint(2), cursor.getint(3))));
			p->set_modified(cursor.getint(5));
			p->unset_dirty();
			unsigned int objsz;
			{
				std::ostringstream blob;
				{
					ArchiveWriteStream ar(blob);
					p->save(ar);
				}
				objsz = blob.str().size();
				of.seekp(fpos, std::ofstream::beg);
				of.write(blob.str().c_str(), objsz);
			}
			{
				BinFileObjEntry& ee(e[ep]);
				ee.set_uuid(p->get_uuid());
				{
					Rect bbox;
					p->get_bbox(bbox);
					ee.set_bbox(bbox);
				}
				{
					std::pair<uint64_t,uint64_t> r(p->get_timebounds());
					ee.set_mintime(r.first);
					ee.set_maxtime(r.second);
				}
				ee.set_modified(p->get_modified());
				ee.set_type(p->get_type());
				ee.set_dataoffs(fpos);
				ee.set_datasize(objsz);
				++ep;
			}
			fpos += objsz;
			if (ep < sizeof(e)/sizeof(e[0]))
				continue;
			of.seekp(BinFileHeader::size + eb * BinFileObjEntry::size, std::ofstream::beg);
			of.write((const char *)&e[0], ep * BinFileObjEntry::size);
			eb += ep;
			ep = 0;
		}
		if (ep > 0) {
			of.seekp(BinFileHeader::size + eb * BinFileObjEntry::size, std::ofstream::beg);
			of.write((const char *)&e[0], ep * BinFileObjEntry::size);
			eb += ep;
		}
	}
}

#if defined(HAVE_WINDOWS_H)

void Database::open_binfile(void)
{
	if (!m_enablebinfile)
		return;
	std::string dbfn(Glib::build_filename(m_path, "adr.db"));
	HANDLE hFile = CreateFile(dbfn.c_str(), GENERIC_READ, FILE_SHARE_READ, 0,
				  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		if (true)
			std::cerr << "Cannot open db file " << dbfn << ": 0x"
				  << std::hex << GetLastError() << std::dec << std::endl;
		return;
	}
	FILETIME dbtime;
	if (!GetFileTime(hFile, 0, 0, &dbtime)) {
		CloseHandle(hFile);
		if (true)
			std::cerr << "Cannot get db file time " << dbfn << ": 0x"
				  << std::hex << GetLastError() << std::dec << std::endl;
		return;
	}
	CloseHandle(hFile);
	std::string binfn(Glib::build_filename(m_path, "adr.bin"));
	hFile = CreateFile(binfn.c_str(), GENERIC_READ, FILE_SHARE_READ, 0,
			   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		if (true)
			std::cerr << "Cannot open bin file " << binfn << ": 0x"
				  << std::hex << GetLastError() << std::dec << std::endl;
		return;
	}
	FILETIME bintime;
	if (!GetFileTime(hFile, 0, 0, &bintime)) {
		CloseHandle(hFile);
		if (true)
			std::cerr << "Cannot get bin file time " << binfn << ": 0x"
				  << std::hex << GetLastError() << std::dec << std::endl;
		return;
	}
	if (dbtime.dwHighDateTime > bintime.dwHighDateTime ||
	    (dbtime.dwHighDateTime == bintime.dwHighDateTime && dbtime.dwLowDateTime > bintime.dwLowDateTime)) {
		CloseHandle(hFile);
		if (true)
			std::cerr << "Bin file " << binfn << " is older than db file " << dbfn << ": "
				  << bintime.dwHighDateTime << ',' << bintime.dwLowDateTime << " vs "
				  << dbtime.dwHighDateTime << ',' << dbtime.dwLowDateTime << std::endl;
		return;
	}
	HANDLE hMap = CreateFileMapping(hFile, 0, PAGE_READONLY, 0, 0, 0);
	if (hMap == INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
		if (true)
			std::cerr << "Cannot create bin file mapping " << binfn << ": 0x"
				  << std::hex << GetLastError() << std::dec << std::endl;
		return;
	}
	m_binfile = reinterpret_cast<uint8_t *>(MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0));
	m_binsize = 0;
	CloseHandle(hFile);
	CloseHandle(hMap);
	if (!m_binfile) {
		if (true)
			std::cerr << "Cannot map bin file " << binfn << ": 0x"
				  << std::hex << GetLastError() << std::dec << std::endl;
		return;
	}
	MEMORY_BASIC_INFORMATION meminfo;
	if (VirtualQuery(m_binfile, &meminfo, sizeof(meminfo)) != sizeof(meminfo)) {
		if (true)
			std::cerr << "Cannot get bin file " << binfn << " mapping information" << std::endl;
		close_binfile();
		return;
	}
	m_binsize = meminfo.RegionSize;
	const BinFileHeader& hdr(*(const BinFileHeader *)m_binfile);
	if (!hdr.check_signature()) {
		if (true)
			std::cerr << "Invalid signature on bin file " << binfn << std::endl;
		close_binfile();
		return;
	}
	if (true)
		std::cerr << "Using bin file " << binfn << ": " << hdr.get_objdirentries() << " objects" << std::endl;
}

void Database::close_binfile(void)
{
	if (!m_binfile)
		return;
	if (!UnmapViewOfFile(m_binfile)) {
		if (true)
			std::cerr << "Error unmapping binfile: 0x" << std::hex << GetLastError() << std::dec << std::endl;
	}
	m_binfile = 0;
}

#else

void Database::open_binfile(void)
{
	if (m_binfile)
		return;
	if (!m_enablebinfile)
		return;
	std::string dbfn(Glib::build_filename(m_path, "adr.db"));
	struct stat dbstat;
	if (stat(dbfn.c_str(), &dbstat)) {
		if (true)
			std::cerr << "Cannot stat db file " << dbfn << ": " 
				  << strerror(errno) << " (" << errno << ')' << std::endl;
		return;
	}
	std::string binfn(Glib::build_filename(m_path, "adr.bin"));
	int fd(::open(binfn.c_str(), O_RDONLY /* | O_NOATIME*/, 0));
	if (fd == -1) {
		if (true)
			std::cerr << "Cannot open bin file " << binfn << ": " 
				  << strerror(errno) << " (" << errno << ')' << std::endl;
		return;
	}
	struct stat binstat;
	if (fstat(fd, &binstat)) {
		if (true)
			std::cerr << "Cannot stat bin file " << binfn << ": " 
				  << strerror(errno) << " (" << errno << ')' << std::endl;
		return;
	}
	if (dbstat.st_mtime > binstat.st_mtime) {
		::close(fd);
		if (true)
			std::cerr << "Bin file " << binfn << " is older than db file " << dbfn << ": "
				  << Glib::TimeVal(binstat.st_mtime, 0).as_iso8601() << " vs "
				  << Glib::TimeVal(dbstat.st_mtime, 0).as_iso8601() << std::endl;
		return;
	}
	size_t pgsz(sysconf(_SC_PAGE_SIZE));
	m_binsize = ((binstat.st_size + pgsz - 1) / pgsz) * pgsz;
	m_binfile = reinterpret_cast<uint8_t *>(mmap(0, m_binsize, PROT_READ, MAP_PRIVATE, fd, 0));
	::close(fd);
	if (m_binfile == (const uint8_t *)-1) {
		m_binfile = 0;
		m_binsize = 0;
		if (true)
			std::cerr << "Cannot mmap bin file " << binfn << ": " 
				  << strerror(errno) << " (" << errno << ')' << std::endl;
		return;
	}
	const BinFileHeader& hdr(*(const BinFileHeader *)m_binfile);
	if (!hdr.check_signature()) {
		if (true)
			std::cerr << "Invalid signature on bin file " << binfn << std::endl;
		close_binfile();
		return;
	}
	if (true)
		std::cerr << "Using bin file " << binfn << ": " << hdr.get_objdirentries() << " objects" << std::endl;
}

void Database::close_binfile(void)
{
	if (!m_binfile)
		return;
	munmap(const_cast<uint8_t *>(m_binfile), m_binsize);
	m_binfile = 0;
	m_binsize = 0;
}

#endif

class Database::BinFileFindObj
{
public:
	bool operator()(const BinFileObjEntry& a, const UUID& b) {
		return a.get_uuid() < b;
	}
};

Object::ptr_t Database::load_binfile(const BinFileObjEntry *oi)
{
	if (!oi->get_datasize())
		return Object::ptr_t();
	Object::ptr_t p;
	const uint8_t *data(m_binfile + oi->get_dataoffs());
	unsigned int sz(oi->get_datasize());
	try {
		ArchiveReadBuffer ar(data, data + sz);
		p = Object::create(ar, oi->get_uuid());
	} catch (const std::exception& e) {
		if (false)
			throw;
		std::ostringstream oss;
		oss << e.what() << "; " << oi->get_uuid() << " blob (" << sz << ')' << std::hex;
		for (const uint8_t *p0(data), *p1(data + sz); p0 != p1; ++p0)
			oss << ' ' << std::setfill('0') << std::setw(2) << (unsigned int)*p0;
		throw std::runtime_error(oss.str());
	}
	if (p) {
		p->set_modified(oi->get_modified());
		p->unset_dirty();
	}
	return p;
}

Object::ptr_t Database::load_binfile(const UUID& uuid)
{
	if (!m_binfile || uuid.is_nil())
		return Object::ptr_t();
	const BinFileHeader& hdr(*(const BinFileHeader *)m_binfile);
	const BinFileObjEntry *ob(reinterpret_cast<const BinFileObjEntry *>(m_binfile + hdr.get_objdiroffs()));
	const BinFileObjEntry *oe(ob + hdr.get_objdirentries());
	const BinFileObjEntry *oi(std::lower_bound(ob, oe, uuid, BinFileFindObj()));
	if (oi == oe || oi->get_uuid() != uuid) {
		if (false)
			std::cerr << "Binfile: cannot load object " << uuid << std::endl;
		return Object::ptr_t();
	}
	return load_binfile(oi);
}

Database::findresults_t Database::find_all_binfile(loadmode_t loadmode,
						   uint64_t tmin, uint64_t tmax,
						   Object::type_t typmin, Object::type_t typmax,
						   unsigned int limit)
{
	if (!m_binfile)
		return findresults_t();
	findresults_t r;
	const BinFileHeader& hdr(*(const BinFileHeader *)m_binfile);
	const BinFileObjEntry *oi(reinterpret_cast<const BinFileObjEntry *>(m_binfile + hdr.get_objdiroffs()));
	const BinFileObjEntry *oe(oi + hdr.get_objdirentries());
	for (; oi != oe; ++oi) {
		if (oi->get_maxtime() < tmin || oi->get_mintime() > tmax ||
		    oi->get_type() < typmin || oi->get_type() > typmax)
			continue;
		r.push_back(Link(oi->get_uuid()));
		if (loadmode != loadmode_uuid) {
			r.back().set_obj(load_binfile(oi));
			if (loadmode == loadmode_link && r.back().get_obj())
				r.back().get_obj()->link(*this, ~0U);
		}
		if (!limit)
			continue;
		--limit;
		if (!limit)
			break;
	}
	return r;
}

Database::findresults_t Database::find_by_bbox_binfile(const Rect& bbox, loadmode_t loadmode,
						       uint64_t tmin, uint64_t tmax,
						       Object::type_t typmin, Object::type_t typmax,
						       unsigned int limit)
{
	if (!m_binfile)
		return findresults_t();
	findresults_t r;
	const BinFileHeader& hdr(*(const BinFileHeader *)m_binfile);
	const BinFileObjEntry *oi(reinterpret_cast<const BinFileObjEntry *>(m_binfile + hdr.get_objdiroffs()));
	const BinFileObjEntry *oe(oi + hdr.get_objdirentries());
	for (; oi != oe; ++oi) {
		if (oi->get_maxtime() < tmin || oi->get_mintime() > tmax ||
		    oi->get_type() < typmin || oi->get_type() > typmax)
			continue;
		if (!bbox.is_intersect(oi->get_bbox()))
			continue;
		r.push_back(Link(oi->get_uuid()));
		if (loadmode != loadmode_uuid) {
			r.back().set_obj(load_binfile(oi));
			if (loadmode == loadmode_link && r.back().get_obj())
				r.back().get_obj()->link(*this, ~0U);
		}
		if (!limit)
			continue;
		--limit;
		if (!limit)
			break;
	}
	return r;
}

Database::dctresults_t Database::find_dct_by_uuid(const UUID& uuid, loadmode_t loadmode)
{
	open();
	sqlite3x::sqlite3_command cmd(m_db, "SELECT UUIDA0,UUIDA1,UUIDA2,UUIDA3,UUIDB0,UUIDB1,UUIDB2,UUIDB3,DATA FROM dct "
				      "WHERE (UUIDA0=?1 AND UUIDA1=?2 AND UUIDA2=?3 AND UUIDA3=?4) OR "
				      "(UUIDB0=?1 AND UUIDB1=?2 AND UUIDB2=?3 AND UUIDB3=?4);");
	for (unsigned int i = 0; i < 4; ++i)
		cmd.bind(i + 1, (long long int)uuid.get_word(i));
	return dct_tail(cmd, loadmode);
}

Database::dctresults_t Database::find_dct_by_bbox(const Rect& bbox, loadmode_t loadmode)
{
	open();
	std::ostringstream qstr;
	qstr << "SELECT UUIDA0,UUIDA1,UUIDA2,UUIDA3,UUIDB0,UUIDB1,UUIDB2,UUIDB3,DATA FROM dct WHERE ";
	if (bbox.get_north() + 0x40000000 < 0x40000000 - bbox.get_south())
		qstr << "(SWLAT <= ?4 AND NELAT >= ?2)";
	else
		qstr << "(NELAT >= ?2 AND SWLAT <= ?4)";
	qstr << " AND ((SWLON <= ?3-4294967296 AND NELON >= ?1-4294967296) OR "
		"(SWLON <= ?3 AND NELON >= ?1) OR "
		"(SWLON <= ?3+4294967296 AND NELON >= ?1+4294967296))";
	if (false)
		qstr << " LIMIT ?5";
	qstr << ";";
	sqlite3x::sqlite3_command cmd(m_db, qstr.str());
	cmd.bind(1, (long long int)bbox.get_west());
	cmd.bind(2, (long long int)bbox.get_south());
	cmd.bind(3, (long long int)bbox.get_east_unwrapped());
	cmd.bind(4, (long long int)bbox.get_north());
	return dct_tail(cmd, loadmode);
}

Database::dctresults_t Database::dct_tail(sqlite3x::sqlite3_command& cmd, loadmode_t loadmode)
{
	dctresults_t r;
	sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
	while (cursor.step()) {
		r.push_back(DctLeg(Link(UUID(cursor.getint(0), cursor.getint(1), cursor.getint(2), cursor.getint(3))),
				   Link(UUID(cursor.getint(4), cursor.getint(5), cursor.getint(6), cursor.getint(7)))));
		if (cursor.isnull(8))
			continue;
		int sz;
		uint8_t const *data((uint8_t const *)cursor.getblob(8, sz));
		if (sz < 1)
			continue;
		try {
			ArchiveReadBuffer ar(data, data + sz);
			r.back().load(ar);
		} catch (const std::exception& e) {
			if (false)
				throw;
			std::ostringstream oss;
			oss << e.what() << "; blob" << std::hex;
			for (const uint8_t *p0(data), *p1(data + sz); p0 != p1; ++p0)
				oss << ' ' << std::setfill('0') << std::setw(2) << (unsigned int)*p0;
			throw std::runtime_error(oss.str());
		}
		if (loadmode == loadmode_link)
			r.back().link(*this, ~0U);
	}
	return r;
}

void Database::save_dct(const DctLeg& dl, bool tran)
{
	if (dl.get_uuid(0).is_nil() || dl.get_uuid(1).is_nil())
		return;
	open();
	std::ostringstream blob;
	{
		ArchiveWriteStream ar(blob);
		dl.save(ar);
	}
	sqlite3x::sqlite3_transaction transact(m_db, tran);
	{
		sqlite3x::sqlite3_command cmd(m_db, "DELETE FROM dct WHERE UUIDA0=? AND UUIDA1=? AND UUIDA2=? AND UUIDA3=? "
					      "AND UUIDB0=? AND UUIDB1=? AND UUIDB2=? AND UUIDB3=?;");
		for (unsigned int i = 0; i < 4; ++i) {
			cmd.bind(i + 1, (long long int)dl.get_uuid(0).get_word(i));
			cmd.bind(i + 5, (long long int)dl.get_uuid(1).get_word(i));
		}
		cmd.executenonquery();
	}
	// dct points are ordered by UUID, so this shouldn't be necessary
	if (false) {
		sqlite3x::sqlite3_command cmd(m_db, "DELETE FROM dct WHERE UUIDA0=? AND UUIDA1=? AND UUIDA2=? AND UUIDA3=? "
					      "AND UUIDB0=? AND UUIDB1=? AND UUIDB2=? AND UUIDB3=?;");
		for (unsigned int i = 0; i < 4; ++i) {
			cmd.bind(i + 1, (long long int)dl.get_uuid(1).get_word(i));
			cmd.bind(i + 5, (long long int)dl.get_uuid(0).get_word(i));
		}
		cmd.executenonquery();
	}
	if (!dl.is_empty()) {
		sqlite3x::sqlite3_command cmd(m_db, "INSERT OR REPLACE INTO dct "
					      "(UUIDA0,UUIDA1,UUIDA2,UUIDA3,UUIDB0,UUIDB1,UUIDB2,UUIDB3,SWLAT,NELAT,SWLON,NELON,DATA) "
					      "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?);");
		for (unsigned int i = 0; i < 4; ++i) {
			cmd.bind(i + 1, (long long int)dl.get_uuid(0).get_word(i));
			cmd.bind(i + 5, (long long int)dl.get_uuid(1).get_word(i));
		}
		{
			Rect bbox(dl.get_bbox());
			cmd.bind(9, bbox.get_south());
			cmd.bind(10, bbox.get_north());
			cmd.bind(11, bbox.get_west());
			cmd.bind(12, (long long int)bbox.get_east_unwrapped());
		}
		cmd.bind(13, blob.str().c_str(), blob.str().size());
		cmd.executenonquery();
	}
	if (tran)
		transact.commit();
}

void Database::erase_unref_dct(void)
{
	open();
	sqlite3x::sqlite3_transaction tran(m_db);
	{
		sqlite3x::sqlite3_command cmd(m_db, "DELETE FROM dct WHERE ROWID NOT IN (SELECT D.ROWID FROM dct as D, "
					      "obj AS A where A.UUID0=D.UUIDA0 AND A.UUID1=D.UUIDA1 AND A.UUID2=D.UUIDA2 AND A.UUID3=D.UUIDA3, "
					      "obj AS B where B.UUID0=D.UUIDB0 AND B.UUID1=D.UUIDB1 AND B.UUID2=D.UUIDB2 AND B.UUID3=D.UUIDB3);");
		cmd.executenonquery();
	}
}

unsigned int Database::count_dct(void)
{
	open();
	sqlite3x::sqlite3_command cmd(m_db, "SELECT COUNT(*) FROM dct;");
	return cmd.executeint();
}

const std::string& to_str(ADR::Database::comp_t c)
{
	switch (c) {
	case ADR::Database::comp_startswith:
	{
		static const std::string r("startswith");
		return r;
	}

	case ADR::Database::comp_contains:
	{
		static const std::string r("contains");
		return r;
	}

	case ADR::Database::comp_exact:
	{
		static const std::string r("exact");
		return r;
	}

	case ADR::Database::comp_exact_casesensitive:
	{
		static const std::string r("casesensitive");
		return r;
	}

	case ADR::Database::comp_like:
	{
		static const std::string r("like");
		return r;
	}

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}
