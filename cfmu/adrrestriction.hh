#ifndef ADRRESTRICTION_H
#define ADRRESTRICTION_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <boost/config.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/logic/tribool_io.hpp>

#include "adr.hh"
#include "adrfplan.hh"
#include "interval.hh"
#include "engine.h"

namespace ADR {

class FlightRestriction;

class Message {
public:
	typedef std::set<unsigned int> set_t;
	typedef enum {
		type_error,
		type_warning,
		type_info,
		type_tracecond,
		type_tracetfe,
		type_trace
	} type_t;

	Message(const std::string& text, type_t t, timetype_t tm = 0, const Object::const_ptr_t& p = Object::const_ptr_t());
	const set_t& get_vertexset(void) const { return m_vertexset; }
	const set_t& get_edgeset(void) const { return m_edgeset; }
	const std::string& get_text(void) const { return m_text; }
	const Object::const_ptr_t& get_obj(void) const { return m_obj; }
	uint64_t get_time(void) const { return m_time; }
	const std::string& get_ident(void) const;
	const std::string& get_rule(void) const;
	type_t get_type(void) const { return m_type; }
	static const std::string& get_type_string(type_t t);
	const std::string& get_type_string(void) const { return get_type_string(get_type()); }
	static char get_type_char(type_t t);
	char get_type_char(void) const { return get_type_char(get_type()); }
	std::string to_str(void) const;

protected:
	set_t m_vertexset;
	set_t m_edgeset;
	Object::const_ptr_t m_obj;
	std::string m_text;
	timetype_t m_time;
	type_t m_type;

	friend class RestrictionEval;
	friend class RestrictionElement;
	friend class RestrictionSequence;
	friend class Restrictions;
	friend class Condition;
	friend class FlightRestrictionTimeSlice;
	void add_vertex(unsigned int nr);
	void add_edge(unsigned int nr);
	void insert_vertex(unsigned int nr);
};

class RestrictionElement;

class SegmentsListTimeSlice {
public:
	typedef std::vector<RouteSegmentLink> segments_t;

	SegmentsListTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0, const segments_t& segs = segments_t());
	timetype_t get_starttime(void) const { return m_time.first; }
	timetype_t get_endtime(void) const { return m_time.second; }
	const timepair_t& get_timeinterval(void) const { return m_time; }
	void set_starttime(timetype_t t) { m_time.first = t; }
	void set_endtime(timetype_t t) { m_time.second = t; }
	bool is_valid(void) const { return get_starttime() < get_endtime(); }
	bool is_inside(timetype_t tm) const { return get_starttime() <= tm && tm < get_endtime(); }
	bool is_overlap(timetype_t tm0, timetype_t tm1) const;
	bool is_overlap(const TimeSlice& ts) const { return is_overlap(ts.get_starttime(), ts.get_endtime()); }
	bool is_overlap(const timepair_t& tm) const { return is_overlap(tm.first, tm.second); }
	timetype_t get_overlap(timetype_t tm0, timetype_t tm1) const;
	timetype_t get_overlap(const TimeSlice& ts) const { return get_overlap(ts.get_starttime(), ts.get_endtime()); }
	timetype_t get_overlap(const timepair_t& tm) const { return get_overlap(tm.first, tm.second); }
	const segments_t& get_segments(void) const { return m_segments; }
	segments_t& get_segments(void) { return m_segments; }
	void set_segments(const segments_t& s) { m_segments = s; }
	Rect get_bbox(void) const;
	void recompute(Database& db, const Link& start, const Link& end, const Link& awy);
	int compare(const SegmentsListTimeSlice& x) const;
	bool operator==(const SegmentsListTimeSlice& x) const { return compare(x) == 0; }
	bool operator!=(const SegmentsListTimeSlice& x) const { return compare(x) != 0; }
	bool operator<(const SegmentsListTimeSlice& x) const { return compare(x) < 0; }
	bool operator<=(const SegmentsListTimeSlice& x) const { return compare(x) <= 0; }
	bool operator>(const SegmentsListTimeSlice& x) const { return compare(x) > 0; }
	bool operator>=(const SegmentsListTimeSlice& x) const { return compare(x) >= 0; }
	int compare_all(const SegmentsListTimeSlice& x) const;
	std::ostream& print(std::ostream& os, unsigned int indent) const;

	template<class Archive> void hibernate(Archive& ar) {
		ar.iouint64(m_time.first);
		ar.iouint64(m_time.second);
		{
			uint32_t sz(m_segments.size());
			ar.ioleb(sz);
			if (ar.is_load())
				m_segments.resize(sz);
			for (typename segments_t::iterator i(m_segments.begin()), e(m_segments.end()); i != e; ++i)
				i->hibernate(ar);
		}
	}

protected:
	timepair_t m_time;
	segments_t m_segments;
};

class SegmentsList : public std::set<SegmentsListTimeSlice> {
public:
	int compare(const SegmentsList& x) const;
	Rect get_bbox(const timepair_t& tm) const;
	void recompute(Database& db, const timepair_t& tm, const Link& start, const Link& end, const Link& awy);
	std::ostream& print(std::ostream& os, unsigned int indent) const;

	template<class Archive> void hibernate(Archive& ar) {
		uint32_t sz(size());
		ar.ioleb(sz);
		if (ar.is_load()) {
			clear();
			for (; sz; --sz) {
				SegmentsListTimeSlice seg;
				seg.hibernate(ar);
				insert(seg);
			}
		} else {
			for (iterator i(begin()), e(end()); i != e && sz > 0; ++i, --sz)
				const_cast<SegmentsListTimeSlice&>(*i).hibernate(ar);
		}
	}
};

class RuleSegment {
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

	RuleSegment(type_t t = type_invalid, const AltRange& alt = AltRange(),
		    const Object::const_ptr_t& wpt0 = Object::const_ptr_t(),
		    const Object::const_ptr_t& wpt1 = Object::const_ptr_t(),
		    const Object::const_ptr_t& awy = Object::const_ptr_t(),
		    const SegmentsList& segs = SegmentsList());
	const Object::const_ptr_t& get_wpt0(void) const { return m_wpt[0]; }
	const Object::const_ptr_t& get_wpt1(void) const { return m_wpt[1]; }
	const Object::const_ptr_t& get_wpt(unsigned int i) const { return m_wpt[!!i]; }
	const UUID& get_uuid0(void) const { return get_wpt0() ? get_wpt0()->get_uuid() : UUID::niluuid; }
	const UUID& get_uuid1(void) const { return get_wpt1() ? get_wpt1()->get_uuid() : UUID::niluuid; }
	const UUID& get_uuid(unsigned int i) const { return get_wpt(i) ? get_wpt(i)->get_uuid() : UUID::niluuid; }
	const AltRange& get_alt(void) const { return m_alt; }
	AltRange& get_alt(void) { return m_alt; }
	const Object::const_ptr_t& get_airway(void) const { return m_airway; }
	const UUID& get_airway_uuid(void) const { return get_airway() ? get_airway()->get_uuid() : UUID::niluuid; }
	const SegmentsList& get_segments(void) const { return m_segments; }
	type_t get_type(void) const { return m_type; }
	bool is_airway(void) const { return get_type() == type_airway; }
	bool is_dct(void) const { return get_type() == type_dct; }
	bool is_point(void) const { return get_type() == type_point; }
	bool is_sid(void) const { return get_type() == type_sid; }
	bool is_star(void) const { return get_type() == type_star; }
	bool is_airspace(void) const { return get_type() == type_airspace; }
	bool is_invalid(void) const { return get_type() == type_invalid; }
	bool is_valid(void) const { return get_type() != type_invalid; }
	Glib::RefPtr<RestrictionElement> get_restrictionelement(void) const;
	std::string to_shortstr(timetype_t tm) const;

protected:
	Object::const_ptr_t m_wpt[2];
	Object::const_ptr_t m_airway;
	AltRange m_alt;
	SegmentsList m_segments;
	type_t m_type;
};

class RuleSequence : public std::vector<RuleSegment> {
public:
	std::string to_shortstr(timetype_t tm) const;
};

class RestrictionSequenceResult {
public:
	typedef std::set<unsigned int> set_t;

	RestrictionSequenceResult(const RuleSequence& seq, const set_t& vs = set_t(), const set_t& es = set_t())
		: m_seq(seq), m_vertexset(vs), m_edgeset(es) {}
	const RuleSequence& get_sequence(void) const { return m_seq; }
	const set_t& get_vertexset(void) const { return m_vertexset; }
	const set_t& get_edgeset(void) const { return m_edgeset; }

protected:
	RuleSequence m_seq;
	set_t m_vertexset;
	set_t m_edgeset;

	friend class RestrictionEval;
};

class RestrictionResult : public std::vector<RestrictionSequenceResult> {
public:
	typedef vector<RestrictionSequenceResult> base_t;
	typedef RestrictionSequenceResult::set_t set_t;

	RestrictionResult(const Object::const_ptr_t& rule = Object::const_ptr_t(),
			  timetype_t tm = 0, const set_t& vs = set_t(), const set_t& es = set_t(),
			  unsigned int refloc = ~0U)
		: m_rule(rule), m_time(tm), m_vertexset(vs), m_edgeset(es), m_refloc(refloc) {}
	const Object::const_ptr_t& get_rule(void) const { return m_rule; }
	timetype_t get_time(void) const { return m_time; }
	const FlightRestrictionTimeSlice& get_timeslice(void) const {
		return get_rule()->operator()(get_time()).as_flightrestriction();
	}
	const set_t& get_vertexset(void) const { return m_vertexset; }
	const set_t& get_edgeset(void) const { return m_edgeset; }
	bool has_refloc(void) const { return m_refloc != ~0U; }
	unsigned int get_refloc(void) const { return m_refloc; }

protected:
	Object::const_ptr_t m_rule;
	timetype_t m_time;
	set_t m_vertexset;
	set_t m_edgeset;
	unsigned int m_refloc;

	friend class RestrictionEval;
};

class RestrictionResults : public std::vector<RestrictionResult> {
public:
	bool is_ok(void) const;
};

class RestrictionEvalMessages {
public:
	RestrictionEvalMessages(void);
	RestrictionEvalMessages(const RestrictionEvalMessages& x);
	RestrictionEvalMessages& operator=(const RestrictionEvalMessages& x);

	void message(const std::string& text, Message::type_t t, timetype_t tm);
	void message(const Message& msg);

	typedef std::vector<Message> messages_t;
	typedef messages_t::const_iterator messages_const_iterator;
	const messages_t& get_messages(void) const { return m_messages; }
	messages_t& get_messages(void) { return m_messages; }
	messages_const_iterator messages_begin(void) const { return m_messages.begin(); }
	messages_const_iterator messages_end(void) const { return m_messages.end(); }
	void clear_messages(void) { m_messages.clear(); }

protected:
	messages_t m_messages;
	Object::ptr_t m_currule;
};

class RestrictionEvalBase : public RestrictionEvalMessages {
public:
	RestrictionEvalBase(Database *db = 0);
	RestrictionEvalBase(const RestrictionEvalBase& x);
	~RestrictionEvalBase(void);
	RestrictionEvalBase& operator=(const RestrictionEvalBase& x);

	Database& get_dbref(void) const;
	Database *get_db(void) const { return m_db; }
	void set_db(Database *db = 0) { m_db = db; }

	typedef std::vector<Glib::RefPtr<FlightRestriction> > rules_t;

	void load_rules(void);
	unsigned int count_rules(void) const { return m_allrules.size(); }
	rules_t& get_rules(void) { return m_allrules; }
	const rules_t& get_rules(void) const { return m_allrules; }

	const Object::ptr_t& get_currule(void) const { return m_currule; }

protected:
	TimeTableSpecialDateEval m_specialdateeval;
	rules_t m_allrules;
	Database *m_db;
};

