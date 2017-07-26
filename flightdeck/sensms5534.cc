//
// C++ Implementation: MS5534 Sensor
//
// Description: MS5534 Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <cstring>
#include <iomanip>

#include "sensms5534.h"
#include "sensgpskingftdi.h"
#include "fplan.h"

Sensors::SensorMS5534::SensorMS5534(Sensors& sensors, const Glib::ustring& configgroup)
	: SensorInstance(sensors, configgroup),	m_rawalt(std::numeric_limits<double>::quiet_NaN()),
	  m_altfilter(1.0), m_altratefilter(0.05), m_press(std::numeric_limits<double>::quiet_NaN()),
	  m_temp(std::numeric_limits<double>::quiet_NaN()), m_alt(std::numeric_limits<double>::quiet_NaN()),
	  m_altrate(std::numeric_limits<double>::quiet_NaN()), m_priority(2)
{
	memset(m_cal, 0, sizeof(m_cal));
	// get configuration
        const Glib::KeyFile& cf(get_sensors().get_configfile());
        if (!cf.has_key(get_configgroup(), "priority"))
		get_sensors().get_configfile().set_integer(get_configgroup(), "priority", m_priority);
       	m_priority = cf.get_integer(get_configgroup(), "priority");
        if (!cf.has_key(get_configgroup(), "altfilter"))
		get_sensors().get_configfile().set_double(get_configgroup(), "altfilter", m_altfilter);
       	m_altfilter = cf.get_double(get_configgroup(), "altfilter");
        if (!cf.has_key(get_configgroup(), "vsfilter"))
		get_sensors().get_configfile().set_double(get_configgroup(), "vsfilter", m_altratefilter);
       	m_altratefilter = cf.get_double(get_configgroup(), "vsfilter");
}

Sensors::SensorMS5534::~SensorMS5534()
{
}

void Sensors::SensorMS5534::init(void)
{
	for (unsigned int i = 0; i < get_sensors().size(); ++i) {
		SensorKingGPSFTDI *sens(dynamic_cast<SensorKingGPSFTDI *>(&(get_sensors()[i])));
		if (!sens)
			continue;
		sens->signal_ms5534().connect(sigc::mem_fun(*this, &SensorMS5534::ms5534_result));
		std::ostringstream oss;
		oss << "MS5534: connecting to sensor " << sens->get_name() << ' ' << sens->get_description();
		get_sensors().log(Sensors::loglevel_notice, oss.str());
	}
}

void Sensors::SensorMS5534::get_baroalt(double& alt, double& altrate) const
{
	alt = m_alt;
	altrate = m_altrate;
}

unsigned int Sensors::SensorMS5534::get_baroalt_priority(void) const
{
	return m_priority;
}

Glib::TimeVal Sensors::SensorMS5534::get_baroalt_time(void) const
{
	return m_time;
}

