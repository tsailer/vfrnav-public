//
// C++ Implementation: GeoClue2 Sensor
//
// Description: GeoClue2 Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <limits>
#include <iomanip>
#include <glibmm/datetime.h>

#include "sensgeoclue2.h"

Sensors::SensorGeoclue2::SensorGeoclue2(Sensors& sensors, const Glib::ustring& configgroup)
	: SensorInstance(sensors, configgroup), m_cancellable(g_cancellable_new()), m_manager(0),
	  m_client(0), m_alt(std::numeric_limits<double>::quiet_NaN()),
	  m_crs(std::numeric_limits<double>::quiet_NaN()),
	  m_gs(std::numeric_limits<double>::quiet_NaN()),
	  m_accuracy(std::numeric_limits<double>::quiet_NaN()), m_coordok(false),
	  m_minlevel(GCLUE_ACCURACY_LEVEL_EXACT)
{
	// get configuration
        const Glib::KeyFile& cf(get_sensors().get_configfile());
        if (!cf.has_key(get_configgroup(), "positionpriority")) {
		static const double dfltprio[2] = { 1000.0, 50.0 };
		get_sensors().get_configfile().set_double_list(get_configgroup(), "positionpriority", std::vector<double>(dfltprio, dfltprio + sizeof(dfltprio)/sizeof(dfltprio[0])));
	}
	m_positionpriority = cf.get_double_list(get_configgroup(), "positionpriority");
        if (!cf.has_key(get_configgroup(), "altitudepriority")) {
		static const double dfltprio[2] = { 1000.0, 50.0 };
		get_sensors().get_configfile().set_double_list(get_configgroup(), "altitudepriority", std::vector<double>(dfltprio, dfltprio + sizeof(dfltprio)/sizeof(dfltprio[0])));
	}
	m_altitudepriority = cf.get_double_list(get_configgroup(), "altitudepriority");
	if (!cf.has_key(get_configgroup(), "minlevel"))
                get_sensors().get_configfile().set_integer(get_configgroup(), "minlevel", GCLUE_ACCURACY_LEVEL_EXACT);
	m_minlevel = cf.get_integer(get_configgroup(), "minlevel");
	// set up geoclue2 manager proxy
	GError *error(0);
	m_manager = geo_clue2_manager_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
							     G_DBUS_PROXY_FLAGS_NONE,
							     "org.freedesktop.GeoClue2",
							     "/org/freedesktop/GeoClue2/Manager",
							     m_cancellable,
							     &error);
	if (!m_manager) {
		std::string msg("GeoClue2: Cannot create manager");
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
	geo_clue2_manager_call_get_client(m_manager,
					  m_cancellable,
					  manager_get_client_cb,
					  this);
}

Sensors::SensorGeoclue2::~SensorGeoclue2()
{
	if (m_cancellable) {
		g_cancellable_cancel(m_cancellable);
		g_object_unref(m_cancellable);
	}
	if (m_client) {
		geo_clue2_client_call_stop(m_client, 0, 0, 0);
		g_signal_handlers_disconnect_by_func(G_OBJECT(m_client), (gpointer *)G_CALLBACK(SensorGeoclue2::position_changed_cb), this);
   		g_object_unref(m_client);
	}
	if (m_manager)
		g_object_unref(m_manager);
}

const std::string& Sensors::SensorGeoclue2::to_str(GClueAccuracyLevel acclvl)
{
	switch (acclvl) {
	default:
	{
		static const std::string r("none");
		return r;
	}

	case GCLUE_ACCURACY_LEVEL_COUNTRY:
	{
		static const std::string r("country");
		return r;
	}

	case GCLUE_ACCURACY_LEVEL_CITY:
	{
		static const std::string r("city");
		return r;
	}

	case GCLUE_ACCURACY_LEVEL_NEIGHBORHOOD:
	{
		static const std::string r("neighborhood");
		return r;
	}

	case GCLUE_ACCURACY_LEVEL_STREET:
	{
		static const std::string r("street");
		return r;
	}

	case GCLUE_ACCURACY_LEVEL_EXACT:
	{
		static const std::string r("exact");
		return r;
	}
	}
};

