//
// C++ Implementation: gpsd Daemon GPS Sensor
//
// Description: GPS Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013, 2015
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <limits>
#include <cstring>
#include <iomanip>
#include "sensgpsd.h"

Sensors::SensorGPSD::SensorGPSD(Sensors& sensors, const Glib::ustring& configgroup)
	: SensorGPS(sensors, configgroup), m_gpsopen(false)
{
	m_gpsdata.m_this = this;
	const Glib::KeyFile& cf(get_sensors().get_configfile());
        if (!cf.has_key(get_configgroup(), "server"))
		get_sensors().get_configfile().set_string(get_configgroup(), "server", "localhost");
	m_servername = cf.get_string(get_configgroup(), "server");
        if (!cf.has_key(get_configgroup(), "port"))
		get_sensors().get_configfile().set_string(get_configgroup(), "port", "2947");
	m_serverport = cf.get_string(get_configgroup(), "port");
	try_connect();
}

Sensors::SensorGPSD::~SensorGPSD()
{
	close();
}

bool Sensors::SensorGPSD::close(void)
{
	m_conn.disconnect();
	m_conntimeout.disconnect();
	bool was_open(m_gpsopen);
	if (was_open)
		gps_close(&m_gpsdata.m_gpsdata);
	m_fixtype = fixtype_nofix;
	m_gpsopen = false;
	m_postime = m_alttime = m_crstime = Glib::TimeVal();
	m_pos = Point();
	m_alt = m_altrate = m_crs = m_groundspeed = std::numeric_limits<double>::quiet_NaN();
	return was_open;
}

#if GPSD_API_MAJOR_VERSION >= 5
#define gps_open_r gps_open
#endif

bool Sensors::SensorGPSD::try_connect(void)
{
	if (close()) {
		ParamChanged pc;
		pc.set_changed(parnrfixtype, parnrgroundspeed);
		pc.set_changed(parnrstart, parnrend - 1);
		update(pc);
	}
	memset(&m_gpsdata.m_gpsdata, 0, sizeof(m_gpsdata.m_gpsdata));
	if (gps_open_r(m_servername.c_str(), m_serverport.c_str(), &m_gpsdata.m_gpsdata)) {
		std::ostringstream oss;
		oss << "gpsd: cannot connect " << m_servername << ':' << m_serverport;
		get_sensors().log(Sensors::loglevel_warning, oss.str());
		m_conn = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorGPSD::try_connect), 15);
		return true;
	}
	m_gpsopen = true;
#if GPSD_API_MAJOR_VERSION < 5
	gps_set_raw_hook(&m_gpsdata.m_gpsdata, &SensorGPSD::gps_hook);
#endif
	m_conn = Glib::signal_io().connect(sigc::mem_fun(*this, &SensorGPSD::gps_poll), m_gpsdata.m_gpsdata.gps_fd, Glib::IO_IN);
	gps_stream(&m_gpsdata.m_gpsdata, WATCH_ENABLE, NULL);
	ParamChanged pc;
	pc.set_changed(parnrfixtype, parnrgroundspeed);
	pc.set_changed(parnrstart, parnrend - 1);
	update(pc);
	m_conntimeout = Glib::signal_timeout().connect(sigc::mem_fun(*this, &SensorGPSD::gps_timeout), 5000);
	return false;
}

void Sensors::SensorGPSD::gps_hook(struct gps_data_t *p, char *, size_t)
{
	mygpsdata_t *p1 = reinterpret_cast<mygpsdata_t *>(p);
	p1->m_this->update_gps();
}

bool Sensors::SensorGPSD::gps_poll(Glib::IOCondition iocond)
{
        if (!(iocond & Glib::IO_IN))
                return true;
#if GPSD_API_MAJOR_VERSION >= 5
	if (gps_read(&m_gpsdata.m_gpsdata) >= 0) {
		update_gps();
	        return true;
	}
#else
	if (!::gps_poll(&m_gpsdata.m_gpsdata)) {
	        return true;
	}
#endif
	try_connect();
	return false;
}

