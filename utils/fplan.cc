//
// C++ Implementation: fplan
//
// Description: Flight Plan XML Import/Export Utility (PalmPilot Compatible Format)
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2013, 2014, 2015
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <libxml++/libxml++.h>
#include <unistd.h>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <sqlite3.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdexcept>
#include <stdarg.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

#ifdef HAVE_SYSEXITS_H
#include <sysexits.h>
#else
#define EX_USAGE   64
#define EX_OK      0
#endif

#include "fplan.h"
#include "engine.h"
#include "icaofpl.h"

class DbEngine {
public:
	DbEngine(const std::string& dir_main, Engine::auxdb_mode_t auxdbmode, const std::string& dir_aux);
        const std::string& get_db_maindir(void) const { return m_dbdir_main; }
        const std::string& get_db_auxdir(void) const { return m_dbdir_main; }
	void opendb(void);
	void closedb(void);

        AirportsDb m_airportdb;
        NavaidsDb m_navaiddb;
        WaypointsDb m_waypointdb;

protected:
        Preferences m_prefs;
	std::string m_dbdir_main;
        std::string m_dbdir_aux;
};

DbEngine::DbEngine(const std::string& dir_main, Engine::auxdb_mode_t auxdbmode, const std::string& dir_aux)
	: m_dbdir_main(dir_main)
{
	switch (auxdbmode) {
        case Engine::auxdb_prefs:
                m_dbdir_aux = (Glib::ustring)m_prefs.globaldbdir;
                break;

        case Engine::auxdb_override:
                m_dbdir_aux = dir_aux;
                break;

        default:
                m_dbdir_aux.clear();
                break;
        }
}

void DbEngine::opendb(void)
{
        if (!m_airportdb.is_open()) {
                try {
                        m_airportdb.open(m_dbdir_main);
                        if (!m_dbdir_aux.empty() && m_airportdb.is_dbfile_exists(m_dbdir_aux))
                                m_airportdb.attach(m_dbdir_aux);
                        if (m_airportdb.is_aux_attached())
                                std::cerr << "Auxillary airports database attached" << std::endl;
                } catch (const std::exception& e) {
			std::cerr << "Error opening airports database: " << e.what() << std::endl;
                }
        }
        if (!m_navaiddb.is_open()) {
                try {
                        m_navaiddb.open(m_dbdir_main);
                        if (!m_dbdir_aux.empty() && m_navaiddb.is_dbfile_exists(m_dbdir_aux))
                                m_navaiddb.attach(m_dbdir_aux);
                        if (m_navaiddb.is_aux_attached())
                                std::cerr << "Auxillary navaids database attached" << std::endl;
                } catch (const std::exception& e) {
			std::cerr << "Error opening navaids database: " << e.what() << std::endl;
                }
        }
        if (!m_waypointdb.is_open()) {
                try {
                        m_waypointdb.open(m_dbdir_main);
                        if (!m_dbdir_aux.empty() && m_waypointdb.is_dbfile_exists(m_dbdir_aux))
                                m_waypointdb.attach(m_dbdir_aux);
                        if (m_waypointdb.is_aux_attached())
                                std::cerr << "Auxillary waypoints database attached" << std::endl;
                } catch (const std::exception& e) {
                       	std::cerr << "Error opening waypoints database: " << e.what() << std::endl;
                }
        }
}

void DbEngine::closedb(void)
{
        if (m_airportdb.is_open())
                m_airportdb.close();
        if (m_navaiddb.is_open())
                m_navaiddb.close();
        if (m_waypointdb.is_open())
                m_waypointdb.close();
}

class GarminFplWpt {
public:
	typedef enum {
		type_airport,
		type_vor,
		type_ndb,
		type_intersection,
		type_user
	} type_t;

	GarminFplWpt(const std::string& ident = "", type_t typ = type_user, const Point& coord = Point::invalid, double elev = std::numeric_limits<double>::quiet_NaN(),
		     const std::string& countrycode = "", const std::string& comment = "");
	GarminFplWpt(const AirportsDb::Airport& el);
	GarminFplWpt(const NavaidsDb::Navaid& el);
	GarminFplWpt(const WaypointsDb::Waypoint& el);
	void set_identifier(const std::string& ident);
	void set_countrycode(const std::string& countrycode);
	void set_identifier_raw(const std::string& ident) { m_identifier = ident; }
	void set_countrycode_raw(const std::string& countrycode) { m_countrycode = countrycode; }
	void set_type(type_t t) { m_type = t; }
	void set_type(const std::string& t);
	void set_coord(const Point& pt) { m_coord = pt; }
	void set_elevation(double elev) { m_elevation = elev; }
	void set_comment(const std::string& cmt);
	const std::string& get_identifier(void) const { return m_identifier; }
	const std::string& get_countrycode(void) const { return m_countrycode; }
	const std::string& get_comment(void) const { return m_comment; }
	const Point& get_coord(void) const { return m_coord; }
	double get_elevation(void) const { return m_elevation; }
	type_t get_type(void) const { return m_type; }
	bool operator<(const GarminFplWpt& w) const;
	static const std::string& get_type_string(type_t t);
	const std::string& get_type_string(void) const { return get_type_string(get_type()); }
	void write_xml(xmlTextWriterPtr writer) const;
	void lookup(DbEngine& db);
	void set(FPlanWaypoint& wpt) const;

protected:
	static constexpr double tolerance = 0.1;

	Engine::DbObject::ptr_t m_obj;
	std::string m_identifier;
	std::string m_countrycode;
	std::string m_comment;
	Point m_coord;
	double m_elevation;
	type_t m_type;

	std::string shortestauth(const DbBaseElements::DbElementLabelSourceCoordIcaoNameAuthorityBase::authorityset_t& authset);
};

constexpr double GarminFplWpt::tolerance;

GarminFplWpt::GarminFplWpt(const std::string& ident, type_t typ, const Point& coord, double elev, const std::string& countrycode, const std::string& comment)
	: m_identifier(ident), m_countrycode(countrycode), m_coord(coord), m_elevation(elev), m_type(typ)
{
	set_comment(comment);
}

GarminFplWpt::GarminFplWpt(const AirportsDb::Airport& el)
	: m_identifier(el.get_icao()), m_countrycode(shortestauth(el.get_authorityset())), m_coord(el.get_coord()),
	  m_elevation(el.get_elevation()), m_type(GarminFplWpt::type_airport)
{
	set_comment(el.get_icao_name());
	m_obj = Engine::DbObject::create(el);
}

GarminFplWpt::GarminFplWpt(const NavaidsDb::Navaid& el)
	: m_identifier(el.get_icao()), m_countrycode(shortestauth(el.get_authorityset())), m_coord(el.get_coord()),
	  m_elevation(el.get_elev()), m_type(el.has_vor() ? GarminFplWpt::type_vor : el.has_ndb() ? GarminFplWpt::type_ndb : GarminFplWpt::type_user)
{
	set_comment(el.get_icao_name());
	m_obj = Engine::DbObject::create(el);
}

GarminFplWpt::GarminFplWpt(const WaypointsDb::Waypoint& el)
	: m_identifier(el.get_name()), m_countrycode(shortestauth(el.get_authorityset())), m_coord(el.get_coord()),
	  m_elevation(std::numeric_limits<double>::quiet_NaN()), m_type(type_intersection)
{
	set_comment(el.get_icao_name());
	m_obj = Engine::DbObject::create(el);
}

void GarminFplWpt::set_identifier(const std::string& ident)
{
	std::string x(ident);
	for (std::string::size_type i(0); i < x.size(); ) {
		if (std::islower(x[i])) {
			x[i] += ('A' - 'a');
			++i;
			continue;
		}
		if (std::isalnum(x[i]) || x[i] == '_') {
			++i;
			continue;
		}
		x.erase(i, 1);
	}
	if (x.size() > 5)
		x.erase(5);
	m_identifier = x;
}

void GarminFplWpt::set_countrycode(const std::string& countrycode)
{
	std::string x(countrycode);
	for (std::string::size_type i(0); i < x.size(); ) {
		if (std::islower(x[i])) {
			x[i] += ('A' - 'a');
			++i;
			continue;
		}
		if (std::isalnum(x[i]) || x[i] == '_') {
			++i;
			continue;
		}
		x.erase(i, 1);
	}
	m_countrycode = x;
}

void GarminFplWpt::set_type(const std::string& t)
{
	for (type_t typ(type_airport); typ <= type_user; typ = (type_t)(typ + 1)) {
		if (get_type_string(typ) == t) {
			m_type = typ;
			return;
		}
	}
}

void GarminFplWpt::set_comment(const std::string& cmt)
{
	m_comment = cmt;
	bool spc(false);
	for (std::string::iterator i(m_comment.begin()), e(m_comment.end()); i != e;) {
		if (std::isalnum(*i) || *i == '/') {
			if (std::islower(*i))
				*i += ('A' - 'a');
			spc = false;
			++i;
			continue;
		}
		*i = ' ';
		spc = !spc;
		if (spc) {
			++i;
			continue;
		}
		i = m_comment.erase(i);
		e = m_comment.end();
	}
}

bool GarminFplWpt::operator<(const GarminFplWpt& w) const
{
	int c(get_identifier().compare(w.get_identifier()));
	if (c < 0)
		return true;
	if (c > 0)
		return false;
	if (get_type() < w.get_type())
		return true;
	if (get_type() > w.get_type())
		return false;
	c = get_countrycode().compare(w.get_countrycode());
	return c < 0;
}

const std::string& GarminFplWpt::get_type_string(type_t t)
{
	switch (t) {
	case type_airport:
	{
		static const std::string r("AIRPORT");
		return r;
	}

	case type_vor:
	{
		static const std::string r("VOR");
		return r;
	}

	case type_ndb:
	{
		static const std::string r("NDB");
		return r;
	}

	case type_intersection:
	{
		static const std::string r("INT");
		return r;
	}

	default:
	case type_user:
	{
		static const std::string r("USER WAYPOINT");
		return r;
	}
	}
}

void GarminFplWpt::write_xml(xmlTextWriterPtr writer) const
{
	if (!writer)
		return;
	int rc = xmlTextWriterStartElement(writer, (const xmlChar *)"waypoint");
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterStartElement");
	rc = xmlTextWriterWriteElement(writer, (const xmlChar *)"identifier", (const xmlChar *)get_identifier().c_str());
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
	rc = xmlTextWriterWriteElement(writer, (const xmlChar *)"type", (const xmlChar *)get_type_string().c_str());
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
	rc = xmlTextWriterWriteElement(writer, (const xmlChar *)"country-code", (const xmlChar *)get_countrycode().c_str());
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
	rc = xmlTextWriterWriteFormatElement(writer, (const xmlChar *)"lat", "%.9g", get_coord().get_lat_deg_dbl());
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
	rc = xmlTextWriterWriteFormatElement(writer, (const xmlChar *)"lon", "%.9g", get_coord().get_lon_deg_dbl());
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
	rc = xmlTextWriterWriteElement(writer, (const xmlChar *)"comment", (const xmlChar *)get_comment().c_str());
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
	if (!std::isnan(get_elevation())) {
		rc = xmlTextWriterWriteFormatElement(writer, (const xmlChar *)"elevation", "%.3g", get_elevation());
		if (rc < 0)
			throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
	}
	rc = xmlTextWriterEndElement(writer);
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterEndElement");
}

