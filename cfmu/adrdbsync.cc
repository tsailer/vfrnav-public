//
// C++ Implementation: adrdbsync
//
// Description: AIXM Database/normal Database sync utility
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012, 2013, 2014
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include "adr.hh"
#include "adrdb.hh"
#include "adrgraph.hh"
#include "adrpoint.hh"
#include "adrroute.hh"
#include "adraerodrome.hh"

#include "getopt.h"

namespace {
	struct waypoint_sorter {
		bool operator()(const WaypointsDb::Waypoint& a, const WaypointsDb::Waypoint& b) const {
			return a.get_name() < b.get_name();
		}
	};
};

static void sync_waypoints(ADR::Database& db, WaypointsDb& waypointsdb, ADR::timetype_t dbtime, bool verbose)
{
	typedef std::multiset<WaypointsDb::Waypoint,waypoint_sorter> oldwpts_t;
	oldwpts_t oldwpts;
	{
		WaypointsDb::Waypoint el;
		waypointsdb.loadfirst(el, false, WaypointsDb::Waypoint::subtables_all);
		while (el.is_valid()) {
			oldwpts.insert(el);
			waypointsdb.loadnext(el, false, WaypointsDb::Waypoint::subtables_all);
		}
	}
	ADR::Database::findresults_t r(db.find_all(ADR::Database::loadmode_link, dbtime, dbtime + 1,
						   ADR::Object::type_designatedpoint, ADR::Object::type_designatedpoint, 0));
	waypointsdb.purgedb();
	for (ADR::Database::findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
		const ADR::DesignatedPointTimeSlice& ts(ri->get_obj()->operator()(dbtime).as_designatedpoint());
		if (!ts.is_valid())
			continue;
		WaypointsDb::Waypoint el;
		el.set_name(ts.get_ident());
		el.set_usage(WaypointsDb::Waypoint::usage_bothlevel);
		el.set_coord(ts.get_coord());
		el.set_sourceid(ri->to_str() + "@ADR");
		el.set_modtime(dbtime);
		el.set_label_placement(WaypointsDb::Waypoint::label_e);
		switch (ts.get_type()) {
		case ADR::DesignatedPointTimeSlice::type_icao:
			el.set_type(WaypointsDb::Waypoint::type_icao);
			break;

                case ADR::DesignatedPointTimeSlice::type_coord:
                        el.set_type(WaypointsDb::Waypoint::type_coord);
			break;

                case ADR::DesignatedPointTimeSlice::type_adrboundary:
                case ADR::DesignatedPointTimeSlice::type_adrreference:
			el.set_type(WaypointsDb::Waypoint::type_adhp);
			break;

		case ADR::DesignatedPointTimeSlice::type_terminal:
		case ADR::DesignatedPointTimeSlice::type_user:
			el.set_type(WaypointsDb::Waypoint::type_other);
			break;

		case ADR::DesignatedPointTimeSlice::type_invalid:
		default:
			el.set_type(WaypointsDb::Waypoint::type_unknown);
			break;
		}
		{
			oldwpts_t::const_iterator ob(oldwpts.lower_bound(el));
			oldwpts_t::const_iterator oe(oldwpts.upper_bound(el));
			oldwpts_t::const_iterator oi(oe);
			double dist(std::numeric_limits<double>::max());
			for (; ob != oe; ++ob) {
				if (!ob->is_valid() || ob->get_name() != el.get_name())
					continue;
				double d(ob->get_coord().spheric_distance_nmi_dbl(el.get_coord()));
				if (d > dist)
					continue;
				dist = d;
				oi = ob;
			}
			if (oi != oe && dist < 5) {
				el.set_usage(oi->get_usage());
				el.set_sourceid(oi->get_sourceid());
				el.set_authorityset(oi->get_authorityset());
				el.set_label_placement(oi->get_label_placement());
				oldwpts.erase(oi);
			}
		}
		if (verbose)
			std::cout << "Saving waypoint " << (std::string)el.get_name() << ' ' << el.get_coord().get_lat_str2()
				  << ' ' << el.get_coord().get_lon_str2() << ' ' << el.get_sourceid() << std::endl;
		waypointsdb.save(el);
	}
}

