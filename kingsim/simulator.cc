//
// C++ Implementation: King GPS Simulator
//
// Description: King GPS Simulator
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2012, 2013, 2014
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#ifdef HAVE_PTY_H
#include <pty.h>
#endif
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <cstdlib>
#include <limits>
#include <iomanip>
#include <glibmm/datetime.h>

#include "simulator.h"
#include "wmm.h"


Simulator::Waypoint::Waypoint(const Point& coord, char type, const std::string& ident, float alt)
	: m_coord(coord), m_var(0), m_type(type)
{
	memset(m_ident, ' ', sizeof(m_ident));
	if (!ident.empty())
		memcpy(m_ident, ident.c_str(), std::min(ident.size(), sizeof(m_ident)));
	else
		m_ident[0] = 0x7f;
	WMM wmm;
	wmm.compute(alt * (FPlanWaypoint::ft_to_m / 2000.0), get_coord(), time(0));
	if (wmm)
		m_var = wmm.get_dec();
}

std::string Simulator::Waypoint::get_ident(void) const
{
	if (m_ident[0] == 0x7f)
		return std::string();
	std::string id((const char *)m_ident, sizeof(m_ident));
	while (!id.empty() && id[id.size()-1] == ' ')
		id.resize(id.size() - 1);
	return id;
}

std::string Simulator::Waypoint::get_binary(void) const
{
	std::string ret((const char *)m_ident, sizeof(m_ident));
	{
		double lat(get_coord().get_lat_deg_dbl());
		uint8_t sgn((lat < 0) << 7);
		lat = fabs(lat);
		unsigned int deg(floor(lat));
		lat -= deg;
		lat *= 60.0;
		unsigned int min(floor(lat));
		lat -= min;
		lat *= 100.0;
		unsigned int hmin(floor(lat));
		ret += (char)(sgn | deg);
		ret += (char)(min & 0x3f);
		ret += (char)(hmin & 0x7f);
	}
	{
		double lon(get_coord().get_lon_deg_dbl());
		uint8_t sgn((lon < 0) << 7);
		lon = fabs(lon);
		unsigned int deg(floor(lon));
		lon -= deg;
		lon *= 60.0;
		unsigned int min(floor(lon));
		lon -= min;
		lon *= 100.0;
		unsigned int hmin(floor(lon));
		ret += (char)sgn;
		ret += (char)deg;
		ret += (char)(min & 0x3f);
		ret += (char)(hmin & 0x7f);
	}
	{
		int16_t var(floor(16.0 * get_variation() + 0.5));
		ret += (char)(var >> 8);
		ret += (char)(var & 0xff);
	}
	return ret;
}

const char Simulator::stx;
const char Simulator::etx;

Simulator::Simulator(Engine& engine, const std::string& file, bool dounlink, protocol_t protocol)
	: m_engine(engine), m_devfile(file), m_fd(-1), m_terminate(false),
	  m_pos(104481044, 565221672), m_alt(2500), m_verticalspeed(0), m_targetalt(2500),
	  m_groundspeed(0), m_course(0), m_fplanidx((fplan_t::size_type)-1), m_verbose(0), m_eolmode(eol_cr),
	  m_protocol(protocol)
{
	if (m_protocol == protocol_garmin)
		m_eolmode = eol_lf;
	m_cmdlist["help"] = &Simulator::cmd_help;
	m_cmdlist["quit"] = &Simulator::cmd_quit;
	m_cmdlist["qnh"] = &Simulator::cmd_qnh;
	m_cmdlist["pos"] = &Simulator::cmd_pos;
	m_cmdlist["alt"] = &Simulator::cmd_alt;
	m_cmdlist["course"] = &Simulator::cmd_course;
	m_cmdlist["gs"] = &Simulator::cmd_gs;
	m_cmdlist["fplan"] = &Simulator::cmd_fplan;
	m_cmdlist["verbose"] = &Simulator::cmd_verbose;
	int slave(-1);
        char ttyname[32] = { 0, };
#ifdef HAVE_PTY_H
        if (openpty(&m_fd, &slave, ttyname, 0, 0)) {
		std::ostringstream oss;
		oss << "openpty: cannot create terminal: " << strerror(errno);
                throw std::runtime_error(oss.str());
	}
#else
	throw std::runtime_error("win32: cannot create terminal");
#endif
        /* set mode to raw */
#ifdef HAVE_TERMIOS_H
        struct termios tm;
        memset(&tm, 0, sizeof(tm));
        tm.c_cflag = CS8 | CREAD | CLOCAL;
        if (tcsetattr(m_fd, TCSANOW, &tm))
		std::cerr << "cannot set master terminal attributes: " << strerror(errno) << std::endl;
	memset(&tm, 0, sizeof(tm));
        tm.c_cflag = CS8 | CREAD | CLOCAL;
        if (tcsetattr(slave, TCSANOW, &tm))
		std::cerr << "cannot set slave terminal attributes: " << strerror(errno) << std::endl;
        fchmod(slave, 0600);
	if (!m_devfile.empty()) {
		if (dounlink)
			unlink(m_devfile.c_str());
		if (symlink(ttyname, m_devfile.c_str())) {
			std::cerr << "symlink error: " << ttyname << " -> " << m_devfile << std::endl;
			m_devfile.clear();
		}
	}
	close(slave);
	fcntl(m_fd, F_SETFL, fcntl(m_fd, F_GETFL, 0) | O_NONBLOCK);
#endif
	m_lastperiodic.assign_current_time();
	m_periodic = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Simulator::periodic), 1000, Glib::PRIORITY_DEFAULT);
	std::cout << "Starting GPS Simulator at " << (m_devfile.empty() ? std::string(ttyname) : m_devfile) << std::endl;
}

