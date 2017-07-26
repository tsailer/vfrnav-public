//
// C++ Implementation: Geoclue Sensor
//
// Description: Geoclue Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <limits>
#include <iomanip>
#include <geoclue/geoclue-master.h>
#include <glibmm/datetime.h>

#include "sensgeoclue.h"

Sensors::SensorGeoclue::SensorGeoclue(Sensors& sensors, const Glib::ustring& configgroup)
	: SensorInstance(sensors, configgroup), m_client(0), m_pos(0), m_alt(std::numeric_limits<double>::quiet_NaN()), m_coordok(false),
	  m_acclevel(GEOCLUE_ACCURACY_LEVEL_NONE)
{
	// get configuration
        const Glib::KeyFile& cf(get_sensors().get_configfile());
        if (!cf.has_key(get_configgroup(), "positionpriority")) {
		static const unsigned int dfltprio[7] = { 0, 0, 0, 1, 1, 1, 2 };
		get_sensors().get_configfile().set_integer_list(get_configgroup(), "positionpriority", std::vector<int>(dfltprio, dfltprio + sizeof(dfltprio)/sizeof(dfltprio[0])));
	}
	m_positionpriority = cf.get_integer_list(get_configgroup(), "positionpriority");
        if (!cf.has_key(get_configgroup(), "altitudepriority")) {
		static const unsigned int dfltprio[7] = { 0, 0, 0, 1, 1, 1, 2 };
		get_sensors().get_configfile().set_integer_list(get_configgroup(), "altitudepriority", std::vector<int>(dfltprio, dfltprio + sizeof(dfltprio)/sizeof(dfltprio[0])));
	}
	m_altitudepriority = cf.get_integer_list(get_configgroup(), "altitudepriority");
	GeoclueAccuracyLevel minlevel(GEOCLUE_ACCURACY_LEVEL_NONE);
	{
		bool ok(false);
		if (cf.has_key(get_configgroup(), "minlevel")) {
			Glib::ustring ml(cf.get_string(get_configgroup(), "minlevel"));
			static const GeoclueAccuracyLevel acclvls[] = {
				GEOCLUE_ACCURACY_LEVEL_NONE,
				GEOCLUE_ACCURACY_LEVEL_COUNTRY,
				GEOCLUE_ACCURACY_LEVEL_REGION,
				GEOCLUE_ACCURACY_LEVEL_LOCALITY,
				GEOCLUE_ACCURACY_LEVEL_POSTALCODE,
				GEOCLUE_ACCURACY_LEVEL_STREET,
				GEOCLUE_ACCURACY_LEVEL_DETAILED
			};
			for (int i = 0; i < (int)(sizeof(acclvls)/sizeof(acclvls[0])); ++i) {
				if (ml != to_str(acclvls[i]))
					continue;
				minlevel = acclvls[i];
				ok = true;
				break;
			}
		}
		if (!ok)
			get_sensors().get_configfile().set_string(get_configgroup(), "minlevel", to_str(minlevel));
	}
	GeoclueResourceFlags rsrc(GEOCLUE_RESOURCE_ALL);
	{
		bool ok(false);
		if (cf.has_key(get_configgroup(), "resources")) {
			rsrc = GEOCLUE_RESOURCE_NONE;
			ok = true;
		        std::vector<Glib::ustring> r(cf.get_string_list(get_configgroup(), "resources"));
			for (std::vector<Glib::ustring>::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
				if (*ri == "network")
					rsrc = (GeoclueResourceFlags)(rsrc | GEOCLUE_RESOURCE_NETWORK);
				else if (*ri == "cell")
					rsrc = (GeoclueResourceFlags)(rsrc | GEOCLUE_RESOURCE_CELL);
				else if (*ri == "gps")
					rsrc = (GeoclueResourceFlags)(rsrc | GEOCLUE_RESOURCE_GPS);
				else
					ok = false;
			}
		}
		if (!ok) {
			std::vector<Glib::ustring> v;
			if (rsrc & GEOCLUE_RESOURCE_NETWORK)
				v.push_back("network");
			if (rsrc & GEOCLUE_RESOURCE_CELL)
				v.push_back("cell");
			if (rsrc & GEOCLUE_RESOURCE_GPS)
				v.push_back("gps");
			get_sensors().get_configfile().set_string_list(get_configgroup(), "resources", v);
		}
	}

	// set up geoclue
	GeoclueMaster *master = geoclue_master_get_default();
	if (!master)
		throw std::runtime_error("Geoclue: Cannot get master");
	GError *error(0);
	m_client = geoclue_master_create_client(master, NULL, &error);
	g_object_unref(master);
	if (!m_client) {
		std::string msg("Geoclue: Cannot create master client");
		if (error) {
			msg += ": ";
			msg += error->message;
			g_error_free(error);
			error = 0;
		}
		throw std::runtime_error(msg);
	}
	if (error) {
		g_error_free(error);
		error = 0;
	}
	if (!geoclue_master_client_set_requirements(m_client, minlevel, 0, TRUE, rsrc, &error)) {
		std::string msg("Geoclue: Cannot set requirements");
		if (error) {
			msg += ": ";
			msg += error->message;
			g_error_free(error);
			error = 0;
		}
		throw std::runtime_error(msg);
	}
	if (error) {
		g_error_free(error);
		error = 0;
	}
	m_pos = geoclue_master_client_create_position(m_client, &error);
	if (!m_pos) {
		std::string msg("Geoclue: Cannot create position interface");
		if (error) {
			msg += ": ";
			msg += error->message;
			g_error_free(error);
			error = 0;
		}
		throw std::runtime_error(msg);
	}
	if (error) {
		g_error_free(error);
		error = 0;
	}
	g_signal_connect(G_OBJECT(m_pos), "position-changed", G_CALLBACK(SensorGeoclue::position_changed_cb), this);
}

