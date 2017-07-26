//
// C++ Implementation: LibLocation GPS Sensor
//
// Description: LibLocation GPS Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "senslibloc.h"

Sensors::SensorLibLocation::SensorLibLocation(Sensors& sensors, const Glib::ustring& configgroup)
	: SensorGPS(sensors, configgroup), m_control(0), m_device(0), m_sighandler(0)
{
	m_control = (LocationGPSDControl *)g_object_new(LOCATION_TYPE_GPSD_CONTROL, NULL);
        location_gpsd_control_start(m_control);
        m_device = (LocationGPSDevice *)g_object_new(LOCATION_TYPE_GPS_DEVICE, NULL);
        m_sighandler = g_signal_connect(m_device, "changed", G_CALLBACK(SensorLibLocation::location_changed), this);
        location_gps_device_start(m_device);	
}

Sensors::SensorLibLocation::~SensorLibLocation()
{
        location_gps_device_stop(m_device);
        g_signal_handler_disconnect(m_device, m_sighandler);
        g_object_unref(m_device);
        m_device = 0;
        m_sighandler = 0;
        location_gpsd_control_stop(m_control);
        g_object_unref(m_control);
        m_control = 0;
}

void Sensors::SensorLibLocation::location_changed(LocationGPSDevice *device, gpointer userdata)
{
	static_cast<SensorLibLocation *>(userdata)->loc_changed(device);
}

void Sensors::SensorLibLocation::loc_changed(LocationGPSDevice *device)
{
	ParamChanged pc;
        if (m_device->status == LOCATION_GPS_DEVICE_STATUS_NO_FIX:) {
		m_fixtype = fixtype_nofix;
	} else {
		switch (m_device->fix->mode) {
		case LOCATION_GPS_DEVICE_MODE_NOT_SEEN:
		case LOCATION_GPS_DEVICE_MODE_NO_FIX:
		default:
			m_fixtype = fixtype_nofix;
			break;

		case LOCATION_GPS_DEVICE_MODE_2D:
			m_fixtype = fixtype_2d;
			break;

		case LOCATION_GPS_DEVICE_MODE_3D:
			m_fixtype = fixtype_3d;
			if (m_device->status == LOCATION_GPS_DEVICE_STATUS_DGPS_FIX)
				m_fixtype = fixtype_3d_diff;
			break;
		}
	}
	pc.set_changed(parnrepsilontime, parnrepsiloncrs);
        update(pc);	
}

Glib::TimeVal Sensors::SensorLibLocation::to_timeval(double x)
{
	Glib::TimeVal tv;
	tv.tv_sec = x;
	x -= tv.tv_sec;
	tv.tv_usec = x * 1000000;
	return tv;
}

bool Sensors::SensorLibLocation::get_position(Point& pt) const
{
	if (m_fixtype < fixtype_2d) {
		pt = Point();
		return false;
	}
	pt = Point((int32_t)(m_device->fix->longitude * Point::from_deg_dbl),
		   (int32_t)(m_device->fix->latitude * Point::from_deg_dbl));
	return true;
}

Glib::TimeVal Sensors::SensorLibLocation::get_position_time(void) const
{
	if (m_fixtype < fixtype_2d)
		return Glib::TimeVal();
	return to_timeval(m_device->fix->time);
}

void Sensors::SensorLibLocation::get_truealt(double& alt, double& altrate) const
{
	alt = altrate = std::numeric_limits<double>::quiet_NaN();
	if (m_fixtype < fixtype_3d)
		return;
	alt = m_device->fix->altitude * Point::m_to_ft_dbl;
	altrate = m_device->fix->climb * (Point::m_to_ft_dbl * 60.0);
}

Glib::TimeVal Sensors::SensorLibLocation::get_truealt_time(void) const
{
	return get_position_time();
}

void Sensors::SensorLibLocation::get_course(double& crs, double& gs) const
{
	crs = gs = std::numeric_limits<double>::quiet_NaN();
	if (m_fixtype < fixtype_2d)
		return;
	crs = m_device->fix->track; // track is in degrees
	gs = m_device->fix->speed * Point::mps_to_kts_dbl;
}