Simulator::~Simulator()
{
	m_periodic.disconnect();
	close(m_fd);
	if (!m_devfile.empty())
		unlink(m_devfile.c_str());
}

const std::string& Simulator::to_str(findcoord_t ft)
{
	switch (ft) {
	default:
	case findcoord_invalid:
	{
		static const std::string r("invalid");
		return r;
	}

	case findcoord_coord:
	{
		static const std::string r("coord");
		return r;
	}

	case findcoord_airport:
	{
		static const std::string r("airport");
		return r;
	}

	case findcoord_military:
	{
		static const std::string r("military");
		return r;
	}

	case findcoord_heliport:
	{
		static const std::string r("heliport");
		return r;
	}

	case findcoord_intersection:
	{
		static const std::string r("intersection");
		return r;
	}

	case findcoord_vor:
	{
		static const std::string r("vor");
		return r;
	}

	case findcoord_ndb:
	{
		static const std::string r("ndb");
		return r;
	}
	}
}

char Simulator::waypoint_type(findcoord_t ft)
{
	switch (ft) {
	default:
	case findcoord_invalid:
		return '?';

	case findcoord_coord:
		return 'u';

	case findcoord_airport:
		return 'a';

	case findcoord_military:
		return 'm';

	case findcoord_heliport:
		return 'h';

	case findcoord_intersection:
		return 'i';

	case findcoord_vor:
		return 'v';

	case findcoord_ndb:
		return 'n';
	}
}

bool Simulator::parse_coord(const std::string& name, Point& coord)
{
	static const uint32_t mul[7] = {
		Point::round<uint32_t,double>(Point::from_deg_dbl * 100),
		Point::round<uint32_t,double>(Point::from_deg_dbl * 10),
		Point::round<uint32_t,double>(Point::from_deg_dbl),
		Point::round<uint32_t,double>((Point::from_deg_dbl / 60.0) * 10),
		Point::round<uint32_t,double>((Point::from_deg_dbl / 60.0)),
		Point::round<uint32_t,double>((Point::from_deg_dbl / 60.0) * 0.1),
		Point::round<uint32_t,double>((Point::from_deg_dbl / 60.0) * 0.01)
	};
	unsigned int ptr(1);
	bool lat(true);
	Point pt(0, 0);
	std::string::const_iterator i(name.begin()), e(name.end());
	while (i != e) {
		char ch(*i);
		++i;
		if (ch >= '0' && ch <= '9') {
			if (ptr >= sizeof(mul)/sizeof(mul[0]))
				return false;
			if (lat)
				pt.set_lat(pt.get_lat() + (ch - '0') * mul[ptr]);
			else
				pt.set_lon(pt.get_lon() + (ch - '0') * mul[ptr]);
			++ptr;
			continue;
		}
		if (ch == 'n' || ch == 'N' || ch == 's' || ch == 'S') {
			if (!lat)
				return false;
			if (ch == 's' || ch == 'S')
				pt.set_lat(-pt.get_lat());
			lat = false;
			ptr = 0;
			continue;
		}
		if (ch == 'e' || ch == 'E' || ch == 'w' || ch == 'W') {
			if (lat)
				return false;
			if (ch == 'w' || ch == 'W')
				pt.set_lon(-pt.get_lon());
			break;
		}
		break;
	}
	if (i != e)
		return false;
	coord = pt;
	return true;
}

