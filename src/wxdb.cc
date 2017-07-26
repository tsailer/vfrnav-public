//
// C++ Implementation: wxdb
//
// Description: Weather Objects Database
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <limits>
#include <zlib.h>

#include "wxdb.h"
#include "fplan.h"

WXDatabase::MetarTaf::MetarTaf(const std::string& icao, type_t t, time_t epoch, const std::string& msg, const Point& pt)
        : m_id(0), m_icao(icao), m_message(msg), m_coord(pt), m_epoch(epoch), m_type(t)
{
}

bool WXDatabase::MetarTaf::operator<(const MetarTaf& m) const
{
        {
                int c(get_icao().compare(m.get_icao()));
                if (c)
                        return c < 0;
        }
        if (get_type() < m.get_type())
                return true;
        if (get_type() > m.get_type())
                return false;
        if (get_epoch() > m.get_epoch())
                return true;
        if (get_epoch() < m.get_epoch())
                return false;
	return get_id() < m.get_id();
}

void WXDatabase::MetarTaf::load(sqlite3x::sqlite3_cursor& cursor, sqlite3x::sqlite3_connection& db)
{
        m_id = 0;
	m_epoch = 0;
	if (!cursor.step())
                return;
        m_id = cursor.getint(0);
	m_icao = cursor.getstring(1);
	m_coord = Point(cursor.getint(3), cursor.getint(2));
	m_message = cursor.getstring(4);
	m_type = (type_t)cursor.getint(5);
	m_epoch = cursor.getint(6);
}

void WXDatabase::MetarTaf::save_notran(sqlite3x::sqlite3_connection& db) const
{
	if (!m_id) {
		{
                        sqlite3x::sqlite3_command cmd(db, "SELECT ID FROM metartaf WHERE ICAO=? AND EPOCH=? AND TYPE=?;");
			cmd.bind(1, m_icao);
			cmd.bind(2, (long long int)m_epoch);
			cmd.bind(3, (int)m_type);
                        sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
                        if (cursor.step())
                                m_id = cursor.getint(0);
                }
		if (!m_id) {
			m_id = 1;
			{
				sqlite3x::sqlite3_command cmd(db, "SELECT MAX(ID) FROM metartaf;");
				sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
				if (cursor.step())
					m_id = cursor.getint(0) + 1;
			}
			{
				sqlite3x::sqlite3_command cmd(db, "INSERT INTO metartaf (ID) VALUES(?);");
				cmd.bind(1, (long long int)m_id);
				cmd.executenonquery();
			}
		}
	}
        {
                sqlite3x::sqlite3_command cmd(db, "UPDATE metartaf SET ICAO=?,LAT=?,LON=?,MESSAGE=?,TYPE=?,EPOCH=? WHERE ID=?;");
                cmd.bind(1, m_icao);
                cmd.bind(2, m_coord.get_lat());
                cmd.bind(3, m_coord.get_lon());
                cmd.bind(4, m_message);
                cmd.bind(5, (int)m_type);
                cmd.bind(6, (long long int)m_epoch);
		cmd.bind(7, (long long int)m_id);
                cmd.executenonquery();
        }
}

void WXDatabase::MetarTaf::save(sqlite3x::sqlite3_connection& db) const
{
        sqlite3x::sqlite3_transaction tran(db);
	save_notran(db);
        tran.commit();
}

void WXDatabase::MetarTaf::erase(sqlite3x::sqlite3_connection& db)
{
        if (!is_valid())
                return;
        sqlite3x::sqlite3_transaction tran(db);
	{
		sqlite3x::sqlite3_command cmd(db, "DELETE FROM metartaf WHERE ID=?;");
		cmd.bind(1, (long long int)m_id);
		cmd.executenonquery();
	}
        tran.commit();
        m_id = 0;
}

void WXDatabase::MetarTaf::remove_linefeeds(std::string& m)
{
	for (std::string::size_type n(m.size()); n > 0; ) {
		--n;
		char ch(m[n]);
		if (ch != '\r' && ch != '\n')
			continue;
		m.erase(n, 1);
	}
}

WXDatabase::Chart::Chart(const std::string& dep, const std::string& dest, const std::string& desc,
			 time_t createtime, time_t epoch, unsigned int level, unsigned int type,
			 const std::string& mimetype, const std::string& data)
	: m_id(0), m_departure(dep), m_destination(dest), m_description(desc),
	  m_mimetype(mimetype), m_data(data), m_createtime(createtime), m_epoch(epoch), m_level(level), m_type(type)
{
}

