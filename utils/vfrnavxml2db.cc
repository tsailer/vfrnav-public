//
// C++ Implementation: vfrnavxml2db
//
// Description: Database XML representation to sqlite db conversion
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012, 2013, 2015, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <libxml++/libxml++.h>
#include <sigc++/sigc++.h>
#include <unistd.h>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdexcept>
#include <stdarg.h>
#include <cassert>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>

#include "dbobj.h"

class DbXmlImporter : public xmlpp::SaxParser {
public:
	DbXmlImporter(const Glib::ustring& output_dir, bool pgsql = false, bool trace = false);
	virtual ~DbXmlImporter();

protected:
	virtual void on_start_document();
	virtual void on_end_document();
	virtual void on_start_element(const Glib::ustring& name,
				      const AttributeList& properties);
	virtual void on_end_element(const Glib::ustring& name);
	virtual void on_characters(const Glib::ustring& characters);
	virtual void on_comment(const Glib::ustring& text);
	virtual void on_warning(const Glib::ustring& text);
	virtual void on_error(const Glib::ustring& text);
	virtual void on_fatal_error(const Glib::ustring& text);

	template<class T> bool attr_signed(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,T>& slot);
	template<class T> bool attr_unsigned(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,T>& slot);
	template<class T> bool attr(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,T>& slot);
	bool attr_coord(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,Point::coord_t>& slot);
	bool attr_modtime(const AttributeList& properties, const sigc::slot<void,time_t>& slot);
	bool attr_airport_typecode(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,char>& slot);
	bool attr_airspace_class(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,char>& slot_bdryclass, const sigc::slot<void,uint8_t>& slot_typecode);
	bool attr_waypoint_usage(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,char>& slot);
	bool attr_waypoint_type(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,char>& slot);

	void parse_fplan(const AttributeList& properties);
	void parse_fplan_waypoint(const AttributeList& properties);
	void parse_airport(const AttributeList& properties);
	void parse_airport_rwy(const AttributeList& properties);
	void parse_airport_rwy_he(const AttributeList& properties);
	void parse_airport_rwy_le(const AttributeList& properties);
	void parse_airport_helipad(const AttributeList& properties);
	void parse_airport_comm(const AttributeList& properties);
	void parse_airport_vfrroute(const AttributeList& properties);
	void parse_airport_vfrpoint(const AttributeList& properties);
	void parse_airport_linefeature(const AttributeList& properties);
	void parse_airport_linefeature_node(const AttributeList& properties);
	void parse_airport_pointfeature(const AttributeList& properties);
	void parse_airport_fas(const AttributeList& properties);
	void parse_airspace(const AttributeList& properties);
	void parse_airspace_segment(const AttributeList& properties);
	void parse_airspace_component(const AttributeList& properties);
	void parse_airspace_polygon(const AttributeList& properties);
	void parse_airspace_polygonint(const AttributeList& properties);
	void parse_airspace_polygonpoint(const AttributeList& properties);
	void parse_airspace_polygonintpoint(const AttributeList& properties);
	void parse_navaid(const AttributeList& properties);
	void parse_waypoint(const AttributeList& properties);
	void parse_airway(const AttributeList& properties);
	void parse_track(const AttributeList& properties);
	void parse_track_point(const AttributeList& properties);
	void parse_airportdelete(const AttributeList& properties);
	void parse_airspacedelete(const AttributeList& properties);
	void parse_navaiddelete(const AttributeList& properties);
	void parse_waypointdelete(const AttributeList& properties);
	void parse_airwaydelete(const AttributeList& properties);
	void parse_trackdelete(const AttributeList& properties);

	void end_fplan(void);
	void end_airport(void);
	void end_airspace(void);
	void end_navaid(void);
	void end_waypoint(void);
	void end_airway(void);
	void end_track(void);

	void open_airports_db(void);
	void open_airspaces_db(void);
	void open_navaids_db(void);
	void open_waypoints_db(void);
	void open_airways_db(void);
	void open_tracks_db(void);

	void finalize_airspaces(void);

private:
	typedef enum {
		state_document_c,
		state_vfrnav_c,
		state_airport_c,
		state_airport_rwy_c,
		state_airport_rwy_he_c,
		state_airport_rwy_le_c,
		state_airport_helipad_c,
		state_airport_comm_c,
		state_airport_vfrroute_c,
		state_airport_vfrpoint_c,
		state_airport_linefeature_c,
		state_airport_linefeature_node_c,
		state_airport_pointfeature_c,
		state_airport_fas_c,
		state_airspace_c,
		state_airspace_segment_c,
		state_airspace_component_c,
		state_airspace_polygon_c,
		state_airspace_polygonpoint_c,
		state_airspace_polygon_exterior_c,
		state_airspace_polygon_exteriorpoint_c,
		state_airspace_polygon_interior_c,
		state_airspace_polygon_interiorpoint_c,
		state_navaid_c,
		state_waypoint_c,
		state_airway_c,
		state_track_c,
		state_track_point_c,
		state_elementdelete_c,
		state_fplan_c,
		state_fplan_waypoint_c
	} state_t;
	state_t m_state;
	bool m_trace;

	Glib::ustring m_outputdir;
	bool m_purgedb;
	bool m_pgsql;
	std::unique_ptr<AirportsDbQueryInterface> m_airportsdb;
	std::unique_ptr<AirspacesDbQueryInterface> m_airspacesdb;
	std::unique_ptr<NavaidsDbQueryInterface> m_navaidsdb;
	std::unique_ptr<WaypointsDbQueryInterface> m_waypointsdb;
	std::unique_ptr<AirwaysDbQueryInterface> m_airwaysdb;
	std::unique_ptr<TracksDbQueryInterface> m_tracksdb;
	FPlan *m_fplan;
	FPlanRoute *m_fplanroute;

	AirportsDb::Airport m_arpt;
	AirspacesDb::Airspace m_aspc;
	NavaidsDb::Navaid m_nav;
	WaypointsDb::Waypoint m_wpt;
	AirwaysDb::Airway m_awy;
	TracksDb::Track m_trk;

	template<class T> class ValueObject {
	public:
		ValueObject(T v) : m_v(v) {}
		T get(void) const { return m_v; }
		void set(T v) { m_v = v; }
	private:
		T m_v;
	};

	class AirspaceDeps {
	public:
		class AirspaceDep {
		public:
			AirspaceDep(const Glib::ustring& icao = "", uint8_t tc = 0, char bc = 0) : m_icao(icao), m_typecode(tc), m_bdryclass(bc) {}
			AirspaceDep(const AirspacesDb::Airspace::Component& c) : m_icao(c.get_icao()), m_typecode(c.get_typecode()), m_bdryclass(c.get_bdryclass()) {}

			const Glib::ustring& get_icao(void) const { return m_icao; }
			uint8_t get_typecode(void) const { return m_typecode; }
			char get_bdryclass(void) const { return m_bdryclass; }

		protected:
			Glib::ustring m_icao;
			uint8_t m_typecode;
			char m_bdryclass;
		};

		AirspaceDeps(const Glib::ustring& icao = "", uint8_t tc = 0, char bc = 0) : m_icao(icao), m_typecode(tc), m_bdryclass(bc) {}
		AirspaceDeps(const AirspaceDep& a) : m_icao(a.get_icao()), m_typecode(a.get_typecode()), m_bdryclass(a.get_bdryclass()) {}
		AirspaceDeps(const AirspacesDb::Airspace& a);

		bool operator<(const AirspaceDeps& x) const;

		const Glib::ustring& get_icao(void) const { return m_icao; }
		uint8_t get_typecode(void) const { return m_typecode; }
		char get_bdryclass(void) const { return m_bdryclass; }

		void add_dep(const AirspaceDep& adep) { m_deps.push_back(adep); }
		const AirspaceDep& operator[](unsigned int i) const { return m_deps[i]; }
		unsigned int size(void) const { return m_deps.size(); }

	protected:
		typedef std::vector<AirspaceDep> deps_t;
		deps_t m_deps;
		Glib::ustring m_icao;
		uint8_t m_typecode;
		char m_bdryclass;
	};

	typedef std::map<AirspaceDeps,unsigned int> asdeps_t;
	asdeps_t m_asdeps;
};

DbXmlImporter::AirspaceDeps::AirspaceDeps(const AirspacesDb::Airspace& a)
	: m_icao(a.get_icao()), m_typecode(a.get_typecode()), m_bdryclass(a.get_bdryclass())
{
	for (unsigned int i = 0; i < a.get_nrcomponents(); ++i)
		add_dep(AirspaceDep(a.get_component(i)));
}

bool DbXmlImporter::AirspaceDeps::AirspaceDeps::operator<(const AirspaceDeps& x) const
{
	int r(get_icao().compare(x.get_icao()));
	if (r < 0)
		return true;
	if (r > 0)
		return false;
	if (get_typecode() < x.get_typecode())
		return true;
	if (get_typecode() > x.get_typecode())
		return false;
	return get_bdryclass() < x.get_bdryclass();
}

DbXmlImporter::DbXmlImporter(const Glib::ustring & output_dir, bool pgsql, bool trace)
	: xmlpp::SaxParser(false), m_state(state_document_c), m_trace(trace), m_outputdir(output_dir),
	  m_purgedb(false), m_pgsql(pgsql), m_fplan(0), m_fplanroute(0)
{
}

DbXmlImporter::~DbXmlImporter()
{
}

template<class T> bool DbXmlImporter::attr_signed(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void, T>& slot)
{
	for (AttributeList::const_iterator ai(properties.begin()), ae(properties.end()); ai != ae; ai++) {
		if (ai->name != name)
			continue;
		long long v = strtoll(ai->value.c_str(), 0, 0);
		slot((T)v);
		return true;
	}
	return false;
}

template<class T> bool DbXmlImporter::attr_unsigned(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void, T>& slot)
{
	for (AttributeList::const_iterator ai(properties.begin()), ae(properties.end()); ai != ae; ai++) {
		if (ai->name != name)
			continue;
		unsigned long long v = strtoull(ai->value.c_str(), 0, 0);
		slot((T)v);
		return true;
	}
	return false;
}

template<class T> bool DbXmlImporter::attr(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void, T>& slot)
{
	for (AttributeList::const_iterator ai(properties.begin()), ae(properties.end()); ai != ae; ai++) {
		if (ai->name != name)
			continue;
		slot(ai->value);
		return true;
	}
	return false;
}

bool DbXmlImporter::attr_coord(const AttributeList & properties, const Glib::ustring & name, const sigc::slot< void, Point::coord_t > & slot)
{
	return attr_signed<Point::coord_t>(properties, name, slot);
}

template class DbXmlImporter::ValueObject<Point::coord_t>;