Sensors::SensorGeoclue::~SensorGeoclue()
{
	if (m_pos)
		g_object_unref(m_pos);
	if (m_client)
		g_object_unref(m_client);
}

const std::string& Sensors::SensorGeoclue::to_str(GeoclueAccuracyLevel acclvl)
{
	switch (acclvl) {
	default:
	case GEOCLUE_ACCURACY_LEVEL_NONE:
	{
		static const std::string r("none");
		return r;
	}

	case GEOCLUE_ACCURACY_LEVEL_COUNTRY:
	{
		static const std::string r("country");
		return r;
	}

	case GEOCLUE_ACCURACY_LEVEL_REGION:
	{
		static const std::string r("region");
		return r;
	}

	case GEOCLUE_ACCURACY_LEVEL_LOCALITY:
	{
		static const std::string r("locality");
		return r;
	}

	case GEOCLUE_ACCURACY_LEVEL_POSTALCODE:
	{
		static const std::string r("postcode");
		return r;
	}

	case GEOCLUE_ACCURACY_LEVEL_STREET:
	{
		static const std::string r("street");
		return r;
	}

	case GEOCLUE_ACCURACY_LEVEL_DETAILED:
	{
		static const std::string r("detailed");
		return r;
	}
	}
};

void Sensors::SensorGeoclue::position_changed_cb(GeocluePosition *position, GeocluePositionFields fields, int timestamp, double latitude, double longitude,
						 double altitude, GeoclueAccuracy *accuracy, gpointer userdata)
{
	static_cast<SensorGeoclue *>(userdata)->position_changed(position, fields, timestamp, latitude, longitude, altitude, accuracy);
}

void Sensors::SensorGeoclue::position_changed(GeocluePosition *position, GeocluePositionFields fields, int timestamp, double latitude, double longitude,
						 double altitude, GeoclueAccuracy *accuracy)
{
        geoclue_accuracy_get_details(accuracy, &m_acclevel, NULL, NULL);
	m_coordok = (fields & GEOCLUE_POSITION_FIELDS_LATITUDE &&
		     fields & GEOCLUE_POSITION_FIELDS_LONGITUDE);
	ParamChanged pc;
	if (fields & GEOCLUE_POSITION_FIELDS_LATITUDE) {
		m_coord.set_lat_deg_dbl(latitude);
		pc.set_changed(parnrlat);
	}
	if (fields & GEOCLUE_POSITION_FIELDS_LONGITUDE) {
		m_coord.set_lon_deg_dbl(longitude);
		pc.set_changed(parnrlon);
	}
	if (fields & (GEOCLUE_POSITION_FIELDS_LATITUDE | GEOCLUE_POSITION_FIELDS_LONGITUDE)) {
		m_postime = Glib::TimeVal(timestamp, 0);
		pc.set_changed(parnrtime);
	}
	if (fields & GEOCLUE_POSITION_FIELDS_ALTITUDE) {
		m_alt = altitude * Point::m_to_ft;
		m_alttime = Glib::TimeVal(timestamp, 0);
	} else {
		m_alt = std::numeric_limits<double>::quiet_NaN();
	}
	pc.set_changed(parnraccuracylevel);
	pc.set_changed(parnralt);
	update(pc);
}

