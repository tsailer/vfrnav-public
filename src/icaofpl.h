//
// C++ Interface: icaofpl
//
// Description:
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2012, 2013, 2014, 2017
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef ICAOFPL_H
#define ICAOFPL_H

#include <string>
#include <map>
#include "sysdeps.h"
#include "engine.h"
#include "fplan.h"
#include "aircraft.h"

// Some Notes:
// A leg is considered IFR if its starting point is IFR
// LSZK BARIG/N0123F100 IFR will therefore set BARIG to flight level 100 and set the IFR flag

class IcaoFlightPlan {
  public:
	typedef enum {
		awymode_keep,
		awymode_collapse,
		awymode_collapse_dct,
		awymode_collapse_all
	} awymode_t;

	IcaoFlightPlan(Engine& engine);
	typedef std::vector<std::string> errors_t;
	typedef std::pair<std::string::const_iterator,errors_t> parseresult_t;
	parseresult_t parse(std::string::const_iterator txti, std::string::const_iterator txte, bool expand_airways = true);
	errors_t parse(const std::string& txt, bool expand_airways = true) { return parse(txt.begin(), txt.end(), expand_airways).second; }
	parseresult_t parse_route(std::vector<FPlanWaypoint>& wpts, std::string::const_iterator txti, std::string::const_iterator txte, bool expand_airways = true);
	errors_t parse_route(std::vector<FPlanWaypoint>& wpts, const std::string& txt, bool expand_airways = true) { return parse_route(wpts, txt.begin(), txt.end(), expand_airways).second; }
	parseresult_t parse_garminpilot(std::string::const_iterator txti, std::string::const_iterator txte, bool expand_airways = true);
	errors_t parse_garminpilot(const std::string& txt, bool expand_airways = true) { return parse_garminpilot(txt.begin(), txt.end(), expand_airways).second; }
	void populate(const FPlanRoute& route, awymode_t awymode = awymode_collapse, double dct_limit = 50.0);
	std::string get_fpl(bool line_breaks = false);
	std::string get_item15(void);
	void set_route(FPlanRoute& route);
	void recompute_eet(void);

	const std::string& get_aircraftid(void) const { return m_aircraftid; }
	char get_flightrules(void) const;
	char get_flighttype(void) const { return m_flighttype; }
	unsigned int get_number(void) const { return m_number; }
	const std::string& get_aircrafttype(void) const { return m_aircrafttype; }
	char get_wakecategory(void) const { return m_wakecategory; }
	Aircraft::nav_t get_nav(void) const { return m_nav; }
	Aircraft::com_t get_com(void) const { return m_com; }
	std::string get_equipment_string(void) const { return Aircraft::get_equipment_string(m_nav, m_com); }
	Aircraft::transponder_t get_transponder(void) const { return m_transponder; }
	std::string get_transponder_string(void) const { return Aircraft::get_transponder_string(m_transponder); }
	Aircraft::pbn_t get_pbn(void) const { return m_pbn; }
	std::string get_pbn_string(void) const { return Aircraft::get_pbn_string(m_pbn); }
	const std::string& get_cruisespeed(void) const { return m_cruisespeed; }
	const std::string& get_departure(void) const { return m_departure; }
	const std::string& get_destination(void) const { return m_destination; }
	const std::string& get_alternate1(void) const { return m_alternate1; }
	const std::string& get_alternate2(void) const { return m_alternate2; }
	const Point& get_departurecoord(void) const { return m_departurecoord; }
	const Point& get_destinationcoord(void) const { return m_destinationcoord; }
	time_t get_departuretime(void) const { return m_departuretime; }
	time_t get_totaleet(void) const { return m_totaleet; }
	time_t get_endurance(void) const { return m_endurance; }
	unsigned int get_personsonboard(void) const { return m_personsonboard; }
	const std::string& get_colourmarkings(void) const { return m_colourmarkings; }
	const std::string& get_remarks(void) const { return m_remarks; }
	const std::string& get_picname(void) const { return m_picname; }
	Aircraft::emergency_t get_emergency(void) const { return m_emergency; }
	Aircraft::emergency_t get_emergencyradio(void) const { return m_emergency & Aircraft::emergency_radio_all; }
	std::string get_emergencyradio_string(void) const { return Aircraft::get_emergencyradio_string(m_emergency); }
	Aircraft::emergency_t get_survival(void) const { return m_emergency & Aircraft::emergency_survival_all; }
	std::string get_survival_options(void) const { return Aircraft::get_survival_options(m_emergency); }
	std::string get_survival_string(void) const { return Aircraft::get_survival_string(m_emergency); }
	Aircraft::emergency_t get_lifejackets(void) const { return m_emergency & Aircraft::emergency_jackets_all; }
	std::string get_lifejackets_options(void) const { return Aircraft::get_lifejackets_options(m_emergency); }
	std::string get_lifejackets_string(void) const { return Aircraft::get_lifejackets_string(m_emergency); }
	Aircraft::emergency_t get_dinghies(void) const { return m_emergency & Aircraft::emergency_dinghies_all; }
	uint16_t get_dinghiesnumber(void) const { return m_dinghiesnumber; }
	uint16_t get_dinghiescapacity(void) const { return m_dinghiescapacity; }
	const std::string& get_dinghiescolor(void) const { return m_dinghiescolor; }
	std::string get_dinghies_string(void) const { return Aircraft::get_dinghies_string(m_emergency, m_dinghiesnumber, m_dinghiescapacity, m_dinghiescolor); }

