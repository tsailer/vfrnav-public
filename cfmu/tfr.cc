#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iomanip>

#include "tfr.hh"
#include "aircraft.h"

const Glib::ustring& TrafficFlowRestrictions::FplWpt::get_ident(void) const
{
	switch (get_type()) {
	default:
	case Vertex::type_airport:
		if (get_icao() == "ZZZZ")
			return get_name();
		/* fall through */

	case Vertex::type_undefined:
	case Vertex::type_navaid:
		if (get_icao().empty())
			return get_name();
		return get_icao();

	case Vertex::type_intersection:
		return get_name();
	}
}

bool TrafficFlowRestrictions::FplWpt::operator==(const RuleWpt& r) const
{
	return get_type() == static_cast<FPlanWaypoint::type_t>(r.get_type()) &&
		get_ident() == r.get_ident() &&
		(has_vor() || !r.is_vor()) &&
		(has_ndb() || !r.is_ndb()) &&
		get_coord().spheric_distance_nmi_dbl(r.get_coord()) < 0.1;
}

bool TrafficFlowRestrictions::FplWpt::within(const AltRange& ar) const
{
	// we do not handle modes correctly; and we probably cannot reasonably, as we do not know the 
	// local QNH in advance
	if (ar.is_lower_valid() && get_altitude() < ar.get_lower_alt())
		return false;
	if (ar.is_upper_valid() && get_altitude() > ar.get_upper_alt())
		return false;
	return true;
}

TrafficFlowRestrictions::Fpl& TrafficFlowRestrictions::Fpl::operator=(const FPlanRoute& rte)
{
	clear();
	for (unsigned int i = 0; i < rte.get_nrwpt(); ++i) {
		FplWpt w;
		(FPlanWaypoint&)w = rte[i];
		push_back(w);
	}
	return *this;
}

Rect TrafficFlowRestrictions::Fpl::get_bbox(void) const
{
	if (empty())
		return Rect();
	Rect r(front().get_coord(), front().get_coord());
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		r = r.add(i->get_coord());
	return r;
}