std::string GarminFplWpt::shortestauth(const DbBaseElements::DbElementLabelSourceCoordIcaoNameAuthorityBase::authorityset_t& authset)
{
	DbBaseElements::DbElementLabelSourceCoordIcaoNameAuthorityBase::authorityset_t::const_iterator ai(authset.begin()), ae(authset.end());
	if (ai == ae)
		return "";
	Glib::ustring r(*ai);
	++ai;
	for (; ai != ae; ++ai) {
		if (ai->size() < r.size())
			r = *ai;
	}
	return r;
}

void GarminFplWpt::lookup(DbEngine& db)
{
	if (m_obj || m_identifier.empty())
		return;
	db.opendb();
	switch (m_type) {
	case type_airport:
	{
		AirportsDb::elementvector_t ev(db.m_airportdb.find_by_icao(m_identifier, 0, AirportsDb::comp_exact, 0, AirportsDb::Airport::subtables_none));
		AirportsDb::element_t el;
		uint64_t bestd(std::numeric_limits<uint64_t>::max());
		for (AirportsDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
			uint64_t d(evi->get_coord().simple_distance_rel(get_coord()));
			if (d >= bestd)
				continue;
			bestd = d;
			el = *evi;
		}
		if (el.is_valid() && el.get_coord().spheric_distance_nmi_dbl(get_coord()) <= tolerance)
			m_obj = Engine::DbObject::create(el);
		break;
	}

	case type_vor:
	case type_ndb:
	{
		NavaidsDb::elementvector_t ev(db.m_navaiddb.find_by_icao(m_identifier, 0, NavaidsDb::comp_exact, 0, NavaidsDb::Navaid::subtables_none));
		NavaidsDb::element_t el;
		uint64_t bestd(std::numeric_limits<uint64_t>::max());
		for (NavaidsDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
			if ((m_type == type_vor && !evi->has_vor()) ||
			    (m_type == type_ndb && !evi->has_ndb()))
				continue;
			uint64_t d(evi->get_coord().simple_distance_rel(get_coord()));
			if (d >= bestd)
				continue;
			bestd = d;
			el = *evi;
		}
		if (el.is_valid() && el.get_coord().spheric_distance_nmi_dbl(get_coord()) <= tolerance)
			m_obj = Engine::DbObject::create(el);
		break;
	}

	case type_intersection:
	{
		WaypointsDb::elementvector_t ev(db.m_waypointdb.find_by_name(m_identifier, 0, WaypointsDb::comp_exact, 0, WaypointsDb::Waypoint::subtables_none));
		WaypointsDb::element_t el;
		uint64_t bestd(std::numeric_limits<uint64_t>::max());
		for (WaypointsDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
			uint64_t d(evi->get_coord().simple_distance_rel(get_coord()));
			if (d >= bestd)
				continue;
			bestd = d;
			el = *evi;
		}
		if (el.is_valid() && el.get_coord().spheric_distance_nmi_dbl(get_coord()) <= tolerance)
			m_obj = Engine::DbObject::create(el);
		break;
	}

	default:
		break;
	}
}

void GarminFplWpt::set(FPlanWaypoint& wpt) const
{
	wpt.set_altitude(10000);
	if (m_obj && m_obj->set(wpt))
		return;
	wpt.set_name(get_identifier());
	wpt.set_coord(get_coord());
	switch (get_type()) {
	case type_airport:
		wpt.set_type(FPlanWaypoint::type_airport);
		break;

	case type_vor:
		wpt.set_type(FPlanWaypoint::type_navaid, FPlanWaypoint::navaid_vor);
		break;

	case type_ndb:
		wpt.set_type(FPlanWaypoint::type_navaid, FPlanWaypoint::navaid_ndb);
		break;

	case type_intersection:
		wpt.set_type(FPlanWaypoint::type_intersection);
		break;

	case type_user:
		wpt.set_type(FPlanWaypoint::type_user);
		break;

	default:
		wpt.set_type(FPlanWaypoint::type_undefined);
		break;
	}
}

class GarminFplRtept {
public:
	GarminFplRtept(const std::string& ident, GarminFplWpt::type_t typ, const std::string& countrycode = "")
		: m_identifier(ident), m_countrycode(countrycode), m_type(typ) {}
	const std::string& get_identifier(void) const { return m_identifier; }
	const std::string& get_countrycode(void) const { return m_countrycode; }
	GarminFplWpt::type_t get_type(void) const { return m_type; }
	const std::string& get_type_string(void) const { return GarminFplWpt::get_type_string(get_type()); }
	void write_xml(xmlTextWriterPtr writer) const;

protected:
	std::string m_identifier;
	std::string m_countrycode;
	std::string m_comment;
	Point m_coord;
	double m_elevation;
	GarminFplWpt::type_t m_type;
};

void GarminFplRtept::write_xml(xmlTextWriterPtr writer) const
{
	if (!writer)
		return;
	int rc = xmlTextWriterStartElement(writer, (const xmlChar *)"route-point");
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterStartElement");
	rc = xmlTextWriterWriteElement(writer, (const xmlChar *)"waypoint-identifier", (const xmlChar *)get_identifier().c_str());
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
	rc = xmlTextWriterWriteElement(writer, (const xmlChar *)"waypoint-type", (const xmlChar *)get_type_string().c_str());
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
	rc = xmlTextWriterWriteElement(writer, (const xmlChar *)"waypoint-country-code", (const xmlChar *)get_countrycode().c_str());
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
	rc = xmlTextWriterEndElement(writer);
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterEndElement");
}

class GarminFplRte : public std::vector<GarminFplRtept> {
public:
	GarminFplRte(const std::string& name = "", const std::string& desc = "")
		: m_name(name), m_desc(desc) {}
	const std::string& get_name(void) const { return m_name; }
	const std::string& get_description(void) const { return m_desc; }
	void write_xml(xmlTextWriterPtr writer, unsigned int index = 0) const;

protected:
	std::string m_name;
	std::string m_desc;
};

void GarminFplRte::write_xml(xmlTextWriterPtr writer, unsigned int index) const
{
	if (!writer)
		return;
	if (empty())
		return;
	int rc = xmlTextWriterStartElement(writer, (const xmlChar *)"route");
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterStartElement");
	{
		std::string name(get_name());
		if (name.empty() && !empty())
			name = (*this)[0].get_identifier() + "/" + (*this)[size()-1].get_identifier();
		rc = xmlTextWriterWriteElement(writer, (const xmlChar *)"route-name", (const xmlChar *)name.c_str());
		if (rc < 0)
			throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
	}
	if (!get_description().empty()) {
		rc = xmlTextWriterWriteElement(writer, (const xmlChar *)"route-description", (const xmlChar *)get_description().c_str());
		if (rc < 0)
			throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
	}
	if (index) {
		rc = xmlTextWriterWriteFormatElement(writer, (const xmlChar *)"flight-plan-index", "%u", index);
		if (rc < 0)
			throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
	}
	for (const_iterator i(this->begin()), e(this->end()); i != e; ++i)
		i->write_xml(writer);
	rc = xmlTextWriterEndElement(writer);
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterEndElement");
}

class GarminFplRtes : public std::vector<GarminFplRte> {
public:
	void write_xml(xmlTextWriterPtr writer) const;
};

void GarminFplRtes::write_xml(xmlTextWriterPtr writer) const
{
	unsigned int idx(1);
	for (const_iterator i(this->begin()), e(this->end()); i != e; ++i, ++idx)
		i->write_xml(writer, idx);
}

class GarminFplWpts : public std::set<GarminFplWpt> {
public:
	GarminFplRtept add(DbEngine& db, const FPlanWaypoint& wpt);
	void write_xml(xmlTextWriterPtr writer) const;

protected:
	static constexpr double tolerance = 0.1;

	GarminFplRtept add(const GarminFplWpt& wpt);
};

constexpr double GarminFplWpts::tolerance;

GarminFplRtept GarminFplWpts::add(const GarminFplWpt& wpt)
{
 	GarminFplWpt wptu(wpt);
	// normalize identifier and country code
	wptu.set_identifier(wptu.get_identifier());
	wptu.set_countrycode(wptu.get_countrycode());
     	std::pair<iterator,bool> ins(insert(wptu));
	if (ins.second || ins.first->get_coord().spheric_distance_nmi_dbl(wptu.get_coord()) <= tolerance)
		return GarminFplRtept(ins.first->get_identifier(), ins.first->get_type(), ins.first->get_countrycode());
	wptu.set_type(GarminFplWpt::type_user);
	ins = insert(wptu);
	if (ins.second || ins.first->get_coord().spheric_distance_nmi_dbl(wptu.get_coord()) <= tolerance)
		return GarminFplRtept(ins.first->get_identifier(), ins.first->get_type(), ins.first->get_countrycode());
	{
		std::string id(wptu.get_identifier());
		if (id.size() > 3)
			id.erase(3);
		id += "00";
		wptu.set_identifier(id);
	}
	for (unsigned int nr = 0; nr < 100; ++nr) {
		{
			std::string id(wptu.get_identifier());
			id.erase(id.size() - 2);
			id += ('0' + nr / 10);
			id += ('0' + nr % 10);
			wptu.set_identifier(id);
		}
		ins = insert(wptu);
		if (ins.second || ins.first->get_coord().spheric_distance_nmi_dbl(wptu.get_coord()) <= tolerance)
			return GarminFplRtept(ins.first->get_identifier(), ins.first->get_type(), ins.first->get_countrycode());
	}
	throw std::runtime_error("Cannot create user waypoint");
}

