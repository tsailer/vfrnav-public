//
// C++ Implementation: dbobj
//
// Description: Database Objects
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012, 2014, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#ifdef HAVE_PQXX
#include <pqxx/transactor.hxx>
#include <pqxx/nontransaction.hxx>
#include <pqxx/result.hxx>
#include <pqxx/except.hxx>
#endif

#include "dbobj.h"
#include "dbser.h"

Glib::ustring Conversions::time_str(time_t t)
{
	struct tm tm;
	if (!gmtime_r(&t, &tm))
		return "?";
	char buf[16];
	strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
	buf[sizeof(buf)-1] = 0;
	return buf;
}

Glib::ustring Conversions::time_str(const Glib::ustring& fmt, time_t t)
{
	struct tm tm;
	if (!gmtime_r(&t, &tm))
		return "?";
	char buf[32];
	strftime(buf, sizeof(buf), fmt.c_str(), &tm);
	buf[sizeof(buf)-1] = 0;
	return buf;
}

time_t Conversions::time_parse(const Glib::ustring & str)
{
	time_t t(0);
	struct tm tm;
	memset(&tm, 0, sizeof(tm));
	if (false)
		std::cerr << "Conversions::time_parse: text " << str << std::endl;
	if (sscanf(str.c_str(), "%u-%u-%u %u:%u:%u", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec) >= 3) {
		if (false)
			std::cerr << "Conversions::time_parse: after sscanf " << tm.tm_year << '-' << tm.tm_mon << '-' << tm.tm_mday
				  << ' ' << tm.tm_hour << ':' << tm.tm_min << ':' << tm.tm_sec << std::endl;
		tm.tm_year -= 1900;
		tm.tm_mon--;
		time_t tm0(mktime(&tm));
		gmtime_r(&tm0, &tm);
		time_t tm1(mktime(&tm));
		t = 2 * tm0 - tm1;
	}
	if (false)
		std::cerr << "Conversions::time_parse: text " << str << " val " << t << std::endl;
	return t;
}

Glib::ustring Conversions::timefrac8_str(uint64_t t)
{
	struct tm tm;
	time_t t1(t >> 8);
	if (!gmtime_r(&t1, &tm))
		return "?";
	char buf[32];
	strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
	buf[sizeof(buf)-1] = 0;
	uint32_t t2(((t & 0xff) * 1000 + 0x80) >> 8);
	size_t blen(strlen(buf));
	snprintf(buf + blen, sizeof(buf) - blen, ".%03u", t2);
	buf[sizeof(buf)-1] = 0;
	return buf;
}

Glib::ustring Conversions::timefrac8_str(const Glib::ustring & fmt, uint64_t t)
{
	struct tm tm;
	time_t t1(t >> 8);
	if (!gmtime_r(&t1, &tm))
		return "?";
	char buf[64];
	strftime(buf, sizeof(buf), fmt.c_str(), &tm);
	buf[sizeof(buf)-1] = 0;
	Glib::ustring buf1(buf);
	char buf2[8];
	uint32_t t2(((t & 0xff) * 1000 + 0x80) >> 8);
	snprintf(buf2, sizeof(buf2), "%03u", t2);
	Glib::ustring::size_type p;
	while ((p = buf1.find("%f")) != Glib::ustring::npos)
		buf1.replace(p, 2, buf2);
	return buf1;
}

uint64_t Conversions::timefrac8_parse(const Glib::ustring & str)
{
	uint64_t t(0);
	struct tm tm;
	memset(&tm, 0, sizeof(tm));
	unsigned int tfrac(0);
	if (false)
		std::cerr << "Conversions::timefrac8_parse: text " << str << std::endl;
	if (sscanf(str.c_str(), "%u-%u-%u %u:%u:%u.%u", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec, &tfrac) >= 3) {
		if (false)
			std::cerr << "Conversions::timefrac8_parse: after sscanf " << tm.tm_year << '-' << tm.tm_mon << '-' << tm.tm_mday
				  << ' ' << tm.tm_hour << ':' << tm.tm_min << ':' << tm.tm_sec << '.' << tfrac << std::endl;
		tm.tm_year -= 1900;
		tm.tm_mon--;
		time_t tm0(mktime(&tm));
		gmtime_r(&tm0, &tm);
		time_t tm1(mktime(&tm));
		t = 2 * tm0 - tm1;
		t <<= 8;
		t += std::min((tfrac * 256 + 500) / 1000, 0xFFU);
	}
	if (false)
		std::cerr << "Conversions::timefrac8_parse: text " << str << " val " << t << std::endl;
	return t;
}

Glib::ustring Conversions::freq_str(uint32_t f)
{
	char buf[16];
	if (f >= 1000000 && f <= 999999999) {
		f = (f + 500) / 1000;
		uint32_t f1 = f / 1000;
		f -= f1 * 1000;
		const std::numpunct<char>& np = std::use_facet<std::numpunct<char> >(std::locale());
		snprintf(buf, sizeof(buf), "%03u%c%03u", f1, np.decimal_point(), f);
		return buf;
	}
	if (f >= 1000 && f <= 999999) {
		f = (f + 500) / 1000;
		snprintf(buf, sizeof(buf), "%03u", f);
		return buf;
	}
	if (!f)
		return "";
	snprintf(buf, sizeof(buf), "%uHz", f);
	return buf;
}

Glib::ustring Conversions::alt_str(int32_t a, uint16_t flg)
{
	char buf[16];
	if (flg & FPlanWaypoint::altflag_standard) {
		a = (a + 50) / 100;
		snprintf(buf, sizeof(buf), "FL %d", a);
		return buf;
	}
	snprintf(buf, sizeof(buf), "%d", a);
	return buf;
}

Glib::ustring Conversions::dist_str(float d)
{
	char buf[16];
	snprintf(buf, sizeof(buf), "%5.1f", d);
	return buf;
}

Glib::ustring Conversions::track_str(float t)
{
	char buf[16];
	int tt = (int)(t);
	tt %= 360;
	if (tt < 0)
		tt += 360;
	snprintf(buf, sizeof(buf), "%03d", tt);
	return buf;
}

Glib::ustring Conversions::velocity_str(float v)
{
	char buf[16];
	snprintf(buf, sizeof(buf), "%5.1f", v);
	return buf;
}

Glib::ustring Conversions::verticalspeed_str(float v)
{
	char buf[16];
	snprintf(buf, sizeof(buf), "%4.0f", v);
	return buf;
}

uint32_t Conversions::unsigned_feet_parse(const Glib::ustring & str, bool *valid)
{
	char *cp;
	double v = strtod(str.c_str(), &cp);
	if (!strcmp(cp, "m")) {
		v *= Point::m_to_ft;
		cp++;
	} else if (!strcmp(cp, "km")) {
		v *= Point::m_to_ft * 1000.0;
		cp += 2;
	} else if (!strcmp(cp, "nmi") || !strcmp(cp, "nm")) {
		v *= Point::m_to_ft * 1000.0 / Point::km_to_nmi;
		cp += 2;
		if (*cp == 'i')
			cp++;
	}
	if (valid)
		*valid = ((*cp == ' ') || (!*cp)) && !str.empty();
	return Point::round<int,double>(v);
}

int32_t Conversions::signed_feet_parse(const Glib::ustring & str, bool *valid)
{
	char *cp;
	double v = strtod(str.c_str(), &cp);
	if (!strcmp(cp, "m")) {
		v *= Point::m_to_ft;
		cp++;
	} else if (!strcmp(cp, "km")) {
		v *= Point::m_to_ft * 1000.0;
		cp += 2;
	} else if (!strcmp(cp, "nmi") || !strcmp(cp, "nm")) {
		v *= Point::m_to_ft * 1000.0 / Point::km_to_nmi;
		cp += 2;
		if (*cp == 'i')
			cp++;
	}
	if (valid)
		*valid = ((*cp == ' ') || (!*cp)) && !str.empty();
	return Point::round<int,double>(v);
}

std::ostream & operator <<(std::ostream & os, const DbBaseElements::ElementAddress & addr)
{
	os << addr.get_id();
	switch (addr.get_table()) {
		case DbBaseElements::ElementAddress::table_main:
			os << 'M';
			break;

		case DbBaseElements::ElementAddress::table_aux:
			os << 'A';
			break;

		default:
			os << '(' << (int)addr.get_table() << ')';
			break;
	}
	return os;
}

DbBaseElements::TileNumber::tile_t DbBaseElements::TileNumber::to_tilenumber(const Point & pt)
{
	Point::coord_t lat(pt.get_lat());
	Point::coord_t lon(pt.get_lon() + lon_offset);
	tile_t ret = 1;
	for (unsigned int i = 0; i < tile_bits; i += 2) {
		ret <<= 2;
		ret |= ((lon >> (8*sizeof(lon)-2)) & 2) | ((lat >> (8*sizeof(lat)-1)) & 1);
		lon <<= 1;
		lat <<= 1;
	}
	return ret;
}

DbBaseElements::TileNumber::tile_t DbBaseElements::TileNumber::to_tilenumber(const Rect & r)
{
	tile_t t1(to_tilenumber(r.get_southwest())), t2(to_tilenumber(r.get_northeast()));
	while (t1 != t2) {
		t1 >>= 1;
		t2 >>= 1;
	}
	return t1;
}

DbBaseElements::TileNumber::tilerange_t DbBaseElements::TileNumber::to_tilerange(const Rect & r)
{
	tilerange_t t(to_tilenumber(r.get_southwest()), to_tilenumber(r.get_northeast()));
	unsigned int sh(0);
	while (t.first != t.second) {
		t.first >>= 1;
		t.second >>= 1;
		sh++;
	}
	t.first <<= sh;
	t.second <<= sh;
	t.second |= (1 << sh) - 1;
	return t;
}

DbBaseElements::TileNumber::Iterator::Iterator(const Rect & r)
	: m_tile1(to_tilenumber(r.get_southwest())),
	  m_tile2(to_tilenumber(r.get_northeast())),
	  m_bits(0)
{
	m_tile = m_tile1;
}

DbBaseElements::TileNumber::tile_t DbBaseElements::TileNumber::Iterator::operator ( )(void) const
{
	return m_tile >> m_bits;
}

