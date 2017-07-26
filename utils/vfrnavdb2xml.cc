//
// C++ Implementation: vfrnavdb2xml
//
// Description: Database sqlite db to XML representation conversion
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <libxml++/libxml++.h>
#include <unistd.h>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <sqlite3.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdexcept>
#include <stdarg.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

#include "dbobj.h"

class XmlWriter {
public:
	XmlWriter(void);
	~XmlWriter();
	void open(const Glib::ustring& fn, bool compress = false);
	//void write(const FPlanRoute& route);
	void close(void);
	void write_db(const Glib::ustring& dbfile);

	void write(FPlan& db);
	void write(AirportsDbQueryInterface& db, bool include_aux = false) { ForEachAirport cb(*this); db.for_each(cb, include_aux); }
	void write(AirspacesDbQueryInterface& db, bool include_aux = false) { ForEachAirspace cb(*this); db.for_each(cb, include_aux); }
	void write(NavaidsDbQueryInterface& db, bool include_aux = false) { ForEachNavaid cb(*this); db.for_each(cb, include_aux); }
	void write(WaypointsDbQueryInterface& db, bool include_aux = false) { ForEachWaypoint cb(*this); db.for_each(cb, include_aux); }
	void write(AirwaysDbQueryInterface& db, bool include_aux = false) { ForEachAirway cb(*this); db.for_each(cb, include_aux); }
	void write(TracksDbQueryInterface& db, bool include_aux = false) { ForEachTrack cb(*this); db.for_each(cb, include_aux); }

	void write(const FPlanRoute& route);

private:
	xmlTextWriterPtr writer;

	void write_attr_signed(const Glib::ustring& name, long long v);
	void write_attr_unsigned(const Glib::ustring& name, unsigned long long v);
	void write_attr(const Glib::ustring& name, const Glib::ustring& v);
	void write_attr(const Glib::ustring& name, WaypointsDb::Waypoint::label_placement_t v);
	void write_attr(const Glib::ustring& name, const Point& v);
	void write_attr(const Glib::ustring& name, bool v);
	void write_attr(const Glib::ustring& name, char v);
	void write_attr(const Glib::ustring & name, FPlanWaypoint::pathcode_t pc);
	void write_startelement(const Glib::ustring& name);
	void write_endelement(void);
	void write_modtime(time_t t);

	class ForEachAirport : public AirportsDb::ForEach {
	public:
		ForEachAirport(XmlWriter& xmlw) : m_xmlw(xmlw) {}
		bool operator()(const AirportsDb::Airport& e);
		bool operator()(const std::string& id);

	private:
		XmlWriter& m_xmlw;
	};

	class ForEachAirspace : public AirspacesDb::ForEach {
	public:
		ForEachAirspace(XmlWriter& xmlw) : m_xmlw(xmlw) {}
		bool operator()(const AirspacesDb::Airspace& e);
		bool operator()(const std::string& id);

	private:
		XmlWriter& m_xmlw;
	};

	class ForEachNavaid : public NavaidsDb::ForEach {
	public:
		ForEachNavaid(XmlWriter& xmlw) : m_xmlw(xmlw) {}
		bool operator()(const NavaidsDb::Navaid& e);
		bool operator()(const std::string& id);

	private:
		XmlWriter& m_xmlw;
	};

	class ForEachWaypoint : public WaypointsDb::ForEach {
	public:
		ForEachWaypoint(XmlWriter& xmlw) : m_xmlw(xmlw) {}
		bool operator()(const WaypointsDb::Waypoint& e);
		bool operator()(const std::string& id);

	private:
		XmlWriter& m_xmlw;
	};

	class ForEachAirway : public AirwaysDb::ForEach {
	public:
		ForEachAirway(XmlWriter& xmlw) : m_xmlw(xmlw) {}
		bool operator()(const AirwaysDb::Airway& e);
		bool operator()(const std::string& id);

	private:
		XmlWriter& m_xmlw;
	};

	class ForEachTrack : public TracksDb::ForEach {
	public:
		ForEachTrack(XmlWriter& xmlw) : m_xmlw(xmlw) {}
		bool operator()(const TracksDb::Track& e);
		bool operator()(const std::string& id);

	private:
		XmlWriter& m_xmlw;
	};

	friend class ForEachAirport;
	friend class ForEachAirspace;
	friend class ForEachNavaid;
	friend class ForEachWaypoint;
	friend class ForEachTrack;
};

XmlWriter::XmlWriter(void)
	: writer(0)
{
}