GarminFplRtept GarminFplWpts::add(DbEngine& db, const FPlanWaypoint& wpt)
{
	if (!wpt.get_icao().empty()) {
		{
			AirportsDb::elementvector_t ev(db.m_airportdb.find_by_icao(wpt.get_icao(), 0, AirportsDb::comp_exact, 0, AirportsDb::Airport::subtables_none));
			AirportsDb::element_t el;
			uint64_t bestd(std::numeric_limits<uint64_t>::max());
			for (AirportsDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				uint64_t d(evi->get_coord().simple_distance_rel(wpt.get_coord()));
				if (d >= bestd)
					continue;
				bestd = d;
				el = *evi;
			}
			if (el.is_valid() && el.get_coord().spheric_distance_nmi_dbl(wpt.get_coord()) <= tolerance)
				return add(GarminFplWpt(el));
		}
		{
			NavaidsDb::elementvector_t ev(db.m_navaiddb.find_by_icao(wpt.get_icao(), 0, NavaidsDb::comp_exact, 0, NavaidsDb::Navaid::subtables_none));
			NavaidsDb::element_t el;
			uint64_t bestd(std::numeric_limits<uint64_t>::max());
			for (NavaidsDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				uint64_t d(evi->get_coord().simple_distance_rel(wpt.get_coord()));
				if (d >= bestd)
					continue;
				bestd = d;
				el = *evi;
			}
			if (el.is_valid() && el.get_coord().spheric_distance_nmi_dbl(wpt.get_coord()) <= tolerance)
				return add(GarminFplWpt(el));
		}
	}
	if (!wpt.get_name().empty()) {
		{
			WaypointsDb::elementvector_t ev(db.m_waypointdb.find_by_name(wpt.get_name(), 0, WaypointsDb::comp_exact, 0, WaypointsDb::Waypoint::subtables_none));
			WaypointsDb::element_t el;
			uint64_t bestd(std::numeric_limits<uint64_t>::max());
			for (WaypointsDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				uint64_t d(evi->get_coord().simple_distance_rel(wpt.get_coord()));
				if (d >= bestd)
					continue;
				bestd = d;
				el = *evi;
			}
			if (el.is_valid() && el.get_coord().spheric_distance_nmi_dbl(wpt.get_coord()) <= tolerance)
				return add(GarminFplWpt(el));
		}
	}
	return add(GarminFplWpt(wpt.get_icao().empty() ? (wpt.get_name().empty() ? "USR" : wpt.get_name()) : wpt.get_icao(), GarminFplWpt::type_user, wpt.get_coord(),
				std::numeric_limits<double>::quiet_NaN(), "", wpt.get_note()));
}

void GarminFplWpts::write_xml(xmlTextWriterPtr writer) const
{
	if (!writer)
		return;
	int rc = xmlTextWriterStartElement(writer, (const xmlChar *)"waypoint-table");
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterStartElement");
	for (const_iterator i(this->begin()), e(this->end()); i != e; ++i)
		i->write_xml(writer);
	rc = xmlTextWriterEndElement(writer);
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterEndElement");
}

class FPlanXmlImporter : public xmlpp::SaxParser {
public:
	FPlanXmlImporter(FPlan& db, DbEngine& dbeng);
	virtual ~FPlanXmlImporter();

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

private:
	typedef enum {
		state_document_c,
		state_fplans_c,
		state_fplan_c,
		state_fplanwpt_c,
		state_gpx_c,
		state_gpx_unknown1_c,
		state_gpx_unknown2_c,
		state_gpx_unknown3_c,
		state_gpx_rte_c,
		state_gpx_rte_desc_c,
		state_gpx_rte_unknown1_c,
		state_gpx_rte_rtept_c,
		state_gpx_rte_rtept_ele_c,
		state_gpx_rte_rtept_time_c,
		state_gpx_rte_rtept_name_c,
		state_gpx_rte_rtept_desc_c,
		state_gpx_rte_rtept_cmt_c,
		state_gpx_rte_rtept_unknown1_c,
		state_gpx_rte_rtept_unknown2_c,
		state_garminfpl_c,
		state_garminfpl_created_c,
		state_garminfpl_wpttbl_c,
		state_garminfpl_wpttbl_wpt_c,
		state_garminfpl_wpttbl_wpt_ident_c,
		state_garminfpl_wpttbl_wpt_type_c,
		state_garminfpl_wpttbl_wpt_ctry_c,
		state_garminfpl_wpttbl_wpt_lat_c,
		state_garminfpl_wpttbl_wpt_lon_c,
		state_garminfpl_wpttbl_wpt_cmt_c,
		state_garminfpl_wpttbl_wpt_elevation_c,
		state_garminfpl_rte_c,
		state_garminfpl_rte_rtename_c,
		state_garminfpl_rte_rtedesc_c,
		state_garminfpl_rte_fplidx_c,
		state_garminfpl_rte_rtept_c,
		state_garminfpl_rte_rtept_ident_c,
		state_garminfpl_rte_rtept_type_c,
		state_garminfpl_rte_rtept_ctry_c
	} state_t;
	state_t m_state;
	DbEngine& m_dbeng;
	GarminFplWpts m_garminwpts;
	GarminFplWpt m_garminwpt;
	FPlan& m_db;
	FPlanRoute m_route;
	Glib::ustring m_text;
	uint32_t m_plannr, m_wptnr;
};

FPlanXmlImporter::FPlanXmlImporter(FPlan& db, DbEngine& dbeng)
        : m_state(state_document_c), m_dbeng(dbeng), m_db(db), m_route(db), m_plannr(0), m_wptnr(0)
{
}

FPlanXmlImporter::~FPlanXmlImporter()
{
}

void FPlanXmlImporter::on_start_document()
{
        m_state = state_document_c;
	m_garminwpts.clear();
	m_garminwpt = GarminFplWpt();
}

void FPlanXmlImporter::on_end_document()
{
}