template<> bool DbXmlImporter::attr<Point>(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,Point>& slot)
{
	ValueObject<Point::coord_t> lon(0), lat(0);
	bool r = attr_signed<Point::coord_t>(properties, name + "lon", sigc::mem_fun(lon, &ValueObject<Point::coord_t>::set)) &&
			attr_signed<Point::coord_t>(properties, name + "lat", sigc::mem_fun(lat, &ValueObject<Point::coord_t>::set));
	if (!r)
		return false;
	slot(Point(lon.get(), lat.get()));
	return r;
}

template<> bool DbXmlImporter::attr<NavaidsDb::Navaid::label_placement_t>(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,NavaidsDb::Navaid::label_placement_t>& slot)
{
	ValueObject<Glib::ustring> val("");
	bool r = attr<Glib::ustring>(properties, name, sigc::mem_fun(val, &ValueObject<Glib::ustring>::set));
	if (!r)
		return false;
	static const NavaidsDb::Navaid::label_placement_t vals[] = {
		NavaidsDb::Navaid::label_n,
		NavaidsDb::Navaid::label_e,
		NavaidsDb::Navaid::label_s,
		NavaidsDb::Navaid::label_w,
		NavaidsDb::Navaid::label_center,
		NavaidsDb::Navaid::label_any,
		NavaidsDb::Navaid::label_off
	};
	for (unsigned int i = 0; i < sizeof(vals)/sizeof(vals[0]); i++) {
		if (NavaidsDb::Navaid::get_label_placement_name(vals[i]) != val.get())
			continue;
		slot(vals[i]);
		return true;
	}
	slot(NavaidsDb::Navaid::label_off);
	return true;
}

bool DbXmlImporter::attr_airport_typecode(const AttributeList & properties, const Glib::ustring & name, const sigc::slot< void, char > & slot)
{
	ValueObject<Glib::ustring> val("");
	bool r = attr<Glib::ustring>(properties, name, sigc::mem_fun(val, &ValueObject<Glib::ustring>::set));
	if (!r)
		return false;
	for (char v = 'A'; v <= 'D'; v++) {
		if (AirportsDb::Airport::get_type_string(v) != val.get())
			continue;
		slot(v);
		return true;
	}
	slot('D');
	return true;
}

bool DbXmlImporter::attr_modtime(const AttributeList & properties, const sigc::slot< void, time_t > & slot)
{
	return attr_unsigned<time_t>(properties, "modtime", slot);
}


template<> bool DbXmlImporter::attr<AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_t>(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_t>& slot)
{
	ValueObject<Glib::ustring> val("");
	bool r = attr<Glib::ustring>(properties, name, sigc::mem_fun(val, &ValueObject<Glib::ustring>::set));
	if (!r)
		return false;
	static const AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_t vals[] = {
		AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_arrival,
		AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_departure,
		AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_holding,
		AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_trafficpattern,
		AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_vfrtransition,
		AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_other,
		AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid,
		AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star,
		AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_invalid
	};
	for (unsigned int i = 0; i < sizeof(vals)/sizeof(vals[0]); i++) {
		if (AirportsDb::Airport::VFRRoute::VFRRoutePoint::get_pathcode_string(vals[i]) != val.get())
			continue;
		slot(vals[i]);
		return true;
	}
	slot(AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_invalid);
	return true;
}

template<> bool DbXmlImporter::attr<AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_t>(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_t>& slot)
{
	ValueObject<Glib::ustring> val("");
	bool r = attr<Glib::ustring>(properties, name, sigc::mem_fun(val, &ValueObject<Glib::ustring>::set));
	if (!r)
		return false;
	static const AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_t vals[] = {
		AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_atorabove,
		AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_atorbelow,
		AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_assigned,
		AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_altspecified,
		AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_recommendedalt,
		AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_invalid
	};
	for (unsigned int i = 0; i < sizeof(vals)/sizeof(vals[0]); i++) {
		if (AirportsDb::Airport::VFRRoute::VFRRoutePoint::get_altcode_string(vals[i]) != val.get())
			continue;
		slot(vals[i]);
		return true;
	}
	slot(AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_invalid);
	return true;
}

template<> bool DbXmlImporter::attr<char>(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,char>& slot)
{
	ValueObject<Glib::ustring> val("");
	bool r = attr<Glib::ustring>(properties, name, sigc::mem_fun(val, &ValueObject<Glib::ustring>::set));
	if (!r)
		return false;
	slot(*val.get().c_str());
	return r;
}

template<> bool DbXmlImporter::attr<bool>(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,bool>& slot)
{
	ValueObject<uint32_t> val(0);
	bool r = attr_unsigned<uint32_t>(properties, name, sigc::mem_fun(val, &ValueObject<uint32_t>::set));
	if (!r)
		return false;
	slot(!!val.get());
	return r;
}

bool DbXmlImporter::attr_airspace_class(const AttributeList & properties, const Glib::ustring& name, const sigc::slot< void, char > & slot_bdryclass, const sigc::slot< void, uint8_t > & slot_typecode)
{
	ValueObject<Glib::ustring> val("");
	bool r = attr<Glib::ustring>(properties, name, sigc::mem_fun(val, &ValueObject<Glib::ustring>::set));
	if (!r)
		return false;
	uint8_t tc(0);
	char cls('A');
	AirspacesDb::Airspace::set_class_string(tc, cls, val.get());
	slot_bdryclass(cls);
	slot_typecode(tc);
	return true;
}

template<> bool DbXmlImporter::attr<NavaidsDb::Navaid::navaid_type_t>(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,NavaidsDb::Navaid::navaid_type_t>& slot)
{
	ValueObject<Glib::ustring> val("");
	bool r = attr<Glib::ustring>(properties, name, sigc::mem_fun(val, &ValueObject<Glib::ustring>::set));
	if (!r)
		return false;
	static const NavaidsDb::Navaid::navaid_type_t vals[] = {
		NavaidsDb::Navaid::navaid_invalid,
		NavaidsDb::Navaid::navaid_vor,
		NavaidsDb::Navaid::navaid_vordme,
		NavaidsDb::Navaid::navaid_dme,
		NavaidsDb::Navaid::navaid_vortac,
		NavaidsDb::Navaid::navaid_tacan,
		NavaidsDb::Navaid::navaid_ndb
	};
	for (unsigned int i = 0; i < sizeof(vals)/sizeof(vals[0]); ++i) {
		if (NavaidsDb::Navaid::get_navaid_typename(vals[i]) != val.get())
			continue;
		slot(vals[i]);
		return true;
	}
	slot(NavaidsDb::Navaid::navaid_invalid);
	return true;
}

bool DbXmlImporter::attr_waypoint_usage(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,char>& slot)
{
	ValueObject<Glib::ustring> val("");
	bool r = attr<Glib::ustring>(properties, name, sigc::mem_fun(val, &ValueObject<Glib::ustring>::set));
	if (!r)
		return false;
	static const char vals[] = {
		WaypointsDb::Waypoint::usage_invalid,
		WaypointsDb::Waypoint::usage_highlevel,
		WaypointsDb::Waypoint::usage_lowlevel,
		WaypointsDb::Waypoint::usage_bothlevel,
		WaypointsDb::Waypoint::usage_rnav,
		WaypointsDb::Waypoint::usage_terminal
	};
	for (unsigned int i = 0; i < sizeof(vals)/sizeof(vals[0]); ++i) {
		if (WaypointsDb::Waypoint::get_usage_name(vals[i]) != val.get())
			continue;
		slot(vals[i]);
		return true;
	}
	slot(WaypointsDb::Waypoint::usage_invalid);
	return true;
}

bool DbXmlImporter::attr_waypoint_type(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,char>& slot)
{
	ValueObject<Glib::ustring> val("");
	bool r = attr<Glib::ustring>(properties, name, sigc::mem_fun(val, &ValueObject<Glib::ustring>::set));
	if (!r)
		return false;
	static const char vals[] = {
		WaypointsDb::Waypoint::type_invalid,
		WaypointsDb::Waypoint::type_unknown,
		WaypointsDb::Waypoint::type_adhp,
		WaypointsDb::Waypoint::type_icao,
		WaypointsDb::Waypoint::type_coord,
		WaypointsDb::Waypoint::type_other
	};
	for (unsigned int i = 0; i < sizeof(vals)/sizeof(vals[0]); ++i) {
		if (WaypointsDb::Waypoint::get_type_name(vals[i]) != val.get())
			continue;
		slot(vals[i]);
		return true;
	}
	slot(WaypointsDb::Waypoint::type_invalid);
	return true;
}

template<> bool DbXmlImporter::attr<AirwaysDb::Airway::airway_type_t>(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,AirwaysDb::Airway::airway_type_t>& slot)
{
	ValueObject<Glib::ustring> val("");
	bool r = attr<Glib::ustring>(properties, name, sigc::mem_fun(val, &ValueObject<Glib::ustring>::set));
	if (!r)
		return false;
	static const AirwaysDb::Airway::airway_type_t vals[] = {
		AirwaysDb::Airway::airway_invalid,
		AirwaysDb::Airway::airway_low,
		AirwaysDb::Airway::airway_high,
		AirwaysDb::Airway::airway_both,
		AirwaysDb::Airway::airway_user
	};
	for (unsigned int i = 0; i < sizeof(vals)/sizeof(vals[0]); ++i) {
		if (AirwaysDb::Airway::get_type_name(vals[i]) != val.get())
			continue;
		slot(vals[i]);
		return true;
	}
	slot(AirwaysDb::Airway::airway_invalid);
	return true;
}

template bool DbXmlImporter::attr_signed<int32_t>(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void, int32_t>& slot);
template bool DbXmlImporter::attr_signed<int16_t>(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void, int16_t>& slot);
template bool DbXmlImporter::attr_unsigned<uint32_t>(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void, uint32_t>& slot);
template bool DbXmlImporter::attr_unsigned<uint16_t>(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void, uint16_t>& slot);
template bool DbXmlImporter::attr_unsigned<uint8_t>(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void, uint8_t>& slot);
template bool DbXmlImporter::attr<const Glib::ustring&>(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void, const Glib::ustring&>& slot);
template bool DbXmlImporter::attr<const std::string&>(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void, const std::string&>& slot);
template bool DbXmlImporter::attr<Point>(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,Point>& slot);
template bool DbXmlImporter::attr<NavaidsDb::Navaid::label_placement_t>(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,NavaidsDb::Navaid::label_placement_t>& slot);
template bool DbXmlImporter::attr<AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_t>(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_t>& slot);
template bool DbXmlImporter::attr<AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_t>(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_t>& slot);
template bool DbXmlImporter::attr<char>(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,char>& slot);
template bool DbXmlImporter::attr<bool>(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,bool>& slot);
template bool DbXmlImporter::attr<NavaidsDb::Navaid::navaid_type_t>(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,NavaidsDb::Navaid::navaid_type_t>& slot);
template bool DbXmlImporter::attr<AirwaysDb::Airway::airway_type_t>(const AttributeList& properties, const Glib::ustring& name, const sigc::slot<void,AirwaysDb::Airway::airway_type_t>& slot);

