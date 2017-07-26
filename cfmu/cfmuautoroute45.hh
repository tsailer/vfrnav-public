#ifndef CFMUAUTOROUTE45_H
#define CFMUAUTOROUTE45_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cfmuautoroute.hh"
#include "tfr.hh"

class CFMUAutoroute45 : public CFMUAutoroute {
public:
	CFMUAutoroute45();

	TrafficFlowRestrictions& get_tfr(void) { return m_tfr; }
	const TrafficFlowRestrictions& get_tfr(void) const { return m_tfr; }

	bool check_fplan(TrafficFlowRestrictions::Result& res, const FPlanRoute& route);

	virtual void preload(bool validator = true);
	virtual void reload(void);
	virtual void stop(statusmask_t sm);
	virtual void clear(void);

	virtual void precompute_graph(const Rect& bbox, const std::string& fn);

protected:
	typedef bool (CFMUAutoroute45::*parseresponse_t)(Glib::MatchInfo&);
	static const struct parsefunctions {
		const char *regex;
		parseresponse_t func;
	} parsefunctions[];
	static const char *ignoreregex[];

	virtual bool start_ifr(bool cont = false, bool optfuel = false);
	void logmessage(const TrafficFlowRestrictions::Message& msg);
	void loadtfr(void);
	void tfrpreparedct(void);

	void parse_response(void);
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
	bool parse_response_route179(Glib::MatchInfo& minfo);
	bool parse_response_prof50(Glib::MatchInfo& minfo);
	bool parse_response_prof193(Glib::MatchInfo& minfo);
	bool parse_response_prof194(Glib::MatchInfo& minfo);
	bool parse_response_prof195(Glib::MatchInfo& minfo);
	bool parse_response_prof195b(Glib::MatchInfo& minfo);
	bool parse_response_prof197(Glib::MatchInfo& minfo);
	bool parse_response_prof198(Glib::MatchInfo& minfo);
	bool parse_response_prof199(Glib::MatchInfo& minfo);
	bool parse_response_prof201(Glib::MatchInfo& minfo);
	bool parse_response_prof201b(Glib::MatchInfo& minfo);
	bool parse_response_prof204(Glib::MatchInfo& minfo);
	bool parse_response_prof204b(Glib::MatchInfo& minfo);
	bool parse_response_prof204c(Glib::MatchInfo& minfo);
	bool parse_response_prof204d(Glib::MatchInfo& minfo);
	bool parse_response_prof204e(Glib::MatchInfo& minfo);
	bool parse_response_prof205(Glib::MatchInfo& minfo);
	bool parse_response_prof205b(Glib::MatchInfo& minfo);
	bool parse_response_prof205c(Glib::MatchInfo& minfo);
	bool parse_response_prof206(Glib::MatchInfo& minfo);
	bool parse_response_efpm228(Glib::MatchInfo& minfo);
	bool parse_response_fail(Glib::MatchInfo& minfo);

	class AirspaceCache;

	class DctCache {
	public:
		typedef unsigned long pointid_t;
		typedef TrafficFlowRestrictions::DctParameters::AltRange::set_t set_t;

		class PointID {
		public:
			PointID(const std::string& ident = "", const Point& pt = Point(), pointid_t id = 0) : m_ident(ident), m_point(pt), m_id(id) {}
			const std::string& get_ident(void) const { return m_ident; }
			const Point& get_point(void) const { return m_point; }
			pointid_t get_id(void) const { return m_id; }
			bool operator<(const PointID& x) const;

		protected:
			std::string m_ident;
			Point m_point;
			pointid_t m_id;
		};

		class DctAlt {
		public:
			DctAlt(pointid_t pt0 = 0, pointid_t pt1 = 0, const set_t& alt = set_t(), bool dirty = false) : m_alt(alt), m_point0(pt0), m_point1(pt1), m_dirty(dirty) {}
			const set_t& get_alt(void) const { return m_alt; }
		        set_t& get_alt(void) { return m_alt; }
			pointid_t get_point0(void) const { return m_point0; }
			pointid_t get_point1(void) const { return m_point1; }
			bool operator<(const DctAlt& x) const;
			bool is_dirty(void) const { return m_dirty; }
			void set_dirty(bool x = true) { m_dirty = true; }

		protected:
			set_t m_alt;
			pointid_t m_point0;
			pointid_t m_point1;
			bool m_dirty;
		};

		DctCache(const std::string& path = "", const std::string& revision = "1900-01-01T00:00:00");

		std::string get_property(const std::string& key);
		void set_property(const std::string& key, const std::string& value);

