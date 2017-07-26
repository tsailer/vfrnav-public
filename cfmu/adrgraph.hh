#ifndef ADRGRAPH_H
#define ADRGRAPH_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <limits>
#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>

#include "adr.hh"
#include "adrdb.hh"
#include "adraup.hh"
#include "adraerodrome.hh"
#include "adrunit.hh"
#include "adrpoint.hh"
#include "adrroute.hh"
#include "adrairspace.hh"
#include "interval.hh"

namespace ADR {

class FPLWaypoint;

class GraphVertex {
public:
	GraphVertex(timetype_t tm, const Object::const_ptr_t& obj);
	GraphVertex(const Object::const_ptr_t& obj = Object::const_ptr_t());

	bool is_airport(void) const;
	bool is_navaid(void) const;
	bool is_intersection(void) const;

	bool get(timetype_t tm, AirportsDb::Airport& el) const;
	bool get(timetype_t tm, NavaidsDb::Navaid& el) const;
	bool get(timetype_t tm, WaypointsDb::Waypoint& el) const;

	bool set(timetype_t tm, FPlanWaypoint& el) const;
	bool set(timetype_t tm, FPLWaypoint& el) const;
	unsigned int insert(timetype_t tm, FPlanRoute& route, uint32_t nr = ~0U) const;

	const Object::const_ptr_t& get_object(void) const { return m_obj; }
	const UUID& get_uuid(void) const;
	const PointIdentTimeSlice& get_timeslice(void) const;
	const std::string& get_ident(void) const { return get_timeslice().get_ident(); }
	static bool is_ident_valid(const std::string& id);
	bool is_ident_valid(void) const { return m_validident; }
	const Point& get_coord(void) const { return get_timeslice().get_coord(); }

protected:
	Object::const_ptr_t m_obj;
	uint8_t m_tsindex;
	bool m_validident;
};

class GraphEdge {
public:
	static const float maxmetric;
	static const float invalidmetric;
	static const boost::uuids::uuid matchalluuid;
	static const UUID matchall;

	typedef enum {
		matchcomp_all,
		matchcomp_segment,
		matchcomp_composite,
		matchcomp_route
	} matchcomp_t;

	GraphEdge(timetype_t tm, const Object::const_ptr_t& obj, unsigned int levels = 0);
	GraphEdge(unsigned int levels = 0, float dist = maxmetric, float truetrack = 0);
	GraphEdge(const GraphEdge& x);
	~GraphEdge();
	GraphEdge& operator=(const GraphEdge& x);
	void swap(GraphEdge& x);
	void resize(unsigned int levels);
	unsigned int get_levels(void) const { return m_levels; }
	bool is_dct(void) const { return !get_object(); }
	bool is_awy(bool allowcomposite = false) const;
	bool is_sid(bool allowcomposite = false) const;
	bool is_star(bool allowcomposite = false) const;
	bool is_composite(void) const;
	FPlanWaypoint::pathcode_t get_pathcode(void) const;
	bool is_match(const Object::const_ptr_t& p) const;
	bool is_match(const UUID& uuid) const;
	bool is_match(matchcomp_t mc) const;
	static Object::ptr_t get_matchall(void);
	void clear_solution(void) { m_solution = (uint8_t)~0U; }
	void set_solution(unsigned int pi) { m_solution = pi; }
	unsigned int get_solution(void) const { return m_solution; }
	bool is_solution(void) const { return m_solution != (uint8_t)~0U; }
	bool is_solution(unsigned int pi) const { return m_solution == pi && m_solution != (uint8_t)~0U; }
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
	void set_metric_seg(const ConditionalAvailability& condavail, const TimeTableEval& tte, int level, int levelinc);
	void set_metric_dct(int level, int levelinc, int32_t terrainelev, int32_t corridor5elev);
	bool is_valid(unsigned int pi) const;
	bool is_valid(void) const;
	bool is_solution_valid(void) const { return is_solution() && is_valid(get_solution()); }