const std::string& TrafficFlowRestrictions::Message::get_type_string(type_t t)
{
	switch (t) {
	case type_error:
	{
		static const std::string r("error");
		return r;
	}

	case type_warning:
	{
		static const std::string r("warning");
		return r;
	}

	case type_info:
	{
		static const std::string r("info");
		return r;
	}

	case type_tracecond:
	{
		static const std::string r("tracecond");
		return r;
	}

	case type_tracetfe:
	{
		static const std::string r("tracetfe");
		return r;
	}

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

char TrafficFlowRestrictions::Message::get_type_char(type_t t)
{
	switch (t) {
	case type_error:
		return 'E';

	case type_warning:
		return 'W';

	case type_info:
		return 'I';

	case type_tracecond:
		return 'C';

	case type_tracetfe:
		return 'T';

	default:
		return '?';
	}
}

void TrafficFlowRestrictions::Message::add_vertex(unsigned int nr)
{
	m_vertexset.insert(nr);
}

void TrafficFlowRestrictions::Message::add_edge(unsigned int nr)
{
	m_edgeset.insert(nr);
}

void TrafficFlowRestrictions::Message::insert_vertex(unsigned int nr)
{
	{
		set_t x;
		for (set_t::const_iterator i(m_vertexset.begin()), e(m_vertexset.end()); i != e; ++i) {
			if (*i >= nr)
				x.insert(1U + *i);
			else
				x.insert(*i);
		}
		m_vertexset.swap(x);
	}
	{
		set_t x;
		for (set_t::const_iterator i(m_edgeset.begin()), e(m_edgeset.end()); i != e; ++i) {
			if (*i >= nr)
				x.insert(1U + *i);
			else
				x.insert(*i);
		}
		m_edgeset.swap(x);
	}			
}

void TrafficFlowRestrictions::message(const std::string& text, Message::type_t t, const std::string& rule)
{
	message(Message(text, t, rule));
}

void TrafficFlowRestrictions::message(const Message& msg)
{
	m_messages.push_back(msg);
}

TrafficFlowRestrictions::RuleResult::Alternative::Sequence::RulePoint::RulePoint(const std::string& id, const Point& pt, Vertex::type_t typ, bool vor, bool ndb)
	: m_ident(id), m_coord(pt), m_type(typ), m_vor(vor), m_ndb(ndb)
{
	if (m_type != Vertex::type_navaid)
		m_vor = m_ndb = false;
	m_ndb = m_ndb && !m_vor;
}

TrafficFlowRestrictions::RuleResult::Alternative::Sequence::RulePoint::RulePoint(const FplWpt& wpt, bool vor, bool ndb)
	: m_ident(wpt.get_ident()), m_coord(wpt.get_coord()), m_type(static_cast<Vertex::type_t>(wpt.get_type())), m_vor(vor), m_ndb(ndb)
{
	if (m_type != Vertex::type_navaid)
		m_vor = m_ndb = false;
	m_ndb = m_ndb && !m_vor;
}

TrafficFlowRestrictions::RuleResult::Alternative::Sequence::RulePoint::RulePoint(const RuleWpt& wpt)
	: m_ident(wpt.get_ident()), m_coord(wpt.get_coord()), m_type(wpt.get_type()), m_vor(wpt.is_vor()), m_ndb(wpt.is_ndb())
{
}

TrafficFlowRestrictions::RuleResult::Alternative::Sequence::RulePoint::RulePoint(const Vertex& vv)
	: m_ident(vv.get_ident()), m_coord(vv.get_coord()), m_type(vv.get_type()), m_vor(false), m_ndb(false)
{
	if (m_type == Vertex::type_navaid) {
		NavaidsDb::Navaid n;
		if (vv.get(n) && n.is_valid()) {
			m_vor = n.has_vor();
			m_ndb = n.has_ndb();
		}
	}
}

const std::string& TrafficFlowRestrictions::RuleResult::Alternative::Sequence::get_type_string(type_t t)
{
	switch (t) {
	case type_airway:
	{
		static const std::string r("AWY");
		return r;
	}

	case type_dct:
	{
		static const std::string r("DCT");
		return r;
	}

	case type_point:
	{
		static const std::string r("POINT");
		return r;
	}

	case type_sid:
	{
		static const std::string r("SID");
		return r;
	}

	case type_star:
	{
		static const std::string r("STAR");
		return r;
	}

	case type_airspace:
	{
		static const std::string r("AIRSPACE");
		return r;
	}

	case type_invalid:
	default:
	{
		static const std::string r("INVALID");
		return r;
	}
	}
}

TrafficFlowRestrictions::RuleResult::Alternative::Sequence::Sequence(const Glib::RefPtr<const TFRAirspace>& aspc, const AltRange& altrange)
	: m_point0(), m_point1(), m_ident(""), m_airport(""),
	  m_lwralt(altrange.get_lower_alt()), m_upralt(altrange.get_upper_alt()), m_bdryclass(0), m_type(type_invalid),
	  m_lwraltvalid(altrange.is_lower_valid()), m_upraltvalid(altrange.is_upper_valid())
{
	if (!aspc) {
                if (!m_lwraltvalid)
                        m_lwralt = 0;
                if (!m_upraltvalid)
                        m_upralt = std::numeric_limits<int32_t>::max();
        } else {
		m_ident = aspc->get_ident();
		m_bdryclass = aspc->get_bdryclass();
		m_type = type_airspace;
		if (!m_lwraltvalid || !m_upraltvalid) {
			int32_t altlwr1(std::numeric_limits<int32_t>::max());
			int32_t altupr1(std::numeric_limits<int32_t>::min());
			aspc->get_boundingalt(altlwr1, altupr1);
			if (!m_lwraltvalid)
				m_lwralt = altlwr1;
			if (!m_upraltvalid)
				m_upralt = altupr1;
		}
	}
}

TrafficFlowRestrictions::RuleResult::Alternative::Sequence::Sequence(const RuleWptAlt& rwa)
	: m_point0(rwa.get_wpt()), m_point1(), m_ident(), m_airport(),
	  m_lwralt(rwa.get_alt().get_lower_alt()), m_upralt(rwa.get_alt().get_upper_alt()), m_bdryclass(0), m_type(type_point),
	  m_lwraltvalid(rwa.get_alt().is_lower_valid()), m_upraltvalid(rwa.get_alt().is_upper_valid())
{
	if (!m_lwraltvalid)
		m_lwralt = 0;
	if (!m_upraltvalid)
		m_upralt = std::numeric_limits<int32_t>::max();
}

TrafficFlowRestrictions::RuleResult::Alternative::Sequence::Sequence(const SegWptsAlt& swa)
	: m_point0(swa.get_wpt0()), m_point1(swa.get_wpt1()), m_ident(swa.get_airway()), m_airport(),
	  m_lwralt(swa.get_alt().get_lower_alt()), m_upralt(swa.get_alt().get_upper_alt()), m_bdryclass(0), m_type(swa.is_dct() ? type_dct : type_airway),
	  m_lwraltvalid(swa.get_alt().is_lower_valid()), m_upraltvalid(swa.get_alt().is_upper_valid())
{
	if (!m_lwraltvalid)
		m_lwralt = 0;
	if (!m_upraltvalid)
		m_upralt = std::numeric_limits<int32_t>::max();
}

TrafficFlowRestrictions::Result::Result(const std::string& acft, const std::string& equip, Aircraft::pbn_t pbn, char typeofflight)
	: m_aircrafttype(acft), m_equipment(equip), m_pbn(pbn), m_typeofflight(typeofflight), m_result(true)
{
	m_aircrafttype.resize(4, ' ');
	if (!m_aircrafttype.empty())
		m_aircrafttypeclass = Aircraft::get_aircrafttypeclass(get_aircrafttype());
	m_aircrafttypeclass.resize(3, '?');
	Aircraft::pbn_fix_equipment(m_equipment, m_pbn);
}

bool TrafficFlowRestrictions::RuleWpt::is_same(const RuleWpt& x) const
{
	return get_ident() == x.get_ident() && get_type() == x.get_type() && get_coord().spheric_distance_nmi_dbl(x.get_coord()) <= 0.1;
}

bool TrafficFlowRestrictions::RuleWpt::operator==(const RuleWpt& x) const
{
	return get_ident() == x.get_ident() &&
		get_coord().get_lat() == x.get_coord().get_lat() && get_coord().get_lon() == x.get_coord().get_lon() &&
		get_type() == x.get_type() && is_vor() == x.is_vor() && is_ndb() == x.is_ndb();
}

bool TrafficFlowRestrictions::RuleWpt::operator<(const RuleWpt& x) const
{
	int c(get_ident().compare(x.get_ident()));
	if (c)
		return c < 0;
	if (get_coord().get_lat() < x.get_coord().get_lat())
		return true;
	if (x.get_coord().get_lat() < get_coord().get_lat())
		return false;
	if (get_coord().get_lon() < x.get_coord().get_lon())
		return true;
	if (x.get_coord().get_lon() < get_coord().get_lon())
		return false;
	if (get_type() < x.get_type())
		return true;
	if (x.get_type() < get_type())
		return false;
	if (is_vor() < x.is_vor())
		return true;
	if (x.is_vor() < is_vor())
		return false;
	return is_ndb() < x.is_ndb();
}

TrafficFlowRestrictions::DctParameters::AltRange::AltRange(const set_t& set0, const set_t& set1)
{
	m_set[0] = set0;
	m_set[1] = set1;
}

bool TrafficFlowRestrictions::DctParameters::AltRange::is_inside(unsigned int index, type_t alt) const
{
	return m_set[!!index].is_inside(alt);
}

bool TrafficFlowRestrictions::DctParameters::AltRange::is_empty(void) const
{
	return m_set[0].is_empty() && m_set[1].is_empty();
}

void TrafficFlowRestrictions::DctParameters::AltRange::set_empty(void)
{
	m_set[0].set_empty();
	m_set[1].set_empty();
}

void TrafficFlowRestrictions::DctParameters::AltRange::set_full(void)
{
	m_set[0].set_full();
	m_set[1].set_full();
}

void TrafficFlowRestrictions::DctParameters::AltRange::set_interval(type_t alt0, type_t alt1)
{
	m_set[0] = m_set[1] = set_t::Intvl(alt0, alt1);
}

void TrafficFlowRestrictions::DctParameters::AltRange::set(const TrafficFlowRestrictions::AltRange& ar)
{
	if (ar.is_valid())
		m_set[0] = m_set[1] = set_t::Intvl(ar.get_lower_alt(), ar.get_upper_alt() + 1);
	else
		set_full();
}

void TrafficFlowRestrictions::DctParameters::AltRange::invert(void)
{
	m_set[0] = ~m_set[0];
	m_set[1] = ~m_set[1];
}

TrafficFlowRestrictions::DctParameters::AltRange& TrafficFlowRestrictions::DctParameters::AltRange::operator&=(const AltRange& x)
{
	m_set[0] &= x.m_set[0];
	m_set[1] &= x.m_set[1];
	return *this;
}

TrafficFlowRestrictions::DctParameters::AltRange& TrafficFlowRestrictions::DctParameters::AltRange::operator|=(const AltRange& x)
{
	m_set[0] |= x.m_set[0];
	m_set[1] |= x.m_set[1];
	return *this;
}

TrafficFlowRestrictions::DctParameters::AltRange& TrafficFlowRestrictions::DctParameters::AltRange::operator^=(const AltRange& x)
{
	m_set[0] ^= x.m_set[0];
	m_set[1] ^= x.m_set[1];
	return *this;
}

TrafficFlowRestrictions::DctParameters::AltRange& TrafficFlowRestrictions::DctParameters::AltRange::operator-=(const AltRange& x)
{
	m_set[0] -= x.m_set[0];
	m_set[1] -= x.m_set[1];
	return *this;
}

TrafficFlowRestrictions::DctParameters::AltRange TrafficFlowRestrictions::DctParameters::AltRange::operator~(void) const
{
	return AltRange(~m_set[0], ~m_set[1]);
}

TrafficFlowRestrictions::DctParameters::DctParameters(const std::string& ident0, const Point& coord0,
						      const std::string& ident1, const Point& coord1,
						      AltRange::type_t alt0, AltRange::type_t alt1)
	: m_alt(AltRange::set_t::Intvl(alt0, alt1), AltRange::set_t::Intvl(alt0, alt1)),
	  m_dist(std::numeric_limits<double>::quiet_NaN())
{
	m_ident[0] = ident0;
	m_ident[1] = ident1;
	m_coord[0] = coord0;
	m_coord[1] = coord1;
}

double TrafficFlowRestrictions::DctParameters::get_dist(void) const
{
	if (std::isnan(m_dist))
		m_dist = m_coord[0].spheric_distance_nmi_dbl(m_coord[1]);
	return m_dist;
}

int TrafficFlowRestrictions::DctSegments::DctSegment::compare(const DctSegment& x) const
{
	int c(m_pt0.get_ident().compare(x.m_pt0.get_ident()));
	if (c)
		return c;
	c = m_pt1.get_ident().compare(x.m_pt1.get_ident());
	if (c)
		return c;
	if (m_pt0.get_coord().get_lon() < x.m_pt0.get_coord().get_lon())
		return -1;
	if (m_pt0.get_coord().get_lon() > x.m_pt0.get_coord().get_lon())
		return 1;
	if (m_pt0.get_coord().get_lat() < x.m_pt0.get_coord().get_lat())
		return -1;
	if (m_pt0.get_coord().get_lat() > x.m_pt0.get_coord().get_lat())
		return 1;
	if (m_pt1.get_coord().get_lon() < x.m_pt1.get_coord().get_lon())
		return -1;
	if (m_pt1.get_coord().get_lon() > x.m_pt1.get_coord().get_lon())
		return 1;
	if (m_pt1.get_coord().get_lat() < x.m_pt1.get_coord().get_lat())
		return -1;
	if (m_pt1.get_coord().get_lat() > x.m_pt1.get_coord().get_lat())
		return 1;
	return 0;
}

bool TrafficFlowRestrictions::CompareAirspaces::operator()(const AirspacesDb::Airspace& a, const AirspacesDb::Airspace& b)
{
	if (a.get_typecode() < b.get_typecode())
		return true;
	if (a.get_typecode() > b.get_typecode())
		return false;
	if (a.get_bdryclass() < b.get_bdryclass())
		return true;
	if (a.get_bdryclass() > b.get_bdryclass())
		return false;
	return a.get_icao() < b.get_icao();
}

TrafficFlowRestrictions::CondResult& TrafficFlowRestrictions::CondResult::operator&=(const CondResult& x)
{
	m_result = m_result && x.m_result;
	if (!m_result) {
		m_vertexset.clear();
		m_edgeset.clear();
		m_reflocset.clear();
	} else {
		m_vertexset.insert(x.m_vertexset.begin(), x.m_vertexset.end());
		m_edgeset.insert(x.m_edgeset.begin(), x.m_edgeset.end());
		m_reflocset.insert(x.m_reflocset.begin(), x.m_reflocset.end());
	}
	return *this;
}

TrafficFlowRestrictions::CondResult& TrafficFlowRestrictions::CondResult::operator|=(const CondResult& x)
{
	// determine smaller set of conditions
	if (!m_result) {
		if (!x.m_result) {
			m_vertexset.clear();
			m_edgeset.clear();
			m_reflocset.clear();
		} else {
			m_vertexset = x.m_vertexset;
			m_edgeset = x.m_edgeset;
			m_reflocset = x.m_reflocset;
		}
	} else if (!x.m_result) {
		// keep
	} else {
		if (x.m_vertexset.size() < m_vertexset.size())
			m_vertexset = x.m_vertexset;
		if (x.m_edgeset.size() < m_edgeset.size())
			m_edgeset = x.m_edgeset;
		m_reflocset.insert(x.m_reflocset.begin(), x.m_reflocset.end());
	}
	m_result = m_result || x.m_result;
	return *this;
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::CondResult::operator~(void)
{
	return CondResult(!m_result);
}

unsigned int TrafficFlowRestrictions::CondResult::get_first(void) const
{
	if (m_vertexset.empty()) {
		if (m_edgeset.empty())
			return 0U;
		return *m_edgeset.begin();
	}
	if (m_edgeset.empty())
		return *m_vertexset.begin();
	return std::min(*m_vertexset.begin(), *m_edgeset.begin());
}

unsigned int TrafficFlowRestrictions::CondResult::get_last(void) const
{
	if (m_vertexset.empty()) {
		if (m_edgeset.empty())
			return ~0U;
		return 1U + *--m_edgeset.end();
	}
	if (m_edgeset.empty())
		return *--m_vertexset.end();
	return std::min(*--m_vertexset.end(), 1U + *--m_edgeset.end());
}

const TrafficFlowRestrictions::Graph::vertex_descriptor TrafficFlowRestrictions::invalid_vertex_descriptor;

TrafficFlowRestrictions::Graph::vertex_descriptor TrafficFlowRestrictions::get_vertexdesc(const RuleWpt& wpt)
{
	Graph::vertex_descriptor ux;
	if (m_graph.find_intersection(ux, wpt.get_ident(), wpt.get_coord(), (Vertex::typemask_t)(1U << wpt.get_type()), false, false)) {
		const Vertex& uu(m_graph[ux]);
		if (wpt.get_coord().spheric_distance_nmi_dbl(uu.get_coord()) <= 0.1)
			return ux;
	}
	if (wpt.get_type() == Vertex::type_intersection &&
	    m_graph.find_intersection(ux, wpt.get_ident(), wpt.get_coord(), Vertex::typemask_airport, false, false)) {
		const Vertex& uu(m_graph[ux]);
		if (wpt.get_coord().spheric_distance_nmi_dbl(uu.get_coord()) <= 0.1)
			return ux;
	}
	if (!m_graphbbox.is_inside(wpt.get_coord()))
		return invalid_vertex_descriptor;
	{
		std::ostringstream oss;
		oss << (wpt.is_vor() ? "VOR" : (wpt.is_ndb() ? "NDB" : "Intersection")) << ' ' << wpt.get_ident() << ' '
		    << wpt.get_coord().get_lat_str2() << ' ' << wpt.get_coord().get_lon_str2() << " not found in airway graph";
		m_messages.push_back(Message(oss.str(), Message::type_warning));
	}
	return invalid_vertex_descriptor;
}

TrafficFlowRestrictions::TrafficFlowRestrictions(void)
	: m_airportdb(0), m_navaiddb(0), m_waypointdb(0), m_airwaydb(0), m_airspacedb(0), m_pbn(Aircraft::pbn_none), m_typeofflight('G'), m_shortcircuit(false)
{
}

bool TrafficFlowRestrictions::tracerules_exists(const std::string& ruleid) const
{
	return m_tracerules.find(ruleid) != m_tracerules.end();
}

bool TrafficFlowRestrictions::tracerules_add(const std::string& ruleid)
{
	if (ruleid.empty())
		return false;
	std::pair<tracerules_t::iterator,bool> i(m_tracerules.insert(ruleid));
	return i.second;
}

bool TrafficFlowRestrictions::tracerules_erase(const std::string& ruleid)
{
	tracerules_t::iterator i(m_tracerules.find(ruleid));
	if (i == m_tracerules.end())
		return false;
	m_tracerules.erase(i);
	return true;
}

bool TrafficFlowRestrictions::disabledrules_exists(const std::string& ruleid) const
{
	return m_disabledrules.find(ruleid) != m_disabledrules.end();
}

bool TrafficFlowRestrictions::disabledrules_add(const std::string& ruleid)
{
	if (ruleid.empty())
		return false;
	std::pair<disabledrules_t::iterator,bool> i(m_disabledrules.insert(ruleid));
	return i.second;
}

bool TrafficFlowRestrictions::disabledrules_erase(const std::string& ruleid)
{
	disabledrules_t::iterator i(m_disabledrules.find(ruleid));
	if (i == m_disabledrules.end())
		return false;
	m_disabledrules.erase(i);
	return true;
}

std::vector<TrafficFlowRestrictions::RuleResult> TrafficFlowRestrictions::find_rules(const std::string& name, bool exact)
{
	std::vector<TrafficFlowRestrictions::RuleResult> r(m_tfr.find_rules(name, exact));
	for (std::vector<TrafficFlowRestrictions::RuleResult>::iterator i(r.begin()), e(r.end()); i != e; ++i) {
		if (!disabledrules_exists(i->get_rule()))
			continue;
		i->set_disabled(true);
	}
	return r;
}

void TrafficFlowRestrictions::start_add_rules(void)
{
	m_messages.clear();
	m_loadedtfr.clear();
	m_tfr.clear();
	m_dcttfr.clear();
}

void TrafficFlowRestrictions::end_add_rules(void)
{
	{
		std::ostringstream oss;
		oss << m_loadedtfr.size() << " rules loaded";
		message(oss.str(), Message::type_info);
	}
	m_loadedtfr.simplify();
	{
		std::ostringstream oss;
		oss << m_loadedtfr.size() << " rules after simplification";
		message(oss.str(), Message::type_info);
	}
	m_loadedtfr.simplify_gat(true);
	m_loadedtfr.simplify_mil(false);
	{
		std::ostringstream oss;
		oss << m_loadedtfr.size() << " rules after CIV/GAT reduction";
		message(oss.str(), Message::type_info);
	}
	reset_rules();
}

bool TrafficFlowRestrictions::load_rules(std::vector<Message>& msg, const std::string& fname, AirportsDb& airportdb,
					 NavaidsDb& navaiddb, WaypointsDb& waypointdb, AirwaysDb& airwaydb, AirspacesDb& airspacedb)
{
	start_add_rules();
	bool ret(add_rules(msg, fname, airportdb, navaiddb, waypointdb, airwaydb, airspacedb));
	end_add_rules();
	msg.insert(msg.end(), m_messages.begin(), m_messages.end());
	m_messages.clear();
	return ret;
}

bool TrafficFlowRestrictions::load_binary_rules(std::vector<Message>& msg, const std::string& fname, AirportsDb& airportdb,
						NavaidsDb& navaiddb, WaypointsDb& waypointdb, AirwaysDb& airwaydb, AirspacesDb& airspacedb)
{
	start_add_rules();
	bool ret(add_binary_rules(msg, fname, airportdb, navaiddb, waypointdb, airwaydb, airspacedb));
	end_add_rules();
	msg.insert(msg.end(), m_messages.begin(), m_messages.end());
	m_messages.clear();
	return ret;
}

bool TrafficFlowRestrictions::load_binary_rules(std::vector<Message>& msg, std::istream& is, AirportsDb& airportdb,
						NavaidsDb& navaiddb, WaypointsDb& waypointdb, AirwaysDb& airwaydb, AirspacesDb& airspacedb)
{
	start_add_rules();
	bool ret(add_binary_rules(msg, is, airportdb, navaiddb, waypointdb, airwaydb, airspacedb));
	end_add_rules();
	msg.insert(msg.end(), m_messages.begin(), m_messages.end());
	m_messages.clear();
	return ret;
}

void TrafficFlowRestrictions::reset_rules(void)
{
	m_tfr = m_loadedtfr;
}

void TrafficFlowRestrictions::prepare_dct_rules(void)
{
	m_dcttfr.clear();
	for (TrafficFlowRules::const_iterator i(m_tfr.begin()), e(m_tfr.end()); i != e; ++i)
		if ((*i) && (*i)->is_dct())
			m_dcttfr.push_back(*i);
}

unsigned int TrafficFlowRestrictions::count_rules(void) const
{
	return m_tfr.size();
}

unsigned int TrafficFlowRestrictions::count_dct_rules(void) const
{
	return m_dcttfr.size();
}

void TrafficFlowRestrictions::simplify_rules(void)
{
	m_tfr.simplify();
}

void TrafficFlowRestrictions::simplify_rules_bbox(const Rect& bbox)
{
	m_tfr.simplify_bbox(bbox);
}

void TrafficFlowRestrictions::simplify_rules_altrange(int32_t minalt, int32_t maxalt)
{
	m_tfr.simplify_altrange(minalt, maxalt);
}

void TrafficFlowRestrictions::simplify_rules_aircrafttype(const std::string& acfttype)
{
	if (acfttype.empty())
		return;
	std::string actype(acfttype);
	actype.resize(4, ' ');
	m_tfr.simplify_aircrafttype(actype);
	simplify_rules_aircraftclass(Aircraft::get_aircrafttypeclass(actype));
}

void TrafficFlowRestrictions::simplify_rules_aircraftclass(const std::string& acftclass)
{
	if (acftclass.empty())
		return;
	std::string acclass(acftclass);
	acclass.resize(3, '?');
	m_tfr.simplify_aircraftclass(acclass);
}

void TrafficFlowRestrictions::simplify_rules_equipment(const std::string& equipment, Aircraft::pbn_t pbn)
{
	m_tfr.simplify_equipment(equipment, pbn);
}

void TrafficFlowRestrictions::simplify_rules_typeofflight(char type_of_flight)
{
	m_tfr.simplify_typeofflight(type_of_flight);
}

void TrafficFlowRestrictions::simplify_rules_mil(bool mil)
{
	m_tfr.simplify_mil(mil);
}

void TrafficFlowRestrictions::simplify_rules_gat(bool gat)
{
	m_tfr.simplify_gat(gat);
}

void TrafficFlowRestrictions::simplify_rules_dep(const std::string& ident, const Point& coord)
{
	m_tfr.simplify_dep(ident, coord);
}

void TrafficFlowRestrictions::simplify_rules_dest(const std::string& ident, const Point& coord)
{
	m_tfr.simplify_dest(ident, coord);
}

void TrafficFlowRestrictions::simplify_rules_complexity(void)
{
	m_tfr.simplify_complexity();
}

bool TrafficFlowRestrictions::check_dct(std::vector<Message>& msg, DctParameters& dct, bool trace)
{
	m_messages.clear();
	msg.clear();
	DctParameters::AltRange a(m_dcttfr.evaluate_dct(*this, dct, trace));
	dct.get_alt() = a;
	msg.swap(m_messages);
	return !a.is_empty();
}


TrafficFlowRestrictions::DctSegments TrafficFlowRestrictions::get_dct_segments(void) const
{
	DctSegments seg;
	m_tfr.get_dct_segments(seg);
	return seg;
}

TrafficFlowRestrictions::Result TrafficFlowRestrictions::check_fplan(const FPlanRoute& route, char type_of_flight,
								     const std::string& acft_type, const std::string& equipment, Aircraft::pbn_t pbn,
								     AirportsDb& airportdb, NavaidsDb& navaiddb, WaypointsDb& waypointdb,
								     AirwaysDb& airwaydb, AirspacesDb& airspacedb)
{
	m_rules.clear();
	m_messages.clear();
	m_airportdb = &airportdb;
	m_navaiddb = &navaiddb;
	m_waypointdb = &waypointdb;
	m_airwaydb = &airwaydb;
	m_airspacedb = &airspacedb;
	Result res(acft_type, equipment, pbn, type_of_flight);
	m_aircrafttype = res.get_aircrafttype();
	m_aircrafttypeclass = res.get_aircrafttypeclass();
	m_equipment = res.get_equipment();
	m_pbn = res.get_pbn();
	m_typeofflight = res.get_typeofflight();
	res.m_result = (&route) && m_airportdb && m_navaiddb &&
		m_waypointdb && m_airwaydb && m_airspacedb;
	if (res.m_result) {
		m_route = route;
		load_airway_graph();
		res.m_result = check_fplan_depdest() &&
			(check_fplan_specialcase() || check_fplan_rules());
	}
	res.m_fplan.swap(m_route);
	res.m_rules.swap(m_rules);
	res.m_messages.swap(m_messages);
	m_route.clear();
	m_rules.clear();
	m_messages.clear();
	m_aircrafttype.clear();
	m_aircrafttypeclass.clear();
	m_equipment.clear();
	m_typeofflight = 'G';
	m_airportdb = 0;
	m_navaiddb = 0;
	m_waypointdb = 0;
	m_airwaydb = 0;
	m_airspacedb = 0;
	return res;
}

bool TrafficFlowRestrictions::check_fplan_depdest(void)
{
	if (m_route.size() < 2) {
		m_messages.push_back(Message("Route has less than 2 waypoints", Message::type_error));
		return false;
	}
	if (m_route.size() > 2) {
		FplWpt& wpt0(m_route[m_route.size() - 1]);
		FplWpt& wpt1(m_route[m_route.size() - 2]);
		wpt1.set_flags(wpt1.get_flags() ^ ((wpt1.get_flags() ^ wpt0.get_flags()) & FplWpt::altflag_ifr));
	}
	bool ret(true);
	// find waypoint types
	for (unsigned int wptnr = 0; wptnr < m_route.size(); ++wptnr) {
		FplWpt& wpt(m_route[wptnr]);
		wpt.set_wptnr(wptnr);
		{
			std::string tt(wpt.get_icao());
			Vertex::typemask_t tmask(Vertex::typemask_navaid | Vertex::typemask_intersection);
			if (!wptnr || wptnr + 1U >= m_route.size()) {
				tmask |= Vertex::typemask_airport;
				if (tt == "ZZZZ")
					tt.clear();
			}
			if (tt.empty())
				tt = wpt.get_name();
			Graph::vertex_descriptor u;
			bool ok(false);
			if (!tt.empty())
				ok = m_graph.find_intersection(u, tt, wpt.get_coord(), tmask, false, false);
			if (ok) {
				const Vertex& uu(m_graph[u]);
				double dist(wpt.get_coord().spheric_distance_nmi_dbl(uu.get_coord()));
				ok = dist <= 0.1;
				if (ok) {
					//wpt.set_coord(uu.get_coord());
					wpt.set_vertex_descriptor(u);
					wpt.set_type(static_cast<FPlanWaypoint::type_t>(uu.get_type()));
					if (uu.get_type() == Vertex::type_navaid) {
						NavaidsDb::Navaid el;
						if (uu.get(el) && el.is_valid())
							wpt.set_type(static_cast<FPlanWaypoint::type_t>(uu.get_type()),
								     static_cast<FPlanWaypoint::navaid_type_t>(el.get_navaid_type()));
					}
					continue;
				}
				if (wptnr && (wptnr + 1U < m_route.size()) && !wpt.is_ifr() &&
				    (!wptnr || !m_route[wptnr-1].is_ifr()))
					continue;
				std::ostringstream oss;
				oss << "Waypoint " << wpt.get_icao() << '/' << wpt.get_name() << " ("
				    << wpt.get_coord().get_lat_str2() << ' ' << wpt.get_coord().get_lon_str2() << ") not found (distance"
				    << dist << "nmi)";
				Message msg(oss.str(), Message::type_error);
				msg.add_vertex(wptnr);
				m_messages.push_back(msg);
				ret = false;
				continue;
			}
		}
		if (wptnr && (wptnr + 1U < m_route.size()) && !wpt.is_ifr() &&
		    (!wptnr || !m_route[wptnr-1].is_ifr()))
			continue;
		std::ostringstream oss;
		oss << "Waypoint " << wpt.get_icao() << '/' << wpt.get_name() << " ("
		    << wpt.get_coord().get_lat_str2() << ' ' << wpt.get_coord().get_lon_str2() << ") not found";
		Message msg(oss.str(), Message::type_error);
		msg.add_vertex(wptnr);
		m_messages.push_back(msg);
		ret = false;
	}
	if (m_route.front().get_type() != FPlanWaypoint::type_airport) {
		Message msg("First Waypoint not an Airport", Message::type_error);
		msg.add_vertex(0U);
		m_messages.push_back(msg);
		ret = false;
	}
	if (m_route.back().get_type() != FPlanWaypoint::type_airport) {
		Message msg("Last Waypoint not an Airport", Message::type_error);
		msg.add_vertex(m_route.size() - 1U);
		m_messages.push_back(msg);
		ret = false;
	}
	if (!ret)
		return false;
	// Check and Expand Airways
	for (unsigned int wptnr = 0; wptnr < m_route.size(); ++wptnr) {
		FplWpt& wpt(m_route[wptnr]);
		wpt.set_pathname(wpt.get_pathname().uppercase());
		if (wptnr + 1U >= m_route.size()) {
			wpt.set_pathcode(FPlanWaypoint::pathcode_none);
			wpt.set_pathname("");
			continue;
		}
		if (!wpt.is_ifr()) {
			switch (wpt.get_pathcode()) {
			case FPlanWaypoint::pathcode_vfrdeparture:
                        case FPlanWaypoint::pathcode_vfrarrival:
                        case FPlanWaypoint::pathcode_vfrtransition:
				break;

			default:
				wpt.set_pathcode(FPlanWaypoint::pathcode_none);
				break;
			}
			continue;
		}
		FplWpt& wptn(m_route[wptnr + 1U]);
		switch (wpt.get_pathcode()) {
		case FPlanWaypoint::pathcode_sid:
			if (wptnr) {
				wpt.set_pathcode(FPlanWaypoint::pathcode_none);
				wpt.set_pathname("");
			}
			break;

		case FPlanWaypoint::pathcode_star:
			if (wptnr + 2U != m_route.size()) {
				wpt.set_pathcode(FPlanWaypoint::pathcode_none);
				wpt.set_pathname("");
			}
			break;

		case FPlanWaypoint::pathcode_directto:
			wpt.set_pathname("");
			break;

		default:
			wpt.set_pathcode(FPlanWaypoint::pathcode_none);
			wpt.set_pathname("");
			break;

		case FPlanWaypoint::pathcode_airway:
			if (wpt.get_pathname().empty()) {
				wpt.set_pathcode(FPlanWaypoint::pathcode_none);
				break;
			}
			// expand airway
			if (wpt.get_vertex_descriptor() == invalid_vertex_descriptor ||
			    wptn.get_vertex_descriptor() == invalid_vertex_descriptor ||
			    wpt.get_vertex_descriptor() == wptn.get_vertex_descriptor()) {
				std::ostringstream oss;
				oss << "Invalid Airway " << wpt.get_pathname() << " Segment "
				    << wpt.get_ident() << "->" << wptn.get_ident();
				Message msg(oss.str(), Message::type_error);
				msg.add_edge(wptnr);
				m_messages.push_back(msg);
				ret = false;
				wpt.set_pathcode(FPlanWaypoint::pathcode_directto);
				wpt.set_pathname("");
				break;
			}
			{
				std::vector<Graph::vertex_descriptor> predecessors;
				m_graph.shortest_paths(wpt.get_vertex_descriptor(), wpt.get_pathname(), predecessors);
				Graph::vertex_descriptor v(wptn.get_vertex_descriptor());
				if (v == predecessors[v]) {
					std::ostringstream oss;
					oss << "Airway " << wpt.get_pathname() << ": No Segment "
					    << wpt.get_ident() << "->" << wptn.get_ident();
					Message msg(oss.str(), Message::type_error);
					msg.add_edge(wptnr);
					m_messages.push_back(msg);
					ret = false;
					wpt.set_pathcode(FPlanWaypoint::pathcode_directto);
					wpt.set_pathname("");
					break;
				}
				unsigned int wptnrx(wptnr);
				for (;;) {
					FplWpt& wpt(m_route[wptnr]);
					Graph::vertex_descriptor u(predecessors[v]);
					// check airway vertical restrictions
					{
						Graph::out_edge_iterator e0, e1;
						for (boost::tie(e0, e1) = boost::out_edges(u, m_graph); e0 != e1; ++e0) {
							if (boost::target(*e0, m_graph) != v)
								continue;
							const Edge& ee(m_graph[*e0]);
							if (ee.get_ident() != wpt.get_pathname())
								continue;
							if (!(wpt.get_altitude() >= 100 * ee.get_base_level() &&
							      wpt.get_altitude() <= 100 * ee.get_top_level())) {
								const Vertex& uu(m_graph[u]);
								const Vertex& vv(m_graph[v]);
								std::ostringstream oss;
								oss << "Airway " << wpt.get_pathname() << " Segment "
								    << uu.get_ident() << "->" << vv.get_ident()
								    << " F" << std::setw(3) << std::setfill('0') << ee.get_base_level()
								    << "...F" << std::setw(3) << std::setfill('0') << ee.get_top_level()
								    << " does not include requested level F" << std::setw(3)
								    << std::setfill('0') << ((wpt.get_altitude() + 50) / 100);
								Message msg(oss.str(), Message::type_error);
								msg.add_edge(wptnr);
								m_messages.push_back(msg);
								ret = false;
							} else if (ee.get_corridor5_elev() != AirwaysDb::Airway::nodata) {
								int32_t elev(ee.get_corridor5_elev());
								if (elev >= 5000)
									elev += 1000;
								elev += 1000;
								if (elev > wpt.get_altitude()) {
									const Vertex& uu(m_graph[u]);
									const Vertex& vv(m_graph[v]);
									std::ostringstream oss;
									oss << "Airway " << wpt.get_pathname() << " Segment "
									    << uu.get_ident() << "->" << vv.get_ident()
									    << " MOCA " << elev << "ft is above requested altitude "
									    << wpt.get_altitude() << "ft (5nm corridor elev "
									    << elev;
									if (ee.get_terrain_elev() != AirwaysDb::Airway::nodata)
										oss << ", terrain elev " << ee.get_terrain_elev();
									oss << ')';
									Message msg(oss.str(), Message::type_warning);
									msg.add_edge(wptnr);
									m_messages.push_back(msg);
								}
							}
							break;
						}
						if (e0 == e1) {
							const Vertex& uu(m_graph[u]);
							const Vertex& vv(m_graph[v]);
							std::ostringstream oss;
							oss << "Airway " << wpt.get_pathname() << " Segment "
							    << uu.get_ident() << "->" << vv.get_ident()
							    << " not found";
							Message msg(oss.str(), Message::type_warning);
							msg.add_edge(wptnr);
							m_messages.push_back(msg);
						}
					}
					v = u;
					if (v == wpt.get_vertex_descriptor())
						break;
					const Vertex& vv(m_graph[v]);
					if (false && vv.is_ident_numeric())
						continue;
					if (true && !vv.is_ifr_fpl())
						continue;
					FplWpt wptx(wpt);
					if (!vv.set(wptx)) {
						std::ostringstream oss;
						oss << "Cannot insert Waypoint " << vv.get_ident();
						Message msg(oss.str(), Message::type_error);
						msg.add_vertex(wptnr);
						m_messages.push_back(msg);
						continue;
					}
					wptx.set_type(static_cast<FPlanWaypoint::type_t>(vv.get_type()));
					if (vv.get_type() == Vertex::type_navaid) {
						NavaidsDb::Navaid el;
						if (vv.get(el) && el.is_valid())
						        wptx.set_type(static_cast<FPlanWaypoint::type_t>(vv.get_type()),
								      static_cast<FPlanWaypoint::navaid_type_t>(el.get_navaid_type()));
					}
					wptx.set_pathcode(FPlanWaypoint::pathcode_airway);
					wptx.set_pathname(wpt.get_pathname());
					m_route.insert(m_route.begin() + (wptnr + 1U), wptx);
					++wptnrx;
					for (messages_t::iterator mi(m_messages.begin()), me(m_messages.end()); mi != me; ++mi)
						mi->insert_vertex(wptnr + 1U);
				}
				wptnr = wptnrx;
			}
			break;
		}
		if (wpt.get_pathcode() == FPlanWaypoint::pathcode_none && wptnr > 0U && wptnr < m_route.size() - 2U) {
			Message msg("IFR segment but not AWY/DCT/SID/STAR", Message::type_error);
			msg.add_vertex(wptnr);
			m_messages.push_back(msg);
			ret = false;
		}
	}
	return ret;
}

bool TrafficFlowRestrictions::check_fplan_rules(void)
{
	return m_tfr.evaluate(*this, m_rules);
}

void TrafficFlowRestrictions::load_airway_graph(void)
{
        Rect bbox(m_route.get_bbox());
	Rect bboxov(bbox.oversize_nmi(100.f));
	if (boost::num_vertices(m_graph) && m_graphbbox.is_inside(bboxov))
		return;
	// trash airspace cache as well
	m_airspacecache.clear();
	m_graph.clear();
	m_graphbbox = bboxov.oversize_nmi(50.f);
	{
                NavaidsDb::elementvector_t ev(m_navaiddb->find_by_rect(m_graphbbox, ~0, NavaidsDb::Navaid::subtables_none));
                for (NavaidsDb::elementvector_t::iterator i(ev.begin()); i != ev.end();) {
                        switch (i->get_navaid_type()) {
                        case NavaidsDb::Navaid::navaid_vor:
                        case NavaidsDb::Navaid::navaid_vordme:
                        case NavaidsDb::Navaid::navaid_ndb:
                        case NavaidsDb::Navaid::navaid_ndbdme:
                        case NavaidsDb::Navaid::navaid_vortac:
                        case NavaidsDb::Navaid::navaid_dme:
                                ++i;
                                break;

                        default:
                                i = ev.erase(i);
                                break;
                        }
                }
                m_graph.add(ev);
        }
        {
                WaypointsDb::elementvector_t ev(m_waypointdb->find_by_rect(m_graphbbox, ~0, WaypointsDb::Waypoint::subtables_none));
                for (WaypointsDb::elementvector_t::iterator i(ev.begin()); i != ev.end();) {
                        switch (i->get_type()) {
                        case WaypointsDb::Waypoint::type_icao:
                                ++i;
                                break;

                        case WaypointsDb::Waypoint::type_other:
				if (i->get_icao().size() == 5) {
					++i;
					break;
				}
				i = ev.erase(i);
                                break;

                        default:
                                i = ev.erase(i);
                                break;
                        }
                }
                m_graph.add(ev);
        }
        m_graph.add(m_airwaydb->find_by_area(m_graphbbox, ~0, AirwaysDb::Airway::subtables_all));
        m_graph.add(m_airportdb->find_by_rect(m_graphbbox, ~0, NavaidsDb::Navaid::subtables_none));
        if (true) {
                unsigned int counts[4];
                for (unsigned int i = 0; i < sizeof(counts)/sizeof(counts[0]); ++i)
                        counts[i] = 0;
                Graph::vertex_iterator u0, u1;
                for (boost::tie(u0, u1) = boost::vertices(m_graph); u0 != u1; ++u0) {
                        const Vertex& uu(m_graph[*u0]);
                        unsigned int i(uu.get_type());
                        if (i < sizeof(counts)/sizeof(counts[0]))
                                ++counts[i];
                }
                {
                        std::ostringstream oss;
			oss << "Airway area graph: [" << m_graphbbox.get_southwest().get_lat_str2() << ' ' << m_graphbbox.get_southwest().get_lon_str2()
                            << ' ' << m_graphbbox.get_northeast().get_lat_str2() << ' ' << m_graphbbox.get_northeast().get_lon_str2() << "], "
                            << boost::num_vertices(m_graph) << " vertices, " << boost::num_edges(m_graph) << " edges, "
			    << counts[Vertex::type_airport] << " airports, " << counts[Vertex::type_navaid] << " navaids, "
                            << counts[Vertex::type_intersection] << " intersections, " << counts[Vertex::type_mapelement] << " mapelements, route bbox ["
			    << bbox.get_southwest().get_lat_str2() << ' ' << bbox.get_southwest().get_lon_str2()
			    << ' ' << bbox.get_northeast().get_lat_str2() << ' ' << bbox.get_northeast().get_lon_str2() << ']';
			m_messages.push_back(Message(oss.str(), Message::type_info));
                }
	}
}

void TrafficFlowRestrictions::load_disabled_trace_rules_xml(const xmlpp::Element *el)
{
	xmlpp::Node::NodeList nl(el->get_children("disabled"));
	for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
                const xmlpp::Element *el2(static_cast<const xmlpp::Element *>(*ni));
		xmlpp::Attribute *attr;
		if ((attr = el2->get_attribute("ruleid"))) {
			std::string ruleid(attr->get_value());
			if (!ruleid.empty())
				disabledrules_add(ruleid);
			std::cerr << "Rule \"" << ruleid << "\"" << std::endl;
                }
	}
	nl = el->get_children("trace");
	for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
                const xmlpp::Element *el2(static_cast<const xmlpp::Element *>(*ni));
		xmlpp::Attribute *attr;
		if ((attr = el2->get_attribute("ruleid"))) {
                        std::string ruleid(attr->get_value());
			if (!ruleid.empty())
			        tracerules_add(ruleid);
                }
	}
	nl = el->get_children("enabled");
	for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
                const xmlpp::Element *el2(static_cast<const xmlpp::Element *>(*ni));
		xmlpp::Attribute *attr;
		if ((attr = el2->get_attribute("ruleid"))) {
                        std::string ruleid(attr->get_value());
			if (!ruleid.empty())
			        disabledrules_erase(ruleid);
                }
	}
	nl = el->get_children("untrace");
	for (xmlpp::Node::NodeList::const_iterator ni(nl.begin()), ne(nl.end()); ni != ne; ++ni) {
                const xmlpp::Element *el2(static_cast<const xmlpp::Element *>(*ni));
		xmlpp::Attribute *attr;
		if ((attr = el2->get_attribute("ruleid"))) {
                        std::string ruleid(attr->get_value());
			if (!ruleid.empty())
			        tracerules_erase(ruleid);
                }
	}
}