namespace {
	struct navaid_sorter {
		bool operator()(const NavaidsDb::Navaid& a, const NavaidsDb::Navaid& b) const {
			return a.get_icao() < b.get_icao();
		}
	};
};

static void sync_navaids(ADR::Database& db, NavaidsDb& navaidsdb, ADR::timetype_t dbtime, bool verbose)
{
	typedef std::multiset<NavaidsDb::Navaid,navaid_sorter> oldnas_t;
	oldnas_t oldnas;
	{
		NavaidsDb::Navaid el;
		navaidsdb.loadfirst(el, false, NavaidsDb::Navaid::subtables_all);
		while (el.is_valid()) {
			oldnas.insert(el);
			navaidsdb.loadnext(el, false, NavaidsDb::Navaid::subtables_all);
		}
	}
	ADR::Database::findresults_t r(db.find_all(ADR::Database::loadmode_link, dbtime, dbtime + 1,
						   ADR::Object::type_navaid, ADR::Object::type_navaid, 0));
	navaidsdb.purgedb();
	for (ADR::Database::findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
		const ADR::NavaidTimeSlice& ts(ri->get_obj()->operator()(dbtime).as_navaid());
		if (!ts.is_valid())
			continue;
		NavaidsDb::Navaid el;
		el.set_icao(ts.get_ident());
		el.set_name(ts.get_name());
		el.set_coord(ts.get_coord());
		el.set_dmecoord(ts.get_coord());
		el.set_elev(ts.get_elev());
		el.set_range(0);
		el.set_frequency(0);
		el.set_inservice(true);
		el.set_sourceid(ri->to_str() + "@ADR");
		el.set_modtime(dbtime);
		el.set_label_placement(NavaidsDb::Navaid::label_e);
		switch (ts.get_type()) {
		case ADR::NavaidTimeSlice::type_vor:
  			el.set_navaid_type(NavaidsDb::Navaid::navaid_vor);
			break;

		case ADR::NavaidTimeSlice::type_vor_dme:
			el.set_navaid_type(NavaidsDb::Navaid::navaid_vordme);
			break;

		case ADR::NavaidTimeSlice::type_vortac:
			el.set_navaid_type(NavaidsDb::Navaid::navaid_vortac);
			break;

		case ADR::NavaidTimeSlice::type_tacan:
			el.set_navaid_type(NavaidsDb::Navaid::navaid_tacan);
			break;

		case ADR::NavaidTimeSlice::type_dme:
			el.set_navaid_type(NavaidsDb::Navaid::navaid_dme);
			break;

		case ADR::NavaidTimeSlice::type_ndb:
		case ADR::NavaidTimeSlice::type_ndb_mkr:
			el.set_navaid_type(NavaidsDb::Navaid::navaid_ndb);
			break;

		case ADR::NavaidTimeSlice::type_ndb_dme:
			el.set_navaid_type(NavaidsDb::Navaid::navaid_ndbdme);
			break;

		case ADR::NavaidTimeSlice::type_ils:
		case ADR::NavaidTimeSlice::type_ils_dme:
		case ADR::NavaidTimeSlice::type_loc:
		case ADR::NavaidTimeSlice::type_loc_dme:
		case ADR::NavaidTimeSlice::type_mkr:
		case ADR::NavaidTimeSlice::type_invalid:
		default:
			continue;
		}

		{
			oldnas_t::const_iterator ob(oldnas.lower_bound(el));
			oldnas_t::const_iterator oe(oldnas.upper_bound(el));
			oldnas_t::const_iterator oi(oe);
			double dist(std::numeric_limits<double>::max());
			for (; ob != oe; ++ob) {
				if (!ob->is_valid() || ob->get_icao() != el.get_icao())
					continue;
				double d(ob->get_coord().spheric_distance_nmi_dbl(el.get_coord()));
				if (d > dist)
					continue;
				dist = d;
				oi = ob;
			}
			if (oi != oe && dist < 5) {
				el.set_name(oi->get_name());
				el.set_dmecoord(oi->get_dmecoord());
				el.set_elev(oi->get_elev());
				el.set_range(oi->get_range());
				el.set_frequency(oi->get_frequency());
				el.set_inservice(oi->is_inservice());
				el.set_sourceid(oi->get_sourceid());
				el.set_authorityset(oi->get_authorityset());
				el.set_label_placement(oi->get_label_placement());
				oldnas.erase(oi);
			}
		}
		if (verbose)
			std::cout << "Saving navaid " << (std::string)el.get_icao() << ' ' << (std::string)el.get_name() << ' '
				  << el.get_coord().get_lat_str2() << ' ' << el.get_coord().get_lon_str2()
				  << ' ' << el.get_sourceid() << std::endl;
		navaidsdb.save(el);
	}
}

