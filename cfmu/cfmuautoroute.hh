#ifndef CFMUAUTOROUTE_H
#define CFMUAUTOROUTE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "sysdeps.h"
#include "fplan.h"
#include "dbobj.h"
#include "engine.h"
#include "aircraft.h"
#include "baro.h"
#include "wind.h"
#include "opsperf.h"

#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <glibmm.h>
#include <giomm.h>
#include <sigc++/sigc++.h>

// TODO:
// Path Probing
// Implement Area deletion (for EDGGADF)
// Implement per country DCT limit
// Implement SID/STAR for VFR and IFR

class CFMUAutoroute : public sigc::trackable {
public:
	CFMUAutoroute();
	virtual ~CFMUAutoroute();

	void set_db_directories(const std::string& dir_main = "", Engine::auxdb_mode_t auxdbmode = Engine::auxdb_prefs, const std::string& dir_aux = "");
	const std::string& get_db_maindir(void) const { return m_dbdir_main; }
	const std::string& get_db_auxdir(void) const { return m_dbdir_aux; }
	Preferences& get_prefs(void) { return m_prefs; }

	Aircraft& get_aircraft(void) { return m_aircraft; }
	const Aircraft& get_aircraft(void) const { return m_aircraft; }

	GRIB2& get_grib2(void) { return m_grib2; }
	const GRIB2& get_grib2(void) const { return m_grib2; }

	typedef enum {
		opttarget_time,
		opttarget_fuel,
		opttarget_preferred
	} opttarget_t;
	void set_opttarget(opttarget_t t = opttarget_time);
	opttarget_t get_opttarget(void) const { return m_opttarget; }

	typedef std::set<std::string> sidstarfilter_t;

	const AirportsDb::Airport& get_departure(void) const { return m_airport[0]; }
	void set_departure(const AirportsDb::Airport& el);
	bool set_departure(const std::string& icao, const std::string& name);
	bool get_departure_ifr(void) const { return m_airportifr[0]; }
	void set_departure_ifr(bool ifr);
	const Point& get_sid(void) const { return m_airportconn[0]; }
	void set_sid(const Point& pt = Point::invalid);
	bool set_sid(const std::string& name);
	Engine::AirwayGraphResult::Vertex::type_t get_sidtype(void) const { return m_airportconntype[0]; }
	const std::string& get_sidident(void) const { return m_airportconnident[0]; }
	double get_sidlimit(void) const { return m_airportconnlimit[0]; }
	void set_sidlimit(double l = 20.0);
	double get_sidpenalty(void) const { return m_airportconnpenalty[0]; }
	void set_sidpenalty(double p = 1.0);
	double get_sidoffset(void) const { return m_airportconnoffset[0]; }
	void set_sidoffset(double o = 0.0);
	double get_sidminimum(void) const { return m_airportconnminimum[0]; }
	void set_sidminimum(double m = 0.0);
	bool get_siddb(void) const { return m_airportconndb[0]; }
	void set_siddb(bool db);
	bool get_sidonly(void) const { return m_airportproconly[0]; }
	void set_sidonly(bool o);
	const sidstarfilter_t get_sidfilter(void) const { return m_airportconnfilter[0]; }
	void set_sidfilter(const sidstarfilter_t& filt);
	void clear_sidfilter(void);
	void add_sidfilter(const std::string& procname);