void DbXmlImporter::on_start_document()
{
	if (m_trace)
		std::cerr << "start_document" << std::endl;
	m_state = state_document_c;
}

void DbXmlImporter::on_end_document()
{
	if (m_trace)
		std::cerr << "end_document" << std::endl;
	if (m_airportsdb) {
		m_airportsdb->set_exclusive(false);
		m_airportsdb->close();
		m_airportsdb.reset();
	}
	if (m_airspacesdb) {
		m_airspacesdb->set_exclusive(false);
		m_airspacesdb->close();
		m_airspacesdb.reset();
	}
	if (m_navaidsdb) {
		m_navaidsdb->set_exclusive(false);
		m_navaidsdb->close();
		m_navaidsdb.reset();
	}
	if (m_waypointsdb) {
		m_waypointsdb->set_exclusive(false);
		m_waypointsdb->close();
		m_waypointsdb.reset();
	}
	if (m_airwaysdb) {
		m_airwaysdb->set_exclusive(false);
		m_airwaysdb->close();
		m_airwaysdb.reset();
	}
	if (m_tracksdb) {
		m_tracksdb->set_exclusive(false);
		m_tracksdb->close();
		m_tracksdb.reset();
	}
	if (m_fplanroute)
		delete m_fplanroute;
	m_fplanroute = 0;
	if (m_fplan)
		delete m_fplan;
	m_fplan = 0;
	if (m_state != state_document_c)
		throw std::runtime_error("XML Parser: end document and not document state");
}

void DbXmlImporter::on_start_element(const Glib::ustring& name, const AttributeList& properties)
{
	if (m_trace)
		std::cerr << "start_element " << name << std::endl;
	switch (m_state) {
		case state_document_c:
			if (name == "VFRNav") {
				m_state = state_vfrnav_c;
				return;
			}
			break;

		case state_vfrnav_c:
			if (name == "NavFlightPlan") {
				m_state = state_fplan_c;
				parse_fplan(properties);
				return;
			}
			if (name == "Airport") {
				m_state = state_airport_c;
				parse_airport(properties);
				return;
			}
			if (name == "Airspace") {
				m_state = state_airspace_c;
				parse_airspace(properties);
				return;
			}
			if (name == "Navaid") {
				m_state = state_navaid_c;
				parse_navaid(properties);
				return;
			}
			if (name == "Waypoint") {
				m_state = state_waypoint_c;
				parse_waypoint(properties);
				return;
			}
			if (name == "Airway") {
				m_state = state_airway_c;
				parse_airway(properties);
				return;
			}
			if (name == "Track") {
				m_state = state_track_c;
				parse_track(properties);
				return;
			}
			if (name == "AirportDelete") {
				m_state = state_elementdelete_c;
				parse_airportdelete(properties);
				return;
			}
			if (name == "AirspaceDelete") {
				m_state = state_elementdelete_c;
				parse_airspacedelete(properties);
				return;
			}
			if (name == "NavaidDelete") {
				m_state = state_elementdelete_c;
				parse_navaiddelete(properties);
				return;
			}
			if (name == "WaypointDelete") {
				m_state = state_elementdelete_c;
				parse_waypointdelete(properties);
				return;
			}
			break;

		case state_fplan_c:
			if (name == "WayPoint") {
				m_state = state_fplan_waypoint_c;
				parse_fplan_waypoint(properties);
				return;
			}
			break;

		case state_airport_c:
			if (name == "Runway") {
				m_state = state_airport_rwy_c;
				parse_airport_rwy(properties);
				return;
			}
			if (name == "Helipad") {
				m_state = state_airport_helipad_c;
				parse_airport_helipad(properties);
				return;
			}
			if (name == "Comm") {
				m_state = state_airport_comm_c;
				parse_airport_comm(properties);
				return;
			}
			if (name == "VFRRoute") {
				m_state = state_airport_vfrroute_c;
				parse_airport_vfrroute(properties);
				return;
			}
			if (name == "LineFeature") {
				m_state = state_airport_linefeature_c;
				parse_airport_linefeature(properties);
				return;
			}
			if (name == "PointFeature") {
				m_state = state_airport_pointfeature_c;
				parse_airport_pointfeature(properties);
				return;
			}
			if (name == "FinalApproachSegment") {
				m_state = state_airport_fas_c;
				parse_airport_fas(properties);
				return;
			}
			break;

		case state_airport_rwy_c:
			if (name == "he") {
				m_state = state_airport_rwy_he_c;
				parse_airport_rwy_he(properties);
				return;
			}
			if (name == "le") {
				m_state = state_airport_rwy_le_c;
				parse_airport_rwy_le(properties);
				return;
			}
			break;

		 case state_airport_vfrroute_c:
			if (name == "VFRRoutePoint") {
				m_state = state_airport_vfrpoint_c;
				parse_airport_vfrpoint(properties);
				return;
			}
			break;

		case state_airport_linefeature_c:
			if (name == "Node") {
				m_state = state_airport_linefeature_node_c;
				parse_airport_linefeature_node(properties);
				return;
			}
			break;

		case state_airspace_c:
			if (name == "Segment") {
				m_state = state_airspace_segment_c;
				parse_airspace_segment(properties);
				return;
			}
			if (name == "Component") {
				m_state = state_airspace_component_c;
				parse_airspace_component(properties);
				return;
			}
			if (name == "Polygon") {
				m_state = state_airspace_polygon_c;
				parse_airspace_polygon(properties);
				return;
			}
			break;

		case state_airspace_polygon_c:
			if (name == "Point") {
				m_state = state_airspace_polygonpoint_c;
				parse_airspace_polygonpoint(properties);
				return;
			}
			if (name == "exterior") {
				m_state = state_airspace_polygon_exterior_c;
				return;
			}
			if (name == "interior") {
				m_state = state_airspace_polygon_interior_c;
				parse_airspace_polygonint(properties);
				return;
			}
			break;

		case state_airspace_polygon_exterior_c:
			if (name == "Point") {
				m_state = state_airspace_polygon_exteriorpoint_c;
				parse_airspace_polygonpoint(properties);
				return;
			}
			break;

		case state_airspace_polygon_interior_c:
			if (name == "Point") {
				m_state = state_airspace_polygon_interiorpoint_c;
				parse_airspace_polygonintpoint(properties);
				return;
			}
			break;

		case state_track_c:
			if (name == "Point") {
				m_state = state_track_point_c;
				parse_track_point(properties);
				return;
			}
			break;

		default:
			break;
	}
	std::ostringstream oss;
	oss << "XML Parser: Invalid element " << name << " in state " << (int)m_state;
	throw std::runtime_error(oss.str());
}

void DbXmlImporter::on_end_element(const Glib::ustring& name)
{
	if (m_trace)
		std::cerr << "end_element " << name << std::endl;
	switch (m_state) {
		case state_vfrnav_c:
			m_state = state_document_c;
			finalize_airspaces();
			return;

		case state_elementdelete_c:
			m_state = state_vfrnav_c;
			return;

		case state_airport_c:
			m_state = state_vfrnav_c;
			end_airport();
			return;

		case state_airport_rwy_c:
			m_state = state_airport_c;
			return;

		case state_airport_rwy_he_c:
			m_state = state_airport_rwy_c;
			return;

		case state_airport_rwy_le_c:
			m_state = state_airport_rwy_c;
			return;

		case state_airport_helipad_c:
			m_state = state_airport_c;
			return;

		case state_airport_comm_c:
			m_state = state_airport_c;
			return;

		case state_airport_vfrroute_c:
			m_state = state_airport_c;
			return;

		case state_airport_vfrpoint_c:
			m_state = state_airport_vfrroute_c;
			return;

		case state_airport_linefeature_c:
			m_state = state_airport_c;
			return;

		case state_airport_linefeature_node_c:
			m_state = state_airport_linefeature_c;
			return;

		case state_airport_pointfeature_c:
			m_state = state_airport_c;
			return;

		case state_airport_fas_c:
			m_state = state_airport_c;
			return;

		case state_airspace_c:
			m_state = state_vfrnav_c;
			end_airspace();
			return;

		case state_airspace_segment_c:
			m_state = state_airspace_c;
			return;

		case state_airspace_component_c:
			m_state = state_airspace_c;
			return;

		case state_airspace_polygon_c:
			m_state = state_airspace_c;
			return;

		case state_airspace_polygonpoint_c:
			m_state = state_airspace_polygon_c;
			return;

		case state_airspace_polygon_exterior_c:
			m_state = state_airspace_polygon_c;
			return;

		case state_airspace_polygon_exteriorpoint_c:
			m_state = state_airspace_polygon_exterior_c;
			return;

		case state_airspace_polygon_interior_c:
			m_state = state_airspace_polygon_c;
			return;

		case state_airspace_polygon_interiorpoint_c:
			m_state = state_airspace_polygon_interior_c;
			return;


		case state_navaid_c:
			m_state = state_vfrnav_c;
			end_navaid();
			return;

		case state_waypoint_c:
			m_state = state_vfrnav_c;
			end_waypoint();
			return;

		case state_airway_c:
			m_state = state_vfrnav_c;
			end_airway();
			return;

		case state_track_c:
			m_state = state_vfrnav_c;
			end_track();
			return;

		case state_track_point_c:
			m_state = state_track_c;
			return;

		case state_fplan_c:
			m_state = state_vfrnav_c;
			end_fplan();
			return;

		case state_fplan_waypoint_c:
			m_state = state_fplan_c;
			return;

		default:
			break;
	}
	std::ostringstream oss;
	oss << "XML Parser: Invalid end element in state " << (int)m_state;
	throw std::runtime_error(oss.str());
}

void DbXmlImporter::on_characters(const Glib::ustring& characters)
{
}

void DbXmlImporter::on_comment(const Glib::ustring& text)
{
}

void DbXmlImporter::on_warning(const Glib::ustring& text)
{
	std::cerr << "XML Parser: warning: " << text << std::endl;
	//throw std::runtime_error("XML Parser: warning: " + text);
}

void DbXmlImporter::on_error(const Glib::ustring& text)
{
	std::cerr << "XML Parser: error: " << text << std::endl;
	//throw std::runtime_error("XML Parser: error: " + text);
}

void DbXmlImporter::on_fatal_error(const Glib::ustring& text)
{
	throw std::runtime_error("XML Parser: fatal error: " + text);
}