class RestrictionEval : public RestrictionEvalBase {
protected:
	typedef enum {
		graphmode_foreign,
		graphmode_allocated,
		graphmode_loaded
	} graphmode_t;

public:
	class Waypoint : public FPLWaypoint {
	public:
		Waypoint(const FPLWaypoint& w);
		Graph::vertex_descriptor get_vertex(void) const { return m_vertex; }
		void set_vertex(Graph::vertex_descriptor v) { m_vertex = v; }
		Graph::edge_descriptor get_edge(void) const { return m_edge; }
		void set_edge(Graph::edge_descriptor e) { m_edge = e; }

	protected:
		Graph::vertex_descriptor m_vertex;
		Graph::edge_descriptor m_edge;
	};

	RestrictionEval(Database *db = 0, Graph *graph = 0);
	RestrictionEval(const RestrictionEval& x);
	~RestrictionEval(void);
	RestrictionEval& operator=(const RestrictionEval& x);

	const TimeTableSpecialDateEval& get_specialdateeval(void) const { return m_specialdateeval; }
	const ConditionalAvailability& get_condavail(void) const { return m_condavail; }
	FlightPlan get_fplan(void) const;
	void set_fplan(const FlightPlan& fpl);
	Graph& get_graphref(void);
	Graph *get_graph(void) const { return m_graph; }
	void set_graph(Graph *g = 0);

	using RestrictionEvalBase::message;
	void message(const std::string& text = "", Message::type_t t = Message::type_error);

	const std::string& get_aircrafttype(void) const { return m_fplan.get_aircrafttype(); }
	const std::string& get_equipment(void) const { return m_fplan.get_equipment(); }
	Aircraft::pbn_t get_pbn(void) const { return m_fplan.get_pbn(); }
	char get_flighttype(void) const { return m_fplan.get_flighttype(); }
	uint64_t get_departuretime(void) const { return m_fplan.get_departuretime(); }
	unsigned int wpt_size(void) const { return m_waypoints.size(); }
	Waypoint& wpt(unsigned int idx) { return m_waypoints[idx]; }
	const Waypoint& wpt(unsigned int idx) const { return m_waypoints[idx]; }

	typedef RestrictionResults::const_iterator results_const_iterator;
	const RestrictionResults& get_results(void) const { return m_results; }
	RestrictionResults& get_results(void) { return m_results; }
	results_const_iterator results_begin(void) const { return m_results.begin(); }
	results_const_iterator results_end(void) const { return m_results.end(); }
	void clear_results(void) { m_results.clear(); }

	void load_rules(void);
	void load_aup(AUPDatabase& aupdb, timetype_t starttime, timetype_t endtime);
	bool check_fplan(bool honourprofilerules);

	unsigned int count_srules(void) const { return m_rules.size(); }
	rules_t& get_srules(void) { return m_rules; }
	const rules_t& get_srules(void) const { return m_rules; }
	void reset_rules(void);
	bool disable_rule(const std::string& id);
	bool trace_rule(const std::string& id);
	void simplify_rules(void);
	void simplify_rules_time(timetype_t tm0, timetype_t tm1);
	void simplify_rules_bbox(const Rect& bbox);
	void simplify_rules_altrange(int32_t minalt, int32_t maxalt);
	void simplify_rules_aircrafttype(const std::string& acfttype);
	void simplify_rules_aircraftclass(const std::string& acftclass);
	void simplify_rules_equipment(const std::string& equipment, Aircraft::pbn_t pbn);
	void simplify_rules_typeofflight(char type_of_flight);
	void simplify_rules_mil(bool mil = false);
	void simplify_rules_dep(const Link& arpt);
	void simplify_rules_dest(const Link& arpt);
	void simplify_conditionalavailability(const Graph& g, const timepair_t& tm);

protected:
	ConditionalAvailability m_condavail;
	RestrictionResults m_results;
	rules_t m_rules;
	FlightPlan m_fplan;
	typedef std::vector<Waypoint> waypoints_t;
	waypoints_t m_waypoints;
	Graph *m_graph;
	graphmode_t m_graphmode;

	void graph_add(const Object::ptr_t& p);
	void graph_add(const Database::findresults_t& r);
};

class DctSegments {
public:
	class DctSegment {
	public:
		DctSegment(const Object::const_ptr_t& pt0 = Object::const_ptr_t(),
			   const Object::const_ptr_t& pt1 = Object::const_ptr_t());

		Object::const_ptr_t& operator[](unsigned int x) { return m_pt[!!x]; }
		const Object::const_ptr_t& operator[](unsigned int x) const { return m_pt[!!x]; }
		const UUID& get_uuid(unsigned int x) const;
		int compare(const DctSegment& x) const;
		bool operator<(const DctSegment& x) const { return compare(x) < 0; }
		bool operator>(const DctSegment& x) const { return compare(x) > 0; }
		bool operator<=(const DctSegment& x) const { return compare(x) <= 0; }
		bool operator>=(const DctSegment& x) const { return compare(x) >= 0; }
		bool operator==(const DctSegment& x) const { return compare(x) == 0; }
		bool operator!=(const DctSegment& x) const { return compare(x) != 0; }
		std::pair<double,double> dist(void) const;

	protected:
		Object::const_ptr_t m_pt[2];
	};

	DctSegments(void) {}

	void clear(void) { m_dctseg.clear(); }
	bool empty(void) const { return m_dctseg.empty(); }
	unsigned int size(void) const { return m_dctseg.size(); }
	void add(const DctSegment& x) { m_dctseg.insert(x); }
	bool contains(const DctSegment& x) const { return m_dctseg.find(x) != m_dctseg.end(); }
	bool contains(const Object::const_ptr_t& pt0, const Object::const_ptr_t& pt1) const { return contains(DctSegment(pt0, pt1)); }

protected:
	typedef std::set<DctSegment> dctseg_t;

public:
	typedef dctseg_t::const_iterator const_iterator;
	const_iterator begin(void) const { return m_dctseg.begin(); }
	const_iterator end(void) const { return m_dctseg.end(); }

protected:
	dctseg_t m_dctseg;
};

class DctParameters : public RestrictionEvalBase {
public:
	DctParameters(Database *db = 0, TopoDb30 *topodb = 0, const std::string& topodbpath = "", timetype_t tmmodif = 0,
		      timetype_t tmcutoff = 0, timetype_t tmfuturecutoff = std::numeric_limits<timetype_t>::max(),
		      double maxdist = 50, unsigned int worker = 0, bool all = false, bool verbose = true);
	const TimeTableSpecialDateEval& get_specialdateeval(void) const { return m_specialdateeval; }
	DctParameters& operator&=(const BidirAltRange& x) { m_alt &= x; return *this; }
	DctParameters& operator|=(const BidirAltRange& x) { m_alt |= x; return *this; }
	DctParameters& operator^=(const BidirAltRange& x) { m_alt ^= x; return *this; }
	DctParameters& operator-=(const BidirAltRange& x) { m_alt -= x; return *this; }

	void load_rules(void);
	void load_points(void);
	void load_points(const Database::findresults_t& r) { load_points(r, false); }
	void run(void);
	const DctSegments& get_dctseg(void) const { return m_seg; }
	unsigned int get_errorcnt(void) const { return m_errorcnt; }
	unsigned int get_warncnt(void) const { return m_warncnt; }
	timetype_t get_tmmodified(void) const { return m_tmmodified; }
	timetype_t get_tmcutoff(void) const { return m_tmcutoff; }
	timetype_t get_tmfuturecutoff(void) const { return m_tmfuturecutoff; }

	class Calc : public RestrictionEvalMessages {
	public:
		Calc(const DctParameters& param, const Object::ptr_t& p0, const Object::ptr_t& p1, const timeset_t& tdisc, bool all, bool verbose, bool trace);
		static const BidirAltRange::set_t& get_alt(unsigned int index) { return defaultalt[!!index]; }
		static const BidirAltRange& get_alt(void) { return defaultalt; }
		const DctParameters& get_param(void) const { return *m_param; }
		timetype_t get_time(void) const { return m_tm; }
		timetype_t get_endtime(void) const { return m_tmend; }
		DctLeg& get_leg(void) { return m_leg; }
		const DctLeg& get_leg(void) const { return m_leg; }
		const Object::ptr_t& get_point(unsigned int index) const { return m_leg.get_point(index).get_obj(); }
		const UUID& get_uuid(unsigned int index) const;
		const Point& get_coord(unsigned int index) const;
		const std::string& get_ident(unsigned int index) const;
		const TimeTableEval get_tte(unsigned int index) const;
		const TimeTableEval get_tte(unsigned int index, float crs, float dist) const;
		bool is_airport(unsigned int index) const;
		bool is_airport(void) const;
		double get_dist(void) const { return m_dist; }

		void run(void);
		void run_topo(TopoDb30& topodb);

	protected:
		static const BidirAltRange defaultalt;
		static constexpr double airway_preferred_factor = 1.02;

		const DctParameters *m_param;
		DctLeg m_leg;                // current DCT segment
		timeset_t m_tdisc;
		timetype_t m_tm;             // start time of current interval
		timetype_t m_tmend;          // end time of current interval
		double m_dist;
		bool m_all;
		bool m_verbose;
		bool m_trace;

		void update_altset(DctLeg::altset_t& altset, const BidirAltRange& rdct, const FlightRestrictionTimeSlice& ts, bool altor);
		DctLeg::altset_t run_dct_time(void);
	};

protected:
	class AirportDctLimit {
	public:
		typedef std::map<Link,IntervalSet<int32_t> > dctconnpoints_t;

		AirportDctLimit(const Link& arpt = Link(), timetype_t starttime = 0, timetype_t endtime = 0,
				double lim = 0, const dctconnpoints_t& connpt = dctconnpoints_t());
		int compare(const AirportDctLimit& x) const;
		bool operator<(const AirportDctLimit& x) const { return compare(x) < 0; }
		bool operator>(const AirportDctLimit& x) const { return compare(x) > 0; }
		bool operator<=(const AirportDctLimit& x) const { return compare(x) <= 0; }
		bool operator>=(const AirportDctLimit& x) const { return compare(x) >= 0; }
		bool operator==(const AirportDctLimit& x) const { return compare(x) == 0; }
		bool operator!=(const AirportDctLimit& x) const { return compare(x) != 0; }
		const dctconnpoints_t& get_connpt(void) const { return m_connpt; }
		const Link& get_arpt(void) const { return m_arpt; }
		double get_limit(void) const { return m_limit; }
		timetype_t get_starttime(void) const { return m_time.first; }
		timetype_t get_endtime(void) const { return m_time.second; }
		const timepair_t& get_timeinterval(void) const { return m_time; }
		bool is_valid(void) const { return get_starttime() < get_endtime(); }
		bool is_inside(timetype_t tm) const { return get_starttime() <= tm && tm < get_endtime(); }
		bool is_overlap(timetype_t tm0, timetype_t tm1) const;
		bool is_overlap(const TimeSlice& ts) const { return is_overlap(ts.get_starttime(), ts.get_endtime()); }
		bool is_overlap(const timepair_t& tm) const { return is_overlap(tm.first, tm.second); }
		IntervalSet<int32_t> get_altinterval(const Link& pt, timetype_t tm) const;

	protected:
		dctconnpoints_t m_connpt;
		Link m_arpt;
		timepair_t m_time;
		double m_limit;
	};

	class CalcMT : public Calc {
	public:
		CalcMT(const DctParameters& param, const Object::ptr_t& p0, const Object::ptr_t& p1, const timeset_t& tdisc, bool all, bool verbose, bool trace);
		CalcMT(const Calc& x);
		void run(void);
		void join(void);
		bool is_done(void) const { return g_atomic_int_get(&m_done); }

	protected:
		Glib::Threads::Thread *m_thread;
		gint m_done;

		void do_run(void);
		void set_done(void) { g_atomic_int_set(&m_done, 1); }
	};