	const AirportsDb::Airport& get_destination(void) const { return m_airport[1]; }
	void set_destination(const AirportsDb::Airport& el);
	bool set_destination(const std::string& icao, const std::string& name);
	bool get_destination_ifr(void) const { return m_airportifr[1]; }
	void set_destination_ifr(bool ifr);
	const Point& get_star(void) const { return m_airportconn[1]; }
	void set_star(const Point& pt = Point::invalid);
	bool set_star(const std::string& name);
	Engine::AirwayGraphResult::Vertex::type_t get_startype(void) const { return m_airportconntype[1]; }
	const std::string& get_starident(void) const { return m_airportconnident[1]; }
	double get_starlimit(void) const { return m_airportconnlimit[1]; }
	void set_starlimit(double l = 20.0);
	double get_starpenalty(void) const { return m_airportconnpenalty[1]; }
	void set_starpenalty(double p = 1.0);
	double get_staroffset(void) const { return m_airportconnoffset[1]; }
	void set_staroffset(double o = 0.0);
	double get_starminimum(void) const { return m_airportconnminimum[1]; }
	void set_starminimum(double m = 0.0);
	bool get_stardb(void) const { return m_airportconndb[1]; }
	void set_stardb(bool db);
	bool get_staronly(void) const { return m_airportproconly[1]; }
	void set_staronly(bool o);
	const sidstarfilter_t get_starfilter(void) const { return m_airportconnfilter[1]; }
	void set_starfilter(const sidstarfilter_t& filt);
	void clear_starfilter(void);
	void add_starfilter(const std::string& procname);

	const std::string& get_alternate(unsigned int nr) const;
	void set_alternate(unsigned int nr, const std::string& icao);

	class Crossing {
	public:
		static constexpr double maxradius = 50;
		Crossing() : m_radius(0), m_minlevel(0), m_maxlevel(600), m_type(Engine::AirwayGraphResult::Vertex::type_undefined) { m_coord.set_invalid(); }
		const std::string& get_ident(void) const { return m_ident; }
		void set_ident(const std::string& x = "") { m_ident = x; }
		const Point& get_coord(void) const { return m_coord; }
		void set_coord(const Point& pt) { m_coord = pt; }
		void set_coord(void) { m_coord.set_invalid(); }
		double get_radius(void) const { return m_radius; }
		void set_radius(double r = 0) { m_radius = std::max(std::min(r, maxradius), 0.0); }
		int get_minlevel(void) const { return m_minlevel; }
		int get_maxlevel(void) const { return m_maxlevel; }
		void set_level(int minlevel = 0, int maxlevel = 600) {
			m_minlevel = minlevel;
			m_maxlevel = maxlevel;
			if (m_maxlevel < m_minlevel)
				std::swap(m_maxlevel, m_minlevel);
		}
		Engine::AirwayGraphResult::Vertex::type_t get_type(void) const { return m_type; }
		void set_type(Engine::AirwayGraphResult::Vertex::type_t typ = Engine::AirwayGraphResult::Vertex::type_undefined) { m_type = typ; }

	protected:
		std::string m_ident;
		Point m_coord;
		double m_radius;
		int m_minlevel;
		int m_maxlevel;
		Engine::AirwayGraphResult::Vertex::type_t m_type;
	};
	typedef std::vector<Crossing> crossing_t;

	void set_crossing(unsigned int index, const Point& pt);
	bool set_crossing(unsigned int index, const std::string& ident);
	void set_crossing_radius(unsigned int index, double r = 0);
	void set_crossing_level(unsigned int index, int minlevel = 0, int maxlevel = 600);
	const Point& get_crossing(unsigned int index) const;
	const std::string& get_crossing_ident(unsigned int index) const;
	double get_crossing_radius(unsigned int index) const;
	int get_crossing_minlevel(unsigned int index) const;
	int get_crossing_maxlevel(unsigned int index) const;
	Engine::AirwayGraphResult::Vertex::type_t get_crossing_type(unsigned int index) const;
	unsigned int get_crossing_size(void) const { return m_crossing.size(); }
	void set_crossing_size(unsigned int sz);

	double get_dctlimit(void) const { return m_dctlimit; }
	void set_dctlimit(double l);
	double get_dctpenalty(void) const { return m_dctpenalty; }
	void set_dctpenalty(double l);
	double get_dctoffset(void) const { return m_dctoffset; }
	void set_dctoffset(double l);
	double get_vfraspclimit(void) const { return m_vfraspclimit; }
	void set_vfraspclimit(double l);

