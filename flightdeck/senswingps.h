//
// C++ Interface: Windows Sensors API GPS Sensor
//
// Description: Windows Sensors API GPS Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SENSWINGPS_H
#define SENSWINGPS_H

#include "sensgps.h"
#include <sensorsapi.h>
#include "win/sensors.h"

class Sensors::SensorWinGPS : public Sensors::SensorGPS {
  public:
	SensorWinGPS(Sensors& sensors, const Glib::ustring& configgroup);
	virtual ~SensorWinGPS();

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
		parnrdevname,
		parnrdevmfg,
		parnrdevdesc,
		parnrdevserial,
		parnrpdop,
		parnrhdop,
		parnrvdop,
		parnrerrl,
		parnrerrv,
                parnrend
 	};

	class SensorManagerEvents;
	class SensorEvents;

	friend class SensorManagerEvents;
	friend class SensorEvents;

        bool close(void);
        bool try_connect(void);
	void gps_newdata(ISensor *pSensor, ISensorDataReport *pNewData);
	void gps_statechange(ISensor* pSensor, SensorState state);

	ISensorManager *m_sensmgr;
	ISensor *m_gpssens;
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
	double m_pdop;
	double m_hdop;
	double m_vdop;
	double m_errl;
	double m_errv;
};

#endif /* SENSWINGPS_H */
