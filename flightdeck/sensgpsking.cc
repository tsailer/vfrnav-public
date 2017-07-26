//
// C++ Implementation: King GPS Sensor
//
// Description: GPS Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2014
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits>
#include <cstring>
#include <cstdio>
#include <iomanip>
#include <glibmm/datetime.h>
#include "sensgpsking.h"

// Parse Issues:
// King GPS: Parse Error: F----
// King GPS: Parse Error: d---
// King GPS: Parse Error: e---
// King GPS: Parse Error: l------- -2500.0 -2500.0
// King GPS: Parse Error: o   0.41162 177.043 --:--:--
// FPlan: N00 00.00000 E000 00.00000 DIR
// FPlan: N47 22.58011 E008 45.45004 LSZK

// King GPS: Parse Error: F----
// King GPS: Parse Error: d---
// King GPS: Parse Error: e---
// King GPS: Parse Error: l 1887.2 -2500.0 -2500.0
// King GPS: Parse Error: o   9.20959  63.744 --:--:--
// FPlan: N47 22.58011 E008 45.45004 LSZK
// FPlan: N47 26.46996 E008 58.00999 KUDIS
// FPlan: N47 42.78008 E009 06.47003 ROMIR
// FPlan: N47 47.40005 E009 07.22998 VEDOK
// FPlan: N48 37.10999 E009 15.55000 TGO
// FPlan: N48 54.77989 E009 20.42004 LBU
// FPlan: N49 11.71005 E008 56.97996 TAGIK
// FPlan: N49 27.14996 E008 59.14999 KETEG
// FPlan: N49 30.86998 E008 59.67001 EBATU
// FPlan: N49 45.97000 E008 50.92999 OLKAS
// FPlan: N49 51.43021 E008 44.92001 EGAKA
// FPlan: N49 57.64984 E008 38.49003 EDFE

// J1: first leg (i.e. LSZK->KUDIS)

Sensors::SensorKingGPS::Parser::Data::Data(void)
	: m_gpsalt(std::numeric_limits<double>::quiet_NaN()),
	  m_pressalt(std::numeric_limits<double>::quiet_NaN()),
	  m_baroalt(std::numeric_limits<double>::quiet_NaN()),
	  m_magtrack(std::numeric_limits<double>::quiet_NaN()),
	  m_dtk(std::numeric_limits<double>::quiet_NaN()),
	  m_gs(std::numeric_limits<double>::quiet_NaN()),
	  m_dist(std::numeric_limits<double>::quiet_NaN()),
	  m_destdist(std::numeric_limits<double>::quiet_NaN()),
	  m_xtk(std::numeric_limits<double>::quiet_NaN()),
	  m_tke(std::numeric_limits<double>::quiet_NaN()),
	  m_brg(std::numeric_limits<double>::quiet_NaN()),
	  m_ptk(std::numeric_limits<double>::quiet_NaN()),
	  m_actualvnav(std::numeric_limits<double>::quiet_NaN()),
	  m_desiredvnav(std::numeric_limits<double>::quiet_NaN()),
	  m_verterr(std::numeric_limits<double>::quiet_NaN()),
	  m_vertangleerr(std::numeric_limits<double>::quiet_NaN()),
	  m_poserr(std::numeric_limits<double>::quiet_NaN()),
	  m_magvar(std::numeric_limits<double>::quiet_NaN()),
	  m_msa(std::numeric_limits<double>::quiet_NaN()),
	  m_mesa(std::numeric_limits<double>::quiet_NaN()),
	  m_timesincesolution(std::numeric_limits<double>::quiet_NaN()),
	  m_time(-1, 0), m_day(-1), m_month(-1), m_year(-1), m_hour(-1), m_minute(-1),
	  m_seconds(std::numeric_limits<double>::quiet_NaN()),
	  m_utcoffset(std::numeric_limits<int>::min()),
	  m_timewpt(0), m_leg(~0U), m_satellites(~0U),
	  m_fixtype(fixtype_nofix), m_latset(false), m_lonset(false)
{
}

void Sensors::SensorKingGPS::Parser::Data::set_lat(double x)
{
	m_pos.set_lat_deg_dbl(x);
	m_latset = true;
	if (is_position_ok()) {
		if (std::isnan(m_gpsalt)) {
			if (m_fixtype < fixtype_2d)
				m_fixtype = fixtype_2d;
		} else {
			if (m_fixtype < fixtype_3d)
				m_fixtype = fixtype_3d;
		}
	}
}

void Sensors::SensorKingGPS::Parser::Data::set_lon(double x)
{
	m_pos.set_lon_deg_dbl(x);
	m_lonset = true;
	if (is_position_ok()) {
		if (std::isnan(m_gpsalt)) {
			if (m_fixtype < fixtype_2d)
				m_fixtype = fixtype_2d;
		} else {
			if (m_fixtype < fixtype_3d)
				m_fixtype = fixtype_3d;
		}
	}
}

void Sensors::SensorKingGPS::Parser::Data::set_gpsalt(double v)
{
	m_gpsalt = v;
	if (!std::isnan(m_gpsalt) && is_position_ok()) {
		if (m_fixtype < fixtype_3d)
			m_fixtype = fixtype_3d;
	}
}

void Sensors::SensorKingGPS::Parser::Data::set_dest(const std::string& v)
{
	m_dest = v;
	while (!m_dest.empty() && m_dest[m_dest.size()-1] == ' ')
		m_dest.resize(m_dest.size()-1);
}

void Sensors::SensorKingGPS::Parser::Data::set_dmy(int day, int month, int year)
{
	if (year < 100) {
		year += 1900;
		if (year < 1990)
			year += 100;
	}
	m_year = year;
	m_month = month;
	m_day = day;
}

void Sensors::SensorKingGPS::Parser::Data::set_hms(int hour, int min, double sec)
{
	m_hour = hour;
	m_minute = min;
	m_seconds = sec;
}

bool Sensors::SensorKingGPS::Parser::Data::is_dmy_valid(void) const
{
	return m_year >= 1 && m_year <= 9999 &&
		m_month >= 1 && m_month <= 12 &&
		m_day >= 1 && m_day <= 31;
}

bool Sensors::SensorKingGPS::Parser::Data::is_hms_valid(void) const
{
	return m_hour >= 0 && m_hour <= 23 &&
		m_minute >= 0 && m_minute <= 59 &&
		m_seconds >= 0 && m_seconds < 60.0;
}

