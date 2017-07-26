#ifndef CFMUAUTOROUTE51_H
#define CFMUAUTOROUTE51_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cfmuautoroute.hh"
#include "adr.hh"
#include "adrdb.hh"
#include "adrgraph.hh"
#include "adrfplan.hh"
#include "adrrestriction.hh"

class CFMUAutoroute51 : public CFMUAutoroute {
public:
	CFMUAutoroute51();

	const std::vector<Glib::RefPtr<ADR::FlightRestriction> >& get_tfr(void) const { return m_eval.get_rules(); }

	bool check_fplan(std::vector<ADR::Message>& msgs, ADR::RestrictionResults& res, ADR::FlightPlan& route);

	virtual void preload(bool validator = true);
	virtual void reload(void);
	virtual void stop(statusmask_t sm);
	virtual void clear(void);

	virtual void precompute_graph(const Rect& bbox, const std::string& fn);

protected:
	typedef bool (CFMUAutoroute51::*parseresponse_t)(Glib::MatchInfo&);
	static const struct parsefunctions {
		const char *regex;
		parseresponse_t func;
	} parsefunctions[];
	static const char *ignoreregex[];

	void debug_print_rules(const std::string& fname);
	virtual bool start_ifr(bool cont = false, bool optfuel = false);

	void parse_response(void);
	bool parse_response_route42(Glib::MatchInfo& minfo);
	bool parse_response_route43(Glib::MatchInfo& minfo);
	bool parse_response_route49(Glib::MatchInfo& minfo);
	bool parse_response_route52(Glib::MatchInfo& minfo);
	bool parse_response_route130(Glib::MatchInfo& minfo);
	bool parse_response_route134(Glib::MatchInfo& minfo);
	bool parse_response_route135(Glib::MatchInfo& minfo);
	bool parse_response_route139(Glib::MatchInfo& minfo);
	bool parse_response_route140(Glib::MatchInfo& minfo);
	bool parse_response_route165(Glib::MatchInfo& minfo);
	bool parse_response_route168(Glib::MatchInfo& minfo);
	bool parse_response_route171(Glib::MatchInfo& minfo);
	bool parse_response_route172(Glib::MatchInfo& minfo);
	bool parse_response_route176(Glib::MatchInfo& minfo);
	bool parse_response_route179(Glib::MatchInfo& minfo);
	bool parse_response_prof50(Glib::MatchInfo& minfo);
	bool parse_response_prof193(Glib::MatchInfo& minfo);
	bool parse_response_prof194(Glib::MatchInfo& minfo);
	bool parse_response_prof195(Glib::MatchInfo& minfo);
	bool parse_response_prof195b(Glib::MatchInfo& minfo);
	bool parse_response_prof197(Glib::MatchInfo& minfo);
	bool parse_response_prof197b(Glib::MatchInfo& minfo);
	bool parse_response_prof198(Glib::MatchInfo& minfo);
	bool parse_response_prof199(Glib::MatchInfo& minfo);
	bool parse_response_prof200(Glib::MatchInfo& minfo);
	bool parse_response_prof201(Glib::MatchInfo& minfo);
	bool parse_response_prof201b(Glib::MatchInfo& minfo);
	bool parse_response_prof202(Glib::MatchInfo& minfo);
	bool parse_response_prof204(Glib::MatchInfo& minfo);
	bool parse_response_prof204b(Glib::MatchInfo& minfo);
	bool parse_response_prof204bflr(Glib::MatchInfo& minfo);
	bool parse_response_prof204c(Glib::MatchInfo& minfo);
	bool parse_response_prof204d(Glib::MatchInfo& minfo);
	bool parse_response_prof204e(Glib::MatchInfo& minfo);
	bool parse_response_prof204f(Glib::MatchInfo& minfo);
	bool parse_response_prof204_singleident(const Glib::ustring& ident);
	bool parse_response_prof204_singleident(const Glib::ustring& ident, int altfrom, int altto);
	bool parse_response_prof205(Glib::MatchInfo& minfo);
	bool parse_response_prof205b(Glib::MatchInfo& minfo);
	bool parse_response_prof205c(Glib::MatchInfo& minfo);
	bool parse_response_prof206(Glib::MatchInfo& minfo);
	bool parse_response_efpm228(Glib::MatchInfo& minfo);
	bool parse_response_fail(Glib::MatchInfo& minfo);

	class AirspaceCache;