void Sensors::SensorGeoclue2::manager_get_client_cb(GObject *manager, GAsyncResult *res, gpointer user_data)
{
	static_cast<SensorGeoclue2 *>(user_data)->manager_get_client((GeoClue2Manager *)manager, res);
}

void Sensors::SensorGeoclue2::manager_get_client(GeoClue2Manager *manager, GAsyncResult *res)
{
	GError *error(0);
	gchar *clnt(0);
	gboolean success(geo_clue2_manager_call_get_client_finish(manager, &clnt, res, &error));
	if (!success) {
		std::cerr << "GeoClue2 manager get_client error";
		if (error) {
			std::cerr << ": " << error->message;
			g_error_free(error);
			error = 0;
		}
		std::cerr << std::endl;
		if (clnt)
			g_free(clnt);
		clnt = 0;
		return;
	}
	if (error) {
		g_error_free(error);
		error = 0;
	}
	if (!clnt) {
		std::cerr << "GeoClue2 manager get_client did not return a client string" << std::endl;
		return;
	}
	if (m_client)
		g_object_unref(m_client);
	m_client = geo_clue2_client_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
							   G_DBUS_PROXY_FLAGS_NONE,
							   "org.freedesktop.GeoClue2",
							   clnt,
							   m_cancellable,
							   &error);
	if (!m_client) {
		std::cerr << "GeoClue2: Cannot create client " << clnt;
		if (error) {
			std::cerr << ": " << error->message;
			g_error_free(error);
			error = 0;
		}
		std::cerr << std::endl;
		g_free(clnt);
		clnt = 0;
		return;
	}
	if (error) {
		g_error_free(error);
		error = 0;
	}
	g_free(clnt);
	clnt = 0;
	geo_clue2_client_set_requested_accuracy_level(m_client, m_minlevel);
	geo_clue2_client_set_distance_threshold(m_client, 1);
	geo_clue2_client_set_desktop_id(m_client, PACKAGE);
	g_signal_connect(G_OBJECT(m_client), "location-updated", G_CALLBACK(SensorGeoclue2::position_changed_cb), this);
	geo_clue2_client_call_start(m_client, m_cancellable, 0, 0);
}

void Sensors::SensorGeoclue2::position_changed_cb(GeoClue2Client *client, const gchar *arg_old, const gchar *arg_new, gpointer userdata)
{
	static_cast<SensorGeoclue2 *>(userdata)->position_changed(client, arg_old, arg_new);
}

void Sensors::SensorGeoclue2::position_changed(GeoClue2Client *client, const gchar *arg_old, const gchar *arg_new)
{
	geo_clue2_location_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
					     G_DBUS_PROXY_FLAGS_NONE,
					     "org.freedesktop.GeoClue2",
					     arg_new,
					     m_cancellable,
					     &SensorGeoclue2::new_location_cb,
					     this);
}

void Sensors::SensorGeoclue2::new_location_cb(GObject *client, GAsyncResult *res, gpointer user_data)
{
	static_cast<SensorGeoclue2 *>(user_data)->new_location((GeoClue2Client *)client, res);
}

void Sensors::SensorGeoclue2::new_location(GeoClue2Client *client, GAsyncResult *res)
{
	GError *error(0);
	GeoClue2Location *loc(geo_clue2_location_proxy_new_for_bus_finish(res, &error));
	if (!loc) {
		std::cerr << "GeoClue2 client new location error";
		if (error) {
			std::cerr << ": " << error->message;
			g_error_free(error);
			error = 0;
		}
		std::cerr << std::endl;
		return;
	}
	if (error) {
		g_error_free(error);
		error = 0;
	}
	// update position
	ParamChanged pc;
	{
		double lat(geo_clue2_location_get_latitude(loc));
		double lon(geo_clue2_location_get_longitude(loc));
		m_coord.set_lat_deg_dbl(lat);
		m_coord.set_lon_deg_dbl(lon);
		m_coordok = !std::isnan(lat) && !std::isnan(lon);
	}
	pc.set_changed(parnrlat);
	pc.set_changed(parnrlon);
	m_postime.assign_current_time();
	{
		double alt(geo_clue2_location_get_altitude(loc));
		if (!std::isnan(alt) && alt != DBL_MIN) {
			m_alt = alt * Point::m_to_ft;
			m_alttime = m_postime;
			pc.set_changed(parnralt);
		}
	}
	m_accuracy = geo_clue2_location_get_accuracy(loc);
	pc.set_changed(parnraccuracy);
	{
		double gs(geo_clue2_location_get_speed(loc));
		double crs(geo_clue2_location_get_heading(loc));
		if (gs == -1.0)
			gs = std::numeric_limits<double>::quiet_NaN();
		if (crs == -1.0)
			crs = std::numeric_limits<double>::quiet_NaN();
		if (!std::isnan(gs) && !std::isnan(crs)) {
			m_crs = crs;
			m_gs = gs * Point::mps_to_kts_dbl;
			pc.set_changed(parnrcourse);
			pc.set_changed(parnrgroundspeed);
			m_crstime = m_postime;
		}
	}
	update(pc);
}

