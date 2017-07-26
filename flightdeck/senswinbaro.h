//
// C++ Interface: Windows Sensors API Baro Sensor
//
// Description: Windows Sensors API Baro Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SENSWINBARO_H
#define SENSWINBARO_H

#include "sensors.h"
#include <sensorsapi.h>
#include "win/sensors.h"

class Sensors::SensorWinBaro : public Sensors::SensorInstance {
  public:
	SensorWinBaro(Sensors& sensors, const Glib::ustring& configgroup);
	virtual ~SensorWinBaro();

	virtual void get_baroalt(double& alt, double& altrate) const;
	virtual unsigned int get_baroalt_priority(void) const;
	virtual Glib::TimeVal get_baroalt_time(void) const;

	virtual void get_param_desc(unsigned int pagenr, paramdesc_t& pd);
	using SensorInstance::get_param;
	virtual paramfail_t get_param(unsigned int nr, Glib::ustring& v) const;
	virtual paramfail_t get_param(unsigned int nr, double& v) const;
	virtual paramfail_t get_param(unsigned int nr, int& v) const;
	using SensorInstance::set_param;
	virtual void set_param(unsigned int nr, const Glib::ustring& v);
	virtual void set_param(unsigned int nr, double v);
	virtual void set_param(unsigned int nr, int v);

	virtual std::string logfile_header(void) const;
	virtual std::string logfile_records(void) const;

  protected:
        enum {
                parnrstart = SensorInstance::parnrend,
		parnrdevname = parnrstart,
		parnrdevmfg,
		parnrdevdesc,
		parnrdevserial,
                parnrprio,
                parnraltfilter,
                parnraltratefilter,
                parnralt,
                parnraltrate,
                parnrpress,
                parnrtemp,
                parnrend
 	};

	class SensorManagerEvents;
	class SensorEvents;

	friend class SensorManagerEvents;
	friend class SensorEvents;

        bool close(void);
        bool try_connect(void);
	void baro_newdata(ISensor *pSensor, ISensorDataReport *pNewData);
	void baro_statechange(ISensor* pSensor, SensorState state);

	ISensorManager *m_sensmgr;
	ISensor *m_barosens;
        IcaoAtmosphere<float> m_icao;
        Glib::TimeVal m_time;
        double m_rawalt;
        double m_altfilter;
        double m_altratefilter;
        double m_press;
        double m_temp;
        double m_alt;
        double m_altrate;
        unsigned int m_priority;
};

#endif /* SENSWINBARO_H */
