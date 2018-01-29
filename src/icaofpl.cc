/***************************************************************************
 *   Copyright (C) 2012, 2013, 2014, 2015, 2017 by Thomas Sailer           *
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

#include <limits>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <set>
#include <time.h>
#include "icaofpl.h"
#include "wmm.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/connected_components.hpp>

class IcaoFlightPlan::FindCoord::SortAirportNamelen {
public:
	bool operator()(const AirportsDb::element_t& a, const AirportsDb::element_t& b) const { return a.get_name().size() < b.get_name().size(); }
};

class IcaoFlightPlan::FindCoord::SortNavaidNamelen {
public:
	bool operator()(const NavaidsDb::element_t& a, const NavaidsDb::element_t& b) const { return a.get_name().size() < b.get_name().size(); }
};

class IcaoFlightPlan::FindCoord::SortMapelementNamelen {
public:
	bool operator()(const MapelementsDb::element_t& a, const MapelementsDb::element_t& b) const { return a.get_name().size() < b.get_name().size(); }
};

class IcaoFlightPlan::FindCoord::SortAirportDist {
public:
	SortAirportDist(const Point& pt) : m_point(pt) {}
	bool operator()(const AirportsDb::element_t& a, const AirportsDb::element_t& b) const { return m_point.simple_distance_rel(a.get_coord()) < m_point.simple_distance_rel(b.get_coord()); }

protected:
	Point m_point;
};

class IcaoFlightPlan::FindCoord::SortNavaidDist {
public:
	SortNavaidDist(const Point& pt) : m_point(pt) {}
	bool operator()(const NavaidsDb::element_t& a, const NavaidsDb::element_t& b) const { return m_point.simple_distance_rel(a.get_coord()) < m_point.simple_distance_rel(b.get_coord()); }

protected:
	Point m_point;
};

class IcaoFlightPlan::FindCoord::SortWaypointDist {
public:
	SortWaypointDist(const Point& pt) : m_point(pt) {}
	bool operator()(const WaypointsDb::element_t& a, const WaypointsDb::element_t& b) const { return m_point.simple_distance_rel(a.get_coord()) < m_point.simple_distance_rel(b.get_coord()); }

protected:
	Point m_point;
};

class IcaoFlightPlan::FindCoord::SortMapelementDist {
public:
	SortMapelementDist(const Point& pt) : m_point(pt) {}
	bool operator()(const MapelementsDb::element_t& a, const MapelementsDb::element_t& b) const { return m_point.simple_distance_rel(a.get_coord()) < m_point.simple_distance_rel(b.get_coord()); }

protected:
	Point m_point;
};

class IcaoFlightPlan::FindCoord::SortAirwayDist {
public:
	SortAirwayDist(const Point& pt) : m_point(pt) {}
	bool operator()(const AirwaysDb::element_t& a, const AirwaysDb::element_t& b) const {
		return std::min(m_point.simple_distance_rel(a.get_begin_coord()),
				m_point.simple_distance_rel(a.get_end_coord())) <
			std::min(m_point.simple_distance_rel(b.get_begin_coord()),
				 m_point.simple_distance_rel(b.get_end_coord()));
	}

protected:
	Point m_point;
};

const unsigned int IcaoFlightPlan::FindCoord::flag_airport;
const unsigned int IcaoFlightPlan::FindCoord::flag_navaid;
const unsigned int IcaoFlightPlan::FindCoord::flag_waypoint;
const unsigned int IcaoFlightPlan::FindCoord::flag_mapelement;
const unsigned int IcaoFlightPlan::FindCoord::flag_airway;
const unsigned int IcaoFlightPlan::FindCoord::flag_coord;
const unsigned int IcaoFlightPlan::FindCoord::flag_subtables;
const std::string IcaoFlightPlan::FindCoord::empty;

IcaoFlightPlan::FindCoord::FindCoord(Engine& engine)
	: m_engine(engine), m_coordresult(Point::invalid)
{
}

void IcaoFlightPlan::FindCoord::async_cancel(void)
{
        Engine::AirportResult::cancel(m_airportquery);
        Engine::AirportResult::cancel(m_airportquery1);
        Engine::NavaidResult::cancel(m_navaidquery);
        Engine::NavaidResult::cancel(m_navaidquery1);
        Engine::WaypointResult::cancel(m_waypointquery);
        Engine::MapelementResult::cancel(m_mapelementquery);
	Engine::MapelementResult::cancel(m_airwayelquery);
	Engine::AirwayGraphResult::cancel(m_airwayquery);
	Engine::AreaGraphResult::cancel(m_areaquery);
}

void IcaoFlightPlan::FindCoord::async_clear(void)
{
	m_airportquery = Glib::RefPtr<Engine::AirportResult>();
	m_airportquery1 = Glib::RefPtr<Engine::AirportResult>();
	m_navaidquery = Glib::RefPtr<Engine::NavaidResult>();
	m_navaidquery1 = Glib::RefPtr<Engine::NavaidResult>();
	m_waypointquery = Glib::RefPtr<Engine::WaypointResult>();
	m_mapelementquery = Glib::RefPtr<Engine::MapelementResult>();
	m_airwayelquery = Glib::RefPtr<Engine::AirwayResult>();
	m_airwayquery = Glib::RefPtr<Engine::AirwayGraphResult>();
	m_areaquery = Glib::RefPtr<Engine::AreaGraphResult>();
	m_airports.clear();
	m_navaids.clear();
	m_waypoints.clear();
	m_mapelements.clear();
	m_airwaygraph.clear();
}

void IcaoFlightPlan::FindCoord::async_connectdone(void)
{
	if (m_airportquery)
		m_airportquery->connect(sigc::mem_fun(*this, &FindCoord::async_done));
	if (m_airportquery1)
		m_airportquery1->connect(sigc::mem_fun(*this, &FindCoord::async_done));
	if (m_navaidquery)
		m_navaidquery->connect(sigc::mem_fun(*this, &FindCoord::async_done));
	if (m_navaidquery1)
		m_navaidquery1->connect(sigc::mem_fun(*this, &FindCoord::async_done));
	if (m_waypointquery)
		m_waypointquery->connect(sigc::mem_fun(*this, &FindCoord::async_done));
	if (m_mapelementquery)
		m_mapelementquery->connect(sigc::mem_fun(*this, &FindCoord::async_done));
	if (m_airwayelquery)
		m_airwayelquery->connect(sigc::mem_fun(*this, &FindCoord::async_done));
	if (m_airwayquery)
		m_airwayquery->connect(sigc::mem_fun(*this, &FindCoord::async_done));
	if (m_areaquery)
		m_areaquery->connect(sigc::mem_fun(*this, &FindCoord::async_done));
}

void IcaoFlightPlan::FindCoord::async_done(void)
{
        Glib::Mutex::Lock lock(m_mutex);
        m_cond.broadcast();
}

unsigned int IcaoFlightPlan::FindCoord::async_waitdone(void)
{
	unsigned int flags(0);
        Glib::Mutex::Lock lock(m_mutex);
        for (;;) {
		if ((m_airportquery || m_airportquery1) && (!m_airportquery || m_airportquery->is_done()) && (!m_airportquery1 || m_airportquery1->is_done())) {
			m_airports.clear();
			if (m_airportquery && !m_airportquery->is_error()) {
				m_airports = m_airportquery->get_result();
			} else if (m_airportquery1 && !m_airportquery1->is_error()) {
				m_airports = m_airportquery1->get_result();
				flags |= flag_airport;
			}
			m_airportquery = Glib::RefPtr<Engine::AirportResult>();
			m_airportquery1 = Glib::RefPtr<Engine::AirportResult>();
		}
		if ((m_navaidquery || m_navaidquery1) && (!m_navaidquery || m_navaidquery->is_done()) && (!m_navaidquery1 || m_navaidquery1->is_done())) {
			m_navaids.clear();
			if (m_navaidquery && !m_navaidquery->is_error()) {
				m_navaids = m_navaidquery->get_result();
			} else if (m_navaidquery1 && !m_navaidquery1->is_error()) {
				m_navaids = m_navaidquery1->get_result();
				flags |= flag_navaid;
			}
			m_navaidquery = Glib::RefPtr<Engine::NavaidResult>();
			m_navaidquery1 = Glib::RefPtr<Engine::NavaidResult>();
		}
		if (m_waypointquery && m_waypointquery->is_done()) {
			m_waypoints.clear();
			if (!m_waypointquery->is_error())
				m_waypoints = m_waypointquery->get_result();
			m_waypointquery = Glib::RefPtr<Engine::WaypointResult>();
		}
		if (m_mapelementquery && m_mapelementquery->is_done()) {
			m_mapelements.clear();
			if (!m_mapelementquery->is_error())
				m_mapelements = m_mapelementquery->get_result();
			m_mapelementquery = Glib::RefPtr<Engine::MapelementResult>();
		}
		if (m_airwayelquery && m_airwayelquery->is_done()) {
			m_airways.clear();
			if (!m_airwayelquery->is_error())
				m_airways = m_airwayelquery->get_result();
			m_airwayelquery = Glib::RefPtr<Engine::AirwayResult>();
		}
		if (m_airwayquery && m_airwayquery->is_done()) {
			m_airwaygraph.clear();
			if (!m_airwayquery->is_error())
				m_airwaygraph = m_airwayquery->get_result();
			m_airwayquery = Glib::RefPtr<Engine::AirwayGraphResult>();
		}
		if (m_areaquery && m_areaquery->is_done()) {
			m_airwaygraph.clear();
			if (!m_areaquery->is_error())
				m_airwaygraph = m_areaquery->get_result();
			m_areaquery = Glib::RefPtr<Engine::AreaGraphResult>();
		}
		if (!(m_airportquery || m_airportquery1 || m_navaidquery || m_navaidquery1 || m_waypointquery || m_mapelementquery || m_airwayelquery || m_airwayquery || m_areaquery))
			break;
		m_cond.wait(m_mutex);
	}
	return flags;
}

void IcaoFlightPlan::FindCoord::sort_dist(const Point& pt)
{
	{
		SortAirportDist s(pt);
		sort(m_airports.begin(), m_airports.end(), s);
	}
	{
		SortNavaidDist s(pt);
		sort(m_navaids.begin(), m_navaids.end(), s);
	}
	{
		SortWaypointDist s(pt);
		sort(m_waypoints.begin(), m_waypoints.end(), s);
	}
	{
		SortMapelementDist s(pt);
		sort(m_mapelements.begin(), m_mapelements.end(), s);
	}
	{
		SortAirwayDist s(pt);
		sort(m_airways.begin(), m_airways.end(), s);
	}
}

void IcaoFlightPlan::FindCoord::retain_first(void)
{
	if (!m_airports.empty())
		m_airports.resize(1);
	if (!m_navaids.empty())
		m_navaids.resize(1);
	if (!m_waypoints.empty())
		m_waypoints.resize(1);
	if (!m_mapelements.empty())
		m_mapelements.resize(1);
	if (!m_airways.empty())
		m_airways.resize(1);
}

void IcaoFlightPlan::FindCoord::cut_dist(const Point& pt, float maxdist_km)
{
	if (!m_airports.empty() && pt.spheric_distance_km(m_airports.front().get_coord()) > maxdist_km)
		m_airports.clear();
	if (!m_navaids.empty() && pt.spheric_distance_km(m_navaids.front().get_coord()) > maxdist_km)
		m_navaids.clear();
	if (!m_waypoints.empty() && pt.spheric_distance_km(m_waypoints.front().get_coord()) > maxdist_km)
		m_waypoints.clear();
	if (!m_mapelements.empty() && pt.spheric_distance_km(m_mapelements.front().get_coord()) > maxdist_km)
		m_mapelements.clear();
	if (!m_airways.empty() && std::min(pt.spheric_distance_km(m_airways.front().get_begin_coord()),
					   pt.spheric_distance_km(m_airways.front().get_end_coord())) > maxdist_km)
		m_airways.clear();
	if (!m_coordresult.is_invalid() && pt.spheric_distance_km(m_coordresult) > maxdist_km)
		m_coordresult = Point::invalid;
}

void IcaoFlightPlan::FindCoord::retain_shortest_dist(const Point& pt)
{
	unsigned int what(0);
	uint64_t dist(std::numeric_limits<uint64_t>::max());
	if (!m_coordresult.is_invalid()) {
		uint64_t d(pt.simple_distance_rel(m_coordresult));
		if (d <= dist) {
			dist = d;
			what = 4;
		}
	}
	if (!m_mapelements.empty()) {
		uint64_t d(pt.simple_distance_rel(m_mapelements.front().get_coord()));
		if (d <= dist) {
			dist = d;
			what = 3;
		}
	}
	if (!m_waypoints.empty()) {
		uint64_t d(pt.simple_distance_rel(m_waypoints.front().get_coord()));
		if (d <= dist) {
			dist = d;
			what = 2;
		}
	}
	if (!m_navaids.empty()) {
		uint64_t d(pt.simple_distance_rel(m_navaids.front().get_coord()));
		if (d <= dist) {
			dist = d;
			what = 1;
		}
	}
	if (!m_airports.empty()) {
		uint64_t d(pt.simple_distance_rel(m_airports.front().get_coord()));
		if (d <= dist) {
			dist = d;
			what = 0;
		}
	}
	if (what != 0)
		m_airports.clear();
	if (what != 1)
		m_navaids.clear();
	if (what != 2)
		m_waypoints.clear();
	if (what != 3)
		m_mapelements.clear();
	if (what != 4)
		m_coordresult = Point::invalid;
}

void IcaoFlightPlan::FindCoord::find_by_name(const std::string& icao, const std::string& name, unsigned int flags)
{
	m_coordresult = Point::invalid;
	async_cancel();
	async_clear();
	if (!icao.empty()) {
		if (flags & flag_airport)
			m_airportquery = m_engine.async_airport_find_by_icao(icao, ~0, 0, AirportsDb::comp_exact,
									     (flags & flag_subtables) ? AirportsDb::element_t::subtables_all : AirportsDb::element_t::subtables_vfrroutes);
		if (flags & flag_navaid)
			m_navaidquery = m_engine.async_navaid_find_by_icao(icao, ~0, 0, NavaidsDb::comp_exact,
									   (flags & flag_subtables) ? NavaidsDb::element_t::subtables_all : NavaidsDb::element_t::subtables_none);
	}
	if (!name.empty()) {
		if (flags & flag_airport)
			m_airportquery1 = m_engine.async_airport_find_by_name(name, ~0, 0, AirportsDb::comp_contains,
									      (flags & flag_subtables) ? AirportsDb::element_t::subtables_all : AirportsDb::element_t::subtables_vfrroutes);
		if (flags & flag_navaid)
			m_navaidquery1 = m_engine.async_navaid_find_by_name(name, ~0, 0, NavaidsDb::comp_contains,
									    (flags & flag_subtables) ? NavaidsDb::element_t::subtables_all : NavaidsDb::element_t::subtables_none);
		if (flags & flag_waypoint)
			m_waypointquery = m_engine.async_waypoint_find_by_name(name, ~0, 0, WaypointsDb::comp_exact,
									       (flags & flag_subtables) ? WaypointsDb::element_t::subtables_all : WaypointsDb::element_t::subtables_none);
		if (flags & flag_mapelement)
			m_mapelementquery = m_engine.async_mapelement_find_by_name(name, ~0, 0, MapelementsDb::comp_contains,
										   (flags & flag_subtables) ? MapelementsDb::element_t::subtables_all : MapelementsDb::element_t::subtables_none);
		if (flags & flag_airway)
			m_airwayelquery = m_engine.async_airway_find_by_name(name, ~0, 0, AirwaysDb::comp_exact,
									     (flags & flag_subtables) ? AirwaysDb::element_t::subtables_all : AirwaysDb::element_t::subtables_none);
		if (flags & flag_coord) {
			unsigned int r(m_coordresult.set_str(name));
			if ((Point::setstr_lat | Point::setstr_lon) & ~r)
				m_coordresult.set_invalid();
		}
	}
	async_connectdone();
	{
		unsigned int flags(async_waitdone());
		// discard worse matching airports
		if (flags & flag_airport) {
			std::sort(m_airports.begin(), m_airports.end(), SortAirportNamelen());
			std::string::size_type namelen(0);
			if (!m_airports.empty())
				namelen = m_airports.front().get_name().size();
			for (Engine::AirportResult::elementvector_t::iterator i(m_airports.begin()), e(m_airports.end()); i != e; ++i) {
				if (i->get_name().size() <= namelen)
					continue;
				m_airports.erase(i, e);
				break;
			}
		}
		if (flags & flag_navaid) {
			std::sort(m_navaids.begin(), m_navaids.end(), SortNavaidNamelen());
			std::string::size_type namelen(0);
			if (!m_navaids.empty())
				namelen = m_navaids.front().get_name().size();
			for (Engine::NavaidResult::elementvector_t::iterator i(m_navaids.begin()), e(m_navaids.end()); i != e; ++i) {
				if (i->get_name().size() <= namelen)
					continue;
				m_navaids.erase(i, e);
				break;
			}
		}
	}
	{
		std::sort(m_mapelements.begin(), m_mapelements.end(), SortMapelementNamelen());
		std::string::size_type namelen(0);
		if (!m_mapelements.empty())
			namelen = m_mapelements.front().get_name().size();
		for (Engine::MapelementResult::elementvector_t::iterator i(m_mapelements.begin()), e(m_mapelements.end()); i != e; ++i) {
			if (i->get_name().size() <= namelen)
				continue;
			m_mapelements.erase(i, e);
			break;
		}
	}
}

void IcaoFlightPlan::FindCoord::find_by_ident(const std::string& ident, const std::string& name, unsigned int flags)
{
	m_coordresult = Point::invalid;
	async_cancel();
	async_clear();
	if (!ident.empty()) {
		if (flags & flag_airport)
			m_airportquery = m_engine.async_airport_find_by_icao(ident, ~0, 0, AirportsDb::comp_exact,
									     (flags & flag_subtables) ? AirportsDb::element_t::subtables_all : AirportsDb::element_t::subtables_none);
		if (flags & flag_navaid)
			m_navaidquery = m_engine.async_navaid_find_by_icao(ident, ~0, 0, NavaidsDb::comp_exact,
									   (flags & flag_subtables) ? NavaidsDb::element_t::subtables_all : NavaidsDb::element_t::subtables_none);
		if (flags & flag_waypoint)
			m_waypointquery = m_engine.async_waypoint_find_by_name(ident, ~0, 0, WaypointsDb::comp_exact,
									       (flags & flag_subtables) ? WaypointsDb::element_t::subtables_all : WaypointsDb::element_t::subtables_none);
		if (flags & flag_mapelement)
			m_mapelementquery = m_engine.async_mapelement_find_by_name(ident, ~0, 0, MapelementsDb::comp_contains,
										   (flags & flag_subtables) ? MapelementsDb::element_t::subtables_all : MapelementsDb::element_t::subtables_none);
		if (flags & flag_airway)
			m_airwayelquery = m_engine.async_airway_find_by_name(ident, ~0, 0, AirwaysDb::comp_exact,
									     (flags & flag_subtables) ? AirwaysDb::element_t::subtables_all : AirwaysDb::element_t::subtables_none);
		if (flags & flag_coord) {
			unsigned int r(m_coordresult.set_str(name));
			if ((Point::setstr_lat | Point::setstr_lon) & ~r)
				m_coordresult.set_invalid();
		}
	}
	if (!name.empty()) {
		if (flags & flag_airport)
			m_airportquery1 = m_engine.async_airport_find_by_name(name, ~0, 0, AirportsDb::comp_contains,
									      (flags & flag_subtables) ? AirportsDb::element_t::subtables_all : AirportsDb::element_t::subtables_none);
		if (flags & flag_navaid)
			m_navaidquery1 = m_engine.async_navaid_find_by_name(name, ~0, 0, NavaidsDb::comp_contains,
									    (flags & flag_subtables) ? NavaidsDb::element_t::subtables_all : AirportsDb::element_t::subtables_none);
	}
	async_connectdone();
	{
		unsigned int flags(async_waitdone());
		// discard worse matching airports
		if (flags & flag_airport) {
			std::sort(m_airports.begin(), m_airports.end(), SortAirportNamelen());
			std::string::size_type namelen(0);
			if (!m_airports.empty())
				namelen = m_airports.front().get_name().size();
			for (Engine::AirportResult::elementvector_t::iterator i(m_airports.begin()), e(m_airports.end()); i != e; ++i) {
				if (i->get_name().size() <= namelen)
					continue;
				m_airports.erase(i, e);
				break;
			}
		}
		if (flags & flag_navaid) {
			std::sort(m_navaids.begin(), m_navaids.end(), SortNavaidNamelen());
			std::string::size_type namelen(0);
			if (!m_navaids.empty())
				namelen = m_navaids.front().get_name().size();
			for (Engine::NavaidResult::elementvector_t::iterator i(m_navaids.begin()), e(m_navaids.end()); i != e; ++i) {
				if (i->get_name().size() <= namelen)
					continue;
				m_navaids.erase(i, e);
				break;
			}
		}
	}
	{
		std::sort(m_mapelements.begin(), m_mapelements.end(), SortMapelementNamelen());
		std::string::size_type namelen(0);
		if (!m_mapelements.empty())
			namelen = m_mapelements.front().get_name().size();
		for (Engine::MapelementResult::elementvector_t::iterator i(m_mapelements.begin()), e(m_mapelements.end()); i != e; ++i) {
			if (i->get_name().size() <= namelen)
				continue;
			m_mapelements.erase(i, e);
			break;
		}
	}
}



bool IcaoFlightPlan::FindCoord::find(const std::string& icao, const std::string& name, unsigned int flags, const Point& pt)
{
	find_by_name(icao, name, flags);
	sort_dist(pt);
	retain_first();
	retain_shortest_dist(pt);
       	return !(m_airports.empty() && m_navaids.empty() && m_waypoints.empty() && m_mapelements.empty());
}

void IcaoFlightPlan::FindCoord::find_by_coord(const Point& pt, unsigned int flags, float maxdist_km)
{
	m_coordresult = Point::invalid;
	async_cancel();
	async_clear();
	Rect bbox(pt.simple_box_km(maxdist_km));
	if (flags & flag_airport)
		m_airportquery = m_engine.async_airport_find_bbox(bbox, ~0,
								  (flags & flag_subtables) ? AirportsDb::element_t::subtables_all : AirportsDb::element_t::subtables_vfrroutes);
	if (flags & flag_navaid)
		m_navaidquery = m_engine.async_navaid_find_bbox(bbox, ~0,
								(flags & flag_subtables) ? NavaidsDb::element_t::subtables_all : NavaidsDb::element_t::subtables_none);
	if (flags & flag_waypoint)
		m_waypointquery = m_engine.async_waypoint_find_bbox(bbox, ~0,
								    (flags & flag_subtables) ? WaypointsDb::element_t::subtables_all : WaypointsDb::element_t::subtables_none);
	if (flags & flag_mapelement)
		m_mapelementquery = m_engine.async_mapelement_find_bbox(bbox, ~0,
									(flags & flag_subtables) ? MapelementsDb::element_t::subtables_all : MapelementsDb::element_t::subtables_none);
	if (flags & flag_airway)
		m_airwayelquery = m_engine.async_airway_find_bbox(bbox, ~0,
								  (flags & flag_subtables) ? AirwaysDb::element_t::subtables_all : AirwaysDb::element_t::subtables_none);
	if (flags & flag_coord) {
		m_coordresult.set_lon_deg_dbl(Point::round<int,double>(pt.get_lon_deg_dbl() * 60.0) * (1.0 / 60.0));
		m_coordresult.set_lat_deg_dbl(Point::round<int,double>(pt.get_lat_deg_dbl() * 60.0) * (1.0 / 60.0));
	}
	async_connectdone();
	async_waitdone();
	sort_dist(pt);
}

bool IcaoFlightPlan::FindCoord::find(const Point& pt, unsigned int flags, float maxdist_km)
{
	find_by_coord(pt, flags, maxdist_km);
	retain_first();
	cut_dist(pt, maxdist_km);
	retain_shortest_dist(pt);
	return !(m_airports.empty() && m_navaids.empty() && m_waypoints.empty() && m_mapelements.empty());
}

bool IcaoFlightPlan::FindCoord::find_airway(const std::string& awy)
{
	m_coordresult = Point::invalid;
	async_cancel();
	async_clear();
	m_airwayquery = m_engine.async_airway_graph_find_by_name(awy);
	async_connectdone();
	async_waitdone();
	return !!boost::num_vertices(m_airwaygraph);
}

bool IcaoFlightPlan::FindCoord::find_area(const Rect& rect, Engine::AirwayGraphResult::Vertex::typemask_t tmask)
{
	m_coordresult = Point::invalid;
	async_cancel();
	async_clear();
	m_areaquery = m_engine.async_area_graph_find_bbox(rect, tmask);
	async_connectdone();
	async_waitdone();
	return !!boost::num_vertices(m_airwaygraph);
}

std::string IcaoFlightPlan::FindCoord::get_name(void) const
{
	if (!m_airports.empty())
		return m_airports.front().get_name();
	if (!m_navaids.empty())
		return m_navaids.front().get_name();
	if (!m_waypoints.empty())
		return m_waypoints.front().get_name();
	if (!m_mapelements.empty())
		return m_mapelements.front().get_name();
	return empty;
}

std::string IcaoFlightPlan::FindCoord::get_icao(void) const
{
	if (!m_airports.empty())
		return m_airports.front().get_icao();
	if (!m_navaids.empty())
		return m_navaids.front().get_icao();
	if (!m_waypoints.empty())
		return m_waypoints.front().get_icao();
	return empty;
}

Point IcaoFlightPlan::FindCoord::get_coord(void) const
{
	if (!m_airports.empty())
		return m_airports.front().get_coord();
	if (!m_navaids.empty())
		return m_navaids.front().get_coord();
	if (!m_waypoints.empty())
		return m_waypoints.front().get_coord();
	if (!m_mapelements.empty())
		return m_mapelements.front().get_coord();
	if (!m_coordresult.is_invalid())
		return m_coordresult;
	return Point();
}

bool IcaoFlightPlan::FindCoord::get_airport(AirportsDb::element_t& x) const
{
	if (m_airports.empty())
		return false;
	x = m_airports.front();
	return true;
}

bool IcaoFlightPlan::FindCoord::get_navaid(NavaidsDb::element_t& x) const
{
	if (m_navaids.empty())
		return false;
	x = m_navaids.front();
	return true;
}

bool IcaoFlightPlan::FindCoord::get_waypoint(WaypointsDb::element_t& x) const
{
	if (m_waypoints.empty())
		return false;
	x = m_waypoints.front();
	return true;
}

bool IcaoFlightPlan::FindCoord::get_mapelement(MapelementsDb::element_t& x) const
{
	if (m_mapelements.empty())
		return false;
	x = m_mapelements.front();
	return true;
}

bool IcaoFlightPlan::FindCoord::get_coord(Point& x) const
{
	if (m_coordresult.is_invalid())
		return false;
	x = m_coordresult;
	return true;
}

bool IcaoFlightPlan::FindCoord::get_airwaygraph(Engine::AirwayGraphResult::Graph& x) const
{
	x = m_airwaygraph;
	return !!boost::num_vertices(m_airwaygraph);
}

Engine::DbObject::ptr_t IcaoFlightPlan::FindCoord::get_object(void) const
{
	if (!m_airports.empty())
		return Engine::DbObject::create(m_airports.front());
	if (!m_navaids.empty())
		return Engine::DbObject::create(m_navaids.front());
	if (!m_waypoints.empty())
		return Engine::DbObject::create(m_waypoints.front());
	if (!m_mapelements.empty())
		return Engine::DbObject::create(m_mapelements.front());
	return Engine::DbObject::ptr_t();
}

IcaoFlightPlan::Waypoint::Waypoint(void)
	: m_time(0), m_alt(0), m_flags(0), m_frequency(0), m_pathcode(FPlanWaypoint::pathcode_none)
{
}

IcaoFlightPlan::Waypoint::Waypoint(const FPlanWaypoint& wpt)
	: m_icao(wpt.get_icao()), m_name(wpt.get_name()),
	  m_pathname(wpt.get_pathname()),
	  m_time(wpt.get_time_unix()), m_coord(wpt.get_coord()),
	  m_alt(wpt.get_altitude()), m_frequency(wpt.get_frequency()),
	  m_flags(wpt.get_flags()), m_pathcode(wpt.get_pathcode())
{
}

const std::string& IcaoFlightPlan::Waypoint::get_icao_or_name(void) const
{
	if (get_icao().empty())
		return get_name();
	return get_icao();
}

std::string IcaoFlightPlan::Waypoint::get_coordstr(void) const
{
	std::ostringstream oss;
	double c(get_lat() * Point::to_deg_dbl);
	char ch('N');
	if (c < 0) {
		c = -c;
		ch = 'S';
	}
	int ci(floor(c) + 0.01);
	c -= ci;
	c *= 60;
	oss << std::setw(2) << std::setfill('0') << ci;
	ci = floor(c + 0.5) + 0.01;
	oss << std::setw(2) << std::setfill('0') << ci << ch;
	c = get_lon() * Point::to_deg_dbl;
	ch = 'E';
	if (c < 0) {
		c = -c;
		ch = 'W';
	}
	ci = floor(c) + 0.01;
	c -= ci;
	c *= 60;
	oss << std::setw(3) << std::setfill('0') << ci;
	ci = floor(c + 0.5) + 0.01;
	oss << std::setw(2) << std::setfill('0') << ci << ch;
	return oss.str();
}

bool IcaoFlightPlan::Waypoint::enforce_pathcode_vfrifr(void)
{
	Glib::ustring pn(get_pathname());
	{
		Glib::ustring::size_type n(pn.size()), i(0);
		while (i < n && std::isspace(pn[i]))
			++i;
		while (n > i) {
			--n;
			if (std::isspace(pn[n]))
				continue;
			++n;
			break;
		}
		pn = pn.substr(i, n - i);
	}
	FPlanWaypoint::pathcode_t pc(get_pathcode());
	if (is_ifr()) {
		pn = pn.uppercase();
		switch (pc) {
		case FPlanWaypoint::pathcode_none:
			pc = FPlanWaypoint::pathcode_airway;
			// fall through

		case FPlanWaypoint::pathcode_airway:
			if (pn.empty() || pn == "DCT")
				pc = FPlanWaypoint::pathcode_directto;
			break;

		default:
			pn.clear();
			pc = FPlanWaypoint::pathcode_directto;
			break;
		}
	} else {
		switch (pc) {
		case FPlanWaypoint::pathcode_vfrdeparture:
		case FPlanWaypoint::pathcode_vfrarrival:
		case FPlanWaypoint::pathcode_vfrtransition:
			break;

		default:
			pn.clear();
			pc = FPlanWaypoint::pathcode_none;
			break;
		}
	}
	bool work(false);
	if (get_pathcode() != pc) {
		set_pathcode(pc);
		work = true;
	}
	if (get_pathname() != pn) {
		set_pathname(pn);
		work = true;
	}
	return work;
}

std::string IcaoFlightPlan::Waypoint::to_str(void) const
{
	std::ostringstream oss;
	oss << get_icao() << '/' << get_name() << ' ';
	if (get_pathcode() == FPlanWaypoint::pathcode_none) {
		oss << '-';
	} else {
		oss << FPlanWaypoint::get_pathcode_name(get_pathcode());
		switch (get_pathcode()) {
		case FPlanWaypoint::pathcode_airway:
		case FPlanWaypoint::pathcode_sid:
		case FPlanWaypoint::pathcode_star:
			oss << ' ' << get_pathname();
			break;

		default:
			break;
		}
	}
	if (!get_coord().is_invalid())
		oss << ' ' << get_coord().get_lat_str2() << ' ' << get_coord().get_lon_str2();
	oss << ' ' << (is_standard() ? 'F' : 'A') << std::setfill('0') << std::setw(3)
	    << ((get_altitude() + 50) / 100) << ' '
	    << (is_ifr() ? 'I' : 'V') << "FR "
	    << FPlanWaypoint::get_type_string((FPlanWaypoint::type_t)get_type(), (FPlanWaypoint::navaid_type_t)get_navaid());
	return oss.str();
}

FPlanWaypoint IcaoFlightPlan::Waypoint::get_fplwpt(void) const
{
	FPlanWaypoint fwpt;
	fwpt.set_icao(get_icao());
	fwpt.set_name(get_name());
	fwpt.set_coord(get_coord());
	fwpt.set_note(get_pathname());
	fwpt.set_type(get_type(), get_navaid());
	{
		Engine::DbObject::ptr_t o(get_dbobj());
		if (o) {
			o->set(fwpt);
		}
	}
	fwpt.set_time_unix(get_time());
	fwpt.set_truealt(get_altitude());
	fwpt.set_pathcode(get_pathcode());
	fwpt.set_pathname(get_pathname());
	fwpt.set_altitude(get_altitude());
	fwpt.set_flags(get_flags() & (FPlanWaypoint::altflag_standard | FPlanWaypoint::altflag_ifr));
	return fwpt;
}

IcaoFlightPlan::ParseWaypoint::ParseWaypoint(void)
	: Waypoint(), m_course(-1), m_dist(-1), m_typemask(Vertex::typemask_navaid | Vertex::typemask_intersection | Vertex::typemask_mapelement),
	  m_expanded(false)
{
}

IcaoFlightPlan::ParseWaypoint::ParseWaypoint(const FPlanWaypoint& wpt)
	: Waypoint(wpt), m_course(-1), m_dist(-1), m_typemask(Vertex::typemask_navaid | Vertex::typemask_intersection | Vertex::typemask_mapelement),
	  m_expanded(false)
{
}

void IcaoFlightPlan::ParseWaypoint::set(const Engine::DbObject::ptr_t obj, bool forcearptelev)
{
	if (!obj)
		return;
	FPlanWaypoint wpt;
	if (!obj->set(wpt))
		return;
	set_icao(wpt.get_icao());
	set_name(wpt.get_name());
	set_coord(wpt.get_coord());
	set_frequency(wpt.get_frequency());
	set_type(wpt.get_type(), wpt.get_navaid());
	set_dbobj(obj);
	if (!forcearptelev || get_type() != FPlanWaypoint::type_airport)
		return;
	set_altitude(wpt.get_altitude());
	frob_flags(0, FPlanWaypoint::altflag_standard);
}

void IcaoFlightPlan::ParseWaypoint::upcaseid(void)
{
	set_icao(upcase(get_icao()));
}

float IcaoFlightPlan::ParseWaypoint::process_speedalt(void)
{
	const Glib::ustring& icao(get_icao());
	Glib::ustring::size_type n(icao.find('/'));
	if (n == Glib::ustring::npos)
		return std::numeric_limits<float>::quiet_NaN();
	Glib::ustring::size_type i(n + 1);
	Glib::ustring::size_type nend(icao.size());
	char ch(icao[i]);
	float spd(1);
	if (ch == 'N' || ch == 'K' || ch == 'M') {
		bool mach(ch == 'M');
		if (i + 4 > nend || (!mach && i + 5 > nend) || !std::isdigit(icao[i + 1]) ||
		    !std::isdigit(icao[i + 2]) || !std::isdigit(icao[i + 3]) ||
		    (!mach && !std::isdigit(icao[i + 4]))) {
			std::string err("invalid speed: " + icao);
			set_icao(icao.substr(0, n));
			throw std::runtime_error(err);
		}
		int speed((icao[i + 1] - '0') * 100 + (icao[i + 2] - '0') * 10 + (icao[i + 3] - '0'));
		if (!mach)
			speed = speed * 10 + (icao[i + 4] - '0');
		i += mach ? 4 : 5;
		if (ch == 'K')
			spd = Point::km_to_nmi;
		else if (mach)
			spd = 330.0 * Point::mps_to_kts; // very approximate!
		spd *= speed;
	}
	ch = icao[i];
	if (ch == 'V') {
		if (i + 3 > nend || icao[i + 1] != 'F' ||  icao[i + 2] != 'R') {
			std::string err("invalid altitude: " + icao);
			set_icao(icao.substr(0, n));
			throw std::runtime_error(err);
		}
		i += 3;
		frob_flags(0, FPlanWaypoint::altflag_standard);
		set_altitude(std::numeric_limits<int32_t>::min());
	} else if (ch == 'F' || ch == 'A' || ch == 'S' || ch == 'M') {
		bool metric(ch == 'S' || ch == 'M');
		if (i + 4 > nend || (metric && i + 5 > nend) || !std::isdigit(icao[i + 1]) ||
		    !std::isdigit(icao[i + 2]) || !std::isdigit(icao[i + 3]) ||
		    (metric && !std::isdigit(icao[i + 4]))) {
			std::string err("invalid altitude: " + icao);
			set_icao(icao.substr(0, n));
			throw std::runtime_error(err);
		}
		int alt((icao[i + 1] - '0') * 100 + (icao[i + 2] - '0') * 10 + (icao[i + 3] - '0'));
		if (metric)
			alt = alt * 10 + (icao[i + 4] - '0');
		i += metric ? 5 : 4;
		if (metric)
			alt = Point::round<int,float>(alt * Point::m_to_ft);
		set_altitude(alt * 100);
		frob_flags((ch == 'F' || ch == 'S') ? FPlanWaypoint::altflag_standard : 0, FPlanWaypoint::altflag_standard);
	}
	if (i != nend) {
		std::string err("invalid altitude/speed: " + icao);
		set_icao(icao.substr(0, n));
		throw std::runtime_error(err);
	}
	set_icao(icao.substr(0, n));
	return spd;
}

void IcaoFlightPlan::ParseWaypoint::process_crsdist(void)
{
	const Glib::ustring& icao(get_icao());
	Glib::ustring::size_type i(icao.size());
	set_coursedist();
	if (i < 7 || !isdigit(icao[i - 6]) || !isdigit(icao[i - 5]) || !isdigit(icao[i - 4]) ||
	    !isdigit(icao[i - 3]) || !isdigit(icao[i - 2]) || !isdigit(icao[i - 1]))
		return;
	set_coursedist((icao[i - 6] - '0') * 100 + (icao[i - 5] - '0') * 10 + (icao[i - 4] - '0'),
		       (icao[i - 3] - '0') * 100 + (icao[i - 2] - '0') * 10 + (icao[i - 1] - '0'));
	set_icao(icao.substr(0, i-6));
}

Point IcaoFlightPlan::ParseWaypoint::parse_coord(const std::string& txt)
{
	Point ptinv;
	ptinv.set_invalid();
	std::string::size_type n(txt.size());
	if (n < 7)
		return ptinv;
	if (!std::isdigit(txt[0]) || !std::isdigit(txt[1]))
		return ptinv;
	Point pt;
	std::string::size_type i(2);
	{
		unsigned int deg((txt[0] - '0') *  10 + (txt[1] - '0')), min(0);
		if (std::isdigit(txt[2]) && std::isdigit(txt[3])) {
			min = (txt[2] - '0') *  10 + (txt[3] - '0');
			i = 4;
		}
		if (txt[i] == 'S')
			pt.set_lat_deg(-(deg * (1.0 / 60.0) * min));
		else if (txt[i] == 'N')
			pt.set_lat_deg(deg * (1.0 / 60.0) * min);
		else
			return ptinv;
		++i;
	}
	if (i + 4 > n)
		return ptinv;
	if (!std::isdigit(txt[i]) || !std::isdigit(txt[i + 1]) || !std::isdigit(txt[i + 2]))
		return ptinv;
	{
		unsigned int deg((txt[i] - '0') *  100 + (txt[i + 1] - '0') *  10 + (txt[i + 2] - '0')), min(0);
		i += 3;
		if (i + 3 <= n && std::isdigit(txt[i]) && std::isdigit(txt[i + 1])) {
			min = (txt[i] - '0') *  10 + (txt[i + 1] - '0');
			i += 2;
		}
		if (txt[i] == 'W')
			pt.set_lon_deg(-(deg * (1.0 / 60.0) * min));
		else if (txt[i] == 'E')
			pt.set_lon_deg(deg * (1.0 / 60.0) * min);
		else
			return ptinv;
		++i;
	}
	if (i != n)
		return ptinv;
	return pt;
}

bool IcaoFlightPlan::ParseWaypoint::process_coord(void)
{
	Point pt(parse_coord(get_icao()));
	if (pt.is_invalid())
		return false;
	set_coord(pt);
	set_type(FPlanWaypoint::type_user);
	return true;
}

bool IcaoFlightPlan::ParseWaypoint::process_airportcoord(void)
{
	typedef std::vector<std::string> tok_t;
	tok_t tok;
	{
		const Glib::ustring& name(get_name());
		Glib::ustring::const_iterator i(name.begin()), e(name.end());
	        for (;;) {
			while (i != e && std::isspace(*i))
				++i;
			if (i == e)
				break;
			Glib::ustring::const_iterator i2(i);
			++i2;
			while (i2 != e && !std::isspace(*i2))
				++i2;
			tok.push_back(std::string(i, i2));
			i = i2;
		}
	}
	bool ret(false);
	for (tok_t::iterator i(tok.begin()), e(tok.end()); i != e; ++i) {
		Point pt(parse_coord(*i));
		if (pt.is_invalid())
			continue;
		set_coord(pt);
		i = tok.erase(i);
		ret = true;
		break;
	}
	{
		bool subseq(false);
		std::string t;
		for (tok_t::const_iterator i(tok.begin()), e(tok.end()); i != e; ++i) {
			if (subseq)
				t += " ";
			subseq = true;
			t += *i;
		}
		set_name(t);
	}
	return ret;
}

std::string IcaoFlightPlan::ParseWaypoint::to_str(void) const
{
	std::ostringstream oss;
	oss << Waypoint::to_str();
	if (is_coursedist())
		oss << ' ' << std::setfill('0') << std::setw(3) << get_course()
		    << std::setfill('0') << std::setw(3) << get_dist();
	if (is_expanded())
		oss << " (E)";
	if (!empty()) {
		oss << '[';
		for (unsigned int i = 0, n = size(); i < n; ++i) {
			const Path& p(operator[](i));
			oss << ' ' << (unsigned int)p.get_vertex() << ' ' << p.get_dist();
			if (p.empty())
				continue;
			oss << " (";
			for (unsigned int j = 0, k = p.size(); j < k; ++j)
				oss << ' ' << (unsigned int)p[j];
			oss << " )";
		}
		oss << " ]";
       	}
	return oss.str();
}

const bool IcaoFlightPlan::ParseState::trace_dbload;

IcaoFlightPlan::ParseState::ParseState(Engine& engine)
	: m_engine(engine)
{
}

void IcaoFlightPlan::ParseState::process_speedalt(void)
{
	wpts_t::size_type n(m_wpts.size());
	//int32_t alt(5500);
	int32_t alt(FPlanWaypoint::invalid_altitude);
	uint16_t flags(0);
	if (n) {
		const ParseWaypoint& wpt(m_wpts[0]);
		alt = wpt.get_altitude();
		flags = wpt.get_flags() & (FPlanWaypoint::altflag_standard |
					   FPlanWaypoint::altflag_ifr);
	}
	for (wpts_t::size_type i(0); i < n; ) {
		ParseWaypoint& wpt(m_wpts[i]);
		wpt.upcaseid();
		wpt.set_altitude(alt);
		wpt.frob_flags(flags, FPlanWaypoint::altflag_standard | FPlanWaypoint::altflag_ifr);
		{
			bool ifr(wpt.get_icao() == "IFR");
			if (ifr || (wpt.get_icao() == "VFR")) {
				if (ifr)
					flags |= FPlanWaypoint::altflag_ifr;
				else
					flags &= ~FPlanWaypoint::altflag_ifr;
				if (i) {
					ParseWaypoint& wptp(m_wpts[i - 1]);
					wptp.frob_flags(flags, FPlanWaypoint::altflag_standard | FPlanWaypoint::altflag_ifr);
				}
				m_wpts.erase(m_wpts.begin() + i);
				n = m_wpts.size();
				continue;
			}
		}
		if ((flags & FPlanWaypoint::altflag_ifr) && i && wpt.get_icao() == "DCT") {
			if (i) {
				ParseWaypoint& wptp(m_wpts[i - 1]);
				wptp.set_pathcode(FPlanWaypoint::pathcode_directto);
			}
			m_wpts.erase(m_wpts.begin() + i);
			n = m_wpts.size();
			continue;
		}
		if ((flags & FPlanWaypoint::altflag_ifr) && i && FPlanWaypoint::is_stay(wpt.get_icao())) {
			if (i) {
				ParseWaypoint& wptp(m_wpts[i - 1]);
				wptp.set_pathcode(FPlanWaypoint::pathcode_airway);
				wptp.set_pathname(wpt.get_icao());
			}
			m_wpts.erase(m_wpts.begin() + i);
			n = m_wpts.size();
			continue;
		}
		try {
			float spd(wpt.process_speedalt());
			if (!std::isnan(spd)) {
				alt = wpt.get_altitude();
				flags = wpt.get_flags() & (FPlanWaypoint::altflag_standard |
							   FPlanWaypoint::altflag_ifr);
				m_cruisespeeds[alt] = spd;
			}
		} catch (const std::exception& e) {
			m_errors.push_back(e.what());
		}
		wpt.process_crsdist();
		{
			Point pt;
			pt.set_invalid();
			wpt.set_coord(pt);
		}
		wpt.set_type(FPlanWaypoint::type_undefined);
		wpt.process_coord();
		if (flags & FPlanWaypoint::altflag_ifr) {
			Vertex::typemask_t tm(Vertex::typemask_navaid | Vertex::typemask_intersection);
			if (!i || i + 1 == n)
				tm |= Vertex::typemask_airport;
			wpt.set_typemask(wpt.get_typemask() & tm);
		}
		if (wpt.get_typemask() & Vertex::typemask_airport)
			wpt.process_airportcoord();
		++i;
	}
}

void IcaoFlightPlan::ParseState::process_dblookup(void)
{
	FindCoord f(m_engine);
	wpts_t::size_type n(m_wpts.size());
	for (wpts_t::size_type i(0); i < n; ) {
		ParseWaypoint& wpt(m_wpts[i]);
		if ((wpt.get_typemask() & Vertex::typemask_airport) && !wpt.get_coord().is_invalid()) {
			AirportsDb::element_t arpt;
			if (f.find(wpt.get_coord(), FindCoord::flag_airport, 5) && f.get_airport(arpt) &&
			    arpt.is_valid() && arpt.get_coord().spheric_distance_nmi_dbl(wpt.get_coord()) <= 1) {
				Graph::vertex_descriptor u(m_graph.add(arpt));
				if (u < num_vertices(m_graph)) {
					wpt.add(ParseWaypoint::Path(u));
					++i;
					continue;
				}
			}
		}
		if (!(wpt.get_flags() & FPlanWaypoint::altflag_ifr) || !Vertex::is_ident_numeric(wpt.get_icao())) {
			unsigned int flg(wpt.get_typemask());
			if (AirportsDb::Airport::is_fpl_zzzz(wpt.get_icao()))
				flg &= ~FindCoord::flag_airport;
			f.find_by_ident(wpt.get_icao(), "", flg);
			bool found(false);
			{
				const Engine::AirportResult::elementvector_t& ev(f.get_airports());
				for (Engine::AirportResult::elementvector_t::const_iterator i(ev.begin()), e(ev.end()); i != e; ++i) {
					if (trace_dbload)
						std::cerr << "Ident " << wpt.get_icao() << ": Airport " << i->get_icao_name()
							  << ' ' << i->get_coord().get_lat_str2() << ' ' << i->get_coord().get_lon_str2()
							  << ' ' << (i->get_subtables() ? " subtables" : "") << std::endl;
					if (i->get_icao() != wpt.get_icao())
						continue;
					Graph::vertex_descriptor u(m_graph.add(*i));
					if (u >= num_vertices(m_graph))
						continue;
					found = true;
					wpt.add(ParseWaypoint::Path(u));
				}
			}
			{
				const Engine::NavaidResult::elementvector_t& ev(f.get_navaids());
				for (Engine::NavaidResult::elementvector_t::const_iterator i(ev.begin()), e(ev.end()); i != e; ++i) {
					if (trace_dbload)
						std::cerr << "Ident " << wpt.get_icao() << ": Navaid " << i->get_icao_name()
							  << ' ' << i->get_coord().get_lat_str2() << ' ' << i->get_coord().get_lon_str2()
							  << std::endl;
					if (i->get_icao() != wpt.get_icao())
						continue;
					Graph::vertex_descriptor u(m_graph.add(*i));
					if (u >= num_vertices(m_graph))
						continue;
					found = true;
					wpt.add(ParseWaypoint::Path(u));
				}
			}
			{
				const Engine::WaypointResult::elementvector_t& ev(f.get_waypoints());
				for (Engine::WaypointResult::elementvector_t::const_iterator i(ev.begin()), e(ev.end()); i != e; ++i) {
					if (trace_dbload)
						std::cerr << "Ident " << wpt.get_icao() << ": Intersection " << i->get_icao_name()
							  << ' ' << i->get_coord().get_lat_str2() << ' ' << i->get_coord().get_lon_str2()
							  << std::endl;
					if (i->get_name() != wpt.get_icao())
						continue;
					Graph::vertex_descriptor u(m_graph.add(*i));
					if (u >= num_vertices(m_graph))
						continue;
					found = true;
					wpt.add(ParseWaypoint::Path(u));
				}
			}
			{
				const Engine::MapelementResult::elementvector_t& ev(f.get_mapelements());
				for (Engine::MapelementResult::elementvector_t::const_iterator i(ev.begin()), e(ev.end()); i != e; ++i) {
					if (trace_dbload)
						std::cerr << "Ident " << wpt.get_icao() << ": Map Element " << i->get_name()
							  << ' ' << i->get_coord().get_lat_str2() << ' ' << i->get_coord().get_lon_str2()
							  << std::endl;
					Graph::vertex_descriptor u(m_graph.add(*i));
					if (u >= num_vertices(m_graph))
						continue;
					found = true;
					wpt.add(ParseWaypoint::Path(u));
				}
			}
			if (found) {
				++i;
				continue;
			}
		}
		if (!(wpt.get_flags() & FPlanWaypoint::altflag_ifr)) {
			++i;
			continue;
		}
		{
			bool awy(m_airways.find(wpt.get_icao()) != m_airways.end());
			if (!awy) {
				f.find_by_ident(wpt.get_icao(), "", FindCoord::flag_airway);
				const Engine::AirwayResult::elementvector_t& ev(f.get_airways());
				for (Engine::AirwayResult::elementvector_t::const_iterator i(ev.begin()), e(ev.end()); i != e; ++i) {
					if (trace_dbload)
						std::cerr << "Ident " << wpt.get_icao() << ": Airway Segment " << i->get_name()
							  << ' ' << i->get_begin_name() << ' ' << i->get_begin_coord().get_lat_str2()
							  << ' ' << i->get_begin_coord().get_lon_str2()
							  << ' ' << i->get_end_name() << ' ' << i->get_end_coord().get_lat_str2()
							  << ' ' << i->get_end_coord().get_lon_str2() << std::endl;
					if (!i->contains_name(wpt.get_icao()))
						continue;
					awy = true;
					m_graph.add(*i);
				}
				if (awy)
					m_airways.insert(wpt.get_icao());
			}
			if (awy) {
				if (i) {
					ParseWaypoint& wptp(m_wpts[i - 1]);
					wptp.set_pathcode(FPlanWaypoint::pathcode_airway);
					wptp.set_pathname(wpt.get_icao());
				}
			} else {
				bool err(true);
				if (i) {
					ParseWaypoint& wptp(m_wpts[i - 1]);
					if (wptp.get_typemask() & Vertex::typemask_airport) {
						wptp.set_pathcode(FPlanWaypoint::pathcode_sid);
						wptp.set_pathname(wpt.get_icao());
						err = false;
					} else if (i + 1 < n) {
						ParseWaypoint& wptn(m_wpts[i + 1]);
						if (wptn.get_typemask() & Vertex::typemask_airport) {
							wptp.set_pathcode(FPlanWaypoint::pathcode_star);
							wptp.set_pathname(wpt.get_icao());
							err = false;
						}
					}
				}
				if (err)
					m_errors.push_back("unknown identifier " + wpt.get_icao());
			}
			m_wpts.erase(m_wpts.begin() + i);
			n = m_wpts.size();
		}
	}
	for (wpts_t::size_type i(0); i < n; ++i) {
		ParseWaypoint& wpt(m_wpts[i]);
		if (!wpt.is_coursedist())
			continue;
		// convert magnetic to true
		double course(wpt.get_course());
		WMM wmm;
		wmm.compute(wpt.get_altitude() * (FPlanWaypoint::ft_to_m / 1000.0), wpt.get_coord(), time(0));
		if (wmm)
			course += wmm.get_dec();
		if (wpt.empty()) {
			if (wpt.get_coord().is_invalid()) {
				std::ostringstream oss;
				oss << wpt.get_icao()
				    << std::setfill('0') << std::setw(3) << wpt.get_course()
				    << std::setfill('0') << std::setw(3) << wpt.get_dist();
				wpt.set_icao(oss.str());
				wpt.set_coursedist();
				continue;
			}
			wpt.set_coord(wpt.get_coord().spheric_course_distance_nmi(course, wpt.get_dist()));
                        wpt.set_icao(wpt.get_coord().get_fpl_str());
			wpt.set_type(FPlanWaypoint::type_user);
			wpt.set_coursedist();
			continue;
		}
		typedef std::vector<ParseWaypoint::Path> paths_t;
		paths_t paths;
		for (unsigned int j = 0, k = wpt.size(); j < k; ++j) {
			ParseWaypoint::Path& p(wpt[j]);
			const Vertex& v(m_graph[p.get_vertex()]);
			FPlanWaypoint w;
			{
			       	std::ostringstream oss;
				oss << v.get_ident()
				    << std::setfill('0') << std::setw(3) << wpt.get_course()
				    << std::setfill('0') << std::setw(3) << wpt.get_dist();
				w.set_icao(oss.str());
			}
			w.set_type(FPlanWaypoint::type_user);
			w.set_coord(v.get_coord().spheric_course_distance_nmi(course, wpt.get_dist()));
			Graph::vertex_descriptor u(m_graph.add(Vertex(w)));
			if (u >= boost::num_vertices(m_graph))
				continue;
			paths.push_back(ParseWaypoint::Path(u));
		}
		if (paths.empty()) {
			wpt.set_coursedist();
			m_errors.push_back("Cannot handle course/distance for " + wpt.get_icao());
			continue;
		}
		{
			std::ostringstream oss;
			oss << wpt.get_icao()
			    << std::setfill('0') << std::setw(3) << wpt.get_course()
			    << std::setfill('0') << std::setw(3) << wpt.get_dist();
			wpt.set_icao(oss.str());
			wpt.set_coursedist();
		}
		wpt.clear();
		for (paths_t::const_iterator i(paths.begin()), e(paths.end()); i != e; ++i)
			wpt.add(*i);
	}
	//load_undef();
}

void IcaoFlightPlan::ParseState::process_airways(bool expand)
{
	wpts_t::size_type n(m_wpts.size());
	// forward pass
	wpts_t::size_type previ(n);
	for (wpts_t::size_type i(0); i < n; ++i) {
		ParseWaypoint& wpt(m_wpts[i]);
		if (wpt.get_pathcode() == FPlanWaypoint::pathcode_airway && !wpt.is_stay() &&
		    (wpt.empty() || i + 1 >= n || m_wpts[i + 1].empty())) {
			m_errors.push_back("Airway segment " + wpt.get_icao() + " " + wpt.get_pathname() +
					   " " + m_wpts[i + 1].get_icao() + " has unknown endpoint(s)");
			wpt.set_pathcode(FPlanWaypoint::pathcode_none);
			wpt.set_pathname("");
		}
		if (wpt.empty()) {
			if (!wpt.get_coord().is_invalid())
				previ = i;
			continue;
		}
		if (previ >= n) {
			for (unsigned int j = 0, k = wpt.size(); j < k; ++j) {
				ParseWaypoint::Path& p(wpt[j]);
				p.clear();
				p.set_dist(0);
			}
			previ = i;
			continue;
		}
		const ParseWaypoint& wptp(m_wpts[previ]);
		if (wptp.empty()) {
			for (unsigned int j = 0, k = wpt.size(); j < k; ++j) {
				ParseWaypoint::Path& p(wpt[j]);
				const Vertex& v(m_graph[p.get_vertex()]);
				p.clear();
				p.set_dist(wptp.get_coord().spheric_distance_nmi_dbl(v.get_coord()));
			}
			previ = i;
			continue;
		}
		if (previ + 1 != i || wptp.get_pathcode() != FPlanWaypoint::pathcode_airway || wptp.is_stay()) {
			for (unsigned int j = 0, k = wpt.size(); j < k; ++j) {
				ParseWaypoint::Path& p(wpt[j]);
				const Vertex& v(m_graph[p.get_vertex()]);
				p.clear();
				p.set_dist(std::numeric_limits<double>::max());
				for (unsigned int l = 0, m = wptp.size(); l < m; ++l) {
					const ParseWaypoint::Path& pp(wptp[l]);
					const Vertex& vv(m_graph[pp.get_vertex()]);
					double dist(vv.get_coord().spheric_distance_nmi_dbl(v.get_coord()));
					dist += pp.get_dist();
					if (dist > p.get_dist())
						continue;
					p.clear();
					p.add(pp.get_vertex());
					p.set_dist(dist);
				}
			}
			previ = i;
			continue;
		}
		previ = i;
		// interconnect disjoint airway segments
		{
			Graph::vertex_descriptor_set_t term(m_graph.find_airway_terminals(wptp.get_pathname()));
			unsigned int sz(term.size());
			if (sz < 2) {
				m_errors.push_back("airway " + wptp.get_pathname() + " has no terminals");
				continue;
			} else {
				Graph::AwyEdgeFilter filter(m_graph, wptp.get_pathname());
				typedef boost::filtered_graph<Graph, Graph::AwyEdgeFilter> fg_t;
				fg_t fg(m_graph, filter);
				std::vector<int> comp(boost::num_vertices(m_graph), 0);
				boost::connected_components(m_graph, &comp[0]);
				for (;;) {
					Graph::vertex_descriptor u0, u1;
					int comp0(-1), comp1(-1);
					double bdist(std::numeric_limits<double>::max());
					for (Graph::vertex_descriptor_set_t::const_iterator b(term.begin()), e(term.end()), i(b); i != e; ++i) {
						if (false) {
							const Vertex& v(m_graph[*i]);
							std::cerr << "Airway " << wptp.get_pathname() << " terminal " << v.get_ident()
								  << " comp " << comp[*i] << std::endl;
						}
						for (Graph::vertex_descriptor_set_t::const_iterator j(b); j != e; ++j) {
							if (i == j)
								continue;
							if (comp[*i] == comp[*j])
								continue;
							if (comp0 == -1) {
								comp0 = comp[*i];
								comp1 = comp[*j];
								u0 = *i;
								u1 = *j;
							} else if (comp0 != comp[*i] || comp1 != comp[*j]) {
								continue;
							}
							const Vertex& v0(m_graph[*i]);
							const Vertex& v1(m_graph[*j]);
							double dist(v0.get_coord().spheric_distance_nmi_dbl(v1.get_coord()));
							if (dist > bdist)
								continue;
							bdist = dist;
							u0 = *i;
							u1 = *j;
						}
					}
					if (comp0 == -1)
						break;
					boost::add_edge(u0, u1, Edge("-", 0, 999, AirwaysDb::Airway::nodata,
								     AirwaysDb::Airway::nodata, AirwaysDb::Airway::airway_invalid,
								     bdist), m_graph);
					boost::add_edge(u1, u0, Edge("-", 0, 999, AirwaysDb::Airway::nodata,
								     AirwaysDb::Airway::nodata, AirwaysDb::Airway::airway_invalid,
								     bdist), m_graph);
					boost::connected_components(m_graph, &comp[0]);
					if (false) {
						const Vertex& v0(m_graph[u0]);
						const Vertex& v1(m_graph[u1]);
						std::cerr << "Airway " << wptp.get_pathname() << " temp edge "
							  << v0.get_ident() << " <-> " << v1.get_ident() << std::endl;
					}
				}
			}
		}
		// helper edges
		{
			Graph::vertex_descriptor_set_t awyv(m_graph.find_airway_vertices(wptp.get_pathname()));
			if (awyv.empty()) {
				m_errors.push_back("airway " + wptp.get_pathname() + " has no terminals");
				continue;
			}
			if (false) {
				for (Graph::vertex_descriptor_set_t::const_iterator i(awyv.begin()), e(awyv.end()); i != e; ++i) {
					const Vertex& v1(m_graph[*i]);
					std::cerr << "Airway " << wptp.get_pathname() << ' ' << v1.get_ident()
						  << '(' << (unsigned int)*i << ')';
					Graph::out_edge_iterator e0, e1;
					for (boost::tie(e0, e1) = boost::out_edges(*i, m_graph); e0 != e1; ++e0) {
						const Edge& ee(m_graph[*e0]);
						if (ee.get_ident() != wptp.get_pathname())
							continue;
						std::cerr << ' ' << (unsigned int)boost::target(*e0, m_graph);
					}
					std::cerr << std::endl;
				}
			}
			for (unsigned int j = 0, k = wptp.size(); j < k; ++j) {
				const ParseWaypoint::Path& p(wptp[j]);
				if (awyv.find(p.get_vertex()) != awyv.end())
					continue;
				const Vertex& v0(m_graph[p.get_vertex()]);
				for (Graph::vertex_descriptor_set_t::const_iterator i(awyv.begin()), e(awyv.end()); i != e; ++i) {
					const Vertex& v1(m_graph[*i]);
					boost::add_edge(p.get_vertex(), *i, Edge("-", 0, 999, AirwaysDb::Airway::nodata,
										 AirwaysDb::Airway::nodata, AirwaysDb::Airway::airway_invalid,
										 2 * v0.get_coord().spheric_distance_nmi_dbl(v1.get_coord())), m_graph);
				}
			}
			for (unsigned int j = 0, k = wpt.size(); j < k; ++j) {
				ParseWaypoint::Path& p(wpt[j]);
				p.set_dist(std::numeric_limits<double>::max());
				p.clear();
				if (awyv.find(p.get_vertex()) != awyv.end())
					continue;
				const Vertex& v0(m_graph[p.get_vertex()]);
				for (Graph::vertex_descriptor_set_t::const_iterator i(awyv.begin()), e(awyv.end()); i != e; ++i) {
					const Vertex& v1(m_graph[*i]);
					boost::add_edge(*i, p.get_vertex(), Edge("-", 0, 999, AirwaysDb::Airway::nodata,
										 AirwaysDb::Airway::nodata, AirwaysDb::Airway::airway_invalid,
										 2 * v0.get_coord().spheric_distance_nmi_dbl(v1.get_coord())), m_graph);
				}
			}
		}
		// dijkstra
		for (unsigned int j = 0, k = wptp.size(); j < k; ++j) {
			const ParseWaypoint::Path& p(wptp[j]);
			AwyEdgeFilter filter(m_graph, wptp.get_pathname());
			typedef boost::filtered_graph<Graph, AwyEdgeFilter> fg_t;
			fg_t fg(m_graph, filter);
			std::vector<Graph::vertex_descriptor> predecessors(boost::num_vertices(m_graph), 0);
			std::vector<double> distances(boost::num_vertices(m_graph), 0);
			dijkstra_shortest_paths(m_graph, p.get_vertex(), boost::weight_map(boost::get(&Edge::m_dist, m_graph)).
						predecessor_map(&predecessors[0]).distance_map(&distances[0]));
			for (unsigned int l = 0, m = wpt.size(); l < m; ++l) {
				ParseWaypoint::Path& pp(wpt[l]);
				Graph::vertex_descriptor u(pp.get_vertex());
				if (u == predecessors[u])
					continue;
				double dist(p.get_dist() + distances[u]);
				if (dist > pp.get_dist())
					continue;
				pp.clear();
				pp.set_dist(dist);
				for (;;) {
					Graph::vertex_descriptor u1(predecessors[u]);
					if (u1 == u)
						break;
					pp.add(u1);
					u = u1;
				}
			}
		}
		clear_tempedges();
	}
	if (n > 0) {
		ParseWaypoint& wpt(m_wpts[n - 1]);
		wpt.set_pathcode(FPlanWaypoint::pathcode_none);
	}
	if (true)
		print(std::cerr << "Flight Plan after airways forward processing" << std::endl);
	// backward pass
	previ = n;
	for (wpts_t::size_type i(n); i > 1; ) {
		--i;
		ParseWaypoint& wpt(m_wpts[i]);
		if (wpt.empty()) {
			if (!wpt.get_coord().is_invalid())
				previ = i;
			continue;
		}
		if (wpt.size() > 1) {
			unsigned int bidx(wpt.size());
			double bdist(std::numeric_limits<double>::max());
			for (unsigned int j = 0, k = wpt.size(); j < k; ++j) {
				const ParseWaypoint::Path& p(wpt[j]);
				double dist(p.get_dist());
				if (previ < n) {
					const Vertex& v(m_graph[p.get_vertex()]);
					dist += v.get_coord().spheric_distance_nmi_dbl(m_wpts[previ].get_coord());
				}
				if (dist > bdist && bidx < wpt.size())
					continue;
				bdist = dist;
				bidx = j;
			}
			ParseWaypoint::Path p(wpt[bidx]);
			wpt.clear();
			wpt.add(p);
		}
		const ParseWaypoint::Path& p(wpt[0]);
		load_undef(p.get_vertex());
		const Vertex& v(m_graph[p.get_vertex()]);
		wpt.set(v.get_object(), i + 1 == n);
		ParseWaypoint& wptp(m_wpts[i - 1]);
		if (i > 0 && !p.empty()) {
			Graph::vertex_descriptor u(p[p.size()-1]);
			unsigned int n(wptp.size()), b(n);
			for (unsigned int j = 0; j < n; ++j) {
				if (wptp[j].get_vertex() != u)
					continue;
				b = j;
				break;
			}
			if (b < n) {
				ParseWaypoint::Path pp(wptp[b]);
				wptp.clear();
				wptp.add(pp);
			}
		}
		std::vector<ParseWaypoint> wpts;
		for (unsigned int j(1), k(p.size()); j < k; ++j) {
			const Vertex& v(m_graph[p[j - 1]]);
			ParseWaypoint w(wptp);
			w.set_icao(v.get_ident());
			w.set(v.get_object());
			w.set_coord(v.get_coord());
			w.set_dbobj(v.get_object());
			w.set_expanded(true);
			w.set_pathcode(wptp.get_pathcode());
			w.set_pathname(wptp.get_pathname());
			w.set_type((ParseWaypoint::type_t)v.get_type());
			w.clear();
			w.add(ParseWaypoint::Path(p[j - 1]));
			wpts.push_back(w);
		}
		m_wpts.insert(m_wpts.begin() + i, wpts.rbegin(), wpts.rend());
	}
	if (!m_wpts[0].empty()) {
		ParseWaypoint& wpt(m_wpts[0]);
		const ParseWaypoint::Path& p(wpt[0]);
		load_undef(p.get_vertex());
		const Vertex& v(m_graph[p.get_vertex()]);
		wpt.set(v.get_object(), true);
	}
	// verify airways
	n = m_wpts.size();
	for (wpts_t::size_type i(1); i < n; ++i) {
		ParseWaypoint& wpt(m_wpts[i - 1]);
		const ParseWaypoint& wptn(m_wpts[i]);
		if (wpt.empty() || wptn.empty()) {
			if (wpt.get_pathcode() != FPlanWaypoint::pathcode_airway || wpt.is_stay())
				continue;
			wpt.set_pathcode(FPlanWaypoint::pathcode_none);
			wpt.set_pathname("");
			continue;
		}
		if (wpt.get_pathcode() != FPlanWaypoint::pathcode_airway || wpt.is_stay())
			continue;
		bool ok(false);
		Graph::vertex_descriptor u(wpt[0].get_vertex()), v(wptn[0].get_vertex());
		Graph::edge_iterator e0, e1;
		for (boost::tie(e0, e1) = boost::edges(m_graph); e0 != e1; ++e0) {
			Edge& ee(m_graph[*e0]);
			if (ee.get_ident() != wpt.get_pathname())
				continue;
			ok = true;
			break;
		}
		if (ok)
			continue;
		wpt.set_pathcode(FPlanWaypoint::pathcode_none);
		wpt.set_pathname("");
	}
	// kill expanded segments
	if (!expand && n >= 2) {
		for (wpts_t::size_type i(n - 1); i > 1; --i) {
			const ParseWaypoint& wptp(m_wpts[i - 1]);
			ParseWaypoint& wpt(m_wpts[i]);
			if (!wpt.is_expanded() || wptp.get_pathcode() != FPlanWaypoint::pathcode_airway ||
			    wpt.get_pathcode() != FPlanWaypoint::pathcode_airway || wptp.get_pathname() != wpt.get_pathname())
				continue;
			m_wpts.erase(m_wpts.begin() + i);
		}
	}
}

void IcaoFlightPlan::ParseState::clear_tempedges(void)
{
	for (;;) {
		bool done(true);
 		Graph::edge_iterator e0, e1;
		for (boost::tie(e0, e1) = boost::edges(m_graph); e0 != e1;) {
			Graph::edge_iterator ex(e0);
			++e0;
			Edge& ee(m_graph[*ex]);
			if (ee.get_ident() != "-")
				continue;
			remove_edge(*ex, m_graph);
			if (true) {
				done = false;
				break;
			}
		}
		if (done)
			break;
	}
}

void IcaoFlightPlan::ParseState::load_undef(void)
{
	for (Graph::vertex_descriptor u(0), n(boost::num_vertices(m_graph)); u < n; ++u)
		load_undef(u);
}

void IcaoFlightPlan::ParseState::load_undef(Graph::vertex_descriptor u)
{
	if (u >= boost::num_vertices(m_graph))
		return;
	const Vertex& v(m_graph[u]);
	if (v.get_object())
		return;
	FindCoord f(m_engine);
	f.find_by_name(v.get_ident(), v.get_ident(), FindCoord::flag_navaid | FindCoord::flag_waypoint);
	if (trace_dbload) {
		{
			const Engine::NavaidResult::elementvector_t& ev(f.get_navaids());
			for (Engine::NavaidResult::elementvector_t::const_iterator i(ev.begin()), e(ev.end()); i != e; ++i) {
				std::cerr << "Ident " << v.get_ident() << ": Navaid " << i->get_icao_name()
					  << ' ' << i->get_coord().get_lat_str2() << ' ' << i->get_coord().get_lon_str2()
					  << std::endl;
			}
		}
		{
			const Engine::WaypointResult::elementvector_t& ev(f.get_waypoints());
			for (Engine::WaypointResult::elementvector_t::const_iterator i(ev.begin()), e(ev.end()); i != e; ++i) {
				std::cerr << "Ident " << v.get_ident() << ": Intersection " << i->get_icao_name()
					  << ' ' << i->get_coord().get_lat_str2() << ' ' << i->get_coord().get_lon_str2()
					  << std::endl;
			}
		}
	}
	m_graph.add(f.get_navaids());
	m_graph.add(f.get_waypoints());
}

void IcaoFlightPlan::ParseState::process_time(const std::string& eets)
{
	typedef std::map<std::string,unsigned int> eet_t;
	eet_t eet;
	for (std::string::const_iterator i(eets.begin()), e(eets.end()); i != e; ) {
		if (std::isspace(*i)) {
			++i;
			continue;
		}
		typedef enum {
			mode_ident,
			mode_time,
			mode_invalid
		} mode_t;
		mode_t mode(mode_ident);
		std::string ident;
		unsigned int tm(0);
		unsigned int tmc(0);
		std::string::const_iterator j(i);
		++j;
		ident += std::toupper(*i);
		if (!std::isalpha(*i))
			mode = mode_invalid;
		while (j != e && !std::isspace(*j)) {
			switch (mode) {
			case mode_ident:
				if (std::isalpha(*j)) {
					ident += std::toupper(*j);
					break;
				}
				mode = mode_time;
				/* fall through */
			case mode_time:
				if (!std::isdigit(*j)) {
					mode = mode_invalid;
					break;
				}
				tm = tm * 10 + (*j - '0');
				++tmc;
				break;

			default:
				break;
			}
			++j;
		}
		std::string el(i, j);
		i = j;
		if (ident.size() < 2 || tmc != 4)
			continue;
		tm = (tm / 100) * 60 + (tm % 100);
		eet[ident] = tm;
	}
	if (false) {
		for (cruisespeeds_t::const_iterator i(m_cruisespeeds.begin()), e(m_cruisespeeds.end()); i != e; ++i)
			std::cerr << "process_time: alt " << i->first << " speed " << i->second << std::endl;
		if (m_cruisespeeds.empty())
			std::cerr << "process_time: cruise speed table empty" << std::endl;
	}
	wpts_t::size_type n(m_wpts.size()), previ(n);
	if (!m_wpts[0].get_coord().is_invalid())
		previ = 0;
	for (wpts_t::size_type i(1); i < n; ++i) {
		ParseWaypoint& wpt(m_wpts[i]);
		{
			eet_t::const_iterator j(eet.find(wpt.get_icao_or_name()));
			if (j != eet.end()) {
				wpt.set_time(m_wpts[0].get_time() + j->second * 60);
				if (wpt.get_coord().is_invalid())
					continue;
				previ = i;
				continue;
			}
		}
		if (previ >= n) {
			wpt.set_time(m_wpts[i - 1].get_time());
			if (wpt.get_coord().is_invalid())
				continue;
			previ = i;
			continue;
		}
		const ParseWaypoint& wptp(m_wpts[previ]);
		wpt.set_time(wptp.get_time());
		if (wpt.get_coord().is_invalid())
			continue;
		previ = i;
		if (m_cruisespeeds.empty())
			continue;
		double dist(wptp.get_coord().spheric_distance_nmi_dbl(wpt.get_coord()));
		cruisespeeds_t::const_iterator b(m_cruisespeeds.begin());
		for (cruisespeeds_t::const_iterator i(b), e(m_cruisespeeds.end()); i != e; ++i) {
			if (abs(i->first - wptp.get_altitude()) < abs(b->first - wptp.get_altitude()))
				b = i;
		}
		if (b->second <= 0)
			continue;
		time_t dt(Point::round<time_t,double>(dist / b->second * 3600));
		if (false)
			std::cerr << "process_time: alt " << b->first << " speed " << b->second
				  << " dist " << dist << " deltat " << dt << std::endl;
		wpt.set_time(wpt.get_time() + dt);
	}
}

