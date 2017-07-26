//
// C++ Implementation: Sensors
//
// Description: Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2012, 2013, 2015, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <iomanip>
#include <limits>
#include <sstream>
#include <cstring>
#include <giomm.h>

#include "sensors.h"
#include "sensgps.h"
#include "fplan.h"
#include "maps.h"

#if defined(HAVE_EIGEN3)
#include "sensattitude.h"
#include "sensattpsmove.h"
#if defined(HAVE_BLUEZ)
#include "sensattpsmovebt.h"
#endif
#if defined(HAVE_HIDAPI)
#include "sensattpsmovehid.h"
#endif
#endif

#if defined(HAVE_LIBGPS)
#include "sensgpsd.h"
#endif

#if defined(HAVE_GYPSY)
#include "sensgypsy.h"
#endif

#if defined(HAVE_LIBLOCATION)
#include "senslibloc.h"
#endif

#if defined(HAVE_GEOCLUE)
#include "sensgeoclue.h"
#endif

#if defined(HAVE_GEOCLUE2)
#include "sensgeoclue2.h"
#endif

#include "sensgpskingtty.h"

#if defined(HAVE_FTDI)
#include "sensgpskingftdi.h"
#endif

#include "sensgpsnmea.h"

#include "sensms5534.h"

#include "sensrtladsb.h"
#include "sensremoteadsb.h"

#if defined(HAVE_SENSORSAPI_H)
#include "senswingps.h"
#include "senswinbaro.h"
#include "senswinatt.h"
#endif

#if defined(HAVE_UDEV) && defined(HAVE_LIBUSB1)
#include "sensattstmhub.h"
#endif

Sensors::ConfigFile::ConfigFile(const ConfigFile& cf)
	: m_filename(cf.get_filename()), m_ok(false)
{
	ConfigFile& cf1(const_cast<ConfigFile&>(cf));
	m_ok = load_from_data(cf1.to_data(), Glib::KEY_FILE_KEEP_COMMENTS | Glib::KEY_FILE_KEEP_TRANSLATIONS);
	if (!m_ok)
		return;
	m_ok = has_key("main", "name");
}

Sensors::ConfigFile::ConfigFile(const std::string& read_filename, const std::string& write_filename)
	: m_filename(write_filename), m_ok(false)
{
	m_ok = load_from_file(read_filename, Glib::KEY_FILE_KEEP_COMMENTS | Glib::KEY_FILE_KEEP_TRANSLATIONS);
	if (!m_ok)
		return;
	m_ok = has_key("main", "name");
}

Sensors::ConfigFile::ConfigFile(const std::string& write_filename)
	: m_filename(write_filename), m_ok(false)
{
}

Glib::ustring Sensors::ConfigFile::get_name(void) const
{
	return get_string("main", "name");
}

Glib::ustring Sensors::ConfigFile::get_description(void) const
{
	if (!has_key("main", "description"))
		return "";
	return get_locale_string("main", "description");
}

std::set<Sensors::ConfigFile> Sensors::find_configs(const std::string& dir_main, const std::string& dir_aux)
{
	std::string mdir(dir_main);
        if (mdir.empty())
                mdir = FPlan::get_userdbpath();
        mdir = Glib::build_filename(mdir, "flightdeck");
	std::string adir(dir_aux);
        if (adir.empty())
                adir = Engine::get_default_aux_dir();
        adir = Glib::build_filename(adir, "flightdeck");
	typedef std::set<ConfigFile> cfgs_t;
	cfgs_t cfgs;
	if (!Glib::file_test(mdir, Glib::FILE_TEST_EXISTS) || !Glib::file_test(mdir, Glib::FILE_TEST_IS_DIR)) {
		if (g_mkdir_with_parents(mdir.c_str(), 0755))
			return cfgs;
	}
	if (true)
		std::cerr << "Sensors::find_configs: main dir " << mdir << " aux dir " << adir << std::endl;
	{
		Glib::Dir dir(mdir);
		for (;;) {
			std::string fn(dir.read_name());
			if (fn.empty())
				break;
			fn = Glib::build_filename(mdir, fn);
			ConfigFile c(fn, fn);
			if (!c.is_ok())
				continue;
			std::pair<cfgs_t::iterator, bool> ins(cfgs.insert(c));
			if (true && ins.second)
				std::cerr << "Sensors::find_configs: new config: read " << fn
					  << " write " << fn << std::endl;
		}
	}
	if (Glib::file_test(adir, Glib::FILE_TEST_EXISTS) && Glib::file_test(adir, Glib::FILE_TEST_IS_DIR)) {
		Glib::Dir dir(adir);
		for (;;) {
			std::string fn(dir.read_name());
			if (fn.empty())
				break;
			ConfigFile c(Glib::build_filename(adir, fn), Glib::build_filename(mdir, fn));
			if (!c.is_ok())
				continue;
			std::pair<cfgs_t::iterator, bool> ins(cfgs.insert(c));
			if (true && ins.second)
				std::cerr << "Sensors::find_configs: new config: read " << Glib::build_filename(adir, fn)
					  << " write " << Glib::build_filename(mdir, fn) << std::endl;
		}
	}
	return cfgs;
}

int Sensors::Sensor::Satellite::compare(const Satellite& sat) const
{
	if (get_prn() < sat.get_prn())
		return -1;
	if (get_prn() > sat.get_prn())
		return 1;
	return 0;
}

int Sensors::Sensor::Satellite::compare_all(const Satellite& sat) const
{
	int r(compare(sat));
	if (r)
		return r;
	if (get_azimuth() < sat.get_azimuth())
		return -1;
	if (get_azimuth() > sat.get_azimuth())
		return 1;
	if (get_elevation() < sat.get_elevation())
		return -1;
	if (get_elevation() > sat.get_elevation())
		return 1;
	if (get_snr() < sat.get_snr())
		return -1;
	if (get_snr() > sat.get_snr())
		return 1;
	if (is_used() < sat.is_used())
		return -1;
	if (is_used() > sat.is_used())
		return 1;
	return 0;
}

Sensors::Sensor::GPSFlightPlanWaypoint::GPSFlightPlanWaypoint(const Point& coord, const std::string& ident, bool dest, double var)
	: m_ident(ident), m_coord(coord), m_var(var), m_type(type_undefined), m_dest(dest)
{
	while (!m_ident.empty() && m_ident[m_ident.size()-1] == ' ')
		m_ident.resize(m_ident.size()-1);
}

Sensors::Sensor::GPSFlightPlanWaypoint::GPSFlightPlanWaypoint(const uint8_t *p)
	: m_var(std::numeric_limits<double>::quiet_NaN()), m_type(type_invalid), m_dest(false)
{
	if (!p)
		return;
	m_dest = !!(p[0] & 0x20);
	if (p[1] != 0x7F) {
		m_ident = std::string((const char *)(p + 1), 5);
		while (!m_ident.empty() && m_ident[m_ident.size()-1] == ' ')
			m_ident.resize(m_ident.size()-1);
	}
	if (p[6] != 0x7f) {
		m_coord.set_lat_deg_dbl((p[6] & 0x7f) + (p[7] & 0x3f) * (1.0 / 60.0) + (p[8] & 0x7f) * (1.0 / 60.0 / 100.0));
		if (p[6] & 0x80)
			m_coord.set_lat(-m_coord.get_lat());
		m_coord.set_lon_deg_dbl(p[10] + (p[11] & 0x3f) * (1.0 / 60.0) + (p[12] & 0x7f) * (1.0 / 60.0 / 100.0));
		if (p[9] & 0x80)
			m_coord.set_lon(-m_coord.get_lon());
		m_type = type_undefined;
	}
	if (p[13] != 0x7f) {
		int16_t v = p[13];
		v <<= 8;
		v |= p[14];
		m_var = v * (1.0 / 16.0);
	}
}

void Sensors::Sensor::GPSFlightPlanWaypoint::set_type(type_t typ)
{
	if (m_type != type_invalid)
		m_type = typ;
}

Sensors::Sensor::ADSBTarget::Position::Position(const Glib::TimeVal& ts, const Point& coord, uint8_t type,
						uint32_t cprlat, uint32_t cprlon, bool cprformat, bool timebit,
						altmode_t altmode, uint16_t alt)
	: m_timestamp(ts), m_coord(coord), m_cprlat(cprlat), m_cprlon(cprlon), m_alt(alt),
	  m_altmode(altmode), m_type(type), m_cprformat(cprformat), m_timebit(timebit)
{
}

Sensors::Sensor::ADSBTarget::ADSBTarget(uint32_t icaoaddr, bool ownship)
	: m_timestamp(-1, -1), m_motiontimestamp(-1, -1), m_baroalttimestamp(-1, -1), m_gnssalttimestamp(-1, -1), m_icaoaddr(icaoaddr),
	  m_baroalt(std::numeric_limits<int32_t>::min()), m_gnssalt(std::numeric_limits<int32_t>::min()),
	  m_verticalspeed(0), m_modea(std::numeric_limits<uint16_t>::max()), m_speed(0), m_direction(0), 
	  m_capability(std::numeric_limits<uint8_t>::max()), m_emergencystate(std::numeric_limits<uint8_t>::max()),
	  m_emittercategory(std::numeric_limits<uint8_t>::max()), m_nicsuppa(std::numeric_limits<uint8_t>::max()),
	  m_nicsuppb(std::numeric_limits<uint8_t>::max()), m_nicsuppc(std::numeric_limits<uint8_t>::max()),
	  m_lmotion(lmotion_invalid), m_vmotion(vmotion_invalid), m_ownship(ownship)
{
}