void DbXmlImporter::parse_fplan(const AttributeList & properties)
{
	if (!m_fplan) {
		sqlite3x::sqlite3_connection conn(m_outputdir + "/fplan.db");
		m_fplan = new FPlan(conn.take());
		if (m_purgedb)
			m_fplan->delete_database();
	}
	if (!m_fplanroute)
		m_fplanroute = new FPlanRoute(*m_fplan);
	m_fplanroute->new_fp();
	attr_unsigned<uint32_t>(properties, "TimeOffBlock", sigc::mem_fun(*m_fplanroute, &FPlanRoute::set_time_offblock));
	attr_unsigned<uint32_t>(properties, "SavedTimeOffBlock", sigc::mem_fun(m_fplanroute, &FPlanRoute::set_save_time_offblock));
	attr_unsigned<uint32_t>(properties, "TimeOnBlock", sigc::mem_fun(m_fplanroute, &FPlanRoute::set_time_onblock));
	attr_unsigned<uint32_t>(properties, "SavedTimeOnBlock", sigc::mem_fun(m_fplanroute, &FPlanRoute::set_save_time_onblock));
	attr_unsigned<uint16_t>(properties, "CurrentWaypoint", sigc::mem_fun(m_fplanroute, &FPlanRoute::set_curwpt));
	attr<const Glib::ustring&>(properties, "Note", sigc::mem_fun(*m_fplanroute, &FPlanRoute::set_note));
}

void DbXmlImporter::parse_fplan_waypoint(const AttributeList & properties)
{
	assert(m_fplanroute);
	FPlanWaypoint wpt;
	attr<const Glib::ustring&>(properties, "ICAO", sigc::mem_fun(wpt, &FPlanWaypoint::set_icao));
	attr<const Glib::ustring&>(properties, "name", sigc::mem_fun(wpt, &FPlanWaypoint::set_name));
	attr<const std::string&>(properties, "pathcode", sigc::hide_return(sigc::mem_fun(wpt, &FPlanWaypoint::set_pathcode_name)));
	attr<const Glib::ustring&>(properties, "pathname", sigc::mem_fun(wpt, &FPlanWaypoint::set_pathname));
	attr_unsigned<uint32_t>(properties, "Time", sigc::mem_fun(wpt, &FPlanWaypoint::set_time));
	attr_unsigned<uint32_t>(properties, "SavedTime", sigc::mem_fun(wpt, &FPlanWaypoint::set_save_time));
	attr_unsigned<uint32_t>(properties, "FlightTime", sigc::mem_fun(wpt, &FPlanWaypoint::set_flighttime));
	attr_unsigned<uint32_t>(properties, "frequency", sigc::mem_fun(wpt, &FPlanWaypoint::set_frequency));
	attr_signed<Point::coord_t>(properties, "lon", sigc::mem_fun(wpt, &FPlanWaypoint::set_lon));
	attr_signed<Point::coord_t>(properties, "lat", sigc::mem_fun(wpt, &FPlanWaypoint::set_lat));
	attr_signed<int16_t>(properties, "alt", sigc::mem_fun(wpt, &FPlanWaypoint::set_altitude));
	attr_unsigned<uint16_t>(properties, "flags", sigc::mem_fun(wpt, &FPlanWaypoint::set_flags));
	attr_unsigned<uint16_t>(properties, "winddir", sigc::mem_fun(wpt, &FPlanWaypoint::set_winddir));
	attr_unsigned<uint16_t>(properties, "windspeed", sigc::mem_fun(wpt, &FPlanWaypoint::set_windspeed));
	attr_unsigned<uint16_t>(properties, "qff", sigc::mem_fun(wpt, &FPlanWaypoint::set_qff));
	attr_signed<int16_t>(properties, "isaoffset", sigc::mem_fun(wpt, &FPlanWaypoint::set_isaoffset));
	attr<const Glib::ustring&>(properties, "Note", sigc::mem_fun(wpt, &FPlanWaypoint::set_note));
	attr_signed<int32_t>(properties, "truealt", sigc::mem_fun(wpt, &FPlanWaypoint::set_truealt));
	attr_unsigned<uint16_t>(properties, "truetrack", sigc::mem_fun(wpt, &FPlanWaypoint::set_truetrack));
	attr_unsigned<uint16_t>(properties, "trueheading", sigc::mem_fun(wpt, &FPlanWaypoint::set_trueheading));
	attr_unsigned<uint16_t>(properties, "declination", sigc::mem_fun(wpt, &FPlanWaypoint::set_declination));
	attr_unsigned<uint32_t>(properties, "dist", sigc::mem_fun(wpt, &FPlanWaypoint::set_dist));
	attr_unsigned<uint32_t>(properties, "mass", sigc::mem_fun(wpt, &FPlanWaypoint::set_mass));
	attr_unsigned<uint32_t>(properties, "fuel", sigc::mem_fun(wpt, &FPlanWaypoint::set_fuel));
	attr_unsigned<uint16_t>(properties, "tas", sigc::mem_fun(wpt, &FPlanWaypoint::set_tas));
	attr_unsigned<uint16_t>(properties, "rpm", sigc::mem_fun(wpt, &FPlanWaypoint::set_rpm));
	attr_unsigned<uint16_t>(properties, "mp", sigc::mem_fun(wpt, &FPlanWaypoint::set_mp));
	attr<const std::string&>(properties, "type", sigc::hide_return(sigc::mem_fun(wpt, &FPlanWaypoint::set_type_string)));
	m_fplanroute->insert_wpt(~0, wpt);
}

void DbXmlImporter::end_fplan(void)
{
	assert(m_fplanroute);
	std::cerr << "Saving flight plan";
	if (m_fplanroute->get_nrwpt() > 0)
		std::cerr << ' ' << (*m_fplanroute)[0].get_icao_name() << "->" << (*m_fplanroute)[m_fplanroute->get_nrwpt()-1].get_icao_name();
	std::cerr << std::endl;
	m_fplanroute->save_fp();
}

void DbXmlImporter::parse_airport(const AttributeList & properties)
{
	m_arpt = AirportsDb::Airport();
	attr<const Glib::ustring&>(properties, "sourceid", sigc::mem_fun(m_arpt, &AirportsDb::Airport::set_sourceid));
	{
		open_airports_db();
		AirportsDb::elementvector_t ev(m_airportsdb->find_by_sourceid(m_arpt.get_sourceid()));
		if (ev.size() == 1) {
			std::cerr << "erasing airport " << ev.begin()->get_sourceid() << " address " << ev.begin()->get_address() << std::endl;
			m_airportsdb->erase(*ev.begin());
		}
	}
	attr<Point>(properties, "", sigc::mem_fun(m_arpt, &AirportsDb::Airport::set_coord));
	attr<const Glib::ustring&>(properties, "icao", sigc::mem_fun(m_arpt, &AirportsDb::Airport::set_icao));
	attr<const Glib::ustring&>(properties, "name", sigc::mem_fun(m_arpt, &AirportsDb::Airport::set_name));
	attr<const Glib::ustring&>(properties, "authority", sigc::mem_fun(m_arpt, &AirportsDb::Airport::set_authority));
	attr<const Glib::ustring&>(properties, "vfrremark", sigc::mem_fun(m_arpt, &AirportsDb::Airport::set_vfrrmk));
	attr_signed<int16_t>(properties, "elev", sigc::mem_fun(m_arpt, &AirportsDb::Airport::set_elevation));
	// do not read back bounding box, recompute it
	attr_airport_typecode(properties, "type", sigc::mem_fun(m_arpt, &AirportsDb::Airport::set_typecode));
	attr_unsigned<uint8_t>(properties, "frules", sigc::mem_fun(m_arpt, &AirportsDb::Airport::set_flightrules));
	attr<AirportsDb::Airport::label_placement_t>(properties, "label_placement", sigc::mem_fun(m_arpt, &AirportsDb::Airport::set_label_placement));
	attr_modtime(properties, sigc::mem_fun(m_arpt, &AirportsDb::Airport::set_modtime));
}

void DbXmlImporter::parse_airport_rwy(const AttributeList & properties)
{
	m_arpt.add_rwy(AirportsDb::Airport::Runway("", "", 0, 0, "", m_arpt.get_coord(), m_arpt.get_coord(), 0, 0, 0, 0, m_arpt.get_elevation(), 0, 0, 0, 0, m_arpt.get_elevation(), 0));
	AirportsDb::Airport::Runway& rwy(m_arpt.get_rwy(m_arpt.get_nrrwy()-1));
	attr_unsigned<uint16_t>(properties, "length", sigc::mem_fun(rwy, &AirportsDb::Airport::Runway::set_length));
	attr_unsigned<uint16_t>(properties, "width", sigc::mem_fun(rwy, &AirportsDb::Airport::Runway::set_width));
	attr<const Glib::ustring&>(properties, "surface", sigc::mem_fun(rwy, &AirportsDb::Airport::Runway::set_surface));
	attr_unsigned<uint16_t>(properties, "flags", sigc::mem_fun(rwy, &AirportsDb::Airport::Runway::set_flags));
}

void DbXmlImporter::parse_airport_rwy_he(const AttributeList & properties)
{
	AirportsDb::Airport::Runway& rwy(m_arpt.get_rwy(m_arpt.get_nrrwy()-1));
	attr_unsigned<uint16_t>(properties, "tda", sigc::mem_fun(rwy, &AirportsDb::Airport::Runway::set_he_tda));
	attr_unsigned<uint16_t>(properties, "lda", sigc::mem_fun(rwy, &AirportsDb::Airport::Runway::set_he_lda));
	attr_unsigned<uint16_t>(properties, "disp", sigc::mem_fun(rwy, &AirportsDb::Airport::Runway::set_he_disp));
	attr_unsigned<uint16_t>(properties, "hdg", sigc::mem_fun(rwy, &AirportsDb::Airport::Runway::set_he_hdg));
	attr_unsigned<int16_t>(properties, "elev", sigc::mem_fun(rwy, &AirportsDb::Airport::Runway::set_he_elev));
	attr<const Glib::ustring&>(properties, "ident", sigc::mem_fun(rwy, &AirportsDb::Airport::Runway::set_ident_he));
	attr<Point>(properties, "", sigc::mem_fun(rwy, &AirportsDb::Airport::Runway::set_he_coord));
	for (unsigned int i = 0; i < AirportsDb::Airport::Runway::nr_lights; ++i) {
		std::ostringstream oss;
		oss << "light" << i;
		attr_unsigned<uint8_t>(properties, oss.str(), sigc::bind<0>(sigc::mem_fun(rwy, &AirportsDb::Airport::Runway::set_he_light), i));
	}
}