	void set_aircraft(const Aircraft& acft, const Aircraft::Cruise::EngineParams& ep = Aircraft::Cruise::EngineParams());

	void set_aircraftid(const std::string& x) { m_aircraftid = x; }
	void set_flighttype(char x) { m_flighttype = x; }
	void set_number(unsigned int x) { m_number = x; }
	void set_aircrafttype(const std::string& x) { m_aircrafttype = x; }
	void set_wakecategory(char x) { m_wakecategory = x; }
	void set_nav(Aircraft::nav_t n) { m_nav = n; }
	void set_com(Aircraft::com_t c) { m_com = c; }
	bool set_equipment(const std::string& x) { return Aircraft::parse_navcom(m_nav, m_com, x); }
	void set_transponder(Aircraft::transponder_t t) { m_transponder = t; }
	bool set_transponder(const std::string& x) { return Aircraft::parse_transponder(m_transponder, x); }
	void set_pbn(Aircraft::pbn_t x) { m_pbn = x; }
	bool set_pbn(const std::string& x) { return Aircraft::parse_pbn(m_pbn, x); }
	void set_cruisespeed(const std::string& x) { m_cruisespeed = x; }
	void set_departure(const std::string& x) { m_departure = x; }
	void set_destination(const std::string& x) { m_destination = x; }
	void set_alternate1(const std::string& x) { m_alternate1 = x; }
	void set_alternate2(const std::string& x) { m_alternate2 = x; }
	void set_departuretime(time_t x);
	void set_totaleet(time_t x) { m_totaleet = x; }
	void set_endurance(time_t x) { m_endurance = x; }
	void set_personsonboard(unsigned int x) { m_personsonboard = x; }
	void set_personsonboard_tbn(void) { m_personsonboard = ~0U; }
	void set_colourmarkings(const std::string& x) { m_colourmarkings = x; }
	void set_remarks(const std::string& x) { m_remarks = x; }
	void set_picname(const std::string& x) { m_picname = x; }
	void set_emergencyradio(Aircraft::emergency_t x) { m_emergency ^= (m_emergency ^ x) & Aircraft::emergency_radio_all; }
	bool set_emergencyradio(const std::string& x) { return Aircraft::parse_emergencyradio(m_emergency, x); }
	void set_survival(Aircraft::emergency_t x) { m_emergency ^= (m_emergency ^ x) & Aircraft::emergency_survival_all; }
	bool set_survival(const std::string& x) { return Aircraft::parse_survival(m_emergency, x); }
	void set_lifejackets(Aircraft::emergency_t x) { m_emergency ^= (m_emergency ^ x) & Aircraft::emergency_jackets_all; }
	bool set_lifejackets(const std::string& x) { return Aircraft::parse_lifejackets(m_emergency, x); }
	void set_dinghies(Aircraft::emergency_t x) { m_emergency ^= (m_emergency ^ x) & Aircraft::emergency_dinghies_all; }
	void set_dinghiesnumber(uint16_t x) { m_dinghiesnumber = x; }
	void set_dinghiescapacity(uint16_t x) { m_dinghiescapacity = x; }
	void set_dinghiescolor(const std::string& x) { m_dinghiescolor = x; }
	bool set_dinghies(const std::string& x) { return Aircraft::parse_dinghies(m_emergency, m_dinghiesnumber, m_dinghiescapacity, m_dinghiescolor, x); }