void Simulator::dbfind_done(void)
{
	Glib::Mutex::Lock lock(m_dbfindmutex);
	m_dbfindcond.signal();
}

Simulator::findcoord_t Simulator::find_coord(const std::string& name, Point& coord)
{
	int alt = 0;
	Glib::ustring ident;
	return find_coord(name, coord, ident, alt);
}

Simulator::findcoord_t Simulator::find_coord(const std::string& name, Point& coord, Glib::ustring& ident, int& alt)
{
	if (parse_coord(name, coord))
		return findcoord_coord;
	std::string name1(name);
	double course(0), dist(0);
	if (name1.size() >= 7) {
		std::string ncrs(name1, name1.size() - 6, 3);
		std::string ndist(name1, name1.size() - 3, 3);
		bool ok(true);
		for (unsigned int i = 0; i < 3; ++i)
			if (ncrs[i] < '0' || ncrs[i] > '9' || ndist[i] < '0' || ndist[i] > '9') {
				ok = false;
				break;
			}
		if (ok) {
			course = Glib::Ascii::strtod(ncrs);
			dist = Glib::Ascii::strtod(ndist);
			name1.erase(name1.size() - 6);
			// convert magnetic to true
			WMM wmm;
			wmm.compute(m_alt * (FPlanWaypoint::ft_to_m / 2000.0), m_pos, time(0));
			if (wmm)
				course += wmm.get_dec();
		}
	}
	{
		Glib::RefPtr<Engine::AirportResult> res(m_engine.async_airport_find_by_icao(name1, ~0, 0, AirportsDb::comp_exact, 0));
		{
			Glib::Mutex::Lock lock(m_dbfindmutex);
			res->connect(sigc::mem_fun(*this, &Simulator::dbfind_done));
			for (;;) {
				m_dbfindcond.wait(m_dbfindmutex);
				if (res->is_done() || res->is_error())
					break;
			}
		}
		if (res->is_done() && !res->is_error() && !res->get_result().empty()) {
			Point pt;
			uint64_t bestdist(std::numeric_limits<uint64_t>::max());
			findcoord_t fc(findcoord_airport);
			alt = 0;
			for (Engine::AirportResult::elementvector_t::const_iterator ei(res->get_result().begin()), ee(res->get_result().end()); ei != ee; ++ei) {
				uint64_t dist((coord - ei->get_coord()).length2());
				if (dist > bestdist)
					continue;
				bestdist = dist;
				pt = ei->get_coord();
				fc = findcoord_airport;
				if (ei->get_typecode() == 'C')
					fc = findcoord_military;
				else if (!ei->get_nrrwy() && ei->get_nrhelipads())
					fc = findcoord_heliport;
				alt = ei->get_elevation();
				ident = ei->get_icao();
			}
			coord = pt.spheric_course_distance_nmi(course, dist);
			return fc;
		}
	}
	{
		Glib::RefPtr<Engine::WaypointResult> res(m_engine.async_waypoint_find_by_name(name1, ~0, 0, WaypointsDb::comp_exact, 0));
		{
			Glib::Mutex::Lock lock(m_dbfindmutex);
			res->connect(sigc::mem_fun(*this, &Simulator::dbfind_done));
			for (;;) {
				m_dbfindcond.wait(m_dbfindmutex);
				if (res->is_done() || res->is_error())
					break;
			}
		}
		if (res->is_done() && !res->is_error() && !res->get_result().empty()) {
			Point pt;
			uint64_t bestdist(std::numeric_limits<uint64_t>::max());
			for (Engine::WaypointResult::elementvector_t::const_iterator ei(res->get_result().begin()), ee(res->get_result().end()); ei != ee; ++ei) {
				uint64_t dist((coord - ei->get_coord()).length2());
				if (dist > bestdist)
					continue;
				bestdist = dist;
				pt = ei->get_coord();
				ident = ei->get_name();
			}
			coord = pt.spheric_course_distance_nmi(course, dist);
			return findcoord_intersection;
		}
	}
	{
		Glib::RefPtr<Engine::NavaidResult> res(m_engine.async_navaid_find_by_icao(name1, ~0, 0, NavaidsDb::comp_exact, 0));
		{
			Glib::Mutex::Lock lock(m_dbfindmutex);
			res->connect(sigc::mem_fun(*this, &Simulator::dbfind_done));
			for (;;) {
				m_dbfindcond.wait(m_dbfindmutex);
				if (res->is_done() || res->is_error())
					break;
			}
		}
		if (res->is_done() && !res->is_error() && !res->get_result().empty()) {
			Point pt;
			uint64_t bestdist(std::numeric_limits<uint64_t>::max());
			bool isvor(false);
			alt = 0;
			for (Engine::NavaidResult::elementvector_t::const_iterator ei(res->get_result().begin()), ee(res->get_result().end()); ei != ee; ++ei) {
				bool vor(false);
				if (ei->get_navaid_type() == DbBaseElements::Navaid::navaid_vor || ei->get_navaid_type() == DbBaseElements::Navaid::navaid_vordme)
					vor = true;
				else if (ei->get_navaid_type() != DbBaseElements::Navaid::navaid_ndb)
					continue;
				uint64_t dist((coord - ei->get_coord()).length2());
				if (dist > bestdist)
					continue;
				bestdist = dist;
				pt = ei->get_coord();
				isvor = vor;
				alt = ei->get_elev();
				ident = ei->get_icao();
			}
			coord = pt.spheric_course_distance_nmi(course, dist);
			return isvor ? findcoord_vor : findcoord_ndb;
		}
	}
	return findcoord_invalid;
}	