void FPlanXmlImporter::on_start_element( const Glib::ustring & name, const AttributeList & properties )
{
       switch (m_state) {
       case state_document_c:
	       if (name == "PilotFlightPlans") {
		       m_state = state_fplans_c;
		       for (AttributeList::const_iterator ai(properties.begin()), ae(properties.end()); ai != ae; ai++) {
			       Glib::ustring pname((*ai).name);
			       Glib::ustring pvalue((*ai).value);
			       if (pname == "CurFP") {
				       std::istringstream is(pvalue);
				       uint32_t t;
				       is >> t;
				       // if (is.good()) engine.SetCurPlan(t);
				       continue;
			       }
		       }
		       return;
	       }
	       if (name == "gpx") {
		       m_state = state_gpx_c;
		       return;
	       }
	       if (name == "flight-plan") {
		       m_state = state_garminfpl_c;
		       m_garminwpts.clear();
		       m_garminwpt = GarminFplWpt();
		       return;
	       }
	       break;

       case state_fplans_c:
	       m_text.clear();
	       if (name == "NavFlightPlan") {
		       m_state = state_fplan_c;
		       m_route.new_fp();
		       for (AttributeList::const_iterator ai(properties.begin()), ae(properties.end()); ai != ae; ai++) {
			       Glib::ustring pname((*ai).name);
			       Glib::ustring pvalue((*ai).value);
			       if (pname == "Note") {
				       m_route.set_note(pvalue);
				       continue;
			       }
			       if (pname == "TimeOffBlock") {
				       std::istringstream is(pvalue);
				       uint32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       m_route.set_time_offblock(t);
				       continue;
			       }
			       if (pname == "SavedTimeOffBlock") {
				       std::istringstream is(pvalue);
				       uint32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       m_route.set_save_time_offblock(t);
				       continue;
			       }
			       if (pname == "TimeOnBlock") {
				       std::istringstream is(pvalue);
				       uint32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       m_route.set_time_onblock(t);
				       continue;
			       }
			       if (pname == "SavedTimeOnBlock") {
				       std::istringstream is(pvalue);
				       uint32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       m_route.set_save_time_onblock(t);
				       continue;
			       }
			       if (pname == "CurrentWaypoint") {
				       std::istringstream is(pvalue);
				       uint32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       m_route.set_curwpt(t);
				       continue;
			       }
		       }
		       std::cerr << "new flight plan" << std::endl;
		       return;
	       }
	       break;

       case state_fplan_c:
	       m_text.clear();
	       if (name == "WayPoint") {
		       m_state = state_fplanwpt_c;
		       FPlanWaypoint wpt;
		       for (AttributeList::const_iterator ai(properties.begin()), ae(properties.end()); ai != ae; ai++) {
			       Glib::ustring pname((*ai).name);
			       Glib::ustring pvalue((*ai).value);
			       if (pname == "Note") {
				       wpt.set_note(pvalue);
				       continue;
			       }
			       if (pname == "ICAO") {
				       wpt.set_icao(pvalue);
				       continue;
			       }
			       if (pname == "name") {
				       wpt.set_name(pvalue);
				       continue;
			       }
			       if (pname == "Time") {
				       std::istringstream is(pvalue);
				       uint32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       wpt.set_time(t);
				       continue;
			       }
			       if (pname == "SavedTime") {
				       std::istringstream is(pvalue);
				       uint32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       wpt.set_save_time(t);
				       continue;
			       }
			       if (pname == "FlightTime") {
				       std::istringstream is(pvalue);
				       uint32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       wpt.set_flighttime(t);
				       continue;
			       }
			       if (pname == "frequency") {
				       std::istringstream is(pvalue);
				       uint32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       wpt.set_frequency(t);
				       continue;
			       }
			       if (pname == "lon") {
				       std::istringstream is(pvalue);
				       int32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       wpt.set_lon(t);
				       continue;
			       }
			       if (pname == "lat") {
				       std::istringstream is(pvalue);
				       int32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       wpt.set_lat(t);
				       continue;
			       }
			       if (pname == "alt") {
				       std::istringstream is(pvalue);
				       int32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       wpt.set_altitude(t);
				       continue;
			       }
			       if (pname == "flags") {
				       std::istringstream is(pvalue);
				       uint32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       wpt.set_flags(t);
				       continue;
			       }
			       if (pname == "winddir") {
				       std::istringstream is(pvalue);
				       uint32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       wpt.set_winddir(t);
				       continue;
			       }
			       if (pname == "windspeed") {
				       std::istringstream is(pvalue);
				       uint32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       wpt.set_windspeed(t);
				       continue;
			       }
			       if (pname == "qff") {
				       std::istringstream is(pvalue);
				       uint32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       wpt.set_qff(t);
				       continue;
			       }
			       if (pname == "isaoffset") {
				       std::istringstream is(pvalue);
				       int32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       wpt.set_isaoffset(t);
				       continue;
			       }
			       if (pname == "pathcode") {
				       wpt.set_pathcode_name(pvalue);
				       continue;
			       }
			       if (pname == "pathname") {
				       wpt.set_pathname(pvalue);
				       continue;
			       }
			       if (pname == "truealt") {
				       std::istringstream is(pvalue);
				       int32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       wpt.set_truealt(t);
				       continue;
			       }
			       if (pname == "dist") {
				       std::istringstream is(pvalue);
				       uint32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       wpt.set_dist(t);
				       continue;
			       }
			       if (pname == "mass") {
				       std::istringstream is(pvalue);
				       uint32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       wpt.set_mass(t);
				       continue;
			       }
			       if (pname == "fuel") {
				       std::istringstream is(pvalue);
				       uint32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       wpt.set_fuel(t);
				       continue;
			       }
			       if (pname == "truetrack") {
				       std::istringstream is(pvalue);
				       uint16_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       wpt.set_truetrack(t);
				       continue;
			       }
			       if (pname == "trueheading") {
				       std::istringstream is(pvalue);
				       uint32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       wpt.set_trueheading(t);
				       continue;
			       }
			       if (pname == "declination") {
				       std::istringstream is(pvalue);
				       uint32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       wpt.set_declination(t);
				       continue;
			       }
			       if (pname == "tas") {
				       std::istringstream is(pvalue);
				       uint32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       wpt.set_tas(t);
				       continue;
			       }
			       if (pname == "rpm") {
				       std::istringstream is(pvalue);
				       uint32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       wpt.set_rpm(t);
				       continue;
			       }
			       if (pname == "mp") {
				       std::istringstream is(pvalue);
				       uint32_t t;
				       is >> t;
				       if (is.fail())
					       continue;
				       wpt.set_mp(t);
				       continue;
			       }
			       if (pname == "type") {
				       wpt.set_type_string(pvalue);
				       continue;
			       }
        	       }
		       m_route.insert_wpt(~0, wpt);
		       std::cerr << "new waypoint: " << (m_route.get_nrwpt()-1) << ": " << wpt.get_icao().raw() << ' ' << wpt.get_name().raw() << std::endl;
		       return;
	       }
	       break;

       case state_gpx_c:
	       if (name == "rte") {
		       m_state = state_gpx_rte_c;
		       m_route.new_fp();
		       return;
	       }
	       m_state = state_gpx_unknown1_c;
	       return;

       case state_gpx_unknown1_c:
	       m_state = state_gpx_unknown2_c;
	       return;

       case state_gpx_unknown2_c:
	       m_state = state_gpx_unknown3_c;
	       return;

       case state_gpx_unknown3_c:
	       break;

       case state_gpx_rte_c:
	       m_text.clear();
	       if (name == "desc") {
		       m_state = state_gpx_rte_desc_c;
		       return;
	       }
	       if (name == "rtept") {
		       m_state = state_gpx_rte_rtept_c;
		       FPlanWaypoint wpt;
		       wpt.set_altitude(5000);
		       for (AttributeList::const_iterator ai(properties.begin()), ae(properties.end()); ai != ae; ai++) {
			       Glib::ustring pname((*ai).name);
			       Glib::ustring pvalue((*ai).value);
			       if (pname == "lon") {
				       std::istringstream is(pvalue);
				       double t;
				       is >> t;
				       if (is.fail())
					       continue;
				       wpt.set_lon_deg(t);
				       continue;
			       }
			       if (pname == "lat") {
				       std::istringstream is(pvalue);
				       double t;
				       is >> t;
				       if (is.fail())
					       continue;
				       wpt.set_lat_deg(t);
				       continue;
			       }
		       }
		       m_route.insert_wpt(~0, wpt);
		       return;
	       }
	       m_state = state_gpx_rte_unknown1_c;
	       return;

       case state_gpx_rte_desc_c:
	       break;

       case state_gpx_rte_unknown1_c:
	       break;

       case state_gpx_rte_rtept_c:
	       m_text.clear();
	       if (name == "ele") {
		       m_state = state_gpx_rte_rtept_ele_c;
		       return;
	       }
	       if (name == "time") {
		       m_state = state_gpx_rte_rtept_time_c;
		       return;
	       }
	       if (name == "name") {
		       m_state = state_gpx_rte_rtept_name_c;
		       return;
	       }
	       if (name == "desc") {
		       m_state = state_gpx_rte_rtept_desc_c;
		       return;
	       }
	       if (name == "cmt") {
		       m_state = state_gpx_rte_rtept_cmt_c;
		       return;
	       }
	       m_state = state_gpx_rte_rtept_unknown1_c;
	       return;

       case state_gpx_rte_rtept_ele_c:
	       break;

       case state_gpx_rte_rtept_time_c:
	       break;

       case state_gpx_rte_rtept_name_c:
	       break;

       case state_gpx_rte_rtept_desc_c:
	       break;

       case state_gpx_rte_rtept_cmt_c:
	       break;

       case state_gpx_rte_rtept_unknown1_c:
	       m_state = state_gpx_rte_rtept_unknown2_c;
	       return;

       case state_gpx_rte_rtept_unknown2_c:
	       break;

       case state_garminfpl_c:
	       m_text.clear();
	       if (name == "created") {
		       m_state = state_garminfpl_created_c;
		       return;
	       }
	       if (name == "waypoint-table") {
		       m_state = state_garminfpl_wpttbl_c;
		       return;
	       }
	       if (name == "route") {
		       m_state = state_garminfpl_rte_c;
		       m_route.new_fp();
		       return;
	       }
	       break;

       case state_garminfpl_created_c:
	       break;

       case state_garminfpl_wpttbl_c:
	       if (name == "waypoint") {
		       m_state = state_garminfpl_wpttbl_wpt_c;
		       m_garminwpt = GarminFplWpt();
		       return;
	       }
	       break;

       case state_garminfpl_wpttbl_wpt_c:
	       m_text.clear();
	       if (name == "identifier") {
		       m_state = state_garminfpl_wpttbl_wpt_ident_c;
		       return;
	       }
	       if (name == "type") {
		       m_state = state_garminfpl_wpttbl_wpt_type_c;
		       return;
	       }
	       if (name == "country-code") {
		       m_state = state_garminfpl_wpttbl_wpt_ctry_c;
		       return;
	       }
	       if (name == "lat") {
		       m_state = state_garminfpl_wpttbl_wpt_lat_c;
		       return;
	       }
	       if (name == "lon") {
		       m_state = state_garminfpl_wpttbl_wpt_lon_c;
		       return;
	       }
	       if (name == "comment") {
		       m_state = state_garminfpl_wpttbl_wpt_cmt_c;
		       return;
	       }
	       if (name == "elevation") {
		       m_state = state_garminfpl_wpttbl_wpt_elevation_c;
		       return;
	       }
	       break;

       case state_garminfpl_wpttbl_wpt_ident_c:
	       break;

       case state_garminfpl_wpttbl_wpt_type_c:
	       break;

       case state_garminfpl_wpttbl_wpt_ctry_c:
	       break;

       case state_garminfpl_wpttbl_wpt_lat_c:
	       break;

       case state_garminfpl_wpttbl_wpt_lon_c:
	       break;

       case state_garminfpl_wpttbl_wpt_cmt_c:
	       break;

       case state_garminfpl_wpttbl_wpt_elevation_c:
	       break;

       case state_garminfpl_rte_c:
	       m_text.clear();
	       if (name == "route-name") {
		       m_state = state_garminfpl_rte_rtename_c;
		       return;
	       }
	       if (name == "route-description") {
		       m_state = state_garminfpl_rte_rtedesc_c;
		       return;
	       }
	       if (name == "flight-plan-index") {
		       m_state = state_garminfpl_rte_fplidx_c;
		       return;
	       }
	       if (name == "route-point") {
		       m_state = state_garminfpl_rte_rtept_c;
		       m_garminwpt = GarminFplWpt();
		       return;
	       }
	       break;

       case state_garminfpl_rte_rtename_c:
	       break;

       case state_garminfpl_rte_rtedesc_c:
	       break;

       case state_garminfpl_rte_fplidx_c:
	       break;

       case state_garminfpl_rte_rtept_c:
	       m_text.clear();
	       if (name == "waypoint-identifier") {
		       m_state = state_garminfpl_rte_rtept_ident_c;
		       return;
	       }
	       if (name == "waypoint-type") {
		       m_state = state_garminfpl_rte_rtept_type_c;
		       return;
	       }
	       if (name == "waypoint-country-code") {
		       m_state = state_garminfpl_rte_rtept_ctry_c;
		       return;
	       }
	       break;

       case state_garminfpl_rte_rtept_ident_c:
	       break;

       case state_garminfpl_rte_rtept_type_c:
	       break;

       case state_garminfpl_rte_rtept_ctry_c:
	       break;

       default:
	       break;
       }
       std::ostringstream oss;
       oss << "XML Parser: Invalid element " << name << " in state " << (int)m_state;
       throw std::runtime_error(oss.str());
}

