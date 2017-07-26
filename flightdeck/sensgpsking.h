//
// C++ Interface: King GPS Sensor
//
// Description: GPS Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013, 2014
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SENSGPSKING_H
#define SENSGPSKING_H

#include "sysdeps.h"

#include <fstream>

#include "sensgps.h"

class Sensors::SensorKingGPS : public Sensors::SensorGPS {
  public:
	SensorKingGPS(Sensors& sensors, const Glib::ustring& configgroup);
	virtual ~SensorKingGPS();

	virtual bool get_position(Point& pt) const;
	virtual Glib::TimeVal get_position_time(void) const;

	virtual void get_truealt(double& alt, double& altrate) const;
	virtual Glib::TimeVal get_truealt_time(void) const;

	virtual void get_baroalt(double& alt, double& altrate) const;
	virtual unsigned int get_baroalt_priority(void) const;
	virtual Glib::TimeVal get_baroalt_time(void) const;

	virtual void get_course(double& crs, double& gs) const;
	virtual Glib::TimeVal get_course_time(void) const;

	virtual void get_param_desc(unsigned int pagenr, paramdesc_t& pd);
	using SensorGPS::get_param;
	virtual paramfail_t get_param(unsigned int nr, Glib::ustring& v) const;
	virtual paramfail_t get_param(unsigned int nr, double& v) const;
	virtual paramfail_t get_param(unsigned int nr, int& v) const;
	virtual paramfail_t get_param(unsigned int nr, gpsflightplan_t& fpl, unsigned int& curwpt) const;
	using SensorGPS::set_param;
	virtual void set_param(unsigned int nr, int v);

	virtual std::string logfile_header(void) const;
	virtual std::string logfile_records(void) const;

  protected:
	static const struct baudrates {
		unsigned int baud;
		unsigned int tbaud;
	} baudrates[8];

	class Parser {
	  public:
		class Data {
		  public:
			Data(void);
			const Point& get_position(void) const { return m_pos; }
			bool is_lat_set(void) const { return m_latset; }
			bool is_lon_set(void) const { return m_lonset; }
			bool is_position_ok(void) const { return is_lat_set() && is_lon_set(); }
			const gpsflightplan_t& get_fplan(void) const { return m_fplan; }
			gpsflightplan_t& get_fplan(void) { return m_fplan; }
			const std::string& get_swcode(void) const { return m_swcode; }
			const std::string& get_navflags(void) const { return m_navflags; }
			const std::string& get_flags(void) const { return m_flags; }
			const std::string& get_destination(void) const { return m_dest; }
			double get_gpsalt(void) const { return m_gpsalt; }
			double get_pressalt(void) const { return m_pressalt; }
			double get_baroalt(void) const { return m_baroalt; }
			double get_magtrack(void) const { return m_magtrack; }
			double get_dtk(void) const { return m_dtk; }
			double get_gs(void) const { return m_gs; }
			double get_dist(void) const { return m_dist; }
			double get_destdist(void) const { return m_destdist; }
			double get_xtk(void) const { return m_xtk; }
			double get_tke(void) const { return m_tke; }
			double get_brg(void) const { return m_brg; }
			double get_ptk(void) const { return m_ptk; }
			double get_actualvnav(void) const { return m_actualvnav; }
			double get_desiredvnav(void) const { return m_desiredvnav; }
			double get_verterr(void) const { return m_verterr; }
			double get_vertangleerr(void) const { return m_vertangleerr; }
			double get_poserr(void) const { return m_poserr; }
			double get_magvar(void) const { return m_magvar; }
			double get_msa(void) const { return m_msa; }
			double get_mesa(void) const { return m_mesa; }
			double get_timesincesolution(void) const { return m_timesincesolution; }
			const Glib::TimeVal& get_time(void) const { return m_time; }
			int get_utcoffset(void) const { return m_utcoffset; }
			unsigned int get_timewpt(void) const { return m_timewpt; }
			unsigned int get_leg(void) const { return m_leg; }
			unsigned int get_satellites(void) const { return m_satellites; }
			fixtype_t get_fixtype(void) const { return m_fixtype; }

			void set_lat(double x);
			void set_lon(double x);
			void set_swcode(const std::string& v) { m_swcode = v; }
			void set_navflags(const std::string& v) { m_navflags = v; }
			void set_flags(const std::string& v) { m_flags = v; }
			void set_dest(const std::string& v);
			void set_gpsalt(double v);
			void set_pressalt(double v) { m_pressalt = v; }
			void set_baroalt(double v) { m_baroalt = v; }
			void set_magtrack(double v) { m_magtrack = v; }
			void set_dtk(double v) { m_dtk = v; }
			void set_gs(double v) { m_gs = v; }
			void set_dist(double v) { m_dist = v; }
			void set_destdist(double v) { m_destdist = v; }
			void set_xtk(double v) { m_xtk = v; }
			void set_tke(double v) { m_tke = v; }
			void set_brg(double v) { m_brg = v; }
			void set_ptk(double v) { m_ptk = v; }
			void set_actualvnav(double v) { m_actualvnav = v; }
			void set_desiredvnav(double v) { m_desiredvnav = v; }
			void set_verterr(double v) { m_verterr = v; }
			void set_vertangleerr(double v) { m_vertangleerr = v; }
			void set_poserr(double v) { m_poserr = v; }
			void set_magvar(double v) { m_magvar = v; }
			void set_msa(double v) { m_msa = v; }
			void set_mesa(double v) { m_mesa = v; }
			void set_timesincesolution(double v) { m_timesincesolution = v; }
			void set_time(const Glib::TimeVal& v) { m_time = v; }
			void set_utcoffset(int v) { m_utcoffset = v; }
			void set_timewpt(unsigned int v) { m_timewpt = v; }
			void set_leg(unsigned int v) { m_leg = v; }
			void set_satellites(unsigned int v) { m_satellites = v; }
			void set_fixtype(fixtype_t v) { m_fixtype = v; }
			void set_wpttypes(const std::string& v) { m_wpttypes = v; }
			void set_dmy(int day, int month, int year);
			void set_hms(int hour, int min, double sec);
			bool is_dmy_valid(void) const;
			bool is_hms_valid(void) const;
			void finalize(void);