void XmlWriter::close(void)
{
	if (writer) {
		int rc = xmlTextWriterEndElement(writer);
		if (rc < 0)
			throw std::runtime_error("XmlWriter: Error at xmlTextWriterEndElement");
		rc = xmlTextWriterEndDocument(writer);
		if (rc < 0)
			throw std::runtime_error("XmlWriter: Error at xmlTextWriterEndDocument");
		xmlFreeTextWriter(writer);
	}
	writer = 0;
}

void XmlWriter::open(const Glib::ustring & fn, bool compress)
{
	close();
	writer = xmlNewTextWriterFilename(fn.c_str(), compress);
	int rc = xmlTextWriterStartDocument(writer, NULL, "UTF-8", NULL);
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterStartDocument");
	rc = xmlTextWriterStartElement(writer, (const xmlChar *)"VFRNav");
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterStartElement");
	rc = xmlTextWriterSetIndent(writer, 1);
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterSetIndent");
}

void XmlWriter::write_attr_signed(const Glib::ustring & name, long long v)
{
	int rc = xmlTextWriterWriteFormatAttribute(writer, (const xmlChar *)name.c_str(), "%lld", v);
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
}

void XmlWriter::write_attr_unsigned(const Glib::ustring & name, unsigned long long v)
{
	int rc = xmlTextWriterWriteFormatAttribute(writer, (const xmlChar *)name.c_str(), "%llu", v);
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteFormatAttribute");
}

void XmlWriter::write_attr(const Glib::ustring & name, const Glib::ustring & v)
{
	int rc = xmlTextWriterWriteAttribute(writer, (const xmlChar *)name.c_str(), (const xmlChar *)v.c_str());
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterWriteAttribute");
}

void XmlWriter::write_attr(const Glib::ustring & name, WaypointsDb::Waypoint::label_placement_t v)
{
	write_attr(name, WaypointsDb::Waypoint::get_label_placement_name(v));
}

void XmlWriter::write_attr(const Glib::ustring & name, const Point & v)
{
	write_attr_signed(name + "lon", v.get_lon());
	write_attr_signed(name + "lat", v.get_lat());
}

void XmlWriter::write_attr(const Glib::ustring & name, bool v)
{
	write_attr_unsigned(name, (unsigned int)v);
}

void XmlWriter::write_attr(const Glib::ustring & name, char v)
{
	Glib::ustring s;
	s.push_back(v);
	write_attr(name, s);
}

void XmlWriter::write_attr(const Glib::ustring & name, FPlanWaypoint::pathcode_t pc)
{
	write_attr(name, FPlanWaypoint::get_pathcode_name(pc));
}

void XmlWriter::write_modtime(time_t t)
{
	write_attr_unsigned("modtime", t);
}

void XmlWriter::write_startelement(const Glib::ustring & name)
{
	int rc = xmlTextWriterStartElement(writer, (const xmlChar *)name.c_str());
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterStartElement");
}

void XmlWriter::write_endelement(void)
{
	int rc = xmlTextWriterEndElement(writer);
	if (rc < 0)
		throw std::runtime_error("XmlWriter: Error at xmlTextWriterEndElement");
}

void XmlWriter::write(FPlan & db)
{
	FPlanRoute route(db);
	bool ok(route.load_first_fp());
	while (ok) {
		write(route);
		ok = route.load_next_fp();
	}
}

void XmlWriter::write(const FPlanRoute& route)
{
	write_startelement("NavFlightPlan");
	write_attr_unsigned("TimeOffBlock", route.get_time_offblock());
	write_attr_unsigned("SavedTimeOffBlock", route.get_save_time_offblock());
	write_attr_unsigned("TimeOnBlock", route.get_time_onblock());
	write_attr_unsigned("SavedTimeOnBlock", route.get_save_time_onblock());
	write_attr_unsigned("CurrentWaypoint", route.get_curwpt());
	write_attr("Note", route.get_note());

	for (unsigned int nr = 0; nr < route.get_nrwpt(); ++nr) {
		const FPlanWaypoint& wpt(route[nr]);
		write_startelement("WayPoint");
		write_attr("ICAO", wpt.get_icao());
		write_attr("name", wpt.get_name());
		write_attr("pathcode", wpt.get_pathcode_name());
		write_attr("pathname", wpt.get_pathname());
		write_attr_unsigned("Time", wpt.get_time());
		write_attr_unsigned("SavedTime", wpt.get_save_time());
		write_attr_unsigned("FlightTime", wpt.get_flighttime());
		write_attr_unsigned("frequency", wpt.get_frequency());
		write_attr_signed("lon", wpt.get_lon());
		write_attr_signed("lat", wpt.get_lat());
		write_attr_signed("alt", wpt.get_altitude());
		write_attr_unsigned("flags", wpt.get_flags());
		write_attr_unsigned("winddir", wpt.get_winddir());
		write_attr_unsigned("windspeed", wpt.get_windspeed());
		write_attr_unsigned("qff", wpt.get_qff());
		write_attr_signed("isaoffset", wpt.get_isaoffset());
		write_attr("Note", wpt.get_note());
		write_attr_signed("truealt", wpt.get_truealt());
		write_attr_unsigned("truetrack", wpt.get_truetrack());
		write_attr_unsigned("trueheading", wpt.get_trueheading());
		write_attr_unsigned("declination", wpt.get_declination());
		write_attr_unsigned("dist", wpt.get_dist());
		write_attr_unsigned("mass", wpt.get_mass());
		write_attr_unsigned("fuel", wpt.get_fuel());
		write_attr_unsigned("tas", wpt.get_tas());
		write_attr_unsigned("rpm", wpt.get_rpm());
		write_attr_unsigned("mp", wpt.get_mp());
		write_attr("type", wpt.get_type_string());
		write_endelement();
	}
	write_endelement();
}