void FPlanXmlImporter::on_end_element(const Glib::ustring & name)
{
	switch (m_state) {
	case state_fplans_c:
		m_state = state_document_c;
		return;

	case state_fplan_c:
		m_state = state_fplans_c;
                m_route.save_fp();
                m_route.new_fp();
		return;

	case state_fplanwpt_c:
		m_state = state_fplan_c;
		return;

	case state_gpx_c:
		m_state = state_document_c;
		return;

	case state_gpx_unknown1_c:
		m_state = state_gpx_c;
		return;

	case state_gpx_unknown2_c:
		m_state = state_gpx_unknown1_c;
		return;

	case state_gpx_unknown3_c:
		m_state = state_gpx_unknown2_c;
		return;

	case state_gpx_rte_c:
		m_state = state_gpx_c;
                m_route.save_fp();
                m_route.new_fp();
		return;

	case state_gpx_rte_desc_c:
		m_route.set_note(m_text);
		m_state = state_gpx_rte_c;
		return;

	case state_gpx_rte_unknown1_c:
		m_state = state_gpx_rte_c;
		return;

	case state_gpx_rte_rtept_c:
		m_state = state_gpx_rte_c;
		return;

	case state_gpx_rte_rtept_ele_c:
		if (m_route.get_nrwpt() > 0) {
			FPlanWaypoint& wpt(m_route[m_route.get_nrwpt()-1]);
			std::istringstream is(m_text);
			double t;
			is >> t;
			if (!is.fail())
				wpt.set_altitude(t * Point::m_to_ft_dbl);
		}
		m_state = state_gpx_rte_rtept_c;
		return;

	case state_gpx_rte_rtept_time_c:
		if (m_route.get_nrwpt() > 0) {
			FPlanWaypoint& wpt(m_route[m_route.get_nrwpt()-1]);
			Glib::TimeVal tv;
			if (tv.assign_from_iso8601(m_text)) {
				wpt.set_time_unix(tv.tv_sec);
			} else {
				std::cerr << "Cannot parse ISO8601 time: \"" << m_text << "\"" << std::endl;
			}
		}
		m_state = state_gpx_rte_rtept_c;
		return;

	case state_gpx_rte_rtept_name_c:
		if (m_route.get_nrwpt() > 0) {
			FPlanWaypoint& wpt(m_route[m_route.get_nrwpt()-1]);
			wpt.set_name(m_text);
		}
		m_state = state_gpx_rte_rtept_c;
		return;

	case state_gpx_rte_rtept_desc_c:
		if (m_route.get_nrwpt() > 0) {
			FPlanWaypoint& wpt(m_route[m_route.get_nrwpt()-1]);
			wpt.set_icao(m_text);
		}
		m_state = state_gpx_rte_rtept_c;
		return;

	case state_gpx_rte_rtept_cmt_c:
		if (m_route.get_nrwpt() > 0) {
			FPlanWaypoint& wpt(m_route[m_route.get_nrwpt()-1]);
			wpt.set_note(m_text);
		}
		m_state = state_gpx_rte_rtept_c;
		return;

	case state_gpx_rte_rtept_unknown1_c:
		m_state = state_gpx_rte_rtept_c;
		return;

	case state_gpx_rte_rtept_unknown2_c:
		m_state = state_gpx_rte_rtept_unknown1_c;
		return;

	case state_garminfpl_c:
		m_state = state_document_c;
		return;

	case state_garminfpl_created_c:
		m_state = state_garminfpl_c;
		return;

	case state_garminfpl_wpttbl_c:
		m_state = state_garminfpl_c;
		return;

	case state_garminfpl_wpttbl_wpt_c:
	{
		m_state = state_garminfpl_wpttbl_c;
		m_garminwpt.lookup(m_dbeng);
		std::pair<GarminFplWpts::iterator,bool> ins(m_garminwpts.insert(m_garminwpt));
		if (!ins.second)
			std::cerr << "Garmin FPL: duplicate waypoint " << m_garminwpt.get_identifier()
				  << '/' << m_garminwpt.get_type_string() << '/' << m_garminwpt.get_countrycode() << std::endl;
		return;
	}

	case state_garminfpl_wpttbl_wpt_ident_c:
		m_state = state_garminfpl_wpttbl_wpt_c;
		m_garminwpt.set_identifier_raw(m_text);
		return;

	case state_garminfpl_wpttbl_wpt_type_c:
		m_state = state_garminfpl_wpttbl_wpt_c;
		m_garminwpt.set_type(m_text);
		return;

	case state_garminfpl_wpttbl_wpt_ctry_c:
		m_state = state_garminfpl_wpttbl_wpt_c;
		m_garminwpt.set_countrycode_raw(m_text);
		return;

	case state_garminfpl_wpttbl_wpt_lat_c:
	{
		m_state = state_garminfpl_wpttbl_wpt_c;
		Point pt(m_garminwpt.get_coord());
		pt.set_lat_deg_dbl(strtod(m_text.c_str(), 0));
		m_garminwpt.set_coord(pt);
       		return;
	}

	case state_garminfpl_wpttbl_wpt_lon_c:
	{
		m_state = state_garminfpl_wpttbl_wpt_c;
		Point pt(m_garminwpt.get_coord());
		pt.set_lon_deg_dbl(strtod(m_text.c_str(), 0));
		m_garminwpt.set_coord(pt);
       		return;
	}

	case state_garminfpl_wpttbl_wpt_cmt_c:
		m_state = state_garminfpl_wpttbl_wpt_c;
		m_garminwpt.set_comment(m_text);
		return;

	case state_garminfpl_wpttbl_wpt_elevation_c:
	{
		m_state = state_garminfpl_wpttbl_wpt_c;
		m_garminwpt.set_elevation(strtod(m_text.c_str(), 0));
		return;
	}

	case state_garminfpl_rte_c:
		m_state = state_garminfpl_c;
                m_route.save_fp();
                m_route.new_fp();
		return;

	case state_garminfpl_rte_rtename_c:
		m_state = state_garminfpl_rte_c;
		return;

	case state_garminfpl_rte_rtedesc_c:
		m_state = state_garminfpl_rte_c;
		m_route.set_note(m_text);
		return;

	case state_garminfpl_rte_fplidx_c:
		m_state = state_garminfpl_rte_c;
		return;

	case state_garminfpl_rte_rtept_c:
	{
		m_state = state_garminfpl_rte_c;
		GarminFplWpts::const_iterator it(m_garminwpts.find(m_garminwpt));
		if (it == m_garminwpts.end()) {
			std::cerr << "ERROR: Garmin FPL Waypoint " << m_garminwpt.get_identifier()
				  << '/' << m_garminwpt.get_type_string() << '/' << m_garminwpt.get_countrycode()
				  << " not found in waypoint table" << std::endl;
			return;
		}
		FPlanWaypoint wpt;
		it->set(wpt);
		m_route.insert_wpt(~0U, wpt);
		return;
	}

	case state_garminfpl_rte_rtept_ident_c:
		m_state = state_garminfpl_rte_rtept_c;
		m_garminwpt.set_identifier_raw(m_text);
		return;

	case state_garminfpl_rte_rtept_type_c:
		m_state = state_garminfpl_rte_rtept_c;
		m_garminwpt.set_type(m_text);
		return;

	case state_garminfpl_rte_rtept_ctry_c:
		m_state = state_garminfpl_rte_rtept_c;
		m_garminwpt.set_countrycode_raw(m_text);
		return;

	default:
		break;
        }
        std::ostringstream oss;
        oss << "XML Parser: Invalid end element in state " << (int)m_state;
        throw std::runtime_error(oss.str());
}

void FPlanXmlImporter::on_characters(const Glib::ustring& characters)
{
	m_text += characters;
}

void FPlanXmlImporter::on_comment(const Glib::ustring& text)
{
}

void FPlanXmlImporter::on_warning(const Glib::ustring& text)
{
}

void FPlanXmlImporter::on_error(const Glib::ustring& text)
{
}

void FPlanXmlImporter::on_fatal_error(const Glib::ustring & text)
{
}

class XmlWriter {
public:
	typedef enum {
		mode_native,
		mode_gpx,
		mode_garminfpl
	} mode_t;

	XmlWriter(DbEngine& db);
	~XmlWriter();
	XmlWriter(const XmlWriter& x) = delete;
	XmlWriter& operator=(const XmlWriter& x) = delete;
	void open(const Glib::ustring& fn, bool compress = false, mode_t mode = mode_native);
	void write(const FPlanRoute& route);
	void close(void);


private:
	GarminFplWpts m_garminwpts;
	GarminFplRtes m_garminrtes;
	DbEngine& m_db;
	xmlTextWriterPtr m_writer;
	xmlBufferPtr m_buffer;
	unsigned int m_number;
	mode_t m_mode;

	void write_native(const FPlanRoute& route);
	void write_gpx(const FPlanRoute& route);
	void write_garminfpl(const FPlanRoute& route);
	void write_header_native(void);
	void write_header_gpx(void);
	void write_header_garminfpl(void);
	void write_footer_garminfpl(void);
};

XmlWriter::XmlWriter(DbEngine& db)
        : m_db(db), m_writer(0), m_buffer(0), m_number(0), m_mode(mode_native)
{
}

void XmlWriter::close(void)
{
        if (m_writer) {
		switch (m_mode) {
		default:
			break;

		case mode_garminfpl:
			write_footer_garminfpl();
			break;
		}
                int rc = xmlTextWriterEndElement(m_writer);
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterEndElement");
                rc = xmlTextWriterEndDocument(m_writer);
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterEndDocument");
                xmlFreeTextWriter(m_writer);
        }
        m_writer = 0;
	if (m_buffer) {
		std::cout << m_buffer->content;
		xmlBufferFree(m_buffer);
		m_buffer = 0;
	}
}

void XmlWriter::open(const Glib::ustring& fn, bool compress, mode_t mode)
{
        close();
	m_number = 1;
	m_mode = mode;
	m_garminwpts.clear();
	m_garminrtes.clear();
	if (fn.empty() || fn == "-") {
		m_buffer = xmlBufferCreate();
		if (!m_buffer)
			throw std::runtime_error("XmlWriter: cannot allocate buffer");
		m_writer = xmlNewTextWriterMemory(m_buffer, 0);
	} else {
		m_writer = xmlNewTextWriterFilename(fn.c_str(), compress);
	}
	if (!m_writer)
		throw std::runtime_error("XmlWriter: cannot create xmlWriter");
        int rc = xmlTextWriterStartDocument(m_writer, NULL, "UTF-8", NULL);
        if (rc < 0)
                throw std::runtime_error("XmlWriter: Error at xmlTextWriterStartDocument");
	switch (m_mode) {
	default:
		write_header_native();
		return;

	case mode_gpx:
		write_header_gpx();
		return;

	case mode_garminfpl:
		write_header_garminfpl();
		return;
	}
}

void XmlWriter::write_header_native(void)
{
        int rc = xmlTextWriterStartElement(m_writer, (const xmlChar *)"PilotFlightPlans");
        if (rc < 0)
                throw std::runtime_error("XmlWriter: Error at xmlTextWriterStartElement");
        rc = xmlTextWriterSetIndent(m_writer, 1);
        if (rc < 0)
                throw std::runtime_error("XmlWriter: Error at xmlTextWriterSetIndent");
}

void XmlWriter::write_header_gpx(void)
{
        int rc = xmlTextWriterStartElement(m_writer, (const xmlChar *)"gpx");
        if (rc < 0)
                throw std::runtime_error("XmlWriter: Error at xmlTextWriterStartElement");
        rc = xmlTextWriterSetIndent(m_writer, 1);
        if (rc < 0)
                throw std::runtime_error("XmlWriter: Error at xmlTextWriterSetIndent");
	rc = xmlTextWriterWriteAttribute(m_writer, (const xmlChar *)"version", (const xmlChar *)"1.1");
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteAttribute");
	rc = xmlTextWriterWriteAttribute(m_writer, (const xmlChar *)"creator", (const xmlChar *)"vfrnav " VERSION " - http://www.autorouter.eu");
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteAttribute");
	rc = xmlTextWriterWriteAttribute(m_writer, (const xmlChar *)"xmlns:xsi", (const xmlChar *)"http://www.w3.org/2001/XMLSchema-instance");
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteAttribute");
	rc = xmlTextWriterWriteAttribute(m_writer, (const xmlChar *)"xmlns", (const xmlChar *)"http://www.topografix.com/GPX/1/1");
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteAttribute");
	rc = xmlTextWriterWriteAttribute(m_writer, (const xmlChar *)"xsi:schemaLocation", (const xmlChar *)"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd");
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteAttribute");
	if (false) {
		rc = xmlTextWriterWriteAttribute(m_writer, (const xmlChar *)"xmlns:navaid", (const xmlChar *)"http://navaid.com/GPX/NAVAID/1/0");
		if (rc < 0)
			throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteAttribute");
	}
	// metadata
	rc = xmlTextWriterStartElement(m_writer, (const xmlChar *)"metadata");
        if (rc < 0)
                throw std::runtime_error("XmlWriter: Error at xmlTextWriterStartElement");
	// link
	rc = xmlTextWriterStartElement(m_writer, (const xmlChar *)"link");
        if (rc < 0)
                throw std::runtime_error("XmlWriter: Error at xmlTextWriterStartElement");
	rc = xmlTextWriterWriteAttribute(m_writer, (const xmlChar *)"href", (const xmlChar *)"http://www.baycom.org/~tom/vfrnav/");
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteAttribute");
	rc = xmlTextWriterEndElement(m_writer);
        if (rc < 0)
                throw std::runtime_error("XmlWriter: Error at xmlTextWriterEndElement");
	// author
	rc = xmlTextWriterStartElement(m_writer, (const xmlChar *)"author");
        if (rc < 0)
                throw std::runtime_error("XmlWriter: Error at xmlTextWriterStartElement");
	rc = xmlTextWriterWriteElement(m_writer, (const xmlChar *)"name", (const xmlChar *)Glib::get_real_name().c_str());
        if (rc < 0)
                throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
	rc = xmlTextWriterEndElement(m_writer);
        if (rc < 0)
                throw std::runtime_error("XmlWriter: Error at xmlTextWriterEndElement");
	// time
	{
		Glib::TimeVal tv;
		tv.assign_current_time();
		rc = xmlTextWriterWriteElement(m_writer, (const xmlChar *)"time", (const xmlChar *)tv.as_iso8601().c_str());
		if (rc < 0)
			throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
	}
	rc = xmlTextWriterEndElement(m_writer);
        if (rc < 0)
                throw std::runtime_error("XmlWriter: Error at xmlTextWriterEndElement");
}