Sensors::Sensor::ADSBTarget::ADSBTarget(const Glib::TimeVal& ts, uint32_t icaoaddr, bool ownship)
	: m_timestamp(ts), m_motiontimestamp(-1, -1), m_baroalttimestamp(-1, -1), m_gnssalttimestamp(-1, -1), m_icaoaddr(icaoaddr),
	  m_baroalt(std::numeric_limits<int32_t>::min()), m_gnssalt(std::numeric_limits<int32_t>::min()),
	  m_verticalspeed(0), m_modea(std::numeric_limits<uint16_t>::max()), m_speed(0), m_direction(0), 
	  m_capability(std::numeric_limits<uint8_t>::max()), m_emergencystate(std::numeric_limits<uint8_t>::max()),
	  m_emittercategory(std::numeric_limits<uint8_t>::max()), m_nicsuppa(std::numeric_limits<uint8_t>::max()),
	  m_nicsuppb(std::numeric_limits<uint8_t>::max()), m_nicsuppc(std::numeric_limits<uint8_t>::max()),
	  m_lmotion(lmotion_invalid), m_vmotion(vmotion_invalid), m_ownship(ownship)
{
}

void Sensors::Sensor::ADSBTarget::set_baroalt(const Glib::TimeVal& tv, int32_t baroalt)
{
	m_timestamp = tv;
	m_baroalttimestamp = tv;
	m_baroalt = baroalt;
}

void Sensors::Sensor::ADSBTarget::set_baroalt(const Glib::TimeVal& tv, int32_t baroalt, int32_t verticalspeed)
{
	m_timestamp = tv;
	m_baroalttimestamp = tv;
	m_baroalt = baroalt;
	m_verticalspeed = verticalspeed;
}

void Sensors::Sensor::ADSBTarget::set_gnssalt(const Glib::TimeVal& tv, int32_t gnssalt)
{
	m_timestamp = tv;
	m_gnssalttimestamp = tv;
	m_gnssalt = gnssalt;
}

void Sensors::Sensor::ADSBTarget::set_gnssalt(const Glib::TimeVal& tv, int32_t gnssalt, int32_t verticalspeed)
{
	m_timestamp = tv;
	m_gnssalttimestamp = tv;
	m_gnssalt = gnssalt;
	m_verticalspeed = verticalspeed;
}

void Sensors::Sensor::ADSBTarget::set_modea(const Glib::TimeVal& tv, uint16_t modea)
{
	m_timestamp = tv;
	m_modea = modea;
}

void Sensors::Sensor::ADSBTarget::set_modea(const Glib::TimeVal& tv, uint16_t modea, uint8_t emergencystate)
{
	m_timestamp = tv;
	m_modea = modea;
	m_emergencystate = emergencystate;
}

void Sensors::Sensor::ADSBTarget::set_ident(const Glib::TimeVal& tv, const std::string& ident, uint8_t emittercategory)
{
	m_timestamp = tv;
	m_ident = ident;
	m_emittercategory = emittercategory;
}

void Sensors::Sensor::ADSBTarget::set_motion(const Glib::TimeVal& tv, int16_t speed, uint16_t direction, lmotion_t lmotion,
					     int32_t verticalspeed, vmotion_t vmotion)
{
	m_timestamp = tv;
	m_motiontimestamp = tv;
	m_verticalspeed = verticalspeed;
	m_speed = speed;
	m_direction = direction;
	m_lmotion = lmotion;
	m_vmotion = vmotion;
	
}

void Sensors::Sensor::ADSBTarget::add_position(const Position& pos)
{
	m_timestamp = pos.get_timestamp();
	m_positions.push_back(pos);
	for (unsigned int i = 0; i + 1 < m_positions.size(); ) {
		Glib::TimeVal tv(m_positions.back().get_timestamp() - m_positions[i].get_timestamp());
		tv.subtract_seconds(60);
		if (tv.negative()) {
			++i;
			continue;
		}
		m_positions.erase(m_positions.begin() + i);
	}
}

Sensors::Sensor::ParamChanged::ParamChanged(void)
{
	clear_all_changed();
}

bool Sensors::Sensor::ParamChanged::is_changed(unsigned int nr) const
{
	unsigned int bnr(nr & 31);
	unsigned int wnr(nr >> 5);
	if (wnr >= sizeof(m_changed)/sizeof(m_changed[0]))
		return false;
	return !!(m_changed[wnr] & (1U << bnr));
}

bool Sensors::Sensor::ParamChanged::is_changed(unsigned int from, unsigned int to) const
{
	if (to < from)
		return false;
       	unsigned int bnr0(from & 31);
	unsigned int wnr0(from >> 5);
       	unsigned int bnr1(to & 31);
	unsigned int wnr1(to >> 5);
	uint32_t mask(~((1 << bnr0) - 1));
	uint32_t maske((1 << bnr1) - 1);
	maske <<= 1;
	maske |= 1;
	for (; wnr0 < sizeof(m_changed)/sizeof(m_changed[0]); ++wnr0, mask |= ~0U) {
		uint32_t x(m_changed[wnr0] & mask);
		if (wnr0 == wnr1)
			return !!(x & maske);
		if (x)
			return true;
	}
	return false;
}

void Sensors::Sensor::ParamChanged::set_changed(unsigned int nr) 
{
	unsigned int bnr(nr & 31);
	unsigned int wnr(nr >> 5);
	if (wnr >= sizeof(m_changed)/sizeof(m_changed[0]))
		return;
	m_changed[wnr] |= (1U << bnr);
}

void Sensors::Sensor::ParamChanged::set_changed(unsigned int from, unsigned int to)
{
	if (to < from)
		return;
       	unsigned int bnr0(from & 31);
	unsigned int wnr0(from >> 5);
       	unsigned int bnr1(to & 31);
	unsigned int wnr1(to >> 5);
	uint32_t mask(~((1 << bnr0) - 1));
	uint32_t maske((1 << bnr1) - 1);
	maske <<= 1;
	maske |= 1;
	for (; wnr0 < sizeof(m_changed)/sizeof(m_changed[0]); ++wnr0, mask = ~0U) {
		if (wnr0 == wnr1) {
			m_changed[wnr0] |= mask & maske;
			break;
		}
		m_changed[wnr0] |= mask;
	}
}

void Sensors::Sensor::ParamChanged::set_all_changed(void)
{
	memset(m_changed, 0xff, sizeof(m_changed));
}

void Sensors::Sensor::ParamChanged::clear_changed(unsigned int nr)
{
	unsigned int bnr(nr & 31);
	unsigned int wnr(nr >> 5);
	if (wnr >= sizeof(m_changed)/sizeof(m_changed[0]))
		return;
	m_changed[wnr] &= ~(1U << bnr);
}

void Sensors::Sensor::ParamChanged::clear_changed(unsigned int from, unsigned int to)
{
	if (to < from)
		return;
       	unsigned int bnr0(from & 31);
	unsigned int wnr0(from >> 5);
       	unsigned int bnr1(to & 31);
	unsigned int wnr1(to >> 5);
	uint32_t mask(~((1 << bnr0) - 1));
	uint32_t maske((1 << bnr1) - 1);
	maske <<= 1;
	maske |= 1;
	mask = ~mask;
	maske = ~maske;
	for (; wnr0 < sizeof(m_changed)/sizeof(m_changed[0]); ++wnr0, mask = 0U) {
		if (wnr0 == wnr1) {
			m_changed[wnr0] &= mask | maske;
			break;
		}
		m_changed[wnr0] &= mask;
	}
}

void Sensors::Sensor::ParamChanged::clear_all_changed(void)
{
	memset(m_changed, 0x00, sizeof(m_changed));
}

Sensors::Sensor::ParamChanged& Sensors::Sensor::ParamChanged::operator&=(const ParamChanged& x)
{
	for (unsigned int i = 0; i < sizeof(m_changed)/sizeof(m_changed[0]); ++i)
		m_changed[i] &= x.m_changed[i];
	return *this;
}

Sensors::Sensor::ParamChanged& Sensors::Sensor::ParamChanged::operator|=(const ParamChanged& x)
{
	for (unsigned int i = 0; i < sizeof(m_changed)/sizeof(m_changed[0]); ++i)
		m_changed[i] |= x.m_changed[i];
	return *this;
}

Sensors::Sensor::ParamChanged Sensors::Sensor::ParamChanged::operator&(const ParamChanged& x) const
{
	ParamChanged r(*this);
	r &= x;
	return r;
}

Sensors::Sensor::ParamChanged Sensors::Sensor::ParamChanged::operator|(const ParamChanged& x) const
{
	ParamChanged r(*this);
	r |= x;
	return r;
}

Sensors::Sensor::ParamChanged Sensors::Sensor::ParamChanged::operator~(void) const
{
	ParamChanged r(*this);
	for (unsigned int i = 0; i < sizeof(m_changed)/sizeof(m_changed[0]); ++i)
		r.m_changed[i] = ~r.m_changed[i];
	return r;
}

Sensors::Sensor::ParamChanged::operator bool(void) const
{
	for (unsigned int i = 0; i < sizeof(m_changed)/sizeof(m_changed[0]); ++i)
		if (m_changed[i])
			return true;
	return false;
}

Sensors::Sensor::Sensor()
{
}

Sensors::Sensor::~Sensor()
{
}

bool Sensors::Sensor::get_position(Point& pt) const
{
	pt = Point();
	return false;
}

unsigned int Sensors::Sensor::get_position_priority(void) const
{
	return 0;
}

Glib::TimeVal Sensors::Sensor::get_position_time(void) const
{
	return Glib::TimeVal();
}