void DbXmlImporter::parse_airport_rwy_le(const AttributeList & properties)
{
	AirportsDb::Airport::Runway& rwy(m_arpt.get_rwy(m_arpt.get_nrrwy()-1));
	attr_unsigned<uint16_t>(properties, "tda", sigc::mem_fun(rwy, &AirportsDb::Airport::Runway::set_le_tda));
	attr_unsigned<uint16_t>(properties, "lda", sigc::mem_fun(rwy, &AirportsDb::Airport::Runway::set_le_lda));
	attr_unsigned<uint16_t>(properties, "disp", sigc::mem_fun(rwy, &AirportsDb::Airport::Runway::set_le_disp));
	attr_unsigned<uint16_t>(properties, "hdg", sigc::mem_fun(rwy, &AirportsDb::Airport::Runway::set_le_hdg));
	attr_unsigned<int16_t>(properties, "elev", sigc::mem_fun(rwy, &AirportsDb::Airport::Runway::set_le_elev));
	attr<const Glib::ustring&>(properties, "ident", sigc::mem_fun(rwy, &AirportsDb::Airport::Runway::set_ident_le));
	attr<Point>(properties, "", sigc::mem_fun(rwy, &AirportsDb::Airport::Runway::set_le_coord));
	for (unsigned int i = 0; i < AirportsDb::Airport::Runway::nr_lights; ++i) {
		std::ostringstream oss;
		oss << "light" << i;
		attr_unsigned<uint8_t>(properties, oss.str(), sigc::bind<0>(sigc::mem_fun(rwy, &AirportsDb::Airport::Runway::set_le_light), i));
	}
}

void DbXmlImporter::parse_airport_helipad(const AttributeList & properties)
{
	m_arpt.add_helipad(AirportsDb::Airport::Helipad("", 0, 0, "", m_arpt.get_coord(), 0, m_arpt.get_elevation(), 0));
	AirportsDb::Airport::Helipad& hp(m_arpt.get_helipad(m_arpt.get_nrhelipads()-1));
	attr<const Glib::ustring&>(properties, "ident", sigc::mem_fun(hp, &AirportsDb::Airport::Helipad::set_ident));
	attr_unsigned<uint16_t>(properties, "length", sigc::mem_fun(hp, &AirportsDb::Airport::Helipad::set_length));
	attr_unsigned<uint16_t>(properties, "width", sigc::mem_fun(hp, &AirportsDb::Airport::Helipad::set_width));
	attr<const Glib::ustring&>(properties, "surface", sigc::mem_fun(hp, &AirportsDb::Airport::Helipad::set_surface));
	attr<Point>(properties, "", sigc::mem_fun(hp, &AirportsDb::Airport::Helipad::set_coord));
	attr_unsigned<uint16_t>(properties, "hdg", sigc::mem_fun(hp, &AirportsDb::Airport::Helipad::set_hdg));
	attr_unsigned<int16_t>(properties, "elev", sigc::mem_fun(hp, &AirportsDb::Airport::Helipad::set_elev));
	attr_unsigned<uint16_t>(properties, "flags", sigc::mem_fun(hp, &AirportsDb::Airport::Helipad::set_flags));
}

void DbXmlImporter::parse_airport_comm(const AttributeList & properties)
{
	AirportsDb::Airport::Comm comm("", "", "");
	attr<const Glib::ustring&>(properties, "name", sigc::mem_fun(comm, &AirportsDb::Airport::Comm::set_name));
	attr<const Glib::ustring&>(properties, "sector", sigc::mem_fun(comm, &AirportsDb::Airport::Comm::set_sector));
	attr<const Glib::ustring&>(properties, "oprhours", sigc::mem_fun(comm, &AirportsDb::Airport::Comm::set_oprhours));
	if (!attr_unsigned<uint32_t>(properties, "freq0", sigc::bind<0>(sigc::mem_fun(comm, &AirportsDb::Airport::Comm::set_freq), 0)))
		attr_unsigned<uint32_t>(properties, "freq", sigc::bind<0>(sigc::mem_fun(comm, &AirportsDb::Airport::Comm::set_freq), 0));
	attr_unsigned<uint32_t>(properties, "freq1", sigc::bind<0>(sigc::mem_fun(comm, &AirportsDb::Airport::Comm::set_freq), 1));
	attr_unsigned<uint32_t>(properties, "freq2", sigc::bind<0>(sigc::mem_fun(comm, &AirportsDb::Airport::Comm::set_freq), 2));
	attr_unsigned<uint32_t>(properties, "freq3", sigc::bind<0>(sigc::mem_fun(comm, &AirportsDb::Airport::Comm::set_freq), 3));
	attr_unsigned<uint32_t>(properties, "freq4", sigc::bind<0>(sigc::mem_fun(comm, &AirportsDb::Airport::Comm::set_freq), 4));
	m_arpt.add_comm(comm);
}

void DbXmlImporter::parse_airport_vfrroute(const AttributeList & properties)
{
	AirportsDb::Airport::VFRRoute rte("");
	attr<const Glib::ustring&>(properties, "name", sigc::mem_fun(rte, &AirportsDb::Airport::VFRRoute::set_name));
	attr_signed<int32_t>(properties, "minalt", sigc::mem_fun(rte, &AirportsDb::Airport::VFRRoute::set_minalt));
	attr_signed<int32_t>(properties, "maxalt", sigc::mem_fun(rte, &AirportsDb::Airport::VFRRoute::set_maxalt));
	m_arpt.add_vfrrte(rte);
}

void DbXmlImporter::parse_airport_vfrpoint(const AttributeList & properties)
{
	AirportsDb::Airport::VFRRoute::VFRRoutePoint pt("", m_arpt.get_coord(), 1000, AirportsDb::Airport::label_off, 
							'C', AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_arrival,
							AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_atorabove, true);
	attr<const Glib::ustring&>(properties, "name", sigc::mem_fun(pt, &AirportsDb::Airport::VFRRoute::VFRRoutePoint::set_name));
	attr<Point>(properties, "", sigc::mem_fun(pt, &AirportsDb::Airport::VFRRoute::VFRRoutePoint::set_coord));
	attr_signed<int16_t>(properties, "alt", sigc::mem_fun(pt, &AirportsDb::Airport::VFRRoute::VFRRoutePoint::set_altitude));
	attr<AirportsDb::Airport::label_placement_t>(properties, "label_placement", sigc::mem_fun(pt, &AirportsDb::Airport::VFRRoute::VFRRoutePoint::set_label_placement));
	attr<char>(properties, "symbol", sigc::mem_fun(pt, &AirportsDb::Airport::VFRRoute::VFRRoutePoint::set_ptsym));
	attr<AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_t>(properties, "pathcode", sigc::mem_fun(pt, &AirportsDb::Airport::VFRRoute::VFRRoutePoint::set_pathcode));
	attr<AirportsDb::Airport::VFRRoute::VFRRoutePoint::altcode_t>(properties, "altcode", sigc::mem_fun(pt, &AirportsDb::Airport::VFRRoute::VFRRoutePoint::set_altcode));
	attr<bool>(properties, "atairport", sigc::mem_fun(pt, &AirportsDb::Airport::VFRRoute::VFRRoutePoint::set_at_airport));
	AirportsDb::Airport::VFRRoute& rte(m_arpt.get_vfrrte(m_arpt.get_nrvfrrte()-1));
	rte.add_point(pt);
}

void DbXmlImporter::parse_airport_linefeature(const AttributeList & properties)
{
	m_arpt.add_linefeature(AirportsDb::Airport::Polyline());
	AirportsDb::Airport::Polyline& pl(m_arpt.get_linefeature(m_arpt.get_nrlinefeatures()-1));
	attr<const Glib::ustring&>(properties, "name", sigc::mem_fun(pl, &AirportsDb::Airport::Polyline::set_name));
	attr<const Glib::ustring&>(properties, "surface", sigc::mem_fun(pl, &AirportsDb::Airport::Polyline::set_surface));
	attr_unsigned<uint16_t>(properties, "flags", sigc::mem_fun(pl, &AirportsDb::Airport::Polyline::set_flags));
}

void DbXmlImporter::parse_airport_linefeature_node(const AttributeList & properties)
{
	AirportsDb::Airport::Polyline& pl(m_arpt.get_linefeature(m_arpt.get_nrlinefeatures()-1));
	pl.push_back(AirportsDb::Airport::PolylineNode());
	AirportsDb::Airport::PolylineNode& pn(pl.back());
	attr<Point>(properties, "", sigc::mem_fun(pn, &AirportsDb::Airport::PolylineNode::set_point));
	attr<Point>(properties, "cp", sigc::mem_fun(pn, &AirportsDb::Airport::PolylineNode::set_prev_controlpoint));
	attr<Point>(properties, "cn", sigc::mem_fun(pn, &AirportsDb::Airport::PolylineNode::set_next_controlpoint));
	attr_unsigned<uint16_t>(properties, "flags", sigc::mem_fun(pn, &AirportsDb::Airport::PolylineNode::set_flags));
	for (unsigned int i = 0; i < AirportsDb::Airport::PolylineNode::nr_features; ++i) {
		std::ostringstream oss;
		oss << "feature" << i;
		attr_unsigned<AirportsDb::Airport::PolylineNode::feature_t>(properties, oss.str(), sigc::bind<0>(sigc::mem_fun(pn, &AirportsDb::Airport::PolylineNode::set_feature), i));
	}
}

void DbXmlImporter::parse_airport_pointfeature(const AttributeList & properties)
{
	m_arpt.add_pointfeature(AirportsDb::Airport::PointFeature());
	AirportsDb::Airport::PointFeature& pf(m_arpt.get_pointfeature(m_arpt.get_nrpointfeatures()-1));
	attr_unsigned<AirportsDb::Airport::PointFeature::feature_t>(properties, "feature", sigc::mem_fun(pf, &AirportsDb::Airport::PointFeature::set_feature));
	attr<Point>(properties, "", sigc::mem_fun(pf, &AirportsDb::Airport::PointFeature::set_coord));
	attr_unsigned<uint16_t>(properties, "hdg", sigc::mem_fun(pf, &AirportsDb::Airport::PointFeature::set_hdg));
	attr_unsigned<int16_t>(properties, "elev", sigc::mem_fun(pf, &AirportsDb::Airport::PointFeature::set_elev));
	attr_unsigned<unsigned int>(properties, "subtype", sigc::mem_fun(pf, &AirportsDb::Airport::PointFeature::set_subtype));
	attr_signed<int>(properties, "attr1", sigc::mem_fun(pf, &AirportsDb::Airport::PointFeature::set_attr1));
	attr_signed<int>(properties, "attr2", sigc::mem_fun(pf, &AirportsDb::Airport::PointFeature::set_attr2));
	attr<const Glib::ustring&>(properties, "name", sigc::mem_fun(pf, &AirportsDb::Airport::PointFeature::set_name));
	attr<const Glib::ustring&>(properties, "rwyident", sigc::mem_fun(pf, &AirportsDb::Airport::PointFeature::set_rwyident));
}