		void load(const Rect& bbox);
		void flush(void);

		pointid_t find_point(const std::string& ident, const Point& pt);
		const DctAlt *find_dct(pointid_t pt0, pointid_t pt1) const;
		void add_dct(pointid_t pt0, pointid_t pt1, const set_t& alt);

		unsigned int get_nrpoints(void) const { return m_points.size(); }
		unsigned int get_nrdcts(void) const { return m_dcts.size(); }

	protected:
		sqlite3x::sqlite3_connection m_db;
		typedef std::set<PointID> points_t;
		points_t m_points;
		typedef std::set<DctAlt> dcts_t;
		dcts_t m_dcts;
		pointid_t m_pointid;
		pointid_t m_pointidalloc;
	};

	typedef uint32_t lgraphairwayindex_t;
	static const lgraphairwayindex_t airwaymatchall = 0x0003FFFFU;
	static const lgraphairwayindex_t airwaymatchnone = airwaymatchall - 1U;
	static const lgraphairwayindex_t airwaymatchawy = airwaymatchnone - 1U;
	static const lgraphairwayindex_t airwaymatchdctawy = airwaymatchawy - 1U;
	static const lgraphairwayindex_t airwaymatchdctawysidstar = airwaymatchdctawy - 1U;
	static const lgraphairwayindex_t airwaymatchawysidstar = airwaymatchdctawysidstar - 1U;
	static const lgraphairwayindex_t airwaymatchsidstar = airwaymatchawysidstar - 1U;
	static const lgraphairwayindex_t airwaysid = airwaymatchsidstar - 1U;
	static const lgraphairwayindex_t airwaystar = airwaysid - 1U;
	static const lgraphairwayindex_t airwaydct = airwaystar - 1U;
	typedef std::set<std::string> lgraphairwayidents_t;

	class LVertex {
	public:
		LVertex(Engine::DbObject::ptr_t obj = Engine::DbObject::ptr_t());

		bool is_airport(void) const;
		bool is_navaid(void) const;
		bool is_intersection(void) const;
		bool is_mapelement(void) const;

		bool get(AirportsDb::Airport& el) const;
		bool get(NavaidsDb::Navaid& el) const;
		bool get(WaypointsDb::Waypoint& el) const;
		bool get(MapelementsDb::Mapelement& el) const;

		bool set(FPlanWaypoint& el) const;
		unsigned int insert(FPlanRoute& route, uint32_t nr = ~0U) const;

		Engine::DbObject::ptr_t get_object(void) const { return m_obj; }
		const std::string& get_ident(void) const { return m_ident; }
		bool is_ident_valid(void) const;
		const Point& get_coord(void) const { return m_coord; }

	protected:
		Engine::DbObject::ptr_t m_obj;
		Point m_coord;
		std::string m_ident;
	};

	class LEdge {
	public:
		static const float maxmetric;
		static const float invalidmetric;
		static const lgraphairwayindex_t identmask    = 0x0003FFFFU;
		static const lgraphairwayindex_t levelsmask   = 0x01FC0000U;
		static const lgraphairwayindex_t solutionmask = 0xFE000000U;

		LEdge(unsigned int levels = 0, lgraphairwayindex_t id = airwaydct, float dist = maxmetric, float truetrack = 0);
		LEdge(const LEdge& x);
		~LEdge();
		LEdge& operator=(const LEdge& x);
		void swap(LEdge& x);
		void resize(unsigned int levels);
		unsigned int get_levels(void) const { return (m_ident & levelsmask) >> 18; }
		void set_ident(lgraphairwayindex_t id) { m_ident ^= (m_ident ^ id) & identmask; }
		lgraphairwayindex_t get_ident(void) const { return m_ident & identmask; }
		bool is_dct(void) const { return get_ident() == airwaydct; }
		static bool is_match(lgraphairwayindex_t edgeid, lgraphairwayindex_t id);
		bool is_match(lgraphairwayindex_t id) const { return is_match(get_ident(), id); }
		void clear_solution(void) { m_ident |= solutionmask; }
		void set_solution(unsigned int pi) { m_ident ^= ((m_ident ^ (pi << 25)) & solutionmask); }
		unsigned int get_solution(void) const { return (m_ident & solutionmask) >> 25; }
		bool is_solution(void) const { return !!(solutionmask & ~m_ident); }
		bool is_solution(unsigned int pi) const { lgraphairwayindex_t x(pi); x <<= 25; return !((x ^ m_ident) & solutionmask) && (solutionmask & ~x); }
		void set_dist(float dist = maxmetric) { m_dist = dist; }
		float get_dist(void) const { return m_dist; }
		void set_truetrack(float tt = 0) { m_truetrack = tt; }
		float get_truetrack(void) const { return m_truetrack; }
		void set_metric(unsigned int pi, float metric = invalidmetric);
		float get_metric(unsigned int pi) const;
		void set_solution_metric(float metric = invalidmetric) { set_metric(get_solution(), metric); }
		float get_solution_metric(void) const { return get_metric(get_solution()); }
		void set_all_metric(float metric = invalidmetric);
		void clear_metric(unsigned int pi) { set_metric(pi, invalidmetric); }
		void clear_solution_metric(void) { clear_metric(get_solution()); }
		void clear_metric(void) { set_all_metric(invalidmetric); }
		void scale_metric(float scale = 1, float offset = 0);
		bool is_valid(unsigned int pi) const;
		bool is_valid(void) const;
		bool is_solution_valid(void) const { return is_solution() && is_valid(get_solution()); }

