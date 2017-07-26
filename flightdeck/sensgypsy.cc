//
// C++ Implementation: Gypsy Daemon GPS Sensor
//
// Description: GPS Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <limits>
#include <iomanip>
#include "sensgypsy.h"

#ifdef HAVE_GCONF
#include <gconf/gconf-client.h>
#endif

const unsigned int Sensors::SensorGypsy::timeout_reconnect;
const unsigned int Sensors::SensorGypsy::timeout_data;

Sensors::SensorGypsy::SensorGypsy(Sensors& sensors, const Glib::ustring& configgroup)
	: SensorGPS(sensors, configgroup), m_control(0), m_device(0), m_device_sighandler(0), m_deviceconn_sighandler(0),
	  m_position(0), m_position_sighandler(0), m_course(0), m_course_sighandler(0),
	  m_accuracy(0), m_accuracy_sighandler(0), m_satellite(0), m_satellite_sighandler(0),
	  m_time(0), m_time_sighandler(0), m_fixtime(0),
	  m_alt(std::numeric_limits<double>::quiet_NaN()),
	  m_altrate(std::numeric_limits<double>::quiet_NaN()),
	  m_crs(std::numeric_limits<double>::quiet_NaN()),
	  m_groundspeed(std::numeric_limits<double>::quiet_NaN()),
	  m_pdop(std::numeric_limits<double>::quiet_NaN()),
	  m_hdop(std::numeric_limits<double>::quiet_NaN()),
	  m_vdop(std::numeric_limits<double>::quiet_NaN()),
	  m_baudrate(0)
{
	const Glib::KeyFile& cf(get_sensors().get_configfile());
        if (!cf.has_key(get_configgroup(), "port")) {
		std::string dfltport("/dev/ttyUSB0");
#ifdef HAVE_GCONF
		GConfClient *client(gconf_client_get_default());
		if (client) {
			char *bdaddr(gconf_client_get_string(client, "/apps/geoclue/master/org.freedesktop.Geoclue.GPSDevice", 0));
			if (bdaddr)
				dfltport = bdaddr;
			g_object_unref(client);
		}
#endif
		get_sensors().get_configfile().set_string(get_configgroup(), "port", dfltport);
	}
	m_port = cf.get_string(get_configgroup(), "port");
        if (!cf.has_key(get_configgroup(), "baudrate"))
                get_sensors().get_configfile().set_integer(get_configgroup(), "baudrate", m_baudrate);
        m_baudrate = cf.get_integer(get_configgroup(), "baudrate");
	try_connect();
}

Sensors::SensorGypsy::~SensorGypsy()
{
	close();
}