bool Sensors::Sensor::is_position_ok(const Glib::TimeVal& tv) const
{
	Point pt;
	if (!get_position(pt))
		return false;
	Glib::TimeVal tv1(get_position_time());
	if (!tv1.valid())
		return false;
	tv1.add_seconds(2);
	tv1.subtract(tv);
	if (tv1.negative())
		return false;
	return true;
}

bool Sensors::Sensor::is_position_ok(void) const
{
	Glib::TimeVal tv;
	tv.assign_current_time();
	return is_position_ok(tv);
}

void Sensors::Sensor::get_truealt(double& alt, double& altrate) const
{
	alt = altrate = std::numeric_limits<double>::quiet_NaN();
}

unsigned int Sensors::Sensor::get_truealt_priority(void) const
{
	return 0;
}

Glib::TimeVal Sensors::Sensor::get_truealt_time(void) const
{
	return Glib::TimeVal();
}

bool Sensors::Sensor::is_truealt_ok(const Glib::TimeVal& tv) const
{
	double alt, altrate;
	get_truealt(alt, altrate);
	if (std::isnan(alt) || std::isnan(altrate))
		return false;
	Glib::TimeVal tv1(get_truealt_time());
	if (!tv1.valid())
		return false;
	tv1.add_seconds(2);
	tv1.subtract(tv);
	if (tv1.negative())
		return false;
	return true;
}

bool Sensors::Sensor::is_truealt_ok(void) const
{
	Glib::TimeVal tv;
	tv.assign_current_time();
	return is_truealt_ok(tv);
}

void Sensors::Sensor::get_baroalt(double& alt, double& altrate) const
{
	alt = altrate = std::numeric_limits<double>::quiet_NaN();
}

unsigned int Sensors::Sensor::get_baroalt_priority(void) const
{
	return 0;
}

Glib::TimeVal Sensors::Sensor::get_baroalt_time(void) const
{
	return Glib::TimeVal();
}

bool Sensors::Sensor::is_baroalt_ok(const Glib::TimeVal& tv) const
{
	double alt, altrate;
	get_baroalt(alt, altrate);
	if (std::isnan(alt) || std::isnan(altrate))
		return false;
	Glib::TimeVal tv1(get_baroalt_time());
	if (!tv1.valid())
		return false;
	tv1.add_seconds(2);
	tv1.subtract(tv);
	if (tv1.negative())
		return false;
	return true;
}

bool Sensors::Sensor::is_baroalt_ok(void) const
{
	Glib::TimeVal tv;
	tv.assign_current_time();
	return is_baroalt_ok(tv);
}

void Sensors::Sensor::get_course(double& crs, double& gs) const
{
	crs = gs = std::numeric_limits<double>::quiet_NaN();
}

unsigned int Sensors::Sensor::get_course_priority(void) const
{
	return 0;
}

Glib::TimeVal Sensors::Sensor::get_course_time(void) const
{
	return Glib::TimeVal();
}

bool Sensors::Sensor::is_course_ok(const Glib::TimeVal& tv) const
{
	double crs, gs;
	get_course(crs, gs);
	if (std::isnan(crs) || std::isnan(gs))
		return false;
	Glib::TimeVal tv1(get_course_time());
	if (!tv1.valid())
		return false;
	tv1.add_seconds(2);
	tv1.subtract(tv);
	if (tv1.negative())
		return false;
	return true;
}

bool Sensors::Sensor::is_course_ok(void) const
{
	Glib::TimeVal tv;
	tv.assign_current_time();
	return is_course_ok(tv);
}

void Sensors::Sensor::get_heading(double& hdg) const
{
	hdg = std::numeric_limits<double>::quiet_NaN();
}

unsigned int Sensors::Sensor::get_heading_priority(void) const
{
	return 0;
}

Glib::TimeVal Sensors::Sensor::get_heading_time(void) const
{
	return Glib::TimeVal();
}

bool Sensors::Sensor::is_heading_ok(const Glib::TimeVal& tv) const
{
	double hdg;
	get_heading(hdg);
	if (std::isnan(hdg))
		return false;
	Glib::TimeVal tv1(get_heading_time());
	if (!tv1.valid())
		return false;
	tv1.add_seconds(2);
	tv1.subtract(tv);
	if (tv1.negative())
		return false;
	return true;
}

bool Sensors::Sensor::is_heading_ok(void) const
{
	Glib::TimeVal tv;
	tv.assign_current_time();
	return is_heading_ok(tv);
}

bool Sensors::Sensor::is_heading_true(void) const
{
	return false;
}

void Sensors::Sensor::get_attitude(double& pitch, double& bank, double& slip, double& rate) const
{
	pitch = bank = slip = rate = std::numeric_limits<double>::quiet_NaN();
}

unsigned int Sensors::Sensor::get_attitude_priority(void) const
{
	return 0;
}

Glib::TimeVal Sensors::Sensor::get_attitude_time(void) const
{
	return Glib::TimeVal();
}

bool Sensors::Sensor::is_attitude_ok(const Glib::TimeVal& tv) const
{
	double pitch,  bank, slip, rate;
	get_attitude(pitch,  bank, slip, rate);
	if (std::isnan(pitch) || std::isnan(bank) || std::isnan(slip) || std::isnan(rate))
		return false;
	Glib::TimeVal tv1(get_attitude_time());
	if (!tv1.valid())
		return false;
	tv1.add_seconds(1);
	tv1.subtract(tv);
	if (tv1.negative())
		return false;
	return true;
}

bool Sensors::Sensor::is_attitude_ok(void) const
{
	Glib::TimeVal tv;
	tv.assign_current_time();
	return is_attitude_ok(tv);
}

void Sensors::Sensor::get_param_desc(unsigned int pagenr, paramdesc_t& pd)
{
	pd.clear();
}

Sensors::Sensor::paramfail_t Sensors::Sensor::get_param(unsigned int nr, Glib::ustring& v) const
{
	v.clear();
	return Sensors::Sensor::paramfail_fail;
}

Sensors::Sensor::paramfail_t Sensors::Sensor::get_param(unsigned int nr, int& v) const
{
	v = 0;
	return Sensors::Sensor::paramfail_fail;
}

Sensors::Sensor::paramfail_t Sensors::Sensor::get_param(unsigned int nr, double& v) const
{
	v = std::numeric_limits<double>::quiet_NaN();
	return Sensors::Sensor::paramfail_fail;
}

Sensors::Sensor::paramfail_t Sensors::Sensor::get_param(unsigned int nr, double& pitch, double& bank, double& slip, double& hdg, double& rate) const
{
	pitch = bank = slip = hdg = rate = std::numeric_limits<double>::quiet_NaN();
	return Sensors::Sensor::paramfail_fail;
}

Sensors::Sensor::paramfail_t Sensors::Sensor::get_param(unsigned int nr, satellites_t& sat) const
{
	sat.clear();
	return Sensors::Sensor::paramfail_fail;
}

Sensors::Sensor::paramfail_t Sensors::Sensor::get_param(unsigned int nr, gpsflightplan_t& fpl, unsigned int& curwpt) const
{
	fpl.clear();
	curwpt = ~0U;
	return Sensors::Sensor::paramfail_fail;
}

Sensors::Sensor::paramfail_t Sensors::Sensor::get_param(unsigned int nr, adsbtargets_t& targets) const
{
	targets.clear();
	return Sensors::Sensor::paramfail_fail;
}

void Sensors::Sensor::set_param(unsigned int nr, const Glib::ustring& v)
{
}

void Sensors::Sensor::set_param(unsigned int nr, int v)
{
}

void Sensors::Sensor::set_param(unsigned int nr, double v)
{
}

const Glib::ustring Sensors::Sensor::empty = "";

class Sensors::DummySensor : public Sensor {
public:
	virtual std::string logfile_header(void) const { return std::string(); }
	virtual std::string logfile_records(void) const { return std::string(); }	
};

Sensors::SensorInstance::SensorInstance(Sensors& sensors, const Glib::ustring& configgroup)
	: m_sensors(sensors), m_configgroup(configgroup), m_refcount(1)
{
	const Glib::KeyFile& cf(get_sensors().get_configfile());
	if (cf.has_key(get_configgroup(), "name"))
		m_name = cf.get_string(get_configgroup(), "name");
	if (cf.has_key(get_configgroup(), "description"))
		m_description = cf.get_locale_string(get_configgroup(), "description");
}

void Sensors::SensorInstance::reference(void) const
{
	++m_refcount;
}

void Sensors::SensorInstance::unreference(void) const
{
	if (--m_refcount)
		return;
	delete this;
}

void Sensors::SensorInstance::update(const ParamChanged& pc)
{
	get_sensors().update_sensors();
	m_updateparams.emit(pc);
}

void Sensors::SensorInstance::get_param_desc(unsigned int pagenr, paramdesc_t& pd)
{
	pd.clear();
	if (pagenr)
		return;
	pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Device Name", "Device Name", ""));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_admin, parnrname, "Name", "Short Name to identify sensor data in Indicators", ""));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_admin, parnrdesc, "Description", "Sensor Description", ""));
}

Sensors::SensorInstance::paramfail_t Sensors::SensorInstance::get_param(unsigned int nr, Glib::ustring& v) const
{
	switch (nr) {
	default:
		return paramfail_fail;

	case parnrname:
		v = get_name();
		return paramfail_ok;

	case parnrdesc:
		v = get_description();
		return paramfail_ok;
	}
}