	int get_baselevel(void) const { return m_levels[0]; }
	int get_toplevel(void) const { return m_levels[1]; }
	void set_levels(int base, int top);
	double get_maxdescent(void) const { return m_maxdescent; }
	void set_maxdescent(double d);
	bool get_honour_levelchangetrackmiles(void) const { return m_honourlevelchangetrackmiles; }
	void set_honour_levelchangetrackmiles(bool lvltrk);
	bool get_honour_opsperftrackmiles(void) const { return m_honouropsperftrackmiles; }
	void set_honour_opsperftrackmiles(bool lvltrk);

	bool get_honour_awy_levels(void) const { return m_honourawylevels; }
	void set_honour_awy_levels(bool awylvl);
	bool get_honour_profilerules(void) const { return m_honourprofilerules; }
	void set_honour_profilerules(bool prules);

	void set_preferred_level(int level);
	int get_preferred_level(void) const { return m_preferredlevel; }
	void set_preferred_penalty(double p);
	double get_preferred_penalty(void) const { return m_preferredpenalty; }
	void set_preferred_climb(double p);
	double get_preferred_climb(void) const { return m_preferredclimb; }
	void set_preferred_descent(double p);
	double get_preferred_descent(void) const { return m_preferreddescent; }

	class ExcludeRegion {
	public:
		ExcludeRegion(const std::string& aspcid, const std::string& aspctype, int minlevel = 0, int maxlevel = 999,
			      double awylimit = 0, double dctlimit = 0, double dctoffset = 0, double dctscale = 1)
			: m_aspcid(aspcid), m_aspctype(aspctype), m_minlevel(minlevel), m_maxlevel(maxlevel),
			  m_awylimit(awylimit), m_dctlimit(dctlimit), m_dctoffset(dctoffset), m_dctscale(dctscale) {}
		ExcludeRegion(const Rect& bbox, int minlevel = 0, int maxlevel = 999,
			      double awylimit = 0, double dctlimit = 0, double dctoffset = 0, double dctscale = 1)
			: m_bbox(bbox), m_minlevel(minlevel), m_maxlevel(maxlevel),
			  m_awylimit(awylimit), m_dctlimit(dctlimit), m_dctoffset(dctoffset), m_dctscale(dctscale) {}
		bool is_airspace(void) const { return !(m_aspcid.empty() && m_aspctype.empty()); }
		const std::string& get_airspace_id(void) const { return m_aspcid; }
		const std::string& get_airspace_type(void) const { return m_aspctype; }
		const Rect& get_bbox(void) const { return m_bbox; }
		int get_minlevel(void) const { return m_minlevel; }
		int get_maxlevel(void) const { return m_maxlevel; }
		double get_awylimit(void) const { return m_awylimit; }
		double get_dctlimit(void) const { return m_dctlimit; }
		double get_dctoffset(void) const { return m_dctoffset; }
		double get_dctscale(void) const { return m_dctscale; }

	protected:
		std::string m_aspcid;
		std::string m_aspctype;
		Rect m_bbox;
		int m_minlevel;
		int m_maxlevel;
		double m_awylimit;
		double m_dctlimit;
		double m_dctoffset;
		double m_dctscale;
	};
	typedef std::vector<ExcludeRegion> excluderegions_t;

	const excluderegions_t& get_excluderegions(void) const { return m_excluderegions; }
	void set_excluderegions(const excluderegions_t& r);
	void add_excluderegion(const ExcludeRegion& r);
	void clear_excluderegions(void);

	bool get_force_enroute_ifr(void) const { return m_forceenrifr; }
	void set_force_enroute_ifr(bool ifr);

	bool get_tfr_available(void) const { return m_tfravailable; }
	bool get_tfr_enabled(void) const { return m_tfrenable; }
	void set_tfr_enabled(bool ena = true) {	m_tfrenable = ena; }
	bool get_tfr_available_enabled(void) const { return get_tfr_available() && get_tfr_enabled(); }