void XmlWriter::write_header_garminfpl(void)
{
        int rc = xmlTextWriterStartElement(m_writer, (const xmlChar *)"flight-plan");
        if (rc < 0)
                throw std::runtime_error("XmlWriter: Error at xmlTextWriterStartElement");
        rc = xmlTextWriterSetIndent(m_writer, 1);
        if (rc < 0)
                throw std::runtime_error("XmlWriter: Error at xmlTextWriterSetIndent");
	rc = xmlTextWriterWriteAttribute(m_writer, (const xmlChar *)"xmlns", (const xmlChar *)"http://www8.garmin.com/xmlschemas/FlightPlan/v1");
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteAttribute");
	{
		Glib::TimeVal tv;
		tv.assign_current_time();
		rc = xmlTextWriterWriteElement(m_writer, (const xmlChar *)"created", (const xmlChar *)tv.as_iso8601().c_str());
		if (rc < 0)
			throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
	}
#if 0
	rc = xmlTextWriterStartElement(m_writer, (const xmlChar *)"waypoint-table");
        if (rc < 0)
                throw std::runtime_error("XmlWriter: Error at xmlTextWriterStartElement");
#endif
}

void XmlWriter::write_footer_garminfpl(void)
{
	m_garminwpts.write_xml(m_writer);
	m_garminrtes.write_xml(m_writer);
	m_garminwpts.clear();
	m_garminrtes.clear();
#if 0
	int rc = xmlTextWriterEndElement(m_writer);
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterEndElement");
#endif
}

void XmlWriter::write(const FPlanRoute& route)
{
	switch (m_mode) {
	default:
		write_native(route);
		return;

	case mode_gpx:
		write_gpx(route);
		return;

	case mode_garminfpl:
		write_garminfpl(route);
		return;
	}
}

void XmlWriter::write_native(const FPlanRoute& route)
{
        int rc = xmlTextWriterStartElement(m_writer, (const xmlChar *)"NavFlightPlan");
        if (rc < 0)
                throw std::runtime_error("XmlWriter: Error at xmlTextWriterStartElement");
        rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"TimeOffBlock", "%u", route.get_time_offblock());
        if (rc < 0)
                throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
        rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"SavedTimeOffBlock", "%u", route.get_save_time_offblock());
        if (rc < 0)
                throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
        rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"TimeOnBlock", "%u", route.get_time_onblock());
        if (rc < 0)
                throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
        rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"SavedTimeOnBlock", "%u", route.get_save_time_onblock());
        if (rc < 0)
                throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
        rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"CurrentWaypoint", "%u", route.get_curwpt());
        if (rc < 0)
                throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
        rc = xmlTextWriterWriteAttribute(m_writer, (const xmlChar *)"Note", (const xmlChar *)route.get_note().c_str());
        if (rc < 0)
                throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteAttribute");
        for (unsigned int nr = 0; nr < route.get_nrwpt(); nr++) {
                const FPlanWaypoint& wpt(route[nr]);
                rc = xmlTextWriterStartElement(m_writer, (const xmlChar *)"WayPoint");
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterStartElement");
                rc = xmlTextWriterWriteAttribute(m_writer, (const xmlChar *)"ICAO", (const xmlChar *)wpt.get_icao().c_str());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteAttribute");
                rc = xmlTextWriterWriteAttribute(m_writer, (const xmlChar *)"name", (const xmlChar *)wpt.get_name().c_str());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteAttribute");
                rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"Time", "%u", wpt.get_time());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
                rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"SavedTime", "%u", wpt.get_save_time());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
                rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"FlightTime", "%u", wpt.get_flighttime());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
                rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"frequency", "%u", wpt.get_frequency());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
                rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"lon", "%d", wpt.get_lon());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
                rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"lat", "%d", wpt.get_lat());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
                rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"alt", "%d", wpt.get_altitude());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
                rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"flags", "%u", wpt.get_flags());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
                rc = xmlTextWriterWriteAttribute(m_writer, (const xmlChar *)"Note", (const xmlChar *)wpt.get_note().c_str());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteAttribute");
                rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"winddir", "%u", wpt.get_winddir());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
                rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"windspeed", "%u", wpt.get_windspeed());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
                rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"qff", "%u", wpt.get_qff());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
                rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"isaoffset", "%d", wpt.get_isaoffset());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
		rc = xmlTextWriterWriteAttribute(m_writer, (const xmlChar *)"pathcode", (const xmlChar *)wpt.get_pathcode_name().c_str());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteAttribute");
                rc = xmlTextWriterWriteAttribute(m_writer, (const xmlChar *)"pathname", (const xmlChar *)wpt.get_pathname().c_str());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteAttribute");
		rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"truealt", "%d", wpt.get_truealt());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
 		rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"dist", "%u", wpt.get_dist());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
 		rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"mass", "%u", wpt.get_mass());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
 		rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"fuel", "%u", wpt.get_fuel());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
 		rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"truetrack", "%u", wpt.get_truetrack());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
 		rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"trueheading", "%u", wpt.get_trueheading());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
 		rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"declination", "%u", wpt.get_declination());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
 		rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"tas", "%u", wpt.get_tas());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
 		rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"rpm", "%u", wpt.get_rpm());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
 		rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"mp", "%u", wpt.get_mp());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
		rc = xmlTextWriterWriteAttribute(m_writer, (const xmlChar *)"type", (const xmlChar *)wpt.get_type_string().c_str());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteAttribute");
		rc = xmlTextWriterEndElement(m_writer);
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterEndElement");
        }
        rc = xmlTextWriterEndElement(m_writer);
        if (rc < 0)
                throw std::runtime_error("XmlWriter: Error at xmlTextWriterEndElement");
}

void XmlWriter::write_gpx(const FPlanRoute& route)
{
        int rc = xmlTextWriterStartElement(m_writer, (const xmlChar *)"rte");
        if (rc < 0)
                throw std::runtime_error("XmlWriter: Error at xmlTextWriterStartElement");
	if (route.get_nrwpt() > 0) {
		Glib::ustring name;
		name += route[0].get_icao_name() + " -> " + route[route.get_nrwpt() - 1].get_icao_name();
		rc = xmlTextWriterWriteElement(m_writer, (const xmlChar *)"name", (const xmlChar *)name.c_str());
		if (rc < 0)
			throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
		rc = xmlTextWriterWriteElement(m_writer, (const xmlChar *)"desc", (const xmlChar *)route.get_note().c_str());
		if (rc < 0)
			throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
	}
	{
		const Glib::ustring& note(route.get_note());
		if (!note.empty()) {
			rc = xmlTextWriterWriteElement(m_writer, (const xmlChar *)"cmt", (const xmlChar *)note.c_str());
			if (rc < 0)
				throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
		}
	}
	rc = xmlTextWriterWriteFormatElement(m_writer, (const xmlChar *)"number", "%u", m_number);
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
	++m_number;
        for (unsigned int nr = 0; nr < route.get_nrwpt(); nr++) {
                const FPlanWaypoint& wpt(route[nr]);
                rc = xmlTextWriterStartElement(m_writer, (const xmlChar *)"rtept");
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterStartElement");
                rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"lat", "%.9g", wpt.get_coord().get_lat_deg_dbl());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
                rc = xmlTextWriterWriteFormatAttribute(m_writer, (const xmlChar *)"lon", "%.9g", wpt.get_coord().get_lon_deg_dbl());
                if (rc < 0)
                        throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
		rc = xmlTextWriterWriteFormatElement(m_writer, (const xmlChar *)"ele", "%.9g", (double)wpt.get_true_altitude() * Point::ft_to_m_dbl);
		if (rc < 0)
			throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
		rc = xmlTextWriterWriteElement(m_writer, (const xmlChar *)"name", (const xmlChar *)wpt.get_name().c_str());
		if (rc < 0)
			throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
		rc = xmlTextWriterWriteElement(m_writer, (const xmlChar *)"desc", (const xmlChar *)wpt.get_icao().c_str());
		if (rc < 0)
			throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
		rc = xmlTextWriterWriteElement(m_writer, (const xmlChar *)"cmt", (const xmlChar *)wpt.get_note().c_str());
		if (rc < 0)
			throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
		{
			Glib::TimeVal tv(wpt.get_time_unix(), 0);
			rc = xmlTextWriterWriteElement(m_writer, (const xmlChar *)"time", (const xmlChar *)tv.as_iso8601().c_str());
			if (rc < 0)
				throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
		}
		rc = xmlTextWriterWriteElement(m_writer, (const xmlChar *)"sym", (const xmlChar *)"Waypoint");
		if (rc < 0)
			throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteElement");
		rc = xmlTextWriterEndElement(m_writer);
		if (rc < 0)
			throw std::runtime_error("XmlWriter: Error at xmlTextWriterEndElement");
	}
        rc = xmlTextWriterEndElement(m_writer);
        if (rc < 0)
                throw std::runtime_error("XmlWriter: Error at xmlTextWriterEndElement");
}

void XmlWriter::write_garminfpl(const FPlanRoute& route)
{
	m_db.opendb();
	m_garminrtes.push_back(GarminFplRte("", route.get_note()));
	for (unsigned int nr = 0; nr < route.get_nrwpt(); ++nr)
		m_garminrtes.back().push_back(m_garminwpts.add(m_db, route[nr]));
}