static void sync_airways(ADR::Database& db, AirwaysDb& airwaysdb, ADR::timetype_t dbtime, bool verbose)
{
	ADR::Database::findresults_t r(db.find_all(ADR::Database::loadmode_link, dbtime, dbtime + 1,
						   ADR::Object::type_routesegment, ADR::Object::type_routesegment, 0));
	airwaysdb.purgedb();
	for (ADR::Database::findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
		const ADR::RouteSegmentTimeSlice& ts(ri->get_obj()->operator()(dbtime).as_routesegment());
		if (!ts.is_valid())
			continue;
		const ADR::RouteTimeSlice& tsr(ts.get_route().get_obj()->operator()(dbtime).as_route());
		const ADR::PointIdentTimeSlice& tss(ts.get_start().get_obj()->operator()(dbtime).as_point());
		const ADR::PointIdentTimeSlice& tse(ts.get_end().get_obj()->operator()(dbtime).as_point());
		if (!tsr.is_valid() || !tss.is_valid() || !tse.is_valid())
			continue;
		AirwaysDb::Airway el;
		el.set_begin_coord(tss.get_coord());
		el.set_begin_name(tss.get_ident());
		el.set_end_coord(tse.get_coord());
		el.set_end_name(tse.get_ident());
		el.set_name(tsr.get_ident());
		if (ts.get_altrange().is_lower_valid())
			el.set_base_level((ts.get_altrange().get_lower_alt() + 99) / 100);
		else
			el.set_base_level(0);
		if (ts.get_altrange().is_upper_valid())
			el.set_top_level(ts.get_altrange().get_upper_alt() / 100);
		else
			el.set_top_level(999);
		el.set_terrain_elev(ts.get_terrainelev());
		el.set_corridor5_elev(ts.get_corridor5elev());
		if (el.get_top_level() <= 245)
			el.set_type(AirwaysDb::Airway::airway_low);
		else if (el.get_base_level() >= 245)
			el.set_type(AirwaysDb::Airway::airway_high);
		else
			el.set_type(AirwaysDb::Airway::airway_both);
		el.set_sourceid(ri->to_str() + "@ADR");
		el.set_modtime(dbtime);
		el.set_label_placement(AirwaysDb::Airway::label_e);
		if (verbose)
			std::cout << "Saving airway " << (std::string)el.get_begin_name() << ' ' << el.get_begin_coord().get_lat_str2()
				  << ' ' << el.get_begin_coord().get_lon_str2() << ' ' << (std::string)el.get_end_name() << ' '
				  << el.get_end_coord().get_lat_str2() << ' ' << el.get_end_coord().get_lon_str2() << ' '
				  << (std::string)el.get_name() << ' ' << el.get_sourceid() << std::endl;
		airwaysdb.save(el);
	}
}

namespace {
	struct route_sorter {
		bool operator()(const AirportsDb::Airport::VFRRoute& a, const AirportsDb::Airport::VFRRoute& b) const {
			if (!a.size() && b.size())
				return true;
			if (a.size() && !b.size())
				return false;
			if (a.size() && b.size()) {
				bool asid(a[a.size() - 1U].get_pathcode() == AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid);
				bool astar(a[0U].get_pathcode() == AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star);
				bool bsid(b[b.size() - 1U].get_pathcode() == AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid);
				bool bstar(b[0U].get_pathcode() == AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star);
				if (asid && bstar)
					return true;
				if (bsid && astar)
					return false;
			}
			if (a.get_name() < b.get_name())
				return true;
			if (b.get_name() < a.get_name())
				return false;
			return a.size() < b.size();
		}
	};
};