	void set_tfr_trace(const std::string& rules);
	std::string get_tfr_trace(void) const;
	void set_tfr_disable(const std::string& rules);
	std::string get_tfr_disable(void) const;
	void set_tfr_savefile(const std::string& fn) { m_tfrsavefile = fn; }
	std::string get_tfr_savefile(void) const { return m_tfrsavefile; }

	bool get_precompgraph_enabled(void) const { return m_precompgraphenable; }
	void set_precompgraph_enabled(bool ena = true);

	bool get_wind_enabled(void) const { return m_windenable; }
	void set_wind_enabled(bool ena = true);

	double get_qnh(void) const { return m_qnh; }
	void set_qnh(double q = IcaoAtmosphere<double>::std_sealevel_pressure);
	double get_isaoffs(void) const { return m_isaoffs; }
	void set_isaoffs(double t = 0);
	double get_engine_rpm(void) const { return m_enginerpm; }
	void set_engine_rpm(double l);
	double get_engine_mp(void) const { return m_enginemp; }
	void set_engine_mp(double l);
	double get_engine_bhp(void) const { return m_enginebhp; }
	void set_engine_bhp(double l);

	time_t get_deptime(void) const { return m_route.get_time_offblock_unix(); }
	void set_deptime(time_t t);
	unsigned int get_maxlocaliteration(void) const { return m_maxlocaliteration; }
	void set_maxlocaliteration(unsigned int i = ~0U) { m_maxlocaliteration = i; }
	unsigned int get_maxremoteiteration(void) const { return m_maxremoteiteration; }
	void set_maxremoteiteration(unsigned int i = ~0U) { m_maxremoteiteration = i; }

	typedef enum {
		validator_default,
		validator_cfmu,
		validator_eurofpl
	} validator_t;

	validator_t get_validator(void) const { return m_validator; }
	void set_validator(validator_t v = validator_cfmu);
	std::string get_validator_socket(void) const;
	void set_validator_socket(const std::string& path = "");
	const std::string& get_validator_binary(void) const { return m_childbinary; }
	void set_validator_binary(const std::string& path = "");
	int get_validator_xdisplay(void) const { return m_childxdisplay; }
	void set_validator_xdisplay(int xdisp = -1);

	const std::string& get_logprefix(void) const { return m_logprefix; }
	void set_logprefix(const std::string& pfx) { m_logprefix = pfx; }
	const std::string& get_logdir(void) const { return m_logdir; }

	const FPlanRoute& get_route(void) const { return m_route; }
	std::string get_simplefpl(void) const;
	typedef std::vector<std::string> validationresponse_t;
	const validationresponse_t& get_validationresponse(void) const { return m_validationresponse; }

	double get_gcdistance(void) const;
	double get_routedistance(void) const;
	unsigned int get_mintime(bool wind = true) const;
	unsigned int get_routetime(bool wind = true) const;
	double get_minfuel(bool wind = true) const;
	double get_routefuel(bool wind = true) const;
	unsigned int get_local_iterationcount(void) const { return m_iterationcount[0]; }
	unsigned int get_remote_iterationcount(void) const { return m_iterationcount[1]; }
	unsigned int get_iterationcount(void) const { return get_local_iterationcount() + get_remote_iterationcount(); }

	void get_cruise_performance(int& alt, double& densityalt, double& tas) const;

	bool is_running(void) const;
	bool is_errorfree(void) const;

	Glib::TimeVal get_wallclocktime(void) const;
	Glib::TimeVal get_validatortime(void) const;