	class LVertexLevel {
	public:
		LVertexLevel(ADR::Graph::vertex_descriptor v = 0, unsigned int pi = 0) : m_vertex(v), m_perfindex(pi) {}
		ADR::Graph::vertex_descriptor get_vertex(void) const { return m_vertex; }
		void set_vertex(ADR::Graph::vertex_descriptor v) { m_vertex = v; }
		unsigned int get_perfindex(void) const { return m_perfindex; }
		void set_perfindex(unsigned int pi) { m_perfindex = pi; }
		int compare(const LVertexLevel& x) const;
		bool operator==(const LVertexLevel& x) const { return compare(x) == 0; }
		bool operator!=(const LVertexLevel& x) const { return compare(x) != 0; }
		bool operator<(const LVertexLevel& x) const { return compare(x) < 0; }
		bool operator<=(const LVertexLevel& x) const { return compare(x) <= 0; }
		bool operator>(const LVertexLevel& x) const { return compare(x) > 0; }
		bool operator>=(const LVertexLevel& x) const { return compare(x) >= 0; }

	protected:
		ADR::Graph::vertex_descriptor m_vertex;
		unsigned int m_perfindex;
	};

	class LVertexLevelTime : public LVertexLevel {
	public:
		LVertexLevelTime(ADR::Graph::vertex_descriptor v = 0, unsigned int pi = 0, ADR::timetype_t tm = 0)
			: LVertexLevel(v, pi), m_time(tm) {}
		LVertexLevelTime(const LVertexLevel& v, ADR::timetype_t tm = 0)
			: LVertexLevel(v), m_time(tm) {}
		ADR::timetype_t get_time(void) const { return m_time; }
		void set_time(ADR::timetype_t t) { m_time = t; }
		int compare(const LVertexLevelTime& x) const;
		int compare_notime(const LVertexLevelTime& x) const { return LVertexLevel::compare(x); }
		bool operator==(const LVertexLevelTime& x) const { return compare(x) == 0; }
		bool operator!=(const LVertexLevelTime& x) const { return compare(x) != 0; }
		bool operator<(const LVertexLevelTime& x) const { return compare(x) < 0; }
		bool operator<=(const LVertexLevelTime& x) const { return compare(x) <= 0; }
		bool operator>(const LVertexLevelTime& x) const { return compare(x) > 0; }
		bool operator>=(const LVertexLevelTime& x) const { return compare(x) >= 0; }

	protected:
		ADR::timetype_t m_time;
	};

	class LVertexEdge : public LVertexLevelTime {
	public:
		// v should be the target of e
		LVertexEdge(const LVertexLevelTime& v = LVertexLevelTime(), const ADR::Object::const_ptr_t& e = ADR::Object::const_ptr_t())
			: LVertexLevelTime(v), m_edge(e) {}
		const ADR::Object::const_ptr_t& get_edge(void) const { return m_edge; }
		const ADR::UUID& get_uuid(void) const { return m_edge ? m_edge->get_uuid() : ADR::UUID::niluuid; }
		int compare(const LVertexEdge& x) const;
		int compare_notime(const LVertexEdge& x) const;
		bool operator==(const LVertexEdge& x) const { return compare(x) == 0; }
		bool operator!=(const LVertexEdge& x) const { return compare(x) != 0; }
		bool operator<(const LVertexEdge& x) const { return compare(x) < 0; }
		bool operator<=(const LVertexEdge& x) const { return compare(x) <= 0; }
		bool operator>(const LVertexEdge& x) const { return compare(x) > 0; }
		bool operator>=(const LVertexEdge& x) const { return compare(x) >= 0; }

	protected:
		ADR::Object::const_ptr_t m_edge;
	};

	class LRoute : public std::vector <LVertexEdge> {
	public:
		LRoute(void) : m_metric(std::numeric_limits<double>::quiet_NaN()) {}

