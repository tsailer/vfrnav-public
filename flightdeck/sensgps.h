//
// C++ Interface: GPS Sensor
//
// Description: GPS Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SENSGPS_H
#define SENSGPS_H

#include "sensors.h"

class Sensors::SensorGPS : public Sensors::SensorInstance {
  public:
	SensorGPS(Sensors& sensors, const Glib::ustring& configgroup);
	virtual ~SensorGPS();

	//virtual bool get_position(Point& pt) const;
	virtual unsigned int get_position_priority(void) const;
	//virtual Glib::TimeVal get_position_time(void) const;
	//virtual bool is_position_ok(const Glib::TimeVal& tv) const;
	//virtual bool is_position_ok(void) const;

	//virtual void get_truealt(double& alt, double& altrate) const;
	virtual unsigned int get_truealt_priority(void) const;
	//virtual Glib::TimeVal get_truealt_time(void) const;
	//virtual bool is_truealt_ok(const Glib::TimeVal& tv) const;
	//virtual bool is_truealt_ok(void) const;

	//virtual void get_course(double& crs, double& gs) const;
	virtual unsigned int get_course_priority(void) const;
	//virtual Glib::TimeVal get_course_time(void) const;
	//virtual bool is_course_ok(const Glib::TimeVal& tv) const;
	//virtual bool is_course_ok(void) const;

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
		parnrtime,
		parnrlat,
		parnrlon,
		parnralt,
		parnraltrate,
		parnrcourse,
		parnrgroundspeed,
		parnrposprio0,
		parnrposprio1,
		parnrposprio2,
		parnrposprio3,
		parnraltprio0,
		parnraltprio1,
		parnraltprio2,
		parnraltprio3,
		parnrcrsprio0,
		parnrcrsprio1,
		parnrcrsprio2,
		parnrcrsprio3,
		parnrend
	};

	typedef enum {
		fixtype_nofix,
		fixtype_2d,
		fixtype_3d,
		fixtype_3d_diff
	} fixtype_t;

	fixtype_t m_fixtype;
	unsigned int m_positionpriority[4];
	unsigned int m_altitudepriority[4];
	unsigned int m_coursepriority[4];

	static const std::string& to_str(fixtype_t ft);
};

#endif /* SENSGPS_H */