	// FIXME: check time
	bool is_overlap(int32_t alt0, int32_t alt1) const;
	bool is_inside(int32_t alt0, int32_t alt1) const;
	bool is_inside(int32_t alt0, int32_t alt1, timetype_t tm, const TimeTableSpecialDateEval& ttsde) const;
	IntervalSet<int32_t> get_altrange(void) const;
	IntervalSet<int32_t> get_altrange(timetype_t tm) const;
	IntervalSet<int32_t> get_altrange(const ConditionalAvailability& condavail, const TimeTableEval& tte) const;
	IntervalSet<int32_t> get_altrange(const ConditionalAvailability& condavail, const TimeTableEval& tte, timetype_t& tuntil) const;

	bool is_forward(void) const { return !(m_tsindex & 0x80); }
	bool is_backward(void) const { return !is_forward(); }
	void set_backward(bool x = true) { if (x) m_tsindex |= 0x80; else m_tsindex &= ~0x80; }
	void set_forward(bool x = true) { set_backward(!x); }

	bool is_noroute(void) const { return !!(m_routetsindex & 0x80); }
	void set_noroute(bool x = true) { if (x) m_routetsindex |= 0x80; else m_routetsindex &= ~0x80; }

	const Object::const_ptr_t& get_object(void) const { return m_obj; }
	const UUID& get_uuid(void) const;
	const SegmentTimeSlice& get_timeslice(void) const;
	const Object::ptr_t& get_start_object(void) const { return get_timeslice().get_start().get_obj(); }
	const Object::ptr_t& get_end_object(void) const { return get_timeslice().get_end().get_obj(); }
	const Object::const_ptr_t& get_route_object(bool allowcomposite = false) const;
	const IdentTimeSlice& get_route_timeslice(bool allowcomposite = false) const;
	const UUID& get_route_uuid(bool allowcomposite = false) const;
	const std::string& get_route_ident(bool allowcomposite = false) const { return get_route_timeslice(allowcomposite).get_ident(); }
	const std::string& get_route_ident_or_dct(bool allowcomposite = false) const;
	RouteSegmentLink get_routesegmentlink(void) const;

protected:
	Object::const_ptr_t m_obj;

public:
	float m_dist;

protected:
	float m_truetrack;
	float *m_metric;
	uint8_t m_tsindex;
	uint8_t m_routetsindex;
	uint8_t m_levels;
	uint8_t m_solution;

	unsigned int get_tsindex(void) const { return m_tsindex & 0x7f; }
	unsigned int get_routetsindex(void) const { return m_routetsindex & 0x7f; }
	IntervalSet<int32_t> get_altrange(const SegmentTimeSlice& seg) const;
};

class Graph : public boost::adjacency_list<boost::multisetS, boost::vecS, boost::bidirectionalS, GraphVertex, GraphEdge> {
public:
	class UUIDSort {
	public:
		bool operator()(const Object::const_ptr_t& a, const Object::const_ptr_t& b) const {
			if (!b)
				return false;
			if (!a)
				return true;
			return a->get_uuid() < b->get_uuid();
		}
	};

	typedef std::set<Object::const_ptr_t, UUIDSort> objectset_t;

	static const bool restartedgescan = true;

protected:
	typedef std::map<Object::const_ptr_t, objectset_t, UUIDSort> dependson_t;
	typedef std::map<std::string, objectset_t> nameindex_t;

public:
	typedef boost::adjacency_list<boost::multisetS, boost::vecS, boost::bidirectionalS, GraphVertex, GraphEdge> base_t;

	static const vertex_descriptor invalid_vertex_descriptor = (vertex_descriptor)~0UL;

	class ObjectEdgeFilter {
	public:
		ObjectEdgeFilter(void) : m_graph(0) {}
		ObjectEdgeFilter(const Graph& g, const Object::const_ptr_t& p, GraphEdge::matchcomp_t mc) : m_graph(&g), m_p(p), m_matchcomp(mc) {}
		template <typename E> bool operator()(const E& e) const {
			const GraphEdge& ee((*m_graph)[e]);
			return ee.is_match(m_p) && ee.is_match(m_matchcomp);
		}