void DbXmlImporter::parse_airport_fas(const AttributeList & properties)
{
	m_arpt.add_finalapproachsegment(AirportsDb::Airport::MinimalFAS());
	AirportsDb::Airport::MinimalFAS& fas(m_arpt.get_finalapproachsegment(m_arpt.get_nrfinalapproachsegments()-1));
	attr<const Glib::ustring&>(properties, "refpathid", sigc::mem_fun(fas, &AirportsDb::Airport::MinimalFAS::set_referencepathident));
	attr<Point>(properties, "ftp", sigc::mem_fun(fas, &AirportsDb::Airport::MinimalFAS::set_ftp));
	attr<Point>(properties, "fpap", sigc::mem_fun(fas, &AirportsDb::Airport::MinimalFAS::set_fpap));
	attr_unsigned<uint8_t>(properties, "optyp", sigc::mem_fun(fas, &AirportsDb::Airport::MinimalFAS::set_optyp));
	attr_unsigned<uint8_t>(properties, "rwy", sigc::mem_fun(fas, &AirportsDb::Airport::MinimalFAS::set_rwy));
	attr_unsigned<uint8_t>(properties, "rte", sigc::mem_fun(fas, &AirportsDb::Airport::MinimalFAS::set_rte));
	attr_unsigned<uint8_t>(properties, "refpathds", sigc::mem_fun(fas, &AirportsDb::Airport::MinimalFAS::set_referencepathdataselector));
	attr_unsigned<int16_t>(properties, "ftpheight", sigc::mem_fun(fas, &AirportsDb::Airport::MinimalFAS::set_ftp_height));
	attr_unsigned<uint16_t>(properties, "apchtch", sigc::mem_fun(fas, &AirportsDb::Airport::MinimalFAS::set_approach_tch));
	attr_unsigned<uint16_t>(properties, "gpa", sigc::mem_fun(fas, &AirportsDb::Airport::MinimalFAS::set_glidepathangle));
	attr_unsigned<uint8_t>(properties, "cwidth", sigc::mem_fun(fas, &AirportsDb::Airport::MinimalFAS::set_coursewidth));
	attr_unsigned<uint8_t>(properties, "dlenoffs", sigc::mem_fun(fas, &AirportsDb::Airport::MinimalFAS::set_dlengthoffset));
	attr_unsigned<uint8_t>(properties, "hal", sigc::mem_fun(fas, &AirportsDb::Airport::MinimalFAS::set_hal));
	attr_unsigned<uint8_t>(properties, "val", sigc::mem_fun(fas, &AirportsDb::Airport::MinimalFAS::set_val));
}

void DbXmlImporter::open_airports_db(void)
{
	if (m_airportsdb)
		return;
#ifdef HAVE_PQXX
	if (m_pgsql)
		m_airportsdb.reset(new AirportsPGDb(m_outputdir));
	else
#endif
		m_airportsdb.reset(new AirportsDb(m_outputdir));
     	m_airportsdb->set_exclusive(true);
	m_airportsdb->sync_off();
	if (m_purgedb)
		m_airportsdb->purgedb();
}

void DbXmlImporter::end_airport(void)
{
	m_arpt.recompute_bbox();
	open_airports_db();
	std::cerr << "Saving airport " << m_arpt.get_sourceid() << " address " << m_arpt.get_address() << std::endl;
	m_airportsdb->save(m_arpt);
	m_arpt = AirportsDb::Airport();
}

void DbXmlImporter::parse_airspace(const AttributeList & properties)
{
	m_aspc = AirspacesDb::Airspace();
	attr<const Glib::ustring&>(properties, "sourceid", sigc::mem_fun(m_aspc, &AirspacesDb::Airspace::set_sourceid));
	{
		open_airspaces_db();
		AirspacesDb::elementvector_t ev(m_airspacesdb->find_by_sourceid(m_aspc.get_sourceid()));
		if (ev.size() == 1) {
			std::cerr << "erasing airport " << ev.begin()->get_sourceid() << " address " << ev.begin()->get_address() << std::endl;
			m_airspacesdb->erase(*ev.begin());
		}
	}
	// do not read back bounding box, recompute it
	attr<Point>(properties, "label", sigc::mem_fun(m_aspc, &AirspacesDb::Airspace::set_labelcoord));
	attr<const Glib::ustring&>(properties, "icao", sigc::mem_fun(m_aspc, &AirspacesDb::Airspace::set_icao));
	attr<const Glib::ustring&>(properties, "name", sigc::mem_fun(m_aspc, &AirspacesDb::Airspace::set_name));
	attr<const Glib::ustring&>(properties, "ident", sigc::mem_fun(m_aspc, &AirspacesDb::Airspace::set_ident));
	attr<const Glib::ustring&>(properties, "classexcept", sigc::mem_fun(m_aspc, &AirspacesDb::Airspace::set_classexcept));
	attr<const Glib::ustring&>(properties, "efftime", sigc::mem_fun(m_aspc, &AirspacesDb::Airspace::set_efftime));
	attr<const Glib::ustring&>(properties, "note", sigc::mem_fun(m_aspc, &AirspacesDb::Airspace::set_note));
	attr<const Glib::ustring&>(properties, "commname", sigc::mem_fun(m_aspc, &AirspacesDb::Airspace::set_commname));
	attr_unsigned<uint32_t>(properties, "commfreq0", sigc::bind<0>(sigc::mem_fun(m_aspc, &AirspacesDb::Airspace::set_commfreq), 0));
	attr_unsigned<uint32_t>(properties, "commfreq1", sigc::bind<0>(sigc::mem_fun(m_aspc, &AirspacesDb::Airspace::set_commfreq), 1));
	attr_airspace_class(properties, "class", sigc::mem_fun(m_aspc, &AirspacesDb::Airspace::set_bdryclass), sigc::mem_fun(m_aspc, &AirspacesDb::Airspace::set_typecode));
	attr_signed<int32_t>(properties, "gndelevmin", sigc::mem_fun(m_aspc, &AirspacesDb::Airspace::set_gndelevmin));
	attr_signed<int32_t>(properties, "gndelevmax", sigc::mem_fun(m_aspc, &AirspacesDb::Airspace::set_gndelevmax));
	attr_unsigned<uint32_t>(properties, "width", sigc::mem_fun(m_aspc, &AirspacesDb::Airspace::set_width));
	attr_signed<int32_t>(properties, "altlwr", sigc::mem_fun(m_aspc, &AirspacesDb::Airspace::set_altlwr));
	attr_signed<int32_t>(properties, "altupr", sigc::mem_fun(m_aspc, &AirspacesDb::Airspace::set_altupr));
	attr_unsigned<uint8_t>(properties, "altlwrflags", sigc::mem_fun(m_aspc, &AirspacesDb::Airspace::set_altlwrflags));
	attr_unsigned<uint8_t>(properties, "altuprflags", sigc::mem_fun(m_aspc, &AirspacesDb::Airspace::set_altuprflags));
	attr<AirspacesDb::Airspace::label_placement_t>(properties, "label_placement", sigc::mem_fun(m_aspc, &AirspacesDb::Airspace::set_label_placement));
	attr_modtime(properties, sigc::mem_fun(m_aspc, &AirspacesDb::Airspace::set_modtime));
}

void DbXmlImporter::parse_airspace_segment(const AttributeList & properties)
{
	AirspacesDb::Airspace::Segment seg;
	attr<Point>(properties, "c1", sigc::mem_fun(seg, &AirspacesDb::Airspace::Segment::set_coord1));
	attr<Point>(properties, "c2", sigc::mem_fun(seg, &AirspacesDb::Airspace::Segment::set_coord2));
	attr<Point>(properties, "c0", sigc::mem_fun(seg, &AirspacesDb::Airspace::Segment::set_coord0));
	attr<char>(properties, "shape", sigc::mem_fun(seg, &AirspacesDb::Airspace::Segment::set_shape));
	attr_signed<int32_t>(properties, "radius1", sigc::mem_fun(seg, &AirspacesDb::Airspace::Segment::set_radius1));
	attr_signed<int32_t>(properties, "radius2", sigc::mem_fun(seg, &AirspacesDb::Airspace::Segment::set_radius2));
	m_aspc.add_segment(seg);
}

void DbXmlImporter::parse_airspace_component(const AttributeList & properties)
{
	AirspacesDb::Airspace::Component comp;
	attr<const Glib::ustring&>(properties, "icao", sigc::mem_fun(comp, &AirspacesDb::Airspace::Component::set_icao));
	attr_airspace_class(properties, "class", sigc::mem_fun(comp, &AirspacesDb::Airspace::Component::set_bdryclass), sigc::mem_fun(comp, &AirspacesDb::Airspace::Component::set_typecode));
	attr<const Glib::ustring&>(properties, "operator", sigc::mem_fun(comp, &AirspacesDb::Airspace::Component::set_operator_string));
	m_aspc.add_component(comp);
}

void DbXmlImporter::parse_airspace_polygon(const AttributeList & properties)
{
	m_aspc.get_polygon().push_back(PolygonHole());
}

void DbXmlImporter::parse_airspace_polygonint(const AttributeList & properties)
{
	PolygonHole& ph(m_aspc.get_polygon().back());
	ph.add_interior(PolygonSimple());
}

void DbXmlImporter::parse_airspace_polygonpoint(const AttributeList & properties)
{
	PolygonHole& ph(m_aspc.get_polygon().back());
	attr<Point>(properties, "", sigc::bound_mem_functor1<void,PolygonSimple,const Point&>(ph.get_exterior(), &PolygonSimple::push_back));
}

void DbXmlImporter::parse_airspace_polygonintpoint(const AttributeList & properties)
{
	PolygonHole& ph(m_aspc.get_polygon().back());
	attr<Point>(properties, "", sigc::bound_mem_functor1<void,PolygonSimple,const Point&>(ph[ph.get_nrinterior() - 1], &PolygonSimple::push_back));
}

void DbXmlImporter::open_airspaces_db(void)
{
	if (m_airspacesdb)
		return;
#ifdef HAVE_PQXX
	if (m_pgsql)
		m_airspacesdb.reset(new AirspacesPGDb(m_outputdir));
	else
#endif
		m_airspacesdb.reset(new AirspacesDb(m_outputdir));
	m_airspacesdb->set_exclusive(true);
	m_airspacesdb->sync_off();
	if (m_purgedb)
		m_airspacesdb->purgedb();
}

void DbXmlImporter::end_airspace(void)
{
	open_airspaces_db();
	// only recompute for non-composited airspaces
	if (!m_aspc.get_nrcomponents()) {
		m_aspc.recompute_lineapprox(*m_airspacesdb.get());
	} else {
		std::pair<asdeps_t::iterator,bool> ins(m_asdeps.insert(std::make_pair(AirspaceDeps(m_aspc), 0U)));
		if (!ins.second)
			std::cerr << "Warning: duplicate airspace " << m_aspc.get_icao() << '/' << m_aspc.get_class_string() << std::endl;
	}
	m_aspc.recompute_bbox();
	std::cerr << "Saving airspace " << m_aspc.get_sourceid() << " address " << m_aspc.get_address() << std::endl;
	m_airspacesdb->save(m_aspc);
	m_aspc = AirspacesDb::Airspace();
}