bool Sensors::SensorGeoclue::get_position(Point& pt) const
{
	pt = m_coord;
	return m_coordok;
}

unsigned int Sensors::SensorGeoclue::get_position_priority(void) const
{
	if (m_positionpriority.empty())
		return 0;
	return m_positionpriority[std::min((unsigned int)m_positionpriority.size() - 1U, (unsigned int)m_acclevel)];
}

Glib::TimeVal Sensors::SensorGeoclue::get_position_time(void) const
{
	return m_postime;
}

void Sensors::SensorGeoclue::get_truealt(double& alt, double& altrate) const
{
	
	alt = m_alt;
	altrate = std::numeric_limits<double>::quiet_NaN();
}

unsigned int Sensors::SensorGeoclue::get_truealt_priority(void) const
{
	if (m_altitudepriority.empty())
		return 0;
	return m_altitudepriority[std::min((unsigned int)m_altitudepriority.size() - 1U, (unsigned int)m_acclevel)];
}

Glib::TimeVal Sensors::SensorGeoclue::get_truealt_time(void) const
{
	return m_alttime;
}

void Sensors::SensorGeoclue::get_param_desc(unsigned int pagenr, paramdesc_t& pd)
{
	SensorInstance::get_param_desc(pagenr, pd);

	pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Fix", "Current Fix", ""));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnraccuracylevel, "Fix Accuracy", "Fix Accuracy Level", ""));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrtime, "Fix Time", "Fix Time in UTC", "UTC"));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrlat, "Latitude", "Latitude", ""));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrlon, "Longitude", "Longitude", ""));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnralt, "Altitude", "True Altitude", "ft", 0, -99999.0, 99999.0, 1.0, 10.0));

	pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Position Priorities", "Position Priority Values for different Accuracy Levels", ""));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrposprio0, "None", "Position Priority for Accuracy Level None; higher values mean this sensor is preferred to other sensors delivering position information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrposprio1, "Country", "Position Priority for Accuracy Level Country; higher values mean this sensor is preferred to other sensors delivering position information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrposprio2, "Region", "Position Priority for Accuracy Level Region; higher values mean this sensor is preferred to other sensors delivering position information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrposprio3, "Locality", "Position Priority for Accuracy Level Locality; higher values mean this sensor is preferred to other sensors delivering position information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrposprio4, "Postalcode", "Position Priority for Accuracy Level Postalcode; higher values mean this sensor is preferred to other sensors delivering position information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrposprio5, "Street", "Position Priority for Accuracy Level Street; higher values mean this sensor is preferred to other sensors delivering position information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrposprio6, "Detailed", "Position Priority for Accuracy Level Detailed; higher values mean this sensor is preferred to other sensors delivering position information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Altitude Priorities", "Altitude Priority Values for different Accuracy Levels", ""));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnraltprio0, "None", "Altitude Priority for Accuracy Level None; higher values mean this sensor is preferred to other sensors delivering altitude information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnraltprio1, "Country", "Altitude Priority for Accuracy Level Country; higher values mean this sensor is preferred to other sensors delivering altitude information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnraltprio2, "Region", "Altitude Priority for Accuracy Level Region; higher values mean this sensor is preferred to other sensors delivering altitude information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnraltprio3, "Locality", "Altitude Priority for Accuracy Level Locality; higher values mean this sensor is preferred to other sensors delivering altitude information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnraltprio4, "Postalcode", "Altitude Priority for Accuracy Level Postalcode; higher values mean this sensor is preferred to other sensors delivering altitude information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnraltprio5, "Street", "Altitude Priority for Accuracy Level Street; higher values mean this sensor is preferred to other sensors delivering altitude information", "", 0, 0, 9999, 1, 10));
	pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnraltprio6, "Detailed", "Altitude Priority for Accuracy Level Detailed; higher values mean this sensor is preferred to other sensors delivering position information", "", 0, 0, 9999, 1, 10));
}