bool WXDatabase::Chart::operator<(const Chart& c) const
{
        if (get_epoch() > c.get_epoch())
                return true;
        if (get_epoch() < c.get_epoch())
                return false;
        if (get_level() < c.get_level())
                return true;
        if (get_level() > c.get_level())
                return false;
        if (get_type() < c.get_type())
                return true;
        if (get_type() > c.get_type())
                return false;
        if (get_createtime() > c.get_createtime())
                return true;
        if (get_createtime() < c.get_createtime())
                return false;
        {
                int x(get_departure().compare(c.get_departure()));
                if (x)
                        return x < 0;
        }
        {
                int x(get_destination().compare(c.get_destination()));
                if (x)
                        return x < 0;
        }
        {
                int x(get_description().compare(c.get_description()));
                if (x)
			return x < 0;
        }
	return get_id() < c.get_id();
}

void WXDatabase::Chart::load(sqlite3x::sqlite3_cursor& cursor, sqlite3x::sqlite3_connection& db)
{
        m_id = 0;
	m_epoch = 0;
	if (!cursor.step())
                return;
	m_id = cursor.getint(0);
	m_departure = cursor.getstring(1);
	m_destination = cursor.getstring(2);
	m_description = cursor.getstring(3);
	m_mimetype = cursor.getstring(4);
	m_createtime = cursor.getint(5);
	m_epoch = cursor.getint(6);
	m_level = cursor.getint(7);
	m_type = cursor.getint(8);
	m_data.clear();
	if (!cursor.isnull(9)) {
		int sz;
		const char *cp(static_cast<const char *>(cursor.getblob(9, sz)));
		m_data.insert(m_data.begin(), cp, cp + sz);
	}
}

void WXDatabase::Chart::save_notran(sqlite3x::sqlite3_connection& db) const
{
	if (!m_id) {
                m_id = 1;
                {
                        sqlite3x::sqlite3_command cmd(db, "SELECT MAX(ID) FROM chart;");
                        sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
                        if (cursor.step())
                                m_id = cursor.getint(0) + 1;
                }
                {
                        sqlite3x::sqlite3_command cmd(db, "INSERT INTO chart (ID) VALUES(?);");
                        cmd.bind(1, (long long int)m_id);
                        cmd.executenonquery();
                }
        }
        {
                sqlite3x::sqlite3_command cmd(db, "UPDATE chart SET DEP=?,DEST=?,DESC=?,MIMETYPE=?,CREATETIME=?,EPOCH=?,LEVEL=?,TYPE=?,DATA=? WHERE ID=?;");
                cmd.bind(1, m_departure);
                cmd.bind(2, m_destination);
                cmd.bind(3, m_description);
                cmd.bind(4, m_mimetype);
                cmd.bind(5, (long long int)m_createtime);
		cmd.bind(6, (long long int)m_epoch);
		cmd.bind(7, (long long int)m_level);
		cmd.bind(8, (long long int)m_type);
		cmd.bind(9, &m_data[0], m_data.size());
		cmd.bind(10, (long long int)m_id);
                cmd.executenonquery();
        }
}

void WXDatabase::Chart::save(sqlite3x::sqlite3_connection& db) const
{
        sqlite3x::sqlite3_transaction tran(db);
	save_notran(db);
        tran.commit();
}

void WXDatabase::Chart::erase(sqlite3x::sqlite3_connection& db)
{
        if (!is_valid())
                return;
        sqlite3x::sqlite3_transaction tran(db);
	{
		sqlite3x::sqlite3_command cmd(db, "DELETE FROM chart WHERE ID=?;");
		cmd.bind(1, (long long int)m_id);
		cmd.executenonquery();
	}
        tran.commit();
        m_id = 0;
}

WXDatabase::WXDatabase()
{
}

void WXDatabase::open(const Glib::ustring& path, const Glib::ustring& dbname)
{
        Glib::ustring dbname1(path);
        if (dbname1.empty())
                dbname1 = FPlan::get_userdbpath();
        dbname1 = Glib::build_filename(dbname1, dbname);
        m_db.open(dbname1);
        create_tables();
        create_indices();
}

void WXDatabase::take(sqlite3 *dbh)
{
        m_db.take(dbh);
        if (!m_db.db())
                return;
        create_tables();
        create_indices();
}

void WXDatabase::close(void)
{
        m_db.close();
}

void WXDatabase::purgedb(void)
{
        drop_indices();
        drop_tables();
        create_tables();
        create_indices();
}

void WXDatabase::sync_off(void)
{
        sqlite3x::sqlite3_command cmd(m_db, "PRAGMA synchronous = OFF;");
        cmd.executenonquery();
}

void WXDatabase::analyze(void)
{
        sqlite3x::sqlite3_command cmd(m_db, "ANALYZE;");
        cmd.executenonquery();
}

void WXDatabase::vacuum(void)
{
        sqlite3x::sqlite3_command cmd(m_db, "VACUUM;");
        cmd.executenonquery();
}