bool Sensors::SensorGeoclue2::get_position(Point& pt) const
{
	pt = m_coord;
	return m_coordok;
}

unsigned int Sensors::SensorGeoclue2::get_position_priority(void) const
{
	unsigned int prio(0);
	if (std::isnan(m_accuracy) || m_accuracy < 0)
		return prio;
	for (std::vector<double>::size_type i(0), sz(m_positionpriority.size()); i < sz; ++i)
		if (!std::isnan(m_positionpriority[i]) && m_positionpriority[i] >= 0 &&
		    m_accuracy <= m_positionpriority[i])
			prio = i;
	return prio;
}

Glib::TimeVal Sensors::SensorGeoclue2::get_position_time(void) const
{
	return m_postime;
}

void Sensors::SensorGeoclue2::get_truealt(double& alt, double& altrate) const
{
	
	alt = m_alt;
	altrate = std::numeric_limits<double>::quiet_NaN();
}

unsigned int Sensors::SensorGeoclue2::get_truealt_priority(void) const
{
	unsigned int prio(0);
	if (std::isnan(m_accuracy) || m_accuracy < 0)
		return prio;
	for (std::vector<double>::size_type i(0), sz(m_altitudepriority.size()); i < sz; ++i)
		if (!std::isnan(m_altitudepriority[i]) && m_altitudepriority[i] >= 0 &&
		    m_accuracy <= m_altitudepriority[i])
			prio = i;
	return prio;
}

Glib::TimeVal Sensors::SensorGeoclue2::get_truealt_time(void) const
{
	return m_alttime;
}

void Sensors::SensorGeoclue2::get_course(double& crs, double& gs) const
{
	if (std::isnan(m_crs) || std::isnan(m_gs)) {
		crs = gs = std::numeric_limits<double>::quiet_NaN();
		return;
	}
	crs = m_crs;
	gs = m_gs;
}

Glib::TimeVal Sensors::SensorGeoclue2::get_course_time(void) const
{
	return m_crstime;
}

void Sensors::SensorGeoclue2::get_param_desc(unsigned int pagenr, paramdesc_t& pd)
{
	SensorInstance::get_param_desc(pagenr, pd);

	pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Fix", "Current Fix", ""));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrtime, "Fix Time", "Fix Time in UTC", "UTC"));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrlat, "Latitude", "Latitude", ""));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrlon, "Longitude", "Longitude", ""));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnralt, "Altitude", "True Altitude", "ft", 0, -99999.0, 99999.0, 1.0, 10.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrcourse, "Course", "True Course", "°", 0, -999.0, 999.0, 1.0, 10.0));
        pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrgroundspeed, "Ground Speed", "Ground Speed", "kt", 0, 0.0, 999.0, 1.0, 10.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnraccuracy, "Accuracy", "Accuracy", "m", 0, 0.0, 99999.0, 1.0, 10.0));

	pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Position Priorities", "Accuracies needed for Position Priority Values", ""));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrposprio0, "Prio 1", "Accuracy needed for Position Priority 1", "m", 0, 0.0, 99999.0, 1.0, 10.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrposprio1, "Prio 2", "Accuracy needed for Position Priority 2", "m", 0, 0.0, 99999.0, 1.0, 10.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrposprio2, "Prio 3", "Accuracy needed for Position Priority 3", "m", 0, 0.0, 99999.0, 1.0, 10.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrposprio3, "Prio 4", "Accuracy needed for Position Priority 4", "m", 0, 0.0, 99999.0, 1.0, 10.0));
	pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Altitude Priorities", "Accuracies needed for Altitude Priority Values", ""));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnraltprio0, "Prio 1", "Accuracy needed for Altitude Priority 1", "m", 0, 0.0, 99999.0, 1.0, 10.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnraltprio1, "Prio 2", "Accuracy needed for Altitude Priority 2", "m", 0, 0.0, 99999.0, 1.0, 10.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnraltprio2, "Prio 3", "Accuracy needed for Altitude Priority 3", "m", 0, 0.0, 99999.0, 1.0, 10.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnraltprio3, "Prio 4", "Accuracy needed for Altitude Priority 4", "m", 0, 0.0, 99999.0, 1.0, 10.0));
}