Sensors::SensorGeoclue::paramfail_t Sensors::SensorGeoclue::get_param(unsigned int nr, Glib::ustring& v) const
{
	switch (nr) {
	default:
		break;

	case parnraccuracylevel:
		v = to_str(m_acclevel);
		return paramfail_ok;

	case parnrtime:
	{
		v.clear();
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

Sensors::SensorGeoclue::paramfail_t Sensors::SensorGeoclue::get_param(unsigned int nr, double& v) const
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
	}
	return SensorInstance::get_param(nr, v);
}

Sensors::SensorGeoclue::paramfail_t Sensors::SensorGeoclue::get_param(unsigned int nr, int& v) const
{
	switch (nr) {
	default:
		break;

	case parnrposprio0:
	case parnrposprio1:
	case parnrposprio2:
	case parnrposprio3:
	case parnrposprio4:
	case parnrposprio5:
	case parnrposprio6:
		if (m_positionpriority.size() <= nr - parnrposprio0)
			v = 0;
		else
			v = m_positionpriority[nr - parnrposprio0];
		return paramfail_ok;

	case parnraltprio0:
	case parnraltprio1:
	case parnraltprio2:
	case parnraltprio3:
	case parnraltprio4:
	case parnraltprio5:
	case parnraltprio6:
		if (m_altitudepriority.size() <= nr - parnraltprio0)
			v = 0;
		else
			v = m_altitudepriority[nr - parnraltprio0];
		return paramfail_ok;
	}
	return SensorInstance::get_param(nr, v);
}

void Sensors::SensorGeoclue::set_param(unsigned int nr, int v)
{
	switch (nr) {
	default:
		SensorInstance::set_param(nr, v);
		return;

	case parnrposprio0:
	case parnrposprio1:
	case parnrposprio2:
	case parnrposprio3:
	case parnrposprio4:
	case parnrposprio5:
	case parnrposprio6:
		if (m_positionpriority.size() <= nr - parnrposprio0)
			m_positionpriority.resize(nr + 1 - parnrposprio0, 0);
		if (m_positionpriority[nr - parnrposprio0] == (unsigned int)v)
			return;
		m_positionpriority[nr - parnrposprio0] = v;
		break;

	case parnraltprio0:
	case parnraltprio1:
	case parnraltprio2:
	case parnraltprio3:
	case parnraltprio4:
	case parnraltprio5:
	case parnraltprio6:
		if (m_altitudepriority.size() <= nr - parnraltprio0)
			m_altitudepriority.resize(nr + 1 - parnraltprio0, 0);
		if (m_altitudepriority[nr - parnraltprio0] == (unsigned int)v)
			return;
		m_altitudepriority[nr - parnraltprio0] = v;
		break;
	}
	ParamChanged pc;
	pc.set_changed(nr);
	update(pc);
}

std::string Sensors::SensorGeoclue::logfile_header(void) const
{
	return SensorInstance::logfile_header() + ",FixTime,Lat,Lon,Alt,AccLevel";
}

std::string Sensors::SensorGeoclue::logfile_records(void) const
{
	std::ostringstream oss;
	oss << SensorInstance::logfile_records() << std::fixed << std::setprecision(6) << ',';
	if (m_postime.valid()) {
                Glib::DateTime dt(Glib::DateTime::create_now_utc(m_postime));
                oss << dt.format("%Y-%m-%d %H:%M:%S");
	}
	oss << ',';
	if (!m_coordok || m_coord.is_invalid())
		oss << ',';
	else
		oss << m_coord.get_lat_deg_dbl() << ',' << m_coord.get_lon_deg_dbl();
	oss << ',';
	if (!std::isnan(m_alt))
		oss << m_alt;
	oss << ',' << to_str(m_acclevel);
	return oss.str();
}