void Sensors::SensorGPSD::update_gps(void)
{
	if (true)
		std::cerr << "GPS: set 0x" << std::hex << m_gpsdata.m_gpsdata.set << std::dec << " status " << m_gpsdata.m_gpsdata.status
			  << " fixmode " << m_gpsdata.m_gpsdata.fix.mode << std::endl;
	m_conntimeout.disconnect();
	m_conntimeout = Glib::signal_timeout().connect(sigc::mem_fun(*this, &SensorGPSD::gps_timeout), 5000);
	ParamChanged pc;
	if (m_gpsdata.m_gpsdata.set & MODE_SET) {
		fixtype_t oldfixtype(m_fixtype);
		if ((m_gpsdata.m_gpsdata.set & STATUS_SET) && 
		    m_gpsdata.m_gpsdata.status == STATUS_NO_FIX) {
			m_fixtype = fixtype_nofix;
		} else {
			switch (m_gpsdata.m_gpsdata.fix.mode) {
			case MODE_NOT_SEEN:
			case MODE_NO_FIX:
			default:
				m_fixtype = fixtype_nofix;
				break;

			case MODE_2D:
				m_fixtype = fixtype_2d;
				break;

			case MODE_3D:
				m_fixtype = fixtype_3d;
#if GPSD_API_MAJOR_VERSION < 6
				if (m_gpsdata.m_gpsdata.status == STATUS_DGPS_FIX &&
				    (m_gpsdata.m_gpsdata.set & STATUS_SET))
					m_fixtype = fixtype_3d_diff;
#endif
				break;
			}
		}
		if (oldfixtype != m_fixtype) {
			pc.set_changed(parnrfixtype, parnrsatellites);
			pc.set_changed(parnrepsilontime, parnrepsilongroundspeed);
		}
	}
	if (m_gpsdata.m_gpsdata.set & LATLON_SET) {
		m_pos = Point((int32_t)(m_gpsdata.m_gpsdata.fix.longitude * Point::from_deg_dbl),
			      (int32_t)(m_gpsdata.m_gpsdata.fix.latitude * Point::from_deg_dbl));
		m_postime.assign_current_time();
		pc.set_changed(parnrtime, parnrlon);
		pc.set_changed(parnrepsilontime, parnrepsilonlon);
	}
	if (m_gpsdata.m_gpsdata.set & ALTITUDE_SET) {
		m_alt = m_gpsdata.m_gpsdata.fix.altitude * Point::m_to_ft_dbl;
		m_alttime.assign_current_time();
		pc.set_changed(parnralt);
		pc.set_changed(parnrepsilonalt);
	}
	if (m_gpsdata.m_gpsdata.set & CLIMB_SET) {
		m_altrate = m_gpsdata.m_gpsdata.fix.climb * (Point::m_to_ft_dbl * 60.0);
		pc.set_changed(parnraltrate);
		pc.set_changed(parnrepsilonaltrate);
	}
	if (m_gpsdata.m_gpsdata.set & TRACK_SET) {
		m_crs = m_gpsdata.m_gpsdata.fix.track; // track is in degrees
		m_crstime.assign_current_time();
		pc.set_changed(parnrcourse);
		pc.set_changed(parnrepsiloncourse);
	}
	if (m_gpsdata.m_gpsdata.set & SPEED_SET) {
		m_groundspeed = m_gpsdata.m_gpsdata.fix.speed * Point::mps_to_kts_dbl;
		pc.set_changed(parnrgroundspeed);
		pc.set_changed(parnrepsilongroundspeed);
	}
#if GPSD_API_MAJOR_VERSION >= 6
	if (m_gpsdata.m_gpsdata.set & SATELLITE_SET) {
		std::set<int> satused;
		for (int i = 0; i < m_gpsdata.m_gpsdata.satellites_used; ++i) {
			satused.insert(m_gpsdata.m_gpsdata.skyview[i].used);
			if (true)
				std::cerr << "SV PRN used: " << m_gpsdata.m_gpsdata.skyview[i].used << std::endl;
		}
		satellites_t sat;
		for (int i = 0; i < m_gpsdata.m_gpsdata.satellites_visible; ++i) {
			sat.push_back(Satellite(m_gpsdata.m_gpsdata.skyview[i].PRN, m_gpsdata.m_gpsdata.skyview[i].azimuth, m_gpsdata.m_gpsdata.skyview[i].elevation,
						m_gpsdata.m_gpsdata.skyview[i].ss, satused.find(m_gpsdata.m_gpsdata.skyview[i].PRN) != satused.end()));
			if (true)
				std::cerr << "SV: " << m_gpsdata.m_gpsdata.skyview[i].PRN << std::endl;
		}
		m_sat.swap(sat);
		pc.set_changed(parnrsatellites);
	}
#else
	if (m_gpsdata.m_gpsdata.set & SATELLITE_SET) {
		std::set<int> satused;
		for (int i = 0; i < m_gpsdata.m_gpsdata.satellites_used; ++i) {
			satused.insert(m_gpsdata.m_gpsdata.used[i]);
			if (true)
				std::cerr << "SV PRN used: " << m_gpsdata.m_gpsdata.used[i] << std::endl;
		}
		satellites_t sat;
		for (int i = 0; i < m_gpsdata.m_gpsdata.satellites_visible; ++i) {
			sat.push_back(Satellite(m_gpsdata.m_gpsdata.PRN[i], m_gpsdata.m_gpsdata.azimuth[i], m_gpsdata.m_gpsdata.elevation[i],
						m_gpsdata.m_gpsdata.ss[i], satused.find(m_gpsdata.m_gpsdata.PRN[i]) != satused.end()));
			if (true)
				std::cerr << "SV: " << m_gpsdata.m_gpsdata.PRN[i] << std::endl;
		}
		m_sat.swap(sat);
		pc.set_changed(parnrsatellites);
	}
#endif
	pc.set_changed(parnrdevice);
	update(pc);
}