	TopoDb30 *m_topodb;
	std::string m_topodbpath;
	Graph m_graph;
	DctSegments m_seg;
	timeset_t m_routetimedisc;
	timeset_t m_ruletimedisc;
	MultiPolygonHole m_ecacpoly;
	Rect m_ecacbbox;
	typedef std::vector<Object::ptr_t> points_t;
	points_t m_points;
	typedef std::vector<double> dctradius_t;
	dctradius_t m_dctradius;
	typedef std::set<AirportDctLimit> airportdctlimit_t;
	airportdctlimit_t m_sidlimit;
	airportdctlimit_t m_starlimit;
	BidirAltRange m_alt;
	mutable Glib::Threads::Mutex m_mutex;
	mutable Glib::Threads::Cond m_cond;
	typedef std::list<Calc> results_t;
	mutable results_t m_results;
	timetype_t m_tmmodified;     // dct recomputed for rules that changed not before this time
	timetype_t m_tmcutoff;       // we are not interested in dct edges for times earlier than this time
	timetype_t m_tmfuturecutoff; // we are not interested in dct edges for times later than this time
	double m_maxdist;            // maximum DCT edge to consider (except DCT edges mentioned in rules)
	unsigned int m_errorcnt;
	unsigned int m_warncnt;
	gint m_pptot;
	mutable gint m_ppcnt;
	unsigned int m_dctcnt;
	unsigned int m_worker;
	mutable unsigned int m_resultscnt;
	mutable unsigned int m_finished;
	bool m_all;
	bool m_verbose;
	bool m_trace;

	void load_points(const Database::findresults_t& r, bool alldct);
	const airportdctlimit_t& get_sidlimit(void) const { return m_sidlimit; }
	const airportdctlimit_t& get_starlimit(void) const { return m_starlimit; }
	const Graph& get_graph(void) const { return m_graph; }
	const rules_t& get_allrules(void) const { return m_allrules; }
	const DctSegments get_seg(void) const { return m_seg; }
	const timeset_t& get_routetimedisc(void) const { return m_routetimedisc; }
	double get_maxdist(void) const { return m_maxdist; }
	void run_threaded(points_t::size_type i0 = 0, points_t::size_type istride = 1, TopoDb30 *topodb = 0) const;
	void result(results_t& x) const;
	void finished(void) const;

	Glib::Threads::Mutex& get_mutex(void) const { return m_mutex; }
	void signal(void) const;
	bool finishone(std::list<CalcMT>& calc, bool saveempty);
};

class CondResult {
public:
	typedef RestrictionResult::set_t set_t;

	CondResult(boost::logic::tribool result = boost::logic::indeterminate, bool xngedgeinv = false)
		: m_refloc(~0U), m_result(result), m_xngedgeinv(xngedgeinv) {}
	boost::logic::tribool get_result(void) const { return m_result; }
	void set_result(boost::logic::tribool result = boost::logic::indeterminate) { m_result = result; }
	set_t& get_vertexset(void) { return m_vertexset; }
	const set_t& get_vertexset(void) const { return m_vertexset; }
	set_t& get_edgeset(void) { return m_edgeset; }
	const set_t& get_edgeset(void) const { return m_edgeset; }
	set_t& get_xngedgeset(void) { return m_xngedgeset; }
	const set_t& get_xngedgeset(void) const { return m_xngedgeset; }
	bool is_xngedgeinv(void) const { return m_xngedgeinv; }
	bool has_refloc(void) const { return m_refloc != ~0U; }
	unsigned int get_refloc(void) const { return m_refloc; }
	bool set_refloc(unsigned int rl);
	void clear_refloc(void) { m_refloc = ~0U; }
	unsigned int get_first(void) const;
	unsigned int get_last(void) const;
	unsigned int get_seqorder(unsigned int min = 0U) const;
	CondResult& operator&=(const CondResult& x);
	CondResult& operator|=(const CondResult& x);
	CondResult operator~(void);
	CondResult operator&(const CondResult& x) { CondResult r(*this); r &= x; return r; }
	CondResult operator|(const CondResult& x) { CondResult r(*this); r |= x; return r; }

protected:
	set_t m_vertexset;
	set_t m_edgeset;
	set_t m_xngedgeset; // crossing edges, needed to handle DCT Limits
	unsigned int m_refloc;
	boost::logic::tribool m_result;
	bool m_xngedgeinv;
};

class RestrictionElement {
public:
	typedef Glib::RefPtr<RestrictionElement> ptr_t;
	typedef Glib::RefPtr<const RestrictionElement> const_ptr_t;

	RestrictionElement(const AltRange& alt = AltRange());
	virtual ~RestrictionElement();

	void reference(void) const;
	void unreference(void) const;
	virtual ptr_t clone(void) const = 0;
	virtual ptr_t recompute(Database& db, const timepair_t& tm) const { return ptr_t(); }
	virtual void add_bbox(Rect& bbox, const TimeSlice& ts) const = 0;
	virtual bool is_bbox(const Rect& bbox, const timepair_t& tm) const = 0;
	virtual bool is_altrange(int32_t minalt, int32_t maxalt, const timepair_t& tm) const;
	const AltRange& get_altrange(void) const { return m_alt; }
	virtual CondResult evaluate(RestrictionEval& tfrs) const = 0;
	virtual CondResult evaluate_trace(RestrictionEval& tfrs, unsigned int altcnt, unsigned int seqcnt) const;
	virtual RuleSegment get_rule(void) const = 0;
	virtual bool is_valid_dct(void) const { return false; }
	virtual BidirAltRange evaluate_dct(DctParameters::Calc& dct) const;
	virtual BidirAltRange evaluate_dct_trace(DctParameters::Calc& dct, unsigned int altcnt, unsigned int seqcnt) const;
	virtual void get_dct_segments(DctSegments& seg) const;
	typedef std::map<Link,IntervalSet<int32_t> > dctconnpoints_t;
	virtual bool is_deparr_dct(Link& arpt, bool& arr, dctconnpoints_t& connpt) const { return false; }
	virtual std::vector<ptr_t> clone_crossingpoints(const std::vector<RuleSegment>& pts) const { return std::vector<ptr_t>(); }
	virtual std::vector<ptr_t> clone_crossingsegments(const std::vector<RuleSegment>& segs) const { return std::vector<ptr_t>(); }
	virtual bool is_mandatoryinbound(const timepair_t& tint, const std::vector<RuleSegment>& pts) const { return false; }
	virtual bool is_mandatoryoutbound(const timepair_t& tint, const std::vector<RuleSegment>& pts) const { return false; }
	virtual std::ostream& print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const = 0;
	virtual std::string to_str(timetype_t tm) const;
	virtual std::string to_shortstr(timetype_t tm) const;

	typedef enum {
		type_invalid,
		type_route,
		type_point,
		type_sidstar,
		type_airspace
	} type_t;

	virtual type_t get_type(void) const = 0;
	static ptr_t create(type_t t = type_invalid);
	static ptr_t create(ArchiveReadStream& ar);
	static ptr_t create(ArchiveReadBuffer& ar);
	static ptr_t create(ArchiveWriteStream& ar);
	static ptr_t create(LinkLoader& ar);
	static ptr_t create(DependencyGenerator<UUID::set_t>& ar);
	static ptr_t create(DependencyGenerator<Link::LinkSet>& ar);
	virtual void io(ArchiveReadStream& ar) = 0;
	virtual void io(ArchiveReadBuffer& ar) = 0;
	virtual void io(ArchiveWriteStream& ar) const = 0;
	virtual void io(LinkLoader& ar) = 0;
	virtual void io(DependencyGenerator<UUID::set_t>& ar) const = 0;
	virtual void io(DependencyGenerator<Link::LinkSet>& ar) const = 0;

	template<class Archive> void hibernate(Archive& ar) {
		m_alt.hibernate(ar);
	}

protected:
	mutable gint m_refcount;
	AltRange m_alt;
};

class RestrictionElementRoute : public RestrictionElement {
public:
	RestrictionElementRoute(const AltRange& alt = AltRange(), const Link& pt0 = Link(), const Link& pt1 = Link(),
				const Link& rte = Link(), const SegmentsList& segs = SegmentsList());
	virtual ptr_t clone(void) const;
	virtual ptr_t recompute(Database& db, const timepair_t& tm) const;
	virtual void add_bbox(Rect& bbox, const TimeSlice& ts) const;
	virtual bool is_bbox(const Rect& bbox, const timepair_t& tm) const;
	virtual CondResult evaluate(RestrictionEval& tfrs) const;
	virtual RuleSegment get_rule(void) const;
	virtual bool is_valid_dct(void) const { return m_route.is_nil(); }
	virtual BidirAltRange evaluate_dct(DctParameters::Calc& dct) const;
	virtual void get_dct_segments(DctSegments& seg) const;
	virtual bool is_deparr_dct(Link& arpt, bool& arr, dctconnpoints_t& connpt) const;
	virtual std::vector<ptr_t> clone_crossingsegments(const std::vector<RuleSegment>& segs) const;
	virtual bool is_mandatoryinbound(const timepair_t& tint, const std::vector<RuleSegment>& pts) const;
	virtual bool is_mandatoryoutbound(const timepair_t& tint, const std::vector<RuleSegment>& pts) const;
	// FIXME: find altitude of route segment!
	virtual std::ostream& print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const;
	virtual std::string to_str(timetype_t tm) const;
	virtual std::string to_shortstr(timetype_t tm) const;

	virtual type_t get_type(void) const { return type_route; }
	virtual void io(ArchiveReadStream& ar);
	virtual void io(ArchiveReadBuffer& ar);
	virtual void io(ArchiveWriteStream& ar) const;
	virtual void io(LinkLoader& ar);
	virtual void io(DependencyGenerator<UUID::set_t>& ar) const;
	virtual void io(DependencyGenerator<Link::LinkSet>& ar) const;

	template<class Archive> void hibernate(Archive& ar) {
		RestrictionElement::hibernate(ar);
		ar.io(m_point[0]);
		ar.io(m_point[1]);
		ar.io(m_route);
		m_segments.hibernate(ar);
	}

protected:
	Link m_point[2];
	Link m_route;
	SegmentsList m_segments;
};

class RestrictionElementPoint : public RestrictionElement {
public:
	RestrictionElementPoint(const AltRange& alt = AltRange(), const Link& pt = Link());
	virtual ptr_t clone(void) const;
	virtual void add_bbox(Rect& bbox, const TimeSlice& ts) const;
	virtual bool is_bbox(const Rect& bbox, const timepair_t& tm) const;
	virtual CondResult evaluate(RestrictionEval& tfrs) const;
	virtual RuleSegment get_rule(void) const;
	virtual bool is_valid_dct(void) const { return true; }
	virtual BidirAltRange evaluate_dct(DctParameters::Calc& dct) const;
	virtual std::vector<ptr_t> clone_crossingpoints(const std::vector<RuleSegment>& pts) const;
	virtual std::ostream& print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const;
	virtual std::string to_str(timetype_t tm) const;
	virtual std::string to_shortstr(timetype_t tm) const;

	virtual type_t get_type(void) const { return type_point; }
	virtual void io(ArchiveReadStream& ar);
	virtual void io(ArchiveReadBuffer& ar);
	virtual void io(ArchiveWriteStream& ar) const;
	virtual void io(LinkLoader& ar);
	virtual void io(DependencyGenerator<UUID::set_t>& ar) const;
	virtual void io(DependencyGenerator<Link::LinkSet>& ar) const;

	template<class Archive> void hibernate(Archive& ar) {
		RestrictionElement::hibernate(ar);
		ar.io(m_point);
	}

protected:
	Link m_point;
};

class RestrictionElementSidStar : public RestrictionElement {
public:
	RestrictionElementSidStar(const AltRange& alt = AltRange(), const Link& proc = Link(), bool star = false);
	virtual ptr_t clone(void) const;
	virtual void add_bbox(Rect& bbox, const TimeSlice& ts) const;
	virtual bool is_bbox(const Rect& bbox, const timepair_t& tm) const;
	virtual CondResult evaluate(RestrictionEval& tfrs) const;
	virtual RuleSegment get_rule(void) const;
	virtual bool is_mandatoryinbound(const timepair_t& tint, const std::vector<RuleSegment>& pts) const;
	virtual bool is_mandatoryoutbound(const timepair_t& tint, const std::vector<RuleSegment>& pts) const;
	virtual std::ostream& print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const;
	virtual std::string to_str(timetype_t tm) const;
	virtual std::string to_shortstr(timetype_t tm) const;

