#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iomanip>

#include "cfmuautoroute45.hh"
#include "fplan.h"


CFMUAutoroute45::CFMUIntel::ForbiddenPoint::ForbiddenPoint(const std::string& ident, const Point& pt)
	: m_ident(ident), m_pt(pt)
{
}

bool CFMUAutoroute45::CFMUIntel::ForbiddenPoint::operator<(const ForbiddenPoint& x) const
{
	int c(get_ident().compare(x.get_ident()));
	if (c)
		return c < 0;
	if (get_pt().get_lat() < x.get_pt().get_lat())
		return true;
	if (x.get_pt().get_lat() < get_pt().get_lat())
		return false;
	return get_pt().get_lon() < x.get_pt().get_lon();
}

bool CFMUAutoroute45::CFMUIntel::ForbiddenPoint::is_valid(void) const
{
	return !get_ident().empty() && !get_pt().is_invalid();
}

void CFMUAutoroute45::CFMUIntel::ForbiddenPoint::save(sqlite3x::sqlite3_command& cmd) const
{
	cmd.bind(1, get_pt().get_lat());
	cmd.bind(2, get_pt().get_lon());
	cmd.bind(3, get_ident());
}

void CFMUAutoroute45::CFMUIntel::ForbiddenPoint::load(sqlite3x::sqlite3_cursor& cursor)
{
	m_pt = Point(cursor.getint(1), cursor.getint(0));
	m_ident = cursor.getstring(2);
}

CFMUAutoroute45::CFMUIntel::ForbiddenSegment::ForbiddenSegment(const std::string& fromident, const Point& frompt, unsigned int fromalt,
							     const std::string& toident, const Point& topt, unsigned int toalt,
							     lgraphairwayindex_t awy, const std::string& awyident)
	: m_fromident(fromident), m_toident(toident), m_awyident(awyident), m_frompt(frompt), m_topt(topt), m_fromalt(fromalt), m_toalt(toalt)
{
	if (awy == airwaydct) {
		m_awyident = "|";
	} else if (awy == airwaysid) {
		m_awyident = ">";
	} else if (awy == airwaystar) {
		m_awyident = "<";
	} else if (!LEdge::is_match(awy, airwaymatchawy)) {
		m_awyident.clear();
	}
}

CFMUAutoroute45::lgraphairwayindex_t CFMUAutoroute45::CFMUIntel::ForbiddenSegment::get_awyident(std::string& id) const
{
	if (get_awyident() == "|") {
		id.clear();
		return airwaydct;
	}
	if (get_awyident() == ">") {
		id.clear();
		return airwaysid;
	}
	if (get_awyident() == "<") {
		id.clear();
		return airwaystar;
	}
	id = get_awyident();
	return airwaymatchawy;
}

bool CFMUAutoroute45::CFMUIntel::ForbiddenSegment::operator<(const ForbiddenSegment& x) const
{
	int c(get_fromident().compare(x.get_fromident()));
	if (c)
		return c < 0;
	if (get_frompt().get_lat() < x.get_frompt().get_lat())
		return true;
	if (x.get_frompt().get_lat() < get_frompt().get_lat())
		return false;
	if (get_frompt().get_lon() < x.get_frompt().get_lon())
		return true;
	if (x.get_frompt().get_lon() < get_frompt().get_lon())
		return false;
	if (get_fromalt() < x.get_fromalt())
		return true;
	if (x.get_fromalt() < get_fromalt())
		return false;
	c = get_toident().compare(x.get_toident());
	if (c)
		return c < 0;
	if (get_topt().get_lat() < x.get_topt().get_lat())
		return true;
	if (x.get_topt().get_lat() < get_topt().get_lat())
		return false;
	if (get_topt().get_lon() < x.get_topt().get_lon())
		return true;
	if (x.get_topt().get_lon() < get_topt().get_lon())
		return false;
	if (get_toalt() < x.get_toalt())
		return true;
	if (x.get_toalt() < get_toalt())
		return false;
	c = get_awyident().compare(x.get_awyident());
	return c < 0;
}