bool XmlWriter::ForEachAirport::operator()(const AirportsDb::Airport & e)
{
	m_xmlw.write_startelement("Airport");
	m_xmlw.write_attr("sourceid", e.get_sourceid());
	m_xmlw.write_attr("", e.get_coord());
	m_xmlw.write_attr("icao", e.get_icao());
	m_xmlw.write_attr("name", e.get_name());
	m_xmlw.write_attr("authority", e.get_authority());
	m_xmlw.write_attr("vfrremark", e.get_vfrrmk());
	m_xmlw.write_attr_signed("elev", e.get_elevation());
	m_xmlw.write_attr("sw", e.get_bbox().get_southwest());
	m_xmlw.write_attr("ne", e.get_bbox().get_northeast());
	m_xmlw.write_attr("type", e.get_type_string());
	m_xmlw.write_attr_unsigned("frules", e.get_flightrules());
	m_xmlw.write_attr("label_placement", e.get_label_placement());
	m_xmlw.write_modtime(e.get_modtime());
	for (unsigned int i = 0; i < e.get_nrrwy(); ++i) {
		const AirportsDb::Airport::Runway& rwy(e.get_rwy(i));
		m_xmlw.write_startelement("Runway");
		m_xmlw.write_attr_unsigned("length", rwy.get_length());
		m_xmlw.write_attr_unsigned("width", rwy.get_width());
		m_xmlw.write_attr("surface", rwy.get_surface());
		m_xmlw.write_attr_unsigned("flags", rwy.get_flags());
		m_xmlw.write_startelement("he");
		m_xmlw.write_attr("ident", rwy.get_ident_he());
		m_xmlw.write_attr_unsigned("tda", rwy.get_he_tda());
		m_xmlw.write_attr_unsigned("lda", rwy.get_he_lda());
		m_xmlw.write_attr("", rwy.get_he_coord());
		m_xmlw.write_attr_unsigned("disp", rwy.get_he_disp());
		m_xmlw.write_attr_unsigned("hdg", rwy.get_he_hdg());
		m_xmlw.write_attr_signed("elev", rwy.get_he_elev());
		for (unsigned int j = 0; j < AirportsDb::Airport::Runway::nr_lights; ++j) {
			std::ostringstream oss;
			oss << "light" << j;
			m_xmlw.write_attr_unsigned(oss.str(), rwy.get_he_light(j));
		}
		m_xmlw.write_endelement();
		m_xmlw.write_startelement("le");
		m_xmlw.write_attr("ident", rwy.get_ident_le());
		m_xmlw.write_attr_unsigned("tda", rwy.get_le_tda());
		m_xmlw.write_attr_unsigned("lda", rwy.get_le_lda());
		m_xmlw.write_attr("", rwy.get_le_coord());
		m_xmlw.write_attr_unsigned("disp", rwy.get_le_disp());
		m_xmlw.write_attr_unsigned("hdg", rwy.get_le_hdg());
		m_xmlw.write_attr_signed("elev", rwy.get_le_elev());
		for (unsigned int j = 0; j < AirportsDb::Airport::Runway::nr_lights; ++j) {
			std::ostringstream oss;
			oss << "light" << j;
			m_xmlw.write_attr_unsigned(oss.str(), rwy.get_le_light(j));
		}
		m_xmlw.write_endelement();
		m_xmlw.write_endelement();
	}
	for (unsigned int i = 0; i < e.get_nrhelipads(); ++i) {
		const AirportsDb::Airport::Helipad& hp(e.get_helipad(i));
		m_xmlw.write_startelement("Helipad");
		m_xmlw.write_attr("ident", hp.get_ident());
		m_xmlw.write_attr_unsigned("length", hp.get_length());
		m_xmlw.write_attr_unsigned("width", hp.get_width());
		m_xmlw.write_attr("surface", hp.get_surface());
		m_xmlw.write_attr("", hp.get_coord());
		m_xmlw.write_attr_unsigned("hdg", hp.get_hdg());
		m_xmlw.write_attr_signed("elev", hp.get_elev());
		m_xmlw.write_attr_unsigned("flags", hp.get_flags());
		m_xmlw.write_endelement();
	}
	for (unsigned int i = 0; i < e.get_nrcomm(); ++i) {
		const AirportsDb::Airport::Comm& comm(e.get_comm(i));
		m_xmlw.write_startelement("Comm");
		m_xmlw.write_attr("name", comm.get_name());
		m_xmlw.write_attr("sector", comm.get_sector());
		m_xmlw.write_attr("oprhours", comm.get_oprhours());
		if (comm[0])
			m_xmlw.write_attr_unsigned("freq0", comm[0]);
		if (comm[1])
			m_xmlw.write_attr_unsigned("freq1", comm[1]);
		if (comm[2])
			m_xmlw.write_attr_unsigned("freq2", comm[2]);
		if (comm[3])
			m_xmlw.write_attr_unsigned("freq3", comm[3]);
		if (comm[4])
			m_xmlw.write_attr_unsigned("freq4", comm[4]);
		m_xmlw.write_endelement();
	}
	for (unsigned int i = 0; i < e.get_nrvfrrte(); ++i) {
		const AirportsDb::Airport::VFRRoute& rte(e.get_vfrrte(i));
		m_xmlw.write_startelement("VFRRoute");
		m_xmlw.write_attr("name", rte.get_name());
		m_xmlw.write_attr_signed("minalt", rte.get_minalt());
		m_xmlw.write_attr_signed("maxalt", rte.get_maxalt());
		for (unsigned int j = 0; j < rte.size(); j++) {
			const AirportsDb::Airport::VFRRoute::VFRRoutePoint& pt(rte[j]);
			m_xmlw.write_startelement("VFRRoutePoint");
			m_xmlw.write_attr("name", pt.get_name());
			m_xmlw.write_attr("", pt.get_coord());
			m_xmlw.write_attr_signed("alt", pt.get_altitude());
			m_xmlw.write_attr("label_placement", pt.get_label_placement());
			m_xmlw.write_attr("symbol", pt.get_ptsym());
			m_xmlw.write_attr("pathcode", pt.get_pathcode_string());
			m_xmlw.write_attr("altcode", pt.get_altcode_string());
			m_xmlw.write_attr("atairport", pt.is_at_airport());
			m_xmlw.write_endelement();
		}
		m_xmlw.write_endelement();
	}
	for (unsigned int i = 0; i < e.get_nrlinefeatures(); ++i) {
		const AirportsDb::Airport::Polyline& lf(e.get_linefeature(i));
		m_xmlw.write_startelement("LineFeature");
		m_xmlw.write_attr("name", lf.get_name());
		m_xmlw.write_attr("surface", lf.get_surface());
		m_xmlw.write_attr_unsigned("flags", lf.get_flags());
		for (AirportsDb::Airport::Polyline::const_iterator ni(lf.begin()), ne(lf.end()); ni != ne; ++ni) {
			m_xmlw.write_startelement("Node");
			m_xmlw.write_attr("", ni->get_point());
			m_xmlw.write_attr("cp", ni->get_prev_controlpoint());
			m_xmlw.write_attr("cn", ni->get_next_controlpoint());
			m_xmlw.write_attr_unsigned("flags", ni->get_flags());
			for (unsigned int i = 0; i < AirportsDb::Airport::PolylineNode::nr_features; ++i) {
				std::ostringstream oss;
				oss << "feature" << i;
				m_xmlw.write_attr_unsigned(oss.str(), ni->get_feature(i));
			}
			m_xmlw.write_endelement();
		}
		m_xmlw.write_endelement();
	}
	for (unsigned int i = 0; i < e.get_nrpointfeatures(); ++i) {
		const AirportsDb::Airport::PointFeature& pf(e.get_pointfeature(i));
		m_xmlw.write_startelement("PointFeature");
		m_xmlw.write_attr_unsigned("feature", pf.get_feature());
		m_xmlw.write_attr("", pf.get_coord());
		m_xmlw.write_attr_unsigned("hdg", pf.get_hdg());
		m_xmlw.write_attr_signed("elev", pf.get_elev());
		m_xmlw.write_attr_unsigned("subtype", pf.get_subtype());
		m_xmlw.write_attr_signed("attr1", pf.get_attr1());
		m_xmlw.write_attr_signed("attr2", pf.get_attr2());
		m_xmlw.write_attr("name", pf.get_name());
		m_xmlw.write_attr("rwyident", pf.get_rwyident());
		m_xmlw.write_endelement();
	}
	for (unsigned int i = 0; i < e.get_nrfinalapproachsegments(); ++i) {
		const AirportsDb::Airport::MinimalFAS& fas(e.get_finalapproachsegment(i));
		m_xmlw.write_startelement("FinalApproachSegment");
		m_xmlw.write_attr("refpathid", fas.get_referencepathident());
		m_xmlw.write_attr("ftp", fas.get_ftp());
		m_xmlw.write_attr("fpap", fas.get_fpap());
		m_xmlw.write_attr_unsigned("optyp", fas.get_optyp());
		m_xmlw.write_attr_unsigned("rwy", fas.get_rwy());
		m_xmlw.write_attr_unsigned("rte", fas.get_rte());
		m_xmlw.write_attr_unsigned("refpathds", fas.get_referencepathdataselector());
		m_xmlw.write_attr_signed("ftpheight", fas.get_ftp_height());
		m_xmlw.write_attr_unsigned("apchtch", fas.get_approach_tch());
		m_xmlw.write_attr_unsigned("gpa", fas.get_glidepathangle());
		m_xmlw.write_attr_unsigned("cwidth", fas.get_coursewidth());
		m_xmlw.write_attr_unsigned("dlenoffs", fas.get_dlengthoffset());
		m_xmlw.write_attr_unsigned("hal", fas.get_hal());
		m_xmlw.write_attr_unsigned("val", fas.get_val());
		m_xmlw.write_endelement();
	}
	m_xmlw.write_endelement();
	return true;
}