std::ostream& IcaoFlightPlan::ParseState::print(std::ostream& os) const
{
	for (wpts_t::size_type i = 0, n = size(); i < n; ++i) {
		const ParseWaypoint& wpt(operator[](i));
		os << '[' << i << "] " << wpt.to_str() << std::endl;
	}
	return os;
}

const bool IcaoFlightPlan::trace_parsestate;

IcaoFlightPlan::IcaoFlightPlan(Engine& engine)
	: m_engine(engine), m_aircraftid("ZZZZZ"), m_aircrafttype("P28R"), m_departuretime(0), m_totaleet(0), m_endurance(0),
	  m_nav(Aircraft::nav_lpv | Aircraft::nav_dme | Aircraft::nav_adf | Aircraft::nav_gnss | Aircraft::nav_pbn),
	  m_com(Aircraft::com_standard | Aircraft::com_vhf_833), m_transponder(Aircraft::transponder_modes_s),
	  m_emergency(Aircraft::emergency_radio_elt), m_pbn(Aircraft::pbn_b2), m_number(1), m_personsonboard(0),
	  m_departureflags(0), m_destinationflags(0), m_dinghiesnumber(0), m_dinghiescapacity(0),
	  m_defaultalt(5000), m_flighttype('G'), m_wakecategory('L')
{
	m_departurecoord.set_invalid();
	m_destinationcoord.set_invalid();
	time(&m_departuretime);
	m_departuretime += 60 * 60;
        Preferences& prefs(m_engine.get_prefs());
	{
		std::ostringstream oss;
		oss << 'N' << std::setw(4) << std::setfill('0') << (int)prefs.vcruise;
		m_cruisespeed = oss.str();
	}
}