	protected:
	        lgraphairwayindex_t m_ident;

	public:
		float m_dist;

	protected:
		float m_truetrack;
		float *m_metric;
	};

	class LGraph : public boost::adjacency_list<boost::multisetS, boost::vecS, boost::bidirectionalS, LVertex, LEdge> {
	public:
		bool is_valid_connection(unsigned int piu, unsigned int piv, edge_descriptor e) const;
		bool is_valid_connection(vertex_descriptor u, unsigned int piu, vertex_descriptor v, unsigned int piv, edge_descriptor e) const;
		bool is_valid_connection_precheck(unsigned int piu, unsigned int piv, edge_descriptor e) const;
		bool is_valid_connection_precheck(vertex_descriptor u, unsigned int piu, vertex_descriptor v, unsigned int piv, edge_descriptor e) const;
		std::pair<edge_descriptor,bool> find_edge(vertex_descriptor u, vertex_descriptor v, lgraphairwayindex_t awy = airwaymatchall) const;
		
	};

	typedef std::multimap<std::string,LGraph::vertex_descriptor> LVerticesIdentMap;

	class LVertexLevel {
	public:
		LVertexLevel(LGraph::vertex_descriptor v = 0, unsigned int pi = 0) : m_vertex(v), m_perfindex(pi) {}
		LGraph::vertex_descriptor get_vertex(void) const { return m_vertex; }
		void set_vertex(LGraph::vertex_descriptor v) { m_vertex = v; }
		unsigned int get_perfindex(void) const { return m_perfindex; }
		void set_perfindex(unsigned int pi) { m_perfindex = pi; }
		bool operator<(const LVertexLevel& x) const;
		bool operator>(const LVertexLevel& x) const { return x < *this; }
		bool operator==(const LVertexLevel& x) const;
		bool operator!=(const LVertexLevel& x) const { return !(*this == x); }

	protected:
		LGraph::vertex_descriptor m_vertex;
		unsigned int m_perfindex;
	};

	class LVertexLevels {
	public:
		LVertexLevels(LGraph::vertex_descriptor v = 0, unsigned int pi0 = 0, unsigned int pi1 = 0) : m_vertex(v), m_perfindex0(pi0), m_perfindex1(pi1) {}
		LGraph::vertex_descriptor get_vertex(void) const { return m_vertex; }
		void set_vertex(LGraph::vertex_descriptor v) { m_vertex = v; }
		unsigned int get_perfindex0(void) const { return m_perfindex0; }
		unsigned int get_perfindex1(void) const { return m_perfindex1; }
		void set_perfindex0(unsigned int pi) { m_perfindex0 = pi; }
		void set_perfindex1(unsigned int pi) { m_perfindex1 = pi; }
		bool operator<(const LVertexLevels& x) const;
		bool operator>(const LVertexLevels& x) const { return x < *this; }
		bool operator==(const LVertexLevels& x) const;
		bool operator!=(const LVertexLevels& x) const { return !(*this == x); }

	protected:
		LGraph::vertex_descriptor m_vertex;
		unsigned int m_perfindex0;
		unsigned int m_perfindex1;
	};

	class CompareVertexLevelsVertexOnly {
	public:
		bool operator()(const LVertexLevels& a, const LVertexLevels& b);
	};