Glib::TimeVal Sensors::SensorLibLocation::get_course_time(void) const
{
	return get_position_time();
}

void Sensors::SensorLibLocation::get_param_desc(unsigned int pagenr, paramdesc_t& pd)
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
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrepsilonhor, "Horizontal", "Horizontal Uncertainty", "m", 3, -999999, 999999, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrepsilonalt, "Altitude", "Altitude Uncertainty", "m", 3, -999999, 999999, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrepsiloncrs, "Track", "Track Uncertainty", "°", 3, -999999, 999999, 0.0, 0.0));
}

Sensors::SensorLibLocation::paramfail_t Sensors::SensorLibLocation::get_param(unsigned int nr, double& v) const
{
	switch (nr) {
	default:
		break;
   
	case parnrepsilontime:
		if (!m_device || !m_device->fix)
			return paramfail_fail;
		v = m_device->fix->ept;
		return m_fixtype < fixtype_2d ? paramfail_fail : paramfail_ok;

	case parnrepsilonhor:
		if (!m_device || !m_device->fix)
			return paramfail_fail;
		v = m_device->fix->eph;
		return m_fixtype < fixtype_2d ? paramfail_fail : paramfail_ok;

	case parnrepsilonvert:
		if (!m_device || !m_device->fix)
			return paramfail_fail;
		v = m_device->fix->epv;
		return m_fixtype < fixtype_2d ? paramfail_fail : paramfail_ok;

	case parnrepsiloncrs:
		if (!m_device || !m_device->fix)
			return paramfail_fail;
		v = m_device->fix->epc;
		return m_fixtype < fixtype_2d ? paramfail_fail : paramfail_ok;
	}
	return SensorGPS::get_param(nr, v);
}

Sensors::SensorLibLocation::paramfail_t Sensors::SensorLibLocation::get_param(unsigned int nr, satellites_t& sat) const
{
	if (!m_device)
		return paramfail_fail;
	sat.clear();
	for (int i = 0; i < m_device->satellites_in_view; i++) {
		LocationGPSDeviceSatellite *sat(static_cast<LocationGPSDeviceSatellite *>(g_ptr_array_index(m_device->satellites, i)));
		sat.push_back(Satellite(sat->prn, sat->azimuth, sat->elevation, sat->signal_strength, !!sat->in_use));
	}
	return paramfail_fail;
}

std::string Sensors::SensorLibLocation::logfile_header(void) const
{
	return SensorGPS::logfile_header() + ",FixTime,Lat,Lon,Alt,VS,CRS,GS";
}

std::string Sensors::SensorLibLocation::logfile_records(void) const
{
	std::ostringstream oss;
	oss << SensorGPS::logfile_records() << std::fixed << std::setprecision(6) << ',';
	if (m_device && m_fixtype >= fixtype_2d) {
                Glib::DateTime dt(Glib::DateTime::create_now_utc(to_timeval(m_device->fix->time)));
                oss << dt.format("%Y-%m-%d %H:%M:%S");
	}
	oss << ',';
	if (!m_device || m_fixtype < fixtype_2d || std::isnan(m_device->fix->latitude) || std::isnan(m_device->fix->longitude))
		oss << ',';
	else
		oss << m_device->fix->latitude << ',' << m_device->fix->longitude;
	oss << ',';
	if (m_device && m_fixtype >= fixtype_2d && !std::isnan(m_device->fix->altitude))
		oss << m_device->fix->altitude * Point::m_to_ft_dbl;
	oss << ',';
	if (m_device && m_fixtype >= fixtype_2d && !std::isnan(m_device->fix->climb))
		oss << m_device->fix->climb * (Point::m_to_ft_dbl * 60.0);
	oss << ',';
	if (m_device && m_fixtype >= fixtype_2d && !std::isnan(m_device->fix->track))
		oss << m_device->fix->track;
	oss << ',';
	if (m_device && m_fixtype >= fixtype_2d && !std::isnan(m_device->fix->speed))
		oss << m_device->fix->speed * Point::mps_to_kts_dbl;
	return oss.str();
}