void IcaoFlightPlan::clear(void)
{
	m_route.clear();
	m_otherinfo.clear();
	m_aircraftid = "ZZZZZ";
	m_aircrafttype = "P28R";
	m_nav = Aircraft::nav_lpv | Aircraft::nav_dme | Aircraft::nav_adf | Aircraft::nav_gnss | Aircraft::nav_pbn;
	m_com = Aircraft::com_standard | Aircraft::com_vhf_833;
	m_transponder = Aircraft::transponder_modes_s;
	m_emergency = Aircraft::emergency_radio_elt;
	m_cruisespeed.clear();
	m_departure.clear();
	m_destination.clear();
	m_alternate1.clear();
	m_alternate2.clear();
	m_sid.clear();
	m_star.clear();
	m_dinghiescolor.clear();
	m_colourmarkings.clear();
	m_remarks.clear();
	m_picname.clear();
	m_cruisespeeds.clear();
	m_departurecoord.set_invalid();
	m_destinationcoord.set_invalid();
	m_departuretime = 0;
	m_totaleet = 0;
	m_endurance = 0;
	m_pbn = Aircraft::pbn_b2;
	m_number = 1;
	m_personsonboard = 0;
	m_departureflags = 0;
	m_destinationflags = 0;
	m_dinghiesnumber = 0;
	m_dinghiescapacity = 0;
	m_defaultalt = 5000;
	m_flighttype = 'G';
	m_wakecategory = 'L';

	time(&m_departuretime);
	m_departuretime += 60 * 60;
        Preferences& prefs(m_engine.get_prefs());
	{
		std::ostringstream oss;
		oss << 'N' << std::setw(4) << std::setfill('0') << (int)prefs.vcruise;
		m_cruisespeed = oss.str();
	}
}