	virtual type_t get_type(void) const { return type_sidstar; }
	virtual void io(ArchiveReadStream& ar);
	virtual void io(ArchiveReadBuffer& ar);
	virtual void io(ArchiveWriteStream& ar) const;
	virtual void io(LinkLoader& ar);
	virtual void io(DependencyGenerator<UUID::set_t>& ar) const;
	virtual void io(DependencyGenerator<Link::LinkSet>& ar) const;

	template<class Archive> void hibernate(Archive& ar) {
		RestrictionElement::hibernate(ar);
		ar.io(m_proc);
		ar.iouint8(m_star);
	}

protected:
	Link m_proc;
	bool m_star;
};

class RestrictionElementAirspace : public RestrictionElement {
public:
	RestrictionElementAirspace(const AltRange& alt = AltRange(), const Link& aspc = Link());
	virtual ptr_t clone(void) const;
	virtual void add_bbox(Rect& bbox, const TimeSlice& ts) const;
	virtual bool is_bbox(const Rect& bbox, const timepair_t& tm) const;
	virtual bool is_altrange(int32_t minalt, int32_t maxalt, const timepair_t& tm) const;
	virtual CondResult evaluate(RestrictionEval& tfrs) const;
	virtual RuleSegment get_rule(void) const;
	const Link& get_airspace(void) const { return m_airspace; }
	virtual std::ostream& print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const;
	virtual std::string to_str(timetype_t tm) const;
	virtual std::string to_shortstr(timetype_t tm) const;

	virtual type_t get_type(void) const { return type_airspace; }
	virtual void io(ArchiveReadStream& ar);
	virtual void io(ArchiveReadBuffer& ar);
	virtual void io(ArchiveWriteStream& ar) const;
	virtual void io(LinkLoader& ar);
	virtual void io(DependencyGenerator<UUID::set_t>& ar) const;
	virtual void io(DependencyGenerator<Link::LinkSet>& ar) const;

	template<class Archive> void hibernate(Archive& ar) {
		RestrictionElement::hibernate(ar);
		ar.io(m_airspace);
	}

protected:
        Link m_airspace;
};

class RestrictionSequence {
public:
	RestrictionSequence(void);
	void clone(void);
	bool recompute(Database& db, const timepair_t& tm);
	void add_bbox(Rect& bbox, const TimeSlice& ts) const;
	bool is_bbox(const Rect& bbox, const timepair_t& tm) const;
	bool is_altrange(int32_t minalt, int32_t maxalt, const timepair_t& tm) const;
	CondResult evaluate(RestrictionEval& tfrs) const;
	CondResult evaluate_trace(RestrictionEval& tfrs, unsigned int altcnt) const;
	RuleSequence get_rule(void) const;
	bool is_valid_dct(void) const;
	BidirAltRange evaluate_dct(DctParameters::Calc& dct) const;
	BidirAltRange evaluate_dct_trace(DctParameters::Calc& dct, unsigned int altcnt) const;
	void get_dct_segments(DctSegments& seg) const;
	bool is_deparr_dct(Link& arpt, bool& arr, RestrictionElement::dctconnpoints_t& connpt) const;
	bool clone_crossingpoints(std::vector<RestrictionSequence>& seq, const std::vector<RuleSegment>& pts) const;
	bool clone_crossingsegments(std::vector<RestrictionSequence>& seq, const std::vector<RuleSegment>& segs) const;
	bool is_mandatoryinbound(const timepair_t& tint, const std::vector<RuleSegment>& pts) const;
	bool is_mandatoryoutbound(const timepair_t& tint, const std::vector<RuleSegment>& pts) const;
	std::ostream& print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const;
	std::string to_str(timetype_t tm) const;
	std::string to_shortstr(timetype_t tm) const;

protected:
	typedef std::vector<RestrictionElement::ptr_t> elements_t;

public:
	typedef elements_t::size_type size_type;

	size_type size(void) const { return m_elements.size(); }
	RestrictionElement::ptr_t operator[](size_type idx) { return m_elements[idx]; }
	RestrictionElement::const_ptr_t operator[](size_type idx) const { return m_elements[idx]; }
	void add(RestrictionElement::ptr_t e);

	void io(ArchiveReadStream& ar);
	void io(ArchiveReadBuffer& ar);
	void io(ArchiveWriteStream& ar) const;
	void io(LinkLoader& ar);
	void io(DependencyGenerator<UUID::set_t>& ar) const;
	void io(DependencyGenerator<Link::LinkSet>& ar) const;

	template<class Archive> void hibernate(Archive& ar) {
		uint32_t sz(m_elements.size());
		ar.ioleb(sz);
		if (ar.is_load()) {
			m_elements.clear();
			for (; sz > 0; --sz)
				m_elements.push_back(RestrictionElement::create(ar));
		} else if (ar.is_save()) {
			for (elements_t::iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
				uint8_t x(RestrictionElement::type_invalid);
				if (*i)
					x = (*i)->get_type();
				ar.iouint8(x);
				if (!*i)
					continue;
				(*i)->io(ar);
			}
		} else {
			for (elements_t::iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
				if (*i)
					(*i)->io(ar);
		}
	}

protected:
	elements_t m_elements;
};

class Restrictions {
public:
	Restrictions(void);
	void clone(void);
	bool recompute(Database& db, const timepair_t& tm);
	void add_bbox(Rect& bbox, const TimeSlice& ts) const;
	bool simplify_bbox(const Rect& bbox, const timepair_t& tm);
	bool simplify_altrange(int32_t minalt, int32_t maxalt, const timepair_t& tm);
	bool evaluate(RestrictionEval& tfrs, RestrictionResult& result, bool forbidden) const;
	bool evaluate_trace(RestrictionEval& tfrs, RestrictionResult& result, bool forbidden) const;
	bool is_valid_dct(void) const;
	BidirAltRange evaluate_dct(DctParameters::Calc& dct, bool forbidden) const;
	BidirAltRange evaluate_dct_trace(DctParameters::Calc& dct, bool forbidden) const;
	void get_dct_segments(DctSegments& seg) const;
	bool is_deparr_dct(Link& arpt, bool& arr, RestrictionElement::dctconnpoints_t& connpt) const;
	bool clone_crossingpoints(Restrictions& r, const std::vector<RuleSegment>& pts) const;
	bool clone_crossingsegments(Restrictions& r, const std::vector<RuleSegment>& segs) const;
	bool is_mandatoryinbound(const timepair_t& tint, const std::vector<RuleSegment>& pts) const;
	bool is_mandatoryoutbound(const timepair_t& tint, const std::vector<RuleSegment>& pts) const;
	std::ostream& print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const;
	std::string to_str(timetype_t tm) const;
	std::string to_shortstr(timetype_t tm) const;

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

	void io(ArchiveReadStream& ar);
	void io(ArchiveReadBuffer& ar);
	void io(ArchiveWriteStream& ar) const;
	void io(LinkLoader& ar);
	void io(DependencyGenerator<UUID::set_t>& ar) const;
	void io(DependencyGenerator<Link::LinkSet>& ar) const;

	template<class Archive> void hibernate(Archive& ar) {
		uint32_t sz(m_elements.size());
		ar.ioleb(sz);
		if (ar.is_load())
			m_elements.resize(sz);
		for (elements_t::iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
			i->io(ar);
	}

protected:
	elements_t m_elements;
};

class Condition {
public:
	typedef Glib::RefPtr<Condition> ptr_t;
	typedef Glib::RefPtr<const Condition> const_ptr_t;
	static const bool ifr_refloc = true;

	typedef enum {
		civmil_civ = 'C',
		civmil_mil = 'M',
		civmil_invalid = 0
	} civmil_t;

	class Path {
	public:
		Path(const Path *backptr = 0, const Condition *condptr = 0) : m_backptr(backptr), m_condptr(condptr) {}
		const Path *get_back(void) const { return m_backptr; }
		const Condition *get_cond(void) const { return m_condptr; }
	protected:
		const Path *m_backptr;
		const Condition *m_condptr;
	};

	Condition(uint32_t childnum = 0);
	virtual ~Condition();

	void reference(void) const;
	void unreference(void) const;
	unsigned int get_childnum(void) const { return m_childnum; }
	void set_childnum(unsigned int x) { m_childnum = x; }
	virtual bool is_refloc(void) const { return false; }
	virtual bool is_constant(void) const { return false; }
	virtual bool get_constvalue(void) const { return false; }
	virtual std::ostream& print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const = 0;
	virtual std::string to_str(timetype_t tm) const = 0;
	virtual void add_bbox(Rect& bbox, const TimeSlice& ts) const {}
	virtual ptr_t clone(void) const = 0;
	virtual ptr_t recompute(Database& db, const timepair_t& tm) const { return ptr_t(); }
	virtual ptr_t simplify(void) const;
	virtual ptr_t simplify_bbox(const Rect& bbox, const timepair_t& tm) const;
	virtual ptr_t simplify_altrange(int32_t minalt, int32_t maxalt, const timepair_t& tm) const;
	virtual ptr_t simplify_aircrafttype(const std::string& acfttype) const;
	virtual ptr_t simplify_aircraftclass(const std::string& acftclass) const;
	virtual ptr_t simplify_equipment(const std::string& equipment, Aircraft::pbn_t pbn) const;
	virtual ptr_t simplify_typeofflight(char type_of_flight) const;
	virtual ptr_t simplify_mil(bool mil) const;
	virtual ptr_t simplify_dep(const Link& arpt, const timepair_t& tm) const;
	virtual ptr_t simplify_dest(const Link& arpt, const timepair_t& tm) const;
	virtual ptr_t simplify_conditionalavailability(const Graph& g, const ConditionalAvailability& condavail, const TimeTableSpecialDateEval& tsde, const timepair_t& tm) const;
	virtual CondResult evaluate(RestrictionEval& tfrs) const = 0;
	virtual CondResult evaluate_trace(RestrictionEval& tfrs, const Path *backptr = 0) const;
	virtual RuleSegment get_resultsequence(void) const;
	typedef enum {
		routestatic_staticfalse,
		routestatic_statictrue,
		routestatic_staticunknown,
		routestatic_nonstatic
	} routestatic_t;
	virtual routestatic_t is_routestatic(RuleSegment& seg) const { return is_routestatic() ? routestatic_staticunknown : routestatic_nonstatic; }
	virtual bool is_routestatic(void) const { return false; }
	virtual bool is_dct(void) const { return false; }
	virtual bool is_valid_dct(bool allowarrdep, civmil_t& civmil) const { return false; }
	virtual BidirAltRange evaluate_dct(DctParameters::Calc& dct) const;
	virtual BidirAltRange evaluate_dct_trace(DctParameters::Calc& dct, const Path *backptr = 0) const;
	virtual bool is_deparr(std::set<Link>& dep, std::set<Link>& dest) const { return false; }
	virtual bool is_deparr_dct(Link& arpt, bool& arr, double& dist, civmil_t& civmil) const { return false; }
	virtual bool is_enroute_dct(Link& aspc, AltRange& alt, double& dist, civmil_t& civmil) const { return false; }
	virtual std::vector<RuleSequence> get_mandatory(void) const;
	virtual std::vector<RuleSequence> get_mandatory_seq(void) const;
	virtual std::vector<RuleSegment> get_mandatory_or(void) const;
	virtual ptr_t extract_crossingpoints(std::vector<RuleSegment>& pts, const timepair_t& tm,
					     const Object::const_ptr_t& aspc = Object::const_ptr_t()) const { return ptr_t(); }
	virtual ptr_t extract_crossingsegments(std::vector<RuleSegment>& segs, const timepair_t& tm,
					       const Object::const_ptr_t& aspc = Object::const_ptr_t()) const { return ptr_t(); }
	virtual ptr_t extract_crossingairspaces(std::vector<RuleSegment>& aspcs) const { return ptr_t(); }
	typedef enum {
		forbiddenpoint_true,
		forbiddenpoint_truesamesegbefore,
		forbiddenpoint_truesamesegat,
		forbiddenpoint_truesamesegafter,
		forbiddenpoint_trueotherseg,
		forbiddenpoint_false,
		forbiddenpoint_falsesamesegbefore,
		forbiddenpoint_falsesamesegat,
		forbiddenpoint_falsesamesegafter,
		forbiddenpoint_falseotherseg,
		forbiddenpoint_invalid
	} forbiddenpoint_t;
	virtual forbiddenpoint_t evaluate_forbidden_point(const UUID& pt, const UUID& dep, const UUID& dest,
							  const TimeTableEval& tte, const Point& v1c, 
							  const UUID& v0, const UUID& v1,
							  const UUID& awy, int32_t alt) const { return forbiddenpoint_invalid; }

