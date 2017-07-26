//
// C++ Interface: nwxweather
//
// Description: nwx weather service
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef NWXWEATHER_H
#define NWXWEATHER_H

#include "sysdeps.h"

#include <limits>
#include <set>
#include <stdint.h>
#include <glibmm.h>
#include <gdkmm.h>
#include <libxml/tree.h>

#include "geom.h"
#include "fplan.h"
#include "wxdb.h"

class NWXWeather {
  public:
	NWXWeather(void);
	~NWXWeather(void);

	class WindsAloft {
	  public:
		WindsAloft(const Point& coord = Point(), int16_t alt = 0, uint16_t flags = 0, time_t tm = 0,
			   double winddir = std::numeric_limits<double>::quiet_NaN(),
			   double windspeed = std::numeric_limits<double>::quiet_NaN(),
			   double qff = std::numeric_limits<double>::quiet_NaN(),
			   double temp = std::numeric_limits<double>::quiet_NaN());
		WindsAloft(const FPlanWaypoint& wpt);

                Point get_coord(void) const { return m_coord; }
                Point::coord_t get_lon(void) const { return m_coord.get_lon(); }
                Point::coord_t get_lat(void) const { return m_coord.get_lat(); }
                void set_coord(const Point& c) { m_coord = c; }
                void set_lon(Point::coord_t lo) { m_coord.set_lon(lo); }
                void set_lat(Point::coord_t la) { m_coord.set_lat(la); }
                void set_lon_rad(float lo) { m_coord.set_lon_rad(lo); }
                void set_lat_rad(float la) { m_coord.set_lat_rad(la); }
                void set_lon_deg(float lo) { m_coord.set_lon_deg(lo); }
                void set_lat_deg(float la) { m_coord.set_lat_deg(la); }
                void set_lon_deg_dbl(double lo) { m_coord.set_lon_deg_dbl(lo); }
                void set_lat_deg_dbl(double la) { m_coord.set_lat_deg_dbl(la); }
                int16_t get_altitude(void) const { return m_alt; }
                void set_altitude(int16_t a) { m_alt = a; }
                uint16_t get_flags(void) const { return m_flags; }
                void set_flags(uint16_t f) { m_flags = f; }
                time_t get_time(void) const { return m_time; }
                void set_time(time_t t) { m_time = t; }
		double get_winddir(void) const { return m_winddir; }
		void set_winddir(double x) { m_winddir = x; }
		double get_windspeed(void) const { return m_windspeed; }
		void set_windspeed(double x) { m_windspeed = x; }
		double get_qff(void) const { return m_qff; }
		void set_qff(double x) { m_qff = x; }
		double get_temp(void) const { return m_temp; }
		void set_temp(double x) { m_temp = x; }

	  protected:
		Point m_coord;
		int16_t m_alt;
		uint16_t m_flags;
		time_t m_time;
		double m_winddir;
		double m_windspeed;
		double m_qff;
		double m_temp;
	};

	typedef std::vector<WindsAloft> windvector_t;

	class EpochsLevels {
	  public:
		typedef std::set<time_t> epochs_t;
		typedef epochs_t::iterator epoch_iterator;
		typedef epochs_t::const_iterator epoch_const_iterator;
		typedef epochs_t::reverse_iterator epoch_reverse_iterator;
		typedef epochs_t::const_reverse_iterator epoch_const_reverse_iterator;
		typedef std::set<unsigned int> levels_t;
		typedef levels_t::iterator level_iterator;
		typedef levels_t::const_iterator level_const_iterator;
		typedef levels_t::reverse_iterator level_reverse_iterator;
		typedef levels_t::const_reverse_iterator level_const_reverse_iterator;

		EpochsLevels(void);
		epoch_iterator epoch_begin() { return m_epochs.begin(); }
		epoch_const_iterator epoch_begin() const { return m_epochs.begin(); }
		epoch_iterator epoch_end() { return m_epochs.end(); }
		epoch_const_iterator epoch_end() const { return m_epochs.end(); }
		epoch_reverse_iterator epoch_rbegin() { return m_epochs.rbegin(); }
		epoch_const_reverse_iterator epoch_rbegin() const { return m_epochs.rbegin(); }
		epoch_reverse_iterator epoch_rend() { return m_epochs.rend(); }
		epoch_const_reverse_iterator epoch_rend() const { return m_epochs.rend(); }
		epoch_iterator epoch_find_nearest(time_t t);
		epoch_const_iterator epoch_find_nearest(time_t t) const;
		level_iterator level_begin() { return m_levels.begin(); }
		level_const_iterator level_begin() const { return m_levels.begin(); }
		level_iterator level_end() { return m_levels.end(); }
		level_const_iterator level_end() const { return m_levels.end(); }
		level_reverse_iterator level_rbegin() { return m_levels.rbegin(); }
		level_const_reverse_iterator level_rbegin() const { return m_levels.rbegin(); }
		level_reverse_iterator level_rend() { return m_levels.rend(); }
		level_const_reverse_iterator level_rend() const { return m_levels.rend(); }
		level_iterator level_find_nearest(unsigned int l);
		level_const_iterator level_find_nearest(unsigned int l) const;