void Sensors::SensorInstance::set_param(unsigned int nr, const Glib::ustring& v)
{
	switch (nr) {
	default:
		return;

	case parnrname:
		if (m_name == v)
			return;
		m_name = v;
		get_sensors().get_configfile().set_string(get_configgroup(), "name", m_name);
		break;

	case parnrdesc:
		if (m_description == v)
			return;
		m_description = v;
		get_sensors().get_configfile().set_string(get_configgroup(), "description", m_description);
		break;
	}
	ParamChanged pc;
	pc.set_changed(nr);
	update(pc);
}

Sensors::MapAircraft::MapAircraft(uint32_t icaoaddr)
	: m_timestamp(Glib::TimeVal(-1, 0)), m_icaoaddr(icaoaddr),
	  m_baroalt(std::numeric_limits<int32_t>::min()),
	  m_truealt(std::numeric_limits<int32_t>::min()),
	  m_vs(std::numeric_limits<int32_t>::min()),
	  m_modea(std::numeric_limits<uint16_t>::max()),
	  m_crs(std::numeric_limits<uint16_t>::max()),
	  m_speed(std::numeric_limits<uint16_t>::max())
{
	m_coord.set_invalid();
}

Sensors::MapAircraft::MapAircraft(const Sensor::ADSBTarget& acft)
	:  m_registration(ModeSMessage::decode_registration(acft.get_icaoaddr())), m_ident(acft.get_ident()), 
	   m_timestamp(acft.get_timestamp()), m_icaoaddr(acft.get_icaoaddr()),
	   m_baroalt(std::numeric_limits<int32_t>::min()),
	   m_truealt(std::numeric_limits<int32_t>::min()),
	   m_vs(std::numeric_limits<int32_t>::min()),
	   m_modea(acft.get_modea()),
	   m_crs(std::numeric_limits<uint16_t>::max()),
	   m_speed(std::numeric_limits<uint16_t>::max())
{
	m_coord.set_invalid();
	if (!acft.empty()) {
		m_coord = acft.back().get_coord();
		if (!m_coord.is_invalid())
			m_timestamp = acft.back().get_timestamp();
	}
	if (acft.get_gnssalt() != std::numeric_limits<int32_t>::min())
		m_truealt = acft.get_gnssalt();
	if (acft.get_baroalt() != std::numeric_limits<int32_t>::min())
		m_baroalt = acft.get_baroalt();
	if (acft.get_vmotion() != Sensor::ADSBTarget::vmotion_invalid) {
		m_vs = acft.get_verticalspeed();
	}
	if (acft.get_lmotion() != Sensor::ADSBTarget::lmotion_invalid) {
		m_crs = acft.get_direction();
		m_speed = acft.get_speed();
	}
}

bool Sensors::MapAircraft::operator<(const MapAircraft& x) const
{
	return get_icaoaddr() < x.get_icaoaddr();
}

bool Sensors::MapAircraft::is_changed(const MapAircraft& x) const
{
	if (get_registration() != x.get_registration())
		return true;
	if (get_ident() != x.get_ident())
		return true;
	if (get_timestamp() != x.get_timestamp())
		return true;
	if (get_coord() != x.get_coord())
		return true;
	if (get_icaoaddr() != x.get_icaoaddr())
		return true;
	if (get_baroalt() != x.get_baroalt())
		return true;
	if (get_truealt() != x.get_truealt())
		return true;
	if (get_vs() != x.get_vs())
		return true;
	if (get_modea() != x.get_modea())
		return true;
	if (get_crs() != x.get_crs())
		return true;
	if (get_speed() != x.get_speed())
		return true;
	return false;
}

std::string Sensors::MapAircraft::get_line1(void) const
{
	std::ostringstream oss;
	bool subseq(false);
	if (is_registration_valid()) {
		if (subseq)
			oss << ' ';
		subseq = true;
		oss << get_registration();
	}
	if (is_ident_valid()) {
		if (subseq)
			oss << ' ';
		subseq = true;
		oss << get_ident();
	}
	if (is_modea_valid()) {
		if (subseq)
			oss << ' ';
		subseq = true;
		oss << 'A' << (char)('0' + ((get_modea() >> 9) & 7)) << (char)('0' + ((get_modea() >> 6) & 7))
		    << (char)('0' + ((get_modea() >> 3) & 7)) << (char)('0' + (get_modea() & 7));
	}
	return oss.str();
}

std::string Sensors::MapAircraft::get_line2(void) const
{
	std::ostringstream oss;
	bool subseq(false);
	if (is_baroalt_valid()) {
		if (subseq)
			oss << ' ';
		subseq = true;
		oss << get_baroalt() << '\'';
	}
	if (is_vs_valid()) {
		if (subseq)
			oss << ' ';
		subseq = true;
		oss << get_vs() << "ft/min";
	}
	if (is_crs_speed_valid()) {
		if (subseq)
			oss << ' ';
		subseq = true;
		oss << get_speed() << "kts";
	}
	return oss.str();
}

Sensors::Sensors(void)
	: m_engine(0), m_route(*(FPlan *)0), m_configfiledirty(false), m_altimeterstd(false), m_iasmode(iasmode_manual),
	  m_positionok(false), m_manualhdgtrue(false),
	  m_position(104481044, 565221672), m_altrate(std::numeric_limits<double>::quiet_NaN()),
	  m_crsmag(std::numeric_limits<double>::quiet_NaN()), m_crstrue(std::numeric_limits<double>::quiet_NaN()),
	  m_groundspeed(std::numeric_limits<double>::quiet_NaN()),
	  m_hdgmag(std::numeric_limits<double>::quiet_NaN()), m_hdgtrue(std::numeric_limits<double>::quiet_NaN()),
	  m_pitch(std::numeric_limits<double>::quiet_NaN()), m_bank(std::numeric_limits<double>::quiet_NaN()),
	  m_slip(std::numeric_limits<double>::quiet_NaN()), m_rate(std::numeric_limits<double>::quiet_NaN()),
	  m_manualhdg(std::numeric_limits<double>::quiet_NaN()), m_cruise(std::numeric_limits<double>::quiet_NaN(), 2400, 22), m_fuel(0), m_fuelflow(0),
	  m_bhp(std::numeric_limits<double>::quiet_NaN()), m_tkoffldgspeed(40), m_navbrg2tracktrue(std::numeric_limits<double>::quiet_NaN()),
	  m_navbrg2trackmag(std::numeric_limits<double>::quiet_NaN()), m_navbrg2dist(std::numeric_limits<double>::quiet_NaN()),
	  m_navcurdist(std::numeric_limits<double>::quiet_NaN()), m_navcurxtk(std::numeric_limits<double>::quiet_NaN()), m_navmaxxtk(1.0),
	  m_navgs(std::numeric_limits<double>::quiet_NaN()), m_navroc(std::numeric_limits<double>::quiet_NaN()),
	  m_navtimetowpt(std::numeric_limits<double>::quiet_NaN()), m_navcuralt(0),
	  m_navcurtrack(0), m_navnexttrack(0), m_navholdtrack(0), m_navcurtrackerr(0), m_navnexttrackerr(0), m_navcuraltflags(0), m_navcurwpt(0),
	  m_navlastwpttime(0), m_navflighttime(0), m_navflighttimerunning(false), m_navbrg2ena(false), m_navmode(navmode_off)
{
	m_fueltime.assign_current_time();
	log_internal_close();
}

Sensors::~Sensors()
{
	log_internal_close();
	close_track();
	m_sensors.clear();
	if (!m_configfilename.empty()) {
		std::vector<double> pos(2);
		pos[0] = m_position.get_lon_deg_dbl();
		pos[1] = m_position.get_lat_deg_dbl();
		get_configfile().set_double_list("main", "lastposition", pos);
	}
	m_elevation.reset();
	if (m_elevationquery) {
		Engine::ElevationMapResult::cancel(m_elevationquery);
		m_elevationquery.reset();
	}	m_sensorsupdate.disconnect();
	save_config();
}

void Sensors::initacft(void)
{
	if (!get_configfile().has_key("main", "aircraft"))
		return;
	std::string acft(get_configfile().get_string("main", "aircraft"));
	if (acft.empty())
		return;
	std::string mfile(Glib::build_filename(FPlan::get_userdbpath(), "aircraft"));
	if (!Glib::file_test(mfile, Glib::FILE_TEST_EXISTS) || !Glib::file_test(mfile, Glib::FILE_TEST_IS_DIR)) {
                if (g_mkdir_with_parents(mfile.c_str(), 0755))
			mfile.clear();
	}
	if (!mfile.empty())
		mfile = Glib::build_filename(mfile, acft);
	std::string afile(Glib::build_filename(PACKAGE_DATA_DIR, "aircraft", acft));
	if (Glib::file_test(mfile, Glib::FILE_TEST_EXISTS) && m_aircraft.load_file(mfile)) {
		m_aircraftfile = mfile;
		if (m_aircraft.get_dist().recalculatepoly(false) ||
		    m_aircraft.get_climb().recalculatepoly(false) ||
		    m_aircraft.get_descent().recalculatepoly(false))
			m_aircraft.save_file(mfile);
		return;
	}
	if (Glib::file_test(afile, Glib::FILE_TEST_EXISTS) && m_aircraft.load_file(afile)) {
		m_aircraftfile = afile;
		return;
	}
	if (acft.size() < 4 || acft.substr(acft.size() - 4) != ".xml") {
		mfile += ".xml";
		afile += ".xml";
		if (Glib::file_test(mfile, Glib::FILE_TEST_EXISTS) && m_aircraft.load_file(mfile)) {
			m_aircraftfile = mfile;
			if (m_aircraft.get_dist().recalculatepoly(false) ||
			    m_aircraft.get_climb().recalculatepoly(false) ||
			    m_aircraft.get_descent().recalculatepoly(false))
				m_aircraft.save_file(mfile);
			return;
		}
		if (Glib::file_test(afile, Glib::FILE_TEST_EXISTS) && m_aircraft.load_file(afile)) {
			m_aircraftfile = afile;
			return;
		}
	}
	if (Glib::file_test(mfile, Glib::FILE_TEST_EXISTS))
		return;
	m_aircraft.save_file(mfile);
	m_aircraftfile = mfile;
}