static void add_sidstar(std::set<AirportsDb::Airport::VFRRoute,route_sorter>& rtes, ADR::Database& db, const ADR::Object::ptr_t& proc, ADR::timetype_t dbtime)
{
	if (!proc)
		return;
	const ADR::StandardInstrumentTimeSlice& ts(proc->operator()(dbtime).as_sidstar());
	if (!ts.is_valid())
		return;
	ADR::Graph graph;
	{
		ADR::Database::findresults_t r(db.find_dependson(proc->get_uuid(), ADR::Database::loadmode_link, dbtime, dbtime + 1,
								 ADR::Object::type_departureleg, ADR::Object::type_arrivalleg, 0));
		for (ADR::Database::findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri)
			graph.add(dbtime, ri->get_obj(), 0);
	}
	AirportsDb::Airport::VFRRoute rte(ts.get_ident());
	ADR::Graph::vertex_descriptor v;
	{
		bool ok;
		boost::tie(v, ok) = graph.find_vertex(ts.get_airport());
		if (!ok)
			return;
	}
	{
		const ADR::AirportTimeSlice& tsa(ts.get_airport().get_obj()->operator()(dbtime).as_airport());
		if (!tsa.is_valid())
			return;
		AirportsDb::Airport::VFRRoute::VFRRoutePoint pt(tsa.get_ident(), tsa.get_coord(), tsa.get_elev(),
								AirportsDb::Airport::label_e, ' ',
								AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_invalid,
								AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_assigned, true);
		rte.add_point(pt);
	}
	if (ts.as_sid().is_valid()) {
		rte[0].set_pathcode(AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid);
		for (;;) {
			ADR::Graph::vertex_descriptor u(v);
			ADR::Graph::edge_descriptor e;
			{
				ADR::Graph::out_edge_iterator e0, e1;
				for (boost::tie(e0, e1) = boost::out_edges(v, graph); e0 != e1; ++e0) {
					u = boost::target(*e0, graph);
					e = *e0;
					break;
				}
			}
			if (u == v)
				break;
			{
				const ADR::GraphEdge& ee(graph[e]);
				const ADR::SegmentTimeSlice& tss(ee.get_timeslice());
				if (tss.is_valid()) {
					rte.set_minalt(0);
					rte.set_maxalt(99900);
					if (tss.get_altrange().is_lower_valid())
						rte.set_minalt(tss.get_altrange().get_lower_alt());
					if (tss.get_altrange().is_upper_valid())
						rte.set_maxalt(tss.get_altrange().get_upper_alt());
				}
			}
			v = u;
			{
				const ADR::GraphVertex& vv(graph[v]);
				AirportsDb::Airport::VFRRoute::VFRRoutePoint pt(vv.get_ident(), vv.get_coord(),
										std::min((int32_t)rte.get_minalt(), (int32_t)32767),
										AirportsDb::Airport::label_e,
										ts.get_connpoints().find(vv.get_uuid()) == ts.get_connpoints().end() ? 'N' : 'C',
										AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid,
										AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_atorabove, false);
				rte.add_point(pt);
			}
		}
	}
	if (ts.as_star().is_valid()) {
		rte[0].set_pathcode(AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star);
		for (;;) {
			ADR::Graph::vertex_descriptor u(v);
			ADR::Graph::edge_descriptor e;
			{
				ADR::Graph::in_edge_iterator e0, e1;
				for (boost::tie(e0, e1) = boost::in_edges(v, graph); e0 != e1; ++e0) {
					u = boost::source(*e0, graph);
					e = *e0;
					break;
				}
			}
			if (u == v)
				break;
			{
				const ADR::GraphEdge& ee(graph[e]);
				const ADR::SegmentTimeSlice& tss(ee.get_timeslice());
				if (tss.is_valid()) {
					rte.set_minalt(0);
					rte.set_maxalt(99900);
					if (tss.get_altrange().is_lower_valid())
						rte.set_minalt(tss.get_altrange().get_lower_alt());
					if (tss.get_altrange().is_upper_valid())
						rte.set_maxalt(tss.get_altrange().get_upper_alt());
				}
			}
			v = u;
			{
				const ADR::GraphVertex& vv(graph[v]);
				AirportsDb::Airport::VFRRoute::VFRRoutePoint pt(vv.get_ident(), vv.get_coord(),
										std::min((int32_t)rte.get_minalt(), (int32_t)32767),
										AirportsDb::Airport::label_e,
										ts.get_connpoints().find(vv.get_uuid()) == ts.get_connpoints().end() ? 'N' : 'C',
										AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star,
										AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_atorabove, false);
				rte.add_point(pt);
			}
		}
		rte.reverse();
	}
	if (!rte.size())
		return;
	rtes.insert(rte);
}