bool Sensors::SensorGPSD::gps_timeout(void)
{
	m_conntimeout.disconnect();
	m_fixtype = fixtype_nofix;
	ParamChanged pc;
	pc.set_changed(parnrfixtype, parnrsatellites);
	pc.set_changed(parnrepsilontime, parnrepsilongroundspeed);
	update(pc);
	m_sat.clear();
	m_conntimeout = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorGPSD::try_connect), 15);
	return false;
}

Glib::TimeVal Sensors::SensorGPSD::to_timeval(double x)
{
	Glib::TimeVal tv;
	tv.tv_sec = x;
	x -= tv.tv_sec;
	tv.tv_usec = x * 1000000;
	return tv;
}

bool Sensors::SensorGPSD::get_position(Point& pt) const
{
	if (m_fixtype < fixtype_2d) {
		pt = Point();
		return false;
	}
	pt = m_pos;
	return true;
}

Glib::TimeVal Sensors::SensorGPSD::get_position_time(void) const
{
	return m_postime;
}

void Sensors::SensorGPSD::get_truealt(double& alt, double& altrate) const
{
	alt = altrate = std::numeric_limits<double>::quiet_NaN();
	if (m_fixtype < fixtype_3d)
		return;
	alt = m_alt;
	altrate = m_altrate;
}

Glib::TimeVal Sensors::SensorGPSD::get_truealt_time(void) const
{
	return m_alttime;
}

void Sensors::SensorGPSD::get_course(double& crs, double& gs) const
{
	crs = gs = std::numeric_limits<double>::quiet_NaN();
	if (m_fixtype < fixtype_2d)
		return;
	crs = m_crs;
	gs = m_groundspeed;
}

