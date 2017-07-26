#ifndef TFR_H
#define TFR_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <limits>
#include <boost/config.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/logic/tribool_io.hpp>
#include <sqlite3x.hpp>
#include <libxml++/libxml++.h>

#include "sysdeps.h"
#include "aircraft.h"
#include "fplan.h"
#include "dbobj.h"
#include "engine.h"
#include "interval.hh"

class TrafficFlowRestrictions {
protected:
	typedef std::set<std::string> tracerules_t;
	typedef std::set<std::string> disabledrules_t;

	class BinLoader;
	friend class BinLoader;
	class TFRAirspace;

public:
        typedef Engine::AirwayGraphResult::Graph Graph;
        typedef Engine::AirwayGraphResult::Vertex Vertex;
        typedef Engine::AirwayGraphResult::Edge Edge;

	static const Graph::vertex_descriptor invalid_vertex_descriptor = ~0UL;

	class RuleWpt;
	class AltRange;
	class RuleWptAlt;
	class SegWptsAlt;

	class FplWpt : public FPlanWaypoint {
	public:
		typedef Vertex::type_t type_t;

		FplWpt(void) : m_vertex_descriptor(invalid_vertex_descriptor), m_wptnr(0), m_type(Vertex::type_undefined), m_navaid_type(NavaidsDb::Navaid::navaid_invalid) {}
		void set_vertex_descriptor(Graph::vertex_descriptor u = invalid_vertex_descriptor) { m_vertex_descriptor = u; }
		Graph::vertex_descriptor get_vertex_descriptor(void) const { return m_vertex_descriptor; }
		void set_wptnr(unsigned int w) { m_wptnr = w; }
		unsigned int get_wptnr(void) const { return m_wptnr; }
		bool has_ndb(void) const { return get_type() == type_navaid && NavaidsDb::Navaid::has_ndb(static_cast<DbBaseElements::Navaid::navaid_type_t>(get_navaid())); }
		bool has_vor(void) const { return get_type() == type_navaid && NavaidsDb::Navaid::has_vor(static_cast<DbBaseElements::Navaid::navaid_type_t>(get_navaid())); }
		bool has_dme(void) const { return get_type() == type_navaid && NavaidsDb::Navaid::has_dme(static_cast<DbBaseElements::Navaid::navaid_type_t>(get_navaid())); }
		const Glib::ustring& get_ident(void) const;
		bool operator==(const RuleWpt& r) const;
		bool within(const AltRange& ar) const;

	protected:
		Graph::vertex_descriptor m_vertex_descriptor;
		unsigned int m_wptnr;
		type_t m_type;
		NavaidsDb::Navaid::navaid_type_t m_navaid_type;
	};

	class Fpl : public std::vector<FplWpt> {
	public:
		Fpl(void) {}
		Fpl(const FPlanRoute& rte) { *this = rte; }
		Fpl& operator=(const FPlanRoute& rte);
		Rect get_bbox(void) const;
	};

	class Message {
	public:
		typedef std::set<unsigned int> set_t;
		typedef enum {
			type_error,
			type_warning,
			type_info,
			type_tracecond,
			type_tracetfe
		} type_t;

		Message(const std::string& text = "", type_t t = type_error, const std::string& rule = "") : m_text(text), m_rule(rule), m_type(t) {}
		const set_t& get_vertexset(void) const { return m_vertexset; }
		const set_t& get_edgeset(void) const { return m_edgeset; }
		const std::string& get_text(void) const { return m_text; }
		const std::string& get_rule(void) const { return m_rule; }
		type_t get_type(void) const { return m_type; }
		static const std::string& get_type_string(type_t t);
		const std::string& get_type_string(void) const { return get_type_string(get_type()); }
		static char get_type_char(type_t t);
		char get_type_char(void) const { return get_type_char(get_type()); }

	protected:
		set_t m_vertexset;
		set_t m_edgeset;
		std::string m_text;
		std::string m_rule;
		type_t m_type;

		friend class TrafficFlowRestrictions;
		void add_vertex(unsigned int nr);
		void add_edge(unsigned int nr);
		void insert_vertex(unsigned int nr);
	};

	class RuleResult {
	public:
		typedef std::set<unsigned int> set_t;

		class Alternative {
		public:

			class Sequence {
			public:
				typedef enum {
					type_airway,
					type_dct,
					type_point,
					type_sid,
					type_star,
					type_airspace,
					type_invalid
				} type_t;

				class RulePoint {
				public:
					RulePoint(const std::string& id = "", const Point& pt = Point(), Vertex::type_t typ = Vertex::type_undefined,
						  bool vor = false, bool ndb = false);
					RulePoint(const FplWpt& wpt, bool vor = false, bool ndb = false);
					RulePoint(const RuleWpt& wpt);
					RulePoint(const Vertex& vv);
					const std::string& get_ident(void) const { return m_ident; }
					const Point& get_coord(void) const { return m_coord; }
					Vertex::type_t get_type(void) const { return m_type; }
					bool is_vor(void) const { return m_vor; }
					bool is_ndb(void) const { return m_ndb; }
					bool is_valid(void) const { return m_type != Vertex::type_undefined; }

				protected:
					std::string m_ident;
					Point m_coord;
					Vertex::type_t m_type;
					bool m_vor;
					bool m_ndb;
				};

				Sequence(type_t t = type_dct, const RulePoint& pt0 = RulePoint(), const RulePoint& pt1 = RulePoint(),
					 int32_t lwralt = 0, int32_t upralt = std::numeric_limits<int32_t>::max(),
					 const std::string& id = "", const std::string& apt = "", char bc = 0,
					 bool lwraltvalid = false, bool upraltvalid = false)
					: m_point0(pt0), m_point1(pt1), m_ident(id), m_airport(apt),
					  m_lwralt(lwralt), m_upralt(upralt), m_bdryclass(bc), m_type(t),
					  m_lwraltvalid(lwraltvalid), m_upraltvalid(upraltvalid) {}
				Sequence(const Glib::RefPtr<const TFRAirspace>& aspc, const AltRange& altrange);
				Sequence(const RuleWptAlt& rwa);
				Sequence(const SegWptsAlt& swa);
				const RulePoint& get_point0(void) const { return m_point0; }
				const RulePoint& get_point1(void) const { return m_point1; }
				const std::string& get_ident(void) const { return m_ident; }
				const std::string& get_airport(void) const { return m_airport; }
				int32_t get_lower_alt(void) const { return m_lwralt; }
				int32_t get_upper_alt(void) const { return m_upralt; }
				char get_bdryclass(void) const { return m_bdryclass; }
				type_t get_type(void) const { return m_type; }
				bool is_valid(void) const { return m_type != type_invalid; }
				static const std::string& get_type_string(type_t t);
				const std::string& get_type_string(void) const { return get_type_string(get_type()); }
				bool is_lower_alt_valid(void) const { return m_lwraltvalid; }
				bool is_upper_alt_valid(void) const { return m_upraltvalid; }
				bool is_alt_valid(void) const { return is_lower_alt_valid() && is_upper_alt_valid(); }
				int32_t get_lower_alt_if_valid(void) const { if (is_lower_alt_valid()) return get_lower_alt(); return std::numeric_limits<int32_t>::min(); }
				int32_t get_upper_alt_if_valid(void) const { if (is_upper_alt_valid()) return get_upper_alt(); return std::numeric_limits<int32_t>::min(); }

			protected:
				RulePoint m_point0;
				RulePoint m_point1;
				std::string m_ident;
				std::string m_airport;
				int32_t m_lwralt;
				int32_t m_upralt;
				char m_bdryclass;
				type_t m_type;
				bool m_lwraltvalid;
				bool m_upraltvalid;
			};

			Alternative(const set_t& vs = set_t(), const set_t& es = set_t()) : m_vertexset(vs), m_edgeset(es) {}
			unsigned int size(void) const { return m_seq.size(); }
			bool empty(void) const { return m_seq.empty(); }
			const Sequence& operator[](unsigned int idx) const { return m_seq[idx]; }
			// restrictive element vertex/edge set
			const set_t& get_vertexset(void) const { return m_vertexset; }
			const set_t& get_edgeset(void) const { return m_edgeset; }

		protected:
			typedef std::vector<Sequence> seq_t;
			seq_t m_seq;
			set_t m_vertexset;
			set_t m_edgeset;

			friend class TrafficFlowRestrictions;
			friend class RestrictionSequence;
			friend class ConditionAnd;
			friend class ConditionSequence;
			void add_sequence(const Sequence& s) { m_seq.push_back(s); }
		};

		typedef enum {
			codetype_mandatory,
			codetype_forbidden,
			codetype_closed
		} codetype_t;

		RuleResult(const std::string& rule = "", const std::string& desc = "", const std::string& oprgoal = "",
			   const std::string& cond = "", codetype_t ct = codetype_closed, bool dct = false, bool uncond = false,
			   bool rtestatic = false, bool mandinbound = false, bool disabled = false,
			   const set_t& vs = set_t(), const set_t& es = set_t(), const set_t& rls = set_t())
			: m_vertexset(vs), m_edgeset(es), m_reflocset(rls), m_rule(rule), m_desc(desc), m_oprgoal(oprgoal),
			  m_cond(cond), m_codetype(ct), m_dct(dct), m_uncond(uncond), m_routestatic(rtestatic),
			  m_mandatoryinbound(mandinbound), m_disabled(disabled) {}
		unsigned int size(void) const { return m_elements.size(); }
		bool empty(void) const { return m_elements.empty(); }
		const Alternative& operator[](unsigned int idx) const { return m_elements[idx]; }
		unsigned int crossingcond_size(void) const { return m_crossingcond.size(); }
		bool crossingcond_empty(void) const { return m_crossingcond.empty(); }
		const Alternative::Sequence& crossingcond(unsigned int idx) const { return m_crossingcond[idx]; }
		// condition vertex/edge set
		const set_t& get_vertexset(void) const { return m_vertexset; }
		const set_t& get_edgeset(void) const { return m_edgeset; }
		const set_t& get_reflocset(void) const { return m_reflocset; }
		const std::string& get_rule(void) const { return m_rule; }
		const std::string& get_desc(void) const { return m_desc; }
		const std::string& get_oprgoal(void) const { return m_oprgoal; }
		const std::string& get_cond(void) const { return m_cond; }
		codetype_t get_codetype(void) const { return m_codetype; }
		bool is_dct(void) const { return m_dct; }
		bool is_unconditional(void) const { return m_uncond; }
		bool is_routestatic(void) const { return m_routestatic; }
		bool is_mandatoryinbound(void) const { return m_mandatoryinbound; }
		bool is_disabled(void) const { return m_disabled; }

