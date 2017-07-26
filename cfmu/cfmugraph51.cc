#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fstream>
#include <iostream>
#include <iomanip>

#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/johnson_all_pairs_shortest.hpp>
#include <boost/graph/floyd_warshall_shortest.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/uuid/string_generator.hpp>

#include "cfmuautoroute51.hh"
#include "interval.hh"

const bool CFMUAutoroute51::lgraphawyvertdct;
constexpr double CFMUAutoroute51::localforbiddenpenalty;

int CFMUAutoroute51::LVertexLevel::compare(const LVertexLevel& x) const
{
	if (get_vertex() < x.get_vertex())
		return -1;
	if (x.get_vertex() < get_vertex())
		return 1;
	if (get_perfindex() < x.get_perfindex())
		return -1;
	if (x.get_perfindex() < get_perfindex())
		return 1;
	return 0;
}

int CFMUAutoroute51::LVertexLevelTime::compare(const LVertexLevelTime& x) const
{
	{
		int c(LVertexLevel::compare(x));
		if (c)
			return c;
	}
	if (get_time() < x.get_time())
		return -1;
	if (x.get_time() < get_time())
		return 1;
	return 0;
}

int CFMUAutoroute51::LVertexEdge::compare(const LVertexEdge& x) const
{
	{
		int c(LVertexLevelTime::compare(x));
		if (c)
			return c;
	}
	const ADR::UUID& a(get_uuid());
	const ADR::UUID& b(get_uuid());
	if (a < b)
		return -1;
	if (b < a)
		return 1;
	return 0;
}

int CFMUAutoroute51::LVertexEdge::compare_notime(const LVertexEdge& x) const
{
	{
		int c(LVertexLevelTime::compare_notime(x));
		if (c)
			return c;
	}
	const ADR::UUID& a(get_uuid());
	const ADR::UUID& b(get_uuid());
	if (a < b)
		return -1;
	if (b < a)
		return 1;
	return 0;
}

int CFMUAutoroute51::LRoute::compare(const LRoute& x) const
{
	if (get_metric() < x.get_metric())
		return -1;
	if (x.get_metric() < get_metric())
		return 1;
	for (size_type i(0), n(std::min(size(), x.size())); i < n; ++i) {
		int c((*this)[i].compare(x[i]));
		if (c)
			return c;
	}
	if (size() < x.size())
		return -1;
	if (x.size() < size())
		return 1;
	return 0;
}

bool CFMUAutoroute51::LRoute::is_repeated_nodes(const ADR::Graph& g) const
{
	typedef std::set<ADR::Graph::vertex_descriptor> nmap_t;
	nmap_t nmap;
	typedef std::set<std::string> imap_t;
	imap_t imap;
	for (const_iterator i(begin()), e(end()); i != e;) {
		ADR::Graph::vertex_descriptor vd(i->get_vertex());
		{
			std::pair<nmap_t::iterator,bool> x(nmap.insert(vd));
			if (!x.second)
				return true;
		}
		const ADR::GraphVertex& v(g[vd]);
		{
			std::pair<imap_t::iterator,bool> x(imap.insert(v.get_ident()));
			if (!x.second)
				return true;
		}
		++i;
	}
	return false;
}

bool CFMUAutoroute51::LRoute::is_existing(const ADR::Graph& g) const
{
	for (LRoute::size_type i(1); i < this->size(); ++i) {
		ADR::Graph::edge_descriptor e;
		bool ok;
		boost::tie(e, ok) = this->get_edge(i, g);
		if (!ok) {
			if (false)
				std::cerr << "LRoute::is_existing: path " << (i-1) << "->" << i << " no edge" << std::endl;
			return false;
		}
		unsigned int piu((*this)[i - 1].get_perfindex());
		unsigned int piv((*this)[i].get_perfindex());
		if (g[boost::source(e, g)].is_airport())
			piu = g[e].get_levels();
		if (g[boost::target(e, g)].is_airport())
			piv = g[e].get_levels();
		if (!g.is_valid_connection(piu, piv, e)) {
			if (false)
				std::cerr << "LRoute::is_existing: path " << (i-1) << '(' << piu << ")->"
					  << i << '(' << piv << ") invalid connection" << std::endl;
			return false;
		}
	}
	return true;
}

std::pair<ADR::Graph::edge_descriptor,bool> CFMUAutoroute51::LRoute::get_edge(const LVertexEdge& u, const LVertexEdge& v, const ADR::Graph& g)
{
	return get_edge(u.get_vertex(), v, g);
}

std::pair<ADR::Graph::edge_descriptor,bool> CFMUAutoroute51::LRoute::get_edge(ADR::Graph::vertex_descriptor u, const LVertexEdge& v, const ADR::Graph& g)
{
	return g.find_edge(u, v.get_vertex(), v.get_edge());
}

std::pair<ADR::Graph::edge_descriptor,bool> CFMUAutoroute51::LRoute::get_edge(size_type i, const ADR::Graph& g) const
{
	if (i < 1 || i >= size())
		return std::pair<ADR::Graph::edge_descriptor,bool>(ADR::Graph::edge_descriptor(), false);
	return get_edge((*this)[i - 1], (*this)[i], g);
}

void CFMUAutoroute51::logmessage(const ADR::Message& msg)
{
	std::ostringstream oss;
	oss << msg.get_type_char() << ':';
	if (!msg.get_rule().empty())
		oss << " R:" << msg.get_rule();
	if (!msg.get_vertexset().empty()) {
		oss << " V:";
		bool subseq(false);
		for (ADR::Message::set_t::const_iterator i(msg.get_vertexset().begin()), e(msg.get_vertexset().end()); i != e; ++i) {
			if (subseq)
				oss << ',';
			subseq = true;
			oss << *i;
		}
	}
	if (!msg.get_edgeset().empty()) {
		oss << " E:";
		bool subseq(false);
		for (ADR::Message::set_t::const_iterator i(msg.get_edgeset().begin()), e(msg.get_edgeset().end()); i != e; ++i) {
			if (subseq)
				oss << ',';
			subseq = true;
			oss << *i;
		}
	}
	oss << ' ' << msg.get_text();
	log_t lt(log_debug1);
	switch (msg.get_type()) {
	case ADR::Message::type_error:
		lt = log_normal;
		break;

	case ADR::Message::type_warning:
		lt = log_debug0;
		break;

	default:
		lt = log_debug1;
		break;
	}
	m_signal_log(lt, oss.str());
}

void CFMUAutoroute51::clearlgraph(void)
{
	m_graph.clear();
	m_solutionvertices.clear();
	m_vertexdep = m_vertexdest = 0;
	lgraphmodified();
}

void CFMUAutoroute51::lgraphaddedge(ADR::Graph& g, ADR::GraphEdge& edge, ADR::Graph::vertex_descriptor u, ADR::Graph::vertex_descriptor v, setmetric_t setmetric, bool log)
{
	if (!edge.is_valid())
		return;
	const unsigned int pis(m_performance.size());
	bool hasawy[pis];
	for (unsigned int pi = 0; pi < pis; ++pi)
		hasawy[pi] = false;
	ADR::Graph::edge_descriptor eedge, edct;
	bool foundedge(false), founddct(false);
	{
		const ADR::GraphVertex& uu(g[u]);
		const ADR::GraphVertex& vv(g[v]);
		bool first(true);
		ADR::Graph::out_edge_iterator e0, e1;
		for (boost::tie(e0, e1) = boost::out_edges(u, g); e0 != e1; ++e0) {
			if (boost::target(*e0, g) != v)
				continue;
			ADR::GraphEdge& ee(g[*e0]);
			if (first) {
				edge.set_dist(ee.get_dist());
				edge.set_truetrack(ee.get_truetrack());
				first = false;
			}
			if (ee.is_match(edge.get_object())) {
				foundedge = true;
				eedge = *e0;
			}
			if (ee.is_dct()) {
				founddct = true;
				edct = *e0;
			} else {
				for (unsigned int pi = 0; pi < pis; ++pi)
					if (ee.is_valid(pi))
						hasawy[pi] = true;
			}
		}
		if (first) {
			edge.set_dist(uu.get_coord().spheric_distance_nmi(vv.get_coord()));
		        edge.set_truetrack(uu.get_coord().spheric_true_course(vv.get_coord()));
		}
		for (unsigned int pi = 0; pi < pis; ++pi) {
			if (!edge.is_valid(pi))
				continue;
			if (!edge.is_dct()) {
				hasawy[pi] = true;
			} else if (hasawy[pi]) {
				edge.clear_metric(pi);
				continue;
			}
			if (setmetric == setmetric_dist || setmetric == setmetric_metric) {
				float m(edge.get_dist());
				if (edge.is_dct()) {
					m *= get_dctpenalty();
					m += get_dctoffset();
				}
				if (setmetric == setmetric_metric) {
					const Performance::Cruise& cruise(m_performance.get_cruise(pi));
 					if (m_windenable) {
						Wind<double> wnd(cruise.get_wind(uu.get_coord().halfway(vv.get_coord())));
						wnd.set_crs_tas(edge.get_truetrack(), cruise.get_tas());
						if (wnd.get_gs() >= 0.1)
							m *= cruise.get_tas() / wnd.get_gs();
					}
					m *= cruise.get_metricpernmi();
				}
				edge.set_metric(pi, m);
			}
		}
	}
	if (foundedge) {
		ADR::GraphEdge& ee(g[eedge]);
		for (unsigned int pi = 0; pi < pis; ++pi) {
			if (!edge.is_valid(pi))
				continue;
			if (!ee.is_valid(pi) || edge.get_metric(pi) < ee.get_metric(pi)) {
				ee.set_metric(pi, edge.get_metric(pi));
				if (log)
					lgraphlogaddedge(g, eedge, pi);
			}
		}
	} else {
		boost::tie(eedge, foundedge) = boost::add_edge(u, v, edge, g);
		if (foundedge && log) {
			ADR::GraphEdge& ee(g[eedge]);
			for (unsigned int pi = 0; pi < pis; ++pi) {
				if (!ee.is_valid(pi))
					continue;
				lgraphlogaddedge(g, eedge, pi);
			}
		}
	}
	if (founddct) {
		ADR::GraphEdge& ee(g[edct]);
		for (unsigned int pi = 0; pi < pis; ++pi) {
			if (!hasawy[pi] || !ee.is_valid(pi))
				continue;
			if (log)
				lgraphlogkilledge(g, edct, pi);
			ee.clear_metric(pi);
		}
	}
}

void CFMUAutoroute51::lgraphkilledge(ADR::Graph& g, ADR::Graph::edge_descriptor e, unsigned int pi, bool log)
{
	ADR::GraphEdge& ee(g[e]);
	if (!ee.is_valid(pi))
		return;
	if (log)
		lgraphlogkilledge(g, e, pi);
	ee.clear_metric(pi);
}

void CFMUAutoroute51::lgraphkillalledges(ADR::Graph& g, ADR::Graph::edge_descriptor e, bool log)
{
	ADR::GraphEdge& ee(g[e]);
	const unsigned int pis(m_performance.size());
	for (unsigned int pi = 0; pi < pis; ++pi) {
		if (!ee.is_valid(pi))
			continue;
		if (log)
			lgraphlogkilledge(g, e, pi);
		ee.clear_metric(pi);
	}
}

void CFMUAutoroute51::lgraphadd(const ADR::Object::ptr_t& p)
{
	if (!p)
		return;
	p->link(m_db, ~0U);
	const unsigned int pis(m_performance.size());
	unsigned int added(m_graph.add(get_deptime(), p, pis));
	switch (p->get_type()) {
	case ADR::Object::type_sid:
	case ADR::Object::type_star:
	case ADR::Object::type_route:
		if (!added)
			return;
		break;

	default:
		return;
	}
	lgraphadd(m_db.find_dependson(p->get_uuid(), ADR::Database::loadmode_link, get_deptime(), get_deptime() + 1,
				      ADR::Object::type_pointstart, ADR::Object::type_lineend, 0));
}

void CFMUAutoroute51::lgraphadd(const ADR::Database::findresults_t& r)
{
	for (ADR::Database::findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri)
		lgraphadd(ri->get_obj());
}

std::string CFMUAutoroute51::lgraphstat(void) const
{
	unsigned int countsid(0), countsiddct(0), countstar(0), countstardct(0);
	bool hasdep(m_vertexdep < boost::num_vertices(m_graph));
	bool hasdest(m_vertexdest < boost::num_vertices(m_graph));
	if (hasdep) {
		ADR::Graph::out_edge_iterator e0, e1;
		for (boost::tie(e0, e1) = boost::out_edges(m_vertexdep, m_graph); e0 != e1; ++e0) {
			const ADR::GraphEdge& ee(m_graph[*e0]);
			if (!ee.is_valid() || ee.is_noroute())
				continue;
			bool dct(ee.is_dct());
			countsiddct += dct;
			countsid += !dct;
		}
	}
	if (hasdest) {
		ADR::Graph::in_edge_iterator e0, e1;
		for (boost::tie(e0, e1) = boost::in_edges(m_vertexdest, m_graph); e0 != e1; ++e0) {
			const ADR::GraphEdge& ee(m_graph[*e0]);
			if (!ee.is_valid() || ee.is_noroute())
				continue;
			bool dct(ee.is_dct());
			countstardct += dct;
			countstar += !dct;
		}
	}
	std::ostringstream oss;
	oss << boost::num_vertices(m_graph) << " vertices, " << boost::num_edges(m_graph)
	    << " edges";
	if (hasdep || hasdest)
		oss << " (";
	if (hasdep)
		oss << m_graph[m_vertexdep].get_ident() << ' ' << (get_departure_ifr() ? 'I' : 'V')
		    << "FR " << countsid << " SID " << countsiddct << " DCT";
	if (hasdep && hasdest)
		oss << ", ";
	if (hasdest)
		oss << m_graph[m_vertexdest].get_ident() << ' ' << (get_destination_ifr() ? 'I' : 'V')
		    << "FR " << countstar << " STAR " << countstardct << " DCT";
	if (hasdep || hasdest)
		oss << ')';
	return oss.str();
}

bool CFMUAutoroute51::lgraphload(const Rect& bbox)
{
	m_signal_log(log_precompgraph, "");
	if (true) {
		std::ostringstream oss;
		oss << "Loading Airway graph: [" << bbox.get_southwest().get_lat_str2() << ' ' << bbox.get_southwest().get_lon_str2()
		    << ' ' << bbox.get_northeast().get_lat_str2() << ' ' << bbox.get_northeast().get_lon_str2() << ']';
		m_signal_log(log_debug0, oss.str());
	}
	{
		ADR::Database::findresults_t r(m_db.find_by_bbox(bbox, ADR::Database::loadmode_link, get_deptime(), get_deptime() + 1,
								 ADR::Object::type_routesegment, ADR::Object::type_routesegment));
		lgraphadd(r);
	}
	// compute basic metric now to avoid edges from being killed as invalid
	const unsigned int pis(m_performance.size());
	{
		int level(0), levelinc(1000);
		if (pis) {
			const Performance::Cruise& cruise(m_performance.get_cruise(0));
			level = cruise.get_altitude();
			if (pis >= 2) {
				const Performance::Cruise& cruise(m_performance.get_cruise(1));
				levelinc = cruise.get_altitude() - level;
			}
		}
		ADR::Graph::edge_iterator e0, e1;
		for (boost::tie(e0, e1) = boost::edges(m_graph); e0 != e1; ++e0) {
			ADR::GraphEdge& ee(m_graph[*e0]);
			if (!ee.is_dct()) {
				ee.clear_metric();
				ADR::GraphVertex& uu(m_graph[boost::source(*e0, m_graph)]);
				ADR::TimeTableEval tte(get_deptime(), uu.get_coord(), m_eval.get_specialdateeval());
				ee.set_metric_seg(m_eval.get_condavail(), tte, level, levelinc);
			}
		}
	}
	if (true) {
		std::ostringstream oss;
		oss << "Routing Graph after airway population: " << boost::num_vertices(m_graph) << " V, " << boost::num_edges(m_graph) << " E";
		if (false)
			oss << ", descriptor sizes: vertex " << sizeof(ADR::Graph::vertex_descriptor) << " edge " << sizeof(ADR::Graph::edge_descriptor);
		m_signal_log(log_debug0, oss.str());
	}
	{
		ADR::Database::dctresults_t r(m_db.find_dct_by_bbox(bbox, ADR::Database::loadmode_link));
		for (ADR::Database::dctresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
			ADR::Graph::vertex_descriptor v[2];
			{
				unsigned int i(0);
				for (; i < 2; ++i) {
					const ADR::Link& pt(ri->get_point(i));
					if (pt.get_obj() &&
					    pt.get_obj()->get_type() >= ADR::Object::type_airport && 
					    pt.get_obj()->get_type() <= ADR::Object::type_airportend)
						break;
					bool ok;
					boost::tie(v[i], ok) = m_graph.find_vertex(pt);
					if (ok)
						continue;
					if (!pt.get_obj())
						break;
					m_graph.add(m_eval.get_departuretime(), pt.get_obj(), pis);
					boost::tie(v[i], ok) = m_graph.find_vertex(pt);
					if (!ok)
						break;
				}
				if (i < 2)
					continue;
			}
			float tt, dist;
			Point pt[2];
			bool invalid(false);
			for (unsigned int i = 0; i < 2; ++i) {
				const ADR::GraphVertex& vv(m_graph[v[i]]);
 				pt[i] = vv.get_coord();
				invalid = invalid || !vv.is_ident_valid() || pt[i].is_invalid();
			}
			if (invalid)
				continue;
			tt = pt[0].spheric_true_course(pt[1]);
			dist = pt[0].spheric_distance_nmi(pt[1]);
			if (dist > get_dctlimit())
				continue;
			float metric(dist * get_dctpenalty() + get_dctoffset());
			for (unsigned int i(0); i < 2; ++i) {
				ADR::BidirAltRange::set_t ar(ri->get_altrange(ADR::TimeTableEval(get_deptime(), pt[i], m_eval.get_specialdateeval()))[i]);
				if (false && (m_graph[v[0]].get_ident() == "DITON" || m_graph[v[1]].get_ident() == "WIL"))
					std::cerr << "DCT: " << m_graph[v[i]].get_ident() << "->" << m_graph[v[!i]].get_ident()
						  << ' ' << ar.to_str() << std::endl;
				ADR::GraphEdge edge(pis, dist, tt);
				for (unsigned int pi = 0; pi < pis; ++pi) {
					const Performance::Cruise& cruise(m_performance.get_cruise(pi));
					if (!ar.is_inside(cruise.get_altitude()))
						continue;
					edge.set_metric(pi, metric);
				}
				if (!edge.is_valid())
					continue;
				boost::add_edge(v[i], v[!i], edge, m_graph);
			}
		}
	}
	lgraphmodified();
	{
		std::ostringstream oss;
		oss << "Routing Graph after airway/DCT population: " << boost::num_vertices(m_graph) << " V, " << boost::num_edges(m_graph) << " E";
		if (false)
			oss << ", descriptor sizes: vertex " << sizeof(ADR::Graph::vertex_descriptor) << " edge " << sizeof(ADR::Graph::edge_descriptor);
		m_signal_log(log_debug0, oss.str());
	}
	return true;
}

bool CFMUAutoroute51::lgraphcheckintersect(const Point& pt0, const Point& pt1, const Rect& bbox)
{
	return bbox.is_inside(pt0) || bbox.is_inside(pt1) || bbox.is_intersect(pt0, pt1);
}

bool CFMUAutoroute51::lgraphexcluderegions(void)
{
	bool work(false);
	for (excluderegions_t::const_iterator ei(m_excluderegions.begin()), ee(m_excluderegions.end()); ei != ee; ++ei) {
		ADR::Object::ptr_t aspc;
		const ADR::AirspaceTimeSlice *aspcts(0);
		Rect bbox(ei->get_bbox());
		if (ei->is_airspace()) {
			ADR::Database::findresults_t r(m_db.find_by_ident(ei->get_airspace_id(), ADR::Database::comp_exact, 0,
									  ADR::Database::loadmode_link, get_deptime(), get_deptime()+1,
									  ADR::Object::type_airspace, ADR::Object::type_airspace, 0));
			for (ADR::Database::findresults_t::iterator ri(r.begin()), re(r.end()); ri != re; ) {
				const ADR::AirspaceTimeSlice& ts(ri->get_obj()->operator()(get_deptime()).as_airspace());
				if (!ts.is_valid() || ts.get_ident() != ei->get_airspace_id() || to_str(ts.get_type()) != ei->get_airspace_type()) {
					ri = r.erase(ri);
					re = r.end();
					continue;
				}
				if (!aspc) {
					aspc = ri->get_obj();
					aspcts = &ts;
				}
				++ri;
			}
			if (!aspc) {
				std::ostringstream oss;
				oss << "Exclude Regions: Airspace " << ei->get_airspace_id() << '/' << ei->get_airspace_type()
				    << " not found";
				m_signal_log(log_debug0, oss.str());
				continue;
			}
			aspcts->get_bbox(bbox);
			if (r.size() > 1) {
				std::ostringstream oss;
				oss << "Exclude Regions: Multiple Airspaces " << ei->get_airspace_id() << '/' << ei->get_airspace_type()
				    << " found";
				m_signal_log(log_debug0, oss.str());
			}
		}
		const unsigned int pis(m_performance.size());
		unsigned int pi0(~0U), pi1(~0U);
		for (unsigned int pi = 0; pi < pis; ++pi) {
			const Performance::Cruise& cruise(m_performance.get_cruise(pi));
			if (cruise.get_altitude() < 100 * ei->get_minlevel() || cruise.get_altitude() > 100 * ei->get_maxlevel())
				continue;
			if (pi0 == ~0U)
				pi0 = pi;
			pi1 = pi;
		}
		{
			std::ostringstream oss;
			oss << "Exclude ";
			if (aspc)
				oss << "Airspace " << aspcts->get_ident() << '/' << to_str(aspcts->get_type());
			else
				oss << "Region " << bbox;
			if (pi0 != ~0U)
				oss << ' ' << m_performance.get_cruise(pi0).get_fplalt()
				    << ".." << m_performance.get_cruise(pi1).get_fplalt();
			oss << " AWY limit " << ei->get_awylimit()
			    << " DCT limit " << ei->get_dctlimit()
			    << " offset " << ei->get_dctoffset()
			    << " scale " << ei->get_dctscale();
			m_signal_log(log_graphrule, oss.str());
		}
		if (pi0 == ~0U)
			continue;
		unsigned int edgeremoved(0), edgedct(0), edgeawy(0);
		{
			ADR::Graph::vertex_iterator v0i, ve;
			for (boost::tie(v0i, ve) = boost::vertices(m_graph); v0i != ve; ++v0i) {
				const ADR::GraphVertex& vv0(m_graph[*v0i]);
				if (*v0i == m_vertexdep || *v0i == m_vertexdest)
					continue;
				ADR::Graph::vertex_iterator v1i(v0i);
				for (++v1i; v1i != ve; ++v1i) {
					const ADR::GraphVertex& vv1(m_graph[*v1i]);
					if (*v1i == m_vertexdep || *v1i == m_vertexdest)
						continue;
					int inters(-1);
					ADR::Graph::out_edge_iterator e0, e1;
					for (tie(e0, e1) = out_edges(*v0i, m_graph); e0 != e1; ++e0) {
						if (target(*e0, m_graph) != *v1i)
							continue;
						if (inters == -1) {
							if (aspc)
								inters = aspcts->is_intersect(ADR::TimeTableEval(get_deptime(), vv0.get_coord(), m_eval.get_specialdateeval()),
											      vv1.get_coord(), ADR::AltRange::altignore);
							else
								inters = lgraphcheckintersect(vv0.get_coord(), vv1.get_coord(), bbox);
							if (!inters)
								break;
						}
						ADR::GraphEdge& ee(m_graph[*e0]);
						for (unsigned int pi = pi0; pi <= pi1; ++pi) {
							if (!ee.is_valid(pi))
								continue;
							if (false) {
								std::ostringstream oss;
								oss << "Airspace Segment: " << lgraphvertexname(source(*e0, m_graph), pi)
								    << ' ' << ee.get_route_ident_or_dct(true)
								    << ' ' << lgraphvertexname(target(*e0, m_graph), pi)
								    << " metric " << ee.get_metric(pi) << " limit " << (ee.is_dct() ? ei->get_dctlimit() : ei->get_awylimit());
								m_signal_log(log_debug0, oss.str());
							}
							if (ee.is_dct()) {
								if (ee.get_metric(pi) <= ei->get_dctlimit()) {
									ee.set_metric(pi, ee.get_metric(pi) * ei->get_dctscale() + ei->get_dctoffset());
									++edgedct;
									continue;
								}
							} else {
								if (ee.get_metric(pi) <= ei->get_awylimit()) {
									++edgeawy;
									continue;
								}
							}
							if (false) {
								std::ostringstream oss;
								oss << "Deleting Airspace Segment: " << lgraphvertexname(source(*e0, m_graph), pi)
								    << ' ' << ee.get_route_ident_or_dct(true)
								    << ' ' << lgraphvertexname(target(*e0, m_graph), pi)
								    << " metric " << ee.get_metric(pi) << " limit " << (ee.is_dct() ? ei->get_dctlimit() : ei->get_awylimit());
								m_signal_log(log_debug0, oss.str());
							}
							ee.clear_metric(pi);
							++edgeremoved;
							work = true;
						}
					}
					if (!inters)
						break;
					for (tie(e0, e1) = out_edges(*v1i, m_graph); e0 != e1; ++e0) {
						if (target(*e0, m_graph) != *v0i)
							continue;
						if (inters == -1) {
							if (aspc)
								inters = aspcts->is_intersect(ADR::TimeTableEval(get_deptime(), vv1.get_coord(), m_eval.get_specialdateeval()),
											      vv0.get_coord(), ADR::AltRange::altignore);
							else
								inters = lgraphcheckintersect(vv0.get_coord(), vv1.get_coord(), bbox);
							if (!inters)
								break;
						}
						ADR::GraphEdge& ee(m_graph[*e0]);
						for (unsigned int pi = pi0; pi <= pi1; ++pi) {
							if (!ee.is_valid(pi))
								continue;
							if (false) {
								std::ostringstream oss;
								oss << "Airspace Segment: " << lgraphvertexname(source(*e0, m_graph), pi)
								    << ' ' << ee.get_route_ident_or_dct(true)
								    << ' ' << lgraphvertexname(target(*e0, m_graph), pi)
								    << " metric " << ee.get_metric(pi) << " limit " << (ee.is_dct() ? ei->get_dctlimit() : ei->get_awylimit());
								m_signal_log(log_debug0, oss.str());
							}
							if (ee.is_dct()) {
								if (ee.get_metric(pi) <= ei->get_dctlimit()) {
									ee.set_metric(pi, ee.get_metric(pi) * ei->get_dctscale() + ei->get_dctoffset());
									++edgedct;
									continue;
								}
							} else {
								if (ee.get_metric(pi) <= ei->get_awylimit()) {
									++edgeawy;
									continue;
								}
							}
							if (false) {
								std::ostringstream oss;
								oss << "Deleting Airspace Segment: " << lgraphvertexname(source(*e0, m_graph), pi)
								    << ' ' << ee.get_route_ident_or_dct(true)
								    << ' ' << lgraphvertexname(target(*e0, m_graph), pi)
								    << " metric " << ee.get_metric(pi) << " limit " << (ee.is_dct() ? ei->get_dctlimit() : ei->get_awylimit());
								m_signal_log(log_debug0, oss.str());
							}
							ee.clear_metric(pi);
							++edgeremoved;
							work = true;
						}
					}
				}
			}
		}
		{
			std::ostringstream oss;
			oss << "Exclude ";
			if (aspc)
				oss << "Airspace " << aspcts->get_ident() << '/' << to_str(aspcts->get_type());
			else
				oss << "Region " << bbox;
			if (pi0 != ~0U)
				oss << ' ' << m_performance.get_cruise(pi0).get_fplalt()
				    << ".." << m_performance.get_cruise(pi1).get_fplalt();
			oss << " AWY limit " << ei->get_awylimit()
			    << " DCT limit " << ei->get_dctlimit()
			    << " offset " << ei->get_dctoffset()
			    << " scale " << ei->get_dctscale()
			    << " edges removed " << edgeremoved
			    << " dct " << edgedct
			    << " awy " << edgeawy;
			m_signal_log(log_normal, oss.str());
		}
	}
	lgraphmodified_noedgekill();
	{
		std::ostringstream oss;
		oss << "Routing Graph after exclude regions: " << boost::num_vertices(m_graph) << " V, " << boost::num_edges(m_graph) << " E";
		m_signal_log(log_debug0, oss.str());
	}
	return work;
}

void CFMUAutoroute51::lgraphedgemetric(void)
{
	const unsigned int pis(m_performance.size());
	ADR::Graph::edge_iterator e0, e1;
	for (boost::tie(e0, e1) = boost::edges(m_graph); e0 != e1; ++e0) {
		ADR::GraphEdge& ee(m_graph[*e0]);
		ADR::Graph::vertex_descriptor u(boost::source(*e0, m_graph));
		ADR::Graph::vertex_descriptor v(boost::target(*e0, m_graph));
		const ADR::GraphVertex& uu(m_graph[u]);
		const ADR::GraphVertex& vv(m_graph[v]);
		for (unsigned int pi = 0; pi < pis; ++pi) {
			if (!ee.is_valid(pi))
				continue;
			const Performance::Cruise& cruise(m_performance.get_cruise(pi));
			float m(ee.get_metric(pi));
			if (m_windenable) {
				Wind<double> wnd(cruise.get_wind(uu.get_coord().halfway(vv.get_coord())));
				wnd.set_crs_tas(ee.get_truetrack(), cruise.get_tas());
				if (wnd.get_gs() >= 0.1)
					m *= cruise.get_tas() / wnd.get_gs();
			}
			m *= cruise.get_metricpernmi();
			if (false) {
				std::ostringstream oss;
				oss << "lgraphedgemetric: " << lgraphvertexname(u, pi) << "--"
				    << ee.get_route_ident_or_dct(true) << "->"
				    << lgraphvertexname(v, pi) << " metric " << ee.get_metric(pi)
				    << " --> " << m;
				m_signal_log(log_debug0, oss.str());
			}
			ee.set_metric(pi, m);
		}
	}
	lgraphmodified_noedgekill();
}

bool CFMUAutoroute51::lgraphaddsidstar(void)
{
	if (!get_departure().is_valid() || !get_destination().is_valid())
		return false;
	// add airport objects
	ADR::Object::ptr_t depobj, destobj;
	{
		if (get_departure().get_icao() != "ZZZZ") {
			ADR::Database::findresults_t r(m_db.find_by_ident(get_departure().get_icao(), ADR::Database::comp_exact, 0,
									  ADR::Database::loadmode_link, get_deptime(), get_deptime()+1,
									  ADR::Object::type_airport, ADR::Object::type_airportend, 0));
			for (ADR::Database::findresults_t::iterator ri(r.begin()), re(r.end()); ri != re; ) {
				const ADR::AirportTimeSlice& ts(ri->get_obj()->operator()(get_deptime()).as_airport());
				if (!ts.is_valid() || ts.get_ident() != get_departure().get_icao()) {
					ri = r.erase(ri);
					re = r.end();
					continue;
				}
				if (!depobj)
					depobj = ri->get_obj();
				++ri;
			}
		}
		if (!depobj) {
			// make ZZZZ node
			ADR::Airport::ptr_t p(new ADR::Airport(ADR::UUID::from_str("DEP:" + get_departure().get_icao() + get_departure().get_name())));
			ADR::AirportTimeSlice ts(0, std::numeric_limits<ADR::timetype_t>::max());
			ts.set_name(get_departure().get_name());
			ts.set_coord(get_departure().get_coord());
			ts.set_ident("ZZZZ");
			ts.set_flags(ADR::AirportTimeSlice::flag_civ | ADR::AirportTimeSlice::flag_mil |
				     ADR::AirportTimeSlice::flag_depifr | ADR::AirportTimeSlice::flag_arrifr);
			{
				TopoDb30::elev_t e(m_topodb.get_elev(ts.get_coord()));
				if (e != TopoDb30::nodata) {
					if (e == TopoDb30::ocean)
						e = 0;
					ts.set_elev(Point::round<ADR::AirportTimeSlice::elev_t,float>(e * Point::m_to_ft));
				}
			}
			p->add_timeslice(ts);
			depobj = p;
		}
		if (get_destination().get_icao() != "ZZZZ") {
			ADR::Database::findresults_t r(m_db.find_by_ident(get_destination().get_icao(), ADR::Database::comp_exact, 0,
									  ADR::Database::loadmode_link, get_deptime(), get_deptime()+1,
									  ADR::Object::type_airport, ADR::Object::type_airportend, 0));
			for (ADR::Database::findresults_t::iterator ri(r.begin()), re(r.end()); ri != re; ) {
				const ADR::AirportTimeSlice& ts(ri->get_obj()->operator()(get_deptime()).as_airport());
				if (!ts.is_valid() || ts.get_ident() != get_destination().get_icao()) {
					ri = r.erase(ri);
					re = r.end();
					continue;
				}
				if (!destobj)
					destobj = ri->get_obj();
				++ri;
			}
		}
		if (!destobj) {
			// make ZZZZ node
			ADR::Airport::ptr_t p(new ADR::Airport(ADR::UUID::from_str("DEST:" + get_destination().get_icao() + get_destination().get_name())));
			ADR::AirportTimeSlice ts(0, std::numeric_limits<ADR::timetype_t>::max());
			ts.set_name(get_destination().get_name());
			ts.set_coord(get_destination().get_coord());
			ts.set_ident("ZZZZ");
			ts.set_flags(ADR::AirportTimeSlice::flag_civ | ADR::AirportTimeSlice::flag_mil |
				     ADR::AirportTimeSlice::flag_depifr | ADR::AirportTimeSlice::flag_arrifr);
			{
				TopoDb30::elev_t e(m_topodb.get_elev(ts.get_coord()));
				if (e != TopoDb30::nodata) {
					if (e == TopoDb30::ocean)
						e = 0;
					ts.set_elev(Point::round<ADR::AirportTimeSlice::elev_t,float>(e * Point::m_to_ft));
				}
			}
			p->add_timeslice(ts);
			destobj = p;
		}
		lgraphadd(depobj);
		lgraphadd(destobj);
		std::pair<ADR::Graph::vertex_descriptor,bool> vdep(m_graph.find_vertex(depobj));
		std::pair<ADR::Graph::vertex_descriptor,bool> vdest(m_graph.find_vertex(destobj));
		if (!vdep.second || !vdest.second) {
			if (true) {
				std::ostringstream oss;
				oss << "Departure and/or Destination vertex not found";
				m_signal_log(log_debug0, oss.str());
			}
			return false;
		}
		m_vertexdep = vdep.first;
		m_vertexdest = vdest.first;
	}
	std::set<ADR::UUID> sidfilter, starfilter;
	for (sidstarfilter_t::const_iterator fi(m_airportconnfilter[0].begin()), fe(m_airportconnfilter[0].end()); fi != fe; ++fi) {
		try {
                        boost::uuids::string_generator gen;
                        ADR::UUID u(gen(*fi));
                        if (!u.is_nil())
				sidfilter.insert(u);
			continue;
                } catch (const std::runtime_error& e) {
                }
		ADR::Database::findresults_t r(m_db.find_by_ident(*fi, ADR::Database::comp_exact, 0,
								  ADR::Database::loadmode_obj, get_deptime(), get_deptime()+1,
								  ADR::Object::type_sid, ADR::Object::type_sid, 0));
		for (ADR::Database::findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
			const ADR::StandardInstrumentDepartureTimeSlice& ts(ri->get_obj()->operator()(get_deptime()).as_sid());
			if (!ts.is_valid())
				continue;
			if (ts.get_airport() != depobj->get_uuid())
				continue;
			sidfilter.insert(*ri);
		}
	}
	for (sidstarfilter_t::const_iterator fi(m_airportconnfilter[1].begin()), fe(m_airportconnfilter[1].end()); fi != fe; ++fi) {
		try {
                        boost::uuids::string_generator gen;
                        ADR::UUID u(gen(*fi));
                        if (!u.is_nil())
				starfilter.insert(u);
			continue;
                } catch (const std::runtime_error& e) {
                }
		ADR::Database::findresults_t r(m_db.find_by_ident(*fi, ADR::Database::comp_exact, 0,
								  ADR::Database::loadmode_obj, get_deptime(), get_deptime()+1,
								  ADR::Object::type_star, ADR::Object::type_star, 0));
		for (ADR::Database::findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
			const ADR::StandardInstrumentArrivalTimeSlice& ts(ri->get_obj()->operator()(get_deptime()).as_star());
			if (!ts.is_valid())
				continue;
			if (ts.get_airport() != destobj->get_uuid())
				continue;
			starfilter.insert(*ri);
		}
	}
	// kill preexisting dep/dest DCT
	{
		ADR::Graph::out_edge_iterator e0, e1;
		for (boost::tie(e0, e1) = boost::out_edges(m_vertexdep, m_graph); e0 != e1; ++e0) {
			ADR::GraphEdge& ee(m_graph[*e0]);
			if (!ee.is_dct())
				continue;
			ee.clear_metric();
		}
	}
	{
		ADR::Graph::in_edge_iterator e0, e1;
		for (boost::tie(e0, e1) = boost::in_edges(m_vertexdest, m_graph); e0 != e1; ++e0) {
			ADR::GraphEdge& ee(m_graph[*e0]);
			if (!ee.is_dct())
				continue;
			ee.clear_metric();
		}
	}
	// m_graph[vertex] is not stable over vertex inserts, so do not keep a reference
	const ADR::UUID& depuuid(m_graph[m_vertexdep].get_uuid());
	const ADR::UUID& destuuid(m_graph[m_vertexdest].get_uuid());
	const Point& depcoord(m_graph[m_vertexdep].get_coord());
	const Point& destcoord(m_graph[m_vertexdest].get_coord());
	// add SID/STAR
	const unsigned int pis(m_performance.size());
	unsigned int countsid(0), countstar(0);
	std::set<ADR::Link> sidconnpoint, starconnpoint;
	if (get_departure_ifr() && (get_siddb() || get_sid().is_invalid())) {
		ADR::TimeTableEval tte(get_deptime(), depcoord, m_eval.get_specialdateeval());
		ADR::Database::findresults_t r(m_db.find_dependson(depuuid, ADR::Database::loadmode_link, get_deptime(), get_deptime() + 1,
								   ADR::Object::type_sid, ADR::Object::type_sid, 0));
		for (ADR::Database::findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
			if (!sidfilter.empty() && sidfilter.find(*ri) == sidfilter.end())
				continue;
			const ADR::StandardInstrumentDepartureTimeSlice& ts(ri->get_obj()->operator()(get_deptime()).as_sid());
			if (!ts.is_valid() || ts.get_status() != ADR::StandardInstrumentDepartureTimeSlice::status_usable)
				continue;
			if (!ts.get_timetable().is_inside(tte))
				continue;
			ri->get_obj()->link(m_db, ~0U);
			if (!get_sid().is_invalid()) {
				bool ok(false);
				for (ADR::StandardInstrumentTimeSlice::connpoints_t::const_iterator
					     cpi(ts.get_connpoints().begin()), cpe(ts.get_connpoints().end()); cpi != cpe; ++cpi) {
					const ADR::PointIdentTimeSlice& tsp(cpi->get_obj()->operator()(get_deptime()).as_point());
					if (!tsp.is_valid())
						continue;
					if (tsp.get_coord().spheric_distance_nmi_dbl(get_sid()) > 5)
						continue;
					ok = true;
					break;
				}
				if (!ok)
					continue;
			}
			m_graph.add(get_deptime(), ri->get_obj(), pis);
			++countsid;
			sidconnpoint.insert(ts.get_connpoints().begin(), ts.get_connpoints().end());
			ADR::Database::findresults_t rs(m_db.find_dependson(ri->get_obj()->get_uuid(), ADR::Database::loadmode_link, get_deptime(), get_deptime() + 1,
									    ADR::Object::type_departureleg, ADR::Object::type_departureleg, 0));
			for (ADR::Database::findresults_t::const_iterator rsi(rs.begin()), rse(rs.end()); rsi != rse; ++rsi) {
				const ADR::SegmentTimeSlice& tss(rsi->get_obj()->operator()(get_deptime()).as_segment());
				if (!tss.is_valid())
					continue;
				rsi->get_obj()->link(m_db, ~0U);
				lgraphadd(rsi->get_obj());
				if (false) {
					const ADR::PointIdentTimeSlice& tsst(tss.get_start().get_obj()->operator()(get_deptime()).as_point());
					const ADR::PointIdentTimeSlice& tsen(tss.get_end().get_obj()->operator()(get_deptime()).as_point());
					std::ostringstream oss;
					oss << "SID segment " << ts.get_ident() << ' ' << tsst.get_ident() << "->" << tsen.get_ident();
					m_signal_log(log_debug0, oss.str());
				}
			}
		}
	}
	if (get_destination_ifr() && (get_stardb() || get_star().is_invalid())) {
		ADR::timetype_t tmroute;
		{
			double rtetm(0);
			for (unsigned int pi(0); pi < pis; ++pi) {
				const Performance::Cruise& cruise(m_performance.get_cruise(pi));
				rtetm += cruise.get_secpernmi();
			}
			rtetm /= pis;
			rtetm *= depcoord.spheric_distance_nmi_dbl(destcoord);
			tmroute = Point::round<ADR::timetype_t,double>(rtetm);
		}
		ADR::TimeTableEval tte(get_deptime() + tmroute, destcoord, m_eval.get_specialdateeval());
		ADR::Database::findresults_t r(m_db.find_dependson(destuuid, ADR::Database::loadmode_link, get_deptime(), get_deptime() + 1,
								   ADR::Object::type_star, ADR::Object::type_star, 0));
		for (ADR::Database::findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
			if (!starfilter.empty() && starfilter.find(*ri) == starfilter.end())
				continue;
			const ADR::StandardInstrumentArrivalTimeSlice& ts(ri->get_obj()->operator()(get_deptime()).as_star());
			if (!ts.is_valid() || ts.get_status() != ADR::StandardInstrumentArrivalTimeSlice::status_usable)
				continue;
			if (!ts.get_timetable().is_inside(tte))
				continue;
			ri->get_obj()->link(m_db, ~0U);
			if (!get_star().is_invalid()) {
				bool ok(false);
				for (ADR::StandardInstrumentTimeSlice::connpoints_t::const_iterator
					     cpi(ts.get_connpoints().begin()), cpe(ts.get_connpoints().end()); cpi != cpe; ++cpi) {
					const ADR::PointIdentTimeSlice& tsp(cpi->get_obj()->operator()(get_deptime()).as_point());
					if (!tsp.is_valid())
						continue;
					if (tsp.get_coord().spheric_distance_nmi_dbl(get_star()) > 5)
						continue;
					ok = true;
					break;
				}
				if (!ok)
					continue;
			}
			m_graph.add(get_deptime(), ri->get_obj(), pis);
			++countstar;
			starconnpoint.insert(ts.get_connpoints().begin(), ts.get_connpoints().end());
			ADR::Database::findresults_t rs(m_db.find_dependson(ri->get_obj()->get_uuid(), ADR::Database::loadmode_link, get_deptime(), get_deptime() + 1,
									    ADR::Object::type_arrivalleg, ADR::Object::type_arrivalleg, 0));
			for (ADR::Database::findresults_t::const_iterator rsi(rs.begin()), rse(rs.end()); rsi != rse; ++rsi) {
				const ADR::SegmentTimeSlice& tss(rsi->get_obj()->operator()(get_deptime()).as_segment());
				if (!tss.is_valid())
					continue;
				rsi->get_obj()->link(m_db, ~0U);
				lgraphadd(rsi->get_obj());
				if (false) {
					const ADR::PointIdentTimeSlice& tsst(tss.get_start().get_obj()->operator()(get_deptime()).as_point());
					const ADR::PointIdentTimeSlice& tsen(tss.get_end().get_obj()->operator()(get_deptime()).as_point());
					std::ostringstream oss;
					oss << "STAR segment " << ts.get_ident() << ' ' << tsst.get_ident() << "->" << tsen.get_ident();
					m_signal_log(log_debug0, oss.str());
				}
			}
		}
	}
	// compute basic metric now to avoid edges from being killed as invalid
	{
		int level(0), levelinc(1000);
		if (pis) {
			const Performance::Cruise& cruise(m_performance.get_cruise(0));
			level = cruise.get_altitude();
			if (pis >= 2) {
				const Performance::Cruise& cruise(m_performance.get_cruise(1));
				levelinc = cruise.get_altitude() - level;
			}
		}
		ADR::Graph::edge_iterator e0, e1;
		for (boost::tie(e0, e1) = boost::edges(m_graph); e0 != e1; ++e0) {
			ADR::GraphEdge& ee(m_graph[*e0]);
			if (ee.is_sid() || ee.is_star()) {
				ee.clear_metric();
				ADR::GraphVertex& uu(m_graph[boost::source(*e0, m_graph)]);
				ADR::TimeTableEval tte(get_deptime(), uu.get_coord(), m_eval.get_specialdateeval());
				ee.set_metric_seg(m_eval.get_condavail(), tte, level, levelinc);
			}
		}
	}
	// add DCT
	ADR::Graph::vertex_descriptor lastvertex(boost::num_vertices(m_graph));
	double sidlimit(get_sidlimit());
	double starlimit(get_starlimit());
	double sidarealimit(std::numeric_limits<double>::max()), stararealimit(std::numeric_limits<double>::max());
	ADR::FlightRestrictionTimeSlice::dctconnpoints_t sidconn, starconn;
	bool siddctrule(false), stardctrule(false), siddctarearule(false), stardctarearule(false);
	for (ADR::RestrictionEval::rules_t::const_iterator ri(m_eval.get_srules().begin()), re(m_eval.get_srules().end()); ri != re; ++ri) {
		const ADR::FlightRestrictionTimeSlice& tsr((*ri)->operator()(get_deptime()).as_flightrestriction());
		if (!tsr.is_valid())
			continue;
		{
			ADR::Link arpt;
			bool arr;
			double dist;
			ADR::FlightRestrictionTimeSlice::dctconnpoints_t connpt;
			ADR::Condition::civmil_t civmil;
			if (tsr.is_deparr_dct(arpt, arr, dist, connpt, civmil) && civmil != ADR::Condition::civmil_mil) {
				bool dctdep(!arr && arpt == depuuid);
				bool dctarr(arr && arpt == destuuid);
				if (dctdep || dctarr) {
					std::ostringstream oss;
					oss << (arr ? "Arrival" : "Departure") << " DCT [" << tsr.get_ident() << ',';
					const ADR::PointIdentTimeSlice& ts(arpt.get_obj()->operator()(get_deptime()).as_point());
					if (ts.is_valid())
						oss << ts.get_ident();
					else
						oss << arpt;
					oss << "]: Limit " << std::fixed << std::setprecision(1) << dist;
					if (!connpt.empty())
						oss << " exceptions";
					for (ADR::FlightRestrictionTimeSlice::dctconnpoints_t::const_iterator i(connpt.begin()), e(connpt.end()); i != e; ++i) {
						const ADR::PointIdentTimeSlice& ts(i->first.get_obj()->operator()(get_deptime()).as_point());
						if (ts.is_valid())
							oss << ' ' << ts.get_ident();
						else
							oss << ' ' << (i->first);
						oss << " [" << i->second.to_str() << ']';
					}
					m_signal_log(log_normal, oss.str());
				}
				if (dctdep) {
					sidlimit = dist;
					sidconn.swap(connpt);
					siddctrule = true;
				}
				if (dctarr) {
					starlimit = dist;
					starconn.swap(connpt);
					stardctrule = true;
				}
			}
		}
		{
			std::set<ADR::Link> dep, dest;
			if (tsr.is_deparr(dep, dest)) {
				if (get_departure_ifr() && dep.find(depuuid) != dep.end()) {
					m_airportifr[0] = false;
					std::ostringstream oss;
					oss << "Departure aerodrome " << m_graph[m_vertexdep].get_ident()
					    << " does not support IFR [" << tsr.get_ident() << ']';
					m_signal_log(log_normal, oss.str());
					if (false)
						(*ri)->print(std::cerr) << std::endl;
				}
				if (get_destination_ifr() && dest.find(destuuid) != dest.end()) {
					m_airportifr[1] = false;
					std::ostringstream oss;
					oss << "Destination aerodrome " << m_graph[m_vertexdest].get_ident()
					    << " does not support IFR [" << tsr.get_ident() << ']';
					m_signal_log(log_normal, oss.str());
					if (false)
						(*ri)->print(std::cerr) << std::endl;
				}
			}
		}
		if (pis) {
			ADR::Link aspc;
			ADR::AltRange alt;
			double dist;
			ADR::Condition::civmil_t civmil;
			if (tsr.is_enroute_dct(aspc, alt, dist, civmil) && civmil != ADR::Condition::civmil_mil && !std::isnan(dist)) {
				ADR::Airspace::const_ptr_t pa(ADR::Airspace::const_ptr_t::cast_dynamic(aspc.get_obj()));
				const Performance::Cruise& cruise(m_performance.get_cruise(0));
				if (get_departure_ifr() && pa) {
					ADR::TimeTableEval tte(get_deptime(), depcoord, m_eval.get_specialdateeval());
					if (pa->is_inside(tte, cruise.get_altitude(), alt, depuuid)) {
						sidarealimit = std::min(sidarealimit, dist);
						siddctarearule = true;
						std::ostringstream oss;
						oss << "Departure aerodrome " << m_graph[m_vertexdep].get_ident()
						    << " area DCT limit [" << tsr.get_ident() << "] D"
						    << std::fixed << std::setprecision(1) << dist << " max D" << sidarealimit;
						m_signal_log(log_normal, oss.str());
					}
				}
				if (get_destination_ifr() && pa) {
					ADR::TimeTableEval tte(get_deptime(), destcoord, m_eval.get_specialdateeval());
					if (pa->is_inside(tte, cruise.get_altitude(), alt, destuuid)) {
						stararealimit = std::min(stararealimit, dist);
						stardctarearule = true;
						std::ostringstream oss;
						oss << "Destination aerodrome " << m_graph[m_vertexdest].get_ident()
						    << " area DCT limit [" << tsr.get_ident() << "] D"
						    << std::fixed << std::setprecision(1) << dist << " max D" << stararealimit;
						m_signal_log(log_normal, oss.str());
					}
				}		
			}
		}
	}
	if (!get_departure_ifr()) {
		sidlimit = get_sidlimit();
		sidconn.clear();
		siddctrule = false;
		siddctarearule = false;
	}
	if (!get_destination_ifr()) {
		starlimit = get_starlimit();
		starconn.clear();
		stardctrule = false;
		stardctarearule = false;
	}
	if (!get_sid().is_invalid()) {
		ADR::FlightRestrictionTimeSlice::dctconnpoints_t connpt;
		double dist(std::numeric_limits<double>::max());
		Rect bbox(get_sid(), get_sid());
		bbox = bbox.oversize_nmi(10.f);
		ADR::Database::findresults_t r(m_db.find_by_bbox(bbox, ADR::Database::loadmode_link, get_deptime(), get_deptime()+1,
								 ADR::Object::type_navaid, ADR::Object::type_designatedpoint, 0));
		for (ADR::Database::findresults_t::iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
			static const IntervalSet<int32_t> fullalt(IntervalSet<int32_t>::Intvl(0, std::numeric_limits<int32_t>::max()));
			{
				const ADR::NavaidTimeSlice& tsn(ri->get_obj()->operator()(get_deptime()).as_navaid());
				if (tsn.is_valid() && (tsn.is_vor() || tsn.is_dme() || tsn.is_ndb())) {
					double d(tsn.get_coord().spheric_distance_nmi_dbl(get_sid()));
					if (d < dist) {
						dist = d;
						connpt.clear();
						connpt.insert(ADR::FlightRestrictionTimeSlice::dctconnpoints_t::value_type(*ri, fullalt));
					}
				}
			}
			{
				const ADR::DesignatedPointTimeSlice& tsd(ri->get_obj()->operator()(get_deptime()).as_designatedpoint());
				if (tsd.is_valid() && tsd.get_type() == ADR::DesignatedPointTimeSlice::type_icao) {
					double d(tsd.get_coord().spheric_distance_nmi_dbl(get_sid()));
					if (d < dist) {
						dist = d;
						connpt.clear();
						connpt.insert(ADR::FlightRestrictionTimeSlice::dctconnpoints_t::value_type(*ri, fullalt));
					}
				}
			}
		}
		if (connpt.empty() || dist > 5) {
			std::ostringstream oss;
			oss << "SID Point " << get_sid().get_lat_str2() << ' ' << get_sid().get_lon_str2() << " not found";
			m_signal_log(log_normal, oss.str());
			return false;
		}
		sidlimit = 0;
		sidconn.swap(connpt);
	}
	if (!get_star().is_invalid()) {
		ADR::FlightRestrictionTimeSlice::dctconnpoints_t connpt;
		double dist(std::numeric_limits<double>::max());
		Rect bbox(get_star(), get_star());
		bbox = bbox.oversize_nmi(10.f);
		ADR::Database::findresults_t r(m_db.find_by_bbox(bbox, ADR::Database::loadmode_link, get_deptime(), get_deptime()+1,
								 ADR::Object::type_navaid, ADR::Object::type_designatedpoint, 0));
		for (ADR::Database::findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
			static const IntervalSet<int32_t> fullalt(IntervalSet<int32_t>::Intvl(0, std::numeric_limits<int32_t>::max()));
			{
				const ADR::NavaidTimeSlice& tsn(ri->get_obj()->operator()(get_deptime()).as_navaid());
				if (tsn.is_valid() && (tsn.is_vor() || tsn.is_dme() || tsn.is_ndb())) {
					double d(tsn.get_coord().spheric_distance_nmi_dbl(get_star()));
					if (d < dist) {
						dist = d;
						connpt.clear();
						connpt.insert(ADR::FlightRestrictionTimeSlice::dctconnpoints_t::value_type(*ri, fullalt));
					}
				}
			}
			{
				const ADR::DesignatedPointTimeSlice& tsd(ri->get_obj()->operator()(get_deptime()).as_designatedpoint());
				if (tsd.is_valid() && tsd.get_type() == ADR::DesignatedPointTimeSlice::type_icao) {
					double d(tsd.get_coord().spheric_distance_nmi_dbl(get_star()));
					if (d < dist) {
						dist = d;
						connpt.clear();
						connpt.insert(ADR::FlightRestrictionTimeSlice::dctconnpoints_t::value_type(*ri, fullalt));
					}
				}
			}
		}
		if (connpt.empty() || dist > 5) {
			std::ostringstream oss;
			oss << "STAR Point " << get_star().get_lat_str2() << ' ' << get_star().get_lon_str2() << " not found";
			m_signal_log(log_normal, oss.str());
			return false;
		}
		starlimit = 0;
		starconn.swap(connpt);
	}
	unsigned int countsiddct(0), countstardct(0);
	// add departure DCT edges from database
	bool dbdctsid(false);
	if (get_departure_ifr() && get_sid().is_invalid() && (!get_sidonly() || !countsid)) {
		ADR::Database::dctresults_t r(m_db.find_dct_by_uuid(depuuid, ADR::Database::loadmode_link));
		for (ADR::Database::dctresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
			unsigned int dir(2);
			for (unsigned int i(0); i < 2; ++i) {
				if (ri->get_point(i) != depuuid)
					continue;
				dir = i;
				break;
			}
			if (dir >= 2)
				continue;
			ADR::Graph::vertex_descriptor v;
			{
				const ADR::Link& pt(ri->get_point(!dir));
				if (pt.get_obj() &&
				    pt.get_obj()->get_type() >= ADR::Object::type_airport && 
				    pt.get_obj()->get_type() <= ADR::Object::type_airportend)
					continue;
				bool ok;
				boost::tie(v, ok) = m_graph.find_vertex(pt);
				if (!ok) {
					if (!pt.get_obj())
						continue;
					m_graph.add(m_eval.get_departuretime(), pt.get_obj(), pis);
					boost::tie(v, ok) = m_graph.find_vertex(pt);
					if (!ok)
						continue;
				}
			}
			float tt, dist;
			Point pt[2];
			pt[0] = m_graph[m_vertexdep].get_coord();
			{
				const ADR::GraphVertex& vv(m_graph[v]);
 				pt[1] = vv.get_coord();
				if (!vv.is_ident_valid() || pt[1].is_invalid())
					continue;
			}
			tt = pt[0].spheric_true_course(pt[1]);
			dist = pt[0].spheric_distance_nmi(pt[1]);
			float metric(std::max(dist, (float)m_airportconnminimum[0]) + m_airportconnoffset[0]);
			ADR::BidirAltRange::set_t ar(ri->get_altrange(ADR::TimeTableEval(get_deptime(), pt[0], m_eval.get_specialdateeval()))[dir]);
			if (false && (m_graph[v].get_ident() == "DITON"))
				std::cerr << "DCT: " << m_graph[m_vertexdep].get_ident() << "->" << m_graph[v].get_ident()
					  << ' ' << ar.to_str() << std::endl;
			ADR::GraphEdge edge(pis, dist, tt);
			for (unsigned int pi = 0; pi < pis; ++pi) {
				const Performance::Cruise& cruise(m_performance.get_cruise(pi));
				if (!ar.is_inside(cruise.get_altitude()))
					continue;
				edge.set_metric(pi, metric);
			}
			if (sidconnpoint.find(m_graph[v].get_uuid()) != sidconnpoint.end()) {
				ADR::Graph::in_edge_iterator e0, e1;
				for (boost::tie(e0, e1) = boost::in_edges(v, m_graph); e0 != e1; ++e0) {
					ADR::GraphEdge& ee(m_graph[*e0]);
					if (!ee.is_sid(true))
						continue;
					for (unsigned int pi = 0; pi < pis; ++pi) {
						if (!ee.is_valid(pi))
							continue;
						edge.clear_metric(pi);
					}
				}
			}
			if (!edge.is_valid())
				continue;
			boost::add_edge(m_vertexdep, v, edge, m_graph);
			++countsiddct;
			dbdctsid = true;
		}
	}
	// add destination DCT edges from database
	bool dbdctstar(false);
	if (get_destination_ifr() && get_star().is_invalid() && (!get_staronly() || !countstar)) {
		ADR::Database::dctresults_t r(m_db.find_dct_by_uuid(destuuid, ADR::Database::loadmode_link));
		for (ADR::Database::dctresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
			unsigned int dir(2);
			for (unsigned int i(0); i < 2; ++i) {
				if (ri->get_point(i) != destuuid)
					continue;
				dir = !i;
				break;
			}
			if (dir >= 2)
				continue;
			ADR::Graph::vertex_descriptor v;
			{
				const ADR::Link& pt(ri->get_point(dir));
				if (pt.get_obj() &&
				    pt.get_obj()->get_type() >= ADR::Object::type_airport && 
				    pt.get_obj()->get_type() <= ADR::Object::type_airportend)
					continue;
				bool ok;
				boost::tie(v, ok) = m_graph.find_vertex(pt);
				if (!ok) {
					if (!pt.get_obj())
						continue;
					m_graph.add(m_eval.get_departuretime(), pt.get_obj(), pis);
					boost::tie(v, ok) = m_graph.find_vertex(pt);
					if (!ok)
						continue;
				}
			}
			float tt, dist;
			Point pt[2];
			pt[1] = m_graph[m_vertexdest].get_coord();
			{
				const ADR::GraphVertex& vv(m_graph[v]);
 				pt[0] = vv.get_coord();
				if (!vv.is_ident_valid() || pt[0].is_invalid())
					continue;
			}
			tt = pt[0].spheric_true_course(pt[1]);
			dist = pt[0].spheric_distance_nmi(pt[1]);
			float metric(std::max(dist, (float)m_airportconnminimum[1]) + m_airportconnoffset[1]);
			ADR::BidirAltRange::set_t ar(ri->get_altrange(ADR::TimeTableEval(get_deptime(), pt[0], m_eval.get_specialdateeval()))[dir]);
			if (false && (m_graph[v].get_ident() == "DITON"))
				std::cerr << "DCT: " << m_graph[m_vertexdep].get_ident() << "->" << m_graph[v].get_ident()
					  << ' ' << ar.to_str() << std::endl;
			ADR::GraphEdge edge(pis, dist, tt);
			for (unsigned int pi = 0; pi < pis; ++pi) {
				const Performance::Cruise& cruise(m_performance.get_cruise(pi));
				if (!ar.is_inside(cruise.get_altitude()))
					continue;
				edge.set_metric(pi, metric);
			}
			if (starconnpoint.find(m_graph[v].get_uuid()) != starconnpoint.end()) {
				ADR::Graph::out_edge_iterator e0, e1;
				for (boost::tie(e0, e1) = boost::out_edges(v, m_graph); e0 != e1; ++e0) {
					ADR::GraphEdge& ee(m_graph[*e0]);
					if (!ee.is_star(true))
						continue;
					for (unsigned int pi = 0; pi < pis; ++pi) {
						if (!ee.is_valid(pi))
							continue;
						edge.clear_metric(pi);
					}
				}
			}
			if (!edge.is_valid())
				continue;
			boost::add_edge(v, m_vertexdest, edge, m_graph);
			++countstardct;
			dbdctstar = true;
		}
	}
	bool nondbdctsid(!dbdctsid && (sidlimit > 0 || !sidconn.empty()) && (!get_sidonly() || !countsid));
	bool nondbdctstar(!dbdctstar && (starlimit > 0 || !starconn.empty()) && (!get_staronly() || !countstar));
	if (!get_sid().is_invalid())
		nondbdctsid = !countsid;
	if (!get_star().is_invalid())
		nondbdctstar = !countstar;
	if (nondbdctsid || nondbdctstar) {
		{
			std::ostringstream oss;
			oss << "Generating non-database ";
			if (nondbdctsid)
				oss << "SID";
			if (nondbdctsid && nondbdctstar)
				oss << '/';
			if (nondbdctstar)
				oss << "STAR";
			oss << " DCT edges";
			if (nondbdctsid) {
				oss << ", SID limit " << sidlimit;
				bool subseq(false);
				for (ADR::FlightRestrictionTimeSlice::dctconnpoints_t::const_iterator i(sidconn.begin()), e(sidconn.end()); i != e; ++i) {
					const ADR::IdentTimeSlice& ts(i->first.get_obj()->operator()(get_deptime()).as_ident());
					if (!ts.is_valid())
						continue;
					oss << (subseq ? "," : " connections ") << ts.get_ident();
					subseq = true;
				}
			}
			if (nondbdctstar) {
				oss << ", STAR limit " << starlimit;
				bool subseq(false);
				for (ADR::FlightRestrictionTimeSlice::dctconnpoints_t::const_iterator i(starconn.begin()), e(starconn.end()); i != e; ++i) {
					const ADR::IdentTimeSlice& ts(i->first.get_obj()->operator()(get_deptime()).as_ident());
					if (!ts.is_valid())
						continue;
					oss << (subseq ? "," : " connections ") << ts.get_ident();
					subseq = true;
				}
			}
			m_signal_log(log_normal, oss.str());
		}
		for (;;) {
			ADR::Graph::vertex_iterator vi, ve;
			for (boost::tie(vi, ve) = boost::vertices(m_graph); vi != ve; ++vi) {
				if (*vi == m_vertexdep || *vi == m_vertexdest)
					continue;
				const ADR::GraphVertex& vv(m_graph[*vi]);
				if (!vv.is_ident_valid())
					continue;
				if (!vv.get_object() ||
				    (vv.get_object()->get_type() != ADR::Object::type_navaid &&
				     vv.get_object()->get_type() != ADR::Object::type_designatedpoint))
					continue;
				{
					const ADR::GraphVertex& sdep(m_graph[m_vertexdep]);
					double depdist(vv.get_coord().spheric_distance_nmi_dbl(sdep.get_coord()));
					double deptt(sdep.get_coord().spheric_true_course(vv.get_coord()));
					IntervalSet<int32_t> depalt;
					if (!dbdctsid) {
						ADR::FlightRestrictionTimeSlice::dctconnpoints_t::const_iterator i(sidconn.find(vv.get_uuid()));
						if (i != sidconn.end())
							depalt = i->second;
						if (depdist <= sidlimit)
							depalt = IntervalSet<int32_t>::Intvl(0, std::numeric_limits<int32_t>::max());
					}
					if (!siddctrule && siddctarearule && depdist > sidarealimit)
						depalt.set_empty();
					if (!depalt.is_empty()) {
						ADR::GraphEdge edge(pis, depdist, deptt);
						edge.clear_metric();
						for (unsigned int pi = 0; pi < pis; ++pi) {
							const Performance::Cruise& cruise(m_performance.get_cruise(pi));
							if (depalt.is_inside(cruise.get_altitude()))
								edge.set_metric(pi, std::max(depdist, m_airportconnminimum[0]) + m_airportconnoffset[0]);
						}
						if (sidconnpoint.find(vv.get_uuid()) != sidconnpoint.end()) {
							ADR::Graph::in_edge_iterator e0, e1;
							for (boost::tie(e0, e1) = boost::in_edges(*vi, m_graph); e0 != e1; ++e0) {
								ADR::GraphEdge& ee(m_graph[*e0]);
								if (!ee.is_sid(true))
									continue;
								for (unsigned int pi = 0; pi < pis; ++pi) {
									if (!ee.is_valid(pi))
										continue;
									edge.clear_metric(pi);
								}
							}
						}
						if (edge.is_valid()) {
							boost::add_edge(m_vertexdep, *vi, edge, m_graph);
							++countsiddct;
						}
					}
				}
				{
					const ADR::GraphVertex& sdest(m_graph[m_vertexdest]);
					double destdist(vv.get_coord().spheric_distance_nmi_dbl(sdest.get_coord()));
					double desttt(vv.get_coord().spheric_true_course(sdest.get_coord()));
					IntervalSet<int32_t> destalt;
					if (!dbdctstar) {
						ADR::FlightRestrictionTimeSlice::dctconnpoints_t::const_iterator i(starconn.find(vv.get_uuid()));
						if (i != starconn.end())
							destalt = i->second;
						if (destdist <= starlimit)
							destalt = IntervalSet<int32_t>::Intvl(0, std::numeric_limits<int32_t>::max());
					}
					if (!stardctrule && stardctarearule && destdist > stararealimit)
						destalt.set_empty();
					if (!destalt.is_empty()) {
						ADR::GraphEdge edge(pis, destdist, desttt);
						edge.clear_metric();
						for (unsigned int pi = 0; pi < pis; ++pi) {
							const Performance::Cruise& cruise(m_performance.get_cruise(pi));
							if (destalt.is_inside(cruise.get_altitude()))
								edge.set_metric(pi, std::max(destdist, m_airportconnminimum[1]) + m_airportconnoffset[1]);
						}
						if (starconnpoint.find(vv.get_uuid()) != starconnpoint.end()) {
							ADR::Graph::out_edge_iterator e0, e1;
							for (boost::tie(e0, e1) = boost::out_edges(*vi, m_graph); e0 != e1; ++e0) {
								ADR::GraphEdge& ee(m_graph[*e0]);
								if (!ee.is_star(true))
									continue;
								for (unsigned int pi = 0; pi < pis; ++pi) {
									if (!ee.is_valid(pi))
										continue;
									edge.clear_metric(pi);
								}
							}
						}
						if (edge.is_valid()) {
							boost::add_edge(*vi, m_vertexdest, edge, m_graph);
							++countstardct;
						}
					}
				}
			}
			if ((countsid || countsiddct) && (countstar || countstardct))
				break;
			bool chg(false);
			if (get_departure_ifr() && !(countsid || countsiddct)) {
				sidlimit = get_sidlimit();
				sidconn.clear();
				siddctrule = false;
				siddctarearule = false;
				m_airportifr[0] = false;
				chg = true;
				std::ostringstream oss;
				oss << "Unable to find departure connections for aerodrome "
				    << m_graph[m_vertexdep].get_ident() << ", retrying VFR";
				m_signal_log(log_normal, oss.str());
			}
			if (get_destination_ifr() && !(countstar || countstardct)) {
				starlimit = get_starlimit();
				starconn.clear();
				stardctrule = false;
				stardctarearule = false;
				m_airportifr[1] = false;
				chg = true;
				std::ostringstream oss;
				oss << "Unable to find destination connections for aerodrome "
				    << m_graph[m_vertexdest].get_ident() << ", retrying VFR";
				m_signal_log(log_normal, oss.str());
			}
			if (!chg)
				break;
		}
	}
	lgraphmodified_noedgekill();
	{
		std::ostringstream oss;
		oss << "Routing Graph after SID/STAR population: " << boost::num_vertices(m_graph) << " V, " << boost::num_edges(m_graph)
		    << " E (" << m_graph[m_vertexdep].get_ident() << ' ' << (get_departure_ifr() ? 'I' : 'V') << "FR "
		    << countsid << " SID " << countsiddct << " DCT, " << m_graph[m_vertexdest].get_ident() << ' '
		    << (get_destination_ifr() ? 'I' : 'V') << "FR " << countstar << " STAR " << countstardct << " DCT)";
		m_signal_log(log_debug0, oss.str());
	}
	return true;
}

void CFMUAutoroute51::lgraphcompositeedges(void)
{
	const unsigned int pis(m_performance.size());
	bool work(false);
	// remove airway invalid vertices
	typedef std::set<std::string> invident_t;
	invident_t invident;
	{
		ADR::Graph::vertex_iterator vi, ve;
		for (boost::tie(vi, ve) = boost::vertices(m_graph); vi != ve; ++vi) {
			if (*vi == m_vertexdep || *vi == m_vertexdest)
				continue;
			const ADR::GraphVertex& vv(m_graph[*vi]);
			bool valid(true);
			if (!vv.is_ident_valid())
				valid = false;
			const ADR::PointIdentTimeSlice& ts(vv.get_timeslice());
			// see TrafficFlowRestrictions::load_airway_graph
			if (valid) {
				valid = false;
				{
					const ADR::NavaidTimeSlice& tsn(ts.as_navaid());
					if (tsn.is_valid()) {
						switch (tsn.get_type()) {
						case ADR::NavaidTimeSlice::type_vor:
						case ADR::NavaidTimeSlice::type_vor_dme:
						case ADR::NavaidTimeSlice::type_vortac:
						case ADR::NavaidTimeSlice::type_tacan:
						case ADR::NavaidTimeSlice::type_dme:
						case ADR::NavaidTimeSlice::type_ndb:
						case ADR::NavaidTimeSlice::type_ndb_dme:
						case ADR::NavaidTimeSlice::type_ndb_mkr:
							valid = true;
							break;

						case ADR::NavaidTimeSlice::type_mkr:
							valid = vv.get_ident().size() >= 3;
							break;

						default:
							break;
						}
					}
				}
				{
					const ADR::DesignatedPointTimeSlice& tsd(ts.as_designatedpoint());
					if (tsd.is_valid()) {
						switch (tsd.get_type()) {
						case ADR::DesignatedPointTimeSlice::type_icao:
							valid = true;
							break;

						default:
							break;
						}
					}
				}
			}
			if (false && vv.get_ident().substr(0, 3) == "WAL") {
				std::ostringstream oss;
				oss << "lgraphkillinvalidsuper: " << lgraphvertexname(*vi) << ": " << (valid ? "" : "in") << "valid";
				m_signal_log(log_debug0, oss.str());
			}
			if (valid)
				continue;
			invident.insert(vv.get_ident());
			work = work || boost::out_degree(*vi, m_graph) || boost::in_degree(*vi, m_graph);
			ADR::Graph::in_edge_iterator e0, z0;
			for(boost::tie(e0, z0) = boost::in_edges(*vi, m_graph); e0 != z0; ++e0) {
				if (boost::target(*e0, m_graph) != *vi || boost::source(*e0, m_graph) == *vi)
					continue;
				ADR::GraphEdge& ee0(m_graph[*e0]);
				ee0.set_noroute(true);
				if (!ee0.is_awy(false))
					continue;
				ADR::Graph::out_edge_iterator e1, z1;
				for(boost::tie(e1, z1) = boost::out_edges(*vi, m_graph); e1 != z1; ++e1) {
					if (boost::source(*e1, m_graph) != *vi || boost::target(*e1, m_graph) == *vi)
						continue;
					ADR::GraphEdge& ee1(m_graph[*e1]);
					ee1.set_noroute(true);
					if (!ee1.is_match(ee0.get_route_uuid(false)))
						continue;
					if (boost::source(*e0, m_graph) == boost::target(*e1, m_graph))
						continue;
					ADR::GraphEdge edge(get_deptime(), ee0.get_route_object(), pis);
					edge.set_dist(ee0.get_dist() + ee1.get_dist());
					{
						Point pt0(m_graph[boost::source(*e0, m_graph)].get_coord());
						Point pt1(m_graph[boost::target(*e1, m_graph)].get_coord());
						edge.set_truetrack(pt0.spheric_true_course(pt1));
					}
					for (unsigned int pi = 0; pi < pis; ++pi)
						edge.set_metric(pi, ee0.get_metric(pi) + ee1.get_metric(pi));
					lgraphaddedge(edge, boost::source(*e0, m_graph), boost::target(*e1, m_graph), setmetric_none, false);
					if (false) {
						std::ostringstream oss;
						oss << "lgraphkillinvalidsuper: avoiding " << lgraphvertexname(*vi)
						    << ": " << lgraphvertexname(boost::source(*e0, m_graph))
						    << ' ' << ee0.get_route_ident_or_dct(true)
						    << ' ' << lgraphvertexname(boost::target(*e1, m_graph))
						    << " metric";
						for (unsigned int pi = 0; pi < pis; ++pi)
							oss << ' ' << edge.get_metric(pi);
						m_signal_log(log_debug0, oss.str());
					}
				}
			}
		}
	}
	// SID
	{
		ADR::Graph::out_edge_iterator e0, e1;
		for(boost::tie(e0, e1) = boost::out_edges(m_vertexdep, m_graph); e0 != e1; ++e0) {
			const ADR::Object::const_ptr_t& proc(m_graph[*e0].get_route_object());
			const ADR::StandardInstrumentTimeSlice& tsproc(proc->operator()(get_deptime()).as_sidstar());
			if (!tsproc.is_valid())
				continue;
			ADR::Graph::UUIDEdgeFilter filter(m_graph, proc->get_uuid(), ADR::GraphEdge::matchcomp_segment);
			typedef boost::filtered_graph<ADR::Graph, ADR::Graph::UUIDEdgeFilter> fg_t;
			fg_t fg(m_graph, filter);
			if (tsproc.get_status() != ADR::StandardInstrumentTimeSlice::status_usable) {
				fg_t::edge_iterator e0, e1;
				for(boost::tie(e0, e1) = boost::edges(fg); e0 != e1; ++e0) {
					ADR::GraphEdge& ee(m_graph[*e0]);
					ee.set_noroute(true);
				}
				continue;
			}
			ADR::StandardInstrumentTimeSlice::connpoints_t connpt(tsproc.get_connpoints());
			{
				const ADR::GraphVertex& vdep(m_graph[m_vertexdep]);
				fg_t::edge_iterator e0, e1;
				for(boost::tie(e0, e1) = boost::edges(fg); e0 != e1; ++e0) {
					ADR::GraphEdge& ee(m_graph[*e0]);
					const ADR::SegmentTimeSlice& ts(ee.get_timeslice());
					if (ts.is_valid() && ts.get_start() == vdep.get_uuid() && connpt.find(ts.get_end()) != connpt.end() && ee.is_valid()) {
						connpt.erase(ts.get_end());
						continue;
					}
					ee.set_noroute(true);
					// hack to get at least one level for SID edges that lie below the minimum cruise altitude
					if (!ts.is_valid() || ee.is_valid())
						continue;
					const Performance::Cruise& cruise(m_performance.get_cruise(0));
					const ADR::DepartureLegTimeSlice& dep(ts.as_departureleg());
					if (!dep.is_valid())
						continue;
					if (!dep.get_altrange().is_overlap(0, cruise.get_altitude() - 1))
						continue;
					ee.set_metric(0, ee.get_dist() * cruise.get_metricpernmi());
					if (true)
						std::cerr << "SID: " << ee.get_route_ident() << ' ' << lgraphvertexname(boost::source(*e0, m_graph))
							  << ' ' << lgraphvertexname(boost::target(*e0, m_graph)) << " force routability "
							  << ee.get_metric(0) << std::endl;
				}
			}
			work = work || !connpt.empty();
			for (ADR::StandardInstrumentTimeSlice::connpoints_t::const_iterator p0(connpt.begin()), p1(connpt.end()); p0 != p1; ++p0) {
				ADR::Graph::vertex_descriptor v;
				{
					bool ok;
					boost::tie(v, ok) = m_graph.find_vertex(*p0);
					if (!ok)
						continue;
				}
				if (m_vertexdep == v)
					continue;
				std::vector<ADR::Graph::vertex_descriptor> predecessors(boost::num_vertices(m_graph), 0);
				dijkstra_shortest_paths(fg, m_vertexdep, boost::weight_map(boost::get(&ADR::GraphEdge::m_dist, m_graph)).
							predecessor_map(&predecessors[0]));
				if (predecessors[v] == v) {
					std::ostringstream oss;
					oss << "lgraphcompositeedges: no path found for route " << tsproc.get_ident()
					    << " from " << m_graph[m_vertexdep].get_ident() << " to " << m_graph[v].get_ident();
					m_signal_log(log_debug0, oss.str());
					continue;
				}
				std::vector<ADR::Graph::edge_descriptor> edges;
				{
					ADR::Graph::vertex_descriptor v1(v);
					for (;;) {
						ADR::Graph::vertex_descriptor u1(predecessors[v1]);
						if (u1 == v1)
							break;
						fg_t::out_edge_iterator e0, e1;
						for(boost::tie(e0, e1) = boost::out_edges(u1, fg); e0 != e1; ++e0) {
							if (boost::target(*e0, fg) != v1)
								continue;
							edges.insert(edges.begin(), *e0);
							break;
						}
						if (e0 == e1) {
							edges.clear();
							break;
						}
						v1 = u1;
					}
				}
				if (edges.empty()) {
					std::ostringstream oss;
					oss << "lgraphcompositeedges: no edges found for route " << tsproc.get_ident()
					    << " from " << m_graph[m_vertexdep].get_ident() << " to " << m_graph[v].get_ident();
					m_signal_log(log_debug0, oss.str());
					continue;
				}
				ADR::GraphEdge edge(get_deptime(), proc, pis);
				edge.set_all_metric(0);
				{
					Point pt0(m_graph[m_vertexdep].get_coord());
					Point pt1(m_graph[v].get_coord());
					edge.set_truetrack(pt0.spheric_true_course(pt1));
				}
				float dist(0);
				bool first(true);
				// iterate over SID edges in forward direction (airport -> connection point)
				for (std::vector<ADR::Graph::edge_descriptor>::const_iterator e0(edges.begin()), e1(edges.end()); e0 != e1; ++e0) {
					const ADR::GraphEdge& ee(m_graph[*e0]);
					if (!false)
						std::cerr << "SID " << ee.get_route_ident() << ' ' << lgraphvertexname(boost::source(*e0, m_graph))
							  << ' ' << lgraphvertexname(boost::target(*e0, m_graph)) << " dist " << ee.get_dist() << std::endl;
					dist += ee.get_dist();
					float oldmetric[pis];
					for (unsigned int pi = 0; pi < pis; ++pi)
						oldmetric[pi] = edge.get_metric(pi);
					edge.clear_metric();
					for (unsigned int pi = 0; pi < pis; ++pi) {
						if (!ee.is_valid(pi))
							continue;
						unsigned int pix(pis);
						for (unsigned int pi0 = 0; pi0 < pis; ++pi0) {
							if (ADR::Graph::is_metric_invalid(oldmetric[pi0]))
								continue;
							if (pi0 <= pi || pix >= pis)
								pix = pi0;
						}
						if (pix >= pis)
							continue;
						const Performance::LevelChange& lvlchg(m_performance.get_levelchange(first ? pis : pix, pi));
						edge.set_metric(pi, oldmetric[pix] + ee.get_metric(pi) + lvlchg.get_metricpenalty());
						if (false)
							std::cerr << "SID " << tsproc.get_ident() << ' ' << lgraphvertexname(boost::source(*e0, m_graph), pi)
								  << ' ' << lgraphvertexname(boost::target(*e0, m_graph), first ? pis : pix)
								  << " metric " << edge.get_metric(pi) << std::endl;
					}
					first = false;
				}
				// subtract level change metric
				for (unsigned int pi = 0; pi < pis; ++pi) {
					if (!edge.is_valid(pi))
						continue;
					const Performance::LevelChange& lvlchg(m_performance.get_levelchange(pis, pi));
					edge.set_metric(pi, std::max(0., edge.get_metric(pi) - lvlchg.get_metricpenalty()));
				}
				edge.set_dist(dist);
				lgraphaddedge(edge, m_vertexdep, v, setmetric_none, false);
				if (false) {
					std::cerr << "SID " << edge.get_route_ident() << ' ' << lgraphvertexname(m_vertexdep)
						  << ' ' << lgraphvertexname(v) << " D" << edge.get_dist() << " |";
					for (unsigned int pi = 0; pi < pis; ++pi) {
						if (!edge.is_valid(pi))
							continue;
						std::cerr << ' ' << edge.get_metric(pi);
					}
					std::cerr << std::endl;
				}
			}
		}
	}
	// STAR
	{
		ADR::Graph::in_edge_iterator e0, e1;
		for(boost::tie(e0, e1) = boost::in_edges(m_vertexdest, m_graph); e0 != e1; ++e0) {
			const ADR::Object::const_ptr_t& proc(m_graph[*e0].get_route_object());
			const ADR::StandardInstrumentTimeSlice& tsproc(proc->operator()(get_deptime()).as_sidstar());
			if (!tsproc.is_valid())
				continue;
			ADR::Graph::UUIDEdgeFilter filter(m_graph, proc->get_uuid(), ADR::GraphEdge::matchcomp_segment);
			typedef boost::filtered_graph<ADR::Graph, ADR::Graph::UUIDEdgeFilter> fg_t;
			fg_t fg(m_graph, filter);
			if (tsproc.get_status() != ADR::StandardInstrumentTimeSlice::status_usable) {
				fg_t::edge_iterator e0, e1;
				for(boost::tie(e0, e1) = boost::edges(fg); e0 != e1; ++e0) {
					ADR::GraphEdge& ee(m_graph[*e0]);
					ee.set_noroute(true);
				}
				continue;
			}
			ADR::StandardInstrumentTimeSlice::connpoints_t connpt(tsproc.get_connpoints());
			{
				const ADR::GraphVertex& vdest(m_graph[m_vertexdest]);
				fg_t::edge_iterator e0, e1;
				for(boost::tie(e0, e1) = boost::edges(fg); e0 != e1; ++e0) {
					ADR::GraphEdge& ee(m_graph[*e0]);
					const ADR::SegmentTimeSlice& ts(ee.get_timeslice());
					if (ts.is_valid() && ts.get_end() == vdest.get_uuid() && connpt.find(ts.get_start()) != connpt.end() && ee.is_valid()) {
						connpt.erase(ts.get_start());
						continue;
					}
					ee.set_noroute(true);
					// hack to get at least one level for STAR edges that lie below the minimum cruise altitude
					if (!ts.is_valid() || ee.is_valid())
						continue;
					const Performance::Cruise& cruise(m_performance.get_cruise(0));
					const ADR::ArrivalLegTimeSlice& arr(ts.as_arrivalleg());
					if (!arr.is_valid())
						continue;
					if (!arr.get_altrange().is_overlap(0, cruise.get_altitude() - 1))
						continue;
					ee.set_metric(0, ee.get_dist() * cruise.get_metricpernmi());
					if (true)
						std::cerr << "STAR: " << ee.get_route_ident() << ' ' << lgraphvertexname(boost::source(*e0, m_graph))
							  << ' ' << lgraphvertexname(boost::target(*e0, m_graph)) << " force routability "
							  << ee.get_metric(0) << std::endl;
				}
			}
			work = work || !connpt.empty();
			for (ADR::StandardInstrumentTimeSlice::connpoints_t::const_iterator p0(connpt.begin()), p1(connpt.end()); p0 != p1; ++p0) {
				ADR::Graph::vertex_descriptor v;
				{
					bool ok;
					boost::tie(v, ok) = m_graph.find_vertex(*p0);
					if (!ok)
						continue;
				}
				if (m_vertexdest == v)
					continue;
				std::vector<ADR::Graph::vertex_descriptor> predecessors(boost::num_vertices(m_graph), 0);
				dijkstra_shortest_paths(fg, v, boost::weight_map(boost::get(&ADR::GraphEdge::m_dist, m_graph)).
							predecessor_map(&predecessors[0]));
				if (predecessors[m_vertexdest] == m_vertexdest) {
					std::ostringstream oss;
					oss << "lgraphcompositeedges: no path found for route " << tsproc.get_ident()
					    << " from " << m_graph[v].get_ident() << " to " << m_graph[m_vertexdest].get_ident();
					m_signal_log(log_debug0, oss.str());
					continue;
				}
				std::vector<ADR::Graph::edge_descriptor> edges;
				{
					ADR::Graph::vertex_descriptor v1(m_vertexdest);
					for (;;) {
						ADR::Graph::vertex_descriptor u1(predecessors[v1]);
						if (u1 == v1)
							break;
						fg_t::out_edge_iterator e0, e1;
						for(boost::tie(e0, e1) = boost::out_edges(u1, fg); e0 != e1; ++e0) {
							if (boost::target(*e0, fg) != v1)
								continue;
							edges.push_back(*e0);
							break;
						}
						if (e0 == e1) {
							edges.clear();
							break;
						}
						v1 = u1;
					}
				}
				if (edges.empty()) {
					std::ostringstream oss;
					oss << "lgraphcompositeedges: no edges found for route " << tsproc.get_ident()
					    << " from " << m_graph[v].get_ident() << " to " << m_graph[m_vertexdest].get_ident();
					m_signal_log(log_debug0, oss.str());
					continue;
				}
				ADR::GraphEdge edge(get_deptime(), proc, pis);
				edge.set_all_metric(0);
				{
					Point pt0(m_graph[v].get_coord());
					Point pt1(m_graph[m_vertexdest].get_coord());
					edge.set_truetrack(pt0.spheric_true_course(pt1));
				}
				float dist(0);
				bool first(true);
				// iterate over STAR edges in reverse direction (airport -> connection point)
				for (std::vector<ADR::Graph::edge_descriptor>::const_iterator e0(edges.begin()), e1(edges.end()); e0 != e1; ++e0) {
					const ADR::GraphEdge& ee(m_graph[*e0]);
					if (false)
						std::cerr << "STAR " << ee.get_route_ident() << ' ' << lgraphvertexname(boost::source(*e0, m_graph))
							  << ' ' << lgraphvertexname(boost::target(*e0, m_graph)) << " dist " << ee.get_dist() << std::endl;
					dist += ee.get_dist();
					float oldmetric[pis];
					for (unsigned int pi = 0; pi < pis; ++pi)
						oldmetric[pi] = edge.get_metric(pi);
					edge.clear_metric();
					for (unsigned int pi = 0; pi < pis; ++pi) {
						if (!ee.is_valid(pi))
							continue;
						unsigned int pix(pis);
						for (unsigned int pi0 = 0; pi0 < pis; ++pi0) {
							if (ADR::Graph::is_metric_invalid(oldmetric[pi0]))
								continue;
							if (pi0 <= pi || pix >= pis)
								pix = pi0;
						}
						if (pix >= pis)
							continue;
						const Performance::LevelChange& lvlchg(m_performance.get_levelchange(pi, first ? pis : pix));
						edge.set_metric(pi, oldmetric[pix] + ee.get_metric(pi) + lvlchg.get_metricpenalty());
						if (false)
							std::cerr << "STAR " << tsproc.get_ident() << ' ' << lgraphvertexname(boost::source(*e0, m_graph), pi)
								  << ' ' << lgraphvertexname(boost::target(*e0, m_graph), first ? pis : pix)
								  << " metric " << edge.get_metric(pi) << std::endl;
					}
					first = false;
				}
				// subtract level change metric
				for (unsigned int pi = 0; pi < pis; ++pi) {
					if (!edge.is_valid(pi))
						continue;
					const Performance::LevelChange& lvlchg(m_performance.get_levelchange(pi, pis));
					edge.set_metric(pi, std::max(0., edge.get_metric(pi) - lvlchg.get_metricpenalty()));
				}
				edge.set_dist(dist);
				lgraphaddedge(edge, v, m_vertexdest, setmetric_none, false);
				if (false) {
					std::cerr << "STAR " << edge.get_route_ident() << ' ' << lgraphvertexname(m_vertexdep)
						  << ' ' << lgraphvertexname(v) << " D" << edge.get_dist() << " |";
					for (unsigned int pi = 0; pi < pis; ++pi) {
						if (!edge.is_valid(pi))
							continue;
						std::cerr << ' ' << edge.get_metric(pi);
					}
					std::cerr << std::endl;
				}
			}
		}
	}
	if (true) {
		std::ostringstream oss;
		oss << "lgraphcompositeedges: avoiding ";
		bool subseq(false);
		for (invident_t::const_iterator i(invident.begin()), e(invident.end()); i != e; ++i) {
			if (subseq)
				oss << ',';
			else
				subseq = true;
			oss << *i;
		}
		m_signal_log(log_debug0, oss.str());
	}
	if (false && !work)
		return;
	lgraphmodified();
	{
		std::ostringstream oss;
		oss << "Routing Graph after composite edges: " << boost::num_vertices(m_graph)
		    << " V, " << boost::num_edges(m_graph) << " E";
		m_signal_log(log_debug0, oss.str());
	}
}

bool CFMUAutoroute51::lgraphapplymandatoryinbound(ADR::Graph& g, const ADR::FlightRestrictionTimeSlice& tsr, bool logrule)
{
	if (!tsr.is_valid())
		return false;
	switch (tsr.get_type()) {
	case ADR::FlightRestrictionTimeSlice::type_mandatory:
	case ADR::FlightRestrictionTimeSlice::type_allowed:
		break;

	default:
		return false;
	}		
	if (!tsr.is_mandatoryinbound())
		return false;
	const unsigned int pis(m_performance.size());
	bool work(false);
	bool first(true);
	std::set<ADR::UUID> pts;
	for (unsigned int i(0), n(tsr.get_restrictions().size()); i < n; ++i) {
		ADR::RuleSequence alt(tsr.get_restrictions()[i].get_rule());
		if (!alt.size())
			continue;
		const ADR::RuleSegment& seq(alt.back());
		ADR::UUID ptuuid;
		switch (seq.get_type()) {
		case ADR::RuleSegment::type_airway:
		case ADR::RuleSegment::type_dct:
			ptuuid = seq.get_uuid1();
			break;

		case ADR::RuleSegment::type_star:
		{
			const ADR::StandardInstrumentTimeSlice& ts(seq.get_airway()->operator()(get_deptime()).as_sidstar());
			if (!ts.is_valid())
				continue;
			ptuuid = ts.get_airport();
			break;
		}

		default:
			continue;
		}
		if (ptuuid.is_nil())
			continue;
		if (pts.find(ptuuid) != pts.end())
			continue;
		pts.insert(ptuuid);
		ADR::Graph::vertex_descriptor v;
		{
			bool ok;
			boost::tie(v, ok) = g.find_vertex(ptuuid);
			if (!ok)
				continue;
		}
		ADR::Graph::in_edge_iterator e0, e1;
		for (boost::tie(e0, e1) = boost::in_edges(v, g); e0 != e1; ++e0) {
			ADR::GraphEdge& ee(g[*e0]);
			ADR::Graph::vertex_descriptor u(boost::source(*e0, g));
			const ADR::GraphVertex& uu(g[u]);
			for (unsigned int pi = 0; pi < pis; ++pi) {
				if (!ee.is_valid(pi))
					continue;
				bool ok(false);
				for (unsigned int i(0), n(tsr.get_restrictions().size()); i < n; ++i) {
					ADR::RuleSequence alt(tsr.get_restrictions()[i].get_rule());
					if (!alt.size())
						continue;
					const ADR::RuleSegment& seq(alt.back());
					switch (seq.get_type()) {
					case ADR::RuleSegment::type_dct:
						if (!ee.is_dct())
							continue;
						if (seq.get_uuid1() != ptuuid)
							continue;
						if (seq.get_uuid0() != uu.get_uuid())
							continue;
						break;

					case ADR::RuleSegment::type_airway:
						if (!ee.is_match(seq.get_airway()))
							continue;
						if (seq.get_uuid1() != ptuuid)
							continue;
						if (seq.get_uuid0() != uu.get_uuid())
							continue;
						break;

					case ADR::RuleSegment::type_star:
					{
						if (!ee.is_match(seq.get_airway()))
							continue;
						const ADR::StandardInstrumentTimeSlice& ts(seq.get_airway()->operator()(get_deptime()).as_sidstar());
						if (!ts.is_valid())
							continue;
						if (ts.get_airport() != ptuuid)
							continue;
						break;
					}

					default:
						continue;
					}
					const Performance::Cruise& cruise(m_performance.get_cruise(pi));
					if (!seq.get_alt().is_inside(cruise.get_altitude()))
						continue;
					ok = true;
					break;
				}
				if (!ok)
					continue;
				if (first) {
					ADR::TimeTableEval tte(get_deptime(), uu.get_coord(), m_eval.get_specialdateeval());
					if (!tsr.get_timetable().is_inside(tte))
						return false;
					if (logrule) {
						lgraphlogrule(tsr, get_deptime());
						if (false)
							tsr.print(std::cerr << "Mandatoryinbound:" << std::endl) << std::endl;
						logrule = false;
					}
					first = false;
				}
				lgraphkilledge(g, *e0, pi, &g == &m_graph);
				work = true;
			}
		}
	}
	return work;
}

bool CFMUAutoroute51::lgraphapplymandatoryoutbound(ADR::Graph& g, const ADR::FlightRestrictionTimeSlice& tsr, bool logrule)
{
	if (!tsr.is_valid())
		return false;
	switch (tsr.get_type()) {
	case ADR::FlightRestrictionTimeSlice::type_mandatory:
	case ADR::FlightRestrictionTimeSlice::type_allowed:
		break;

	default:
		return false;
	}		
	if (!tsr.is_mandatoryoutbound())
		return false;
	const unsigned int pis(m_performance.size());
	bool work(false);
	bool first(true);
	std::set<ADR::UUID> pts;
	for (unsigned int i(0), n(tsr.get_restrictions().size()); i < n; ++i) {
		ADR::RuleSequence alt(tsr.get_restrictions()[i].get_rule());
		if (!alt.size())
			continue;
		const ADR::RuleSegment& seq(alt.back());
		ADR::UUID ptuuid;
		switch (seq.get_type()) {
		case ADR::RuleSegment::type_airway:
		case ADR::RuleSegment::type_dct:
			ptuuid = seq.get_uuid0();
			break;

		case ADR::RuleSegment::type_sid:
		{
			const ADR::StandardInstrumentTimeSlice& ts(seq.get_airway()->operator()(get_deptime()).as_sidstar());
			if (!ts.is_valid())
				continue;
			ptuuid = ts.get_airport();
			break;
		}

		default:
			continue;
		}
		if (ptuuid.is_nil())
			continue;
		if (pts.find(ptuuid) != pts.end())
			continue;
		pts.insert(ptuuid);
		ADR::Graph::vertex_descriptor v;
		{
			bool ok;
			boost::tie(v, ok) = g.find_vertex(ptuuid);
			if (!ok)
				continue;
		}
		ADR::Graph::out_edge_iterator e0, e1;
		for (boost::tie(e0, e1) = boost::out_edges(v, g); e0 != e1; ++e0) {
			ADR::GraphEdge& ee(g[*e0]);
			ADR::Graph::vertex_descriptor u(boost::target(*e0, g));
			const ADR::GraphVertex& uu(g[u]);
			for (unsigned int pi = 0; pi < pis; ++pi) {
				if (!ee.is_valid(pi))
					continue;
				bool ok(false);
				for (unsigned int i(0), n(tsr.get_restrictions().size()); i < n; ++i) {
					ADR::RuleSequence alt(tsr.get_restrictions()[i].get_rule());
					if (!alt.size())
						continue;
					const ADR::RuleSegment& seq(alt.back());
					switch (seq.get_type()) {
					case ADR::RuleSegment::type_dct:
						if (!ee.is_dct())
							continue;
						if (seq.get_uuid0() != ptuuid)
							continue;
						if (seq.get_uuid1() != uu.get_uuid())
							continue;
						break;

					case ADR::RuleSegment::type_airway:
						if (!ee.is_match(seq.get_airway()))
							continue;
						if (seq.get_uuid0() != ptuuid)
							continue;
						if (seq.get_uuid1() != uu.get_uuid())
							continue;
						break;

					case ADR::RuleSegment::type_sid:
					{
						if (!ee.is_match(seq.get_airway()))
							continue;
						const ADR::StandardInstrumentTimeSlice& ts(seq.get_airway()->operator()(get_deptime()).as_sidstar());
						if (!ts.is_valid())
							continue;
						if (ts.get_airport() != ptuuid)
							continue;
						break;
					}

					default:
						continue;
					}
					const Performance::Cruise& cruise(m_performance.get_cruise(pi));
					if (!seq.get_alt().is_inside(cruise.get_altitude()))
						continue;
					ok = true;
					break;
				}
				if (!ok)
					continue;
				if (first) {
					ADR::TimeTableEval tte(get_deptime(), uu.get_coord(), m_eval.get_specialdateeval());
					if (!tsr.get_timetable().is_inside(tte))
						return false;
					if (logrule) {
						lgraphlogrule(tsr, get_deptime());
						if (false)
							tsr.print(std::cerr << "Mandatoryoutbound:" << std::endl) << std::endl;
						logrule = false;
					}
					first = false;
				}
				lgraphkilledge(g, *e0, pi, &g == &m_graph);
				work = true;
			}
		}
	}
	return work;
}

bool CFMUAutoroute51::lgraphapplyforbiddenpoint(ADR::Graph& g, const ADR::FlightRestrictionTimeSlice& tsr, bool logrule)
{
	if (!tsr.is_valid() || !tsr.get_condition())
		return false;
	if (false && tsr.get_ident() != "ES5572A")
		return false;
	if (false && tsr.get_ident() == "ES5572A")
		return false;
	bool mandatory(tsr.get_type() == ADR::FlightRestrictionTimeSlice::type_mandatory ||
		       tsr.get_type() == ADR::FlightRestrictionTimeSlice::type_allowed);
	if (!mandatory && tsr.get_type() != ADR::FlightRestrictionTimeSlice::type_forbidden)
		return false;
	const unsigned int pis(m_performance.size());
	bool work(false);
	const ADR::GraphVertex& vdep(g[m_vertexdep]);
	const ADR::GraphVertex& vdest(g[m_vertexdest]);
	unsigned int n(tsr.get_restrictions().size());
	if (mandatory && n > 1)
		return false;
	bool first(true);
	for (unsigned int i(0); i < n; ++i) {
		ADR::RuleSequence alt(tsr.get_restrictions()[i].get_rule());
		if (alt.size() != 1)
			continue;
		if (!alt.front().is_point())
			continue;
		const ADR::UUID& pt(alt.front().get_uuid0());
		const ADR::AltRange ar(alt.front().get_alt());
		ADR::Graph::vertex_descriptor u;
		{
			bool ok;
			boost::tie(u, ok) = g.find_vertex(pt);
			if (!ok)
				continue;
		}
		const ADR::GraphVertex& uu(g[u]);
		{
			// handle out edges
			ADR::Graph::out_edge_iterator e0, e1;
			for (boost::tie(e0, e1) = boost::out_edges(u, g); e0 != e1; ++e0) {
				ADR::GraphEdge& ee(g[*e0]);
				if (ee.is_noroute() && (ee.is_sid(false) || ee.is_star(false)))
					continue;
				ADR::Graph::vertex_descriptor v(boost::target(*e0, g));
				const ADR::GraphVertex& vv(g[v]);
				for (unsigned int pi = 0; pi < pis; ++pi) {
					if (!ee.is_valid(pi))
						continue;
					const Performance::Cruise& cruise(m_performance.get_cruise(pi));
					int32_t alt(cruise.get_altitude());
					if ((mandatory && ar.is_inside(alt)) || (!mandatory && !ar.is_inside(alt)))
						continue;
					if (false)
						std::cerr << "Segment " << uu.get_ident() << " -> " << vv.get_ident() << " pi " << pi << std::endl;
					ADR::TimeTableEval tte(get_deptime(), uu.get_coord(), m_eval.get_specialdateeval());
					ADR::Condition::forbiddenpoint_t fp(tsr.get_condition()->evaluate_forbidden_point(pt, vdep.get_uuid(), vdest.get_uuid(),
															  tte, vv.get_coord(),
															  uu.get_uuid(), vv.get_uuid(),
															  ee.get_route_uuid(true), alt));
					switch (fp) {
					case ADR::Condition::forbiddenpoint_true:
					case ADR::Condition::forbiddenpoint_truesamesegbefore:
					case ADR::Condition::forbiddenpoint_truesamesegat:
					case ADR::Condition::forbiddenpoint_truesamesegafter:
					case ADR::Condition::forbiddenpoint_trueotherseg:
						break;

					default:
						continue;
					}
					if (first) {
						if (!tsr.get_timetable().is_inside(tte))
							return false;
						if (logrule) {
							lgraphlogrule(tsr, get_deptime());
							if (false)
								tsr.print(std::cerr << "Forbiddenpoint:" << std::endl) << std::endl;
							logrule = false;
						}
						first = false;
					}
					lgraphkilledge(g, *e0, pi, &g == &m_graph);
					work = true;
				}
			}
		}
		{
			// handle in edges
			ADR::Graph::in_edge_iterator e0, e1;
			for (boost::tie(e0, e1) = boost::in_edges(u, g); e0 != e1; ++e0) {
				ADR::GraphEdge& ee(g[*e0]);
				if (ee.is_noroute() && (ee.is_sid(false) || ee.is_star(false)))
					continue;
				ADR::Graph::vertex_descriptor v(boost::source(*e0, g));
				const ADR::GraphVertex& vv(g[v]);
				for (unsigned int pi = 0; pi < pis; ++pi) {
					if (!ee.is_valid(pi))
						continue;
					const Performance::Cruise& cruise(m_performance.get_cruise(pi));
					int32_t alt(cruise.get_altitude());
					if ((mandatory && ar.is_inside(alt)) || (!mandatory && !ar.is_inside(alt)))
						continue;
					if (false)
						std::cerr << "Segment " << vv.get_ident() << " -> " << uu.get_ident() << " pi " << pi << std::endl;
					ADR::TimeTableEval tte(get_deptime(), vv.get_coord(), m_eval.get_specialdateeval());
					ADR::Condition::forbiddenpoint_t fp(tsr.get_condition()->evaluate_forbidden_point(pt, vdep.get_uuid(), vdest.get_uuid(),
															  tte, uu.get_coord(),
															  vv.get_uuid(), uu.get_uuid(),
															  ee.get_route_uuid(true), alt));
					switch (fp) {
					case ADR::Condition::forbiddenpoint_true:
					case ADR::Condition::forbiddenpoint_truesamesegbefore:
					case ADR::Condition::forbiddenpoint_truesamesegat:
					case ADR::Condition::forbiddenpoint_truesamesegafter:
					case ADR::Condition::forbiddenpoint_trueotherseg:
						break;

					default:
						continue;
					}
					if (first) {
						if (!tsr.get_timetable().is_inside(tte))
							return false;
						if (logrule) {
							lgraphlogrule(tsr, get_deptime());
							if (false)
								tsr.print(std::cerr << "Forbiddenpoint:" << std::endl) << std::endl;
							logrule = false;
						}
						first = false;
					}
					lgraphkilledge(g, *e0, pi, &g == &m_graph);
					work = true;
				}
			}
		}
	}
	return work;
}

void CFMUAutoroute51::lgraphapplylocalrules(void)
{
	bool work(false);
	for (ADR::RestrictionEval::rules_t::const_iterator ri(m_eval.get_srules().begin()), re(m_eval.get_srules().end()); ri != re; ++ri) {
		const ADR::FlightRestrictionTimeSlice& tsr((*ri)->operator()(get_deptime()).as_flightrestriction());
		if (!tsr.is_valid() || !tsr.is_enabled())
			continue;
		if (tsr.get_procind() == ADR::FlightRestrictionTimeSlice::procind_fpr && !get_honour_profilerules())
			continue;
		if (false && tsr.get_ident() == "EK5551B")
			tsr.print(std::cerr) << std::endl;
		bool work1(false);
		{
			std::vector<ADR::RuleSegment> fs(tsr.get_forbiddensegments());
			bool first(true);
			for (std::vector<ADR::RuleSegment>::const_iterator i(fs.begin()), e(fs.end()); i != e; ++i) {
				if (i->get_type() == ADR::RuleSegment::type_airway && i->get_segments().empty())
					continue;
				if (first) {
					Point pt;
					pt.set_invalid();
					{
						const ADR::PointIdentTimeSlice& ts(i->get_wpt0()->operator()(get_deptime()).as_point());
						if (ts.is_valid())
							pt = ts.get_coord();
					}
					if (pt.is_invalid()) {
						const ADR::TimeSlice& ts(i->get_wpt0()->operator()(get_deptime()));
						if (ts.is_valid()) {
							Rect bbox;
							ts.get_bbox(bbox);
							if (!bbox.get_southwest().is_invalid() && !bbox.get_northeast().is_invalid())
								pt = bbox.boxcenter();
						}
					}
					ADR::TimeTableEval tte(get_deptime(), pt, m_eval.get_specialdateeval());
					if (!tsr.get_timetable().is_inside(tte))
						break;
					lgraphlogrule(tsr, get_deptime());
					if (false)
						tsr.print(std::cerr << "Forbiddensegments:" << std::endl) << std::endl;
					first = false;
				}
				work1 = lgraphtfrrulekillforbidden(*i, true) || work1;
			}
		}
		bool work2(lgraphapplymandatoryinbound(m_graph, tsr, true));
		bool work3(lgraphapplymandatoryoutbound(m_graph, tsr, true));
		bool work4(lgraphapplyforbiddenpoint(m_graph, tsr, true));
		work = work || work1 || work2 || work3 || work4;
		if (true && (work1 || work2 || work3 || work4)) {
			std::ostringstream oss;
			oss << "Routing graph after " << tsr.get_ident()
			    << (work1 ? " forbiddensegment" : "") << (work2 ? " mandatoryinbound" : "")
			    << (work3 ? " mandatoryoutbound" : "") << (work4 ? " forbiddenpoint" : "")
			    << " application: " << lgraphstat();
			m_signal_log(log_debug0, oss.str());
		}
	}
	if (work)
		lgraphmodified();
}

void CFMUAutoroute51::lgraphcrossing(void)
{
	m_crossingmandatory.clear();
	const unsigned int pis(m_performance.size());
	for (crossing_t::iterator ci(m_crossing.begin()), ce(m_crossing.end()); ci != ce; ++ci) {
		if (ci->get_coord().is_invalid())
			continue;
		unsigned int pimin(~0U), pimax(0);
                for (unsigned int pi = 0; pi < pis; ++pi) {
                        const Performance::Cruise& cruise(m_performance.get_cruise(pi));
                        if (cruise.get_altitude() < 100 * ci->get_minlevel())
				continue;
			pimin = pi;
			break;
		}
                for (unsigned int pi = pis; pi > 0; ) {
                        const Performance::Cruise& cruise(m_performance.get_cruise(--pi));
 			if (cruise.get_altitude() > 100 * ci->get_maxlevel())
				continue;
			pimax = pi;
			break;
		}
		if (pimin > pimax) {
			std::ostringstream oss;
                        oss << "Crossing Point " << ci->get_coord().get_lat_str2() << ' ' << ci->get_coord().get_lon_str2()
			    << " F" << std::setw(3) << std::setfill('0') << ci->get_minlevel()
			    << "..F" << std::setw(3) << std::setfill('0') << ci->get_maxlevel()
			    << " R" << ci->get_radius()
			    << " levels cannot be satisfied, dropping";
                        m_signal_log(log_normal, oss.str());
			continue;
		}
		typedef std::multimap<double,ADR::Graph::vertex_descriptor> vertexmap_t;
		vertexmap_t vertexmap;
		{
			ADR::Graph::vertex_iterator vi, ve;
			std::string ident;
			Engine::AirwayGraphResult::Vertex::type_t type(Engine::AirwayGraphResult::Vertex::type_undefined);
			double iddist(std::numeric_limits<double>::max());
			for (boost::tie(vi, ve) = boost::vertices(m_graph); vi != ve; ++vi) {
				const ADR::GraphVertex& vv(m_graph[*vi]);
				double dist(vv.get_coord().spheric_distance_nmi_dbl(ci->get_coord()));
				if (dist > Crossing::maxradius)
					continue;
				vertexmap.insert(std::make_pair(dist, *vi));
				if (dist >= iddist)
					continue;
				iddist = dist;
				ident = vv.get_ident();
				if (vv.is_airport())
					type = Engine::AirwayGraphResult::Vertex::type_airport;
				else if (vv.is_navaid())
					type = Engine::AirwayGraphResult::Vertex::type_navaid;
				else if (vv.is_intersection())
					type = Engine::AirwayGraphResult::Vertex::type_intersection;
				else 
					type = Engine::AirwayGraphResult::Vertex::type_undefined;
			}
			if (ci->get_ident().empty() && iddist <= 0.1) {
				ci->set_ident(ident);
				ci->set_type(type);
			}
		}
		if (vertexmap.empty()) {
			std::ostringstream oss;
                        oss << "Crossing Point " << ci->get_coord().get_lat_str2() << ' ' << ci->get_coord().get_lon_str2()
			    << " F" << std::setw(3) << std::setfill('0') << ci->get_minlevel()
			    << "..F" << std::setw(3) << std::setfill('0') << ci->get_maxlevel()
			    << " R" << ci->get_radius()
			    << " lies more than " << Crossing::maxradius << "nm from any significant point/navaid, dropping";
                        m_signal_log(log_normal, oss.str());
			continue;
		}
		LGMandatoryAlternative alt;
		{
			std::ostringstream oss;
			oss << "USER:" << ci->get_ident() << " F" << std::setw(3) << std::setfill('0') << ci->get_minlevel()
			    << "..F" << std::setw(3) << std::setfill('0') << ci->get_maxlevel()
			    << " R" << ci->get_radius();
			alt.set_rule(oss.str());
		}
		vertexmap_t::const_iterator vi(vertexmap.upper_bound(ci->get_radius()));
		if (vi == vertexmap.begin())
			++vi;
		for (vertexmap_t::const_iterator vi2(vertexmap.begin()); vi2 != vi; ++vi2) {
			LGMandatorySequence seq;
			seq.push_back(LGMandatoryPoint(vi2->second, pimin, pimax, ADR::Object::ptr_t()));
			alt.push_back(seq);
		}
		m_crossingmandatory.push_back(alt);
	}
	if (true) {
		m_signal_log(log_debug0, "Crossing Points");
		for (unsigned int i = 0; i < m_crossingmandatory.size(); ++i) {
			const LGMandatoryAlternative& alt(m_crossingmandatory[i]);
			{
				std::ostringstream oss;
				oss << "  Crossing Rule " << i;
				m_signal_log(log_debug0, oss.str());
			}
			for (unsigned int j = 0; j < alt.size(); ++j) {
				const LGMandatorySequence& seq(alt[j]);
				std::ostringstream oss;
				oss << "    ";
				for (unsigned int k = 0; k < seq.size(); ++k) {
					const LGMandatoryPoint& pt(seq[k]);
					const ADR::GraphVertex& vv(m_graph[pt.get_v()]);
					oss << vv.get_ident();
					unsigned int pi0(pt.get_pi0());
					unsigned int pi1(pt.get_pi1());
					if (pi0 < pis && pi1 < pis)
						oss << ' ' << m_performance.get_cruise(pi0).get_fplalt()
						    << ".." << m_performance.get_cruise(pi1).get_fplalt();
					if (k + 1 < seq.size())
						oss << ' ' << lgraphawyname(pt.get_airway()) << ' ';
				}
				m_signal_log(log_debug0, oss.str());
			}
		}
	}
}

void CFMUAutoroute51::lgraphedgemetric(double& cruisemetric, double& levelchgmetric, double& trknmi,
				       ADR::timetype_t& cruisetime, ADR::timetype_t& levelchgtime,
				       unsigned int piu, unsigned int piv,
				       const ADR::GraphEdge& ee, const Performance& perf)
{
	cruisemetric = levelchgmetric = trknmi = 0;
	cruisetime = levelchgtime = 0;
	const unsigned int pis(ee.get_levels());
	unsigned int pic(piu);
	// level change penalty
	if (piu >= pis) {
		if (piv >= pis) {
			cruisemetric = levelchgmetric = std::numeric_limits<double>::quiet_NaN();
			return;
		}
		pic = piv;
		piu = pis;
	} else if (piv >= pis) {
		piv = pis;
	}
	const Performance::Cruise& cruise(perf.get_cruise(pic));
	cruisemetric = ee.get_metric(pic);
	cruisetime = Point::round<ADR::timetype_t,double>(cruise.get_secpernmi() * ee.get_dist());
	const Performance::LevelChange& lvlchg(perf.get_levelchange(piu, piv));
	levelchgmetric = lvlchg.get_metricpenalty();
	levelchgtime = Point::round<ADR::timetype_t,double>(lvlchg.get_timepenalty());
	trknmi = std::max(lvlchg.get_tracknmi(), lvlchg.get_opsperftracknmi());
}

void CFMUAutoroute51::lgraphupdatemetric(ADR::Graph& g, LRoute& r)
{
	if (r.size() < 2) {
		r.set_metric(std::numeric_limits<double>::quiet_NaN());
		return;
	}
	const unsigned int pis(m_performance.size());
	double m(0);
	for (LRoute::size_type i(1); i < r.size(); ++i) {
		unsigned int piu(r[i-1].get_perfindex());
		unsigned int piv(r[i].get_perfindex());
		if (g[r[i-1].get_vertex()].is_airport())
			piu = pis;
		if (g[r[i].get_vertex()].is_airport())
			piv = pis;
		ADR::Graph::edge_descriptor e;
		bool ok;
		tie(e, ok) = r.get_edge(i, g);
		if (!ok) {
			r.set_metric(std::numeric_limits<double>::infinity());
			std::ostringstream oss;
			oss << "Route Update Metric: Leg " << lgraphvertexname(g, r[i-1].get_vertex(), piu)
			    << "--" << lgraphawyname(r[i].get_edge()) << "->"
			    << lgraphvertexname(g, r[i].get_vertex(), piv) << " does not exist";
			m_signal_log(log_debug0, oss.str());
			return;
		}
		if (!g.is_valid_connection(piu, piv, e)) {
			r.set_metric(std::numeric_limits<double>::infinity());
			std::ostringstream oss;
			oss << "Route Update Metric: Leg " << lgraphvertexname(g, r[i-1].get_vertex(), piu)
			    << " (" << piu << ") --" << lgraphawyname(r[i].get_edge()) << "-> "
			    << lgraphvertexname(g, r[i].get_vertex(), piv) << " (" << piv << ") has no connection";
			m_signal_log(log_debug0, oss.str());
			return;
		}
		const ADR::GraphEdge& ee(g[e]);
		double cruisemetric, levelchgmetric, trknmi;
		ADR::timetype_t cruisetime, levelchgtime;
		lgraphedgemetric(cruisemetric, levelchgmetric, trknmi, cruisetime, levelchgtime, piu, piv, ee, m_performance);
		m += cruisemetric + levelchgmetric;
		if (is_metric_invalid(m)) {
			r.set_metric(std::numeric_limits<double>::infinity());
			std::ostringstream oss;
			oss << "Route Update Metric: Leg " << lgraphvertexname(g, r[i-1].get_vertex(), piu)
			    << "--" << lgraphawyname(r[i].get_edge()) << "->"
			    << lgraphvertexname(g, r[i].get_vertex(), piv) << " has invalid metric " << m
			    << " (cruise " << cruisemetric << " level change " << levelchgmetric << ')';
			m_signal_log(log_debug0, oss.str());
			return;
		}
		r[i].set_time(r[i-1].get_time() + cruisetime + levelchgtime);
	}
	r.set_metric(m);
}

void CFMUAutoroute51::lgraphclearcurrent(void)
{
	while (m_route.get_nrwpt())
		m_route.delete_wpt(0);
	m_routetime = 0;
	m_routefuel = 0;
	m_validationresponse.clear();
	m_solutionvertices.clear();
	{
		ADR::Graph::edge_iterator e0, e1;
		for (tie(e0, e1) = edges(m_graph); e0 != e1; ++e0) {
			ADR::GraphEdge& ee(m_graph[*e0]);
			ee.clear_solution();
		}
	}
}

bool CFMUAutoroute51::lgraphsetcurrent(const LRoute& r)
{
	if (false) {
		std::ostringstream oss;
		oss << "lgraphsetcurrent: " << lgraphprint(r);
		m_signal_log(log_debug0, oss.str());
	}
	lgraphclearcurrent();
	if (r.size() < 2)
		return false;
	ADR::FlightPlan fplan;
	fplan.set_aircraft(get_aircraft(), get_engine_rpm(), get_engine_mp(), get_engine_bhp());
	fplan.set_flighttype('G');
	fplan.set_number(1);
	fplan.set_alternate1("");
	fplan.set_alternate2("");
	fplan.set_departuretime(get_deptime());
	fplan.set_personsonboard_tbn();
	fplan.set_remarks("");
	fplan.resize(r.size());
	{
		ADR::Graph::vertex_descriptor v(r.back().get_vertex());
		m_solutionvertices.insert((LVertexLevel)r.back());
		const ADR::GraphVertex& vv(m_graph[v]);
		ADR::FPLWaypoint& wpt(fplan.back());
		wpt.set_ptobj(vv.get_object());
		if (!wpt.get_ptobj()) {
			if (true) {
				std::ostringstream oss;
				oss << "Cannot insert " << vv.get_ident() << " into flight plan";
				m_signal_log(log_debug0, oss.str());
			}
			lgraphclearcurrent();
			return false;
		}
		wpt.set_from_objects(true);
		wpt.set_time_unix(r.back().get_time());
		wpt.frob_flags(get_destination_ifr() ? FPlanWaypoint::altflag_ifr : 0, FPlanWaypoint::altflag_ifr);
		wpt.set_wptnr(fplan.size() - 1);
		wpt.set_pathcode(FPlanWaypoint::pathcode_none);
	}
	const unsigned int pis(m_performance.size());
	for (LRoute::size_type i(r.size()); i > 1; ) {
		--i;
		ADR::Graph::vertex_descriptor u(r[i-1].get_vertex()), v(r[i].get_vertex());
		unsigned int pi0(r[i-1].get_perfindex()), pi1(r[i].get_perfindex());
		m_solutionvertices.insert((LVertexLevel)r[i-1]);
		const ADR::GraphVertex& uu(m_graph[u]);
		const ADR::GraphVertex& vv(m_graph[v]);
		ADR::Graph::edge_descriptor e;
		{
			bool ok;
			tie(e, ok) = r.get_edge(i, m_graph);
			if (!ok) {
				if (true) {
					std::ostringstream oss;
					oss << "k shortest path: Solution Edge " << lgraphvertexname(u)
					    << "--" << lgraphawyname(r[i].get_edge()) << "->"
					    << lgraphvertexname(v) << " not found";
					m_signal_log(log_debug0, oss.str());
				}
				lgraphclearcurrent();
				return false;
			}
		}
		if (uu.is_airport())
			pi0 = pis;
		if (vv.is_airport())
			pi1 = pis;
		ADR::GraphEdge& ee(m_graph[e]);
		if (pi0 < pis) {
			ee.set_solution(pi0);
		} else if (pi1 < pis) {
			ee.set_solution(pi1);
		} else {
			if (true) {
				std::ostringstream oss;
				oss << "k shortest path: Solution Edge " << lgraphvertexname(u)
				    << "--" << lgraphawyname(r[i].get_edge()) << "->"
				    << lgraphvertexname(v) << " has invalid perf indices";
				m_signal_log(log_debug0, oss.str());
			}
			lgraphclearcurrent();
			return false;
		}
		if (false) {
			const ADR::GraphEdge& ee(m_graph[e]);
			std::ostringstream oss;
			oss << "Solution Edge: " << lgraphvertexname(u, pi0) << uu.get_coord().get_lat_str2() << ' '
			    << uu.get_coord().get_lon_str2() << " --" << ee.get_route_ident_or_dct(true) << "-> "
			    << lgraphvertexname(v, pi1) << vv.get_coord().get_lat_str2() << ' ' << vv.get_coord().get_lon_str2();
			m_signal_log(log_debug0, oss.str());
		}
		ADR::FPLWaypoint& wpt(fplan[i-1]);
		wpt.set_ptobj(uu.get_object());
		if (!wpt.get_ptobj()) {
			if (true) {
				std::ostringstream oss;
				oss << "Cannot insert " << uu.get_ident() << " into flight plan";
				m_signal_log(log_debug0, oss.str());
			}
			lgraphclearcurrent();
			return false;
		}
		wpt.set_pathobj(ee.get_object());
		wpt.set_time_unix(r[i-1].get_time());
		{
			const CFMUAutoroute51::Performance::Cruise& cruise(m_performance.get_cruise(pi0, pi1));
			wpt.set_altitude(cruise.get_altitude());
		}
		wpt.frob_flags(FPlanWaypoint::altflag_ifr | FPlanWaypoint::altflag_standard,
			       FPlanWaypoint::altflag_ifr | FPlanWaypoint::altflag_standard);
		wpt.set_from_objects(i <= 1);
		wpt.set_wptnr(i - 1);
	}
	fplan.front().frob_flags(get_departure_ifr() ? FPlanWaypoint::altflag_ifr : 0, FPlanWaypoint::altflag_ifr);
	if (!fplan.front().is_ifr() && fplan.front().get_pathcode() == FPlanWaypoint::pathcode_directto)
		fplan.front().set_pathcode(FPlanWaypoint::pathcode_none);
	if (fplan.size() >= 2) {
		const ADR::FPLWaypoint& wpt1(fplan[fplan.size() - 1]);
		ADR::FPLWaypoint& wpt0(fplan[fplan.size() - 2]);
		wpt0.set_flags(wpt0.get_flags() ^ ((wpt0.get_flags() ^ wpt1.get_flags()) & FPlanWaypoint::altflag_ifr));
		if (!wpt0.is_ifr() && wpt0.get_pathcode() == FPlanWaypoint::pathcode_directto)
			wpt0.set_pathcode(FPlanWaypoint::pathcode_none);
		fplan.set_totaleet(wpt1.get_time_unix() - fplan.front().get_time_unix());
	}
	for (LRoute::size_type i(1), n(r.size()); i < n; ++i) {
		if (fplan[i].get_pathcode() == FPlanWaypoint::pathcode_sid)
			continue;
		unsigned int pi(r[i].get_perfindex());
		const CFMUAutoroute51::Performance::Cruise& cruise(m_performance.get_cruise(pi));
		fplan.set_cruisespeed(cruise.get_fpltas());
		break;
	}
	fplan.recompute_dist();
	//fplan.recompute_decl();
	{
		ADR::FlightPlan::errors_t err(fplan.expand_composite(m_graph));
		for (ADR::FlightPlan::errors_t::const_iterator i(err.begin()), e(err.end()); i != e; ++i)
			m_signal_log(log_normal, "lgraphsetcurrent: " + *i);
		if (!err.empty())
			return false;
	}
	if (true) {
		for (unsigned int i(0), n(fplan.size()); i < n; ++i) {
			const ADR::FPLWaypoint& wpt(fplan[i]);
			std::ostringstream oss;
			oss << "F[" << i << '/' << wpt.get_wptnr() << "]: " << wpt.get_ident() << ' ' << wpt.get_type_string() << ' ';
			switch (wpt.get_pathcode()) {
			case FPlanWaypoint::pathcode_sid:
			case FPlanWaypoint::pathcode_star:
			case FPlanWaypoint::pathcode_airway:
				oss << wpt.get_pathname();
				break;

			case FPlanWaypoint::pathcode_directto:
				oss << "DCT";
				break;

			default:
				oss << '-';
				break;
			}
			oss << ' ' << wpt.get_coord().get_lat_str2() << ' ' << wpt.get_coord().get_lon_str2()
			    << " D" << std::fixed << std::setprecision(1) << wpt.get_dist_nmi()
			    << " T" << std::setprecision(0) << wpt.get_truetrack_deg()
			    << ' ' << (wpt.is_standard() ? 'F' : 'A') << std::setw(3) << std::setfill('0') << ((wpt.get_altitude() + 50) / 100)
			    << ' ' << wpt.get_icao() << '/' << wpt.get_name() << ' ' << (wpt.is_ifr() ? 'I' : 'V') << "FR";
			{
				time_t t(wpt.get_time_unix());
				struct tm tm;
				if (gmtime_r(&t, &tm)) {
					oss << ' ' << std::setw(2) << std::setfill('0') << tm.tm_hour
					    << ':' << std::setw(2) << std::setfill('0') << tm.tm_min;
					if (t >= 48 * 60 * 60)
						oss << ' ' << std::setw(4) << std::setfill('0') << (tm.tm_year + 1900)
						    << '-' << std::setw(2) << std::setfill('0') << (tm.tm_mon + 1)
						    << '-' << std::setw(2) << std::setfill('0') << tm.tm_mday;
				}
			}
			if (wpt.get_ptobj())
				oss << " ptobj " << wpt.get_ptobj()->get_uuid();
			if (wpt.get_pathobj())
				oss << " pathobj " << wpt.get_pathobj()->get_uuid();
			if (wpt.is_expanded())
				oss << " (E)";
			m_signal_log(log_debug0, oss.str());
		}
	}
	m_eval.set_fplan(fplan);
	for (ADR::FlightPlan::const_iterator fi(fplan.begin()), fe(fplan.end()); fi != fe; ++fi)
		m_route.insert_wpt(~0, *fi);
	updatefpl();
	return true;
}

void CFMUAutoroute51::lgraphmodified_noedgekill(void)
{
	m_solutiontree.clear();
	m_solutionpool.clear();
}

void CFMUAutoroute51::lgraphmodified(void)
{
	m_graph.kill_empty_edges();
	lgraphmodified_noedgekill();
}

bool CFMUAutoroute51::lgraphisinsolutiontree(const LRoute& r) const
{
	const LGraphSolutionTreeNode *tree(&m_solutiontree);
	for (LRoute::size_type i(0); i < r.size(); ++i) {
		LGraphSolutionTreeNode::const_iterator ti(tree->find(r[i]));
		if (ti == tree->end())
			return false;
		tree = &ti->second;
	}
	return true;
}

bool CFMUAutoroute51::lgraphinsertsolutiontree(const LRoute& r)
{
	bool intree(true);
	LGraphSolutionTreeNode *tree(&m_solutiontree);
	for (LRoute::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
		std::pair<LGraphSolutionTreeNode::iterator,bool> ins(tree->insert(LGraphSolutionTreeNode::value_type(LVertexEdge(*ri), LGraphSolutionTreeNode())));
		tree = &ins.first->second;
		intree = intree && !ins.second;
	}
	return intree;
}

std::string CFMUAutoroute51::lgraphprint(ADR::Graph& g, const LRoute& r)
{
	std::ostringstream oss;
	for (LRoute::const_iterator rb(r.begin()), ri(rb), re(r.end()); ri != re; ++ri) {
		if (ri != rb)
			oss << ' ' << lgraphawyname(ri->get_edge());
		oss << ' ' << lgraphvertexname(g, *ri);
	}
	if (oss.str().empty())
		return std::string();
	return oss.str().substr(1);
}

class CFMUAutoroute51::DijkstraCore {
public:
	typedef enum {
		white,
		gray,
		black
	} color_t;

	class DijkstraQueue : public LVertexLevelTime {
	public:
		DijkstraQueue(const LVertexLevelTime& v, double d = 0) : LVertexLevelTime(v), m_dist(d) {}
		DijkstraQueue(const LVertexLevel& v, ADR::timetype_t tm, double d = 0) : LVertexLevelTime(v, tm), m_dist(d) {}
		DijkstraQueue(ADR::Graph::vertex_descriptor v = 0, unsigned int pi = 0, ADR::timetype_t tm = 0, double d = 0)
			: LVertexLevelTime(v, pi, tm), m_dist(d) {}
		double get_dist(void) const { return m_dist; }
		int compare(const DijkstraQueue& x) const {
			if (get_dist() < x.get_dist())
				return -1;
			if (x.get_dist() < get_dist())
				return 1;
			return LVertexLevelTime::compare_notime(x);
		}
		bool operator==(const DijkstraQueue& x) const { return compare(x) == 0; }
		bool operator!=(const DijkstraQueue& x) const { return compare(x) != 0; }
		bool operator<(const DijkstraQueue& x) const { return compare(x) < 0; }
		bool operator<=(const DijkstraQueue& x) const { return compare(x) <= 0; }
		bool operator>(const DijkstraQueue& x) const { return compare(x) > 0; }
		bool operator>=(const DijkstraQueue& x) const { return compare(x) >= 0; }

	protected:
		double m_dist;
	};

	DijkstraCore(const ADR::Graph& g, const Performance& performance, const ADR::TimeTableSpecialDateEval& ttsde,
		     ADR::Graph::vertex_descriptor vertexdep, ADR::Graph::vertex_descriptor vertexdest);
	void init(void);
	void set_source(const LVertexLevelTime& u);
	void set_source_nodisttime(const LVertexLevelTime& u);
	void rebuild_queue(void);
	void run(const ADR::UUID& awy = ADR::GraphEdge::matchall, bool solutionedgeonly = false);
	void run_once(const ADR::UUID& awy = ADR::GraphEdge::matchall, bool solutionedgeonly = false);
	LRoute get_route(const LVertexLevel& v);
	void mark_all_white(void);
	void mark_path(const LVertexLevel& v);
	void mark_white_infinite(void);
	void mark_white_selfpred(void);
	void mark_white_infinite_selfpred(void);
	void copy_gray_paths(const DijkstraCore& x);
	bool path_contains(const LVertexLevel& v, const LVertexLevel& x);
	bool path_contains(const LVertexLevel& v, ADR::Graph::vertex_descriptor x);
	void swap(DijkstraCore& x);
	unsigned int get_pisize(void) const { return m_state.get_pisize(); }

	class LVertexLevelState {
	public:
		static const uint64_t timemask  = 0x003fffffffffffff;
		static const uint64_t colormask = 0x00c0000000000000;
		static const uint64_t perfmask  = 0xff00000000000000;
		static const unsigned int colorshift = 54;
		static const unsigned int perfshift  = 56;

		LVertexLevelState(const LVertexLevel& v = LVertexLevel(0, 0), ADR::timetype_t tm = 0,
				  const ADR::Object::const_ptr_t& e = ADR::Object::ptr_t(),
				  double dist = std::numeric_limits<double>::max(), color_t color = white);
		LVertexLevel get_pred(void) const;
		void set_pred(const LVertexLevel& v);
		ADR::timetype_t get_time(void) const { return m_time; }
		void set_time(ADR::timetype_t tm) { m_time ^= (m_time ^ tm) & timemask; }
		void add_time(ADR::timetype_t tm) { set_time(get_time() + tm); }
	        const ADR::Object::const_ptr_t& get_edge(void) const { return m_edge; }
		void set_edge(const ADR::Object::const_ptr_t& e) { m_edge = e; }
		const ADR::UUID& get_uuid(void) const;
		const ADR::UUID& get_route_uuid(void) const;
		double get_dist(void) const { return m_dist; }
		void set_dist(double dist = std::numeric_limits<double>::quiet_NaN()) { m_dist = dist; }
		void set_dist_max(void) { set_dist(std::numeric_limits<double>::max()); }
		color_t get_color(void) const { return (color_t)((m_time & colormask) >> colorshift); }
		void set_color(color_t c) { m_time ^= (m_time ^ (((uint64_t)c) << colorshift)) & colormask; }

	protected:
		ADR::Graph::vertex_descriptor m_vertex; /* 8 bytes */
		double m_dist; /* 8 bytes */
		ADR::Object::const_ptr_t m_edge; /* 8 bytes */
		uint64_t m_time; /* 8 bytes */
	};

	class StateVector {
	public:
		StateVector(unsigned int nrvert = 0, unsigned int pisize = 0) { init(nrvert, pisize); }
		void init(unsigned int nrvert = 0, unsigned int pisize = 0);
		LVertexLevelState& operator[](const LVertexLevel& x);
		const LVertexLevelState& operator[](const LVertexLevel& x) const;
		LVertexLevelState& operator[](unsigned int x);
		const LVertexLevelState& operator[](unsigned int x) const;
		unsigned int get_pisize(void) const { return m_pisize; }
		unsigned int get_size(void) const { return m_pred.size(); }
		void swap(StateVector& x) { m_pred.swap(x.m_pred); std::swap(m_pisize, x.m_pisize); }

	protected:
		std::vector<LVertexLevelState> m_pred;
		unsigned int m_pisize;
	};

	static inline const std::string& to_str(CFMUAutoroute51::DijkstraCore::color_t c)
	{
		switch (c) {
		case CFMUAutoroute51::DijkstraCore::white:
		{
			static const std::string r("white");
			return r;
		}

		case CFMUAutoroute51::DijkstraCore::gray:
		{
			static const std::string r("gray");
			return r;
		}

		case CFMUAutoroute51::DijkstraCore::black:
		{
			static const std::string r("black");
			return r;
		}

		default:
		{
			static const std::string r("?");
			return r;
		}
		}
	}

	const ADR::Graph& m_graph;
	const Performance& m_performance;
	const ADR::TimeTableSpecialDateEval& m_ttsde;
        StateVector m_state;
	typedef std::set<DijkstraQueue> queue_t;
	queue_t m_queue;
	ADR::Graph::vertex_descriptor m_vertexdep;
	ADR::Graph::vertex_descriptor m_vertexdest;
};

const uint64_t CFMUAutoroute51::DijkstraCore::LVertexLevelState::timemask;
const uint64_t CFMUAutoroute51::DijkstraCore::LVertexLevelState::colormask;
const uint64_t CFMUAutoroute51::DijkstraCore::LVertexLevelState::perfmask;
const unsigned int CFMUAutoroute51::DijkstraCore::LVertexLevelState::colorshift;
const unsigned int CFMUAutoroute51::DijkstraCore::LVertexLevelState::perfshift;

CFMUAutoroute51::DijkstraCore::LVertexLevelState::LVertexLevelState(const LVertexLevel& v, ADR::timetype_t tm, const ADR::Object::const_ptr_t& e,
								    double dist, color_t color)
	: m_vertex(v.get_vertex()), m_dist(dist), m_edge(e),
	  m_time((tm & timemask) | ((((uint64_t)v.get_perfindex()) << perfshift) & perfmask) |
		 ((((uint64_t)color) << colorshift) & colormask))
{
}

CFMUAutoroute51::LVertexLevel CFMUAutoroute51::DijkstraCore::LVertexLevelState::get_pred(void) const
{
	return LVertexLevel(m_vertex, (m_time & perfmask) >> perfshift);
}

void CFMUAutoroute51::DijkstraCore::LVertexLevelState::set_pred(const LVertexLevel& v)
{
	m_vertex = v.get_vertex();
	m_time ^= (m_time ^ (((uint64_t)v.get_perfindex()) << perfshift)) & perfmask;
}

const ADR::UUID& CFMUAutoroute51::DijkstraCore::LVertexLevelState::get_uuid(void) const
{
	return get_edge() ? get_edge()->get_uuid() : ADR::UUID::niluuid;
}

const ADR::UUID& CFMUAutoroute51::DijkstraCore::LVertexLevelState::get_route_uuid(void) const
{
	const ADR::RouteSegmentTimeSlice& ts(get_edge()->operator()(get_time()).as_routesegment());
	if (!ts.is_valid() || !ts.get_route().get_obj())
		return ADR::UUID::niluuid;
	return ts.get_route().get_obj()->get_uuid();
}

void CFMUAutoroute51::DijkstraCore::StateVector::init(unsigned int nrvert, unsigned int pisize)
{
	m_pred.clear();
	m_pisize = pisize;
	m_pred.resize(m_pisize * nrvert);
	for (ADR::Graph::vertex_descriptor i = 0; i < nrvert; ++i)
		for (unsigned int pi = 0; pi < m_pisize; ++pi) {
			LVertexLevel v(i, pi);
			(*this)[v] = LVertexLevelState(v, 0, ADR::Object::ptr_t(), std::numeric_limits<double>::max(), white);
		}
}

CFMUAutoroute51::DijkstraCore::LVertexLevelState& CFMUAutoroute51::DijkstraCore::StateVector::operator[](const LVertexLevel& x)
{
	unsigned int idx(x.get_vertex() * m_pisize + x.get_perfindex());
	if (idx >= m_pred.size() || x.get_perfindex() >= m_pisize)
		throw std::runtime_error("DijkstraCore::StateVector index error");
	return m_pred[idx];
}

const CFMUAutoroute51::DijkstraCore::LVertexLevelState& CFMUAutoroute51::DijkstraCore::StateVector::operator[](const LVertexLevel& x) const
{
	unsigned int idx(x.get_vertex() * m_pisize + x.get_perfindex());
	if (idx >= m_pred.size() || x.get_perfindex() >= m_pisize)
		throw std::runtime_error("DijkstraCore::StateVector index error");
	return m_pred[idx];
}

CFMUAutoroute51::DijkstraCore::LVertexLevelState& CFMUAutoroute51::DijkstraCore::StateVector::operator[](unsigned int x)
{
	if (x >= m_pred.size())
		throw std::runtime_error("DijkstraCore::StateVector index error");
	return m_pred[x];
}

const CFMUAutoroute51::DijkstraCore::LVertexLevelState& CFMUAutoroute51::DijkstraCore::StateVector::operator[](unsigned int x) const
{
	if (x >= m_pred.size())
		throw std::runtime_error("DijkstraCore::StateVector index error");
	return m_pred[x];
}

CFMUAutoroute51::DijkstraCore::DijkstraCore(const ADR::Graph& g, const Performance& performance, const ADR::TimeTableSpecialDateEval& ttsde,
					    ADR::Graph::vertex_descriptor vertexdep, ADR::Graph::vertex_descriptor vertexdest)
	: m_graph(g), m_performance(performance), m_ttsde(ttsde), m_vertexdep(vertexdep), m_vertexdest(vertexdest)
{
	init();
}

void CFMUAutoroute51::DijkstraCore::init(void)
{
	m_state.init(boost::num_vertices(m_graph), m_performance.size());
}

void CFMUAutoroute51::DijkstraCore::set_source(const LVertexLevelTime& u)
{
	LVertexLevelState& stateu(m_state[u]);
	stateu.set_color(gray);
	stateu.set_dist(0);
	stateu.set_time(u.get_time());
	m_queue.insert(DijkstraQueue(u, stateu.get_dist()));
}

void CFMUAutoroute51::DijkstraCore::set_source_nodisttime(const LVertexLevelTime& u)
{
	LVertexLevelState& stateu(m_state[u]);
	stateu.set_color(gray);
	m_queue.insert(DijkstraQueue(u, stateu.get_dist()));
}

void CFMUAutoroute51::DijkstraCore::rebuild_queue(void)
{
	m_queue.clear();
	const unsigned int pis(get_pisize());
	for (unsigned int i = 0, n = boost::num_vertices(m_graph); i < n; ++i)
		for (unsigned int pi = 0; pi < pis; ++pi) {
			LVertexLevel u(i, pi);
			const LVertexLevelState& stateu(m_state[u]);
			if (stateu.get_color() != gray)
				continue;
			m_queue.insert(DijkstraQueue(u, stateu.get_time(), stateu.get_dist()));
		}
}

void CFMUAutoroute51::DijkstraCore::run_once(const ADR::UUID& awy, bool solutionedgeonly)
{
	const unsigned int pis(get_pisize());
	LVertexLevelTime u(*m_queue.begin());
	m_queue.erase(m_queue.begin());
	LVertexLevelState& stateu(m_state[u]);
	const ADR::GraphVertex& uu(m_graph[u.get_vertex()]);
	unsigned int piu(u.get_perfindex());
	if (u.get_vertex() == m_vertexdep)
		piu = pis;
	ADR::Graph::out_edge_iterator e0, e1;
	for (boost::tie(e0, e1) = boost::out_edges(u.get_vertex(), m_graph); e0 != e1; ++e0) {
		const ADR::GraphEdge& ee(m_graph[*e0]);
		if (ee.is_noroute())
			continue;
		if (!ee.is_match(awy))
			continue;
		if (solutionedgeonly && !ee.is_solution())
			continue;
		ADR::Graph::vertex_descriptor vdv(boost::target(*e0, m_graph));
		for (unsigned int piv = 0; piv < pis; ++piv) {
			LVertexLevel v(vdv, piv);
			if (vdv == m_vertexdest)
				piv = pis;
			LVertexLevelState& statev(m_state[v]);
			if (false) {
				if (uu.get_ident() == "LSZH") {
					const ADR::GraphVertex& vv(m_graph[v.get_vertex()]);
					const ADR::GraphEdge& ee(m_graph[*e0]);
					double cruisemetric, levelchgmetric, trknmi;
					ADR::timetype_t cruisetime, levelchgtime;
					lgraphedgemetric(cruisemetric, levelchgmetric, trknmi, cruisetime, levelchgtime, piu, piv, ee, m_performance);
					std::cerr << "run_once: " << uu.get_ident() << '/' << u.get_perfindex()
						  << "->" << vv.get_ident() << '/' << v.get_perfindex()
						  << (ee.is_match(awy) ? " awymatch" : "")
						  << (solutionedgeonly ? " solonly" : "")
						  << (ee.is_solution() ? " solution" : "")
						  << ' ' << to_str(statev.get_color())
						  << " metric old " << statev.get_dist()
						  << " new " << (stateu.get_dist() + cruisemetric + levelchgmetric)
						  << " (prev " << stateu.get_dist() << " cruise " << cruisemetric
						  << " lvlchg " << levelchgmetric << " tracknmi " << trknmi << ") dist " << ee.get_dist() << std::endl;
				}
			}
			if (statev.get_color() == black)
				continue;
			if (!m_graph.is_valid_connection_precheck(piu, piv, *e0))
				continue;
			double cruisemetric, levelchgmetric, trknmi, dctconnpenalty(0);
			ADR::timetype_t cruisetime, levelchgtime;
			lgraphedgemetric(cruisemetric, levelchgmetric, trknmi, cruisetime, levelchgtime, piu, piv, ee, m_performance);
			if (trknmi > ee.get_dist() && !(ee.is_sid(true) || ee.is_star(true))) {
				if (piu != pis && piv != pis)
					continue;
				unsigned int pic(std::min(piu, piv));
				if (pic < pis) {
					const Performance::Cruise& cruise(m_performance.get_cruise(pic));
					dctconnpenalty = 1000 * cruise.get_metricpernmi() * trknmi / ee.get_dist();
				}
			}
			double newmetric(stateu.get_dist() + cruisemetric + levelchgmetric + dctconnpenalty);
			if (is_metric_invalid(newmetric)) {
				const ADR::GraphVertex& vv(m_graph[v.get_vertex()]);
				const ADR::GraphEdge& ee(m_graph[*e0]);
				std::cerr << "run_once: invalid metric " << uu.get_ident() << '/' << u.get_perfindex()
					  << "->" << vv.get_ident() << '/' << v.get_perfindex()
					  << (ee.is_match(awy) ? " awymatch" : "")
					  << (solutionedgeonly ? " solonly" : "")
					  << (ee.is_solution() ? " solution" : "")
					  << ' ' << to_str(statev.get_color())
					  << " prev " << stateu.get_dist() << " cruise " << cruisemetric
					  << " lvlchg " << levelchgmetric << " tracknmi " << trknmi
					  << " dist " << ee.get_dist() << " dctconn " << dctconnpenalty << std::endl;
				continue;
			}
			if (newmetric >= statev.get_dist())
				continue;
			if (!m_graph.is_valid_connection(piu, piv, *e0))
				continue;
			ADR::timetype_t newtime(u.get_time() + cruisetime + levelchgtime);
			// check whether new edge does not violate some flight plan requirements
			// this violates the optimality principle, hence this dijkstra implementation
			// is not strictly optimal. However, we do not want to incur the cost of a full
			// global optimization.
			// check time for route segments
			if (false) {
				int32_t alt0, alt1;
				if (piu >= pis) {
					alt0 = alt1 = m_performance.get_cruise(piv).get_altitude();
				} else if (piv >= pis) {
					alt0 = alt1 = m_performance.get_cruise(piu).get_altitude();
				} else {
					alt0 = m_performance.get_cruise(piu).get_altitude();
					alt1 = m_performance.get_cruise(piv).get_altitude();
					if (alt0 > alt1)
						std::swap(alt0, alt1);
				}
				if (!ee.is_inside(alt0, alt1, u.get_time(), m_ttsde))
					continue;
			}
			// check the flight plan does not contain duplicate nodes
			{
				const ADR::GraphVertex& vv(m_graph[v.get_vertex()]);
				const std::string& id(vv.get_ident());
				if (id.size() >= 2 && !Engine::AirwayGraphResult::Vertex::is_ident_numeric(id)) {
					LVertexLevel w(u);
					bool ok(true);
					for (;;) {
						const ADR::GraphVertex& ww(m_graph[w.get_vertex()]);
						if (ww.get_ident() == vv.get_ident()) {
							ok = false;
							break;
						}
						LVertexLevel wold(w);
						w = m_state[w].get_pred();
						if (w == wold)
							break;
					}
					if (!ok)
						continue;
				}
			}
			// edge is a new better edge
			if (statev.get_color() == gray) {
				queue_t::iterator i(m_queue.find(DijkstraQueue(v, statev.get_time(), statev.get_dist())));
				if (i == m_queue.end()) {
					const ADR::GraphVertex& vv(m_graph[v.get_vertex()]);
					std::ostringstream oss;
					oss << "lgraphdijkstra: gray node " << vv.get_ident() << '/' << v.get_perfindex()
					    << " dist " << statev.get_dist() << " not found in m_queue";
					throw std::runtime_error(oss.str());
				} else {
					m_queue.erase(i);
				}
			}
			statev = LVertexLevelState(u, newtime, ee.get_object(), newmetric, gray);
			m_queue.insert(DijkstraQueue(v, newtime, statev.get_dist()));
		}
	}
	stateu.set_color(black);
}

void CFMUAutoroute51::DijkstraCore::run(const ADR::UUID& awy, bool solutionedgeonly)
{
	// DIJKSTRA(G, s, w)
	//   for each vertex u in V (This loop is not run in dijkstra_shortest_paths_no_init)
	//     d[u] := infinity 
	//     p[u] := u 
	//     color[u] := WHITE
	//   end for
	//   color[s] := GRAY 
	//   d[s] := 0 
	//   INSERT(Q, s)
	//   while (Q != Ø)
	//     u := EXTRACT-MIN(Q)
	//     S := S U { u }
	//     for each vertex v in Adj[u]
	//       if (w(u,v) + d[u] < d[v])
	//         d[v] := w(u,v) + d[u]
	//         p[v] := u 
	//         if (color[v] = WHITE) 
	//           color[v] := GRAY
	//           INSERT(Q, v) 
	//         else if (color[v] = GRAY)
	//           DECREASE-KEY(Q, v)
	//       else
	//         ...
	//     end for
	//     color[u] := BLACK
	//   end while
	//   return (d, p)
	while (!m_queue.empty())
		run_once(awy, solutionedgeonly);
}

CFMUAutoroute51::LRoute CFMUAutoroute51::DijkstraCore::get_route(const LVertexLevel& v)
{
	LRoute r;
	{
		LVertexLevel w(v);
		for (;;) {
			const LVertexLevelState& statew(m_state[w]);
			LVertexLevel u(statew.get_pred());
			if (false) {
				const ADR::GraphVertex& uu(m_graph[u.get_vertex()]);
				const ADR::GraphVertex& ww(m_graph[w.get_vertex()]);
				std::cerr << "CFMUAutoroute51::DijkstraCore::get_route: "
					  << uu.get_ident() << '/' << u.get_perfindex() << ' '
					  << lgraphawyname(statew.get_edge(), statew.get_time()) << ' '
					  << ww.get_ident() << '/' << w.get_perfindex() << std::endl;
			}
			if (u == w) {
				r.insert(r.begin(), LVertexEdge(LVertexLevelTime(w, statew.get_time())));
				break;
			}
			r.insert(r.begin(), LVertexEdge(LVertexLevelTime(w, statew.get_time()), statew.get_edge()));
			w = u;
		}
	}
	if (r.size() <= 1)
		r.clear();
	return r;
}

void CFMUAutoroute51::DijkstraCore::mark_all_white(void)
{
	const unsigned int pis(get_pisize());
	for (unsigned int i = 0, n = boost::num_vertices(m_graph); i < n; ++i)
		for (unsigned int pi = 0; pi < pis; ++pi)
			m_state[LVertexLevel(i, pi)].set_color(white);
}

void CFMUAutoroute51::DijkstraCore::mark_path(const LVertexLevel& v)
{
	{
		LVertexLevelState& state(m_state[v]);
		if (state.get_pred() == v)
			return;
		state.set_color(gray);
	}
	LVertexLevel vv(v);
	for (;;) {
		LVertexLevel u(m_state[vv].get_pred());
		if (u == vv)
			break;
		LVertexLevelState& stateu(m_state[u]);
		if (stateu.get_color() == white)
			stateu.set_color(black);
		vv = u;
	}
}

bool CFMUAutoroute51::DijkstraCore::path_contains(const LVertexLevel& v, const LVertexLevel& x)
{
	LVertexLevel vv(v);
	for (;;) {
		if (vv == x)
			return true;
		LVertexLevel u(m_state[vv].get_pred());
		if (u == vv)
			break;
		vv = u;
	}
	return false;
}

bool CFMUAutoroute51::DijkstraCore::path_contains(const LVertexLevel& v, ADR::Graph::vertex_descriptor x)
{
	LVertexLevel vv(v);
	for (;;) {
		if (vv.get_vertex() == x)
			return true;
		LVertexLevel u(m_state[vv].get_pred());
		if (u == vv)
			break;
		vv = u;
	}
	return false;
}

void CFMUAutoroute51::DijkstraCore::mark_white_infinite(void)
{
	const unsigned int pis(get_pisize());
	for (unsigned int i = 0, n = boost::num_vertices(m_graph); i < n; ++i)
		for (unsigned int pi = 0; pi < pis; ++pi) {
			LVertexLevelState& state(m_state[LVertexLevel(i, pi)]);
			if (state.get_color() == white)
				state.set_dist_max();
		}
}

void CFMUAutoroute51::DijkstraCore::mark_white_selfpred(void)
{
	const unsigned int pis(get_pisize());
	for (unsigned int i = 0, n = boost::num_vertices(m_graph); i < n; ++i)
		for (unsigned int pi = 0; pi < pis; ++pi) {
			LVertexLevel lv(i, pi);
			LVertexLevelState& state(m_state[lv]);
			if (state.get_color() == white)
				state.set_pred(lv);
		}
}

void CFMUAutoroute51::DijkstraCore::mark_white_infinite_selfpred(void)
{
	const unsigned int pis(get_pisize());
	for (unsigned int i = 0, n = boost::num_vertices(m_graph); i < n; ++i)
		for (unsigned int pi = 0; pi < pis; ++pi) {
			LVertexLevel lv(i, pi);
			LVertexLevelState& state(m_state[lv]);
			if (state.get_color() != white)
				continue;
			state.set_dist_max();
			state.set_pred(lv);
		}
}

void CFMUAutoroute51::DijkstraCore::copy_gray_paths(const DijkstraCore& x)
{
	if (boost::num_vertices(m_graph) != boost::num_vertices(x.m_graph) ||
	    (boost::num_vertices(m_graph) && (m_state.get_size() != x.m_state.get_size() ||
					       m_state.get_pisize() != x.m_state.get_pisize())))
		throw std::runtime_error("copy_gray_paths: invalid graphs");
	const unsigned int pis(get_pisize());
	for (unsigned int i = 0, n = boost::num_vertices(m_graph); i < n; ++i) {
		for (unsigned int pi = 0; pi < pis; ++pi) {
			LVertexLevel v(i, pi);
			{
				const LVertexLevelState& statev(x.m_state[v]);
				if (statev.get_color() != gray)
					continue;
				if (statev.get_pred() == v)
					continue;
			}
			for (;;) {
				const LVertexLevelState& statev(x.m_state[v]);
				LVertexLevel u(statev.get_pred());
				color_t col;
				{
					const LVertexLevelState& statevv(m_state[v]);
					if (!(statev.get_dist() < statevv.get_dist()))
						break;
					col = statevv.get_color();
				}
				if (col == white)
					col = black;
				m_state[v] = LVertexLevelState(u, statev.get_time(), statev.get_edge(), statev.get_dist(), col);
				if (u == v)
					break;
				v = u;
			}
			m_state[LVertexLevel(i, pi)].set_color(gray);
		}
	}
}

void CFMUAutoroute51::DijkstraCore::swap(DijkstraCore& x)
{
	if (boost::num_vertices(m_graph) != boost::num_vertices(x.m_graph) ||
	    (boost::num_vertices(m_graph) && (m_state.get_size() != x.m_state.get_size() ||
					       m_state.get_pisize() != x.m_state.get_pisize())))
		throw std::runtime_error("swap: invalid graphs");
	m_state.swap(x.m_state);
	m_queue.swap(x.m_queue);
}

CFMUAutoroute51::LRoute CFMUAutoroute51::lgraphdijkstra(ADR::Graph& g, const LVertexLevelTime& u, const LVertexLevel& v, const LRoute& baseroute, bool solutionedgeonly, LGMandatory mandatory)
{
	const unsigned int pis(m_performance.size());
	DijkstraCore d(g, m_performance, m_eval.get_specialdateeval(), m_vertexdep, m_vertexdest);
	d.m_state[u].set_dist(0);
	d.m_state[u].set_time(u.get_time());
	// start at specific route
	if (!baseroute.empty()) {
		LRoute::size_type i(1), n(baseroute.size());
		if (((LVertexLevel)baseroute[0]) != (LVertexLevel)u) {
			{
				DijkstraCore::LVertexLevelState& state(d.m_state[(LVertexLevel)baseroute[0]]);
				state.set_dist(0);
				state.set_time(baseroute[0].get_time());
			}
			for (; i < n; ++i) {
				LVertexLevel vv(baseroute[i]);
				LVertexLevel uu(baseroute[i - 1]);
				DijkstraCore::LVertexLevelState& stateuu(d.m_state[uu]);
				DijkstraCore::LVertexLevelState& statevv(d.m_state[vv]);
				statevv.set_pred(uu);
				std::pair<ADR::Graph::edge_descriptor,bool> ee(LRoute::get_edge(baseroute[i - 1], baseroute[i], g));
				if (!ee.second) {
					std::ostringstream oss;
					oss << "lgraphdijkstra: base route edge not found";
					m_signal_log(log_debug0, oss.str());
					return LRoute();
				}
				stateuu.set_color(DijkstraCore::black);
				const ADR::GraphEdge& eee(g[ee.first]);
				statevv.set_edge(eee.get_object());
				unsigned int piu(uu.get_perfindex());
				unsigned int piv(vv.get_perfindex());
				if (uu.get_vertex() == m_vertexdep)
					piu = pis;
				if (vv.get_vertex() == m_vertexdest)
					piv = pis;
				double cruisemetric, levelchgmetric, trknmi;
				ADR::timetype_t cruisetime, levelchgtime;
				lgraphedgemetric(cruisemetric, levelchgmetric, trknmi, cruisetime, levelchgtime, piu, piv, eee, m_performance);
				double newmetric(stateuu.get_dist() + cruisemetric + levelchgmetric);
				if (is_metric_invalid(newmetric)) {
					std::ostringstream oss;
					oss << "lgraphdijkstra: base route edge metric " << newmetric << " invalid";
					m_signal_log(log_debug0, oss.str());
					return LRoute();
				}
				statevv.set_dist(newmetric);
				statevv.set_time(stateuu.get_time() + cruisetime + levelchgtime);
				if (vv == u)
					break;
			}
			if (i >= n) {
				std::ostringstream oss;
				oss << "lgraphdijkstra: start vertex not found in base route";
				m_signal_log(log_debug0, oss.str());
			}
		}
	}
	d.set_source_nodisttime(u);
	for (;;) {
		try {
			d.run(ADR::GraphEdge::matchall, solutionedgeonly);
		} catch (const std::exception& e) {
			m_signal_log(log_debug0, e.what());
			return LRoute();
		}
		if (mandatory.empty())
			break;
		// find the mandatory rule that is closest to the source
		unsigned int mrule(~0U);
		{
			double bestdist(std::numeric_limits<double>::max());
			for (unsigned int mr = 0; mr < mandatory.size(); ++mr) {
				const LGMandatoryAlternative& alt(mandatory[mr]);
				for (unsigned int ma = 0; ma < alt.size(); ++ma) {
					const LGMandatorySequence& seq(alt[ma]);
					if (seq.empty())
						continue;
					if (true) {
						std::ostringstream oss;
						oss << "lgraphdijkstra: trying mandatory sequence";
						for (unsigned int si = 0; si < seq.size(); ++si)
							oss << ' ' << lgraphvertexname(g, seq[si].get_v(), seq[si].get_pi0(), seq[si].get_pi1())
							    << ' ' << lgraphawyname(seq[si].get_airway());
						m_signal_log(log_debug0, oss.str());
					}
					double dist(std::numeric_limits<double>::max());
					for (unsigned int pi = seq[0].get_pi0(); pi <= seq[0].get_pi1(); ++pi) {
						LVertexLevel v(seq[0].get_v(), pi);
						const DijkstraCore::LVertexLevelState& statev(d.m_state[v]);
						if (v == statev.get_pred())
							continue;
						if (!(statev.get_dist() < dist) || !(statev.get_dist() < bestdist))
							continue;
						if (seq.size() > 1) {
							ADR::Graph::out_edge_iterator e0, e1;
							for (boost::tie(e0, e1) = boost::out_edges(v.get_vertex(), g); e0 != e1; ++e0) {
								const ADR::GraphEdge& ee(g[*e0]);
								if (!ee.is_match(seq[0].get_airway()))
									continue;
								if (!ee.is_valid(v.get_perfindex()))
									continue;
								break;
							}
							if (e0 == e1)
								continue;
						}
						dist = statev.get_dist();
					}
					if (!(dist < bestdist))
						continue;
					bestdist = dist;
					mrule = mr;
				}
			}
		}
		if (mrule >= mandatory.size()) {
			std::ostringstream oss;
			oss << "lgraphdijkstra: cannot satisfy mandatory rule";
			if (mandatory.size() >= 2)
				oss << 's';
			bool subseq(false);
			for (LGMandatory::const_iterator mi(mandatory.begin()), me(mandatory.end()); mi != me; ++mi) {
				if (subseq)
					oss << ',';
				subseq = true;
				oss << ' ' << mi->get_rule();
			}
			m_signal_log(log_debug0, oss.str());
			return LRoute();
		}
		// remove all but paths to beginning of mandatory alternatives from dijkstra results
		d.mark_all_white();
		{
			const LGMandatoryAlternative& alt(mandatory[mrule]);
			for (unsigned int ma = 0; ma < alt.size(); ++ma) {
				const LGMandatorySequence& seq(alt[ma]);
				if (seq.empty())
					continue;
				for (unsigned int pi = seq[0].get_pi0(); pi <= seq[0].get_pi1(); ++pi) {
					LVertexLevel v(seq[0].get_v(), pi);
					// avoid marking a path that already contains points from the mandatory sequence
					{
						unsigned int mp;
						for (mp = 1; mp < seq.size(); ++mp) {
							const LGMandatoryPoint& pt(seq[mp]);
							if (d.path_contains(v, pt.get_v()))
								break;
						}
						if (mp < seq.size())
							continue;
					}
					const DijkstraCore::LVertexLevelState& statev(d.m_state[v]);
					d.mark_path(v);
					if (v.get_vertex() == u.get_vertex())
						d.m_state[LVertexLevel(m_vertexdep, 0)].set_color(DijkstraCore::gray);
					if (true && (statev.get_pred() != v || v.get_vertex() == u.get_vertex())) {
						std::ostringstream oss;
						oss << "lgraphdijkstra: mandatory rule " << mrule << " alt " << ma << " marking "
						    << lgraphvertexname(g, v) << " dist " << statev.get_dist();
						m_signal_log(log_debug0, oss.str());
					}
				}
			}
		}
		d.mark_white_infinite_selfpred();
		{
			const LGMandatoryAlternative& alt(mandatory[mrule]);
			DijkstraCore dr(g, m_performance, m_eval.get_specialdateeval(), m_vertexdep, m_vertexdest);
			for (unsigned int ma = 0; ma < alt.size(); ++ma) {
				const LGMandatorySequence& seq(alt[ma]);
				if (seq.empty())
					continue;
				DijkstraCore ds(d);
				for (unsigned int mp = 0; ; ++mp) {
					const LGMandatoryPoint& pt(seq[mp]);
					ds.mark_all_white();
					for (unsigned int pi = pt.get_pi0(); pi <= pt.get_pi1(); ++pi) {
						LVertexLevel v(pt.get_v(), pi);
						const DijkstraCore::LVertexLevelState& statev(d.m_state[v]);
						ds.mark_path(v);
						if (v.get_vertex() == u.get_vertex())
							ds.m_state[LVertexLevel(m_vertexdep, 0)].set_color(DijkstraCore::gray);
						if (true && (statev.get_pred() != v || v.get_vertex() == u.get_vertex())) {
							std::ostringstream oss;
							oss << "lgraphdijkstra: mandatory rule " << mrule << " alt " << ma << " point " << mp << " marking "
							    << lgraphvertexname(g, v) << " dist " << statev.get_dist();
							if (false) {
								LRoute r(ds.get_route(v));
								if (!r.empty()) {
									lgraphupdatemetric(g, r);
									oss << " route " << lgraphprint(g, r) << " metric " << r.get_metric();
								}
							}
							m_signal_log(log_debug0, oss.str());
						}
					}
					if (mp + 1 >= seq.size())
						break;
					ds.mark_white_infinite_selfpred();
					ds.rebuild_queue();
					if (!pt.get_airway()) {
						ADR::Graph::out_edge_iterator e0, e1;
						for (boost::tie(e0, e1) = boost::out_edges(pt.get_v(), g); e0 != e1; ++e0) {
							if (boost::target(*e0, g) != seq[mp + 1].get_v())
								continue;
							const ADR::GraphEdge& ee(g[*e0]);
							if (!ee.is_dct())
								continue;
							break;
						}
						if (e0 == e1) {
							const ADR::GraphVertex& uu(g[pt.get_v()]);
							const ADR::GraphVertex& vv(g[seq[mp + 1].get_v()]);
							int minalt(-1);
							double dist(uu.get_coord().spheric_distance_nmi_dbl(vv.get_coord()));
							TopoDb30::Profile prof(m_topodb.get_profile(uu.get_coord(), vv.get_coord(), 5));
							if (prof.empty()) {
								std::ostringstream oss;
								oss << "lgraphdijkstra: DCT: Error computing ground clearance: " << uu.get_ident() << ' '
								    << vv.get_ident() << " dist " << std::setprecision(1) << dist << "nmi";
								m_signal_log(log_debug0, oss.str());
							} else {
								TopoDb30::elev_t el(prof.get_maxelev());
								if (el != TopoDb30::nodata) {
									minalt = prof.get_maxelev() * Point::m_to_ft;
									minalt = std::max(minalt, 0);
									if (minalt >= 5000)
										minalt += 1000;
									minalt += 1000;
								}
							}
							ADR::GraphEdge edge(pis);
 							for (unsigned int pi = 0; pi < pis; ++pi) {
								if (m_performance.get_cruise(pi).get_altitude() < minalt)
									continue;
								edge.set_metric(pi, 1);
							}
							lgraphaddedge(g, edge, pt.get_v(), seq[mp + 1].get_v(), setmetric_metric, false);
							for (boost::tie(e0, e1) = boost::out_edges(pt.get_v(), g); e0 != e1; ++e0) {
								if (boost::target(*e0, g) != seq[mp + 1].get_v())
									continue;
								const ADR::GraphEdge& ee(g[*e0]);
								if (!ee.is_dct())
									continue;
								break;
							}
						}
						if (e0 != e1) {
							const ADR::GraphEdge& ee(g[*e0]);
							if (!solutionedgeonly || ee.is_solution()) {
								for (unsigned int pi = pt.get_pi0(); pi <= pt.get_pi1(); ++pi) {
									if (!ee.is_valid(pi))
										continue;
									LVertexLevel u(pt.get_v(), pi);
									const DijkstraCore::LVertexLevelState& stateu(ds.m_state[u]);
									if (stateu.get_pred() == u || is_metric_invalid(stateu.get_dist()))
										continue;
									LVertexLevel v(seq[mp + 1].get_v(), pi);
									DijkstraCore::LVertexLevelState& statev(ds.m_state[v]);
									if (statev.get_color() == DijkstraCore::black)
										continue;
									statev.set_pred(u);
								        statev.set_dist(stateu.get_dist() + ee.get_metric(pi));
									ADR::timetype_t dt(Point::round<ADR::timetype_t,double>(m_performance.get_cruise(pi).get_secpernmi()
																* ee.get_dist()));
									statev.set_time(stateu.get_time() + dt);
									statev.set_color(DijkstraCore::gray);
									statev.set_edge(ee.get_object());
								}
							}
						}
					} else {
						ds.run(pt.get_uuid(), solutionedgeonly);
					}
				}
				dr.copy_gray_paths(ds);
				if (true) {
					for (unsigned int i = 0, n = boost::num_vertices(g); i < n; ++i) {
						for (unsigned int pi = 0; pi < pis; ++pi) {
							LVertexLevel ii(i, pi);
							if (ds.m_state[ii].get_color() != DijkstraCore::gray)
								continue;
							LRoute r(ds.get_route(ii));
							if (r.empty())
								continue;
							lgraphupdatemetric(g, r);
							std::ostringstream oss;
							oss << "lgraphdijkstra: mandatory rule " << mrule << " alt " << ma << " route " << lgraphprint(g, r) << " metric " << r.get_metric();
							m_signal_log(log_debug0, oss.str());
						}
					}
				}
			}
			d.swap(dr);
			d.rebuild_queue();
			if (true) {
				for (unsigned int i = 0, n = boost::num_vertices(g); i < n; ++i) {
					for (unsigned int pi = 0; pi < pis; ++pi) {
						LVertexLevel ii(i, pi);
						if (d.m_state[ii].get_color() != DijkstraCore::gray)
							continue;
						if (false) {
							std::ostringstream oss;
							oss << "lgraphdijkstra: gray node ";
						        LVertexLevel v(ii);
							for (;;) {
								oss << lgraphvertexname(g, v) << " metric " << d.m_state[v].get_dist();
							        LVertexLevel u(d.m_state[v].get_pred());
								if (u == v)
									break;
								v = u;
								oss << " <-- " << DijkstraCore::to_str(d.m_state[v].get_color()) << ' ';
							}
							m_signal_log(log_debug0, oss.str());
						}
						LRoute r(d.get_route(ii));
						if (r.empty())
							continue;
						lgraphupdatemetric(g, r);
						std::ostringstream oss;
						oss << "lgraphdijkstra: mandatory rule " << mrule << " route " << lgraphprint(g, r) << " metric " << r.get_metric();
						m_signal_log(log_debug0, oss.str());
					}
				}
			}
		}
		mandatory.erase(mandatory.begin() + mrule);
		if (d.m_queue.empty()) {
			std::ostringstream oss;
			oss << "lgraphdijkstra: no route after mandatory processing";
			m_signal_log(log_debug0, oss.str());
			return LRoute();
		}
	}
	LRoute r(d.get_route(v));
	if (r.empty())
		return r;
	lgraphupdatemetric(g, r);
	{
		LVertexLevel x(baseroute.empty() ? (LVertexLevel)u : (LVertexLevel)baseroute[0]);
		if ((LVertexLevel)r[0] != x) {
			std::ostringstream oss;
			oss << "lgraphdijkstra: final route invalid starting point "
			    << lgraphvertexname(g, (LVertexLevel)r[0]) << '(' << ((LVertexLevel)r[0]).get_perfindex()
			    << ") expected " << lgraphvertexname(x) << '(' << x.get_perfindex() << ')';
			m_signal_log(log_debug0, oss.str());
		}
	}
	return r;
}

CFMUAutoroute51::LRoute CFMUAutoroute51::lgraphnextsolution(void)
{
	LRoute r;
	for (;;) {
		bool newroute(m_solutionpool.empty());
		if (newroute) {
			r = lgraphdijkstra(LVertexLevelTime(m_vertexdep, 0, get_deptime()), LVertexLevel(m_vertexdest, 0),
					   LRoute(), false, m_crossingmandatory);
			lgraphupdatemetric(r);
			if (is_metric_invalid(r.get_metric())) {
				r.clear();
				return r;
			}
			m_solutionpool.insert(r);
			// check if already in solution tree; return if not, otherwise use as solution to base other solutions off
			if (!lgraphinsertsolutiontree(r))
				return r;
		}
		r = *m_solutionpool.begin();
		m_solutionpool.erase(m_solutionpool.begin());
		// verify if solution still exists in graph
		bool ok(r.is_existing(m_graph));
		if (true) {
			std::ostringstream oss;
			oss << "k shortest path: Route " << lgraphprint(r) << " metric " << r.get_metric();
			if (ok)
				oss << " still exists";
			else
				oss << " no longer exists";
			m_signal_log(log_debug0, oss.str());
		}
		if (ok)
			break;
		if (newroute) {
			m_signal_log(log_debug0, "Internal Error");
			stop(statusmask_stoppingerrorinternalerror);
		}
	}
	lgraphinsertsolutiontree(r);
	// generate new solutions
	// "abuse" solution flag to enable/disable edges
	{
		ADR::Graph::edge_iterator e0, e1;
		for (tie(e0, e1) = edges(m_graph); e0 != e1; ++e0) {
			ADR::GraphEdge& ee(m_graph[*e0]);
			ee.set_solution(0);
		}
	}
        ADR::Graph::SolutionEdgeFilter filter(m_graph);
        typedef boost::filtered_graph<ADR::Graph, ADR::Graph::SolutionEdgeFilter> fg_t;
        fg_t fg(m_graph, filter);
	LGraphSolutionTreeNode *tree(&m_solutiontree);
	for (LRoute::size_type i(0); i + 1 < r.size(); ++i) {
		if (tree) {
			LGraphSolutionTreeNode::iterator ti(tree->find((LVertexEdge)r[i]));
			if (ti == tree->end())
				tree = 0;
			else
				tree = &ti->second;
		}
		if (tree) {
			for (LGraphSolutionTreeNode::iterator ti(tree->begin()), te(tree->end()); ti != te;) {
				LGraphSolutionTreeNode::iterator tix(ti);
				++ti;
				ADR::Graph::edge_descriptor e;
                                bool ok;
				boost::tie(e, ok) = LRoute::get_edge(r[i], tix->first, m_graph);
				if (!ok) {
					if (true) {
						std::ostringstream oss;
						oss << "k shortest path: Leg " << lgraphvertexname(r[i])
						    << "--" << lgraphawyname(tix->first.get_edge()) << "->"
						    << lgraphvertexname(tix->first) << " does not exist in graph";
						m_signal_log(log_debug0, oss.str());
					}
					tree->erase(tix);
					continue;
				}
				m_graph[e].clear_solution();
			}
		}
		// kill other vertices with the same ident
		{
			ADR::Graph::vertex_descriptor v(r[i].get_vertex());
			const ADR::GraphVertex& vv(m_graph[v]);
			const std::string& ident(vv.get_ident());
			for (ADR::Graph::vertex_descriptor i = 0; i < boost::num_vertices(m_graph); ++i) {
				if (i == v)
					continue;
				const ADR::GraphVertex& ii(m_graph[i]);
				if (ii.get_ident() != ident)
					continue;
				ADR::Graph::out_edge_iterator e0, e1;
				for (boost::tie(e0, e1) = boost::out_edges(v, m_graph); e0 != e1; ++e0) {
					ADR::GraphEdge& ee(m_graph[*e0]);
					ee.clear_solution();
					if (false) {
						std::ostringstream oss;
						oss << "k shortest path: Killing edge " << lgraphvertexname(boost::source(*e0, m_graph))
						    << '(' << (unsigned int)boost::source(*e0, m_graph) << ')'
						    << "--" << ee.get_route_ident_or_dct(true)
						    << "->" << lgraphvertexname(boost::target(*e0, m_graph))
						    << '(' << (unsigned int)boost::target(*e0, m_graph) << ')';
						m_signal_log(log_debug0, oss.str());
					}
				}
			}
		}
		LRoute r1;
		r1 = lgraphdijkstra(r[i], LVertexLevel(m_vertexdest, 0), r, true, m_crossingmandatory);
		if (false && tree) {
			ADR::Graph::out_edge_iterator e0, e1;
			for (boost::tie(e0, e1) = boost::out_edges(r[i].get_vertex(), m_graph); e0 != e1; ++e0) {
				ADR::GraphEdge& ee(m_graph[*e0]);
				if (boost::target(*e0, m_graph) == r[i+1].get_vertex() &&
				    ee.get_object() == r[i+1].get_edge())
					continue;
				ee.clear_solution();
			}
		}
		// kill all out-edges to avoid revisiting a node already in the solution
		{
			ADR::Graph::out_edge_iterator e0, e1;
			for (boost::tie(e0, e1) = boost::out_edges(r[i].get_vertex(), m_graph); e0 != e1; ++e0) {
				ADR::GraphEdge& ee(m_graph[*e0]);
				ee.clear_solution();
				if (false) {
					std::ostringstream oss;
					oss << "k shortest path: Killing edge " << lgraphvertexname(boost::source(*e0, m_graph))
					    << '(' << (unsigned int)boost::source(*e0, m_graph) << ')'
					    << "--" << ee.get_route_ident_or_dct(true)
					    << "->" << lgraphvertexname(boost::target(*e0, m_graph))
					    << '(' << (unsigned int)boost::target(*e0, m_graph) << ')';
					m_signal_log(log_debug0, oss.str());
				}
			}
		}
		if (r1.size() < 2)
			continue;
		lgraphupdatemetric(r1);
		if (is_metric_invalid(r1.get_metric())) {
			if (true) {
				std::ostringstream oss;
				oss << "k shortest path: Route";
		        	for (LRoute::const_iterator rb(r1.begin()), ri(rb), re(r1.end()); ri != re; ++ri) {
					if (ri != rb)
						oss << ' ' << lgraphawyname(ri->get_edge());
					oss << ' ' << lgraphvertexname((LVertexLevel)*ri);
				}
				oss << " has no metric";
				m_signal_log(log_debug0, oss.str());
			}
			continue;
		}
		if (true) {
			std::ostringstream oss;
			oss << "k shortest path: Route";
			for (LRoute::const_iterator rb(r1.begin()), ri(rb), re(r1.end()); ri != re; ++ri) {
				if (ri != rb)
					oss << ' ' << lgraphawyname(ri->get_edge());
				oss << ' ' << lgraphvertexname((LVertexLevel)*ri) << '(' << (unsigned int)ri->get_vertex() << ')';
			}
			oss << " metric " << r1.get_metric() << " added to solution pool";
			m_signal_log(log_debug0, oss.str());
		}
		m_solutionpool.insert(r1);
	}
	// check for too many solutions in the pool
	while (!m_solutionpool.empty() && !m_solutionpool.begin()->is_existing(m_graph))
		m_solutionpool.erase(m_solutionpool.begin());
	{
		static const lgraphsolutionpool_t::size_type maxsz(16384);
		lgraphsolutionpool_t::size_type sz(m_solutionpool.size());
		if (sz > maxsz) {
			lgraphsolutionpool_t::size_type n(0);
			for (lgraphsolutionpool_t::iterator i(m_solutionpool.begin()), e(m_solutionpool.end()); i != e; ) {
				lgraphsolutionpool_t::iterator i2(i);
				++i;
				if (n >= maxsz || !i2->is_existing(m_graph)) {
					m_solutionpool.erase(i2);
					continue;
				}
				++n;
			}
		}
	}
	if (m_solutionpool.empty())
		return LRoute();
	return *m_solutionpool.begin();
}

void CFMUAutoroute51::lgraphlogtofile(void)
{
	std::string filename("/tmp/graph_XXXXXX");
	{
		int fd(Glib::mkstemp(filename));
		std::ofstream ofg(filename.c_str());
		close(fd);
		m_graph.print(ofg) << std::endl;
	}
	std::ostringstream oss;
	oss << "Logging Routing Graph: " << filename;
	m_signal_log(log_debug0, oss.str());
}

void CFMUAutoroute51::lgraphroute(void)
{
	if (false)
		lgraphlogtofile();
	for (;;) {
		if (m_vertexdep >= boost::num_vertices(m_graph) || m_vertexdest >= boost::num_vertices(m_graph) ||
		    m_vertexdep == m_vertexdest) {
			m_signal_log(log_debug0, "Invalid departure and/or destination, stopping...");
			stop(statusmask_stoppingerrorinternalerror);
			return;
		}
		if (m_iterationcount[0] > m_maxlocaliteration || m_iterationcount[1] > m_maxremoteiteration) {
			stop(statusmask_stoppingerroriteration);
			return;
		}
		if (is_stopped())
			return;
		lgraphclearcurrent();
		LRoute r;
		{
			Glib::TimeVal tv;
			tv.assign_current_time();
			r = lgraphnextsolution();
			{
				Glib::TimeVal tv1;
				tv1.assign_current_time();
				tv = tv1 - tv;
			}
			if (true) {
				std::ostringstream oss;
				oss << "Next route computation: " << std::fixed << std::setprecision(3) << tv.as_double()
				    << "s, solution pool size " << m_solutionpool.size();
				m_signal_log(log_debug0, oss.str());
			}
		}
		if (r.size() < 2) {
			if (true) {
				unsigned int countsid(0), countstar(0);
				{
					ADR::Graph::out_edge_iterator e0, e1;
					for(boost::tie(e0, e1) = boost::out_edges(m_vertexdep, m_graph); e0 != e1; ++e0) {
						const ADR::GraphEdge& ee(m_graph[*e0]);
						if (!ee.is_valid() || ee.is_noroute())
							continue;
						++countsid;
					}
				}
				{
					ADR::Graph::in_edge_iterator e0, e1;
					for(boost::tie(e0, e1) = boost::in_edges(m_vertexdest, m_graph); e0 != e1; ++e0) {
						const ADR::GraphEdge& ee(m_graph[*e0]);
						if (!ee.is_valid() || ee.is_noroute())
							continue;
						++countstar;
					}
				}
				std::ostringstream oss;
				oss << "No route found; " << countsid << " SID edges, " << countstar << " STAR edges";
				m_signal_log(log_normal, oss.str());
				statusmask_t sm(statusmask_none);
				if (!countsid)
					sm |= statusmask_stoppingerrorsid;
				if (!countstar)
					sm |= statusmask_stoppingerrorstar;
				if (!sm)
					sm |= statusmask_stoppingerrorenroute;
				stop(sm);
			}
			return;
		}
		if (!lgraphsetcurrent(r)) {
			stop(statusmask_stoppingerrorinternalerror);
			return;
		}
		if (r.is_repeated_nodes(m_graph)) {
			if (true) {
				std::ostringstream oss;
				oss << "Flight Plan has duplicate points " << lgraphprint(r);
				m_signal_log(log_debug0, oss.str());
			}
			continue;
		}
		m_signal_log(log_fplproposal, get_simplefpl());
		if (true) {
			std::ostringstream oss;
			oss << "Proposed metric " << r.get_metric() << " fpl " << get_simplefpl();
			m_signal_log(log_debug0, oss.str());
		}
		bool nochange(lgraphroutelocaltfr());
		if (is_stopped())
			return;
		if (nochange)
			break;
		m_signal_log(log_fpllocalvalidation, "");
		++m_iterationcount[0];
		if (true) {
			Glib::signal_idle().connect_once(sigc::mem_fun(*this, &CFMUAutoroute51::lgraphroute), Glib::PRIORITY_HIGH_IDLE);
			return;
		}
	}
	lgraphsubmitroute();
}

bool CFMUAutoroute51::lgraphroutelocaltfr(void)
{
	if (!get_tfr_enabled())
		return true;
	ADR::RestrictionResults res;
	bool work(lgraphlocaltfrcheck(res));
	bool nochange(res.is_ok());
	if (nochange)
		return true;
	LGMandatory mrules(m_crossingmandatory);
	ADR::Graph graph(m_graph);
	for (unsigned int mdepth = 0; mdepth < 3; ++mdepth) {
		LGMandatory mrules1(lgraphmandatoryroute(res));
		if (mrules1.empty()) {
			if (mdepth > 0) {
				m_signal_log(log_debug0, "Mandatory Routing unsuccessful");
				ADR::FlightPlan f(m_eval.get_fplan());
				bool work(false);
				for (ADR::RestrictionResults::const_iterator ri(res.begin()), re(res.end()); ri != re; ++ri) {
					const ADR::RestrictionResult& rule(*ri);
					lgraphloglocaltfr(rule, true);
					if (nochange)
						continue;
					const ADR::FlightRestrictionTimeSlice& tsrule(rule.get_timeslice());
					if (!tsrule.is_valid() || !tsrule.is_enabled())
						continue;
					work = lgraphtfrruleresult(m_graph, rule, f) || work;
				}
				if (work)
					lgraphmodified();
			}
			return false;
		}
		LRoute rbest;
		LGMandatory mrulesbest;
		ADR::Graph graphbest;
		double metricbest(std::numeric_limits<double>::max());
		for (LGMandatory::const_iterator mi(mrules1.begin()), me(mrules1.end()); mi != me; ++mi) {
			LGMandatory mrules2(mrules);
			mrules2.insert(mrules2.end(), *mi);
			if (true) {
				m_signal_log(log_debug0, "Trying Mandatory Routing");
				for (unsigned int i = 0; i < mrules2.size(); ++i) {
					const LGMandatoryAlternative& alt(mrules2[i]);
					{
						std::ostringstream oss;
						oss << "  Mandatory Rule " << i << ' ' << alt.get_rule();
						m_signal_log(log_debug0, oss.str());
					}
					for (unsigned int j = 0; j < alt.size(); ++j) {
						const LGMandatorySequence& seq(alt[j]);
						std::ostringstream oss;
						oss << "    ";
						for (unsigned int k = 0; k < seq.size(); ++k) {
							const LGMandatoryPoint& pt(seq[k]);
							const ADR::GraphVertex& vv(m_graph[pt.get_v()]);
							oss << vv.get_ident();
							unsigned int pi0(pt.get_pi0());
							unsigned int pi1(pt.get_pi1());
							if (pi0 < m_performance.size() && pi1 < m_performance.size())
								oss << ' ' << m_performance.get_cruise(pi0).get_fplalt()
								    << ".." << m_performance.get_cruise(pi1).get_fplalt();
							if (k + 1 < seq.size())
								oss << ' ' << lgraphawyname(pt.get_airway()) << ' ';
						}
						m_signal_log(log_debug0, oss.str());
					}
				}
			}
			LRoute r(lgraphdijkstra(graph, LVertexLevelTime(m_vertexdep, 0, get_deptime()), LVertexLevel(m_vertexdest, 0),
						LRoute(), false, mrules2));
			if (r.empty()) {
				m_signal_log(log_debug0, "No Mandatory Route found");
			} else {
				lgraphupdatemetric(graph, r);
				if (true) {
					std::ostringstream oss;
					oss << "Mandatory Route " << lgraphprint(graph, r) << " metric " << r.get_metric();
					m_signal_log(log_debug0, oss.str());
				}
				if (r.get_metric() < metricbest && !ADR::Graph::is_metric_invalid(r.get_metric())) {
					metricbest = r.get_metric();
					mrulesbest = mrules2;
					rbest = r;
					graphbest = graph;
				}
			}
			if (is_stopped()) {
				m_signal_log(log_debug0, "Mandatory Routing unsuccessful");
				return nochange;
			}
			// check whether avoiding the mandatory rule instead of complying with it gives a better route
			for (LGMandatory::const_iterator mi(mrules1.begin()), me(mrules1.end()); mi != me; ++mi) {
				for (unsigned int i = 0; i < res.size(); ++i) {
					const ADR::FlightRestrictionTimeSlice& rule(res[i].get_timeslice());
					if (!rule.is_valid() || !rule.is_enabled())
						continue;
					if (rule.get_type() != ADR::FlightRestrictionTimeSlice::type_mandatory &&
					    rule.get_type() != ADR::FlightRestrictionTimeSlice::type_allowed)
						continue;
					if (rule.get_ident() != mi->get_rule())
						continue;
					std::vector<ADR::RuleSegment> cc(rule.get_crossingcond());
					if (cc.empty())
						continue;
					ADR::Graph g(graph);
					bool work(false);
					for (std::vector<ADR::RuleSegment>::const_iterator ci(cc.begin()), ce(cc.end()); ci != ce; ++ci)
						work = lgraphtfrrulekillforbidden(g, *ci, false) || work;
					if (!work)
						continue;
					r = lgraphdijkstra(g, LVertexLevelTime(m_vertexdep, 0, get_deptime()), LVertexLevel(m_vertexdest, 0),
							   LRoute(), false, mrules);
					if (r.empty()) {
						m_signal_log(log_debug0, "No Avoiding Mandatory Route found");
					} else {
						lgraphupdatemetric(g, r);
						if (true) {
							std::ostringstream oss;
							oss << "Avoiding Mandatory Route " << lgraphprint(r) << " metric " << r.get_metric();
							m_signal_log(log_debug0, oss.str());
						}
						if (r.get_metric() < metricbest && !ADR::Graph::is_metric_invalid(r.get_metric())) {
							metricbest = r.get_metric();
							mrulesbest = mrules;
							rbest = r;
							graphbest.swap(g);
						}
					}
				}
			}
			if (is_stopped()) {
				m_signal_log(log_debug0, "Mandatory Routing unsuccessful");
				return nochange;
			}
		}
		if (metricbest == std::numeric_limits<double>::max()) {
			// try to find a mandatory route that can be prevented by preventing its conditions
			for (LGMandatory::const_iterator mi(mrules1.begin()), me(mrules1.end()); mi != me; ++mi) {
				for (unsigned int i = 0; i < res.size(); ++i) {
					const ADR::FlightRestrictionTimeSlice& rule(res[i].get_timeslice());
					if (!rule.is_valid() || !rule.is_enabled())
						continue;
					if (rule.get_type() != ADR::FlightRestrictionTimeSlice::type_mandatory && 
					    rule.get_type() != ADR::FlightRestrictionTimeSlice::type_allowed)
						continue;
					if (rule.get_ident() != mi->get_rule())
						continue;
					std::vector<ADR::RuleSegment> cc(rule.get_crossingcond());
					if (cc.empty())
						continue;
					lgraphloglocaltfr(res[i], false);
					bool work(false);
					for (std::vector<ADR::RuleSegment>::const_iterator ci(cc.begin()), ce(cc.end()); ci != ce; ++ci)
						work = lgraphtfrrulekillforbidden(*ci, true) || work;
				}
			}
			break;
		}
		mrules.swap(mrulesbest);
		graph.swap(graphbest);
		lgraphinsertsolutiontree(rbest);
		if (!lgraphsetcurrent(rbest))
			break;
		m_signal_log(log_fplproposal, get_simplefpl());
		if (true) {
			std::ostringstream oss;
			oss << "Proposed metric " << rbest.get_metric() << " fpl " << get_simplefpl();
			m_signal_log(log_debug0, oss.str());
		}
		work = lgraphlocaltfrcheck(graph, res) || work;
		nochange = res.is_ok();
		if (nochange)
			break;
	}
	m_signal_log(log_debug0, nochange ? "Mandatory Routing successful" : "Mandatory Routing unsuccessful");
	return nochange;
}

void CFMUAutoroute51::lgraphlogrule(const ADR::FlightRestrictionTimeSlice& tsrule, ADR::timetype_t tm)
{
	if (!tsrule.is_valid()) {
		m_signal_log(log_graphrule, "I");
		m_signal_log(log_graphruledesc, "");
		m_signal_log(log_graphruleoprgoal, "");
		m_signal_log(log_fpllocalvalidation, "I");
		return;
	}
	typedef ADR::RestrictionResult::set_t set_t;
	std::ostringstream oss;
	oss << tsrule.get_ident() << ' ' << tsrule.get_type_char();
	if (tsrule.is_dct() || tsrule.is_strict_dct() || tsrule.is_unconditional() || tsrule.is_routestatic() ||
	    tsrule.is_mandatoryinbound() || tsrule.is_mandatoryoutbound() || !tsrule.is_enabled() ||
	    tsrule.is_trace() || tsrule.is_keep()) {
		oss << " (";
		if (!tsrule.is_enabled())
			oss << '-';
		if (tsrule.is_trace())
			oss << 'T';
		{
			ADR::Condition::civmil_t civmil;
			if (tsrule.is_strict_dct(civmil))
				oss << 'D';
			else if (tsrule.is_dct(civmil))
				oss << 'd';
			else
				civmil = ADR::Condition::civmil_invalid;
			switch (civmil) {
			case ADR::Condition::civmil_civ:
				oss << 'c';
				break;

			case ADR::Condition::civmil_mil:
				oss << 'm';
				break;

			default:
				break;
			}
		}
		if (tsrule.is_unconditional())
			oss << 'U';
		if (tsrule.is_routestatic())
			oss << 'S';
		if (tsrule.is_mandatoryinbound())
			oss << 'I';
		if (tsrule.is_mandatoryoutbound())
			oss << 'O';
		if (tsrule.is_keep())
			oss << 'K';
		oss << ')';
	}
	{
		std::vector<ADR::RuleSegment> cc(tsrule.get_crossingcond());
		if (!cc.empty()) {
			oss << " X:";
			for (std::vector<ADR::RuleSegment>::const_iterator ci(cc.begin()), ce(cc.end()); ci != ce; ++ci)
				oss << ' ' << ci->to_shortstr(tm);
		}
	}
	{
		std::vector<ADR::RuleSequence> mand(tsrule.get_mandatory());
		if (!mand.empty()) {
			oss << " M:";
			bool subseq(false);
			for (std::vector<ADR::RuleSequence>::const_iterator mi(mand.begin()), me(mand.end()); mi != me; ++mi) {
				if (subseq)
					oss << " /";
				subseq = true;
				oss << ' ' << mi->to_shortstr(tm);
			}
		}
	}
	for (unsigned int i(0), n(tsrule.get_restrictions().size()); i < n; ++i) {
		if (i)
			oss << " /";
		oss << ' ' << tsrule.get_restrictions()[i].to_shortstr(tm);
	}
	m_signal_log(log_graphrule, oss.str());
	m_signal_log(log_graphruledesc, tsrule.get_instruction());
	m_signal_log(log_graphruleoprgoal, "");
}

void CFMUAutoroute51::lgraphloglocaltfr(const ADR::RestrictionResult& rule, bool logvalidation)
{
	const ADR::FlightRestrictionTimeSlice& tsrule(rule.get_timeslice());
	if (!tsrule.is_valid()) {
		m_signal_log(log_graphrule, "I");
		m_signal_log(log_graphruledesc, "");
		m_signal_log(log_graphruleoprgoal, "");
		m_signal_log(log_fpllocalvalidation, "I");
		return;
	}
	typedef ADR::RestrictionResult::set_t set_t;
	std::ostringstream oss;
	oss << tsrule.get_ident() << ' ' << tsrule.get_type_char();
	if (tsrule.is_dct() || tsrule.is_unconditional() || tsrule.is_routestatic() || tsrule.is_mandatoryinbound() || !tsrule.is_enabled()) {
		oss << " (";
		if (!tsrule.is_enabled())
			oss << '-';
		if (!tsrule.is_dct())
			oss << 'D';
		if (!tsrule.is_unconditional())
			oss << 'U';
		if (!tsrule.is_routestatic())
			oss << 'S';
		if (!tsrule.is_mandatoryinbound())
			oss << 'I';
		oss << ')';
	}
	{
		std::vector<ADR::RuleSegment> cc(tsrule.get_crossingcond());
		if (!cc.empty()) {
			oss << " X:";
			for (std::vector<ADR::RuleSegment>::const_iterator ci(cc.begin()), ce(cc.end()); ci != ce; ++ci)
				oss << ' ' << ci->to_shortstr(rule.get_time());
		}
	}
	{
		std::vector<ADR::RuleSequence> mand(tsrule.get_mandatory());
		if (!mand.empty()) {
			oss << " M:";
			bool subseq(false);
			for (std::vector<ADR::RuleSequence>::const_iterator mi(mand.begin()), me(mand.end()); mi != me; ++mi) {
				if (subseq)
					oss << " /";
				subseq = true;
				oss << ' ' << mi->to_shortstr(rule.get_time());
			}
		}
	}
	if (!rule.get_vertexset().empty()) {
		oss << " V:";
		bool subseq(false);
		for (set_t::const_iterator i(rule.get_vertexset().begin()), e(rule.get_vertexset().end()); i != e; ++i) {
			if (subseq)
				oss << ',';
			subseq = true;
			oss << *i;
		}
	}
	if (!rule.get_edgeset().empty()) {
		oss << " E:";
		bool subseq(false);
		for (set_t::const_iterator i(rule.get_edgeset().begin()), e(rule.get_edgeset().end()); i != e; ++i) {
			if (subseq)
				oss << ',';
			subseq = true;
			oss << *i;
		}
	}
	if (!rule.has_refloc())
		oss << " L:" << rule.get_refloc();
	{
		bool subseq(true);
		for (ADR::RestrictionResult::const_iterator ri(rule.begin()), re(rule.end()); ri != re; ++ri) {
			const ADR::RestrictionSequenceResult alt(*ri);
			if (subseq)
				oss << " /";
			subseq = true;
			if (!alt.get_vertexset().empty()) {
				oss << " V:";
				bool subseq(false);
				for (set_t::const_iterator i(alt.get_vertexset().begin()), e(alt.get_vertexset().end()); i != e; ++i) {
					if (subseq)
						oss << ',';
					subseq = true;
					oss << *i;
				}
			}
			if (!alt.get_edgeset().empty()) {
				oss << " E:";
				bool subseq(false);
				for (set_t::const_iterator i(alt.get_edgeset().begin()), e(alt.get_edgeset().end()); i != e; ++i) {
					if (subseq)
						oss << ',';
					subseq = true;
					oss << *i;
				}
			}
			oss << ' ' << alt.get_sequence().to_shortstr(rule.get_time());
		}
	}
	m_signal_log(log_graphrule, oss.str());
	m_signal_log(log_graphruledesc, tsrule.get_instruction());
	m_signal_log(log_graphruleoprgoal, "");
	m_signal_log(log_fpllocalvalidation, oss.str());
}

bool CFMUAutoroute51::lgraphlocaltfrcheck(ADR::Graph& g, ADR::RestrictionResults& res)
{
	if (!get_tfr_enabled()) {
		res = ADR::RestrictionResults();
		return false;
	}
	bool nochange;
	{
		Glib::TimeVal tv;
		tv.assign_current_time();
		m_eval.set_graph(&g);
		nochange = m_eval.check_fplan(get_honour_profilerules());
		res = m_eval.get_results();
		m_eval.set_graph(&m_graph);
		{
			Glib::TimeVal tv1;
			tv1.assign_current_time();
			tv = tv1 - tv;
		}
		if (true) {
			std::ostringstream oss;
			oss << "Local TFR Check: " << (nochange ? "OK" : "NOT OK") << ", " << res.size()
			    << " rules " << (res.is_ok() ? "OK" : "NOT OK") << ' '
			    << std::fixed << std::setprecision(3) << tv.as_double() << 's';
			m_signal_log(log_debug0, oss.str());
		}
	}
        // Parsed Flight Plan
        if (false) {
                for (unsigned int i = 0; i < m_route.get_nrwpt(); ++i) {
                        const FPlanWaypoint& wpt(m_route[i]);
                        std::ostringstream oss;
                        oss << "PFPL[" << i << "]: " << wpt.get_icao() << '/' << wpt.get_name() << ' ';
                        switch (wpt.get_pathcode()) {
                        case FPlanWaypoint::pathcode_sid:
                        case FPlanWaypoint::pathcode_star:
                        case FPlanWaypoint::pathcode_airway:
                                oss << wpt.get_pathname();
                                break;

                        case FPlanWaypoint::pathcode_directto:
                                oss << "DCT";
                                break;

                        default:
                                oss << '-';
                                break;
                        }
                        oss << ' ' << wpt.get_coord().get_lat_str2() << ' ' << wpt.get_coord().get_lon_str2()
                            << ' ' << (wpt.is_standard() ? 'F' : 'A') << std::setw(3) << std::setfill('0') << ((wpt.get_altitude() + 50) / 100)
                            << ' ' << (wpt.is_ifr() ? 'I' : 'V') << "FR";
			m_signal_log(log_debug0, oss.str());
		}
        }
	// Flight Plan
	ADR::FlightPlan f(m_eval.get_fplan());
	if (false) {
		{
                        std::ostringstream oss;
                        oss << "FR: " << f.get_aircrafttype() << ' ' << Aircraft::get_aircrafttypeclass(f.get_aircrafttype()) << ' '
                            << f.get_equipment() << " PBN/" << f.get_pbn_string() << ' ' << f.get_flighttype();
			m_signal_log(log_debug0, oss.str());
                }
		for (unsigned int i = 0; i < f.size(); ++i) {
                        const ADR::FPLWaypoint& wpt(f[i]);
                        std::ostringstream oss;
                        oss << "F[" << i << '/' << wpt.get_wptnr() << "]: " << wpt.get_ident() << ' ' << wpt.get_type_string() << ' ';
                        switch (wpt.get_pathcode()) {
                        case FPlanWaypoint::pathcode_sid:
                        case FPlanWaypoint::pathcode_star:
                        case FPlanWaypoint::pathcode_airway:
                                oss << wpt.get_pathname();
                                break;

			case FPlanWaypoint::pathcode_directto:
                                oss << "DCT";
                                break;

                        default:
                                oss << '-';
                                break;
                        }
                        oss << ' ' << wpt.get_coord().get_lat_str2() << ' ' << wpt.get_coord().get_lon_str2()
                            << ' ' << (wpt.is_standard() ? 'F' : 'A') << std::setw(3) << std::setfill('0') << ((wpt.get_altitude() + 50) / 100)
                            << ' ' << wpt.get_icao() << '/' << wpt.get_name() << ' ' << (wpt.is_ifr() ? 'I' : 'V') << "FR";
			{
				time_t t(wpt.get_time_unix());
				struct tm tm;
				if (gmtime_r(&t, &tm)) {
					oss << ' ' << std::setw(2) << std::setfill('0') << tm.tm_hour
					    << ':' << std::setw(2) << std::setfill('0') << tm.tm_min;
					if (t >= 48 * 60 * 60)
						oss << ' ' << std::setw(4) << std::setfill('0') << (tm.tm_year + 1900)
						    << '-' << std::setw(2) << std::setfill('0') << (tm.tm_mon + 1)
						    << '-' << std::setw(2) << std::setfill('0') << tm.tm_mday;
				}
			}
			if (wpt.get_ptobj())
				oss << " ptobj " << wpt.get_ptobj()->get_uuid();
			if (wpt.get_pathobj())
				oss << " pathobj " << wpt.get_pathobj()->get_uuid();
			if (wpt.is_expanded())
				oss << " (E)";
			m_signal_log(log_debug0, oss.str());
                 }
        }
 	// Log Messages
	for (ADR::RestrictionEval::messages_const_iterator mi(m_eval.messages_begin()), me(m_eval.messages_end()); mi != me; ++mi) {
		const ADR::Message& msg(*mi);
		logmessage(msg);
		if (!nochange)
			lgraphtfrmessage(msg, f);
	}
	bool work(false);
	for (ADR::RestrictionResults::const_iterator ri(res.begin()), re(res.end()); ri != re; ++ri) {
		const ADR::RestrictionResult& rule(*ri);
		lgraphloglocaltfr(rule, true);
		if (nochange)
			continue;
		const ADR::FlightRestrictionTimeSlice& tsrule(rule.get_timeslice());
		if (!tsrule.is_valid() || !tsrule.is_enabled())
			continue;
		work = lgraphtfrruleresult(g, rule, f) || work;
	}
	m_signal_log(log_fpllocalvalidation, "");
	if (work && &g == &m_graph)
		lgraphmodified();
	return work;
}

void CFMUAutoroute51::lgraphmandatoryroute(LGMandatoryAlternative& m, const ADR::RuleSequence& alt)
{
	if (!alt.size())
		return;
	const unsigned int pis(m_performance.size());
	bool altok(true);
	LGMandatorySequence mseq;
	for (unsigned int j = 0; j < alt.size();) {
		const ADR::RuleSegment& seq(alt[j]);
		++j;
		unsigned int pimin(std::numeric_limits<unsigned int>::max()), pimax(std::numeric_limits<unsigned int>::min());
		for (unsigned int pi = 0; pi < pis; ++pi) {
			const Performance::Cruise& cruise(m_performance.get_cruise(pi));
			if (!seq.get_alt().is_inside(cruise.get_altitude()))
				continue;
			pimin = std::min(pimin, pi);
			pimax = std::max(pimax, pi);
		}
		if (pimin > pimax) {
			altok = false;
			j = ~0U;
			break;
		}
		ADR::Graph::vertex_descriptor v0(~0U), v1(~0U);
		ADR::Object::const_ptr_t awy;
		switch (seq.get_type()) {
		default:
			altok = false;
			j = ~0U;
			break;

		case ADR::RuleSegment::type_airway:
			awy = seq.get_airway();
			if (!awy) {
				altok = false;
				j = ~0U;
				break;
			}
			// fall through

		case ADR::RuleSegment::type_dct:
		{
			std::pair<ADR::Graph::vertex_descriptor,bool> vd(m_graph.find_vertex(seq.get_wpt1()));
			if (!vd.second || vd.first >= boost::num_edges(m_graph)) {
				altok = false;
				j = ~0U;
				break;
			}
			v1 = vd.first;
		}
		// fall through

		case ADR::RuleSegment::type_point:
		{
			std::pair<ADR::Graph::vertex_descriptor,bool> vd(m_graph.find_vertex(seq.get_wpt0()));
			if (!vd.second || vd.first >= boost::num_edges(m_graph)) {
				altok = false;
				j = ~0U;
				break;
			}
			v0 = vd.first;
			mseq.push_back(LGMandatoryPoint(v0, pimin, pimax, awy));
			if (v1 < boost::num_edges(m_graph))
				mseq.push_back(LGMandatoryPoint(v1, pimin, pimax, ADR::GraphEdge::get_matchall()));
			break;
		}

		case ADR::RuleSegment::type_sid:
		{
			awy = seq.get_airway();
			if (!awy) {
				altok = false;
				j = ~0U;
				break;
			}
			v0 = m_vertexdep;
			ADR::Graph::out_edge_iterator e0, e1;
			for (boost::tie(e0, e1) = boost::out_edges(v0, m_graph); e0 != e1; ++e0) {
				ADR::GraphEdge& ee(m_graph[*e0]);
				if (!ee.is_match(awy))
					continue;
				v1 = boost::target(*e0, m_graph);
				break;
			}
			if (v1 >= boost::num_edges(m_graph)) {
				altok = false;
				j = ~0U;
				break;
			}
			mseq.push_back(LGMandatoryPoint(v0, pimin, pimax, awy));
			mseq.push_back(LGMandatoryPoint(v1, pimin, pimax, ADR::GraphEdge::get_matchall()));
			break;
		}

		case ADR::RuleSegment::type_star:
		{
			awy = seq.get_airway();
			if (!awy) {
				altok = false;
				j = ~0U;
				break;
			}
			v1 = m_vertexdest;
			ADR::Graph::in_edge_iterator e0, e1;
			for (boost::tie(e0, e1) = boost::in_edges(v1, m_graph); e0 != e1; ++e0) {
				ADR::GraphEdge& ee(m_graph[*e0]);
				if (!ee.is_match(awy))
					continue;
				v0 = boost::source(*e0, m_graph);
				break;
			}
			if (v0 >= boost::num_edges(m_graph)) {
				altok = false;
				j = ~0U;
				break;
			}
			mseq.push_back(LGMandatoryPoint(v0, pimin, pimax, awy));
			mseq.push_back(LGMandatoryPoint(v1, pimin, pimax, ADR::GraphEdge::get_matchall()));
			break;
		}
		}
	}
	if (!altok || mseq.empty())
		return;
	for (unsigned int i = 1, n = mseq.size(); i < n;) {
		if (mseq[i-1].get_v() != mseq[i].get_v()) {
			++i;
			continue;
		}
		mseq.erase(mseq.begin() + (i - 1));
		n = mseq.size();
	}
	m.push_back(mseq);
}

CFMUAutoroute51::LGMandatory CFMUAutoroute51::lgraphmandatoryroute(const ADR::RestrictionResults& res)
{
	// check whether the remaining violated rules are all mandatory and can be handled
	if (res.empty())
		return LGMandatory();
	LGMandatory m;
	for (ADR::RestrictionResults::const_iterator ri(res.begin()), re(res.end()); ri != re; ++ri) {
		const ADR::RestrictionResult& rule(*ri);
		const ADR::FlightRestrictionTimeSlice& tsrule(rule.get_timeslice());
		if (!tsrule.is_valid() || !tsrule.is_enabled())
			continue;
		if (tsrule.get_type() != ADR::FlightRestrictionTimeSlice::type_mandatory &&
		    tsrule.get_type() != ADR::FlightRestrictionTimeSlice::type_allowed)
			return LGMandatory();
		if (tsrule.is_strict_dct() || tsrule.is_deparr_dct())
			return LGMandatory();
		if (false)
			tsrule.print(std::cerr << "Mandatory rule " << tsrule.get_ident() << std::endl);
		m.push_back(LGMandatoryAlternative(tsrule.get_ident()));
		for (ADR::RestrictionResult::const_iterator ri(rule.begin()), re(rule.end()); ri != re; ++ri) {
			if (false)
				std::cerr << "Mandatory " << tsrule.get_ident() << ": " << ri->get_sequence().to_shortstr(get_deptime()) << std::endl;
			lgraphmandatoryroute(m.back(), ri->get_sequence());
		}
		{
			std::vector<ADR::RuleSequence> mand(tsrule.get_mandatory());
			for (std::vector<ADR::RuleSequence>::const_iterator mi(mand.begin()), me(mand.end()); mi != me; ++mi)
				lgraphmandatoryroute(m.back(), *mi);
		}
		if (false)
			std::cerr << "Mandatory " << tsrule.get_ident() << " count " << m.back().size() << std::endl;
		if (m.back().empty())
			return LGMandatory();
	}
	return m;
}

void CFMUAutoroute51::lgraphsubmitroute(void)
{
	if (m_route.get_flightrules() == 'V') {
		m_validationresponse.push_back("NO ERRORS");
		m_signal_log(log_fpllocalvalidation, m_validationresponse.back());
		m_signal_log(log_fpllocalvalidation, "");
		parse_response();
		return;
	}
	m_tvvalidatestart.assign_current_time();
	++m_iterationcount[1];
	if (!child_is_running()) {
		child_close();
		child_run();
	}
	if (!child_is_running()) {
		m_signal_log(log_debug0, "Cannot restart child, stopping...");
		stop(statusmask_stoppingerrorvalidatortimeout);
		return;
	}
	child_send(get_simplefpl() + "\n");
	m_childtimeoutcount = 0;
	m_connchildtimeout.disconnect();
	m_connchildtimeout = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &CFMUAutoroute51::child_timeout), child_get_timeout());
}

bool CFMUAutoroute51::lgraphtfrrulekillforbidden(ADR::Graph& g, const ADR::RuleSegment& seq, bool log)
{
	switch (seq.get_type()) {
	case ADR::RuleSegment::type_airspace:
	{
		const ADR::AirspaceTimeSlice& ts(seq.get_wpt0()->operator()(get_deptime()).as_airspace());
		if (!ts.is_valid()) {
			std::ostringstream oss;
			oss << "lgraphtfrruleresult: airspace not valid for UUID " << seq.get_uuid0();
			m_signal_log(log_debug0, oss.str());
			return false;
		}
		Rect bbox;
		ts.get_bbox(bbox);
		unsigned int pi0(~0U), pi1(~0U);
		for (unsigned int pi = 0; pi < m_performance.size(); ++pi) {
			const Performance::Cruise& cruise(m_performance.get_cruise(pi));
			if (!seq.get_alt().is_inside(cruise.get_altitude()))
				continue;
			if (pi0 == ~0U)
				pi0 = pi;
			pi1 = pi;
		}
		if (log) {
			std::ostringstream oss;
			oss << "lgraphtfrruleresult: excluding airspace " << ts.get_ident() << '/' << ts.get_type();
			{
				std::string x(seq.get_alt().to_shortstr());
				if (!x.empty())
					oss << ' ' << x;
			}
			if (pi0 != ~0U)
				oss << " (" << m_performance.get_cruise(pi0).get_fplalt()
				    << ".." << m_performance.get_cruise(pi1).get_fplalt()
				    << ')';
			m_signal_log(log_debug0, oss.str());
		}
		if (pi0 == ~0U)
			return false;
		Glib::TimeVal tv;
		tv.assign_current_time();
		unsigned int edgecnt(0);
		ADR::Graph::vertex_iterator v0i, ve;
		for (boost::tie(v0i, ve) = boost::vertices(g); v0i != ve; ++v0i) {
			if ((!get_departure_ifr() && *v0i == m_vertexdep) ||
			    (!get_destination_ifr() && *v0i == m_vertexdest))
				continue;
			const ADR::GraphVertex& vv0(g[*v0i]);
			ADR::Graph::vertex_iterator v1i(v0i);
			for (++v1i; v1i != ve; ++v1i) {
				if ((!get_departure_ifr() && *v1i == m_vertexdep) ||
				    (!get_destination_ifr() && *v1i == m_vertexdest))
					continue;
				const ADR::GraphVertex& vv1(g[*v1i]);
				{
					Rect r(vv0.get_coord(), vv0.get_coord());
					r = r.add(vv1.get_coord());
					if (!r.is_intersect(bbox)) {
						if (false && (vv0.get_ident() == "UTOPO" || vv1.get_ident() == "UTOPO")) {
							std::ostringstream oss;
							oss << "lgraphtfrruleresult: " << ts.get_ident() << '/' << ts.get_type()
							    << ' ' << vv0.get_ident() << " <-> " << vv1.get_ident()
							    << " does not intersect airspace?""? " << bbox << " / " << r;
							m_signal_log(log_debug0, oss.str());
						}
						continue;
					}
				}
				IntervalSet<int32_t> altrange;
				bool altrangenotcalc(true);
				ADR::Graph::out_edge_iterator e0, e1;
				for (tie(e0, e1) = out_edges(*v0i, g); e0 != e1; ++e0) {
					if (target(*e0, g) != *v1i)
						continue;
					if (altrangenotcalc) {
						altrange = ts.get_point_intersect_altitudes(ADR::TimeTableEval(get_deptime(), vv0.get_coord(), m_eval.get_specialdateeval()),
											    vv1.get_coord(), seq.get_alt(), vv0.get_uuid(), vv1.get_uuid());
						altrangenotcalc = false;
						if (false && (vv0.get_ident() == "ULKIG" || vv1.get_ident() == "ULKIG")) {
							std::ostringstream oss;
							oss << "lgraphtfrruleresult: " << ts.get_ident() << '/' << ts.get_type() << ' '
							    << vv0.get_ident() << " <-> " << vv1.get_ident() << ": " << altrange.to_str();
							m_signal_log(log_debug0, oss.str());
						}
					}
					ADR::GraphEdge& ee(g[*e0]);
					for (unsigned int pi = pi0; pi <= pi1; ++pi) {
						const Performance::Cruise& cruise(m_performance.get_cruise(pi));
						if (!altrange.is_inside(cruise.get_altitude()))
							break;
						if (!ee.is_valid(pi))
							continue;
						++edgecnt;
						ee.clear_metric(pi);
						if (false)
							lgraphlogkilledge(g, *e0, pi);
					}
				}
				for (tie(e0, e1) = out_edges(*v1i, g); e0 != e1; ++e0) {
					if (target(*e0, g) != *v0i)
						continue;
					if (altrangenotcalc) {
						altrange = ts.get_point_intersect_altitudes(ADR::TimeTableEval(get_deptime(), vv0.get_coord(), m_eval.get_specialdateeval()),
											    vv1.get_coord(), seq.get_alt(), vv0.get_uuid(), vv1.get_uuid());
						altrangenotcalc = false;
						if (false && (vv0.get_ident() == "ULKIG" || vv1.get_ident() == "ULKIG")) {
							std::ostringstream oss;
							oss << "lgraphtfrruleresult: " << ts.get_ident() << '/' << ts.get_type() << ' '
							    << vv0.get_ident() << " <-> " << vv1.get_ident() << ": " << altrange.to_str();
							m_signal_log(log_debug0, oss.str());
						}
					}
					ADR::GraphEdge& ee(g[*e0]);
					for (unsigned int pi = pi0; pi <= pi1; ++pi) {
						const Performance::Cruise& cruise(m_performance.get_cruise(pi));
						if (!altrange.is_inside(cruise.get_altitude()))
							break;
						if (!ee.is_valid(pi))
							continue;
						++edgecnt;
						ee.clear_metric(pi);
						if (false)
							lgraphlogkilledge(g, *e0, pi);
					}
				}
			}
		}
		if (log) {
			Glib::TimeVal tv1;
			tv1.assign_current_time();
			tv1 -= tv;
			std::ostringstream oss;
			oss << "lgraphtfrruleresult: excluding airspace " << ts.get_ident() << '/' << ts.get_type() << ": "
			    << edgecnt << " edges removed, " << std::fixed << std::setprecision(3) << tv1.as_double() << 's';
			m_signal_log(log_debug0, oss.str());
		}
		return !!edgecnt;
	}

	case ADR::RuleSegment::type_point:
	{
		const unsigned int pis(m_performance.size());
		if (!pis)
			return false;
		unsigned int edgecnt(0);
		ADR::Graph::vertex_iterator vi, ve;
		for (boost::tie(vi, ve) = boost::vertices(g); vi != ve; ++vi) {
			const ADR::GraphVertex& vv(g[*vi]);
			if (vv.get_uuid() != seq.get_uuid0())
				continue;
			{
				ADR::Graph::out_edge_iterator e0, e1;
				for (boost::tie(e0, e1) = boost::out_edges(*vi, g); e0 != e1; ++e0) {
					const ADR::GraphEdge& ee(g[*e0]);
					for (unsigned int pi = 0; pi < pis; ++pi) {
						if (!ee.is_valid(pi))
							continue;
						if (!seq.get_alt().is_inside(m_performance.get_cruise(pi).get_altitude()))
							continue;
						lgraphkilledge(g, *e0, pi, log);
						++edgecnt;
					}
				}
			}
			{
				ADR::Graph::in_edge_iterator e0, e1;
				for (boost::tie(e0, e1) = boost::in_edges(*vi, g); e0 != e1; ++e0) {
					ADR::GraphEdge& ee(g[*e0]);
					for (unsigned int pi = 0; pi < pis; ++pi) {
						if (!ee.is_valid(pi))
							continue;
						if (!seq.get_alt().is_inside(m_performance.get_cruise(pi).get_altitude()))
							continue;
						lgraphkilledge(g, *e0, pi, log);
						++edgecnt;
					}
				}
			}
		}
		return !!edgecnt;
	}

	case ADR::RuleSegment::type_airway:
	case ADR::RuleSegment::type_dct:
	{
		const unsigned int pis(m_performance.size());
		if (!pis)
			return false;
		unsigned int edgecnt(0);
		ADR::Graph::vertex_iterator v0i, ve;
		for (boost::tie(v0i, ve) = boost::vertices(g); v0i != ve; ++v0i) {
			const ADR::GraphVertex& vv0(g[*v0i]);
			if (vv0.get_uuid() != seq.get_uuid0())
				continue;
			ADR::Graph::out_edge_iterator e0, e1;
			for (boost::tie(e0, e1) = boost::out_edges(*v0i, g); e0 != e1; ++e0) {
				const ADR::GraphEdge& ee(g[*e0]);
				if (!ee.is_match(seq.get_airway()))
					continue;
				ADR::Graph::vertex_descriptor v1(boost::target(*e0, g));
				const ADR::GraphVertex& vv1(g[v1]);
				if (vv1.get_uuid() != seq.get_uuid1())
					continue;
				for (unsigned int pi = 0; pi < pis; ++pi) {
					if (!ee.is_valid(pi))
						continue;
					if (!seq.get_alt().is_inside(m_performance.get_cruise(pi).get_altitude()))
						continue;
					lgraphkilledge(g, *e0, pi, log);
					++edgecnt;
				}
			}
		}
		return !!edgecnt;
	}

	case ADR::RuleSegment::type_sid:
	{
		const unsigned int pis(m_performance.size());
		if (!pis)
			return false;
		unsigned int edgecnt(0);
		ADR::Graph::out_edge_iterator e0, e1;
		for (boost::tie(e0, e1) = boost::out_edges(m_vertexdep, g); e0 != e1; ++e0) {
			const ADR::GraphEdge& ee(g[*e0]);
			if (!ee.is_match(seq.get_airway()))
				continue;
			for (unsigned int pi = 0; pi < pis; ++pi) {
				if (!ee.is_valid(pi))
					continue;
				if (!seq.get_alt().is_inside(m_performance.get_cruise(pi).get_altitude()))
					continue;
				lgraphkilledge(g, *e0, pi, log);
				++edgecnt;
			}
		}
		return !!edgecnt;
	}

	case ADR::RuleSegment::type_star:
	{
		const unsigned int pis(m_performance.size());
		if (!pis)
			return false;
		unsigned int edgecnt(0);
		ADR::Graph::in_edge_iterator e0, e1;
		for (boost::tie(e0, e1) = boost::in_edges(m_vertexdest, g); e0 != e1; ++e0) {
			const ADR::GraphEdge& ee(g[*e0]);
			if (!ee.is_match(seq.get_airway()))
				continue;
			for (unsigned int pi = 0; pi < pis; ++pi) {
				if (!ee.is_valid(pi))
					continue;
				if (!seq.get_alt().is_inside(m_performance.get_cruise(pi).get_altitude()))
					continue;
				lgraphkilledge(g, *e0, pi, log);
				++edgecnt;
			}
		}
		return !!edgecnt;
	}

	default:
		return false;
	}
}

IntervalSet<int32_t> CFMUAutoroute51::lgraphcrossinggate(const ADR::RuleSegment& seq, const ADR::RuleSegment& cc)
{
	switch (seq.get_type()) {
	case ADR::RuleSegment::type_airway:
	case ADR::RuleSegment::type_dct:
	case ADR::RuleSegment::type_point:
		switch (cc.get_type()) {
		case ADR::RuleSegment::type_airspace:
		{
			const ADR::AirspaceTimeSlice& ts(cc.get_wpt0()->operator()(get_deptime()).as_airspace());
			if (!ts.is_valid()) {
				std::ostringstream oss;
				oss << "lgraphtfrruleresult: airspace not valid for UUID " << seq.get_uuid0();
				m_signal_log(log_debug0, oss.str());
				return IntervalSet<int32_t>();
			}
			const ADR::PointIdentTimeSlice& ts0(seq.get_wpt0()->operator()(get_deptime()).as_point());
			if (!ts0.is_valid())
				return IntervalSet<int32_t>();
			IntervalSet<int32_t> altrange;
			if (seq.get_type() == ADR::RuleSegment::type_point) {
				altrange = ts.get_point_altitudes(ADR::TimeTableEval(get_deptime(), ts0.get_coord(), m_eval.get_specialdateeval()),
								  cc.get_alt(), seq.get_uuid0());
			} else {
				const ADR::PointIdentTimeSlice& ts1(seq.get_wpt1()->operator()(get_deptime()).as_point());
				if (!ts1.is_valid())
					return IntervalSet<int32_t>();
				altrange = ts.get_point_intersect_altitudes(ADR::TimeTableEval(get_deptime(), ts0.get_coord(), m_eval.get_specialdateeval()),
									    ts1.get_coord(), cc.get_alt(), seq.get_uuid0(), seq.get_uuid1());
			}
			altrange &= seq.get_alt().get_interval();
			return altrange;
		}

		case ADR::RuleSegment::type_airway:
		case ADR::RuleSegment::type_dct:
		case ADR::RuleSegment::type_point:
		{
			{
				bool peq(seq.get_uuid0() == cc.get_uuid0());
				if (!peq && cc.get_type() == ADR::RuleSegment::type_point &&
				    (seq.get_type() == ADR::RuleSegment::type_airway ||
				     seq.get_type() == ADR::RuleSegment::type_dct))
					peq = seq.get_uuid1() == cc.get_uuid0();
				if (!peq)
					return IntervalSet<int32_t>();
			}
			IntervalSet<int32_t> altrange(seq.get_alt().get_interval());
			altrange &= cc.get_alt().get_interval();
			if (cc.get_type() == ADR::RuleSegment::type_point)
				return altrange;
			if (seq.get_type() == ADR::RuleSegment::type_point ||
			    seq.get_type() != cc.get_type())
				return IntervalSet<int32_t>();
			if (seq.get_uuid1() != cc.get_uuid1())
				return IntervalSet<int32_t>();
			if (seq.get_type() == ADR::RuleSegment::type_dct)
				return altrange;
			if (seq.get_airway_uuid() != cc.get_airway_uuid())
				return IntervalSet<int32_t>();
			return altrange;
		}

		default:
			return IntervalSet<int32_t>();
		}

	default:
		return IntervalSet<int32_t>();
	}
}

bool CFMUAutoroute51::lgraphtfrruleresult(ADR::Graph& g, const ADR::RestrictionResult& rule, const ADR::FlightPlan& fpl)
{
	const ADR::FlightRestrictionTimeSlice& tsrule(rule.get_timeslice());
	if (!tsrule.is_valid() || !tsrule.is_enabled())
		return false;
	// actually modify the graph
	typedef ADR::RestrictionResult::set_t set_t;
	switch (tsrule.get_type()) {
	case ADR::FlightRestrictionTimeSlice::type_forbidden:
	{
		std::vector<ADR::RuleSegment> cc(tsrule.get_crossingcond());
		bool work(false);
		for (ADR::RestrictionResult::const_iterator ri(rule.begin()), re(rule.end()); ri != re; ++ri) {
			const ADR::RestrictionSequenceResult& alt(*ri);
			if (alt.get_sequence().size() != 1)
				continue;
			const ADR::RuleSegment& seq(alt.get_sequence()[0]);
			{
				ADR::RuleSegment seq1(seq);
				if (tsrule.is_routestatic(seq1)) {
					work = lgraphtfrrulekillforbidden(g, seq1, &g == &m_graph) || work;
					continue;
				}
			}
			for (std::vector<ADR::RuleSegment>::const_iterator ci(cc.begin()), ce(cc.end()); ci != ce; ++ci) {
				IntervalSet<int32_t> altrange(lgraphcrossinggate(seq, *ci));
				if (false && tsrule.get_ident() == "ED2202A") {
					std::ostringstream oss;
					oss << "lgraphtfrruleresult: " << tsrule.get_ident() << " crossingcond "
					    << ci->to_shortstr(get_deptime()) << " restriction "
					    << seq.to_shortstr(get_deptime()) << " result " << altrange.to_str();
					m_signal_log(log_debug0, oss.str());
				}
				for (IntervalSet<int32_t>::const_iterator ii(altrange.begin()), ie(altrange.end()); ii != ie; ++ii) {
					ADR::AltRange ar(ii->get_lower(), ADR::AltRange::alt_std,
							 ii->get_upper() - 1, ADR::AltRange::alt_std);
					ADR::RuleSegment seq1(seq.get_type(), ar, seq.get_wpt0(), seq.get_wpt1(), seq.get_airway());
					work = lgraphtfrrulekillforbidden(g, seq1, &g == &m_graph) || work;
				}
			}
		}
		if (work) {
			if (&g == &m_graph)
				lgraphmodified();
			return true;
		}
		for (ADR::RestrictionResult::const_iterator ri(rule.begin()), re(rule.end()); ri != re; ++ri) {
			const ADR::RestrictionSequenceResult& alt(*ri);
			if (alt.get_sequence().size() == 1) {
				ADR::RuleSegment seq1(alt.get_sequence()[0]);
				if (tsrule.is_routestatic(seq1) && lgraphtfrrulekillforbidden(g, seq1, &g == &m_graph)) {
					work = true;
					continue;
				}
			}
			if (tsrule.is_routestatic()) {
				for (set_t::const_iterator it(alt.get_edgeset().begin()), e(alt.get_edgeset().end()); it != e; ++it) {
					unsigned int i(*it);
					const ADR::FPLWaypoint& wpt0(fpl[i]);
					const ADR::FPLWaypoint& wpt1(fpl[i + 1U]);
					work = lgraphdisconnectsolutionedge(g, wpt0.get_ptobj(), wpt1.get_ptobj()) || work;
				}
				if (!alt.get_edgeset().empty())
					continue;
			}
			if (alt.get_edgeset().empty()) {
				unsigned int i;
				{
					set_t::const_iterator it(alt.get_vertexset().begin()), e(alt.get_vertexset().end());
					if (it == e)
						continue;
					i = *it;
					++it;
					if (it != e)
						continue;
				}
				if (i >= fpl.size())
					continue;
				if (!rule.get_edgeset().empty())
					continue;
				{
					set_t vset, vset1;
					vset1.insert(i);
					std::set_difference(alt.get_vertexset().begin(), alt.get_vertexset().end(), vset1.begin(), vset1.end(),
							    std::insert_iterator<set_t>(vset, vset.begin()));
					if (!vset.empty())
						continue;
					vset.clear();
					vset1.insert(0U);
					vset1.insert(fpl.size() - 1U);
					std::set_difference(rule.get_vertexset().begin(), rule.get_vertexset().end(), vset1.begin(), vset1.end(),
							    std::insert_iterator<set_t>(vset, vset.begin()));
					if (!vset.empty())
						continue;
				}
				const ADR::FPLWaypoint& wpt(fpl[i]);
				work = lgraphdisconnectsolutionvertex(g, wpt.get_ptobj()) || work;
				continue;
			}
			unsigned int i;
			{
				set_t::const_iterator it(alt.get_edgeset().begin()), e(alt.get_edgeset().end());
				if (it == e)
					continue;
				i = *it;
				++it;
				if (it != e)
					continue;
			}
			if (i + 1U >= fpl.size())
				continue;
			{
				set_t vset, vset1;
				vset1.insert(i);
				vset1.insert(i + 1U);
				std::set_difference(alt.get_vertexset().begin(), alt.get_vertexset().end(), vset1.begin(), vset1.end(),
						    std::insert_iterator<set_t>(vset, vset.begin()));
				if (!vset.empty())
					continue;
				vset.clear();
				vset1.insert(0U);
				vset1.insert(fpl.size() - 1U);
				std::set_difference(rule.get_vertexset().begin(), rule.get_vertexset().end(), vset1.begin(), vset1.end(),
						    std::insert_iterator<set_t>(vset, vset.begin()));
				if (!vset.empty())
					continue;
				vset.clear();
				std::set_difference(rule.get_edgeset().begin(), rule.get_edgeset().end(),
						    alt.get_edgeset().begin(), alt.get_edgeset().end(),
						    std::insert_iterator<set_t>(vset, vset.begin()));
				if (!vset.empty())
					continue;
			}
			const ADR::FPLWaypoint& wpt0(fpl[i]);
			const ADR::FPLWaypoint& wpt1(fpl[i + 1U]);
			work = lgraphdisconnectsolutionedge(g, wpt0.get_ptobj(), wpt1.get_ptobj()) || work;
		}
		if (work) {
			if (&g == &m_graph)
				lgraphmodified();
			return true;
		}
		break;
	}

	case ADR::FlightRestrictionTimeSlice::type_closed:
	{
		{
			bool work(false);
			std::vector<ADR::RuleSegment> cc(tsrule.get_crossingcond());
			for (std::vector<ADR::RuleSegment>::const_iterator ci(cc.begin()), ce(cc.end()); ci != ce; ++ci)
				work = lgraphtfrrulekillforbidden(g, *ci, &g == &m_graph) || work;
			if (work) {
				lgraphmodified();
				return true;
			}
		}
		if (rule.get_edgeset().empty()) {
			unsigned int i;
			{
				set_t vset, vset1;
				vset1.insert(0U);
				vset1.insert(fpl.size() - 1U);
				std::set_difference(rule.get_vertexset().begin(), rule.get_vertexset().end(), vset1.begin(), vset1.end(),
						    std::insert_iterator<set_t>(vset, vset.begin()));
				set_t::const_iterator it(vset.begin()), e(vset.end());
				if (it == e)
					break;
				i = *it;
				++it;
				if (it != e)
					break;
			}
			if (i >= fpl.size())
				break;
			const ADR::FPLWaypoint& wpt(fpl[i]);
			if (lgraphdisconnectsolutionvertex(g, wpt.get_ptobj())) {
				lgraphmodified();
				return true;
			}
			break;
		}
		unsigned int i;
		{
			set_t::const_iterator it(rule.get_edgeset().begin()), e(rule.get_edgeset().end());
			if (it == e)
				break;
			i = *it;
			++it;
			if (it != e)
				break;
		}
		if (i + 1U >= fpl.size())
			break;
		{
			set_t vset, vset1;
			vset1.insert(i);
			vset1.insert(i + 1U);
			vset1.insert(0U);
			vset1.insert(fpl.size() - 1U);
			std::set_difference(rule.get_vertexset().begin(), rule.get_vertexset().end(), vset1.begin(), vset1.end(),
					    std::insert_iterator<set_t>(vset, vset.begin()));
			if (!vset.empty())
				break;
		}
		const ADR::FPLWaypoint& wpt0(fpl[i]);
		const ADR::FPLWaypoint& wpt1(fpl[i + 1U]);
		if (lgraphdisconnectsolutionedge(g, wpt0.get_ptobj(), wpt1.get_ptobj())) {
			if (&g == &m_graph)
				lgraphmodified();
			return true;
		}
		break;
	}

	default:
	case ADR::FlightRestrictionTimeSlice::type_mandatory:
	case ADR::FlightRestrictionTimeSlice::type_allowed:
		if (tsrule.is_mandatoryinbound()) {
			const unsigned int pis(m_performance.size());
			bool work(false);
			std::set<ADR::UUID> pts;
			for (ADR::RestrictionResult::const_iterator ri(rule.begin()), re(rule.end()); ri != re; ++ri) {
				const ADR::RestrictionSequenceResult& alt(*ri);
				if (!alt.get_sequence().size())
					continue;
				const ADR::RuleSegment& seq(alt.get_sequence().back());
				ADR::UUID ptuuid;
				switch (seq.get_type()) {
				case ADR::RuleSegment::type_airway:
				case ADR::RuleSegment::type_dct:
					ptuuid = seq.get_uuid1();
					break;

				case ADR::RuleSegment::type_star:
				{
					const ADR::StandardInstrumentTimeSlice& ts(seq.get_airway()->operator()(get_deptime()).as_sidstar());
					if (!ts.is_valid())
						continue;
					ptuuid = ts.get_airport();
					break;
				}

				default:
					continue;
				}
				if (ptuuid.is_nil())
					continue;
				if (pts.find(ptuuid) != pts.end())
					continue;
				pts.insert(ptuuid);
				ADR::Graph::vertex_descriptor v;
				{
					bool ok;
					boost::tie(v, ok) = g.find_vertex(ptuuid);
					if (!ok)
						continue;
				}
				ADR::Graph::in_edge_iterator e0, e1;
				for (boost::tie(e0, e1) = boost::in_edges(v, g); e0 != e1; ++e0) {
					ADR::GraphEdge& ee(g[*e0]);
					ADR::Graph::vertex_descriptor u(boost::source(*e0, g));
					const ADR::GraphVertex& uu(g[u]);
					for (unsigned int pi = 0; pi < pis; ++pi) {
						if (!ee.is_valid(pi))
							continue;
						bool ok(false);
						for (ADR::RestrictionResult::const_iterator ri(rule.begin()), re(rule.end()); ri != re; ++ri) {
							const ADR::RestrictionSequenceResult& alt(*ri);
							if (!alt.get_sequence().size())
								continue;
							const ADR::RuleSegment& seq(alt.get_sequence().back());
							switch (seq.get_type()) {
							case ADR::RuleSegment::type_dct:
								if (!ee.is_dct())
									continue;
								if (seq.get_uuid1() != ptuuid)
									continue;
								if (seq.get_uuid0() != uu.get_uuid())
									continue;
								break;

							case ADR::RuleSegment::type_airway:
								if (!ee.is_match(seq.get_airway()))
									continue;
								if (seq.get_uuid1() != ptuuid)
									continue;
								if (seq.get_uuid0() != uu.get_uuid())
									continue;
								break;

							case ADR::RuleSegment::type_star:
							{
								if (!ee.is_match(seq.get_airway()))
									continue;
								const ADR::StandardInstrumentTimeSlice& ts(seq.get_airway()->operator()(get_deptime()).as_sidstar());
								if (!ts.is_valid())
									continue;
								if (ts.get_airport() != ptuuid)
									continue;
								break;
							}

							default:
								continue;
							}
							const Performance::Cruise& cruise(m_performance.get_cruise(pi));
							if (!seq.get_alt().is_inside(cruise.get_altitude()))
								continue;
							ok = true;
							break;
						}
						if (ok)
							continue;
						lgraphkilledge(g, *e0, pi, &g == &m_graph);
						work = true;
					}
				}
			}
		}
		if (tsrule.is_mandatoryoutbound()) {
			const unsigned int pis(m_performance.size());
			bool work(false);
			std::set<ADR::UUID> pts;
			for (ADR::RestrictionResult::const_iterator ri(rule.begin()), re(rule.end()); ri != re; ++ri) {
				const ADR::RestrictionSequenceResult& alt(*ri);
				if (!alt.get_sequence().size())
					continue;
				const ADR::RuleSegment& seq(alt.get_sequence().back());
				ADR::UUID ptuuid;
				switch (seq.get_type()) {
				case ADR::RuleSegment::type_airway:
				case ADR::RuleSegment::type_dct:
					ptuuid = seq.get_uuid0();
					break;

				case ADR::RuleSegment::type_sid:
				{
					const ADR::StandardInstrumentTimeSlice& ts(seq.get_airway()->operator()(get_deptime()).as_sidstar());
					if (!ts.is_valid())
						continue;
					ptuuid = ts.get_airport();
					break;
				}

				default:
					continue;
				}
				if (ptuuid.is_nil())
					continue;
				if (pts.find(ptuuid) != pts.end())
					continue;
				pts.insert(ptuuid);
				ADR::Graph::vertex_descriptor v;
				{
					bool ok;
					boost::tie(v, ok) = g.find_vertex(ptuuid);
					if (!ok)
						continue;
				}
				ADR::Graph::out_edge_iterator e0, e1;
				for (boost::tie(e0, e1) = boost::out_edges(v, g); e0 != e1; ++e0) {
					ADR::GraphEdge& ee(g[*e0]);
					ADR::Graph::vertex_descriptor u(boost::target(*e0, g));
					const ADR::GraphVertex& uu(g[u]);
					for (unsigned int pi = 0; pi < pis; ++pi) {
						if (!ee.is_valid(pi))
							continue;
						bool ok(false);
						for (ADR::RestrictionResult::const_iterator ri(rule.begin()), re(rule.end()); ri != re; ++ri) {
							const ADR::RestrictionSequenceResult& alt(*ri);
							if (!alt.get_sequence().size())
								continue;
							const ADR::RuleSegment& seq(alt.get_sequence().back());
							switch (seq.get_type()) {
							case ADR::RuleSegment::type_dct:
								if (!ee.is_dct())
									continue;
								if (seq.get_uuid0() != ptuuid)
									continue;
								if (seq.get_uuid1() != uu.get_uuid())
									continue;
								break;

							case ADR::RuleSegment::type_airway:
								if (!ee.is_match(seq.get_airway()))
									continue;
								if (seq.get_uuid0() != ptuuid)
									continue;
								if (seq.get_uuid1() != uu.get_uuid())
									continue;
								break;

							case ADR::RuleSegment::type_sid:
							{
								if (!ee.is_match(seq.get_airway()))
									continue;
								const ADR::StandardInstrumentTimeSlice& ts(seq.get_airway()->operator()(get_deptime()).as_sidstar());
								if (!ts.is_valid())
									continue;
								if (ts.get_airport() != ptuuid)
									continue;
								break;
							}

							default:
								continue;
							}
							const Performance::Cruise& cruise(m_performance.get_cruise(pi));
							if (!seq.get_alt().is_inside(cruise.get_altitude()))
								continue;
							ok = true;
							break;
						}
						if (ok)
							continue;
						lgraphkilledge(g, *e0, pi, &g == &m_graph);
						work = true;
					}
				}
			}
			if (work) {
				if (&g == &m_graph)
					lgraphmodified();
				return true;
			}
		}
		if (tsrule.is_dct() || tsrule.is_deparr_dct()) {
			bool work(false);
			for (set_t::const_iterator it(rule.get_edgeset().begin()), e(rule.get_edgeset().end()); it != e; ++it) {
				unsigned int i(*it);
				if (i + 1U >= fpl.size())
					continue;
				const ADR::FPLWaypoint& wpt0(fpl[i]);
				const ADR::FPLWaypoint& wpt1(fpl[i + 1U]);
				work = lgraphdisconnectsolutionedge(g, wpt0.get_ptobj(), wpt1.get_ptobj()) || work;
			}
			if (work) {
				if (&g == &m_graph)
					lgraphmodified();
				return true;
			}
		}
		if (false)
			tsrule.print(std::cerr << "Mandatory Rule: " << tsrule.get_ident() << std::endl);
		break;
	}
	double scale(localforbiddenpenalty);
	bool work(false);
	switch (tsrule.get_type()) {
	case ADR::FlightRestrictionTimeSlice::type_mandatory:
	case ADR::FlightRestrictionTimeSlice::type_allowed:
		scale = 1.0 / localforbiddenpenalty;
		// fall through

	case ADR::FlightRestrictionTimeSlice::type_forbidden:
		for (ADR::RestrictionResult::const_iterator ri(rule.begin()), re(rule.end()); ri != re; ++ri) {
			const ADR::RestrictionSequenceResult& alt(*ri);
			if (alt.get_edgeset().empty()) {
				for (set_t::const_iterator i(alt.get_vertexset().begin()), e(alt.get_vertexset().end()); i != e; ++i) {
					if (*i >= fpl.size())
						continue;
					const ADR::FPLWaypoint& wpt(fpl[*i]);
					work = lgraphscalesolutionvertex(g, wpt.get_ptobj(), scale) || work;
				}
				continue;
			}
			for (set_t::const_iterator i(alt.get_edgeset().begin()), e(alt.get_edgeset().end()); i != e; ++i) {
				if (*i >= fpl.size() + 1U)
					continue;
				const ADR::FPLWaypoint& wpt0(fpl[*i]);
				const ADR::FPLWaypoint& wpt1(fpl[*i + 1U]);
				work = lgraphscalesolutionedge(g, wpt0.get_ptobj(), wpt1.get_ptobj(), scale) || work;
			}
		}
		break;

	case ADR::FlightRestrictionTimeSlice::type_closed:
		if (rule.get_edgeset().empty()) {
			for (set_t::const_iterator i(rule.get_vertexset().begin()), e(rule.get_vertexset().end()); i != e; ++i) {
				if (*i >= fpl.size())
					continue;
				const ADR::FPLWaypoint& wpt(fpl[*i]);
				work = lgraphscalesolutionvertex(g, wpt.get_ptobj(), scale) || work;
			}
			break;
		}
		for (set_t::const_iterator i(rule.get_edgeset().begin()), e(rule.get_edgeset().end()); i != e; ++i) {
			if (*i >= fpl.size() + 1U)
				continue;
			const ADR::FPLWaypoint& wpt0(fpl[*i]);
			const ADR::FPLWaypoint& wpt1(fpl[*i + 1U]);
			work = lgraphscalesolutionedge(g, wpt0.get_ptobj(), wpt1.get_ptobj(), scale) || work;
		}
		break;

	default:
		break;
	}
	if (work && &g == &m_graph)
		lgraphmodified();
	return work;
}

void CFMUAutoroute51::lgraphtfrmessage(const ADR::Message& msg, const ADR::FlightPlan& fpl)
{
	if (msg.get_type() != ADR::Message::type_error)
		return;
	// actually modify the graph
	typedef ADR::RestrictionResult::set_t set_t;
	if (msg.get_edgeset().empty()) {
		unsigned int i;
		{
			set_t vset, vset1;
			vset1.insert(0U);
			vset1.insert(fpl.size() - 1U);
			std::set_difference(msg.get_vertexset().begin(), msg.get_vertexset().end(), vset1.begin(), vset1.end(),
					    std::insert_iterator<set_t>(vset, vset.begin()));
			set_t::const_iterator it(vset.begin()), e(vset.end());
			if (it == e)
				return;
			i = *it;
			++it;
			if (it != e)
				return;
		}
		if (i >= fpl.size())
			return;
		const ADR::FPLWaypoint& wpt(fpl[i]);
		if (lgraphdisconnectsolutionvertex(wpt.get_ptobj()))
			return;
		return;
	}
	unsigned int i;
	{
		set_t::const_iterator it(msg.get_edgeset().begin()), e(msg.get_edgeset().end());
		if (it == e)
			return;
		i = *it;
		++it;
		if (it != e)
			return;
	}
	if (i + 1U >= fpl.size())
		return;
	{
		set_t vset, vset1;
		vset1.insert(i);
		vset1.insert(i + 1U);
		vset1.insert(0U);
		vset1.insert(fpl.size() - 1U);
		std::set_difference(msg.get_vertexset().begin(), msg.get_vertexset().end(), vset1.begin(), vset1.end(),
				    std::insert_iterator<set_t>(vset, vset.begin()));
		if (!vset.empty())
			return;
	}
	const ADR::FPLWaypoint& wpt0(fpl[i]);
	const ADR::FPLWaypoint& wpt1(fpl[i + 1U]);
	if (lgraphdisconnectsolutionedge(wpt0.get_ptobj(), wpt1.get_ptobj()))
		return;
}

std::string CFMUAutoroute51::lgraphvertexname(const ADR::Graph& g, ADR::Graph::vertex_descriptor v)
{
	const ADR::GraphVertex& vv(g[v]);
	std::ostringstream oss;
	oss << vv.get_ident();
	return oss.str();
}

std::string CFMUAutoroute51::lgraphvertexname(const ADR::Graph& g, ADR::Graph::vertex_descriptor v, unsigned int pi)
{
	const ADR::GraphVertex& vv(g[v]);
	std::ostringstream oss;
	oss << vv.get_ident();
	if (pi >= m_performance.size() || v == m_vertexdep || v == m_vertexdest)
		return oss.str();
	const Performance::Cruise& cruise(m_performance.get_cruise(pi));
	oss << ':' << cruise.get_fplalt();
	return oss.str();
}

std::string CFMUAutoroute51::lgraphvertexname(const ADR::Graph& g, ADR::Graph::vertex_descriptor v, unsigned int pi0, unsigned int pi1)
{
	const ADR::GraphVertex& vv(g[v]);
	std::ostringstream oss;
	oss << vv.get_ident();
	if ((pi0 >= m_performance.size() && pi1 >= m_performance.size()) || v == m_vertexdep || v == m_vertexdest)
		return oss.str();
	oss << ':';
	if (pi0 < m_performance.size()) {
		const Performance::Cruise& cruise(m_performance.get_cruise(pi0));
		oss << cruise.get_fplalt();
	}
	oss << "...";
	if (pi1 < m_performance.size()) {
		const Performance::Cruise& cruise(m_performance.get_cruise(pi1));
		oss << cruise.get_fplalt();
	}
	return oss.str();
}

const std::string& CFMUAutoroute51::lgraphawyname(const ADR::Object::const_ptr_t& awy, ADR::timetype_t tm)
{
	static const std::string dct("DCT");
	static const std::string matchall("[ALL]");
	static const std::string unknown("?""?");
	if (!awy)
		return dct;
	if (awy->get_uuid() == ADR::GraphEdge::matchall)
		return matchall;
	const ADR::TimeSlice& ts(awy->operator()(tm));
	if (!ts.is_valid())
		return unknown;
	{
		const ADR::IdentTimeSlice& tsi(ts.as_ident());
		if (tsi.is_valid())
			return tsi.get_ident();
	}
	{
		const ADR::SegmentTimeSlice& tss(ts.as_segment());
		if (tss.is_valid()) {
			const ADR::Link& rte(tss.get_route());
			const ADR::IdentTimeSlice& tsi(rte.get_obj()->operator()(tm).as_ident());
			if (tsi.is_valid())
				return tsi.get_ident();
		}
	}
	return unknown;
}

const std::string& CFMUAutoroute51::lgraphawyname(const ADR::Object::const_ptr_t& awy) const
{
	return lgraphawyname(awy, get_deptime());
}

void CFMUAutoroute51::lgraphlogkilledge(const ADR::Graph& g, ADR::Graph::edge_descriptor e, unsigned int pi)
{
	const ADR::GraphEdge& ee(g[e]);
	std::ostringstream oss;
	oss << "- " << lgraphvertexname(source(e, g), pi)
	    << ' ' << ee.get_route_ident_or_dct(true) << ' '
	    << lgraphvertexname(target(e, g), pi);
	m_signal_log(log_graphchange, oss.str());
}

void CFMUAutoroute51::lgraphlogkillalledges(const ADR::Graph& g, ADR::Graph::edge_descriptor e)
{
        const unsigned int pis(m_performance.size());
	const ADR::GraphEdge& ee(g[e]);
	for (unsigned int pi = 0; pi < pis; ++pi) {
		if (!ee.is_valid(pi))
			continue;
		lgraphlogkilledge(e, pi);
	}
}

void CFMUAutoroute51::lgraphlogaddedge(const ADR::Graph& g, ADR::Graph::edge_descriptor e, unsigned int pi)
{
	const ADR::GraphEdge& ee(g[e]);
	std::ostringstream oss;
	oss << "+ " << lgraphvertexname(source(e, g), pi)
	    << ' ' << ee.get_route_ident_or_dct(true) << ' '
	    << lgraphvertexname(target(e, g), pi);
	m_signal_log(log_graphchange, oss.str());
}

void CFMUAutoroute51::lgraphlogscaleedge(const ADR::Graph& g, ADR::Graph::edge_descriptor e, unsigned int pi, double scale)
{
	const ADR::GraphEdge& ee(g[e]);
	std::ostringstream oss;
	oss << "S " << lgraphvertexname(source(e, g), pi)
	    << ' ' << ee.get_route_ident_or_dct(true) << ' '
	    << lgraphvertexname(target(e, g), pi) << " : "
	    << ee.get_metric(pi) << " * " << scale << " -> " << (ee.get_metric(pi) * scale);
	m_signal_log(log_graphchange, oss.str());
}

void CFMUAutoroute51::lgraphlogrenameedge(const ADR::Graph& g, ADR::Graph::edge_descriptor e, unsigned int pi, const ADR::UUID& awyname)
{
	const ADR::GraphEdge& ee(g[e]);
	std::ostringstream oss;
	oss << "R " << lgraphvertexname(source(e, g), pi)
	    << ' ' << ee.get_route_ident_or_dct(true) << ' '
	    << lgraphvertexname(target(e, g), pi) << " -> " << awyname;
	m_signal_log(log_graphchange, oss.str());
}

bool CFMUAutoroute51::lgraphdisconnectvertex(const std::string& ident, int lvlfrom, int lvlto, bool intel)
{
	bool work(false);
	std::string identu(Glib::ustring(ident).uppercase());
	lvlfrom *= 100;
	lvlto *= 100;
	bool allalt(lvlfrom == 0 && lvlto >= 60000);
        const unsigned int pis(m_performance.size());
	for (solutionvertices_t::const_iterator svi(m_solutionvertices.begin()), sve(m_solutionvertices.end()); svi != sve; ++svi) {
		LVertexLevel v(*svi);
		const ADR::GraphVertex& vv(m_graph[v.get_vertex()]);
		if (vv.get_ident() != identu)
			continue;
		// prevent departure or destination from being disconnected if an intersection has the same ident
		if (allalt && (v.get_vertex() == m_vertexdep || v.get_vertex() == m_vertexdest))
			continue;
		const Performance::Cruise& cruise(m_performance.get_cruise(v.get_perfindex()));
		if (cruise.get_altitude() < lvlfrom || cruise.get_altitude() > lvlto)
			continue;
		{
			ADR::Graph::out_edge_iterator e0, e1;
			for (tie(e0, e1) = out_edges(v.get_vertex(), m_graph); e0 != e1; ++e0) {
				for (unsigned int pi = 0; pi < pis; ++pi) {
					const Performance::Cruise& cruise(m_performance.get_cruise(pi));
					if (cruise.get_altitude() < lvlfrom || cruise.get_altitude() > lvlto)
						continue;
					lgraphkilledge(*e0, pi, true);
					work = true;
				}
			}
		}
		{
			ADR::Graph::in_edge_iterator e0, e1;
			for (tie(e0, e1) = in_edges(v.get_vertex(), m_graph); e0 != e1; ++e0) {
				for (unsigned int pi = 0; pi < pis; ++pi) {
					const Performance::Cruise& cruise(m_performance.get_cruise(pi));
					if (cruise.get_altitude() < lvlfrom || cruise.get_altitude() > lvlto)
						continue;
					lgraphkilledge(*e0, pi, true);
					work = true;
				}
			}
		}
	}
	return work;
}

bool CFMUAutoroute51::lgraphdisconnectsolutionvertex(ADR::Graph& g, const ADR::Object::const_ptr_t& wpt)
{
	if (!wpt)
		return false;
	bool work(false);
	ADR::Graph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(g); e0 != e1; ++e0) {
		ADR::GraphEdge& ee(g[*e0]);
		if (!ee.is_solution())
			continue;
		const ADR::GraphVertex& uu(g[source(*e0, g)]);
		if (uu.get_uuid() != wpt->get_uuid()) {
			const ADR::GraphVertex& vv(g[target(*e0, g)]);
			if (vv.get_uuid() != wpt->get_uuid())
				continue;
		}
		lgraphkillsolutionedge(g, *e0, &g == &m_graph);
		work = true;
	}
	return work;
}

bool CFMUAutoroute51::lgraphdisconnectsolutionvertex(ADR::Graph& g, const std::string& ident)
{
	bool work(false);
	std::string identu(Glib::ustring(ident).uppercase());
	ADR::Graph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(g); e0 != e1; ++e0) {
		ADR::GraphEdge& ee(g[*e0]);
		if (!ee.is_solution())
			continue;
		const ADR::GraphVertex& uu(g[source(*e0, g)]);
		if (uu.get_ident() != identu) {
			const ADR::GraphVertex& vv(g[target(*e0, g)]);
			if (vv.get_ident() != identu)
				continue;
		}
		lgraphkillsolutionedge(g, *e0, &g == &m_graph);
		work = true;
	}
	return work;
}

bool CFMUAutoroute51::lgraphdisconnectsolutionedge(ADR::Graph& g, const ADR::Object::const_ptr_t& wpt0, const ADR::Object::const_ptr_t& wpt1, const ADR::UUID& awyname)
{
	if (!wpt0 || !wpt1)
		return false;
	bool work(false);
	const unsigned int pis(m_performance.size());
	for (;;) {
		bool done(true);
		ADR::Graph::edge_iterator e0, e1;
		for (tie(e0, e1) = edges(g); e0 != e1; ++e0) {
			ADR::GraphEdge& ee(g[*e0]);
			if (!ee.is_solution())
				continue;
			if (!ee.is_match(awyname))
				continue;
			if (false) {
				ADR::Graph::vertex_descriptor u(source(*e0, g));
				const ADR::GraphVertex& uu(g[u]);
				ADR::Graph::vertex_descriptor v(target(*e0, g));
				const ADR::GraphVertex& vv(g[v]);
				std::ostringstream oss;
				oss << "lgraphdisconnectsolutionedge: edge " << uu.get_ident() << " --> " << vv.get_ident();
				if (ee.is_awy(true))
					oss << " (AWY)";
				else if (ee.is_sid(true))
					oss << " (SID)";
				else if (ee.is_star(true))
					oss << " (STAR)";
				m_signal_log(log_debug0, oss.str());
			}
			ADR::Graph::vertex_descriptor u(source(*e0, g));
			const ADR::GraphVertex& uu(g[u]);
			if (uu.get_uuid() != wpt0->get_uuid())
				continue;
			ADR::Graph::vertex_descriptor v(target(*e0, g));
			const ADR::GraphVertex& vv(g[v]);
			if (vv.get_uuid() != wpt1->get_uuid())
				continue;
			if (ee.is_awy(true)) {
				ADR::GraphEdge edge(pis);
				edge.set_metric(ee.get_solution(), 1);
				lgraphaddedge(g, edge, u, v, setmetric_metric, true);
				work = true;
			}
			lgraphkillsolutionedge(g, *e0, &g == &m_graph);
			work = true;
		}
		if (done)
			break;
	}
	return work;
}

bool CFMUAutoroute51::lgraphdisconnectsolutionedge(ADR::Graph& g, const std::string& id0, const std::string& id1, const ADR::UUID& awyname)
{
	bool work(false);
	std::string id0u(Glib::ustring(id0).uppercase());
	std::string id1u(Glib::ustring(id1).uppercase());
	if (false) {
		std::ostringstream oss;
		oss << "lgraphdisconnectsolutionedge: " << id0u << " --> " << id1u;
		m_signal_log(log_debug0, oss.str());
	}
	const unsigned int pis(m_performance.size());
	for (;;) {
		bool done(true);
		ADR::Graph::edge_iterator e0, e1;
		for (tie(e0, e1) = edges(g); e0 != e1; ++e0) {
			ADR::GraphEdge& ee(g[*e0]);
			if (!ee.is_solution())
				continue;
			if (!ee.is_match(awyname))
				continue;
			if (false) {
				ADR::Graph::vertex_descriptor u(source(*e0, g));
				const ADR::GraphVertex& uu(g[u]);
				ADR::Graph::vertex_descriptor v(target(*e0, g));
				const ADR::GraphVertex& vv(g[v]);
				std::ostringstream oss;
				oss << "lgraphdisconnectsolutionedge: edge " << uu.get_ident() << " --> " << vv.get_ident();
				if (ee.is_awy(true))
					oss << " (AWY)";
				else if (ee.is_sid(true))
					oss << " (SID)";
				else if (ee.is_star(true))
					oss << " (STAR)";
				m_signal_log(log_debug0, oss.str());
			}
			ADR::Graph::vertex_descriptor u(source(*e0, g));
			const ADR::GraphVertex& uu(g[u]);
			if (uu.get_ident() != id0u)
				continue;
			ADR::Graph::vertex_descriptor v(target(*e0, g));
			const ADR::GraphVertex& vv(g[v]);
			if (vv.get_ident() != id1u)
				continue;
			if (ee.is_awy(true)) {
				ADR::GraphEdge edge(pis);
				edge.set_metric(ee.get_solution(), 1);
				lgraphaddedge(g, edge, u, v, setmetric_metric, true);
				work = true;
			}
			lgraphkillsolutionedge(g, *e0, &g == &m_graph);
			work = true;
		}
		if (done)
			break;
	}
	return work;
}

bool CFMUAutoroute51::lgraphscalesolutionvertex(ADR::Graph& g, const ADR::Object::const_ptr_t& wpt, double scale)
{
	if (!wpt)
		return false;
	bool work(false);
	ADR::Graph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(g); e0 != e1; ++e0) {
		ADR::GraphEdge& ee(g[*e0]);
		if (!ee.is_solution())
			continue;
		const ADR::GraphVertex& uu(g[source(*e0, g)]);
		if (uu.get_uuid() != wpt->get_uuid()) {
			const ADR::GraphVertex& vv(g[target(*e0, g)]);
			if (vv.get_uuid() != wpt->get_uuid())
				continue;
		}
		double newmetric(ee.get_solution_metric() * scale);
		if (newmetric != ee.get_solution_metric()) {
			if (&g == &m_graph)
				lgraphlogscaleedge(g, *e0, ee.get_solution(), scale);
			work = true;
		}
		ee.set_solution_metric(newmetric);
	}
	return work;
}

bool CFMUAutoroute51::lgraphscalesolutionvertex(ADR::Graph& g, const std::string& ident, double scale)
{
	bool work(false);
	std::string identu(Glib::ustring(ident).uppercase());
	ADR::Graph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(g); e0 != e1; ++e0) {
		ADR::GraphEdge& ee(g[*e0]);
		if (!ee.is_solution())
			continue;
		const ADR::GraphVertex& uu(g[source(*e0, g)]);
		if (uu.get_ident() != identu) {
			const ADR::GraphVertex& vv(g[target(*e0, g)]);
			if (vv.get_ident() != identu)
				continue;
		}
		double newmetric(ee.get_solution_metric() * scale);
		if (newmetric != ee.get_solution_metric()) {
			if (&g == &m_graph)
				lgraphlogscaleedge(g, *e0, ee.get_solution(), scale);
			work = true;
		}
		ee.set_solution_metric(newmetric);
	}
	return work;
}

bool CFMUAutoroute51::lgraphscalesolutionedge(ADR::Graph& g, const ADR::Object::const_ptr_t& wpt0, const ADR::Object::const_ptr_t& wpt1, double scale)
{
	if (!wpt0 || !wpt1)
		return false;
	bool work(false);
	ADR::Graph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(g); e0 != e1; ++e0) {
		ADR::GraphEdge& ee(g[*e0]);
		if (!ee.is_solution())
			continue;
		const ADR::GraphVertex& uu(g[source(*e0, g)]);
		if (uu.get_uuid() != wpt0->get_uuid())
			continue;
		const ADR::GraphVertex& vv(g[target(*e0, g)]);
		if (vv.get_uuid() != wpt1->get_uuid())
			continue;
		double newmetric(ee.get_solution_metric() * scale);
		if (newmetric != ee.get_solution_metric()) {
			if (&g == &m_graph)
				lgraphlogscaleedge(g, *e0, ee.get_solution(), scale);
			work = true;
		}
		ee.set_solution_metric(newmetric);
	}
	return work;
}

bool CFMUAutoroute51::lgraphscalesolutionedge(ADR::Graph& g, const std::string& id0, const std::string& id1, double scale)
{
	bool work(false);
	std::string id0u(Glib::ustring(id0).uppercase());
	std::string id1u(Glib::ustring(id1).uppercase());
	ADR::Graph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(g); e0 != e1; ++e0) {
		ADR::GraphEdge& ee(g[*e0]);
		if (!ee.is_solution())
			continue;
		const ADR::GraphVertex& uu(g[source(*e0, g)]);
		if (uu.get_ident() != id0u)
			continue;
		const ADR::GraphVertex& vv(g[target(*e0, g)]);
		if (vv.get_ident() != id1u)
			continue;
		double newmetric(ee.get_solution_metric() * scale);
		if (newmetric != ee.get_solution_metric()) {
			if (&g == &m_graph)
				lgraphlogscaleedge(g, *e0, ee.get_solution(), scale);
			work = true;
		}
		ee.set_solution_metric(newmetric);
	}
	return work;
}

bool CFMUAutoroute51::lgraphkilledge(const std::string& id0, const std::string& id1, const ADR::UUID& awymatch,
				     int lvlfrom, int lvlto, bool bidir, bool solutiononly, bool todct)
{
	bool work(false);
	std::string id0u(Glib::ustring(id0).uppercase());
	std::string id1u(Glib::ustring(id1).uppercase());
	lvlfrom *= 100;
	lvlto *= 100;
	ADR::Graph::UUIDEdgeFilter filter(m_graph, awymatch, ADR::GraphEdge::matchcomp_route);
	typedef boost::filtered_graph<ADR::Graph, ADR::Graph::UUIDEdgeFilter> fg_t;
	fg_t fg(m_graph, filter);
        const unsigned int pis(m_performance.size());
	{
		fg_t::vertex_iterator vb, ve;
		boost::tie(vb, ve) = boost::vertices(fg);
		for (fg_t::vertex_iterator v0i(vb); v0i != ve; ++v0i) {
			fg_t::vertex_descriptor u(*v0i);
			const ADR::GraphVertex& uu(fg[u]);
			if (uu.get_ident() != id0u)
				continue;
			for (fg_t::vertex_iterator v1i(vb); v1i != ve; ++v1i) {
				fg_t::vertex_descriptor v(*v1i);
				const ADR::GraphVertex& vv(fg[v]);
				if (vv.get_ident() != id1u)
					continue;
				if (false)
					std::cerr << "lgraphkilledge: trying path " << uu.get_ident() << " (" << u << ") -> "
						  << vv.get_ident() << " (" << v << ") alt " << lvlfrom << ".." << lvlto << std::endl;
				if (awymatch.is_nil()) {
					{
						fg_t::out_edge_iterator e0, e1;
						for (tie(e0, e1) = out_edges(u, fg); e0 != e1; ++e0) {
							if (target(*e0, fg) != v)
								continue;
							const ADR::GraphEdge& ee(fg[*e0]);
							if (!ee.is_dct())
								continue;
							if (solutiononly && !ee.is_solution())
								continue;
							for (unsigned int pi = 0; pi < pis; ++pi) {
								if (!ee.is_valid(pi))
									continue;
								if (solutiononly && ee.get_solution() != pi)
									continue;
								const Performance::Cruise& cruise(m_performance.get_cruise(pi));
								int alt(cruise.get_altitude());
								if (alt < lvlfrom || alt > lvlto)
									continue;
								lgraphkilledge(*e0, pi, true);
								work = true;
							}
						}
					}
					if (bidir) {
						fg_t::out_edge_iterator e0, e1;
						for (tie(e0, e1) = out_edges(v, fg); e0 != e1; ++e0) {
							if (target(*e0, fg) != u)
								continue;
							const ADR::GraphEdge& ee(fg[*e0]);
							if (!ee.is_dct())
								continue;
							if (solutiononly && !ee.is_solution())
								continue;
							for (unsigned int pi = 0; pi < pis; ++pi) {
								if (!ee.is_valid(pi))
									continue;
								if (solutiononly && ee.get_solution() != pi)
									continue;
								const Performance::Cruise& cruise(m_performance.get_cruise(pi));
								int alt(cruise.get_altitude());
								if (alt < lvlfrom || alt > lvlto)
									continue;
								lgraphkilledge(*e0, pi, true);
								work = true;
							}
						}
					}
					continue;
				}
				std::vector<fg_t::vertex_descriptor> predecessors(boost::num_vertices(fg), 0);
				dijkstra_shortest_paths(fg, u, boost::weight_map(boost::get(&ADR::GraphEdge::m_dist, m_graph)).
							predecessor_map(&predecessors[0]));
				fg_t::vertex_descriptor ve(v);
				while (ve != u) {
					fg_t::vertex_descriptor ue(predecessors[ve]);
					if (false)
						std::cerr << "lgraphkilledge: trying edge " << fg[ue].get_ident() << " (" << ue << ") -> "
							  << fg[ve].get_ident() << " (" << ve << ')' << std::endl;
					if (ue == ve)
						break;
					{
						ADR::GraphEdge edge(pis);
						fg_t::out_edge_iterator e0, e1;
						for (tie(e0, e1) = out_edges(ue, fg); e0 != e1; ++e0) {
							if (target(*e0, fg) != ve)
								continue;
							const ADR::GraphEdge& ee(fg[*e0]);
							if (!ee.is_match(awymatch))
								continue;
							if (solutiononly && !ee.is_solution())
								continue;
							for (unsigned int pi = 0; pi < pis; ++pi) {
								if (!ee.is_valid(pi))
									continue;
								if (solutiononly && ee.get_solution() != pi)
									continue;
								const Performance::Cruise& cruise(m_performance.get_cruise(pi));
								int alt(cruise.get_altitude());
								if (alt < lvlfrom || alt > lvlto)
									continue;
								if (todct && !ee.is_dct())
									edge.set_metric(pi, 1);
								lgraphkilledge(*e0, pi, true);
								work = true;
							}
						}
						lgraphaddedge(edge, ue, ve, setmetric_metric, true);
					}
					if (bidir) {
						ADR::GraphEdge edge(pis);
						fg_t::out_edge_iterator e0, e1;
						for (tie(e0, e1) = out_edges(ve, fg); e0 != e1; ++e0) {
							if (target(*e0, fg) != ue)
								continue;
							const ADR::GraphEdge& ee(fg[*e0]);
							if (!ee.is_match(awymatch))
								continue;
							if (solutiononly && !ee.is_solution())
								continue;
							for (unsigned int pi = 0; pi < pis; ++pi) {
								if (!ee.is_valid(pi))
									continue;
								if (solutiononly && ee.get_solution() != pi)
									continue;
								const Performance::Cruise& cruise(m_performance.get_cruise(pi));
								int alt(cruise.get_altitude());
								if (alt < lvlfrom || alt > lvlto)
									continue;
								if (todct && !ee.is_dct())
									edge.set_metric(pi, 1);
								lgraphkilledge(*e0, pi, true);
								work = true;
							}
						}
						lgraphaddedge(edge, ve, ue, setmetric_metric, true);
					}
					ve = ue;
				}
			}
		}
	}
	return work;
}

bool CFMUAutoroute51::lgraphkilledge(const std::string& id0, const std::string& id1, const std::string& awyname,
				     int lvlfrom, int lvlto, bool bidir, bool solutiononly, bool todct)
{
	std::string awyidu(Glib::ustring(awyname).uppercase());
	ADR::Graph::findresult_t r(m_graph.find_ident(awyidu));
	bool work(false);
	for (; r.first != r.second; ++r.first) {
		const ADR::Object::const_ptr_t& p(*r.first);
		if (!p)
			continue;
		if (false)
			std::cerr << "lgraphkilledge: awy " << awyidu << " object " << p->get_uuid() << std::endl;
		if (p->get_type() != ADR::Object::type_route &&
		    p->get_type() != ADR::Object::type_sid &&
		    p->get_type() != ADR::Object::type_star)
			continue;
		work = lgraphkilledge(id0, id1, p->get_uuid(), lvlfrom, lvlto, bidir, solutiononly, todct) || work;
	}
	return work;
}

bool CFMUAutoroute51::lgraphchangeoutgoing(const std::string& id, const std::string& awyname)
{
	std::string awyidu(Glib::ustring(awyname).uppercase());
	ADR::Graph::findresult_t r(m_graph.find_ident(awyidu));
	bool work(false);
	for (; r.first != r.second; ++r.first) {
		const ADR::Object::const_ptr_t& p(*r.first);
		if (!p)
			continue;
		if (p->get_type() != ADR::Object::type_route &&
		    p->get_type() != ADR::Object::type_sid &&
		    p->get_type() != ADR::Object::type_star)
			continue;
		work = lgraphchangeoutgoing(id, p->get_uuid()) || work;
	}
	return work;
}

bool CFMUAutoroute51::lgraphchangeoutgoing(const std::string& id, const ADR::UUID& awyname)
{
	bool work(false);
	std::string identu(Glib::ustring(id).uppercase());
	const unsigned int pis(m_performance.size());
	for (solutionvertices_t::const_iterator svi(m_solutionvertices.begin()), sve(m_solutionvertices.end()); svi != sve; ++svi) {
		LVertexLevel v(*svi);
		const ADR::GraphVertex& vv(m_graph[v.get_vertex()]);
		if (vv.get_ident() != identu)
			continue;
		ADR::Graph::out_edge_iterator e0, e1;
		for (tie(e0, e1) = out_edges(v.get_vertex(), m_graph); e0 != e1; ++e0) {
			ADR::GraphEdge& ee(m_graph[*e0]);
			if (!ee.is_match(awyname))
				continue;
			ADR::GraphEdge edge(pis);
			for (unsigned int pi = 0; pi < pis; ++pi) {
				if (!ee.is_valid(pi))
					continue;
				if (!ee.is_dct())
					edge.set_metric(pi, 1);
				lgraphkilledge(*e0, pi, true);
				work = true;
			}
			lgraphaddedge(edge, source(*e0, m_graph), target(*e0, m_graph), setmetric_metric, true);
		}
	}
	return work;
}

bool CFMUAutoroute51::lgraphchangeincoming(const std::string& id, const std::string& awyname)
{
	std::string awyidu(Glib::ustring(awyname).uppercase());
	ADR::Graph::findresult_t r(m_graph.find_ident(awyidu));
	bool work(false);
	for (; r.first != r.second; ++r.first) {
		const ADR::Object::const_ptr_t& p(*r.first);
		if (!p)
			continue;
		if (p->get_type() != ADR::Object::type_route &&
		    p->get_type() != ADR::Object::type_sid &&
		    p->get_type() != ADR::Object::type_star)
			continue;
		work = lgraphchangeincoming(id, p->get_uuid()) || work;
	}
	return work;
}

bool CFMUAutoroute51::lgraphchangeincoming(const std::string& id, const ADR::UUID& awyname)
{
	bool work(false);
	std::string identu(Glib::ustring(id).uppercase());
	const unsigned int pis(m_performance.size());
	for (solutionvertices_t::const_iterator svi(m_solutionvertices.begin()), sve(m_solutionvertices.end()); svi != sve; ++svi) {
		LVertexLevel v(*svi);
		const ADR::GraphVertex& vv(m_graph[v.get_vertex()]);
		if (vv.get_ident() != identu)
			continue;
		ADR::Graph::in_edge_iterator e0, e1;
		for (tie(e0, e1) = in_edges(v.get_vertex(), m_graph); e0 != e1; ++e0) {
			ADR::GraphEdge& ee(m_graph[*e0]);
			if (!ee.is_match(awyname))
				continue;
			ADR::GraphEdge edge(pis);
			for (unsigned int pi = 0; pi < pis; ++pi) {
				if (!ee.is_valid(pi))
					continue;
				if (!ee.is_dct())
					edge.set_metric(pi, 1);
				lgraphkilledge(*e0, pi, true);
				work = true;
			}
			lgraphaddedge(edge, source(*e0, m_graph), target(*e0, m_graph), setmetric_metric, true);
		}
	}
	return work;
}

bool CFMUAutoroute51::lgraphedgetodct(const std::string& awyname)
{
	std::string awyidu(Glib::ustring(awyname).uppercase());
	ADR::Graph::findresult_t r(m_graph.find_ident(awyidu));
	bool work(false);
	for (; r.first != r.second; ++r.first) {
		const ADR::Object::const_ptr_t& p(*r.first);
		if (!p)
			continue;
		if (p->get_type() != ADR::Object::type_route &&
		    p->get_type() != ADR::Object::type_sid &&
		    p->get_type() != ADR::Object::type_star)
			continue;
		work = lgraphedgetodct(p->get_uuid()) || work;
	}
	return work;
}

bool CFMUAutoroute51::lgraphedgetodct(const ADR::UUID& awy)
{
	if (awy.is_nil())
		return false;
	bool work(false);
	const unsigned int pis(m_performance.size());
	ADR::Graph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(m_graph); e0 != e1; ++e0) {
		ADR::GraphEdge& ee(m_graph[*e0]);
		if (!ee.is_solution())
			continue;
		if (!ee.is_match(awy))
			continue;
		if (!ee.is_solution_valid())
			continue;
		ADR::GraphEdge edge(pis);
		edge.set_metric(ee.get_solution(), 1);
		lgraphkillsolutionedge(*e0, true);
		lgraphaddedge(edge, source(*e0, m_graph), target(*e0, m_graph), setmetric_metric, true);
		work = true;
	}
	return work;
}

bool CFMUAutoroute51::lgraphkilloutgoing(const std::string& id)
{
	std::string identu(Glib::ustring(id).uppercase());
	bool work(false);
	ADR::Graph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(m_graph); e0 != e1; ++e0) {
		ADR::GraphEdge& ee(m_graph[*e0]);
		if (!ee.is_solution())
			continue;
		ADR::Graph::vertex_descriptor v(source(*e0, m_graph));
		const ADR::GraphVertex& vv(m_graph[v]);
		if (vv.get_ident() != identu)
			continue;
		lgraphkillsolutionedge(*e0);
		work = true;
	}
	return work;
}

bool CFMUAutoroute51::lgraphkillincoming(const std::string& id)
{
	std::string identu(Glib::ustring(id).uppercase());
	bool work(false);
	ADR::Graph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(m_graph); e0 != e1; ++e0) {
		ADR::GraphEdge& ee(m_graph[*e0]);
		if (!ee.is_solution())
			continue;
		ADR::Graph::vertex_descriptor v(target(*e0, m_graph));
		const ADR::GraphVertex& vv(m_graph[v]);
		if (vv.get_ident() != identu)
			continue;
		lgraphkillsolutionedge(*e0);
		work = true;
	}
	return work;
}

bool CFMUAutoroute51::lgraphkillsid(const std::string& id)
{
	std::string identu(Glib::ustring(id).uppercase());
	bool work(false);
	ADR::Graph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(m_graph); e0 != e1; ++e0) {
		ADR::GraphEdge& ee(m_graph[*e0]);
		if (!ee.is_solution())
			continue;
		if (!ee.is_sid())
			continue;
		ADR::Graph::vertex_descriptor v(target(*e0, m_graph));
		const ADR::GraphVertex& vv(m_graph[v]);
		if (vv.get_ident() != identu)
			continue;
		lgraphkillsolutionedge(*e0);
		work = true;
	}
	return work;
}

bool CFMUAutoroute51::lgraphkillstar(const std::string& id)
{
	std::string identu(Glib::ustring(id).uppercase());
	bool work(false);
	ADR::Graph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(m_graph); e0 != e1; ++e0) {
		ADR::GraphEdge& ee(m_graph[*e0]);
		if (!ee.is_solution())
			continue;
		if (!ee.is_star())
			continue;
		ADR::Graph::vertex_descriptor u(source(*e0, m_graph));
		const ADR::GraphVertex& uu(m_graph[u]);
		if (uu.get_ident() != identu)
			continue;
		lgraphkillsolutionedge(*e0);
		work = true;
	}
	return work;
}

bool CFMUAutoroute51::lgraphkillsidedge(const std::string& id, int lvlfrom, int lvlto, bool solutiononly, bool todct)
{
	std::string awyidu(Glib::ustring(id).uppercase());
	ADR::Graph::findresult_t r(m_graph.find_ident(awyidu));
	bool work(false);
	for (; r.first != r.second; ++r.first) {
		const ADR::Object::const_ptr_t& p(*r.first);
		if (!p)
			continue;
		if (p->get_type() != ADR::Object::type_sid)
			continue;
		work = lgraphkilledge(p->get_uuid(), lvlfrom, lvlto, solutiononly, todct) || work;
	}
	return work;
}

bool CFMUAutoroute51::lgraphkillstaredge(const std::string& id, int lvlfrom, int lvlto, bool solutiononly, bool todct)
{
	std::string awyidu(Glib::ustring(id).uppercase());
	ADR::Graph::findresult_t r(m_graph.find_ident(awyidu));
	bool work(false);
	for (; r.first != r.second; ++r.first) {
		const ADR::Object::const_ptr_t& p(*r.first);
		if (!p)
			continue;
		if (p->get_type() != ADR::Object::type_star)
			continue;
		work = lgraphkilledge(p->get_uuid(), lvlfrom, lvlto, solutiononly, todct) || work;
	}
	return work;
}

bool CFMUAutoroute51::lgraphkilledge(const ADR::UUID& awymatch, int lvlfrom, int lvlto, bool solutiononly, bool todct)
{
	bool work(false);
	const unsigned int pis(m_performance.size());
	unsigned int pi0(pis - 1U), pi1(0);
	for (unsigned int pi(0); pi < pis; ++pi) {
		const Performance::Cruise& cruise(m_performance.get_cruise(pi));
		int alt(cruise.get_altitude());
		if (alt < 100 * lvlfrom || alt > 100 * lvlto)
			continue;
		pi0 = std::min(pi0, pi);
		pi1 = std::max(pi1, pi);
	}
	if (pi0 > pi1)
		return false;
	ADR::Graph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(m_graph); e0 != e1; ++e0) {
		ADR::GraphEdge& ee(m_graph[*e0]);
		if (solutiononly && !ee.is_solution())
			continue;
		if (!ee.is_match(awymatch))
			continue;
		ADR::GraphEdge edge(pis);
		edge.clear_metric();
		for (unsigned int pi(pi0); pi <= pi1; ++pi) {
			if (!ee.is_valid(pi))
				continue;
			if (solutiononly && !ee.get_solution() == pi)
				continue;
			if (todct && !ee.is_dct())
				edge.set_metric(pi, 1);
			lgraphkilledge(*e0, pi, true);
			work = true;
		}
		lgraphaddedge(edge, boost::source(*e0, m_graph), boost::target(*e0, m_graph), setmetric_metric, true);
	}
	return work;
}

bool CFMUAutoroute51::lgraphkillclosesegments(const std::string& id0, const std::string& id1, const std::string& awyid, const ADR::UUID& solutionmatch)
{
        std::string awyidu(Glib::ustring(awyid).uppercase());
	ADR::Graph::findresult_t r(m_graph.find_ident(awyidu));
	bool work(false);
	for (; r.first != r.second; ++r.first) {
		const ADR::Object::const_ptr_t& p(*r.first);
		if (!p)
			continue;
		if (p->get_type() != ADR::Object::type_route)
			continue;
		work = lgraphkillclosesegments(id0, id1, p, solutionmatch);
	}
	return work;
}

bool CFMUAutoroute51::lgraphkillclosesegments(const std::string& id0, const std::string& id1, const ADR::Object::const_ptr_t& awy, const ADR::UUID& solutionmatch)
{
	if (!awy || awy->get_uuid().is_nil())
		return false;
        std::string id0u(Glib::ustring(id0).uppercase());
        std::string id1u(Glib::ustring(id1).uppercase());
	bool work(false);
        ADR::Graph::UUIDEdgeFilter filter(m_graph, awy->get_uuid(), ADR::GraphEdge::matchcomp_route);
        typedef boost::filtered_graph<ADR::Graph, ADR::Graph::UUIDEdgeFilter> fg_t;
        fg_t fg(m_graph, filter);
	if (false) {
		std::ostringstream oss;
		oss << "lgraphkillclosesegments: " << id0u << "--" << lgraphawyname(awy) << "->" << id1u;
		m_signal_log(log_debug0, oss.str());
	}
	// find combinations of source and destination supernodes
	ADR::Graph::vertex_iterator v0i, v0e;
	for (boost::tie(v0i, v0e) = boost::vertices(m_graph); v0i != v0e; ++v0i) {
		const ADR::GraphVertex& vv0(m_graph[*v0i]);
		if (vv0.get_ident() != id0u)
                        continue;
		ADR::Graph::vertex_iterator v1i, v1e;
		for (boost::tie(v1i, v1e) = boost::vertices(m_graph); v1i != v1e; ++v1i) {
			if (*v0i == *v1i)
				continue;
			const ADR::GraphVertex& vv1(m_graph[*v1i]);
			if (vv1.get_ident() != id1u)
                                continue;
			if (false) {
				std::ostringstream oss;
				oss << "lgraphkillclosesegments: trying route "
				    << vv0.get_ident() << ' ' << vv0.get_coord().get_lat_str2() << ' ' << vv0.get_coord().get_lon_str2()
				    << " -> "
				    << vv1.get_ident() << ' ' << vv1.get_coord().get_lat_str2() << ' ' << vv1.get_coord().get_lon_str2();
				m_signal_log(log_debug0, oss.str());
			}
			// find path from source to destination
			std::vector<ADR::Graph::vertex_descriptor> predecessors(boost::num_vertices(m_graph), 0);
			dijkstra_shortest_paths(fg, *v0i, boost::weight_map(boost::get(&ADR::GraphEdge::m_dist, m_graph)).
						predecessor_map(&predecessors[0]));
			ADR::Graph::vertex_descriptor ve(*v1i);
			while (ve != *v0i) {
				ADR::Graph::vertex_descriptor ue(predecessors[ve]);
				if (ve == ue)
					break;
				const ADR::GraphVertex& uue(m_graph[ue]);
				const ADR::GraphVertex& vve(m_graph[ve]);
				PolygonSimple poly(PolygonSimple::line_width_rectangular(uue.get_coord(), vve.get_coord(), 5.0));
				Rect bbox(poly.get_bbox());
				if (false) {
					std::ostringstream oss;
					oss << "lgraphkillclosesegments: trying segment "
					    << lgraphvertexname(ue) << ' ' << uue.get_coord().get_lat_str2() << ' ' << uue.get_coord().get_lon_str2()
					    << " -> "
					    << lgraphvertexname(ve) << ' ' << vve.get_coord().get_lat_str2() << ' ' << vve.get_coord().get_lon_str2()
					    << " poly";
					for (unsigned int i = 0; i < poly.size(); ++i)
						oss << ' ' << poly[i].get_lat_str2() << ' ' << poly[i].get_lon_str2();
					m_signal_log(log_debug0, oss.str());
				}
				ADR::Graph::edge_iterator e0, e1;
				for (tie(e0, e1) = edges(m_graph); e0 != e1; ++e0) {
					ADR::GraphEdge& ee(m_graph[*e0]);
					if (!ee.is_solution())
						continue;
					if (!ee.is_match(solutionmatch))
						continue;
					if (!ee.is_solution_valid())
						continue;
					ADR::Graph::vertex_descriptor u(source(*e0, m_graph));
					ADR::Graph::vertex_descriptor v(target(*e0, m_graph));
					const ADR::GraphVertex& uu(m_graph[u]);
					const ADR::GraphVertex& vv(m_graph[v]);
					{
						Rect bboxe(uu.get_coord(), uu.get_coord());
						bboxe = bboxe.add(vv.get_coord());
						if (!bbox.is_intersect(bboxe))
							continue;
					}
					if (!poly.windingnumber(uu.get_coord()) &&
					    !poly.windingnumber(vv.get_coord()) &&
					    !poly.is_intersection(uu.get_coord(), vv.get_coord()))
						continue;
					lgraphkillsolutionedge(*e0, true);
					work = true;
				}
				ve = ue;
			}
		}
	}
	return work;
}

bool CFMUAutoroute51::lgraphsolutiongroundclearance(void)
{
	m_signal_log(log_graphrule, "Ground Clearance");
	bool work(false);
        const unsigned int pis(m_performance.size());
	ADR::Graph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(m_graph); e0 != e1; ++e0) {
		ADR::GraphEdge& ee(m_graph[*e0]);
		if (!ee.is_solution())
			continue;
		if (!ee.is_solution_valid())
			continue;
		ADR::Graph::vertex_descriptor u(source(*e0, m_graph));
		if (u == m_vertexdep)
			continue;
		ADR::Graph::vertex_descriptor v(target(*e0, m_graph));
		if (v == m_vertexdest)
			continue;
		if (u == v)
			continue;
		const ADR::GraphVertex& uu(m_graph[u]);
		const ADR::GraphVertex& vv(m_graph[v]);
		int32_t minalt(0);
		{
			TopoDb30::Profile prof(m_topodb.get_profile(uu.get_coord(), vv.get_coord(), 5));
			if (prof.empty()) {
				std::ostringstream oss;
				oss << "Error computing ground clearance: " << lgraphvertexname(source(*e0, m_graph), ee.get_solution())
				    << ' ' << ee.get_route_ident_or_dct(true) << ' '
				    << lgraphvertexname(target(*e0, m_graph), ee.get_solution());
				m_signal_log(log_normal, oss.str());
				continue;
			}
			int32_t alt(prof.get_maxelev() * Point::m_to_ft);
			minalt = alt + 1000;
			if (alt >= 5000)
				minalt += 1000;
			{
				std::ostringstream oss;
				oss << "Leg: " << lgraphvertexname(source(*e0, m_graph), ee.get_solution())
				    << ' ' << ee.get_route_ident_or_dct(true) << ' '
				    << lgraphvertexname(target(*e0, m_graph), ee.get_solution())
				    << " ground elevation " << alt << "ft minimum level FL" << ((minalt + 99) / 100);
				m_signal_log(log_normal, oss.str());
			}
		}
		for (unsigned int pi = 0; pi < pis; ++pi) {
			const Performance::Cruise& cruise(m_performance.get_cruise(pi));
			if (cruise.get_altitude() >= minalt)
				break;
			ADR::Graph::out_edge_iterator e0, e1;
			tie(e0, e1) = boost::edge_range(u, v, m_graph);
			for (; e0 != e1; ++e0) {
				ADR::GraphEdge& ee(m_graph[*e0]);
				if (ee.is_solution(pi))
					work = true;
				lgraphkilledge(*e0, pi, true);
			}
			tie(e0, e1) = boost::edge_range(v, u, m_graph);
			for (; e0 != e1; ++e0) {
				ADR::GraphEdge& ee(m_graph[*e0]);
				if (ee.is_solution(pi))
					work = true;
				lgraphkilledge(*e0, pi, true);
			}
		}
	}
	return work;
}

void CFMUAutoroute51::parse_response(void)
{
	m_connchildtimeout.disconnect();
	if (is_stopped())
		return;
	m_signal_statuschange(statusmask_newvalidateresponse);
	bool work(m_validationresponse.size() == 1 && (m_validationresponse[0] == "NO ERRORS" ||
						       m_validationresponse[0] == "(R)ROUTE152: FLIGHT NOT APPLICABLE TO IFPS"));
	if (!work) {
		work = true;
		for (validationresponse_t::const_iterator i(m_validationresponse.begin()), e(m_validationresponse.end()); i != e; ++i) {
			if (i->empty())
				continue;
			const char * const *ignrx(ignoreregex);
			work = false;
			while (*ignrx) {
				Glib::RefPtr<Glib::Regex> rx(Glib::Regex::create(*ignrx++));
				Glib::MatchInfo minfo;
				if (rx->match(*i, minfo)) {
					work = true;
					break;
				}
			}
			if (!work)
				break;
		}
	}
	if (work) {
		if (m_pathprobe < 0) {
			if (lgraphsolutiongroundclearance()) {
				lgraphroute();
				return;
			}
			m_done = true;
			m_signal_log(log_fplproposal, get_simplefpl());
			stop(statusmask_none);
			return;
		}
		for (;;) {
			++m_pathprobe;
			if (m_pathprobe >= m_route.get_nrwpt())
				break;
			const FPlanWaypoint& wpt(m_route[m_pathprobe]);
			if (wpt.get_flags() & FPlanWaypoint::altflag_ifr &&
			    wpt.get_pathcode() != FPlanWaypoint::pathcode_sid &&
			    wpt.get_pathcode() != FPlanWaypoint::pathcode_star)
				break;
		}
		if (m_pathprobe >= m_route.get_nrwpt()) {
			m_pathprobe = -1;
			m_signal_log(log_fplproposal, get_simplefpl());
			m_signal_log(log_normal, "No progress made; stopping...");
			stop(statusmask_stoppingerrorenroute);
			return;
		}
		lgraphroute();
		return;
	}
	work = false;
	for (validationresponse_t::const_iterator i(m_validationresponse.begin()), e(m_validationresponse.end()); i != e; ++i) {
		if (i->empty())
			continue;
		m_signal_log(log_graphrule, *i);
		{
			std::string::size_type n(i->rfind('['));
			if (n != std::string::npos) {
				std::string::size_type ne(i->find(']', n + 1));
				if (ne != std::string::npos && ne - n >= 2 && ne - n <= 11) {
					std::string ruleid(i->substr(n + 1, ne - n - 1));
					for (ADR::RestrictionEval::rules_t::const_iterator
						     ri(m_eval.get_rules().begin()), re(m_eval.get_rules().end()); ri != re; ++ri) {
						const ADR::FlightRestrictionTimeSlice& ts((*ri)->operator()(get_deptime()).as_flightrestriction());
						if (!ts.is_valid() || ts.get_ident() != ruleid)
							continue;
						m_signal_log(log_graphruledesc, ts.get_instruction());
						m_signal_log(log_graphruleoprgoal, "");
						break;
					}
				}
			}
		}
		if (false)
			std::cerr << "Message: " << *i << std::endl;
		const struct parsefunctions *pf(parsefunctions);
		for (; pf->regex; ++pf) {
			Glib::RefPtr<Glib::Regex> rx(Glib::Regex::create(pf->regex));
			Glib::MatchInfo minfo;
			if (!rx->match(*i, minfo))
				continue;
			if (pf->func) {
				work = (this->*(pf->func))(minfo) || work;
				if (m_running)
					break;
				m_signal_log(log_fplproposal, get_simplefpl());
				m_signal_log(log_normal, "Stopping...");
				return;
			}
			break;
		}
		if (!pf->regex) {
			const char * const *ignrx(ignoreregex);
			while (*ignrx) {
				Glib::RefPtr<Glib::Regex> rx(Glib::Regex::create(*ignrx++));
				Glib::MatchInfo minfo;
				if (!rx->match(*i, minfo))
					continue;
				break;
			}
			if (!*ignrx)
				m_signal_log(log_normal, "Error Message not understood: \"" + *i + "\"");
		}
	}
	if (work)
		lgraphmodified();
	if (!work && m_pathprobe >= 0 && m_pathprobe + 1 < m_route.get_nrwpt()) {
		std::string id0(m_route[m_pathprobe].get_icao());
		if (id0.empty())
			id0 = m_route[m_pathprobe].get_name();
		std::string id1(m_route[m_pathprobe + 1].get_icao());
		if (id1.empty())
			id1 = m_route[m_pathprobe + 1].get_name();
		work = lgraphkilledge(id0, id1, ADR::GraphEdge::matchall, 0, 999, false, true, true);
	}
	// disable path probing
	if (work || true) {
		if (m_pathprobe >= 0)
			m_signal_log(log_normal, "Resuming normal operation...");
		m_pathprobe = -1;
		lgraphroute();
		return;
	}
	for (;;) {
		++m_pathprobe;
		if (m_pathprobe >= m_route.get_nrwpt())
			break;
		const FPlanWaypoint& wpt(m_route[m_pathprobe]);
		if (wpt.get_flags() & FPlanWaypoint::altflag_ifr &&
		    wpt.get_pathcode() != FPlanWaypoint::pathcode_sid &&
		    wpt.get_pathcode() != FPlanWaypoint::pathcode_star)
			break;
	}
	if (m_pathprobe >= m_route.get_nrwpt()) {
		m_pathprobe = -1;
		m_signal_log(log_fplproposal, get_simplefpl());
		m_signal_log(log_normal, "No progress made; stopping...");
		stop(statusmask_stoppingerrorenroute);
		return;
	}
	if (m_pathprobe == 0)
		m_signal_log(log_normal, "No progress made; starting path probing...");
	lgraphroute();
}

const struct CFMUAutoroute51::parsefunctions CFMUAutoroute51::parsefunctions[] = {
	{ "^ROUTE42: THE SID ([[:alnum:]]+) IS NOT VALID", &CFMUAutoroute51::parse_response_route42 },
	{ "^ROUTE42: THE STAR ([[:alnum:]]+) IS NOT VALID", &CFMUAutoroute51::parse_response_route43 },
	{ "^ROUTE43: THE SID ([[:alnum:]]+) IS NOT VALID", &CFMUAutoroute51::parse_response_route42 },
	{ "^ROUTE43: THE STAR ([[:alnum:]]+) IS NOT VALID", &CFMUAutoroute51::parse_response_route43 },
	{ "^ROUTE44: THE SID ([[:alnum:]]+) IS NOT VALID", &CFMUAutoroute51::parse_response_route42 },
	{ "^ROUTE44: THE STAR ([[:alnum:]]+) IS NOT VALID", &CFMUAutoroute51::parse_response_route43 },
	{ "^ROUTE45: THE SID ([[:alnum:]]+) IS NOT VALID", &CFMUAutoroute51::parse_response_route42 },
	{ "^ROUTE45: THE STAR ([[:alnum:]]+) IS NOT VALID", &CFMUAutoroute51::parse_response_route43 },
	{ "^ROUTE46: THE SID ([[:alnum:]]+) IS NOT VALID", &CFMUAutoroute51::parse_response_route42 },
	{ "^ROUTE46: THE STAR ([[:alnum:]]+) IS NOT VALID", &CFMUAutoroute51::parse_response_route43 },
	{ "^ROUTE47: THE SID ([[:alnum:]]+) IS NOT VALID", &CFMUAutoroute51::parse_response_route42 },
	{ "^ROUTE47: THE STAR ([[:alnum:]]+) IS NOT VALID", &CFMUAutoroute51::parse_response_route43 },
	{ "^ROUTE48: THE SID ([[:alnum:]]+) IS NOT VALID", &CFMUAutoroute51::parse_response_route42 },
	{ "^ROUTE48: THE STAR ([[:alnum:]]+) IS NOT VALID", &CFMUAutoroute51::parse_response_route43 },
	{ "^ROUTE49: THE POINT ([[:alnum:]]+) IS UNKNOWN IN THE CONTEXT OF THE ROUTE", &CFMUAutoroute51::parse_response_route49 },
	{ "^ROUTE52: THE DCT SEGMENT ([[:alnum:]]+)..([[:alnum:]]+) IS FORBIDDEN", &CFMUAutoroute51::parse_response_route52 },
	{ "^ROUTE130: UNKNOWN DESIGNATOR ([[:alnum:]]+)", &CFMUAutoroute51::parse_response_route130 },
	{ "^ROUTE134: THE STAR LIMIT IS EXCEEDED FOR AERODROME .*? CONNECTING TO ([[:alnum:]]+).", &CFMUAutoroute51::parse_response_route134 },
	{ "^ROUTE135: THE SID LIMIT IS EXCEEDED FOR AERODROME .*? CONNECTING TO ([[:alnum:]]+).", &CFMUAutoroute51::parse_response_route135 },
	{ "^ROUTE139: ([[:alnum:]]+) IS PRECEDED BY ([[:alnum:]]+) WHICH IS NOT ONE OF ITS POINTS", &CFMUAutoroute51::parse_response_route139 },
	{ "^ROUTE140: ([[:alnum:]]+) IS FOLLOWED BY ([[:alnum:]]+) WHICH IS NOT ONE OF ITS POINTS", &CFMUAutoroute51::parse_response_route140 },
	{ "^ROUTE165: THE DCT SEGMENT ([[:alnum:]]+)..([[:alnum:]]+)", &CFMUAutoroute51::parse_response_route165 },
	{ "^ROUTE168: INVALID DCT ([[:alnum:]]+)..([[:alnum:]]+).", &CFMUAutoroute51::parse_response_route168 },
	{ "^ROUTE171: CANNOT EXPAND THE ROUTE ([[:alnum:]]+)", &CFMUAutoroute51::parse_response_route171 },
	{ "^ROUTE172: MULTIPLE ROUTES BETWEEN ([[:alnum:]]+) AND ([[:alnum:]]+). ([[:alnum:]]+) IS SUGGESTED.", &CFMUAutoroute51::parse_response_route172 },
	{ "^ROUTE176: FLIGHT LEVEL AT ([[:alnum:]]+)/N([[:digit:]]+)F([[:digit:]]+) IS INVALID OR INCOMPATIBLE WITH AIRCRAFT PERFORMANCE", &CFMUAutoroute51::parse_response_route176 },
	{ "^ROUTE179: CRUISING FLIGHT LEVEL INVALID OR INCOMPATIBLE WITH AIRCRAFT PERFORMANCE", &CFMUAutoroute51::parse_response_route179 },
	{ "^PROF50: CLIMBING/DESCENDING OUTSIDE THE VERTICAL LIMITS OF SEGMENT ([[:alnum:]]+) ([[:alnum:]]+) ([[:alnum:]]+)", &CFMUAutoroute51::parse_response_prof50 },
	{ "^PROF193: IFR OPERATIONS AT AERODROME ([[:alnum:]]+) ARE NOT PERMITTED", &CFMUAutoroute51::parse_response_prof193 },
	{ "^PROF194: ([[:alnum:]]+) ([[:alnum:]]+) ([[:alnum:]]+) IS NOT AVAILABLE IN FL RANGE F([[:digit:]]+)..F([[:digit:]]+)", &CFMUAutoroute51::parse_response_prof194 },
	{ "^PROF195: ([[:alnum:]]+) ([[:alnum:]]+) ([[:alnum:]]+) DOES NOT EXIST IN FL RANGE F([[:digit:]]+)..F([[:digit:]]+)", &CFMUAutoroute51::parse_response_prof195 },
	{ "^PROF195: ([[:alnum:]]+) ([[:alnum:]]+) ([[:alnum:]]+) DOES NOT EXIST IN FL RANGE", &CFMUAutoroute51::parse_response_prof195b },
	{ "^PROF197: RS: ([[:alnum:]]+) ([[:alnum:]]+) ([[:alnum:]]+):F([[:digit:]]+)..F([[:digit:]]+) IS CLOSED FOR CRUISING", &CFMUAutoroute51::parse_response_prof197 },
	{ "^PROF197: RS: ([[:alnum:]]+):F([[:digit:]]+)..F([[:digit:]]+) IS CLOSED FOR CRUISING", &CFMUAutoroute51::parse_response_prof197b },
	{ "^PROF198: ([[:alnum:]]+) ([[:alnum:]]+) ([[:alnum:]]+) IS A CDR 3 IN FL RANGE F([[:digit:]]+)..F([[:digit:]]+)", &CFMUAutoroute51::parse_response_prof198 },
	{ "^PROF199: ([[:alnum:]]+) ([[:alnum:]]+) ([[:alnum:]]+) IS A CLOSED CDR 2 IN FL RANGE F([[:digit:]]+)..F([[:digit:]]+)", &CFMUAutoroute51::parse_response_prof199 },
	{ "^PROF200: ([[:alnum:]]+) ([[:alnum:]]+) ([[:alnum:]]+) IS A CLOSED CDR 1 IN FL RANGE F([[:digit:]]+)..F([[:digit:]]+)", &CFMUAutoroute51::parse_response_prof200 },
	{ "^PROF201: CANNOT CLIMB OR DESCEND ON ([[:alnum:]]+) ([[:alnum:]]+) ([[:alnum:]]+) IN FL RANGE CLOSED", &CFMUAutoroute51::parse_response_prof201 },
	{ "^PROF201: CANNOT CLIMB OR DESCEND ON ([[:alnum:]]+) ([[:alnum:]]+) ([[:alnum:]]+) IN FL RANGE F([[:digit:]]+)..F([[:digit:]]+)", &CFMUAutoroute51::parse_response_prof201b },
	{ "^PROF202: ([[:alnum:]]+) ([[:alnum:]]+) ([[:alnum:]]+) IS NOT AVAILABLE IN FL RANGE F([[:digit:]]+)..F([[:digit:]]+)", &CFMUAutoroute51::parse_response_prof202 },
	{ "^PROF204: RS: TRAFFIC VIA ([[:alnum:]]+) IS ON FORBIDDEN ROUTE REF:\\[([[:alnum:]]+)\\].* DCT ", &CFMUAutoroute51::parse_response_prof204f },
	{ "^PROF204: RS: TRAFFIC VIA ([[:alnum:]]+) IS ON FORBIDDEN ROUTE REF:\\[([[:alnum:]]+)\\] ([[:alnum:]]+)", &CFMUAutoroute51::parse_response_prof204e },
	{ "^PROF204: RS: TRAFFIC VIA ((?:[[:alnum:]]+)(?: [[:alnum:]]+)*?)(?: \\[[[:alnum:]]+\\.\\.[[:alnum:]]+\\])? IS ON FORBIDDEN ROUTE", &CFMUAutoroute51::parse_response_prof204 },
	{ "^PROF204: RS: TRAFFIC VIA ((?:[[:alnum:]]+)(?: [[:alnum:]]+)*?):F([[:digit:]]+)..F([[:digit:]]+)(?: \\[[[:alnum:]]+\\.\\.[[:alnum:]]+\\])? IS ON FORBIDDEN ROUTE", &CFMUAutoroute51::parse_response_prof204b },
	{ "^PROF204: RS: TRAFFIC VIA ((?:[[:alnum:]]+)(?: [[:alnum:]]+)*?):FLR..F([[:digit:]]+)(?: \\[[[:alnum:]]+\\.\\.[[:alnum:]]+\\])? IS ON FORBIDDEN ROUTE", &CFMUAutoroute51::parse_response_prof204bflr },
	{ "^PROF204: RS: TRAFFIC VIA ([[:alnum:]]+) ([[:alnum:]]+):F([[:digit:]]+)..F([[:digit:]]+) IS ON FORBIDDEN ROUTE", &CFMUAutoroute51::parse_response_prof204c },
	{ "^PROF204: RS: TRAFFIC VIA ([[:alnum:]]+) ([[:alnum:]]+) ([[:alnum:]]+):F([[:digit:]]+)..F([[:digit:]]+) IS ON FORBIDDEN ROUTE", &CFMUAutoroute51::parse_response_prof204d },
	{ "^PROF205: RS: TRAFFIC VIA ([[:alnum:]]+) IS OFF MANDATORY ROUTE", &CFMUAutoroute51::parse_response_prof205 },
	{ "^PROF205: RS: TRAFFIC VIA ((?:[[:alnum:]]+)(?: [[:alnum:]]+)*?) IS OFF MANDATORY ROUTE", &CFMUAutoroute51::parse_response_prof205b },
	{ "^PROF205: RS: TRAFFIC VIA ((?:[[:alnum:]]+)(?: [[:alnum:]]+)*?):F([[:digit:]]+)..F([[:digit:]]+) IS OFF MANDATORY ROUTE", &CFMUAutoroute51::parse_response_prof205c },
	{ "^PROF206: THE DCT SEGMENT ([[:alnum:]]+) .. ([[:alnum:]]+) IS NOT AVAILABLE IN FL RANGE F([[:digit:]]+)..F([[:digit:]]+)", &CFMUAutoroute51::parse_response_prof206 },
	{ "^EFPM228: INVALID VALUE((?: [[:alnum:]]+)*?) \\(([[:alnum:]]+)\\)", &CFMUAutoroute51::parse_response_efpm228 },
	{ "^FAIL: (.*)", &CFMUAutoroute51::parse_response_fail },
	{ 0, 0 }
};




const char *CFMUAutoroute51::ignoreregex[] = {
	// ignore 8.33 carriage errors
	"^PROF188:",
	"^PROF189:",
	"^PROF190:",
	0
};

bool CFMUAutoroute51::parse_response_route42(Glib::MatchInfo& minfo)
{
	return lgraphkillsidedge(minfo.fetch(1).uppercase(), 0, 999, false, false);
}

bool CFMUAutoroute51::parse_response_route43(Glib::MatchInfo& minfo)
{
	return lgraphkillstaredge(minfo.fetch(1).uppercase(), 0, 999, false, false);
}

bool CFMUAutoroute51::parse_response_route49(Glib::MatchInfo& minfo)
{
	return lgraphdisconnectvertex(minfo.fetch(1).uppercase(), 0, 600, true);
}

bool CFMUAutoroute51::parse_response_route52(Glib::MatchInfo& minfo)
{
	return lgraphkilledge(minfo.fetch(1), minfo.fetch(2), ADR::UUID::niluuid, 0, 999, true, false, false);
}

bool CFMUAutoroute51::parse_response_route130(Glib::MatchInfo& minfo)
{
	std::string idu(minfo.fetch(1).uppercase());
	bool work(lgraphdisconnectvertex(idu, 0, 999, true));
	work = lgraphedgetodct(idu) || work;
	return work;
}

bool CFMUAutoroute51::parse_response_route134(Glib::MatchInfo& minfo)
{
	bool work(false);
	ADR::Graph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(m_graph); e0 != e1; ++e0) {
		ADR::GraphEdge& ee(m_graph[*e0]);
		if (!ee.is_solution())
			continue;
		if (!ee.is_solution_valid())
			continue;
		if (!ee.is_star())
			continue;
		lgraphkillsolutionedge(*e0, true);
		work = true;
	}
	return work;
}

bool CFMUAutoroute51::parse_response_route135(Glib::MatchInfo& minfo)
{
	bool work(false);
	ADR::Graph::edge_iterator e0, e1;
	for (tie(e0, e1) = edges(m_graph); e0 != e1; ++e0) {
		ADR::GraphEdge& ee(m_graph[*e0]);
		if (!ee.is_solution())
			continue;
		if (!ee.is_solution_valid())
			continue;
		if (!ee.is_sid())
			continue;
		lgraphkillsolutionedge(*e0, true);
		work = true;
	}
	return work;
}

bool CFMUAutoroute51::parse_response_route139(Glib::MatchInfo& minfo)
{
	return lgraphchangeoutgoing(minfo.fetch(2), minfo.fetch(1));
}

bool CFMUAutoroute51::parse_response_route140(Glib::MatchInfo& minfo)
{
	return lgraphchangeincoming(minfo.fetch(2), minfo.fetch(1));
}

bool CFMUAutoroute51::parse_response_route165(Glib::MatchInfo& minfo)
{
	return lgraphkilledge(minfo.fetch(1), minfo.fetch(2), ADR::UUID::niluuid, 0, 999, true, false, false);
}

bool CFMUAutoroute51::parse_response_route168(Glib::MatchInfo& minfo)
{
	return lgraphkilledge(minfo.fetch(1), minfo.fetch(2), ADR::UUID::niluuid, 0, 999, true, false, false);
}

bool CFMUAutoroute51::parse_response_route171(Glib::MatchInfo& minfo)
{
	return lgraphedgetodct(minfo.fetch(1));
}

bool CFMUAutoroute51::parse_response_route172(Glib::MatchInfo& minfo)
{
	// minfo.fetch(3)
	return lgraphkilledge(minfo.fetch(1), minfo.fetch(2), ADR::UUID::niluuid, 0, 999, false, false, false);
}

bool CFMUAutoroute51::parse_response_route176(Glib::MatchInfo& minfo)
{
	int alt(strtol(minfo.fetch(3).c_str(), 0, 10));
	unsigned int pis(m_performance.size());
	for (unsigned int pi = 0; pi < pis; ++pi) {
		const Performance::Cruise& cruise(m_performance.get_cruise(pi));
		if (cruise.get_altitude() < alt * 100)
			continue;
		pis = pi;
		break;
	}
	if (pis >= m_performance.size())
		return false;
	if (!pis) {
		stop(statusmask_stoppingerrorinternalerror);
		return false;
	}
	m_performance.resize(pis);
	{
		ADR::Graph::edge_iterator e0, e1;
		for (tie(e0, e1) = edges(m_graph); e0 != e1; ++e0) {
			ADR::GraphEdge& ee(m_graph[*e0]);
			ee.resize(pis);
		}
	}
	return true;
}

bool CFMUAutoroute51::parse_response_route179(Glib::MatchInfo& minfo)
{
	unsigned int pis(0);
	{
		ADR::Graph::edge_iterator e0, e1;
		for (tie(e0, e1) = edges(m_graph); e0 != e1; ++e0) {
			ADR::GraphEdge& ee(m_graph[*e0]);
			if (!ee.is_solution())
				continue;
			if (!ee.is_solution_valid())
				continue;
			pis = std::max(pis, ee.get_solution());
		}
	}
	if (pis >= m_performance.size())
		return false;
	if (!pis) {
		stop(statusmask_stoppingerrorinternalerror);
		return false;
	}
	m_performance.resize(pis);
	{
		ADR::Graph::edge_iterator e0, e1;
		for (tie(e0, e1) = edges(m_graph); e0 != e1; ++e0) {
			ADR::GraphEdge& ee(m_graph[*e0]);
			ee.resize(pis);
		}
	}
	return true;
}

bool CFMUAutoroute51::parse_response_prof50(Glib::MatchInfo& minfo)
{
	if (lgraphkilledge(minfo.fetch(1), minfo.fetch(3), minfo.fetch(2), 0, 999, true, true, false))
		return true;
	return lgraphkilledge(minfo.fetch(1), minfo.fetch(3), ADR::UUID::niluuid, 0, 999, true, true, false);
}

bool CFMUAutoroute51::parse_response_prof193(Glib::MatchInfo& minfo)
{
	bool work(false);
	std::cerr << "Invalid IFR: " << minfo.fetch(1) << " DEP " << get_departure().get_icao() << " DEST " << get_destination().get_icao() << std::endl;
	if (get_departure().is_valid() && get_departure().get_icao() == minfo.fetch(1)) {
		work = work || get_departure_ifr();
	        m_airportifr[0] = false;
	}
	if (get_destination().is_valid() && get_destination().get_icao() == minfo.fetch(1)) {
		work = work || get_destination_ifr();
	        m_airportifr[1] = false;
	}
	return work;
}

bool CFMUAutoroute51::parse_response_prof194(Glib::MatchInfo& minfo)
{
	return parse_response_prof195(minfo);
}

bool CFMUAutoroute51::parse_response_prof195(Glib::MatchInfo& minfo)
{
	int altfrom(strtol(minfo.fetch(4).c_str(), 0, 10));
	int altto(strtol(minfo.fetch(5).c_str(), 0, 10));
	if (altfrom > altto)
		return false;
	if (lgraphkilledge(minfo.fetch(1), minfo.fetch(3), minfo.fetch(2), altfrom, altto, true, false, false))
		return true;
	return lgraphkilledge(minfo.fetch(1), minfo.fetch(3), ADR::UUID::niluuid, altfrom, altto, true, false, false);
}

bool CFMUAutoroute51::parse_response_prof195b(Glib::MatchInfo& minfo)
{
	return lgraphkilledge(minfo.fetch(1), minfo.fetch(3), minfo.fetch(2), 0, 999, true, false, false);
}

bool CFMUAutoroute51::parse_response_prof197(Glib::MatchInfo& minfo)
{
	int altfrom(strtol(minfo.fetch(4).c_str(), 0, 10));
	int altto(strtol(minfo.fetch(5).c_str(), 0, 10));
	if (altfrom > altto)
		return false;
	if (lgraphkilledge(minfo.fetch(1), minfo.fetch(3), minfo.fetch(2), altfrom, altto, true, true, false))
		return true;
	if (lgraphkilledge(minfo.fetch(1), minfo.fetch(3), ADR::UUID::niluuid, altfrom, altto, true, true, false))
		return true;
	return lgraphkilledge(minfo.fetch(1), minfo.fetch(3), minfo.fetch(2), altfrom, altto, false, false, true);
}

bool CFMUAutoroute51::parse_response_prof197b(Glib::MatchInfo& minfo)
{
	int altfrom(strtol(minfo.fetch(2).c_str(), 0, 10));
	int altto(strtol(minfo.fetch(3).c_str(), 0, 10));
	if (altfrom > altto)
		return false;
	return parse_response_prof204_singleident(minfo.fetch(1), altfrom, altto);
}

bool CFMUAutoroute51::parse_response_prof198(Glib::MatchInfo& minfo)
{
	return parse_response_prof195(minfo);
}

bool CFMUAutoroute51::parse_response_prof199(Glib::MatchInfo& minfo)
{
	return parse_response_prof195(minfo);
}

bool CFMUAutoroute51::parse_response_prof200(Glib::MatchInfo& minfo)
{
	return parse_response_prof195(minfo);
}

bool CFMUAutoroute51::parse_response_prof201(Glib::MatchInfo& minfo)
{
	if (parse_response_prof50(minfo))
		return true;
	if (lgraphkillclosesegments(minfo.fetch(0), minfo.fetch(2), minfo.fetch(1), ADR::UUID::niluuid))
		return true;
	bool work(lgraphkillsid(minfo.fetch(1)));
	work = lgraphkillstar(minfo.fetch(3)) || work;
	return work;
}

bool CFMUAutoroute51::parse_response_prof201b(Glib::MatchInfo& minfo)
{
	int altfrom(strtol(minfo.fetch(4).c_str(), 0, 10));
	int altto(strtol(minfo.fetch(5).c_str(), 0, 10));
	if (altfrom > altto)
		return false;
	if (lgraphkilledge(minfo.fetch(1), minfo.fetch(3), minfo.fetch(2), altfrom, altto, true, true, false))
		return true;
	if (lgraphkilledge(minfo.fetch(1), minfo.fetch(3), ADR::UUID::niluuid, altfrom, altto, true, true, false))
		return true;
	if (lgraphkillclosesegments(minfo.fetch(0), minfo.fetch(2), minfo.fetch(1), ADR::UUID::niluuid))
		return true;
	bool work(lgraphkillsid(minfo.fetch(1)));
	work = lgraphkillstar(minfo.fetch(3)) || work;
	return work;
}

bool CFMUAutoroute51::parse_response_prof202(Glib::MatchInfo& minfo)
{
	int altfrom(strtol(minfo.fetch(4).c_str(), 0, 10));
	int altto(strtol(minfo.fetch(5).c_str(), 0, 10));
	if (altfrom > altto)
		return false;
	if (lgraphkilledge(minfo.fetch(1), minfo.fetch(3), minfo.fetch(2), altfrom, altto, false, false, true))
		return true;
	if (lgraphkillclosesegments(minfo.fetch(0), minfo.fetch(2), minfo.fetch(1), ADR::UUID::niluuid))
		return true;
	return false;
}

bool CFMUAutoroute51::parse_response_prof204(Glib::MatchInfo& minfo)
{
	bool work(false);
	if (false)
		std::cerr << "PROF204: \"" << minfo.fetch(1) << "\" \"" << minfo.fetch(0) << "\"" << std::endl;
	typedef std::vector<std::string> token_t;
	token_t token(Glib::Regex::split_simple(" +", minfo.fetch(1)));
	for (int i = 0; i < token.size(); ++i) {
 	 	if (i + 2 < token.size() &&
		    !Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[i]) &&
		    Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[i + 1]) &&
		    !Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[i + 2])) {
			if (false)
				std::cerr << "PROF204: kill " << token[i] << "--" << token[i + 1] << "->" << token[i + 2] << std::endl;
			bool work2(lgraphkilledge(token[i], token[i + 2], token[i + 1], 0, 999, true, true, false));
			if (!work2) {
				if (true)
					std::cerr << "PROF204: " << token[i] << "--" << token[i + 1] << "->" << token[i + 2]
						  << " did not match, trying DCT" << std::endl;
				work2 = lgraphkilledge(token[i], token[i + 2], ADR::UUID::niluuid, 0, 999, true, true, false);
				if (!work2) {
					if (false)
						std::cerr << "PROF204: " << token[i] << "--->" << token[i + 2] << " did not match, trying airway convert to DCT" << std::endl;
					work2 = lgraphedgetodct(token[i + 1]);
					if (!work2) {
						if (false)
							std::cerr << "PROF204: could not convert " << token[i + 1]
								  << " to DCT, removing incoming/outgoing edges" << std::endl;
						work2 = lgraphkilloutgoing(token[i]);
						work2 = lgraphkillincoming(token[i + 2]) || work2;
					}
				}
				work = work2 || work;
			}
			i += 2;
			continue;
		}
		work = parse_response_prof204_singleident(token[i]) || work;
	}
	if (work)
		return true;
	if (token.size() >= 3 && 
	    !Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[0]) &&
	    Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[1]) &&
	    !Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[2])) {
		work = lgraphkillclosesegments(token[0], token[2], token[1], ADR::UUID::niluuid);
	}
	return work;
}

bool CFMUAutoroute51::parse_response_prof204b(Glib::MatchInfo& minfo)
{
	int altfrom(strtol(minfo.fetch(2).c_str(), 0, 10));
	int altto(strtol(minfo.fetch(3).c_str(), 0, 10));
	if (altfrom > altto)
		return false;
	bool work(false);
	if (false)
		std::cerr << "PROF204: \"" << minfo.fetch(1) << "\" \"" << minfo.fetch(0) << "\"" << std::endl;
	typedef std::vector<std::string> token_t;
	token_t token(Glib::Regex::split_simple(" +", minfo.fetch(1)));
 	for (int i = 0; i < token.size(); ++i) {
 	 	if (i + 2 < token.size() &&
		    !Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[i]) &&
		    Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[i + 1]) &&
		    !Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[i + 2])) {
			if (false)
				std::cerr << "PROF204: kill " << token[i] << "--" << token[i + 1] << "->" << token[i + 2] << std::endl;
			bool work2(lgraphkilledge(token[i], token[i + 2], token[i + 1], altfrom, altto, true, true, false));
			if (!work2) {
				if (false)
					std::cerr << "PROF204: " << token[i] << "--" << token[i + 1] << "->" << token[i + 2]
						  << " did not match, trying DCT" << std::endl;
				work2 = lgraphkilledge(token[i], token[i + 2], ADR::UUID::niluuid, altfrom, altto, true, true, false);
				if (!work2) {
					if (false)
						std::cerr << "PROF204: " << token[i] << "--->" << token[i + 2] << " did not match, trying airway convert to DCT" << std::endl;
					work2 = lgraphedgetodct(token[i + 1]);
					if (!work2) {
						if (false)
							std::cerr << "PROF204: could not convert " << token[i + 1]
								  << " to DCT, removing incoming/outgoing edges" << std::endl;
						work2 = lgraphkilloutgoing(token[i]);
						work2 = lgraphkillincoming(token[i + 2]) || work2;
					}
				}
				work = work2 || work;
			}
			i += 2;
			continue;
		}
		if (false)
			std::cerr << "PROF204: kill " << token[i] << std::endl;
		work = parse_response_prof204_singleident(token[i], altfrom, altto) || work;
	}
	return work;
}

bool CFMUAutoroute51::parse_response_prof204bflr(Glib::MatchInfo& minfo)
{
	int altfrom(0);
	int altto(strtol(minfo.fetch(2).c_str(), 0, 10));
	if (altfrom > altto)
		return false;
	bool work(false);
	if (false)
		std::cerr << "PROF204: \"" << minfo.fetch(1) << "\" \"" << minfo.fetch(0) << "\"" << std::endl;
	typedef std::vector<std::string> token_t;
	token_t token(Glib::Regex::split_simple(" +", minfo.fetch(1)));
 	for (int i = 0; i < token.size(); ++i) {
 	 	if (i + 2 < token.size() &&
		    !Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[i]) &&
		    Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[i + 1]) &&
		    !Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[i + 2])) {
			if (false)
				std::cerr << "PROF204: kill " << token[i] << "--" << token[i + 1] << "->" << token[i + 2] << std::endl;
			bool work2(lgraphkilledge(token[i], token[i + 2], token[i + 1], altfrom, altto, true, true, false));
			if (!work2) {
				if (false)
					std::cerr << "PROF204: " << token[i] << "--" << token[i + 1] << "->" << token[i + 2]
						  << " did not match, trying DCT" << std::endl;
				work2 = lgraphkilledge(token[i], token[i + 2], ADR::UUID::niluuid, altfrom, altto, true, true, false);
				if (!work2) {
					if (false)
						std::cerr << "PROF204: " << token[i] << "--->" << token[i + 2] << " did not match, trying airway convert to DCT" << std::endl;
					work2 = lgraphedgetodct(token[i + 1]);
					if (!work2) {
						if (false)
							std::cerr << "PROF204: could not convert " << token[i + 1]
								  << " to DCT, removing incoming/outgoing edges" << std::endl;
						work2 = lgraphkilloutgoing(token[i]);
						work2 = lgraphkillincoming(token[i + 2]) || work2;
					}
				}
				work = work2 || work;
			}
			i += 2;
			continue;
		}
		if (false)
			std::cerr << "PROF204: kill " << token[i] << std::endl;
		work = parse_response_prof204_singleident(token[i], altfrom, altto) || work;
	}
	return work;
}

bool CFMUAutoroute51::parse_response_prof204c(Glib::MatchInfo& minfo)
{
	int altfrom(strtol(minfo.fetch(3).c_str(), 0, 10));
	int altto(strtol(minfo.fetch(4).c_str(), 0, 10));
	if (altfrom > altto)
		return false;
	return lgraphkilledge(minfo.fetch(1), minfo.fetch(2), ADR::UUID::niluuid, altfrom, altto, true, false, false);
}

bool CFMUAutoroute51::parse_response_prof204d(Glib::MatchInfo& minfo)
{
	int altfrom(strtol(minfo.fetch(4).c_str(), 0, 10));
	int altto(strtol(minfo.fetch(5).c_str(), 0, 10));
	if (altfrom > altto)
		return false;
	if (lgraphkilledge(minfo.fetch(1), minfo.fetch(3), minfo.fetch(2), altfrom, altto, true, true, false))
		return true;
	return lgraphkilledge(minfo.fetch(1), minfo.fetch(3), ADR::UUID::niluuid, altfrom, altto, true, true, false);
}

bool CFMUAutoroute51::parse_response_prof204e(Glib::MatchInfo& minfo)
{
	if (lgraphdisconnectsolutionvertex(minfo.fetch(3)))
		return true;
	return parse_response_prof204_singleident(minfo.fetch(1));
}

bool CFMUAutoroute51::parse_response_prof204f(Glib::MatchInfo& minfo)
{
	{
		bool work(lgraphchangeoutgoing(minfo.fetch(1), ADR::UUID::niluuid));
		work = lgraphchangeincoming(minfo.fetch(1), ADR::UUID::niluuid) || work;
		if (work)
			return true;
	}
	return parse_response_prof204_singleident(minfo.fetch(1));
}

bool CFMUAutoroute51::parse_response_prof204_singleident(const Glib::ustring& ident)
{
	if (false)
		std::cerr << "PROF204/5: kill " << ident << std::endl;
	bool work(lgraphdisconnectsolutionvertex(ident));
	if (work)
		return work;
	if (ident.size() >= 2) {
		ADR::Database::findresults_t r(m_db.find_by_ident(ident, ADR::Database::comp_exact, 0, ADR::Database::loadmode_link,
								  get_deptime(), get_deptime()+1, ADR::Object::type_airspace,
								  ADR::Object::type_airspace, 0));
		for (ADR::Database::findresults_t::iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
			const ADR::AirspaceTimeSlice& ts(ri->get_obj()->operator()(get_deptime()).as_airspace());
			if (!ts.is_valid() || ts.get_ident() != ident)
				continue;
			{
				std::ostringstream oss;
				oss << "A " << ts.get_ident();
				m_signal_log(log_graphchange, oss.str());
			}
			Rect bbox;
			ts.get_bbox(bbox);
			ADR::Graph::edge_iterator e0, e1;
			for (tie(e0, e1) = edges(m_graph); e0 != e1; ++e0) {
				ADR::GraphEdge& ee(m_graph[*e0]);
				if (!ee.is_solution())
					continue;
				ADR::Graph::vertex_descriptor u(source(*e0, m_graph));
				const ADR::GraphVertex& uu(m_graph[u]);
				ADR::Graph::vertex_descriptor v(target(*e0, m_graph));
				const ADR::GraphVertex& vv(m_graph[v]);
				if (!ts.is_intersect(ADR::TimeTableEval(get_deptime(), uu.get_coord(), m_eval.get_specialdateeval()),
						     vv.get_coord(), ADR::AltRange::altignore, ADR::AltRange()))
					continue;
				lgraphkillsolutionedge(*e0, true);
				work = true;
			}
		}
	}
	return work;
}

bool CFMUAutoroute51::parse_response_prof204_singleident(const Glib::ustring& ident, int altfrom, int altto)
{
	if (false) {
		std::cerr << "PROF204/5: kill " << ident;
		if (altfrom < 0)
			std::cerr << " FLR";
		else
			std::cerr << " F" << std::setw(3) << std::setfill('0') << altfrom;
		if (altto < 0)
			std::cerr << "..CEIL";
		else
			std::cerr << "..F" << std::setw(3) << std::setfill('0') << altto;
		std::cerr << std::endl;
	}
	bool work(lgraphdisconnectvertex(ident, std::max(altfrom, 0), (altto < 0) ? 999 : altto));
	if (work)
		return work;
	ADR::AltRange ar(altfrom * 100, ADR::AltRange::alt_std, altto * 100, ADR::AltRange::alt_std);
	if (altfrom < 0) {
		ar.set_lower_mode(ADR::AltRange::alt_floor);
		ar.set_lower_alt(0);
	}
	if (altto < 0) {
		ar.set_upper_mode(ADR::AltRange::alt_ceiling);
		ar.set_upper_alt(ADR::AltRange::altmax);
	}
	unsigned int pi0(~0U), pi1(0);
	{
		const unsigned int pis(m_performance.size());
		for (unsigned int pi = 0; pi < pis; ++pi) {
			const Performance::Cruise& cruise(m_performance.get_cruise(pi));
			if (cruise.get_altitude() < ar.get_lower_alt() || cruise.get_altitude() > ar.get_upper_alt())
				continue;
			pi0 = std::min(pi0, pi);
			pi1 = std::max(pi1, pi);
		}
	}
	if (pi0 <= pi1 && ident.size() >= 2) {
		ADR::Database::findresults_t r(m_db.find_by_ident(ident, ADR::Database::comp_exact, 0, ADR::Database::loadmode_link,
								  get_deptime(), get_deptime()+1, ADR::Object::type_airspace,
								  ADR::Object::type_airspace, 0));
		for (ADR::Database::findresults_t::iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
			const ADR::AirspaceTimeSlice& ts(ri->get_obj()->operator()(get_deptime()).as_airspace());
			if (!ts.is_valid() || ts.get_ident() != ident)
				continue;
			{
				const Performance::Cruise& cruise0(m_performance.get_cruise(pi0));
				const Performance::Cruise& cruise1(m_performance.get_cruise(pi1));
				std::ostringstream oss;
				oss << "A " << ts.get_ident() << " F"
				    << std::setfill('0') << std::setw(3) << (cruise0.get_altitude() / 100) << "..F"
				    << std::setfill('0') << std::setw(3) << ((cruise1.get_altitude() + 99) / 100);
				m_signal_log(log_graphchange, oss.str());
			}
			Rect bbox;
			ts.get_bbox(bbox);
			ADR::Graph::edge_iterator e0, e1;
			for (tie(e0, e1) = edges(m_graph); e0 != e1; ++e0) {
				ADR::GraphEdge& ee(m_graph[*e0]);
				ADR::Graph::vertex_descriptor u(source(*e0, m_graph));
				const ADR::GraphVertex& uu(m_graph[u]);
				ADR::Graph::vertex_descriptor v(target(*e0, m_graph));
				const ADR::GraphVertex& vv(m_graph[v]);
				IntervalSet<int32_t> as(ts.get_point_intersect_altitudes(ADR::TimeTableEval(get_deptime(), uu.get_coord(), m_eval.get_specialdateeval()),
											 vv.get_coord(), ar, uu.get_uuid(), vv.get_uuid()));
				for (unsigned int pi = pi0; pi <= pi1; ++pi) {
					if (!ee.is_valid(pi))
						continue;
					const Performance::Cruise& cruise(m_performance.get_cruise(pi));
					if (!as.is_inside(cruise.get_altitude()))
						continue;		
					lgraphkilledge(*e0, pi, true);
					work = true;
				}
			}
		}
	}
	return work;
}

bool CFMUAutoroute51::parse_response_prof205(Glib::MatchInfo& minfo)
{
	return lgraphdisconnectsolutionvertex(minfo.fetch(1));
}

bool CFMUAutoroute51::parse_response_prof205b(Glib::MatchInfo& minfo)
{
	bool work(false);
	typedef std::vector<std::string> token_t;
	token_t token(Glib::Regex::split_simple(" +", minfo.fetch(1)));
 	for (int i = 0; i < token.size(); ++i) {
 	 	if (i + 2 < token.size() &&
		    !Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[i]) &&
		    Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[i + 1]) &&
		    !Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[i + 2])) {
			bool work2(lgraphkilledge(token[i], token[i + 2], token[i + 1], 0, 999, true, true, false));
			work2 = work2 || lgraphkilledge(token[i], token[i + 2], ADR::UUID::niluuid, 0, 999, true, true, false);
			work = work || work2;
			i += 2;
			continue;
		}
		work = parse_response_prof204_singleident(token[i]) || work;
	}
	return work;
}

bool CFMUAutoroute51::parse_response_prof205c(Glib::MatchInfo& minfo)
{
	int altfrom(strtol(minfo.fetch(4).c_str(), 0, 10));
	int altto(strtol(minfo.fetch(5).c_str(), 0, 10));
	if (altfrom > altto)
		return false;
	bool work(false);
	typedef std::vector<std::string> token_t;
	token_t token(Glib::Regex::split_simple(" +", minfo.fetch(1)));
 	for (int i = 0; i < token.size(); ++i) {
 	 	if (i + 2 < token.size() &&
		    !Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[i]) &&
		    Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[i + 1]) &&
		    !Engine::AirwayGraphResult::Vertex::is_ident_numeric(token[i + 2])) {
			bool work2(lgraphkilledge(token[i], token[i + 2], token[i + 1], altfrom, altto, true, true, false));
			work2 = work2 || lgraphkilledge(token[i], token[i + 2], ADR::UUID::niluuid, altfrom, altto, true, true, false);
			work = work || work2;
			i += 2;
			continue;
		}
		work = parse_response_prof204_singleident(token[i]) || work;
	}
	return work;
}

bool CFMUAutoroute51::parse_response_prof206(Glib::MatchInfo& minfo)
{
	int altfrom(strtol(minfo.fetch(3).c_str(), 0, 10));
	int altto(strtol(minfo.fetch(4).c_str(), 0, 10));
	if (altfrom > altto)
		return false;
	return lgraphkilledge(minfo.fetch(1), minfo.fetch(2), ADR::UUID::niluuid, altfrom, altto, true, false, false);
}

bool CFMUAutoroute51::parse_response_efpm228(Glib::MatchInfo& minfo)
{
	if (minfo.fetch(2).uppercase() == "ADEP") {
		m_signal_log(log_debug0, "Invalid departure, stopping...");
		stop(statusmask_stoppingerrorinternalerror);
	} else if (minfo.fetch(2).uppercase() == "ADES") {
		m_signal_log(log_debug0, "Invalid destination, stopping...");
		stop(statusmask_stoppingerrorinternalerror);
	} else if (minfo.fetch(2).uppercase() == "ARCTYP") {
		m_signal_log(log_debug0, "Invalid aircraft type" + minfo.fetch(1) + ", stopping...");
		stop(statusmask_stoppingerrorinternalerror);
	} else {
		m_signal_log(log_debug0, "Unknown IFPS error, stopping...");
		stop(statusmask_stoppingerrorinternalerror);
	}
	return false;
}

bool CFMUAutoroute51::parse_response_fail(Glib::MatchInfo& minfo)
{
	m_signal_log(log_debug0, "Fatal Error: " + minfo.fetch(1));
	stop(statusmask_stoppingerrorinternalerror);
	return false;
}