void WXDatabase::interrupt(void)
{
        sqlite3_interrupt(m_db.db());
}

void WXDatabase::expire_metartaf(time_t cutoffepoch)
{
	sqlite3x::sqlite3_transaction tran(m_db);
	sqlite3x::sqlite3_command cmd(m_db, "DELETE FROM metartaf WHERE EPOCH<?;");
	cmd.bind(1, (long long int)cutoffepoch);
	cmd.executenonquery();
	tran.commit();
}

void WXDatabase::expire_chart(time_t cutoffepoch)
{
	sqlite3x::sqlite3_transaction tran(m_db);
	sqlite3x::sqlite3_command cmd(m_db, "DELETE FROM chart WHERE EPOCH<?;");
	cmd.bind(1, (long long int)cutoffepoch);
	cmd.executenonquery();
	tran.commit();
}

void WXDatabase::save(const MetarTaf& mt)
{
	mt.save(m_db);
}

void WXDatabase::save(const Chart& ch)
{
	ch.save(m_db);
}

template <typename T> void WXDatabase::save(T b, T e)
{
	sqlite3x::sqlite3_transaction tran(m_db);
	for (; b != e; ++b)
		b->save_notran(m_db);
        tran.commit();
}

template void WXDatabase::save(std::vector<MetarTaf>::const_iterator b, std::vector<MetarTaf>::const_iterator e);
template void WXDatabase::save(std::set<MetarTaf>::const_iterator b, std::set<MetarTaf>::const_iterator e);

void WXDatabase::loadfirst(MetarTaf& mt)
{
	mt = MetarTaf();
	sqlite3x::sqlite3_command cmd(m_db, "SELECT ID,ICAO,LAT,LON,MESSAGE,TYPE,EPOCH FROM metartaf ORDER BY ID ASC LIMIT 1;");
	sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
	mt.load(cursor, m_db);
}

void WXDatabase::loadnext(MetarTaf& mt)
{
        uint64_t id(mt.get_id());
	mt = MetarTaf();
	sqlite3x::sqlite3_command cmd(m_db, "SELECT ID,ICAO,LAT,LON,MESSAGE,TYPE,EPOCH FROM metartaf WHERE ID > ? ORDER BY ID ASC LIMIT 1;");
	cmd.bind(1, (long long int)id);
	sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
	mt.load(cursor, m_db);
}

void WXDatabase::loadfirst(Chart& ch)
{
	ch = Chart();
	sqlite3x::sqlite3_command cmd(m_db, "SELECT ID,DEP,DEST,DESC,MIMETYPE,CREATETIME,EPOCH,LEVEL,TYPE,DATA FROM chart ORDER BY ID ASC LIMIT 1;");
	sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
	ch.load(cursor, m_db);
}

void WXDatabase::loadnext(Chart& ch)
{
        uint64_t id(ch.get_id());
	ch = Chart();
	sqlite3x::sqlite3_command cmd(m_db, "SELECT ID,DEP,DEST,DESC,MIMETYPE,CREATETIME,EPOCH,LEVEL,TYPE,DATA FROM chart WHERE ID > ? ORDER BY ID ASC LIMIT 1;");
	cmd.bind(1, (long long int)id);
	sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
	ch.load(cursor, m_db);
}

WXDatabase::metartafvector_t WXDatabase::loadid_metartaf(uint64_t id, const std::string& order, unsigned int limit)
{
	std::string qs("SELECT ID,ICAO,LAT,LON,MESSAGE,TYPE,EPOCH FROM metartaf WHERE ID>=?1");
	if (!order.empty())
                qs += " ORDER BY " + order;
        if (limit)
                qs += " LIMIT ?2";
        qs += ";";
	sqlite3x::sqlite3_command cmd(m_db, qs);
        cmd.bind(1, (long long int)id);
        if (limit)
                cmd.bind(2, (long long int)limit);
        sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
	metartafvector_t ret;
        for (;;) {
                ret.push_back(MetarTaf());
                ret.back().load(cursor, m_db);
                if (ret.back().is_valid())
                        continue;
                ret.pop_back();
                break;
        }
        return ret;
}

WXDatabase::chartvector_t WXDatabase::loadid_chart(uint64_t id, const std::string& order, unsigned int limit)
{
	std::string qs("SELECT ID,DEP,DEST,DESC,MIMETYPE,CREATETIME,EPOCH,LEVEL,TYPE,DATA FROM chart WHERE ID>=?1");
	if (!order.empty())
                qs += " ORDER BY " + order;
        if (limit)
                qs += " LIMIT ?2";
        qs += ";";
	sqlite3x::sqlite3_command cmd(m_db, qs);
        cmd.bind(1, (long long int)id);
        if (limit)
                cmd.bind(2, (long long int)limit);
        sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
	chartvector_t ret;
        for (;;) {
                ret.push_back(Chart());
                ret.back().load(cursor, m_db);
                if (ret.back().is_valid())
                        continue;
                ret.pop_back();
                break;
        }
        return ret;
}