	void otherinfo_clear(void) { m_otherinfo.clear(); }
	void otherinfo_clear(const std::string& category);
	const std::string& otherinfo_get(const std::string& category);
	void otherinfo_add(const Aircraft::OtherInfo& oi) { otherinfo_add(oi.get_category(), oi.get_text()); }
	void otherinfo_add(const std::string& category, const std::string& text) { otherinfo_add(category, text.begin(), text.end()); }
	void otherinfo_add(const std::string& category, std::string::const_iterator texti, std::string::const_iterator texte);
	static std::vector<std::string> tokenize(const std::string& s);
	static std::string untokenize(const std::vector<std::string>& s);

	bool replace_profile(const FPlanRoute::Profile& p = FPlanRoute::Profile());

	void pbn_fix_equipment(void) { Aircraft::pbn_fix_equipment(m_nav, m_pbn); }
	void equipment_canonicalize(void);

	bool mark_vfrroute_begin(FPlanRoute& route);
	bool mark_vfrroute_end(FPlanRoute& route);

	static bool is_aiport_paris_tma(const std::string& icao); // Paris group airport
	static bool is_aiport_lfpnv(const std::string& icao);
	static bool is_aiport_lfob(const std::string& icao);
	static bool is_route_pogo(const std::string& dep, const std::string& dest); // POGO airport group
	bool is_route_pogo(void) const { return is_route_pogo(get_departure(), get_destination()); }
	static int32_t get_route_pogo_alt(const std::string& dep, const std::string& dest);
	int32_t get_route_pogo_alt(void) const { return get_route_pogo_alt(get_departure(), get_destination()); }

	class FindCoord {
	  public:
		static const unsigned int flag_airport    = Engine::AirwayGraphResult::Vertex::typemask_airport;
		static const unsigned int flag_navaid     = Engine::AirwayGraphResult::Vertex::typemask_navaid;
		static const unsigned int flag_waypoint   = Engine::AirwayGraphResult::Vertex::typemask_intersection;
		static const unsigned int flag_mapelement = Engine::AirwayGraphResult::Vertex::typemask_mapelement;
		static const unsigned int flag_airway     = 1 << 8;
		static const unsigned int flag_coord      = 1 << 9;
		static const unsigned int flag_subtables  = 1 << 31;