	typedef enum {
		statusmask_none                          = 0,
		statusmask_starting                      = 1U << 0,
		statusmask_stoppingdone                  = 1U << 1,
		statusmask_stoppingerrorsid              = 1U << 2,
		statusmask_stoppingerrorstar             = 1U << 3,
		statusmask_stoppingerrorenroute          = 1U << 4,
		statusmask_stoppingerrorvalidatortimeout = 1U << 5,
		statusmask_stoppingerrorinternalerror    = 1U << 6,
		statusmask_stoppingerroriteration        = 1U << 7,
		statusmask_stoppingerroruser             = 1U << 8,
		statusmask_newfpl                        = 1U << 9,
		statusmask_newvalidateresponse           = 1U << 10,
		statusmask_stoppingerror = (statusmask_stoppingerrorsid | statusmask_stoppingerrorstar | statusmask_stoppingerrorenroute |
					    statusmask_stoppingerrorvalidatortimeout | statusmask_stoppingerrorinternalerror |
					    statusmask_stoppingerroriteration | statusmask_stoppingerroruser)
	} statusmask_t;
	sigc::signal<void,statusmask_t> signal_statuschange(void) { return m_signal_statuschange; }

	typedef enum {
		log_fplproposal,
		log_fpllocalvalidation,
		log_fplremotevalidation,
		log_graphrule,
		log_graphruledesc,
		log_graphruleoprgoal,
		log_graphchange,
		log_precompgraph,
		log_weatherdata,
		log_normal,
		log_debug0,
		log_debug1
	} log_t;
	sigc::signal<void,log_t,const std::string&> signal_log(void) { return m_signal_log; }

	virtual void preload(bool validator = true);
	virtual void reload(void);
	virtual void start(void);
	virtual void cont(void);
	virtual void stop(statusmask_t sm);
	virtual void clear(void);

	virtual void precompute_graph(const Rect& bbox, const std::string& fn) = 0;

protected:
	class Performance {
	public:
		class Cruise {
		public:
			Cruise(int alt = 0, double da = 0, double ta = 0, double rpm = 0, double mp = 0,
			       double secpernmi = 0, double fuelpersec = 0, double metricpernmi = 0,
			       Glib::RefPtr<GRIB2::LayerResult> windu = Glib::RefPtr<GRIB2::LayerResult>(),
			       Glib::RefPtr<GRIB2::LayerResult> windv = Glib::RefPtr<GRIB2::LayerResult>(),
			       Glib::RefPtr<GRIB2::LayerResult> temp = Glib::RefPtr<GRIB2::LayerResult>());

			int get_altitude(void) const { return m_altitude; }
			double get_densityaltitude(void) const { return m_densityaltitude; }
			double get_truealtitude(void) const { return m_truealt; }
			static std::string get_fplalt(int alt);
			std::string get_fplalt(void) const { return get_fplalt(get_altitude()); }

			// Cruise Performance
			double get_secpernmi(void) const { return m_secpernmi; }
			double get_fuelpersec(void) const { return m_fuelpersec; }
			double get_metricpernmi(void) const { return m_metricpernmi; }

			// For FPL
			double get_tas(void) const { return 3600.0 / m_secpernmi; }
			double get_inverse_tas(void) const { return (1.0 / 3600.0) * m_secpernmi; }
			static std::string get_fpltas(double tas);
			std::string get_fpltas(void) const { return get_fpltas(get_tas()); }

			// Wind
			bool is_wind(void) const { return m_windu && m_windv; }
			Wind<double> get_wind(const Point& pt) const;
			bool is_temp(void) const { return !!m_temp; }
			double get_temp(const Point& pt) const;

			// Engine
			double get_rpm(void) const { return m_rpm; }
			double get_mp(void) const { return m_mp; }

		protected:
			Glib::RefPtr<GRIB2::LayerResult> m_windu;
			Glib::RefPtr<GRIB2::LayerResult> m_windv;
			Glib::RefPtr<GRIB2::LayerResult> m_temp;
			double m_secpernmi;
			double m_fuelpersec;
			double m_metricpernmi;
			double m_rpm;
			double m_mp;
			double m_densityaltitude;
			double m_truealt;
			int m_altitude;
		};