	class LVertexDescEdge {
	public:
		LVertexDescEdge(unsigned int levels, LGraph::vertex_descriptor v = 0, lgraphairwayindex_t id = airwaydct,
				float dist = LEdge::maxmetric, float truetrack = 0) : m_edge(levels, id, dist, truetrack), m_vertex(v) {}
		LGraph::vertex_descriptor get_vertex(void) const { return m_vertex; }
		void set_vertex(LGraph::vertex_descriptor v) { m_vertex = v; }
		LEdge& get_edge(void) const { return const_cast<LEdge&>(m_edge); }
		bool operator<(const LVertexDescEdge& x) const { return get_vertex() < x.get_vertex(); }
		bool operator>(const LVertexDescEdge& x) const { return x < *this; }
		bool operator==(const LVertexDescEdge& x) const { return get_vertex() == x.get_vertex(); }
		bool operator!=(const LVertexDescEdge& x) const { return !(*this == x); }

	protected:
		LEdge m_edge;
		LGraph::vertex_descriptor m_vertex;
	};

	class LVertexEdge : public LVertexLevel {
	public:
		// v should be the target of e
		LVertexEdge(const LVertexLevel& v = LVertexLevel(), lgraphairwayindex_t e = airwaymatchnone) : LVertexLevel(v), m_edge(e) {}
		lgraphairwayindex_t get_edge(void) const { return m_edge; }
		bool is_match(lgraphairwayindex_t id) const { return LEdge::is_match(get_edge(), id); }
		bool operator<(const LVertexEdge& x) const;
		bool operator>(const LVertexEdge& x) const { return x < *this; }

	protected:
		lgraphairwayindex_t m_edge;
	};

	class LRoute : public std::vector <LVertexEdge> {
	public:
		LRoute(void) : m_metric(std::numeric_limits<double>::quiet_NaN()) {}

		bool operator<(const LRoute& x) const;
		void set_metric(double metric) { m_metric = metric; }
		double get_metric(void) const { return m_metric; }
		bool is_inf(void) const { return std::isinf(get_metric()); }
		bool is_nan(void) const { return std::isnan(get_metric()); }
		bool is_repeated_nodes(const LGraph& g) const;
		bool is_existing(const LGraph& g) const;
		std::pair<LGraph::edge_descriptor,bool> get_edge(size_type i, const LGraph& g) const;
		static std::pair<LGraph::edge_descriptor,bool> get_edge(const LVertexEdge& u, const LVertexEdge& v, const LGraph& g);
		static std::pair<LGraph::edge_descriptor,bool> get_edge(LGraph::vertex_descriptor u, const LVertexEdge& v, const LGraph& g);

	protected:
		double m_metric;
	};

	class LGraphSolutionTreeNode;
	class LGraphSolutionTreeNode : public std::map<LVertexEdge,LGraphSolutionTreeNode> {
	};

	class LEdgeAdd {
	public:
		LEdgeAdd(LGraph::vertex_descriptor v0 = 0, LGraph::vertex_descriptor v1 = 0, double metric = std::numeric_limits<double>::max());
		LGraph::vertex_descriptor get_v0(void) const { return m_v0; }
		LGraph::vertex_descriptor get_v1(void) const { return m_v1; }
		double get_metric(void) const { return m_metric; }
		bool operator<(const LEdgeAdd& x) const;

	protected:
		LGraph::vertex_descriptor m_v0;
		LGraph::vertex_descriptor m_v1;
		double m_metric;
	};

	class LGraphAwyEdgeFilter {
	public:
		LGraphAwyEdgeFilter(void) : m_lgraph(0), m_airway(airwaymatchnone) {}
		LGraphAwyEdgeFilter(LGraph& g, lgraphairwayindex_t awy) : m_lgraph(&g), m_airway(awy) {}
		template <typename E> bool operator()(const E& e) const { return (*m_lgraph)[e].is_match(m_airway); }

	protected:
		LGraph *m_lgraph;
		lgraphairwayindex_t m_airway;
	};

	class LGraphSolutionEdgeFilter {
	public:
		LGraphSolutionEdgeFilter(void) : m_lgraph(0) {}
		LGraphSolutionEdgeFilter(LGraph& g) : m_lgraph(&g) {}
		template <typename E> bool operator()(const E& e) const { return (*m_lgraph)[e].is_solution(); }

	protected:
		LGraph *m_lgraph;
	};

	class DijkstraCore;
	class LevelEdgeWeightMap;
	friend double get(const LevelEdgeWeightMap& wm, LGraph::edge_descriptor e);

	class LGMandatoryPoint {
	public:
		LGMandatoryPoint(LGraph::vertex_descriptor v = 0, unsigned int pi0 = 0, unsigned int pi1 = 0,
				 lgraphairwayindex_t awy = airwaymatchnone) : m_v(v), m_pi0(pi0), m_pi1(pi1), m_airway(awy) {}
	        LGraph::vertex_descriptor get_v(void) const { return m_v; }
	        unsigned int get_pi0(void) const { return m_pi0; }
	        unsigned int get_pi1(void) const { return m_pi1; }
		lgraphairwayindex_t get_airway(void) const { return m_airway; }