bool XmlWriter::ForEachAirport::operator()(const std::string & id)
{
	m_xmlw.write_startelement("AirportDelete");
	m_xmlw.write_attr("sourceid", id);
	m_xmlw.write_endelement();
	return true;
}

bool XmlWriter::ForEachAirspace::operator()(const AirspacesDb::Airspace & e)
{
	m_xmlw.write_startelement("Airspace");
	m_xmlw.write_attr("sourceid", e.get_sourceid());
	m_xmlw.write_attr("sw", e.get_bbox().get_southwest());
	m_xmlw.write_attr("ne", e.get_bbox().get_northeast());
	m_xmlw.write_attr("label", e.get_labelcoord());
	m_xmlw.write_attr("icao", e.get_icao());
	m_xmlw.write_attr("name", e.get_name());
	m_xmlw.write_attr("ident", e.get_ident());
	m_xmlw.write_attr("classexcept", e.get_classexcept());
	m_xmlw.write_attr("efftime", e.get_efftime());
	m_xmlw.write_attr("note", e.get_note());
	m_xmlw.write_attr("commname", e.get_commname());
	if (e.get_commfreq(0))
		m_xmlw.write_attr_unsigned("commfreq0", e.get_commfreq(0));
	if (e.get_commfreq(1))
		m_xmlw.write_attr_unsigned("commfreq1", e.get_commfreq(1));
	m_xmlw.write_attr("class", e.get_class_string());
	m_xmlw.write_attr_signed("gndelevmin", e.get_gndelevmin());
	m_xmlw.write_attr_signed("gndelevmax", e.get_gndelevmax());
	m_xmlw.write_attr_unsigned("width", e.get_width());
	m_xmlw.write_attr_signed("altlwr", e.get_altlwr());
	m_xmlw.write_attr_signed("altupr", e.get_altupr());
	m_xmlw.write_attr_unsigned("altlwrflags", e.get_altlwrflags());
	m_xmlw.write_attr_unsigned("altuprflags", e.get_altuprflags());
	m_xmlw.write_attr("label_placement", e.get_label_placement());
	m_xmlw.write_modtime(e.get_modtime());
	for (unsigned int i = 0; i < e.get_nrsegments(); ++i) {
		const AirspacesDb::Airspace::Segment& seg(e[i]);
		m_xmlw.write_startelement("Segment");
		m_xmlw.write_attr("c1", seg.get_coord1());
		m_xmlw.write_attr("c2", seg.get_coord2());
		m_xmlw.write_attr("c0", seg.get_coord0());
		m_xmlw.write_attr("shape", seg.get_shape());
		m_xmlw.write_attr_signed("radius1", seg.get_radius1());
		m_xmlw.write_attr_signed("radius2", seg.get_radius2());
		m_xmlw.write_endelement();
	}
	for (unsigned int i = 0; i < e.get_nrcomponents(); ++i) {
		const AirspacesDb::Airspace::Component& comp(e.get_component(i));
		m_xmlw.write_startelement("Component");
		m_xmlw.write_attr("icao", comp.get_icao());
		m_xmlw.write_attr("class", AirspacesDb::Airspace::get_class_string(comp.get_bdryclass(), comp.get_typecode()));
		m_xmlw.write_attr("operator", comp.get_operator_string());
		m_xmlw.write_endelement();
	}
	const MultiPolygonHole& poly(e.get_polygon());
	for (MultiPolygonHole::const_iterator phi(poly.begin()), phe(poly.end()); phi != phe; ++phi) {
		const PolygonHole& ph(*phi);
		m_xmlw.write_startelement("Polygon");
		if (ph.get_nrinterior())
			m_xmlw.write_startelement("exterior");
		{
			const PolygonSimple& p(ph.get_exterior());
			for (PolygonSimple::const_iterator pi(p.begin()), pe(p.end()); pi != pe; ++pi) {
				m_xmlw.write_startelement("Point");
				m_xmlw.write_attr("", *pi);
				m_xmlw.write_endelement();
			}
		}
		if (ph.get_nrinterior())
			m_xmlw.write_endelement();
		for (unsigned int i = 0; i < ph.get_nrinterior(); ++i) {
			m_xmlw.write_startelement("interior");
			const PolygonSimple& p(ph[i]);
			for (PolygonSimple::const_iterator pi(p.begin()), pe(p.end()); pi != pe; ++pi) {
				m_xmlw.write_startelement("Point");
				m_xmlw.write_attr("", *pi);
				m_xmlw.write_endelement();
			}
			m_xmlw.write_endelement();
		}
		m_xmlw.write_endelement();
	}
	m_xmlw.write_endelement();
	return true;
}