		class LevelChange {
		public:
			LevelChange(double tracknmi = 0, double timepenalty = 0, double fuelpenalty = 0, double metricpenalty = 0, double optracknmi = 0);
			double get_tracknmi(void) const { return m_tracknmi; }
			double get_timepenalty(void) const { return m_timepenalty; }
			double get_fuelpenalty(void) const { return m_fuelpenalty; }
			double get_metricpenalty(void) const { return m_metricpenalty; }
			double get_opsperftracknmi(void) const { return m_opsperftracknmi; }

		protected:
			double m_tracknmi;
			double m_timepenalty;
			double m_fuelpenalty;
			double m_metricpenalty;
			double m_opsperftracknmi;
		};

		Performance(void);
		bool empty(void) const { return m_cruise.empty(); }
		unsigned int size(void) const { return m_cruise.size(); }
		const Cruise& get_cruise(unsigned int pi) const;
		const Cruise& get_cruise(unsigned int piu, unsigned int piv) const;
		const LevelChange& get_levelchange(unsigned int piu, unsigned int piv) const;
		void clear(void);
		void add(const Cruise& x);
		void set(unsigned int piu, unsigned int piv, const LevelChange& lvlchg);
		void resize(unsigned int pis);
		unsigned int findcruiseindex(int alt) const;
		int get_minaltitude(void) const;
		int get_maxaltitude(void) const;
		const Cruise& get_lowest_cruise(void) const;
		const Cruise& get_highest_cruise(void) const;

	protected:
		typedef std::vector<Cruise> cruise_t;
		cruise_t m_cruise;
		typedef std::vector<std::vector<LevelChange> > levelchange_t;
		levelchange_t m_levelchange;
		static const Cruise nullcruise;
	};

	class VFRRouter {
	public:
		typedef Engine::AirwayGraphResult::Graph Graph;
		typedef Engine::AirwayGraphResult::Vertex Vertex;
		typedef Engine::AirwayGraphResult::Edge Edge;

		VFRRouter(CFMUAutoroute& ar);
		bool load_graph(const Rect& bbox);
		bool set_departure(const AirportsDb::Airport& arpt);
		bool set_destination(const AirportsDb::Airport& arpt);
		void add_dct(double dctlim, double sidlim, double starlim);
		bool add_sid(const Point& pt = Point::invalid);
		bool add_sid(const AirportsDb::Airport& arpt);
		bool add_star(const Point& pt = Point::invalid);
		bool add_star(const AirportsDb::Airport& arpt);
		void set_metric(void);
		void exclude_airspace(int baselevel, int toplevel, double maxarea = std::numeric_limits<double>::max());
		void exclude_regions(void);
		bool autoroute(FPlanRoute& route);
		const Graph& get_graph(void) const { return m_graph; }
		Graph& get_graph(void) { return m_graph; }

	protected:
		CFMUAutoroute& m_ar;
		Graph m_graph;
		Rect m_bbox;
		Graph::vertex_descriptor m_dep;
		Graph::vertex_descriptor m_dest;
		typedef std::vector<AirportsDb::Airport::VFRRoute> sidstar_t;
		sidstar_t m_sid;
		sidstar_t m_star;

		Graph::vertex_descriptor find_airport(const AirportsDb::Airport& arpt);

		void remove_sid(void);
		void remove_star(void);
	};