	protected:
		LGraph::vertex_descriptor m_v;
		unsigned int m_pi0;
		unsigned int m_pi1;
		lgraphairwayindex_t m_airway;
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

	class CFMUIntel {
	public:
		class ForbiddenPoint {
		public:
			ForbiddenPoint(const std::string& ident = "", const Point& pt = Point::invalid);
			const std::string& get_ident(void) const { return m_ident; }
			const Point& get_pt(void) const { return m_pt; }
			bool operator<(const ForbiddenPoint& x) const;
			bool is_valid(void) const;
			void save(sqlite3x::sqlite3_command& cmd) const;
			void load(sqlite3x::sqlite3_cursor& cursor);

		protected:
			std::string m_ident;
			Point m_pt;
		};

		class ForbiddenSegment {
		public:
			ForbiddenSegment(const std::string& fromident = "", const Point& frompt = Point::invalid, unsigned int fromalt = 0,
					 const std::string& toident = "", const Point& topt = Point::invalid, unsigned int toalt = 0,
					 lgraphairwayindex_t awy = airwaymatchnone, const std::string& awyident = "");
			const std::string& get_fromident(void) const { return m_fromident; }
			const std::string& get_toident(void) const { return m_toident; }
			const std::string& get_awyident(void) const { return m_awyident; }
			const Point& get_frompt(void) const { return m_frompt; }
			const Point& get_topt(void) const { return m_topt; }
			unsigned int get_fromalt(void) const { return m_fromalt; }
			unsigned int get_toalt(void) const { return m_toalt; }
		        lgraphairwayindex_t get_awyident(std::string& id) const;
			bool operator<(const ForbiddenSegment& x) const;
			bool is_valid(void) const;
			void save(sqlite3x::sqlite3_command& cmd) const;
			void load(sqlite3x::sqlite3_cursor& cursor);

		protected:
			std::string m_fromident;
			std::string m_toident;
			std::string m_awyident;
			Point m_frompt;
			Point m_topt;
			unsigned int m_fromalt;
			unsigned int m_toalt;
		};

		CFMUIntel();
		~CFMUIntel();
		void clear(void);
		void flush(void);
		void add(const ForbiddenPoint& p);
		void add(const ForbiddenSegment& s);
		void open(const std::string& maindir, const std::string& auxdir = "");
		void close(void);
		bool is_open(void) const;
		bool is_empty(void) const;
		void find(const Rect& bbox,
			  const sigc::slot<void,const ForbiddenPoint&>& ptfunc = sigc::slot<void,const ForbiddenPoint&>());
		void find(const Rect& bbox,
			  const sigc::slot<void,const ForbiddenSegment&>& segfunc = sigc::slot<void,const ForbiddenSegment&>());

	protected:
		sqlite3x::sqlite3_connection m_db;
		typedef std::set<ForbiddenPoint> forbiddenpoints_t;
		forbiddenpoints_t m_forbiddenpoints;
		typedef std::set<ForbiddenSegment> forbiddensegments_t;
		forbiddensegments_t m_forbiddensegments;
	};

	TrafficFlowRestrictions m_tfr;
	LGraph m_lgraph;
	typedef std::vector<std::string> lgraphairwaytable_t;
	lgraphairwaytable_t m_lgraphairwaytable;
	typedef std::map<std::string,lgraphairwayindex_t> lgraphairwayrevtable_t;
	lgraphairwayrevtable_t m_lgraphairwayrevtable;
	LVerticesIdentMap m_lverticesidentmap;
	typedef std::set<LVertexLevel> solutionvertices_t;
	solutionvertices_t m_solutionvertices;
	// Yen's k shortest loopless paths algorithm
	LGraphSolutionTreeNode m_solutiontree;
	typedef std::set<LRoute> lgraphsolutionpool_t;
	lgraphsolutionpool_t m_solutionpool;
	LGraph::vertex_descriptor m_vertexdep;
	LGraph::vertex_descriptor m_vertexdest;
	LGMandatory m_crossingmandatory;
	CFMUIntel m_cfmuintel;
	static const bool lgraphawyvertdct = false;
	static constexpr double localforbiddenpenalty = 1.1;