		  protected:
			gpsflightplan_t m_fplan;
			Point m_pos;
			std::string m_swcode;
			std::string m_navflags;
			std::string m_flags;
			std::string m_dest;
			std::string m_wpttypes;
			double m_gpsalt;
			double m_pressalt;
			double m_baroalt;
			double m_magtrack;
			double m_dtk;
			double m_gs;
			double m_dist;
			double m_destdist;
			double m_xtk;
			double m_tke;
			double m_brg;
			double m_ptk;
			double m_actualvnav;
			double m_desiredvnav;
			double m_verterr;
			double m_vertangleerr;
			double m_poserr;
			double m_magvar;
			double m_msa;
			double m_mesa;
			double m_timesincesolution;
			Glib::TimeVal m_time;
			int m_day;
			int m_month;
			int m_year;
			int m_hour;
			int m_minute;
			double m_seconds;
			int m_utcoffset;
			unsigned int m_timewpt;
			unsigned int m_leg;
			unsigned int m_satellites;
			fixtype_t m_fixtype;
			bool m_latset;
			bool m_lonset;
		};

		Parser(void);
		bool parse(const std::string& raw) { m_raw += raw; return parse(); }
		template <typename T> bool parse(T b, T e) { m_raw.insert(m_raw.end(), b, e); return parse(); }
		bool parse(void);
		operator const Data&(void) const { return m_data; }

		static const char stx = 2;
		static const char etx = 3;
		static const char cr = '\r';
		static const char lf = '\n';

	  protected:
		typedef bool (Parser::*parser_function_t)(std::string::const_iterator si, std::string::const_iterator se);
		static const struct parser_table {
			char item;
			unsigned int minlength;
			parser_function_t parsefunc;
		} parser_table[];

		static std::string::const_iterator skipws(std::string::const_iterator si, std::string::const_iterator se);
		static bool parsenum(double& v, std::string::const_iterator& si, std::string::const_iterator se, char sgnp = 0, char sgnn = 0);
		static bool parsenum(int& v, std::string::const_iterator& si, std::string::const_iterator se, char sgnp = 0, char sgnn = 0);
		static bool expect(char ch, std::string::const_iterator& si, std::string::const_iterator se);
		static bool parsestr(std::string& v, std::string::const_iterator& si, std::string::const_iterator se);

		void set_garmin_fix(void);

		bool parse_lat(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_lon(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_tk(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_gs(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_dis(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_ete(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_xtk(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_tke(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_dtk(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_leg(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_dest(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_brg(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_ptk(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_poserr(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_magvar(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_timesincesol(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_navflags(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_flags(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_msa(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_mesa(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_date(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_time(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_swcode(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_wpt(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_wpttype(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_latlongs(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_alt(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_trkerr(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_vnav(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_distbrgtime(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_loctime(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_poffs(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_exttime(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_exttimesincesol(std::string::const_iterator si, std::string::const_iterator se);
		bool parse_sensstat(std::string::const_iterator si, std::string::const_iterator se);

		Data m_data;
		std::string m_raw;
		bool m_waitstx;
	};

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
		parnrbaroaltprio0 = parnrstart,
		parnrbaroaltprio1,
		parnrbaroaltprio2,
		parnrbaroaltprio3,
		parnrbaroalt,
		parnrbaroaltrate,
		parnrpressalt,
		parnrdtk,
		parnrxtk,
		parnrtke,
		parnrbrg,
		parnrptk,
		parnrmagvar,
		parnrmsa,
		parnrmesa,
		parnrtimewpt,
		parnrdist,
		parnrdest,
		parnractualvnav,
		parnrdesiredvnav,
		parnrverterr,
		parnrvertangleerr,
		parnrsatellites,
		parnrposerr,
		parnrsolutiontime,
		parnrgpstime,
		parnrutcoffset,
		parnrleg,
		parnrfplwpt,
		parnrnavflags,
		parnrflags,
		parnrswcode,
		parnrend
	};

	void invalidate_gps(void);
	void invalidate_update_gps(void);
	void update_gps(ParamChanged& pc);
	void update_gps(void);
	bool parse(const std::string& raw);
	template <typename T> bool parse(T b, T e);
	bool parse(void);

	Parser m_parser;
	Parser::Data m_data;
        Glib::TimeVal m_time;
	double m_gpsaltrate;
	double m_baroaltrate;
	unsigned int m_baroaltitudepriority[4];
	std::ofstream m_tracefile;
};

#endif /* SENSGPSKING_H */
