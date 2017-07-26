//
// C++ Implementation: GPS Sensor
//
// Description: GPS Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <glibmm/datetime.h>

#include "sensgps.h"

Sensors::SensorGPS::SensorGPS(Sensors& sensors, const Glib::ustring& configgroup)
	: SensorInstance(sensors, configgroup), m_fixtype(fixtype_nofix)
{
	// get configuration
        const Glib::KeyFile& cf(get_sensors().get_configfile());
        if (!cf.has_key(get_configgroup(), "positionpriority")) {
		static const unsigned int dfltprio[4] = { 0, 2, 3, 4 };
		get_sensors().get_configfile().set_integer_list(get_configgroup(), "positionpriority", std::vector<int>(dfltprio, dfltprio + sizeof(dfltprio)/sizeof(dfltprio[0])));
	}
	{
		Glib::ArrayHandle<int> a(cf.get_integer_list(get_configgroup(), "positionpriority"));
		unsigned int nr(std::min(a.size(), sizeof(m_positionpriority)/sizeof(m_positionpriority[0])));
		for (unsigned int i = 0; i < nr; ++i)
			m_positionpriority[i] = a.data()[i];
		for (; nr < sizeof(m_positionpriority)/sizeof(m_positionpriority[0]); ++nr)
				m_positionpriority[nr] = nr ? m_positionpriority[nr-1] : 0;
	}
        if (!cf.has_key(get_configgroup(), "altitudepriority"))
		get_sensors().get_configfile().set_integer_list(get_configgroup(), "altitudepriority", std::vector<int>(m_positionpriority, m_positionpriority + sizeof(m_positionpriority)/sizeof(m_positionpriority[0])));
	{
		Glib::ArrayHandle<int> a(cf.get_integer_list(get_configgroup(), "altitudepriority"));
		unsigned int nr(std::min(a.size(), sizeof(m_altitudepriority)/sizeof(m_altitudepriority[0])));
		for (unsigned int i = 0; i < nr; ++i)
			m_altitudepriority[i] = a.data()[i];
		for (; nr < sizeof(m_altitudepriority)/sizeof(m_altitudepriority[0]); ++nr)
				m_altitudepriority[nr] = nr ? m_altitudepriority[nr-1] : 0;
	}
        if (!cf.has_key(get_configgroup(), "coursepriority"))
		get_sensors().get_configfile().set_integer_list(get_configgroup(), "coursepriority", std::vector<int>(m_positionpriority, m_positionpriority + sizeof(m_positionpriority)/sizeof(m_positionpriority[0])));
	{
		Glib::ArrayHandle<int> a(cf.get_integer_list(get_configgroup(), "coursepriority"));
		unsigned int nr(std::min(a.size(), sizeof(m_coursepriority)/sizeof(m_coursepriority[0])));
		for (unsigned int i = 0; i < nr; ++i)
			m_coursepriority[i] = a.data()[i];
		for (; nr < sizeof(m_coursepriority)/sizeof(m_coursepriority[0]); ++nr)
				m_coursepriority[nr] = nr ? m_coursepriority[nr-1] : 0;
	}
}

Sensors::SensorGPS::~SensorGPS()
{
}

unsigned int Sensors::SensorGPS::get_position_priority(void) const
{
	return m_positionpriority[m_fixtype];
}

unsigned int Sensors::SensorGPS::get_truealt_priority(void) const
{
	return m_altitudepriority[m_fixtype];
}

unsigned int Sensors::SensorGPS::get_course_priority(void) const
{
	return m_coursepriority[m_fixtype];
}

const std::string& Sensors::SensorGPS::to_str(fixtype_t ft)
{
	switch (ft) {
	default:
	case fixtype_nofix:
	{
		static const std::string r("No Fix");
		return r;
	}

	case fixtype_2d:
	{
		static const std::string r("2D");
		return r;
	}

	case fixtype_3d:
	{
		static const std::string r("3D");
		return r;
	}

	case fixtype_3d_diff:
	{
		static const std::string r("3D Differential");
		return r;
	}

	}
}