	void clearlgraph(void);
	lgraphairwayindex_t lgraphairway2index(const std::string& awy, bool create = false);
	const std::string& lgraphindex2airway(lgraphairwayindex_t idx, bool specialnames = false);
	LGraph::vertex_descriptor lgraphaddvertex(Engine::DbObject::ptr_t obj);
	typedef enum {
		setmetric_none,
		setmetric_dist,
		setmetric_metric
	} setmetric_t;
	void lgraphaddedge(LGraph& g, LEdge& ee, LGraph::vertex_descriptor u, LGraph::vertex_descriptor v, setmetric_t setmetric = setmetric_none, bool log = false);
	void lgraphaddedge(LEdge& ee, LGraph::vertex_descriptor u, LGraph::vertex_descriptor v, setmetric_t setmetric = setmetric_none, bool log = false) {
		lgraphaddedge(m_lgraph, ee, u, v, setmetric, log);
	}
	void lgraphkilledge(LGraph& g, LGraph::edge_descriptor e, unsigned int pi, bool log = false);
	void lgraphkillsolutionedge(LGraph& g, LGraph::edge_descriptor e, bool log = false) { lgraphkilledge(g, e, m_lgraph[e].get_solution(), log); }
	void lgraphkillalledges(LGraph& g, LGraph::edge_descriptor e, bool log = false);
	void lgraphkilledge(LGraph::edge_descriptor e, unsigned int pi, bool log = false) { lgraphkilledge(m_lgraph, e, pi, log); }
	void lgraphkillsolutionedge(LGraph::edge_descriptor e, bool log = false) { lgraphkilledge(e, m_lgraph[e].get_solution(), log); }
	void lgraphkillalledges(LGraph::edge_descriptor e, bool log = false) { lgraphkillalledges(m_lgraph, e, log); }
	LEdge lgraphbestedgemetric(LGraph::vertex_descriptor u, LGraph::vertex_descriptor v, lgraphairwayindex_t awy = airwaymatchall);
	bool lgraphkillemptyedges(void);
	bool lgraphload(const Rect& bbox);
	void lgraphkillinvalidsuper(void);
	static bool lgraphcheckintersect(const Point& pt0, const Point& pt1, const Rect& bbox);
	static bool lgraphcheckintersect(const Point& pt0, const Point& pt1, const Rect& bbox, const MultiPolygonHole& poly);
	void lgraphexcluderegions(void);
	void lgraphadddct(void);
	void lgraphkilllongdct(void);
	void lgraphedgemetric(void);
	bool lgraphaddsidstar(void);
	void lgraphapplycfmuintel(const Rect& bbox);
	void lgraphapplycfmuintelpt(const CFMUIntel::ForbiddenPoint& p);
	void lgraphapplycfmuintelseg(const CFMUIntel::ForbiddenSegment& s);
	void lgraphcrossing(void);
	static void lgraphedgemetric(double& cruisemetric, double& levelchgmetric, double& trknmi, unsigned int piu, unsigned int piv,
				     const LEdge& ee, const Performance& perf);
	void lgraphupdatemetric(LGraph& g, LRoute& r);
	void lgraphupdatemetric(LRoute& r) { lgraphupdatemetric(m_lgraph, r); }
	void lgraphclearcurrent(void);
	bool lgraphsetcurrent(const LRoute& r);
	void lgraphmodified(void);
	bool lgraphisinsolutiontree(const LRoute& r) const;
	bool lgraphinsertsolutiontree(const LRoute& r);
	std::string lgraphprint(LGraph& g, const LRoute& r);
	std::string lgraphprint(const LRoute& r) { return lgraphprint(m_lgraph, r); }
	LRoute lgraphdijkstra(LGraph& g, const LVertexLevel& u, const LVertexLevel& v, const LRoute& baseroute = LRoute(),
			      bool solutionedgeonly = false, LGMandatory mandatory = LGMandatory());
	LRoute lgraphdijkstra(const LVertexLevel& u, const LVertexLevel& v, const LRoute& baseroute = LRoute(),
			      bool solutionedgeonly = false, LGMandatory mandatory = LGMandatory()) {
		return lgraphdijkstra(m_lgraph, u, v, baseroute, solutionedgeonly, mandatory);
	}
	LRoute lgraphnextsolution(void);
	void lgraphroute(void);
	bool lgraphroutelocaltfr(void);
	static std::string lgraphlocaltfrseqstr(const TrafficFlowRestrictions::RuleResult::Alternative::Sequence& el);
	void lgraphloglocaltfr(const TrafficFlowRestrictions::RuleResult& rule, bool logvalidation = false);
	bool lgraphlocaltfrcheck(TrafficFlowRestrictions::Result& res);
	LGMandatory lgraphmandatoryroute(const TrafficFlowRestrictions::Result& res);
	void lgraphsubmitroute(void);
	bool lgraphtfrrulekillforbidden(LGraph& g, const TrafficFlowRestrictions::RuleResult::Alternative::Sequence& seq, bool log = true);
	bool lgraphtfrrulekillforbidden(const TrafficFlowRestrictions::RuleResult::Alternative::Sequence& seq, bool log = true) {
		return lgraphtfrrulekillforbidden(m_lgraph, seq, log);
	}
	IntervalSet<int32_t> lgraphcrossinggate(const TrafficFlowRestrictions::RuleResult::Alternative::Sequence& seq,
						const TrafficFlowRestrictions::RuleResult::Alternative::Sequence& cc);
	bool lgraphtfrruleresult(const TrafficFlowRestrictions::RuleResult& rule, const TrafficFlowRestrictions::Fpl& fpl);
	void lgraphtfrmessage(const TrafficFlowRestrictions::Message& msg, const TrafficFlowRestrictions::Fpl& fpl);
	std::string lgraphvertexname(const LGraph& g, LGraph::vertex_descriptor v);
	std::string lgraphvertexname(const LGraph& g, LGraph::vertex_descriptor v, unsigned int pi);
	std::string lgraphvertexname(const LGraph& g, LGraph::vertex_descriptor v, unsigned int pi0, unsigned int pi1);
	std::string lgraphvertexname(const LGraph& g, const LVertexLevel& v) { return lgraphvertexname(g, v.get_vertex(), v.get_perfindex()); }
	std::string lgraphvertexname(LGraph::vertex_descriptor v) { return lgraphvertexname(m_lgraph, v); }
	std::string lgraphvertexname(LGraph::vertex_descriptor v, unsigned int pi) { return lgraphvertexname(m_lgraph, v, pi); }
	std::string lgraphvertexname(LGraph::vertex_descriptor v, unsigned int pi0, unsigned int pi1) { return lgraphvertexname(m_lgraph, v, pi0, pi1); }
	std::string lgraphvertexname(const LVertexLevel& v) { return lgraphvertexname(v.get_vertex(), v.get_perfindex()); }
	void lgraphlogkillalledges(const LGraph& g, LGraph::edge_descriptor e);
	void lgraphlogkilledge(const LGraph& g, LGraph::edge_descriptor e, unsigned int pi);
	void lgraphlogkillsolutionedge(const LGraph& g, LGraph::edge_descriptor e) { lgraphlogkilledge(e, m_lgraph[e].get_solution()); }
	void lgraphlogkillalledges(LGraph::edge_descriptor e) { lgraphlogkillalledges(m_lgraph, e); }
	void lgraphlogkilledge(LGraph::edge_descriptor e, unsigned int pi) { lgraphlogkilledge(m_lgraph, e, pi); }
	void lgraphlogkillsolutionedge(LGraph::edge_descriptor e) { lgraphlogkillsolutionedge(m_lgraph, e); }
	void lgraphlogaddedge(const LGraph& g, LGraph::edge_descriptor e, unsigned int pi);
	void lgraphlogaddedge(LGraph::edge_descriptor e, unsigned int pi) { lgraphlogaddedge(m_lgraph, e, pi); }
	void lgraphlogrenameedge(const LGraph& g, LGraph::edge_descriptor e, unsigned int pi, lgraphairwayindex_t awyname);
	void lgraphlogrenameedge(LGraph::edge_descriptor e, unsigned int pi, lgraphairwayindex_t awyname) { lgraphlogrenameedge(m_lgraph, e, pi, awyname); }
	void lgraphlogscaleedge(const LGraph& g, LGraph::edge_descriptor e, unsigned int pi, double scale);
	void lgraphlogscaleedge(LGraph::edge_descriptor e, unsigned int pi, double scale) { lgraphlogscaleedge(m_lgraph, e, pi, scale); }
	bool lgraphdisconnectvertex(const std::string& ident, int lvlfrom = 0, int lvlto = 600, bool intel = false);
	bool lgraphdisconnectsolutionvertex(const std::string& ident);
	bool lgraphdisconnectsolutionedge(const std::string& id0, const std::string& id1, lgraphairwayindex_t awyname = airwaymatchall);
	bool lgraphscalesolutionvertex(const std::string& ident, double scale);
	bool lgraphscalesolutionedge(const std::string& id0, const std::string& id1, double scale);
	bool lgraphmodifyedge(const std::string& id0, const std::string& id1, lgraphairwayindex_t awyremove,
			      lgraphairwayindex_t awyadd = airwaymatchnone, int lvlfrom = 0, int lvlto = 600, bool bidir = false);
	bool lgraphmodifyawysegment(const std::string& id0, const std::string& id1, lgraphairwayindex_t awymatch,
				    lgraphairwayindex_t awyrename = airwaymatchnone, int lvlfrom = 0, int lvlto = 600, bool bidir = false, bool solutiononly = false);
	bool lgraphmodifysolutionsegment(const std::string& id0, const std::string& id1, lgraphairwayindex_t awymatch,
					 lgraphairwayindex_t awyrename = airwaymatchnone, int lvlfrom = 0, int lvlto = 600, bool matchbothid = true);
	bool lgraphchangeoutgoing(const std::string& id, lgraphairwayindex_t awyname);
	bool lgraphchangeincoming(const std::string& id, lgraphairwayindex_t awyname);
	bool lgraphedgetodct(lgraphairwayindex_t awy);
	bool lgraphkilloutgoing(const std::string& id);
	bool lgraphkillincoming(const std::string& id);
	bool lgraphkillsid(const std::string& id);
	bool lgraphkillstar(const std::string& id);
	bool lgraphkillclosesegments(const std::string& id0, const std::string& id1, const std::string& awyid,
				     lgraphairwayindex_t solutionmatch = airwaymatchdctawysidstar);
	bool lgraphkillclosesegments(const std::string& id0, const std::string& id1, lgraphairwayindex_t awy,
				     lgraphairwayindex_t solutionmatch = airwaymatchdctawysidstar);
	bool lgraphsolutiongroundclearance(void);