bool XmlWriter::ForEachAirspace::operator()(const std::string & id)
{
	m_xmlw.write_startelement("AirspaceDelete");
	m_xmlw.write_attr("sourceid", id);
	m_xmlw.write_endelement();
	return true;
}

bool XmlWriter::ForEachNavaid::operator()(const NavaidsDb::Navaid & e)
{
	m_xmlw.write_startelement("Navaid");
	m_xmlw.write_attr("sourceid", e.get_sourceid());
	m_xmlw.write_attr("", e.get_coord());
	m_xmlw.write_attr("icao", e.get_icao());
	m_xmlw.write_attr("name", e.get_name());
	m_xmlw.write_attr("authority", e.get_authority());
	m_xmlw.write_attr("dme", e.get_dmecoord());
	m_xmlw.write_attr_signed("elev", e.get_elev());
	m_xmlw.write_attr_signed("range", e.get_range());
	m_xmlw.write_attr_unsigned("freq", e.get_frequency());
	m_xmlw.write_attr("type", e.get_navaid_typename());
	m_xmlw.write_attr("label_placement", e.get_label_placement());
	m_xmlw.write_attr("inservice", e.is_inservice());
	m_xmlw.write_modtime(e.get_modtime());
	m_xmlw.write_endelement();
	return true;
}