XmlWriter::~XmlWriter()
{
        //close();
        if (m_writer)
                xmlFreeTextWriter(m_writer);
}

/**
 * Class that writes a flight plan to a ini file format
 * as defined by Microsoft FSX, FS9 or X-Plane 10.
 *
 * For more information on the FSX ini file format refer to
 * http://msdn.microsoft.com/en-us/library/cc707071.aspx
 *
 * Note: Assumption is made that FS9 flightplan format is
 * identical to FSX format.
 *
 * For information on the X-Plane file format refer to
 * https://flightplandatabase.com/dev/specification
 *
 * Note: This documentation is sketchy at best, a better
 * source might result in some code changes.
 */
class IniWriter {
public:
	typedef enum {
		mode_fsx,
		mode_fs9,
		mode_xpx
	} mode_t;

	IniWriter(void);
	~IniWriter();
	IniWriter(const IniWriter& x) = delete;
	IniWriter& operator=(const IniWriter& x) = delete;
	void open(const Glib::ustring& fn, mode_t mode);
	void write(const FPlanRoute& route);
	void close(void);

private:
	void write_fsx(const FPlanRoute& route);
	void write_xpx(const FPlanRoute& route);

	// Functions for printing coords and altitude to match FSX format
	std::string get_lon_str_fsx(Point pt) const;
	std::string get_lat_str_fsx(Point pt) const;
	std::string get_true_altitude_fsx(float alt) const;
	// Functions for printing coords and altitude to match X-Plane format
	std::string get_lon_str_xpx(Point pt) const;
	std::string get_lat_str_xpx(Point pt) const;
	std::string get_true_altitude_xpx(float alt) const;

	static const char endl[];	// We're publishing for a Windows-by-definition-platform, and FSX seems to need this
	mode_t m_mode;
	Glib::ustring m_fn;
	std::stringstream m_stringbuf;
};

const char IniWriter::endl[] = "\r\n";

IniWriter::IniWriter(void)
        : m_stringbuf(std::ios::in|std::ios::out)
{
}

IniWriter::~IniWriter()
{
}

void IniWriter::open(const Glib::ustring& fn, mode_t mode)
{
	m_mode = mode;
	m_fn = fn;
}

void IniWriter::close(void)
{
	if (m_fn.empty() || m_fn == "-") {
		std::cout << m_stringbuf.str();
		m_stringbuf.str(std::string());

	} else {
		std::ofstream ofs(m_fn.c_str(), std::ofstream::out);
		if (!ofs) {
			m_stringbuf.str(std::string());
			throw std::runtime_error("IniWriter: cannot allocate output stream");
		}

		ofs << m_stringbuf.str();
		ofs.close();
		m_stringbuf.str(std::string());
	}
}

/**
 * Write a X-Plane flight plan
 * Some docs here, but not really up to snuff: https://flightplandatabase.com/dev/specification
 */
void IniWriter::write_xpx(const FPlanRoute& route)
{
	if (!route.get_nrwpt())
		return;
	// Write header section
	Glib::ustring name;
	m_stringbuf << "I" << endl
		    << "3 version" << endl
		    << "1" << endl	// what should really go here?
		    << route.get_nrwpt()-1 << endl;

	// Write waypoint section
	for (unsigned int nr = 0; nr < route.get_nrwpt(); nr++) {
		const FPlanWaypoint& wpt(route[nr]);
		bool isUserWaypoint = false;

		/**
		 * 1 - Airport ICAO
		 * 2 - NDB
		 * 3 - VOR
		 * 11 - Fix
		 * 28 - Lat/Lon Position
		 *
		 * vfrnav waypoint types:
		 *	typedef enum {
		 *		type_airport = 0,
		 *		type_intersection = 2,
		 *		type_mapelement = 3,
		 *		type_undefined = 4,
		 *		type_fplwaypoint = 5,
		 *		type_vfrreportingpt = 6,
		 *		type_user = 7
		 *	} type_t;
		 */
		switch (wpt.get_type()) {
		case FPlanWaypoint::type_airport:
			m_stringbuf << "1 ";
			break;

		case FPlanWaypoint::type_navaid:
			switch (wpt.get_navaid()) {
			default:
			case FPlanWaypoint::navaid_vor:
			case FPlanWaypoint::navaid_vordme:
			case FPlanWaypoint::navaid_vortac:
				m_stringbuf << "3 ";
				break;

			case FPlanWaypoint::navaid_ndb:
			case FPlanWaypoint::navaid_ndbdme:
				m_stringbuf << "2 ";
				break;
			}
			break;

		case FPlanWaypoint::type_intersection:
			m_stringbuf << "11 ";
			break;

		default:
		case FPlanWaypoint::type_user:
			m_stringbuf << "28 ";
			isUserWaypoint = true;
			break;
		}

		if (isUserWaypoint) {
			m_stringbuf << get_lat_str_xpx(wpt.get_coord()) << "_" << get_lon_str_xpx(wpt.get_coord()) << ' '
				    << get_true_altitude_xpx(wpt.get_true_altitude()) << ' '
				    << wpt.get_coord().get_lat_deg() << ' ' << wpt.get_coord().get_lon_deg() << endl;

		} else {
			if (wpt.get_icao().empty())
				m_stringbuf << wpt.get_name();
			else
				m_stringbuf << wpt.get_icao();

			m_stringbuf << ' ' << get_true_altitude_xpx(wpt.get_true_altitude()) << ' ';
			m_stringbuf << wpt.get_coord().get_lat_deg() << ' ' << wpt.get_coord().get_lon_deg() << endl;
		}
	}
}


/**
 * Write a Microsoft FSX Flighplan (.PLN) file
 * For a description of the PLN format refer to http://msdn.microsoft.com/en-us/library/cc707071.aspx
 */
void IniWriter::write_fsx(const FPlanRoute& route)
{
	if (!route.get_nrwpt())
		return;
	// Write header section
	Glib::ustring name;
	m_stringbuf << "[flightplan]" << endl
		    << "title=" << route[0].get_icao() << " to " << route[route.get_nrwpt() - 1].get_icao() << endl
		    << "description=" << route[0].get_icao() << ", " << route[route.get_nrwpt() - 1].get_icao() << endl
		    << "type=";
	switch (route.get_flightrules()) {	// The mapping here is somewhat arbitrary...
	case 'I':
	case 'Y':
	case 'Z':
		m_stringbuf << "IFR" << endl;
		break;

	case 'V':
	default:
		m_stringbuf << "VFR" << endl;
		break;
	}

	m_stringbuf << "routetype="
	// Victor airways begin at 18000ft (US) - does FSX care for differences in Europe?
		    << ((route.max_altitude() < 18000) ? '2' : '3') << endl
		    << "cruising_altitude=" << route.max_altitude() << endl;

	{
		const FPlanWaypoint& wpt(route[0]);
		m_stringbuf << "departure_id=" << wpt.get_icao() << ", "
			    << get_lat_str_fsx(wpt.get_coord()) << ", " << get_lon_str_fsx(wpt.get_coord()) << ", "
			    << get_true_altitude_fsx(wpt.get_true_altitude()) << endl;
	}
	{
		const FPlanWaypoint& wpt(route[route.get_nrwpt() - 1]);
		m_stringbuf << "destination_id=" << wpt.get_icao() << ", "
			    << get_lat_str_fsx(wpt.get_coord()) << ", " << get_lon_str_fsx(wpt.get_coord()) << ", "
			    << get_true_altitude_fsx(wpt.get_true_altitude()) << endl;
	}

	m_stringbuf << "departure_name=" << route[0].get_name() << endl
		    << "destination_name=" << route[route.get_nrwpt() - 1].get_name() << endl;

	// Write waypoint section
	for (unsigned int nr = 0; nr < route.get_nrwpt(); nr++) {
		const FPlanWaypoint& wpt(route[nr]);
		m_stringbuf << "waypoint." << nr << "=";

		if (wpt.get_icao() != "")
			m_stringbuf << wpt.get_name() << ", ";
		else
			m_stringbuf << wpt.get_icao() << ", ";

		/**
	 	 * FSX Waypoint types:
	 	 * A : airport
	 	 * I : intersection
	 	 * V : VOR
	 	 * N : NDB
	 	 * U : user
		 * T : ATC
	 	 *
	 	 * vfrnav waypoint types:
		 *	typedef enum {
	 	 *		type_airport = 0,
	 	 *		type_intersection = 2,
	 	 *		type_mapelement = 3,
	 	 *		type_undefined = 4,
	 	 *		type_fplwaypoint = 5,
	 	 *		type_vfrreportingpt = 6,
	 	 *		type_user = 7
	 	 *	} type_t;
		 */
		switch (wpt.get_type()) {
		case FPlanWaypoint::type_airport:
			m_stringbuf << "A, ";
			break;

		case FPlanWaypoint::type_navaid:
			switch (wpt.get_navaid()) {
			default:
			case FPlanWaypoint::navaid_vor:
			case FPlanWaypoint::navaid_vordme:
			case FPlanWaypoint::navaid_vortac:
				m_stringbuf << "V, ";
				break;

			case FPlanWaypoint::navaid_ndb:
			case FPlanWaypoint::navaid_ndbdme:
				m_stringbuf << "N, ";
				break;
			}
			break;

		case FPlanWaypoint::type_intersection:
			m_stringbuf << "I, ";
			break;

		case FPlanWaypoint::type_vfrreportingpt:
			m_stringbuf << "T, ";
			break;

		default:
		case FPlanWaypoint::type_user:
			m_stringbuf << "U, ";
			break;
		}

		m_stringbuf << get_lat_str_fsx(wpt.get_coord()) << ", " << get_lon_str_fsx(wpt.get_coord()) << ", "
			    << get_true_altitude_fsx(wpt.get_true_altitude()) << ',' << endl;
	}
}

void IniWriter::write(const FPlanRoute& route)
{
	switch (m_mode) {
	default:
	case mode_fs9:
	case mode_fsx:
		write_fsx(route);
		break;

	case mode_xpx:
		write_xpx(route);
		break;
	}
}

/**
 * Target format example: +000000122
 */
std::string IniWriter::get_true_altitude_fsx(float alt) const
{
	std::ostringstream oss;
	oss << '+' << std::setfill('0') << std::setw(9) << std::fixed << std::setprecision(2)
	    << std::max(alt, 0.f);	// assuming there are no negative altitudes
	return oss.str();
}

/**
 * Target format example: N47* 45.92'
 */
std::string IniWriter::get_lat_str_fsx(Point pt) const
{
	int32_t min(pt.get_lat());
	char sgn((min < 0) ? 'S' : 'N');
	min = round(abs(min) * (Point::to_deg_dbl * 6000000));
	int32_t deg(min / 6000000);
	min -= deg * 6000000;
	std::ostringstream oss;
	oss << sgn << std::setw(2) << std::setfill('0') << deg << "* "
	    << std::fixed << std::setw(2) << std::setfill('0') << std::setprecision(2) << (min * 0.00001) << '\'';
	return oss.str();
}