bool Sensors::SensorGypsy::try_connect(void)
{
	if (close()) {
		ParamChanged pc;
		pc.set_changed(parnrfixtype, parnrgroundspeed);
		pc.set_changed(parnrstart, parnrend - 1);
		update(pc);
	}
	m_control = gypsy_control_get_default();
	GError *error(0);
	char *path = gypsy_control_create(m_control, m_port.c_str(), &error);
	if (!path) {
		std::ostringstream oss;
		oss << "gypsy: cannot connect " << m_port << ": " << error->message;
		get_sensors().log(Sensors::loglevel_warning, oss.str());
		g_error_free(error);
		error = 0;
		g_object_unref(G_OBJECT(m_control));
		m_control = 0;
		m_conn = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorGypsy::try_connect), timeout_reconnect);
		return true;
	}
        m_device = gypsy_device_new(path);
	m_device_sighandler = g_signal_connect(m_device, "fix-status-changed", G_CALLBACK(SensorGypsy::device_changed), this);
	m_deviceconn_sighandler = g_signal_connect(m_device, "connection-changed", G_CALLBACK(SensorGypsy::connection_changed), this);
        m_position = gypsy_position_new(path);
        m_position_sighandler = g_signal_connect(m_position, "position-changed", G_CALLBACK(SensorGypsy::position_changed), this);
        m_course = gypsy_course_new(path);
        m_course_sighandler = g_signal_connect(m_course, "course-changed", G_CALLBACK(SensorGypsy::course_changed), this);
        m_accuracy = gypsy_accuracy_new(path);
        m_accuracy_sighandler = g_signal_connect(m_accuracy, "accuracy-changed", G_CALLBACK(SensorGypsy::accuracy_changed), this);
        m_satellite = gypsy_satellite_new(path);
        m_satellite_sighandler = g_signal_connect(m_satellite, "satellites-changed", G_CALLBACK(SensorGypsy::satellite_changed), this);
        m_time = gypsy_time_new(path);
        m_time_sighandler = g_signal_connect(m_time, "time-changed", G_CALLBACK(SensorGypsy::time_changed), this);
	g_free(path);
	if (m_baudrate != 0) {
		GHashTable *goptions;
		GValue speed_val = { 0, };
		g_value_init(&speed_val, G_TYPE_UINT);
		g_value_set_uint(&speed_val, m_baudrate);
		goptions = g_hash_table_new(g_str_hash, g_str_equal);
		g_hash_table_insert(goptions, (gpointer)"BaudRate", &speed_val);
		if (!gypsy_device_set_start_options(m_device, goptions, &error)) {
			std::ostringstream oss;
			oss << "gypsy: cannot set baud rate " << m_baudrate << ": " << error->message;
			get_sensors().log(Sensors::loglevel_warning, oss.str());
			g_error_free(error);
			error = 0;
		}
		g_hash_table_destroy(goptions);
	}
        gypsy_device_start(m_device, &error);
	if (error) {
		std::ostringstream oss;
		oss << "gypsy: cannot start device " << m_port << " (" << path << "): " << error->message;
		get_sensors().log(Sensors::loglevel_warning, oss.str());
		g_free(path);
		close();
		m_conn = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorGypsy::try_connect), timeout_reconnect);
		return true;
	}
	m_conn = Glib::signal_timeout().connect(sigc::mem_fun(*this, &SensorGypsy::gps_timeout), timeout_data);
	get_gypsy_device();
	get_gypsy_position();
	get_gypsy_course();
	get_gypsy_accuracy();
	get_gypsy_satellite();
	get_gypsy_time();
	return false;
}

bool Sensors::SensorGypsy::get_gypsy_device(void)
{
	GError *error(0);
	GypsyDeviceFixStatus fix_status(gypsy_device_get_fix_status(m_device, &error));
	if (error) {
		std::ostringstream oss;
		oss << "gypsy: cannot get initial device status " << m_port << ": " << error->message;
		get_sensors().log(Sensors::loglevel_warning, oss.str());
		g_error_free(error);
		error = 0;
		return false;
	}
	dev_changed(m_device, fix_status);
	return true;
}

bool Sensors::SensorGypsy::get_gypsy_position(void)
{
	GError *error(0);
	int timestamp;
	double latitude;
	double longitude;
	double altitude;
	GypsyPositionFields f(gypsy_position_get_position(m_position, &timestamp, &latitude, &longitude, &altitude, &error));
	if (error) {
		std::ostringstream oss;
		oss << "gypsy: cannot get initial position " << m_port << ": " << error->message;
		get_sensors().log(Sensors::loglevel_warning, oss.str());
		g_error_free(error);
		error = 0;
		return false;
	}
	pos_changed(m_position, f, timestamp, latitude, longitude, altitude);
	return true;
}

bool Sensors::SensorGypsy::get_gypsy_course(void)
{
	GError *error(0);
	int timestamp;
	double speed;
	double direction;
	double climb;
	GypsyCourseFields f(gypsy_course_get_course(m_course, &timestamp, &speed, &direction, &climb, &error));
	if (error) {
		std::ostringstream oss;
		oss << "gypsy: cannot get initial course " << m_port << ": " << error->message;
		get_sensors().log(Sensors::loglevel_warning, oss.str());
		g_error_free(error);
		error = 0;
		return false;
	}
	crs_changed(m_course, f, timestamp, speed, direction, climb);
	return true;
}

bool Sensors::SensorGypsy::get_gypsy_accuracy(void)
{
	GError *error(0);
	double pdop;
	double hdop;
	double vdop;
	GypsyAccuracyFields f(gypsy_accuracy_get_accuracy(m_accuracy, &pdop, &hdop, &vdop, &error));
	if (error) {
		std::ostringstream oss;
		oss << "gypsy: cannot get initial accuracy " << m_port << ": " << error->message;
		get_sensors().log(Sensors::loglevel_warning, oss.str());
		g_error_free(error);
		error = 0;
		return false;
	}
	acc_changed(m_accuracy, f, pdop, hdop, vdop);
	return true;
}