	class GraphSerializeBase {
	public:
		GraphSerializeBase(void);

		const Rect& get_bbox(void) const { return m_bbox; }
		int16_t get_minlevel(void) const { return m_minlevel; }
		int16_t get_maxlevel(void) const { return m_maxlevel; }

		void set_bbox(const Rect& bbox) { m_bbox = bbox; }
		void set_minlevel(int16_t l) { m_minlevel = l; }
		void set_maxlevel(int16_t l) { m_maxlevel = l; }

		bool is_valid(void) const;

	protected:
		static const char signature[];
		Rect m_bbox;
		int16_t m_minlevel;
		int16_t m_maxlevel;
	};

	class GraphWrite : public GraphSerializeBase {
	public:
		GraphWrite(std::ostream& os);
		static bool is_save(void) { return true; }
		static bool is_load(void) { return false; }
		template <typename T> void io(const T& v);
		template <typename T> inline void ioint8(T v) { int8_t vv(v); io(vv); }
		template <typename T> inline void ioint16(T v) { int16_t vv(v); io(vv); }
		template <typename T> inline void ioint32(T v) { int32_t vv(v); io(vv); }
		template <typename T> inline void ioint64(T v) { int64_t vv(v); io(vv); }
		template <typename T> inline void iouint8(T v) { uint8_t vv(v); io(vv); }
		template <typename T> inline void iouint16(T v) { uint16_t vv(v); io(vv); }
		template <typename T> inline void iouint32(T v) { uint32_t vv(v); io(vv); }
		template <typename T> inline void iouint64(T v) { uint64_t vv(v); io(vv); }
		void header(void);