        Preferences m_prefs;
	AirportsDb m_airportdb;
	NavaidsDb m_navaiddb;
	WaypointsDb m_waypointdb;
	AirwaysDb m_airwaydb;
	AirspacesDb m_airspacedb;
	MapelementsDb m_mapelementdb;
	TopoDb30 m_topodb;
	GRIB2 m_grib2;
	OperationsPerformance m_opsperf;
	FPlan m_fplandb;
	typedef std::set<std::string> ruleset_t;
	ruleset_t m_systemdisabledrules;
	ruleset_t m_systemtracerules;
	ruleset_t m_disabledrules;
	ruleset_t m_tracerules;
	std::string m_tfrsavefile;
	std::string m_logprefix;
	std::string m_logdir;
	std::string m_dbdir_main;
	std::string m_dbdir_aux;
	Aircraft m_aircraft;
	FPlanRoute m_route;
	sigc::signal<void,statusmask_t> m_signal_statuschange;
	sigc::signal<void,log_t,const std::string&> m_signal_log;
	Performance m_performance;
	Glib::RefPtr<GRIB2::LayerResult> m_grib2prmsl;
	unsigned int m_routetime;
	unsigned int m_routezerowindtime;
	double m_routefuel;
	double m_routezerowindfuel;
	AirportsDb::Airport m_airport[2];
	Point m_airportconn[2];
	std::string m_airportconnident[2];
	sidstarfilter_t m_airportconnfilter[2];
	double m_airportconnlimit[2];
	double m_airportconnpenalty[2];
	double m_airportconnoffset[2];
	double m_airportconnminimum[2];
	Engine::AirwayGraphResult::Vertex::type_t m_airportconntype[2];
	bool m_airportconndb[2];
	bool m_airportifr[2];
	bool m_airportproconly[2];
	std::string m_alternate[2];
	crossing_t m_crossing;
	double m_maxdescent;
	double m_dctlimit;
	double m_dctpenalty;
	double m_dctoffset;
	double m_vfraspclimit;
	double m_qnh;
	double m_isaoffs;
	double m_enginerpm;
	double m_enginemp;
	double m_enginebhp;
	int m_levels[2];
	time_t m_starttime;
	int m_preferredlevel;
	double m_preferredpenalty;
	double m_preferredclimb;
	double m_preferreddescent;
	excluderegions_t m_excluderegions;
	std::string m_callsign;
	validationresponse_t m_validationresponse;
	Glib::RefPtr<Gio::Socket> m_childsock;
#ifdef G_OS_UNIX
	Glib::RefPtr<Gio::UnixSocketAddress> m_childsockaddr;
#else
	Glib::RefPtr<Gio::InetSocketAddress> m_childsockaddr;
#endif
	std::string m_childstdout;
	std::string m_childbinary;
	sigc::connection m_connchildtimeout;
	sigc::connection m_connchildwatch;
	sigc::connection m_connchildstdout;
	Glib::RefPtr<Glib::IOChannel> m_childchanstdin;
	Glib::RefPtr<Glib::IOChannel> m_childchanstdout;
	Glib::Pid m_childpid;
	unsigned int m_childtimeoutcount;
	int m_childxdisplay;
	unsigned int m_iterationcount[2];
	int m_pathprobe;
	opttarget_t m_opttarget;
	validator_t m_validator;
	Glib::TimeVal m_tvelapsed;
	Glib::TimeVal m_tvvalidator;
	Glib::TimeVal m_tvvalidatestart;
	unsigned int m_maxlocaliteration;
	unsigned int m_maxremoteiteration;
	bool m_childrun;
	bool m_forceenrifr;
	bool m_tfravailable;
	bool m_tfrenable;
	bool m_tfrloaded;
	bool m_honourawylevels;
	bool m_honourprofilerules;
	bool m_honourlevelchangetrackmiles;
	bool m_honouropsperftrackmiles;
	bool m_precompgraphenable;
	bool m_windenable;
	bool m_grib2loaded;
	bool m_opsperfloaded;
	bool m_running;
	bool m_done;

	static const bool lgraphrestartedgescan = false;