void Sensors::init(const Glib::ustring& cfgdata, const std::string& cfgfile)
{
	m_configfilename = cfgfile;
	if (!m_configfile.load_from_data(cfgdata, Glib::KEY_FILE_KEEP_COMMENTS | Glib::KEY_FILE_KEEP_TRANSLATIONS))
		throw std::runtime_error("Sensors::init: cannot load config data");
	m_configfiledirty = false;
	if (get_configfile().has_key("main", "name"))
		m_name = get_configfile().get_string("main", "name");
	if (get_configfile().has_key("main", "description"))
		m_description = get_configfile().get_locale_string("main", "description");
	if (get_configfile().has_key("main", "lastposition")) {
		Glib::ArrayHandle<double> pos(get_configfile().get_double_list("main", "lastposition"));
		if (pos.size() >= 2) {
			m_position.set_lon_deg_dbl(pos.data()[0]);
			m_position.set_lat_deg_dbl(pos.data()[1]);
		}
	}
	if (!get_configfile().has_key("main", "cruiserpm"))
		get_configfile().set_double("main", "cruiserpm", m_cruise.get_rpm());
	m_cruise.set_rpm(get_configfile().get_double("main", "cruiserpm"));
	if (!get_configfile().has_key("main", "cruisemp"))
		get_configfile().set_double("main", "cruisemp", m_cruise.get_mp());
	m_cruise.set_mp(get_configfile().get_double("main", "cruisemp"));
	if (!get_configfile().has_key("main", "cruisebhp"))
		get_configfile().set_double("main", "cruisebhp", m_cruise.get_bhp());
	m_cruise.set_bhp(get_configfile().get_double("main", "cruisebhp"));
	if (!get_configfile().has_key("main", "autoias"))
		get_configfile().set_integer("main", "autoias", (unsigned int)m_iasmode);
	m_iasmode = (iasmode_t)get_configfile().get_integer("main", "autoias");
	if (!get_configfile().has_key("main", "tkoffldgspeed"))
		get_configfile().set_double("main", "tkoffldgspeed", m_tkoffldgspeed);
	m_tkoffldgspeed = get_configfile().get_double("main", "tkoffldgspeed");
	if (get_configfile().has_group("airdata") && get_configfile().has_key("airdata", "tatprobe_ct"))
		m_airdata.set_tatprobe_ct(get_configfile().get_double("airdata", "tatprobe_ct"));
	else
		get_configfile().set_double("airdata", "tatprobe_ct", m_airdata.get_tatprobe_ct());
	if (!get_configfile().has_key("airdata", "qnh"))
		get_configfile().set_double("airdata", "qnh", IcaoAtmosphere<double>::std_sealevel_pressure);
	if (!get_configfile().has_key("airdata", "temperature_offset"))
		get_configfile().set_double("airdata", "temperature_offset", 0);
	m_airdata.set_qnh_tempoffs(get_configfile().get_double("airdata", "qnh"),
				   get_configfile().get_double("airdata", "temperature_offset"));
	if (!get_configfile().has_key("airdata", "standard"))
		get_configfile().set_integer("airdata", "standard", 0);
	m_altimeterstd = !!get_configfile().get_integer("airdata", "standard");
	// load aircraft model
	initacft();
	if (std::isnan(m_cruise.get_bhp())) {
		m_cruise.set_bhp(get_aircraft().get_maxbhp() * 0.65);
		get_configfile().set_double("main", "cruisebhp", m_cruise.get_bhp());
	}
	// class factory
	for (int sensnum = 1; sensnum <= 64; ++sensnum) {
		std::ostringstream sensnumstr;
		sensnumstr << "sensor" << sensnum;
		if (!get_configfile().has_group(sensnumstr.str()) ||
		    !get_configfile().has_key(sensnumstr.str(), "name") ||
		    !get_configfile().has_key(sensnumstr.str(), "type"))
			break;
		Glib::ustring stype(get_configfile().get_string(sensnumstr.str(), "type"));
		Glib::RefPtr<SensorInstance> sens;
		try {
#if defined(HAVE_EIGEN3)
#if defined(HAVE_UDEV) && defined(HAVE_HIDAPI)
			if (stype == "psmove" || stype == "psmovehid") {
				Glib::RefPtr<SensorInstance> sens1(new SensorAttitudePSMoveHID(*this, sensnumstr.str()));
				sens.swap(sens1);
			}
#if defined(HAVE_BLUEZ)
			else
#endif
#endif
#if defined(HAVE_BLUEZ)
			if (stype == "psmove" || stype == "psmovebt") {
				Glib::RefPtr<SensorInstance> sens1(new SensorAttitudePSMoveBT(*this, sensnumstr.str()));
				sens.swap(sens1);
			}
#endif
#endif

#if defined(HAVE_LIBGPS)
			if (stype == "gpsd") {
				Glib::RefPtr<SensorInstance> sens1(new SensorGPSD(*this, sensnumstr.str()));
				sens.swap(sens1);
			}
#endif

#if defined(HAVE_GYPSY)
			if (stype == "gypsy") {
				Glib::RefPtr<SensorInstance> sens1(new SensorGypsy(*this, sensnumstr.str()));
				sens.swap(sens1);
			}
#endif

#if defined(HAVE_LIBLOCATION)
			if (stype == "location" || stype == "liblocation") {
				Glib::RefPtr<SensorInstance> sens1(new SensorLibLocation(*this, sensnumstr.str()));
				sens.swap(sens1);
			}
#endif

#if defined(HAVE_GEOCLUE)
			if (stype == "geoclue") {
				Glib::RefPtr<SensorInstance> sens1(new SensorGeoclue(*this, sensnumstr.str()));
				sens.swap(sens1);
			}
#endif
#if defined(HAVE_GEOCLUE2)
			if (stype == "geoclue2") {
				Glib::RefPtr<SensorInstance> sens1(new SensorGeoclue2(*this, sensnumstr.str()));
				sens.swap(sens1);
			}
#endif
			if (stype == "gpsking") {
				Glib::RefPtr<SensorInstance> sens1(new SensorKingGPSTTY(*this, sensnumstr.str()));
				sens.swap(sens1);
			}
#if defined(HAVE_FTDI)
			if (stype == "gpskingftdi") {
				Glib::RefPtr<SensorInstance> sens1(new SensorKingGPSFTDI(*this, sensnumstr.str()));
				sens.swap(sens1);
			}
#endif
			if (stype == "gpsnmea") {
				Glib::RefPtr<SensorInstance> sens1(new SensorNMEAGPS(*this, sensnumstr.str()));
				sens.swap(sens1);
			}
			if (stype == "ms5534") {
				Glib::RefPtr<SensorInstance> sens1(new SensorMS5534(*this, sensnumstr.str()));
				sens.swap(sens1);
			}
#if defined(HAVE_SENSORSAPI_H)
			if (stype == "wingps") {
				Glib::RefPtr<SensorInstance> sens1(new SensorWinGPS(*this, sensnumstr.str()));
				sens.swap(sens1);
			}
			if (stype == "winbaro") {
				Glib::RefPtr<SensorInstance> sens1(new SensorWinBaro(*this, sensnumstr.str()));
				sens.swap(sens1);
			}
			if (stype == "winahrs" || stype == "winattitude" || stype == "winatt") {
				Glib::RefPtr<SensorInstance> sens1(new SensorWinAttitude(*this, sensnumstr.str()));
				sens.swap(sens1);
			}
#endif
#if defined(HAVE_UDEV) && defined(HAVE_LIBUSB1)
			if (stype == "stmsensorhub") {
				Glib::RefPtr<SensorInstance> sens1(new SensorAttitudeSTMSensorHub(*this, sensnumstr.str()));
				sens.swap(sens1);
			}
#endif
			if (stype == "rtladsb") {
				Glib::RefPtr<SensorInstance> sens1(new SensorRTLADSB(*this, sensnumstr.str()));
				sens.swap(sens1);
			}
			if (stype == "remoteadsb") {
				Glib::RefPtr<SensorInstance> sens1(new SensorRemoteADSB(*this, sensnumstr.str()));
				sens.swap(sens1);
			}
			if (!sens)
				log(Sensors::loglevel_error, "Error creating sensor " + sensnumstr.str() +
						  " type " + stype + ": sensor type unknown");	
		} catch (const Glib::Exception& e) {
			log(Sensors::loglevel_error, "Error creating sensor " + sensnumstr.str() +
			    " type " + stype + ": " + e.what());
		} catch (const std::exception& e) {
			log(Sensors::loglevel_error, "Error creating sensor " + sensnumstr.str() +
			    " type " + stype + ": " + e.what());
		}
		if (!sens)
			continue;
		m_sensors.push_back(sens);
	}
	for (sensors_t::iterator si(m_sensors.begin()), se(m_sensors.end()); si != se; ++si) {
		Glib::RefPtr<SensorInstance> sens(*si);
		sens->init();
		std::ostringstream oss;
		oss << "Sensor: " << sens->get_name() << ' ' << sens->get_description();
		log(Sensors::loglevel_notice, oss.str());
	}
	if (!get_configfile().has_key("main", "logfile"))
		get_configfile().set_integer("main", "logfile", 0);
	if (get_configfile().get_integer("main", "logfile"))
		log_internal_open();
	save_config();
}

