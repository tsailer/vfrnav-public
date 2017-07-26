#ifndef ADRFPLAN_H
#define ADRFPLAN_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <limits>
#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>

#include "adr.hh"
#include "adrdb.hh"
#include "adrgraph.hh"
#include "adraerodrome.hh"
#include "adrpoint.hh"
#include "adrroute.hh"
#include "adrairspace.hh"

#include "aircraft.h"

namespace ADR {

// Some Notes:
// A leg is considered IFR if its starting point is IFR
// LSZK BARIG/N0123F100 IFR will therefore set BARIG to flight level 100 and set the IFR flag

class FPLWaypoint : public FPlanWaypoint {
public:
	FPLWaypoint(void);
	FPLWaypoint(const FPlanWaypoint& w);

	void set_expanded(bool exp = false) { if (exp) m_flags |= (1 << 0); else m_flags &= ~(1 << 0);  }
	bool is_expanded(void) const { return !!(m_flags & (1 << 0)); }

	void set_disabled(bool dis = false) { if (dis) m_flags |= (1 << 1); else m_flags &= ~(1 << 1);  }
	bool is_disabled(void) const { return !!(m_flags & (1 << 1)); }

	void set_wptnr(unsigned int w) { m_wptnr = w; }
	unsigned int get_wptnr(void) const { return m_wptnr; }

	const Object::const_ptr_t& get_ptobj(void) const { return m_ptobj; }
	void set_ptobj(const Object::const_ptr_t& p) { m_ptobj = p; }
	const Object::const_ptr_t& get_pathobj(void) const { return m_pathobj; }
	void set_pathobj(const Object::const_ptr_t& p) { m_pathobj = p; }
	const UUID& get_ptuuid(void) const;
	const UUID& get_pathuuid(void) const;

	bool is_path_match(const Object::const_ptr_t& p) const;
	bool is_path_match(const UUID& uuid) const;

	void set_from_objects(bool term = false);

	const Glib::ustring& get_icao_or_name(void) const;
	std::string get_coordstr(void) const;
	bool enforce_pathcode_vfrifr(void);
	std::string to_str(void) const;

protected:
	Object::const_ptr_t m_ptobj;
	Object::const_ptr_t m_pathobj;
	unsigned int m_wptnr;
	uint8_t m_flags;
};

class FlightPlan : public std::vector<FPLWaypoint> {
protected:
	typedef std::vector<FPLWaypoint> base_t;

public:
	typedef enum {
		awymode_keep,
		awymode_collapse,
		awymode_collapse_dct,
		awymode_collapse_all
	} awymode_t;

	FlightPlan(void);
	typedef std::vector<std::string> errors_t;
	typedef std::pair<std::string::const_iterator,errors_t> parseresult_t;
	parseresult_t parse(Database& db, std::string::const_iterator txti, std::string::const_iterator txte, bool expand_airways = true);
	errors_t parse(Database& db, const std::string& txt, bool expand_airways = true) { return parse(db, txt.begin(), txt.end(), expand_airways).second; }
	parseresult_t parse_route(Database& db, std::vector<FPlanWaypoint>& wpts, std::string::const_iterator txti, std::string::const_iterator txte, bool expand_airways = true);
	errors_t parse_route(Database& db, std::vector<FPlanWaypoint>& wpts, const std::string& txt, bool expand_airways = true) { return parse_route(db, wpts, txt.begin(), txt.end(), expand_airways).second; }
	errors_t reparse(Database& db, bool do_expand_airways = true);
	errors_t populate(Database& db, const FPlanRoute& route, bool expand_airways = true);
	errors_t expand_composite(const Graph& g);
	std::string get_fpl(void);
	std::string get_item15(void);
	void recompute_eet(void);