bool Sensors::SensorGypsy::get_gypsy_satellite(void)
{
	bool ret(true);
	GError *error(0);
	GPtrArray *satellites(gypsy_satellite_get_satellites(m_satellite, &error));
	if (error) {
		std::ostringstream oss;
		oss << "gypsy: cannot get initial satellites " << m_port << ": " << error->message;
		get_sensors().log(Sensors::loglevel_warning, oss.str());
		g_error_free(error);
		error = 0;
		ret = false;
	} else {
		sat_changed(m_satellite, satellites);
	}
	if (satellites)
		gypsy_satellite_free_satellite_array(satellites);
	return ret;
}	 

bool Sensors::SensorGypsy::get_gypsy_time(void)
{
	GError *error(0);
	int timestamp;
	gboolean ok(gypsy_time_get_time(m_time, &timestamp, &error));
	if (error) {
		std::ostringstream oss;
		oss << "gypsy: cannot get initial time " << m_port << ": " << error->message;
		get_sensors().log(Sensors::loglevel_warning, oss.str());
		g_error_free(error);
		error = 0;
		return false;
	}
	if (ok)
		tim_changed(m_time, timestamp);
	return true;
}

bool Sensors::SensorGypsy::close(void)
{
	bool was_open(!!m_control);
	m_conn.disconnect();
        if (m_position) {
                g_signal_handler_disconnect(m_position, m_position_sighandler);
                g_object_unref(m_position);
                m_position = 0;
                m_position_sighandler = 0;
        }
        if (m_course) {
                g_signal_handler_disconnect(m_course, m_course_sighandler);
                g_object_unref(m_course);
                m_course = 0;
                m_course_sighandler = 0;
        }
        if (m_accuracy) {
                g_signal_handler_disconnect(m_accuracy, m_accuracy_sighandler);
                g_object_unref(m_accuracy);
                m_accuracy = 0;
                m_accuracy_sighandler = 0;
        }
        if (m_satellite) {
                g_signal_handler_disconnect(m_satellite, m_satellite_sighandler);
                g_object_unref(m_satellite);
                m_satellite = 0;
                m_satellite_sighandler = 0;
        }
        if (m_time) {
                g_signal_handler_disconnect(m_time, m_time_sighandler);
                g_object_unref(m_time);
                m_time = 0;
                m_time_sighandler = 0;
        }
        if (m_device) {
                g_signal_handler_disconnect(m_device, m_device_sighandler);
		g_signal_handler_disconnect(m_device, m_deviceconn_sighandler);
                g_object_unref(m_device);
                m_device = 0;
		m_device_sighandler = 0;
		m_deviceconn_sighandler = 0;
        }
	if (m_control)
		g_object_unref(G_OBJECT(m_control));
	m_control = 0;
	m_timestamp = Glib::TimeVal();
	m_pos = Point();
	m_alt = m_altrate = m_crs = m_groundspeed = m_pdop = m_hdop = m_vdop = std::numeric_limits<double>::quiet_NaN();
	m_fixtype = fixtype_nofix;
	m_fixtime = 0;
	m_sat.clear();
	return was_open;
}

bool Sensors::SensorGypsy::gps_timeout(void)
{
	if (true)
		std::cerr << "Gypsy: timeout" << std::endl;
	if (true) {
		if (get_gypsy_device() &&
		    get_gypsy_position() &&
		    get_gypsy_course() &&
		    get_gypsy_accuracy() &&
		    get_gypsy_satellite() &&
		    get_gypsy_time()) {
			m_conn = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorGypsy::try_connect), timeout_data);
			return false;
		}
		close();
		ParamChanged pc;
		pc.set_changed(parnrfixtype, parnrsatellites);
		pc.set_changed(parnrsatellites, parnrgpstime);
		pc.set_changed(parnrpdop, parnrvdop);
		update(pc);
		m_conn = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorGypsy::try_connect), timeout_reconnect);
	} else if (true) {
		m_fixtype = fixtype_nofix;
		ParamChanged pc;
		pc.set_changed(parnrfixtype, parnrgroundspeed);
		pc.set_changed(parnrsatellites, parnrgpstime);
		pc.set_changed(parnrpdop, parnrvdop);
		update(pc);
	} else {
		close();
		ParamChanged pc;
		pc.set_changed(parnrfixtype, parnrsatellites);
		pc.set_changed(parnrsatellites, parnrgpstime);
		pc.set_changed(parnrpdop, parnrvdop);
		update(pc);
		m_conn = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorGypsy::try_connect), timeout_reconnect);
	}
	return false;
}

