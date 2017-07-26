//
// C++ Interface: GeoClue2 Sensor
//
// Description: GeoClue2 Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SENSGEOCLUE2_H
#define SENSGEOCLUE2_H

#include "geoclue2.h"

#include "sensors.h"

class Sensors::SensorGeoclue2 : public Sensors::SensorInstance {
  public:
	SensorGeoclue2(Sensors& sensors, const Glib::ustring& configgroup);
	virtual ~SensorGeoclue2();

	virtual bool get_position(Point& pt) const;
	virtual unsigned int get_position_priority(void) const;
	virtual Glib::TimeVal get_position_time(void) const;

	virtual void get_truealt(double& alt, double& altrate) const;
	virtual unsigned int get_truealt_priority(void) const;
	virtual Glib::TimeVal get_truealt_time(void) const;

	virtual void get_course(double& crs, double& gs) const;
	virtual Glib::TimeVal get_course_time(void) const;

	virtual void get_param_desc(unsigned int pagenr, paramdesc_t& pd);
	using SensorInstance::get_param;
	virtual paramfail_t get_param(unsigned int nr, Glib::ustring& v) const;
	virtual paramfail_t get_param(unsigned int nr, double& v) const;
	virtual paramfail_t get_param(unsigned int nr, int& v) const;
	using SensorInstance::set_param;
	virtual void set_param(unsigned int nr, double v);

	virtual std::string logfile_header(void) const;
	virtual std::string logfile_records(void) const;

  protected:
  
	typedef enum {
		GCLUE_ACCURACY_LEVEL_COUNTRY = 1,
		GCLUE_ACCURACY_LEVEL_CITY = 4,
		GCLUE_ACCURACY_LEVEL_NEIGHBORHOOD = 5,
		GCLUE_ACCURACY_LEVEL_STREET = 6,
		GCLUE_ACCURACY_LEVEL_EXACT = 8,
	} GClueAccuracyLevel;

	enum {
                parnrstart = SensorInstance::parnrend,
		parnrfixtype = parnrstart,
		parnrtime,
		parnrlat,
		parnrlon,
		parnralt,
		parnrcourse,
		parnrgroundspeed,
		parnraccuracy,
		parnrposprio0,
		parnrposprio1,
		parnrposprio2,
		parnrposprio3,
		parnraltprio0,
		parnraltprio1,
		parnraltprio2,
		parnraltprio3,
		parnrend
	};

	std::vector<double> m_positionpriority;
	std::vector<double> m_altitudepriority;
	GCancellable *m_cancellable;
	GeoClue2Manager *m_manager;
	GeoClue2Client *m_client;

	Glib::TimeVal m_postime;
	Glib::TimeVal m_alttime;
	Glib::TimeVal m_crstime;
	Point m_coord;
	double m_alt;
	double m_crs;
	double m_gs;
	double m_accuracy;
	bool m_coordok;
	guint m_minlevel;

	static const std::string& to_str(GClueAccuracyLevel acclvl);
	static void manager_get_client_cb(GObject *manager, GAsyncResult *res, gpointer user_data);
	void manager_get_client(GeoClue2Manager *manager, GAsyncResult *res);
	static void position_changed_cb(GeoClue2Client *client, const gchar *arg_old, const gchar *arg_new, gpointer userdata);
	void position_changed(GeoClue2Client *client, const gchar *arg_old, const gchar *arg_new);
	static void new_location_cb(GObject *client, GAsyncResult *res, gpointer user_data);
	void new_location(GeoClue2Client *client, GAsyncResult *res);

};

#endif /* SENSGEOCLUE2_H */