void DbXmlImporter::parse_navaid(const AttributeList & properties)
{
	m_nav = NavaidsDb::Navaid();
	attr<const Glib::ustring&>(properties, "sourceid", sigc::mem_fun(m_nav, &NavaidsDb::Navaid::set_sourceid));
	{
		open_navaids_db();
		NavaidsDb::elementvector_t ev(m_navaidsdb->find_by_sourceid(m_nav.get_sourceid()));
		if (ev.size() == 1) {
			std::cerr << "erasing navaid " << ev.begin()->get_sourceid() << " address " << ev.begin()->get_address() << std::endl;
			m_navaidsdb->erase(*ev.begin());
		}
	}
	attr<Point>(properties, "", sigc::mem_fun(m_nav, &NavaidsDb::Navaid::set_coord));
	attr<const Glib::ustring&>(properties, "icao", sigc::mem_fun(m_nav, &NavaidsDb::Navaid::set_icao));
	attr<const Glib::ustring&>(properties, "name", sigc::mem_fun(m_nav, &NavaidsDb::Navaid::set_name));
	attr<const Glib::ustring&>(properties, "authority", sigc::mem_fun(m_nav, &NavaidsDb::Navaid::set_authority));
	attr<Point>(properties, "dme", sigc::mem_fun(m_nav, &NavaidsDb::Navaid::set_dmecoord));
	attr_signed<int32_t>(properties, "elev", sigc::mem_fun(m_nav, &NavaidsDb::Navaid::set_elev));
	attr_signed<int32_t>(properties, "range", sigc::mem_fun(m_nav, &NavaidsDb::Navaid::set_range));
	attr_unsigned<uint32_t>(properties, "freq", sigc::mem_fun(m_nav, &NavaidsDb::Navaid::set_frequency));
	attr<NavaidsDb::Navaid::navaid_type_t>(properties, "type", sigc::mem_fun(m_nav, &NavaidsDb::Navaid::set_navaid_type));
	attr<NavaidsDb::Navaid::label_placement_t>(properties, "label_placement", sigc::mem_fun(m_nav, &NavaidsDb::Navaid::set_label_placement));
	attr<bool>(properties, "inservice", sigc::mem_fun(m_nav, &NavaidsDb::Navaid::set_inservice));
	attr_modtime(properties, sigc::mem_fun(m_nav, &NavaidsDb::Navaid::set_modtime));
}

void DbXmlImporter::open_navaids_db(void)
{
	if (m_navaidsdb)
		return;
#ifdef HAVE_PQXX
	if (m_pgsql)
		m_navaidsdb.reset(new NavaidsPGDb(m_outputdir));
	else
#endif
		m_navaidsdb.reset(new NavaidsDb(m_outputdir));
	m_navaidsdb->set_exclusive(true);
	m_navaidsdb->sync_off();
	if (m_purgedb)
		m_navaidsdb->purgedb();
}

void DbXmlImporter::end_navaid(void)
{
	open_navaids_db();
	std::cerr << "Saving navaid " << m_nav.get_sourceid() << " address " << m_nav.get_address() << std::endl;
	m_navaidsdb->save(m_nav);
	m_nav = NavaidsDb::Navaid();
}

void DbXmlImporter::parse_waypoint(const AttributeList & properties)
{
	m_wpt = WaypointsDb::Waypoint();
	attr<const Glib::ustring&>(properties, "sourceid", sigc::mem_fun(m_wpt, &WaypointsDb::Waypoint::set_sourceid));
	{
		open_waypoints_db();
		WaypointsDb::elementvector_t ev(m_waypointsdb->find_by_sourceid(m_wpt.get_sourceid()));
		if (ev.size() == 1) {
			std::cerr << "erasing waypoint " << ev.begin()->get_sourceid() << " address " << ev.begin()->get_address() << std::endl;
			m_waypointsdb->erase(*ev.begin());
		}
	}
	m_wpt.set_type(WaypointsDb::Waypoint::type_unknown);
	attr<Point>(properties, "", sigc::mem_fun(m_wpt, &WaypointsDb::Waypoint::set_coord));
	attr<const Glib::ustring&>(properties, "icao", sigc::mem_fun(m_wpt, &WaypointsDb::Waypoint::set_icao));
	attr<const Glib::ustring&>(properties, "name", sigc::mem_fun(m_wpt, &WaypointsDb::Waypoint::set_name));
	attr<const Glib::ustring&>(properties, "authority", sigc::mem_fun(m_wpt, &WaypointsDb::Waypoint::set_authority));
	attr_waypoint_usage(properties, "usage", sigc::mem_fun(m_wpt, &WaypointsDb::Waypoint::set_usage));
	attr_waypoint_type(properties, "type", sigc::mem_fun(m_wpt, &WaypointsDb::Waypoint::set_type));
	attr<NavaidsDb::Navaid::label_placement_t>(properties, "label_placement", sigc::mem_fun(m_wpt, &WaypointsDb::Waypoint::set_label_placement));
	attr_modtime(properties, sigc::mem_fun(m_wpt, &WaypointsDb::Waypoint::set_modtime));
}

void DbXmlImporter::open_waypoints_db(void)
{
	if (m_waypointsdb)
		return;
#ifdef HAVE_PQXX
	if (m_pgsql)
		m_waypointsdb.reset(new WaypointsPGDb(m_outputdir));
	else
#endif
		m_waypointsdb.reset(new WaypointsDb(m_outputdir));
	m_waypointsdb->set_exclusive(true);
	m_waypointsdb->sync_off();
	if (m_purgedb)
		m_waypointsdb->purgedb();
}

void DbXmlImporter::end_waypoint(void)
{
	open_waypoints_db();
	std::cerr << "Saving waypoint " << m_wpt.get_sourceid() << " address " << m_wpt.get_address() << std::endl;
	m_waypointsdb->save(m_wpt);
	m_wpt = WaypointsDb::Waypoint();
}

void DbXmlImporter::parse_airway(const AttributeList & properties)
{
	m_awy = AirwaysDb::Airway();
	attr<const Glib::ustring&>(properties, "sourceid", sigc::mem_fun(m_awy, &AirwaysDb::Airway::set_sourceid));
	{
		open_airways_db();
		AirwaysDb::elementvector_t ev(m_airwaysdb->find_by_sourceid(m_wpt.get_sourceid()));
		if (ev.size() == 1) {
			std::cerr << "erasing airway " << ev.begin()->get_sourceid() << " address " << ev.begin()->get_address() << std::endl;
			m_airwaysdb->erase(*ev.begin());
		}
	}
	attr<Point>(properties, "begin", sigc::mem_fun(m_awy, &AirwaysDb::Airway::set_begin_coord));
	attr<Point>(properties, "end", sigc::mem_fun(m_awy, &AirwaysDb::Airway::set_end_coord));
	attr<const Glib::ustring&>(properties, "beginname", sigc::mem_fun(m_awy, &AirwaysDb::Airway::set_begin_name));
	attr<const Glib::ustring&>(properties, "endname", sigc::mem_fun(m_awy, &AirwaysDb::Airway::set_end_name));
	attr<const Glib::ustring&>(properties, "name", sigc::mem_fun(m_awy, &AirwaysDb::Airway::set_name));
	attr_signed<int16_t>(properties, "baselevel", sigc::mem_fun(m_awy, &AirwaysDb::Airway::set_base_level));
	attr_signed<int16_t>(properties, "toplevel", sigc::mem_fun(m_awy, &AirwaysDb::Airway::set_top_level));
	attr_signed<int16_t>(properties, "terrainelev", sigc::mem_fun(m_awy, &AirwaysDb::Airway::set_terrain_elev));
	attr_signed<int16_t>(properties, "corridor5elev", sigc::mem_fun(m_awy, &AirwaysDb::Airway::set_corridor5_elev));
	attr<AirwaysDb::Airway::airway_type_t>(properties, "type", sigc::mem_fun(m_awy, &AirwaysDb::Airway::set_type));
	attr<AirwaysDb::Airway::label_placement_t>(properties, "label_placement", sigc::mem_fun(m_awy, &AirwaysDb::Airway::set_label_placement));
	attr_modtime(properties, sigc::mem_fun(m_awy, &AirwaysDb::Airway::set_modtime));
}

void DbXmlImporter::open_airways_db(void)
{
	if (m_airwaysdb)
		return;
#ifdef HAVE_PQXX
	if (m_pgsql)
		m_airwaysdb.reset(new AirwaysPGDb(m_outputdir));
	else
#endif
		m_airwaysdb.reset(new AirwaysDb(m_outputdir));
	m_airwaysdb->set_exclusive(true);
	m_airwaysdb->sync_off();
	if (m_purgedb)
		m_airwaysdb->purgedb();
}

void DbXmlImporter::end_airway(void)
{
	open_airways_db();
	std::cerr << "Saving airway " << m_awy.get_sourceid() << " address " << m_awy.get_address() << std::endl;
	m_airwaysdb->save(m_awy);
	m_awy = AirwaysDb::Airway();
}


void DbXmlImporter::parse_track(const AttributeList & properties)
{
	m_trk = TracksDb::Track();
	attr<const Glib::ustring&>(properties, "sourceid", sigc::mem_fun(m_trk, &TracksDb::Track::set_sourceid));
	{
		open_tracks_db();
		TracksDb::elementvector_t ev(m_tracksdb->find_by_sourceid(m_trk.get_sourceid()));
		if (ev.size() == 1) {
			std::cerr << "erasing track " << ev.begin()->get_sourceid() << " address " << ev.begin()->get_address() << std::endl;
			m_tracksdb->erase(*ev.begin());
		}
	}
	attr<const Glib::ustring&>(properties, "fromicao", sigc::mem_fun(m_trk, &TracksDb::Track::set_fromicao));
	attr<const Glib::ustring&>(properties, "fromname", sigc::mem_fun(m_trk, &TracksDb::Track::set_fromname));
	attr<const Glib::ustring&>(properties, "toicao", sigc::mem_fun(m_trk, &TracksDb::Track::set_toicao));
	attr<const Glib::ustring&>(properties, "toname", sigc::mem_fun(m_trk, &TracksDb::Track::set_toname));
	attr<const Glib::ustring&>(properties, "notes", sigc::mem_fun(m_trk, &TracksDb::Track::set_notes));
	attr_unsigned<time_t>(properties, "offblocktime", sigc::mem_fun(m_trk, &TracksDb::Track::set_offblocktime));
	attr_unsigned<time_t>(properties, "takeofftime", sigc::mem_fun(m_trk, &TracksDb::Track::set_takeofftime));
	attr_unsigned<time_t>(properties, "landingtime", sigc::mem_fun(m_trk, &TracksDb::Track::set_landingtime));
	attr_unsigned<time_t>(properties, "onblocktime", sigc::mem_fun(m_trk, &TracksDb::Track::set_onblocktime));
	attr_modtime(properties, sigc::mem_fun(m_trk, &TracksDb::Track::set_modtime));
}