	template <typename T> static bool is_nan_or_inf(T x) { return std::isnan(x) || std::isinf(x); }
	void opendb(void);
	void closedb(void);
	std::string get_tfr_xml_filename(void) const { return Glib::build_filename(get_db_auxdir(), "eRAD.xml"); }
	std::string get_tfr_bin_filename(void) const { return Glib::build_filename(get_db_auxdir(), "eRAD.bin"); }
	std::string get_tfr_local_xml_filename(void) const { return Glib::build_filename(get_db_auxdir(), "eRAD_local.xml"); }
	std::string get_tfr_local_bin_filename(void) const { return Glib::build_filename(get_db_auxdir(), "eRAD_local.bin"); }
	std::string get_tfr_user_xml_filename(void) const { return Glib::build_filename(FPlan::get_userdbpath(), "eRAD.xml"); }
	std::string get_tfr_user_bin_filename(void) const { return Glib::build_filename(FPlan::get_userdbpath(), "eRAD.bin"); }
	std::string get_tfr_global_rules_filename(void) const { return Glib::build_filename(get_db_auxdir(), "eRAD_rules.xml"); }
	std::string get_tfr_local_rules_filename(void) const { return Glib::build_filename(FPlan::get_userdbpath(), "eRAD_rules.xml"); }
	void check_tfr_available(void);
	bool find_airport(AirportsDb::Airport& el, const std::string& icao, const std::string& name);
	bool find_point(Point& pt, std::string& ident, Engine::AirwayGraphResult::Vertex::type_t& typ, const std::string& name, bool ifr, const Point& ptnear, bool exact);
	bool is_optfuel(void) const { return m_opttarget == opttarget_fuel; }
	bool is_optpreferred(void) const { return m_opttarget == opttarget_preferred; }
	void updatefpl(void);
	Rect get_bbox(void) const;
	void logwxlayers(const GRIB2::layerlist_t& ll, int alt = -1);
	void computeperformance(void);
	unsigned int findperfindex(int alt) const { return m_performance.findcruiseindex(alt); }
	void start(bool cont);
	bool start_vfr(void);
	virtual bool start_ifr(bool cont = false, bool optfuel = false) = 0;
	bool start_ifr_specialcase(void);
	bool start_ifr_specialcase_pogo(void);
	void preload_wind(void);
	void preload_opsperf(void);
	bool is_stopped(void);
	void child_watch(GPid pid, int child_status);
	void child_run(void);
	void child_close(void);
	bool child_stdout_handler(Glib::IOCondition iocond);
	bool child_socket_handler(Glib::IOCondition iocond);
	bool child_timeout(void);
	void child_send(const std::string& tx);
	bool child_is_running(void) const;
	void child_configure(void);
	unsigned int child_get_timeout(void) const { return m_childrun ? 120 : 30; }
	template <typename T> static bool is_metric_invalid(T m) { return std::isnan(m) || std::isinf(m) || m == std::numeric_limits<T>::max(); }

	virtual void parse_response(void) = 0;
};

inline CFMUAutoroute::statusmask_t& operator|=(CFMUAutoroute::statusmask_t& a, CFMUAutoroute::statusmask_t b) { return a = (CFMUAutoroute::statusmask_t)(a | b); }
inline CFMUAutoroute::statusmask_t& operator&=(CFMUAutoroute::statusmask_t& a, CFMUAutoroute::statusmask_t b) { return a = (CFMUAutoroute::statusmask_t)(a & b); }
inline CFMUAutoroute::statusmask_t& operator^=(CFMUAutoroute::statusmask_t& a, CFMUAutoroute::statusmask_t b) { return a = (CFMUAutoroute::statusmask_t)(a ^ b); }
inline CFMUAutoroute::statusmask_t operator|(CFMUAutoroute::statusmask_t a, CFMUAutoroute::statusmask_t b) { a |= b; return a; }
inline CFMUAutoroute::statusmask_t operator&(CFMUAutoroute::statusmask_t a, CFMUAutoroute::statusmask_t b) { a &= b; return a; }
inline CFMUAutoroute::statusmask_t operator^(CFMUAutoroute::statusmask_t a, CFMUAutoroute::statusmask_t b) { a ^= b; return a; }

extern const std::string& to_str(CFMUAutoroute::opttarget_t t);
extern const std::string& to_str(CFMUAutoroute::log_t l);

#endif /* CFMUAUTOROUTE_H */
