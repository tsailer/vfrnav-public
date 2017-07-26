/***************************************************************************
 *   Copyright (C) 2012, 2013 by Thomas Sailer                             *
 *   t.sailer@alumni.ethz.ch                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "sysdeps.h"
#include "engine.h"

#ifdef HAVE_BOOST

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

Engine::DbThread::AirwayGraphResult::Edge::Edge(const Glib::ustring& ident, int16_t blevel, int16_t tlevel,
						element_t::elev_t telev, element_t::elev_t c5elev,
						element_t::airway_type_t typ, double dist)
	: m_ident(ident), m_dist(dist), m_baselevel(blevel), m_toplevel(tlevel),
	m_terrainelev(telev),  m_corridor5elev(c5elev), m_type(typ)
{
}

Engine::DbThread::AirwayGraphResult::Vertex::Vertex(const AirportsDb::Airport& el)
	: m_type(type_airport)
{
	if (!el.is_valid())
		return;
	m_ident = el.get_icao();
	if (m_ident.empty() || m_ident == "ZZZZ")
		m_ident = el.get_name();
	m_coord = el.get_coord();
	Engine::DbObject::ptr_t p(Engine::DbObject::create(el));
	m_obj.swap(p);
}

Engine::DbThread::AirwayGraphResult::Vertex::Vertex(const NavaidsDb::Navaid& el)
	: m_type(type_navaid)
{
	if (!el.is_valid())
		return;
	m_ident = el.get_icao();
	if (m_ident.empty())
		m_ident = el.get_name();
	m_coord = el.get_coord();
	Engine::DbObject::ptr_t p(Engine::DbObject::create(el));
	m_obj.swap(p);
}

Engine::DbThread::AirwayGraphResult::Vertex::Vertex(const WaypointsDb::Waypoint& el)
	: m_type(type_intersection)
{
	if (!el.is_valid())
		return;
	m_ident = el.get_name();
	m_coord = el.get_coord();
	Engine::DbObject::ptr_t p(Engine::DbObject::create(el));
	m_obj.swap(p);
}

Engine::DbThread::AirwayGraphResult::Vertex::Vertex(const MapelementsDb::Mapelement& el)
	: m_type(type_mapelement)
{
	if (!el.is_valid())
		return;
	m_ident = el.get_name();
	m_coord = el.get_coord();
	Engine::DbObject::ptr_t p(Engine::DbObject::create(el));
	m_obj.swap(p);
}

Engine::DbThread::AirwayGraphResult::Vertex::Vertex(const FPlanWaypoint& el)
	: m_type(type_fplwaypoint)
{
	m_ident = el.get_icao();
	if (m_ident.empty())
		m_ident = el.get_name();
	m_coord = el.get_coord();
	Engine::DbObject::ptr_t p(Engine::DbObject::create(el));
	m_obj.swap(p);
}

bool Engine::DbThread::AirwayGraphResult::Vertex::is_airport(void) const
{
	if (!m_obj)
		return false;
	return m_obj->is_airport();
}

bool Engine::DbThread::AirwayGraphResult::Vertex::is_navaid(void) const
{
	if (!m_obj)
		return false;
	return m_obj->is_navaid();
}

bool Engine::DbThread::AirwayGraphResult::Vertex::is_intersection(void) const
{
	if (!m_obj)
		return false;
	return m_obj->is_intersection();
}

bool Engine::DbThread::AirwayGraphResult::Vertex::is_mapelement(void) const
{
	if (!m_obj)
		return false;
	return m_obj->is_mapelement();
}

bool Engine::DbThread::AirwayGraphResult::Vertex::is_fplwaypoint(void) const
{
	if (!m_obj)
		return false;
	return m_obj->is_fplwaypoint();
}

bool Engine::DbThread::AirwayGraphResult::Vertex::get(AirportsDb::Airport& el) const
{
	if (!m_obj)
		return false;
	return m_obj->get(el);
}

bool Engine::DbThread::AirwayGraphResult::Vertex::get(NavaidsDb::Navaid& el) const
{
	if (!m_obj)
		return false;
	return m_obj->get(el);
}

bool Engine::DbThread::AirwayGraphResult::Vertex::get(WaypointsDb::Waypoint& el) const
{
	if (!m_obj)
		return false;
	return m_obj->get(el);
}

bool Engine::DbThread::AirwayGraphResult::Vertex::get(MapelementsDb::Mapelement& el) const
{
	if (!m_obj)
		return false;
	return m_obj->get(el);
}

bool Engine::DbThread::AirwayGraphResult::Vertex::get(FPlanWaypoint& el) const
{
	if (!m_obj)
		return false;
	return m_obj->get(el);
}

bool Engine::DbThread::AirwayGraphResult::Vertex::set(FPlanWaypoint& el) const
{
	if (!m_obj)
		return false;
	return m_obj->set(el);
}

unsigned int Engine::DbThread::AirwayGraphResult::Vertex::insert(FPlanRoute& route, uint32_t nr) const
{
	if (!m_obj)
		return false;
	return m_obj->insert(route, nr);
}

bool Engine::DbThread::AirwayGraphResult::Vertex::is_ident_numeric(const Glib::ustring& id)
{
	for (Glib::ustring::const_iterator i(id.begin()), e(id.end()); i != e; ++i)
		if (std::isdigit(*i))
			return true;
	return false;
}

bool Engine::DbThread::AirwayGraphResult::Vertex::is_ident_numeric(const std::string& id)
{
	for (Glib::ustring::const_iterator i(id.begin()), e(id.end()); i != e; ++i)
		if (std::isdigit(*i))
			return true;
	return false;
}

bool Engine::DbThread::AirwayGraphResult::Vertex::is_ifr_fpl(void) const
{
	switch (get_type()) {
	case type_navaid:
	{
		NavaidsDb::Navaid el;
		if (!get(el) || !el.is_valid())
			return false;
		if (!el.has_vor() && !el.has_ndb())
			return false;
		// FIXME: filter out approach aids
		return true;
	}

	case type_intersection:
	{
		WaypointsDb::Waypoint el;
		if (!get(el) || !el.is_valid())
			return false;
		if (el.get_type() == WaypointsDb::Waypoint::type_icao)
			return true;
		if (el.get_type() == WaypointsDb::Waypoint::type_other && el.get_icao().size() == 5)
			return true;
		return false;
	}

	default:
		return false;
	}
}

Engine::DbThread::AirwayGraphResult::Vertex::type_t Engine::DbThread::AirwayGraphResult::Vertex::from_type_string(const std::string& x)
{
	for (Engine::DbThread::AirwayGraphResult::Vertex::type_t t(Engine::DbThread::AirwayGraphResult::Vertex::type_airport);
	     t <= Engine::DbThread::AirwayGraphResult::Vertex::type_vfrreportingpt;
	     t = (Engine::DbThread::AirwayGraphResult::Vertex::type_t)(t + 1)) {
		if (to_str(t) != x)
			continue;
		return t;
	}
	return Engine::DbThread::AirwayGraphResult::Vertex::type_undefined;
}

const std::string& to_str(Engine::AirwayGraphResult::Vertex::type_t t)
{
	switch (t) {
	case Engine::AirwayGraphResult::Vertex::type_airport:
	{
		static const std::string r("airport");
		return r;
	}

	case Engine::AirwayGraphResult::Vertex::type_navaid:
	{
		static const std::string r("navaid");
		return r;
	}

	case Engine::AirwayGraphResult::Vertex::type_intersection:
	{
		static const std::string r("intersection");
		return r;
	}

	case Engine::AirwayGraphResult::Vertex::type_mapelement:
	{
		static const std::string r("mapelement");
		return r;
	}

	default:
	case Engine::AirwayGraphResult::Vertex::type_undefined:
	{
		static const std::string r("undefined");
		return r;
	}

	case Engine::AirwayGraphResult::Vertex::type_fplwaypoint:
	{
		static const std::string r("fplwaypoint");
		return r;
	}

	case Engine::AirwayGraphResult::Vertex::type_vfrreportingpt:
	{
		static const std::string r("vfrreportingpt");
		return r;
	}
	}
}

std::string to_str(Engine::AirwayGraphResult::Vertex::typemask_t tm)
{
	std::string r;
	bool subseq(false);
	for (Engine::AirwayGraphResult::Vertex::type_t t(Engine::AirwayGraphResult::Vertex::type_airport);
	     t <= Engine::AirwayGraphResult::Vertex::type_vfrreportingpt;
	     t = (Engine::AirwayGraphResult::Vertex::type_t)(t + 1)) {
		if (!(tm & (1U << t)))
			continue;
		if (subseq)
			r += ",";
		subseq = true;
		r += to_str(t);
	}
	return r;
}

constexpr double Engine::DbThread::AirwayGraphResult::Graph::intersection_tolerance;
constexpr double Engine::DbThread::AirwayGraphResult::Graph::navaid_tolerance;

void Engine::DbThread::AirwayGraphResult::Graph::clear(void)
{
	boostgraph_t::clear();
	m_intersectionmap.clear();
}

Engine::DbThread::AirwayGraphResult::Graph::vertex_descriptor Engine::DbThread::AirwayGraphResult::Graph::add(const Vertex& vn)
{
	if (vn.get_ident().empty())
		return num_vertices(*this);
	vertex_descriptor v0;
	Glib::ustring vt(vn.get_ident().uppercase());
	bool newv(true);
	float tol(intersection_tolerance);
	if (vn.get_type() == Vertex::type_navaid)
		tol = navaid_tolerance;
	Vertex::typemask_t tmask((Vertex::typemask_t)(1 << vn.get_type()) | Vertex::typemask_undefined);
	for (intersectionmap_t::const_iterator i0(m_intersectionmap.lower_bound(vt)), i1(m_intersectionmap.upper_bound(vt)); i0 != i1; ++i0) {
		const Vertex& v((*this)[i0->second]);
		if (!v.is_match(tmask))
			continue;
		if (vn.get_coord().simple_distance_nmi(v.get_coord()) >= tol)
			continue;
		newv = false;
		v0 = i0->second;
		break;
	}
	if (newv) {
		v0 = boost::add_vertex(vn, *this);
		m_intersectionmap.insert(std::make_pair(vt, v0));
	} else {
		Vertex& v((*this)[v0]);
		if (v.get_type() != vn.get_type() || !v.get_object())
			v = vn;
	}
	if (false) {
		const Vertex& v((*this)[v0]);
		std::cerr << "Adding " << vn.get_type_string() << ' ' << v.get_ident() << ' '
			  << v.get_coord().get_lat_str2() << ' ' << v.get_coord().get_lon_str2() << std::endl;
	}
	return v0;
}

std::pair<Engine::DbThread::AirwayGraphResult::Graph::vertex_descriptor,
	  Engine::DbThread::AirwayGraphResult::Graph::vertex_descriptor> Engine::DbThread::AirwayGraphResult::Graph::add(const element_t& el)
{
	if (el.get_base_level() > el.get_top_level())
		return std::pair<vertex_descriptor,vertex_descriptor>(num_vertices(*this), num_vertices(*this));
	vertex_descriptor v0, v1;
	{
		Glib::ustring vt(el.get_begin_name().uppercase());
		bool newv(true);
		for (intersectionmap_t::const_iterator i0(m_intersectionmap.lower_bound(vt)), i1(m_intersectionmap.upper_bound(vt)); i0 != i1; ++i0) {
			const Vertex& v((*this)[i0->second]);
			if (!v.is_match(Vertex::typemask_awyendpoints))
				continue;
			if (el.get_begin_coord().simple_distance_nmi(v.get_coord()) >= intersection_tolerance)
				continue;
			newv = false;
			v0 = i0->second;
			break;
		}
		if (newv) {
			v0 = boost::add_vertex(Vertex(el.get_begin_name(), el.get_begin_coord(), Vertex::type_undefined), *this);
			m_intersectionmap.insert(std::make_pair(vt, v0));
		}
	}
	{
		Glib::ustring vt(el.get_end_name().uppercase());
		bool newv(true);
		for (intersectionmap_t::const_iterator i0(m_intersectionmap.lower_bound(vt)), i1(m_intersectionmap.upper_bound(vt)); i0 != i1; ++i0) {
			const Vertex& v((*this)[i0->second]);
			if (!v.is_match(Vertex::typemask_awyendpoints))
				continue;
			if (el.get_end_coord().simple_distance_nmi(v.get_coord()) >= intersection_tolerance)
				continue;
			newv = false;
			v1 = i0->second;
			break;
		}
		if (newv) {
			v1 = boost::add_vertex(Vertex(el.get_end_name(), el.get_end_coord(), Vertex::type_undefined), *this);
			m_intersectionmap.insert(std::make_pair(vt, v1));
		}
	}
	const Vertex& vv0((*this)[v0]);
	const Vertex& vv1((*this)[v1]);
	double dist(vv0.get_coord().spheric_distance_nmi_dbl(vv1.get_coord()));
	element_t::nameset_t names(el.get_name_set());
	for (element_t::nameset_t::const_iterator ni(names.begin()), ne(names.end()); ni != ne; ++ni) {
		// assume airways are bidirectional, so insert both edge directions into digraph
		Glib::ustring name(ni->uppercase());
		if (false)
			std::cerr << "Adding Edges: " << vv0.get_ident() << "<->" << vv1.get_ident() << ": " << name
				  << ' ' << el.get_base_level() << "..." << el.get_top_level() << std::endl;
		boost::add_edge(v0, v1, Edge(name, el.get_base_level(), el.get_top_level(),
					     el.get_terrain_elev(), el.get_corridor5_elev(), el.get_type(), dist), *this);
		boost::add_edge(v1, v0, Edge(name, el.get_base_level(), el.get_top_level(),
					     el.get_terrain_elev(), el.get_corridor5_elev(), el.get_type(), dist), *this);
	}
	return std::pair<vertex_descriptor,vertex_descriptor>(v0, v1);
}

Engine::DbThread::AirwayGraphResult::Graph::vertex_descriptor Engine::DbThread::AirwayGraphResult::Graph::add(const AirportsDb::element_t& el)
{
	if (!el.is_valid())
		return num_vertices(*this);
	return add(Vertex(el));
}

Engine::DbThread::AirwayGraphResult::Graph::vertex_descriptor Engine::DbThread::AirwayGraphResult::Graph::add(const NavaidsDb::element_t& el)
{
	if (!el.is_valid())
		return num_vertices(*this);
	return add(Vertex(el));
}

Engine::DbThread::AirwayGraphResult::Graph::vertex_descriptor Engine::DbThread::AirwayGraphResult::Graph::add(const WaypointsDb::element_t& el)
{
	if (!el.is_valid())
		return num_vertices(*this);
	return add(Vertex(el));
}

Engine::DbThread::AirwayGraphResult::Graph::vertex_descriptor Engine::DbThread::AirwayGraphResult::Graph::add(const MapelementsDb::element_t& el)
{
	if (!el.is_valid())
		return num_vertices(*this);
	Vertex vn(el);
	// only "point" types are useful
	switch (el.get_typecode()) {
	case MapelementsDb::Mapelement::maptyp_city:
	case MapelementsDb::Mapelement::maptyp_village:
	case MapelementsDb::Mapelement::maptyp_spot:
	case MapelementsDb::Mapelement::maptyp_landmark:
	case MapelementsDb::Mapelement::maptyp_lake:
	case MapelementsDb::Mapelement::maptyp_lake_t:
		break;

	default:
		return num_vertices(*this);
	}
	return add(Vertex(el));
}

void Engine::DbThread::AirwayGraphResult::Graph::add(const elementvector_t& ev)
{
	add(ev.begin(), ev.end());
}

void Engine::DbThread::AirwayGraphResult::Graph::add(const AirportsDb::elementvector_t& ev)
{
	add(ev.begin(), ev.end());
}

void Engine::DbThread::AirwayGraphResult::Graph::add(const NavaidsDb::elementvector_t& ev)
{
	add(ev.begin(), ev.end());
}

void Engine::DbThread::AirwayGraphResult::Graph::add(const WaypointsDb::elementvector_t& ev)
{
	add(ev.begin(), ev.end());
}

void Engine::DbThread::AirwayGraphResult::Graph::add(const MapelementsDb::elementvector_t& ev)
{
	add(ev.begin(), ev.end());
}

bool Engine::DbThread::AirwayGraphResult::Graph::find_intersection(vertex_descriptor& u, const Glib::ustring& name, Vertex::typemask_t tmask,
								   bool require_outedge, bool require_inedge)
{
	Glib::ustring vt(name.uppercase());
	for (intersectionmap_t::const_iterator i0(m_intersectionmap.lower_bound(vt)), i1(m_intersectionmap.upper_bound(vt)); i0 != i1; ++i0) {
		const Vertex& v((*this)[i0->second]);
		if (!v.is_match(tmask))
			continue;
		if (require_outedge && !boost::out_degree(i0->second, *this))
			continue;
		if (require_inedge && !boost::in_degree(i0->second, *this))
			continue;
		u = i0->second;
		return true;
	}
	return false;
}

bool Engine::DbThread::AirwayGraphResult::Graph::find_intersection(vertex_descriptor& u, const Glib::ustring& name, const Point& pt, Vertex::typemask_t tmask,
								   bool require_outedge, bool require_inedge)
{
	uint64_t dist(std::numeric_limits<uint64_t>::max());
	Glib::ustring vt(name.uppercase());
	for (intersectionmap_t::const_iterator i0(m_intersectionmap.lower_bound(vt)), i1(m_intersectionmap.upper_bound(vt)); i0 != i1; ++i0) {
		const Vertex& v((*this)[i0->second]);
		if (!v.is_match(tmask))
			continue;
		if (require_outedge && !boost::out_degree(i0->second, *this))
			continue;
		if (require_inedge && !boost::in_degree(i0->second, *this))
			continue;
		uint64_t d(pt.simple_distance_rel((*this)[i0->second].get_coord()));
		if (d >= dist)
			continue;
		dist = d;
		u = i0->second;
	}
	return (dist < std::numeric_limits<uint64_t>::max());
}

bool Engine::DbThread::AirwayGraphResult::Graph::find_intersection(vertex_descriptor& u, const Glib::ustring& name, const Glib::ustring& awyname, Vertex::typemask_t tmask,
								   bool require_outedge, bool require_inedge)
{
	AwyEdgeFilter filter(*this, awyname);
	typedef boost::filtered_graph<Graph, AwyEdgeFilter> fg_t;
	fg_t fg(*this, filter);
	Glib::ustring vt(name.uppercase());
	for (intersectionmap_t::const_iterator i0(m_intersectionmap.lower_bound(vt)), i1(m_intersectionmap.upper_bound(vt)); i0 != i1; ++i0) {
		const Vertex& v((*this)[i0->second]);
		if (!v.is_match(tmask))
			continue;
		degree_size_type outdeg(boost::out_degree(i0->second, fg));
		if (require_outedge && !outdeg)
			continue;
		degree_size_type indeg(boost::in_degree(i0->second, fg));
		if (require_inedge && !indeg)
			continue;
		if (outdeg + indeg < 1)
			continue;
		u = i0->second;
		return true;
	}
	return false;
}

Engine::DbThread::AirwayGraphResult::Graph::vertex_descriptor_set_t Engine::DbThread::AirwayGraphResult::Graph::find_airway_terminals(const Glib::ustring& awyname)
{
	AwyEdgeFilter filter(*this, awyname);
	typedef boost::filtered_graph<Graph, AwyEdgeFilter> fg_t;
	fg_t fg(*this, filter);
	vertex_descriptor_set_t r;
	fg_t::vertex_iterator v0, v1;
	boost::tie(v0, v1) = boost::vertices(fg);
	for (; v0 != v1; ++v0) {
		degree_size_type outdeg(boost::out_degree(*v0, fg));
		if (outdeg > 1)
			continue;
		degree_size_type indeg(boost::in_degree(*v0, fg));
		if (indeg > 1)
			continue;
		if (outdeg + indeg < 1)
			continue;
		r.insert(*v0);
	}
	return r;
}

Engine::DbThread::AirwayGraphResult::Graph::vertex_descriptor_set_t Engine::DbThread::AirwayGraphResult::Graph::find_airway_begin(const Glib::ustring& awyname)
{
	AwyEdgeFilter filter(*this, awyname);
	typedef boost::filtered_graph<Graph, AwyEdgeFilter> fg_t;
	fg_t fg(*this, filter);
	vertex_descriptor_set_t r;
	fg_t::vertex_iterator v0, v1;
	boost::tie(v0, v1) = boost::vertices(fg);
	for (; v0 != v1; ++v0) {
		degree_size_type outdeg(boost::out_degree(*v0, fg));
		if (outdeg != 1)
			continue;
		degree_size_type indeg(boost::in_degree(*v0, fg));
		if (indeg > 1)
			continue;
		r.insert(*v0);
	}
	return r;
}

Engine::DbThread::AirwayGraphResult::Graph::vertex_descriptor_set_t Engine::DbThread::AirwayGraphResult::Graph::find_airway_end(const Glib::ustring& awyname)
{
	AwyEdgeFilter filter(*this, awyname);
	typedef boost::filtered_graph<Graph, AwyEdgeFilter> fg_t;
	fg_t fg(*this, filter);
	vertex_descriptor_set_t r;
	fg_t::vertex_iterator v0, v1;
	boost::tie(v0, v1) = boost::vertices(fg);
	for (; v0 != v1; ++v0) {
		degree_size_type outdeg(boost::out_degree(*v0, fg));
		if (outdeg > 1)
			continue;
		degree_size_type indeg(boost::in_degree(*v0, fg));
		if (indeg != 1)
			continue;
		r.insert(*v0);
	}
	return r;
}

Engine::DbThread::AirwayGraphResult::Graph::vertex_descriptor_set_t Engine::DbThread::AirwayGraphResult::Graph::find_airway_vertices(const Glib::ustring& awyname)
{
	AwyEdgeFilter filter(*this, awyname);
	typedef boost::filtered_graph<Graph, AwyEdgeFilter> fg_t;
	fg_t fg(*this, filter);
	vertex_descriptor_set_t r;
	fg_t::vertex_iterator v0, v1;
	boost::tie(v0, v1) = boost::vertices(fg);
	for (; v0 != v1; ++v0) {
		degree_size_type outdeg(boost::out_degree(*v0, fg));
		degree_size_type indeg(boost::in_degree(*v0, fg));
		if (!outdeg && !indeg)
			continue;
		r.insert(*v0);
	}
	return r;
}

Engine::DbThread::AirwayGraphResult::Graph::vertex_descriptor_set_t Engine::DbThread::AirwayGraphResult::Graph::find_intersections(const Glib::ustring& name)
{
	vertex_descriptor_set_t r;
	Glib::ustring vt(name.uppercase());
	for (intersectionmap_t::const_iterator i0(m_intersectionmap.lower_bound(vt)), i1(m_intersectionmap.upper_bound(vt)); i0 != i1; ++i0)
		r.insert(i0->second);
	return r;
}

bool Engine::DbThread::AirwayGraphResult::Graph::find_nearest(vertex_descriptor& u, const Point& pt, Vertex::typemask_t tmask,
							      bool require_outedge, bool require_inedge)
{
	uint64_t dist(std::numeric_limits<uint64_t>::max());
	Graph::vertex_iterator v0, v1;
	boost::tie(v0, v1) = boost::vertices(*this);
	for (; v0 != v1; ++v0) {
		const Vertex& v((*this)[*v0]);
		if (!v.is_match(tmask))
			continue;
		if (require_outedge && !boost::out_degree(*v0, *this))
			continue;
		if (require_inedge && !boost::in_degree(*v0, *this))
			continue;
		uint64_t d(pt.simple_distance_rel(v.get_coord()));
		if (d >= dist)
			continue;
		if (require_outedge && !boost::out_degree(*v0, *this))
			continue;
		dist = d;
		u = *v0;
	}
	return (dist < std::numeric_limits<uint64_t>::max());
}

bool Engine::DbThread::AirwayGraphResult::Graph::find_nearest(vertex_descriptor& u, const Point& pt, const Glib::ustring& awyname, Vertex::typemask_t tmask,
							      bool require_outedge, bool require_inedge)
{
	AwyEdgeFilter filter(*this, awyname);
	typedef boost::filtered_graph<Graph, AwyEdgeFilter> fg_t;
	fg_t fg(*this, filter);
	uint64_t dist(std::numeric_limits<uint64_t>::max());
	fg_t::vertex_iterator v0, v1;
	boost::tie(v0, v1) = boost::vertices(fg);
	for (; v0 != v1; ++v0) {
		const Vertex& v(fg[*v0]);
		if (!v.is_match(tmask))
			continue;
		if (require_outedge && !boost::out_degree(*v0, fg))
			continue;
		if (require_inedge && !boost::in_degree(*v0, fg))
			continue;
		uint64_t d(pt.simple_distance_rel(v.get_coord()));
		if (d >= dist)
			continue;
		dist = d;
		u = *v0;
	}
	return (dist < std::numeric_limits<uint64_t>::max());
}

bool Engine::DbThread::AirwayGraphResult::Graph::find_edge(edge_descriptor& u, const Glib::ustring& beginname, const Glib::ustring& endname)
{
	Glib::ustring bname(beginname.uppercase());
	Glib::ustring ename(endname.uppercase());
	for (intersectionmap_t::const_iterator i0(m_intersectionmap.lower_bound(bname)), i1(m_intersectionmap.upper_bound(bname)); i0 != i1; ++i0) {
		Graph::out_edge_iterator e0, e1;
		for (boost::tie(e0, e1) = boost::out_edges(i0->second, *this); e0 != e1; ++e0) {
			const Vertex& v1((*this)[boost::target(*e0, *this)]);
			if (v1.get_ident() != ename)
				continue;
			u = *e0;
			return true;
		}
	}
	return false;
}

bool Engine::DbThread::AirwayGraphResult::Graph::find_edge(edge_descriptor& u, const Glib::ustring& beginname, const Glib::ustring& endname, const Glib::ustring& awyname)
{
	Glib::ustring bname(beginname.uppercase());
	Glib::ustring ename(endname.uppercase());
	Glib::ustring aname(awyname.uppercase());
	for (intersectionmap_t::const_iterator i0(m_intersectionmap.lower_bound(bname)), i1(m_intersectionmap.upper_bound(bname)); i0 != i1; ++i0) {
		Graph::out_edge_iterator e0, e1;
		for (boost::tie(e0, e1) = boost::out_edges(i0->second, *this); e0 != e1; ++e0) {
			const Edge& e((*this)[*e0]);
			if (e.get_ident() != aname)
				continue;
			const Vertex& v1((*this)[boost::target(*e0, *this)]);
			if (v1.get_ident() != ename)
				continue;
			u = *e0;
			return true;
		}
	}
	return false;
}

void Engine::DbThread::AirwayGraphResult::Graph::exclude_levels(edge_descriptor e, int16_t fromlevel, int16_t tolevel)
{
	if (fromlevel > tolevel)
		return;
	Edge& ee((*this)[e]);
	if (tolevel < ee.get_base_level() ||
	    fromlevel > ee.get_top_level())
		return;
	if (fromlevel < ee.get_base_level())
		ee.set_base_level(tolevel + 1);
	if (tolevel > ee.get_top_level())
		ee.set_top_level(fromlevel - 1);
	if (ee.get_base_level() > ee.get_top_level())
		boost::remove_edge(e, *this);
}

void Engine::DbThread::AirwayGraphResult::Graph::shortest_paths(vertex_descriptor u, std::vector<vertex_descriptor>& predecessors)
{
	predecessors.clear();
	predecessors.resize(boost::num_vertices(*this), 0);
	dijkstra_shortest_paths(*this, u, boost::weight_map(boost::get(&Edge::m_dist, *this)).
				predecessor_map(&predecessors[0]));
}

void Engine::DbThread::AirwayGraphResult::Graph::shortest_paths(vertex_descriptor u, std::vector<vertex_descriptor>& predecessors, std::vector<double>& distances)
{
	predecessors.clear();
	distances.clear();
	predecessors.resize(boost::num_vertices(*this), 0);
	distances.resize(boost::num_vertices(*this), 0.0);
	dijkstra_shortest_paths(*this, u, boost::weight_map(boost::get(&Edge::m_dist, *this)).
				predecessor_map(&predecessors[0]).
				distance_map(&distances[0]));
}

void Engine::DbThread::AirwayGraphResult::Graph::shortest_paths(vertex_descriptor u, const Glib::ustring& awyname, std::vector<vertex_descriptor>& predecessors)
{
	predecessors.clear();
	predecessors.resize(boost::num_vertices(*this), 0);
	AwyEdgeFilter filter(*this, awyname);
	typedef boost::filtered_graph<Graph, AwyEdgeFilter> fg_t;
	fg_t fg(*this, filter);
	dijkstra_shortest_paths(fg, u, boost::weight_map(boost::get(&Edge::m_dist, *this)).
				predecessor_map(&predecessors[0]));
}

void Engine::DbThread::AirwayGraphResult::Graph::shortest_paths(vertex_descriptor u, const Glib::ustring& awyname, std::vector<vertex_descriptor>& predecessors, std::vector<double>& distances)
{
	predecessors.clear();
	distances.clear();
	predecessors.resize(boost::num_vertices(*this), 0);
	distances.resize(boost::num_vertices(*this), 0.0);
	AwyEdgeFilter filter(*this, awyname);
	typedef boost::filtered_graph<Graph, AwyEdgeFilter> fg_t;
	fg_t fg(*this, filter);
	dijkstra_shortest_paths(fg, u, boost::weight_map(boost::get(&Edge::m_dist, *this)).
				predecessor_map(&predecessors[0]).
				distance_map(&distances[0]));
}

void Engine::DbThread::AirwayGraphResult::Graph::shortest_paths_metric(vertex_descriptor u, std::vector<vertex_descriptor>& predecessors)
{
	predecessors.clear();
	predecessors.resize(boost::num_vertices(*this), 0);
	dijkstra_shortest_paths(*this, u, boost::weight_map(boost::get(&Edge::m_metric, *this)).
				predecessor_map(&predecessors[0]));
}

void Engine::DbThread::AirwayGraphResult::Graph::shortest_paths_metric(vertex_descriptor u, std::vector<vertex_descriptor>& predecessors, std::vector<double>& distances)
{
	predecessors.clear();
	distances.clear();
	predecessors.resize(boost::num_vertices(*this), 0);
	distances.resize(boost::num_vertices(*this), 0.0);
	dijkstra_shortest_paths(*this, u, boost::weight_map(boost::get(&Edge::m_metric, *this)).
				predecessor_map(&predecessors[0]).
				distance_map(&distances[0]));
}

void Engine::DbThread::AirwayGraphResult::Graph::shortest_paths_metric(vertex_descriptor u, const Glib::ustring& awyname, std::vector<vertex_descriptor>& predecessors)
{
	predecessors.clear();
	predecessors.resize(boost::num_vertices(*this), 0);
	AwyEdgeFilter filter(*this, awyname);
	typedef boost::filtered_graph<Graph, AwyEdgeFilter> fg_t;
	fg_t fg(*this, filter);
	dijkstra_shortest_paths(fg, u, boost::weight_map(boost::get(&Edge::m_metric, *this)).
				predecessor_map(&predecessors[0]));
}

void Engine::DbThread::AirwayGraphResult::Graph::shortest_paths_metric(vertex_descriptor u, const Glib::ustring& awyname, std::vector<vertex_descriptor>& predecessors, std::vector<double>& distances)
{
	predecessors.clear();
	distances.clear();
	predecessors.resize(boost::num_vertices(*this), 0);
	distances.resize(boost::num_vertices(*this), 0.0);
	AwyEdgeFilter filter(*this, awyname);
	typedef boost::filtered_graph<Graph, AwyEdgeFilter> fg_t;
	fg_t fg(*this, filter);
	dijkstra_shortest_paths(fg, u, boost::weight_map(boost::get(&Edge::m_metric, *this)).
				predecessor_map(&predecessors[0]).
				distance_map(&distances[0]));
}

unsigned int Engine::DbThread::AirwayGraphResult::Graph::add_dct_edges(vertex_descriptor u, double maxdist, bool efrom, bool eto, Vertex::typemask_t tmask, int16_t baselevel, int16_t toplevel,
								       bool skip_numeric_int)
{
	if (!efrom && !eto)
		return 0;
	if (maxdist < 0)
		return 0;
	if (!tmask)
		return 0;
	unsigned int count(0);
	typedef std::map<vertex_descriptor,double> vertexmap_t;
	vertexmap_t vset;
	const Vertex& uu((*this)[u]);
	{
		vertex_iterator v0, v1;
		for (boost::tie(v0, v1) = boost::vertices(*this); v0 != v1; ++v0) {
			vertex_descriptor v(*v0);
			if (u == v)
				continue;
			const Vertex& vv((*this)[v]);
			if (!vv.is_match(tmask))
				continue;
			if (skip_numeric_int && vv.is_match(Vertex::typemask_intersection) && vv.is_ident_numeric())
				continue;
			double dist(uu.get_coord().spheric_distance_nmi_dbl(vv.get_coord()));
			if (dist > maxdist)
				continue;
			vset.insert(std::make_pair(v, dist));
		}
	}
	if (efrom) {
		vertexmap_t vset1(vset);
		Graph::out_edge_iterator e0, e1;
		for (boost::tie(e0, e1) = boost::out_edges(u, *this); e0 != e1; ++e0) {
			vertexmap_t::iterator i(vset1.find(boost::target(*e0, *this)));
			if (i != vset1.end())
				vset1.erase(i);
		}
		for (vertexmap_t::const_iterator i(vset1.begin()), e(vset1.end()); i != e; ++i, ++count) {
			boost::add_edge(u, i->first, Edge("", baselevel, toplevel, AirwaysDb::Airway::airway_invalid, i->second), *this);
			if (!false)
				continue;
			const Vertex& uu((*this)[u]);
			const Vertex& vv((*this)[i->first]);
			std::cerr << "dct edge: " << uu.get_ident() << "->" << vv.get_ident() << " dist " << i->second
				  << " alt " << baselevel << "..." << toplevel << std::endl;
		}
	}
	if (eto) {
		Graph::in_edge_iterator e0, e1;
		for (boost::tie(e0, e1) = boost::in_edges(u, *this); e0 != e1; ++e0) {
			vertexmap_t::iterator i(vset.find(boost::source(*e0, *this)));
			if (i != vset.end())
				vset.erase(i);
		}
		for (vertexmap_t::const_iterator i(vset.begin()), e(vset.end()); i != e; ++i, ++count) {
			boost::add_edge(i->first, u, Edge("", baselevel, toplevel, AirwaysDb::Airway::airway_invalid, i->second), *this);
			if (!false)
				continue;
			const Vertex& uu((*this)[i->first]);
			const Vertex& vv((*this)[u]);
			std::cerr << "dct edge: " << uu.get_ident() << "->" << vv.get_ident() << " dist " << i->second
				  << " alt " << baselevel << "..." << toplevel << std::endl;
		}
	}
	return count;
}

unsigned int Engine::DbThread::AirwayGraphResult::Graph::add_dct_edges(double maxdist, Vertex::typemask_t tmask, int16_t baselevel, int16_t toplevel,
								       bool skip_numeric_int)
{
	if (maxdist < 0)
		return 0;
	if (!tmask)
		return 0;
	unsigned int count(0);
	typedef std::set<vertex_descriptor> vertexset_t;
	vertexset_t vset;
	{
		vertex_iterator v0, v1;
		for (boost::tie(v0, v1) = boost::vertices(*this); v0 != v1; ++v0) {
			vertex_descriptor v(*v0);
			const Vertex& vv((*this)[v]);
			if (!vv.is_match(tmask))
				continue;
			if (skip_numeric_int && vv.is_match(Vertex::typemask_intersection) && vv.is_ident_numeric())
				continue;
			vset.insert(v);
		}
	}
	for (vertexset_t::const_iterator i(vset.begin()), e(vset.end()); i != e; ++i)
		count += add_dct_edges(*i, maxdist, true, false, tmask, baselevel, toplevel, skip_numeric_int);
	return count;
}

void Engine::DbThread::AirwayGraphResult::Graph::delete_disconnected_vertices(void)
{
	// for speed, construct a second graph, then swap
	typedef std::map<vertex_descriptor, vertex_descriptor> vertexmap_t;
	vertexmap_t vmap;
	Graph g;
	{
		vertex_iterator v0, v1;
		for (boost::tie(v0, v1) = boost::vertices(*this); v0 != v1; ++v0) {
			if (boost::out_degree(*v0, *this) <= 0 && boost::in_degree(*v0, *this) <= 0) {
				if (false) {
					const Vertex& vv((*this)[*v0]);
					std::cerr << "dropping unconnected vertex: " << vv.get_ident() << std::endl;
				}
				continue;
			}
			const Vertex& vv((*this)[*v0]);
			vertex_descriptor u(boost::add_vertex(vv, g));
			vmap.insert(std::make_pair(*v0, u));
		}
	}
	{
		Graph::edge_iterator e0, e1;
		for (boost::tie(e0, e1) = boost::edges(*this); e0 != e1; ++e0) {
			const Edge& ee((*this)[*e0]);
			vertexmap_t::const_iterator is(vmap.find(boost::source(*e0, *this)));
			vertexmap_t::const_iterator it(vmap.find(boost::target(*e0, *this)));
			g_assert(is != vmap.end());
			g_assert(it != vmap.end());
			//std::pair<edge_descriptor, bool> ins(boost::add_edge(is->second, it->second, ee, g));
			std::pair<edge_descriptor, bool> ins(boost::add_edge(is->second, it->second, g));
			g_assert(ins.second);
			Edge& ee1(g[ins.first]);
			ee1 = ee;
		}
	}
	g.m_intersectionmap.clear();
	{
		vertex_iterator v0, v1;
		for (boost::tie(v0, v1) = boost::vertices(g); v0 != v1; ++v0) {
			const Vertex& vv(g[*v0]);
			g.m_intersectionmap.insert(std::make_pair(vv.get_ident().uppercase(), *v0));
		}
	}
	this->swap(g);
}

void Engine::DbThread::AirwayGraphResult::Graph::clear_objects(void)
{
	vertex_iterator v0, v1;
	for (boost::tie(v0, v1) = boost::vertices(*this); v0 != v1; ++v0) {
		Vertex& vv((*this)[*v0]);
		vv.clear_object();
	}
}

#endif /* HAVE_BOOST */