	typedef enum {
		type_invalid,
		type_and,
		type_seq,
		type_constant,
		type_crossingairspace1,
		type_crossingairspace2,
		type_crossingdct,
		type_crossingairway,
		type_crossingpoint,
		type_crossingdeparr,
		type_crossingdeparrairspace,
		type_crossingsidstar,
		type_crossingairspaceactive,
		type_crossingairwayavailable,
		type_dctlimit,
		type_aircraft,
		type_flight
	} type_t;

	virtual type_t get_type(void) const = 0;
	static ptr_t create(type_t t = type_invalid);
	static ptr_t create(ArchiveReadStream& ar);
	static ptr_t create(ArchiveReadBuffer& ar);
	static ptr_t create(ArchiveWriteStream& ar);
	static ptr_t create(LinkLoader& ar);
	static ptr_t create(DependencyGenerator<UUID::set_t>& ar);
	static ptr_t create(DependencyGenerator<Link::LinkSet>& ar);
	virtual void io(ArchiveReadStream& ar) = 0;
	virtual void io(ArchiveReadBuffer& ar) = 0;
	virtual void io(ArchiveWriteStream& ar) const = 0;
	virtual void io(LinkLoader& ar) = 0;
	virtual void io(DependencyGenerator<UUID::set_t>& ar) const = 0;
	virtual void io(DependencyGenerator<Link::LinkSet>& ar) const = 0;

	template<class Archive> void hibernate(Archive& ar) {
		ar.ioleb(m_childnum);
	}

protected:
	mutable gint m_refcount;
	uint32_t m_childnum;

	std::string get_path(const Path *backptr) const;
	void trace(RestrictionEvalMessages& tfrs, const CondResult& r, const Path *backptr) const;
	void trace(RestrictionEvalMessages& tfrs, const BidirAltRange& r, const Path *backptr) const;
};

class ConditionAnd : public Condition {
public:
	ConditionAnd(uint32_t childnum = 0, bool resultinv = false);
	void add(const_ptr_t p, bool inv);
	virtual bool is_refloc(void) const;
	virtual bool is_constant(void) const;
	virtual bool get_constvalue(void) const;
	virtual std::ostream& print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const;
	virtual std::string to_str(timetype_t tm) const;
	virtual void add_bbox(Rect& bbox, const TimeSlice& ts) const;
	virtual ptr_t clone(void) const;
	virtual ptr_t recompute(Database& db, const timepair_t& tm) const;
	virtual ptr_t simplify(void) const;
	virtual ptr_t simplify_bbox(const Rect& bbox, const timepair_t& tm) const;
	virtual ptr_t simplify_altrange(int32_t minalt, int32_t maxalt, const timepair_t& tm) const;
	virtual ptr_t simplify_aircrafttype(const std::string& acfttype) const;
	virtual ptr_t simplify_aircraftclass(const std::string& acftclass) const;
	virtual ptr_t simplify_equipment(const std::string& equipment, Aircraft::pbn_t pbn) const;
	virtual ptr_t simplify_typeofflight(char type_of_flight) const;
	virtual ptr_t simplify_mil(bool mil) const;
	virtual ptr_t simplify_dep(const Link& arpt, const timepair_t& tm) const;
	virtual ptr_t simplify_dest(const Link& arpt, const timepair_t& tm) const;
	virtual ptr_t simplify_conditionalavailability(const Graph& g, const ConditionalAvailability& condavail, const TimeTableSpecialDateEval& tsde, const timepair_t& tm) const;
	virtual CondResult evaluate(RestrictionEval& tfrs) const;
	virtual CondResult evaluate_trace(RestrictionEval& tfrs, const Path *backptr = 0) const;
	virtual routestatic_t is_routestatic(RuleSegment& seg) const;
	virtual bool is_routestatic(void) const;
	virtual bool is_dct(void) const;
	virtual bool is_valid_dct(bool allowarrdep, civmil_t& civmil) const;
	virtual BidirAltRange evaluate_dct(DctParameters::Calc& dct) const;
	virtual BidirAltRange evaluate_dct_trace(DctParameters::Calc& dct, const Path *backptr = 0) const;
	virtual bool is_deparr(std::set<Link>& dep, std::set<Link>& dest) const;
	virtual bool is_deparr_dct(Link& arpt, bool& arr, double& dist, civmil_t& civmil) const;
	virtual bool is_enroute_dct(Link& aspc, AltRange& alt, double& dist, civmil_t& civmil) const;
	virtual std::vector<RuleSequence> get_mandatory(void) const;
	virtual std::vector<RuleSegment> get_mandatory_or(void) const;
	virtual ptr_t extract_crossingpoints(std::vector<RuleSegment>& pts, const timepair_t& tm,
					     const Object::const_ptr_t& aspc = Object::const_ptr_t()) const;
	virtual ptr_t extract_crossingsegments(std::vector<RuleSegment>& segs, const timepair_t& tm,
					     const Object::const_ptr_t& aspc = Object::const_ptr_t()) const;
	virtual ptr_t extract_crossingairspaces(std::vector<RuleSegment>& aspcs) const;

	virtual forbiddenpoint_t evaluate_forbidden_point(const UUID& pt, const UUID& dep, const UUID& dest,
							  const TimeTableEval& tte, const Point& v1c, 
							  const UUID& v0, const UUID& v1, const UUID& awy, int32_t alt) const;
	unsigned int size(void) const { return m_cond.size(); }
	Condition::const_ptr_t operator[](unsigned int x) const { return m_cond[x].get_ptr(); }
	bool is_inv(void) const { return m_inv; }
	bool is_inv(unsigned int x) const { return m_cond[x].is_inv(); }

	virtual type_t get_type(void) const { return type_and; }
	virtual void io(ArchiveReadStream& ar);
	virtual void io(ArchiveReadBuffer& ar);
	virtual void io(ArchiveWriteStream& ar) const;
	virtual void io(LinkLoader& ar);
	virtual void io(DependencyGenerator<UUID::set_t>& ar) const;
	virtual void io(DependencyGenerator<Link::LinkSet>& ar) const;

	template<class Archive> void hibernate(Archive& ar) {
		Condition::hibernate(ar);
		ar.iouint8(m_inv);
		uint32_t sz(m_cond.size());
		ar.ioleb(sz);
		if (ar.is_load())
			m_cond.resize(sz);
		for (cond_t::iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i)
			i->hibernate(ar);
	}

protected:
	class CondInv {
	public:
		CondInv(const_ptr_t p = const_ptr_t(), bool inv = false) : m_ptr(p), m_inv(inv) {}
		const_ptr_t get_ptr(void) const { return m_ptr; }
		bool is_inv(void) const { return m_inv; }
		void set_ptr(const_ptr_t p) { m_ptr = p; }

		template<class Archive> void hibernate(Archive& ar) {
			if (ar.is_load()) {
				m_ptr = Condition::create(ar);
			} else if (ar.is_save()) {
				uint8_t x(type_invalid);
				if (m_ptr)
					x = m_ptr->get_type();
				ar.iouint8(x);
				if (m_ptr)
					ptr_t::cast_const(m_ptr)->io(ar);
			} else {
				if (m_ptr)
					ptr_t::cast_const(m_ptr)->io(ar);
			}
			ar.iouint8(m_inv);
		}

	protected:
		const_ptr_t m_ptr;
		bool m_inv;
	};
	typedef std::vector<CondInv> cond_t;
	cond_t m_cond;
	bool m_inv;

	typedef std::vector<const_ptr_t> seq_t;
	ptr_t simplify(const seq_t& seq) const;
};

class ConditionSequence : public Condition {
public:
	ConditionSequence(uint32_t childnum = 0);
	void add(const_ptr_t p);
	virtual bool is_refloc(void) const;
	virtual bool is_constant(void) const;
	virtual bool get_constvalue(void) const;
	virtual std::ostream& print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const;
	virtual std::string to_str(timetype_t tm) const;
	virtual void add_bbox(Rect& bbox, const TimeSlice& ts) const;
	virtual ptr_t clone(void) const;
	virtual ptr_t recompute(Database& db, const timepair_t& tm) const;
	virtual ptr_t simplify(void) const;
	virtual ptr_t simplify_bbox(const Rect& bbox, const timepair_t& tm) const;
	virtual ptr_t simplify_altrange(int32_t minalt, int32_t maxalt, const timepair_t& tm) const;
	virtual ptr_t simplify_aircrafttype(const std::string& acfttype) const;
	virtual ptr_t simplify_aircraftclass(const std::string& acftclass) const;
	virtual ptr_t simplify_equipment(const std::string& equipment, Aircraft::pbn_t pbn) const;
	virtual ptr_t simplify_typeofflight(char type_of_flight) const;
	virtual ptr_t simplify_mil(bool mil) const;
	virtual ptr_t simplify_dep(const Link& arpt, const timepair_t& tm) const;
	virtual ptr_t simplify_dest(const Link& arpt, const timepair_t& tm) const;
	virtual ptr_t simplify_conditionalavailability(const Graph& g, const ConditionalAvailability& condavail, const TimeTableSpecialDateEval& tsde, const timepair_t& tm) const;
	virtual CondResult evaluate(RestrictionEval& tfrs) const;
	virtual CondResult evaluate_trace(RestrictionEval& tfrs, const Path *backptr = 0) const;
	virtual routestatic_t is_routestatic(RuleSegment& seg) const;
	virtual bool is_routestatic(void) const;
	virtual std::vector<RuleSequence> get_mandatory_seq(void) const;
	virtual ptr_t extract_crossingpoints(std::vector<RuleSegment>& pts, const timepair_t& tm,
					     const Object::const_ptr_t& aspc = Object::const_ptr_t()) const;
	virtual ptr_t extract_crossingsegments(std::vector<RuleSegment>& segs, const timepair_t& tm,
					       const Object::const_ptr_t& aspc = Object::const_ptr_t()) const;
	virtual ptr_t extract_crossingairspaces(std::vector<RuleSegment>& aspcs) const;
	virtual forbiddenpoint_t evaluate_forbidden_point(const UUID& pt, const UUID& dep, const UUID& dest,
							  const TimeTableEval& tte, const Point& v1c, 
							  const UUID& v0, const UUID& v1, const UUID& awy, int32_t alt) const;
	unsigned int size(void) const { return m_seq.size(); }
	Condition::const_ptr_t operator[](unsigned int x) const { return m_seq[x]; }

	virtual type_t get_type(void) const { return type_seq; }
	virtual void io(ArchiveReadStream& ar);
	virtual void io(ArchiveReadBuffer& ar);
	virtual void io(ArchiveWriteStream& ar) const;
	virtual void io(LinkLoader& ar);
	virtual void io(DependencyGenerator<UUID::set_t>& ar) const;
	virtual void io(DependencyGenerator<Link::LinkSet>& ar) const;