		int compare(const LRoute& x) const;
		bool operator==(const LRoute& x) const { return compare(x) == 0; }
		bool operator!=(const LRoute& x) const { return compare(x) != 0; }
		bool operator<(const LRoute& x) const { return compare(x) < 0; }
		bool operator<=(const LRoute& x) const { return compare(x) <= 0; }
		bool operator>(const LRoute& x) const { return compare(x) > 0; }
		bool operator>=(const LRoute& x) const { return compare(x) >= 0; }
		void set_metric(double metric) { m_metric = metric; }
		double get_metric(void) const { return m_metric; }
		bool is_inf(void) const { return std::isinf(get_metric()); }
		bool is_nan(void) const { return std::isnan(get_metric()); }
		bool is_repeated_nodes(const ADR::Graph& g) const;
		bool is_existing(const ADR::Graph& g) const;
		std::pair<ADR::Graph::edge_descriptor,bool> get_edge(size_type i, const ADR::Graph& g) const;
		static std::pair<ADR::Graph::edge_descriptor,bool> get_edge(const LVertexEdge& u, const LVertexEdge& v, const ADR::Graph& g);
		static std::pair<ADR::Graph::edge_descriptor,bool> get_edge(ADR::Graph::vertex_descriptor u, const LVertexEdge& v, const ADR::Graph& g);

	protected:
		double m_metric;
	};

	class LGraphSolutionTreeNodeSorter {
	public:
		bool operator()(const LVertexEdge& a, const LVertexEdge& b) const { return a.compare_notime(b) < 0; }
	};

	class LGraphSolutionTreeNode;
	class LGraphSolutionTreeNode : public std::map<LVertexEdge,LGraphSolutionTreeNode,LGraphSolutionTreeNodeSorter> {
	};

	class DijkstraCore;

	class LGMandatoryPoint {
	public:
		LGMandatoryPoint(ADR::Graph::vertex_descriptor v = 0, unsigned int pi0 = 0, unsigned int pi1 = 0,
				 const ADR::Object::const_ptr_t& awy = ADR::Object::const_ptr_t())
			: m_v(v), m_pi0(pi0), m_pi1(pi1), m_airway(awy) {}
	        ADR::Graph::vertex_descriptor get_v(void) const { return m_v; }
	        unsigned int get_pi0(void) const { return m_pi0; }
	        unsigned int get_pi1(void) const { return m_pi1; }
		const ADR::Object::const_ptr_t& get_airway(void) const { return m_airway; }
		const ADR::UUID& get_uuid(void) const { return m_airway ? m_airway->get_uuid() : ADR::UUID::niluuid; }

	protected:
		ADR::Graph::vertex_descriptor m_v;
		unsigned int m_pi0;
		unsigned int m_pi1;
		ADR::Object::const_ptr_t m_airway;
	};

	class LGMandatorySequence : public std::vector<LGMandatoryPoint> {
	};

	class LGMandatoryAlternative : public std::vector<LGMandatorySequence> {
	public:
		LGMandatoryAlternative(const std::string& rule_id = "") : m_rule(rule_id) {}
		const std::string& get_rule(void) const { return m_rule; }
		void set_rule(const std::string& rule_id) { m_rule = rule_id; }

	protected:
		std::string m_rule;
	};

	class LGMandatory : public std::vector<LGMandatoryAlternative> {
	};

	ADR::Graph m_graph;
	ADR::Database m_db;
	ADR::AUPDatabase m_aupdb;
	ADR::RestrictionEval m_eval;
	typedef std::set<LVertexLevel> solutionvertices_t;
	solutionvertices_t m_solutionvertices;
	// Yen's k shortest loopless paths algorithm
	LGraphSolutionTreeNode m_solutiontree;
	typedef std::set<LRoute> lgraphsolutionpool_t;
	lgraphsolutionpool_t m_solutionpool;
	ADR::Graph::vertex_descriptor m_vertexdep;
	ADR::Graph::vertex_descriptor m_vertexdest;
	LGMandatory m_crossingmandatory;
	static const bool lgraphawyvertdct = false;
	static constexpr double localforbiddenpenalty = 1.1;