		FindCoord(Engine& engine);
		void find_by_name(const std::string& icao, const std::string& name, unsigned int flags);
		void find_by_ident(const std::string& ident, const std::string& name, unsigned int flags);
		void find_by_coord(const Point& pt, unsigned int flags, float maxdist_km);
		bool find(const std::string& icao, const std::string& name, unsigned int flags, const Point& pt = Point());
		bool find(const Point& pt, unsigned int flags, float maxdist_km);
		bool find_airway(const std::string& awy);
		bool find_area(const Rect& rect, Engine::AirwayGraphResult::Vertex::typemask_t tmask = Engine::AirwayGraphResult::Vertex::typemask_all);
		std::string get_name(void) const;
		std::string get_icao(void) const;
		Point get_coord(void) const;
		bool get_airport(AirportsDb::element_t& x) const;
		bool get_navaid(NavaidsDb::element_t& x) const;
		bool get_waypoint(WaypointsDb::element_t& x) const;
		bool get_mapelement(MapelementsDb::element_t& x) const;
		bool get(AirportsDb::element_t& x) const { return get_airport(x); }
		bool get(NavaidsDb::element_t& x) const { return get_navaid(x); }
		bool get(WaypointsDb::element_t& x) const { return get_waypoint(x); }
		bool get(MapelementsDb::element_t& x) const { return get_mapelement(x); }
		Engine::DbObject::ptr_t get_object(void) const;
		bool get_coord(Point& x) const;
		bool get_airwaygraph(Engine::AirwayGraphResult::Graph& x) const;
		const Engine::AirportResult::elementvector_t& get_airports(void) const { return m_airports; }
		const Engine::NavaidResult::elementvector_t& get_navaids(void) const { return m_navaids; }
		const Engine::WaypointResult::elementvector_t& get_waypoints(void) const { return m_waypoints; }
		const Engine::MapelementResult::elementvector_t& get_mapelements(void) const { return m_mapelements; }
		const Engine::AirwayResult::elementvector_t& get_airways(void) const { return m_airways; }

	  protected:
		class SortAirportNamelen;
		class SortNavaidNamelen;
		class SortMapelementNamelen;
		class SortAirportDist;
		class SortNavaidDist;
		class SortWaypointDist;
		class SortMapelementDist;
		class SortAirwayDist;

		static const std::string empty;

		Engine::AirportResult::elementvector_t m_airports;
		Engine::NavaidResult::elementvector_t m_navaids;
		Engine::WaypointResult::elementvector_t m_waypoints;
		Engine::MapelementResult::elementvector_t m_mapelements;
		Engine::AirwayResult::elementvector_t m_airways;
		Engine::AirwayGraphResult::Graph m_airwaygraph;
		Glib::RefPtr<Engine::AirportResult> m_airportquery;
		Glib::RefPtr<Engine::AirportResult> m_airportquery1;
		Glib::RefPtr<Engine::NavaidResult> m_navaidquery;
		Glib::RefPtr<Engine::NavaidResult> m_navaidquery1;
		Glib::RefPtr<Engine::WaypointResult> m_waypointquery;
		Glib::RefPtr<Engine::MapelementResult> m_mapelementquery;
		Glib::RefPtr<Engine::AirwayResult> m_airwayelquery;
		Glib::RefPtr<Engine::AirwayGraphResult> m_airwayquery;
		Glib::RefPtr<Engine::AreaGraphResult> m_areaquery;
		Glib::Cond m_cond;
		Glib::Mutex m_mutex;
		Engine& m_engine;
		Point m_coordresult;

		void async_cancel(void);
		void async_clear(void);
		void async_connectdone(void);
		void async_done(void);
		unsigned int async_waitdone(void);
		void sort_dist(const Point& pt);
		void retain_first(void);
		void cut_dist(const Point& pt, float maxdist_km);
		void retain_shortest_dist(const Point& pt);
	};

  protected:
	class Waypoint {
	  public:
		typedef FPlanWaypoint::pathcode_t pathcode_t;
                typedef FPlanWaypoint::type_t type_t;
                typedef FPlanWaypoint::navaid_type_t navaid_type_t;
		typedef Engine::DbObject::ptr_t dbobject_ptr_t;

                Waypoint(void);
                Waypoint(const FPlanWaypoint& wpt);

