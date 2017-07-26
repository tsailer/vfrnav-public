#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iomanip>

#include "cfmuautoroute45.hh"
#include "fplan.h"

bool CFMUAutoroute45::DctCache::PointID::operator<(const PointID& x) const
{
	if (m_point.get_lat() < x.m_point.get_lat())
		return true;
	if (x.m_point.get_lat() < m_point.get_lat())
		return false;
	return m_ident < x.m_ident;
}

bool CFMUAutoroute45::DctCache::DctAlt::operator<(const DctAlt& x) const
{
	if (m_point0 < x.m_point0)
		return true;
	if (x.m_point0 < m_point0)
		return false;
	return m_point1 < x.m_point1;
}

CFMUAutoroute45::DctCache::DctCache(const std::string& path, const std::string& revision)
	: m_pointid(0), m_pointidalloc(0)
{
	std::string dbname(path);
	if (dbname.empty())
                dbname = FPlan::get_userdbpath();
        dbname = Glib::build_filename(dbname, "dctcache.db");
	m_db.open(dbname);
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS properties (KEY TEXT UNIQUE NOT NULL, VALUE TEXT);");
		cmd.executenonquery();
	}
	{
		std::string rev(get_property("revision"));
		if (rev != revision) {
			if (true)
				std::cerr << "DCT Cache: requested revision " << revision << " cache revision " << rev << ": flushing" << std::endl;
			{
				sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS points_latlonident;");
				cmd.executenonquery();
			}
			{
				sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS dct_pt01;");
				cmd.executenonquery();
			}
			{
				sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS points;");
				cmd.executenonquery();
			}
			{
				sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS dct;");
				cmd.executenonquery();
			}
			set_property("revision", revision);
		}
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS points (ID INTEGER PRIMARY KEY NOT NULL, LAT INTEGER, LON INTEGER, IDENT TEXT);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS dct (PT0 INTEGER NOT NULL, PT1 INTEGER NOT NULL, ALTLWR INTEGER, ALTUPR INTEGER);");
		cmd.executenonquery();
	}
        {
		sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS points_latlonident ON points(LAT,LON,IDENT);");
		cmd.executenonquery();
	}
        {
		sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS dct_pt01 ON dct(PT0,PT1);");
		cmd.executenonquery();
	}
        {
		sqlite3x::sqlite3_command cmd(m_db, "SELECT MAX(ID) FROM points;");
		sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
		if (cursor.step())
			m_pointidalloc = m_pointid = cursor.getint(0) + 1;
	}
}

std::string CFMUAutoroute45::DctCache::get_property(const std::string& key)
{
	sqlite3x::sqlite3_command cmd(m_db, "SELECT VALUE FROM properties WHERE KEY=?;");
	cmd.bind(1, key);
	sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
	if (cursor.step())
		return cursor.getstring(0);
	return "";
}

void CFMUAutoroute45::DctCache::set_property(const std::string& key, const std::string& value)
{
	sqlite3x::sqlite3_command cmd(m_db, "INSERT OR REPLACE INTO properties (KEY,VALUE) VALUES (?,?);");
	cmd.bind(1, key);
	cmd.bind(2, value);
	cmd.executenonquery();
}

CFMUAutoroute45::DctCache::pointid_t CFMUAutoroute45::DctCache::find_point(const std::string& ident, const Point& pt)
{
	std::pair<points_t::iterator,bool> i(m_points.insert(PointID(ident, pt, m_pointidalloc)));
	if (i.second)
		++m_pointidalloc;
	return i.first->get_id();
}

const CFMUAutoroute45::DctCache::DctAlt *CFMUAutoroute45::DctCache::find_dct(pointid_t pt0, pointid_t pt1) const
{
	dcts_t::const_iterator i(m_dcts.find(DctAlt(pt0, pt1)));
	if (i == m_dcts.end())
		return 0;
	return &*i;
}

void CFMUAutoroute45::DctCache::add_dct(pointid_t pt0, pointid_t pt1, const set_t& alt)
{
	std::pair<dcts_t::iterator,bool> i(m_dcts.insert(DctAlt(pt0, pt1, alt, true)));
	if (i.second)
		return;
	DctAlt& d(const_cast<DctAlt&>(*i.first));
	d.set_dirty(true);
	d.get_alt() = alt;
}