bool Simulator::periodic(void)
{
	Glib::TimeVal tv;
	{
		Glib::TimeVal tv2;
		tv2.assign_current_time();
		tv = tv2 - m_lastperiodic;
		m_lastperiodic = tv2;
	}
	if (m_verticalspeed != 0) {
		m_alt += m_verticalspeed * (1.0 / 60.0) * tv.as_double();
		if ((m_alt >= m_targetalt && m_verticalspeed > 0) ||
		    (m_alt <= m_targetalt && m_verticalspeed < 0)) {
			m_alt = m_targetalt;
			m_verticalspeed = 0;
		}
	}
	m_pos = m_pos.spheric_course_distance_nmi(m_course, m_groundspeed * (1.0 / 3600.0) * tv.as_double());
	if (m_verbose >= 1)
		std::cout << "Position " << m_pos.get_lat_str2() << ' ' << m_pos.get_lon_str2() << " Altitude " << m_alt << "ft" << std::endl;
	if (!m_txbuf.empty())
		return true;
	WMM wmm;
	wmm.compute(m_alt * (FPlanWaypoint::ft_to_m / 2000.0), m_pos, m_lastperiodic.tv_sec);
       	{
		std::ostringstream oss;
		std::ostringstream ossx;
		oss << stx;
		ossx << std::fixed;
		if (m_protocol == protocol_garmin)
			oss << "z" << std::setfill('0') << std::setw(5) << std::fixed << Point::round<int32_t,double>(m_alt) << eol();
		{
			double x(fabs(m_pos.get_lat() * Point::to_deg_dbl));
			int deg(floor(x));
			x -= deg;
			int min(Point::round<int,double>(x * 6000));
			oss << 'A' << (m_pos.get_lat() < 0 ? 'S' : 'N')
			    << ' ' << std::setfill('0') << std::setw(2) << deg
			    << ' ' << std::setfill('0') << std::setw(4) << min << eol();
		}
		{
			double x(fabs(m_pos.get_lon() * Point::to_deg_dbl));
			int deg(floor(x));
			x -= deg;
			int min(Point::round<int,double>(x * 6000));
			oss << 'B' << (m_pos.get_lon() < 0 ? 'W' : 'E')
			    << ' ' << std::setfill('0') << std::setw(3) << deg
			    << ' ' << std::setfill('0') << std::setw(4) << min << eol();
		}
		if (m_protocol == protocol_trimble) {
			double min(fabs(m_pos.get_lat() * Point::to_deg_dbl));
			int deg(floor(min));
			min -= deg;
			min *= 60;
			ossx << 'k' << (m_pos.get_lat() < 0 ? 'S' : 'N')
			     << ' ' << std::setfill('0') << std::setw(2) << deg
			     << ' ' << std::setfill('0') << std::setw(8) << std::setprecision(5) << min;
			min = fabs(m_pos.get_lon() * Point::to_deg_dbl);
			deg = floor(min);
			min -= deg;
			min *= 60;
			ossx << ' ' << (m_pos.get_lon() < 0 ? 'W' : 'E')
			     << ' ' << std::setfill('0') << std::setw(3) << deg
			     << ' ' << std::setfill('0') << std::setw(8) << std::setprecision(5) << min
			     << ' ' << std::setfill('0') << std::setw(5) << std::setprecision(1) << m_groundspeed << eol();
		}
		{
			double crs(m_course);
			if (wmm)
				crs -= wmm.get_dec();
			while (crs < 0)
				crs += 360;
			while (crs >= 360)
				crs -= 360;
			oss << 'C' << std::setfill('0') << std::setw(3) << Point::round<int,double>(crs) << eol();
		}
		oss << 'D' << std::setfill('0') << std::setw(3) << Point::round<int,double>(m_groundspeed) << eol();
		if (!m_fplan.empty() && m_fplanidx < m_fplan.size()) {
			// check advance fplan index
			while (m_fplanidx + 1 < m_fplan.size()) {
				uint16_t course(m_course * ((1 << 16) / 360.0));
				uint16_t wpcourse(m_pos.spheric_true_course(m_fplan[m_fplanidx].get_coord()) * ((1 << 16) / 360.0));
				int16_t brg(course - wpcourse);
				if (abs(brg) <= 0x4000)
					break;
				++m_fplanidx;
			}
			double distwpt(m_pos.spheric_distance_nmi_dbl(m_fplan[m_fplanidx].get_coord()));
			double dist(distwpt);
			for (fplan_t::size_type i(m_fplanidx + 1); i < m_fplan.size(); ++i)
				dist += m_fplan[i-1].get_coord().spheric_distance_nmi_dbl(m_fplan[i].get_coord());
			double brg(m_pos.spheric_true_course(m_fplan[m_fplanidx].get_coord()));
			double dtk(brg);
			if (m_fplanidx)
				dtk = m_fplan[m_fplanidx-1].get_coord().spheric_true_course(m_fplan[m_fplanidx].get_coord());
			double xtk(0);
			{
				Point vec2(m_pos.simple_unwrap(m_fplan[m_fplanidx].get_coord()));
				Point vec1(vec2);
				if (m_fplanidx)
					vec1 = m_fplan[m_fplanidx-1].get_coord().simple_unwrap(m_fplan[m_fplanidx].get_coord());
				xtk = vec2.get_lat_deg_dbl() * vec1.get_lon_deg_dbl() - vec2.get_lon_deg_dbl() * vec1.get_lat_deg_dbl();
				xtk *= 60.0;
			}
			double tke(m_course - dtk);
			while (tke < -180)
				tke += 360;
			while (tke > 180)
				tke -= 360;
			{
				int dist1(floor(dist * 100 + 0.5));
				oss << 'E' << std::setfill('0') << std::setw((m_protocol == protocol_trimble) ? 6 : 5) << dist1 << eol();
			}
			if (m_protocol == protocol_trimble) {
				int min(floor(dist / std::max(m_groundspeed, 1.0) * 60 + 0.5));
				int hr(min / 60);
				min -= hr * 60;
				if (hr > 99) {
					hr = 99;
					min = 59;
				}
				oss << 'F' << std::setfill('0') << std::setw(2) << hr
				    << std::setfill('0') << std::setw(2) << min << eol();
			}
			{
				int xtk1(floor(xtk * 100 + 0.5));
				oss << 'G' << (xtk1 < 0 ? 'L' : 'R') << std::setfill('0') << std::setw(4) << std::min(abs(xtk1), 9999) << eol();
			}
			if (m_protocol == protocol_trimble) {
				int tke1(floor(tke * 10 + 0.5));
				oss << 'H' << (tke1 < 0 ? 'L' : 'R') << std::setfill('0') << std::setw(4) << abs(tke1) << eol();
			}
			{
				double dtk2(dtk);
				if (wmm)
					dtk2 -= wmm.get_dec();
				while (dtk2 < 0)
					dtk2 += 360;
				while (dtk2 >= 360)
					dtk2 -= 360;
				int dtk1(floor(dtk2 * 10 + 0.5));
				oss << 'I' << std::setfill('0') << std::setw(4) << dtk1 << eol();
				if (m_protocol == protocol_trimble)
					oss << 'J' << std::setfill('0') << std::setw(2) << (m_fplanidx + 1) << eol();
				double brg2(brg);
				if (wmm)
					brg2 -= wmm.get_dec();
				while (brg2 < 0)
					brg2 += 360;
				while (brg2 >= 360)
					brg2 -= 360;
				int brg1(floor(brg2 * 10 + 0.5));
				oss 
					<< 'L' << std::setfill('0') << std::setw(4) << brg1 << eol();
				if (m_protocol == protocol_trimble) {
					double crs(m_course);
					if (wmm)
						crs -= wmm.get_dec();
					while (crs < 0)
						crs += 360;
					while (crs >= 360)
						crs -= 360;
					ossx << "m " << std::setfill('0') << std::setw(6) << std::setprecision(2) << std::fixed << crs
					     << ' ' << std::setfill('0') << std::setw(6) << std::setprecision(2) << std::fixed << dtk2
					     << ' ' << (xtk < 0 ? 'L' : 'R') << std::setfill('0') << std::setw(8) << std::setprecision(5) << std::fixed << fabs(xtk)
					     << ' ' << (tke < 0 ? 'L' : 'R') << std::setfill('0') << std::setw(6) << std::setprecision(2) << std::fixed << fabs(tke)
					     << eol();
				}
			}
			if (m_protocol == protocol_trimble) {
				int sec(floor(distwpt / std::max(m_groundspeed, 1.0) * (60 * 60) + 0.5));
				int hr(sec / (60 * 60));
				sec -= hr * (60 * 60);
				int min(sec / 60);
				sec -= min * 60;
				if (hr > 99) {
					hr = 99;
					min = 59;
					sec = 59;
				}
				oss << "o " << std::setfill('0') << std::setw(9) << std::setprecision(5) << std::fixed << distwpt
				    << ' ' << std::setfill('0') << std::setw(7) << std::setprecision(3) << std::fixed << brg
				    << ' ' << std::setfill('0') << std::setw(2) << hr
				    << ':' << std::setfill('0') << std::setw(2) << min
				    << ':' << std::setfill('0') << std::setw(2) << sec << eol();
			}
		}
		if (!m_fplan.empty()) {
			oss << 'K' << m_fplan.back().get_ident() << eol();
		}
		if (m_protocol == protocol_trimble)
			oss << "M0000" << eol() << "P001" << eol();
		{
			int var(0);
			if (wmm)
				var = Point::round<int,double>(wmm.get_dec() * 10);
			oss << 'Q' << (var < 0 ? 'W' : 'E') << std::setfill('0') << std::setw(3) << var << eol();
		}
		if (m_protocol == protocol_trimble)
			oss << "c001" << eol();
		if (m_protocol == protocol_garmin)
			oss << "S----" << (m_pos.is_invalid() ? 'N' : '-') << eol();
		oss << "T-----------" << eol();
		if (m_protocol == protocol_trimble)
			oss << "s020000" << eol();
		if (m_protocol == protocol_garmin) {
			if (m_fplan.empty()) {
				oss << "l------" << eol();
			} else {
				double dist(m_pos.spheric_distance_nmi_dbl(m_fplan[m_fplan.size() - 1].get_coord()));
				oss << 'l' << std::setfill('0') << std::setw(6) << Point::round<int32_t,double>(dist * 10) << eol();
			}
		}
		if (m_protocol == protocol_trimble) {
                        float press, baroalt;
                        m_athmosphereqnh.altitude_to_pressure(&press, 0, m_alt * FPlanWaypoint::ft_to_m);
                        m_athmospherestd.pressure_to_altitude(&baroalt, 0, press);
                        baroalt *= FPlanWaypoint::m_to_ft;
			ossx << "l " << std::setfill('0') << std::setw(7) << std::setprecision(1) << std::fixed << m_alt
			     << ' ' << std::setfill('0') << std::setw(7) << std::setprecision(1) << std::fixed << baroalt
			     << ' ' << std::setfill('0') << std::setw(7) << std::setprecision(1) << std::fixed << baroalt << eol();
		}
		if (m_protocol == protocol_trimble) {
			Glib::DateTime dt(Glib::DateTime::create_now_utc(m_lastperiodic));
			Glib::DateTime dtloc(Glib::DateTime::create_now_local(m_lastperiodic));
			oss << 'i' << dt.format("%m/%d/%y") << eol()
			    << 'j' << dt.format("%H:%M:%S") << eol();
			ossx << "p " << dtloc.format("%H:%M:%S") << ' '
			     << std::fixed << std::setprecision(1) << (dt.get_utc_offset() / (double)(1000L * 1000L * 60L * 60L)) << eol()
			     << "q R0.000" << eol()
			     << "r " << dt.format("%H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << ((dt.get_microsecond() + 500) / 1000) << eol()
			     << "u 000.024" << eol();
		}
		if (m_protocol == protocol_trimble)
			ossx << "n " << (m_verticalspeed < 0 ? '-' : '+') << std::fixed << std::setprecision(1) << std::setw(6) << std::setfill('0')
			     << fabs(m_verticalspeed) << " ---- ---- ----" << eol()
			     << "z G.GPS-3D:4 LOR:0" << eol();
		if (!m_fplan.empty()) {
			for (fplan_t::size_type i(0); i < m_fplan.size(); ++i) {
				char ch(i + 1);
				if (i + 1 == m_fplan.size()) {
					ch |= 0x40;
					if (m_fplanidx == i)
						ch |= 0x20;
				}
				oss << 'w' << std::setfill('0') << std::setw(2) << (i + 1) << ch << m_fplan[i].get_binary() << eol();
			}
			if (m_protocol == protocol_trimble) {
				oss << "t ";
				for (fplan_t::size_type i(0); i < m_fplan.size(); ++i)
					oss << m_fplan[i].get_type();
				oss << eol();
			}
		}
		oss << ossx.str() << etx;
		m_txbuf += oss.str();
		if (m_verbose >= 2) {
			std::cout << "Tx: ";
			for (std::string::const_iterator si(oss.str().begin()), se(oss.str().end()); si != se; ++si) {
				if (*si >= 32 && *si < 128) {
					std::cout << *si;
					continue;
				}
				std::ostringstream oss;
				oss << "\\x" << std::hex << std::setfill('0') << std::setw(2) << (unsigned int)*si;
				std::cout << oss.str();
			}
			std::cout << std::endl;
		}
	}
      	serial_output(Glib::IO_OUT);
	return true;
}

bool Simulator::serial_output(Glib::IOCondition iocond)
{
	m_txconn.disconnect();
	if (m_txbuf.empty())
		return false;
	if (iocond & Glib::IO_OUT) {
		int r(write(m_fd, m_txbuf.c_str(), m_txbuf.size()));
		if (r == -1) {
			if (errno == EINTR || errno == EAGAIN) {
				m_txconn = Glib::signal_io().connect(sigc::mem_fun(*this, &Simulator::serial_output), m_fd, Glib::IO_OUT);
				return false;
			}
			throw std::runtime_error(std::string("I/O error: ") + strerror(errno));
		}
		if (r > 0)
			m_txbuf.erase(0, r);
	}
	if (m_txbuf.empty())
		return false;
	m_txconn = Glib::signal_io().connect(sigc::mem_fun(*this, &Simulator::serial_output), m_fd, Glib::IO_OUT);
	return false;
}

const char *Simulator::eol(void) const
{
	static const char * const eols[3] = { "\r\n", "\r", "\n" };
	return eols[m_eolmode];
}

bool Simulator::command(const std::string& cmd)
{
	cmdline_t cmdline;
	for (std::string::const_iterator i(cmd.begin()), e(cmd.end()); i != e;) {
		if (std::isspace(*i)) {
			++i;
			continue;
		}
		std::string::const_iterator i2(i);
		++i2;
		while (i2 != e && !std::isspace(*i2))
			++i2;
		cmdline.push_back(std::string(i, i2));
		i = i2;
	}
	if (cmdline.empty())
		return m_terminate;
	{
		cmdlist_t::const_iterator ci(m_cmdlist.find(cmdline.front()));
		if (ci != m_cmdlist.end()) {
			(this->*(ci->second))(cmdline);
			return m_terminate;
		}
	}
	std::cout << "Invalid command: " << cmdline.front() << std::endl;
	cmd_help(cmdline);
	return m_terminate;
}

void Simulator::cmd_help(const cmdline_t& args)
{
	std::cout << "Available commands:";
	for (cmdlist_t::const_iterator ci(m_cmdlist.begin()), ce(m_cmdlist.end()); ci != ce; ++ci)
		std::cout << ' ' << ci->first;
	std::cout << std::endl;
}

void Simulator::cmd_quit(const cmdline_t& args)
{
	m_terminate = true;
}

void Simulator::cmd_qnh(const cmdline_t& args)
{
	if (args.size() >= 2) {
		double tempoffs(0);
		if (args.size() >= 3)
			tempoffs = Glib::Ascii::strtod(args[2]);
		m_athmosphereqnh.set(Glib::Ascii::strtod(args[1]), tempoffs + (IcaoAtmosphere<float>::degc_to_kelvin + 15.0));
	}
	std::cout << "QNH " << m_athmosphereqnh.get_qnh()
		  << " TempOffs " << (m_athmosphereqnh.get_temp() - (IcaoAtmosphere<float>::degc_to_kelvin + 15.0))
		  << std::endl;
}

void Simulator::cmd_pos(const cmdline_t& args)
{
	std::string type;
	if (args.size() >= 2) {
		Point pos(m_pos);
		int alt(0);
		Glib::ustring ident;
		findcoord_t fc(find_coord(args[1], pos, ident, alt));
		if (fc == findcoord_invalid) {
			std::cerr << "Invalid coordinate " << args[1] << std::endl;
		} else {
			m_pos = pos;
			type = ident + " " + to_str(fc) + " ";
		}
	}
	std::cout << "Position " << type << m_pos.get_lat_str2() << ' ' << m_pos.get_lon_str2() << std::endl;
}

void Simulator::cmd_alt(const cmdline_t& args)
{
	if (args.size() >= 3) {
		m_targetalt = Glib::Ascii::strtod(args[1]);
		m_verticalspeed = fabs(Glib::Ascii::strtod(args[2]));
		if (m_targetalt < m_alt)
			m_verticalspeed = -m_verticalspeed;
	} else if (args.size() >= 2) {
		m_alt = m_targetalt = Glib::Ascii::strtod(args[1]);
		m_verticalspeed = 0;
	}
	std::cout << "Altitude " << m_alt << "ft";
	if (m_verticalspeed < 0)
		std::cout << " descend to " << m_targetalt << "ft VS " << -m_verticalspeed << "ft/min" << std::endl;
	else if (m_verticalspeed > 0)
		std::cout << " climb to " << m_targetalt << "ft VS " << m_verticalspeed << "ft/min" << std::endl;
	else
		std::cout << std::endl;
}

void Simulator::cmd_course(const cmdline_t& args)
{
	WMM wmm;
	wmm.compute(m_alt * (FPlanWaypoint::ft_to_m / 2000.0), m_pos, time(0));
	if (args.size() >= 2) {
		m_course = Glib::Ascii::strtod(args[1]);
		// convert magnetic to true
		if (wmm)
			m_course += wmm.get_dec();
	}
	double course(m_course);
	// convert true to magnetic
	if (wmm)
		course -= wmm.get_dec();
	std::cout << "Course " << course << "°M (" << m_course << "°T)" << std::endl;
}

void Simulator::cmd_gs(const cmdline_t& args)
{
	if (args.size() >= 2)
		m_groundspeed = Glib::Ascii::strtod(args[1]);
	std::cout << "Groundspeed " << m_groundspeed << "kts" << std::endl;
}

void Simulator::cmd_fplan(const cmdline_t& args)
{
	if (args.size() >= 2) {
		m_fplan.clear();
		if (args[1] != "-") {
			Point coord(m_pos);
			for (cmdline_t::size_type i(1); i < args.size(); ++i) {
				Glib::ustring ident;
				int alt(m_alt);
				findcoord_t fc(find_coord(args[i], coord, ident, alt));
				if (fc == findcoord_invalid) {
					std::cerr << "Invalid Waypoint " << args[i] << std::endl;
					continue;
				}
				m_fplan.push_back(Waypoint(coord, waypoint_type(fc), (fc == findcoord_coord ? Glib::ustring() : ident), alt));
			}
		}
		m_fplanidx = (fplan_t::size_type)-1;
		uint64_t dist(std::numeric_limits<uint64_t>::max());
		uint16_t course(m_course * ((1 << 16) / 360.0));
		for (fplan_t::size_type i(0); i < m_fplan.size(); ++i) {
			uint16_t wpcourse(m_pos.simple_true_course(m_fplan[i].get_coord()) * ((1 << 16) / 360.0));
			int16_t brg(course - wpcourse);
			if (abs(brg) > 0x4000)
				continue;
			uint64_t d(m_pos.simple_distance_rel(m_fplan[i].get_coord()));
			if (d > dist)
				continue;
			dist = d;
			m_fplanidx = i;
		}
		if (m_fplanidx == (fplan_t::size_type)-1 && m_fplan.size() > 0)
			m_fplanidx = m_fplan.size() - 1;
	}
	if (m_fplan.empty()) {
		std::cout << "No flightplan." << std::endl;
		return;
	}
	std::cout << "Flightplan (" << m_fplanidx << "):";
	for (fplan_t::const_iterator wi(m_fplan.begin()), we(m_fplan.end()); wi != we; ++wi) {
		std::cout << ' ' << wi->get_ident()
			  << ' ' << wi->get_coord().get_lat_str2() << ' ' << wi->get_coord().get_lon_str2()
			  << ' ' << wi->get_type() << ' ' << wi->get_variation();
	}
	std::cout << std::endl;
}

void Simulator::cmd_verbose(const cmdline_t& args)
{
	if (args.size() >= 2)
		m_verbose = strtoul(args[1].c_str(), 0, 0);
	std::cout << "Verbosity level " << m_verbose << std::endl;
}