/**
 * Target format example: W003* 26.27'
 */
std::string IniWriter::get_lon_str_fsx(Point pt) const
{
	int32_t min(pt.get_lon());
	char sgn((min < 0) ? 'W' : 'E');
	min = round(abs(min) * (Point::to_deg_dbl * 6000000));
	int32_t deg(min / 6000000);
	min -= deg * 6000000;
	std::ostringstream oss;
	oss << sgn << std::setw(3) << std::setfill('0') << deg << "* "
	    << std::fixed << std::setw(2) << std::setfill('0') << std::setprecision(2) << (min * 0.00001) << '\'';
	return oss.str();
}

/**
 * Target format example: 00122
 */
std::string IniWriter::get_true_altitude_xpx(float alt) const
{
	std::ostringstream oss;
	oss << std::setfill('0') << std::setw(5) << std::fixed << std::setprecision(0)
	    << std::max(alt, 0.f);	// assuming there are no negative altitudes
	return oss.str();
}

/**
 * Target format example: +49.221
 */
std::string IniWriter::get_lat_str_xpx(Point pt) const
{
	float latitude = pt.get_lat() * Point::to_deg;
	std::ostringstream oss;
	oss << ((latitude >= 0) ? '+' : '-')
	    << std::fixed << std::setw(6) << std::setfill('0') << std::setprecision(3)
	    << fabsf(latitude);
	return oss.str();
}

/**
 * Target format example: -002.046
 */
std::string IniWriter::get_lon_str_xpx(Point pt) const
{
	float longitude = pt.get_lon() * Point::to_deg;
	std::ostringstream oss;
	oss << ((longitude >= 0) ? '+' : '-')
	    << std::fixed << std::setw(7) << std::setfill('0') << std::setprecision(3)
	    << fabsf(longitude);
	return oss.str();
}


int main(int argc, char *argv[])
{
	static struct option long_options[] = {
		{ "maindir", required_argument, 0, 'm' },
		{ "auxdir", required_argument, 0, 'a' },
		{ "disableaux", no_argument, 0, 'A' },
                { "import", optional_argument, 0, 'i' },
                { "export", optional_argument, 0, 'e' },
                { "gpx", optional_argument, 0, 'g' },
                { "garmin-fpl", optional_argument, 0, 'f' },
                { "fpl", optional_argument, 0, 'f' },
                { "fs9", optional_argument, 0, 0x400 },
		{ "fsx", optional_argument, 0, 0x401 },
		{ "xpx", optional_argument, 0, 0x402 },
		{ "clear", no_argument, 0, 'c' },
		{ "records", required_argument, 0, 'r' },
		{ "icao-flightplan", required_argument, 0, 'F' },
		{ "flightplan", required_argument, 0, 'F' },
		{ "fpl", required_argument, 0, 'F' },
                {0, 0, 0, 0}
	};
	Glib::ustring filename;
	int mode(0), doclear(0), c, err(0);
	typedef std::set<FPlanRoute::id_t> routeids_t;
	routeids_t routeids;
	Glib::ustring dir_main(""), dir_aux("");
	Engine::auxdb_mode_t auxdbmode(Engine::auxdb_prefs);
	bool dis_aux(false);
	std::string icaofpl;

	while ((c = getopt_long(argc, argv, "m:a:Ai:e:g:f:r:cF:", long_options, 0)) != EOF) {
                switch (c) {
		case 'm':
			if (optarg)
				dir_main = optarg;
			break;

		case 'a':
			if (optarg)
				dir_aux = optarg;
			break;

		case 'A':
			dis_aux = true;
			break;

		case 'c':
			doclear = 1;
			break;

		case 'i':
			if (optarg)
				filename = optarg;
			mode = 1;
			break;

		case 'e':
			if (optarg)
				filename = optarg;
			mode = 2 + XmlWriter::mode_native;
			break;

		case 'g':
			if (optarg)
				filename = optarg;
			mode = 2 + XmlWriter::mode_gpx;
			break;

		case 'f':
			if (optarg)
				filename = optarg;
			mode = 2 + XmlWriter::mode_garminfpl;
			break;

		case 0x400:
			if (optarg)
				filename = optarg;
			mode = 10 + IniWriter::mode_fs9;
			break;

		case 0x401:
			if (optarg)
				filename = optarg;
			mode = 10 + IniWriter::mode_fsx;
			break;

		case 0x402:
			if (optarg)
				filename = optarg;
			mode = 10 + IniWriter::mode_xpx;
			break;

		case 'r':
		{
			if (!optarg)
				break;
			const char *cp = optarg;
			for (;;) {
				while (std::isspace(*cp))
					++cp;
				if (!*cp)
					break;
				char *endp;
				FPlanRoute::id_t id(strtoul(cp, &endp, 0));
				if (endp == cp) {
					++err;
					break;
				}
				routeids.insert(id);
				cp = endp;
				if (*cp == ',' || *cp == ':')
					++cp;
			}
			break;
		}

		case 'F':
			if (optarg)
				icaofpl = optarg;
			break;

		default:
			++err;
			break;
                }
        }
        if (err) {
                std::cerr << "usage: vfrnavfplan [-m <dir>] [-a <dir>] [-A] [-i <fn>] [-e <fn>] [-f <fn>] [-g <fn>] [-r <ids>] [-F <fpl>]" << std::endl;
                return EX_USAGE;
        }
	if (dis_aux)
		auxdbmode = Engine::auxdb_none;
	else if (!dir_aux.empty())
		auxdbmode = Engine::auxdb_override;
	DbEngine dbeng(dir_main, auxdbmode, dir_aux);
        FPlan db;
        if (doclear)
                db.delete_database();
        switch (mode) {
	case 1:
	{
		try {
			FPlanXmlImporter parser(db, dbeng);
			parser.set_substitute_entities(true);
			if (!filename.empty())
				parser.parse_file(filename);
			for (; optind < argc; ++optind)
				parser.parse_file(argv[optind]);
		} catch (const xmlpp::exception& ex) {
			std::cerr << "libxml++ exception: " << ex.what() << std::endl;
		} catch (const std::exception& ex) {
			std::cerr << "exception: " << ex.what() << std::endl;
		}
		break;
	}

	case 2 + XmlWriter::mode_native:
	case 2 + XmlWriter::mode_gpx:
	case 2 + XmlWriter::mode_garminfpl:
	{
		if (filename.empty()) {
			if (optind < argc) {
				filename = argv[optind++];
			} else {
				std::cerr << "vfrnavfplan: export: no filename given" << std::endl;
				return EX_USAGE;
			}
		}
		if (optind < argc)
			std::cerr << "vfrnavfplan: export: superfluous arguments" << std::endl;
		try {
			XmlWriter xw(dbeng);
			xw.open(filename, false, (XmlWriter::mode_t)(mode - 2));
			FPlanRoute route(db);
			if (!icaofpl.empty()) {
				Engine eng(dir_main, auxdbmode, dir_aux, false, false);
				IcaoFlightPlan icaof(eng);
			        IcaoFlightPlan::errors_t err(icaof.parse(icaofpl, true));
				if (err.empty()) {
					icaof.set_route(route);
					route.fix_invalid_altitudes(eng);
					std::cerr << "writing flight plan " << icaofpl << std::endl;
					xw.write(route);
				} else {
					std::cerr << "cannot parse flight plan: " << icaofpl << std::endl;
					for (IcaoFlightPlan::errors_t::const_iterator i(err.begin()), e(err.end()); i != e; ++i)
						std::cerr << "  " << *i << std::endl;
				}
			}
			if (routeids.empty() && icaofpl.empty()) {
				bool work = route.load_first_fp();
				while (work) {
					std::cerr << "writing route id " << route.get_id() << std::endl;
					xw.write(route);
					work = route.load_next_fp();
				}
			} else {
				for (routeids_t::const_iterator ri(routeids.begin()), re(routeids.end()); ri != re; ++ri) {
					try {
						route.load_fp(*ri);
					} catch (const std::exception& ex) {
						std::cerr << "Cannot load flight plan " << *ri << ": " << ex.what() << std::endl;
						continue;
					}
					std::cerr << "writing route id " << route.get_id() << std::endl;
					xw.write(route);
				}
			}
			xw.close();
		} catch (const std::exception& ex) {
			std::cerr << "exception: " << ex.what() << std::endl;
		}
		break;
	}

	case 10 + IniWriter::mode_fs9:
	case 10 + IniWriter::mode_fsx:
	case 10 + IniWriter::mode_xpx:
	{
		if (filename.empty()) {
			if (optind < argc) {
				filename = argv[optind++];
			} else {
				std::cerr << "vfrnavfplan: export: no filename given" << std::endl;
				return EX_USAGE;
			}
		}
		if (optind < argc)
			std::cerr << "vfrnavfplan: export: superfluous arguments" << std::endl;
		try {
			IniWriter iw;
			iw.open(filename, (IniWriter::mode_t)(mode - 10));
			FPlanRoute route(db);
			if (!icaofpl.empty()) {
				Engine eng(dir_main, auxdbmode, dir_aux, false, false);
				IcaoFlightPlan icaof(eng);
			        IcaoFlightPlan::errors_t err(icaof.parse(icaofpl, true));
				if (err.empty()) {
					icaof.set_route(route);
					route.fix_invalid_altitudes(eng);
					std::cerr << "writing flight plan " << icaofpl << std::endl;
					iw.write(route);
				} else {
					std::cerr << "cannot parse flight plan: " << icaofpl << std::endl;
					for (IcaoFlightPlan::errors_t::const_iterator i(err.begin()), e(err.end()); i != e; ++i)
						std::cerr << "  " << *i << std::endl;
				}
			}
			if (routeids.empty() && icaofpl.empty()) {
				bool work = route.load_first_fp();
				while (work) {
					std::cerr << "writing route id " << route.get_id() << std::endl;
					iw.write(route);
					work = route.load_next_fp();
				}
			} else {
				for (routeids_t::const_iterator ri(routeids.begin()), re(routeids.end()); ri != re; ++ri) {
					try {
						route.load_fp(*ri);
					} catch (const std::exception& ex) {
						std::cerr << "Cannot load flight plan " << *ri << ": " << ex.what() << std::endl;
						continue;
					}
					std::cerr << "writing route id " << route.get_id() << std::endl;
					iw.write(route);
				}
			}
			iw.close();
		} catch (const std::exception& ex) {
			std::cerr << "exception: " << ex.what() << std::endl;
		}
		break;
	}

	default:
	{
		std::cerr << "must select import or export" << std::endl;
		return EX_USAGE;
	}
        }
        return EX_OK;
}