	protected:
		typedef std::vector<Alternative> elements_t;
		elements_t m_elements;
		typedef std::vector<Alternative::Sequence> crossingcond_t;
		crossingcond_t m_crossingcond;
		set_t m_vertexset;
		set_t m_edgeset;
		set_t m_reflocset;
		std::string m_rule;
		std::string m_desc;
		std::string m_oprgoal;
		std::string m_cond;
		codetype_t m_codetype;
		bool m_dct;
		bool m_uncond;
		bool m_routestatic;
		bool m_mandatoryinbound;
		bool m_disabled;

		friend class TrafficFlowRestrictions;
		friend class TrafficFlowRule;
		void add_element(const Alternative& e) { m_elements.push_back(e); }
		void add_crossingcond(const Alternative::Sequence& e) { m_crossingcond.push_back(e); }
		void set_disabled(bool x = true) { m_disabled = x; }
	};

	class Result {
	public:
		Result(const std::string& acft = "", const std::string& equip = "", Aircraft::pbn_t pbn = Aircraft::pbn_none, char typeofflight = 'G');
		operator bool(void) const { return m_result; }
		unsigned int size(void) const { return m_rules.size(); }
		bool empty(void) const { return m_rules.empty(); }
		const RuleResult& operator[](unsigned int idx) const { return m_rules[idx]; }
		unsigned int messages_size(void) const { return m_messages.size(); }
		bool messages_empty(void) const { return m_messages.empty(); }
		const Message& get_message(unsigned int idx) const { return m_messages[idx]; }
		const Fpl& get_fplan(void) const { return m_fplan; }
		const std::string& get_aircrafttype(void) const { return m_aircrafttype; }
		const std::string& get_aircrafttypeclass(void) const { return m_aircrafttypeclass; }
		const std::string& get_equipment(void) const { return m_equipment; }
		Aircraft::pbn_t get_pbn(void) const { return m_pbn; }
		std::string get_pbn_string(void) const { return Aircraft::get_pbn_string(get_pbn()); }
		char get_typeofflight(void) const { return m_typeofflight; }

	protected:
		typedef std::vector<RuleResult> rules_t;
		rules_t m_rules;
		typedef std::vector<Message> messages_t;
		messages_t m_messages;
		Fpl m_fplan;
		std::string m_aircrafttype;
		std::string m_aircrafttypeclass;
		std::string m_equipment;
		Aircraft::pbn_t m_pbn;
		char m_typeofflight;
		bool m_result;

		friend class TrafficFlowRestrictions;
		friend class TrafficFlowRule;
	};

	class RuleWpt {
	public:
		RuleWpt(FplWpt::type_t t = Vertex::type_undefined, const std::string& ident = "", const Point& coord = Point(), bool vor = false, bool ndb = false)
			: m_ident(ident), m_coord(coord), m_type(t), m_vor(vor), m_ndb(ndb) {}
		FplWpt::type_t get_type(void) const { return m_type; }
		const std::string& get_ident(void) const { return m_ident; }
		const Point& get_coord(void) const { return m_coord; }
		bool is_vor(void) const { return m_vor; }
		bool is_ndb(void) const { return m_ndb; }

		void set_type(FplWpt::type_t t) { m_type = t; }
		void set_ident(const std::string& id) { m_ident = id; }
		void set_coord(const Point& pt) { m_coord = pt; }
		void set_lat(Point::coord_t x) { m_coord.set_lat(x); }
		void set_lon(Point::coord_t x) { m_coord.set_lon(x); }
		void set_vor(bool x) { m_vor = x; }
		void set_ndb(bool x) { m_ndb = x; }

		void save_binary(std::ostream& os) const;
		void load_binary(BinLoader& ldr);

		bool is_same(const RuleWpt& x) const;

		bool operator==(const RuleWpt& x) const;
		bool operator!=(const RuleWpt& x) const { return !(*this == x); }
		bool operator<(const RuleWpt& x) const;
		bool operator>(const RuleWpt& x) const { return (x < *this); }
		bool operator<=(const RuleWpt& x) const { return !(*this > x); }
		bool operator>=(const RuleWpt& x) const { return !(*this < x); }

	protected:
		std::string m_ident;
		Point m_coord;
		FplWpt::type_t m_type;
		bool m_vor;
		bool m_ndb;
	};

	class AltRange {
	public:
		typedef enum {
			alt_first,
			alt_qnh = alt_first,
			alt_std,
			alt_height,
			alt_invalid
		} alt_t;

		AltRange(int32_t lwr = 0, alt_t lwrmode = alt_invalid, int32_t upr = std::numeric_limits<int32_t>::max(), alt_t uprmode = alt_invalid);
		alt_t get_lower_mode(void) const { return m_lwrmode; }
		int32_t get_lower_alt(void) const { return m_lwralt; }
		alt_t get_upper_mode(void) const { return m_uprmode; }
		int32_t get_upper_alt(void) const { return m_upralt; }
		void set_lower_mode(alt_t x) { m_lwrmode = x; }
		void set_lower_alt(int32_t x) { m_lwralt = x; }
		void set_upper_mode(alt_t x) { m_uprmode = x; }
		void set_upper_alt(int32_t x) { m_upralt = x; }
		static const std::string& get_mode_string(alt_t x);
		const std::string& get_lower_mode_string(void) const { return get_mode_string(get_lower_mode()); }
		const std::string& get_upper_mode_string(void) const { return get_mode_string(get_upper_mode()); }
		void set_lower_mode(const std::string& x);
		void set_upper_mode(const std::string& x);
		bool is_lower_valid(void) const;
		bool is_upper_valid(void) const;
		bool is_valid(void) const;
		int32_t get_lower_alt_if_valid(void) const;
		int32_t get_upper_alt_if_valid(void) const;

		void save_binary(std::ostream& os) const;
		void load_binary(BinLoader& ldr);
		std::string to_str(void) const;

	protected:
		int32_t m_lwralt;
		int32_t m_upralt;
		alt_t m_lwrmode;
		alt_t m_uprmode;
	};

	class RuleWptAlt {
	public:
		RuleWptAlt(const RuleWpt& wpt, const AltRange& alt) : m_wpt(wpt), m_alt(alt) {}
		const RuleWpt& get_wpt(void) const { return m_wpt; }
		const AltRange& get_alt(void) const { return m_alt; }

	protected:
		RuleWpt m_wpt;
		AltRange m_alt;
	};

	class SegWptsAlt {
	public:
		SegWptsAlt(const RuleWpt& wpt0, const RuleWpt& wpt1, const AltRange& alt, const std::string& awy = "")
			: m_wpt0(wpt0), m_wpt1(wpt1), m_alt(alt), m_airway(awy) {}
		const RuleWpt& get_wpt0(void) const { return m_wpt0; }
		const RuleWpt& get_wpt1(void) const { return m_wpt1; }
		const AltRange& get_alt(void) const { return m_alt; }
		const std::string& get_airway(void) const { return m_airway; }
		bool is_dct(void) const { return m_airway.empty(); }

	protected:
		RuleWpt m_wpt0;
		RuleWpt m_wpt1;
		AltRange m_alt;
		std::string m_airway;
	};

	TrafficFlowRestrictions(void);
	const std::string& get_origin(void) const { return m_origin; }
	const std::string& get_created(void) const { return m_created; }
	const std::string& get_effective(void) const { return m_effective; }

	Result check_fplan(const FPlanRoute& route, char type_of_flight,
			   const std::string& acft_type, const std::string& equipment, Aircraft::pbn_t pbn,
			   AirportsDb& airportdb, NavaidsDb& navaiddb, WaypointsDb& waypointdb,
			   AirwaysDb& airwaydb, AirspacesDb& airspacedb);

	Result check_fplan(const FPlanRoute& route, char type_of_flight,
			   const std::string& acft_type, const std::string& equipment, const std::string& pbn,
			   AirportsDb& airportdb, NavaidsDb& navaiddb, WaypointsDb& waypointdb,
			   AirwaysDb& airwaydb, AirspacesDb& airspacedb) {
		return check_fplan(route, type_of_flight, acft_type, equipment, Aircraft::parse_pbn(pbn),
				   airportdb, navaiddb, waypointdb, airwaydb, airspacedb);
	}

	typedef tracerules_t::const_iterator tracerules_const_iterator_t;
	tracerules_const_iterator_t tracerules_begin(void) const { return m_tracerules.begin(); }
	tracerules_const_iterator_t tracerules_end(void) const { return m_tracerules.end(); }
	bool tracerules_empty(void) const { return m_tracerules.empty(); }
	bool tracerules_exists(const std::string& ruleid) const;
	bool tracerules_add(const std::string& ruleid);
	bool tracerules_erase(const std::string& ruleid);
	void tracerules_clear(void) { m_tracerules.clear(); }
	unsigned int tracerules_count(void) const { return m_tracerules.size(); }

	typedef disabledrules_t::const_iterator disabledrules_const_iterator_t;
	disabledrules_const_iterator_t disabledrules_begin(void) const { return m_disabledrules.begin(); }
	disabledrules_const_iterator_t disabledrules_end(void) const { return m_disabledrules.end(); }
	bool disabledrules_empty(void) const { return m_disabledrules.empty(); }
	bool disabledrules_exists(const std::string& ruleid) const;
	bool disabledrules_add(const std::string& ruleid);
	bool disabledrules_erase(const std::string& ruleid);
	void disabledrules_clear(void) { m_disabledrules.clear(); }
	unsigned int disabledrules_count(void) const { return m_disabledrules.size(); }

	void save_disabled_trace_rules_file(const std::string& fname) const;
	bool load_disabled_trace_rules_file(const std::string& fname);
	std::string save_disabled_trace_rules_string(void) const;
	bool load_disabled_trace_rules_string(const std::string& data);