Sensors::Sensor& Sensors::operator[](unsigned int idx)
{
	const Sensor& s(const_cast<const Sensors *>(this)->operator[](idx));
	return const_cast<Sensor&>(s);
}

const Sensors::Sensor& Sensors::operator[](unsigned int idx) const
{
	static const DummySensor dummysensor;
	if (idx >= m_sensors.size())
		return dummysensor;
	return *(m_sensors[idx].operator->());
}

void Sensors::set_engine(Engine& eng)
{
	m_engine = &eng;
	if (m_engine)
		m_route = FPlanRoute(m_engine->get_fplan_db());
	else
		m_route = FPlanRoute(*(FPlan *)0);
}

Glib::KeyFile& Sensors::get_configfile(void)
{
	m_configfiledirty = true;
	return m_configfile;
}

const Glib::KeyFile& Sensors::get_configfile(void) const
{
	return m_configfile;
}

void Sensors::save_config(void)
{
	if (!m_configfiledirty)
		return;
	Glib::RefPtr<Gio::File> f(Gio::File::create_for_path(m_configfilename));
	std::string new_etag;
	f->replace_contents(get_configfile().to_data(), "", new_etag);
	m_configfiledirty = false;
}

void Sensors::set_altimeter_std(bool setstd)
{
	bool chg(!setstd ^ !m_altimeterstd);
	m_altimeterstd = !!setstd;
	if (chg) {
		update_sensors_core(change_airdata);
		get_configfile().set_integer("airdata", "standard", m_altimeterstd);
		save_config();
	}
}

void Sensors::set_altimeter_qnh(double qnh)
{
	m_altimeterstd = false;
	m_airdata.set_qnh(qnh);
	update_sensors_core(change_airdata);
	get_configfile().set_double("airdata", "qnh", m_airdata.get_qnh());
	save_config();
}

double Sensors::get_altimeter_qnh(void) const
{
	return m_airdata.get_qnh();
}

double Sensors::get_altimeter_tempoffs(void) const
{
	return m_airdata.get_tempoffs();
}

void Sensors::set_tatprobe_ct(double ct)
{
	m_airdata.set_tatprobe_ct(ct);
	update_sensors_core(change_airdata);
	get_configfile().set_double("airdata", "tatprobe_ct", m_airdata.get_tatprobe_ct());
	save_config();
}

void Sensors::set_airdata_oat(double oat)
{
	m_airdata.set_oat(oat);
	update_sensors_core(change_airdata);
	get_configfile().set_double("airdata", "temperature_offset", m_airdata.get_tempoffs());
	save_config();	
}

void Sensors::set_airdata_rat(double oat)
{
	m_airdata.set_rat(oat);
	update_sensors_core(change_airdata);
	get_configfile().set_double("airdata", "temperature_offset", m_airdata.get_tempoffs());
	save_config();	
}

void Sensors::set_airdata_cas(double cas)
{
	m_airdata.set_cas(cas);
	update_sensors_core(change_airdata);
}

void Sensors::set_airdata_tas(double tas)
{
	m_airdata.set_tas(tas);
	update_sensors_core(change_airdata);
}

void Sensors::set_manualheading(double hdg, bool istrue)
{
	m_manualhdgtrue = istrue;
	m_manualhdg = hdg;
}

bool Sensors::get_position(Point& pt) const
{
	pt = m_position;
	return m_positionok;
}

bool Sensors::get_position(Point& pt, TopoDb30::elev_t& elev) const
{
	pt = m_position;
	elev = TopoDb30::nodata;
	if (!m_positionok || !m_elevation)
		return m_positionok;
	Rect bbox(m_elevation->get_bbox());
	if (!bbox.is_inside(m_position))
		return true;
	Point pt2(bbox.get_northeast() - bbox.get_southwest());
	unsigned int h(m_elevation->get_height());
	unsigned int w(m_elevation->get_width());
	if (!pt2.get_lon() || !pt2.get_lat() || !h || !w)
		return true;
	Point pt1(m_position.get_lon() - bbox.get_west(), bbox.get_north() - m_position.get_lat());
	int x(pt1.get_lon() * w / pt2.get_lon());
	int y(pt1.get_lat() * h / pt2.get_lat());
	if (false)
		std::cerr << "get_position: x " << x << " y " << y
			  << " (Rect " << bbox << " h " << h << " w " << w << ')' << std::endl;
	elev = m_elevation->operator()(x, y);
	return true;
}

void Sensors::get_altitude(double& baroalt, double& truealt, double& altrate) const
{
	baroalt = m_airdata.get_pressure_altitude();
	truealt = m_airdata.get_true_altitude();
	altrate = m_altrate;
}

void Sensors::get_altitude(double& baroalt, double& truealt) const
{
	baroalt = m_airdata.get_pressure_altitude();
	truealt = m_airdata.get_true_altitude();
}

void Sensors::get_course(double& crsmag, double& crstrue, double& groundspeed) const
{
	crsmag = m_crsmag;
	crstrue = m_crstrue;
	groundspeed = m_groundspeed;
}

void Sensors::get_course(double& crsmag, double& groundspeed) const
{
	crsmag = m_crsmag;
	groundspeed = m_groundspeed;
}

void Sensors::get_course(double& crsmag) const
{
	crsmag = m_crsmag;
}

void Sensors::get_heading(double& hdgmag, double& hdgtrue) const
{
	hdgmag = m_hdgmag;
	hdgtrue = m_hdgtrue;
}

void Sensors::get_heading(double& hdgmag) const
{
	hdgmag = m_hdgmag;
}

void Sensors::get_attitude(double& pitch, double& bank, double& slip, double& rate) const
{
	pitch = m_pitch;
	bank = m_bank;
	slip = m_slip;
	rate = m_rate;
}

Sensors::mapangle_t Sensors::get_mapangle(double& angle, mapangle_t flags) const
{
	if (false)
		std::cerr << "get_mapangle: flags 0x" << std::hex << (unsigned int)flags << std::dec
			  << " hdg " << m_hdgtrue << "T " << m_hdgmag << "M M" << m_manualhdg << "M "
			  << " crs " << m_crstrue << "T " << m_crsmag << "M" << std::endl;
	double r;
	angle = r = std::numeric_limits<double>::quiet_NaN();
	if (flags & mapangle_heading) {
		r = (flags & mapangle_true) ? m_hdgtrue : m_hdgmag;
		if (!std::isnan(r)) {
			angle = r;
			return mapangle_heading | (flags & mapangle_true);
		}
	}
	if (flags & mapangle_course) {
		r = (flags & mapangle_true) ? m_crstrue : m_crsmag;
		if (!std::isnan(r)) {
			angle = r;
			return mapangle_course | (flags & mapangle_true);
		}
	}
	if (flags & mapangle_manualheading) {
		r = m_manualhdg;
		if (!std::isnan(r)) {
			if (m_manualhdgtrue && !(flags & mapangle_true))
				r -= get_declination();
			else if (!m_manualhdgtrue && (flags & mapangle_true))
				r += get_declination();
			angle = r;
			return mapangle_manualheading | (flags & mapangle_true);
		}
	}
	return (flags & mapangle_true);
}

void Sensors::update_elevation_map(void)
{
	if (m_elevationquery && m_elevationquery->is_done()) {
		if (!m_elevationquery->is_error()) {
			m_elevation = m_elevationquery;
			if (false)
				std::cerr << "Elevation Map: " << m_elevation->get_bbox()
					  << " (current " << m_position << ')' << std::endl;
		}
		m_elevationquery.reset();
	}
	if (!m_engine)
		return;
	if (!m_positionok) {
		if (false)
			std::cerr << "Elevation Map Deallocate" << std::endl;
		m_elevation.reset();
		if (m_elevationquery) {
			Engine::ElevationMapResult::cancel(m_elevationquery);
			m_elevationquery.reset();
		}
		return;
	}
	if (m_elevationquery)
		return;
	if (m_elevation) {
		Rect bbox(m_elevation->get_bbox());
		Point pt(bbox.get_northeast() - bbox.get_southwest());
		pt.set_lat(pt.get_lat() / 4);
		pt.set_lon(pt.get_lon() / 4);
		bbox = Rect(bbox.get_southwest() + pt, bbox.get_northeast() - pt);
		if (bbox.is_inside(m_position))
			return;
	}
	m_elevationquery = m_engine->async_elevation_map(m_position.simple_box_nmi(20.0));
	//if (m_elevationquery)
	//	m_elevationquery->connect(sigc::mem_fun(*this, &Sensors::async_done));
}