void IcaoFlightPlan::trimleft(std::string::const_iterator& txti, std::string::const_iterator txte)
{
	std::string::const_iterator txti2(txti);
	for (; txti2 != txte && isspace(*txti2); ++txti2);
	txti = txti2;
}

void IcaoFlightPlan::trimright(std::string::const_iterator txti, std::string::const_iterator& txte)
{
	std::string::const_iterator txte2(txte);
	while (txti != txte2) {
		std::string::const_iterator txte3(txte2);
		--txte3;
		if (!isspace(*txte3))
			break;
		txte2 = txte3;
	}
	txte = txte2;
}

void IcaoFlightPlan::trim(std::string::const_iterator& txti, std::string::const_iterator& txte)
{
	trimleft(txti, txte);
	trimright(txti, txte);
}

std::string IcaoFlightPlan::upcase(const std::string& txt)
{
	Glib::ustring x(txt);
	return x.uppercase();
}

bool IcaoFlightPlan::parsetxt(std::string& txt, unsigned int len, std::string::const_iterator& txti, std::string::const_iterator txte, bool slashsep)
{
	trimleft(txti, txte);
	txt.clear();
	while (txti != txte) {
		if (isspace(*txti) || (slashsep && *txti == '/') || *txti == '-' || *txti == '(' || *txti == ')')
			break;
		txt.push_back(*txti);
		++txti;
		if (len && txt.size() >= len)
			break;
	}
	trimleft(txti, txte);
	return len ? txt.size() == len : !txt.empty();
}

bool IcaoFlightPlan::parsenum(unsigned int& num, unsigned int digits, std::string::const_iterator& txti, std::string::const_iterator txte)
{
	trimleft(txti, txte);
	num = 0;
	unsigned int d = 0;
	while (txti != txte) {
		if (!isdigit(*txti))
			break;
		num *= 10;
		num += (*txti) - '0';
		++d;
		++txti;
		if (digits && d >= digits)
			break;
	}
	trimleft(txti, txte);
	return d && (!digits || d == digits);
}

bool IcaoFlightPlan::parsetime(time_t& t, std::string::const_iterator& txti, std::string::const_iterator txte)
{
	t = 0;
	unsigned int n;
	if (!parsenum(n, 4, txti, txte))
		return false;
	t = ((n / 100) * 60 + (n % 100)) * 60;
	return true;
}

bool IcaoFlightPlan::parsespeed(std::string& spdstr, float& spd, std::string::const_iterator& txti, std::string::const_iterator txte)
{
	spd = 0;
	spdstr.clear();
	trimleft(txti, txte);
	if (txti == txte)
		return false;
	char ch(*txti);
	++txti;
	float factor(1);
	unsigned int len(4);
	switch (ch) {
	case 'N':
		break;

	case 'K':
		factor = Point::km_to_nmi;
		break;

	case 'M':
		factor = 330.0 * Point::mps_to_kts; // very approximate!
		len = 3;
		break;

	default:
		return false;
	}
	unsigned int num;
	if (!parsenum(num, len, txti, txte))
		return false;
	spd = num * factor;
	std::ostringstream oss;
	oss << ch << std::setw(len) << std::setfill('0') << num;
	spdstr = oss.str();
	return true;
}

bool IcaoFlightPlan::parsealt(int& alt, unsigned int& flags, std::string::const_iterator& txti, std::string::const_iterator txte)
{
	alt = 0;
	flags &= ~FPlanWaypoint::altflag_standard;
	trimleft(txti, txte);
	if (txti == txte)
		return false;
	char ch(*txti);
	++txti;
	if (ch == 'V') {
		std::string s;
		if (!parsetxt(s, 2, txti, txte))
			return false;
		if (s != "FR")
			return false;
		flags |= 1 << 16;
		return true;
	}
	if (ch == 'F')
		flags |= FPlanWaypoint::altflag_standard;
	else if (ch != 'A')
		return false;
	unsigned int num;
	if (!parsenum(num, 3, txti, txte))
		return false;
	alt = num * 100;
	return true;
}

void IcaoFlightPlan::otherinfo_clear(const std::string& category)
{
	otherinfo_t::iterator i(m_otherinfo.find(category));
	if (i != m_otherinfo.end())
		m_otherinfo.erase(i);
}

const std::string& IcaoFlightPlan::otherinfo_get(const std::string& category)
{
	static const std::string empty;
	otherinfo_t::iterator i(m_otherinfo.find(category));
	if (i == m_otherinfo.end())
		return empty;
	return i->second;
}

void IcaoFlightPlan::otherinfo_add(const std::string& category, std::string::const_iterator texti, std::string::const_iterator texte)
{
	if (category.empty())
		return;
	trim(texti, texte);
	std::pair<otherinfo_t::iterator,bool> ins(m_otherinfo.insert(std::make_pair(category, std::string(texti, texte))));
	if (ins.second)
		return;
	ins.first->second.push_back(' ');
	ins.first->second.insert(ins.first->second.end(), texti, texte);
}

void IcaoFlightPlan::set_departuretime(time_t x)
{
	m_departuretime = x;
	otherinfo_clear("DOF");
	if (x < 24*60*60)
		return;
	struct tm tm;
        if (!gmtime_r(&x, &tm))
                return;
	std::ostringstream oss;
	oss << std::setw(2) << std::setfill('0') << (tm.tm_year % 100)
	    << std::setw(2) << std::setfill('0') << (tm.tm_mon + 1)
	    << std::setw(2) << std::setfill('0') << tm.tm_mday;
	otherinfo_add("DOF", oss.str());
}