bool Sensors::SensorGypsy::gps_close(void)
{
	if (true)
		std::cerr << "Gypsy: close" << std::endl;
	close();
	ParamChanged pc;
	pc.set_changed(parnrfixtype, parnrsatellites);
	pc.set_changed(parnrsatellites, parnrgpstime);
	pc.set_changed(parnrpdop, parnrvdop);
	update(pc);
	m_conn = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &SensorGypsy::try_connect), timeout_reconnect);
	return false;
}

void Sensors::SensorGypsy::position_changed(GypsyPosition *position, GypsyPositionFields fields_set, int timestamp,
					    double latitude, double longitude, double altitude, gpointer userdata)
{
	static_cast<SensorGypsy *>(userdata)->pos_changed(position, fields_set, timestamp, latitude, longitude, altitude);
}

void Sensors::SensorGypsy::course_changed(GypsyCourse *course, GypsyCourseFields fields, int timestamp,
					  double speed, double direction, double climb, gpointer userdata)
{
	static_cast<SensorGypsy *>(userdata)->crs_changed(course, fields, timestamp, speed, direction, climb);
}

void Sensors::SensorGypsy::accuracy_changed(GypsyAccuracy *accuracy, GypsyAccuracyFields fields,
					    double position, double horizontal, double vertical, gpointer userdata)
{
	static_cast<SensorGypsy *>(userdata)->acc_changed(accuracy, fields, position, horizontal, vertical);
}

void Sensors::SensorGypsy::satellite_changed(GypsySatellite *satellite, GPtrArray *satellites, gpointer userdata)
{
	static_cast<SensorGypsy *>(userdata)->sat_changed(satellite, satellites);
}

void Sensors::SensorGypsy::time_changed(GypsyTime *time, int timestamp, gpointer userdata)
{
	static_cast<SensorGypsy *>(userdata)->tim_changed(time, timestamp);
}

void Sensors::SensorGypsy::device_changed(GypsyDevice *device, GypsyDeviceFixStatus fix_status, gpointer userdata)
{
	static_cast<SensorGypsy *>(userdata)->dev_changed(device, fix_status);
}

void Sensors::SensorGypsy::connection_changed(GypsyDevice *device, gboolean connected, gpointer userdata)
{
	static_cast<SensorGypsy *>(userdata)->conn_changed(device, connected);
}

void Sensors::SensorGypsy::pos_changed(GypsyPosition *position, GypsyPositionFields fields_set, int timestamp,
				       double latitude, double longitude, double altitude)
{
	m_conn.disconnect();
	m_conn = Glib::signal_timeout().connect(sigc::mem_fun(*this, &SensorGypsy::gps_timeout), timeout_data);
	m_timestamp.assign_current_time();
	ParamChanged pc;
	if (fields_set & GYPSY_POSITION_FIELDS_ALTITUDE) {
		m_alt = altitude * Point::m_to_ft_dbl;
		pc.set_changed(parnralt);
	}
	if (fields_set & GYPSY_POSITION_FIELDS_LATITUDE) {
		m_pos.set_lat_deg_dbl(latitude);
		pc.set_changed(parnrlat);
	}
	if (fields_set & GYPSY_POSITION_FIELDS_LONGITUDE) {
		m_pos.set_lon_deg_dbl(longitude);
		pc.set_changed(parnrlon);
	}
	if (false) {
		std::cerr << "Gypsy: position changed";
		if (fields_set & GYPSY_POSITION_FIELDS_ALTITUDE)
			std::cerr << " alt " << m_alt << "ft";
		if (fields_set & GYPSY_POSITION_FIELDS_LATITUDE)
			std::cerr << " lat " << m_pos.get_lat_str2();
		if (fields_set & GYPSY_POSITION_FIELDS_LONGITUDE)
			std::cerr << " lon " << m_pos.get_lon_str2();
		std::cerr << std::endl;
	}
	update(pc);	
}

