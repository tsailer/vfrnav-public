//
// C++ Interface: NMEA GPS Sensor
//
// Description: GPS Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SENSGPSNMEA_H
#define SENSGPSNMEA_H

#include "sysdeps.h"

#ifdef __MINGW32__
#include <windows.h>
#endif

#include <fstream>

#include "sensgps.h"

class Sensors::SensorNMEAGPS : public Sensors::SensorGPS {
  public:
	SensorNMEAGPS(Sensors& sensors, const Glib::ustring& configgroup);
	virtual ~SensorNMEAGPS();

	virtual bool get_position(Point& pt) const;
	virtual Glib::TimeVal get_position_time(void) const;

	virtual void get_truealt(double& alt, double& altrate) const;
	virtual Glib::TimeVal get_truealt_time(void) const;

	virtual void get_course(double& crs, double& gs) const;
	virtual Glib::TimeVal get_course_time(void) const;

	virtual void get_param_desc(unsigned int pagenr, paramdesc_t& pd);
	using SensorGPS::get_param;
	virtual paramfail_t get_param(unsigned int nr, Glib::ustring& v) const;
	virtual paramfail_t get_param(unsigned int nr, double& v) const;
	virtual paramfail_t get_param(unsigned int nr, int& v) const;
        virtual paramfail_t get_param(unsigned int nr, satellites_t& sat) const;
	using SensorGPS::set_param;
	virtual void set_param(unsigned int nr, const Glib::ustring& v);
	virtual void set_param(unsigned int nr, int v);

	virtual std::string logfile_header(void) const;
	virtual std::string logfile_records(void) const;

  protected:
#ifdef __MINGW32__
	static const struct baudrates {
		unsigned int baud;
		DWORD tbaud;
	} baudrates[8];
#else
	static const struct baudrates {
		unsigned int baud;
		unsigned int tbaud;
	} baudrates[8];
#endif

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
		parnrpdop,
		parnrhdop,
		parnrvdop,
		parnrgpstime,
		parnrdevice,
                parnrbaudrate,
		parnrend
	};

	static const unsigned int timeout_reconnect = 15;
	static const unsigned int timeout_timeoutreconnect = 5;
	static const unsigned int timeout_data = 2500;

	bool parse(const std::string& raw);
	template <typename T> bool parse(T b, T e);
	bool parse(void);
	bool parse_line(std::string::const_iterator lb, std::string::const_iterator le);
	static double parse_double(const std::string& s);
	static bool parse_uint(unsigned int& v, const std::string& s);
	typedef std::vector<std::string> tokens_t;
	bool parse_gpgga(const tokens_t& tok);
	bool parse_gprmc(const tokens_t& tok);
	bool parse_gpgsa(const tokens_t& tok);
	bool parse_gpgsv(const tokens_t& tok);

        void invalidate_update_gps(void);
        bool close(void);
        bool try_connect(void);
        bool gps_poll(Glib::IOCondition iocond);
        bool gps_timeout(void);

	static const char cr = '\r';
	static const char lf = '\n';
	std::string m_raw;
        Glib::TimeVal m_time;
        Glib::TimeVal m_fixtime;
	Point m_pos;
	double m_course;
	double m_gs;
	double m_alt;
	double m_altrate;
	double m_altfixtime;
	double m_pdop;
	double m_hdop;
	double m_vdop;
	satellites_t m_satellites;
	satellites_t m_satparse;
	std::set<unsigned int> m_satinuse;
        Glib::ustring m_device;
        sigc::connection m_conn;
        sigc::connection m_conntimeout;
#ifdef __MINGW32__
	HANDLE m_fd;
#else
        int m_fd;
#endif
        unsigned int m_baudrate;
	std::ofstream m_tracefile;
};

#endif /* SENSGPSNMEA_H */