void CFMUAutoroute45::DctCache::load(const Rect& bbox)
{
	m_points.clear();
	m_dcts.clear();
	m_pointidalloc = m_pointid = 0;
        {
		sqlite3x::sqlite3_command cmd(m_db, "SELECT MAX(ID) FROM points;");
		sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
		if (cursor.step())
			m_pointidalloc = m_pointid = cursor.getint(0) + 1;
	}
	std::string cond("(LAT >= ? AND LAT <= ?) AND ");
	if (bbox.get_east() < bbox.get_west())
		cond += "(LON >= ? OR LON <= ?)";
	else
		cond += "(LON >= ? AND LON <= ?)";
        {
		sqlite3x::sqlite3_command cmd(m_db, "SELECT ID,LAT,LON,IDENT FROM points WHERE " + cond + ";");
		cmd.bind(1, bbox.get_south());
		cmd.bind(2, bbox.get_north());
		cmd.bind(3, bbox.get_west());
		cmd.bind(4, bbox.get_east());
		sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
		while (cursor.step()) {
			std::pair<points_t::iterator,bool> i(m_points.insert(PointID(cursor.getstring(3), Point(cursor.getint(2), cursor.getint(1)), cursor.getint(0))));
			if (!i.second)
				std::cerr << "DctCache: duplicate point " << i.first->get_ident() << ' ' << i.first->get_point().get_lat_str2()
					  << ' '  << i.first->get_point().get_lon_str2() << std::endl;
		}
	}
        {
		sqlite3x::sqlite3_command cmd(m_db, "SELECT DISTINCT A.PT0,A.PT1,A.ALTLWR,A.ALTUPR FROM dct AS A CROSS JOIN points AS B WHERE (A.PT0 = B.ROWID OR A.PT1 = B.ROWID) AND " + cond + ";");
		cmd.bind(1, bbox.get_south());
		cmd.bind(2, bbox.get_north());
		cmd.bind(3, bbox.get_west());
		cmd.bind(4, bbox.get_east());
		sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
		while (cursor.step()) {
			set_t::Intvl alt(cursor.getint(2), cursor.getint(3));
			std::pair<dcts_t::iterator,bool> i(m_dcts.insert(DctAlt(cursor.getint(0), cursor.getint(1), alt, false)));
			if (i.second)
				continue;
			DctAlt& d(const_cast<DctAlt&>(*i.first));
			d.get_alt() |= alt;
		}
	}
}

void CFMUAutoroute45::DctCache::flush(void)
{
	{
		sqlite3x::sqlite3_command cmd(m_db, "BEGIN TRANSACTION;");
		cmd.executenonquery();
	}
	for (points_t::const_iterator pi(m_points.begin()), pe(m_points.end()); pi != pe; ++pi) {
		if (pi->get_id() < m_pointid)
			continue;
		sqlite3x::sqlite3_command cmd(m_db, "INSERT INTO points (ID,LAT,LON,IDENT) VALUES (?,?,?,?);");
		cmd.bind(1, (sqlite3x::int64_t)pi->get_id());
		cmd.bind(2, pi->get_point().get_lat());
		cmd.bind(3, pi->get_point().get_lon());
		cmd.bind(4, pi->get_ident());
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "COMMIT TRANSACTION;");
		cmd.executenonquery();
	}
	m_pointid = m_pointidalloc;
	{
		sqlite3x::sqlite3_command cmd(m_db, "BEGIN TRANSACTION;");
		cmd.executenonquery();
	}
	for (dcts_t::const_iterator di(m_dcts.begin()), de(m_dcts.end()); di != de; ++di) {
		if (!di->is_dirty())
			continue;
		{
			sqlite3x::sqlite3_command cmd(m_db, "DELETE FROM dct WHERE PT0=? AND PT1=?;");
			cmd.bind(1, (sqlite3x::int64_t)di->get_point0());
			cmd.bind(2, (sqlite3x::int64_t)di->get_point1());
			cmd.executenonquery();
		}
		set_t::const_iterator ii(di->get_alt().begin()), ie(di->get_alt().end());
		if (ii == ie) {
			// add an empty interval
			sqlite3x::sqlite3_command cmd(m_db, "INSERT INTO dct (PT0,PT1,ALTLWR,ALTUPR) VALUES (?,?,?,?);");
			cmd.bind(1, (sqlite3x::int64_t)di->get_point0());
			cmd.bind(2, (sqlite3x::int64_t)di->get_point1());
			cmd.bind(3, std::numeric_limits<set_t::type_t>::max());
			cmd.bind(4, std::numeric_limits<set_t::type_t>::max());
			cmd.executenonquery();
			continue;
		}
		for (; ii != ie; ++ii) {
			sqlite3x::sqlite3_command cmd(m_db, "INSERT INTO dct (PT0,PT1,ALTLWR,ALTUPR) VALUES (?,?,?,?);");
			cmd.bind(1, (sqlite3x::int64_t)di->get_point0());
			cmd.bind(2, (sqlite3x::int64_t)di->get_point1());
			cmd.bind(3, ii->get_lower());
			cmd.bind(4, ii->get_upper());
			cmd.executenonquery();
		}
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "COMMIT TRANSACTION;");
		cmd.executenonquery();
	}
	for (dcts_t::const_iterator di(m_dcts.begin()), de(m_dcts.end()); di != de; ++di) {
		if (!di->is_dirty())
			continue;
		DctAlt& d(const_cast<DctAlt&>(*di));
		d.set_dirty(false);
	}
}