static void sync_airports(ADR::Database& db, AirportsDb& airportsdb, ADR::timetype_t dbtime, bool verbose)
{
	ADR::Database::findresults_t r(db.find_all(ADR::Database::loadmode_link, dbtime, dbtime + 1,
						   ADR::Object::type_airport, ADR::Object::type_airportend, 0));
	for (ADR::Database::findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
		const ADR::AirportTimeSlice& ts(ri->get_obj()->operator()(dbtime).as_airport());
		if (!ts.is_valid())
			continue;
		AirportsDb::Airport el;
		{
			AirportsDb::elementvector_t ev(airportsdb.find_by_icao(ts.get_ident(), 0, AirportsDb::comp_exact,
									       AirportsDb::element_t::subtables_all));
			double dist(std::numeric_limits<double>::max());
			for (AirportsDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				if (!evi->is_valid() || evi->get_icao() != ts.get_ident())
					continue;
				double d(ts.get_coord().spheric_distance_nmi_dbl(evi->get_coord()));
				if (d > dist)
					continue;
				el = *evi;
				dist = d;
			}
			if (dist > 5)
				el = AirportsDb::Airport();
		}
		el.set_coord(ts.get_coord());
		el.set_flightrules((AirportsDb::Airport::flightrules_arr_vfr |
				    AirportsDb::Airport::flightrules_dep_vfr) |
				   (ts.is_depifr() ? AirportsDb::Airport::flightrules_dep_ifr : 0) |
				   (ts.is_arrifr() ? AirportsDb::Airport::flightrules_arr_ifr : 0));
		el.set_coord(ts.get_coord());
		if (!el.is_valid()) {
			el.set_icao(ts.get_ident());
			el.set_name(ts.get_name());
			el.set_elevation(ts.get_elev());
			el.set_typecode(ts.is_civ() ? (ts.is_mil() ? 'B' : 'A') : (ts.is_mil() ? 'C' : 'D'));
			el.recompute_bbox();
			el.set_sourceid(ri->to_str() + "@ADR");
			el.set_modtime(dbtime);
			el.set_label_placement(AirportsDb::Airport::label_e);
			airportsdb.save(el);
			continue;
		}
		el.set_typecode(ts.is_civ() ? (ts.is_mil() ? 'B' : 'A') : (ts.is_mil() ? 'C' : 'D'));
		el.set_modtime(dbtime);
		// kill existing SID/STAR routes
		for (unsigned int i = 0, n = el.get_nrvfrrte(); i < n;) {
			AirportsDb::Airport::VFRRoute& rte(el.get_vfrrte(i));
			if (!rte.size()) {
				++i;
				continue;
			}
			{
				const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[rte.size() - 1U]);
				if (pt.get_pathcode() == AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid) {
					el.erase_vfrrte(i);
					--n;
					continue;
				}
			}
			{
				const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[0U]);
				if (pt.get_pathcode() == AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star) {
					el.erase_vfrrte(i);
					--n;
					continue;
				}
			}
			++i;
		}
		// re-add new SID/STAR routes
		{
			ADR::Database::findresults_t r(db.find_dependson(*ri, ADR::Database::loadmode_link, dbtime, dbtime + 1,
									 ADR::Object::type_sid, ADR::Object::type_star, 0));
			std::set<AirportsDb::Airport::VFRRoute,route_sorter> rtes;
			for (ADR::Database::findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri)
				add_sidstar(rtes, db, ri->get_obj(), dbtime);
			for (std::set<AirportsDb::Airport::VFRRoute,route_sorter>::const_iterator ri(rtes.begin()), re(rtes.end()); ri != re; ++ri)
				el.add_vfrrte(*ri);
		}
		if (verbose)
			std::cout << "Saving airport " << (std::string)el.get_icao() << ' ' << (std::string)el.get_name()
				  << ' ' << el.get_coord().get_lat_str2() << ' ' << el.get_coord().get_lon_str2()
				  << ' ' << el.get_sourceid() << std::endl;
		airportsdb.save(el);
	}
}