	protected:
		std::ostream& m_os;
	};

	class GraphRead : public GraphSerializeBase {
	public:
		GraphRead(std::istream& is);
		static bool is_save(void) { return false; }
		static bool is_load(void) { return true; }
		template <typename T> void io(T& v);
		template <typename T> inline void ioint8(T& v) { int8_t vv; io(vv); v = (T)vv; }
		template <typename T> inline void ioint16(T& v) { int16_t vv; io(vv); v = (T)vv; }
		template <typename T> inline void ioint32(T& v) { int32_t vv; io(vv); v = (T)vv; }
		template <typename T> inline void ioint64(T& v) { int64_t vv; io(vv); v = (T)vv; }
		template <typename T> inline void iouint8(T& v) { uint8_t vv; io(vv); v = (T)vv; }
		template <typename T> inline void iouint16(T& v) { uint16_t vv; io(vv); v = (T)vv; }
		template <typename T> inline void iouint32(T& v) { uint32_t vv; io(vv); v = (T)vv; }
		template <typename T> inline void iouint64(T& v) { uint64_t vv; io(vv); v = (T)vv; }
		void header(void);

	protected:
		std::istream& m_is;
	};

	class GraphFile;

	void lgraphsave(std::ostream& os, const Rect& bbox);
	void lgraphbinload(GraphRead& gr, const Rect& bbox);
	bool lgraphbinload(const Rect& bbox);
};


#endif /* CFMUAUTOROUTE45_H */