bool CFMUAutoroute45::CFMUIntel::ForbiddenSegment::is_valid(void) const
{
	return !get_fromident().empty() && !get_frompt().is_invalid() &&
		!get_toident().empty() && !get_topt().is_invalid() &&
		!get_awyident().empty();
}

void CFMUAutoroute45::CFMUIntel::ForbiddenSegment::save(sqlite3x::sqlite3_command& cmd) const
{
	cmd.bind(1, get_frompt().get_lat());
	cmd.bind(2, get_frompt().get_lon());
	cmd.bind(3, get_fromident());
	cmd.bind(4, (sqlite3x::int64_t)get_fromalt());
	cmd.bind(5, get_topt().get_lat());
	cmd.bind(6, get_topt().get_lon());
	cmd.bind(7, get_toident());
	cmd.bind(8, (sqlite3x::int64_t)get_toalt());
	cmd.bind(9, get_awyident());
}

void CFMUAutoroute45::CFMUIntel::ForbiddenSegment::load(sqlite3x::sqlite3_cursor& cursor)
{
	m_frompt = Point(cursor.getint(1), cursor.getint(0));
	m_fromident = cursor.getstring(2);
	m_fromalt = cursor.getint(3);
	m_topt = Point(cursor.getint(5), cursor.getint(4));
	m_toident = cursor.getstring(6);
	m_toalt = cursor.getint(7);
	m_awyident = cursor.getstring(8);
}

CFMUAutoroute45::CFMUIntel::CFMUIntel()
{
}

CFMUAutoroute45::CFMUIntel::~CFMUIntel()
{
	close();
}

void CFMUAutoroute45::CFMUIntel::clear(void)
{
	m_forbiddenpoints.clear();
	m_forbiddensegments.clear();
}

void CFMUAutoroute45::CFMUIntel::flush(void)
{
	sqlite3x::sqlite3_transaction tr(m_db);
	for (forbiddenpoints_t::const_iterator i(m_forbiddenpoints.begin()), e(m_forbiddenpoints.end()); i != e; ++i) {
		sqlite3x::sqlite3_command cmd(m_db, "INSERT OR IGNORE INTO forbiddenpoints (LAT,LON,IDENT) VALUES (?,?,?);");
		i->save(cmd);
		cmd.executenonquery();
	}
	for (forbiddensegments_t::const_iterator i(m_forbiddensegments.begin()), e(m_forbiddensegments.end()); i != e; ++i) {
		sqlite3x::sqlite3_command cmd(m_db, "INSERT OR IGNORE INTO forbiddensegments (FROMLAT,FROMLON,FROMIDENT,FROMALT,TOLAT,TOLON,TOIDENT,TOALT,AWYIDENT) VALUES (?,?,?,?,?,?,?,?,?);");
		i->save(cmd);
		cmd.executenonquery();
	}
	tr.commit();
	clear();
}

void CFMUAutoroute45::CFMUIntel::add(const ForbiddenPoint& p)
{
	if (!p.is_valid())
		return;
	m_forbiddenpoints.insert(p);
}

void CFMUAutoroute45::CFMUIntel::add(const ForbiddenSegment& s)
{
	if (!s.is_valid())
		return;
	m_forbiddensegments.insert(s);
}