void Sensors::SensorMS5534::ms5534_result(uint64_t cal, uint16_t press, uint16_t temp)
{
	Glib::TimeVal tdiff(m_time);
	m_time.assign_current_time();
	tdiff -= m_time;
	if (false)
		std::cerr << "ms5534_result: new result: press " << press << " temp " << temp << " time since last " << -tdiff.as_double() << std::endl;
	ParamChanged pc;
	{
		int16_t ncal[6];
		ncal[0] = (cal >> 49) & 0x7FFF;
		ncal[4] = (cal >> 38) & 0x7FF;
		ncal[5] = (cal >> 32) & 0x3F;
		ncal[3] = (cal >> 22) & 0x3FF;
		ncal[2] = (cal >> 6) & 0x3FF;
		ncal[1] = ((cal >> 10) & 0xFC0) | (cal & 0x03F);
		for (unsigned int i = 0; i < 6; ++i) {
			if (m_cal[i] != ncal[i])
				pc.set_changed(parnrc1 + i);
			m_cal[i] = ncal[i];
		}
	}
	pc.set_changed(parnralt, parnrtemp);
	if (press == (uint16_t)~0 || temp == (uint16_t)~0) {
		m_rawalt = m_alt = m_altrate = m_altfilter = m_altratefilter = m_temp = m_press =
			std::numeric_limits<double>::quiet_NaN();
		update(pc);
		return;
	}
	int32_t UT1(m_cal[4] * 8 + 20224);
	double dT(temp);
	dT -= UT1;
	if (temp >= UT1) {
		m_temp = 20.0 + dT * (m_cal[5] + 50) * (0.1 / (1 << 10));
	} else {
		dT -= dT * dT * (1.0 / (1 << 16));
		m_temp = 20.0 + dT * (m_cal[5] + 54) * (0.1 / (1 << 10));
	}
	{
		double OFF(m_cal[1] * 4 + (m_cal[3] - 512) * dT * (1.0 / (1 << 12)));
		double SENS(m_cal[0] + m_cal[2] * dT * (1.0 / (1 << 10)) + 24576);
		int32_t pdiff(press);
		pdiff -= 7168;
		double p(SENS * pdiff * (1.0 / (1 << 14)) - OFF);
		m_press = p * (1.0 / (1 << 5)) + 250;
	}
	{
		Glib::TimeVal tdiff2(tdiff);
		tdiff2.add_seconds(10);
		float a(0);
		m_icao.pressure_to_altitude(&a, 0, m_press);
		a *= FPlanWaypoint::m_to_ft;
		if (std::isnan(m_alt) || tdiff2.negative())
			m_alt = a;
		else
			m_alt += (a - m_alt) * m_altfilter;
		double adiff(a - m_rawalt);
		m_rawalt = a;
		if (tdiff.negative()) {
			adiff *= (-60.0) / tdiff.as_double();
			if (std::isnan(m_altrate) || tdiff2.negative())
				m_altrate = adiff;
			else
				m_altrate += (adiff - m_altrate) * m_altratefilter;
		} else {
			m_altrate = std::numeric_limits<double>::quiet_NaN();
		}
	}
	update(pc);
}

void Sensors::SensorMS5534::get_param_desc(unsigned int pagenr, paramdesc_t& pd)
{
	SensorInstance::get_param_desc(pagenr, pd);
	if (pagenr)
		return;

	pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Measurement", "Current Measurement", ""));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnralt, "Altitude", "Baro Altitude", "ft", 1, -99999.0, 99999.0, 1.0, 10.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnraltrate, "Climb", "Climb Rate", "ft/min", 1, -9999.0, 9999.0, 1.0, 10.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrpress, "Pressure", "Sensor Pressure", "hPa", 2, 0.0, 1999.0, 1.0, 10.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrtemp, "Temperature", "Sensor Temperature", "°C", 1, -99.0, 99.0, 1.0, 10.0));

	pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Priorities", "Priority Values", ""));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrprio, "Alt Prio", "Altitude Priority", "", 0, 0, 9999, 1, 10));

	pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Filter", "Filter Coefficients", ""));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnraltfilter, "Alt Filter", "Altitude Filter", "", 3, 0.0, 1.0, 0.01, 0.1));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnraltratefilter, "VS Filter", "Vertical Speed (Altitude Rate) Filter", "", 3, 0.0, 1.0, 0.01, 0.1));

	pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Calibration", "Calibration Values", ""));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_readonly, parnrc1, "C1", "Pressure Sensitivity", "", 0, 0, 32767, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_readonly, parnrc2, "C2", "Pressure Offset", "", 0, 0, 32767, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_readonly, parnrc3, "C3", "Temperature Coefficient of Pressure Sensitivity", "", 0, 0, 32767, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_readonly, parnrc4, "C4", "Temperature Coefficient of Pressure Offset", "", 0, 0, 32767, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_readonly, parnrc5, "C5", "Reference Temperature", "", 0, 0, 32767, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_readonly, parnrc6, "C6", "Temperature Coefficient of Temperature", "", 0, 0, 32767, 1, 10));
}