void Sensors::SensorGPS::get_param_desc(unsigned int pagenr, paramdesc_t& pd)
{
	SensorInstance::get_param_desc(pagenr, pd);
	if (pagenr)
		return;

	pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Fix", "Current Fix", ""));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrfixtype, "Fix type", "GPS Fix Type", ""));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrtime, "Time UTC", "Time in UTC", ""));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrlat, "Latitude", "Latitude", ""));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrlon, "Longitude", "Longitude", ""));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnralt, "Altitude", "True Altitude", "ft", 0, -99999.0, 99999.0, 1.0, 10.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnraltrate, "Climb", "Climb Rate", "ft/min", 0, -9999.0, 9999.0, 1.0, 10.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrcourse, "Course", "True Course", "°", 0, -999.0, 999.0, 1.0, 10.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrgroundspeed, "Ground Speed", "Ground Speed", "kt", 0, 0.0, 999.0, 1.0, 10.0));

	pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Priorities", "Priority Values", ""));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrposprio0, "Pos Prio No Fix", "Position Priority when GPS has no fix; higher values mean this sensor is preferred to other sensors delivering position information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrposprio1, "Pos Prio 2D", "Position Priority when GPS has 2D fix; higher values mean this sensor is preferred to other sensors delivering position information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrposprio2, "Pos Prio 3D", "Position Priority when GPS has 3D fix; higher values mean this sensor is preferred to other sensors delivering position information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrposprio3, "Pos Prio 3D Diff", "Position Priority when GPS has 3D with Differential Information fix; higher values mean this sensor is preferred to other sensors delivering position information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnraltprio0, "Alt Prio No Fix", "Altitude Priority when GPS has no fix; higher values mean this sensor is preferred to other sensors delivering altitude information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnraltprio1, "Alt Prio 2D", "Altitude Priority when GPS has 2D fix; higher values mean this sensor is preferred to other sensors delivering altitude information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnraltprio2, "Alt Prio 3D", "Altitude Priority when GPS has 3D fix; higher values mean this sensor is preferred to other sensors delivering altitude information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnraltprio3, "Alt Prio 3D Diff", "Altitude Priority when GPS has 3D with Differential Information fix; higher values mean this sensor is preferred to other sensors delivering altitude information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrcrsprio0, "Course Prio No Fix", "Course Priority when GPS has no fix; higher values mean this sensor is preferred to other sensors delivering course information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrcrsprio1, "Course Prio 2D", "Course Priority when GPS has 2D fix; higher values mean this sensor is preferred to other sensors delivering course information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrcrsprio2, "Course Prio 3D", "Course Priority when GPS has 3D fix; higher values mean this sensor is preferred to other sensors delivering course information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrcrsprio3, "Course Prio 3D Diff", "Course Priority when GPS has 3D with Differential Information fix; higher values mean this sensor is preferred to other sensors delivering course information", "", 0, 0, 9999, 1, 10));
}

Sensors::SensorGPS::paramfail_t Sensors::SensorGPS::get_param(unsigned int nr, Glib::ustring& v) const
{
	switch (nr) {
	default:
		break;

	case parnrfixtype:
		v = to_str(m_fixtype);
		std::cerr << "Fixtype: " << v << std::endl;
		return paramfail_ok;

	case parnrtime:
	{
		v.clear();
		if (m_fixtype < fixtype_2d)
			return paramfail_fail;
		Glib::TimeVal t(get_position_time());
		if (!t.valid())
			return paramfail_fail;
		Glib::DateTime dt(Glib::DateTime::create_now_utc(t));
		v = dt.format("%Y-%m-%d %H:%M:%S");
		return paramfail_ok;
	}

	case parnrlat:
	{
		v.clear();
		Point pt;
		if (!is_position_ok() || !get_position(pt))
			return paramfail_fail;
		v = pt.get_lat_str();
		return paramfail_ok;
	}

	case parnrlon:
	{
		v.clear();
		Point pt;
		if (!is_position_ok() || !get_position(pt))
			return paramfail_fail;
		v = pt.get_lon_str();
		return paramfail_ok;
	}

	}
	return SensorInstance::get_param(nr, v);
}

