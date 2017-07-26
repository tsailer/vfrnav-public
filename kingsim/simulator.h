//
// C++ Interface: King GPS Simulator
//
// Description: King GPS Simulator
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2014, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "sysdeps.h"
#include "engine.h"
#include "baro.h"

#include <glibmm.h>
#include <sigc++/sigc++.h>

class Simulator : public sigc::trackable {
  public:
	typedef enum {
		protocol_trimble,
		protocol_garmin
	} protocol_t;

	Simulator(Engine& engine, const std::string& file = "", bool dounlink = true, protocol_t protocol = protocol_trimble);
	~Simulator();

	bool command(const std::string& cmd);

  protected:
	static const char stx = 2;
	static const char etx = 3;

	class Waypoint {
	  public:
		Waypoint(const Point& coord = Point(), char type = 'u', const std::string& ident = "", float alt = 0.0);
		const Point& get_coord(void) const { return m_coord; }
		char get_type(void) const { return m_type; }
		float get_variation(void) const { return m_var; }
		std::string get_ident(void) const;
		std::string get_binary(void) const;

	  protected:
		Point m_coord;
		float m_var;
		uint8_t m_ident[5];
		char m_type;

	};

	Engine& m_engine;
	typedef std::vector<std::string> cmdline_t;
        typedef void (Simulator::*cmd_function_t)(const cmdline_t&);
        typedef std::map<std::string,cmd_function_t> cmdlist_t;
        cmdlist_t m_cmdlist;
	std::string m_devfile;
	int m_fd;
	bool m_terminate;
	Point m_pos;
	double m_alt;
	double m_verticalspeed;
	double m_targetalt;
	double m_groundspeed;
	double m_course;
        IcaoAtmosphere<float> m_athmospherestd;
        IcaoAtmosphere<float> m_athmosphereqnh;
	Glib::Cond m_dbfindcond;
	Glib::Mutex m_dbfindmutex;
	sigc::connection m_periodic;
	Glib::TimeVal m_lastperiodic;
	std::string m_txbuf;
	sigc::connection m_txconn;
	typedef std::vector<Waypoint> fplan_t;
	fplan_t m_fplan;
	fplan_t::size_type m_fplanidx;
	unsigned int m_verbose;

	typedef enum {
		eol_crlf,
		eol_cr,
		eol_lf
	} eol_t;
	eol_t m_eolmode;
	protocol_t m_protocol;

	typedef enum {
		findcoord_invalid,
		findcoord_coord,
		findcoord_airport,
		findcoord_military,
		findcoord_heliport,
		findcoord_intersection,
		findcoord_vor,
		findcoord_ndb
	} findcoord_t;
	static const std::string& to_str(findcoord_t ft);
	static char waypoint_type(findcoord_t ft);
	findcoord_t find_coord(const std::string& name, Point& coord);
	findcoord_t find_coord(const std::string& name, Point& coord, Glib::ustring& ident, int& alt);
	bool parse_coord(const std::string& name, Point& coord);
	void dbfind_done(void);
	bool periodic(void);
	bool serial_output(Glib::IOCondition iocond);
	const char *eol(void) const;

	void cmd_help(const cmdline_t& args);
	void cmd_quit(const cmdline_t& args);
	void cmd_qnh(const cmdline_t& args);
	void cmd_pos(const cmdline_t& args);
	void cmd_alt(const cmdline_t& args);
	void cmd_course(const cmdline_t& args);
	void cmd_gs(const cmdline_t& args);
	void cmd_fplan(const cmdline_t& args);
	void cmd_verbose(const cmdline_t& args);
};

#endif /* SIMULATOR_H */