bool XmlWriter::ForEachNavaid::operator()(const std::string & id)
{
	m_xmlw.write_startelement("NavaidDelete");
	m_xmlw.write_attr("sourceid", id);
	m_xmlw.write_endelement();
	return true;
}

bool XmlWriter::ForEachWaypoint::operator()(const WaypointsDb::Waypoint & e)
{
	m_xmlw.write_startelement("Waypoint");
	m_xmlw.write_attr("sourceid", e.get_sourceid());
	m_xmlw.write_attr("", e.get_coord());
	m_xmlw.write_attr("icao", e.get_icao());
	m_xmlw.write_attr("name", e.get_name());
	m_xmlw.write_attr("authority", e.get_authority());
	m_xmlw.write_attr("usage", e.get_usage_name());
	m_xmlw.write_attr("type", e.get_type_name());
	m_xmlw.write_attr("label_placement", e.get_label_placement());
	m_xmlw.write_modtime(e.get_modtime());
	m_xmlw.write_endelement();
	return true;
}

bool XmlWriter::ForEachWaypoint::operator()(const std::string & id)
{
	m_xmlw.write_startelement("WaypointDelete");
	m_xmlw.write_attr("sourceid", id);
	m_xmlw.write_endelement();
	return true;
}

bool XmlWriter::ForEachAirway::operator()(const AirwaysDb::Airway & e)
{
	m_xmlw.write_startelement("Airway");
	m_xmlw.write_attr("sourceid", e.get_sourceid());
	m_xmlw.write_attr("begin", e.get_begin_coord());
	m_xmlw.write_attr("end", e.get_end_coord());
	m_xmlw.write_attr("beginname", e.get_begin_name());
	m_xmlw.write_attr("endname", e.get_end_name());
	m_xmlw.write_attr("name", e.get_name());
	m_xmlw.write_attr_signed("baselevel", e.get_base_level());
	m_xmlw.write_attr_signed("toplevel", e.get_top_level());
	m_xmlw.write_attr_signed("terrainelev", e.get_terrain_elev());
	m_xmlw.write_attr_signed("corridor5elev", e.get_corridor5_elev());
	m_xmlw.write_attr("type", e.get_type_name());
	m_xmlw.write_attr("label_placement", e.get_label_placement());
	m_xmlw.write_modtime(e.get_modtime());
	m_xmlw.write_endelement();
	return true;
}

