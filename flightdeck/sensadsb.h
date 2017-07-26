//
// C++ Interface: ADS-B Decoder
//
// Description: ADS-B Decoder
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SENSADSB_H
#define SENSADSB_H

#include "sensors.h"
#include "modes.h"

class Sensors::SensorADSB : public Sensors::SensorInstance {
  public:
	SensorADSB(Sensors& sensors, const Glib::ustring& configgroup);
	virtual ~SensorADSB();

	virtual bool get_position(Point& pt) const;
	virtual unsigned int get_position_priority(void) const;
	virtual Glib::TimeVal get_position_time(void) const;
	//virtual bool is_position_ok(const Glib::TimeVal& tv) const;
	//virtual bool is_position_ok(void) const;

	virtual void get_truealt(double& alt, double& altrate) const;
	virtual unsigned int get_truealt_priority(void) const;
	virtual Glib::TimeVal get_truealt_time(void) const;
	//virtual bool is_truealt_ok(const Glib::TimeVal& tv) const;
	//virtual bool is_truealt_ok(void) const;

	virtual void get_baroalt(double& alt, double& altrate) const;
	virtual unsigned int get_baroalt_priority(void) const;
	virtual Glib::TimeVal get_baroalt_time(void) const;
	//virtual bool is_baroalt_ok(const Glib::TimeVal& tv) const;
	//virtual bool is_baroalt_ok(void) const;

	virtual void get_course(double& crs, double& gs) const;
	virtual unsigned int get_course_priority(void) const;
	virtual Glib::TimeVal get_course_time(void) const;
	//virtual bool is_course_ok(const Glib::TimeVal& tv) const;
	//virtual bool is_course_ok(void) const;

	virtual void get_heading(double& hdg) const;
	virtual unsigned int get_heading_priority(void) const;
	virtual Glib::TimeVal get_heading_time(void) const;
	//virtual bool is_heading_ok(const Glib::TimeVal& tv) const;
	//virtual bool is_heading_ok(void) const;
	virtual bool is_heading_true(void) const;

	virtual void get_param_desc(unsigned int pagenr, paramdesc_t& pd);
	using SensorInstance::get_param;
	virtual paramfail_t get_param(unsigned int nr, Glib::ustring& v) const;
	virtual paramfail_t get_param(unsigned int nr, double& v) const;
	virtual paramfail_t get_param(unsigned int nr, int& v) const;
	virtual paramfail_t get_param(unsigned int nr, adsbtargets_t& targets) const;
	using SensorInstance::set_param;
	virtual void set_param(unsigned int nr, int v);
	virtual void set_param(unsigned int nr, const Glib::ustring& v);

  protected:
        enum {
                parnrstart = SensorInstance::parnrend,
		parnrtime = parnrstart,
		parnrlat,
		parnrlon,
		parnrbaroalt,
		parnrbaroaltrate,
		parnrgnssalt,
		parnrgnssaltrate,
		parnrcourse,
		parnrgroundspeed,
		parnrownship,
		parnrownshipreg,
		parnrownshipctry,
		parnrcorrectberr,
		parnrposprio,
		parnrbaroaltprio,
		parnrgnssaltprio,
		parnrcrsprio,
		parnrhdgprio,
		parnrtargets,
		parnrend
	};

	void receive(const ModeSMessage& msg);
	void clear(void);

	void expire_targets(ParamChanged& pc);
	bool expire(void);

	sigc::connection m_connexpire;
	adsbtargets_t m_targets;
	adsbtargets_t::const_iterator m_ownshipptr;
	uint32_t m_ownship;
	unsigned int m_correctbiterrors;
	unsigned int m_positionpriority;
	unsigned int m_baroaltitudepriority;
	unsigned int m_gnssaltitudepriority;
	unsigned int m_coursepriority;
	unsigned int m_headingpriority;
	std::ofstream m_tracefile;
};

#endif /* SENSADSB_H */
