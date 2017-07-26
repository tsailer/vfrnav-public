//
// C++ Interface: LibLocation GPS Sensor
//
// Description: LibLocation GPS Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SENSLIBLOC_H
#define SENSLIBLOC_H

#include "sensgps.h"

#ifdef HAVE_LIBLOCATION
extern "C" {
#include <location/location-gps-device.h>
#include <location/location-gpsd-control.h>
}
#endif

class Sensors::SensorLibLocation : public Sensors::SensorGPS {
  public:
	SensorLibLocation(Sensors& sensors, const Glib::ustring& configgroup);
	virtual ~SensorLibLocation();

	virtual bool get_position(Point& pt) const;
	virtual Glib::TimeVal get_position_time(void) const;

	virtual void get_truealt(double& alt, double& altrate) const;
	virtual Glib::TimeVal get_truealt_time(void) const;

	virtual void get_course(double& crs, double& gs) const;
	virtual Glib::TimeVal get_course_time(void) const;

	virtual void get_param_desc(unsigned int pagenr, paramdesc_t& pd);
	using SensorInstance::get_param;
	virtual paramfail_t get_param(unsigned int nr, double& v) const;
	virtual paramfail_t get_param(unsigned int nr, satellites_t& sat) const;

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
		parnrepsilontime,
		parnrepsilonhor,
		parnrepsilonvert,
		parnrepsiloncrs,
		parnrend
	};

	static void location_changed(LocationGPSDevice *device, gpointer userdata);
	void loc_changed(LocationGPSDevice *device);
	static Glib::TimeVal to_timeval(double x);

	LocationGPSDControl *m_control;
	LocationGPSDevice *m_device;
	gulong m_sighandler;
};

#endif /* SENSLIBLOC_H */