Sensors::SensorGPS::paramfail_t Sensors::SensorGPS::get_param(unsigned int nr, double& v) const
{
	switch (nr) {
	default:
		break;

	case parnralt:
	{
		if (!is_truealt_ok()) {
			v = std::numeric_limits<double>::quiet_NaN();
			return paramfail_fail;
		}
		double v1;
		get_truealt(v, v1);
		return std::isnan(v) ? paramfail_fail : paramfail_ok;
	}

	case parnraltrate:
	{
		if (!is_truealt_ok()) {
			v = std::numeric_limits<double>::quiet_NaN();
			return paramfail_fail;
		}
		double v1;
		get_truealt(v1, v);
		return std::isnan(v) ? paramfail_fail : paramfail_ok;
	}

	case parnrcourse:
	{
		if (!is_course_ok()) {
			v = std::numeric_limits<double>::quiet_NaN();
			return paramfail_fail;
		}
		double v1;
		get_course(v, v1);
		return std::isnan(v) ? paramfail_fail : paramfail_ok;
	}

	case parnrgroundspeed:
	{
		if (!is_course_ok()) {
			v = std::numeric_limits<double>::quiet_NaN();
			return paramfail_fail;
		}
		double v1;
		get_course(v1, v);
		return std::isnan(v) ? paramfail_fail : paramfail_ok;
	}

	}
	return SensorInstance::get_param(nr, v);
}

Sensors::SensorGPS::paramfail_t Sensors::SensorGPS::get_param(unsigned int nr, int& v) const
{
	switch (nr) {
	default:
		break;

	case parnrposprio0:
	case parnrposprio1:
	case parnrposprio2:
	case parnrposprio3:
		v = m_positionpriority[nr - parnrposprio0];
		return paramfail_ok;

	case parnraltprio0:
	case parnraltprio1:
	case parnraltprio2:
	case parnraltprio3:
		v = m_altitudepriority[nr - parnraltprio0];
		return paramfail_ok;

	case parnrcrsprio0:
	case parnrcrsprio1:
	case parnrcrsprio2:
	case parnrcrsprio3:
		v = m_coursepriority[nr - parnrcrsprio0];
		return paramfail_ok;
	}
	return SensorInstance::get_param(nr, v);
}

void Sensors::SensorGPS::set_param(unsigned int nr, int v)
{
	switch (nr) {
	default:
		SensorInstance::set_param(nr, v);
		return;

	case parnrposprio0:
	case parnrposprio1:
	case parnrposprio2:
	case parnrposprio3:
		if (m_positionpriority[nr - parnrposprio0] == (unsigned int)v)
			return;
		m_positionpriority[nr - parnrposprio0] = v;
		get_sensors().get_configfile().set_integer_list(get_configgroup(), "positionpriority", std::vector<int>(m_positionpriority, m_positionpriority + sizeof(m_positionpriority)/sizeof(m_positionpriority[0])));
		break;

	case parnraltprio0:
	case parnraltprio1:
	case parnraltprio2:
	case parnraltprio3:
		if (m_altitudepriority[nr - parnraltprio0] == (unsigned int)v)
			return;
		m_altitudepriority[nr - parnraltprio0] = v;
		get_sensors().get_configfile().set_integer_list(get_configgroup(), "altitudepriority", std::vector<int>(m_altitudepriority, m_altitudepriority + sizeof(m_altitudepriority)/sizeof(m_altitudepriority[0])));
		break;

	case parnrcrsprio0:
	case parnrcrsprio1:
	case parnrcrsprio2:
	case parnrcrsprio3:
		if (m_coursepriority[nr - parnrcrsprio0] == (unsigned int)v)
			return;
		m_coursepriority[nr - parnrcrsprio0] = v;
		get_sensors().get_configfile().set_integer_list(get_configgroup(), "coursepriority", std::vector<int>(m_coursepriority, m_coursepriority + sizeof(m_coursepriority)/sizeof(m_coursepriority[0])));
		break;
	}
	ParamChanged pc;
	pc.set_changed(nr);
	update(pc);
}

std::string Sensors::SensorGPS::logfile_header(void) const
{
	return SensorInstance::logfile_header() + ",FixType";
}

std::string Sensors::SensorGPS::logfile_records(void) const
{
	return SensorInstance::logfile_records() + "," + to_str(m_fixtype);
}