Sensors::SensorMS5534::paramfail_t Sensors::SensorMS5534::get_param(unsigned int nr, double& v) const
{
	switch (nr) {
	default:
		break;

	case parnraltfilter:
		v = m_altfilter;
		return paramfail_ok;

	case parnraltratefilter:
		v = m_altratefilter;
		return paramfail_ok;

	case parnralt:
	{
		if (!is_baroalt_ok()) {
			v = std::numeric_limits<double>::quiet_NaN();
			return paramfail_fail;
		}
		double v1;
		get_baroalt(v, v1);
		return std::isnan(v) ? paramfail_fail : paramfail_ok;
	}

	case parnraltrate:
	{
		if (!is_baroalt_ok()) {
			v = std::numeric_limits<double>::quiet_NaN();
			return paramfail_fail;
		}
		double v1;
		get_baroalt(v1, v);
		return std::isnan(v) ? paramfail_fail : paramfail_ok;
	}

	case parnrpress:
	{
		if (!is_baroalt_ok()) {
			v = std::numeric_limits<double>::quiet_NaN();
			return paramfail_fail;
		}
		v = m_press;
		return std::isnan(v) ? paramfail_fail : paramfail_ok;
	}

	case parnrtemp:
	{
		if (!is_baroalt_ok()) {
			v = std::numeric_limits<double>::quiet_NaN();
			return paramfail_fail;
		}
		v = m_temp;
		return std::isnan(v) ? paramfail_fail : paramfail_ok;
	}

	}
	return SensorInstance::get_param(nr, v);
}

Sensors::SensorMS5534::paramfail_t Sensors::SensorMS5534::get_param(unsigned int nr, int& v) const
{
	switch (nr) {
	default:
		break;

	case parnrprio:
		v = m_priority;
		return paramfail_ok;

	case parnrc1:
	case parnrc2:
	case parnrc3:
	case parnrc4:
	case parnrc5:
	case parnrc6:
		v = m_cal[nr - parnrc1];
		return is_baroalt_ok() ? paramfail_ok : paramfail_fail;
	}
	return SensorInstance::get_param(nr, v);
}

void Sensors::SensorMS5534::set_param(unsigned int nr, int v)
{
	switch (nr) {
	default:
		SensorInstance::set_param(nr, v);
		return;

	case parnrprio:
		if (m_priority == (unsigned int)v)
			return;
		m_priority = v;
		get_sensors().get_configfile().set_integer(get_configgroup(), "priority", m_priority);
		break;
	}
	ParamChanged pc;
	pc.set_changed(nr);
	update(pc);
}

void Sensors::SensorMS5534::set_param(unsigned int nr, double v)
{
	switch (nr) {
	default:
		SensorInstance::set_param(nr, v);
		return;

	case parnraltfilter:
		if (m_altfilter == v)
			return;
		m_altfilter = v;
		get_sensors().get_configfile().set_double(get_configgroup(), "altfilter", m_altfilter);
		break;

	case parnraltratefilter:
		if (m_altratefilter == v)
			return;
		m_altratefilter = v;
		get_sensors().get_configfile().set_double(get_configgroup(), "vsfilter", m_altratefilter);
		break;
	}
	ParamChanged pc;
	pc.set_changed(nr);
	update(pc);
}

std::string Sensors::SensorMS5534::logfile_header(void) const
{
	return SensorInstance::logfile_header() + ",Alt,VS,Press,Temp";
}

std::string Sensors::SensorMS5534::logfile_records(void) const
{
	std::ostringstream oss;
	oss << SensorInstance::logfile_records() << std::fixed << std::setprecision(1) << ',';
	if (!std::isnan(m_alt))
		oss << m_alt;
	oss << ',';
	if (!std::isnan(m_altrate))
		oss << m_altrate;
	oss << ',';
	if (!std::isnan(m_press))
		oss << m_press;
	oss << ',';
	if (!std::isnan(m_temp))
		oss << m_temp;
	return oss.str();
}