	template<class Archive> void hibernate(Archive& ar) {
		Condition::hibernate(ar);
		uint32_t sz(m_seq.size());
		ar.ioleb(sz);
		if (ar.is_load()) {
			m_seq.clear();
			for (; sz > 0; --sz)
				m_seq.push_back(Condition::create(ar));
		} else if (ar.is_save()) {
			for (seq_t::iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
				uint8_t x(type_invalid);
				if (*i)
					x = (*i)->get_type();
				ar.iouint8(x);
				if (!*i)
					continue;
				ptr_t::cast_const(*i)->io(ar);
			}
		} else {
			for (seq_t::iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i)
				if (*i)
					ptr_t::cast_const(*i)->io(ar);
		}
	}

protected:
	typedef std::vector<const_ptr_t> seq_t;
	seq_t m_seq;

	ptr_t simplify(const seq_t& seq) const;
};

class ConditionConstant : public Condition {
public:
	ConditionConstant(uint32_t childnum = 0, bool val = false);
	virtual bool is_constant(void) const { return true; }
	virtual bool get_constvalue(void) const { return m_value; }
	virtual std::ostream& print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const;
	virtual std::string to_str(timetype_t tm) const;
	virtual ptr_t clone(void) const;
	virtual CondResult evaluate(RestrictionEval& tfrs) const;
	virtual routestatic_t is_routestatic(RuleSegment& seg) const { return m_value ? routestatic_statictrue : routestatic_staticfalse; }
	virtual bool is_routestatic(void) const { return true; }
	virtual bool is_valid_dct(bool allowarrdep, civmil_t& civmil) const { return true; }
	virtual BidirAltRange evaluate_dct(DctParameters::Calc& dct) const;
	virtual forbiddenpoint_t evaluate_forbidden_point(const UUID& pt, const UUID& dep, const UUID& dest,
							  const TimeTableEval& tte, const Point& v1c, 
							  const UUID& v0, const UUID& v1, const UUID& awy, int32_t alt) const {
		return get_constvalue() ? forbiddenpoint_true : forbiddenpoint_false;
	}

	virtual type_t get_type(void) const { return type_constant; }
	virtual void io(ArchiveReadStream& ar);
	virtual void io(ArchiveReadBuffer& ar);
	virtual void io(ArchiveWriteStream& ar) const;
	virtual void io(LinkLoader& ar);
	virtual void io(DependencyGenerator<UUID::set_t>& ar) const;
	virtual void io(DependencyGenerator<Link::LinkSet>& ar) const;

	template<class Archive> void hibernate(Archive& ar) {
		Condition::hibernate(ar);
		ar.iouint8(m_value);
	}

protected:
	bool m_value;
};

class ConditionAltitude : public Condition {
public:
	ConditionAltitude(uint32_t childnum = 0, const AltRange& alt = AltRange());
	virtual ptr_t simplify_altrange(int32_t minalt, int32_t maxalt, const timepair_t& tm) const;
	const AltRange& get_alt(void) const { return m_alt; }

	template<class Archive> void hibernate(Archive& ar) {
		Condition::hibernate(ar);
		m_alt.hibernate(ar);
	}

protected:
	AltRange m_alt;

	bool check_alt(int32_t alt) const;
	std::string to_altstr(void) const { return m_alt.to_str(); }
};

class ConditionCrossingAirspace1 : public ConditionAltitude {
public:
	ConditionCrossingAirspace1(uint32_t childnum = 0, const AltRange& alt = AltRange(), const Link& aspc = Link(), bool refloc = false);
	virtual bool is_refloc(void) const { return m_refloc; }
	virtual std::ostream& print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const;
	virtual std::string to_str(timetype_t tm) const;
	virtual void add_bbox(Rect& bbox, const TimeSlice& ts) const;
	virtual ptr_t clone(void) const;
	virtual ptr_t simplify_bbox(const Rect& bbox, const timepair_t& tm) const;
	virtual CondResult evaluate(RestrictionEval& tfrs) const;
	virtual routestatic_t is_routestatic(RuleSegment& seg) const;
	virtual bool is_valid_dct(bool allowarrdep, civmil_t& civmil) const { return true; }
	virtual bool is_enroute_dct(Link& aspc, AltRange& alt, double& dist, civmil_t& civmil) const;
	virtual BidirAltRange evaluate_dct(DctParameters::Calc& dct) const;
	virtual ptr_t extract_crossingairspaces(std::vector<RuleSegment>& aspcs) const;
	const Link& get_airspace(void) const { return m_airspace; }
	virtual forbiddenpoint_t evaluate_forbidden_point(const UUID& pt, const UUID& dep, const UUID& dest,
							  const TimeTableEval& tte, const Point& v1c, 
							  const UUID& v0, const UUID& v1, const UUID& awy, int32_t alt) const;

	virtual type_t get_type(void) const { return type_crossingairspace1; }
	virtual void io(ArchiveReadStream& ar);
	virtual void io(ArchiveReadBuffer& ar);
	virtual void io(ArchiveWriteStream& ar) const;
	virtual void io(LinkLoader& ar);
	virtual void io(DependencyGenerator<UUID::set_t>& ar) const;
	virtual void io(DependencyGenerator<Link::LinkSet>& ar) const;

	template<class Archive> void hibernate(Archive& ar) {
		ConditionAltitude::hibernate(ar);
		ar.io(m_airspace);
		ar.iouint8(m_refloc);
	}

protected:
	Link m_airspace;
	bool m_refloc;
};

class ConditionCrossingAirspace2 : public ConditionAltitude {
public:
	ConditionCrossingAirspace2(uint32_t childnum = 0, const AltRange& alt = AltRange(),
				   const Link& aspc0 = Link(), const Link& aspc1 = Link(), bool refloc = false);
	virtual bool is_refloc(void) const { return m_refloc; }
	virtual std::ostream& print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const;
	virtual std::string to_str(timetype_t tm) const;
	virtual void add_bbox(Rect& bbox, const TimeSlice& ts) const;
	virtual ptr_t clone(void) const;
	virtual ptr_t simplify_bbox(const Rect& bbox, const timepair_t& tm) const;
	virtual CondResult evaluate(RestrictionEval& tfrs) const;
	virtual bool is_valid_dct(bool allowarrdep, civmil_t& civmil) const { return true; }
	virtual BidirAltRange evaluate_dct(DctParameters::Calc& dct) const;
	static uint8_t rev8(uint8_t x);
	virtual forbiddenpoint_t evaluate_forbidden_point(const UUID& pt, const UUID& dep, const UUID& dest,
							  const TimeTableEval& tte, const Point& v1c, 
							  const UUID& v0, const UUID& v1, const UUID& awy, int32_t alt) const;

	virtual type_t get_type(void) const { return type_crossingairspace2; }
	virtual void io(ArchiveReadStream& ar);
	virtual void io(ArchiveReadBuffer& ar);
	virtual void io(ArchiveWriteStream& ar) const;
	virtual void io(LinkLoader& ar);
	virtual void io(DependencyGenerator<UUID::set_t>& ar) const;
	virtual void io(DependencyGenerator<Link::LinkSet>& ar) const;

	template<class Archive> void hibernate(Archive& ar) {
		ConditionAltitude::hibernate(ar);
		ar.io(m_airspace[0]);
		ar.io(m_airspace[1]);
		ar.iouint8(m_refloc);
	}

protected:
	static constexpr float fuzzy_dist = 0.0;
	static const unsigned int fuzzy_exponent = 0;
	Link m_airspace[2];
	bool m_refloc;
};

class ConditionCrossingDct : public ConditionAltitude {
public:
	ConditionCrossingDct(uint32_t childnum = 0, const AltRange& alt = AltRange(),
			     const Link& wpt0 = Link(), const Link& wpt1 = Link(), bool refloc = false);
	virtual bool is_refloc(void) const { return m_refloc; }
	virtual std::ostream& print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const;
	virtual std::string to_str(timetype_t tm) const;
	virtual void add_bbox(Rect& bbox, const TimeSlice& ts) const;
	virtual ptr_t clone(void) const;
	virtual ptr_t simplify_bbox(const Rect& bbox, const timepair_t& tm) const;
	virtual CondResult evaluate(RestrictionEval& tfrs) const;
	virtual RuleSegment get_resultsequence(void) const;
	virtual routestatic_t is_routestatic(RuleSegment& seg) const;
	virtual bool is_valid_dct(bool allowarrdep, civmil_t& civmil) const { return true; }
	virtual BidirAltRange evaluate_dct(DctParameters::Calc& dct) const;
	virtual ptr_t extract_crossingsegments(std::vector<RuleSegment>& segs, const timepair_t& tm,
					       const Object::const_ptr_t& aspc = Object::const_ptr_t()) const;
	virtual forbiddenpoint_t evaluate_forbidden_point(const UUID& pt, const UUID& dep, const UUID& dest,
							  const TimeTableEval& tte, const Point& v1c, 
							  const UUID& v0, const UUID& v1, const UUID& awy, int32_t alt) const;
	const Link& get_wpt0(void) const { return m_wpt[0]; }
	const Link& get_wpt1(void) const { return m_wpt[1]; }

	virtual type_t get_type(void) const { return type_crossingdct; }
	virtual void io(ArchiveReadStream& ar);
	virtual void io(ArchiveReadBuffer& ar);
	virtual void io(ArchiveWriteStream& ar) const;
	virtual void io(LinkLoader& ar);
	virtual void io(DependencyGenerator<UUID::set_t>& ar) const;
	virtual void io(DependencyGenerator<Link::LinkSet>& ar) const;

	template<class Archive> void hibernate(Archive& ar) {
		ConditionAltitude::hibernate(ar);
		ar.io(m_wpt[0]);
		ar.io(m_wpt[1]);
		ar.iouint8(m_refloc);
	}

protected:
	Link m_wpt[2];
	bool m_refloc;
};

class ConditionCrossingAirway : public ConditionCrossingDct {
public:
	ConditionCrossingAirway(uint32_t childnum = 0, const AltRange& alt = AltRange(),
				const Link& wpt0 = Link(), const Link& wpt1 = Link(),
				const Link& awyname = Link(), bool refloc = false, const SegmentsList& segs = SegmentsList());
	virtual bool is_refloc(void) const { return m_refloc; }
	virtual std::ostream& print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const;
	virtual std::string to_str(timetype_t tm) const;
	virtual void add_bbox(Rect& bbox, const TimeSlice& ts) const;
	virtual ptr_t clone(void) const;
	virtual ptr_t recompute(Database& db, const timepair_t& tm) const;
	virtual ptr_t simplify_bbox(const Rect& bbox, const timepair_t& tm) const;
	virtual CondResult evaluate(RestrictionEval& tfrs) const;
	virtual RuleSegment get_resultsequence(void) const;
	virtual routestatic_t is_routestatic(RuleSegment& seg) const;
	// FIXME: Airway crossing altitude!
	virtual ptr_t extract_crossingsegments(std::vector<RuleSegment>& segs, const timepair_t& tm,
					       const Object::const_ptr_t& aspc = Object::const_ptr_t()) const;
	virtual forbiddenpoint_t evaluate_forbidden_point(const UUID& pt, const UUID& dep, const UUID& dest,
							  const TimeTableEval& tte, const Point& v1c, 
							  const UUID& v0, const UUID& v1, const UUID& awy, int32_t alt) const;
	const Link& get_awyname(void) const { return m_awy; }

	virtual type_t get_type(void) const { return type_crossingairway; }
	virtual void io(ArchiveReadStream& ar);
	virtual void io(ArchiveReadBuffer& ar);
	virtual void io(ArchiveWriteStream& ar) const;
	virtual void io(LinkLoader& ar);
	virtual void io(DependencyGenerator<UUID::set_t>& ar) const;
	virtual void io(DependencyGenerator<Link::LinkSet>& ar) const;