void TrafficFlowRestrictions::save_disabled_trace_rules_xml(xmlpp::Element *el) const
{
	for (disabledrules_const_iterator_t i(disabledrules_begin()), e(disabledrules_end()); i != e; ++i) {
		xmlpp::Element *el2(el->add_child("disabled"));
                el2->set_attribute("ruleid", *i);
	}
	for (tracerules_const_iterator_t i(tracerules_begin()), e(tracerules_end()); i != e; ++i) {
		xmlpp::Element *el2(el->add_child("trace"));
                el2->set_attribute("ruleid", *i);
	}
}

void TrafficFlowRestrictions::save_disabled_trace_rules_file(const std::string& fname) const
{
        std::auto_ptr<xmlpp::Document> doc(new xmlpp::Document());
	save_disabled_trace_rules_xml(doc->create_root_node("tfrruleset"));
        doc->write_to_file_formatted(fname);
}

bool TrafficFlowRestrictions::load_disabled_trace_rules_file(const std::string& fname)
{
        xmlpp::DomParser p(fname);
        if (!p)
                return false;
        xmlpp::Document *doc(p.get_document());
        xmlpp::Element *el(doc->get_root_node());
        if (el->get_name() != "tfrruleset")
                return false;
	load_disabled_trace_rules_xml(el);
	return true;
}

std::string TrafficFlowRestrictions::save_disabled_trace_rules_string(void) const
{
        std::auto_ptr<xmlpp::Document> doc(new xmlpp::Document());
	save_disabled_trace_rules_xml(doc->create_root_node("tfrruleset"));
        return doc->write_to_string_formatted();
}

bool TrafficFlowRestrictions::load_disabled_trace_rules_string(const std::string& data)
{
        xmlpp::DomParser p;
        p.parse_memory(data);
        if (!p)
                return false;
        xmlpp::Document *doc(p.get_document());
        xmlpp::Element *el(doc->get_root_node());
        if (el->get_name() != "tfrruleset")
                return false;
	load_disabled_trace_rules_xml(el);
	return true;
}