IcaoFlightPlan::parseresult_t IcaoFlightPlan::parse(std::string::const_iterator txti, std::string::const_iterator txte, bool do_expand_airways)
{
        errors_t errors;
	clear();
	{
		trimleft(txti, txte);
		if (txti != txte && *txti == '-') {
			++txti;
			trimleft(txti, txte);
		}
		if (txti == txte || *txti != '(') {
			errors.push_back("opening parenthesis not found");
			return std::make_pair(txti, errors);
		}
		++txti;
		std::string s;
		if (!parsetxt(s, 3, txti, txte) || s != "FPL") {
			errors.push_back("flight plan does not start with FPL");
			return std::make_pair(txti, errors);
		}
		if (txti == txte || *txti != '-') {
			errors.push_back("dash expected after FPL");
			return std::make_pair(txti, errors);
		}
		++txti;
	}
	{
		std::string s;
		if (!parsetxt(s, 0, txti, txte) || s.size() < 1 || s.size() > 7) {
			errors.push_back("invalid callsign");
			return std::make_pair(txti, errors);
		}
		set_aircraftid(s);
		if (txti == txte || *txti != '-') {
			errors.push_back("dash expected after callsign");
			return std::make_pair(txti, errors);
		}
		++txti;
		trim(txti, txte);
	}
	if (txti == txte || (*txti != 'V' && *txti != 'I' && *txti != 'Z' && *txti != 'Y')) {
		errors.push_back("invalid flight rules");
		return std::make_pair(txti, errors);
	}
	m_departureflags = 0;
	if (*txti == 'I' || *txti == 'Y')
		m_departureflags |= FPlanWaypoint::altflag_ifr;
	++txti;
	trim(txti, txte);
	if (txti == txte || !isalpha(*txti)) {
		errors.push_back("invalid flight type");
		return std::make_pair(txti, errors);
	}
	set_flighttype(*txti);
	++txti;
	trim(txti, txte);
	if (txti == txte || *txti != '-') {
		errors.push_back("dash expected aircraft type");
		return std::make_pair(txti, errors);
	}
	++txti;
	{
		unsigned int num;
		set_number(1);
		if (parsenum(num, 0, txti, txte))
			set_number(num);
	}
	{
		std::string s;
		if (!parsetxt(s, 0, txti, txte) || s.size() < 2 || s.size() > 4) {
			errors.push_back("invalid aircraft type");
			return std::make_pair(txti, errors);
		}
		set_aircrafttype(s);
	}
	if (txti == txte || *txti != '/') {
		errors.push_back("slash expected");
		return std::make_pair(txti, errors);
	}
	++txti;
	trim(txti, txte);
	if (txti == txte || !isalpha(*txti)) {
		errors.push_back("invalid wake turbulence category");
		return std::make_pair(txti, errors);
	}
	set_wakecategory(*txti);
	++txti;
	trim(txti, txte);
	if (txti == txte || *txti != '-') {
		errors.push_back("dash expected before equipment");
		return std::make_pair(txti, errors);
	}
	++txti;
	{
		std::string s;
		if (!parsetxt(s, 0, txti, txte)) {
			errors.push_back("invalid equipment");
			return std::make_pair(txti, errors);
		}
		if (!set_equipment(s)) {
			errors.push_back("invalid equipment");
			return std::make_pair(txti, errors);
		}
	}
	if (txti == txte || *txti != '/') {
		errors.push_back("slash expected");
		return std::make_pair(txti, errors);
	}
	++txti;
	trim(txti, txte);
	{
		std::string s;
		if (txti == txte || !parsetxt(s, 0, txti, txte) || s.size() < 1) {
			errors.push_back("invalid transponder");
			return std::make_pair(txti, errors);
		}
		if (!set_transponder(s)) {
			errors.push_back("invalid transponder");
			return std::make_pair(txti, errors);
		}
	}
	trim(txti, txte);
	if (txti == txte || *txti != '-') {
		errors.push_back("dash expected before departure");
		return std::make_pair(txti, errors);
	}
	++txti;
	ParseState state(m_engine);
	{
		std::string s;
		if (!parsetxt(s, 4, txti, txte)) {
			errors.push_back("invalid departure");
			return std::make_pair(txti, errors);
		}
		ParseWaypoint w;
		w.set_icao(s);
		w.set_type(FPlanWaypoint::type_airport);
		w.set_coord(Point::invalid);
		w.set_typemask(ParseWaypoint::Vertex::typemask_airport);
		state.add(w);
	}
	{
		time_t t;;
		if (!parsetime(t, txti, txte)) {
			errors.push_back("invalid off-block");
			return std::make_pair(txti, errors);
		}
		state[0].set_time(t);
	}
	if (txti == txte || *txti != '-') {
		errors.push_back("dash expected before speed");
		return std::make_pair(txti, errors);
	}
	++txti;
	// parse airspeed and altitude
	{
		std::string spdstr;
		float spd;
		if (!parsespeed(spdstr, spd, txti, txte)) {
			std::string::const_iterator txti1(txti);
			std::string txt;
			if (parsetxt(txt, 0, txti1, txte))
				txt = " " + txt;
			errors.push_back("invalid speed" + txt);
			return std::make_pair(txti, errors);
		}
		set_cruisespeed(spdstr);
		int alt(0);
		unsigned int altflags(0);
		if (!parsealt(alt, altflags, txti, txte)) {
			std::string::const_iterator txti1(txti);
			std::string txt;
			if (parsetxt(txt, 0, txti1, txte))
				txt = " " + txt;
			errors.push_back("invalid altitude" + txt);
			return std::make_pair(txti, errors);
		}
		m_departureflags &= ~FPlanWaypoint::altflag_standard;
		m_departureflags |= altflags & FPlanWaypoint::altflag_standard;
		state[0].set_altitude(alt);
		state[0].set_flags(m_departureflags);
		state.add_cruisespeed(alt, spd);
	}
	// parse route
	m_sid.clear();
	m_star.clear();
	m_route.clear();
	while (txti != txte && *txti != '-' && *txti != ')') {
		std::string txt;
		if (!parsetxt(txt, 0, txti, txte, false)) {
			errors.push_back("invalid route element " + txt);
			return std::make_pair(txti, errors);
		}
		ParseWaypoint w;
		w.set_icao(txt);
		w.set_type(FPlanWaypoint::type_undefined);
		w.set_coord(Point::invalid);
		w.set_typemask(ParseWaypoint::Vertex::typemask_navaid |
			       ParseWaypoint::Vertex::typemask_intersection |
			       ParseWaypoint::Vertex::typemask_mapelement);
		state.add(w);
	}
	if (txti == txte || *txti != '-') {
		errors.push_back("dash expected before destination");
		return std::make_pair(txti, errors);
	}
	++txti;
	{
		std::string s;
		if (!parsetxt(s, 4, txti, txte)) {
			errors.push_back("invalid destination");
			return std::make_pair(txti, errors);
		}
		ParseWaypoint w;
		w.set_icao(s);
		w.set_type(FPlanWaypoint::type_airport);
		w.set_coord(Point::invalid);
		w.set_typemask(ParseWaypoint::Vertex::typemask_airport);
		state.add(w);
	}
	{
		time_t t;;
		if (!parsetime(t, txti, txte)) {
			errors.push_back("invalid total eet");
			return std::make_pair(txti, errors);
		}
		set_totaleet(t);
	}
	if (txti != txte && std::isalpha(*txti)) {
		std::string s;
		if (!parsetxt(s, 4, txti, txte)) {
			errors.push_back("invalid first alternate");
			return std::make_pair(txti, errors);
		}
		set_alternate1(s);
	}
	if (txti != txte && std::isalpha(*txti)) {
		std::string s;
		if (!parsetxt(s, 4, txti, txte)) {
			errors.push_back("invalid second alternate");
			return std::make_pair(txti, errors);
		}
		set_alternate2(s);
	}
	if (txti != txte && *txti == '-') {
		++txti;
		std::string cat;
		while (txti != txte && *txti != '-' && *txti != ')') {
			std::string s;
			if (!parsetxt(s, 0, txti, txte)) {
				errors.push_back("invalid other information");
				return std::make_pair(txti, errors);
			}
			if (txti != txte && *txti == '/') {
				cat.swap(s);
				++txti;
				continue;
			}
			if (cat.empty()) {
				errors.push_back("invalid other information");
				return std::make_pair(txti, errors);
			}
			if (cat == "PBN") {
				if (!set_pbn(s)) {
					errors.push_back("invalid PBN");
					return std::make_pair(txti, errors);
				}
			} else {
				otherinfo_add(cat, s);
			}
		}
		if (txti != txte && *txti == '-') {
			++txti;
			std::string cat, emergencyradio, survival, lifejackets, dinghies;
			while (txti != txte && *txti != '-' && *txti != ')') {
				std::string s;
				if (!parsetxt(s, 0, txti, txte)) {
					errors.push_back("invalid other2 information");
					return std::make_pair(txti, errors);
				}
				if (txti != txte && *txti == '/') {
					cat.swap(s);
					++txti;
					if (cat == "E") {
						time_t t;
						if (!parsetime(t, txti, txte)) {
							errors.push_back("invalid endurance");
							return std::make_pair(txti, errors);
						}
						set_endurance(t);
						continue;
					}
					if (cat == "P") {
						unsigned int num;
						set_personsonboard_tbn();
						if (parsenum(num, 0, txti, txte)) {
							set_personsonboard(num);
							continue;
						}
						if (!parsetxt(s, 3, txti, txte) || s != "TBN") {
							errors.push_back("invalid number of persons on board");
							return std::make_pair(txti, errors);
						}
						continue;
					}
					if (cat == "S")
						set_survival(get_survival() | Aircraft::emergency_survival);
					if (cat == "J")
						set_lifejackets(get_lifejackets() | Aircraft::emergency_jackets);
					continue;
				}
				if (cat.empty()) {
					errors.push_back("invalid other2 information");
					return std::make_pair(txti, errors);
				}
				if (cat == "A") {
					if (get_colourmarkings().empty())
						set_colourmarkings(s);
					else
						set_colourmarkings(get_colourmarkings() + " " + s);
					continue;
				}
				if (cat == "N") {
					if (get_remarks().empty())
						set_remarks(s);
					else
						set_remarks(get_remarks() + " " + s);
					continue;
				}
				if (cat == "C") {
					if (get_picname().empty())
						set_picname(s);
					else
						set_picname(get_picname() + " " + s);
					continue;
				}
				if (cat == "R") {
					if (!emergencyradio.empty())
						emergencyradio.push_back(' ');
					emergencyradio += s;
					continue;
				}
				if (cat == "S") {
			        	if (!survival.empty())
						survival.push_back(' ');
					survival += s;
					continue;
				}
				if (cat == "J") {
			        	if (!lifejackets.empty())
						lifejackets.push_back(' ');
					lifejackets += s;
					continue;
				}
				if (cat == "D") {
					if (!dinghies.empty())
						dinghies.push_back(' ');
					dinghies += s;
					continue;
				}
			}
			if (!set_emergencyradio(emergencyradio)) {
				errors.push_back("invalid emergency radio");
				return std::make_pair(txti, errors);
			}
			if (!set_survival(survival)) {
				errors.push_back("invalid survival equipment");
				return std::make_pair(txti, errors);
			}
			if (!set_lifejackets(lifejackets)) {
				errors.push_back("invalid lifejackets");
				return std::make_pair(txti, errors);
			}
			if (!set_dinghies(dinghies)) {
				errors.push_back("invalid dinghies");
				return std::make_pair(txti, errors);
			}
		}
	}
	{
		const std::string& dof(otherinfo_get("DOF"));
		bool ok(dof.size() == 6);
		unsigned int dofn(0);
		if (ok) {
			char *cp;
			dofn = strtoul(dof.c_str(), &cp, 10);
			ok = !*cp;
		}
		if (ok) {
			struct tm tm;
			time_t x(state[0].get_time());
			if (gmtime_r(&x, &tm)) {
				tm.tm_mday = dofn % 100U;
				tm.tm_mon = (dofn / 100U) % 100U - 1U;
				tm.tm_year = dofn / 10000U + 100U;
				time_t t = timegm(&tm);
				if (t != -1)
					state[0].set_time(t);
			}
		}
	}
	state[0].set_name(otherinfo_get("DEP"));
	state[state.size() - 1].set_name(otherinfo_get("DEST"));
	if (trace_parsestate)
		state.print(std::cerr << "Flight Plan after Parsing" << std::endl);
	state.process_speedalt();
	if (trace_parsestate)
		state.print(std::cerr << "Flight Plan after Speed/Alt processing" << std::endl);
	state.process_dblookup();
	if (trace_parsestate)
		state.print(std::cerr << "Flight Plan after DB lookup" << std::endl);
	state.process_airways(do_expand_airways);
	state.process_time(otherinfo_get("EET"));
	if (trace_parsestate)
		state.print(std::cerr << "Flight Plan after airways processing" << std::endl);
	{
		ParseState::wpts_t::size_type n(state.size());
		if (n < 2) {
			errors.push_back("no waypoints after processing");
			return std::make_pair(txti, errors);
		}
		const ParseWaypoint& dep(state[0]);
		const ParseWaypoint& dest(state[n - 1]);
		const ParseWaypoint& destp(state[n - 2]);
		m_departure = dep.get_icao();
		m_destination = dest.get_icao();
		set_departuretime(dep.get_time());
		m_departureflags = dep.get_flags() & ~FPlanWaypoint::altflag_standard;
		m_destinationflags = dest.get_flags() & ~FPlanWaypoint::altflag_standard;
		m_departurecoord = dep.get_coord();
		m_destinationcoord = dest.get_coord();
		if (dep.get_pathcode() == FPlanWaypoint::pathcode_sid)
			m_sid = dep.get_pathname();
		if (destp.get_pathcode() == FPlanWaypoint::pathcode_star)
			m_star = destp.get_pathname();
		for (ParseState::wpts_t::size_type i(2); i < n; ++i) {
			m_route.push_back(state[i - 1]);
			if (m_route.back().get_altitude() == std::numeric_limits<int32_t>::min())
				m_route.back().set_altitude(m_defaultalt);
		}
	}
	if (AirportsDb::Airport::is_fpl_zzzz(m_departure)) {
		m_departure = "ZZZZ";
		if (!m_departurecoord.is_invalid() && otherinfo_get("DEP").empty())
			otherinfo_add("DEP", m_departurecoord.get_fpl_str());
	} else if (!m_departurecoord.is_invalid()) {
		otherinfo_clear("DEP");
	}
	if (AirportsDb::Airport::is_fpl_zzzz(m_destination)) {
		m_departure = "ZZZZ";
		if (!m_destinationcoord.is_invalid() && otherinfo_get("DEST").empty())
			otherinfo_add("DEST", m_destinationcoord.get_fpl_str());
	} else if (!m_destinationcoord.is_invalid()) {
		otherinfo_clear("DEST");
	}
	{
		typedef std::set<unsigned int> staynr_t;
		staynr_t staynr;
		for (route_t::const_iterator ri(m_route.begin()), re(m_route.end()); ri != re; ++ri) {
			unsigned int nr, tm;
			if (!ri->is_stay(nr, tm))
				continue;
			if (staynr.find(nr) != staynr.end()) {
				std::ostringstream oss;
				oss << "duplicate STAY" << nr;
				errors.push_back(oss.str());
			}
			staynr.insert(nr);
		}
		if (!staynr.empty() && (*staynr.rbegin() - *staynr.begin()) != (staynr.size() - 1))
			errors.push_back("multiple STAY but not consecutive");
		for (staynr_t::const_iterator si(staynr.begin()), se(staynr.end()); si != se; ++si) {
			std::ostringstream oss;
			oss << "STAYINFO" << (*si + 1);
			if (!otherinfo_get(oss.str()).empty())
				continue;
			errors.push_back(oss.str() + " missing");
		}
	}
	m_cruisespeeds = state.get_cruisespeeds();
	normalize_pogo();
	errors.insert(errors.end(), state.get_errors().begin(), state.get_errors().end());
	return std::make_pair(txti, errors);
}

IcaoFlightPlan::parseresult_t IcaoFlightPlan::parse_route(std::vector<FPlanWaypoint>& wpts, std::string::const_iterator txti, std::string::const_iterator txte, bool expand_airways)
{
	bool hasstart(!wpts.empty());
	ParseState state(m_engine);
	if (hasstart)
		state.add(wpts.back());
	errors_t errors;
	while (txti != txte) {
		trimleft(txti, txte);
		if (txti == txte)
			break;
		if (*txti == '-') {
			++txti;
			continue;
		}
		std::string txt;
		if (!parsetxt(txt, 0, txti, txte, false)) {
			errors.push_back("invalid route element " + txt);
			break;
		}
		ParseWaypoint w;
		w.set_icao(txt);
		w.set_type(FPlanWaypoint::type_undefined);
		w.set_coord(Point::invalid);
		w.set_typemask(ParseWaypoint::Vertex::typemask_navaid |
			       ParseWaypoint::Vertex::typemask_intersection |
			       ParseWaypoint::Vertex::typemask_mapelement |
			       ParseWaypoint::Vertex::typemask_airport);
		state.add(w);
	}
	if (trace_parsestate)
		state.print(std::cerr << "Flight Plan after Parsing" << std::endl);
	state.process_speedalt();
	if (trace_parsestate)
		state.print(std::cerr << "Flight Plan after Speed/Alt processing" << std::endl);
	state.process_dblookup();
	if (trace_parsestate)
		state.print(std::cerr << "Flight Plan after DB lookup" << std::endl);
	state.process_airways(expand_airways);
	state.process_time();
	if (trace_parsestate)
		state.print(std::cerr << "Flight Plan after airways processing" << std::endl);
	for (ParseState::wpts_t::size_type i(hasstart), n(state.size()); i < n; ++i) {
		wpts.push_back(state[i].get_fplwpt());
		if (wpts.back().get_altitude() == std::numeric_limits<int32_t>::min())
			wpts.back().set_altitude(m_defaultalt);
	}
	errors.insert(errors.end(), state.get_errors().begin(), state.get_errors().end());
	return std::make_pair(txti, errors);
}