	const std::string& get_aircraftid(void) const { return m_aircraftid; }
	char get_flightrules(void) const;
	char get_flighttype(void) const { return m_flighttype; }
	unsigned int get_number(void) const { return m_number; }
	const std::string& get_aircrafttype(void) const { return m_aircrafttype; }
	char get_wakecategory(void) const { return m_wakecategory; }
	const std::string& get_equipment(void) const { return m_equipment; }
	const std::string& get_transponder(void) const { return m_transponder; }
	Aircraft::pbn_t get_pbn(void) const { return m_pbn; }
	std::string get_pbn_string(void) const { return Aircraft::get_pbn_string(m_pbn); }
	const std::string& get_cruisespeed(void) const { return m_cruisespeed; }
	const std::string& get_alternate1(void) const { return m_alternate1; }
	const std::string& get_alternate2(void) const { return m_alternate2; }
	time_t get_departuretime(void) const { return m_departuretime; }
	time_t get_totaleet(void) const { return m_totaleet; }
	time_t get_endurance(void) const { return m_endurance; }
	unsigned int get_personsonboard(void) const { return m_personsonboard; }
	const std::string& get_colourmarkings(void) const { return m_colourmarkings; }
	const std::string& get_remarks(void) const { return m_remarks; }
	const std::string& get_picname(void) const { return m_picname; }
	const std::string& get_emergencyradio(void) const { return m_emergencyradio; }
	const std::string& get_survival(void) const { return m_survival; }
	const std::string& get_lifejackets(void) const { return m_lifejackets; }
	const std::string& get_dinghies(void) const { return m_dinghies; }

	void set_aircraft(const Aircraft& acft,
			  double rpm = std::numeric_limits<double>::quiet_NaN(),
			  double mp = std::numeric_limits<double>::quiet_NaN(),
			  double bhp = std::numeric_limits<double>::quiet_NaN());

	void set_aircraftid(const std::string& x) { m_aircraftid = x; }
	void set_flighttype(char x) { m_flighttype = x; }
	void set_number(unsigned int x) { m_number = x; }
	void set_aircrafttype(const std::string& x) { m_aircrafttype = x; }
	void set_wakecategory(char x) { m_wakecategory = x; }
	void set_equipment(const std::string& x) { m_equipment = x; }
	void set_transponder(const std::string& x) { m_transponder = x; }
	void set_pbn(Aircraft::pbn_t x) { m_pbn = x; }
	void set_pbn(const std::string& x) { m_pbn = Aircraft::parse_pbn(x); }
	void set_cruisespeed(const std::string& x) { m_cruisespeed = x; }
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
	void set_emergencyradio(const std::string& x) { m_emergencyradio = x; }
	void set_survival(const std::string& x) { m_survival = x; }
	void set_lifejackets(const std::string& x) { m_lifejackets = x; }
	void set_dinghies(const std::string& x) { m_dinghies = x; }

	typedef std::set<Aircraft::OtherInfo> otherinfo_t;
	const otherinfo_t& get_otherinfo(void) const { return m_otherinfo; }
	void otherinfo_clear(void) { m_otherinfo.clear(); }
	void otherinfo_clear(const std::string& category);
	const Glib::ustring& otherinfo_get(const std::string& category);
	void otherinfo_add(const Aircraft::OtherInfo& oi) { otherinfo_add(oi.get_category(), oi.get_text()); }
	void otherinfo_add(const std::string& category, const std::string& text) { otherinfo_add(category, text.begin(), text.end()); }
	void otherinfo_add(const std::string& category, std::string::const_iterator texti, std::string::const_iterator texte);
	void otherinfo_replace(const std::string& category, const std::string& text) { otherinfo_replace(category, text.begin(), text.end()); }
	void otherinfo_replace(const std::string& category, std::string::const_iterator texti, std::string::const_iterator texte);
	static std::vector<std::string> tokenize(const std::string& s);
	static std::string untokenize(const std::vector<std::string>& s);
	void add_eet(const std::string& ident, time_t flttime);
	void remove_eet(const std::string& ident);

	void pbn_fix_equipment(void) { Aircraft::pbn_fix_equipment(m_equipment, m_pbn); }

	bool is_route_pogo(void) const;

	void disable_none(void);
	void disable_unnecessary(bool keep_turnpoints = true, bool include_dct = false, float maxdev = 0.5);
	void fix_max_dct_distance(double dct_limit = 50.0);
	void erase_unnecessary_airway(bool keep_turnpoints = true, bool include_dct = false);
	void add_eet(void);
	void clear(void);