void Sensors::SensorGypsy::crs_changed(GypsyCourse *course, GypsyCourseFields fields, int timestamp,
				       double speed, double direction, double climb)
{
	m_conn.disconnect();
	m_conn = Glib::signal_timeout().connect(sigc::mem_fun(*this, &SensorGypsy::gps_timeout), timeout_data);
	m_timestamp.assign_current_time();
	ParamChanged pc;
	if (fields & GYPSY_COURSE_FIELDS_SPEED) {
		m_groundspeed = speed * Point::mps_to_kts_dbl;
		pc.set_changed(parnrgroundspeed);
	}
	if (fields & GYPSY_COURSE_FIELDS_DIRECTION) {
		m_crs = direction;
		pc.set_changed(parnrcourse);
	}
	if (fields & GYPSY_COURSE_FIELDS_CLIMB)	{
		m_altrate = climb * (Point::m_to_ft * 60.0);
		pc.set_changed(parnraltrate);
	}
	if (false) {
		std::cerr << "Gypsy: course changed";
		if (fields & GYPSY_COURSE_FIELDS_SPEED)
			std::cerr << " speed " << m_groundspeed << "kts";
		if (fields & GYPSY_COURSE_FIELDS_DIRECTION)
			std::cerr << " dir " << m_crs;
		if (fields & GYPSY_COURSE_FIELDS_CLIMB)
			std::cerr << " climb " << m_altrate << "ft/min";
		std::cerr << std::endl;
	}
        update(pc);	
}

void Sensors::SensorGypsy::acc_changed(GypsyAccuracy *accuracy, GypsyAccuracyFields fields,
				       double position, double horizontal, double vertical)
{
	m_conn.disconnect();
	m_conn = Glib::signal_timeout().connect(sigc::mem_fun(*this, &SensorGypsy::gps_timeout), timeout_data);
	m_timestamp.assign_current_time();
	ParamChanged pc;
	if (fields & GYPSY_ACCURACY_FIELDS_POSITION) {
		m_pdop = position;
		pc.set_changed(parnrpdop);
	}
	if (fields & GYPSY_ACCURACY_FIELDS_HORIZONTAL) {
		m_hdop = horizontal;
		pc.set_changed(parnrhdop);
	}
	if (fields & GYPSY_ACCURACY_FIELDS_VERTICAL) {
		m_vdop = vertical;
		pc.set_changed(parnrvdop);
	}
	if (false) {
		std::cerr << "Gypsy: accuracy changed";
		if (fields & GYPSY_ACCURACY_FIELDS_POSITION)
			std::cerr << " pdop " << m_pdop;
		if (fields & GYPSY_ACCURACY_FIELDS_HORIZONTAL)
			std::cerr << " hdop " << m_hdop;
		if (fields & GYPSY_ACCURACY_FIELDS_VERTICAL)
			std::cerr << " vdop " << m_vdop;
		std::cerr << std::endl;
	}
        update(pc);	
}

void Sensors::SensorGypsy::sat_changed(GypsySatellite *satellite, GPtrArray *satellites)
{
	m_conn.disconnect();
	m_conn = Glib::signal_timeout().connect(sigc::mem_fun(*this, &SensorGypsy::gps_timeout), timeout_data);
	m_timestamp.assign_current_time();
	if (false)
		std::cerr << "Gypsy: satellites changed " << satellites->len << std::endl;
	satellites_t sat;
	for (int i = 0; i < (int)satellites->len; ++i) {
		GypsySatelliteDetails *details(static_cast<GypsySatelliteDetails *>(satellites->pdata[i]));
		sat.push_back(Satellite(details->satellite_id, details->azimuth, details->elevation,
					details->snr, details->in_use));
	}
	m_sat.swap(sat);
	ParamChanged pc;
	pc.set_changed(parnrsatellites);
        update(pc);	
}

void Sensors::SensorGypsy::tim_changed(GypsyTime *time, int timestamp)
{
	m_conn.disconnect();
	m_conn = Glib::signal_timeout().connect(sigc::mem_fun(*this, &SensorGypsy::gps_timeout), timeout_data);
	m_timestamp.assign_current_time();
	if (false)
		std::cerr << "Gypsy: time changed " << timestamp << std::endl;
	m_fixtime = timestamp;
	ParamChanged pc;
	pc.set_changed(parnrtime);
	pc.set_changed(parnrgpstime);
        update(pc);	
	//get_gypsy_device();
	//get_gypsy_position();
}