bool XmlWriter::ForEachAirway::operator()(const std::string & id)
{
	m_xmlw.write_startelement("AirwayDelete");
	m_xmlw.write_attr("sourceid", id);
	m_xmlw.write_endelement();
	return true;
}

bool XmlWriter::ForEachTrack::operator()(const TracksDb::Track & e)
{
	m_xmlw.write_startelement("Track");
	m_xmlw.write_attr("sourceid", e.get_sourceid());
	m_xmlw.write_attr("fromicao", e.get_fromicao());
	m_xmlw.write_attr("fromname", e.get_fromname());
	m_xmlw.write_attr("toicao", e.get_toicao());
	m_xmlw.write_attr("toname", e.get_toname());
	m_xmlw.write_attr("notes", e.get_notes());
	m_xmlw.write_attr_unsigned("offblocktime", e.get_offblocktime());
	m_xmlw.write_attr_unsigned("takeofftime", e.get_takeofftime());
	m_xmlw.write_attr_unsigned("landingtime", e.get_landingtime());
	m_xmlw.write_attr_unsigned("onblocktime", e.get_onblocktime());
	m_xmlw.write_modtime(e.get_modtime());
	for (unsigned int i = 0; i < e.size(); ++i) {
		const TracksDb::Track::TrackPoint& tp(e[i]);
		m_xmlw.write_startelement("Point");
		m_xmlw.write_attr("", tp.get_coord());
		m_xmlw.write_attr_unsigned("time", tp.get_timefrac8());
		if (tp.is_alt_valid())
			m_xmlw.write_attr_signed("alt", tp.get_alt());
		m_xmlw.write_endelement();
	}
	m_xmlw.write_endelement();
	return true;
}

bool XmlWriter::ForEachTrack::operator()(const std::string & id)
{
	m_xmlw.write_startelement("TrackDelete");
	m_xmlw.write_attr("sourceid", id);
	m_xmlw.write_endelement();
	return true;
}

void XmlWriter::write_db(const Glib::ustring & dbfile)
{
	sqlite3x::sqlite3_connection db(dbfile);
	sqlite3x::sqlite3_command cmd(db, "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite%';");
	sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
	while (cursor.step()) {
		std::string tblname(cursor.getstring(0));
		if (tblname == "fplan") {
			cursor.close();
			cmd.finalize();
			FPlan fpdb(db.take());
			write(fpdb);
			return;
		}
		if (tblname == "airports") {
			cursor.close();
			cmd.finalize();
			AirportsDb arptdb;
			arptdb.DbBase<AirportsDb::element_t>::take(db.take());
			write(arptdb);
			return;
		}
		if (tblname == "airspaces") {
			cursor.close();
			cmd.finalize();
			AirspacesDb adb;
			adb.DbBase<AirspacesDb::element_t>::take(db.take());
			write(adb);
			return;
		}
		if (tblname == "navaids") {
			cursor.close();
			cmd.finalize();
			NavaidsDb navdb;
			navdb.DbBase<NavaidsDb::element_t>::take(db.take());
			write(navdb);
			return;
		}
		if (tblname == "waypoints") {
			cursor.close();
			cmd.finalize();
			WaypointsDb wdb;
			wdb.DbBase<WaypointsDb::element_t>::take(db.take());
			write(wdb);
			return;
		}
		if (tblname == "airways") {
			cursor.close();
			cmd.finalize();
			AirwaysDb awdb;
			awdb.DbBase<AirwaysDb::element_t>::take(db.take());
			write(awdb);
			return;
		}
		if (tblname == "tracks") {
			cursor.close();
			cmd.finalize();
			TracksDb wdb;
			wdb.DbBase<TracksDb::element_t>::take(db.take());
			write(wdb);
			return;
		}
	}
	throw std::runtime_error("cannot determine database type " + dbfile);
}