	float total_distance_km(void) const;
	float total_distance_nmi(void) const { return total_distance_km() * Point::km_to_nmi; }
	double total_distance_km_dbl(void) const;
	double total_distance_nmi_dbl(void) const { return total_distance_km_dbl() * Point::km_to_nmi_dbl; }
	float gc_distance_km(void) const;
	float gc_distance_nmi(void) const { return gc_distance_km() * Point::km_to_nmi; }
	double gc_distance_km_dbl(void) const;
	double gc_distance_nmi_dbl(void) const { return gc_distance_km_dbl() * Point::km_to_nmi_dbl; }
	int32_t max_altitude(void) const;
	int32_t max_altitude(uint16_t& flags) const;
	Rect get_bbox(void) const;

	void recompute_dist(void);
	void recompute_decl(void);

	void set_winddir(uint16_t dir);
	void set_winddir_deg(float dir) { set_winddir(dir * FPlanWaypoint::from_deg); }
	void set_winddir_rad(float dir) { set_winddir(dir * FPlanWaypoint::from_rad); }
	void set_windspeed(uint16_t speed);
	void set_windspeed_kts(float speed) { set_windspeed(std::min(speed * 64.f, 65535.f)); }
	void set_qff(uint16_t press);
	void set_qff_hpa(float press) { set_qff(std::min(press * 32.f, 65535.f)); }
	void set_isaoffset(int16_t temp);
	void set_isaoffset_kelvin(float temp) { set_isaoffset(std::min(std::max(temp * 256.f, -32768.f), 32767.f)); }
	void set_declination(uint16_t decl);
	void set_declination_deg(float decl) { set_declination(decl * FPlanWaypoint::from_deg); }
	void set_declination_rad(float decl) { set_declination(decl * FPlanWaypoint::from_rad); }
	void set_rpm(uint16_t rpm);
	void set_mp(uint16_t mp);
	void set_mp_inhg(float mp) { set_mp(Point::round<uint16_t,float>(mp * 256.0)); }
	void set_tas(uint16_t tas);
	void set_tas_kts(uint16_t tas) { set_tas(Point::round<uint16_t,float>(tas * 16.0)); }

	void recompute(const Aircraft& acft, float qnh = std::numeric_limits<float>::quiet_NaN(), float tempoffs = std::numeric_limits<float>::quiet_NaN(),
		       double bhp = std::numeric_limits<double>::quiet_NaN(), double rpm = std::numeric_limits<double>::quiet_NaN(),
		       double mp = std::numeric_limits<double>::quiet_NaN());

	void turnpoints(bool include_dct = true, float maxdev = 0.5f);
	FPlanRoute::GFSResult gfs(GRIB2& grib2);
	void fix_invalid_altitudes(unsigned int nr);
	void fix_invalid_altitudes(unsigned int nr, TopoDb30& topodb, Database& db);
	void fix_invalid_altitudes(void);
	void fix_invalid_altitudes(TopoDb30& topodb, Database& db);
	void clamp_msl(void);
	void compute_terrain(TopoDb30& topodb, double corridor_nmi = 5., bool roundcaps = false);

	void add_fir_eet(Database& db, const TimeTableSpecialDateEval& ttsde);
	void remove_fir_eet(Database& db);

protected:
	class ParseWaypoint : public FPLWaypoint {
	public:
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
                ParseWaypoint(const FPLWaypoint& wpt);
		ParseWaypoint(const FPlanWaypoint& wpt);
		int16_t get_course(void) const { return m_course; }
		int16_t get_dist(void) const { return m_dist; }
		void set_coursedist(int16_t crs = -1, int16_t dist = -1) { m_course = crs; m_dist = dist; }
		bool is_coursedist(void) const { return get_course() >= 0 && get_dist() > 0; }

		void upcaseid(void);
		bool process_speedalt(uint16_t& flags, int32_t& alt, float& speed);
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
	};