		const std::string& get_icao(void) const { return m_icao; }
                void set_icao(const std::string& t) { m_icao = t; }
                const std::string& get_name(void) const { return m_name; }
                void set_name(const std::string& t) { m_name = t; }
		pathcode_t get_pathcode(void) const { return m_pathcode; }
		void set_pathcode(FPlanWaypoint::pathcode_t pc) { m_pathcode = pc; }
                const std::string& get_pathname(void) const { return m_pathname; }
                void set_pathname(const std::string& t) { m_pathname = t; }
		bool is_stay(unsigned int& nr, unsigned int& tm) const {
			return get_pathcode() == FPlanWaypoint::pathcode_airway &&
				FPlanWaypoint::is_stay(get_pathname(), nr, tm);
		}
		bool is_stay(void) const {
			unsigned int nr, tm;
			return is_stay(nr, tm);
		}
		const std::string& get_icao_or_name(void) const;
                time_t get_time(void) const { return m_time; }
                void set_time(time_t t) { m_time = t; }
                const Point& get_coord(void) const { return m_coord; }
                Point::coord_t get_lon(void) const { return m_coord.get_lon(); }
                Point::coord_t get_lat(void) const { return m_coord.get_lat(); }
                void set_coord(const Point& c) { m_coord = c; }
                void set_lon(Point::coord_t lo) { m_coord.set_lon(lo); }
                void set_lat(Point::coord_t la) { m_coord.set_lat(la); }
                void set_lon_deg(float lo) { m_coord.set_lon_deg(lo); }
                void set_lat_deg(float la) { m_coord.set_lat_deg(la); }
		std::string get_coordstr(void) const;
                int32_t get_altitude(void) const { return m_alt; }
                void set_altitude(int32_t a) { m_alt = a; }
                uint16_t get_flags(void) const { return m_flags; }
                void set_flags(uint16_t f) { m_flags = f; }
		void frob_flags(uint16_t f, uint16_t m) { m_flags = (m_flags & ~m) ^ f; }
		bool is_standard(void) const { return !!(get_flags() & FPlanWaypoint::altflag_standard); }
                bool is_ifr(void) const { return !!(get_flags() & FPlanWaypoint::altflag_ifr); }
		uint32_t get_frequency(void) const { return m_frequency; }
                void set_frequency(uint32_t f) { m_frequency = f; }
                type_t get_type(void) const { return m_type; }
                navaid_type_t get_navaid(void) const { return m_navaid; }
                void set_type(type_t typ = FPlanWaypoint::type_undefined,
			      navaid_type_t navaid = FPlanWaypoint::navaid_invalid) { m_type = typ; m_navaid = navaid; }
		dbobject_ptr_t get_dbobj(void) const { return m_dbobj; }
		void set_dbobj(const dbobject_ptr_t& p = dbobject_ptr_t()) { m_dbobj = p; }

		bool enforce_pathcode_vfrifr(void);

		FPlanWaypoint get_fplwpt(void) const;
		std::string to_str(void) const;

        private:
		dbobject_ptr_t m_dbobj;
                std::string m_icao;
                std::string m_name;
                std::string m_pathname;
                time_t m_time;
                Point m_coord;
                int32_t m_alt;
		uint32_t m_frequency;
                uint16_t m_flags;
		pathcode_t m_pathcode;
                type_t m_type;
                navaid_type_t m_navaid;
	};

	class ParseWaypoint : public Waypoint {
	  public:
		typedef Engine::AirwayGraphResult::Graph Graph;
		typedef Engine::AirwayGraphResult::Vertex Vertex;
		typedef Engine::AirwayGraphResult::Edge Edge;

		class Path {
		public:
			Path(Graph::vertex_descriptor u = ~0U, double dist = std::numeric_limits<double>::quiet_NaN())
				: m_u(u), m_dist(dist) {}
			Graph::vertex_descriptor get_vertex(void) const { return m_u; }
			double get_dist(void) const { return m_dist; }
			void set_dist(double x) { m_dist = x; }
			void clear(void) { m_prev.clear(); }
			unsigned int size(void) const { return m_prev.size(); }
			bool empty(void) const { return m_prev.empty(); }
			void add(Graph::vertex_descriptor u) { m_prev.push_back(u); }
			Graph::vertex_descriptor operator[](unsigned int x) const { return m_prev[x]; }