Glib::TimeVal Sensors::SensorGPSD::get_course_time(void) const
{
	return m_crstime;
}

void Sensors::SensorGPSD::get_param_desc(unsigned int pagenr, paramdesc_t& pd)
{
	SensorGPS::get_param_desc(pagenr, pd);
	{
		paramdesc_t::iterator pdi(pd.begin()), pde(pd.end());
		if (pdi != pde)
			++pdi;
		for (; pdi != pde; ++pdi)
			if (pdi->get_type() == ParamDesc::type_section)
				break;
		pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_admin, parnrservername, "Server Name", "Server Name", ""));
		++pdi;
		pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_admin, parnrserverport, "Server Port", "Server Port", ""));
		++pdi;
		pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrdevice, "Device", "Device Path", ""));
	}
	pd.push_back(ParamDesc(ParamDesc::type_satellites, ParamDesc::editable_readonly, parnrsatellites, "Satellites", "Satellite Azimuth/Elevation and Signal Strengths", ""));
	
	pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Uncertainties", "Uncertainties", ""));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrepsilontime, "Time", "Time Uncertainty", "s", 3, -999999, 999999, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrepsilonlat, "Latitude", "Latitude Uncertainty", "m", 3, -999999, 999999, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrepsilonlon, "Longitude", "Longitude Uncertainty", "m", 3, -999999, 999999, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrepsilonalt, "Altitude", "Altitude Uncertainty", "m", 3, -999999, 999999, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrepsilonaltrate, "Vertical Speed", "Vertical Speed Uncertainty", "m/s", 3, -999999, 999999, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrepsiloncourse, "Track", "Track Uncertainty", "°", 3, -999999, 999999, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrepsilongroundspeed, "Ground Speed", "Ground Speed Uncertainty", "m/s", 3, -999999, 999999, 0.0, 0.0));
}

Sensors::SensorGPSD::paramfail_t Sensors::SensorGPSD::get_param(unsigned int nr, Glib::ustring& v) const
{
	switch (nr) {
	default:
		break;

	case parnrservername:
		v = m_servername;
		return paramfail_ok;

	case parnrserverport:
		v = m_serverport;
		return paramfail_ok;

	case parnrdevice:
		v = m_gpsdata.m_gpsdata.dev.path;
		return m_gpsopen ? paramfail_ok : paramfail_fail;
	}
	return SensorGPS::get_param(nr, v);
}

Sensors::SensorGPSD::paramfail_t Sensors::SensorGPSD::get_param(unsigned int nr, double& v) const
{
	switch (nr) {
	default:
		break;
   
	case parnrepsilontime:
		v = m_gpsdata.m_gpsdata.fix.ept;
		return m_fixtype < fixtype_2d ? paramfail_fail : paramfail_ok;

	case parnrepsilonlat:
		v = m_gpsdata.m_gpsdata.fix.epy;
		return m_fixtype < fixtype_2d ? paramfail_fail : paramfail_ok;

	case parnrepsilonlon:
		v = m_gpsdata.m_gpsdata.fix.epx;
		return m_fixtype < fixtype_2d ? paramfail_fail : paramfail_ok;

	case parnrepsilonalt:
		v = m_gpsdata.m_gpsdata.fix.epv;
		return m_fixtype < fixtype_2d ? paramfail_fail : paramfail_ok;

	case parnrepsilonaltrate:
		v = m_gpsdata.m_gpsdata.fix.epc;
		return m_fixtype < fixtype_2d ? paramfail_fail : paramfail_ok;

	case parnrepsiloncourse:
		v = m_gpsdata.m_gpsdata.fix.epd;
		return m_fixtype < fixtype_2d ? paramfail_fail : paramfail_ok;

	case parnrepsilongroundspeed:
		v = m_gpsdata.m_gpsdata.fix.eps;
		return m_fixtype < fixtype_2d ? paramfail_fail : paramfail_ok;
	}
	return SensorGPS::get_param(nr, v);
}