void CFMUAutoroute45::CFMUIntel::open(const std::string& maindir, const std::string& auxdir)
{
	std::string dbname(maindir);
	if (dbname.empty())
                dbname = FPlan::get_userdbpath();
        dbname = Glib::build_filename(dbname, "cfmuintel.db");
	m_db.open(dbname);
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS forbiddenpoints (LAT INTEGER, LON INTEGER, IDENT TEXT NOT NULL, "
					      "CONSTRAINT unq UNIQUE(LAT,LON,IDENT));");
		cmd.executenonquery();
	}
        {
		sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS forbiddenpoints_latlonident ON forbiddenpoints(LAT,LON,IDENT);");
		cmd.executenonquery();
	}
        {
		sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS forbiddenpoints_identlatlon ON forbiddenpoints(IDENT,LAT,LON);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS forbiddensegments (FROMLAT INTEGER, FROMLON INTEGER, FROMIDENT TEXT NOT NULL, FROMALT INTEGER, "
					      "TOLAT INTEGER, TOLON INTEGER, TOIDENT TEXT NOT NULL, TOALT INTEGER, AWYIDENT TEXT, "
					      "CONSTRAINT unq UNIQUE(FROMLAT,FROMLON,FROMIDENT,FROMALT,TOLAT,TOLON,TOIDENT,TOALT,AWYIDENT));");
		cmd.executenonquery();
	}
        {
		sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS forbiddensegments_latlonident ON forbiddensegments(FROMLAT,FROMLON,FROMIDENT);");
		cmd.executenonquery();
	}
        {
		sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS forbiddensegments_identlatlon ON forbiddensegments(FROMIDENT,FROMLAT,FROMLON);");
		cmd.executenonquery();
	}
        if (!auxdir.empty()) {
		std::string dbname(Glib::build_filename(auxdir, "cfmuintel.db"));
		try {
			sqlite3x::sqlite3_command cmd(m_db, "ATTACH DATABASE ?1 AS aux;");
			cmd.bind(1, dbname);
			cmd.executenonquery();
		} catch (...) {
		}
        }
}

void CFMUAutoroute45::CFMUIntel::close(void)
{
	m_db.close();
}

bool CFMUAutoroute45::CFMUIntel::is_open(void) const
{
	return !!m_db.db();
}

bool CFMUAutoroute45::CFMUIntel::is_empty(void) const
{
	return m_forbiddenpoints.empty() && m_forbiddensegments.empty();
}

void CFMUAutoroute45::CFMUIntel::find(const Rect& bbox, const sigc::slot<void,const ForbiddenPoint&>& ptfunc)
{
	flush();
	std::string cond("(LAT >= ? AND LAT <= ?) AND ");
	if (bbox.get_east() < bbox.get_west())
		cond += "(LON >= ? OR LON <= ?)";
	else
		cond += "(LON >= ? AND LON <= ?)";
	sqlite3x::sqlite3_command cmd(m_db, "SELECT LAT,LON,IDENT FROM forbiddenpoints WHERE " + cond + ";");
	cmd.bind(1, bbox.get_south());
	cmd.bind(2, bbox.get_north());
	cmd.bind(3, bbox.get_west());
	cmd.bind(4, bbox.get_east());
	sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
	while (cursor.step()) {
		ForbiddenPoint p;
		p.load(cursor);
		if (!p.is_valid() || !bbox.is_inside(p.get_pt()))
			continue;
		ptfunc(p);
	}
}

void CFMUAutoroute45::CFMUIntel::find(const Rect& bbox, const sigc::slot<void,const ForbiddenSegment&>& segfunc)
{
	flush();
	std::string cond("(FROMLAT >= ? AND FROMLAT <= ?) AND ");
	if (bbox.get_east() < bbox.get_west())
		cond += "(FROMLON >= ? OR FROMLON <= ?)";
	else
		cond += "(FROMLON >= ? AND FROMLON <= ?)";
	sqlite3x::sqlite3_command cmd(m_db, "SELECT FROMLAT,FROMLON,FROMIDENT,FROMALT,TOLAT,TOLON,TOIDENT,TOALT,AWYIDENT FROM forbiddensegments WHERE " + cond + ";");
	cmd.bind(1, bbox.get_south());
	cmd.bind(2, bbox.get_north());
	cmd.bind(3, bbox.get_west());
	cmd.bind(4, bbox.get_east());
	sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
	while (cursor.step()) {
		ForbiddenSegment s;
		s.load(cursor);
		if (!s.is_valid() || !bbox.is_inside(s.get_frompt()) || !bbox.is_inside(s.get_topt()))
			continue;
		segfunc(s);
	}
}