	template<class Archive> void hibernate(Archive& ar) {
		ConditionCrossingDct::hibernate(ar);
		ar.io(m_awy);
		m_segments.hibernate(ar);
	}

protected:
	Link m_awy;
	SegmentsList m_segments;
};

class ConditionCrossingPoint : public ConditionAltitude {
public:
	ConditionCrossingPoint(uint32_t childnum = 0, const AltRange& alt = AltRange(), const Link& wpt = Link(), bool refloc = false);
	virtual bool is_refloc(void) const { return m_refloc; }
	virtual std::ostream& print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const;
	virtual std::string to_str(timetype_t tm) const;
	virtual void add_bbox(Rect& bbox, const TimeSlice& ts) const;
	virtual ptr_t clone(void) const;
	virtual ptr_t simplify_bbox(const Rect& bbox, const timepair_t& tm) const;
	virtual CondResult evaluate(RestrictionEval& tfrs) const;
	virtual RuleSegment get_resultsequence(void) const;
	virtual routestatic_t is_routestatic(RuleSegment& seg) const;
	virtual bool is_valid_dct(bool allowarrdep, civmil_t& civmil) const { return true; }
	virtual BidirAltRange evaluate_dct(DctParameters::Calc& dct) const;
	virtual ptr_t extract_crossingpoints(std::vector<RuleSegment>& pts, const timepair_t& tm,
					     const Object::const_ptr_t& aspc = Object::const_ptr_t()) const;
	virtual forbiddenpoint_t evaluate_forbidden_point(const UUID& pt, const UUID& dep, const UUID& dest,
							  const TimeTableEval& tte, const Point& v1c, 
							  const UUID& v0, const UUID& v1, const UUID& awy, int32_t alt) const;
	const Link& get_wpt(void) const { return m_wpt; }

	virtual type_t get_type(void) const { return type_crossingpoint; }
	virtual void io(ArchiveReadStream& ar);
	virtual void io(ArchiveReadBuffer& ar);
	virtual void io(ArchiveWriteStream& ar) const;
	virtual void io(LinkLoader& ar);
	virtual void io(DependencyGenerator<UUID::set_t>& ar) const;
	virtual void io(DependencyGenerator<Link::LinkSet>& ar) const;

	template<class Archive> void hibernate(Archive& ar) {
		ConditionAltitude::hibernate(ar);
		ar.io(m_wpt);
		ar.iouint8(m_refloc);
	}

protected:
	Link m_wpt;
	bool m_refloc;
};

class ConditionDepArr : public Condition {
public:
	ConditionDepArr(uint32_t childnum = 0, const Link& airport = Link(), bool arr = false, bool refloc = false);
	virtual bool is_refloc(void) const { return m_refloc; }
	virtual std::ostream& print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const;
	virtual std::string to_str(timetype_t tm) const;
	virtual void add_bbox(Rect& bbox, const TimeSlice& ts) const;
	virtual ptr_t clone(void) const;
	virtual ptr_t simplify_bbox(const Rect& bbox, const timepair_t& tm) const;
	virtual ptr_t simplify_dep(const Link& arpt, const timepair_t& tm) const;
	virtual ptr_t simplify_dest(const Link& arpt, const timepair_t& tm) const;
	virtual CondResult evaluate(RestrictionEval& tfrs) const;
	virtual bool is_routestatic(void) const { return true; }
	virtual bool is_valid_dct(bool allowarrdep, civmil_t& civmil) const { return allowarrdep; }
	virtual bool is_deparr(std::set<Link>& dep, std::set<Link>& dest) const;
	virtual bool is_deparr_dct(Link& arpt, bool& arr, double& dist, civmil_t& civmil) const;
	virtual ptr_t extract_crossingpoints(std::vector<RuleSegment>& pts, const timepair_t& tm,
					     const Object::const_ptr_t& aspc = Object::const_ptr_t()) const;
	virtual forbiddenpoint_t evaluate_forbidden_point(const UUID& pt, const UUID& dep, const UUID& dest,
							  const TimeTableEval& tte, const Point& v1c, 
							  const UUID& v0, const UUID& v1, const UUID& awy, int32_t alt) const;

	virtual type_t get_type(void) const { return type_crossingdeparr; }
	virtual void io(ArchiveReadStream& ar);
	virtual void io(ArchiveReadBuffer& ar);
	virtual void io(ArchiveWriteStream& ar) const;
	virtual void io(LinkLoader& ar);
	virtual void io(DependencyGenerator<UUID::set_t>& ar) const;
	virtual void io(DependencyGenerator<Link::LinkSet>& ar) const;

	template<class Archive> void hibernate(Archive& ar) {
		Condition::hibernate(ar);
		ar.io(m_airport);
		ar.iouint8(m_arr);
		ar.iouint8(m_refloc);
	}

protected:
	Link m_airport;
	bool m_arr;
	bool m_refloc;
};

class ConditionDepArrAirspace : public Condition {
public:
	ConditionDepArrAirspace(uint32_t childnum = 0, const Link& aspc = Link(), bool arr = false, bool refloc = false);
	virtual bool is_refloc(void) const { return m_refloc; }
	virtual std::ostream& print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const;
	virtual std::string to_str(timetype_t tm) const;
	virtual void add_bbox(Rect& bbox, const TimeSlice& ts) const;
	virtual ptr_t clone(void) const;
	virtual ptr_t simplify_bbox(const Rect& bbox, const timepair_t& tm) const;
	virtual ptr_t simplify_dep(const Link& arpt, const timepair_t& tm) const;
	virtual ptr_t simplify_dest(const Link& arpt, const timepair_t& tm) const;
	virtual CondResult evaluate(RestrictionEval& tfrs) const;

	virtual type_t get_type(void) const { return type_crossingdeparrairspace; }
	virtual void io(ArchiveReadStream& ar);
	virtual void io(ArchiveReadBuffer& ar);
	virtual void io(ArchiveWriteStream& ar) const;
	virtual void io(LinkLoader& ar);
	virtual void io(DependencyGenerator<UUID::set_t>& ar) const;
	virtual void io(DependencyGenerator<Link::LinkSet>& ar) const;

	template<class Archive> void hibernate(Archive& ar) {
		Condition::hibernate(ar);
		ar.io(m_airspace);
		ar.iouint8(m_arr);
		ar.iouint8(m_refloc);
	}

protected:
	Link m_airspace;
	bool m_arr;
	bool m_refloc;
};

class ConditionSidStar : public Condition {
public:
	ConditionSidStar(uint32_t childnum = 0, const Link& proc = Link(), bool star = false, bool refloc = false);
	virtual bool is_refloc(void) const { return m_refloc; }
	virtual std::ostream& print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const;
	virtual std::string to_str(timetype_t tm) const;
	virtual void add_bbox(Rect& bbox, const TimeSlice& ts) const;
	virtual ptr_t clone(void) const;
	virtual ptr_t simplify_bbox(const Rect& bbox, const timepair_t& tm) const;
	virtual ptr_t simplify_dep(const Link& arpt, const timepair_t& tm) const;
	virtual ptr_t simplify_dest(const Link& arpt, const timepair_t& tm) const;
	virtual CondResult evaluate(RestrictionEval& tfrs) const;

	virtual type_t get_type(void) const { return type_crossingsidstar; }
	virtual void io(ArchiveReadStream& ar);
	virtual void io(ArchiveReadBuffer& ar);
	virtual void io(ArchiveWriteStream& ar) const;
	virtual void io(LinkLoader& ar);
	virtual void io(DependencyGenerator<UUID::set_t>& ar) const;
	virtual void io(DependencyGenerator<Link::LinkSet>& ar) const;

	template<class Archive> void hibernate(Archive& ar) {
		Condition::hibernate(ar);
		ar.io(m_proc);
		ar.iouint8(m_star);
		ar.iouint8(m_refloc);
	}

protected:
	Link m_proc;
	bool m_star;
	bool m_refloc;
};

class ConditionCrossingAirspaceActive : public Condition {
public:
	ConditionCrossingAirspaceActive(uint32_t childnum = 0, const Link& aspc = Link());
	virtual std::ostream& print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const;
	virtual std::string to_str(timetype_t tm) const;
	virtual ptr_t clone(void) const;
	virtual ptr_t simplify_conditionalavailability(const Graph& g, const ConditionalAvailability& condavail, const TimeTableSpecialDateEval& tsde, const timepair_t& tm) const;
	virtual CondResult evaluate(RestrictionEval& tfrs) const;

	virtual type_t get_type(void) const { return type_crossingairspaceactive; }
	virtual void io(ArchiveReadStream& ar);
	virtual void io(ArchiveReadBuffer& ar);
	virtual void io(ArchiveWriteStream& ar) const;
	virtual void io(LinkLoader& ar);
	virtual void io(DependencyGenerator<UUID::set_t>& ar) const;
	virtual void io(DependencyGenerator<Link::LinkSet>& ar) const;

	template<class Archive> void hibernate(Archive& ar) {
		Condition::hibernate(ar);
		ar.io(m_airspace);
	}

protected:
	Link m_airspace;
};

class ConditionCrossingAirwayAvailable : public ConditionAltitude {
public:
	ConditionCrossingAirwayAvailable(uint32_t childnum = 0, const AltRange& alt = AltRange(),
					 const Link& wpt0 = Link(), const Link& wpt1 = Link(), const Link& awy = Link());
	virtual std::ostream& print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const;
	virtual std::string to_str(timetype_t tm) const;
	virtual ptr_t clone(void) const;
	virtual ptr_t simplify_conditionalavailability(const Graph& g, const ConditionalAvailability& condavail, const TimeTableSpecialDateEval& tsde, const timepair_t& tm) const;
	virtual CondResult evaluate(RestrictionEval& tfrs) const;

	virtual type_t get_type(void) const { return type_crossingairwayavailable; }
	virtual void io(ArchiveReadStream& ar);
	virtual void io(ArchiveReadBuffer& ar);
	virtual void io(ArchiveWriteStream& ar) const;
	virtual void io(LinkLoader& ar);
	virtual void io(DependencyGenerator<UUID::set_t>& ar) const;
	virtual void io(DependencyGenerator<Link::LinkSet>& ar) const;

	template<class Archive> void hibernate(Archive& ar) {
		ConditionAltitude::hibernate(ar);
		ar.io(m_wpt[0]);
		ar.io(m_wpt[1]);
		ar.io(m_awy);
	}

protected:
	Link m_wpt[2];
	Link m_awy;

	int compute_crossing_band(IntervalSet<int32_t>& r, const Graph& g, const ConditionalAvailability& condavail,
				  const TimeTableSpecialDateEval& tsde, timetype_t tm, timetype_t& tuntil) const;
};

class ConditionDctLimit : public Condition {
public:
	ConditionDctLimit(uint32_t childnum = 0, double limit = 0);
	virtual std::ostream& print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const;
	virtual std::string to_str(timetype_t tm) const;
	virtual ptr_t clone(void) const;
	virtual CondResult evaluate(RestrictionEval& tfrs) const;
	virtual bool is_dct(void) const { return true; }
	virtual bool is_valid_dct(bool allowarrdep, civmil_t& civmil) const { return true; }
	virtual BidirAltRange evaluate_dct(DctParameters::Calc& dct) const;
	virtual bool is_deparr_dct(Link& arpt, bool& arr, double& dist, civmil_t& civmil) const;
	virtual bool is_enroute_dct(Link& aspc, AltRange& alt, double& dist, civmil_t& civmil) const;

	virtual type_t get_type(void) const { return type_dctlimit; }
	virtual void io(ArchiveReadStream& ar);
	virtual void io(ArchiveReadBuffer& ar);
	virtual void io(ArchiveWriteStream& ar) const;
	virtual void io(LinkLoader& ar);
	virtual void io(DependencyGenerator<UUID::set_t>& ar) const;
	virtual void io(DependencyGenerator<Link::LinkSet>& ar) const;

	template<class Archive> void hibernate(Archive& ar) {
		Condition::hibernate(ar);
		uint32_t dlim(ldexp(m_dctlimit, 16));
		ar.io(dlim);
		if (ar.is_load())
			m_dctlimit = ldexp(dlim, -16);
	}

protected:
	double m_dctlimit;
};

class ConditionAircraft : public Condition {
public:
	typedef enum {
		acfttype_landplane = 'L',
		acfttype_seaplane = 'S',
		acfttype_amphibian = 'A',
		acfttype_helicopter = 'H',
		acfttype_gyrocopter = 'G',
		acfttype_tiltwing = 'T',
		acfttype_invalid = 0
	} acfttype_t;

