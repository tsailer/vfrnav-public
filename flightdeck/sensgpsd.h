//
// C++ Interface: gpsd Daemon GPS Sensor
//
// Description: GPS Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SENSGPSD_H
#define SENSGPSD_H

#include "sysdeps.h"

#ifdef HAVE_GPS_H
#include <gps.h>
#endif

#include "sensgps.h"

class Sensors::SensorGPSD : public Sensors::SensorGPS {
  public:
	SensorGPSD(Sensors& sensors, const Glib::ustring& configgroup);
	virtual ~SensorGPSD();

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
		parnrservername,
		parnrserverport,
		parnrdevice,
		parnrepsilontime,
		parnrepsilonlat,
		parnrepsilonlon,
		parnrepsilonalt,
		parnrepsilonaltrate,
		parnrepsiloncourse,
		parnrepsilongroundspeed,
		parnrend
	};

	typedef struct {
		struct gps_data_t m_gpsdata;
		SensorGPSD *m_this;
	} mygpsdata_t;

	bool close(void);
	bool try_connect(void);
	static void gps_hook(struct gps_data_t *, char *, size_t);
	bool gps_poll(Glib::IOCondition iocond);
	bool gps_timeout(void);
	void update_gps(void);
	static Glib::TimeVal to_timeval(double x);

	Glib::ustring m_servername;
	Glib::ustring m_serverport;
	sigc::connection m_conn;
	sigc::connection m_conntimeout;
	mygpsdata_t m_gpsdata;
	satellites_t m_sat;
        Glib::TimeVal m_postime;
        Glib::TimeVal m_alttime;
        Glib::TimeVal m_altratetime;
        Glib::TimeVal m_crstime;
        Glib::TimeVal m_gstime;
        Glib::TimeVal m_sattime;
        Point m_pos;
        double m_alt;
        double m_altrate;
        double m_crs;
        double m_groundspeed;
	bool m_gpsopen;
};

#endif /* SENSGPSD_H */