	class ParseState {
	public:
		ParseState(Database& db);
		void process_speedalt(void);
		void process_dblookup(bool termarpt);
		void process_dblookupcoord(void);
		void process_airways(bool expand, bool termarpt);
		void process_time(const std::string& eets = "");

		uint64_t get_departuretime(void) const;
		Rect get_bbox(void) const;

		typedef std::vector<ParseWaypoint> wpts_t;
		const wpts_t& get_wpts(void) const { return m_wpts; }
		bool empty(void) const { return m_wpts.empty(); }
		wpts_t::size_type size(void) const { return m_wpts.size(); }
		const ParseWaypoint& operator[](wpts_t::size_type i) const { return m_wpts[i]; }
		ParseWaypoint& operator[](wpts_t::size_type i) { return m_wpts[i]; }
		void add(const ParseWaypoint& w) { m_wpts.push_back(w); }

		typedef std::vector<std::string> errors_t;
		const errors_t& get_errors(void) const { return m_errors; }

		typedef std::map<int,float> cruisespeeds_t;
		const cruisespeeds_t& get_cruisespeeds(void) const { return m_cruisespeeds; }
		void add_cruisespeed(int alt, float spd) { m_cruisespeeds[alt] = spd; }

		std::ostream& print(std::ostream& os) const;

	protected:
		static const bool trace_dbload = false;
		Database& m_db;
		wpts_t m_wpts;
		Graph m_graph;
		errors_t m_errors;
		cruisespeeds_t m_cruisespeeds;

		void graph_add(const Object::ptr_t& p);
		void graph_add(const Database::findresults_t& r);
	};

	class EETSort {
	public:
		bool operator()(const std::string& a, const std::string& b) const;
	};

	static const bool trace_parsestate = false;
	otherinfo_t m_otherinfo;
	std::string m_aircraftid;
	std::string m_aircrafttype;
	std::string m_equipment;
	std::string m_transponder;
	std::string m_cruisespeed;
	std::string m_alternate1;
	std::string m_alternate2;
	std::string m_colourmarkings;
	std::string m_remarks;
	std::string m_picname;
	std::string m_emergencyradio;
	std::string m_survival;
	std::string m_lifejackets;
	std::string m_dinghies;
	typedef std::map<int,float> cruisespeeds_t;
	cruisespeeds_t m_cruisespeeds;
	time_t m_departuretime;
	time_t m_totaleet;
	time_t m_endurance;
	Aircraft::pbn_t m_pbn;
	unsigned int m_number;
	unsigned int m_personsonboard;
	unsigned int m_defaultalt;
	char m_flighttype;
	char m_wakecategory;

	typedef std::set<std::string> nameset_t;
	static nameset_t intersect(const nameset_t& s1, const nameset_t& s2);
	float get_cruisespeed(int& alt) const;
	bool has_pathcodes(void) const;
	bool enforce_pathcode_vfrdeparr(void);
	bool enforce_pathcode_vfrifr(void);
	void normalize_pogo(void);
	void compute_cruisespeeds(double& avgtas, double& avgff, const Aircraft& acft,
				  const Aircraft::Cruise::EngineParams& ep = Aircraft::Cruise::EngineParams());
	static void trimleft(std::string::const_iterator& txti, std::string::const_iterator txte);
	static void trimright(std::string::const_iterator txti, std::string::const_iterator& txte);
	static void trim(std::string::const_iterator& txti, std::string::const_iterator& txte);
	static std::string upcase(const std::string& txt);
	static bool parsetxt(std::string& txt, unsigned int len, std::string::const_iterator& txti, std::string::const_iterator txte,
			     bool slashsep = true);
	static bool parsenum(unsigned int& num, unsigned int digits, std::string::const_iterator& txti, std::string::const_iterator txte);
	static bool parsetime(time_t& t, std::string::const_iterator& txti, std::string::const_iterator txte);
	static bool parsespeed(std::string& spdstr, float& spd, std::string::const_iterator& txti, std::string::const_iterator txte);
	static bool parsealt(int32_t& alt, unsigned int& flags, std::string::const_iterator& txti, std::string::const_iterator txte);
};

};

#endif /* ADRFPLAN_H */