void Sensors::SensorKingGPS::Parser::Data::finalize(void)
{
	{
		std::string::const_iterator si(m_wpttypes.begin()), se(m_wpttypes.end());
		for (gpsflightplan_t::iterator wi(m_fplan.begin()), we(m_fplan.end()); wi != we && si != se; ++wi, ++si) {
			if (wi->get_type() != GPSFlightPlanWaypoint::type_undefined)
				continue;
			GPSFlightPlanWaypoint::type_t t(GPSFlightPlanWaypoint::type_undefined);
			switch (*si) {
			case 'a':
			case 'A':
				t = GPSFlightPlanWaypoint::type_airport;
				break;

			case 'm':
			case 'M':
				t = GPSFlightPlanWaypoint::type_military;
				break;

			case 'h':
			case 'H':
				t = GPSFlightPlanWaypoint::type_heliport;
				break;

			case 's':
			case 'S':
				t = GPSFlightPlanWaypoint::type_seaplane;
				break;

			case 'v':
			case 'V':
				t = GPSFlightPlanWaypoint::type_vor;
				break;

			case 'n':
			case 'N':
				t = GPSFlightPlanWaypoint::type_ndb;
				break;

			case 'i':
			case 'I':
				t = GPSFlightPlanWaypoint::type_intersection;
				break;

			case 'u':
			case 'U':
				t = GPSFlightPlanWaypoint::type_userdefined;
				break;

			default:
				break;
			}
			if (t == GPSFlightPlanWaypoint::type_undefined) {
				std::cerr << "King GPS: Parse: invalid waypoint type " << *si << std::endl;
				continue;
			}
			wi->set_type(t);
		}
	}
	for (gpsflightplan_t::iterator wi(m_fplan.begin()), we(m_fplan.end()); wi != we;) {
		GPSFlightPlanWaypoint::type_t t(wi->get_type());
		if (t != GPSFlightPlanWaypoint::type_invalid &&
		    (t != GPSFlightPlanWaypoint::type_undefined || m_wpttypes.empty())) {
			++wi;
			continue;
		}
		std::cerr << "King GPS: Parse: dropping invalid waypoint " << wi->get_ident() << std::endl;
		wi = m_fplan.erase(wi);
		we = m_fplan.end();
	}
	if (true) {
		for (gpsflightplan_t::iterator wi(m_fplan.begin()), we(m_fplan.end()); wi != we; ++wi)
			std::cerr << "FPlan: " << wi->get_coord().get_lat_str2() << ' '
				  << wi->get_coord().get_lon_str2() << ' ' << wi->get_ident() << std::endl;
	}
	m_time = Glib::TimeVal(-1, 0);
	if (is_dmy_valid() && is_hms_valid()) {
		Glib::DateTime dt(Glib::DateTime::create_utc(m_year, m_month, m_day, m_hour, m_minute, m_seconds));
		Glib::TimeVal tv;
		if (dt.to_timeval(tv))
			m_time = tv;
	}
	if (!is_position_ok())
		m_pos = Point();
}

const char Sensors::SensorKingGPS::Parser::stx;
const char Sensors::SensorKingGPS::Parser::etx;
const char Sensors::SensorKingGPS::Parser::cr;
const char Sensors::SensorKingGPS::Parser::lf;

const struct Sensors::SensorKingGPS::Parser::parser_table Sensors::SensorKingGPS::Parser::parser_table[] = {
	{ 'A', 1, &Parser::parse_lat },
	{ 'B', 1, &Parser::parse_lon },
	{ 'C', 1, &Parser::parse_tk },
	{ 'D', 1, &Parser::parse_gs },
	{ 'E', 1, &Parser::parse_dis },
	{ 'F', 1, &Parser::parse_ete },
	{ 'G', 1, &Parser::parse_xtk },
	{ 'H', 1, &Parser::parse_tke },
	{ 'I', 1, &Parser::parse_dtk },
	{ 'J', 1, &Parser::parse_leg },
	{ 'K', 1, &Parser::parse_dest },
	{ 'L', 1, &Parser::parse_brg },
	{ 'M', 1, &Parser::parse_ptk },
	{ 'P', 1, &Parser::parse_poserr },
	{ 'Q', 1, &Parser::parse_magvar },
	{ 'c', 1, &Parser::parse_timesincesol },
	{ 'S', 1, &Parser::parse_navflags },
	{ 'T', 1, &Parser::parse_flags },
	{ 'd', 1, &Parser::parse_msa },
	{ 'e', 1, &Parser::parse_mesa },
	{ 'i', 1, &Parser::parse_date },
	{ 'j', 1, &Parser::parse_time },
	{ 's', 1, &Parser::parse_swcode },
	{ 'w', 18, &Parser::parse_wpt },
	{ 't', 1, &Parser::parse_wpttype },
	{ 'k', 1, &Parser::parse_latlongs },
	{ 'l', 1, &Parser::parse_alt },
	{ 'm', 1, &Parser::parse_trkerr },
	{ 'n', 1, &Parser::parse_vnav },
	{ 'o', 1, &Parser::parse_distbrgtime },
	{ 'p', 1, &Parser::parse_loctime },
	{ 'q', 1, &Parser::parse_poffs },
	{ 'r', 1, &Parser::parse_exttime },
	{ 'u', 1, &Parser::parse_exttimesincesol },
	{ 'z', 1, &Parser::parse_sensstat },
	{ 0, 1, 0 }
};

Sensors::SensorKingGPS::Parser::Parser(void)
	: m_waitstx(true)
{
}

bool Sensors::SensorKingGPS::Parser::parse(void)
{
	for (;;) {
		if (m_waitstx) {
			std::string::iterator i(m_raw.begin()), e(m_raw.end());
			for (; i != e; ++i) {
				if (*i != stx)
					continue;
				++i;
				m_waitstx = false;
				m_data = Data();
				break;
			}
			m_raw.erase(m_raw.begin(), i);
			if (m_waitstx)
				return false;
		}
		if (m_raw.empty())
			return false;
		std::string::iterator i(m_raw.begin()), e(m_raw.end());
		while (i != e && (*i == cr || *i == lf || std::isspace(*i)))
			++i;
		if (i != m_raw.begin()) {
			m_raw.erase(m_raw.begin(), i);
			i = m_raw.begin();
			e = m_raw.end();
		}
		if (i == e)
			return false;
		if (*i == etx) {
			++i;
			m_raw.erase(m_raw.begin(), i);
			m_waitstx = true;
			m_data.finalize();
			return true;
		}
		const struct parser_table *ptbl = parser_table;
		while (ptbl->parsefunc && ptbl->item != *i)
			++ptbl;
		if (m_raw.size() < ptbl->minlength)
			return false;
		i += ptbl->minlength;
		while (i != e && *i != cr && *i != lf)
			++i;
		if (i == e) {
			if (m_raw.size() < 256)
				return false;
			std::cerr << "King GPS: Parse: line length >= 256 bytes, resynchronizing" << std::endl;
			m_raw.clear();
			m_waitstx = true;
			return false;
		}
		if (!ptbl->parsefunc)
			std::cerr << "King GPS: Unknown Sentence: " << std::string(m_raw.begin(), i) << std::endl;
		else if (!(this->*(ptbl->parsefunc))(m_raw.begin() + 1, i))
			std::cerr << "King GPS: Parse Error: " << std::string(m_raw.begin(), i) << std::endl;
		while (i != e && (*i == cr || *i == lf || std::isspace(*i)))
			++i;
		m_raw.erase(m_raw.begin(), i);
	}
}

std::string::const_iterator Sensors::SensorKingGPS::Parser::skipws(std::string::const_iterator si, std::string::const_iterator se)
{
	while (si != se && std::isspace(*si))
		++si;
	return si;
}