		static time_t parse_epoch(xmlChar *txt);
		void add_epoch(time_t t);
		void add_epoch(xmlChar *txt);
		void add_level(unsigned int l);
		void add_level(xmlChar *txt);

	  protected:
		epochs_t m_epochs;
		levels_t m_levels;
	};

	typedef WXDatabase::MetarTaf MetarTaf;

	void abort(void);
	void winds_aloft(const FPlanRoute& route, const sigc::slot<void,const windvector_t&>& cb);
	void epochs_and_levels(const sigc::slot<void,const EpochsLevels&>& cb);
	typedef std::set<MetarTaf> metartaf_t;
	void metar_taf(const FPlanRoute& route, const sigc::slot<void,const metartaf_t&>& cb,
		       unsigned int metar_maxage = 0, unsigned int metar_count = 2, unsigned int taf_maxage = 0, unsigned int taf_count = 1);
	typedef std::vector<std::string> themes_t;
	void chart(const FPlanRoute& route, const Rect& bbox, time_t epoch, unsigned int level, const themes_t& themes, bool svg,
		   const sigc::slot<void,const std::string&,const std::string&,time_t,unsigned int>& cb, unsigned int width = 1024, unsigned int height = 768);
	void profile_graph(const FPlanRoute& route, time_t epoch, const themes_t& themes, bool svg,
			   const sigc::slot<void,const std::string&,const std::string&,time_t>& cb, unsigned int width = 1024, unsigned int height = 768);
	void profile_graph(const FPlanRoute& route, bool svg, const sigc::slot<void,const std::string&,const std::string&,time_t>& cb,
			   unsigned int width = 1024, unsigned int height = 768);

  protected:
	static const char nwxweather_url[];
	Glib::Dispatcher m_dispatch;
	sigc::slot<void,const windvector_t&> m_windsaloft_done;
	sigc::slot<void,const EpochsLevels&> m_epochslevels_done;
	sigc::slot<void,const metartaf_t&> m_metartaf_done;
	sigc::slot<void,const std::string&,const std::string&,time_t,unsigned int> m_charts_done;
	std::vector<FPlanWaypoint> m_route;
	unsigned int m_width;
	unsigned int m_height;
	time_t m_profilegraph_epoch;
	std::vector<char> m_txbuffer;
	std::vector<char> m_rxbuffer;
	Glib::Threads::Thread *m_thread;
	typedef enum {
		mode_abort,
		mode_windsaloft,
		mode_epochslevels,
		mode_metartaf,
		mode_chart,
		mode_pgepochs
	} mode_t;
	mode_t m_mode;
	bool m_profilegraph_svg;

	void complete(void);
	static size_t my_write_func_1(const void *ptr, size_t size, size_t nmemb, NWXWeather *wx);
	static size_t my_read_func_1(void *ptr, size_t size, size_t nmemb, NWXWeather *wx);
	static int my_progress_func_1(NWXWeather *wx, double t, double d, double ultotal, double ulnow);
	size_t my_write_func(const void *ptr, size_t size, size_t nmemb);
	size_t my_read_func(void *ptr, size_t size, size_t nmemb);
	int my_progress_func(double t, double d, double ultotal, double ulnow);
	static xmlDocPtr empty_request(xmlNodePtr& reqnode);
	void start_request(xmlDocPtr doc, mode_t mode);
	void curlio(void);
	static bool add_route(xmlNodePtr node, const FPlanRoute& route);
	static bool add_route(xmlNodePtr node, const std::vector<FPlanWaypoint>& route);
	static void add_waypoint(xmlNodePtr node, const FPlanWaypoint& wpt);
	void parse_windsaloft(xmlDocPtr doc, xmlNodePtr reqnode);
	void parse_epochslevels(xmlDocPtr doc, xmlNodePtr reqnode);
	void parse_metartaf(xmlDocPtr doc, xmlNodePtr reqnode);
	void parse_chart(xmlDocPtr doc, xmlNodePtr reqnode);
	void parse_profilegraph_epochs(xmlDocPtr doc, xmlNodePtr reqnode);
	void parse_profilegraph(xmlDocPtr doc, xmlNodePtr reqnode);
};

#endif /* NWXWEATHER_H */