	std::vector<RuleResult> find_rules(const std::string& name, bool exact = true);
	bool load_rules(std::vector<Message>& msg, const std::string& fname, AirportsDb& airportdb,
			NavaidsDb& navaiddb, WaypointsDb& waypointdb, AirwaysDb& airwaydb, AirspacesDb& airspacedb);
	bool load_binary_rules(std::vector<Message>& msg, const std::string& fname, AirportsDb& airportdb,
			       NavaidsDb& navaiddb, WaypointsDb& waypointdb, AirwaysDb& airwaydb, AirspacesDb& airspacedb);
	bool load_binary_rules(std::vector<Message>& msg, std::istream& is, AirportsDb& airportdb,
			       NavaidsDb& navaiddb, WaypointsDb& waypointdb, AirwaysDb& airwaydb, AirspacesDb& airspacedb);
	bool save_binary_rules(const std::string& fname) const;
	bool save_binary_rules(std::ostream& os) const;
	void start_add_rules(void);
	bool add_rules(std::vector<Message>& msg, const std::string& fname, AirportsDb& airportdb,
		       NavaidsDb& navaiddb, WaypointsDb& waypointdb, AirwaysDb& airwaydb, AirspacesDb& airspacedb);
	bool add_binary_rules(std::vector<Message>& msg, const std::string& fname, AirportsDb& airportdb,
			      NavaidsDb& navaiddb, WaypointsDb& waypointdb, AirwaysDb& airwaydb, AirspacesDb& airspacedb);
	bool add_binary_rules(std::vector<Message>& msg, std::istream& is, AirportsDb& airportdb,
			      NavaidsDb& navaiddb, WaypointsDb& waypointdb, AirwaysDb& airwaydb, AirspacesDb& airspacedb);
	void end_add_rules(void);
	void reset_rules(void);
	void prepare_dct_rules(void);
	unsigned int count_rules(void) const;
	unsigned int count_dct_rules(void) const;
	void simplify_rules(void);
	void simplify_rules_bbox(const Rect& bbox);
	void simplify_rules_altrange(int32_t minalt, int32_t maxalt);
	void simplify_rules_aircrafttype(const std::string& acfttype);
	void simplify_rules_aircraftclass(const std::string& acftclass);
	void simplify_rules_equipment(const std::string& equipment, Aircraft::pbn_t pbn);
	void simplify_rules_typeofflight(char type_of_flight);
	void simplify_rules_mil(bool mil = false);
	void simplify_rules_gat(bool gat = true);
	void simplify_rules_dep(const std::string& ident, const Point& coord);
	void simplify_rules_dest(const std::string& ident, const Point& coord);
	void simplify_rules_complexity(void);

	class DctParameters {
	public:
		class AltRange {
		public:
			typedef IntervalSet<int32_t> set_t;
			typedef set_t::type_t type_t;
			AltRange(const set_t& set0 = set_t(), const set_t& set1 = set_t());

			bool is_inside(unsigned int index, type_t alt) const;
			bool is_empty(void) const;
			const set_t& operator[](unsigned int idx) const { return m_set[!!idx]; }
			set_t& operator[](unsigned int idx) { return m_set[!!idx]; }
			void set_empty(void);
			void set_full(void);
			void set_interval(type_t alt0, type_t alt1);
			void set(const TrafficFlowRestrictions::AltRange& ar);
			void invert(void);
			AltRange& operator&=(const AltRange& x);
			AltRange& operator|=(const AltRange& x);
			AltRange& operator^=(const AltRange& x);
			AltRange& operator-=(const AltRange& x);
			AltRange operator~(void) const;

		protected:
			set_t m_set[2];
		};

		DctParameters(const std::string& ident0 = "", const Point& coord0 = Point(),
			      const std::string& ident1 = "", const Point& coord1 = Point(),
			      AltRange::type_t alt0 = std::numeric_limits<AltRange::type_t>::min(),
			      AltRange::type_t alt1 = std::numeric_limits<AltRange::type_t>::max());
		const std::string& get_ident(unsigned int index) const { return m_ident[!!index]; }
		const Point& get_coord(unsigned int index) const { return m_coord[!!index]; }
		const AltRange::set_t& get_alt(unsigned int index) const { return m_alt[!!index]; }
		AltRange::set_t& get_alt(unsigned int index) { return m_alt[!!index]; }
		const AltRange& get_alt(void) const { return m_alt; }
		AltRange& get_alt(void) { return m_alt; }
		DctParameters& operator&=(const AltRange& x) { m_alt &= x; return *this; }
		DctParameters& operator|=(const AltRange& x) { m_alt |= x; return *this; }
		DctParameters& operator^=(const AltRange& x) { m_alt ^= x; return *this; }
		DctParameters& operator-=(const AltRange& x) { m_alt -= x; return *this; }
		double get_dist(void) const;

	protected:
		AltRange m_alt;
		std::string m_ident[2];
		Point m_coord[2];
		mutable double m_dist;
	};

	bool check_dct(std::vector<Message>& msg, DctParameters& dct, bool trace = false);

	class DctSegments {
	public:
		typedef RuleResult::Alternative::Sequence::RulePoint RulePoint;

		class DctSegment {
		public:
			DctSegment(const std::string& id0 = "", const Point& pt0 = Point(), Vertex::type_t typ0 = Vertex::type_undefined,
				   bool vor0 = false, bool ndb0 = false,
				   const std::string& id1 = "", const Point& pt1 = Point(), Vertex::type_t typ1 = Vertex::type_undefined,
				   bool vor1 = false, bool ndb1 = false) : m_pt0(id0, pt0, typ0, vor0, ndb0), m_pt1(id1, pt1, typ1, vor1, ndb1) {}
			DctSegment(const RuleWpt& wpt0, const RuleWpt& wpt1) : m_pt0(wpt0), m_pt1(wpt1) {}

			RulePoint& operator[](unsigned int x) { return x ? m_pt1 : m_pt0; }
			const RulePoint& operator[](unsigned int x) const { return x ? m_pt1 : m_pt0; }
			int compare(const DctSegment& x) const;
			bool operator<(const DctSegment& x) const { return compare(x) < 0; }
			bool operator>(const DctSegment& x) const { return compare(x) > 0; }
			bool operator==(const DctSegment& x) const { return compare(x) == 0; }
			bool operator!=(const DctSegment& x) const { return compare(x) != 0; }

		protected:
			RulePoint m_pt0;
			RulePoint m_pt1;
		};

		DctSegments(void) {}

		bool empty(void) const { return m_dctseg.empty(); }
		unsigned int size(void) const { return m_dctseg.size(); }
		const DctSegment& operator[](unsigned int x) const { return m_dctseg[x]; }
		void add(const DctSegment& x) { m_dctseg.push_back(x); }

	protected:
		typedef std::vector<DctSegment> dctseg_t;
		dctseg_t m_dctseg;
	};

	DctSegments get_dct_segments(void) const;

	/* used by the expression tree evaluation */
	Graph::vertex_descriptor get_vertexdesc(const RuleWpt& wpt);
	const Fpl& get_route(void) const { return m_route; }
	AirportsDb& get_airportdb(void) { return *m_airportdb; }
	NavaidsDb& get_navaiddb(void) { return *m_navaiddb; }
	WaypointsDb& get_waypointdb(void) { return *m_waypointdb; }
	AirwaysDb& get_airwaydb(void) { return *m_airwaydb; }
	AirspacesDb& get_airspacedb(void) { return *m_airspacedb; }
	const Graph& get_graph(void) const { return m_graph; }
	const Rect& get_graphbbox(void) const { return m_graphbbox; }
	const std::string& get_aircrafttype(void) const { return m_aircrafttype; }
	const std::string& get_aircrafttypeclass(void) const { return m_aircrafttypeclass; }
	const std::string& get_equipment(void) const { return m_equipment; }
	Aircraft::pbn_t get_pbn(void) const { return m_pbn; }
	std::string get_pbn_string(void) const { return Aircraft::get_pbn_string(get_pbn()); }
	char get_typeofflight(void) const { return m_typeofflight; }
	void set_origin(const std::string& x) { m_origin = x; }
	void set_created(const std::string& x) { m_created = x; }
	void set_effective(const std::string& x) { m_effective = x; }
	void message(const std::string& text = "", Message::type_t t = Message::type_error, const std::string& rule = "");
	void message(const Message& msg);

protected:
	class CondResult {
	public:
		CondResult(boost::logic::tribool result = boost::logic::indeterminate) : m_result(result) {}
		boost::logic::tribool get_result(void) const { return m_result; }
		void set_result(boost::logic::tribool result = boost::logic::indeterminate) { m_result = result; }
		typedef std::set<unsigned int> set_t;
		set_t& get_vertexset(void) { return m_vertexset; }
		const set_t& get_vertexset(void) const { return m_vertexset; }
		set_t& get_edgeset(void) { return m_edgeset; }
		const set_t& get_edgeset(void) const { return m_edgeset; }
		set_t& get_reflocset(void) { return m_reflocset; }
		const set_t& get_reflocset(void) const { return m_reflocset; }
		unsigned int get_first(void) const;
		unsigned int get_last(void) const;
		CondResult& operator&=(const CondResult& x);
		CondResult& operator|=(const CondResult& x);
		CondResult operator~(void);
		CondResult operator&(const CondResult& x) { CondResult r(*this); r &= x; return r; }
		CondResult operator|(const CondResult& x) { CondResult r(*this); r |= x; return r; }

	protected:
		set_t m_vertexset;
		set_t m_edgeset;
		set_t m_reflocset;
		boost::logic::tribool m_result;
	};

	class Loader;
	//friend class Loader;

	class TrafficFlowRule;

	class Timesheet {
	public:
		typedef enum {
			workhr_first,
			workhr_h24 = workhr_first,
			workhr_timesheet,
			workhr_invalid
		} workhr_t;

		class Element {
		public:
			typedef enum {
				timeref_first,
				timeref_utc = timeref_first,
				timeref_utcw,
				timeref_invalid
			} timeref_t;

			Element(void);