	protected:
		const Graph *m_graph;
		Object::const_ptr_t m_p;
		GraphEdge::matchcomp_t m_matchcomp;
	};

	class UUIDEdgeFilter {
	public:
		UUIDEdgeFilter(void) : m_graph(0) {}
		UUIDEdgeFilter(const Graph& g, const UUID& uuid, GraphEdge::matchcomp_t mc) : m_graph(&g), m_uuid(uuid), m_matchcomp(mc) {}
		template <typename E> bool operator()(const E& e) const {
			const GraphEdge& ee((*m_graph)[e]);
			return ee.is_match(m_uuid) && ee.is_match(m_matchcomp);
		}

	protected:
		const Graph *m_graph;
		UUID m_uuid;
		GraphEdge::matchcomp_t m_matchcomp;
	};

	class IdentEdgeFilter {
	public:
		IdentEdgeFilter(void) : m_graph(0) {}
		IdentEdgeFilter(const Graph& g, const std::string& id, GraphEdge::matchcomp_t mc) : m_graph(&g), m_ident(id), m_matchcomp(mc) {}
		template <typename E> bool operator()(const E& e) const {
			const GraphEdge& ee((*m_graph)[e]);
			return ee.get_route_ident(true) == m_ident && ee.is_match(m_matchcomp);
		}

	protected:
		const Graph *m_graph;
		std::string m_ident;
		GraphEdge::matchcomp_t m_matchcomp;
	};

	class SolutionEdgeFilter {
	public:
		SolutionEdgeFilter(void) : m_graph(0) {}
		SolutionEdgeFilter(const Graph& g) : m_graph(&g) {}
		template <typename E> bool operator()(const E& e) const { return (*m_graph)[e].is_solution(); }

	protected:
		const Graph *m_graph;
	};

	class AirwayAltEdgeFilter {
	public:
		AirwayAltEdgeFilter(void) : m_graph(0), m_alt0(0), m_alt1(0) {}
		AirwayAltEdgeFilter(const Graph& g, int32_t alt0, int32_t alt1) : m_graph(&g), m_alt0(alt0), m_alt1(alt1) {}
		template <typename E> bool operator()(const E& e) const { const GraphEdge& ee((*m_graph)[e]); return ee.is_inside(m_alt0, m_alt1) && ee.is_awy(); }

	protected:
		const Graph *m_graph;
		int32_t m_alt0;
		int32_t m_alt1;
	};

	class TimeEdgeFilter {
	public:
		TimeEdgeFilter(void) : m_graph(0) {}
		TimeEdgeFilter(const Graph& g, timetype_t tm, const Rect& bbox = Rect(Point::invalid, Point::invalid)) : m_graph(&g), m_tm(tm), m_bbox(bbox) {}
		template <typename E> bool operator()(const E& e) const {
			const GraphEdge& ee((*m_graph)[e]);
			const Object::const_ptr_t& p(ee.get_object());
			if (!p)
				return true;
			const SegmentTimeSlice& ts(p->operator()(m_tm).as_segment());
			if (!ts.is_valid())
				return false;
			const GraphVertex& v0((*m_graph)[boost::source(e, *m_graph)]);
			const GraphVertex& v1((*m_graph)[boost::target(e, *m_graph)]);
			if (ee.is_forward()) {
				if (v0.get_uuid() != ts.get_start() ||
				    v1.get_uuid() != ts.get_end())
					return false;
			} else {
				if (v1.get_uuid() != ts.get_start() ||
				    v0.get_uuid() != ts.get_end())
					return false;
			}
			if (m_bbox.get_southwest().is_invalid() || m_bbox.get_northeast().is_invalid())
				return true;
			const PointIdentTimeSlice& ts0(v0.get_object()->operator()(m_tm).as_point());
			const PointIdentTimeSlice& ts1(v1.get_object()->operator()(m_tm).as_point());
			if (ts0.is_valid() && ts1.is_valid() && m_bbox.is_inside(ts0.get_coord()) && m_bbox.is_inside(ts1.get_coord()))
				return true;
			return false;
		}