IcaoFlightPlan::parseresult_t IcaoFlightPlan::parse_garminpilot(std::string::const_iterator txti, std::string::const_iterator txte, bool do_expand_airways)
{
	errors_t errors;
	{
		static const char prefix[] = "garminpilot://flightplan?";
		std::string::const_iterator ti(txti);
		for (const char *cp = prefix; *cp; ++cp, ++ti)
			if (ti == txte || *cp != *ti) {
				errors.push_back("invalid prefix");
				return std::make_pair(txti, errors);
			}
		txti = ti;
	}
	typedef std::map<std::string,std::string> params_t;
	params_t params;
	while (txti != txte) {
		std::string::const_iterator ti1(txti);
		while (ti1 != txte && *ti1 != '=' && *ti1 != '&')
			++ti1;
		std::string::const_iterator ti2(ti1);
		if (ti2 != txte && *ti2 == '=')
			++ti2;
		std::string::const_iterator ti3(ti2);
		while (ti3 != txte && *ti3 != '&')
			++ti3;
		if (ti1 == txti) {
			errors.push_back("empty parameter");
			txti = ti3;
			continue;
		}
		if (!params.insert(params_t::value_type(std::string(txti, ti1), std::string(ti2, ti3))).second)
			errors.push_back("duplicate parameter " + std::string(txti, ti1));
		txti = ti3;
		if (txti != txte)
			++txti;
	}
	// aircraft
	{
		params_t::const_iterator pi(params.find("aircraft"));
		if (pi != params.end())
			set_aircraftid(pi->second);
	}
	// etd
	{
		params_t::const_iterator pi(params.find("etd"));
		if (pi != params.end()) {
			const char *cp(pi->second.c_str());
			char *cp1;
			time_t tm(strtoul(cp, &cp1, 10));
			if (cp1 == cp || *cp1)
				errors.push_back("invalid etd " + pi->second);
			else
				set_departuretime(tm);
		}
	}
	// speed
	double speed(std::numeric_limits<double>::quiet_NaN());
	{
		params_t::const_iterator pi(params.find("speed"));
		if (pi != params.end()) {
			const char *cp(pi->second.c_str());
			char *cp1;
			double spd(strtod(cp, &cp1));
			if (cp1 == cp || *cp1) {
				errors.push_back("invalid speed " + pi->second);
			} else {
				speed = spd;
				std::ostringstream oss;
				oss << 'N' << std::setw(4) << std::fixed << std::setprecision(0) << std::setfill('0') << speed;
				set_cruisespeed(oss.str());
			}
		}
	}
	// altitude
	int32_t altitude(5000);
	{
		params_t::const_iterator pi(params.find("altitude"));
		if (pi != params.end()) {
			const char *cp(pi->second.c_str());
			char *cp1;
			int32_t alt(strtol(cp, &cp1, 10));
			if (cp1 == cp || *cp1)
				errors.push_back("invalid altitude " + pi->second);
			else
				altitude = alt;
		}
	}
	// route
	ParseState state(m_engine);
	if (!std::isnan(speed))
		state.add_cruisespeed(altitude, speed);
	{
		params_t::const_iterator pi(params.find("route"));
		if (pi != params.end()) {
			m_departureflags = FPlanWaypoint::altflag_ifr | FPlanWaypoint::altflag_standard;
			m_sid.clear();
			m_star.clear();
			m_route.clear();
			for (std::string::const_iterator ri(pi->second.begin()), re(pi->second.end()); ri != re; ) {
				std::string::const_iterator wi0(ri);
				while (ri != re && *ri != '+')
					++ri;
				std::string::const_iterator wi1(ri);
				if (ri != re)
					++ri;
				if (wi0 == wi1)
					continue;
				ParseWaypoint w;
				w.set_icao(std::string(wi0, wi1));
				w.set_type(FPlanWaypoint::type_undefined);
				w.set_coord(Point::invalid);
				w.set_typemask(ParseWaypoint::Vertex::typemask_navaid |
					       ParseWaypoint::Vertex::typemask_intersection |
					       ParseWaypoint::Vertex::typemask_mapelement);
				w.set_altitude(altitude);
				w.set_flags(m_departureflags);
				// check for coordinate waypoint
				{
					const char *cp(w.get_icao().c_str());
					char *cp1;
					double lat(strtod(cp, &cp1));
					if (cp1 != cp && *cp1 == '/') {
						cp = cp1 + 1;
						double lon(strtod(cp, &cp1));
						if (cp1 != cp && !*cp1) {
							Point pt;
							pt.set_lat_deg_dbl(lat);
							pt.set_lon_deg_dbl(lon);
							w.set_coord(pt);
							w.set_type(FPlanWaypoint::type_user);
							w.set_icao(pt.get_fpl_str(Point::fplstrmode_degminsec));
						}
					}
				}
				state.add(w);
			}
		}
	}
	if (!state.empty()) {
		state.front().set_type(FPlanWaypoint::type_airport);
		state.front().set_typemask(ParseWaypoint::Vertex::typemask_airport);
		state.back().set_type(FPlanWaypoint::type_airport);
		state.back().set_typemask(ParseWaypoint::Vertex::typemask_airport);
	}
	if (trace_parsestate)
		state.print(std::cerr << "Flight Plan after Parsing" << std::endl);
	state.process_speedalt();
	if (trace_parsestate)
		state.print(std::cerr << "Flight Plan after Speed/Alt processing" << std::endl);
	state.process_dblookup();
	if (trace_parsestate)
		state.print(std::cerr << "Flight Plan after DB lookup" << std::endl);
	state.process_airways(do_expand_airways);
	if (trace_parsestate)
		state.print(std::cerr << "Flight Plan after airways processing" << std::endl);
	{
		ParseState::wpts_t::size_type n(state.size());
		if (n < 2) {
			errors.push_back("no waypoints after processing");
			return std::make_pair(txti, errors);
		}
		const ParseWaypoint& dep(state[0]);
		const ParseWaypoint& dest(state[n - 1]);
		const ParseWaypoint& destp(state[n - 2]);
		m_departure = dep.get_icao();
		m_destination = dest.get_icao();
		set_departuretime(dep.get_time());
		m_departureflags = dep.get_flags() & ~FPlanWaypoint::altflag_standard;
		m_destinationflags = dest.get_flags() & ~FPlanWaypoint::altflag_standard;
		m_departurecoord = dep.get_coord();
		m_destinationcoord = dest.get_coord();
		if (dep.get_pathcode() == FPlanWaypoint::pathcode_sid)
			m_sid = dep.get_pathname();
		if (destp.get_pathcode() == FPlanWaypoint::pathcode_star)
			m_star = destp.get_pathname();
		for (ParseState::wpts_t::size_type i(2); i < n; ++i) {
			m_route.push_back(state[i - 1]);
			if (m_route.back().get_altitude() == std::numeric_limits<int32_t>::min())
				m_route.back().set_altitude(m_defaultalt);
		}
	}
	if (!std::isnan(speed) && speed > 0) {
		double dist(0);
		for (ParseState::wpts_t::size_type i0(0), n(state.size()); i0 < n; ) {
			ParseWaypoint& w0(state[i0]);
			if (w0.get_coord().is_invalid()) {
				++i0;
				continue;
			}
			ParseState::wpts_t::size_type i1(i0 + 1);
			while (i1 < n && state[i1].get_coord().is_invalid())
				++i1;
			if (i1 >= n)
				break;
			const ParseWaypoint& w1(state[i1]);
			dist += w0.get_coord().spheric_distance_nmi_dbl(w1.get_coord());
			i0 = i1;
		}
		dist /= speed;
		dist *= 3600;
		if (!std::isnan(dist) && dist > 0)
			set_totaleet(Point::round<time_t,double>(dist));
	}
	if (AirportsDb::Airport::is_fpl_zzzz(m_departure)) {
		m_departure = "ZZZZ";
		if (!m_departurecoord.is_invalid() && otherinfo_get("DEP").empty())
			otherinfo_add("DEP", m_departurecoord.get_fpl_str());
	} else if (!m_departurecoord.is_invalid()) {
		otherinfo_clear("DEP");
	}
	if (AirportsDb::Airport::is_fpl_zzzz(m_destination)) {
		m_departure = "ZZZZ";
		if (!m_destinationcoord.is_invalid() && otherinfo_get("DEST").empty())
			otherinfo_add("DEST", m_destinationcoord.get_fpl_str());
	} else if (!m_destinationcoord.is_invalid()) {
		otherinfo_clear("DEST");
	}
	m_cruisespeeds = state.get_cruisespeeds();
	normalize_pogo();
	errors.insert(errors.end(), state.get_errors().begin(), state.get_errors().end());
	return std::make_pair(txti, errors);
}

Rect IcaoFlightPlan::get_bbox(void) const
{
	route_t::const_iterator ri(m_route.begin()), re(m_route.end());
	if (ri == re)
                return Rect();
        Point pt(ri->get_coord());
	++ri;
        Point sw(0, 0), ne(0, 0);
        for (; ri != re; ++ri) {
                Point p(ri->get_coord() - pt);
                sw = Point(std::min(sw.get_lon(), p.get_lon()), std::min(sw.get_lat(), p.get_lat()));
                ne = Point(std::max(ne.get_lon(), p.get_lon()), std::max(ne.get_lat(), p.get_lat()));
        }
        return Rect(sw + pt, ne + pt);
}

char IcaoFlightPlan::get_flightrules(void) const
{
	unsigned int ftype(!!(m_departureflags & FPlanWaypoint::altflag_ifr));
	{
		unsigned int ft(!!(m_destinationflags & FPlanWaypoint::altflag_ifr));
		ft ^= ftype;
		ft &= 1;
		ftype |= ft << 1;
	}
	static const char frules[4] = { 'V', 'I', 'Z', 'Y' };
	if (ftype & 2)
		return frules[ftype];
	route_t::const_iterator ri(m_route.begin()), re(m_route.end());
	if (ri == re)
		return frules[ftype];
	for (;;) {
		unsigned int ft(ri->is_ifr());
		++ri;
		if (ri == re)
			break;
		ft ^= ftype;
		ft &= 1;
		ftype |= ft << 1;
		if (ft)
			break;
	}
	return frules[ftype];
}

void IcaoFlightPlan::equipment_canonicalize(void)
{
	Aircraft::equipment_with_standard(m_nav, m_com);
}

IcaoFlightPlan::nameset_t IcaoFlightPlan::intersect(const nameset_t& s1, const nameset_t& s2)
{
	nameset_t r;
	std::set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(), std::inserter(r, r.begin()));
	return r;
}

void IcaoFlightPlan::populate(const FPlanRoute& route, awymode_t awymode, double dct_limit)
{
	clear();
	uint16_t nrwpt = route.get_nrwpt();
	if (!nrwpt)
		return;
	m_departurecoord = route[0].get_coord();
	m_destinationcoord = route[nrwpt - 1].get_coord();
	set_departuretime(route.get_time_offblock_unix());
	m_totaleet = route[nrwpt - 1].get_time_unix() - route[0].get_time_unix();
	m_departure = upcase(route[0].get_icao());
	otherinfo_clear("DEP");
	if (AirportsDb::Airport::is_fpl_zzzz(m_departure)) {
		m_departure = "ZZZZ";
		otherinfo_add("DEP", upcase(route[0].get_name()) + " " + route[0].get_coord().get_fpl_str());
		if (false)
			std::cerr << "DEP: " << otherinfo_get("DEP") << std::endl;
	}
	m_departureflags = route[0].get_flags();
	m_destination = upcase(route[nrwpt - 1].get_icao());
	otherinfo_clear("DEST");
	if (AirportsDb::Airport::is_fpl_zzzz(m_destination)) {
		m_destination = "ZZZZ";
		otherinfo_add("DEST", upcase(route[nrwpt - 1].get_name()) + " " + route[nrwpt - 1].get_coord().get_fpl_str());
		if (false)
			std::cerr << "DEST: " << otherinfo_get("DEST") << std::endl;
	}
	m_destinationflags = route[nrwpt - 1].get_flags();
	m_defaultalt = std::max(0, (int)std::max(route[0].get_altitude(), route[nrwpt - 1].get_altitude()));
	if (m_defaultalt >= 5000)
		m_defaultalt += 1000;
	m_defaultalt += 1999;
	m_defaultalt /= 1000;
	m_defaultalt *= 1000;
	{
		bool have_ifr(!!((m_departureflags | m_destinationflags) & FPlanWaypoint::altflag_ifr));
		for (uint16_t nr = 1; nr + 1 < nrwpt; ++nr) {
			if (route[nr].is_ifr() && route[nr].get_pathcode() == FPlanWaypoint::pathcode_sid)
				continue;
			if ((Engine::AirwayGraphResult::Vertex::is_ident_numeric(route[nr].get_ident()) ||
			     route[nr].get_ident().size() < 2) &&
			    route[nr].get_pathcode() == FPlanWaypoint::pathcode_airway &&
			    route[nr-1].get_pathcode() == FPlanWaypoint::pathcode_airway &&
			    route[nr].get_pathname() == route[nr-1].get_pathname() &&
			    (route[nr].get_flags() & route[nr-1].get_flags() & FPlanWaypoint::altflag_ifr))
				continue;
			m_route.push_back(route[nr]);
			m_route.back().set_time(route[nr].get_time_unix() - route[0].get_time_unix());
			if (nr + 2 == nrwpt)
				m_route.back().set_flags(m_route.back().get_flags() ^
							 ((m_route.back().get_flags() ^ m_destinationflags) & FPlanWaypoint::altflag_ifr));
			have_ifr = have_ifr || m_route.back().is_ifr();
			if (route[nr].is_ifr() && route[nr].get_pathcode() == FPlanWaypoint::pathcode_star)
				break;
		}
		if (have_ifr) {
			static const char *ifpsra = /*"IFPS RTE AMDT ACPT"*/ "IFPSRA";
			const std::string& rmk(otherinfo_get("RMK"));
			if (rmk.find(ifpsra) == std::string::npos)
				otherinfo_add("RMK", ifpsra);
		}
	}
	bool has_pc(has_pathcodes());
	if (m_departureflags & FPlanWaypoint::altflag_ifr) {
		if (route[0].get_pathcode() == FPlanWaypoint::pathcode_sid) {
			m_sid = route[0].get_pathname();
			{
				std::string::size_type n(m_sid.find('-'));
				if (n != std::string::npos)
					m_sid.erase(n);
			}
			if (m_sid.size() < 3) {
				m_sid.clear();
			} else {
				if (std::isdigit(m_sid[m_sid.size() - 2]) &&
				    std::isalpha(m_sid[m_sid.size() - 1]) &&
				    m_route.size() >= 2) {
					const std::string *name(&m_route.front().get_icao());
					if (name->empty() || *name == m_departure)
						name = &m_route.front().get_name();
					if (m_sid.substr(0, m_sid.size() - 2) == name->substr(0, m_sid.size() - 2))
						m_sid = *name + m_sid.substr(m_sid.size() - 2);
				}
			}
		}
	} else {
		bool vd(route[0].get_pathcode() == FPlanWaypoint::pathcode_vfrdeparture);
		if (vd) {
			while (!m_route.empty()) {
				const Waypoint& wpt(m_route[0]);
				if (m_route[0].is_ifr())
					break;
				if (wpt.get_pathcode() != FPlanWaypoint::pathcode_vfrdeparture) {
					if (wpt.get_pathcode() != FPlanWaypoint::pathcode_none)
						break;
					m_route.erase(m_route.begin(), m_route.begin() + 1);
					break;
				}
				m_route.erase(m_route.begin(), m_route.begin() + 1);
			}
		} else if (!has_pc) {
			erase_vfrroute_begin();
		}
	}
	if (!m_route.empty()) {
		if (m_destinationflags & FPlanWaypoint::altflag_ifr) {
			if (m_route[m_route.size() - 1U].get_pathcode() == FPlanWaypoint::pathcode_star) {
				m_star = m_route[m_route.size() - 1U].get_pathname();
				{
					std::string::size_type n(m_star.find('-'));
					if (n != std::string::npos)
						m_star.erase(n);
				}
				if (m_star.size() < 3) {
					m_star.clear();
				} else {
					if (std::isdigit(m_star[m_star.size() - 2]) &&
					    std::isalpha(m_star[m_star.size() - 1]) &&
					    m_route.size() >= 2) {
						const std::string *name(&m_route.back().get_icao());
						if (name->empty() || *name == m_destination)
							name = &m_route.back().get_name();
						if (m_star.substr(0, m_star.size() - 2) == name->substr(0, m_star.size() - 2))
							m_star = *name + m_star.substr(m_star.size() - 2);
					}
				}
			}
		} else {
			bool va(m_route[m_route.size() - 1U].get_pathcode() == FPlanWaypoint::pathcode_vfrarrival);
			if (va) {
				while (!m_route.empty()) {
					const Waypoint& wpt(m_route[m_route.size() - 1U]);
					if (wpt.is_ifr() || (m_route.size() >= 2 && m_route[m_route.size() - 2U].is_ifr()) ||
					    wpt.get_pathcode() != FPlanWaypoint::pathcode_vfrarrival)
						break;
					m_route.resize(m_route.size() - 1U);
				}
			} else if (!has_pc) {
				erase_vfrroute_end();
			}
		}
	}
	redo_route_names();
       	find_airways();
	enforce_pathcode_vfrifr();
	fix_max_dct_distance(dct_limit);
	if (awymode == awymode_collapse || awymode == awymode_collapse_all)
		erase_unnecessary_airway(awymode != awymode_collapse_all);
	add_eet();
	normalize_pogo();
	if (false) {
		std::cerr << "Other Info:";
                for (otherinfo_t::const_iterator oi(m_otherinfo.begin()), oe(m_otherinfo.end()); oi != oe; ++oi)
			std::cerr << ' ' << oi->first << '/' << oi->second;
		std::cerr << std::endl;
	}
}

bool IcaoFlightPlan::has_pathcodes(void) const
{
        for (unsigned int i = 0; i < m_route.size(); ++i)
		if (m_route[i].get_pathcode() != FPlanWaypoint::pathcode_none)
			return true;
	return false;
}

bool IcaoFlightPlan::enforce_pathcode_vfrdeparr(void)
{
	unsigned int nr(m_route.size());
	if (nr < 1U)
		return false;
	bool work(false);
	if (!(m_departureflags & FPlanWaypoint::altflag_ifr)) {
		unsigned int i(0);
		for (; i + 1U < nr; ++i) {
			const Waypoint& wpt(m_route[i]);
			if (wpt.get_icao() != m_departure || wpt.is_ifr() ||
			    wpt.get_pathcode() == FPlanWaypoint::pathcode_vfrarrival ||
			    wpt.get_pathcode() == FPlanWaypoint::pathcode_vfrtransition)
				break;
		}
		if (i > 0) {
			work = true;
			do {
				--i;
				m_route[i].set_pathcode(FPlanWaypoint::pathcode_vfrdeparture);
			} while (i > 0);
		}
	}
	if (!(m_destinationflags & FPlanWaypoint::altflag_ifr)) {
		unsigned int i(nr);
		if (i > 0) {
			--i;
			while (i > 0) {
				--i;
				const Waypoint& wpt(m_route[i]);
				if (wpt.get_icao() != m_destination || wpt.is_ifr() ||
				    wpt.get_pathcode() == FPlanWaypoint::pathcode_vfrdeparture ||
				    wpt.get_pathcode() == FPlanWaypoint::pathcode_vfrtransition)
					break;
			}
		}
		if (i < nr - 1) {
			work = true;
			do {
				++i;
				m_route[i].set_pathcode(FPlanWaypoint::pathcode_vfrarrival);
			} while (i < nr - 1);
		}
	}
	{
		Waypoint& wpt(m_route[nr - 1]);
		work = work || wpt.get_pathcode() != FPlanWaypoint::pathcode_none ||
			!wpt.get_pathname().empty();
		wpt.set_pathcode(FPlanWaypoint::pathcode_none);
		wpt.set_pathname("");
	}
	return work;
}

bool IcaoFlightPlan::enforce_pathcode_vfrifr(void)
{
	unsigned int nr(m_route.size());
	bool work(false);
	for (unsigned int i = 0; i < nr;) {
		Waypoint& wpt(m_route[i]);
		++i;
		if (i >= nr) {
			work = work || wpt.get_pathcode() != FPlanWaypoint::pathcode_none ||
				!wpt.get_pathname().empty();
			wpt.set_pathcode(FPlanWaypoint::pathcode_none);
			wpt.set_pathname("");
			break;
		}
		work = wpt.enforce_pathcode_vfrifr() || work;
	}
	return work;
}

bool IcaoFlightPlan::mark_vfrroute_begin(FPlanRoute& route)
{
	if (route.get_nrwpt() < 2U)
		return false;
	AirportsDb::element_t apt;
	{
		FindCoord f(m_engine);
		if (!f.find(route[0U].get_icao(), route[0U].get_name(), FindCoord::flag_airport))
			return false;
		if (!f.get_airport(apt))
			return false;
	}
	unsigned int bestroute(0);
	unsigned int bestcount(0);
	for (unsigned int rtenr(0); rtenr < apt.get_nrvfrrte(); ++rtenr) {
		const AirportsDb::Airport::VFRRoute& rte(apt.get_vfrrte(rtenr));
		if (!rte.size())
			continue;
		unsigned int ptnr(0);
		for (; ptnr < rte.size() && ptnr <= route.get_nrwpt(); ++ptnr)
			if (rte[ptnr].get_coord() != route[ptnr].get_coord())
				break;
		if (ptnr > bestcount) {
			bestcount = ptnr;
			bestroute = rtenr;
		}
	}
	if (bestcount < 2U)
		return false;
	--bestcount;
	const AirportsDb::Airport::VFRRoute& rte(apt.get_vfrrte(bestroute));
	for (unsigned int ptnr(0); ptnr < bestcount; ++ptnr) {
		FPlanWaypoint& wpt(route[ptnr]);
		const AirportsDb::Airport::VFRRoute::VFRRoutePoint& vfrpt(rte[ptnr]);
		wpt.set_pathname(rte.get_name());
		switch (vfrpt.get_pathcode()) {
		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_arrival:
			wpt.set_pathcode(FPlanWaypoint::pathcode_vfrarrival);
			break;

		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_departure:
			wpt.set_pathcode(FPlanWaypoint::pathcode_vfrdeparture);
			break;

		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_vfrtransition:
			wpt.set_pathcode(FPlanWaypoint::pathcode_vfrtransition);
			break;

		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_holding:
		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_trafficpattern:
		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_other:
		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_invalid:
		default:
			wpt.set_pathcode(FPlanWaypoint::pathcode_none);
			break;
		}
	}
	return true;
}

bool IcaoFlightPlan::mark_vfrroute_end(FPlanRoute& route)
{
	if (route.get_nrwpt() < 2U)
		return false;
	AirportsDb::element_t apt;
	{
		FindCoord f(m_engine);
		if (!f.find(route[route.get_nrwpt() - 1U].get_icao(), route[route.get_nrwpt() - 1U].get_name(), FindCoord::flag_airport))
			return false;
		if (!f.get_airport(apt))
			return false;
	}
	unsigned int bestroute(0);
	unsigned int bestcount(0);
	for (unsigned int rtenr(0); rtenr < apt.get_nrvfrrte(); ++rtenr) {
		const AirportsDb::Airport::VFRRoute& rte(apt.get_vfrrte(rtenr));
		if (!rte.size())
			continue;
		unsigned int ptnr(1);
		for (; ptnr < rte.size() && ptnr <= route.get_nrwpt(); ++ptnr)
			if (rte[rte.size() - ptnr].get_coord() != route[route.get_nrwpt() - ptnr].get_coord())
				break;
		--ptnr;
		if (ptnr > bestcount) {
			bestcount = ptnr;
			bestroute = rtenr;
		}
	}
	if (bestcount < 2U)
		return false;
	const AirportsDb::Airport::VFRRoute& rte(apt.get_vfrrte(bestroute));
	for (unsigned int ptnr(2); ptnr <= bestcount; ++ptnr) {
		FPlanWaypoint& wpt(route[route.get_nrwpt() - ptnr]);
		const AirportsDb::Airport::VFRRoute::VFRRoutePoint& vfrpt(rte[rte.size() - ptnr]);
		wpt.set_pathname(rte.get_name());
		switch (vfrpt.get_pathcode()) {
		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_arrival:
			wpt.set_pathcode(FPlanWaypoint::pathcode_vfrarrival);
			break;

		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_departure:
			wpt.set_pathcode(FPlanWaypoint::pathcode_vfrdeparture);
			break;

		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_vfrtransition:
			wpt.set_pathcode(FPlanWaypoint::pathcode_vfrtransition);
			break;

		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_holding:
		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_trafficpattern:
		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_other:
		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_invalid:
		default:
			wpt.set_pathcode(FPlanWaypoint::pathcode_none);
			break;
		}
	}
	return true;
}