		protected:
			typedef std::vector<Graph::vertex_descriptor> prev_t;
			prev_t m_prev;
			Graph::vertex_descriptor m_u;
			double m_dist;
		};

		ParseWaypoint(void);
                ParseWaypoint(const FPlanWaypoint& wpt);
		int16_t get_course(void) const { return m_course; }
		int16_t get_dist(void) const { return m_dist; }
		void set_coursedist(int16_t crs = -1, int16_t dist = -1) { m_course = crs; m_dist = dist; }
		bool is_coursedist(void) const { return get_course() >= 0 && get_dist() > 0; }
		Vertex::typemask_t get_typemask(void) const { return m_typemask; }
		void set_typemask(Vertex::typemask_t tm = Vertex::typemask_navaid | Vertex::typemask_intersection | Vertex::typemask_mapelement) { m_typemask = tm; }
		void set_expanded(bool exp = false) { m_expanded = exp; }
		bool is_expanded(void) const { return m_expanded; }

		void set(const Engine::DbObject::ptr_t obj, bool forcearptelev = false);

		void upcaseid(void);
		float process_speedalt(void);
		void process_crsdist(void);
		static Point parse_coord(const std::string& txt);
		bool process_coord(void);
		bool process_airportcoord(void);

		void add(const Path& path) { m_path.push_back(path); }
		void clear(void) { m_path.clear(); }
		Path& operator[](unsigned int idx) { return m_path[idx]; }
		const Path& operator[](unsigned int idx) const { return m_path[idx]; }
		unsigned int size(void) const { return m_path.size(); }
		bool empty(void) const { return m_path.empty(); }

		std::string to_str(void) const;

	  protected:
		typedef std::vector<Path> path_t;
		path_t m_path;
		int16_t m_course;
		int16_t m_dist;
		Vertex::typemask_t m_typemask;
		bool m_expanded;
	};

	class ParseState {
	  public:
		typedef Engine::AirwayGraphResult::Graph Graph;
		typedef Engine::AirwayGraphResult::Vertex Vertex;
		typedef Engine::AirwayGraphResult::Edge Edge;

		class AwyEdgeFilter {
		public:
			AwyEdgeFilter(void) : m_graph(0) {}
			AwyEdgeFilter(Graph& g, const Glib::ustring& awyname) : m_graph(&g), m_awyname(awyname.uppercase()) {}
			template <typename E> bool operator()(const E& e) const { const Glib::ustring& id((*m_graph)[e].get_ident()); return id == m_awyname || id == "-"; }

		protected:
			Graph *m_graph;
			Glib::ustring m_awyname;
		};

		ParseState(Engine& engine);
		void process_speedalt(void);
		void process_dblookup(void);
		void process_airways(bool expand);
		void process_time(const std::string& eets = "");

		typedef std::vector<ParseWaypoint> wpts_t;
		const wpts_t& get_wpts(void) const { return m_wpts; }
		bool empty(void) const { return m_wpts.empty(); }
		wpts_t::size_type size(void) const { return m_wpts.size(); }
		const ParseWaypoint& operator[](wpts_t::size_type i) const { return m_wpts[i]; }
		ParseWaypoint& operator[](wpts_t::size_type i) { return m_wpts[i]; }
		const ParseWaypoint& front(void) const { return m_wpts.front(); }
		ParseWaypoint& front(void) { return m_wpts.front(); }
		const ParseWaypoint& back(void) const { return m_wpts.back(); }
		ParseWaypoint& back(void) { return m_wpts.back(); }
		void add(const ParseWaypoint& w) { m_wpts.push_back(w); }

		typedef std::vector<std::string> errors_t;
		const errors_t& get_errors(void) const { return m_errors; }