XmlWriter::~XmlWriter()
{
	//close();
	if (writer)
		xmlFreeTextWriter(writer);
}



int main(int argc, char *argv[])
{
	typedef enum {
		dbtype_airports,
		dbtype_airspaces,
		dbtype_navaids,
		dbtype_waypoints,
		dbtype_airways,
		dbtype_tracks
	} dbtype_t;
	static struct option long_options[] = {
		{ "output", required_argument, 0, 'o' },
		{ "compress", no_argument, 0, 'c' },
#ifdef HAVE_PQXX
		{ "pgairports", required_argument, 0, 0x400 + dbtype_airports },
		{ "pgairspaces", required_argument, 0, 0x400 + dbtype_airspaces },
		{ "pgnavaids", required_argument, 0, 0x400 + dbtype_navaids },
		{ "pgwaypoints", required_argument, 0, 0x400 + dbtype_waypoints },
		{ "pgairways", required_argument, 0, 0x400 + dbtype_airways },
		{ "pgtracks", required_argument, 0, 0x400 + dbtype_tracks },
#endif
		{0, 0, 0, 0}
	};
	typedef std::pair<dbtype_t,std::string> dbpair_t;
	typedef std::vector<dbpair_t> dbpairs_t;
	dbpairs_t dbpairs;
	Glib::ustring outfile("out.xml");
	int c, err(0);
	bool do_compr(false);

	while ((c = getopt_long(argc, argv, "o:c", long_options, 0)) != EOF) {
		switch (c) {
		case 'c':
			do_compr = 1;
			break;

		case 'o':
			if (optarg)
				outfile = optarg;
			break;

		case 0x400 + dbtype_airports:
		case 0x400 + dbtype_airspaces:
		case 0x400 + dbtype_navaids:
		case 0x400 + dbtype_waypoints:
		case 0x400 + dbtype_airways:
		case 0x400 + dbtype_tracks:
			if (optarg)
				dbpairs.push_back(dbpair_t((dbtype_t)(c - 0x400), optarg));
			break;

		default:
			err++;
			break;
		}
	}
	if (err) {
		std::cerr << "usage: vfrnavdb2xml [-o <fn>] [-c]"
#ifdef HAVE_PQXX
			" [--pgairports <dbstr>] [--pgairspaces <dbstr>] [--pgnavaids <dbstr>]"
			" [--pgwaypoints <dbstr>] [--pgairways <dbstr>] [--pgtracks <dbstr>]"
#endif
			  << std::endl;
		return EX_USAGE;
	}
	try {
		XmlWriter xw;
		xw.open(outfile, do_compr);
#ifdef HAVE_PQXX
		for (dbpairs_t::const_iterator di(dbpairs.begin()), de(dbpairs.end()); di != de; ++di) {
			switch (di->first) {
			case dbtype_airports:
			{
				AirportsPGDb db(di->second, AirportsPGDb::read_only);
				xw.write(db);
				break;
			}

			case dbtype_airspaces:
			{
				AirspacesPGDb db(di->second, AirspacesPGDb::read_only);
				xw.write(db);
				break;
			}

			case dbtype_navaids:
			{
				NavaidsPGDb db(di->second, NavaidsPGDb::read_only);
				xw.write(db);
				break;
			}

			case dbtype_waypoints:
			{
				WaypointsPGDb db(di->second, WaypointsPGDb::read_only);
				xw.write(db);
				break;
			}

			case dbtype_airways:
			{
				AirwaysPGDb db(di->second, AirwaysPGDb::read_only);
				xw.write(db);
				break;
			}

			case dbtype_tracks:
			{
				TracksPGDb db(di->second, TracksPGDb::read_only);
				xw.write(db);
				break;
			}

			default:
				break;
			}
		}
#endif
		for (; optind < argc; optind++) {
			std::cerr << "Parsing file " << argv[optind] << std::endl;
			xw.write_db(argv[optind]);
		}
		xw.close();
	} catch (const std::exception& ex) {
		std::cerr << "exception: " << ex.what() << std::endl;
		return EX_DATAERR;
	}
	return EX_OK;
}
