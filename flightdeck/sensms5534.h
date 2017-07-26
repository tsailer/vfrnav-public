//
// C++ Interface: MS5534 Sensor
//
// Description: MS5534 Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SENSMS5534_H
#define SENSMS5534_H

#include "sensors.h"
#include "baro.h"

class Sensors::SensorMS5534 : public Sensors::SensorInstance {
  public:
	SensorMS5534(Sensors& sensors, const Glib::ustring& configgroup);
	virtual ~SensorMS5534();

	void init(void);

	virtual void get_baroalt(double& alt, double& altrate) const;
	virtual unsigned int get_baroalt_priority(void) const;
	virtual Glib::TimeVal get_baroalt_time(void) const;

	virtual void get_param_desc(unsigned int pagenr, paramdesc_t& pd);
	using SensorInstance::get_param;
	virtual paramfail_t get_param(unsigned int nr, double& v) const;
	virtual paramfail_t get_param(unsigned int nr, int& v) const;
	using SensorInstance::set_param;
	virtual void set_param(unsigned int nr, int v);
	virtual void set_param(unsigned int nr, double v);

	virtual std::string logfile_header(void) const;
	virtual std::string logfile_records(void) const;

  protected:
        enum {
                parnrstart = SensorInstance::parnrend,
		parnrprio = parnrstart,
		parnraltfilter,
		parnraltratefilter,
		parnralt,
		parnraltrate,
		parnrpress,
		parnrtemp,
		parnrc1,
		parnrc2,
		parnrc3,
		parnrc4,
		parnrc5,
		parnrc6,
		parnrend
	};

	void ms5534_result(uint64_t cal, uint16_t press, uint16_t temp);

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
	int16_t m_cal[6];
};

#endif /* SENSMS5534_H */