Sensors::SensorGeoclue2::paramfail_t Sensors::SensorGeoclue2::get_param(unsigned int nr, Glib::ustring& v) const
{
	switch (nr) {
	default:
		break;

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

Sensors::SensorGeoclue2::paramfail_t Sensors::SensorGeoclue2::get_param(unsigned int nr, double& v) const
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

	case parnraccuracy:
		v = m_accuracy;
		return std::isnan(v) ? paramfail_fail : paramfail_ok;

	case parnrposprio0:
	case parnrposprio1:
	case parnrposprio2:
	case parnrposprio3:
		if (m_positionpriority.size() <= nr - parnrposprio0)
			v = std::numeric_limits<double>::quiet_NaN();
		else
			v = m_positionpriority[nr - parnrposprio0];
		return paramfail_ok;

	case parnraltprio0:
	case parnraltprio1:
	case parnraltprio2:
	case parnraltprio3:
		if (m_altitudepriority.size() <= nr - parnraltprio0)
			v = std::numeric_limits<double>::quiet_NaN();
		else
			v = m_altitudepriority[nr - parnraltprio0];
		return paramfail_ok;
	}
	return SensorInstance::get_param(nr, v);
}

Sensors::SensorGeoclue2::paramfail_t Sensors::SensorGeoclue2::get_param(unsigned int nr, int& v) const
{
	switch (nr) {
	default:
		break;
	}
	return SensorInstance::get_param(nr, v);
}

void Sensors::SensorGeoclue2::set_param(unsigned int nr, double v)
{
	switch (nr) {
	default:
		SensorInstance::set_param(nr, v);
		return;

	case parnrposprio0:
	case parnrposprio1:
	case parnrposprio2:
	case parnrposprio3:
		if (m_positionpriority.size() <= nr - parnrposprio0)
			m_positionpriority.resize(nr + 1 - parnrposprio0, std::numeric_limits<double>::quiet_NaN());
		if (m_positionpriority[nr - parnrposprio0] == v)
			return;
		m_positionpriority[nr - parnrposprio0] = v;
		break;

	case parnraltprio0:
	case parnraltprio1:
	case parnraltprio2:
	case parnraltprio3:
		if (m_altitudepriority.size() <= nr - parnraltprio0)
			m_altitudepriority.resize(nr + 1 - parnraltprio0, std::numeric_limits<double>::quiet_NaN());
		if (m_altitudepriority[nr - parnraltprio0] == v)
			return;
		m_altitudepriority[nr - parnraltprio0] = v;
		break;
	}
	ParamChanged pc;
	pc.set_changed(nr);
	update(pc);
}

std::string Sensors::SensorGeoclue2::logfile_header(void) const
{
	return SensorInstance::logfile_header() + ",FixTime,Lat,Lon,Alt,CRS,GS,Acc";
}

std::string Sensors::SensorGeoclue2::logfile_records(void) const
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
	oss << ',';
	if (!std::isnan(m_crs))
		oss << m_crs;
	oss << ',';
	if (!std::isnan(m_gs))
		oss << m_gs;
	oss << ',';
	if (!std::isnan(m_accuracy))
		oss << m_accuracy;
	return oss.str();
}