void IcaoFlightPlan::erase_vfrroute_begin(void)
{
	if (m_departureflags & FPlanWaypoint::altflag_ifr)
		return;
	AirportsDb::element_t apt;
	{
		FindCoord f(m_engine);
		if (!f.find(m_departure == "ZZZZ" ? "" : m_departure, otherinfo_get("DEP"), FindCoord::flag_airport))
			return;
		if (!f.get_airport(apt))
			return;
	}
	unsigned int bestroute(0);
	unsigned int bestcount(0);
	for (unsigned int rtenr(0); rtenr < apt.get_nrvfrrte(); ++rtenr) {
		const AirportsDb::Airport::VFRRoute& rte(apt.get_vfrrte(rtenr));
		if (!rte.size())
			continue;
		if (!rte[0].is_at_airport())
			continue;
		unsigned int ptnr(1);
		for (; ptnr < rte.size() && ptnr <= m_route.size(); ++ptnr)
			if (rte[ptnr].get_coord() != m_route[ptnr-1].get_coord())
				break;
		--ptnr;
		if (ptnr > bestcount) {
			bestcount = ptnr;
			bestroute = rtenr;
		}
	}
	if (!bestcount)
		return;
	m_route.erase(m_route.begin(), m_route.begin() + bestcount);
}

void IcaoFlightPlan::erase_vfrroute_end(void)
{
	if (m_destinationflags & FPlanWaypoint::altflag_ifr)
		return;
	AirportsDb::element_t apt;
	{
		FindCoord f(m_engine);
		if (!f.find(m_destination == "ZZZZ" ? "" : m_destination, otherinfo_get("DEST"), FindCoord::flag_airport))
			return;
		if (!f.get_airport(apt))
			return;
	}
	unsigned int bestroute(0);
	unsigned int bestcount(0);
	for (unsigned int rtenr(0); rtenr < apt.get_nrvfrrte(); ++rtenr) {
		const AirportsDb::Airport::VFRRoute& rte(apt.get_vfrrte(rtenr));
		if (!rte.size())
			continue;
		if (!rte[rte.size()-1].is_at_airport())
			continue;
		unsigned int ptnr(1);
		for (; ptnr < rte.size() && ptnr <= m_route.size(); ++ptnr)
			if (rte[rte.size()-1-ptnr].get_coord() != m_route[m_route.size() - ptnr].get_coord())
				break;
		--ptnr;
		if (ptnr > bestcount) {
			bestcount = ptnr;
			bestroute = rtenr;
		}
	}
	if (!bestcount)
		return;
	m_route.erase(m_route.end() - bestcount, m_route.end());
}

void IcaoFlightPlan::redo_route_names(void)
{
	for (route_t::iterator ri(m_route.begin()), re(m_route.end()); ri != re; ++ri) {
		Waypoint& wpt(*ri);
		FindCoord f(m_engine);
		unsigned int acceptflags(wpt.is_ifr() ? (FindCoord::flag_navaid | FindCoord::flag_waypoint)
					 : (FindCoord::flag_airport | FindCoord::flag_navaid | FindCoord::flag_waypoint));
		if (true)
			std::cerr << "IcaoFlightPlan::redo_route_names: try " << wpt.get_icao() << ' ' << wpt.get_name() << std::endl;
		bool ok(f.find(wpt.get_icao(), wpt.get_name(), acceptflags));
		ok = ok && (wpt.get_coord().spheric_distance_km(f.get_coord()) < 0.1);
		if (!ok) {
			if (true)
				std::cerr << "IcaoFlightPlan::redo_route_names: try " << wpt.get_coord().get_lat_str2()
					  << ' ' << wpt.get_coord().get_lon_str2() << std::endl;
			ok = f.find(wpt.get_coord(), acceptflags, 0.1);
		}
		if (ok) {
			if (true)
				std::cerr << "IcaoFlightPlan::redo_route_names: set " << wpt.get_icao() << ' ' << wpt.get_name() << std::endl;
			wpt.set_icao(f.get_icao());
			wpt.set_name(f.get_name());
			continue;
		}
		wpt.set_icao("");
		wpt.set_name("");
	}
}

void IcaoFlightPlan::find_airways(void)
{
	if (m_route.size() < 2)
		return;
	// add airways to IFR segments
	static const float awy_tolerance(0.1);
	Engine::AirwayGraphResult::Graph areagraph;
	{
		FindCoord f(m_engine);
		f.find_area(get_bbox().oversize_nmi(200));
		f.get_airwaygraph(areagraph);
	}
	for (route_t::iterator ri(m_route.begin()), re(m_route.end()); ri != re; ) {
		Waypoint& wpt1(*ri);
		++ri;
		if (ri == re)
			break;
		if (!wpt1.is_ifr())
			continue;
		switch (wpt1.get_pathcode()) {
		case FPlanWaypoint::pathcode_directto:
			wpt1.set_pathname("");
			continue;

		case FPlanWaypoint::pathcode_airway:
		case FPlanWaypoint::pathcode_sid:
		case FPlanWaypoint::pathcode_star:
			if (!wpt1.get_pathname().empty())
				continue;
			/* fall through */

		default:
			wpt1.set_pathcode(FPlanWaypoint::pathcode_none);
		case FPlanWaypoint::pathcode_none:
			wpt1.set_pathname("");
			break;
		}
		Waypoint& wpt2(*ri);
		Engine::AirwayGraphResult::Graph::vertex_descriptor u1;
		if (!areagraph.find_nearest(u1, wpt1.get_coord(),
					    Engine::AirwayGraphResult::Vertex::typemask_navaid |
					    Engine::AirwayGraphResult::Vertex::typemask_intersection |
					    Engine::AirwayGraphResult::Vertex::typemask_undefined,
					    true, false)) {
			if (true)
				std::cerr << "Point not found: " << wpt1.get_icao_or_name() << ' '
					  << wpt1.get_coord().get_lat_str2() << ' ' << wpt1.get_coord().get_lon_str2() << std::endl;
			continue;
		}
		const Engine::AirwayGraphResult::Vertex& v1(areagraph[u1]);
		if ((v1.get_ident() != wpt1.get_icao_or_name() && !wpt1.get_icao_or_name().empty()) ||
		    wpt1.get_coord().spheric_distance_nmi(v1.get_coord()) > awy_tolerance) {
			if (true)
				std::cerr << "Point found for: " << wpt1.get_icao_or_name() << ' '
					  << wpt1.get_coord().get_lat_str2() << ' ' << wpt1.get_coord().get_lon_str2()
					  << " has different ID or is too far away: " << v1.get_ident() << ' '
					  << v1.get_coord().get_lat_str2() << ' ' << v1.get_coord().get_lon_str2() << std::endl;
			continue;
		}
		Engine::AirwayGraphResult::Graph::vertex_descriptor u2;
		if (!areagraph.find_nearest(u2, wpt2.get_coord(),
					    Engine::AirwayGraphResult::Vertex::typemask_navaid |
					    Engine::AirwayGraphResult::Vertex::typemask_intersection |
					    Engine::AirwayGraphResult::Vertex::typemask_undefined,
					    false, true)) {
			if (true)
				std::cerr << "Point not found: " << wpt2.get_icao_or_name() << ' '
					  << wpt2.get_coord().get_lat_str2() << ' ' << wpt2.get_coord().get_lon_str2() << std::endl;
			continue;
		}
		const Engine::AirwayGraphResult::Vertex& v2(areagraph[u2]);
		if (u1 == u2) {
			if (true)
				std::cerr << "Points: " << v1.get_ident() << ' '
					  << v1.get_coord().get_lat_str2() << ' ' << v1.get_coord().get_lon_str2()
					  << " and: " << v2.get_ident() << ' '
					  << v2.get_coord().get_lat_str2() << ' ' << v2.get_coord().get_lon_str2()
					  << " are the same" << std::endl;
			continue;
		}
		if ((v2.get_ident() != wpt2.get_icao_or_name() && !wpt2.get_icao_or_name().empty()) ||
		    wpt2.get_coord().spheric_distance_nmi(v2.get_coord()) > awy_tolerance) {
			if (true)
				std::cerr << "Point found for: " << wpt2.get_icao_or_name() << ' '
					  << wpt2.get_coord().get_lat_str2() << ' ' << wpt2.get_coord().get_lon_str2()
					  << " has different ID or is too far away: " << v2.get_ident() << ' '
					  << v2.get_coord().get_lat_str2() << ' ' << v2.get_coord().get_lon_str2() << std::endl;
			continue;
		}
		std::vector<Engine::AirwayGraphResult::Graph::vertex_descriptor> predecessors;
		areagraph.shortest_paths(u1, predecessors);
		Engine::AirwayGraphResult::Graph::vertex_descriptor uu2(u2);
		Engine::AirwayGraphResult::Graph::vertex_descriptor uu1(predecessors[uu2]);
		if (uu1 == uu2) {
			if (true)
				std::cerr << "No Path from point: " << v1.get_ident() << ' '
					  << v1.get_coord().get_lat_str2() << ' ' << v1.get_coord().get_lon_str2()
					  << " to point: " << v2.get_ident() << ' '
					  << v2.get_coord().get_lat_str2() << ' ' << v2.get_coord().get_lon_str2() << std::endl;
       			continue;
		}
		nameset_t awynames;
		for (bool first = true; uu2 != u1; uu2 = uu1, uu1 = predecessors[uu2]) {
			nameset_t an;
			int a((wpt1.get_altitude() + 50) / 100);
			Engine::AirwayGraphResult::Graph::out_edge_iterator ei0, ei1;
#if 0
			tie(ei0, ei1) = boost::edge_range(uu1, uu2, areagraph);
#else
			tie(ei0, ei1) = boost::out_edges(uu1, areagraph);
#endif
			for (; ei0 != ei1; ++ei0) {
#if 1
				if (boost::target(*ei0, areagraph) != uu2)
					continue;
#endif
				const Engine::AirwayGraphResult::Edge& e(areagraph[*ei0]);
				if (!e.is_level_ok(a)) {
					if (true)
						std::cerr << "Skipping: " << v1.get_ident() << "->" << v2.get_ident() << ": " << e.get_ident()
							  << ": level " << a << " out of range " << e.get_base_level() << "..." << e.get_top_level() << std::endl;
					continue;
				}
				if (true)
					std::cerr << v1.get_ident() << "->" << v2.get_ident() << ": " << e.get_ident() << std::endl;
				an.insert(e.get_ident());
			}

			if (first) {
				awynames.swap(an);
				first = false;
				continue;
			}
			awynames = intersect(awynames, an);
			if (awynames.empty())
				break;
		}
		if (awynames.empty()) {
			if (true)
				std::cerr << "No single airway found from " << v1.get_ident() << " to " << v2.get_ident() << std::endl;
			continue;
		}
		wpt1.set_pathcode(FPlanWaypoint::pathcode_airway);
		wpt1.set_pathname(*awynames.begin());
		if (wpt1.get_name().empty())
			wpt1.set_name(v1.get_ident());
		if (wpt2.get_name().empty())
			wpt2.set_name(v2.get_ident());
	}
}

void IcaoFlightPlan::fix_max_dct_distance(double dct_limit)
{
	// check DCT limit
	for (route_t::iterator ri(m_route.end()), rb(m_route.begin()); ri != rb; ) {
		--ri;
		if (ri == rb)
			break;
		Waypoint& wpt2(*ri);
		route_t::iterator ri1(ri);
		--ri1;
		Waypoint& wpt1(*ri1);
		// VFR or DCT - check for too long segments (limit at 50nmi)
		if (!wpt1.is_ifr() || wpt1.get_pathname().empty()) {
			double dist(wpt1.get_coord().spheric_distance_nmi_dbl(wpt2.get_coord()));
			if (dist <= dct_limit)
				continue;
	                float crs(wpt1.get_coord().spheric_true_course(wpt2.get_coord()));
			unsigned int nseg(ceil(dist / dct_limit) + 0.1);
			if (nseg > 1)
				dist /= nseg;
			while (nseg > 1) {
				--nseg;
				Waypoint w(wpt1);
				w.set_coord(wpt1.get_coord().spheric_course_distance_nmi(crs, dist * nseg));
				w.set_name("");
				w.set_icao("");
				ri = m_route.insert(ri, w);
				rb = m_route.begin();
			}
			continue;
		}
	}
}

void IcaoFlightPlan::erase_unnecessary_airway(bool keep_turnpoints, bool include_dct)
{
	for (unsigned int nr = 0; nr + 2U < m_route.size(); ) {
		unsigned int nr1(nr);
		for (; nr1 + 2U < m_route.size(); ++nr1) {
			const Waypoint& wpt2(m_route[nr1 + 2]);
			const Waypoint& wpt1(m_route[nr1 + 1]);
			const Waypoint& wpt0(m_route[nr]);
			if (!(wpt0.get_flags() & wpt1.get_flags() & FPlanWaypoint::altflag_ifr) ||
			    ((wpt0.get_flags() ^ wpt1.get_flags()) & FPlanWaypoint::altflag_standard) ||
			    (wpt0.get_altitude() != wpt1.get_altitude()))
				break;
			if ((wpt0.get_pathcode() != FPlanWaypoint::pathcode_airway || wpt1.get_pathcode() != FPlanWaypoint::pathcode_airway ||
			     wpt0.get_pathname().empty() || wpt0.get_pathname() != wpt1.get_pathname()) &&
			    (!include_dct || wpt0.get_pathcode() != FPlanWaypoint::pathcode_directto || wpt1.get_pathcode() != FPlanWaypoint::pathcode_directto))
				break;
			if (keep_turnpoints || wpt0.get_pathcode() == FPlanWaypoint::pathcode_directto) {
				static const float maxdev(0.5);
				bool ok(true);
				for (unsigned int i = nr; i <= nr1; ) {
					++i;
					const Waypoint& wptx(m_route[i]);
					Point pt(wptx.get_coord().spheric_nearest_on_line(wpt0.get_coord(), wpt2.get_coord()));
					if (pt.spheric_distance_nmi(wptx.get_coord()) <= maxdev)
						continue;
					ok = false;
					break;
				}
				if (!ok)
					break;
			}
		}
		if (nr1 == nr) {
			++nr;
			continue;
		}
		while (nr1 > nr) {
			m_route.erase(m_route.begin() + nr1);
			--nr1;
		}
	}
}

void IcaoFlightPlan::add_eet(void)
{
	bool clr(true);
	for (route_t::iterator ri(m_route.begin()), re(m_route.end()); ri != re; ++ri) {
		time_t t(ri->get_time());
		if (!t)
			continue;
		struct tm tm;
		if (!gmtime_r(&t, &tm))
			continue;
		const std::string& ident(ri->get_icao_or_name());
		if (Engine::AirwayGraphResult::Vertex::is_ident_numeric(ident) ||
		    ident.size() < 2)
			continue;
		if (clr)
			otherinfo_clear("EET");
		clr = false;
		std::ostringstream oss;
		if (!ident.empty())
			oss << upcase(ident);
		else
			oss << ri->get_coordstr();
		oss << std::setw(2) << std::setfill('0') << tm.tm_hour
		    << std::setw(2) << std::setfill('0') << tm.tm_min;
		otherinfo_add("EET", oss.str());
	}
}

std::vector<std::string> IcaoFlightPlan::tokenize(const std::string& s)
{
	std::vector<std::string> r;
	for (std::string::const_iterator si(s.begin()), se(s.end()); si != se; ) {
		if (std::isspace(*si)) {
			++si;
			continue;
		}
		std::string::const_iterator si2(si);
		++si2;
		while (si2 != se && !std::isspace(*si2))
			++si2;
		r.push_back(std::string(si, si2));
		si = si2;
	}
	return r;
}

std::string IcaoFlightPlan::untokenize(const std::vector<std::string>& s)
{
	bool space(false);
	std::string r;
	for (std::vector<std::string>::const_iterator si(s.begin()), se(s.end()); si != se; ++si) {
		if (space)
			r.push_back(' ');
		else
			space = true;
		r += *si;
	}
	return r;
}

bool IcaoFlightPlan::replace_profile(const FPlanRoute::Profile& p)
{
	std::vector<std::string> rmk(tokenize(otherinfo_get("RMK")));
	bool chg(false);
	for (std::vector<std::string>::iterator ri(rmk.begin()), re(rmk.end()); ri != re;) {
		if (ri->compare(0, 4, "BOC:") && ri->compare(0, 4, "TOC:") &&
		    ri->compare(0, 4, "BOD:") && ri->compare(0, 4, "TOD:") &&
		    ri->compare(0, 4, "DAL:") && ri->compare(0, 5, "TAXI:")) {
			++ri;
			continue;
		}
		ri = rmk.erase(ri);
		re = rmk.end();
		chg = true;
	}
	for (FPlanRoute::Profile::const_iterator pi(p.begin()), pe(p.end()); pi != pe; ++pi) {
		if (pi->get_type() == FPlanRoute::LevelChange::type_invalid)
			continue;
		unsigned int min((pi->get_flttime() + 30) / 60);
		unsigned int hour(min / 60);
		min -= hour * 60;
		std::ostringstream oss;
		oss << to_str(pi->get_type()) << ":D" << Point::round<int32_t,double>(pi->get_dist())
		    << 'F' << std::setw(3) << std::setfill('0') << ((pi->get_alt() + 50)/100)
		    << 'T' << std::setw(2) << std::setfill('0') << hour << std::setw(2) << std::setfill('0') << min;
		rmk.push_back(oss.str());
		chg = true;
	}
	if (!std::isnan(p.get_dist())) {
		int32_t d(Point::round<int32_t,double>(p.get_dist()));
		std::ostringstream oss;
		oss << "DAL:D" << d << "AD";
		if (AirportsDb::Airport::is_fpl_zzzz(get_destination()))
			oss << "ZZZZ";
		else
			oss << get_destination();
		rmk.push_back(oss.str());
		chg = true;
	}
	for (FPlanRoute::Profile::const_distat_iterator i(p.begin_distat()), e(p.end_distat()); i != e; ++i) {
		unsigned int x(i->first);
		if (!x)
			continue;
		--x;
		if (x >= m_route.size() || std::isnan(i->second))
			continue;
		int32_t d(Point::round<int32_t,double>(i->second));
		std::ostringstream oss;
		oss << "DAL:D" << d << "PT";
		if (m_route[x].get_icao().empty())
			oss << m_route[x].get_name();
		else
			oss << m_route[x].get_icao();
		rmk.push_back(oss.str());
		chg = true;
	}
	if (!m_route.empty()) {
		time_t tt(m_route.front().get_time() - get_departuretime());
		if (tt >= 0 && tt <= 60*60) {
			std::ostringstream oss;
			oss << "TAXI:" << ((tt + 30) / 60);
			rmk.push_back(oss.str());
			chg = true;
		}
	}
	if (!chg)
		return false;
	otherinfo_clear("RMK");
	otherinfo_add("RMK", untokenize(rmk));
	return true;
}

bool IcaoFlightPlan::is_aiport_paris_tma(const std::string& icao) // Paris group airport
{
	if (icao.size() != 4)
		return false;
	if ((icao[0] != 'L' && icao[0] != 'l') ||
	    (icao[1] != 'F' && icao[1] != 'f') ||
	    (icao[2] != 'P' && icao[2] != 'p'))
		return false;
	switch (icao[3]) {
	default:
		return false;

	case 'B': // Le Bourget
	case 'b':
	case 'G': // Charles De Gaulle
	case 'g':
	case 'N': // Toussus Le Noble
	case 'n':
	case 'O': // Paris Orly
	case 'o':
	case 'T': // Pontoise Cormeilles En Vexin
	case 't':
	case 'V': // Villacoublay Velizy
	case 'v':
		return true;

	case 'C': // Creil
	case 'c':
	case 'M': // Melun Villaroche
	case 'm':
		return true;
	}
}

bool IcaoFlightPlan::is_aiport_lfpnv(const std::string& icao)
{
	if (icao.size() != 4)
		return false;
	if ((icao[0] != 'L' && icao[0] != 'l') ||
	    (icao[1] != 'F' && icao[1] != 'f') ||
	    (icao[2] != 'P' && icao[2] != 'p'))
		return false;
	switch (icao[3]) {
	default:
		return false;

	case 'N': // Toussus Le Noble
	case 'n':
	case 'V': // Villacoublay Velizy
	case 'v':
		return true;
	}
}

bool IcaoFlightPlan::is_aiport_lfob(const std::string& icao)
{
	if (icao.size() != 4)
		return false;
	if ((icao[0] != 'L' && icao[0] != 'l') ||
	    (icao[1] != 'F' && icao[1] != 'f') ||
	    (icao[2] != 'O' && icao[2] != 'o') ||
	    (icao[3] != 'B' && icao[3] != 'b'))
		return false;
	return true;
}

bool IcaoFlightPlan::is_route_pogo(const std::string& dep, const std::string& dest)
{
	if (is_aiport_lfob(dep))
		return is_aiport_lfpnv(dest);
	if (is_aiport_lfob(dest))
		return is_aiport_lfpnv(dep);
	return is_aiport_paris_tma(dep) && is_aiport_paris_tma(dest);
}

int32_t IcaoFlightPlan::get_route_pogo_alt(const std::string& dep, const std::string& dest)
{
	if (!is_route_pogo(dep, dest))
		return 0;
	static const struct pogoalt {
		const char *icao;
		int32_t alt;
	} pogoalt[] = {
		{ "LFOB", 4000 },
		{ "LFPB", 3000 },
		{ "LFPC", 3000 }, // ??
		{ "LFPG", 5000 },
		{ "LFPM", 3000 }, // ??
		{ "LFPN", 3000 },
		{ "LFPO", 5000 },
		{ "LFPT", 3000 },
		{ "LFPV", 3000 },
		{ 0, 3000 }
	};
	const struct pogoalt *pg = pogoalt;
	for (; pg->icao; ++pg)
		if (dep == pg->icao)
			return pg->alt;
	return 0;
}