void Sensors::SensorGypsy::dev_changed(GypsyDevice *device, GypsyDeviceFixStatus fix_status)
{
	m_conn.disconnect();
	m_conn = Glib::signal_timeout().connect(sigc::mem_fun(*this, &SensorGypsy::gps_timeout), timeout_data);
	m_timestamp.assign_current_time();
	if (false) {
		std::cerr << "Gypsy: device changed ";
		switch (fix_status) {
		default:
			std::cerr << (unsigned int)fix_status;
			break;

		case GYPSY_DEVICE_FIX_STATUS_INVALID:
			std::cerr << "Invalid";
			break;

		case GYPSY_DEVICE_FIX_STATUS_NONE:
			std::cerr << "None";
			break;

		case GYPSY_DEVICE_FIX_STATUS_2D:
			std::cerr << "2D";
			break;

		case GYPSY_DEVICE_FIX_STATUS_3D:
			std::cerr << "3D";
			break;
		}
		std::cerr << std::endl;
	}
	switch (fix_status) {
	case GYPSY_DEVICE_FIX_STATUS_INVALID:
	case GYPSY_DEVICE_FIX_STATUS_NONE:
	default:
		if (m_fixtype == fixtype_nofix)
			return;
		m_fixtype = fixtype_nofix;
		break;

	case GYPSY_DEVICE_FIX_STATUS_2D:
		if (m_fixtype == fixtype_2d)
			return;
		m_fixtype = fixtype_2d;
		break;

	case GYPSY_DEVICE_FIX_STATUS_3D:
		if (m_fixtype == fixtype_3d)
			return;
		m_fixtype = fixtype_3d;
		break;
	}
	ParamChanged pc;
	pc.set_changed(parnrfixtype, parnrgroundspeed);
	pc.set_changed(parnrsatellites, parnrgpstime);
	pc.set_changed(parnrpdop, parnrvdop);
        update(pc);
}

void Sensors::SensorGypsy::conn_changed(GypsyDevice *device, gboolean connected)
{
	m_conn.disconnect();
	m_conn = Glib::signal_timeout().connect(sigc::mem_fun(*this, &SensorGypsy::gps_timeout), timeout_data);
	m_timestamp.assign_current_time();
	if (true)
		std::cerr << "Gypsy: connection changed: " << (connected ? "connected" : "disconnected") << std::endl;
	if (!connected)
		gps_close();
}

bool Sensors::SensorGypsy::get_position(Point& pt) const
{
	pt = m_pos;
	return m_fixtype >= fixtype_2d;
}

Glib::TimeVal Sensors::SensorGypsy::get_position_time(void) const
{
	return m_timestamp;
}

void Sensors::SensorGypsy::get_truealt(double& alt, double& altrate) const
{
	if (m_fixtype < fixtype_3d) {
		alt = altrate = std::numeric_limits<double>::quiet_NaN();
		return;
	}
	alt = m_alt;
	altrate = m_altrate;
}

Glib::TimeVal Sensors::SensorGypsy::get_truealt_time(void) const
{
	return m_timestamp;
}

void Sensors::SensorGypsy::get_course(double& crs, double& gs) const
{
	if (m_fixtype < fixtype_2d) {
		crs = gs = std::numeric_limits<double>::quiet_NaN();
		return;
	}
	crs = m_crs;
	gs = m_groundspeed;
}

Glib::TimeVal Sensors::SensorGypsy::get_course_time(void) const
{
	return m_timestamp;
}

void Sensors::SensorGypsy::get_param_desc(unsigned int pagenr, paramdesc_t& pd)
{
	SensorGPS::get_param_desc(pagenr, pd);
	{
		paramdesc_t::iterator pdi(pd.begin()), pde(pd.end());
		if (pdi != pde)
			++pdi;
		for (; pdi != pde; ++pdi)
			if (pdi->get_type() == ParamDesc::type_section)
				break;
		pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_admin, parnrport, "Port", "GPS Device Port", ""));
                ++pdi;
                pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrbaudrate, "Baud Rate", "Baud Rate", "", 0, 0, 230400, 1, 10));
	}
	pd.push_back(ParamDesc(ParamDesc::type_satellites, ParamDesc::editable_readonly, parnrsatellites, "Satellites", "Satellite Azimuth/Elevation and Signal Strengths", ""));
	
	pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Dilution of Precision", "Dilution of Precision", ""));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrpdop, "Positional", "Positional Dilution of Precision", "", 3, -999999, 999999, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrhdop, "Horizontal", "Horizontal Dilution of Precision", "", 3, -999999, 999999, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrvdop, "Vertical", "Vertical Dilution of Precision", "", 3, -999999, 999999, 0.0, 0.0));

	pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Status", "GPS Device Status", ""));
	pd.push_back(ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrgpstime, "GPS Time", "GPS Receiver Time", ""));
}