	void logmessage(const ADR::Message& msg);
	void clearlgraph(void);
	typedef enum {
		setmetric_none,
		setmetric_dist,
		setmetric_metric
	} setmetric_t;
	void lgraphaddedge(ADR::Graph& g, ADR::GraphEdge& ee, ADR::Graph::vertex_descriptor u, ADR::Graph::vertex_descriptor v, setmetric_t setmetric = setmetric_none, bool log = false);
	void lgraphaddedge(ADR::GraphEdge& ee, ADR::Graph::vertex_descriptor u, ADR::Graph::vertex_descriptor v, setmetric_t setmetric = setmetric_none, bool log = false) {
		lgraphaddedge(m_graph, ee, u, v, setmetric, log);
	}
	void lgraphkilledge(ADR::Graph& g, ADR::Graph::edge_descriptor e, unsigned int pi, bool log = false);
	void lgraphkillsolutionedge(ADR::Graph& g, ADR::Graph::edge_descriptor e, bool log = false) { lgraphkilledge(g, e, m_graph[e].get_solution(), log); }
	void lgraphkillalledges(ADR::Graph& g, ADR::Graph::edge_descriptor e, bool log = false);
	void lgraphkilledge(ADR::Graph::edge_descriptor e, unsigned int pi, bool log = false) { lgraphkilledge(m_graph, e, pi, log); }
	void lgraphkillsolutionedge(ADR::Graph::edge_descriptor e, bool log = false) { lgraphkilledge(e, m_graph[e].get_solution(), log); }
	void lgraphkillalledges(ADR::Graph::edge_descriptor e, bool log = false) { lgraphkillalledges(m_graph, e, log); }
	void lgraphadd(const ADR::Object::ptr_t& p);
	void lgraphadd(const ADR::Database::findresults_t& r);
	std::string lgraphstat(void) const;
	bool lgraphload(const Rect& bbox);
	static bool lgraphcheckintersect(const Point& pt0, const Point& pt1, const Rect& bbox);
	bool lgraphexcluderegions(void);
	void lgraphedgemetric(void);
	bool lgraphaddsidstar(void);
	void lgraphcompositeedges(void);
	bool lgraphapplymandatoryinbound(ADR::Graph& g, const ADR::FlightRestrictionTimeSlice& tsr, bool logrule);
	bool lgraphapplymandatoryoutbound(ADR::Graph& g, const ADR::FlightRestrictionTimeSlice& tsr, bool logrule);
	bool lgraphapplyforbiddenpoint(ADR::Graph& g, const ADR::FlightRestrictionTimeSlice& tsr, bool logrule);
	void lgraphapplylocalrules(void);
	void lgraphcrossing(void);
	static void lgraphedgemetric(double& cruisemetric, double& levelchgmetric, double& trknmi,
				     ADR::timetype_t& cruisetime, ADR::timetype_t& levelchgtime,
				     unsigned int piu, unsigned int piv,
				     const ADR::GraphEdge& ee, const Performance& perf);
	void lgraphupdatemetric(ADR::Graph& g, LRoute& r);
	void lgraphupdatemetric(LRoute& r) { lgraphupdatemetric(m_graph, r); }
	void lgraphclearcurrent(void);
	bool lgraphsetcurrent(const LRoute& r);
	void lgraphmodified_noedgekill(void);
	void lgraphmodified(void);
	bool lgraphisinsolutiontree(const LRoute& r) const;
	bool lgraphinsertsolutiontree(const LRoute& r);
	std::string lgraphprint(ADR::Graph& g, const LRoute& r);
	std::string lgraphprint(const LRoute& r) { return lgraphprint(m_graph, r); }
	LRoute lgraphdijkstra(ADR::Graph& g, const LVertexLevelTime& u, const LVertexLevel& v, const LRoute& baseroute = LRoute(),
			      bool solutionedgeonly = false, LGMandatory mandatory = LGMandatory());
	LRoute lgraphdijkstra(const LVertexLevelTime& u, const LVertexLevel& v, const LRoute& baseroute = LRoute(),
			      bool solutionedgeonly = false, LGMandatory mandatory = LGMandatory()) {
		return lgraphdijkstra(m_graph, u, v, baseroute, solutionedgeonly, mandatory);
	}
	LRoute lgraphnextsolution(void);
	void lgraphroute(void);
	bool lgraphroutelocaltfr(void);
	void lgraphlogrule(const ADR::FlightRestrictionTimeSlice& tsrule, ADR::timetype_t tm);
	void lgraphloglocaltfr(const ADR::RestrictionResult& rule, bool logvalidation = false);
	bool lgraphlocaltfrcheck(ADR::Graph& g, ADR::RestrictionResults& res);
	bool lgraphlocaltfrcheck(ADR::RestrictionResults& res) { return lgraphlocaltfrcheck(m_graph, res); }
	void lgraphmandatoryroute(LGMandatoryAlternative& m, const ADR::RuleSequence& alt);
	LGMandatory lgraphmandatoryroute(const ADR::RestrictionResults& res);
	void lgraphsubmitroute(void);
	bool lgraphtfrrulekillforbidden(ADR::Graph& g, const ADR::RuleSegment& seq, bool log = true);
	bool lgraphtfrrulekillforbidden(const ADR::RuleSegment& seq, bool log = true) {
		return lgraphtfrrulekillforbidden(m_graph, seq, log);
	}
	IntervalSet<int32_t> lgraphcrossinggate(const ADR::RuleSegment& seq, const ADR::RuleSegment& cc);
	bool lgraphtfrruleresult(ADR::Graph& g, const ADR::RestrictionResult& rule, const ADR::FlightPlan& fpl);
	bool lgraphtfrruleresult(const ADR::RestrictionResult& rule, const ADR::FlightPlan& fpl) { return lgraphtfrruleresult(m_graph, rule, fpl); }
	void lgraphtfrmessage(const ADR::Message& msg, const ADR::FlightPlan& fpl);
	std::string lgraphvertexname(const ADR::Graph& g, ADR::Graph::vertex_descriptor v);
	std::string lgraphvertexname(const ADR::Graph& g, ADR::Graph::vertex_descriptor v, unsigned int pi);
	std::string lgraphvertexname(const ADR::Graph& g, ADR::Graph::vertex_descriptor v, unsigned int pi0, unsigned int pi1);
	std::string lgraphvertexname(const ADR::Graph& g, const LVertexLevel& v) { return lgraphvertexname(g, v.get_vertex(), v.get_perfindex()); }
	std::string lgraphvertexname(ADR::Graph::vertex_descriptor v) { return lgraphvertexname(m_graph, v); }
	std::string lgraphvertexname(ADR::Graph::vertex_descriptor v, unsigned int pi) { return lgraphvertexname(m_graph, v, pi); }
	std::string lgraphvertexname(ADR::Graph::vertex_descriptor v, unsigned int pi0, unsigned int pi1) { return lgraphvertexname(m_graph, v, pi0, pi1); }
	std::string lgraphvertexname(const LVertexLevel& v) { return lgraphvertexname(v.get_vertex(), v.get_perfindex()); }
	static const std::string& lgraphawyname(const ADR::Object::const_ptr_t& awy, ADR::timetype_t tm);
	const std::string& lgraphawyname(const ADR::Object::const_ptr_t& awy) const;
	void lgraphlogkillalledges(const ADR::Graph& g, ADR::Graph::edge_descriptor e);
	void lgraphlogkilledge(const ADR::Graph& g, ADR::Graph::edge_descriptor e, unsigned int pi);
	void lgraphlogkillsolutionedge(const ADR::Graph& g, ADR::Graph::edge_descriptor e) { lgraphlogkilledge(e, m_graph[e].get_solution()); }
	void lgraphlogkillalledges(ADR::Graph::edge_descriptor e) { lgraphlogkillalledges(m_graph, e); }
	void lgraphlogkilledge(ADR::Graph::edge_descriptor e, unsigned int pi) { lgraphlogkilledge(m_graph, e, pi); }
	void lgraphlogkillsolutionedge(ADR::Graph::edge_descriptor e) { lgraphlogkillsolutionedge(m_graph, e); }
	void lgraphlogaddedge(const ADR::Graph& g, ADR::Graph::edge_descriptor e, unsigned int pi);
	void lgraphlogaddedge(ADR::Graph::edge_descriptor e, unsigned int pi) { lgraphlogaddedge(m_graph, e, pi); }
	void lgraphlogrenameedge(const ADR::Graph& g, ADR::Graph::edge_descriptor e, unsigned int pi, const ADR::UUID& awyname);
	void lgraphlogrenameedge(ADR::Graph::edge_descriptor e, unsigned int pi, const ADR::UUID& awyname) { lgraphlogrenameedge(m_graph, e, pi, awyname); }
	void lgraphlogscaleedge(const ADR::Graph& g, ADR::Graph::edge_descriptor e, unsigned int pi, double scale);
	void lgraphlogscaleedge(ADR::Graph::edge_descriptor e, unsigned int pi, double scale) { lgraphlogscaleedge(m_graph, e, pi, scale); }
	bool lgraphdisconnectvertex(const std::string& ident, int lvlfrom = 0, int lvlto = 600, bool intel = false);
	bool lgraphdisconnectsolutionvertex(ADR::Graph& g, const std::string& ident);
	bool lgraphdisconnectsolutionvertex(const std::string& ident) { return lgraphdisconnectsolutionvertex(m_graph, ident); }
	bool lgraphdisconnectsolutionvertex(ADR::Graph& g, const ADR::Object::const_ptr_t& wpt);
	bool lgraphdisconnectsolutionvertex(const ADR::Object::const_ptr_t& wpt) { return lgraphdisconnectsolutionvertex(m_graph, wpt); }
	bool lgraphdisconnectsolutionedge(ADR::Graph& g, const std::string& id0, const std::string& id1, const ADR::UUID& awyname = ADR::GraphEdge::matchall);
	bool lgraphdisconnectsolutionedge(const std::string& id0, const std::string& id1, const ADR::UUID& awyname = ADR::GraphEdge::matchall) { return lgraphdisconnectsolutionedge(m_graph, id0, id1, awyname); }
	bool lgraphdisconnectsolutionedge(ADR::Graph& g, const ADR::Object::const_ptr_t& wpt0, const ADR::Object::const_ptr_t& wpt1, const ADR::UUID& awyname = ADR::GraphEdge::matchall);
	bool lgraphdisconnectsolutionedge(const ADR::Object::const_ptr_t& wpt0, const ADR::Object::const_ptr_t& wpt1, const ADR::UUID& awyname = ADR::GraphEdge::matchall) { return lgraphdisconnectsolutionedge(m_graph, wpt0, wpt1, awyname); }
	bool lgraphscalesolutionvertex(ADR::Graph& g, const std::string& ident, double scale);
	bool lgraphscalesolutionvertex(const std::string& ident, double scale) { return lgraphscalesolutionvertex(m_graph, ident, scale); }
	bool lgraphscalesolutionvertex(ADR::Graph& g, const ADR::Object::const_ptr_t& wpt, double scale);
	bool lgraphscalesolutionvertex(const ADR::Object::const_ptr_t& wpt, double scale) { return lgraphscalesolutionvertex(m_graph, wpt, scale); }
	bool lgraphscalesolutionedge(ADR::Graph& g, const std::string& id0, const std::string& id1, double scale);
	bool lgraphscalesolutionedge(const std::string& id0, const std::string& id1, double scale) { return lgraphscalesolutionedge(m_graph, id0, id1, scale); }
	bool lgraphscalesolutionedge(ADR::Graph& g, const ADR::Object::const_ptr_t& wpt0, const ADR::Object::const_ptr_t& wpt1, double scale);
	bool lgraphscalesolutionedge(const ADR::Object::const_ptr_t& wpt0, const ADR::Object::const_ptr_t& wpt1, double scale) { return lgraphscalesolutionedge(m_graph, wpt0, wpt1, scale); }
	bool lgraphkilledge(const std::string& id0, const std::string& id1, const ADR::UUID& awymatch = ADR::GraphEdge::matchall,
			    int lvlfrom = 0, int lvlto = 999, bool bidir = false, bool solutiononly = false, bool todct = false);
	bool lgraphkilledge(const std::string& id0, const std::string& id1, const std::string& awyname,
			    int lvlfrom = 0, int lvlto = 999, bool bidir = false, bool solutiononly = false, bool todct = false);
	bool lgraphchangeoutgoing(const std::string& id, const ADR::UUID& awyname);
	bool lgraphchangeoutgoing(const std::string& id, const std::string& awyname);
	bool lgraphchangeincoming(const std::string& id, const ADR::UUID& awyname);
	bool lgraphchangeincoming(const std::string& id, const std::string& awyname);
	bool lgraphedgetodct(const std::string& awyname);
	bool lgraphedgetodct(const ADR::UUID& awy);
	bool lgraphkilloutgoing(const std::string& id);
	bool lgraphkillincoming(const std::string& id);
	bool lgraphkillsid(const std::string& id);
	bool lgraphkillstar(const std::string& id);
	bool lgraphkillsidedge(const std::string& id, int lvlfrom = 0, int lvlto = 999, bool solutiononly = false, bool todct = false);
	bool lgraphkillstaredge(const std::string& id, int lvlfrom = 0, int lvlto = 999, bool solutiononly = false, bool todct = false);
	bool lgraphkilledge(const ADR::UUID& awymatch, int lvlfrom = 0, int lvlto = 999, bool solutiononly = false, bool todct = false);
	bool lgraphkillclosesegments(const std::string& id0, const std::string& id1, const std::string& awyid,
				     const ADR::UUID& solutionmatch = ADR::GraphEdge::matchall);
	bool lgraphkillclosesegments(const std::string& id0, const std::string& id1, const ADR::Object::const_ptr_t& awy,
				     const ADR::UUID& solutionmatch = ADR::GraphEdge::matchall);
	bool lgraphsolutiongroundclearance(void);
	void lgraphlogtofile(void);
};


#endif /* CFMUAUTOROUTE51_H */