bool Sensors::SensorKingGPS::Parser::parsenum(double& v, std::string::const_iterator& si, std::string::const_iterator se, char sgnp, char sgnn)
{
	v = std::numeric_limits<double>::quiet_NaN();
	si = skipws(si, se);
	if (si == se)
		return false;
	if (*si == '-') {
		std::string::const_iterator si2(si);
		++si2;
		unsigned int dash(1);
		while (si2 != se && *si2 == '-') {
			++si2;
			++dash;
		}
		if (dash >= 2) {
			si = si2;
			return true;
		}
	}
	double sgn(1.0);
	if (sgnp && (*si == sgnp || (sgnp >= 'A' && sgnp <= 'Z' && *si == (sgnp + ('a' - 'A'))))) {
		++si;
		si = skipws(si, se);
		if (si == se)
			return false;
	} else if (sgnn && (*si == sgnn || (sgnn >= 'A' && sgnn <= 'Z' && *si == (sgnn + ('a' - 'A'))))) {
		++si;
		si = skipws(si, se);
		if (si == se)
			return false;
		sgn = -1.0;
	}
	std::string::const_iterator si2(si);
	bool haspt(false);
	while (si2 != se) {
		if (std::isdigit(*si2)) {
			++si2;
			continue;
		}
		if (*si2 == '.') {
			if (haspt)
				break;
			haspt = true;
			++si2;
			continue;
		}
		break;
	}
	if (si2 == si)
		return false;
	v = sgn * Glib::Ascii::strtod(Glib::ustring(si, si2));
	si = skipws(si2, se);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parsenum(int& v, std::string::const_iterator& si, std::string::const_iterator se, char sgnp, char sgnn)
{
	v = 0;
	si = skipws(si, se);
	if (si == se)
		return false;
	int sgn(1);
	if (sgnp && (*si == sgnp || (sgnp >= 'A' && sgnp <= 'Z' && *si == (sgnp + ('a' - 'A'))))) {
		++si;
		si = skipws(si, se);
		if (si == se)
			return false;
	} else if (sgnn && (*si == sgnn || (sgnn >= 'A' && sgnn <= 'Z' && *si == (sgnn + ('a' - 'A'))))) {
		++si;
		si = skipws(si, se);
		if (si == se)
			return false;
		sgn = -1;
	}
	std::string::const_iterator si2(si);
	int val(0);
	while (si2 != se) {
		if (std::isdigit(*si2)) {
			val *= 10;
			val += *si2 - '0';
			++si2;
			continue;
		}
		break;
	}
	if (si2 == si)
		return false;
	v = sgn * val;
	si = skipws(si2, se);
	return true;
}

bool Sensors::SensorKingGPS::Parser::expect(char ch, std::string::const_iterator& si, std::string::const_iterator se)
{
	si = skipws(si, se);
	if (si == se)
		return false;
	if (*si != ch)
		return false;
	++si;
	si = skipws(si, se);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parsestr(std::string& v, std::string::const_iterator& si, std::string::const_iterator se)
{
	v.clear();
	si = skipws(si, se);
	std::string::const_iterator si2(si);
	while (si2 != se && !std::isspace(*si2))
		++si2;
	if (si2 == si)
		return false;
	v = std::string(si, si2);
	si = skipws(si2, se);
	return true;
}

void Sensors::SensorKingGPS::Parser::set_garmin_fix(void)
{
	if (m_data.get_navflags().size() != 5 || m_data.get_navflags()[4] != '-')
		m_data.set_fixtype(fixtype_nofix);
	else if (std::isnan(m_data.get_gpsalt()))
		m_data.set_fixtype(fixtype_2d);
	else
		m_data.set_fixtype(fixtype_3d);
}

bool Sensors::SensorKingGPS::Parser::parse_lat(std::string::const_iterator si, std::string::const_iterator se)
{
	int deg(0), min(0);
	if (!parsenum(deg, si, se, 'N', 'S') ||
	    !parsenum(min, si, se))
		return false;
	if (si != se)
		return false;
	if (m_data.is_lat_set())
		return true;
	double x(deg);
	if (deg < 0)
		x -= min * (1.0 / 60.0 / 100.0);
	else
		x += min * (1.0 / 60.0 / 100.0);	
	m_data.set_lat(x);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_lon(std::string::const_iterator si, std::string::const_iterator se)
{
	int deg(0), min(0);
	if (!parsenum(deg, si, se, 'E', 'W') ||
	    !parsenum(min, si, se))
		return false;
	if (si != se)
		return false;
	if (m_data.is_lon_set())
		return true;
	double x(deg);
	if (deg < 0)
		x -= min * (1.0 / 60.0 / 100.0);
	else
		x += min * (1.0 / 60.0 / 100.0);	
	m_data.set_lon(x);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_tk(std::string::const_iterator si, std::string::const_iterator se)
{
	double x(std::numeric_limits<double>::quiet_NaN());
	if (!parsenum(x, si, se))
		return false;
	if (si != se)
		return false;
	if (!std::isnan(m_data.get_magtrack()))
		return true;
	m_data.set_magtrack(x);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_gs(std::string::const_iterator si, std::string::const_iterator se)
{
	double x(std::numeric_limits<double>::quiet_NaN());
	if (!parsenum(x, si, se))
		return false;
	if (si != se)
		return false;
	if (!std::isnan(m_data.get_gs()))
		return true;
	m_data.set_gs(x);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_dis(std::string::const_iterator si, std::string::const_iterator se)
{
	double x(std::numeric_limits<double>::quiet_NaN());
	if (!parsenum(x, si, se))
		return false;
	if (si != se)
		return false;
	if (!std::isnan(m_data.get_dist()))
		return true;
	m_data.set_dist(x * 0.01);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_ete(std::string::const_iterator si, std::string::const_iterator se)
{
	int x(0);
	if (!parsenum(x, si, se))
		return false;
	if (si != se)
		return false;
	if (m_data.get_timewpt())
		return true;
	x = (x / 100) * 60 + (x % 100);
	m_data.set_timewpt(x * 60);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_xtk(std::string::const_iterator si, std::string::const_iterator se)
{
	double x(std::numeric_limits<double>::quiet_NaN());
	if (!parsenum(x, si, se, 'R', 'L'))
		return false;
	if (si != se)
		return false;
	if (!std::isnan(m_data.get_xtk()))
		return true;
	m_data.set_xtk(x * 0.01);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_tke(std::string::const_iterator si, std::string::const_iterator se)
{
	int x(0);
	if (!parsenum(x, si, se, 'R', 'L'))
		return false;
	if (si != se)
		return false;
	if (!std::isnan(m_data.get_tke()))
		return true;
	m_data.set_tke(x * 0.1);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_dtk(std::string::const_iterator si, std::string::const_iterator se)
{
	double x(std::numeric_limits<double>::quiet_NaN());
	if (!parsenum(x, si, se))
		return false;
	if (si != se)
		return false;
	if (!std::isnan(m_data.get_dtk()))
		return true;
	m_data.set_dtk(x * 0.1);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_leg(std::string::const_iterator si, std::string::const_iterator se)
{
	int x(0);
	if (!parsenum(x, si, se))
		return false;
	if (si != se)
		return false;
	m_data.set_leg(x);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_dest(std::string::const_iterator si, std::string::const_iterator se)
{
	std::string x;
	if (!parsestr(x, si, se))
		return false;
	if (si != se)
		return false;
	m_data.set_dest(x);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_brg(std::string::const_iterator si, std::string::const_iterator se)
{
	double x(std::numeric_limits<double>::quiet_NaN());
	if (!parsenum(x, si, se))
		return false;
	if (si != se)
		return false;
	if (!std::isnan(m_data.get_brg()))
		return true;
	m_data.set_brg(x * 0.1);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_ptk(std::string::const_iterator si, std::string::const_iterator se)
{
	double x(std::numeric_limits<double>::quiet_NaN());
	if (!parsenum(x, si, se, 'R', 'L'))
		return false;
	if (si != se)
		return false;
	if (!std::isnan(m_data.get_ptk()))
		return true;
	m_data.set_ptk(x * 0.1);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_poserr(std::string::const_iterator si, std::string::const_iterator se)
{
	int x(0);
	if (!parsenum(x, si, se))
		return false;
	if (si != se)
		return false;
	m_data.set_poserr(x * 0.1);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_magvar(std::string::const_iterator si, std::string::const_iterator se)
{
	int x(0);
	if (!parsenum(x, si, se, 'E', 'W'))
		return false;
	if (si != se)
		return false;
	m_data.set_magvar(x * 0.1);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_timesincesol(std::string::const_iterator si, std::string::const_iterator se)
{
	int x(0);
	if (!parsenum(x, si, se))
		return false;
	if (si != se)
		return false;
	if (!std::isnan(m_data.get_timesincesolution()))
		return true;
	m_data.set_timesincesolution(x * 0.1);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_navflags(std::string::const_iterator si, std::string::const_iterator se)
{
	std::string x;
	if (!parsestr(x, si, se))
		return false;
	if (si != se)
		return false;
	m_data.set_navflags(x);
	set_garmin_fix();
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_flags(std::string::const_iterator si, std::string::const_iterator se)
{
	std::string x;
	if (!parsestr(x, si, se))
		return false;
	if (si != se)
		return false;
	m_data.set_flags(x);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_msa(std::string::const_iterator si, std::string::const_iterator se)
{
	double x(std::numeric_limits<double>::quiet_NaN());
	if (!parsenum(x, si, se))
		return false;
	if (si != se)
		return false;
	m_data.set_msa(x * 100.0);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_mesa(std::string::const_iterator si, std::string::const_iterator se)
{
	double x(std::numeric_limits<double>::quiet_NaN());
	if (!parsenum(x, si, se))
		return false;
	if (si != se)
		return false;
	m_data.set_mesa(x * 100.0);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_date(std::string::const_iterator si, std::string::const_iterator se)
{
	int day(0), mon(0), year(0);
	if (!parsenum(mon, si, se) ||
	    !expect('/', si, se) ||
	    !parsenum(day, si, se) ||
	    !expect('/', si, se) ||
	    !parsenum(year, si, se))
		return false;
	if (si != se)
		return false;
	m_data.set_dmy(day, mon, year);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_time(std::string::const_iterator si, std::string::const_iterator se)
{
	int hour(0), min(0), sec(0);
	if (!parsenum(hour, si, se) ||
	    !expect(':', si, se) ||
	    !parsenum(min, si, se) ||
	    !expect(':', si, se) ||
	    !parsenum(sec, si, se))
		return false;
	if (si != se)
		return false;
	if (m_data.is_hms_valid())
		return true;
	m_data.set_hms(hour, min, sec);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_swcode(std::string::const_iterator si, std::string::const_iterator se)
{
	std::string x;
	if (!parsestr(x, si, se))
		return false;
	if (si != se)
		return false;
	m_data.set_swcode(x);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_wpt(std::string::const_iterator si, std::string::const_iterator se)
{
	if ((se - si) < 17)
		return false;
	if (!std::isdigit(*si))
		return false;
	unsigned int wptnum(*si - '0');
	++si;
	if (!std::isdigit(*si))
		return false;
	wptnum *= 10;
	wptnum += *si - '0';
	++si;
	if (!wptnum || wptnum > 31)
		return false;
	if (m_data.get_fplan().size() < wptnum)
		m_data.get_fplan().resize(wptnum);
	--wptnum;
	m_data.get_fplan()[wptnum] = GPSFlightPlanWaypoint((const uint8_t *)&*si);
	if ((*(const uint8_t *)&*si) & 0x20 && m_data.get_leg() == ~0U)
		m_data.set_leg(wptnum);
	si += 15;
	si = skipws(si, se);
	if (si != se)
		return false;
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_wpttype(std::string::const_iterator si, std::string::const_iterator se)
{
	std::string x;
	if (!parsestr(x, si, se))
		return false;
	if (si != se)
		return false;
	m_data.set_wpttypes(x);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_latlongs(std::string::const_iterator si, std::string::const_iterator se)
{
	int latdeg(0), londeg(0);
	double lat(0), lon(0), gs(0);
	if (!parsenum(latdeg, si, se, 'N', 'S') ||
	    !parsenum(lat, si, se) ||
	    !parsenum(londeg, si, se, 'E', 'W') ||
	    !parsenum(lon, si, se) ||
	    !parsenum(gs, si, se))
		return false;
	lat *= (1.0 / 60.0);
	lon *= (1.0 / 60.0);
	if (latdeg < 0)
		lat = latdeg - lat;
	else
		lat = latdeg + lat;
	if (londeg < 0)
		lon = londeg - lon;
	else
		lon = londeg + lon;
	m_data.set_lat(lat);
	m_data.set_lon(lon);
	m_data.set_gs(gs);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_alt(std::string::const_iterator si, std::string::const_iterator se)
{
	double gpsalt(0), pressalt(0), baroalt(0);
	if (!parsenum(gpsalt, si, se))
		return false;
	if (!parsenum(pressalt, si, se)) {
		m_data.set_destdist(gpsalt * 0.1);
		return true;
	}
	if (!parsenum(baroalt, si, se))
		return false;
	m_data.set_gpsalt(gpsalt);
	m_data.set_pressalt(pressalt);
	m_data.set_baroalt(baroalt);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_trkerr(std::string::const_iterator si, std::string::const_iterator se)
{
	double mtk(0), dtk(0), xtk(0), tke(0);
	if (!parsenum(mtk, si, se) ||
	    !parsenum(dtk, si, se) ||
	    !parsenum(xtk, si, se, 'R', 'L') ||
	    !parsenum(tke, si, se, 'R', 'L'))
		return false;
	m_data.set_magtrack(mtk);
	m_data.set_dtk(dtk);
	m_data.set_xtk(xtk);
	m_data.set_tke(tke);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_vnav(std::string::const_iterator si, std::string::const_iterator se)
{
	double av(0), dv(0), ve(0), vae(0);
	if (!parsenum(av, si, se, '+', '-') ||
	    !parsenum(dv, si, se, '+', '-') ||
	    !parsenum(ve, si, se, '+', '-') ||
	    !parsenum(vae, si, se, '+', '-'))
		return false;
	m_data.set_actualvnav(av);
	m_data.set_desiredvnav(dv);
	m_data.set_verterr(ve);
	m_data.set_vertangleerr(vae);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_distbrgtime(std::string::const_iterator si, std::string::const_iterator se)
{
	double brg(0), dist(0);
	int hour(0), min(0), sec(0);
	if (!parsenum(brg, si, se) ||
	    !parsenum(dist, si, se) ||
	    !parsenum(hour, si, se) ||
	    !expect(':', si, se) ||
	    !parsenum(min, si, se) ||
	    !expect(':', si, se) ||
	    !parsenum(sec, si, se))
		return false;
	m_data.set_brg(brg);
	m_data.set_dist(dist);
	m_data.set_timewpt(hour * (60 * 60) + min * 60 + sec);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_loctime(std::string::const_iterator si, std::string::const_iterator se)
{
	double tzoffs(0);
	int hour(0), min(0), sec(0);
	if (!parsenum(hour, si, se) ||
	    !expect(':', si, se) ||
	    !parsenum(min, si, se) ||
	    !expect(':', si, se) ||
	    !parsenum(sec, si, se) ||
	    !parsenum(tzoffs, si, se, '+', '-'))
		return false;
	m_data.set_utcoffset(tzoffs * (60 * 60));
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_poffs(std::string::const_iterator si, std::string::const_iterator se)
{
	double x(0);
	if (!parsenum(x, si, se, 'R', 'L'))
		return false;
	if (si != se)
		return false;
	m_data.set_ptk(x);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_exttime(std::string::const_iterator si, std::string::const_iterator se)
{
	int hour(0), min(0);
	double sec(0);
	if (!parsenum(hour, si, se) ||
	    !expect(':', si, se) ||
	    !parsenum(min, si, se) ||
	    !expect(':', si, se) ||
	    !parsenum(sec, si, se))
		return false;
	if (si != se)
		return false;
	m_data.set_hms(hour, min, sec);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_exttimesincesol(std::string::const_iterator si, std::string::const_iterator se)
{
	double x(0);
	if (!parsenum(x, si, se))
		return false;
	if (si != se)
		return false;
	m_data.set_timesincesolution(x);
	return true;
}

bool Sensors::SensorKingGPS::Parser::parse_sensstat(std::string::const_iterator si, std::string::const_iterator se)
{
	si = skipws(si, se);
	if (si == se)
		return false;
	char inuse(*si);
	switch (inuse) {
	case 'D':
	case 'd':
		m_data.set_fixtype(fixtype_3d_diff);
		++si;
		break;

	case 'G':
	case 'g':
		m_data.set_fixtype(fixtype_3d);
		++si;
		break;

	case 'L':
	case 'l':
		m_data.set_fixtype(fixtype_2d);
		++si;
		break;

	case '-':
		{
			std::string::const_iterator si1(si);
			while (si1 != se && *si1 == '-')
				++si1;
			if (si1 == se && si1 - si >= 5) {
				set_garmin_fix();
				return true;
			}
		}
		m_data.set_fixtype(fixtype_nofix);
		++si;
		break;

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	{
		double gpsalt(0);
		if (!parsenum(gpsalt, si, se))
			return false;
		m_data.set_gpsalt(gpsalt);
		set_garmin_fix();
		return true;
	}

	default:
		return false;
	}
	si = skipws(si, se);
	if (si == se)
		return false;
	if (*si == '.') {
		++si;
		si = skipws(si, se);
		if (si == se)
			return false;
	}
	std::string x;
	while (parsestr(x, si, se)) {
		if (!x.compare(0, 3, "GPS")) {
			std::string::const_iterator si2(x.begin() + 3), se2(x.end());
			if (!expect('-', si2, se2))
				return false;
			if (si2 == se2)
				return false;
			switch (*si2) {
			case '0':
				m_data.set_fixtype(fixtype_nofix);
				break;

			case '2':
				m_data.set_fixtype(fixtype_2d);
				break;

			case '3':
				break;

			default:
				return false;
			}
			++si2;
			if (!expect('D', si2, se2))
				return false;
			if (si2 != se2 && *si2 == ':') {
				++si2;
				si2 = skipws(si2, se2);
			}
			int nrsat(0);
			if (!parsenum(nrsat, si2, se2))
				return false;
			if (si2 != se2)
				return false;
			m_data.set_satellites(nrsat);
			continue;
		}
		if (!x.compare(0, 3, "LOR")) {
			std::string::const_iterator si2(x.begin() + 3), se2(x.end());
			si2 = skipws(si2, se2);
			if (si2 != se2 && *si2 == ':') {
				++si2;
				si2 = skipws(si2, se2);
			}
			int nrtx(0);
			if (!parsenum(nrtx, si2, se2))
				return false;
			if (si2 != se2)
				return false;
			continue;
		}
		return false;
	}
	if (si != se)
		return false;
	return true;
}

Sensors::SensorKingGPS::SensorKingGPS(Sensors& sensors, const Glib::ustring& configgroup)
	: SensorGPS(sensors, configgroup), m_gpsaltrate(std::numeric_limits<double>::quiet_NaN()),
	  m_baroaltrate(std::numeric_limits<double>::quiet_NaN())
{
        const Glib::KeyFile& cf(get_sensors().get_configfile());
	if (!cf.has_key(get_configgroup(), "baroaltitudepriority"))
		get_sensors().get_configfile().set_integer_list(get_configgroup(), "baroaltitudepriority", std::vector<int>(m_altitudepriority, m_altitudepriority + sizeof(m_altitudepriority)/sizeof(m_altitudepriority[0])));
	{
		Glib::ArrayHandle<int> a(cf.get_integer_list(get_configgroup(), "baroaltitudepriority"));
		unsigned int nr(std::min(a.size(), sizeof(m_baroaltitudepriority)/sizeof(m_baroaltitudepriority[0])));
		for (unsigned int i = 0; i < nr; ++i)
			m_baroaltitudepriority[i] = a.data()[i];
		for (; nr < sizeof(m_baroaltitudepriority)/sizeof(m_baroaltitudepriority[0]); ++nr)
				m_baroaltitudepriority[nr] = nr ? m_baroaltitudepriority[nr-1] : 0;
	}
        if (cf.has_key(get_configgroup(), "tracefile")) {
		std::string fn(cf.get_string(get_configgroup(), "tracefile"));
		m_tracefile.open(fn.c_str(), std::ofstream::out | std::ofstream::app | std::ofstream::binary);
		if (!m_tracefile.is_open())
			get_sensors().log(Sensors::loglevel_warning, std::string("king gps: cannot open trace file ") + fn);
	}
}

Sensors::SensorKingGPS::~SensorKingGPS()
{
}

bool Sensors::SensorKingGPS::parse(const std::string& raw)
{
	if (m_tracefile.is_open())
		m_tracefile << raw;
	return m_parser.parse(raw);
}

template <typename T>
bool Sensors::SensorKingGPS::parse(T b, T e)
{
	if (m_tracefile.is_open())
		m_tracefile << std::string(b, e);
	return m_parser.parse(b, e);
}

template bool Sensors::SensorKingGPS::parse<const char *>(const char *b, const char *e);
template bool Sensors::SensorKingGPS::parse<char *>(char *b, char *e);

bool Sensors::SensorKingGPS::parse(void)
{
	return m_parser.parse();
}

void Sensors::SensorKingGPS::update_gps(ParamChanged& pc)
{
	const Parser::Data& newdata(m_parser);
	Glib::TimeVal tvdiff(m_time);
	m_time.assign_current_time();
	if (tvdiff.valid() && m_time.valid())
		tvdiff = m_time - tvdiff;
	else
		tvdiff = Glib::TimeVal(-1, 0);
	double newgpsaltrate(std::numeric_limits<double>::quiet_NaN());
	double newbaroaltrate(std::numeric_limits<double>::quiet_NaN());
	{
		double mul(-1);
		if (m_data.get_time().valid() && m_data.get_time().tv_sec >= 0 &&
		    newdata.get_time().valid() && newdata.get_time().tv_sec >= 0) {
			double mul = (newdata.get_time() - m_data.get_time()).as_double();
			if (!std::isnan(m_data.get_timesincesolution()) && !std::isnan(newdata.get_timesincesolution()))
				mul += m_data.get_timesincesolution() - newdata.get_timesincesolution();
		} else if (tvdiff.valid() && tvdiff.tv_sec >= 0) {
			mul = tvdiff.as_double();
			if (mul < 0.5 || mul > 1.5)
				mul = -1;
		}
		if (!std::isnan(mul) && mul > 0) {
			mul = 60.0 / mul;
			newgpsaltrate = (newdata.get_gpsalt() - m_data.get_gpsalt()) * mul;
			newbaroaltrate = (newdata.get_baroalt() - m_data.get_baroalt()) * mul;
		}
	}
	if (false)
		std::cerr << "King GPS: update: fixmode " << to_str(newdata.get_fixtype()) << std::endl;
	pc.set_changed(parnrtime);
	if (m_data.get_fixtype() != newdata.get_fixtype())
		pc.set_changed(parnrfixtype);
	if (m_data.get_position().get_lat() != newdata.get_position().get_lat() ||
	    m_data.is_position_ok() != newdata.is_position_ok())
		pc.set_changed(parnrlat);
	if (m_data.get_position().get_lon() != newdata.get_position().get_lon() ||
	    m_data.is_position_ok() != newdata.is_position_ok())
		pc.set_changed(parnrlon);
	if (m_data.get_gpsalt() != newdata.get_gpsalt() ||
	    m_gpsaltrate != newgpsaltrate) {
		pc.set_changed(parnralt);
		pc.set_changed(parnraltrate);
	}
	if (m_data.get_magtrack() != newdata.get_magtrack())
		pc.set_changed(parnrcourse);
	if (m_data.get_gs() != newdata.get_gs())
		pc.set_changed(parnrgroundspeed);
	if (m_data.get_baroalt() != newdata.get_baroalt())
		pc.set_changed(parnrbaroalt);
	if (m_baroaltrate != newbaroaltrate)
		pc.set_changed(parnrbaroaltrate);
	if (m_data.get_pressalt() != newdata.get_pressalt())
		pc.set_changed(parnrpressalt);
	if (m_data.get_dtk() != newdata.get_dtk())
		pc.set_changed(parnrdtk);
	if (m_data.get_xtk() != newdata.get_xtk())
		pc.set_changed(parnrxtk);
	if (m_data.get_tke() != newdata.get_tke())
		pc.set_changed(parnrtke);
	if (m_data.get_brg() != newdata.get_brg())
		pc.set_changed(parnrbrg);
	if (m_data.get_dist() != newdata.get_dist())
		pc.set_changed(parnrdist);
	if (m_data.get_ptk() != newdata.get_ptk())
		pc.set_changed(parnrptk);
	if (m_data.get_magvar() != newdata.get_magvar())
		pc.set_changed(parnrmagvar);
	if (m_data.get_msa() != newdata.get_msa())
		pc.set_changed(parnrmsa);
	if (m_data.get_mesa() != newdata.get_mesa())
		pc.set_changed(parnrmesa);
	if (m_data.get_timewpt() != newdata.get_timewpt())
		pc.set_changed(parnrtimewpt);
	if (m_data.get_destination() != newdata.get_destination())
		pc.set_changed(parnrdest);
	if (m_data.get_actualvnav() != newdata.get_actualvnav())
		pc.set_changed(parnractualvnav);
	if (m_data.get_desiredvnav() != newdata.get_desiredvnav())
		pc.set_changed(parnrdesiredvnav);
	if (m_data.get_verterr() != newdata.get_verterr())
		pc.set_changed(parnrverterr);
	if (m_data.get_vertangleerr() != newdata.get_vertangleerr())
		pc.set_changed(parnrvertangleerr);
	if (m_data.get_satellites() != newdata.get_satellites())
		pc.set_changed(parnrsatellites);
	if (m_data.get_poserr() != newdata.get_poserr())
		pc.set_changed(parnrposerr);
	if (m_data.get_timesincesolution() != newdata.get_timesincesolution())
		pc.set_changed(parnrsolutiontime);
	if (m_data.get_utcoffset() != newdata.get_utcoffset())
		pc.set_changed(parnrutcoffset);
	if (m_data.get_time() != newdata.get_time())
		pc.set_changed(parnrgpstime);
	if (m_data.get_leg() != newdata.get_leg())
		pc.set_changed(parnrleg);
	if (m_data.get_fplan().size() != newdata.get_fplan().size())
		pc.set_changed(parnrfplwpt);
	if (m_data.get_navflags() != newdata.get_navflags())
		pc.set_changed(parnrnavflags);
	if (m_data.get_flags() != newdata.get_flags())
		pc.set_changed(parnrflags);
	if (m_data.get_swcode() != newdata.get_swcode())
		pc.set_changed(parnrswcode);
	m_data = newdata;
	m_gpsaltrate = newgpsaltrate;
	m_baroaltrate = newbaroaltrate;
	m_fixtype = m_data.get_fixtype();
}

void Sensors::SensorKingGPS::update_gps(void)
{
	ParamChanged pc;
	update_gps(pc);
	update(pc);
}

void Sensors::SensorKingGPS::invalidate_gps(void)
{
	m_fixtype = fixtype_nofix;
	m_data = Parser::Data();
	m_gpsaltrate = std::numeric_limits<double>::quiet_NaN();
	m_baroaltrate = std::numeric_limits<double>::quiet_NaN();
	m_time = Glib::TimeVal();
}

void Sensors::SensorKingGPS::invalidate_update_gps(void)
{
	invalidate_gps();
	ParamChanged pc;
	pc.set_changed(parnrfixtype, parnrgroundspeed);
	pc.set_changed(parnrbaroalt, parnrswcode);
	if (false) {
		std::cerr << "King: fixtype " << to_str(m_fixtype) << std::endl;
		for (unsigned int nr = 0; nr < 128; ++nr)
			if (pc.is_changed(nr))
				std::cerr << "King: invalidate " << nr << std::endl;
	}
	update(pc);
}

bool Sensors::SensorKingGPS::get_position(Point& pt) const
{
	if (m_fixtype < fixtype_2d) {
		pt = Point();
		return false;
	}
	pt = m_data.get_position();
	return true;
}

Glib::TimeVal Sensors::SensorKingGPS::get_position_time(void) const
{
	return m_time;
}

void Sensors::SensorKingGPS::get_truealt(double& alt, double& altrate) const
{
	alt = altrate = std::numeric_limits<double>::quiet_NaN();
	if (false)
		std::cerr << "get_truealt: fixtype " << to_str(m_fixtype) << " alt " << m_data.get_gpsalt()
			  << " altrate " << m_gpsaltrate << std::endl;
	if (m_fixtype < fixtype_3d)
		return;
	alt = m_data.get_gpsalt();
	altrate = m_gpsaltrate;
}

Glib::TimeVal Sensors::SensorKingGPS::get_truealt_time(void) const
{
	return m_time;
}

unsigned int Sensors::SensorKingGPS::get_baroalt_priority(void) const
{
	return m_baroaltitudepriority[m_fixtype];
}

void Sensors::SensorKingGPS::get_baroalt(double& alt, double& altrate) const
{
	alt = m_data.get_baroalt();
	altrate = m_baroaltrate;
}

Glib::TimeVal Sensors::SensorKingGPS::get_baroalt_time(void) const
{
	return m_time;
}

void Sensors::SensorKingGPS::get_course(double& crs, double& gs) const
{
	crs = gs = std::numeric_limits<double>::quiet_NaN();
	if (m_fixtype < fixtype_2d)
		return;
	crs = m_data.get_magtrack();
	gs = m_data.get_gs();
}

Glib::TimeVal Sensors::SensorKingGPS::get_course_time(void) const
{
	return m_time;
}

void Sensors::SensorKingGPS::get_param_desc(unsigned int pagenr, paramdesc_t& pd)
{
	SensorGPS::get_param_desc(pagenr, pd);
	{
		paramdesc_t::iterator pdi(pd.begin()), pde(pd.end());
		if (pdi != pde)
			++pdi;
		for (; pdi != pde; ++pdi)
			if (pdi->get_type() == ParamDesc::type_section && pdi->get_label() == "Priorities")
				break;
		if (pdi != pde)
			++pdi;
		for (; pdi != pde; ++pdi)
			if (pdi->get_type() == ParamDesc::type_section)
				break;
		pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrbaroaltprio0, "Baro Prio No Fix", "Barometric Altitude Priority when GPS has no fix; higher values mean this sensor is preferred to other sensors delivering altitude information", "", 0, 0, 9999, 1, 10));
		++pdi;
		pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrbaroaltprio1, "Baro Prio 2D", "Barometric Altitude Priority when GPS has 2D fix; higher values mean this sensor is preferred to other sensors delivering altitude information", "", 0, 0, 9999, 1, 10));
		++pdi;
		pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrbaroaltprio2, "Baro Prio 3D", "Barometric Altitude Priority when GPS has 3D fix; higher values mean this sensor is preferred to other sensors delivering altitude information", "", 0, 0, 9999, 1, 10));
		++pdi;
		pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrbaroaltprio3, "Baro Prio 3D Diff", "Barometric Altitude Priority when GPS has 3D with Differential Information fix; higher values mean this sensor is preferred to other sensors delivering altitude information", "", 0, 0, 9999, 1, 10));
	}

	{
		paramdesc_t::iterator pdi(pd.begin()), pde(pd.end());
		if (pdi != pde)
			++pdi;
		for (; pdi != pde; ++pdi)
			if (pdi->get_type() == ParamDesc::type_section && pdi->get_label() == "Fix")
				break;
		if (pdi != pde)
			++pdi;
		for (; pdi != pde; ++pdi)
			if (pdi->get_type() == ParamDesc::type_section)
				break;
		pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrbaroalt, "Baro Alt", "Barometric Altitude", "ft", 0, -99999.0, 99999.0, 1.0, 10.0));
		++pdi;
		pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrbaroaltrate, "Baro Climb", "Barometric Climb Rate", "ft/min", 0, -9999.0, 9999.0, 1.0, 10.0));
		++pdi;
		pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrpressalt, "Press Alt", "Pressure Altitude", "ft", 0, -99999.0, 99999.0, 1.0, 10.0));
		++pdi;
		pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrmagvar, "Variation", "Magnetic Variation", "°", 1, -180.0, 360.0, 1.0, 10.0));
	}

	pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Waypoint", "Waypoint", ""));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrdtk, "DTK", "Desired Track", "°", 1, -180.0, 360.0, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrxtk, "XTK", "Cross Track Error", "nm", 2, -999999, 999999, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrtke, "TKE", "Track Error", "°", 1, -180.0, 360.0, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrbrg, "BRG", "Bearing", "°", 1, -180.0, 360.0, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrptk, "PTK", "Parallel Track", "nm", 3, -999999, 999999, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrmsa, "MSA", "Minimum Safe Altitude", "ft", 0, -999999, 999999, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrmesa, "MESA", "Minimum Enroute Safe Altitude", "ft", 0, -999999, 999999, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrtimewpt, "Time", "Time to Waypoint", ""));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrdist, "Dist", "Distance", "nmi", 2, -999999, 999999, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrdest, "Dest", "Destination Identifier", ""));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnractualvnav, "Actual VNAV", "Actual Vertical Navigation", "ft/min", 1, -999999, 999999, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrdesiredvnav, "Desired VNAV", "Desired Vertical Navigation", "ft/min", 1, -999999, 999999, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrverterr, "Vert Err", "Vertical Error", "ft", 1, -999999, 999999, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrvertangleerr, "Vert Ang Err", "Vertical Angle Error", "°", 2, -180.0, 360.0, 0.0, 0.0));

	pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Status", "GPS Device Status", ""));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_readonly, parnrsatellites, "# Satellites", "Number of Satellites used in Solution", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrsolutiontime, "Solution Time", "Time since Solution", "", 3, -999999, 999999, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrutcoffset, "UTC Offset", "Offset of the Local time to UTC", "", 1, -999999, 999999, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrgpstime, "GPS Time", "GPS Receiver Time", ""));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrposerr, "Pos Err", "Position Error", "", 1, -999999, 999999, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrnavflags, "Nav Flags", "Navigation Flags", ""));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrflags, "Flags", "Error Flags", ""));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrswcode, "SW Code", "Software Code", ""));

	pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Flight Plan", "Flight Plan", ""));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_readonly, parnrleg, "Leg", "Current Leg", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_readonly, parnrfplwpt, "# wpt", "Number of Waypoints", "", 0, 0, 9999, 1, 10));
}

Sensors::SensorKingGPS::paramfail_t Sensors::SensorKingGPS::get_param(unsigned int nr, Glib::ustring& v) const
{
	switch (nr) {
	default:
		break;

	case parnrtimewpt:
		v.clear();
		if (m_fixtype < fixtype_2d)
			return paramfail_fail;
		{
			unsigned int sec(m_data.get_timewpt());
			unsigned int hour(sec / 3600);
			sec -= hour * 3600;
			unsigned int min(sec / 60);
			sec -= min * 60;
			char buf[16];
			snprintf(buf, sizeof(buf), "%02u:%02u:%02u", hour, min, sec);
			v = buf;
		}
		return paramfail_ok;

	case parnrdest:
		v = m_data.get_destination();
		return v.empty() ? paramfail_fail : paramfail_ok;

	case parnrnavflags:
		v = m_data.get_navflags();
		return v.empty() ? paramfail_fail : paramfail_ok;

	case parnrflags:
		v = m_data.get_flags();
		return v.empty() ? paramfail_fail : paramfail_ok;

	case parnrswcode:
		v = m_data.get_swcode();
		return v.empty() ? paramfail_fail : paramfail_ok;

	case parnrgpstime:
	{
                v.clear();
                if (m_fixtype < fixtype_2d)
                        return paramfail_fail;
                Glib::TimeVal t(m_data.get_time());
                if (!t.valid())
                        return paramfail_fail;
                Glib::DateTime dt(Glib::DateTime::create_now_utc(t));
                v = dt.format("%Y-%m-%d %H:%M:%S");
                return paramfail_ok;
        }
	}
	return SensorGPS::get_param(nr, v);
}

Sensors::SensorKingGPS::paramfail_t Sensors::SensorKingGPS::get_param(unsigned int nr, double& v) const
{
	switch (nr) {
	default:
		break;

	case parnrbaroalt:
		v = m_data.get_baroalt();
		return std::isnan(v) ? paramfail_fail : paramfail_ok;

	case parnrbaroaltrate:
		v = m_baroaltrate;
		return std::isnan(v) ? paramfail_fail : paramfail_ok;

	case parnrpressalt:
		v = m_data.get_pressalt();
		return std::isnan(v) ? paramfail_fail : paramfail_ok;

	case parnrdtk:
		v = m_data.get_dtk();
		return (m_fixtype < fixtype_2d || std::isnan(v)) ? paramfail_fail : paramfail_ok;

	case parnrxtk:
		v = m_data.get_xtk();
		return (m_fixtype < fixtype_2d || std::isnan(v)) ? paramfail_fail : paramfail_ok;

	case parnrtke:
		v = m_data.get_tke();
		return (m_fixtype < fixtype_2d || std::isnan(v)) ? paramfail_fail : paramfail_ok;

	case parnrbrg:
		v = m_data.get_brg();
		return (m_fixtype < fixtype_2d || std::isnan(v)) ? paramfail_fail : paramfail_ok;

	case parnrdist:
		v = m_data.get_dist();
		return (m_fixtype < fixtype_2d || std::isnan(v)) ? paramfail_fail : paramfail_ok;

	case parnrptk:
		v = m_data.get_ptk();
		return (m_fixtype < fixtype_2d || std::isnan(v)) ? paramfail_fail : paramfail_ok;

	case parnrmagvar:
		v = m_data.get_magvar();
		return (m_fixtype < fixtype_2d || std::isnan(v)) ? paramfail_fail : paramfail_ok;

	case parnrmsa:
		v = m_data.get_msa();
		return (m_fixtype < fixtype_2d || std::isnan(v)) ? paramfail_fail : paramfail_ok;

	case parnrmesa:
		v = m_data.get_mesa();
		return (m_fixtype < fixtype_2d || std::isnan(v)) ? paramfail_fail : paramfail_ok;

	case parnractualvnav:
		v = m_data.get_actualvnav();
		return (m_fixtype < fixtype_2d || std::isnan(v)) ? paramfail_fail : paramfail_ok;

	case parnrdesiredvnav:
		v = m_data.get_desiredvnav();
		return (m_fixtype < fixtype_2d || std::isnan(v)) ? paramfail_fail : paramfail_ok;

	case parnrverterr:
		v = m_data.get_verterr();
		return (m_fixtype < fixtype_2d || std::isnan(v)) ? paramfail_fail : paramfail_ok;

	case parnrvertangleerr:
		v = m_data.get_vertangleerr();
		return (m_fixtype < fixtype_2d || std::isnan(v)) ? paramfail_fail : paramfail_ok;

	case parnrposerr:
		v = m_data.get_poserr();
		return (m_fixtype < fixtype_2d || std::isnan(v)) ? paramfail_fail : paramfail_ok;

	case parnrsolutiontime:
		v = m_data.get_timesincesolution();
		return (m_fixtype < fixtype_2d || std::isnan(v)) ? paramfail_fail : paramfail_ok;

	case parnrutcoffset:
		if (m_data.get_utcoffset() == std::numeric_limits<int>::min()) {
			v = 0;
			return paramfail_fail;
		}
		v = m_data.get_utcoffset() * (1.0 / 3600);
		return paramfail_ok;
	}
	return SensorGPS::get_param(nr, v);
}

Sensors::SensorKingGPS::paramfail_t Sensors::SensorKingGPS::get_param(unsigned int nr, int& v) const
{
	switch (nr) {
	default:
		break;

	case parnrbaroaltprio0:
	case parnrbaroaltprio1:
	case parnrbaroaltprio2:
	case parnrbaroaltprio3:
		v = m_baroaltitudepriority[nr - parnrbaroaltprio0];
		return paramfail_ok;

	case parnrleg:
		v = m_data.get_leg();
		return (!m_data.get_fplan().size()) ? paramfail_fail : paramfail_ok;

	case parnrfplwpt:
		v = m_data.get_fplan().size();
		return paramfail_ok;

	case parnrsatellites:
		v = m_data.get_satellites();
		if (m_fixtype < fixtype_2d || v >= 32) {
			v = 0;
			return paramfail_fail;
		}
		return paramfail_ok;
	}
	return SensorGPS::get_param(nr, v);
}

Sensors::SensorKingGPS::paramfail_t Sensors::SensorKingGPS::get_param(unsigned int nr, gpsflightplan_t& fpl, unsigned int& curwpt) const
{
	fpl = m_data.get_fplan();
	curwpt = m_data.get_leg();
	if (!fpl.empty())
		return paramfail_ok;
	return SensorGPS::get_param(nr, fpl, curwpt);
}

void Sensors::SensorKingGPS::set_param(unsigned int nr, int v)
{
	switch (nr) {
	default:
		SensorGPS::set_param(nr, v);
		return;

	case parnrbaroaltprio0:
	case parnrbaroaltprio1:
	case parnrbaroaltprio2:
	case parnrbaroaltprio3:
		if (m_baroaltitudepriority[nr - parnrbaroaltprio0] == (unsigned int)v)
			return;
		m_baroaltitudepriority[nr - parnrbaroaltprio0] = v;
		get_sensors().get_configfile().set_integer_list(get_configgroup(), "baroaltitudepriority", std::vector<int>(m_baroaltitudepriority, m_baroaltitudepriority + sizeof(m_baroaltitudepriority)/sizeof(m_baroaltitudepriority[0])));
		break;
	}
	ParamChanged pc;
	pc.set_changed(nr);
	update(pc);
}

std::string Sensors::SensorKingGPS::logfile_header(void) const
{
	return SensorGPS::logfile_header() + ",FixTime,Lat,Lon,GPSAlt,PressAlt,BaroAlt,VS,MagTrk,DTK,GS,Dist,XTK,TKE,BRG,PTK"
		",AVNAV,DVNAV,VERR,VAERR,PosERR,MagVar,MSA,MESA,GPSTime,UTCoffs,TimeWpt,Leg,NrSat,Dest,Flags,SWcode";
}

std::string Sensors::SensorKingGPS::logfile_records(void) const
{
	std::ostringstream oss;
	oss << SensorGPS::logfile_records() << std::fixed << std::setprecision(6) << ',';
	if (m_time.valid() && m_fixtype >= fixtype_2d) {
                Glib::DateTime dt(Glib::DateTime::create_now_utc(m_time));
                oss << dt.format("%Y-%m-%d %H:%M:%S");
	}
	oss << ',';
	if (m_fixtype >= fixtype_2d && m_data.is_lat_set())
		oss << m_data.get_position().get_lat_deg_dbl();
	oss << ',';
	if (m_fixtype >= fixtype_2d && m_data.is_lon_set())
		oss << m_data.get_position().get_lon_deg_dbl();
	oss << ',';
	{
		double v(m_data.get_gpsalt());
		if (!std::isnan(v))
			oss << v;
		oss << ',';
	}
	{
		double v(m_data.get_pressalt());
		if (!std::isnan(v))
			oss << v;
		oss << ',';
	}
	{
		double v(m_data.get_baroalt());
		if (!std::isnan(v))
			oss << v;
		oss << ',';
	}
	if (!std::isnan(m_baroaltrate))
		oss << m_baroaltrate;
	oss << ',';
	{
		double v(m_data.get_magtrack());
		if (!std::isnan(v))
			oss << v;
		oss << ',';
	}
	{
		double v(m_data.get_dtk());
		if (!std::isnan(v))
			oss << v;
		oss << ',';
	}
	{
		double v(m_data.get_gs());
		if (!std::isnan(v))
			oss << v;
		oss << ',';
	}
	{
		double v(m_data.get_dist());
		if (!std::isnan(v))
			oss << v;
		oss << ',';
	}
	{
		double v(m_data.get_xtk());
		if (!std::isnan(v))
			oss << v;
		oss << ',';
	}
	{
		double v(m_data.get_tke());
		if (!std::isnan(v))
			oss << v;
		oss << ',';
	}
	{
		double v(m_data.get_brg());
		if (!std::isnan(v))
			oss << v;
		oss << ',';
	}
	{
		double v(m_data.get_ptk());
		if (!std::isnan(v))
			oss << v;
		oss << ',';
	}
	{
		double v(m_data.get_actualvnav());
		if (!std::isnan(v))
			oss << v;
		oss << ',';
	}
	{
		double v(m_data.get_desiredvnav());
		if (!std::isnan(v))
			oss << v;
		oss << ',';
	}
	{
		double v(m_data.get_verterr());
		if (!std::isnan(v))
			oss << v;
		oss << ',';
	}
	{
		double v(m_data.get_vertangleerr());
		if (!std::isnan(v))
			oss << v;
		oss << ',';
	}
	{
		double v(m_data.get_poserr());
		if (!std::isnan(v))
			oss << v;
		oss << ',';
	}
	{
		double v(m_data.get_magvar());
		if (!std::isnan(v))
			oss << v;
		oss << ',';
	}
	{
		double v(m_data.get_msa());
		if (!std::isnan(v))
			oss << v;
		oss << ',';
	}
	{
		double v(m_data.get_mesa());
		if (!std::isnan(v))
			oss << v;
		oss << ',';
	}
	{
		double v(m_data.get_timesincesolution());
		if (!std::isnan(v))
			oss << v;
		oss << ',';
	}
	{
                Glib::TimeVal t(m_data.get_time());
                if (t.valid()) {
			Glib::DateTime dt(Glib::DateTime::create_now_utc(t));
			oss << dt.format("%Y-%m-%d %H:%M:%S");
		}
		oss << ',';
	}
	oss << m_data.get_utcoffset() << ',' << m_data.get_timewpt() << ',' << m_data.get_leg() << ',' << m_data.get_satellites()
	    << ',' << m_data.get_destination() << ',' << m_data.get_flags() << ',' << m_data.get_swcode();
	return oss.str();
}