Sensors::SensorGypsy::paramfail_t Sensors::SensorGypsy::get_param(unsigned int nr, Glib::ustring& v) const
{
	switch (nr) {
	default:
		break;

	case parnrgpstime:
	{
                v.clear();
                if (m_fixtype < fixtype_2d)
                        return paramfail_fail;
		if (!m_fixtime)
                        return paramfail_fail;
                Glib::DateTime dt(Glib::DateTime::create_now_utc(m_fixtime));
                v = dt.format("%Y-%m-%d %H:%M:%S");
                return paramfail_ok;
        }

	case parnrport:
		v = m_port;
		return paramfail_ok;
	}
	return SensorGPS::get_param(nr, v);
}


Sensors::SensorGypsy::paramfail_t Sensors::SensorGypsy::get_param(unsigned int nr, double& v) const
{
	switch (nr) {
	default:
		break;

	case parnrpdop:
		v = m_pdop;
		return m_control && !std::isnan(m_pdop) ? paramfail_ok : paramfail_fail;

	case parnrhdop:
		v = m_hdop;
		return m_control && !std::isnan(m_hdop) ? paramfail_ok : paramfail_fail;

	case parnrvdop:
		v = m_vdop;
		return m_control && !std::isnan(m_vdop) ? paramfail_ok : paramfail_fail;
	}
	return SensorGPS::get_param(nr, v);
}

Sensors::SensorGypsy::paramfail_t Sensors::SensorGypsy::get_param(unsigned int nr, int& v) const
{
        switch (nr) {
        default:
                break;

        case parnrbaudrate:
                v = m_baudrate;
                return paramfail_ok;
        }
        return SensorGPS::get_param(nr, v);
}

Sensors::SensorGypsy::paramfail_t Sensors::SensorGypsy::get_param(unsigned int nr, satellites_t& sat) const
{
	sat = m_sat;
	return m_control ? paramfail_ok : paramfail_fail;
}

void Sensors::SensorGypsy::set_param(unsigned int nr, const Glib::ustring& v)
{
	switch (nr) {
	default:
		SensorGPS::set_param(nr, v);
		return;

	case parnrport:
		if (v == m_port)
			return;
		m_port = v;
		get_sensors().get_configfile().set_string(get_configgroup(), "port", m_port);
		try_connect();
		break;
	}
	ParamChanged pc;
	pc.set_changed(nr);
	update(pc);
}

void Sensors::SensorGypsy::set_param(unsigned int nr, int v)
{
        switch (nr) {
        default:
                SensorGPS::set_param(nr, v);
                return;

        case parnrbaudrate:
		if ((unsigned int)v == m_baudrate)
			return;
		m_baudrate = v;
                get_sensors().get_configfile().set_integer(get_configgroup(), "baudrate", v);
		try_connect();
                break;
        }
        ParamChanged pc;
        pc.set_changed(nr);
        update(pc);
}

std::string Sensors::SensorGypsy::logfile_header(void) const
{
	return SensorGPS::logfile_header() + ",FixTime,Lat,Lon,Alt,VS,CRS,GS,PDOP,HDOP,VDOP,NrSat";
}

std::string Sensors::SensorGypsy::logfile_records(void) const
{
	std::ostringstream oss;
	oss << SensorGPS::logfile_records() << std::fixed << std::setprecision(6) << ',';
	if (m_fixtime && m_fixtype >= fixtype_2d) {
                Glib::DateTime dt(Glib::DateTime::create_now_utc(m_fixtime));
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
	if (!std::isnan(m_pdop))
		oss << m_pdop;
	oss << ',';
	if (!std::isnan(m_hdop))
		oss << m_hdop;
	oss << ',';
	if (!std::isnan(m_vdop))
		oss << m_vdop;
	oss << ',' << m_sat.size();
	return oss.str();
}