Sensors::SensorGPSD::paramfail_t Sensors::SensorGPSD::get_param(unsigned int nr, int& v) const
{
	switch (nr) {
	default:
		break;

	}
	return SensorGPS::get_param(nr, v);
}

Sensors::SensorGPSD::paramfail_t Sensors::SensorGPSD::get_param(unsigned int nr, satellites_t& sat) const
{
	sat = m_sat;
	return m_gpsopen ? paramfail_ok : paramfail_fail;
}

void Sensors::SensorGPSD::set_param(unsigned int nr, const Glib::ustring& v)
{
	switch (nr) {
	default:
		SensorGPS::set_param(nr, v);
		return;

	case parnrservername:
		if (v == m_servername)
			return;
		m_servername = v;
		get_sensors().get_configfile().set_string(get_configgroup(), "server", m_servername);
		try_connect();
		break;

	case parnrserverport:
		if (v == m_serverport)
			return;
		m_serverport = v;
		get_sensors().get_configfile().set_string(get_configgroup(), "port", m_serverport);
		try_connect();
		break;
	}
	ParamChanged pc;
	pc.set_changed(nr);
	update(pc);
}

std::string Sensors::SensorGPSD::logfile_header(void) const
{
	return SensorGPS::logfile_header() + ",FixTime,Lat,Lon,Alt,VS,CRS,GS,EPX,EPY,EPV,EPC,EPD,EPS,NrSat";
}

std::string Sensors::SensorGPSD::logfile_records(void) const
{
	std::ostringstream oss;
	oss << SensorGPS::logfile_records() << std::fixed << std::setprecision(6) << ',';
	if (m_postime.valid() && m_fixtype >= fixtype_2d) {
                Glib::DateTime dt(Glib::DateTime::create_now_utc(m_postime));
                oss << dt.format("%Y-%m-%d %H:%M:%S");
	}
	oss << ',';
	if (m_fixtype < fixtype_2d || m_pos.is_invalid())
		oss << ',';
	else
		oss << m_pos.get_lat_deg_dbl() << ',' << m_pos.get_lon_deg_dbl();
	oss << ',';
	if (!std::isnan(m_alt))
		oss << m_alt;
	oss << ',';
	if (!std::isnan(m_altrate))
		oss << m_altrate;
	oss << ',';
	if (!std::isnan(m_crs))
		oss << m_crs;
	oss << ',';
	if (!std::isnan(m_groundspeed))
		oss << m_groundspeed;
	oss << ',';
	if (m_fixtype >= fixtype_2d && !std::isnan(m_gpsdata.m_gpsdata.fix.epx))
		oss << m_gpsdata.m_gpsdata.fix.epx;
	oss << ',';
	if (m_fixtype >= fixtype_2d && !std::isnan(m_gpsdata.m_gpsdata.fix.epy))
		oss << m_gpsdata.m_gpsdata.fix.epy;
	oss << ',';
	if (m_fixtype >= fixtype_2d && !std::isnan(m_gpsdata.m_gpsdata.fix.epv))
		oss << m_gpsdata.m_gpsdata.fix.epv;
	oss << ',';
	if (m_fixtype >= fixtype_2d && !std::isnan(m_gpsdata.m_gpsdata.fix.epc))
		oss << m_gpsdata.m_gpsdata.fix.epc;
	oss << ',';
	if (m_fixtype >= fixtype_2d && !std::isnan(m_gpsdata.m_gpsdata.fix.epd))
		oss << m_gpsdata.m_gpsdata.fix.epd;
	oss << ',';
	if (m_fixtype >= fixtype_2d && !std::isnan(m_gpsdata.m_gpsdata.fix.eps))
		oss << m_gpsdata.m_gpsdata.fix.eps;
	oss << ',' << m_sat.size();
	return oss.str();
}