void Sensors::update_sensors_core(change_t changemask)
{
	m_sensorsupdate.disconnect();
	Glib::TimeVal curtime;
	bool altbaro(false), hdgistrue(false);
	unsigned int positionprio(0), altprio(0), courseprio(0), headingprio(0), attitudeprio(0);
	Point position;
	double baroalt, truealt, altrate, crsmag, crstrue, groundspeed, hdgmag, hdgtrue, pitch, bank, slip, rate;
	baroalt = truealt = altrate = crsmag = crstrue = groundspeed = hdgmag = hdgtrue = pitch = bank = slip = rate =
		std::numeric_limits<double>::quiet_NaN();
	curtime.assign_current_time();
	for (sensors_t::const_iterator si(m_sensors.begin()), se(m_sensors.end()); si != se; ++si) {
		const Glib::RefPtr<SensorInstance> sensor(*si);
		if (false)
			std::cerr << "update_sensors: " << sensor->get_name() << " (" << sensor->get_description() << ")" << std::endl;
		// Position
		if (sensor->get_position_priority() >= positionprio && sensor->is_position_ok(curtime)) {
			positionprio = 1 + sensor->get_position_priority();
			sensor->get_position(position);
			m_positiontime = sensor->get_position_time();
		}
		// True Altitude
		if (sensor->get_truealt_priority() >= altprio && sensor->is_truealt_ok(curtime)) {
			altprio = 1 + sensor->get_truealt_priority();
			sensor->get_truealt(truealt, altrate);
			altbaro = false;
			m_alttime = sensor->get_truealt_time();
		}
		// Baro Altitude
		if (sensor->get_baroalt_priority() >= altprio && sensor->is_baroalt_ok(curtime)) {
			altprio = 1 + sensor->get_baroalt_priority();
			sensor->get_baroalt(baroalt, altrate);
			altbaro = true;
			m_alttime = sensor->get_baroalt_time();
		}
		// Course
		if (sensor->get_course_priority() >= courseprio && sensor->is_course_ok(curtime)) {
			courseprio = 1 + sensor->get_course_priority();
			sensor->get_course(crstrue, groundspeed);
			m_coursetime = sensor->get_course_time();
		}
		// Heading
		if (sensor->get_heading_priority() >= headingprio && sensor->is_heading_ok(curtime)) {
			headingprio = 1 + sensor->get_heading_priority();
			hdgistrue = sensor->is_heading_true();
			sensor->get_heading(*(hdgistrue ? &hdgtrue : &hdgmag));
			m_headingtime = sensor->get_heading_time();
		}
		// Attitude
		if (sensor->get_attitude_priority() >= attitudeprio && sensor->is_attitude_ok(curtime)) {
			attitudeprio = 1 + sensor->get_attitude_priority();
			sensor->get_attitude(pitch, bank, slip, rate);
			m_attitudetime = sensor->get_attitude_time();
		}
	}
	// compute altitudes
	{
		float palt(m_airdata.get_pressure_altitude());
		float talt(m_airdata.get_true_altitude());
		if (altbaro)
			m_airdata.set_pressure_altitude(baroalt);
		else
			m_airdata.set_true_altitude(truealt);
		if (palt != m_airdata.get_pressure_altitude() ||
		    talt != m_airdata.get_true_altitude() ||
		    m_altrate != altrate)
			changemask |= change_altitude;
		m_altrate = altrate;
	}
	// Update Position and Altitude
	if ((!m_positionok ^ !positionprio) || (positionprio && m_position != position))
		changemask |= change_position;
	m_positionok = !!positionprio;
	if (m_positionok)
		m_position = position;
	// Update magnetic Model
	if (m_positionok) {
		float alt(m_airdata.get_true_altitude());
		if (std::isnan(alt))
			alt = 0;
		else
			alt *= (FPlanWaypoint::ft_to_m / 1000.0);
		m_wmm.compute(alt, m_position, time(0));
		if (false)
			std::cerr << "WMM Compute: " << m_position.get_lat_str2() << ' ' << m_position.get_lon_str2() << ' ' << alt
				  << " WMM " << (m_wmm ? "true" : "false") << " decl " << m_wmm.get_dec() << ' ' << get_declination() << std::endl;
	}
	update_elevation_map();
	// Compute True/Magnetic
	if ((hdgistrue && std::isnan(hdgtrue)) || (!hdgistrue && std::isnan(hdgmag))) {
		hdgmag = hdgtrue = std::numeric_limits<double>::quiet_NaN();
	} else {
		if (hdgistrue)
			hdgmag = hdgtrue - get_declination();
		else
			hdgtrue = hdgmag + get_declination();
	}
	crsmag = crstrue - get_declination();
	// Update Course, Heading and Attitude
	if (m_crsmag != crsmag || m_crstrue != crstrue || m_groundspeed != groundspeed)
		changemask |= change_course;
	m_crsmag = crsmag;
	m_crstrue = crstrue;
	m_groundspeed = groundspeed;
	if (m_hdgmag != hdgmag || m_hdgtrue != hdgtrue)
		changemask |= change_heading;
	m_hdgmag = hdgmag;
	m_hdgtrue = hdgtrue;
	if (m_pitch != pitch || m_bank != bank || m_slip != slip || m_rate != rate)
		changemask |= change_attitude;
	m_pitch = pitch;
	m_bank = bank;
	m_slip = slip;
	m_rate = rate;
	{
		Glib::TimeVal tv(m_positiontime);
		if (!tv.valid() || (m_alttime.valid() && m_alttime < tv))
			tv = m_alttime;
		if (!tv.valid() || (m_coursetime.valid() && m_coursetime < tv))
			tv = m_coursetime;
		if (!tv.valid() || (m_headingtime.valid() && m_headingtime < tv))
			tv = m_headingtime;
		if (tv.valid())
			tv.add_seconds(1);
		if (!tv.valid() || (m_attitudetime.valid() && m_attitudetime < tv))
			tv = m_attitudetime;
		if (tv.valid()) {
			Glib::TimeVal tv1;
			tv1.assign_current_time();
			tv.subtract(tv1);
			tv.add_seconds(1);
			m_sensorsupdate = Glib::signal_timeout().connect(sigc::bind_return(sigc::mem_fun(*this, &Sensors::update_sensors), false), tv.tv_sec * 1000 + tv.tv_usec / 1000, Glib::PRIORITY_DEFAULT);
		}
	}
	changemask = update_navigation(changemask);
	if (curtime >= m_logtime) {
		m_logtime = curtime;
		m_logtime.add_milliseconds(950);
		log_writerecord();
	}
	if (false)
		std::cerr << "update_sensors: change mask 0x" << std::hex << (unsigned int)changemask << std::dec << std::endl;
       	if (changemask)
		m_sensorschange(changemask);
}

sigc::connection Sensors::connect_change(const sigc::slot<void,change_t>& slot)
{
	return m_sensorschange.connect(slot);
}

sigc::connection Sensors::connect_mapaircraft(const sigc::slot<void,const MapAircraft&>& slot)
{
	return m_mapaircraft.connect(slot);
}

void Sensors::set_mapaircraft(const MapAircraft& acft)
{
	MapAircraft acft1(acft);
	if (acft1.is_truealt_valid() && !acft1.is_baroalt_valid())
		acft1.set_baroalt(Point::round<int32_t,double>(true_to_pressure_altitude(acft1.get_truealt())));
	else if (acft1.is_baroalt_valid() && !acft1.is_truealt_valid())
		acft1.set_truealt(Point::round<int32_t,double>(pressure_to_true_altitude(acft1.get_baroalt())));
	if (true) {
		std::ostringstream oss;
		oss << std::hex << std::setfill('0') << std::setw(6) << acft1.get_icaoaddr() << std::dec;
                if (acft1.is_registration_valid())
			oss << ' ' << acft1.get_registration();
                if (acft1.is_ident_valid())
			oss << ' ' << acft1.get_ident();
                if (acft1.is_timestamp_valid())
			oss << ' ' << acft1.get_timestamp().as_iso8601();
                if (acft1.is_coord_valid())
			oss << ' ' << acft1.get_coord().get_lat_str2() << ' ' << acft1.get_coord().get_lon_str2();
                if (acft1.is_baroalt_valid()) {
			oss << ' ' << acft1.get_baroalt() << '\'';
			if (!acft.is_baroalt_valid())
				oss << '*';
		}
                if (acft1.is_truealt_valid()) {
			oss << ' ' << acft1.get_truealt() << '\'';
			if (!acft.is_truealt_valid())
				oss << '*';
		}
                if (acft1.is_vs_valid())
			oss << ' ' << acft1.get_vs() << "ft/min";
                if (acft1.is_modea_valid())
			oss << " A" << (char)('0'+((acft1.get_modea() >> 9) & 7)) << (char)('0'+((acft1.get_modea() >> 6) & 7))
			    << (char)('0'+((acft1.get_modea() >> 3) & 7)) << (char)('0'+(acft1.get_modea() & 7));
                if (acft1.is_crs_speed_valid())
			oss << ' ' << Point::round<int,double>(acft1.get_crs() * MapRenderer::to_deg_16bit_dbl)
			    << ' ' << acft1.get_speed() << "kts";
		std::cerr << "set_mapaircraft: " << oss.str() << std::endl;
	}
	m_mapaircraft(acft1);
}

void Sensors::log(loglevel_t lvl, const Glib::ustring& msg)
{
	Glib::TimeVal tv;
	tv.assign_current_time();
	m_logmessage(tv, lvl, msg);
	std::cerr << msg << std::endl;
}

bool Sensors::log_is_open(void) const
{
	return m_logfile.is_open();
}

