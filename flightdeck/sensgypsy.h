//
// C++ Interface: Gypsy Daemon GPS Sensor
//
// Description: GPS Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SENSGYPSY_H
#define SENSGYPSY_H

#include "sysdeps.h"

#ifdef HAVE_GYPSY
#include <gypsy/gypsy-device.h>
#include <gypsy/gypsy-accuracy.h>
#include <gypsy/gypsy-control.h>
#include <gypsy/gypsy-course.h>
#include <gypsy/gypsy-position.h>
#include <gypsy/gypsy-satellite.h>
#include <gypsy/gypsy-time.h>
#endif

#include "sensgps.h"

class Sensors::SensorGypsy : public Sensors::SensorGPS {
  public:
	SensorGypsy(Sensors& sensors, const Glib::ustring& configgroup);
	virtual ~SensorGypsy();

	virtual bool get_position(Point& pt) const;
	virtual Glib::TimeVal get_position_time(void) const;

	virtual void get_truealt(double& alt, double& altrate) const;
	virtual Glib::TimeVal get_truealt_time(void) const;

	virtual void get_course(double& crs, double& gs) const;
	virtual Glib::TimeVal get_course_time(void) const;

	virtual void get_param_desc(unsigned int pagenr, paramdesc_t& pd);
	using SensorInstance::get_param;
	virtual paramfail_t get_param(unsigned int nr, Glib::ustring& v) const;
	virtual paramfail_t get_param(unsigned int nr, double& v) const;
	virtual paramfail_t get_param(unsigned int nr, int& v) const;
	virtual paramfail_t get_param(unsigned int nr, satellites_t& sat) const;
	using SensorInstance::set_param;
	virtual void set_param(unsigned int nr, const Glib::ustring& v);
	virtual void set_param(unsigned int nr, int v);

	virtual std::string logfile_header(void) const;
	virtual std::string logfile_records(void) const;

  protected:
        enum {
                parnrstart = SensorGPS::parnrend,
		parnrfixtype = SensorGPS::parnrfixtype,
		parnrtime = SensorGPS::parnrtime,
		parnrlat = SensorGPS::parnrlat,
		parnrlon = SensorGPS::parnrlon,
		parnralt = SensorGPS::parnralt,
		parnraltrate = SensorGPS::parnraltrate,
		parnrcourse = SensorGPS::parnrcourse,
		parnrgroundspeed = SensorGPS::parnrgroundspeed,
		parnrsatellites = parnrstart,
		parnrgpstime,
		parnrport,
		parnrbaudrate,
		parnrpdop,
		parnrhdop,
		parnrvdop,
		parnrend
	};

	static const unsigned int timeout_reconnect = 15;
	static const unsigned int timeout_data = 1500;

	bool try_connect(void);
	bool close(void);
	bool gps_timeout(void);
	bool gps_close(void);
	static void position_changed(GypsyPosition *position, GypsyPositionFields fields_set, int timestamp,
				     double latitude, double longitude, double altitude, gpointer userdata);
	static void course_changed(GypsyCourse *course, GypsyCourseFields fields, int timestamp,
				   double speed, double direction, double climb, gpointer userdata);
	static void accuracy_changed(GypsyAccuracy *accuracy, GypsyAccuracyFields fields,
				     double position, double horizontal, double vertical, gpointer userdata);
	static void satellite_changed(GypsySatellite *satellite, GPtrArray *satellites, gpointer userdata);
	static void time_changed(GypsyTime *time, int timestamp, gpointer userdata);
	static void device_changed(GypsyDevice *device, GypsyDeviceFixStatus fix_status, gpointer userdata);
	static void connection_changed(GypsyDevice *device, gboolean connected, gpointer userdata);
	void pos_changed(GypsyPosition *position, GypsyPositionFields fields_set, int timestamp,
			 double latitude, double longitude, double altitude);
	void crs_changed(GypsyCourse *course, GypsyCourseFields fields, int timestamp,
			 double speed, double direction, double climb);
	void acc_changed(GypsyAccuracy *accuracy, GypsyAccuracyFields fields,
			 double position, double horizontal, double vertical);
	void sat_changed(GypsySatellite *satellite, GPtrArray *satellites);
	void tim_changed(GypsyTime *time, int timestamp);
	void dev_changed(GypsyDevice *device, GypsyDeviceFixStatus fix_status);
	void conn_changed(GypsyDevice *device, gboolean connected);
	bool get_gypsy_device(void);
	bool get_gypsy_position(void);
	bool get_gypsy_course(void);
	bool get_gypsy_accuracy(void);
	bool get_gypsy_satellite(void);
	bool get_gypsy_time(void);

	Glib::ustring m_port;
	sigc::connection m_conn;
	GypsyControl *m_control;
	GypsyDevice *m_device;
	gulong m_device_sighandler;
	gulong m_deviceconn_sighandler;
	GypsyPosition *m_position;
	gulong m_position_sighandler;
	GypsyCourse *m_course;
	gulong m_course_sighandler;
	GypsyAccuracy *m_accuracy;
	gulong m_accuracy_sighandler;
	GypsySatellite *m_satellite;
	gulong m_satellite_sighandler;
	GypsyTime *m_time;
	gulong m_time_sighandler;
	Glib::TimeVal m_timestamp;
	unsigned int m_fixtime;
	Point m_pos;
	double m_alt;
	double m_altrate;
	double m_crs;
	double m_groundspeed;
	double m_pdop;
	double m_hdop;
	double m_vdop;
	satellites_t m_sat;
	unsigned int m_baudrate;
};

#endif /* SENSGYPSY_H */