bool DbBaseElements::TileNumber::Iterator::next(void)
{
	tile_t tile_mask = (1 << tile_bits) - 1;
	tile_t lat_mask = 0x55555555 & tile_mask;
	tile_t lon_mask = 0xAAAAAAAA & tile_mask;
	if ((m_tile ^ m_tile2) & lon_mask) {
		// lon != lon2: increment lon
		m_tile = (m_tile & ~lon_mask) | (((m_tile & lon_mask) + ((3 << m_bits) & lon_mask) + ~lon_mask) & lon_mask);
		return m_bits <= tile_bits;
	}
	// lon = lon1
	m_tile = (m_tile & ~lon_mask) | (m_tile1 & lon_mask);
	if ((m_tile ^ m_tile2) & lat_mask) {
		// lat != lat2: increment lat
		m_tile = (m_tile & ~lat_mask) | (((m_tile & lat_mask) + ((3 << m_bits) & lat_mask) + ~lat_mask) & lat_mask);
		return m_bits <= tile_bits;
	}
	m_bits++;
	m_tile1 &= ~((1 << m_bits) - 1);
	m_tile2 &= ~((1 << m_bits) - 1);
	m_tile = m_tile1;
	return m_bits <= tile_bits;
}

const std::string& to_str(DbBaseElements::DbElementBase::table_t t)
{
	switch (t) {
	case DbBaseElements::DbElementBase::table_main:
	{
		static const std::string r("main");
		return r;
	}

	case DbBaseElements::DbElementBase::table_aux:
	{
		static const std::string r("aux");
		return r;
	}

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

DbBaseCommon::DbBaseCommon(void)
	: m_open(openstate_closed), m_has_rtree(false), m_has_aux_rtree(false)
{
}

DbBaseCommon::~DbBaseCommon(void)
{
	close();
}

void DbBaseCommon::close(void)
{
	if (m_open != openstate_closed)
		m_db.close();
	m_open = openstate_closed;
}

void DbBaseCommon::detach(void)
{
	if (m_open != openstate_auxopen)
		return;
	sqlite3x::sqlite3_command cmd(m_db, "DETACH DATABASE aux;");
	cmd.executenonquery();
	m_open = openstate_mainopen;
}

void DbBaseCommon::dbfunc_simpledist( sqlite3_context * ctxt, int , sqlite3_value ** values )
{
	Point pt1(sqlite3_value_int(values[0]), sqlite3_value_int(values[1]));
	Point pt2(sqlite3_value_int(values[2]), sqlite3_value_int(values[3]));
	sqlite3_result_int64(ctxt, pt1.simple_distance_rel(pt2));
}

void DbBaseCommon::dbfunc_simplerectdist(sqlite3_context * ctxt, int , sqlite3_value ** values)
{
	Rect r(Point(sqlite3_value_int(values[0]), sqlite3_value_int(values[1])),
	       Point(sqlite3_value_int(values[2]), sqlite3_value_int(values[3])));
	Point pt(sqlite3_value_int(values[4]), sqlite3_value_int(values[5]));
	sqlite3_result_int64(ctxt, r.simple_distance_rel(pt));
}

void DbBaseCommon::dbfunc_upperbound(sqlite3_context * ctxt, int , sqlite3_value ** values)
{
	Glib::ustring s((const char *)sqlite3_value_text(values[0]));
	if (false)
		std::cerr << "dbfunc_upperbound: in \"" << s << "\"" << std::endl;
	if (!s.empty())
		s.replace(--s.end(), s.end(), 1, (*(--s.end())) + 1);
	if (false)
		std::cerr << "dbfunc_upperbound: out \"" << s << "\"" << std::endl;
	sqlite3_result_text(ctxt, s.data(), s.bytes(), SQLITE_TRANSIENT);
}

Glib::ustring DbBaseCommon::quote_text_for_like(const Glib::ustring & s, char e)
{
	Glib::ustring r;
	for (Glib::ustring::const_iterator si(s.begin()), se(s.end()); si != se; si++) {
		if (*si == '%' || *si == '_' || (e && *si == e))
			r.push_back(e ? e : '%');
		r.push_back(*si);
	}
	return r;
}

void DbBaseCommon::sync_off(void)
{
	sqlite3x::sqlite3_command cmd(m_db, "PRAGMA synchronous = OFF;");
	cmd.executenonquery();
}

void DbBaseCommon::analyze(void)
{
	sqlite3x::sqlite3_command cmd(m_db, "ANALYZE main;");
	cmd.executenonquery();
}

void DbBaseCommon::vacuum(void)
{
	sqlite3x::sqlite3_command cmd(m_db, "VACUUM;");
	cmd.executenonquery();
}

void DbBaseCommon::set_exclusive(bool excl)
{
	sqlite3x::sqlite3_command cmd(m_db, excl ? "PRAGMA locking_mode=EXCLUSIVE;" :
				      "PRAGMA locking_mode=NORMAL;");
	cmd.executenonquery();
}

unsigned int DbBaseCommon::query_cache_size(void)
{
	sqlite3x::sqlite3_command cmd(m_db, "PRAGMA cache_size;");
	return cmd.executeint();
}

void DbBaseCommon::set_cache_size(unsigned int cs)
{
	sqlite3x::sqlite3_command cmd(m_db, "PRAGMA cache_size = ?;");
	cmd.bind(1, (long long int)cs);
	cmd.executenonquery();
}

void DbBaseCommon::interrupt(void)
{
	sqlite3_interrupt(m_db.db());
}

std::string DbBaseCommon::filename_to_uri(const std::string& fn)
{
	std::string fn2;
	for (std::string::const_iterator i(fn.begin()), e(fn.end()); i != e; ++i) {
		char ch(*i);
		switch (ch) {
		case '?':
			fn2 += "%3f";
			break;

		case '#':
			fn2 += "%23";
			break;

#ifdef __WIN32__
		case '\\':
			ch = '/';
			// fall through
#endif

		case '/':
			if (!fn2.empty() && fn2[fn2.size()-1] == '/')
				break;

			// fall through
		default:
			fn2.push_back(ch);
			break;
		}
	}
#ifdef __WIN32__
	if (fn2.size() >= 2 && fn2[1] == ':' && std::isalpha(fn2[0]))
		return "file:/" + fn2;
#endif
	return "file:" + fn2;
}

template<class T> DbBase<T>::DbBase()
	: DbBaseCommon()
{
}

template<class T> DbBase<T>::DbBase(const Glib::ustring& path)
	: DbBaseCommon()
{
	open(path);
}

template<class T> DbBase<T>::DbBase(const Glib::ustring& path, read_only_tag_t)
	: DbBaseCommon()
{
	open_readonly(path);
}

template<class T> DbBase<T>::DbBase(const Glib::ustring& path, const Glib::ustring& aux_path)
	: DbBaseCommon()
{
	open(path);
	if (!aux_path.empty() && is_dbfile_exists(aux_path))
	        attach(aux_path);
}

template<class T> DbBase<T>::DbBase(const Glib::ustring& path, const Glib::ustring& aux_path, read_only_tag_t)
	: DbBaseCommon()
{
	open_readonly(path);
	if (!aux_path.empty() && is_dbfile_exists(aux_path))
	        attach_readonly(aux_path);
}

template<class T> void DbBase<T>::open(const Glib::ustring & path, const char *dbfilename)
{
	close();
	Glib::ustring dbname(path);
	if (dbname.empty())
		dbname = FPlan::get_userdbpath();
	dbname = Glib::build_filename(dbname, dbfilename ? dbfilename : database_file_name);
	if (false)
		std::cerr << "DbBase: opening database " << dbname << std::endl;
	m_db.open(dbname);
	if (sqlite3_create_function(m_db.db(), "simpledist", 4, /*SQLITE_UTF8*/SQLITE_ANY, 0, &dbfunc_simpledist, 0, 0) != SQLITE_OK)
		throw std::runtime_error("DbBase: cannot add user function simpledist");
	if (sqlite3_create_function(m_db.db(), "simplerectdist", 6, /*SQLITE_UTF8*/SQLITE_ANY, 0, &dbfunc_simplerectdist, 0, 0) != SQLITE_OK)
		throw std::runtime_error("DbBase: cannot add user function simplerectdist");
	if (sqlite3_create_function(m_db.db(), "upperbound", 1, SQLITE_UTF8, 0, &dbfunc_upperbound, 0, 0) != SQLITE_OK)
		throw std::runtime_error("DbBase: cannot add user function upperbound");
	{
		DbSchemaArchiver dbschema(main_table_name);
		{
			element_t e;
			e.hibernate(dbschema);
		}
		if (false)
			dbschema.print_schema(std::cerr);
		dbschema.verify_schema(m_db);
	}
	create_tables();
	create_indices();
	m_has_rtree = m_has_aux_rtree = false;
	m_open = openstate_mainopen;
	if (area_data) {
		try {
			sqlite3x::sqlite3_command cmd1(m_db, std::string("SELECT ID FROM ") + main_table_name + " ORDER BY ID LIMIT 1;");
			int nr1 = cmd1.executeint();
			sqlite3x::sqlite3_command cmd2(m_db, std::string("SELECT ID FROM ") + main_table_name + "_rtree WHERE ID=?;");
			cmd2.bind(1, nr1);
			int nr2 = cmd2.executeint();
			m_has_rtree = (nr2 == nr1);
		} catch (...) {
		}
	}
	if (false)
		std::cerr << main_table_name << ": rtree: " << (m_has_rtree ? "yes" : "no") << std::endl;
}

template<class T> void DbBase<T>::open_readonly(const Glib::ustring& path, const char *dbfilename)
{
	close();
	Glib::ustring dbname(path);
	if (dbname.empty())
		dbname = FPlan::get_userdbpath();
	dbname = Glib::build_filename(dbname, dbfilename ? dbfilename : database_file_name);
	if (false)
		std::cerr << "DbBase: opening database " << dbname << std::endl;
	{
		sqlite3 *h(0);
		if (SQLITE_OK == sqlite3_open_v2(dbname.c_str(), &h, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI, 0)) {
			m_db.take(h);
		} else {
			throw sqlite3x::database_error(("unable to open database " + dbname).c_str());
		}
	}
	if (sqlite3_create_function(m_db.db(), "simpledist", 4, /*SQLITE_UTF8*/SQLITE_ANY, 0, &dbfunc_simpledist, 0, 0) != SQLITE_OK)
		throw std::runtime_error("DbBase: cannot add user function simpledist");
	if (sqlite3_create_function(m_db.db(), "simplerectdist", 6, /*SQLITE_UTF8*/SQLITE_ANY, 0, &dbfunc_simplerectdist, 0, 0) != SQLITE_OK)
		throw std::runtime_error("DbBase: cannot add user function simplerectdist");
	if (sqlite3_create_function(m_db.db(), "upperbound", 1, SQLITE_UTF8, 0, &dbfunc_upperbound, 0, 0) != SQLITE_OK)
		throw std::runtime_error("DbBase: cannot add user function upperbound");
	{
		DbSchemaArchiver dbschema(main_table_name);
		{
			element_t e;
			e.hibernate(dbschema);
		}
		if (false)
			dbschema.print_schema(std::cerr);
		dbschema.verify_schema(m_db);
	}
	m_has_rtree = m_has_aux_rtree = false;
	m_open = openstate_mainopen;
	if (area_data) {
		try {
			sqlite3x::sqlite3_command cmd1(m_db, std::string("SELECT ID FROM ") + main_table_name + " ORDER BY ID LIMIT 1;");
			int nr1 = cmd1.executeint();
			sqlite3x::sqlite3_command cmd2(m_db, std::string("SELECT ID FROM ") + main_table_name + "_rtree WHERE ID=?;");
			cmd2.bind(1, nr1);
			int nr2 = cmd2.executeint();
			m_has_rtree = (nr2 == nr1);
		} catch (...) {
		}
	}
	if (false)
		std::cerr << main_table_name << ": rtree: " << (m_has_rtree ? "yes" : "no") << std::endl;
}

template<class T> void DbBase<T>::attach(const Glib::ustring & path, const char *dbfilename)
{
	if (m_open == openstate_auxopen)
		detach();
	if (m_open != openstate_mainopen)
		return;
	Glib::ustring dbname(path);
	if (dbname.empty())
		dbname = PACKAGE_DATA_DIR;
	dbname += "/";
	dbname += dbfilename ? dbfilename : database_file_name;
	{
		sqlite3x::sqlite3_command cmd(m_db, "ATTACH DATABASE ?1 AS aux;");
		cmd.bind(1, dbname);
		cmd.executenonquery();
	}
	m_has_aux_rtree = false;
	if (area_data) {
		try {
			sqlite3x::sqlite3_command cmd1(m_db, std::string("SELECT ID FROM aux.") + main_table_name + " ORDER BY ID LIMIT 1;");
			int nr1 = cmd1.executeint();
			sqlite3x::sqlite3_command cmd2(m_db, std::string("SELECT ID FROM aux.") + main_table_name + "_rtree WHERE ID=?;");
			cmd2.bind(1, nr1);
			int nr2 = cmd2.executeint();
			m_has_aux_rtree = (nr2 == nr1);
		} catch (...) {
		}
	}
	m_open = openstate_auxopen;
	if (false)
		std::cerr << "aux." << main_table_name << ": rtree: " << (m_has_aux_rtree ? "yes" : "no") << std::endl;
}

template<class T> void DbBase<T>::attach_readonly(const Glib::ustring & path, const char *dbfilename)
{
	if (m_open == openstate_auxopen)
		detach();
	if (m_open != openstate_mainopen)
		return;
	Glib::ustring dbname(path);
	if (dbname.empty())
		dbname = PACKAGE_DATA_DIR;
	dbname += "/";
	dbname += dbfilename ? dbfilename : database_file_name;
	{
		sqlite3x::sqlite3_command cmd(m_db, "ATTACH DATABASE ?1 AS aux;");
		cmd.bind(1, filename_to_uri(dbname) + "?mode=ro");
		cmd.executenonquery();
	}
	m_has_aux_rtree = false;
	if (area_data) {
		try {
			sqlite3x::sqlite3_command cmd1(m_db, std::string("SELECT ID FROM aux.") + main_table_name + " ORDER BY ID LIMIT 1;");
			int nr1 = cmd1.executeint();
			sqlite3x::sqlite3_command cmd2(m_db, std::string("SELECT ID FROM aux.") + main_table_name + "_rtree WHERE ID=?;");
			cmd2.bind(1, nr1);
			int nr2 = cmd2.executeint();
			m_has_aux_rtree = (nr2 == nr1);
		} catch (...) {
		}
	}
	m_open = openstate_auxopen;
	if (false)
		std::cerr << "aux." << main_table_name << ": rtree: " << (m_has_aux_rtree ? "yes" : "no") << std::endl;
}

template<class T> bool DbBase<T>::is_dbfile_exists(const Glib::ustring & path)
{
	Glib::ustring dbname(path);
	if (dbname.empty())
		dbname = FPlan::get_userdbpath();
	dbname = Glib::build_filename(dbname, database_file_name);
	return Glib::file_test(dbname, Glib::FILE_TEST_IS_REGULAR);
}

template<class T> void DbBase<T>::take(sqlite3 * dbh)
{
	close();
	m_db.take(dbh);
	if (!m_db.db())
		return;
	if (sqlite3_create_function(m_db.db(), "simpledist", 4, /*SQLITE_UTF8*/SQLITE_ANY, 0, &dbfunc_simpledist, 0, 0) != SQLITE_OK)
		throw std::runtime_error("DbBase: cannot add user function simpledist");
	if (sqlite3_create_function(m_db.db(), "simplerectdist", 6, /*SQLITE_UTF8*/SQLITE_ANY, 0, &dbfunc_simplerectdist, 0, 0) != SQLITE_OK)
		throw std::runtime_error("DbBase: cannot add user function simplerectdist");
	if (sqlite3_create_function(m_db.db(), "upperbound", 1, SQLITE_UTF8, 0, &dbfunc_upperbound, 0, 0) != SQLITE_OK)
		throw std::runtime_error("DbBase: cannot add user function upperbound");
	create_tables();
	create_indices();
	m_has_rtree = m_has_aux_rtree = false;
	m_open = openstate_mainopen;
	if (area_data) {
		try {
			sqlite3x::sqlite3_command cmd1(m_db, std::string("SELECT ID FROM ") + main_table_name + " ORDER BY ID LIMIT 1;");
			int nr1 = cmd1.executeint();
			sqlite3x::sqlite3_command cmd2(m_db, std::string("SELECT ID FROM ") + main_table_name + "_rtree WHERE ID=?;");
			cmd2.bind(1, nr1);
			int nr2 = cmd2.executeint();
			m_has_rtree = (nr2 == nr1);
		} catch (...) {
		}
	}
	if (false)
		std::cerr << main_table_name << ": rtree: " << (m_has_rtree ? "yes" : "no") << std::endl;
}

template<class T> void DbBase<T>::purgedb(void)
{
	drop_indices();
	drop_tables();
	create_tables();
	create_indices();
}

template <class T> typename DbBase<T>::element_t DbBase<T>::operator()( int64_t id, table_t tbl, unsigned int loadsubtables )
{
	element_t e;
	std::string qs;
	if (tbl == element_t::table_aux) {
		if (m_open != openstate_auxopen)
			return e;
		qs = "SELECT " + std::string(element_t::db_aux_query_string) + " FROM aux." + main_table_name + " WHERE ID=? " +
		     " AND " + delete_field + " NOT IN (SELECT " + delete_field + " FROM " + main_table_name + "_deleted);";
	} else {
		qs = "SELECT " + std::string(element_t::db_query_string) + " FROM " + main_table_name + " WHERE ID=?;";
	}
	sqlite3x::sqlite3_command cmd(m_db, qs);
	cmd.bind(1, (long long int)id);
	sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
	e.load(cursor, m_db, loadsubtables);
	return e;
}

template<class T> void DbBase<T>::loadfirst( element_t & e, bool include_aux, unsigned int loadsubtables )
{
	e = element_t();
	{
		sqlite3x::sqlite3_command cmd(m_db, "SELECT " + std::string(element_t::db_query_string) + " FROM " + main_table_name + " ORDER BY ID ASC LIMIT 1;");
		sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
		e.load(cursor, m_db, loadsubtables);
	}
	if (e.is_valid() || m_open != openstate_auxopen || !include_aux)
		return;
	{
		sqlite3x::sqlite3_command cmd(m_db, "SELECT " + std::string(element_t::db_aux_query_string) + " FROM aux." + main_table_name +
					      " WHERE " + delete_field + " NOT IN (SELECT " + delete_field + " FROM " + main_table_name + "_deleted) ORDER BY ID ASC LIMIT 1;");
		sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
		e.load(cursor, m_db, loadsubtables);
	}
}

template<class T> void DbBase<T>::loadnext( element_t & e, bool include_aux, unsigned int loadsubtables )
{
	uint64_t id(e.get_id());
	table_t tbl(e.get_table());
	e = element_t();
	if (tbl == element_t::table_aux) {
		if (m_open != openstate_auxopen)
			return;
		sqlite3x::sqlite3_command cmd(m_db, "SELECT " + std::string(element_t::db_aux_query_string) + " FROM aux." + main_table_name + " WHERE ID > ? " +
					      " AND " + delete_field + " NOT IN (SELECT " + delete_field + " FROM " + main_table_name + "_deleted) ORDER BY ID ASC LIMIT 1;");
		cmd.bind(1, (long long int)id);
		sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
		e.load(cursor, m_db, loadsubtables);
		return;
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "SELECT " + std::string(element_t::db_query_string) + " FROM " + main_table_name + " WHERE ID > ? ORDER BY ID ASC LIMIT 1;");
		cmd.bind(1, (long long int)id);
		sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
		e.load(cursor, m_db, loadsubtables);
	}
	if (e.is_valid() || m_open != openstate_auxopen || !include_aux)
		return;
	{
		sqlite3x::sqlite3_command cmd(m_db, "SELECT " + std::string(element_t::db_aux_query_string) + " FROM aux." + main_table_name +
						    " WHERE " + delete_field + " NOT IN (SELECT " + delete_field + " FROM " + main_table_name + "_deleted) ORDER BY ID ASC LIMIT 1;");
		sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
		e.load(cursor, m_db, loadsubtables);
	}
}

template <class T> void DbBase<T>::for_each(ForEach& cb, bool include_aux, unsigned int loadsubtables)
{
	if (!include_aux) {
		std::string qs("SELECT " + std::string(delete_field) + " FROM " + main_table_name + "_deleted ORDER BY " + delete_field + ";");
		sqlite3x::sqlite3_command cmd(m_db, qs);
		sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
		while (cursor.step())
			if (!cb(cursor.getstring(0)))
				break;
	}
	std::string qs;
	if (include_aux && m_open == openstate_auxopen) {
		qs += "SELECT " + std::string(element_t::db_aux_query_string) + " FROM aux." + main_table_name +
		      " WHERE " + delete_field + " NOT IN (SELECT " + delete_field + " FROM " + main_table_name + "_deleted) UNION ";
	}
	qs += "SELECT " + std::string(element_t::db_query_string) + " FROM " + main_table_name + " ORDER BY " + order_field;
	sqlite3x::sqlite3_command cmd(m_db, qs);
	sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
	for (;;) {
		element_t e;
		e.load(cursor, m_db, loadsubtables);
		if (!e.is_valid())
			break;
		if (!cb(e))
			break;
	}
}

template <class T> void DbBase<T>::for_each_by_rect(ForEach & cb, const Rect & r, bool include_aux, unsigned int loadsubtables)
{
	std::string qs;
	if (include_aux && m_open == openstate_auxopen) {
		qs += "SELECT " + get_area_select_string(true, r) +
		      " AND " + delete_field + " NOT IN (SELECT " + delete_field + " FROM " + main_table_name + "_deleted) UNION ";
	}
	qs += "SELECT " + get_area_select_string(false, r);
	if (order_field) {
		qs += " ORDER BY ";
		qs += order_field;
	}
	qs += ";";
	if (false) {
		if (false)
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
	sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
	for (;;) {
		element_t e;
		e.load(cursor, m_db, loadsubtables);
		if (!e.is_valid())
			break;
		if (!cb(e))
			break;
	}
}

template <class T> typename DbBase<T>::elementvector_t DbBase<T>::find(const std::string& fldname, const std::string& pattern, char escape, comp_t comp, unsigned int limit, unsigned int loadsubtables)
{
	std::string qsfld;
	Glib::ustring pat(pattern);
	switch (comp) {
	default:
	case DbQueryInterfaceCommon::comp_startswith:
		escape = 0;
		qsfld += std::string("(") + fldname + ">=?1 COLLATE NOCASE) AND (" + fldname + "<upperbound(?1) COLLATE NOCASE)";
		break;

	case DbQueryInterfaceCommon::comp_exact:
		escape = 0;
		qsfld += std::string("(") + fldname + "=?1 COLLATE NOCASE)";
		break;

	case DbQueryInterfaceCommon::comp_exact_casesensitive:
		escape = 0;
		qsfld += std::string("(") + fldname + "=?1)";
		break;

	case DbQueryInterfaceCommon::comp_contains:
		escape = '!';
		pat = std::string("%") + quote_text_for_like(pat, escape) + "%";
		// fall through

	case DbQueryInterfaceCommon::comp_like:
		qsfld += std::string("(") + fldname + " LIKE ?1";
		if (escape)
			qsfld += " ESCAPE ?2";
		qsfld += ')';
		break;
	}
	std::string qs;
	if (m_open == openstate_auxopen) {
		qs += "SELECT " + std::string(element_t::db_aux_query_string) + " FROM aux." + main_table_name + " WHERE " + qsfld +
		      " AND " + delete_field + " NOT IN (SELECT " + delete_field + " FROM " + main_table_name + "_deleted) UNION ";
	}
	qs += "SELECT " + std::string(element_t::db_query_string) + " FROM " + main_table_name + " WHERE " + qsfld;
	if (order_field) {
		qs += " ORDER BY ";
		qs += order_field;
	}
	if (limit)
		qs += " LIMIT ?3";
	qs += ";";
	if (false)
		std::cerr << "sql: " << qs << std::endl;
	sqlite3x::sqlite3_command cmd(m_db, qs);
	cmd.bind(1, pat);
	if (escape)
		cmd.bind(2, std::string(1, escape));
	if (limit)
		cmd.bind(3, (long long int)limit);
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

template <class T> typename DbBase<T>::elementvector_t DbBase<T>::find_by_text(const std::string& pattern, char escape, comp_t comp,  unsigned int limit, unsigned int loadsubtables)
{
	std::string qsfld;
	Glib::ustring pat(pattern);
	switch (comp) {
	default:
	case DbQueryInterfaceCommon::comp_startswith:
		escape = 0;
		break;

	case DbQueryInterfaceCommon::comp_contains:
		escape = '!';
		pat = std::string("%") + quote_text_for_like(pat, escape) + "%";
		break;

	case DbQueryInterfaceCommon::comp_exact:
		escape = 0;
		break;

	case DbQueryInterfaceCommon::comp_like:
		break;
	}
	{
		bool needor(false);
		for (const char **fld = element_t::db_text_fields; *fld; fld++) {
			if (needor)
				qsfld += " OR ";
			const char *fldname(*fld);
			switch (comp) {
			default:
			case DbQueryInterfaceCommon::comp_startswith:
				qsfld += std::string("(") + fldname + ">=?1 COLLATE NOCASE) AND (" + fldname + "<upperbound(?1) COLLATE NOCASE)";
				break;

			case DbQueryInterfaceCommon::comp_exact:
				qsfld += std::string("(") + fldname + "=?1 COLLATE NOCASE)";
				break;

			case DbQueryInterfaceCommon::comp_exact_casesensitive:
				qsfld += std::string("(") + fldname + "=?1)";
				break;

			case DbQueryInterfaceCommon::comp_contains:
			case DbQueryInterfaceCommon::comp_like:
				qsfld += std::string("(") + fldname + " LIKE ?1";
				if (escape)
					qsfld += " ESCAPE ?2";
				qsfld += ')';
				break;
			}
			needor = true;
		}
	}
	std::string qs;
	if (m_open == openstate_auxopen) {
		qs += "SELECT " + std::string(element_t::db_aux_query_string) + " FROM aux." + main_table_name + " WHERE (" + qsfld +
		      ") AND " + delete_field + " NOT IN (SELECT " + delete_field + " FROM " + main_table_name + "_deleted) UNION ";
	}
	qs += "SELECT " + std::string(element_t::db_query_string) + " FROM " + main_table_name + " WHERE (" + qsfld + ')';
	if (order_field) {
		qs += " ORDER BY ";
		qs += order_field;
	}
	if (limit)
		qs += " LIMIT ?3";
	qs += ";";
	if (false)
		std::cerr << "sql: " << qs << " pat: " << pat << std::endl;
	sqlite3x::sqlite3_command cmd(m_db, qs);
	cmd.bind(1, pat);
	if (escape)
		cmd.bind(2, std::string(1, escape));
	if (limit)
		cmd.bind(3, (long long int)limit);
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

template <class T> typename DbBase<T>::elementvector_t DbBase<T>::find_by_time(time_t fromtime, time_t totime, unsigned int limit, unsigned int loadsubtables)
{
	std::string qsfld;
	{
		bool needor(false);
		for (const char **fld = element_t::db_time_fields; *fld; fld++) {
			if (needor)
				qsfld += " OR ";
			qsfld += std::string("(") + (*fld) + " BETWEEN ?1 AND ?2)";
			needor = true;
		}
	}
	std::string qs;
	if (m_open == openstate_auxopen) {
		qs += "SELECT " + std::string(element_t::db_aux_query_string) + " FROM aux." + main_table_name + " WHERE (" + qsfld +
		      ") AND " + delete_field + " NOT IN (SELECT " + delete_field + " FROM " + main_table_name + "_deleted) UNION ";
	}
	qs += "SELECT " + std::string(element_t::db_query_string) + " FROM " + main_table_name + " WHERE (" + qsfld + ')';
	if (order_field) {
		qs += " ORDER BY ";
		qs += order_field;
	}
	if (limit)
		qs += " LIMIT ?3";
	qs += ";";
	if (false)
		std::cerr << "sql: " << qs << std::endl;
	sqlite3x::sqlite3_command cmd(m_db, qs);
	cmd.bind(1, (long long int)fromtime);
	cmd.bind(2, (long long int)totime);
	if (limit)
		cmd.bind(3, (long long int)limit);
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

template <class T> typename DbBase<T>::elementvector_t DbBase<T>::loadid(uint64_t startid, table_t table, const std::string & order, unsigned int limit, unsigned int loadsubtables)
{
	std::string qs;
	elementvector_t ret;
	if (table == element_t::table_aux) {
		if (m_open != openstate_auxopen)
			return ret;
		qs = "SELECT " + std::string(element_t::db_aux_query_string) + " FROM aux." + main_table_name + " WHERE ID>=?1" +
		     " AND " + delete_field + " NOT IN (SELECT " + delete_field + " FROM " + main_table_name + "_deleted)";
	} else {
		qs = "SELECT " + std::string(element_t::db_query_string) + " FROM " + main_table_name + " WHERE ID>=?1";
	}
	if (!order.empty())
		qs += " ORDER BY " + order;
	if (limit)
		qs += " LIMIT ?2";
	qs += ";";
	if (false)
		std::cerr << "sql: " << qs << std::endl;
	sqlite3x::sqlite3_command cmd(m_db, qs);
	cmd.bind(1, (long long int)startid);
	if (limit)
		cmd.bind(2, (long long int)limit);
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

template<class T> std::string DbBase<T>::get_area_select_string(bool auxtable, const Rect& r, const char *sortcol)
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
	bool rtree(auxtable ? m_has_aux_rtree : m_has_rtree);
	oss << " WHERE ";
	if (rtree) {
		oss << "ID IN (SELECT ID FROM " << table_name << "_rtree WHERE "
				<< table_name << "_rtree.NELAT >= ?2 AND " << table_name << "_rtree.SWLAT <= ?4 AND (("
				//<< table_name << "_rtree.SWLON <= ?3-4294967296 AND " << table_name << "_rtree.NELON >= ?1-4294967296) OR ("
				//<< table_name << "_rtree.SWLON <= ?3 AND " << table_name << "_rtree.NELON >= ?1) OR ("
				//<< table_name << "_rtree.SWLON <= ?3+4294967296 AND " << table_name << "_rtree.NELON >= ?1+4294967296))";
				<< table_name << "_rtree.SWLON <= ?3 AND " << table_name << "_rtree.NELON >= ?1)))";
				return oss.str();
	}
	if (false) {
		oss << '(';
		typename element_t::tilerange_t tr(element_t::TileNumber::to_tilerange(r));
		bool needor(false);
		for (typename element_t::TileNumber::tile_t mask = (1 << element_t::TileNumber::tile_bits) - 1; mask; mask >>= 1, tr.first >>= 1, tr.second >>= 1) {
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
		typename element_t::tilerange_t tr(element_t::TileNumber::to_tilerange(r));
		bool needcomma(false);
		for (typename element_t::TileNumber::tile_t mask = (1 << element_t::TileNumber::tile_bits) - 1; mask; mask >>= 1, tr.first >>= 1, tr.second >>= 1) {
			for (typename element_t::TileNumber::tile_t tile = tr.first;; tile = (tile & ~mask) | ((tile + 1) & mask)) {
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
		typename element_t::TileNumber::Iterator ti(r);
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
	return oss.str();
}

template <class T> typename DbBase<T>::elementvector_t DbBase<T>::find_by_rect(const Rect & r, unsigned int limit, unsigned int loadsubtables)
{
	if (false)
		std::cerr << "find_by_rect: loadsubtables 0x" << std::hex << loadsubtables << std::endl;
	std::string qs;
	if (m_open == openstate_auxopen) {
		qs += "SELECT " + get_area_select_string(true, r) +
		      " AND " + delete_field + " NOT IN (SELECT " + delete_field + " FROM " + main_table_name + "_deleted) UNION ";
	}
	qs += "SELECT " + get_area_select_string(false, r);
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
		std::cerr << "find_by_rect: " << main_table_name << " rect " << r << " retsize " << ret.size() << " qs " << qs << std::endl;
	return ret;
}

template <class T> typename DbBase<T>::elementvector_t DbBase<T>::find_nearest(const Point & pt, const Rect & r, unsigned int limit, unsigned int loadsubtables)
{
	const char *sortcol(area_data ? "(simplerectdist(SWLON,SWLAT,NELON,NELAT,?5,?6)) AS 'SORTCOL'" : "(simpledist(LON,LAT,?5,?6)) AS 'SORTCOL'");
	std::string qs;
	if (m_open == openstate_auxopen) {
		qs += "SELECT " + get_area_select_string(true, r, sortcol) +
		      " AND " + delete_field + " NOT IN (SELECT " + delete_field + " FROM " + main_table_name + "_deleted) UNION ";
	}
	qs += "SELECT " + get_area_select_string(false, r, sortcol);
	qs += " ORDER BY SORTCOL";
	if (limit)
		qs += " LIMIT ?7";
	qs += ";";
	if (false) {
		if (false)
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
	cmd.bind(5, (long long int)pt.get_lon());
	cmd.bind(6, (long long int)pt.get_lat());
	if (limit)
		cmd.bind(7, (long long int)limit);
	elementvector_t ret;
	sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
	if (false) {
		for (int i = 0; i < cursor.colcount(); i++)
			std::cerr << "Column " << i << " Name " << cursor.getcolname(i) << std::endl;
	}
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

template <class T> typename DbBase<T>::elementvector_t DbBase<T>::find_nulltile(unsigned int limit, unsigned int loadsubtables)
{
	std::string qs;
	if (m_open == openstate_auxopen) {
		qs += std::string("SELECT ") + element_t::db_aux_query_string + " FROM aux." +
			main_table_name	+ " WHERE TILE IS NULL AND " + delete_field + " NOT IN (SELECT " +
			delete_field + " FROM " + main_table_name + "_deleted) UNION ";
	}
	qs += std::string("SELECT ") + element_t::db_query_string + " FROM " +
		main_table_name	+ " WHERE TILE IS NULL";
	if (limit)
		qs += " LIMIT ?1";
	qs += ";";
	if (false) {
		if (false)
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
	if (limit)
		cmd.bind(1, (long long int)limit);
	elementvector_t ret;
	sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
	if (false) {
		for (int i = 0; i < cursor.colcount(); i++)
			std::cerr << "Column " << i << " Name " << cursor.getcolname(i) << std::endl;
	}
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

template <class T> void DbBase<T>::load_subtables(element_t& el, unsigned int loadsubtables)
{
	if (!loadsubtables)
		return;
	el.load_subtables(m_db, loadsubtables);
}

const unsigned int DbBaseElements::DbElementBase::hibernate_none;
const unsigned int DbBaseElements::DbElementBase::hibernate_id;
const unsigned int DbBaseElements::DbElementBase::hibernate_xmlnonzero;
const unsigned int DbBaseElements::DbElementBase::hibernate_sqlnotnull;
const unsigned int DbBaseElements::DbElementBase::hibernate_sqlunique;
const unsigned int DbBaseElements::DbElementBase::hibernate_sqlindex;
const unsigned int DbBaseElements::DbElementBase::hibernate_sqlnocase;

DbBaseElements::DbElementSourceBase::DbElementSourceBase()
	: m_sourceid(), m_modtime(0)
{
}

DbBaseElements::DbElementLabelBase::DbElementLabelBase()
	: m_label_placement(label_off)
{
}

const Glib::ustring& DbBaseElements::DbElementLabelBase::get_label_placement_name(label_placement_t lp)
{
	switch (lp) {
	default:
	{
		static const Glib::ustring r("Invalid");
		return r;
	}

	case label_n:
	{
		static const Glib::ustring r("N");
		return r;
	}

	case label_e:
	{
		static const Glib::ustring r("E");
		return r;
	}

	case label_s:
	{
		static const Glib::ustring r("S");
		return r;
	}

	case label_w:
	{
		static const Glib::ustring r("W");
		return r;
	}

	case label_center:
	{
		static const Glib::ustring r("Center");
		return r;
	}

	case label_any:
	{
		static const Glib::ustring r("Any");
		return r;
	}

	case label_off:
	{
		static const Glib::ustring r("Off");
		return r;
	}
	}
}

DbBaseElements::DbElementLabelSourceBase::DbElementLabelSourceBase()
	: m_sourceid(), m_modtime(0)
{
}

DbBaseElements::DbElementLabelSourceCoordBase::DbElementLabelSourceCoordBase()
{
	m_coord.set_invalid();
}

DbBaseElements::DbElementLabelSourceCoordIcaoNameBase::DbElementLabelSourceCoordIcaoNameBase()
	: m_icao(), m_name()
{
}

Glib::ustring DbBaseElements::DbElementLabelSourceCoordIcaoNameBase::get_icao_name(void) const
{
	Glib::ustring r(m_icao);
	if (!(m_icao.empty() || m_name.empty()))
		r += " ";
	return r + m_name;
}

DbBaseElements::DbElementLabelSourceCoordIcaoNameAuthorityBase::DbElementLabelSourceCoordIcaoNameAuthorityBase()
	: m_authority()
{
}

DbBaseElements::DbElementLabelSourceCoordIcaoNameAuthorityBase::authorityset_t DbBaseElements::DbElementLabelSourceCoordIcaoNameAuthorityBase::get_authorityset(void) const
{
	authorityset_t as;
	for (Glib::ustring::size_type n(0);;) {
		Glib::ustring::size_type n1(m_authority.find(',', n));
		if (n1 == Glib::ustring::npos) {
			if (n < m_authority.size())
				as.insert(m_authority.substr(n));
			return as;
		}
		if (n1 > n)
			as.insert(m_authority.substr(n, n1 - n));
		n = n1 + 1;
	}
	return as;
}

void DbBaseElements::DbElementLabelSourceCoordIcaoNameAuthorityBase::set_authorityset(const authorityset_t& as)
{
	m_authority.clear();
	bool subseq(false);
	for (authorityset_t::const_iterator ai(as.begin()), ae(as.end()); ai != ae; ++ai) {
		if (subseq)
			m_authority += ',';
		subseq = true;
		m_authority += *ai;
	}
}

void DbBaseElements::DbElementLabelSourceCoordIcaoNameAuthorityBase::add_authority(const Glib::ustring& au)
{
	authorityset_t as(get_authorityset());
	as.insert(au);
	set_authorityset(as);
}

bool DbBaseElements::DbElementLabelSourceCoordIcaoNameAuthorityBase::is_authority(const Glib::ustring& au) const
{
	authorityset_t as(get_authorityset());
	return as.find(au) != as.end();
}

template <class T> void DbQueryInterfaceSpecCommon<T>::load_subtables(elementvector_t& ev, unsigned int loadsubtables)
{
	if (!loadsubtables)
		return;
	for (typename elementvector_t::iterator ei(ev.begin()), ee(ev.end()); ei != ee; ++ei)
		load_subtables(*ei, loadsubtables);
}

template class DbQueryInterfaceSpecCommon<DbBaseElements::Navaid>;
template class DbQueryInterfaceSpecCommon<DbBaseElements::Waypoint>;
template class DbQueryInterfaceSpecCommon<DbBaseElements::Airway>;
template class DbQueryInterfaceSpecCommon<DbBaseElements::Airport>;
template class DbQueryInterfaceSpecCommon<DbBaseElements::Airspace>;
template class DbQueryInterfaceSpecCommon<DbBaseElements::Mapelement>;
template class DbQueryInterfaceSpecCommon<DbBaseElements::Track>;
template class DbQueryInterfaceSpecCommon<DbBaseElements::Label>;

template class DbQueryInterface<DbBaseElements::Navaid>;
template class DbQueryInterface<DbBaseElements::Waypoint>;
template class DbQueryInterface<DbBaseElements::Airway>;
template class DbQueryInterface<DbBaseElements::Airport>;
template class DbQueryInterface<DbBaseElements::Airspace>;
template class DbQueryInterface<DbBaseElements::Mapelement>;
template class DbQueryInterface<DbBaseElements::Track>;
template class DbQueryInterface<DbBaseElements::Label>;

template class DbBase<NavaidsDb::Navaid>;
template class DbBase<WaypointsDb::Waypoint>;
template class DbBase<AirportsDb::Airport>;
template class DbBase<AirspacesDb::Airspace>;
template class DbBase<AirwaysDb::Airway>;
template class DbBase<MapelementsDb::Mapelement>;
template class DbBase<WaterelementsDb::Waterelement>;
template class DbBase<LabelsDb::Label>;
template class DbBase<TracksDb::Track>;

#ifdef HAVE_PQXX

PGDbBaseCommon::PGDbBaseCommon(pqxx::connection_base& conn)
	: m_conn(conn), m_has_rtree(false), m_readonly(false)
{
}

PGDbBaseCommon::PGDbBaseCommon(pqxx::connection_base& conn, read_only_tag_t)
	: m_conn(conn), m_has_rtree(false), m_readonly(true)
{
}

PGDbBaseCommon::~PGDbBaseCommon(void)
{
}

void PGDbBaseCommon::close(void)
{
	m_conn.disconnect();
}

void PGDbBaseCommon::sync_off(void)
{
	try {
		pqxx::work w(m_conn);
		pqxx::result r = w.exec("SET synchronous_commit = 'off'");
		w.commit();
	} catch (const std::exception &e) {
		std::cerr << "PGDbBaseCommon::sync_off: " << e.what() << std::endl;
	}
}

void PGDbBaseCommon::analyze(void)
{
	try {
	        pqxx::nontransaction w(m_conn);
		pqxx::result r = w.exec("VACUUM (ANALYZE)");
		w.commit();
	} catch (const std::exception &e) {
		std::cerr << "PGDbBaseCommon::analyze: " << e.what() << std::endl;
	}
}

void PGDbBaseCommon::vacuum(void)
{
	try {
		pqxx::nontransaction w(m_conn);
		pqxx::result r = w.exec("VACUUM (FULL)");
		w.commit();
	} catch (const std::exception &e) {
		std::cerr << "PGDbBaseCommon::vacuum: " << e.what() << std::endl;
	}
}

void PGDbBaseCommon::interrupt(void)
{
	m_conn.cancel_query();
}

Glib::ustring PGDbBaseCommon::quote_text_for_like(const Glib::ustring & s, char e)
{
	Glib::ustring r;
	for (Glib::ustring::const_iterator si(s.begin()), se(s.end()); si != se; si++) {
		if (*si == '%' || *si == '_' || (e && *si == e))
			r.push_back(e ? e : '%');
		r.push_back(*si);
	}
	return r;
}

void PGDbBaseCommon::define_dbfunc(void)
{
	try {
		pqxx::work w(m_conn);
		w.exec("CREATE SCHEMA IF NOT EXISTS aviationdb;\n");
		w.commit();
	} catch (const std::exception &e) {
		std::cerr << "PGDbBaseCommon::define_dbfunc: schema: " << e.what() << std::endl;
	}
	std::set<std::string> proc;
	try {
		pqxx::read_transaction w(m_conn);
		pqxx::result r(w.exec("SELECT proname FROM pg_catalog.pg_proc,pg_catalog.pg_namespace "
				      "WHERE pronamespace=pg_namespace.oid AND nspname='aviationdb' AND"
				      " proname IN ('simpledist','simplerectdist','upperbound','pointtogeom');"));
		for (pqxx::result::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri)
			if (!(*ri)[0].is_null())
				proc.insert((*ri)[0].as<std::string>());
	} catch (const std::exception &e) {
		if (false)
			std::cerr << "PGDbBaseCommon::define_dbfunc: function list: " << e.what() << std::endl;
	}
	if (proc.find("simpledist") == proc.end()) {
		try {
			pqxx::work w(m_conn);
			w.exec("CREATE FUNCTION aviationdb.simpledist(lon0 INTEGER, lat0 INTEGER, lon1 INTEGER, lat1 INTEGER) RETURNS BIGINT AS\n"
			       "$BODY$\n"
			       "DECLARE\n"
			       "    lon INTEGER;\n"
			       "    lat INTEGER;\n"
			       "BEGIN\n"
			       "    lon = lon1 - lon0;\n"
			       "    lat = lat1 - lat0;\n"
			       "    lon = lon * COS((lat0 + lat1) * (pi() / 2147483648::REAL * 0.5));\n"
			       "    RETURN lon::BIGINT * lon::BIGINT + lat::BIGINT * lat::BIGINT;\n"
			       "END;\n"
			       "$BODY$ LANGUAGE plpgsql;\n");
			w.commit();
		} catch (const std::exception &e) {
			if (false)
				std::cerr << "PGDbBaseCommon::define_dbfunc: simpledist: " << e.what() << std::endl;
		}
	}
	if (proc.find("simplerectdist") == proc.end()) {
		try {
			pqxx::work w(m_conn);
			w.exec("CREATE FUNCTION aviationdb.simplerectdist(lon0 INTEGER, lat0 INTEGER, lon1 INTEGER, lat1 INTEGER, lon INTEGER, lat INTEGER) RETURNS BIGINT AS\n"
			       "$BODY$\n"
			       "DECLARE\n"
			       "    r BIGINT;\n"
			       "    r1 BIGINT;\n"
			       "BEGIN\n"
			       "    r = 0;\n"
			       "    IF ((lon::BIGINT - lon0::BIGINT) & 4294967295::BIGINT) <= ((lon1::BIGINT - lon0::BIGINT) & 4294967295::BIGINT) THEN\n"
			       "        IF lat >= lat0 AND lat <= lat1 THEN\n"
			       "            RETURN 0;\n"
			       "        END IF;\n"
			       "        r = simpledist(lon, lat0, lon, lat);\n"
			       "        r1 = simpledist(lon, lat1, lon, lat);\n"
			       "        IF r1 < r THEN\n"
			       "            r = r1;\n"
			       "        END IF;\n"
			       "    ELSIF lat >= lat0 AND lat <= lat1 THEN\n"
			       "        r = simpledist(lon0, lat, lon, lat);\n"
			       "        r1 = simpledist(lon1, lat, lon, lat);\n"
			       "        IF r1 < r THEN\n"
			       "            r = r1;\n"
			       "        END IF;\n"
			       "    END IF;\n"
			       "    r1 = simpledist(lon0, lat0, lon, lat);\n"
			       "    IF r1 < r THEN\n"
			       "        r = r1;\n"
			       "    END IF;\n"
			       "    r1 = simpledist(lon0, lat1, lon, lat);\n"
			       "    IF r1 < r THEN\n"
			       "        r = r1;\n"
			       "    END IF;\n"
			       "    r1 = simpledist(lon1, lat0, lon, lat);\n"
			       "    IF r1 < r THEN\n"
			       "        r = r1;\n"
			       "    END IF;\n"
			       "    r1 = simpledist(lon1, lat1, lon, lat);\n"
			       "    IF r1 < r THEN\n"
			       "        r = r1;\n"
			       "    END IF;\n"
			       "    RETURN r;\n"
			       "END;\n"
			       "$BODY$ LANGUAGE plpgsql;\n");
			w.commit();
		} catch (const std::exception &e) {
			if (false)
				std::cerr << "PGDbBaseCommon::define_dbfunc: simplerectdist: " << e.what() << std::endl;
		}
	}
	if (proc.find("upperbound") == proc.end()) {
		try {
			pqxx::work w(m_conn);
			w.exec("CREATE FUNCTION aviationdb.upperbound(s VARCHAR) RETURNS VARCHAR AS\n"
			       "$BODY$\n"
			       "DECLARE\n"
			       "    lon INTEGER;\n"
			       "    lat INTEGER;\n"
			       "BEGIN\n"
			       "    IF char_length(s) > 0 THEN\n"
			       "        s = left(s, char_length(s)-1) || chr(ascii(right(s, 1)) + 1);\n"
			       "    END IF;\n"
			       "    RETURN s;\n"
			       "END;\n"
			       "$BODY$ LANGUAGE plpgsql;\n");
			w.commit();
		} catch (const std::exception &e) {
			if (false)
				std::cerr << "PGDbBaseCommon::define_dbfunc: upperbound: " << e.what() << std::endl;
		}
	}
	if (proc.find("pointtogeom") == proc.end()) {
		try {
			pqxx::work w(m_conn);
			w.exec("CREATE FUNCTION aviationdb.pointtogeom(lon INTEGER, lat INTEGER) RETURNS ext.GEOMETRY(POINT, 4326) AS\n"
			       "$BODY$\n"
			       "BEGIN\n"
			       "    IF lat > 1073741824 OR lat < -1073741824 THEN\n"
			       "        RETURN NULL;\n"
			       "    END IF;\n"
			       "    RETURN ext.ST_SetSRID(ext.ST_MakePoint(lon * (90.0 / 1073741824.0), lat * (90.0 / 1073741824.0)), 4326);\n"
			       "END;\n"
			       "$BODY$ LANGUAGE plpgsql;\n");
			w.commit();
		} catch (const std::exception &e) {
			if (false)
				std::cerr << "PGDbBaseCommon::define_dbfunc: pointtogeom: " << e.what() << std::endl;
		}
	}
}

void PGDbBaseCommon::drop_common_tables(void)
{
	pqxx::work w(m_conn);
	w.exec("DROP TABLE IF EXISTS aviationdb.labelsourcecoordicaonameauthoritybase;");
	w.exec("DROP TABLE IF EXISTS aviationdb.labelsourcecoordicaonamebase;");
	w.exec("DROP TABLE IF EXISTS aviationdb.labelsourcecoordbase;");
	w.exec("DROP TABLE IF EXISTS aviationdb.labelsourcebase;");
	w.exec("DROP TABLE IF EXISTS aviationdb.labelcoordbase;");
	w.exec("DROP TABLE IF EXISTS aviationdb.labelbase;");
	w.exec("DROP TABLE IF EXISTS aviationdb.sourcebase;");
	w.exec("DROP TABLE IF EXISTS aviationdb.base;");
	w.commit();
}

void PGDbBaseCommon::create_common_tables(void)
{
	pqxx::work w(m_conn);
	w.exec("CREATE SCHEMA IF NOT EXISTS aviationdb");
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.base ("
	       "ID BIGINT PRIMARY KEY NOT NULL"
	       ");");
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.sourcebase ("
	       "SRCID TEXT UNIQUE NOT NULL,"
	       "MODIFIED BIGINT"
	       ") INHERITS (aviationdb.base);");
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.labelbase ("
	       "LABELPLACEMENT INTEGER"
	       ") INHERITS (aviationdb.base);");
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.labelcoordbase ("
	       "LAT INTEGER,"
	       "LON INTEGER"
	       ") INHERITS (aviationdb.labelbase);");
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.labelsourcebase ("
	       "LABELPLACEMENT SMALLINT"
	       ") INHERITS (aviationdb.sourcebase);");
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.labelsourcecoordbase ("
	       "LAT INTEGER,"
	       "LON INTEGER"
	       ") INHERITS (aviationdb.labelsourcebase);");
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.labelsourcecoordicaonamebase ("
	       "ICAO TEXT,"
	       "NAME TEXT"
	       ") INHERITS (aviationdb.labelsourcecoordbase);");
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.labelsourcecoordicaonameauthoritybase ("
	       "AUTHORITY TEXT"
	       ") INHERITS (aviationdb.labelsourcecoordicaonamebase);");
	w.commit();
}

std::string PGDbBaseCommon::get_str(const uint8_t *wb, const uint8_t *ep)
{
	if (ep == wb)
		return "NULL";
	std::ostringstream oss;
	oss << "ext.ST_SetSRID(ext.ST_GeomFromWKB(E'";
	for (const uint8_t *cp(wb); cp != ep; ++cp) {
		if (*cp < ' ' || *cp >= 127 || *cp == '"' || *cp == '\'' || *cp == '\\') {
			oss << "\\\\" << (char)('0' + ((*cp >> 6) & 3)) << (char)('0' + ((*cp >> 3) & 7)) << (char)('0' + (*cp & 7));
			continue;
		}
		oss << *cp;
	}
	oss << "'),4326)";
	return oss.str();
}

std::string PGDbBaseCommon::get_str(const MultiPolygonHole& p)
{
	MultiPolygonHole p1(p);
	for (MultiPolygonHole::iterator pi(p1.begin()), pe(p1.end()); pi != pe; ) {
		if (pi->get_exterior().size() < 3) {
			pi = p1.erase(pi);
			pe = p1.end();
			continue;
		}
		for (unsigned int i(0), n(pi->get_nrinterior()); i < n; ) {
			if ((*pi)[i].size() < 3) {
				pi->erase_interior(i);
				--n;
				continue;
			}
			++i;
		}
		++pi;
	}
	unsigned int sz(p1.get_wkbsize());
	uint8_t wb[sz];
	return get_str(wb, p1.to_wkb(wb, &wb[sz]));
}

std::string PGDbBaseCommon::get_str(const PolygonHole& p)
{
	PolygonHole p1(p);
	if (p1.get_exterior().size() < 3)
		return "NULL";
	for (unsigned int i(0), n(p1.get_nrinterior()); i < n; ) {
		if (p1[i].size() < 3) {
			p1.erase_interior(i);
			--n;
			continue;
		}
		++i;
	}
	unsigned int sz(p1.get_wkbsize());
	uint8_t wb[sz];
	return get_str(wb, p1.to_wkb(wb, &wb[sz]));
}

std::string PGDbBaseCommon::get_str(const LineString& ls)
{
	unsigned int sz(ls.get_wkbsize());
	uint8_t wb[sz];
	return get_str(wb, ls.to_wkb(wb, &wb[sz]));
}

template <class T> PGDbBase<T>::PGDbBase(pqxx::connection_base& conn)
	: PGDbBaseCommon(conn)
{
	define_dbfunc();
	create_tables();
	create_indices();
	check_rtree();
}

template <class T> PGDbBase<T>::PGDbBase(pqxx::connection_base& conn, read_only_tag_t)
	: PGDbBaseCommon(conn, read_only)
{
	check_rtree();
}

template <class T> void PGDbBase<T>::check_rtree(void)
{
	m_has_rtree = false;
	if (area_data) {
		try {
			pqxx::read_transaction w(m_conn);
			pqxx::result r(w.exec("SELECT tablename FROM pg_catalog.pg_tables WHERE schemaname='aviationdb' AND tablename=" +
					      w.quote(main_table_name + std::string("_rtree")) + ";"));
			for (pqxx::result::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
				m_has_rtree = (*ri)[0].as<std::string>("") == main_table_name + std::string("_rtree");
				if (m_has_rtree)
					break;
			}
		} catch (...) {
		}
	}
	if (false)
		std::cerr << main_table_name << ": rtree: " << (m_has_rtree ? "yes" : "no") << std::endl;
}

template <class T> void PGDbBase<T>::check_readwrite(void)
{
	if (!m_readonly)
		return;
	throw std::runtime_error("read only database");
}

template <class T> void PGDbBase<T>::purgedb(void)
{
	check_readwrite();
	drop_indices();
	drop_tables();
	create_tables();
	create_indices();
}

template <class T> void PGDbBase<T>::save(element_t& e)
{
	check_readwrite();
	e.save(m_conn, m_has_rtree);
}

template <class T> void PGDbBase<T>::erase(element_t& e)
{
	check_readwrite();
	e.erase(m_conn, m_has_rtree);
}

template <class T> void PGDbBase<T>::update_index(element_t& e)
{
	check_readwrite();
	e.update_index(m_conn, m_has_rtree);
}

template <class T> typename PGDbBase<T>::element_t PGDbBase<T>::operator()(int64_t id, table_t tbl, unsigned int loadsubtables)
{
	element_t e;
	std::string qs;
	pqxx::read_transaction w(m_conn);
	if (tbl == element_t::table_aux) {
		return e;
	} else {
		qs = "SELECT " + std::string(element_t::db_query_string) + " FROM " + main_table_name + " WHERE ID=" + w.quote(id) + ";";
	}
	pqxx::result r(w.exec(qs));
	if (r.empty())
		return e;
	e.load(r.front(), w, loadsubtables);
	return e;
}

template <class T> void PGDbBase<T>::loadfirst(element_t& e, bool include_aux, unsigned int loadsubtables)
{
	e = element_t();
	pqxx::read_transaction w(m_conn);
	pqxx::result r(w.exec("SELECT " + std::string(element_t::db_query_string) +
			      " FROM " + main_table_name + " ORDER BY ID ASC LIMIT 1;"));
	if (r.empty())
		return;
	e.load(r.front(), w, loadsubtables);
}

template <class T> void PGDbBase<T>::loadnext(element_t& e, bool include_aux, unsigned int loadsubtables)
{
	uint64_t id(e.get_id());
	table_t tbl(e.get_table());
	e = element_t();
	if (tbl == element_t::table_aux)
		return;
	pqxx::read_transaction w(m_conn);
	pqxx::result r(w.exec("SELECT " + std::string(element_t::db_query_string) +
			      " FROM " + main_table_name + " WHERE ID > " + w.quote(id) + " ORDER BY ID ASC LIMIT 1;"));
	if (r.empty())
		return;
	e.load(r.front(), w, loadsubtables);
}

template <class T> void PGDbBase<T>::for_each(ForEach& cb, bool include_aux, unsigned int loadsubtables)
{
	pqxx::read_transaction w(m_conn);
	pqxx::result r(w.exec("SELECT " + std::string(element_t::db_query_string) +
			      " FROM " + main_table_name + " ORDER BY " + order_field));
	for (pqxx::result::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
		element_t e;
		e.load(*ri, w, loadsubtables);
		if (!e.is_valid())
			continue;
		if (!cb(e))
			break;
	}
}

template <class T> void PGDbBase<T>::for_each_by_rect(ForEach& cb, const Rect& bbox, bool include_aux, unsigned int loadsubtables)
{
	std::string qs;
	pqxx::read_transaction w(m_conn);
	qs = "SELECT " + get_area_select_string(w, bbox);
	if (order_field) {
		qs += " ORDER BY ";
		qs += order_field;
	}
	qs += ";";
	pqxx::result r(w.exec(qs));
	for (pqxx::result::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
		element_t e;
		e.load(*ri, w, loadsubtables);
		if (!e.is_valid())
			continue;
		if (!cb(e))
			break;
	}
}

template <class T> typename PGDbBase<T>::elementvector_t PGDbBase<T>::find_by_text(const std::string& pattern, char escape, comp_t comp, unsigned int limit, unsigned int loadsubtables)
{
	std::string qsfld;
	Glib::ustring pat(pattern);
	switch (comp) {
	default:
	case DbQueryInterfaceCommon::comp_startswith:
		escape = 0;
		break;

	case DbQueryInterfaceCommon::comp_contains:
		escape = '!';
		pat = std::string("%") + quote_text_for_like(pat, escape) + "%";
		break;

	case DbQueryInterfaceCommon::comp_exact:
		escape = 0;
		break;

	case DbQueryInterfaceCommon::comp_like:
		break;
	}
	pqxx::read_transaction w(m_conn);
	{
		bool needor(false);
		for (const char **fld = element_t::db_text_fields; *fld; fld++) {
			if (needor)
				qsfld += " OR ";
			const char *fldname(*fld);
			switch (comp) {
			default:
			case DbQueryInterfaceCommon::comp_startswith:
				qsfld += std::string("((") + fldname + "::ext.CITEXT COLLATE \"C\")>=(" + w.quote((std::string)pat) +
					"::ext.CITEXT COLLATE \"C\")) AND ((" + fldname + "::ext.CITEXT COLLATE \"C\")<(aviationdb.upperbound(" +
					w.quote((std::string)pat) + ")::ext.CITEXT COLLATE \"C\"))";
				break;

			case DbQueryInterfaceCommon::comp_exact:
				qsfld += std::string("((") + fldname + "::ext.CITEXT COLLATE \"C\")=(" + w.quote((std::string)pat) + "::ext.CITEXT COLLATE \"C\"))";
				break;

			case DbQueryInterfaceCommon::comp_exact_casesensitive:
				qsfld += std::string("((") + fldname + " COLLATE \"C\")=(" + w.quote((std::string)pat) + " COLLATE \"C\"))";
				break;

			case DbQueryInterfaceCommon::comp_contains:
			case DbQueryInterfaceCommon::comp_like:
				qsfld += std::string("((") + fldname + "::ext.CITEXT COLLATE \"C\") ILIKE (" + w.quote((std::string)pat) + "::ext.CITEXT COLLATE \"C\")";
				if (escape) {
					std::ostringstream oss;
					oss << (int)escape;
					qsfld += " ESCAPE chr(" + oss.str() + ")";
				}
				qsfld += ')';
				break;
			}
			needor = true;
		}
	}
	std::string qs;
	qs = "SELECT " + std::string(element_t::db_query_string) + " FROM " + main_table_name + " WHERE (" + qsfld + ')';
	if (order_field) {
		qs += " ORDER BY ";
		qs += order_field;
	}
	if (limit)
		qs += " LIMIT " + w.quote(limit);
	qs += ";";
	if (false)
		std::cerr << "sql: " << qs << " pat: " << pat << std::endl;
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

template <class T> typename PGDbBase<T>::elementvector_t PGDbBase<T>::find_by_time(time_t fromtime, time_t totime, unsigned int limit, unsigned int loadsubtables)
{
	pqxx::read_transaction w(m_conn);
	std::string qsfld;
	{
		bool needor(false);
		for (const char **fld = element_t::db_time_fields; *fld; fld++) {
			if (needor)
				qsfld += " OR ";
			qsfld += std::string("(") + (*fld) + " BETWEEN " + w.quote(fromtime) + " AND " + w.quote(totime) + ")";
			needor = true;
		}
	}
	std::string qs;
	qs = "SELECT " + std::string(element_t::db_query_string) + " FROM " + main_table_name + " WHERE (" + qsfld + ')';
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

template <class T> typename PGDbBase<T>::elementvector_t PGDbBase<T>::find_by_rect(const Rect& bbox, unsigned int limit, unsigned int loadsubtables)
{
	if (false)
		std::cerr << "find_by_rect: loadsubtables 0x" << std::hex << loadsubtables << std::endl;
	pqxx::read_transaction w(m_conn);
	std::string qs;
	qs = "SELECT " + get_area_select_string(w, bbox);
	if (order_field) {
		qs += " ORDER BY ";
		qs += order_field;
	}
	if (limit)
		qs += " LIMIT " + w.quote(limit);
	qs += ";";
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

template <class T> typename PGDbBase<T>::elementvector_t PGDbBase<T>::find_nearest(const Point& pt, const Rect& bbox, unsigned int limit, unsigned int loadsubtables)
{
	pqxx::read_transaction w(m_conn);
	std::string qs, sortcol;
	if (area_data)
		sortcol = "(simplerectdist(SWLON,SWLAT,NELON,NELAT," + w.quote(pt.get_lon()) + "," + w.quote(pt.get_lat()) + ")) AS 'SORTCOL'";
	else
		sortcol = "(simpledist(LON,LAT," + w.quote(pt.get_lon()) + "," + w.quote(pt.get_lat()) + ")) AS 'SORTCOL'";
	qs = "SELECT " + get_area_select_string(w, bbox, sortcol.c_str());
	qs += " ORDER BY SORTCOL";
	if (limit)
		qs += " LIMIT " + w.quote(limit);
	qs += ";";
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

template <class T> typename PGDbBase<T>::elementvector_t PGDbBase<T>::find_nulltile(unsigned int limit, unsigned int loadsubtables)
{
	pqxx::read_transaction w(m_conn);
	std::string qs;
	qs += std::string("SELECT ") + element_t::db_query_string + " FROM " +
		main_table_name	+ " WHERE TILE IS NULL";
	if (limit)
		qs += " LIMIT " + w.quote(limit);
	qs += ";";
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

template <class T> std::string PGDbBase<T>::get_area_select_string(pqxx::basic_transaction& w, const Rect& r, const char *sortcol)
{
	std::ostringstream oss;
	oss << element_t::db_query_string;
	if (sortcol)
		oss << ',' << sortcol;
	oss << " FROM " << main_table_name << " WHERE ";
	if (m_has_rtree) {
		oss << "ID IN (SELECT ID FROM " << main_table_name << "_rtree WHERE "
		    << main_table_name << "_rtree.NELAT >= " << w.quote(r.get_south()) << " AND " << main_table_name << "_rtree.SWLAT <= " << w.quote(r.get_north()) << " AND (("
		    //<< main_table_name << "_rtree.SWLON <= " << w.quote(r.get_east_unwrapped()) << "-4294967296 AND " << main_table_name << "_rtree.NELON >= " << w.quote(r.get_west()) << "-4294967296) OR ("
		    //<< main_table_name << "_rtree.SWLON <= " << w.quote(r.get_east_unwrapped()) << " AND " << main_table_name << "_rtree.NELON >= " << w.quote(r.get_west()) << ") OR ("
		    //<< main_table_name << "_rtree.SWLON <= " << w.quote(r.get_east_unwrapped()) << "+4294967296 AND " << main_table_name << "_rtree.NELON >= " << w.quote(r.get_west()) << "+4294967296))";
		    << main_table_name << "_rtree.SWLON <= " << w.quote(r.get_east_unwrapped()) << " AND " << main_table_name << "_rtree.NELON >= " << w.quote(r.get_west()) << ")))";
		return oss.str();
	}
	if (false) {
		oss << '(';
		typename element_t::tilerange_t tr(element_t::TileNumber::to_tilerange(r));
		bool needor(false);
		for (typename element_t::TileNumber::tile_t mask = (1 << element_t::TileNumber::tile_bits) - 1; mask; mask >>= 1, tr.first >>= 1, tr.second >>= 1) {
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
		typename element_t::tilerange_t tr(element_t::TileNumber::to_tilerange(r));
		bool needcomma(false);
		for (typename element_t::TileNumber::tile_t mask = (1 << element_t::TileNumber::tile_bits) - 1; mask; mask >>= 1, tr.first >>= 1, tr.second >>= 1) {
			for (typename element_t::TileNumber::tile_t tile = tr.first;; tile = (tile & ~mask) | ((tile + 1) & mask)) {
				if (needcomma)
					oss << ',';
				needcomma = true;
				oss << tile;
				if (tile == tr.second)
					break;
			}
		}
		oss << ") AND ";
	} else if (false) {
		// disabled, since Achim's code doesn't know how to fill in tile numbers
		oss << "TILE IN (";
		typename element_t::TileNumber::Iterator ti(r);
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
	return oss.str();
}

template <class T> typename PGDbBase<T>::elementvector_t PGDbBase<T>::find(const std::string& fldname, const std::string& pattern, char escape, comp_t comp, unsigned int limit, unsigned int loadsubtables)
{
	pqxx::read_transaction w(m_conn);
	std::string qsfld;
	Glib::ustring pat(pattern);
	switch (comp) {
	default:
	case DbQueryInterfaceCommon::comp_startswith:
		escape = 0;
		qsfld += std::string("(") + fldname + "::ext.CITEXT>=" + w.quote((std::string)pat) + "::ext.CITEXT) AND (" + fldname +
			"::ext.CITEXT<aviationdb.upperbound(" + w.quote((std::string)pat) + ")::ext.CITEXT)";
		break;

	case DbQueryInterfaceCommon::comp_exact:
		escape = 0;
		qsfld += std::string("(") + fldname + "::ext.CITEXT=" + w.quote((std::string)pat) + "::ext.CITEXT)";
		break;

	case DbQueryInterfaceCommon::comp_exact_casesensitive:
		escape = 0;
		qsfld += std::string("(") + fldname + "=" + w.quote((std::string)pat) + ")";
		break;

	case DbQueryInterfaceCommon::comp_contains:
		escape = '!';
		pat = std::string("%") + quote_text_for_like(pat, escape) + "%";
		// fall through

	case DbQueryInterfaceCommon::comp_like:
		qsfld += std::string("(") + fldname + "::ext.CITEXT ILIKE " + w.quote((std::string)pat) + "::ext.CITEXT";
		if (escape) {
			std::ostringstream oss;
			oss << (int)escape;
			qsfld += " ESCAPE CHR(" + oss.str() + ")";
		}
		qsfld += ')';
		break;
	}
	std::string qs;
	qs = "SELECT " + std::string(element_t::db_query_string) + " FROM " + main_table_name + " WHERE " + qsfld;
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

template <class T> typename PGDbBase<T>::elementvector_t PGDbBase<T>::loadid(uint64_t startid, table_t table, const std::string& order, unsigned int limit, unsigned int loadsubtables)
{
	elementvector_t ret;
	if (table == element_t::table_aux)
		return ret;
	pqxx::read_transaction w(m_conn);
	std::string qs("SELECT " + std::string(element_t::db_query_string) + " FROM " + main_table_name + " WHERE ID>=" + w.quote(startid));
	if (!order.empty())
		qs += " ORDER BY " + order;
	if (limit)
		qs += " LIMIT " + w.quote(limit);
	qs += ";";
	if (false)
		std::cerr << "sql: " << qs << std::endl;
	pqxx::result r(w.exec(qs));
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

template <class T> void PGDbBase<T>::load_subtables(element_t& el, unsigned int loadsubtables)
{
	if (!loadsubtables)
		return;
	pqxx::read_transaction w(m_conn);
	el.load_subtables(w, loadsubtables);
}

template <class T> void PGDbBase<T>::load_subtables(elementvector_t& ev, unsigned int loadsubtables)
{
	if (!loadsubtables)
		return;
	pqxx::read_transaction w(m_conn);
	for (typename elementvector_t::iterator ei(ev.begin()), ee(ev.end()); ei != ee; ++ei)
		ei->load_subtables(w, loadsubtables);
}

template class PGDbBase<NavaidsDb::Navaid>;
template class PGDbBase<WaypointsDb::Waypoint>;
template class PGDbBase<AirportsDb::Airport>;
template class PGDbBase<AirspacesDb::Airspace>;
template class PGDbBase<AirwaysDb::Airway>;
template class PGDbBase<MapelementsDb::Mapelement>;
template class PGDbBase<WaterelementsDb::Waterelement>;
template class PGDbBase<LabelsDb::Label>;
template class PGDbBase<TracksDb::Track>;

// create DB
// su postgres
// createdb -e -O postgres aviationdb
// psql -t -d aviationdb -U postgres -a  -c  "GRANT ALL PRIVILEGES ON DATABASE aviationdb to psqlar;"
// psql -f /usr/share/pgsql/contrib/postgis-2.1/postgis.sql -d aviationdb
// psql -f /usr/share/pgsql/contrib/postgis-2.1/spatial_ref_sys.sql -d aviationdb
// psql -d aviationdb -U postgres

/*
 * Table Delete

 drop table aviationdb.airportcomms;
 drop table aviationdb.airportfas;
 drop table aviationdb.airporthelipads;
 drop table aviationdb.airportlinefeatures;
 drop table aviationdb.airportpointfeatures;
 drop table aviationdb.airportrunways;
 drop table aviationdb.airportvfrroutepoints;
 drop table aviationdb.airportvfrroutes;
 drop table aviationdb.airports;

 drop table aviationdb.airspacecomponents;
 drop table aviationdb.airspacesegments;
 drop table aviationdb.airspaces;

 drop table aviationdb.airways;

 drop table aviationdb.navaids;

 drop table aviationdb.waypoints;

 drop table aviationdb.mapelements;

 drop table aviationdb.waterelements;

 drop table aviationdb.labelsourcecoordicaonameauthoritybase;
 drop table aviationdb.labelsourcecoordicaonamebase;
 drop table aviationdb.labelsourcecoordbase;
 drop table aviationdb.labelsourcebase;
 drop table aviationdb.labelcoordbase;
 drop table aviationdb.labelbase;

 */

#endif