void IcaoFlightPlan::normalize_pogo(void)
{
	static const std::string remark("RMK");
	static const std::string pogo("POGO");
	std::vector<std::string> tok(tokenize(otherinfo_get(remark)));
	if ((m_departureflags & m_destinationflags & FPlanWaypoint::altflag_ifr) &&
	    is_route_pogo()) {
		for (std::vector<std::string>::const_iterator i(tok.begin()), e(tok.end()); i != e; ++i)
			if (Glib::ustring(*i).uppercase() == pogo)
				return;
		otherinfo_add(remark, pogo);
		return;
	}
	bool chg(false);
	for (std::vector<std::string>::iterator i(tok.begin()), e(tok.end()); i != e; ++i) {
		if (Glib::ustring(*i).uppercase() != pogo)
			continue;
		i = tok.erase(i);
		e = tok.end();
		chg = true;
	}
	if (!chg)
		return;
	m_otherinfo[remark] = untokenize(tok);
}

std::string IcaoFlightPlan::get_fpl(bool line_breaks)
{
	// defaults
	static const std::string fpl_time = "1000";

	if (get_aircraftid().size() < 1 || get_aircraftid().size() > 7)
		return "Error: invalid aircraft ID: " + get_aircraftid();
	if (get_aircrafttype().size() < 2 || get_aircrafttype().size() > 4)
		return "Error: invalid aircraft type: " + get_aircrafttype();
	if (get_departure().size() != 4)
		return "Error: invalid departure: " + get_departure();
	if (get_destination().size() != 4)
		return "Error: invalid destination: " + get_destination();
	std::ostringstream fpl;
	fpl << "(FPL-" << get_aircraftid() << '-' << get_flightrules() << get_flighttype();
	if (line_breaks)
		fpl << std::endl;
	else
		fpl << ' ';
	fpl << '-';
	if (get_number() > 1U)
		fpl << get_number();
	fpl << get_aircrafttype() << '/' << get_wakecategory()
	    << " -" << get_equipment_string() << '/' << get_transponder_string();
	if (line_breaks)
		fpl << std::endl;
	else
		fpl << ' ';
	fpl << '-' << get_departure();
	{
		time_t t(get_departuretime());
		struct tm tm;
		if (gmtime_r(&t, &tm))
			fpl << std::setw(2) << std::setfill('0') << tm.tm_hour
			    << std::setw(2) << std::setfill('0') << tm.tm_min;
		else
			fpl << fpl_time;
	}
	if (line_breaks)
		fpl << std::endl;
	else
		fpl << ' ';
	fpl << '-' << get_item15();
	if (line_breaks)
		fpl << std::endl;
	else
		fpl << ' ';
	fpl << '-' << get_destination();
	{
		time_t t(get_totaleet());
		struct tm tm;
		if (gmtime_r(&t, &tm))
			fpl << std::setw(2) << std::setfill('0') << tm.tm_hour
			    << std::setw(2) << std::setfill('0') << tm.tm_min;
		else
			fpl << "0000";
	}
	if (!get_alternate1().empty())
		fpl << ' ' << get_alternate1();
	if (!get_alternate2().empty())
		fpl << ' ' << get_alternate2();
	if (line_breaks)
		fpl << std::endl;
	else
		fpl << ' ';
	fpl << '-';
	{
		bool pbndone(get_pbn() == Aircraft::pbn_none);
		bool subseq(false);
		for (otherinfo_t::const_iterator oi(m_otherinfo.begin()), oe(m_otherinfo.end()); oi != oe; ++oi) {
			if (subseq)
				fpl << ' ';
			subseq = true;
			fpl << oi->first << '/' << oi->second;
			if (!pbndone && oi->first == "PBN") {
				fpl << ' ' << get_pbn_string();
				pbndone = true;
			}
		}
		if (!pbndone) {
			if (subseq)
				fpl << ' ';
			subseq = true;
			fpl << "PBN/" << get_pbn_string();
		}
	}
	if (line_breaks)
		fpl << std::endl;
	else
		fpl << ' ';
	fpl << '-';
	{
		bool subseq(false);
		if (get_endurance()) {
			time_t t(get_endurance());
			struct tm tm;
			if (gmtime_r(&t, &tm)) {
				fpl << "E/" << std::setw(2) << std::setfill('0') << tm.tm_hour
				    << std::setw(2) << std::setfill('0') << tm.tm_min;
				subseq = true;
			}
		}
		if (get_personsonboard()) {
			if (subseq)
				fpl << ' ';
			fpl << "P/";
			if (get_personsonboard() == ~0U)
				fpl << "TBN";
			else
				fpl << std::setw(3) << std::setfill('0') << get_personsonboard();
			subseq = true;
		}
		if (get_emergencyradio()) {
			if (subseq)
				fpl << ' ';
			fpl << "R/" << get_emergencyradio_string();
			subseq = true;
		}
		if (get_survival()) {
			if (subseq)
				fpl << ' ';
			fpl << "S/" << get_survival_string();
			subseq = true;
		}
		if (get_lifejackets()) {
			if (subseq)
				fpl << ' ';
			fpl << "J/" << get_lifejackets_string();
			subseq = true;
		}
		if (get_dinghies()) {
			if (subseq)
				fpl << ' ';
			fpl << "D/" << get_dinghies_string();
			subseq = true;
		}
		if (!get_colourmarkings().empty()) {
			if (subseq)
				fpl << ' ';
			fpl << "A/" << get_colourmarkings();
			subseq = true;
		}
		if (!get_remarks().empty()) {
			if (subseq)
				fpl << ' ';
			fpl << "N/" << get_remarks();
			subseq = true;
		}
		if (!get_picname().empty()) {
			if (subseq)
				fpl << ' ';
			fpl << "C/" << get_picname();
			subseq = true;
		}
	}
	fpl << ')';
	return fpl.str();
}

std::string IcaoFlightPlan::get_item15(void)
{
	std::ostringstream fpl;
	uint16_t prevflags(m_departureflags);
	int16_t prevalt(m_defaultalt);
	if (!m_route.empty()) {
		prevflags ^= (prevflags ^ m_route[0].get_flags()) & FPlanWaypoint::altflag_standard;
		prevalt = m_route[0].get_altitude();
	}
	if (!get_cruisespeed().empty()) {
		int alt(prevalt);
		int cs(Point::round<int,float>(get_cruisespeed(alt)));
		fpl << 'N' << std::setw(4) << std::setfill('0') << cs;
	} else {
		fpl << get_cruisespeed();
	}
	if (!(prevflags & FPlanWaypoint::altflag_ifr)) {
		fpl << "VFR";
	} else {
		fpl << ((prevflags & FPlanWaypoint::altflag_standard) ? 'F' : 'A')
		    << std::setw(3) << std::setfill('0') << ((prevalt + 50) / 100);
	}
	if ((m_departureflags & FPlanWaypoint::altflag_ifr) && !m_sid.empty())
		fpl << ' ' << m_sid;
	{
		for (route_t::const_iterator ri(m_route.begin()), re(m_route.end()); ri != re;) {
			const Waypoint& wpt(*ri);
			const Glib::ustring& ident(wpt.get_icao_or_name());
			// skip intermediate waypoints with non-flightplannable names
			if ((ident.empty() || Engine::AirwayGraphResult::Vertex::is_ident_numeric(ident) || ident.size() < 2) &&
			    wpt.get_pathcode() == FPlanWaypoint::pathcode_airway && ri != m_route.begin() && ri != m_route.end()) {
				route_t::const_iterator ri1(ri);
				--ri1;
				const Waypoint& wptprev(*ri1);
				if (wptprev.get_pathcode() == FPlanWaypoint::pathcode_airway &&
				    wpt.get_pathname() == wptprev.get_pathname() &&
				    (wpt.get_flags() & wptprev.get_flags() & FPlanWaypoint::altflag_ifr)) {
					++ri;
					continue;
				}
			}
			++ri;
			fpl << ' ';
			if (!ident.empty())
				fpl << upcase(ident);
			else
				fpl << wpt.get_coordstr();
			{
				// change of level or speed and/or change of rules
				bool lvlchg((prevalt != wpt.get_altitude() ||
					     ((prevflags ^ wpt.get_flags()) & FPlanWaypoint::altflag_standard) ||
					     ((wpt.get_flags() & ~prevflags) & FPlanWaypoint::altflag_ifr)) &&
					    ((wpt.get_flags() | prevflags) & FPlanWaypoint::altflag_ifr));
				bool rulechg((prevflags ^ wpt.get_flags()) & FPlanWaypoint::altflag_ifr);
				prevalt = wpt.get_altitude();
				prevflags = wpt.get_flags();
				if (lvlchg) {
					fpl << '/';
					if (!get_cruisespeed().empty()) {
						int alt(prevalt);
						int cs(Point::round<int,float>(get_cruisespeed(alt)));
						fpl << 'N' << std::setw(4) << std::setfill('0') << cs;
					} else {
						fpl << get_cruisespeed();
					}
					fpl << ((prevflags & FPlanWaypoint::altflag_standard) ? 'F' : 'A')
					    << std::setw(3) << std::setfill('0') << ((prevalt + 50) / 100);
				}
				if (rulechg)
					fpl << ' ' << ((prevflags & FPlanWaypoint::altflag_ifr) ? 'I' : 'V') << "FR";
			}
			if (ri == re)
				break;
			if (true) {
				switch (wpt.get_pathcode()) {
				case FPlanWaypoint::pathcode_airway:
					fpl << ' ' << wpt.get_pathname();
					break;

				case FPlanWaypoint::pathcode_directto:
					fpl << " DCT";
					break;

				default:
					break;
				}
			} else if (prevflags & FPlanWaypoint::altflag_ifr) {
				if (wpt.get_pathname().empty())
					fpl << " DCT";
				else
					fpl << ' ' << wpt.get_pathname();
			}
		}
	}
	if (m_route.empty() && (m_departureflags & m_destinationflags & FPlanWaypoint::altflag_ifr))
		fpl << " DCT";
	if ((prevflags ^ m_destinationflags) & FPlanWaypoint::altflag_ifr)
		fpl << ' ' << ((m_destinationflags & FPlanWaypoint::altflag_ifr) ? 'I' : 'V') << "FR";
	if ((m_destinationflags & FPlanWaypoint::altflag_ifr) && !m_star.empty())
		fpl << ' ' << m_star;
	return fpl.str();
}

void IcaoFlightPlan::set_route(FPlanRoute& route)
{
	route.clear_wpt();
	route.set_time_offblock_unix(get_departuretime());
	route.set_time_onblock_unix(get_departuretime() + get_totaleet());
	{
		FPlanWaypoint wpt;
		wpt.set_icao(get_departure());
		wpt.set_name(otherinfo_get("DEP"));
		if (get_departure() == "ZZZZ")
			wpt.set_icao("");
		wpt.set_type(FPlanWaypoint::type_airport);
		FindCoord f(m_engine);
		if (f.find(wpt.get_icao(), wpt.get_name(), FindCoord::flag_airport)) {
			wpt.set_icao(f.get_icao());
			wpt.set_name(f.get_name());
			wpt.set_coord(f.get_coord());
			AirportsDb::Airport arpt;
			if (f.get_airport(arpt))
				wpt.set(arpt);
		}
		wpt.set_pathcode(FPlanWaypoint::pathcode_none);
		wpt.set_pathname("");
		wpt.set_time_unix(get_departuretime());
		wpt.set_flags(m_departureflags);
		wpt.set_truealt(wpt.get_altitude());
		route.insert_wpt(~0, wpt);
	}
	for (route_t::const_iterator ri(m_route.begin()), re(m_route.end()); ri != re; ++ri) {
		const Waypoint& wpt(*ri);
		route.insert_wpt(~0, wpt.get_fplwpt());
	}
	{
		FPlanWaypoint wpt;
		wpt.set_icao(get_destination());
		wpt.set_name(otherinfo_get("DEST"));
		if (get_departure() == "ZZZZ")
			wpt.set_icao("");
		wpt.set_type(FPlanWaypoint::type_airport);
		FindCoord f(m_engine);
		if (f.find(wpt.get_icao(), wpt.get_name(), FindCoord::flag_airport)) {
			wpt.set_icao(f.get_icao());
			wpt.set_name(f.get_name());
			wpt.set_coord(f.get_coord());
			AirportsDb::Airport arpt;
			if (f.get_airport(arpt))
				wpt.set(arpt);
		}
		wpt.set_pathcode(FPlanWaypoint::pathcode_none);
		wpt.set_pathname("");
		wpt.set_time_unix(get_departuretime() + get_totaleet());
		wpt.set_flags(m_destinationflags);
		wpt.set_truealt(wpt.get_altitude());
		route.insert_wpt(~0, wpt);
	}
	if (route.get_nrwpt() >= 2U && (m_departureflags & FPlanWaypoint::altflag_ifr) && !m_sid.empty()) {
		FPlanWaypoint& wpt(route[0]);
		if (m_sid == "DCT") {
			wpt.set_pathcode(FPlanWaypoint::pathcode_directto);
			wpt.set_pathname("");
		} else {
			wpt.set_pathcode(FPlanWaypoint::pathcode_sid);
			wpt.set_pathname(m_sid);
		}
	}
	if (route.get_nrwpt() >= 3U && (m_destinationflags & FPlanWaypoint::altflag_ifr) && !m_star.empty()) {
		FPlanWaypoint& wpt(route[route.get_nrwpt() - 2U]);
		if (m_star == "DCT") {
			wpt.set_pathcode(FPlanWaypoint::pathcode_directto);
			wpt.set_pathname("");
		} else {
			wpt.set_pathcode(FPlanWaypoint::pathcode_star);
			wpt.set_pathname(m_star);
		}
	}
	route.recompute_dist();
	route.recompute_decl();
	for (unsigned int i = 0U, n = route.get_nrwpt(); i < n; ++i) {
		FPlanWaypoint& wpt(route[i]);
		wpt.set_trueheading(wpt.get_truetrack());
	}
}

void IcaoFlightPlan::recompute_eet(void)
{
	if (m_cruisespeeds.empty())
		return;
	time_t eet(0);
	for (route_t::iterator ri(m_route.begin()), re(m_route.end()); ri != re;) {
		route_t::iterator ri0(ri);
		++ri;
		if (ri == re)
			break;
		int alt(ri0->get_altitude() + ri->get_altitude());
		alt >>= 1;
		float tas(get_cruisespeed(alt));
		if (std::isnan(tas) || tas <= 0)
			return;
		float d(ri0->get_coord().spheric_distance_nmi(ri->get_coord()));
		float t(d / tas * 3600.0);
		eet += t;
	}
	set_totaleet(eet);
}

float IcaoFlightPlan::get_cruisespeed(int& alt) const
{
	if (m_cruisespeeds.empty())
		return std::numeric_limits<float>::quiet_NaN();
	cruisespeeds_t::const_iterator i1(m_cruisespeeds.lower_bound(alt));
	if (i1 == m_cruisespeeds.end()) {
		--i1;
		alt = i1->first;
		return i1->second;
	}
	if (i1->first == alt)
		return i1->second;
	if (i1 == m_cruisespeeds.begin()) {
		alt = i1->first;
		return i1->second;
	}
	cruisespeeds_t::const_iterator i0(i1);
	--i0;
	return (i0->second * (i1->first - alt) + i1->second * (alt - i0->first)) / static_cast<float>(i1->first - i0->first);
}

void IcaoFlightPlan::set_aircraft(const Aircraft& acft, const Aircraft::Cruise::EngineParams& ep)
{
	if (!acft.get_callsign().empty())
		set_aircraftid(acft.get_callsign());
	if (!acft.get_icaotype().empty()) {
		otherinfo_clear("TYP");
		if (acft.get_icaotype() == "P28T") {
			set_aircrafttype("ZZZZ");
			otherinfo_add("TYP", acft.get_icaotype());
		} else {
			set_aircrafttype(acft.get_icaotype());
		}
	}
	set_wakecategory(acft.get_wakecategory());
	if (acft.get_nav() || acft.get_com()) {
		set_nav(acft.get_nav());
		set_com(acft.get_com());
	}
	if (acft.get_transponder())
		set_transponder(acft.get_transponder());
	set_pbn(acft.get_pbn());
	if (acft.get_emergencyradio())
		set_emergencyradio(acft.get_emergencyradio());
	if (acft.get_survival())
		set_survival(acft.get_survival());
	if (acft.get_lifejackets())
		set_lifejackets(acft.get_lifejackets());
	if (acft.get_dinghies()) {
		set_dinghies(acft.get_dinghies());
		set_dinghiesnumber(acft.get_dinghiesnumber());
		set_dinghiescapacity(acft.get_dinghiescapacity());
		set_dinghiescolor(acft.get_dinghiescolor());
	}
	if (!acft.get_picname().empty())
		set_picname(Aircraft::to_ascii(acft.get_picname()));
	for (Aircraft::otherinfo_const_iterator_t oii(acft.otherinfo_begin()), oie(acft.otherinfo_end()); oii != oie; ++oii)
		otherinfo_add(*oii);
	if (!acft.get_crewcontact().empty() && otherinfo_get("RMK").find("CREW CONTACT") == std::string::npos) {
		std::string cc;
		for (Glib::ustring::const_iterator i(acft.get_crewcontact().begin()), e(acft.get_crewcontact().end()); i != e; ++i)
			if (std::isalnum(*i) || *i == '+')
				cc.push_back(*i);
		if (!cc.empty())
			otherinfo_add("RMK", "CREW CONTACT " + cc);
	}
	set_cruisespeed("");
	{
		m_cruisespeeds.clear();
		double avgff(0), avgtas(0);
		unsigned int avgnr(0);
		for (double pa = 0; pa < 60000; pa += 500) {
			Aircraft::Cruise::EngineParams ep1(ep);
			double pa1(pa), tas(0), fuelflow(0), mass(acft.get_mtom()), isaoffs(0), qnh(IcaoAtmosphere<double>::std_sealevel_pressure);
			acft.calculate_cruise(tas, fuelflow, pa1, mass, isaoffs, qnh, ep1);
			if (pa1 < pa - 100 || tas <= 0)
				break;
			m_cruisespeeds.insert(std::make_pair(static_cast<int>(pa1), static_cast<float>(tas)));
			if (pa < 5000)
				continue;
			avgff += fuelflow;
			avgtas += tas;
			++avgnr;
		}
		if (avgnr) {
			avgff /= avgnr;
			avgtas /= avgnr;
			// compute total fuel content
			double fuel(acft.get_useable_fuel());
			if (true)
				std::cerr << "Useable fuel volume: " << fuel << " average cruise fuel flow: " << avgff << std::endl;
			if (avgff > 0 && !std::isnan(fuel) && fuel > 0) {
				fuel /= avgff;
				m_endurance = fuel * 3600.0;
			}
		}
	}
	set_colourmarkings(Aircraft::to_ascii(acft.get_colormarking()));
}

#if 0

(FPL-OEABC-VG
-DV20/L-S/C
-LOWI0800
-N0090VFR WHISKEY
-LOWI0001
-STS/EET0200 , RMK/THIS IS A REMARK DOF/051102
-E/0500 P/003 R/E J/F
A/RED WHITE
C/HUBER)

(FPL-OEABC-VG
-C150/L-OV/C
-LOGG1220
-N0080VFR LH MK VM
-LOWK0130 LOWG
-DOF/050719)

(FPL-OEABC-IN
-A321/M-SEHRWY/S
-LOWW0915
-N0458F340 OSPEN UM984 BZO/N0458F350 UM984 NEDED/N0456F370 UM984
DIVKO UN852 VERSO UL129 LUNIK
-LEPA0157 LEIB
-EET/LIMM0030 LFFF0106 LECB0135 SEL/APHL OPR/TESTAIR DOF/050719
RVR/75 ORGN/LOWWZPZX)

(FPL-OEABC-IS
-B772/H-SEGHIJRWXY/S
-LOWW0913
-N0480F350 ABLOM UL725 ALAMU UY33 NARKA UB499 REBLA UL601 CND UM747
SOBLO/N0490F350 B143 VEKAT UM747 DUKAN B449 DURDY/N0490F370 B449 RJ
A240 AFGAN W615A BUROT B143D ABDAN N644 DI A466 ASARI/N0490F370
A466E DPN L759S AGG L759 KKJ/N0490F370 L759 IBUDA/N0490F370 L759
LIBDI/N0490F370 L759 NISUN/N0490F370 L759 MIPAK/N0490F370 L759 PUT
R325 VIH A464 VBA
-WMKK1046 WSSS
-EET/UKFV0122 URRV0153 UGGG0212 UBBA0244 UTAK0311 UTAA0332 OAKX0426
OPLR0504 VIDF0540 VABF0639 VECF0700 VYYF0838 VOMF0842 VYYF0903
VTBB0925 WMFC1003 SEL/AEBQ OPR/ABC AIR PER/C DAT/SV RALT/VECC
VTSP RMK/AGCS DOF/050719 ORGN/LOWWZPZX)

(FPL-OEABC-IS
-CRJ2/M-SERWY/S
-LOWS0550
-N0450F340 TRAUN L856 MANAL UL856 DERAK UM982 TINIL
-LFPG0120 LFPO
-DOF/050719 RVR/200 ORGN/RPL)

#endif