WXDatabase::metartafresult_t WXDatabase::find_metartaf(const Rect& bbox, time_t metarcutoff, time_t tafcutoff, unsigned int limit)
{
	std::string qs("SELECT ID,ICAO,LAT,LON,MESSAGE,TYPE,EPOCH FROM metartaf WHERE"
		       " (LAT BETWEEN ?2 AND ?4) AND ((LON BETWEEN ?1-4294967296 AND ?3-4294967296) OR (LON BETWEEN ?1 AND ?3))"
		       " AND ((TYPE=?5 AND EPOCH>=?6) OR (TYPE=?7 AND EPOCH>=?8)) ORDER BY ID ASC");
	if (limit)
		qs += " LIMIT ?9";
	qs += ";";
	if (false)
		std::cerr << "find_metartaf: query string \"" << qs << "\"" << std::endl;
        sqlite3x::sqlite3_command cmd(m_db, qs);
        cmd.bind(1, (long long int)bbox.get_west());
        cmd.bind(2, (long long int)bbox.get_south());
        cmd.bind(3, (long long int)bbox.get_east_unwrapped());
        cmd.bind(4, (long long int)bbox.get_north());
	cmd.bind(5, (int)MetarTaf::type_metar);
	cmd.bind(6, (long long int)metarcutoff);
	cmd.bind(7, (int)MetarTaf::type_taf);
	cmd.bind(8, (long long int)tafcutoff);
	if (limit)
		cmd.bind(9, (long long int)limit);
        sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
	metartafresult_t r;
        for (;;) {
                MetarTaf e;
                e.load(cursor, m_db);
                if (!e.is_valid())
                        break;
		r.insert(e);
        }
	return r;
}

WXDatabase::chartresult_t WXDatabase::find_chart(time_t cutoff, unsigned int limit)
{
	std::string qs("SELECT ID,DEP,DEST,DESC,MIMETYPE,CREATETIME,EPOCH,LEVEL,TYPE,DATA FROM chart WHERE"
		       " EPOCH>=?1 ORDER BY ID ASC");
	if (limit)
		qs += " LIMIT ?2";
	qs += ";";
        sqlite3x::sqlite3_command cmd(m_db, qs);
  	cmd.bind(1, (long long int)cutoff);
	if (limit)
		cmd.bind(2, (long long int)limit);
        sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
	chartresult_t r;
        for (;;) {
                Chart e;
                e.load(cursor, m_db);
                if (!e.is_valid())
                        break;
		r.insert(e);
        }
	return r;
}

void WXDatabase::drop_tables(void)
{
        { sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS metartaf;"); cmd.executenonquery(); }
        { sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS chart;"); cmd.executenonquery(); }
}

void WXDatabase::create_tables(void)
{
        {
                sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS metartaf (ID INTEGER PRIMARY KEY,"
                                              "ICAO TEXT,"
                                              "LAT INTEGER,"
                                              "LON INTEGER,"
                                              "MESSAGE TEXT,"
                                              "TYPE INTEGER,"
                                              "EPOCH INTEGER);");
                cmd.executenonquery();
        }
	{
                sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS chart (ID INTEGER PRIMARY KEY,"
                                              "DEP TEXT,"
                                              "DEST TEXT,"
                                              "DESC TEXT,"
                                              "MIMETYPE TEXT,"
                                              "CREATETIME INTEGER,"
                                              "EPOCH INTEGER,"
                                              "LEVEL INTEGER,"
                                              "TYPE INTEGER,"
                                              "DATA BLOB);");
                cmd.executenonquery();
        }
}

void WXDatabase::drop_indices(void)
{
        { sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS metartaf_icao;"); cmd.executenonquery(); }
        { sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS metartaf_latlon;"); cmd.executenonquery(); }
        { sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS metartaf_epochtype;"); cmd.executenonquery(); }
        { sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS chart_epochleveltypecreate;"); cmd.executenonquery(); }
}

void WXDatabase::create_indices(void)
{
        { sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS metartaf_icao ON metartaf(ICAO COLLATE NOCASE);"); cmd.executenonquery(); }
        { sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS metartaf_latlon ON metartaf(LAT,LON);"); cmd.executenonquery(); }
        { sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS metartaf_epochtype ON metartaf(EPOCH,TYPE);"); cmd.executenonquery(); }
        { sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS chart_epochleveltypecreate ON chart(EPOCH,LEVEL,TYPE,CREATETIME);"); cmd.executenonquery(); }
}