	typedef enum {
		engine_piston = 'P',
		engine_turboprop = 'T',
		engine_jet = 'J',
		engine_invalid = 0
	} engine_t;

	typedef enum {
		navspec_rnav1 = 1,
		navspec_invalid = 0
	} navspec_t;

	typedef enum {
		vertsep_rvsm = 1,
		vertsep_invalid = 0
	} vertsep_t;

	ConditionAircraft(uint32_t childnum = 0, const std::string& icaotype = "", uint8_t engines = 0, acfttype_t acfttype = acfttype_invalid, 
			  engine_t engine = engine_invalid, navspec_t navspec = navspec_invalid, vertsep_t vertsep = vertsep_invalid);
	virtual std::ostream& print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const;
	virtual std::string to_str(timetype_t tm) const;
	virtual ptr_t clone(void) const;
	virtual ptr_t simplify_aircrafttype(const std::string& acfttype) const;
	virtual ptr_t simplify_aircraftclass(const std::string& acftclass) const;
	virtual ptr_t simplify_equipment(const std::string& equipment, Aircraft::pbn_t pbn) const;
	virtual CondResult evaluate(RestrictionEval& tfrs) const;

	acfttype_t get_acfttype(void) const { return m_acfttype; }
	engine_t get_engine(void) const { return m_engine; }
	navspec_t get_navspec(void) const { return m_navspec; }
	vertsep_t get_vertsep(void) const { return m_vertsep; }
	const std::string& get_icaotype(void) const { return m_icaotype; }
	unsigned int get_engines(void) const { return m_engines; }

	virtual type_t get_type(void) const { return type_aircraft; }
	virtual void io(ArchiveReadStream& ar);
	virtual void io(ArchiveReadBuffer& ar);
	virtual void io(ArchiveWriteStream& ar) const;
	virtual void io(LinkLoader& ar);
	virtual void io(DependencyGenerator<UUID::set_t>& ar) const;
	virtual void io(DependencyGenerator<Link::LinkSet>& ar) const;

	template<class Archive> void hibernate(Archive& ar) {
		Condition::hibernate(ar);
		ar.io(m_icaotype);
		ar.iouint8(m_engines);
		ar.iouint8(m_acfttype);
		ar.iouint8(m_engine);
		ar.iouint8(m_navspec);
		ar.iouint8(m_vertsep);
	}

protected:
	std::string m_icaotype;
	uint8_t m_engines;
	acfttype_t m_acfttype;
	engine_t m_engine;
	navspec_t m_navspec;
	vertsep_t m_vertsep;
};

class ConditionFlight : public Condition {
public:
	typedef enum {
		purpose_all = 'A',
		purpose_scheduled = 'S',
		purpose_nonscheduled = 'N',
		purpose_private = 'G',
		purpose_participant = 'P',
		purpose_airtraining = 'T',
		purpose_airwork = 'W',
		purpose_invalid = 0
	} purpose_t;

	ConditionFlight(uint32_t childnum = 0, civmil_t civmil = civmil_invalid, purpose_t purpose = purpose_invalid);
	virtual std::ostream& print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const;
	virtual std::string to_str(timetype_t tm) const;
	virtual ptr_t clone(void) const;
	virtual ptr_t simplify_typeofflight(char type_of_flight) const;
	virtual ptr_t simplify_mil(bool mil) const;
	virtual CondResult evaluate(RestrictionEval& tfrs) const;
	virtual bool is_deparr_dct(Link& arpt, bool& arr, double& dist, civmil_t& civmil) const;
	virtual bool is_enroute_dct(Link& aspc, AltRange& alt, double& dist, civmil_t& civmil) const;

	civmil_t get_civmil(void) const { return m_civmil; }
	purpose_t get_purpose(void) const { return m_purpose; }

	virtual type_t get_type(void) const { return type_flight; }
	virtual void io(ArchiveReadStream& ar);
	virtual void io(ArchiveReadBuffer& ar);
	virtual void io(ArchiveWriteStream& ar) const;
	virtual void io(LinkLoader& ar);
	virtual void io(DependencyGenerator<UUID::set_t>& ar) const;
	virtual void io(DependencyGenerator<Link::LinkSet>& ar) const;

	template<class Archive> void hibernate(Archive& ar) {
		Condition::hibernate(ar);
		ar.iouint8(m_civmil);
		ar.iouint8(m_purpose);
	}

protected:
	civmil_t m_civmil;
	purpose_t m_purpose;
};

class FlightRestrictionTimeSlice : public IdentTimeSlice {
public:
	FlightRestrictionTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual timeset_t timediscontinuities(void) const;

	virtual FlightRestrictionTimeSlice& as_flightrestriction(void);
	virtual const FlightRestrictionTimeSlice& as_flightrestriction(void) const;

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

        const std::string& get_instruction(void) const { return m_instruction; }
	void set_instruction(const std::string& x) { m_instruction = x; }

	typedef enum {
		type_allowed,
		type_closed,
		type_mandatory,
		type_forbidden,
		type_invalid
	} type_t;

	type_t get_type(void) const { return m_type; }
	void set_type(type_t t) { m_type = t; }

	typedef enum {
		procind_tfr,
		procind_raddct,
		procind_fradct,
		procind_fpr,
		procind_adcp,
		procind_adfltrule,
		procind_fltprop,
		procind_invalid
	} procind_t;

	procind_t get_procind(void) const { return m_procind; }
	void set_procind(procind_t t) { m_procind = t; }

	bool is_enabled(void) const { return m_enabled; }
	void set_enabled(bool ena = true) { m_enabled = ena; }

	bool is_trace(void) const { return m_trace; }
	void set_trace(bool ena = true) { m_trace = ena; }

	const TimeTable& get_timetable(void) const { return m_timetable; }
	void set_timetable(const TimeTable& tt) { m_timetable = tt; }

	virtual void get_bbox(Rect& bbox) const { bbox = m_bbox; }
	void set_bbox(const Rect& bbox) { m_bbox = bbox; }

	Condition::ptr_t get_condition(void) { return m_condition; }
	Condition::const_ptr_t get_condition(void) const { return m_condition; }
	void set_condition(const Condition::ptr_t& cond) { m_condition = cond; }

	Restrictions& get_restrictions(void)  { return m_restrictions; }
	const Restrictions& get_restrictions(void) const { return m_restrictions; }
	void set_restrictions(const Restrictions& e) { m_restrictions = e; }
	void add_restriction(const RestrictionSequence& e) { m_restrictions.add(e); }
	void add_restrictions(const Restrictions& e) { m_restrictions.add(e); }

	static char get_type_char(type_t t);
	char get_type_char(void) const { return get_type_char(get_type()); }

	bool evaluate(RestrictionEval& tfrs, RestrictionResult& result) const;
	BidirAltRange evaluate_dct(DctParameters::Calc& dct, bool trace) const;
	void get_dct_segments(DctSegments& seg) const;
	bool is_dct(void) const;
	bool is_strict_dct(void) const;
	typedef Condition::civmil_t civmil_t;
	bool is_dct(civmil_t& civmil) const;
	bool is_strict_dct(civmil_t& civmil) const;
	bool is_unconditional(void) const;
	bool is_routestatic(void) const;
	bool is_routestatic(RuleSegment& seg) const;
	bool is_mandatoryinbound(void) const;
	bool is_mandatoryoutbound(void) const;
	bool is_keep(void) const;
	bool is_deparr(std::set<Link>& dep, std::set<Link>& dest) const;
	bool is_deparr_dct(void) const;
	typedef RestrictionElement::dctconnpoints_t dctconnpoints_t;
	bool is_deparr_dct(Link& arpt, bool& arr, double& dist, dctconnpoints_t& connpt, civmil_t& civmil) const;
	bool is_enroute_dct(Link& aspc, AltRange& alt, double& dist, civmil_t& civmil) const;

	Condition::const_ptr_t get_crossingcond(std::vector<RuleSegment>& cc) const;
	std::vector<RuleSegment> get_crossingcond(void) const;
	std::vector<RuleSequence> get_mandatory(void) const;
	std::vector<RuleSegment> get_forbiddensegments(void) const;

	virtual void clone(void);
	virtual bool recompute(void);
	virtual bool recompute(Database& db);

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		IdentTimeSlice::hibernate(ar);
		m_timetable.hibernate(ar);
		ar.io(m_bbox);
		m_restrictions.io(ar);
		if (ar.is_load()) {
			m_condition = Condition::create(ar);
		} else if (ar.is_save()) {
			uint8_t t(Condition::type_invalid);
			if (m_condition)
				t = m_condition->get_type();
			ar.iouint8(t);
			if (m_condition)
				m_condition->io(ar);
		} else {
			if (m_condition)
				m_condition->io(ar);
		}
		ar.io(m_instruction);
		ar.iouint8(m_type);
		ar.iouint8(m_procind);
		ar.iouint8(m_enabled);
	}

protected:
	TimeTable m_timetable;
	Rect m_bbox;
	Restrictions m_restrictions;
	Condition::ptr_t m_condition;
        std::string m_instruction;
 	type_t m_type;
 	procind_t m_procind;
	bool m_enabled;
	bool m_trace; // not saved

	bool is_unconditional_airspace(void) const;
};

class FlightRestriction : public ObjectImpl<FlightRestrictionTimeSlice,Object::type_flightrestriction> {
public:
	typedef Glib::RefPtr<FlightRestriction> ptr_t;
	typedef Glib::RefPtr<const FlightRestriction> const_ptr_t;
	typedef ObjectImpl<FlightRestrictionTimeSlice,Object::type_flightrestriction> objbase_t;

	FlightRestriction(const UUID& uuid = UUID());

	ptr_t clone_obj(void) const { ptr_t p(new FlightRestriction(get_uuid())); clone_impl(p); return p; }
	virtual Object::ptr_t clone(void) const { return clone_obj(); }

	bool evaluate(RestrictionEval& tfrs, RestrictionResult& result) const;
	bool is_keep(void) const;
	ptr_t simplify(void) const;
	ptr_t simplify_bbox(const Rect& bbox) const;
	ptr_t simplify_altrange(int32_t minalt, int32_t maxalt) const;
	ptr_t simplify_aircrafttype(const std::string& acfttype) const;
	ptr_t simplify_aircraftclass(const std::string& acftclass) const;
	ptr_t simplify_equipment(const std::string& equipment, Aircraft::pbn_t pbn) const;
	ptr_t simplify_typeofflight(char type_of_flight) const;
	ptr_t simplify_mil(bool mil = false) const;
	ptr_t simplify_dep(const Link& arpt) const;
	ptr_t simplify_dest(const Link& arpt) const;
	ptr_t simplify_complexity(void) const;
	ptr_t simplify_complexity_crossingpoints(void) const;
	ptr_t simplify_complexity_crossingsegments(void) const;
	ptr_t simplify_complexity_closedairspace(void) const;
	ptr_t simplify_conditionalavailability(const Graph& g, const ConditionalAvailability& condavail, const TimeTableSpecialDateEval& tsde, const timepair_t& tm) const;
};

};

const std::string& to_str(ADR::FlightRestrictionTimeSlice::type_t t);
inline std::ostream& operator<<(std::ostream& os, ADR::FlightRestrictionTimeSlice::type_t t) { return os << to_str(t); }
const std::string& to_str(ADR::FlightRestrictionTimeSlice::procind_t p);
inline std::ostream& operator<<(std::ostream& os, ADR::FlightRestrictionTimeSlice::procind_t p) { return os << to_str(p); }
const std::string& to_str(ADR::RuleSegment::type_t t);
inline std::ostream& operator<<(std::ostream& os, ADR::RuleSegment::type_t t) { return os << to_str(t); }
const std::string& to_str(ADR::Condition::forbiddenpoint_t fp);
inline std::ostream& operator<<(std::ostream& os, ADR::Condition::forbiddenpoint_t fp) { return os << to_str(fp); }

#endif /* ADRRESTRICTION_H */