void Sensors::log_internal_open(void)
{
	log_internal_close();
	std::string logdir(Glib::build_filename(FPlan::get_userdbpath(), "logs"));
	if (!Glib::file_test(logdir, Glib::FILE_TEST_EXISTS) || !Glib::file_test(logdir, Glib::FILE_TEST_IS_DIR))
                g_mkdir_with_parents(logdir.c_str(), 0755);
	m_logtime.assign_current_time();
	std::string logfile(Glib::build_filename(logdir, "log_" + m_logtime.as_iso8601()));
	m_logfile.open(logfile.c_str(), std::ofstream::out | std::ofstream::app);
	m_logtime.subtract_seconds(1);
	{
		std::string hdr1("Time,Position,,Altitude,,,,,Atmosphere,,,,,Track,,"
				 ",,,,Attitude,,,,Engine,,,,,"
				 "Navigation,,,,,,,,,,,,,,,,,,,,,,,");
		std::string hdr2("Time,Lat,Lon,PressAlt,TrueAlt,DensityAlt,VS,AltMode,QNH,Temp,OAT,TAS,IASMode,CRSM,CRST,"
				 "GS,MHDG,HDGM,HDGT,Pitch,Bank,Slip,Rate,RPM,MP,BHP,Fuel,FF,BHP,"
				 "NavMode,FltTmrRun,FltTime,WptTime,CurWpt,CurLat,CurLon,CurTrk,CurTrkErr,"
				 "CurDist,CurXTK,MaxXTK,ROC,NextLat,NextLon,NextTrk,NextTrkErr,HoldTrk,"
				 "Brg2Lat,Brg2Lon,Brg2TrkTrue,Brg2TrkMag,Brg2Dist,WptTime");
		for (sensors_t::const_iterator si(m_sensors.begin()), se(m_sensors.end()); si != se; ++si) {
			const Glib::RefPtr<SensorInstance> sensor(*si);
			std::string hdr(sensor->logfile_header());
			if (hdr.empty())
				continue;
			unsigned int cnt(std::count(hdr.begin(), hdr.end(), ','));
			if (!cnt)
				continue;
			hdr1 += "," + sensor->get_name() + std::string(cnt - 1, ',');
			hdr2 += hdr;
		}
		m_logfile << hdr1 << std::endl << hdr2 << std::endl;
	}
}

void Sensors::log_internal_close(void)
{
	m_logfile.close();
	m_logtime = Glib::TimeVal(std::numeric_limits<long>::max(), 0);
}

void Sensors::log_open(void)
{
	log_internal_open();
	get_configfile().set_integer("main", "logfile", log_is_open());
}

void Sensors::log_close(void)
{
	log_internal_close();
	get_configfile().set_integer("main", "logfile", 0);
}

void Sensors::log_writerecord(void)
{
	if (!log_is_open())
		return;
	m_logfile << m_logtime.as_iso8601() << ',';
	if (m_positionok && !m_position.is_invalid()) {
		std::ostringstream oss;
		oss << std::setprecision(6) << std::fixed << m_position.get_lat_deg_dbl() << ','
		    << m_position.get_lon_deg_dbl();
		m_logfile << oss.str();
	} else {
		m_logfile << ',';
	}
	{
		std::ostringstream oss;
		float palt(m_airdata.get_pressure_altitude());
		float talt(m_airdata.get_true_altitude());
		float dalt(m_airdata.get_density_altitude());
		float qnh(m_airdata.get_qnh());
		float temp(m_airdata.get_temp());
		float oat(m_airdata.get_oat());
		float tas(m_airdata.get_tas());
		oss << std::setprecision(2) << std::fixed << ',';
		if (!std::isnan(palt))
			oss << palt;
		oss << ',';
		if (!std::isnan(talt))
			oss << talt;
		oss << ',';
		if (!std::isnan(dalt))
			oss << dalt;
		oss << ',';
		if (!std::isnan(m_altrate))
			oss << m_altrate;
		oss << ',' << (m_altimeterstd ? "STD" : "QNH") << ',' << std::setprecision(1);
		if (!std::isnan(qnh))
			oss << qnh;
		oss << ',';
		if (!std::isnan(temp))
			oss << temp;
		oss << ',';
		if (!std::isnan(oat))
			oss << oat;
		oss << ',';
		if (!std::isnan(tas))
			oss << tas;
		oss << ',';
		switch (m_iasmode) {
		case iasmode_rpm_mp:
			oss << 'R';
			break;

		case iasmode_bhp_rpm:
			oss << 'B';
			break;

		default:
			oss << '-';
			break;
		}
		m_logfile << oss.str();
	}
	{
		std::ostringstream oss;
		oss << std::setprecision(1) << std::fixed << ',';
		if (!std::isnan(m_crsmag))
			oss << m_crsmag;
		oss << ',';
		if (!std::isnan(m_crstrue))
			oss << m_crstrue;
		oss << ',';
		if (!std::isnan(m_groundspeed))
			oss << m_groundspeed;
		oss << ',';
		if (m_manualhdgtrue && !std::isnan(m_manualhdg))
			oss << m_manualhdg;
		oss << ',';
		if (!std::isnan(m_hdgmag))
			oss << m_hdgmag;
		oss << ',';
		if (!std::isnan(m_hdgtrue))
			oss << m_hdgtrue;
		m_logfile << oss.str();
	}
	{
		std::ostringstream oss;
		oss << std::setprecision(2) << std::fixed << ',';
		if (!std::isnan(m_pitch))
			oss << m_pitch;
		oss << ',';
		if (!std::isnan(m_bank))
			oss << m_bank;
		oss << ',';
		if (!std::isnan(m_slip))
			oss << m_slip;
		oss << ',';
		if (!std::isnan(m_rate))
			oss << m_rate;
		m_logfile << oss.str();
	}
	{
		std::ostringstream oss;
		oss << std::setprecision(1) << std::fixed << ',';
		if (!std::isnan(m_cruise.get_rpm()))
			oss << m_cruise.get_rpm();
		oss << ',';
		if (!std::isnan(m_cruise.get_mp()))
			oss << m_cruise.get_mp();
		oss << ',';
		if (!std::isnan(m_cruise.get_bhp()))
			oss << m_cruise.get_bhp();
		oss << ',';
		if (!std::isnan(m_fuel))
			oss << m_fuel;
		oss << ',';
		if (!std::isnan(m_fuelflow))
			oss << m_fuelflow;
		oss << ',';
		if (!std::isnan(m_bhp))
			oss << m_bhp;
		m_logfile << oss.str();
	}
	m_logfile << ',';
	switch (m_navmode) {
	case navmode_off:
		m_logfile << "OFF";
		break;

	case navmode_direct:
		m_logfile << "DCT";
		break;

	case navmode_directhold:
		m_logfile << "DCTHLD";
		break;

	case navmode_fpl:
		m_logfile << "FPL";
		break;

	case navmode_fplhold:
		m_logfile << "FPLHLD";
		break;

	case navmode_finalapproach:
		m_logfile << "FA";
		break;

	default:
		break;
	}
	m_logfile << ',' << (m_navflighttimerunning ? '1' : '0') << ',';
	if (m_navflighttime) {
		Glib::DateTime dt(Glib::DateTime::create_now_utc(m_navflighttime));
                m_logfile << dt.format("%Y-%m-%d %H:%M:%S");
	}
	m_logfile << ',';
	if (m_navlastwpttime) {
		Glib::DateTime dt(Glib::DateTime::create_now_utc(m_navlastwpttime));
                m_logfile << dt.format("%Y-%m-%d %H:%M:%S");
	}
	m_logfile << ',' << m_navcurwpt << ',';
	if (m_navcur.is_invalid()) {
		m_logfile << ',';
	} else {
		std::ostringstream oss;
		oss << std::setprecision(6) << std::fixed << m_navcur.get_lat_deg_dbl() << ','
		    << m_navcur.get_lon_deg_dbl();
		m_logfile << oss.str();
	}
	{
		std::ostringstream oss;
		oss << ',' << std::setprecision(1) << std::fixed << (m_navcurtrack * track_to_deg)
		    << ',' << (m_navcurtrackerr * track_to_deg)
		    << ',' << m_navcuralt << ',';
		if (!std::isnan(m_navcurdist))
			oss << m_navcurdist;
		oss << ',';
		if (!std::isnan(m_navcurxtk))
			oss << m_navcurxtk;
		oss << ',';
		if (!std::isnan(m_navmaxxtk))
			oss << m_navmaxxtk;
		oss << ',';
		if (!std::isnan(m_navroc))
			oss << m_navroc;
		oss << ',';
		m_logfile << oss.str();
	}
	if (m_navnext.is_invalid()) {
		m_logfile << ',';
	} else {
		std::ostringstream oss;
		oss << std::setprecision(6) << std::fixed << m_navnext.get_lat_deg_dbl() << ','
		    << m_navnext.get_lon_deg_dbl();
		m_logfile << oss.str();
	}
	{
		std::ostringstream oss;
		oss << ',' << std::setprecision(1) << std::fixed << (m_navnexttrack * track_to_deg)
		    << ',' << (m_navnexttrackerr * track_to_deg)
		    << ',' << (m_navholdtrack * track_to_deg)
		    << ',';
		m_logfile << oss.str();
	}
	if (m_navbrg2ena) {
		if (m_navbrg2.is_invalid()) {
			m_logfile << ',';
		} else {
			std::ostringstream oss;
			oss << std::setprecision(6) << std::fixed << m_navbrg2.get_lat_deg_dbl() << ','
			    << m_navbrg2.get_lon_deg_dbl();
			m_logfile << oss.str();
		}
		{
			std::ostringstream oss;
			oss << ',' << std::setprecision(1) << std::fixed;
			if (!std::isnan(m_navbrg2tracktrue))
				oss << m_navbrg2tracktrue;
			oss << ',';
			if (!std::isnan(m_navbrg2trackmag))
				oss << m_navbrg2trackmag;
			oss << ',';
			if (!std::isnan(m_navbrg2dist))
				oss << m_navbrg2dist;
			oss << ',';
			m_logfile << oss.str();
		}
	} else {
		m_logfile << ",,,,,";
	}
	m_logfile << ',';
	if (!std::isnan(m_navtimetowpt)) {
		std::ostringstream oss;
		oss << std::setprecision(1) << std::fixed << m_navtimetowpt;
		m_logfile << oss.str();
	}
	for (sensors_t::const_iterator si(m_sensors.begin()), se(m_sensors.end()); si != se; ++si)
		m_logfile << (*si)->logfile_records();
	m_logfile << std::endl;
}