		typedef std::map<int,float> cruisespeeds_t;
		const cruisespeeds_t& get_cruisespeeds(void) const { return m_cruisespeeds; }
		void add_cruisespeed(int alt, float spd) { m_cruisespeeds[alt] = spd; }

		std::ostream& print(std::ostream& os) const;

	  protected:
		static const bool trace_dbload = false;
		Engine& m_engine;
		wpts_t m_wpts;
		typedef std::set<std::string> airways_t;
		airways_t m_airways;
		Graph m_graph;
		errors_t m_errors;
		cruisespeeds_t m_cruisespeeds;

		void clear_tempedges(void);
		void load_undef(void);
		void load_undef(Graph::vertex_descriptor u);
	};

	static const bool trace_parsestate = true;
	Engine& m_engine;
	typedef std::vector<Waypoint> route_t;
	route_t m_route;
	typedef std::map<std::string,std::string> otherinfo_t;
	otherinfo_t m_otherinfo;
	std::string m_aircraftid;
	std::string m_aircrafttype;
	std::string m_cruisespeed;
	std::string m_departure;
	std::string m_destination;
	std::string m_alternate1;
	std::string m_alternate2;
	std::string m_sid;
	std::string m_star;
	std::string m_dinghiescolor;
	std::string m_colourmarkings;
	std::string m_remarks;
	std::string m_picname;
	typedef std::map<int,float> cruisespeeds_t;
	cruisespeeds_t m_cruisespeeds;
	Point m_departurecoord;
	Point m_destinationcoord;
	time_t m_departuretime;
	time_t m_totaleet;
	time_t m_endurance;
	Aircraft::nav_t m_nav;
	Aircraft::com_t m_com;
	Aircraft::transponder_t m_transponder;
	Aircraft::emergency_t m_emergency;
	Aircraft::pbn_t m_pbn;
	unsigned int m_number;
	unsigned int m_personsonboard;
	uint16_t m_departureflags;
	uint16_t m_destinationflags;
	uint16_t m_dinghiesnumber;
	uint16_t m_dinghiescapacity;
	unsigned int m_defaultalt;
	char m_flighttype;
	char m_wakecategory;

	Rect get_bbox(void) const;
	typedef std::set<std::string> nameset_t;
	static nameset_t intersect(const nameset_t& s1, const nameset_t& s2);
	float get_cruisespeed(int& alt) const;
	void clear(void);
	bool has_pathcodes(void) const;
	bool enforce_pathcode_vfrdeparr(void);
	bool enforce_pathcode_vfrifr(void);
	void erase_vfrroute_begin(void);
	void erase_vfrroute_end(void);
	void redo_route_names(void);
	void find_airways(void);
	void fix_max_dct_distance(double dct_limit = 50.0);
	void erase_unnecessary_airway(bool keep_turnpoints = true, bool include_dct = false);
	void add_eet(void);
	void normalize_pogo(void);
	static void trimleft(std::string::const_iterator& txti, std::string::const_iterator txte);
	static void trimright(std::string::const_iterator txti, std::string::const_iterator& txte);
	static void trim(std::string::const_iterator& txti, std::string::const_iterator& txte);
	static std::string upcase(const std::string& txt);
	static bool parsetxt(std::string& txt, unsigned int len, std::string::const_iterator& txti, std::string::const_iterator txte,
			     bool slashsep = true);
	static bool parsenum(unsigned int& num, unsigned int digits, std::string::const_iterator& txti, std::string::const_iterator txte);
	static bool parsetime(time_t& t, std::string::const_iterator& txti, std::string::const_iterator txte);
	static bool parsespeed(std::string& spdstr, float& spd, std::string::const_iterator& txti, std::string::const_iterator txte);
	static bool parsealt(int& alt, unsigned int& flags, std::string::const_iterator& txti, std::string::const_iterator txte);
};

#endif /* ICAOFPL_H */
