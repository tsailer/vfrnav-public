//
// C++ Interface: Geoclue Sensor
//
// Description: Geoclue Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SENSGEOCLUE_H
#define SENSGEOCLUE_H

#include <geoclue/geoclue-master-client.h>
#include <geoclue/geoclue-position.h>

#include "sensors.h"

class Sensors::SensorGeoclue : public Sensors::SensorInstance {
  public:
	SensorGeoclue(Sensors& sensors, const Glib::ustring& configgroup);
	virtual ~SensorGeoclue();

	virtual bool get_position(Point& pt) const;
	virtual unsigned int get_position_priority(void) const;
	virtual Glib::TimeVal get_position_time(void) const;

	virtual void get_truealt(double& alt, double& altrate) const;
	virtual unsigned int get_truealt_priority(void) const;
	virtual Glib::TimeVal get_truealt_time(void) const;

	virtual void get_param_desc(unsigned int pagenr, paramdesc_t& pd);
	using SensorInstance::get_param;
	virtual paramfail_t get_param(unsigned int nr, Glib::ustring& v) const;
	virtual paramfail_t get_param(unsigned int nr, double& v) const;
	virtual paramfail_t get_param(unsigned int nr, int& v) const;
	using SensorInstance::set_param;
	virtual void set_param(unsigned int nr, int v);

	virtual std::string logfile_header(void) const;
	virtual std::string logfile_records(void) const;

  protected:
        enum {
                parnrstart = SensorInstance::parnrend,
		parnrfixtype = parnrstart,
		parnraccuracylevel,
		parnrtime,
		parnrlat,
		parnrlon,
		parnralt,
		parnrposprio0,
		parnrposprio1,
		parnrposprio2,
		parnrposprio3,
		parnrposprio4,
		parnrposprio5,
		parnrposprio6,
		parnraltprio0,
		parnraltprio1,
		parnraltprio2,
		parnraltprio3,
		parnraltprio4,
		parnraltprio5,
		parnraltprio6,
		parnrend
	};

	std::vector<int> m_positionpriority;
	std::vector<int> m_altitudepriority;
	GeoclueMasterClient *m_client;
	GeocluePosition *m_pos;
	Glib::TimeVal m_postime;
	Glib::TimeVal m_alttime;
	Point m_coord;
	double m_alt;
	bool m_coordok;
	GeoclueAccuracyLevel m_acclevel;

	static const std::string& to_str(GeoclueAccuracyLevel acclvl);
	static void position_changed_cb(GeocluePosition *position, GeocluePositionFields fields, int timestamp, double latitude, double longitude,
					double altitude, GeoclueAccuracy *accuracy, gpointer userdata);
	void position_changed(GeocluePosition *position, GeocluePositionFields fields, int timestamp, double latitude, double longitude,
			      double altitude, GeoclueAccuracy *accuracy);
};

#endif /* SENSGEOCLUE_H */