void DbXmlImporter::parse_track_point(const AttributeList & properties)
{
	TracksDb::Track::TrackPoint tp;
	tp.set_alt_invalid();
	attr_unsigned<uint64_t>(properties, "time", sigc::mem_fun(tp, &TracksDb::Track::TrackPoint::set_timefrac8));
	attr<Point>(properties, "", sigc::mem_fun(tp, &TracksDb::Track::TrackPoint::set_coord));
	attr_signed<TracksDb::Track::TrackPoint::alt_t>(properties, "alt", sigc::mem_fun(tp, &TracksDb::Track::TrackPoint::set_alt));
	m_trk.append(tp);
}

void DbXmlImporter::open_tracks_db(void)
{
	if (m_tracksdb)
		return;
#ifdef HAVE_PQXX
       	if (m_pgsql)
		m_tracksdb.reset(new TracksPGDb(m_outputdir));
	else
#endif
		m_tracksdb.reset(new TracksDb(m_outputdir));
	m_tracksdb->set_exclusive(true);
	m_tracksdb->sync_off();
	if (m_purgedb)
		m_tracksdb->purgedb();
}

void DbXmlImporter::end_track(void)
{
	open_tracks_db();
	std::cerr << "Saving track " << m_trk.get_sourceid() << " address " << m_trk.get_address() << std::endl;
	m_trk.sort();
	m_trk.recompute_bbox();
	m_tracksdb->save(m_trk);
	m_trk = TracksDb::Track();
}

void DbXmlImporter::parse_airportdelete(const AttributeList & properties)
{
	ValueObject<Glib::ustring> val("");
	bool r = attr<Glib::ustring>(properties, "sourceid", sigc::mem_fun(val, &ValueObject<Glib::ustring>::set));
	if (!r)
		return;
	open_airports_db();
	AirportsDb::elementvector_t ev(m_airportsdb->find_by_sourceid(val.get(), 0, AirportsDb::comp_exact_casesensitive, 2, true));
	if (ev.empty()) {
		std::cerr << "airportdelete: no airport with sourceid " << val.get() << " found" << std::endl;
		return;
	}
	if (ev.size() > 1) {
		std::cerr << "airportdelete: multiple airports with sourceid " << val.get() << " found" << std::endl;
		return;
	}
	std::cerr << "Deleting airport " << ev.front().get_sourceid() << std::endl;
	m_airportsdb->erase(ev.front());
}

void DbXmlImporter::parse_airspacedelete(const AttributeList & properties)
{
	ValueObject<Glib::ustring> val("");
	bool r = attr<Glib::ustring>(properties, "sourceid", sigc::mem_fun(val, &ValueObject<Glib::ustring>::set));
	if (!r)
		return;
	open_airspaces_db();
	AirspacesDb::elementvector_t ev(m_airspacesdb->find_by_sourceid(val.get(), 0, AirspacesDb::comp_exact_casesensitive, 2, true));
	if (ev.empty()) {
		std::cerr << "airspacedelete: no airspace with sourceid " << val.get() << " found" << std::endl;
		return;
	}
	if (ev.size() > 1) {
		std::cerr << "airspacedelete: multiple airspaces with sourceid " << val.get() << " found" << std::endl;
		return;
	}
	std::cerr << "Deleting airspace " << ev.front().get_sourceid() << std::endl;
	m_airspacesdb->erase(ev.front());
}

void DbXmlImporter::parse_navaiddelete(const AttributeList & properties)
{
	ValueObject<Glib::ustring> val("");
	bool r = attr<Glib::ustring>(properties, "sourceid", sigc::mem_fun(val, &ValueObject<Glib::ustring>::set));
	if (!r)
		return;
	open_navaids_db();
	NavaidsDb::elementvector_t ev(m_navaidsdb->find_by_sourceid(val.get(), 0, NavaidsDb::comp_exact_casesensitive, 2, true));
	if (ev.empty()) {
		std::cerr << "navaiddelete: no navaid with sourceid " << val.get() << " found" << std::endl;
		return;
	}
	if (ev.size() > 1) {
		std::cerr << "navaiddelete: multiple navaids with sourceid " << val.get() << " found" << std::endl;
		return;
	}
	std::cerr << "Deleting navaid " << ev.front().get_sourceid() << std::endl;
	m_navaidsdb->erase(ev.front());
}

void DbXmlImporter::parse_waypointdelete(const AttributeList & properties)
{
	ValueObject<Glib::ustring> val("");
	bool r = attr<Glib::ustring>(properties, "sourceid", sigc::mem_fun(val, &ValueObject<Glib::ustring>::set));
	if (!r)
		return;
	open_waypoints_db();
	WaypointsDb::elementvector_t ev(m_waypointsdb->find_by_sourceid(val.get(), 0, WaypointsDb::comp_exact_casesensitive, 2, true));
	if (ev.empty()) {
		std::cerr << "waypointdelete: no waypoint with sourceid " << val.get() << " found" << std::endl;
		return;
	}
	if (ev.size() > 1) {
		std::cerr << "waypointdelete: multiple waypoints with sourceid " << val.get() << " found" << std::endl;
		return;
	}
	std::cerr << "Deleting waypoint " << ev.front().get_sourceid() << std::endl;
	m_waypointsdb->erase(ev.front());
}

void DbXmlImporter::parse_airwaydelete(const AttributeList & properties)
{
	ValueObject<Glib::ustring> val("");
	bool r = attr<Glib::ustring>(properties, "sourceid", sigc::mem_fun(val, &ValueObject<Glib::ustring>::set));
	if (!r)
		return;
	open_airways_db();
	AirwaysDb::elementvector_t ev(m_airwaysdb->find_by_sourceid(val.get(), 0, AirwaysDb::comp_exact_casesensitive, 2, true));
	if (ev.empty()) {
		std::cerr << "airwaydelete: no airway with sourceid " << val.get() << " found" << std::endl;
		return;
	}
	if (ev.size() > 1) {
		std::cerr << "airwaydelete: multiple airways with sourceid " << val.get() << " found" << std::endl;
		return;
	}
	std::cerr << "Deleting airway " << ev.front().get_sourceid() << std::endl;
	m_airwaysdb->erase(ev.front());
}

void DbXmlImporter::parse_trackdelete(const AttributeList & properties)
{
	ValueObject<Glib::ustring> val("");
	bool r = attr<Glib::ustring>(properties, "sourceid", sigc::mem_fun(val, &ValueObject<Glib::ustring>::set));
	if (!r)
		return;
	open_tracks_db();
	//TracksDb::elementvector_t ev(m_tracksdb->find_by_sourceid(val.get(), 0, TracksDb::comp_exact_casesensitive, 2, true));
	TracksDb::elementvector_t ev;
	if (ev.empty()) {
		std::cerr << "trackdelete: no track with sourceid " << val.get() << " found" << std::endl;
		return;
	}
	if (ev.size() > 1) {
		std::cerr << "trackdelete: multiple tracks with sourceid " << val.get() << " found" << std::endl;
		return;
	}
	//std::cerr << "Deleting track " << ev.front().get_sourceid() << std::endl;
	m_tracksdb->erase(ev.front());
}

void DbXmlImporter::finalize_airspaces(void)
{
	if (m_asdeps.empty())
		return;
	// number airspaces
	unsigned int nraspc(0);
	typedef std::vector<asdeps_t::const_iterator> aslist_t;
	aslist_t aslist;
	for (asdeps_t::iterator i(m_asdeps.begin()), e(m_asdeps.end()); i != e; ++i) {
		i->second = nraspc++;
		aslist.push_back(i);
	}
	// need to topologically sort the vector first
	typedef boost::adjacency_list<boost::vecS,boost::vecS,boost::directedS,boost::property<boost::vertex_color_t,boost::default_color_type> > Graph;
	typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
	Graph G(nraspc);
	for (asdeps_t::const_iterator ai(m_asdeps.begin()), ae(m_asdeps.end()); ai != ae; ++ai) {
		const AirspaceDeps& adeps(ai->first);
		for (unsigned int i = 0; i < adeps.size(); ++i) {
			const AirspaceDeps::AirspaceDep& adep(adeps[i]);
			asdeps_t::const_iterator ai2(m_asdeps.find(adep));
			if (ai2 == ae)
				continue;
			boost::add_edge(ai->second, ai2->second, G);
		}
	}
	open_airspaces_db();
	typedef std::vector<Vertex> container_t;
	container_t c;
	topological_sort(G, std::back_inserter(c));
	for (container_t::const_iterator ci(c.begin()), ce(c.end()); ci != ce; ++ci) {
		asdeps_t::const_iterator ai(aslist[*ci]);
		const AirspaceDeps& adeps(ai->first);
		AirspacesDb::elementvector_t ev(m_airspacesdb->find_by_icao(adeps.get_icao(), 0, AirspacesDb::comp_exact, 0, AirspacesDb::Airspace::subtables_all));
		for (AirspacesDb::elementvector_t::iterator ei(ev.begin()), ee(ev.end()); ei != ee; ++ei) {
			if (ei->get_bdryclass() != adeps.get_bdryclass() || ei->get_typecode() != adeps.get_typecode() ||
			    ei->get_icao() != adeps.get_icao())
				continue;
			std::cerr << "Composing Airspace " << ei->get_icao() << '/' << ei->get_class_string() << std::endl;
			ei->recompute_lineapprox(*m_airspacesdb.get());
			ei->recompute_bbox();
			std::cerr << "Saving airspace " << ei->get_sourceid() << " address " << ei->get_address() << std::endl;
			m_airspacesdb->save(*ei);
		}
	}
	m_asdeps.clear();
}

int main(int argc, char *argv[])
{
	static struct option long_options[] = {
		{ "dir", required_argument, 0, 'd' },
		{ "pgsql", required_argument, 0, 'p' },
		{ "trace", no_argument, 0, 't' },
		{0, 0, 0, 0}
	};
	Glib::ustring db_dir(".");
	bool pgsql(false), trace(false);
	int c, err(0);

	while ((c = getopt_long(argc, argv, "d:p:t", long_options, 0)) != EOF) {
		switch (c) {
		case 'd':
			if (optarg) {
				db_dir = optarg;
				pgsql = false;
			}
			break;

		case 'p':
			if (optarg) {
				db_dir = optarg;
				pgsql = true;
			}
			break;

		case 't':
			trace = true;
			break;

		default:
			err++;
			break;
		}
	}
	if (err) {
		std::cerr << "usage: vfrnavdb2xml [-d <dir>] [-p <pgsql>]" << std::endl;
		return EX_USAGE;
	}
	try {
		DbXmlImporter parser(db_dir, pgsql, trace);
		parser.set_validate(false); // Do not validate, we do not have a DTD
		parser.set_substitute_entities(true);
		for (; optind < argc; optind++) {
			std::cerr << "Parsing file " << argv[optind] << std::endl;
			parser.parse_file(argv[optind]);
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