	protected:
		const Graph *m_graph;
		timetype_t m_tm;
		Rect m_bbox;
	};

	class AltrangeTimeEdgeFilter : public TimeEdgeFilter {
	public:
		AltrangeTimeEdgeFilter(void) : TimeEdgeFilter() {}
		AltrangeTimeEdgeFilter(const Graph& g, timetype_t tm, const IntervalSet<int32_t>& ar, const Rect& bbox = Rect(Point::invalid, Point::invalid)) : TimeEdgeFilter(g, tm, bbox), m_altrange(ar) {}
		template <typename E> bool operator()(const E& e) const {
			if (!TimeEdgeFilter::operator()(e))
				return false;
			IntervalSet<int32_t> ar((*m_graph)[e].get_altrange(m_tm));
			ar &= m_altrange;
			return !ar.is_empty();
		}

	protected:
		IntervalSet<int32_t> m_altrange;
	};

	class TimeEdgeDistanceMap {
	public:
		typedef Graph::edge_descriptor key_type;
		typedef double value_type;
		typedef const double& reference;
		typedef boost::readable_property_map_tag category;

		TimeEdgeDistanceMap(const Graph& g, timetype_t tm) : m_graph(g), m_tm(tm) {}
		const Graph& get_graph(void) const { return m_graph; }
		timetype_t get_time(void) const { return m_tm; }

	protected:
		const Graph& m_graph;
		timetype_t m_tm;
	};

	void swap(Graph& x);
	void clear(void);

	typedef std::pair<objectset_t::const_iterator, objectset_t::const_iterator> findresult_t;
	findresult_t find_ident(const std::string& ident) const;
	findresult_t find_dependson(const Object::const_ptr_t& p) const;
        findresult_t find_dependson(const UUID& u) const;
	unsigned int add(timetype_t tm, const Object::const_ptr_t& p, unsigned int levels = 0);
	unsigned int add(const Object::const_ptr_t& p, unsigned int levels = 0);
	template <typename T> static bool is_metric_invalid(T m) { return std::isnan(m) || std::isinf(m) || m == std::numeric_limits<T>::max(); }
	bool is_valid_connection(unsigned int piu, unsigned int piv, edge_descriptor e) const;
	bool is_valid_connection(vertex_descriptor u, unsigned int piu, vertex_descriptor v, unsigned int piv, edge_descriptor e) const;
	bool is_valid_connection_precheck(unsigned int piu, unsigned int piv, edge_descriptor e) const;
	bool is_valid_connection_precheck(vertex_descriptor u, unsigned int piu, vertex_descriptor v, unsigned int piv, edge_descriptor e) const;
	std::pair<edge_descriptor,bool> find_edge(vertex_descriptor u, vertex_descriptor v, const Object::const_ptr_t& p) const;
	std::pair<edge_descriptor,bool> find_edge(vertex_descriptor u, vertex_descriptor v, const UUID& uuid) const;
	std::pair<vertex_descriptor,bool> find_vertex(const Object::const_ptr_t& p) const;
	std::pair<vertex_descriptor,bool> find_vertex(const UUID& uuid) const;
	unsigned int kill_empty_edges(void);
	std::ostream& print(std::ostream& os) const;

protected:
	nameindex_t m_nameindex;
	dependson_t m_dependson;
	typedef std::map<Object::const_ptr_t, vertex_descriptor, UUIDSort> vertexmap_t;
	vertexmap_t m_vertexmap;

	static const objectset_t empty_objectset;
	bool add_dep(const Object::const_ptr_t& p, const Object::const_ptr_t& pdep);
};

Graph::TimeEdgeDistanceMap::value_type get(const Graph::TimeEdgeDistanceMap& m, Graph::TimeEdgeDistanceMap::key_type key);

};

#endif /* ADRGRAPH_H */