			unsigned int get_monwef(void) const { return m_monwef; }
			unsigned int get_montil(void) const { return m_montil; }
			unsigned int get_daywef(void) const { return m_daywef; }
			unsigned int get_daytil(void) const { return m_daytil; }
			int32_t get_timewef(void) const { return m_timewef; }
			int32_t get_timetil(void) const { return m_timetil; }
			unsigned int get_day(void) const { return m_day; }
			timeref_t get_timeref(void) const { return m_timeref; }
			bool is_sunrise(void) const { return m_sunrise; }
			bool is_sunset(void) const { return m_sunset; }
			bool is_valid(void) const;
			bool is_inside(time_t t, const Point& pt) const;

			const std::string& get_day_string(void) const { return get_day_string(get_day()); }
			static const std::string& get_day_string(unsigned int d);
			const std::string& get_timeref_string(void) const { return get_timeref_string(get_timeref()); }
			static const std::string& get_timeref_string(timeref_t t);

			void set_monwef(unsigned int x) { m_monwef = x; }
			void set_montil(unsigned int x) { m_montil = x; }
			void set_daywef(unsigned int x) { m_daywef = x; }
			void set_daytil(unsigned int x) { m_daytil = x; }
			void set_datewef(const std::string& x);
			void set_datetil(const std::string& x);
			void set_timewef(int32_t x) { m_timewef = x; }
			void set_timetil(int32_t x) { m_timetil = x; }
			void set_timewef(const std::string& x);
			void set_timetil(const std::string& x);
			void set_day(unsigned int x) { m_day = x; }
			void set_day(const std::string& x);
			void set_timeref(timeref_t x) { m_timeref = x; }
			void set_timeref(const std::string& x);
			void set_sunrise(bool x) { m_sunrise = x; }
			void set_sunset(bool x) { m_sunset = x; }

			std::string to_ustr(void) const;
			void save_binary(std::ostream& os) const;
			void load_binary(BinLoader& ldr);

		protected:
			int32_t m_timewef; // seconds since midnight UTC or SR
			int32_t m_timetil; // seconds since midnight UTC or SS
			uint8_t m_monwef; // 0-11
			uint8_t m_montil; // 0-11
			uint8_t m_daywef; // 1-31
			uint8_t m_daytil; // 1-31
			uint8_t m_day; // 0-6, 0 = sunday
			timeref_t m_timeref;
			bool m_sunrise;
			bool m_sunset;
		};

	protected:
		typedef std::vector<Element> elements_t;

	public:
		Timesheet(void);

		elements_t::size_type size(void) const { return m_elements.size(); }
		const Element& operator[](elements_t::size_type i) const { return m_elements[i]; }

		workhr_t get_workhr(void) const { return m_workhr; }
		void set_workhr(workhr_t x) { m_workhr = x; }
		void set_workhr(const std::string& x);
		const std::string& get_workhr_string(void) const { return get_workhr_string(get_workhr()); }
		static const std::string& get_workhr_string(workhr_t x);
		void add(const Element& el) { m_elements.push_back(el); }
		bool is_valid(void) const;
		bool is_inside(time_t t, const Point& pt) const;
		std::string to_str(void) const;
		void save_binary(std::ostream& os) const;
		void load_binary(BinLoader& ldr);

	protected:
		elements_t m_elements;
		workhr_t m_workhr;
	};

	class TFRAirspace {
	public:
		typedef Glib::RefPtr<TFRAirspace> ptr_t;
		typedef Glib::RefPtr<const TFRAirspace> const_ptr_t;

		TFRAirspace(const std::string& id, const std::string& type, const MultiPolygonHole& poly, const Rect& bbox,
			    char bc, uint8_t tc, int32_t altlwr, int32_t altupr);

		void reference(void) const;
		void unreference(void) const;

		const std::string& get_ident(void) const { return m_id; }
		const std::string& get_type(void) const { return m_type; }
		const MultiPolygonHole& get_poly(void) const { return m_poly; }
		const Rect& get_bbox(void) const { return m_bbox; }
		char get_bdryclass(void) const { return m_bdryclass; }
		uint8_t get_typecode(void) const { return m_typecode; }
		int32_t get_lower_alt(void) const { return m_lwralt; }
		int32_t get_upper_alt(void) const { return m_upralt; }
		void add_component(const_ptr_t aspc, AirspacesDb::Airspace::Component::operator_t oper);
		bool is_inside(const Point& coord) const { return is_inside(coord, 0, 0, std::numeric_limits<int32_t>::max()); }
		bool is_inside(const Point& coord, int32_t alt) const { return is_inside(coord, alt, std::numeric_limits<int32_t>::min(),
											 std::numeric_limits<int32_t>::min()); }
		bool is_inside(const Point& coord, int32_t alt, int32_t altlwr, int32_t altupr) const;
                bool is_intersection(const Point& la, const Point& lb) const { return is_intersection(la, lb, 0, 0, std::numeric_limits<int32_t>::max()); }
		bool is_intersection(const Point& la, const Point& lb, int32_t alt) const { return is_intersection(la, lb, 0, std::numeric_limits<int32_t>::min(),
														   std::numeric_limits<int32_t>::min()); }
                bool is_intersection(const Point& la, const Point& lb, int32_t alt, int32_t altlwr, int32_t altupr) const;
		IntervalSet<int32_t> get_altrange(const Point& coord,
						  int32_t altlwr = std::numeric_limits<int32_t>::min(),
						  int32_t altupr = std::numeric_limits<int32_t>::min()) const;
		IntervalSet<int32_t> get_altrange(const Point& coord0, const Point& coord1,
						  int32_t altlwr = std::numeric_limits<int32_t>::min(),
						  int32_t altupr = std::numeric_limits<int32_t>::min()) const;
		void get_boundingalt(int32_t& lwralt, int32_t& upralt) const;
		void save_binary(std::ostream& os) const;
		static void save_binary(std::ostream& os, const_ptr_t aspc);
		static const_ptr_t load_binary(BinLoader& ldr);

	protected:
		class PointCache {
		public:
			PointCache(const Point& pt = Point(), bool inside = false) : m_point(pt), m_inside(inside) {}
			const Point& get_point(void) const { return m_point; }
			bool is_inside(void) const { return m_inside; }
			bool operator<(const PointCache& x) const;

		protected:
			Point m_point;
			bool m_inside;
		};

		class LineCache {
		public:
			LineCache(const Point& pt0 = Point(), const Point& pt1 = Point(), bool isect = false) : m_point0(pt0), m_point1(pt1), m_intersect(isect) {}
			const Point& get_point0(void) const { return m_point0; }
			const Point& get_point1(void) const { return m_point1; }
			bool is_intersect(void) const { return m_intersect; }
			bool operator<(const LineCache& x) const;

		protected:
			Point m_point0;
			Point m_point1;
			bool m_intersect;
		};

		mutable gint m_refcount;
		std::string m_id;
		std::string m_type;
		MultiPolygonHole m_poly;
		Rect m_bbox;
		typedef std::set<PointCache> pointcache_t;
		mutable pointcache_t m_pointcache;
		typedef std::set<LineCache> linecache_t;
		mutable linecache_t m_linecache;
		typedef std::pair<const_ptr_t,AirspacesDb::Airspace::Component::operator_t> comp_t;
		typedef std::vector<comp_t> comps_t;
		comps_t m_comps;
		int32_t m_lwralt;
		int32_t m_upralt;
		char m_bdryclass;
		uint8_t m_typecode;
	};

	class DbLoader {
	public:
		DbLoader(TrafficFlowRestrictions& tfrs, const sigc::slot<void,Message::type_t,const std::string&,const std::string&>& msg,
			 AirportsDb& airportdb, NavaidsDb& navaiddb, WaypointsDb& waypointdb, AirwaysDb& airwaydb, AirspacesDb& airspacedb);
		virtual ~DbLoader();

		void error(const std::string& text) const;
		void error(const std::string& child, const std::string& text) const;
		void warning(const std::string& text) const;
		void warning(const std::string& child, const std::string& text) const;
		void info(const std::string& text) const;
		void info(const std::string& child, const std::string& text) const;
		void message(Message::type_t mt, const std::string& child, const std::string& text) const { m_message(mt, child, text); }

		TrafficFlowRestrictions& get_tfrs(void) const { return m_tfrs; }
		AirportsDb& get_airportdb(void) const { return m_airportdb; }
		NavaidsDb& get_navaiddb(void) const { return m_navaiddb; }
		WaypointsDb& get_waypointdb(void) const { return m_waypointdb; }
		AirwaysDb& get_airwaydb(void) const { return m_airwaydb; }
		AirspacesDb& get_airspacedb(void) const { return m_airspacedb; }

		TFRAirspace::const_ptr_t find_airspace(const std::string& id, const std::string& type);
		TFRAirspace::const_ptr_t find_airspace(const std::string& id, char bdryclass, uint8_t typecode = AirspacesDb::Airspace::typecode_ead);
		const AirportsDb::Airport& find_airport(const std::string& icao);
		void fill_airport_cache(void);

	protected:
		TrafficFlowRestrictions& m_tfrs;
		sigc::slot<void,Message::type_t,const std::string&,const std::string&> m_message;
		AirportsDb& m_airportdb;
		NavaidsDb& m_navaiddb;
		WaypointsDb& m_waypointdb;
		AirwaysDb& m_airwaydb;
		AirspacesDb& m_airspacedb;

		class AirspaceMapping {
		public:
			AirspaceMapping(const char *name, char bc, const char *name0 = 0, char bc0 = 0,
					const char *name1 = 0, char bc1 = 0, const char *name2 = 0, char bc2 = 0);
			unsigned int size(void) const;
			bool operator<(const AirspaceMapping& x) const;
			std::string get_name(unsigned int i) const;
			char get_bdryclass(unsigned int i) const;

		protected:
			const char *m_names[4];
			char m_bdryclass[4];
		};
		static const AirspaceMapping airspacemapping[];

		static const AirspaceMapping *find_airspace_mapping(const std::string& name, char bc);
		static void check_airspace_mapping(void);
	

		class AirspaceCache {
		public:
			AirspaceCache(const std::string& id, char bc, uint8_t tc);
			AirspaceCache(TFRAirspace::const_ptr_t aspc = TFRAirspace::const_ptr_t());

			const std::string& get_id(void) const { return m_id; }
			char get_bdryclass(void) const { return m_bdryclass; }
			uint8_t get_typecode(void) const { return m_typecode; }
			TFRAirspace::const_ptr_t get_airspace(void) const { return m_airspace; }
			const Glib::ustring& get_type(void) const;