int main(int argc, char *argv[])
{
	static struct option long_options[] = {
		{ "dir", required_argument, 0, 'd' },
		{ "adr-dir", required_argument, 0, 'D' },
		{ "waypoints", no_argument, 0, 0x400 },
		{ "navaids", no_argument, 0, 0x401 },
		{ "airways", no_argument, 0, 0x402 },
		{ "airports", no_argument, 0, 0x403 },
		{0, 0, 0, 0}
	};
	Glib::ustring db_dir(".");
	Glib::ustring adr_db_dir(".");
	ADR::timetype_t dbtime;
	bool verbose(false), dowaypoints(false), donavaids(false), doairways(false), doairports(false);
	int c, err(0);

	{
		Glib::TimeVal tv;
		tv.assign_current_time();
		dbtime = tv.tv_sec;
	}
	while ((c = getopt_long(argc, argv, "d:D:t:v", long_options, 0)) != EOF) {
		switch (c) {
		case 'd':
			if (optarg)
				db_dir = optarg;
			break;

		case 'D':
			if (optarg)
				adr_db_dir = optarg;
			break;

		case 't':
			if (optarg) {
				Glib::TimeVal tv;
				if (tv.assign_from_iso8601(optarg))
					dbtime = tv.tv_sec;
				else
					std::cerr << "cannot parse time " << optarg << std::endl;
			}
			break;

		case 'v':
			verbose = true;
			break;

		case 0x400:
			dowaypoints = true;
			break;

		case 0x401:
			donavaids = true;
			break;

		case 0x402:
			doairways = true;
			break;

		case 0x403:
			doairports = true;
			break;

		default:
			err++;
			break;
		}
	}
	if (err) {
		std::cerr << "usage: adrdbsync [-d <dir>] [-D <adrdir>] [-t <time>] [-v]" << std::endl;
		return EX_USAGE;
	}
	try {
		ADR::Database db(adr_db_dir);
		AirportsDb airportsdb;
		NavaidsDb navaidsdb;
		WaypointsDb waypointsdb;
		AirwaysDb airwaysdb;
		airportsdb.open(db_dir);
		airportsdb.sync_off();
		navaidsdb.open(db_dir);
		navaidsdb.sync_off();
		waypointsdb.open(db_dir);
		waypointsdb.sync_off();
		airwaysdb.open(db_dir);
		airwaysdb.sync_off();
		if (dowaypoints) {
			if (verbose)
				std::cout << "Synchronizing Waypoints " << Glib::TimeVal(dbtime, 0).as_iso8601() << std::endl;
			sync_waypoints(db, waypointsdb, dbtime, verbose);
		}
		if (donavaids) {
			if (verbose)
				std::cout << "Synchronizing Navaids " << Glib::TimeVal(dbtime, 0).as_iso8601() << std::endl;
			sync_navaids(db, navaidsdb, dbtime, verbose);
		}
		if (doairways) {
			if (verbose)
				std::cout << "Synchronizing Airways " << Glib::TimeVal(dbtime, 0).as_iso8601() << std::endl;
			sync_airways(db, airwaysdb, dbtime, verbose);
		}
		if (doairports) {
			if (verbose)
				std::cout << "Synchronizing Airports " << Glib::TimeVal(dbtime, 0).as_iso8601() << std::endl;
			sync_airports(db, airportsdb, dbtime, verbose);
		}
	} catch (const xmlpp::exception& ex) {
		std::cerr << "libxml++ exception: " << ex.what() << std::endl;
		return EX_DATAERR;
	} catch (const std::exception& ex) {
		std::cerr << "exception: " << ex.what() << std::endl;
		return EX_DATAERR;
	}
	return EX_OK;
}