			bool operator<(const AirspaceCache& x) const;

		protected:
			std::string m_id;
			TFRAirspace::const_ptr_t m_airspace;
			char m_bdryclass;
			uint8_t m_typecode;
		};
		typedef std::set<AirspaceCache> airspacecache_t;
		airspacecache_t m_airspacecache;

		class AirportCache : public AirportsDb::Airport {
		public:
			AirportCache() {}
			AirportCache(const AirportsDb::Airport& x) : AirportsDb::Airport(x) {}

			bool operator<(const AirportCache& x) const;
		};
		typedef std::set<AirportCache> airportcache_t;
		airportcache_t m_airportcache;
		bool m_airportcache_filled;
	};

	friend class DbLoader;

	class RestrictionElement {
	public:
		typedef Glib::RefPtr<RestrictionElement> ptr_t;
		typedef Glib::RefPtr<const RestrictionElement> const_ptr_t;

		RestrictionElement(const AltRange& alt = AltRange());
		virtual ~RestrictionElement();

		void reference(void) const;
		void unreference(void) const;
		virtual ptr_t clone(void) const = 0;
		virtual bool is_bbox(const Rect& bbox) const = 0;
		virtual bool is_altrange(int32_t minalt, int32_t maxalt) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs) const = 0;
		virtual CondResult evaluate_trace(TrafficFlowRestrictions& tfrs, const TrafficFlowRule *rule, unsigned int altcnt, unsigned int seqcnt) const;
		virtual RuleResult::Alternative::Sequence get_rule(void) const = 0;
		virtual bool is_valid_dct(void) const { return false; }
		virtual DctParameters::AltRange evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct) const;
		virtual DctParameters::AltRange evaluate_dct_trace(TrafficFlowRestrictions& tfrs, const DctParameters& dct, const TrafficFlowRule *rule,
								   unsigned int altcnt, unsigned int seqcnt) const;
		virtual void get_dct_segments(DctSegments& seg) const;
		virtual std::vector<ptr_t> clone_crossingpoints(const std::vector<RuleWptAlt>& pts) const { return std::vector<ptr_t>(); }
		virtual std::vector<ptr_t> clone_crossingsegments(const std::vector<SegWptsAlt>& segs) const { return std::vector<ptr_t>(); }
		virtual bool is_mandatoryinbound(const std::vector<RuleWptAlt>& pts) const { return false; }
		virtual void save_binary(std::ostream& os) const = 0;
		static ptr_t load_binary(BinLoader& ldr);
		virtual std::string to_str(void) const;

	protected:
		mutable gint m_refcount;
		AltRange m_alt;
	};

	class RestrictionElementRoute : public RestrictionElement {
	public:
		typedef enum {
			match_dct,
			match_awy,
			match_all
		} match_t;

		RestrictionElementRoute(const AltRange& alt, const RuleWpt& pt0, const RuleWpt& pt1, const std::string& rteid, match_t m);
		virtual ptr_t clone(void) const;
		virtual bool is_bbox(const Rect& bbox) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs) const;
		virtual RuleResult::Alternative::Sequence get_rule(void) const;
		virtual bool is_valid_dct(void) const { return m_match == match_dct; }
		virtual DctParameters::AltRange evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct) const;
		virtual void get_dct_segments(DctSegments& seg) const;
		virtual std::vector<ptr_t> clone_crossingsegments(const std::vector<SegWptsAlt>& segs) const;
		virtual bool is_mandatoryinbound(const std::vector<RuleWptAlt>& pts) const;
		// FIXME: find altitude of route segment!
		virtual void save_binary(std::ostream& os) const;
		static ptr_t load_binary(BinLoader& ldr);
		virtual std::string to_str(void) const;

	protected:
		RuleWpt m_point[2];
		std::string m_rteid;
		match_t m_match;
	};

	class RestrictionElementPoint : public RestrictionElement {
	public:
		RestrictionElementPoint(const AltRange& alt, const RuleWpt& pt);
		virtual ptr_t clone(void) const;
		virtual bool is_bbox(const Rect& bbox) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs) const;
		virtual RuleResult::Alternative::Sequence get_rule(void) const;
		virtual bool is_valid_dct(void) const { return true; }
		virtual DctParameters::AltRange evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct) const;
		virtual std::vector<ptr_t> clone_crossingpoints(const std::vector<RuleWptAlt>& pts) const;
		virtual void save_binary(std::ostream& os) const;
		static ptr_t load_binary(BinLoader& ldr);
		virtual std::string to_str(void) const;

	protected:
		RuleWpt m_point;
	};

	class RestrictionElementSidStar : public RestrictionElement {
	public:
		RestrictionElementSidStar(const AltRange& alt, const std::string& procid, const RuleWpt& arpt, bool star);
		virtual ptr_t clone(void) const;
		virtual bool is_bbox(const Rect& bbox) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs) const;
		virtual RuleResult::Alternative::Sequence get_rule(void) const;
		virtual void save_binary(std::ostream& os) const;
		static ptr_t load_binary(BinLoader& ldr, bool star);
		virtual std::string to_str(void) const;

	protected:
		std::string m_procid;
		RuleWpt m_arpt;
		bool m_star;
	};

	class RestrictionElementAirspace : public RestrictionElement {
	public:
		RestrictionElementAirspace(const AltRange& alt, TFRAirspace::const_ptr_t aspc);
		virtual ptr_t clone(void) const;
		virtual bool is_bbox(const Rect& bbox) const;
		virtual bool is_altrange(int32_t minalt, int32_t maxalt) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs) const;
		virtual RuleResult::Alternative::Sequence get_rule(void) const;
		virtual void save_binary(std::ostream& os) const;
		static ptr_t load_binary(BinLoader& ldr);
		TFRAirspace::const_ptr_t get_airspace(void) const { return m_airspace; }
		virtual std::string to_str(void) const;

	protected:
		TFRAirspace::const_ptr_t m_airspace;
	};

	class RestrictionSequence {
	public:
		RestrictionSequence(void);
		void clone(void);
		bool is_bbox(const Rect& bbox) const;
		bool is_altrange(int32_t minalt, int32_t maxalt) const;
		CondResult evaluate(TrafficFlowRestrictions& tfrs) const;
		CondResult evaluate_trace(TrafficFlowRestrictions& tfrs, const TrafficFlowRule *rule, unsigned int altcnt) const;
		RuleResult::Alternative get_rule(void) const;
		bool is_valid_dct(void) const;
		DctParameters::AltRange evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct) const;
		DctParameters::AltRange evaluate_dct_trace(TrafficFlowRestrictions& tfrs, const DctParameters& dct, const TrafficFlowRule *rule, unsigned int altcnt) const;
		void get_dct_segments(DctSegments& seg) const;
		bool clone_crossingpoints(std::vector<RestrictionSequence>& seq, const std::vector<RuleWptAlt>& pts) const;
		bool clone_crossingsegments(std::vector<RestrictionSequence>& seq, const std::vector<SegWptsAlt>& segs) const;
		bool is_mandatoryinbound(const std::vector<RuleWptAlt>& pts) const;
		std::string to_str(void) const;

	protected:
		typedef std::vector<RestrictionElement::ptr_t> elements_t;

	public:
		typedef elements_t::size_type size_type;

		size_type size(void) const { return m_elements.size(); }
		RestrictionElement::ptr_t operator[](size_type idx) { return m_elements[idx]; }
		RestrictionElement::const_ptr_t operator[](size_type idx) const { return m_elements[idx]; }
		void add(RestrictionElement::ptr_t e);
		void save_binary(std::ostream& os) const;
		void load_binary(BinLoader& ldr);

	protected:
		elements_t m_elements;
	};

	class Restrictions {
	public:
		Restrictions(void);
		void clone(void);
		bool simplify_bbox(const Rect& bbox);
		bool simplify_altrange(int32_t minalt, int32_t maxalt);
		bool evaluate(TrafficFlowRestrictions& tfrs, RuleResult& result, bool forbidden) const;
		bool evaluate_trace(TrafficFlowRestrictions& tfrs, RuleResult& result, bool forbidden, const TrafficFlowRule *rule) const;
		bool is_valid_dct(void) const;
		DctParameters::AltRange evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct, bool forbidden) const;
		DctParameters::AltRange evaluate_dct_trace(TrafficFlowRestrictions& tfrs, const DctParameters& dct, bool forbidden, const TrafficFlowRule *rule) const;
		void get_dct_segments(DctSegments& seg) const;
		void set_ruleresult(RuleResult& result) const;
		bool clone_crossingpoints(Restrictions& r, const std::vector<RuleWptAlt>& pts) const;
		bool clone_crossingsegments(Restrictions& r, const std::vector<SegWptsAlt>& segs) const;
		bool is_mandatoryinbound(const std::vector<RuleWptAlt>& pts) const;
		std::string to_str(void) const;

	protected:
		typedef std::vector<RestrictionSequence> elements_t;

	public:
		typedef elements_t::size_type size_type;

		bool empty(void) const { return m_elements.empty(); }
		size_type size(void) const { return m_elements.size(); }
	        RestrictionSequence& operator[](size_type idx) { return m_elements[idx]; }
	        const RestrictionSequence& operator[](size_type idx) const { return m_elements[idx]; }
		void clear(void) { m_elements.clear(); }
		void add(const RestrictionSequence& e) { m_elements.push_back(e); }
		void add(const Restrictions& e) { m_elements.insert(m_elements.end(), e.m_elements.begin(), e.m_elements.end()); }
		void save_binary(std::ostream& os) const;
		void load_binary(BinLoader& ldr);

	protected:
		elements_t m_elements;
	};

	class Condition {
	public:
		typedef Glib::RefPtr<Condition> ptr_t;
		typedef Glib::RefPtr<const Condition> const_ptr_t;
		static const bool ifr_refloc = true;

		class Path {
		public:
			Path(const Path *backptr = 0, const Condition *condptr = 0) : m_backptr(backptr), m_condptr(condptr) {}
			const Path *get_back(void) const { return m_backptr; }
			const Condition *get_cond(void) const { return m_condptr; }
		protected:
			const Path *m_backptr;
			const Condition *m_condptr;
		};

	        Condition(unsigned int childnum);
		virtual ~Condition();

		void reference(void) const;
		void unreference(void) const;
		unsigned int get_childnum(void) const { return m_childnum; }
		virtual bool is_refloc(void) const { return false; }
		virtual bool is_constant(void) const { return false; }
		virtual bool get_constvalue(void) const { return false; }
		virtual std::string to_str(void) const = 0;
		virtual ptr_t clone(void) const = 0;
		virtual ptr_t simplify(void) const;
		virtual ptr_t simplify_bbox(const Rect& bbox) const;
		virtual ptr_t simplify_altrange(int32_t minalt, int32_t maxalt) const;
		virtual ptr_t simplify_aircrafttype(const std::string& acfttype) const;
		virtual ptr_t simplify_aircraftclass(const std::string& acftclass) const;
		virtual ptr_t simplify_equipment(const std::string& equipment, Aircraft::pbn_t pbn) const;
		virtual ptr_t simplify_typeofflight(char type_of_flight) const;
		virtual ptr_t simplify_mil(bool mil = false) const;
		virtual ptr_t simplify_gat(bool gat = true) const;
		virtual ptr_t simplify_dep(const std::string& ident, const Point& coord) const;
		virtual ptr_t simplify_dest(const std::string& ident, const Point& coord) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const = 0;
		virtual CondResult evaluate_trace(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset, const TrafficFlowRule *rule, const Path *backptr = 0) const;
		virtual RuleResult::Alternative::Sequence get_resultsequence(void) const;
		virtual bool is_routestatic(void) const { return false; }
		virtual bool is_dct(void) const { return false; }
		virtual bool is_valid_dct(void) const { return false; }
		virtual DctParameters::AltRange evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct) const;
		virtual DctParameters::AltRange evaluate_dct_trace(TrafficFlowRestrictions& tfrs, const DctParameters& dct, const TrafficFlowRule *rule, const Path *backptr = 0) const;
		virtual std::vector<RuleResult::Alternative> get_mandatory(void) const;
		virtual std::vector<RuleResult::Alternative> get_mandatory_seq(void) const;
		virtual std::vector<RuleResult::Alternative::Sequence> get_mandatory_or(void) const;
		virtual ptr_t extract_crossingpoints(std::vector<RuleWptAlt>& pts) const { return ptr_t(); }
		virtual ptr_t extract_crossingsegments(std::vector<SegWptsAlt>& segs) const { return ptr_t(); }
		typedef std::pair<TFRAirspace::const_ptr_t,AltRange> airspacealt_t;
		virtual ptr_t extract_crossingairspaces(std::vector<airspacealt_t>& aspcs) const { return ptr_t(); }
		virtual void save_binary(std::ostream& os) const = 0;
		static void save_binary(std::ostream& os, const_ptr_t aspc);
		static ptr_t load_binary(BinLoader& ldr);

	protected:
		mutable gint m_refcount;
		unsigned int m_childnum;

		std::string get_path(const Path *backptr) const;
		void trace(TrafficFlowRestrictions& tfrs, const CondResult& r, const TrafficFlowRule *rule, const Path *backptr) const;
		void trace(TrafficFlowRestrictions& tfrs, const DctParameters::AltRange& r, const TrafficFlowRule *rule, const Path *backptr) const;
	};

	class ConditionAnd : public Condition {
	public:
		ConditionAnd(unsigned int childnum, bool resultinv = false);
		void add(const_ptr_t p, bool inv);
		virtual bool is_refloc(void) const;
		virtual bool is_constant(void) const;
		virtual bool get_constvalue(void) const;
		virtual std::string to_str(void) const;
		virtual ptr_t clone(void) const;
		virtual ptr_t simplify(void) const;
		virtual ptr_t simplify_bbox(const Rect& bbox) const;
		virtual ptr_t simplify_altrange(int32_t minalt, int32_t maxalt) const;
		virtual ptr_t simplify_aircrafttype(const std::string& acfttype) const;
		virtual ptr_t simplify_aircraftclass(const std::string& acftclass) const;
		virtual ptr_t simplify_equipment(const std::string& equipment, Aircraft::pbn_t pbn) const;
		virtual ptr_t simplify_typeofflight(char type_of_flight) const;
		virtual ptr_t simplify_mil(bool mil = false) const;
		virtual ptr_t simplify_gat(bool gat = true) const;
		virtual ptr_t simplify_dep(const std::string& ident, const Point& coord) const;
		virtual ptr_t simplify_dest(const std::string& ident, const Point& coord) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const;
		virtual CondResult evaluate_trace(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset, const TrafficFlowRule *rule, const Path *backptr = 0) const;
		virtual bool is_routestatic(void) const;
		virtual bool is_dct(void) const;
		virtual bool is_valid_dct(void) const;
		virtual DctParameters::AltRange evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct) const;
		virtual DctParameters::AltRange evaluate_dct_trace(TrafficFlowRestrictions& tfrs, const DctParameters& dct, const TrafficFlowRule *rule, const Path *backptr = 0) const;
		virtual std::vector<RuleResult::Alternative> get_mandatory(void) const;
		virtual std::vector<RuleResult::Alternative::Sequence> get_mandatory_or(void) const;
		virtual ptr_t extract_crossingpoints(std::vector<RuleWptAlt>& pts) const;
		virtual ptr_t extract_crossingsegments(std::vector<SegWptsAlt>& segs) const;
		virtual ptr_t extract_crossingairspaces(std::vector<airspacealt_t>& aspcs) const;
		virtual void save_binary(std::ostream& os) const;
		static ptr_t load_binary(BinLoader& ldr);
		unsigned int size(void) const { return m_cond.size(); }
		Condition::const_ptr_t operator[](unsigned int x) const { return m_cond[x].get_ptr(); }
		bool is_inv(void) const { return m_inv; }
		bool is_inv(unsigned int x) const { return m_cond[x].is_inv(); }

	protected:
		typedef std::vector<const_ptr_t> seq_t;
		class CondInv {
		public:
			CondInv(const_ptr_t p, bool inv) : m_ptr(p), m_inv(inv) {}
			const_ptr_t get_ptr(void) const { return m_ptr; }
			bool is_inv(void) const { return m_inv; }
			void set_ptr(const_ptr_t p) { m_ptr = p; }

		protected:
			const_ptr_t m_ptr;
			bool m_inv;
		};
		typedef std::vector<CondInv> cond_t;
		cond_t m_cond;
		bool m_inv;

		ptr_t simplify(const seq_t& seq) const;
	};

	class ConditionSequence : public Condition {
	public:
		ConditionSequence(unsigned int childnum);
		void add(const_ptr_t p);
		virtual bool is_refloc(void) const;
		virtual bool is_constant(void) const;
		virtual bool get_constvalue(void) const;
		virtual std::string to_str(void) const;
		virtual ptr_t clone(void) const;
		virtual ptr_t simplify(void) const;
		virtual ptr_t simplify_bbox(const Rect& bbox) const;
		virtual ptr_t simplify_altrange(int32_t minalt, int32_t maxalt) const;
		virtual ptr_t simplify_aircrafttype(const std::string& acfttype) const;
		virtual ptr_t simplify_aircraftclass(const std::string& acftclass) const;
		virtual ptr_t simplify_equipment(const std::string& equipment, Aircraft::pbn_t pbn) const;
		virtual ptr_t simplify_typeofflight(char type_of_flight) const;
		virtual ptr_t simplify_mil(bool mil = false) const;
		virtual ptr_t simplify_gat(bool gat = true) const;
		virtual ptr_t simplify_dep(const std::string& ident, const Point& coord) const;
		virtual ptr_t simplify_dest(const std::string& ident, const Point& coord) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const;
		virtual CondResult evaluate_trace(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset, const TrafficFlowRule *rule, const Path *backptr = 0) const;
		virtual bool is_routestatic(void) const;
		virtual std::vector<RuleResult::Alternative> get_mandatory_seq(void) const;
		virtual ptr_t extract_crossingpoints(std::vector<RuleWptAlt>& pts) const;
		virtual ptr_t extract_crossingsegments(std::vector<SegWptsAlt>& segs) const;
		virtual ptr_t extract_crossingairspaces(std::vector<airspacealt_t>& aspcs) const;
		virtual void save_binary(std::ostream& os) const;
		static ptr_t load_binary(BinLoader& ldr);
		unsigned int size(void) const { return m_seq.size(); }
		Condition::const_ptr_t operator[](unsigned int x) const { return m_seq[x]; }

	protected:
		typedef std::vector<const_ptr_t> seq_t;
		seq_t m_seq;

		ptr_t simplify(const seq_t& seq) const;
	};

	class ConditionConstant : public Condition {
	public:
		ConditionConstant(unsigned int childnum, bool val);
		virtual bool is_constant(void) const { return true; }
		virtual bool get_constvalue(void) const { return m_value; }
		virtual std::string to_str(void) const;
		virtual ptr_t clone(void) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const;
		virtual bool is_routestatic(void) const { return true; }
		virtual bool is_valid_dct(void) const { return true; }
		virtual DctParameters::AltRange evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct) const;
		virtual void save_binary(std::ostream& os) const;
		static ptr_t load_binary(BinLoader& ldr);

	protected:
		bool m_value;
	};

	class ConditionAltitude : public Condition {
	public:
		ConditionAltitude(unsigned int childnum, const AltRange& alt);
		virtual ptr_t simplify_altrange(int32_t minalt, int32_t maxalt) const;
		const AltRange& get_alt(void) const { return m_alt; }

	protected:
		AltRange m_alt;

		bool check_alt(int32_t alt) const;
		std::string to_altstr(void) const { return m_alt.to_str(); }
	};

	class ConditionCrossingAirspace1 : public ConditionAltitude {
	public:
		ConditionCrossingAirspace1(unsigned int childnum, const AltRange& alt, TFRAirspace::const_ptr_t aspc, bool refloc);
		virtual bool is_refloc(void) const { return m_refloc; }
		virtual std::string to_str(void) const;
		virtual ptr_t clone(void) const;
		virtual ptr_t simplify_bbox(const Rect& bbox) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const;
		virtual bool is_valid_dct(void) const { return true; }
		virtual DctParameters::AltRange evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct) const;
		virtual ptr_t extract_crossingairspaces(std::vector<airspacealt_t>& aspcs) const;
		virtual void save_binary(std::ostream& os) const;
		static ptr_t load_binary(BinLoader& ldr);
		TFRAirspace::const_ptr_t get_airspace(void) const { return m_airspace; }

	protected:
		TFRAirspace::const_ptr_t m_airspace;
		bool m_refloc;
	};

	class ConditionCrossingAirspace2 : public ConditionAltitude {
	public:
		ConditionCrossingAirspace2(unsigned int childnum, const AltRange& alt, TFRAirspace::const_ptr_t aspc0, TFRAirspace::const_ptr_t aspc1, bool refloc);
		virtual bool is_refloc(void) const { return m_refloc; }
		virtual std::string to_str(void) const;
		virtual ptr_t clone(void) const;
		virtual ptr_t simplify_bbox(const Rect& bbox) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const;
		virtual bool is_valid_dct(void) const { return true; }
		virtual DctParameters::AltRange evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct) const;
		virtual void save_binary(std::ostream& os) const;
		static ptr_t load_binary(BinLoader& ldr);
		static uint8_t rev8(uint8_t x);

	protected:
		static constexpr float fuzzy_dist = 0.1;
		static const unsigned int fuzzy_exponent = 2;
		TFRAirspace::const_ptr_t m_airspace0;
		TFRAirspace::const_ptr_t m_airspace1;
		bool m_refloc;
	};

	class ConditionCrossingDct : public ConditionAltitude {
	public:
		ConditionCrossingDct(unsigned int childnum, const AltRange& alt, const RuleWpt& wpt0, const RuleWpt& wpt1, bool refloc);
		virtual bool is_refloc(void) const { return m_refloc; }
		virtual std::string to_str(void) const;
		virtual ptr_t clone(void) const;
		virtual ptr_t simplify_bbox(const Rect& bbox) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const;
		virtual RuleResult::Alternative::Sequence get_resultsequence(void) const;
		virtual bool is_valid_dct(void) const { return true; }
		virtual DctParameters::AltRange evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct) const;
		virtual ptr_t extract_crossingsegments(std::vector<SegWptsAlt>& segs) const;
		virtual void save_binary(std::ostream& os) const;
		static ptr_t load_binary(BinLoader& ldr);
		const RuleWpt& get_wpt0(void) const { return m_wpt0; }
		const RuleWpt& get_wpt1(void) const { return m_wpt1; }

	protected:
		RuleWpt m_wpt0;
		RuleWpt m_wpt1;
		bool m_refloc;
	};

	class ConditionCrossingAirway : public ConditionCrossingDct {
	public:
		ConditionCrossingAirway(unsigned int childnum, const AltRange& alt, const RuleWpt& wpt0, const RuleWpt& wpt1, const std::string& awyname, bool refloc);
		virtual bool is_refloc(void) const { return m_refloc; }
		virtual std::string to_str(void) const;
		virtual ptr_t clone(void) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const;
		virtual RuleResult::Alternative::Sequence get_resultsequence(void) const;
		// FIXME: Airway crossing altitude!
		virtual ptr_t extract_crossingsegments(std::vector<SegWptsAlt>& segs) const;
		virtual void save_binary(std::ostream& os) const;
		static ptr_t load_binary(BinLoader& ldr);
		const std::string& get_awyname(void) const { return m_awyname; }

	protected:
		std::string m_awyname;
	};

	class ConditionCrossingPoint : public ConditionAltitude {
	public:
		ConditionCrossingPoint(unsigned int childnum, const AltRange& alt, const RuleWpt& wpt, bool refloc);
		virtual bool is_refloc(void) const { return m_refloc; }
		virtual std::string to_str(void) const;
		virtual ptr_t clone(void) const;
		virtual ptr_t simplify_bbox(const Rect& bbox) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const;
		virtual RuleResult::Alternative::Sequence get_resultsequence(void) const;
		virtual bool is_valid_dct(void) const { return true; }
		virtual DctParameters::AltRange evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct) const;
		virtual ptr_t extract_crossingpoints(std::vector<RuleWptAlt>& pts) const;
		virtual void save_binary(std::ostream& os) const;
		static ptr_t load_binary(BinLoader& ldr);
		const RuleWpt& get_wpt(void) const { return m_wpt; }

	protected:
		RuleWpt m_wpt;
		bool m_refloc;
	};

	class ConditionDepArr : public Condition {
	public:
		ConditionDepArr(unsigned int childnum, const RuleWpt& airport, bool arr, bool refloc);
		virtual bool is_refloc(void) const { return m_refloc; }
		virtual std::string to_str(void) const;
		virtual ptr_t clone(void) const;
		virtual ptr_t simplify_bbox(const Rect& bbox) const;
		virtual ptr_t simplify_dep(const std::string& ident, const Point& coord) const;
		virtual ptr_t simplify_dest(const std::string& ident, const Point& coord) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const;
		virtual bool is_routestatic(void) const { return true; }
		virtual void save_binary(std::ostream& os) const;
		static ptr_t load_binary(BinLoader& ldr);

	protected:
	        RuleWpt m_airport;
		bool m_arr;
		bool m_refloc;
	};

	class ConditionDepArrAirspace : public Condition {
	public:
		ConditionDepArrAirspace(unsigned int childnum, TFRAirspace::const_ptr_t aspc, bool arr, bool refloc);
		virtual bool is_refloc(void) const { return m_refloc; }
		virtual std::string to_str(void) const;
		virtual ptr_t clone(void) const;
		virtual ptr_t simplify_bbox(const Rect& bbox) const;
		virtual ptr_t simplify_dep(const std::string& ident, const Point& coord) const;
		virtual ptr_t simplify_dest(const std::string& ident, const Point& coord) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const;
		virtual void save_binary(std::ostream& os) const;
		static ptr_t load_binary(BinLoader& ldr);

	protected:
	        TFRAirspace::const_ptr_t m_airspace;
		bool m_arr;
		bool m_refloc;
	};

	class ConditionSidStar : public Condition {
	public:
		ConditionSidStar(unsigned int childnum, const RuleWpt& airport, const std::string& procid, bool star, bool refloc);
		virtual bool is_refloc(void) const { return m_refloc; }
		virtual std::string to_str(void) const;
		virtual ptr_t clone(void) const;
		virtual ptr_t simplify_dep(const std::string& ident, const Point& coord) const;
		virtual ptr_t simplify_dest(const std::string& ident, const Point& coord) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const;
		virtual void save_binary(std::ostream& os) const;
		static ptr_t load_binary(BinLoader& ldr);

	protected:
		RuleWpt m_airport;
		std::string m_procid;
		bool m_star;
		bool m_refloc;
	};

	class ConditionCrossingAirspaceActive : public Condition {
	public:
		ConditionCrossingAirspaceActive(unsigned int childnum, TFRAirspace::const_ptr_t aspc);
		virtual bool is_constant(void) const { return true; }
		virtual bool get_constvalue(void) const { return true; }
		virtual std::string to_str(void) const;
		virtual ptr_t clone(void) const;
		virtual ptr_t simplify(void) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const;
		virtual void save_binary(std::ostream& os) const;
		static ptr_t load_binary(BinLoader& ldr);

	protected:
		TFRAirspace::const_ptr_t m_airspace;
	};

	class ConditionCrossingAirwayAvailable : public ConditionAltitude {
	public:
		ConditionCrossingAirwayAvailable(unsigned int childnum, const AltRange& alt, const RuleWpt& wpt0, const RuleWpt& wpt1, const std::string& awyname);
		virtual bool is_constant(void) const { return true; }
		virtual bool get_constvalue(void) const { return false; }
		virtual std::string to_str(void) const;
		virtual ptr_t clone(void) const;
		virtual ptr_t simplify(void) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const;
		virtual void save_binary(std::ostream& os) const;
		static ptr_t load_binary(BinLoader& ldr);

	protected:
		RuleWpt m_wpt0;
		RuleWpt m_wpt1;
		std::string m_awyname;
	};

	class ConditionDctLimit : public Condition {
	public:
		ConditionDctLimit(unsigned int childnum, double limit);
		virtual std::string to_str(void) const;
		virtual ptr_t clone(void) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const;
		virtual bool is_dct(void) const { return true; }
		virtual bool is_valid_dct(void) const { return true; }
		virtual DctParameters::AltRange evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct) const;
		virtual void save_binary(std::ostream& os) const;
		static ptr_t load_binary(BinLoader& ldr);

	protected:
		double m_dctlimit;
	};

	class ConditionPropulsion : public Condition {
	public:
		typedef enum {
			kind_invalid    = 0,
			kind_any        = '*',
			kind_landplane  = 'L',
			kind_seaplane   = 'S',
			kind_amphibian  = 'A',
			kind_helicopter = 'H',
			kind_gyrocopter = 'G',
			kind_tiltwing   = 'T'
		} kind_t;

		typedef enum {
			propulsion_invalid  = 0,
			propulsion_piston    = 'P',
			propulsion_turboprop = 'T',
			propulsion_jet       = 'J'
		} propulsion_t;

		ConditionPropulsion(unsigned int childnum, kind_t kind = kind_invalid, propulsion_t prop = propulsion_invalid, unsigned int nrengines = 0);
		virtual std::string to_str(void) const;
		virtual ptr_t clone(void) const;
		virtual ptr_t simplify_aircraftclass(const std::string& acftclass) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const;
		virtual void save_binary(std::ostream& os) const;
		static ptr_t load_binary(BinLoader& ldr);

	protected:
		kind_t m_kind;
		propulsion_t m_propulsion;
		uint8_t m_nrengines;
	};

	class ConditionAircraftType : public Condition {
	public:
		ConditionAircraftType(unsigned int childnum, const std::string& type);
		virtual std::string to_str(void) const;
		virtual ptr_t clone(void) const;
		virtual ptr_t simplify_aircrafttype(const std::string& acfttype) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const;
		virtual bool is_routestatic(void) const { return true; }
		virtual void save_binary(std::ostream& os) const;
		static ptr_t load_binary(BinLoader& ldr);

	protected:
		std::string m_type;
	};

	class ConditionEquipment : public Condition {
	public:
		typedef enum {
			equipment_invalid      = 0,
			equipment_rnav         = 'R',
			equipment_rvsm         = 'W',
			equipment_833          = 'Y',
			equipment_mls          = 'K',
			equipment_ils          = 'L'
		} equipment_t;

		ConditionEquipment(unsigned int childnum, equipment_t e = equipment_invalid);
		virtual std::string to_str(void) const;
		virtual ptr_t clone(void) const;
		virtual ptr_t simplify_equipment(const std::string& equipment, Aircraft::pbn_t pbn) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const;
		virtual bool is_routestatic(void) const { return true; }
		virtual void save_binary(std::ostream& os) const;
		static ptr_t load_binary(BinLoader& ldr);

	protected:
	        equipment_t m_equipment;
	};

	class ConditionFlight : public Condition {
	public:
		typedef enum {
			flight_invalid      = 0,
			flight_scheduled    = 'S',
			flight_nonscheduled = 'N',
			flight_other        = 'O'
		} flight_t;

		ConditionFlight(unsigned int childnum, flight_t f = flight_invalid);
		virtual std::string to_str(void) const;
		virtual ptr_t clone(void) const;
		virtual ptr_t simplify_typeofflight(char type_of_flight) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const;
		virtual bool is_routestatic(void) const { return true; }
		virtual void save_binary(std::ostream& os) const;
		static ptr_t load_binary(BinLoader& ldr);

	protected:
	        flight_t m_flight;
	};

	class ConditionCivMil : public Condition {
	public:
		typedef enum {
			civmil_invalid    = 0,
			civmil_mil        = 'M',
			civmil_civ        = 'C',
		} civmil_t;

		ConditionCivMil(unsigned int childnum, civmil_t cm = civmil_invalid);
		virtual std::string to_str(void) const;
		virtual ptr_t clone(void) const;
		virtual ptr_t simplify_mil(bool mil = false) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const;
		virtual bool is_routestatic(void) const { return true; }
		virtual void save_binary(std::ostream& os) const;
		static ptr_t load_binary(BinLoader& ldr);

	protected:
	        civmil_t m_civmil;
	};

	class ConditionGeneralAviation : public Condition {
	public:
		ConditionGeneralAviation(unsigned int childnum);
		virtual std::string to_str(void) const;
		virtual ptr_t clone(void) const;
		virtual ptr_t simplify_gat(bool gat = true) const;
		virtual CondResult evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const;
		virtual bool is_routestatic(void) const { return true; }
		virtual void save_binary(std::ostream& os) const;
		static ptr_t load_binary(BinLoader& ldr);

	protected:
	};

	class TrafficFlowRule {
	public:
		typedef Glib::RefPtr<TrafficFlowRule> ptr_t;
		typedef Glib::RefPtr<const TrafficFlowRule> const_ptr_t;
		typedef enum {
			codetype_mandatory = RuleResult::codetype_mandatory,
			codetype_forbidden = RuleResult::codetype_forbidden,
			codetype_closed = RuleResult::codetype_closed,
			codetype_invalid
		} codetype_t;

		TrafficFlowRule(void);
		void reference(void) const;
		void unreference(void) const;
		bool is_valid(void) const;
		codetype_t get_codetype(void) const { return m_codetype; }
		static char get_codetype_char(codetype_t t);
		char get_codetype_char(void) const { return get_codetype_char(get_codetype()); }
		uint64_t get_mid(void) const { return m_mid; }
		const std::string& get_codeid(void) const { return m_codeid; }
		const std::string& get_oprgoal(void) const { return m_oprgoal; }
		const std::string& get_descr(void) const { return m_descr; }
		Timesheet& get_timesheet(void)  { return m_timesheet; }
		const Timesheet& get_timesheet(void) const { return m_timesheet; }
		Restrictions& get_restrictions(void)  { return m_restrictions; }
		const Restrictions& get_restrictions(void) const { return m_restrictions; }
		Condition::ptr_t get_condition(void) { return m_condition; }
		Condition::const_ptr_t get_condition(void) const { return m_condition; }

		void set_codetype(codetype_t x) { m_codetype = x; }
		void set_codetype(char x);
		void set_codetype(const std::string& x);
		void set_mid(unsigned long x) { m_mid = x; }
		void set_codeid(const std::string& x) { m_codeid = x; }
		void set_oprgoal(const std::string& x) { m_oprgoal = x; }
		void set_descr(const std::string& x) { m_descr = x; }
		void set_timesheet(const Timesheet& t) { m_timesheet = t; }
		void add_restriction(const RestrictionSequence& e) { m_restrictions.add(e); }
		void add_restrictions(const Restrictions& e) { m_restrictions.add(e); }
		void set_condition(Condition::ptr_t x) { m_condition = x; }

		void clone(void);
		bool is_keep(void) const;
		ptr_t simplify(void) const;
		ptr_t simplify_bbox(const Rect& bbox) const;
		ptr_t simplify_altrange(int32_t minalt, int32_t maxalt) const;
		ptr_t simplify_aircrafttype(const std::string& acfttype) const;
		ptr_t simplify_aircraftclass(const std::string& acftclass) const;
		ptr_t simplify_equipment(const std::string& equipment, Aircraft::pbn_t pbn) const;
		ptr_t simplify_typeofflight(char type_of_flight) const;
		ptr_t simplify_mil(bool mil = false) const;
		ptr_t simplify_gat(bool gat = true) const;
		ptr_t simplify_dep(const std::string& ident, const Point& coord) const;
		ptr_t simplify_dest(const std::string& ident, const Point& coord) const;
		ptr_t simplify_complexity(void) const;
		bool evaluate(TrafficFlowRestrictions& tfrs, Result& result) const;
		bool evaluate(TrafficFlowRestrictions& tfrs, std::vector<RuleResult>& result) const;
		bool is_dct(void) const;
		bool is_unconditional(void) const;
		bool is_routestatic(void) const;
		bool is_mandatoryinbound(void) const;
		DctParameters::AltRange evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct, bool trace = false) const;
		void get_dct_segments(DctSegments& seg) const;
		RuleResult get_rule(void) const;
		void save_binary(std::ostream& os) const;
		void load_binary(BinLoader& ldr);

	protected:
		Timesheet m_timesheet;
		Restrictions m_restrictions;
		Condition::ptr_t m_condition;
		std::string m_codeid;
		std::string m_oprgoal;
		std::string m_descr;
		uint64_t m_mid;
                mutable gint m_refcount;
		codetype_t m_codetype;

		bool is_unconditional_airspace(void) const;
		ptr_t simplify_complexity_crossingpoints(void) const;
		ptr_t simplify_complexity_crossingsegments(void) const;
		ptr_t simplify_complexity_closedairspace(void) const;
	};

	class TrafficFlowRules : public std::vector<TrafficFlowRule::ptr_t> {
	public:
		void clone(void);
		void simplify(void);
		void simplify_bbox(const Rect& bbox);
		void simplify_altrange(int32_t minalt, int32_t maxalt);
		void simplify_aircrafttype(const std::string& acfttype);
		void simplify_aircraftclass(const std::string& acftclass);
		void simplify_equipment(const std::string& equipment, Aircraft::pbn_t pbn);
		void simplify_typeofflight(char type_of_flight);
		void simplify_mil(bool mil = false);
		void simplify_gat(bool gat = true);
		void simplify_dep(const std::string& ident, const Point& coord);
		void simplify_dest(const std::string& ident, const Point& coord);
		void simplify_complexity(void);
		bool evaluate(TrafficFlowRestrictions& tfrs, Result& result) const;
		bool evaluate(TrafficFlowRestrictions& tfrs, std::vector<RuleResult>& result) const;
		DctParameters::AltRange evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct, bool trace = false) const;
		void get_dct_segments(DctSegments& seg) const;
		std::vector<RuleResult> find_rules(const std::string& name, bool exact = true) const;
		void save_binary(std::ostream& os) const;
		void load_binary(BinLoader& ldr);
	};

	class CompareAirspaces {
	public:
		bool operator()(const AirspacesDb::Airspace& a, const AirspacesDb::Airspace& b);
	};

	Fpl m_route;
	AirportsDb *m_airportdb;
	NavaidsDb *m_navaiddb;
	WaypointsDb *m_waypointdb;
	AirwaysDb *m_airwaydb;
	AirspacesDb *m_airspacedb;
	Graph m_graph;
	Rect m_graphbbox;
	TrafficFlowRules m_loadedtfr;
	TrafficFlowRules m_tfr;
	TrafficFlowRules m_dcttfr;
	typedef std::set<AirspacesDb::Airspace, CompareAirspaces> airspacecache_t;
	airspacecache_t m_airspacecache;
	tracerules_t m_tracerules;
	disabledrules_t m_disabledrules;
	typedef std::vector<RuleResult> rules_t;
	rules_t m_rules;
	typedef std::vector<Message> messages_t;
	messages_t m_messages;
	std::string m_aircrafttype;
	std::string m_aircrafttypeclass;
	std::string m_equipment;
	std::string m_origin;
	std::string m_created;
	std::string m_effective;
	Aircraft::pbn_t m_pbn;
	char m_typeofflight;
	bool m_shortcircuit;

	bool check_fplan_depdest(void);
	bool check_fplan_rules(void);
	bool check_fplan_specialcase(void);
	bool check_fplan_specialcase_pogo(void);
	void load_airway_graph(void);
	void load_disabled_trace_rules_xml(const xmlpp::Element *el);
	void save_disabled_trace_rules_xml(xmlpp::Element *el) const;

	static const char binfile_signature_v1[];
	static const char binfile_signature_v2[];

	static uint8_t loadbinu8(std::istream& is);
	static uint16_t loadbinu16(std::istream& is);
	static uint32_t loadbinu32(std::istream& is);
	static uint64_t loadbinu64(std::istream& is);
	static double loadbindbl(std::istream& is);
	static std::string loadbinstring(std::istream& is);
	static Point loadbinpt(std::istream& is);
	static void savebinu8(std::ostream& os, uint8_t v);
	static void savebinu16(std::ostream& os, uint16_t v);
	static void savebinu32(std::ostream& os, uint32_t v);
	static void savebinu64(std::ostream& os, uint64_t v);
	static void savebindbl(std::ostream& os, double v);
	static void savebinstring(std::ostream& os, const std::string& v);
	static void savebinpt(std::ostream& os, const Point& v);
};

#endif /* TFR_H */
