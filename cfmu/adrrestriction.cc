#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iomanip>
#include <sstream>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

#include "adr.hh"
#include "adrdb.hh"
#include "adrhibernate.hh"
#include "adrrestriction.hh"

using namespace ADR;


Message::Message(const std::string& text, type_t t, uint64_t tm, const Object::const_ptr_t& p)
	: m_obj(p), m_text(text), m_time(tm), m_type(t)
{
}

const std::string& Message::get_type_string(type_t t)
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

	case type_trace:
	{
		static const std::string r("trace");
		return r;
	}

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

char Message::get_type_char(type_t t)
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
		return 'R';

	case type_trace:
		return 'T';

	default:
		return '?';
	}
}

void Message::add_vertex(unsigned int nr)
{
	m_vertexset.insert(nr);
}

void Message::add_edge(unsigned int nr)
{
	m_edgeset.insert(nr);
}

void Message::insert_vertex(unsigned int nr)
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

const std::string& Message::get_ident(void) const
{
	const IdentTimeSlice& ts(get_obj()->operator()(get_time()).as_ident());
	return ts.get_ident();
}

const std::string& Message::get_rule(void) const
{
	const FlightRestrictionTimeSlice& ts(get_obj()->operator()(get_time()).as_flightrestriction());
	return ts.get_ident();
}

std::string Message::to_str(void) const
{
	std::ostringstream oss;
	oss << get_type_char() << ':';
	{
		const std::string& id(get_ident());
		if (!id.empty())
			oss << " R:" << id;
	}
	if (!get_vertexset().empty()) {
		oss << " V:";
		bool subseq(false);
		for (ADR::Message::set_t::const_iterator i(get_vertexset().begin()), e(get_vertexset().end()); i != e; ++i) {
			if (subseq)
				oss << ',';
			subseq = true;
			oss << *i;
		}
	}
	if (!get_edgeset().empty()) {
		oss << " E:";
		bool subseq(false);
		for (ADR::Message::set_t::const_iterator i(get_edgeset().begin()), e(get_edgeset().end()); i != e; ++i) {
			if (subseq)
				oss << ',';
			subseq = true;
			oss << *i;
		}
	}
	oss << ' ' << get_text();
	return oss.str();
}

RuleSegment::RuleSegment(type_t t, const AltRange& alt, const Object::const_ptr_t& wpt0, const Object::const_ptr_t& wpt1,
			 const Object::const_ptr_t& awy, const SegmentsList& segs)
	: m_airway(awy), m_alt(alt), m_segments(segs), m_type(t)
{
	m_wpt[0] = wpt0;
	m_wpt[1] = wpt1;
}

Glib::RefPtr<RestrictionElement> RuleSegment::get_restrictionelement(void) const
{
	Link w0(Object::ptr_t::cast_const(get_wpt0()));
	Link w1(Object::ptr_t::cast_const(get_wpt1()));
	Link a(Object::ptr_t::cast_const(get_airway()));
	switch (get_type()) {
	case type_airway:
		return Glib::RefPtr<RestrictionElement>(new RestrictionElementRoute(get_alt(), w0, w1, a));

	case type_dct:
		return Glib::RefPtr<RestrictionElement>(new RestrictionElementRoute(get_alt(), w0, w1));

	case type_point:
		return Glib::RefPtr<RestrictionElement>(new RestrictionElementPoint(get_alt(), w0));

	case type_sid:
	case type_star:
		return Glib::RefPtr<RestrictionElement>(new RestrictionElementSidStar(get_alt(), a, get_type() == type_star));

	case type_airspace:
		return Glib::RefPtr<RestrictionElement>(new RestrictionElementAirspace(get_alt(), w0));

	case type_invalid:
	default:
		return Glib::RefPtr<RestrictionElement>();
	}
}

const std::string& to_str(ADR::RuleSegment::type_t t)
{
	switch (t) {
	case ADR::RuleSegment::type_airway:
	{
		static const std::string r("airway");
		return r;
	}

	case ADR::RuleSegment::type_dct:
	{
		static const std::string r("dct");
		return r;
	}

	case ADR::RuleSegment::type_point:
	{
		static const std::string r("point");
		return r;
	}

	case ADR::RuleSegment::type_sid:
	{
		static const std::string r("sid");
		return r;
	}

	case ADR::RuleSegment::type_star:
	{
		static const std::string r("star");
		return r;
	}

	case ADR::RuleSegment::type_airspace:
	{
		static const std::string r("airspace");
		return r;
	}

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

std::string RuleSegment::to_shortstr(timetype_t tm) const
{
	Glib::RefPtr<RestrictionElement> re(get_restrictionelement());
	if (!re)
		return "INVALID";
	return re->to_shortstr(tm);
}

std::string RuleSequence::to_shortstr(timetype_t tm) const
{
	std::ostringstream oss;
	bool subseq(false);
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		if (subseq)
			oss << ' ';
		subseq = true;
		oss << i->to_shortstr(tm);
	}
	return oss.str();
}

bool RestrictionResults::is_ok(void) const
{
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		const ADR::FlightRestrictionTimeSlice& rule(i->get_timeslice());
		if (!rule.is_valid() || !rule.is_enabled())
			continue;
		return false;
	}
	return true;
}

RestrictionEvalMessages::RestrictionEvalMessages(void)
{
}

RestrictionEvalMessages::RestrictionEvalMessages(const RestrictionEvalMessages& x)
	: m_messages(x.m_messages), m_currule(x.m_currule)
{
}

RestrictionEvalMessages& RestrictionEvalMessages::operator=(const RestrictionEvalMessages& x)
{
	m_messages = x.m_messages;
	m_currule = x.m_currule;
	return *this;
}

void RestrictionEvalMessages::message(const std::string& text, Message::type_t t, timetype_t tm)
{
	message(Message(text, t, tm, m_currule));
}

void RestrictionEvalMessages::message(const Message& msg)
{
	m_messages.push_back(msg);
}

RestrictionEvalBase::RestrictionEvalBase(Database *db)
	: m_db(db)
{
}

RestrictionEvalBase::RestrictionEvalBase(const RestrictionEvalBase& x)
	: RestrictionEvalMessages(x), m_specialdateeval(x.m_specialdateeval), m_allrules(x.m_allrules), m_db(x.m_db)
{
}

RestrictionEvalBase& RestrictionEvalBase::operator=(const RestrictionEvalBase& x)
{
	RestrictionEvalMessages::operator=(x);
	m_specialdateeval = x.m_specialdateeval;
	m_allrules = x.m_allrules;
	m_db = x.m_db;
	return *this;
}

RestrictionEvalBase::~RestrictionEvalBase(void)
{
}

Database& RestrictionEvalBase::get_dbref(void) const
{
	Database *db(get_db());
	if (db)
		return *db;
	throw std::runtime_error("RestrictionEval::get_dbref: no database");
}

void RestrictionEvalBase::load_rules(void)
{
	Database& db(get_dbref());
	m_allrules.clear();
	// make sure we get unique objects
	db.clear_cache();
	Database::findresults_t r;
	r = db.find_all(Database::loadmode_link, 0, std::numeric_limits<uint64_t>::max(),
			Object::type_flightrestriction, Object::type_flightrestriction, 0);
	db.clear_cache();
	for (Database::findresults_t::iterator i(r.begin()), e(r.end()); i != e; ++i) {
		FlightRestriction::ptr_t p(FlightRestriction::ptr_t::cast_dynamic(i->get_obj()));
		if (!p)
			continue;
		m_allrules.push_back(p);
	}
	m_specialdateeval.load(db);
}

RestrictionEval::Waypoint::Waypoint(const FPLWaypoint& w)
	: FPLWaypoint(w), m_vertex(Graph::invalid_vertex_descriptor), m_edge(Graph::edge_descriptor())
{
}

RestrictionEval::RestrictionEval(Database *db, Graph *graph)
	: RestrictionEvalBase(db), m_graph(graph), m_graphmode(graphmode_foreign)
{
}

RestrictionEval::RestrictionEval(const RestrictionEval& x)
	: RestrictionEvalBase(x), m_rules(x.m_rules), m_fplan(x.m_fplan), m_waypoints(x.m_waypoints),
	  m_graph(x.m_graph), m_graphmode(x.m_graphmode)
{
	if (m_graphmode != graphmode_foreign)
		m_graph = new Graph(*x.m_graph);
}

RestrictionEval& RestrictionEval::operator=(const RestrictionEval& x)
{
	RestrictionEvalBase::operator=(x);
	m_rules = x.m_rules;
	m_fplan = x.m_fplan;
	m_waypoints = x.m_waypoints;
	m_graph = x.m_graph;
	m_graphmode = x.m_graphmode;
	if (m_graphmode != graphmode_foreign)
		m_graph = new Graph(*x.m_graph);
	return *this;
}

RestrictionEval::~RestrictionEval(void)
{
	set_graph(0);
}

FlightPlan RestrictionEval::get_fplan(void) const
{
	FlightPlan fpl(m_fplan);
	fpl.clear();
	for (waypoints_t::const_iterator i(m_waypoints.begin()), e(m_waypoints.end()); i != e; ++i)
		fpl.push_back(*i);
	return fpl;
}

void RestrictionEval::set_fplan(const FlightPlan& fpl)
{
	m_fplan = fpl;
	m_waypoints.clear();
	for (FlightPlan::const_iterator i(m_fplan.begin()), e(m_fplan.end()); i != e; ++i)
		m_waypoints.push_back(*i);
	m_fplan.clear();
}

Graph& RestrictionEval::get_graphref(void)
{
	Graph *g(get_graph());
	if (!g) {
		std::auto_ptr<Graph> gg(new Graph());
		m_graph = g = gg.release();
		m_graphmode = graphmode_allocated;
	}
	Database *db(get_db());
	if (m_graphmode == graphmode_allocated && !m_waypoints.empty() && db) {
		Rect bbox(Point::invalid, Point::invalid);
		bool first(true);
		for (waypoints_t::const_iterator wi(m_waypoints.begin()), we(m_waypoints.end()); wi != we; ++wi) {
			if (wi->get_coord().is_invalid())
				continue;
			if (first) {
				bbox = Rect(wi->get_coord(), wi->get_coord());
				first = false;
				continue;
			}
			bbox = bbox.add(wi->get_coord());
		}
		bbox = bbox.oversize_nmi(100.);
		Database::findresults_t r(db->find_by_bbox(bbox, Database::loadmode_link, get_departuretime(), get_departuretime() + 1,
							   Object::type_pointstart, Object::type_lineend));
		graph_add(r);
		m_graphmode = graphmode_loaded;
	}
	return *g;
}

void RestrictionEval::set_graph(Graph *g)
{
	if (m_graphmode != graphmode_foreign)
		delete m_graph;
	m_graph = g;
	m_graphmode = graphmode_foreign;
}

void RestrictionEval::graph_add(const Object::ptr_t& p)
{
	if (!p || !get_graph() || !get_db())
		return;
	p->link(*get_db(), ~0U);
	unsigned int added(get_graph()->add(get_departuretime(), p));
	switch (p->get_type()) {
	case Object::type_sid:
	case Object::type_star:
	case Object::type_route:
		if (!added)
			return;
		break;

	default:
		return;
	}
	graph_add(get_db()->find_dependson(p->get_uuid(), Database::loadmode_link, get_departuretime(), get_departuretime() + 1,
					   Object::type_pointstart, Object::type_lineend, 0));
}

void RestrictionEval::graph_add(const Database::findresults_t& r)
{
	for (Database::findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri)
		graph_add(ri->get_obj());
}

void RestrictionEval::message(const std::string& text, Message::type_t t)
{
	message(Message(text, t, get_departuretime(), m_currule));
}

void RestrictionEval::load_aup(AUPDatabase& aupdb, timetype_t starttime, timetype_t endtime)
{
	m_condavail.clear();
	if (!get_db())
		return;
	m_condavail.load(*get_db(), aupdb, starttime, endtime);
}

void RestrictionEval::load_rules(void)
{
	Database& db(get_dbref());
	m_allrules.clear();
	m_rules.clear();
	// make sure we get unique objects
	db.clear_cache();
	Database::findresults_t r;
	if (m_waypoints.size()) {
		r = db.find_all(Database::loadmode_link, get_departuretime(), get_departuretime() + 1,
				Object::type_flightrestriction, Object::type_flightrestriction, 0);
	} else {
		r = db.find_all(Database::loadmode_link, 0, std::numeric_limits<uint64_t>::max(),
				Object::type_flightrestriction, Object::type_flightrestriction, 0);
	}
	db.clear_cache();
	for (Database::findresults_t::iterator i(r.begin()), e(r.end()); i != e; ++i) {
		FlightRestriction::ptr_t p(FlightRestriction::ptr_t::cast_dynamic(i->get_obj()));
		if (!p)
			continue;
		m_allrules.push_back(p);
	}
	m_rules = m_allrules;
	m_specialdateeval.load(db);
}

void RestrictionEval::reset_rules(void)
{
	m_rules = m_allrules;
}

bool RestrictionEval::disable_rule(const std::string& id)
{
	bool found(false);
	for (rules_t::iterator ri(m_rules.begin()), re(m_rules.end()); ri != re; ++ri) {
		const FlightRestrictionTimeSlice& ts((*ri)->operator()(get_departuretime()).as_flightrestriction());
		if (!ts.is_valid())
			continue;
		if (ts.get_ident() != id)
			continue;
		found = true;
		if (!ts.is_enabled())
			continue;
		*ri = (*ri)->clone_obj();
		FlightRestrictionTimeSlice& ts1((*ri)->operator()(get_departuretime()).as_flightrestriction());
		ts1.set_enabled(false);
	}
	return found;
}

bool RestrictionEval::trace_rule(const std::string& id)
{
	bool found(false);
	for (rules_t::iterator ri(m_rules.begin()), re(m_rules.end()); ri != re; ++ri) {
		const FlightRestrictionTimeSlice& ts((*ri)->operator()(get_departuretime()).as_flightrestriction());
		if (!ts.is_valid())
			continue;
		if (ts.get_ident() != id)
			continue;
		found = true;
		if (ts.is_trace())
			continue;
		*ri = (*ri)->clone_obj();
		FlightRestrictionTimeSlice& ts1((*ri)->operator()(get_departuretime()).as_flightrestriction());
		ts1.set_trace(true);
	}
	return found;
}

void RestrictionEval::simplify_rules(void)
{
	for (rules_t::iterator ri(m_rules.begin()), re(m_rules.end()); ri != re;) {
		FlightRestriction::ptr_t p(*ri);
		if (!p) {
			ri = m_rules.erase(ri);
			re = m_rules.end();
			continue;
		}
		{
			FlightRestriction::ptr_t p1(p->simplify());
			if (p1)
				p.swap(p1);
		}
		{
			FlightRestriction::ptr_t p1(p->simplify_complexity_crossingpoints());
			if (p1)
				p.swap(p1);
		}
		{
			FlightRestriction::ptr_t p1(p->simplify_complexity_crossingsegments());
			if (p1)
				p.swap(p1);
		}
		{
			FlightRestriction::ptr_t p1(p->simplify_complexity_closedairspace());
			if (p1)
				p.swap(p1);
		}
		if (!p->is_keep()) {
			ri = m_rules.erase(ri);
			re = m_rules.end();
			continue;
		}
		*ri = p;
		++ri;
	}
}

void RestrictionEval::simplify_rules_time(timetype_t tm0, timetype_t tm1)
{
	for (rules_t::iterator ri(m_rules.begin()), re(m_rules.end()); ri != re;) {
		FlightRestriction::ptr_t p(*ri);
		if (!p) {
			ri = m_rules.erase(ri);
			re = m_rules.end();
			continue;
		}
		{
			FlightRestriction::ptr_t p1(FlightRestriction::ptr_t::cast_dynamic(p->simplify_time(tm0, tm1)));
			if (p1)
				p.swap(p1);
		}
		if (!p->is_keep()) {
			ri = m_rules.erase(ri);
			re = m_rules.end();
			continue;
		}
		*ri = p;
		++ri;
	}
}

void RestrictionEval::simplify_rules_bbox(const Rect& bbox)
{
	for (rules_t::iterator ri(m_rules.begin()), re(m_rules.end()); ri != re;) {
		FlightRestriction::ptr_t p(*ri);
		if (!p) {
			ri = m_rules.erase(ri);
			re = m_rules.end();
			continue;
		}
		{
			FlightRestriction::ptr_t p1(p->simplify_bbox(bbox));
			if (p1)
				p.swap(p1);
		}
		if (!p->is_keep()) {
			ri = m_rules.erase(ri);
			re = m_rules.end();
			continue;
		}
		*ri = p;
		++ri;
	}
}

void RestrictionEval::simplify_rules_altrange(int32_t minalt, int32_t maxalt)
{
	for (rules_t::iterator ri(m_rules.begin()), re(m_rules.end()); ri != re;) {
		FlightRestriction::ptr_t p(*ri);
		if (!p) {
			ri = m_rules.erase(ri);
			re = m_rules.end();
			continue;
		}
		{
			FlightRestriction::ptr_t p1(p->simplify_altrange(minalt, maxalt));
			if (p1)
				p.swap(p1);
		}
		if (!p->is_keep()) {
			ri = m_rules.erase(ri);
			re = m_rules.end();
			continue;
		}
		*ri = p;
		++ri;
	}
}

void RestrictionEval::simplify_rules_aircrafttype(const std::string& acfttype)
{
	for (rules_t::iterator ri(m_rules.begin()), re(m_rules.end()); ri != re;) {
		FlightRestriction::ptr_t p(*ri);
		if (!p) {
			ri = m_rules.erase(ri);
			re = m_rules.end();
			continue;
		}
		{
			FlightRestriction::ptr_t p1(p->simplify_aircrafttype(acfttype));
			if (p1)
				p.swap(p1);
		}
		if (!p->is_keep()) {
			ri = m_rules.erase(ri);
			re = m_rules.end();
			continue;
		}
		*ri = p;
		++ri;
	}
}

void RestrictionEval::simplify_rules_aircraftclass(const std::string& acftclass)
{
	for (rules_t::iterator ri(m_rules.begin()), re(m_rules.end()); ri != re;) {
		FlightRestriction::ptr_t p(*ri);
		if (!p) {
			ri = m_rules.erase(ri);
			re = m_rules.end();
			continue;
		}
		{
			FlightRestriction::ptr_t p1(p->simplify_aircraftclass(acftclass));
			if (p1)
				p.swap(p1);
		}
		if (!p->is_keep()) {
			ri = m_rules.erase(ri);
			re = m_rules.end();
			continue;
		}
		*ri = p;
		++ri;
	}
}

void RestrictionEval::simplify_rules_equipment(const std::string& equipment, Aircraft::pbn_t pbn)
{
	for (rules_t::iterator ri(m_rules.begin()), re(m_rules.end()); ri != re;) {
		FlightRestriction::ptr_t p(*ri);
		if (!p) {
			ri = m_rules.erase(ri);
			re = m_rules.end();
			continue;
		}
		{
			FlightRestriction::ptr_t p1(p->simplify_equipment(equipment, pbn));
			if (p1)
				p.swap(p1);
		}
		if (!p->is_keep()) {
			ri = m_rules.erase(ri);
			re = m_rules.end();
			continue;
		}
		*ri = p;
		++ri;
	}
}

void RestrictionEval::simplify_rules_typeofflight(char type_of_flight)
{
	for (rules_t::iterator ri(m_rules.begin()), re(m_rules.end()); ri != re;) {
		FlightRestriction::ptr_t p(*ri);
		if (!p) {
			ri = m_rules.erase(ri);
			re = m_rules.end();
			continue;
		}
		{
			FlightRestriction::ptr_t p1(p->simplify_typeofflight(type_of_flight));
			if (p1)
				p.swap(p1);
		}
		if (!p->is_keep()) {
			ri = m_rules.erase(ri);
			re = m_rules.end();
			continue;
		}
		*ri = p;
		++ri;
	}
}

void RestrictionEval::simplify_rules_mil(bool mil)
{
	for (rules_t::iterator ri(m_rules.begin()), re(m_rules.end()); ri != re;) {
		FlightRestriction::ptr_t p(*ri);
		if (!p) {
			ri = m_rules.erase(ri);
			re = m_rules.end();
			continue;
		}
		{
			FlightRestriction::ptr_t p1(p->simplify_mil(mil));
			if (p1)
				p.swap(p1);
		}
		if (!p->is_keep()) {
			ri = m_rules.erase(ri);
			re = m_rules.end();
			continue;
		}
		*ri = p;
		++ri;
	}
}

void RestrictionEval::simplify_rules_dep(const Link& arpt)
{
	for (rules_t::iterator ri(m_rules.begin()), re(m_rules.end()); ri != re;) {
		FlightRestriction::ptr_t p(*ri);
		if (!p) {
			ri = m_rules.erase(ri);
			re = m_rules.end();
			continue;
		}
		{
			FlightRestriction::ptr_t p1(p->simplify_dep(arpt));
			if (p1)
				p.swap(p1);
		}
		if (!p->is_keep()) {
			ri = m_rules.erase(ri);
			re = m_rules.end();
			continue;
		}
		*ri = p;
		++ri;
	}
}

void RestrictionEval::simplify_rules_dest(const Link& arpt)
{
	for (rules_t::iterator ri(m_rules.begin()), re(m_rules.end()); ri != re;) {
		FlightRestriction::ptr_t p(*ri);
		if (!p) {
			ri = m_rules.erase(ri);
			re = m_rules.end();
			continue;
		}
		{
			FlightRestriction::ptr_t p1(p->simplify_dest(arpt));
			if (p1)
				p.swap(p1);
		}
		if (!p->is_keep()) {
			ri = m_rules.erase(ri);
			re = m_rules.end();
			continue;
		}
		*ri = p;
		++ri;
	}
}

void RestrictionEval::simplify_conditionalavailability(const Graph& g, const timepair_t& tm)
{
	for (rules_t::iterator ri(m_rules.begin()), re(m_rules.end()); ri != re;) {
		FlightRestriction::ptr_t p(*ri);
		if (!p) {
			ri = m_rules.erase(ri);
			re = m_rules.end();
			continue;
		}
		{
			FlightRestriction::ptr_t p1(p->simplify_conditionalavailability(g, get_condavail(), get_specialdateeval(), tm));
			if (p1)
				p.swap(p1);
		}
		if (!p->is_keep()) {
			ri = m_rules.erase(ri);
			re = m_rules.end();
			continue;
		}
		*ri = p;
		++ri;
	}
}

bool RestrictionEval::check_fplan(bool honourprofilerules)
{
	Graph& graph(get_graphref());
	clear_messages();
	clear_results();
	m_currule.clear();
	// check object integrity of the flight plan
	if (m_waypoints.size() < 2) {
		Message m("Degenerate flight plan", Message::type_error);
		message(m);
		return false;
	}
	bool ok(true);
	// check point objects
	for (FlightPlan::size_type i(0), n(m_waypoints.size()); i < n; ++i) {
		Waypoint& wpt(m_waypoints[i]);
		{
			const Object::const_ptr_t& p(wpt.get_ptobj());
			if (p) {
				std::pair<Graph::vertex_descriptor,bool> v(graph.find_vertex(p->get_uuid()));
				if (!v.second) {
					Message m("point " + wpt.get_icao_or_name() + " not found in graph", Message::type_error);
					m.m_vertexset.insert(i);
					message(m);
					ok = false;
					continue;
				}
				wpt.set_vertex(v.first);
				const GraphVertex& vv(graph[v.first]);
				wpt.set_ptobj(vv.get_object());
				m_currule = Object::ptr_t::cast_const(wpt.get_ptobj());
				const PointIdentTimeSlice& ts(wpt.get_ptobj()->operator()(get_departuretime()).as_point());
				if (!ts.is_valid()) {
					Message m("invalid point object", Message::type_error);
					m.m_vertexset.insert(i);
					message(m);
					ok = false;
					continue;
				}
				if (ts.get_coord() != wpt.get_coord()) {
					Message m("point coordinate mismatch: " +
						  wpt.get_coord().get_lat_str2() + " " +
						  wpt.get_coord().get_lon_str2() + " != " +
						  ts.get_coord().get_lat_str2() + " " +
						  ts.get_coord().get_lon_str2(), Message::type_error);
					m.m_vertexset.insert(i);
					message(m);
					ok = false;
				}
				if (ts.get_ident() != wpt.get_icao_or_name()) {
					Message m("ident mismatch: " + wpt.get_icao_or_name() + " != " +
						  ts.get_ident(), Message::type_error);
					m.m_vertexset.insert(i);
					message(m);
					ok = false;
				}
				continue;
			}
			if (wpt.is_ifr() || (i && m_waypoints[i-1].is_ifr())) {
				Message m("IFR leg but unknown points", Message::type_error);
				m.m_vertexset.insert(i);
				message(m);
				ok = false;
			}
		}
	}
	// check path code and path objects
	if (m_waypoints.back().get_pathcode() != FPlanWaypoint::pathcode_none) {
		Message m("Invalid Destination Pathcode", Message::type_error);
		m.m_edgeset.insert(m_waypoints.size() - 1);
		message(m);
		ok = false;
	}
	if (!ok)
		return false;
	for (FlightPlan::size_type i(0), n(m_waypoints.size() - 1); i < n; ++i) {
		Waypoint& wpt(m_waypoints[i]);
		Waypoint& wptn(m_waypoints[i + 1]);
		if (!wpt.is_ifr()) {
			switch (wpt.get_pathcode()) {
			case FPlanWaypoint::pathcode_sid:
			case FPlanWaypoint::pathcode_star:
			case FPlanWaypoint::pathcode_airway:
			case FPlanWaypoint::pathcode_directto:
				wpt.set_pathcode(FPlanWaypoint::pathcode_none);
				break;

			default:
				break;
			}
			wpt.set_pathobj(Object::ptr_t());
			continue;
		}
		switch (wpt.get_pathcode()) {
		case FPlanWaypoint::pathcode_airway:
			if (wpt.is_stay()) {
				wpt.set_pathobj(Object::ptr_t());
				break;
			}
			// fall through

		case FPlanWaypoint::pathcode_sid:
		case FPlanWaypoint::pathcode_star:
		{
			const Object::const_ptr_t& p(wpt.get_pathobj());
			if (!p) {
				Message m("IFR leg but unknown path", Message::type_error);
				m.m_edgeset.insert(i);
				message(m);
				ok = false;
				continue;
			}
			std::pair<Graph::edge_descriptor,bool> e(graph.find_edge(wpt.get_vertex(), wptn.get_vertex(),
										 p->get_uuid()));
			if (!e.second) {
				Message m("edge not found in graph", Message::type_error);
				m.m_edgeset.insert(i);
				message(m);
				ok = false;
				continue;
			}
			wpt.set_edge(e.first);
			const GraphEdge& ee(graph[e.first]);
			if (wpt.get_pathcode() != ee.get_pathcode()) {
				Message m("pathcode " + FPlanWaypoint::get_pathcode_name(wpt.get_pathcode()) +
					  " differs from path object pathcode " + FPlanWaypoint::get_pathcode_name(ee.get_pathcode()),
					  Message::type_error);
				m.m_edgeset.insert(i);
				message(m);
				ok = false;
				continue;
			}
			int32_t alt0(std::min(wpt.get_altitude(), wptn.get_altitude()));
			int32_t alt1(std::max(wpt.get_altitude(), wptn.get_altitude()));
			if (!i)
				alt0 = alt1 = wptn.get_altitude();
			else if (i + 1 >= n)
				alt1 = alt0 = wpt.get_altitude();
			if (ee.is_inside(alt0, alt1))
				break;
			if (ee.is_sid() || ee.is_star()) {
				if (ee.is_inside(alt0, alt0) || ee.is_inside(alt1, alt1))
					break;
			} else if (ee.is_inside(wpt.get_altitude(), wpt.get_altitude())) {
				// check for climbs/descents accross airway names
				IntervalSet<int32_t> ar;
				Graph::out_edge_iterator e0, e1;
				for (boost::tie(e0, e1) = boost::out_edges(wpt.get_vertex(), graph); e0 != e1; ++e0) {
					if (boost::target(*e0, graph) != wptn.get_vertex())
						continue;
					const GraphEdge& e(graph[*e0]);
					if (!e.is_awy())
						continue;
					ar |= e.get_altrange();
				}
				IntervalSet<int32_t> ar1(IntervalSet<int32_t>::Intvl(alt0, alt1 + 1));
				ar1 &= ~ar;
				if (ar1.is_empty())
					break;
			}
			std::ostringstream oss;
			oss << "leg altitude outside route segment altitude (F"
			    << std::setfill('0') << std::setw(3) << ((alt0 + 50) / 100) << "...F"
			    << std::setfill('0') << std::setw(3) << ((alt1 + 50) / 100);
			if (!ee.get_uuid().is_nil())
				oss << " edge " << ee.get_uuid();
			oss << ')';
			Message m(oss.str(), Message::type_error);
			m.m_edgeset.insert(i);
			message(m);
			ok = false;
			break;
		}

		case FPlanWaypoint::pathcode_directto:
			wpt.set_pathobj(Object::ptr_t());
			break;

		case FPlanWaypoint::pathcode_none:
			// FIXME treat as error for now
			// fall through
		default:
		{
			Message m("Invalid Pathcode", Message::type_error);
			m.m_edgeset.insert(i);
			message(m);
			ok = false;
			break;
		}
		}
	}
	if (!ok)
		return false;
	for (FlightPlan::size_type i(0), n(m_waypoints.size() - 1); i < n; ++i) {
		Waypoint& wpt(m_waypoints[i]);
		Waypoint& wptn(m_waypoints[i + 1]);
		if (wpt.get_pathcode() == FPlanWaypoint::pathcode_sid && i &&
		    m_waypoints[i - 1].get_pathcode() != FPlanWaypoint::pathcode_sid) {
			Message m("SID Segment within Route", Message::type_error);
			m.m_edgeset.insert(i);
			message(m);
			ok = false;
			continue;
		}
		if (wpt.get_pathcode() == FPlanWaypoint::pathcode_star && i + 1 < n &&
		    wptn.get_pathcode() != FPlanWaypoint::pathcode_star) {
			Message m("STAR Segment within Route", Message::type_error);
			m.m_edgeset.insert(i);
			message(m);
			ok = false;
			continue;
		}
		// check SID connection point
		if (wpt.get_pathcode() == FPlanWaypoint::pathcode_sid &&
		    wptn.get_pathcode() != FPlanWaypoint::pathcode_sid) {
			const GraphEdge& ee(graph[wpt.get_edge()]);
			const StandardInstrumentTimeSlice& ts(ee.get_route_timeslice().as_sidstar());
			const StandardInstrumentTimeSlice::connpoints_t& cp(ts.get_connpoints());
			if (!ts.is_valid() || cp.find(wptn.get_ptobj()->get_uuid()) == cp.end()) {
				std::ostringstream oss;
				oss << "invalid SID connection point " << wptn.get_icao_or_name()
				    << " for " << ts.get_ident() << "; valid points are";
				for (StandardInstrumentTimeSlice::connpoints_t::const_iterator ci(cp.begin()), ce(cp.end()); ci != ce; ++ci)
					oss << ' ' << ci->get_obj()->operator()(get_departuretime()).as_ident().get_ident();
				Message m(oss.str(), Message::type_error);
				m.m_vertexset.insert(i + 1);
				message(m);
				ok = false;
			}
		}
		// check STAR connection point
		if (wpt.get_pathcode() != FPlanWaypoint::pathcode_star &&
		    wptn.get_pathcode() == FPlanWaypoint::pathcode_star) {
			const GraphEdge& ee(graph[wptn.get_edge()]);
			const StandardInstrumentTimeSlice& ts(ee.get_route_timeslice().as_sidstar());
			const StandardInstrumentTimeSlice::connpoints_t& cp(ts.get_connpoints());
			if (!ts.is_valid() || cp.find(wptn.get_ptobj()->get_uuid()) == cp.end()) {
				std::ostringstream oss;
				oss << "invalid STAR connection point " << wptn.get_icao_or_name()
				    << " for " << ts.get_ident() << "; valid points are";
				for (StandardInstrumentTimeSlice::connpoints_t::const_iterator ci(cp.begin()), ce(cp.end()); ci != ce; ++ci)
					oss << ' ' << ci->get_obj()->operator()(get_departuretime()).as_ident().get_ident();
				Message m(oss.str(), Message::type_error);
				m.m_vertexset.insert(i + 1);
				message(m);
				ok = false;
			}
		}
	}
	// check rules
	bool trace(false);
	RestrictionResult::set_t allowededges;
	for (rules_t::const_iterator ri(m_rules.begin()), re(m_rules.end()); ri != re; ++ri) {
		m_currule = FlightRestriction::ptr_t::cast_const(*ri);
		const FlightRestrictionTimeSlice& ts(m_currule->operator()(get_departuretime()).as_flightrestriction());
		if (!ts.is_valid()) {
			m_currule.clear();
			continue;
		}
		if (ts.get_procind() == FlightRestrictionTimeSlice::procind_fpr && !honourprofilerules) {
			m_currule.clear();
			continue;
		}
		trace = trace || ts.is_trace();
		RestrictionResult result;
		if (!ts.evaluate(*this, result))
			m_results.push_back(result);
	}
	m_currule.clear();
	if (!m_results.empty())
		ok = false;
	return ok;
}

DctSegments::DctSegment::DctSegment(const Object::const_ptr_t& pt0, const Object::const_ptr_t& pt1)
{
	m_pt[0] = pt0;
	m_pt[1] = pt1;
	if (get_uuid(1) < get_uuid(0))
		std::swap(m_pt[0], m_pt[1]);
}

const UUID& DctSegments::DctSegment::get_uuid(unsigned int x) const
{
	const Object::const_ptr_t& p(operator[](x));
	if (!p)
		return UUID::niluuid;
	return p->get_uuid();
}

int DctSegments::DctSegment::compare(const DctSegment& x) const
{
	int c(get_uuid(0).compare(x.get_uuid(0)));
	if (c)
		return c;
	return get_uuid(1).compare(x.get_uuid(1));
}

std::pair<double,double> DctSegments::DctSegment::dist(void) const
{
	std::pair<double,double> r(std::numeric_limits<double>::max(), std::numeric_limits<double>::min());
	for (unsigned int i0(0), n0(operator[](0)->size()); i0 < n0; ++i0) {
		const PointIdentTimeSlice& ts0(operator[](0)->operator[](i0).as_point());
		if (!ts0.is_valid())
			continue;
		if (ts0.get_coord().is_invalid())
			continue;
		for (unsigned int i1(0), n1(operator[](1)->size()); i1 < n1; ++i1) {
			const PointIdentTimeSlice& ts1(operator[](1)->operator[](i1).as_point());
			if (!ts1.is_valid())
				continue;
			if (!ts0.is_overlap(ts1))
				continue;
			if (ts1.get_coord().is_invalid())
				continue;
			double dist(ts0.get_coord().spheric_distance_nmi_dbl(ts1.get_coord()));
			if (std::isnan(dist))
				continue;
			r.first = std::min(r.first, dist);
			r.second = std::max(r.second, dist);
		}
	}
	return r;
}

DctParameters::AirportDctLimit::AirportDctLimit(const Link& arpt, timetype_t starttime, timetype_t endtime, double lim, const dctconnpoints_t& connpt)
	: m_connpt(connpt), m_arpt(arpt), m_time(starttime, endtime), m_limit(lim)
{
}

int DctParameters::AirportDctLimit::compare(const AirportDctLimit& x) const
{
	int c(get_arpt().compare(x.get_arpt()));
	if (c)
		return c;
	if (get_starttime() < x.get_starttime())
		return true;
	if (x.get_starttime() < get_starttime())
		return false;
	return get_endtime() < x.get_endtime();
}

bool DctParameters::AirportDctLimit::is_overlap(timetype_t tm0, timetype_t tm1) const
{
	if (!is_valid() || !(tm0 < tm1))
		return false;
	return tm1 > get_starttime() && tm0 < get_endtime();
}

IntervalSet<int32_t> DctParameters::AirportDctLimit::get_altinterval(const Link& pt, timetype_t tm) const
{
	if (!is_inside(tm))
		return IntervalSet<int32_t>();
	{
		dctconnpoints_t::const_iterator i(get_connpt().find(pt));
		if (i != get_connpt().end())
			return i->second;
	}
	const AirportTimeSlice& tsa(get_arpt().get_obj()->operator()(tm).as_airport());
	if (!tsa.is_valid())
		return IntervalSet<int32_t>();
	if (tsa.get_coord().is_invalid())
		return IntervalSet<int32_t>();
	const PointIdentTimeSlice& tsp(pt.get_obj()->operator()(tm).as_point());
	if (!tsp.is_valid())
		return IntervalSet<int32_t>();
	if (tsp.get_coord().is_invalid())
		return IntervalSet<int32_t>();
	double dist(tsa.get_coord().spheric_distance_nmi_dbl(tsp.get_coord()));
	if (dist > get_limit())
		return IntervalSet<int32_t>();
	return IntervalSet<int32_t>(IntervalSet<int32_t>::Intvl(0, std::numeric_limits<int32_t>::max()));
}

DctParameters::DctParameters(Database *db, TopoDb30 *topodb, const std::string& topodbpath, timetype_t tmmodif, timetype_t tmcutoff,
			     timetype_t tmfuturecutoff, double maxdist, unsigned int worker, bool all, bool verbose)
	: RestrictionEvalBase(db), m_topodb(topodb), m_topodbpath(topodbpath), m_tmmodified(tmmodif),
	  m_tmcutoff(tmcutoff), m_tmfuturecutoff(tmfuturecutoff), m_maxdist(maxdist), m_errorcnt(0), m_warncnt(0),
	  m_ppcnt(0), m_pptot(0), m_dctcnt(0), m_worker(worker), m_resultscnt(0), m_finished(0),
	  m_all(all), m_verbose(verbose), m_trace(false)
{
	// FIXME: crude approximation of the ECAC region
	{
		PolygonSimple ps;
		{
			Point p;
			p.set_lat_deg_dbl(17);
			p.set_lon_deg_dbl(-40);
			ps.push_back(p);
		}
		{
			Point p;
			p.set_lat_deg_dbl(82);
			p.set_lon_deg_dbl(-40);
			ps.push_back(p);
		}
		{
			Point p;
			p.set_lat_deg_dbl(82);
			p.set_lon_deg_dbl(48);
			ps.push_back(p);
		}
		{
			Point p;
			p.set_lat_deg_dbl(17);
			p.set_lon_deg_dbl(48);
			ps.push_back(p);
		}
		m_ecacpoly.push_back(PolygonHole(ps));
	}
	m_ecacbbox = m_ecacpoly.get_bbox();
	if (get_db() && !get_db()->count_dct())
		m_all = true;
}

void DctParameters::load_rules(void)
{
	RestrictionEvalBase::load_rules();
	m_seg.clear();
	m_ruletimedisc.clear();
	m_ruletimedisc.insert((timetype_t)0);
	m_ruletimedisc.insert(std::numeric_limits<timetype_t>::max());
	m_graph.clear();
	m_routetimedisc.clear();
	Rect bbox(Point::invalid, Point::invalid);
	for (rules_t::iterator i(m_allrules.begin()), e(m_allrules.end()); i != e; ) {
		FlightRestriction::ptr_t p(*i);
		if (!p) {
			i = m_allrules.erase(i);
			e = m_allrules.end();
			continue;
		}
		if (false) {
			const IdentTimeSlice& ts(p->operator[](0).as_ident());
			std::cerr << "R:" << ts.get_ident() << ": processing" << std::endl;
		}
		for (unsigned int j(0), n(p->size()); j < n; ++j) {
			const FlightRestrictionTimeSlice& ts(p->operator[](j).as_flightrestriction());
			if (!ts.is_valid())
				continue;
			if (ts.get_starttime() >= m_tmfuturecutoff || ts.get_endtime() <= m_tmcutoff)
				continue;
			Link arpt;
			bool arr;
			double dist;
			FlightRestrictionTimeSlice::dctconnpoints_t connpt;
			Condition::civmil_t civmil;
			if (!ts.is_deparr_dct(arpt, arr, dist, connpt, civmil))
				continue;
			if (civmil == Condition::civmil_mil)
				continue;
			if (std::isnan(dist))
				dist = 0;
			if (dist <= 0 && connpt.empty())
				continue;
			std::pair<airportdctlimit_t::iterator,bool> ins;
			if (arr)
				ins = m_starlimit.insert(AirportDctLimit(arpt, ts.get_starttime(), ts.get_endtime(), dist, connpt));
			else
				ins = m_sidlimit.insert(AirportDctLimit(arpt, ts.get_starttime(), ts.get_endtime(), dist, connpt));
			
			if (ins.second)
				continue;
			std::cerr << "Warning: R:" << ts.get_ident() << " failed to insert airport DCT limit" << std::endl;
			++m_warncnt;
		}
		{
			FlightRestriction::ptr_t p1(p->simplify());
			if (p1) {
				if (false) {
					const IdentTimeSlice& ts(p1->operator[](0).as_ident());
					std::cerr << "R:" << ts.get_ident() << ": simplified" << std::endl;
				}
				p = p1;
			}
		}
		{
			FlightRestriction::ptr_t p1(p->simplify_mil(false));
			if (p1) {
				if (false) {
					const IdentTimeSlice& ts(p1->operator[](0).as_ident());
					std::cerr << "R:" << ts.get_ident() << ": simplified mil" << std::endl;
				}
				p = p1;
			}
		}
		*i = p;
		bool dct(false);
		Rect bbox1(Point::invalid, Point::invalid);
		for (unsigned int j(0), n(p->size()); j < n; ++j) {
			const FlightRestrictionTimeSlice& ts(p->operator[](j).as_flightrestriction());
			if (!ts.is_valid())
				continue;
			if (ts.get_starttime() >= m_tmfuturecutoff || ts.get_endtime() <= m_tmcutoff)
				continue;
			ts.get_dct_segments(m_seg);
			{
				bool dct(ts.is_dct());
				switch (ts.get_procind()) {
				case FlightRestrictionTimeSlice::procind_raddct:
				case FlightRestrictionTimeSlice::procind_fradct:
					if (!dct) {
						std::cerr << "Warning: R:" << ts.get_ident() << " is " << to_str(ts.get_procind())
							  << " but not considered DCT" << std::endl;
						++m_warncnt;
					}
					break;

				default:
					if (dct) {
						std::cerr << "Warning: R:" << ts.get_ident() << " is " << to_str(ts.get_procind())
							  << " but considered DCT" << std::endl;
						++m_warncnt;
					}
					break;
				}
				if (!dct)
					continue;
			}
			dct = true;
			{
				timeset_t tdisc(ts.timediscontinuities());
				m_ruletimedisc.insert(tdisc.begin(), tdisc.end());
				if (false) {
					std::cerr << "R:" << ts.get_ident() << " timeslice ";
					if (ts.get_starttime() == std::numeric_limits<timetype_t>::max())
						std::cerr << "unlimited";
					else
						std::cerr << Glib::TimeVal(ts.get_starttime(), 0).as_iso8601();
					std::cerr << " - ";
					if (ts.get_endtime() == std::numeric_limits<timetype_t>::max())
						std::cerr << "unlimited";
					else
						std::cerr << Glib::TimeVal(ts.get_endtime(), 0).as_iso8601();
					std::cerr << " tdisc";
					for (timeset_t::const_iterator i(tdisc.begin()), e(tdisc.end()); i != e; ++i)
						if (*i == std::numeric_limits<timetype_t>::max())
							std::cerr << " unlimited";
						else
							std::cerr << ' ' << Glib::TimeVal(*i, 0).as_iso8601();
					std::cerr << std::endl;
				}
			}
			{
				Link::LinkSet dep;
				ts.dependencies(dep);
				for (Link::LinkSet::const_iterator di(dep.begin()), de(dep.end()); di != de; ++di) {
					if (!di->get_obj())
						continue;
					timeset_t tdisc(di->get_obj()->timediscontinuities());
					m_ruletimedisc.insert(tdisc.begin(), tdisc.end());
				}
			}
			{
				Link aspc;
				AltRange alt;
				double dist;
				Condition::civmil_t civmil;
				if (ts.is_enroute_dct(aspc, alt, dist, civmil) && civmil != Condition::civmil_mil) {
					if (bbox1.get_southwest().is_invalid() || bbox1.get_northeast().is_invalid()) {
						ts.get_bbox(bbox1);
					} else {
						Rect bbox2;
						ts.get_bbox(bbox2);
						bbox1 = bbox1.add(bbox2);
					}					
				}
			}
		}
		if (!dct) {
			if (false) {
				const IdentTimeSlice& ts(p->operator[](0).as_ident());
				std::cerr << "R:" << ts.get_ident() << ": not DCT, skipping" << std::endl;
			}
			i = m_allrules.erase(i);
			e = m_allrules.end();
			continue;
		}
		if (!bbox1.get_southwest().is_invalid() && !bbox1.get_northeast().is_invalid()) {
			if (bbox.get_southwest().is_invalid() || bbox.get_northeast().is_invalid()) {
				bbox = bbox1;
			} else {
				bbox = bbox.add(bbox1);
			}
		}
		++i;
	}
	if (true) {
		typedef std::set<std::string> set_t;
		set_t names;
		for (rules_t::iterator i(m_allrules.begin()), e(m_allrules.end()); i != e; ++i) {
			Object::ptr_t p(*i);
			for (unsigned int j(0), n(p->size()); j < n; ++j) {
				const FlightRestrictionTimeSlice& ts(p->operator[](j).as_flightrestriction());
				if (!ts.is_valid())
					continue;
				if (ts.get_starttime() >= m_tmfuturecutoff || ts.get_endtime() <= m_tmcutoff)
					continue;
				if (ts.get_ident().empty())
					continue;
				names.insert(ts.get_ident());
			}
		}
		std::cout << "Rules:";
		for (set_t::const_iterator i(names.begin()), e(names.end()); i != e; ++i)
			std::cout << ' ' << *i;
		std::cout << std::endl;
	}
	if (true)
		std::cout << "DCT bounding box: " << bbox.get_southwest().get_lat_str2() << ' '
			  << bbox.get_southwest().get_lon_str2() << ' '
			  << bbox.get_northeast().get_lat_str2() << ' '
			  << bbox.get_northeast().get_lon_str2() << std::endl;
	Database& db(get_dbref());
	Database::findresults_t r;
	r = db.find_by_bbox(bbox, Database::loadmode_link, 0, std::numeric_limits<uint64_t>::max(),
			    Object::type_routesegment, Object::type_routesegment, 0);
	for (Database::findresults_t::iterator i(r.begin()), e(r.end()); i != e; ++i) {
		Object::ptr_t p(i->get_obj());
		if (!p)
			continue;
		{
			Rect bbox;
			p->get_bbox(bbox);
			if (!bbox.is_intersect(m_ecacbbox))
				continue;
		}
		m_graph.add(p);
		{
			timeset_t tdisc(p->timediscontinuities());
			m_routetimedisc.insert(tdisc.begin(), tdisc.end());
		}
		{
			Link::LinkSet dep;
			p->dependencies(dep);
			for (Link::LinkSet::const_iterator di(dep.begin()), de(dep.end()); di != de; ++di) {
				if (!di->get_obj())
					continue;
				timeset_t tdisc(di->get_obj()->timediscontinuities());
				m_routetimedisc.insert(tdisc.begin(), tdisc.end());
			}
		}
	}
}

void DctParameters::load_points(void)
{
	Database& db(get_dbref());
	Database::findresults_t r(db.find_all(Database::loadmode_link, 0, std::numeric_limits<uint64_t>::max(),
					      Object::type_navaid, Object::type_designatedpoint, 0));
	{
		Link::LinkSet arpts;
		for (airportdctlimit_t::const_iterator i(m_sidlimit.begin()), e(m_sidlimit.end()); i != e; ++i)
			arpts.insert(i->get_arpt());
		for (airportdctlimit_t::const_iterator i(m_starlimit.begin()), e(m_starlimit.end()); i != e; ++i)
			arpts.insert(i->get_arpt());
		for (Link::LinkSet::const_iterator i(arpts.begin()), e(arpts.end()); i != e; ++i)
			r.push_back(*i);
	}
	load_points(r, true);
}

void DctParameters::load_points(const Database::findresults_t& r, bool alldct)
{
	Database& db(get_dbref());
	m_points.clear();
	m_dctradius.clear();
	m_trace = !alldct;
	typedef std::map<Object::ptr_t,double,Graph::UUIDSort> pts_t;
	pts_t pts;
	for (Database::findresults_t::const_iterator i(r.begin()), e(r.end()); i != e; ++i) {
		Link l(*i);
		l.load(db);
		Object::ptr_t p(l.get_obj());
		if (!p)
			continue;
		if (p->get_type() >= Object::type_airport && p->get_type() <= Object::type_airportend) {
			AirportDctLimit limlo(l, std::numeric_limits<timetype_t>::min(), std::numeric_limits<timetype_t>::min());
			AirportDctLimit limhi(l, std::numeric_limits<timetype_t>::max(), std::numeric_limits<timetype_t>::max());
			airportdctlimit_t::const_iterator i(m_sidlimit.lower_bound(limlo)), e(m_sidlimit.upper_bound(limhi));
			for (; i != e; ++i)
				if (i->get_arpt() == p->get_uuid())
					break;
			if (i == e) {
				i = m_starlimit.lower_bound(limlo);
				e = m_starlimit.upper_bound(limhi);
				for (; i != e; ++i)
					if (i->get_arpt() == p->get_uuid())
						break;
				if (i == e)
					continue;
			}
		} else if (p->get_type() != Object::type_navaid && p->get_type() != Object::type_designatedpoint) {
			continue;
		}
		bool ok(false);
		bool covered(false);
		double dist1(0);
		for (rules_t::const_iterator ri(m_allrules.begin()), re(m_allrules.end()); ri != re; ++ri) {
			for (unsigned int i(0), n((*ri)->size()); i < n; ++i) {
				const FlightRestrictionTimeSlice& ts((*ri)->operator[](i).as_flightrestriction());
				if (!ts.is_valid() || !ts.is_enabled())
					continue;
				Link aspc;
				AltRange alt;
				double dist;
				Condition::civmil_t civmil;
				if (!ts.is_enroute_dct(aspc, alt, dist, civmil))
					continue;
				// no distance limit - assume some maximum
				if (std::isnan(dist))
					dist = m_maxdist;
				else if (dist > m_maxdist)
					dist = m_maxdist;
				bool ok1(false);
				for (unsigned int i(0), n(p->size()); i < n; ++i) {
					const PointIdentTimeSlice& tsp(p->operator[](i).as_point());
					if (!tsp.is_valid())
						continue;
					for (unsigned int i(0), n(aspc.get_obj()->size()); i < n; ++i) {
						const AirspaceTimeSlice& tsa(aspc.get_obj()->operator[](i).as_airspace());
						if (!tsa.is_valid() || !tsa.is_overlap(tsp))
							continue;
						bool inside(tsa.is_inside(tsp.get_coord(), AltRange::altignore, alt, p->get_uuid()));
						if (false) {
							std::cerr << ts.get_ident() << " (" << tsa.get_ident() << ",D" << dist << "): "
								  << tsp.get_ident() << ' ' << tsp.get_coord().get_lat_str2() << ' '
								  << tsp.get_coord().get_lon_str2() << (inside ? "" : " NOT") << " inside" << std::endl;
						}
						if (!inside) {
							covered = covered || tsa.is_inside(tsp.get_coord(), AltRange::altignore, alt, UUID::niluuid);
							continue;
						}
						ok1 = true;
						if (m_trace) {
							std::cout << ts.get_ident() << " (" << tsa.get_ident() << ",D" << dist << "): "
								  << tsp.get_ident() << ' ' << tsp.get_coord().get_lat_str2() << ' '
								  << tsp.get_coord().get_lon_str2() << std::endl;
						}
						break;
					}
					if (ok1)
						break;
				}
				if (!ok1)
					continue;
				dist1 = std::max(dist1, dist);
				ok = true;
			}
		}
		if (!ok) {
			if (covered)
				dist1 = m_maxdist;
			else
				continue;
		}
		pts.insert(pts_t::value_type(p, dist1));
	}
	for (rules_t::const_iterator ri(m_allrules.begin()), re(m_allrules.end()); ri != re; ++ri) {
		for (unsigned int i(0), n((*ri)->size()); i < n; ++i) {
			const FlightRestrictionTimeSlice& ts((*ri)->operator[](i).as_flightrestriction());
			if (!ts.is_valid() || !ts.is_enabled())
				continue;
			if (ts.get_starttime() >= m_tmfuturecutoff || ts.get_endtime() <= m_tmcutoff)
				continue;
			DctSegments segs;
			ts.get_dct_segments(segs);
			for (DctSegments::const_iterator si(segs.begin()), se(segs.end()); si != se; ++si) {
				const DctSegments::DctSegment& seg(*si);
				std::pair<double,double> dist(seg.dist());
				if (dist.first > dist.second || std::isnan(dist.second))
					continue;
				if (alldct || pts.find(Object::ptr_t::cast_const(seg[0])) != pts.end()) {
					std::pair<pts_t::iterator,bool> ins(pts.insert(pts_t::value_type(Object::ptr_t::cast_const(seg[0]), dist.second)));
					if (!ins.second && (std::isnan(ins.first->second) || ins.first->second < dist.second))
						ins.first->second = dist.second;
				}
				if (alldct || pts.find(Object::ptr_t::cast_const(seg[1])) != pts.end()) {
					std::pair<pts_t::iterator,bool> ins(pts.insert(pts_t::value_type(Object::ptr_t::cast_const(seg[1]), dist.second)));
					if (!ins.second && (std::isnan(ins.first->second) || ins.first->second < dist.second))
						ins.first->second = dist.second;
				}
			}
		}
	}
	for (pts_t::const_iterator pi(pts.begin()), pe(pts.end()); pi != pe; ++pi) {
		m_points.push_back(pi->first);
		m_dctradius.push_back(pi->second);
	}
}

/*

FIXME:
inst/bin/adrimport -d . --dct-all --dct-verbose --dct -P 739a5827-8365-499c-9bee-3d478c9aea32 -P 771f26bc-e3db-4c89-88ba-99b4b3fc6e8a -P 7d49940b-1be2-46ee-9a32-f472602844fb 2>&1

should generate SHM-BOGNA and BOGNA-SITET

DONE: -(FPL-HBPBX-IG -1P28R/L -SDFGLOR/S -EGKA1005 -N0135F070 SHM/N0138F090 DCT BOGNA DCT SITET A34 ETRAT A34 DVL A34 LISEU A34 LGL A34 BOBSA -LFPN0134 -PBN/B2 DOF/140123 ) GC 156.2 route 205.4 fuel 17.0

FIXME:
DCT
  0f0c6359-c0b7-4f50-b1a5-028e1434d9d5 PIVED
  f8c30b35-96f6-4190-8ec4-c0ac7864eb8b

DCT AGIXA-ATMAT dist 181.33nm N61 03.08333 E022 00.00000 - N64 03.78334 E022 22.81667
  002cc3ec-19cb-48ce-b19c-c8d8155728de AGIXA
  0be26352-a6f0-4ba5-a7f4-8916330c2989 ATMAT

DCT SUDIP-KUKUS dist 132.667nm N67 27.25000 E025 03.00000 - N65 59.50000 E029 14.15000
  0f1556a4-13b4-488a-8f65-88a7b483beec SUDIP
  f7eb0c7e-a78e-4b2d-8534-32fac43002da KUKUS


ROUTE168: INVALID DCT SOPAX..LIFFY. DCT ARE NOT ALLOWED TO CROSS THE BORDER BETWEEN EIDCT:000:245 AND EGDCT:000:245. [EIEG400A]
a63d0619-06ad-4181-ac83-f3e095943cd8 SOPAX
d98daca1-48f4-4d75-b4e1-80f99fcb9732 LIFFY

 */

void DctParameters::run(void)
{
	Database& db(get_dbref());
	bool saveempty;
	db.drop_dct_indices();
	m_ppcnt = m_dctcnt = m_finished = 0;
	m_pptot = m_points.size() * (m_points.size() - 1) / 2;
	{
		unsigned int cnt(db.count_dct());
		if (m_verbose)
			std::cout << m_allrules.size() << " DCT rules, " << m_sidlimit.size() << '/' << m_starlimit.size() << " airport DCT limits, "
				  << m_points.size() << " points, " << cnt << " existing DCT, " << m_pptot << " DCT pairs, Airway graph "
				  << boost::num_vertices(m_graph) << " vertices and " << boost::num_edges(m_graph) << " edges" << std::endl;
		saveempty = !!cnt;
	}
	if (true) {
		for (points_t::size_type i(0), n(m_points.size()); i < n; ++i) {
			std::cout << m_points[i]->get_uuid() << " D"
				  << Glib::ustring::format(std::fixed, std::setprecision(1), m_dctradius[i]);
			{
				const IdentTimeSlice& ts(m_points[i]->operator()(m_tmcutoff).as_ident());
				if (ts.is_valid()) {
					std::cout << ' ' << ts.get_ident();
				} else if (m_points[i]->size()) {
					const IdentTimeSlice& ts(m_points[i]->operator[](0).as_ident());
					if (ts.is_valid())
						std::cout << ' ' << ts.get_ident();
				}
			}
			std::cout << std::endl;
		}
	}
	sqlite3x::sqlite3_transaction tran(db.get_db_connection());
	if (m_worker) {
		TopoDb30 topodb[m_worker];
		Glib::Threads::Thread *thread[m_worker];
		if (m_topodbpath.empty()) {
			for (unsigned int i = 0; i < m_worker; ++i)
				thread[i] = Glib::Threads::Thread::create(sigc::bind<0>(sigc::bind<1>(sigc::bind<2>(sigc::mem_fun(*this, &DctParameters::run_threaded), (TopoDb30 *)0), m_worker), i));
		} else {
			for (unsigned int i = 0; i < m_worker; ++i) {
				topodb[i].open(m_topodbpath);
				thread[i] = Glib::Threads::Thread::create(sigc::bind<0>(sigc::bind<1>(sigc::bind<2>(sigc::mem_fun(*this, &DctParameters::run_threaded), &topodb[i]), m_worker), i));
			}
		}
		for (;;) {
			results_t calc;
			for (;;) {
				Glib::Threads::Mutex::Lock lock(m_mutex);
				if (!m_results.empty()) {
					calc.splice(calc.end(), m_results, m_results.begin());
					--m_resultscnt;
					break;
				}
				if (m_finished >= m_worker)
					break;
				m_cond.wait(m_mutex);
			}
			if (calc.empty())
				break;
			m_cond.broadcast();
			bool legempty(calc.front().get_leg().is_empty());
			if (m_trace || (m_verbose && !legempty)) {
				std::cout << "DCT";
				{
					const PointIdentTimeSlice& ts0(calc.front().get_point(0)->operator()(m_tmcutoff).as_point());
					const PointIdentTimeSlice& ts1(calc.front().get_point(1)->operator()(m_tmcutoff).as_point());
					if (ts0.is_valid() && ts1.is_valid()) {
						std::cout << ' ' << ts0.get_ident() << '-' << ts1.get_ident()
							  << " dist " << ts0.get_coord().spheric_distance_nmi_dbl(ts1.get_coord())
							  << "nm " << ts0.get_coord().get_lat_str2() << ' ' << ts0.get_coord().get_lon_str2()
							  << " - " << ts1.get_coord().get_lat_str2() << ' ' << ts1.get_coord().get_lon_str2();
					}
				}
				calc.front().get_leg().print(std::cout, 2, m_tmcutoff) << std::endl;
			}
			if (m_trace || m_verbose) {
				for (messages_t::const_iterator mi(calc.front().get_messages().begin()),
					     me(calc.front().get_messages().end()); mi != me; ++mi) {
					if (mi->get_type() == Message::type_error) {
						std::cerr << mi->to_str() << std::endl;
						continue;
					}
					std::cout << mi->to_str() << std::endl;
				}
			}
			m_messages.clear();
			if (saveempty || !legempty)
				db.save_dct(calc.front().get_leg(), false);
			if (!legempty) {
				++m_dctcnt;
				if (!(m_dctcnt & 0x3FF)) {
					tran.commit();
					tran.begin();
				}
			}
		}
		for (unsigned int i = 0; i < m_worker; ++i)
			thread[i]->join();
		if (m_trace || m_verbose)
			std::cout << "recreating indices" << std::endl;
		tran.commit();
		db.create_dct_indices();
		return;
	}
	std::list<CalcMT> calcmt;
	for (points_t::size_type i0(0), e(m_points.size()); i0 < e; ++i0) {
		const Object::ptr_t& p0(m_points[i0]);
		if (!p0)
			continue;
		timeset_t tdisc0(p0->timediscontinuities());
		tdisc0.insert(m_ruletimedisc.begin(), m_ruletimedisc.end());
		bool arpt0(p0->get_type() >= Object::type_airport && p0->get_type() <= Object::type_airportend);
		if (arpt0) {
			AirportDctLimit limlo(Link(p0), std::numeric_limits<timetype_t>::min(), std::numeric_limits<timetype_t>::min());
			AirportDctLimit limhi(Link(p0), std::numeric_limits<timetype_t>::max(), std::numeric_limits<timetype_t>::max());
			for (airportdctlimit_t::const_iterator i(m_sidlimit.lower_bound(limlo)), e(m_sidlimit.upper_bound(limhi)); i != e; ++i) {
				if (i->get_arpt() != p0->get_uuid())
					continue;
				tdisc0.insert(i->get_starttime());
				tdisc0.insert(i->get_endtime());
			}
			for (airportdctlimit_t::const_iterator i(m_starlimit.lower_bound(limlo)), e(m_starlimit.upper_bound(limhi)); i != e; ++i) {
				if (i->get_arpt() != p0->get_uuid())
					continue;
				tdisc0.insert(i->get_starttime());
				tdisc0.insert(i->get_endtime());
			}
		}
		for (points_t::size_type i1(i0 + 1); i1 < e; ++i1) {
			const Object::ptr_t& p1(m_points[i1]);
			if (!p1)
				continue;
			timeset_t tdisc1(p1->timediscontinuities());
			tdisc1.insert(tdisc0.begin(), tdisc0.end());
			bool arpt1(p1->get_type() >= Object::type_airport && p1->get_type() <= Object::type_airportend);
			if (arpt1) {
				if (arpt0)
					continue;
				AirportDctLimit limlo(Link(p1), std::numeric_limits<timetype_t>::min(), std::numeric_limits<timetype_t>::min());
				AirportDctLimit limhi(Link(p1), std::numeric_limits<timetype_t>::max(), std::numeric_limits<timetype_t>::max());
				for (airportdctlimit_t::const_iterator i(m_sidlimit.lower_bound(limlo)), e(m_sidlimit.upper_bound(limhi)); i != e; ++i) {
					if (i->get_arpt() != p1->get_uuid())
						continue;
					tdisc1.insert(i->get_starttime());
					tdisc1.insert(i->get_endtime());
				}
				for (airportdctlimit_t::const_iterator i(m_starlimit.lower_bound(limlo)), e(m_starlimit.upper_bound(limhi)); i != e; ++i) {
					if (i->get_arpt() != p1->get_uuid())
						continue;
					tdisc1.insert(i->get_starttime());
					tdisc1.insert(i->get_endtime());
				}
			}
			Calc calc(*this, p0, p1, tdisc1, m_all, m_verbose, m_trace);
			bool dorundcttime(true);
			if (!arpt0 && !arpt1) {
				std::pair<double,double> d(calc.get_leg().dist());
				double d1(std::min(m_dctradius[i0], m_dctradius[i1]));
				dorundcttime = d.first <= d1 + 1;
				if (!dorundcttime && m_trace) {
					const PointIdentTimeSlice& ts0(calc.get_point(0)->operator()(m_tmcutoff).as_point());
					const PointIdentTimeSlice& ts1(calc.get_point(1)->operator()(m_tmcutoff).as_point());
					std::cout << "Skipping DCT " << ts0.get_ident() << ' ' << ts1.get_ident() << " D"
						  << Glib::ustring::format(std::fixed, std::setprecision(1), d.first) << "..."
						  << Glib::ustring::format(std::fixed, std::setprecision(1), d.second)
						  << " maxradius " << Glib::ustring::format(std::fixed, std::setprecision(1), d1)
						  << std::endl;
				}
			}
			if (dorundcttime) {
				if (m_worker) {
					for (;;) {
						bool ne;
						{
							Glib::Threads::Mutex::Lock lock(m_mutex);
							if (calcmt.size() < m_worker) {
						        	calcmt.push_back(calc);
								calcmt.back().run();
								break;
							}
							ne = finishone(calcmt, saveempty);
						}
						if (ne) {
							++m_dctcnt;
							if (!(m_dctcnt & 0x3FF)) {
								tran.commit();
								tran.begin();
							}
						}
						++m_ppcnt;
						if (!(m_ppcnt & 0x3FF) || m_ppcnt >= m_pptot) {
							std::ostringstream oss;
							oss << m_ppcnt << '/' << m_pptot << " DCT pairs, " << m_dctcnt << " DCT legs, "
							    << std::fixed << std::setprecision(2) << (m_ppcnt * 100.0 / m_pptot) << '%';
							std::cout << oss.str() << std::endl;
						}
					}
					continue;
				}
				calc.run();
				if (m_topodb)
					calc.run_topo(*m_topodb);
				if (m_trace || (m_verbose && !calc.get_leg().is_empty())) {
					std::cout << "DCT";
					{
						const PointIdentTimeSlice& ts0(calc.get_point(0)->operator()(m_tmcutoff).as_point());
						const PointIdentTimeSlice& ts1(calc.get_point(1)->operator()(m_tmcutoff).as_point());
						if (ts0.is_valid() && ts1.is_valid()) {
							std::cout << ' ' << ts0.get_ident() << '-' << ts1.get_ident()
								  << " dist " << ts0.get_coord().spheric_distance_nmi_dbl(ts1.get_coord())
								  << "nm " << ts0.get_coord().get_lat_str2() << ' ' << ts0.get_coord().get_lon_str2()
								  << " - " << ts1.get_coord().get_lat_str2() << ' ' << ts1.get_coord().get_lon_str2();
						}
					}
					calc.get_leg().print(std::cout, 2, m_tmcutoff) << std::endl;
				}
				if (m_trace || m_verbose) {
					for (messages_t::const_iterator mi(calc.get_messages().begin()), me(calc.get_messages().end()); mi != me; ++mi) {
						if (mi->get_type() == Message::type_error) {
							std::cerr << mi->to_str() << std::endl;
							continue;
						}
						std::cout << mi->to_str() << std::endl;
					}
				}
				m_messages.clear();
				if (saveempty || !calc.get_leg().is_empty())
					db.save_dct(calc.get_leg(), false);
				if (!calc.get_leg().is_empty()) {
					++m_dctcnt;
					if (!(m_dctcnt & 0x3FF)) {
						tran.commit();
						tran.begin();
					}
				}
			}
			++m_ppcnt;
			if (!(m_ppcnt & 0x3FF) || m_ppcnt >= m_pptot) {
				std::ostringstream oss;
				oss << m_ppcnt << '/' << m_pptot << " DCT pairs, " << m_dctcnt << " DCT legs, "
				    << std::fixed << std::setprecision(2) << (m_ppcnt * 100.0 / m_pptot) << '%';
				std::cout << oss.str() << std::endl;
			}
		}
	}
	if (m_worker) {
		for (;;) {
			bool ne;
			{
				Glib::Threads::Mutex::Lock lock(m_mutex);
				if (calcmt.empty())
					break;
				ne = finishone(calcmt, saveempty);
			}
			if (ne) {
				++m_dctcnt;
				if (!(m_dctcnt & 0x3FF)) {
					tran.commit();
					tran.begin();
				}
			}
			++m_ppcnt;
			if (!(m_ppcnt & 0x3FF) || m_ppcnt >= m_pptot) {
				std::ostringstream oss;
				oss << m_ppcnt << '/' << m_pptot << " DCT pairs, " << m_dctcnt << " DCT legs, "
				    << std::fixed << std::setprecision(2) << (m_ppcnt * 100.0 / m_pptot) << '%';
				std::cout << oss.str() << std::endl;
			}
		}
	}
	tran.commit();
	db.create_dct_indices();
}

void DctParameters::run_threaded(points_t::size_type i0, points_t::size_type istride, TopoDb30 *topodb) const
{
	for (points_t::size_type e(m_points.size()); i0 < e; i0 += istride) {
		const Object::ptr_t& p0(m_points[i0]);
		if (!p0) {
			g_atomic_int_add(&m_ppcnt, e - i0 - 1);
			continue;
		}
		timeset_t tdisc0(p0->timediscontinuities());
		tdisc0.insert(m_ruletimedisc.begin(), m_ruletimedisc.end());
		bool arpt0(p0->get_type() >= Object::type_airport && p0->get_type() <= Object::type_airportend);
		if (arpt0) {
			AirportDctLimit limlo(Link(p0), std::numeric_limits<timetype_t>::min(), std::numeric_limits<timetype_t>::min());
			AirportDctLimit limhi(Link(p0), std::numeric_limits<timetype_t>::max(), std::numeric_limits<timetype_t>::max());
			for (airportdctlimit_t::const_iterator i(m_sidlimit.lower_bound(limlo)), e(m_sidlimit.upper_bound(limhi)); i != e; ++i) {
				if (i->get_arpt() != p0->get_uuid())
					continue;
				tdisc0.insert(i->get_starttime());
				tdisc0.insert(i->get_endtime());
			}
			for (airportdctlimit_t::const_iterator i(m_starlimit.lower_bound(limlo)), e(m_starlimit.upper_bound(limhi)); i != e; ++i) {
				if (i->get_arpt() != p0->get_uuid())
					continue;
				tdisc0.insert(i->get_starttime());
				tdisc0.insert(i->get_endtime());
			}
		}
		for (points_t::size_type i1(i0 + 1); i1 < e; ++i1) {
			const Object::ptr_t& p1(m_points[i1]);
			if (!p1) {
				g_atomic_int_add(&m_ppcnt, 1);
				continue;
			}
			timeset_t tdisc1(p1->timediscontinuities());
			tdisc1.insert(tdisc0.begin(), tdisc0.end());
			bool arpt1(p1->get_type() >= Object::type_airport && p1->get_type() <= Object::type_airportend);
			if (arpt1) {
				if (arpt0) {
					g_atomic_int_add(&m_ppcnt, 1);
					continue;
				}
				AirportDctLimit limlo(Link(p1), std::numeric_limits<timetype_t>::min(), std::numeric_limits<timetype_t>::min());
				AirportDctLimit limhi(Link(p1), std::numeric_limits<timetype_t>::max(), std::numeric_limits<timetype_t>::max());
				for (airportdctlimit_t::const_iterator i(m_sidlimit.lower_bound(limlo)), e(m_sidlimit.upper_bound(limhi)); i != e; ++i) {
					if (i->get_arpt() != p1->get_uuid())
						continue;
					tdisc1.insert(i->get_starttime());
					tdisc1.insert(i->get_endtime());
				}
				for (airportdctlimit_t::const_iterator i(m_starlimit.lower_bound(limlo)), e(m_starlimit.upper_bound(limhi)); i != e; ++i) {
					if (i->get_arpt() != p1->get_uuid())
						continue;
					tdisc1.insert(i->get_starttime());
					tdisc1.insert(i->get_endtime());
				}
			}
			results_t calc;
			calc.push_back(Calc(*this, p0, p1, tdisc1, m_all, m_verbose, m_trace));
			bool dorundcttime(true);
			if (!arpt0 && !arpt1) {
				std::pair<double,double> d(calc.back().get_leg().dist());
				double d1(std::min(m_dctradius[i0], m_dctradius[i1]));
				dorundcttime = d.first <= d1 + 1;
				if (!dorundcttime && m_trace) {
					const PointIdentTimeSlice& ts0(calc.back().get_point(0)->operator()(m_tmcutoff).as_point());
					const PointIdentTimeSlice& ts1(calc.back().get_point(1)->operator()(m_tmcutoff).as_point());
					std::cout << "Skipping DCT " << ts0.get_ident() << ' ' << ts1.get_ident() << " D"
						  << Glib::ustring::format(std::fixed, std::setprecision(1), d.first) << "..."
						  << Glib::ustring::format(std::fixed, std::setprecision(1), d.second)
						  << " maxradius " << Glib::ustring::format(std::fixed, std::setprecision(1), d1)
						  << std::endl;
				}
			}
			if (dorundcttime) {
				calc.back().run();
				if (topodb)
					calc.back().run_topo(*topodb);
				result(calc);
			}
			gint ppcnt(g_atomic_int_add(&m_ppcnt, 1) + 1);
			if (!(ppcnt & 0x3FF) || ppcnt >= m_pptot) {
				std::ostringstream oss;
				oss << ppcnt << '/' << m_pptot << " DCT pairs, " << m_dctcnt << " DCT legs, "
				    << std::fixed << std::setprecision(2) << (ppcnt * 100.0 / m_pptot) << '%';
				std::cout << oss.str() << std::endl;
			}
		}
	}
	finished();
}

void DctParameters::result(results_t& x) const
{
	{
		Glib::Threads::Mutex::Lock lock(m_mutex);
		// flow control
		while (m_resultscnt > 1024)
			m_cond.wait(m_mutex);
		m_resultscnt += x.size();
		m_results.splice(m_results.end(), x);
	}
	m_cond.broadcast();
}

void DctParameters::finished(void) const
{
	{
		Glib::Threads::Mutex::Lock lock(m_mutex);
		++m_finished;
	}
	m_cond.broadcast();
}




void DctParameters::signal(void) const
{
	m_cond.broadcast();
}

bool DctParameters::finishone(std::list<CalcMT>& calc, bool saveempty)
{
	bool legempty(false);
	std::list<CalcMT>::iterator ci, ce;
	for (;;) {
		if (calc.empty())
			return false;
		for (ci = calc.begin(), ce = calc.end(); ci != ce; ++ci)
			if (ci->is_done())
				break;
		if (ci != ce)
			break;
		m_cond.wait(m_mutex);
	}
	ci->join();
	if (m_topodb)
		ci->run_topo(*m_topodb);
	legempty = ci->get_leg().is_empty();
	if (m_trace || (m_verbose && !legempty)) {
		std::cout << "DCT";
		{
			const PointIdentTimeSlice& ts0(ci->get_point(0)->operator()(m_tmcutoff).as_point());
			const PointIdentTimeSlice& ts1(ci->get_point(1)->operator()(m_tmcutoff).as_point());
			if (ts0.is_valid() && ts1.is_valid()) {
				std::cout << ' ' << ts0.get_ident() << '-' << ts1.get_ident()
					  << " dist " << ts0.get_coord().spheric_distance_nmi_dbl(ts1.get_coord())
					  << "nm " << ts0.get_coord().get_lat_str2() << ' ' << ts0.get_coord().get_lon_str2()
					  << " - " << ts1.get_coord().get_lat_str2() << ' ' << ts1.get_coord().get_lon_str2();
			}
		}
		ci->get_leg().print(std::cout, 2, m_tmcutoff) << std::endl;
	}
	if (m_trace || m_verbose) {
		for (messages_t::const_iterator mi(ci->get_messages().begin()), me(ci->get_messages().end()); mi != me; ++mi) {
			if (mi->get_type() == Message::type_error) {
				std::cerr << mi->to_str() << std::endl;
				continue;
			}
			std::cout << mi->to_str() << std::endl;
		}
	}
	if (saveempty || !legempty) {
		Database& db(get_dbref());
		db.save_dct(ci->get_leg(), false);
	}
	calc.erase(ci);
	return !legempty;
}

const BidirAltRange DctParameters::Calc::defaultalt(BidirAltRange::set_t::Intvl(0, 66500), BidirAltRange::set_t::Intvl(0, 66500));
constexpr double DctParameters::Calc::airway_preferred_factor;

DctParameters::Calc::Calc(const DctParameters& param, const Object::ptr_t& p0, const Object::ptr_t& p1, const timeset_t& tdisc, bool all, bool verbose, bool trace)
	: m_param(&param), m_leg(DctLeg(Link(p0->get_uuid(), p0), Link(p1->get_uuid(), p1))), m_tdisc(tdisc),
	  m_tm(0), m_tmend(std::numeric_limits<timetype_t>::max()),
	  m_dist(std::numeric_limits<double>::quiet_NaN()), m_all(all), m_verbose(verbose), m_trace(trace)
{
	if (m_leg.get_uuid(0) > m_leg.get_uuid(1))
		m_leg.swapdir();
}

const Point& DctParameters::Calc::get_coord(unsigned int index) const
{
	const PointIdentTimeSlice& ts(get_point(index)->operator()(get_time()).as_point());
	if (ts.is_valid())
		return ts.get_coord();
	return Point::invalid;
}

const UUID& DctParameters::Calc::get_uuid(unsigned int index) const
{
	const Object::const_ptr_t& p(get_point(index));
	if (!p)
		return UUID::niluuid;
	return p->get_uuid();
}

const std::string& DctParameters::Calc::get_ident(unsigned int index) const
{
	static const std::string empty;
	const IdentTimeSlice& id(get_point(index)->operator()(get_time()).as_ident());
	if (id.is_valid())
		return id.get_ident();
	return empty;
}

const TimeTableEval DctParameters::Calc::get_tte(unsigned int index) const
{
	return TimeTableEval(get_time(), get_coord(index), get_param().get_specialdateeval());
}

const TimeTableEval DctParameters::Calc::get_tte(unsigned int index, float crs, float dist) const
{
	return TimeTableEval(get_time(), get_coord(index).simple_course_distance_nmi(crs, dist), get_param().get_specialdateeval());
}

bool DctParameters::Calc::is_airport(unsigned int index) const
{
	const AirportTimeSlice& arpt(get_point(index)->operator()(get_time()).as_airport());
	return arpt.is_valid();
}

bool DctParameters::Calc::is_airport(void) const
{
	return is_airport(0) || is_airport(1);
}

void DctParameters::Calc::run(void)
{
	bool xarpt[2] = { false, false };
	for (unsigned int i = 0; i < 2; ++i)
		xarpt[i] = get_point(i)->get_type() >= Object::type_airport && get_point(i)->get_type() <= Object::type_airportend;
	for (timeset_t::const_iterator ti(m_tdisc.begin()), te(m_tdisc.end()); ti != te; ) {
		m_tm = *ti;
		if (m_tm >= get_param().get_tmfuturecutoff())
			break;
		++ti;
		if (ti == te)
			break;
		m_tmend = *ti;
		if (m_tmend <= get_param().get_tmcutoff())
			continue;
		if (m_tmend > get_param().get_tmfuturecutoff())
			m_tmend = get_param().get_tmfuturecutoff();
		IntervalSet<int32_t> dir[2];
		for (unsigned int j = 0; j < 2; ++j)
			dir[j] = IntervalSet<int32_t>(IntervalSet<int32_t>::Intvl(0, std::numeric_limits<int32_t>::max()));
		for (unsigned int j = 0; j < 2; ++j) {
			if (!xarpt[j])
				continue;
			dir[0].set_empty();
			dir[1].set_empty();
			AirportDctLimit limlo(Link(get_point(j)), std::numeric_limits<timetype_t>::min(), std::numeric_limits<timetype_t>::min());
			AirportDctLimit limhi(Link(get_point(j)), std::numeric_limits<timetype_t>::max(), std::numeric_limits<timetype_t>::max());
			for (airportdctlimit_t::const_iterator i(get_param().get_sidlimit().lower_bound(limlo)),
				     e(get_param().get_sidlimit().upper_bound(limhi)); i != e; ++i) {
				if (i->get_arpt() != get_point(j)->get_uuid())
					continue;
				dir[j] |= i->get_altinterval(Link(get_point(!j)), m_tm);
			}
			for (airportdctlimit_t::const_iterator i(get_param().get_starlimit().lower_bound(limlo)),
				     e(get_param().get_starlimit().upper_bound(limhi)); i != e; ++i) {
				if (i->get_arpt() != get_point(j)->get_uuid())
					continue;
				dir[!j] |= i->get_altinterval(Link(get_point(!j)), m_tm);
			}
		}
		if (dir[0].is_empty() && dir[1].is_empty())
			continue;
		DctLeg::altset_t altset(run_dct_time());
		for (DctLeg::altset_t::const_iterator ai(altset.begin()), ae(altset.end()); ai != ae; ++ai) {
			DctLeg::Alt a(*ai);
			a.get_altrange()[0] &= dir[0];
			a.get_altrange()[1] &= dir[1];
			m_leg.add(a);
		}
	}
	m_leg.simplify();
}

void DctParameters::Calc::run_topo(TopoDb30& topodb)
{
	if (m_leg.get_altset().empty())
		return;
	const PointIdentTimeSlice& ts0(get_point(0)->operator()(get_time()).as_point());
	if (!ts0.is_valid() || ts0.get_coord().is_invalid())
		return;
	const PointIdentTimeSlice& ts1(get_point(1)->operator()(get_time()).as_point());
	if (!ts1.is_valid() || ts1.get_coord().is_invalid())
		return;
	SegmentTimeSlice::profile_t p(SegmentTimeSlice::compute_profile(topodb, ts0.get_coord(), ts1.get_coord()));
	if (p.second == ElevPointIdentTimeSlice::invalid_elev)
		return;
	if (m_trace) {
		std::ostringstream oss;
		oss << ts0.get_ident() << '-' << ts1.get_ident()
		    << ' ' << ts0.get_coord().get_lat_str2() << ' ' << ts0.get_coord().get_lon_str2()
		    << " - " << ts1.get_coord().get_lat_str2() << ' ' << ts1.get_coord().get_lon_str2()
		    << " dist " << std::setprecision(1) << std::fixed << m_dist
		    << " terrain " << p.first << " corridor " << p.second;
		message(oss.str(), Message::type_trace, get_time());
	}
	BidirAltRange::type_t minalt(p.second);
	if (minalt >= 5000)
		minalt += 1000;
	minalt += 1000;
	BidirAltRange minar(BidirAltRange::set_t::Intvl(minalt, std::numeric_limits<BidirAltRange::type_t>::max()), 
			    BidirAltRange::set_t::Intvl(minalt, std::numeric_limits<BidirAltRange::type_t>::max()));
	DctLeg::altset_t altset1;
	for (DctLeg::altset_t::const_iterator ai(m_leg.get_altset().begin()), ae(m_leg.get_altset().end()); ai != ae; ++ai) {
		BidirAltRange ar(ai->get_altrange());
		ar &= minar;
		DctLeg::add(altset1, DctLeg::Alt(ar, ai->get_timetable()));
	}
	m_leg.get_altset().swap(altset1);
}

void DctParameters::Calc::update_altset(DctLeg::altset_t& altset, const BidirAltRange& rdct, const FlightRestrictionTimeSlice& ts, bool altor)
{
	if (altor) {
		if (rdct[0].is_empty() && rdct[1].is_empty())
			return;
	} else {
		if (!rdct[0].is_empty() && !rdct[1].is_empty() &&
		    *rdct[0].begin() == *get_alt()[0].begin() &&
		    *rdct[1].begin() == *get_alt()[1].begin())
			return;
	}
	if (false) {
		std::cerr << "Old Table: R:" << ts.get_ident() << ' ' << rdct[0].to_str() << " / " << rdct[1].to_str() << std::endl;
		for (DctLeg::altset_t::const_iterator ai(altset.begin()), ae(altset.end()); ai != ae; ++ai)
			ai->get_timetable().print(std::cerr << "  " << ai->get_altrange()[0].to_str() << " / "
						  << ai->get_altrange()[1].to_str(), 4) << std::endl;
	}
	DctLeg::altset_t altset1;
	for (DctLeg::altset_t::const_iterator ai(altset.begin()), ae(altset.end()); ai != ae; ++ai) {
		const BidirAltRange& ar0(ai->get_altrange());
		BidirAltRange ar1(ar0);
		if (altor)
			ar1 |= rdct;
		else
			ar1 &= rdct;
		if (ar1 == ar0) {
			DctLeg::add(altset1, *ai);
			if (!false && m_trace)
				message("R:" + ts.get_ident() + " Leg Altitude: " + ar0[0].to_str() + " / " + ar0[1].to_str() +
					" no change", Message::type_trace, get_time());
			continue;
		}
		TimeTable ttdct(ts.get_timetable());
		ttdct.limit(get_time(), get_endtime());
		TimeTableOr tt0(ai->get_timetable());
		TimeTableOr tt1(tt0);
		tt1 &= ttdct;
		ttdct.invert();
		tt0 &= ttdct;
		if (true) {
			tt0.simplify(false);
			tt1.simplify(false);
		}
		if (!tt0.is_never() && (altor || !ar0.is_empty()))
			DctLeg::add(altset1, DctLeg::Alt(ar0, tt0));
		if (!tt1.is_never() && (altor || !ar1.is_empty()))
			DctLeg::add(altset1, DctLeg::Alt(ar1, tt1));
		if (!false && m_trace)
			message("R:" + ts.get_ident() + " Leg Altitude: " + ar0[0].to_str() + " / " + ar0[1].to_str() + " -> " +
				ar1[0].to_str() + " / " + ar1[1].to_str(), Message::type_trace, get_time());
		if (false) {
			ai->get_timetable().print(std::cerr << "Old Leg: " << ar0[0].to_str() << " / " << ar0[1].to_str(), 2) << std::endl;
			tt0.print(std::cerr << "New Leg 0: " << ar0[0].to_str() << " / " << ar0[1].to_str(), 2) << std::endl;
			tt1.print(std::cerr << "New Leg 1: " << ar1[0].to_str() << " / " << ar1[1].to_str(), 2) << std::endl;
		}
	}
	altset.swap(altset1);
	if (false) {
		std::cerr << "New Table: R:" << ts.get_ident() << std::endl;
		for (DctLeg::altset_t::const_iterator ai(altset.begin()), ae(altset.end()); ai != ae; ++ai)
			ai->get_timetable().print(std::cerr << "  " << ai->get_altrange()[0].to_str() << " / "
						  << ai->get_altrange()[1].to_str(), 4) << std::endl;
	}
}

DctLeg::altset_t DctParameters::Calc::run_dct_time(void)
{
	Point coord[2];
	const std::string *ident[2];
	bool arpt(false);
	for (unsigned int i = 0; i < 2; ++i) {
		const PointIdentTimeSlice& ts(get_point(i)->operator()(get_time()).as_point());
		if (!ts.is_valid() || ts.get_ident().size() < 2 ||
		    Engine::AirwayGraphResult::Vertex::is_ident_numeric(ts.get_ident()))
			return DctLeg::altset_t();
		coord[i] = ts.get_coord();
		if (coord[i].is_invalid())
			return DctLeg::altset_t();
		ident[i] = &ts.get_ident();
		const NavaidTimeSlice& tsnav(ts.as_navaid());
		if (tsnav.is_valid()) {
			if (!tsnav.is_vor() && !tsnav.is_dme() && !tsnav.is_tacan() && !tsnav.is_ndb() && !tsnav.is_mkr())
				return DctLeg::altset_t();
		}
		const DesignatedPointTimeSlice& tsint(ts.as_designatedpoint());
		if (tsint.is_valid()) {
			switch (tsint.get_type()) {
			case DesignatedPointTimeSlice::type_icao:
				break;

			default:
				return DctLeg::altset_t();
			}
		}
		const AirportTimeSlice& tsarpt(ts.as_airport());
		if (tsarpt.is_valid()) {
			arpt = true;
		}
	}
	m_dist = coord[0].spheric_distance_nmi_dbl(coord[1]);
	bool isdctseg(get_param().get_seg().contains(get_point(0), get_point(1)));
	if (m_trace) {
		std::ostringstream oss;
		oss << *ident[0] << '-' << *ident[1] << std::fixed << std::setprecision(1)
		    << " dist " << m_dist << " maxdist " << get_param().get_maxdist() << (isdctseg ? " (DCT)" : "")
		    << " time " << Glib::TimeVal(get_time(), 0).as_iso8601() << '-'
		    << Glib::TimeVal(get_endtime(), 0).as_iso8601();
		message(oss.str(), Message::type_trace, get_time());
	}
	if (m_dist > get_param().get_maxdist() && !isdctseg)
		return DctLeg::altset_t();
	if (!m_all && !arpt) {
		bool m(false);
		for (rules_t::const_iterator ri(get_param().get_allrules().begin()), re(get_param().get_allrules().end()); ri != re; ++ri) {
			if ((*ri)->get_modified() < get_param().get_tmmodified())
				continue;
			const FlightRestrictionTimeSlice& ts((*ri)->operator()(get_time()).as_flightrestriction());
			if (!ts.is_valid() || !ts.is_enabled())
				continue;
			if (!ts.is_dct())
				continue;
			Rect bbox;
			ts.get_bbox(bbox);
			if (!bbox.is_intersect(coord[0], coord[1]))
				continue;
			m = true;
			break;
		}
		if (!m)
			return DctLeg::altset_t();
	}
	Rect bbox(coord[0], coord[0]);
	bbox = bbox.add(coord[1]);
	DctLeg::altset_t altset;
	{
		TimeTableOr tt(TimeTableOr::create_always());
		tt.limit(get_time(), get_endtime());
		altset.insert(DctLeg::Alt(get_alt(), tt));
	}
	if (false && m_trace) {
		std::ostringstream oss;
		oss << "Initial Altset: " << std::endl;
		for (DctLeg::altset_t::const_iterator ai(altset.begin()), ae(altset.end()); ai != ae; ++ai)
			ai->get_timetable().print(oss << "  " << ai->get_altrange()[0].to_str() << " / "
						  << ai->get_altrange()[1].to_str(), 4) << std::endl;
		message(oss.str(), Message::type_trace, get_time());
	}
	for (rules_t::const_iterator ri(get_param().get_allrules().begin()), re(get_param().get_allrules().end()); ri != re; ++ri) {
		m_currule = *ri;
		const FlightRestrictionTimeSlice& ts((*ri)->operator()(get_time()).as_flightrestriction());
		if (!ts.is_valid() || !ts.is_enabled() || !ts.is_dct()) {
			if (true && m_trace) {
				std::ostringstream oss;
				oss << "Skipping R:" << (*ri)->operator[](0).as_ident().get_ident()
				    << (ts.is_valid() ? " (V)" : "") << (ts.is_dct() ? " (D)" : "");
				message(oss.str(), Message::type_trace, get_time());
			}
			m_currule.clear();
			continue;
		}
		{
			Rect bbox1;
			(*ri)->get_bbox(bbox1);
			if (!bbox.is_intersect(bbox1)) {
				m_currule.clear();
				continue;
			}
		}
		BidirAltRange rdct(ts.evaluate_dct(*this, m_trace));
		m_currule.clear();
		update_altset(altset, rdct, ts, false);
		if (false && m_trace) {
			std::ostringstream oss;
			oss << "R:" << (*ri)->operator[](0).as_ident().get_ident() << " Altset: " << std::endl;
			for (DctLeg::altset_t::const_iterator ai(altset.begin()), ae(altset.end()); ai != ae; ++ai)
				ai->get_timetable().print(oss << "  " << ai->get_altrange()[0].to_str() << " / "
							  << ai->get_altrange()[1].to_str(), 4) << std::endl;
			message(oss.str(), Message::type_trace, get_time());
		}
		if (altset.empty())
			break;
	}
	// apply allowed altitudes
	DctLeg::clear_empty(altset);
	if (altset.empty())
		return DctLeg::altset_t();
	for (;;) {
		// check that the DCT segment actually saves distance compared to the routing network
		if (isdctseg || arpt)
			break;
		const Graph& graph(get_param().get_graph());
		Graph::vertex_descriptor vd[2];
		{
			unsigned int dir(0);
			for (; dir < 2; ++dir) {
				std::pair<Graph::vertex_descriptor,bool> v(graph.find_vertex(get_point(dir)));
				if (!v.second) {
					if (m_trace)
						message("Point " + *ident[dir] + " not found in routing graph", Message::type_trace, get_time());
					break;
				}
				vd[dir] = v.first;
			}
			if (dir < 2)
				break;
		}
		bbox = bbox.oversize_nmi(10);
		DctLeg::altset_t altset1;
		for (DctLeg::altset_t::const_iterator ai(altset.begin()), ae(altset.end()); ai != ae; ++ai) {
			DctLeg::Alt a(*ai);
			a.get_timetable().simplify(false);
			a.get_timetable().split(get_param().get_routetimedisc());
			for (TimeTableOr::const_iterator tti(a.get_timetable().begin()), tte(a.get_timetable().end()); tti != tte; ++tti) {
				timepair_t bnd(tti->timebounds());
				BidirAltRange ar(a.get_altrange());
				for (unsigned int dir = 0; dir < 2; ++dir) {
					for (;;) {
						Graph::AltrangeTimeEdgeFilter filter(graph, bnd.first, ar[dir], bbox);
						typedef boost::filtered_graph<Graph, Graph::AltrangeTimeEdgeFilter> fg_t;
						fg_t fg(graph, filter);
						std::vector<Graph::vertex_descriptor> predecessors(boost::num_vertices(graph), 0);
						std::vector<double> distances(boost::num_vertices(graph), 0);
						Graph::TimeEdgeDistanceMap wmap(graph, bnd.first);
						dijkstra_shortest_paths(fg, vd[dir], boost::weight_map(wmap).
									predecessor_map(&predecessors[0]).distance_map(&distances[0]));
						// boost::weight_map(boost::get(&GraphEdge::m_dist, graph)).
						// boost::weight_map(wmap).
						Graph::vertex_descriptor v(vd[!dir]);
						if (predecessors[v] == v)
							break;
						if (distances[v] > airway_preferred_factor * m_dist) {
							if (m_trace) {
								std::ostringstream oss;
								oss << "Route " << *ident[dir] << " -> " << *ident[!dir] << ' ' << ar[dir].to_str()
								    << std::fixed << std::setprecision(1) << " dist " << distances[v]
								    << " DCT dist " << m_dist << " keeping DCT";
								message(oss.str(), Message::type_trace, bnd.first);
							}
							break;
						}
						BidirAltRange::set_t rtear;
						rtear.set_full();
						for (;;) {
							Graph::vertex_descriptor u(predecessors[v]);
							if (u == v)
								break;
							BidirAltRange::set_t rtesegar;
							rtesegar.set_empty();
							Graph::out_edge_iterator e0, e1;
							for (boost::tie(e0, e1) = boost::out_edges(u, graph); e0 != e1; ++e0) {
								if (boost::target(*e0, graph) != v)
									continue;
								rtesegar |= graph[*e0].get_altrange(bnd.first);
							}
							rtear &= rtesegar;
							if (rtear.is_empty())
								break;
							v = u;
						}
						BidirAltRange::set_t arx(ar[dir]);
						arx -= rtear;
						if (m_trace) {
							std::ostringstream oss;
							oss << "Route " << *ident[dir] << " -> " << *ident[!dir] << ' ' << ar[dir].to_str()
							    << std::fixed << std::setprecision(1) << " dist " << distances[vd[!dir]]
							    << " DCT dist " << m_dist << " -> " << arx.to_str();
							message(oss.str(), Message::type_trace, bnd.first);
						}
						if (arx == ar[dir])
							break;
						ar[dir] = arx;
					}
				}
				{
					DctLeg::Alt a1(ar);
					a1.get_timetable().push_back(*tti);
					DctLeg::add(altset1, a1);
				}
			}
		}
		altset.swap(altset1);
		if (altset.empty())
			return DctLeg::altset_t();
		break;
	}
	if (m_trace) {
		std::ostringstream oss;
		oss << "Result: " << *ident[0] << '-' << *ident[1] << std::fixed << std::setprecision(1)
		    << " dist " << m_dist << " maxdist " << get_param().get_maxdist() << (isdctseg ? " (DCT)" : "")
		    << " time " << Glib::TimeVal(get_time(), 0).as_iso8601() << '-'
		    << Glib::TimeVal(get_endtime(), 0).as_iso8601() << std::endl;
		for (DctLeg::altset_t::const_iterator ai(altset.begin()), ae(altset.end()); ai != ae; ++ai)
			ai->get_timetable().print(oss << "  " << ai->get_altrange()[0].to_str() << " / "
						  << ai->get_altrange()[1].to_str(), 4) << std::endl;
		message(oss.str(), Message::type_trace, get_time());
	}
	return altset;
}

DctParameters::CalcMT::CalcMT(const DctParameters& param, const Object::ptr_t& p0, const Object::ptr_t& p1, const timeset_t& tdisc, bool all, bool verbose, bool trace)
	: Calc(param, p0, p1, tdisc, all, verbose, trace), m_thread(0), m_done(0)
{
}

DctParameters::CalcMT::CalcMT(const Calc& x)
	: Calc(x), m_thread(0), m_done(0)
{
}

void DctParameters::CalcMT::run(void)
{
	if (m_thread)
		return;
	m_thread = Glib::Threads::Thread::create(sigc::mem_fun(*this, &CalcMT::do_run));
}

void DctParameters::CalcMT::join(void)
{
	if (!m_thread)
		return;
	m_thread->join();
	m_thread = 0;
}

void DctParameters::CalcMT::do_run(void)
{
	Calc::run();
	{
		Glib::Threads::Mutex::Lock lock(get_param().get_mutex());
		set_done();
	}
	get_param().signal();
}

FlightRestriction::FlightRestriction(const UUID& uuid)
	: objbase_t(uuid)
{
}

bool FlightRestriction::evaluate(RestrictionEval& tfrs, RestrictionResult& result) const
{
	const FlightRestrictionTimeSlice& ts(operator()(tfrs.get_departuretime()).as_flightrestriction());
	if (!ts.is_valid())
		return true;
	return ts.evaluate(tfrs, result);
}

bool FlightRestriction::is_keep(void) const
{
	for (unsigned int i(0), n(size()); i < n; ++i) {
		const FlightRestrictionTimeSlice& ts(operator[](i).as_flightrestriction());
		if (!ts.is_valid())
			continue;
		if (ts.is_keep())
			return true;
	}
	return false;
}

FlightRestriction::ptr_t FlightRestriction::simplify(void) const
{
	ptr_t p;
	for (unsigned int i(0), n(size()); i < n; ++i) {
		const FlightRestrictionTimeSlice& ts(operator[](i).as_flightrestriction());
		if (!ts.is_valid())
			continue;
		if (!ts.get_condition())
			continue;
		Condition::ptr_t pc(ts.get_condition()->simplify());
		if (!pc)
			continue;
		if (!p)
			p = clone_obj();
		FlightRestrictionTimeSlice& tsn(p->operator[](i).as_flightrestriction());
		if (!tsn.is_valid())
			continue;
		tsn.set_condition(pc);
	}
	return p;
}

FlightRestriction::ptr_t FlightRestriction::simplify_bbox(const Rect& bbox) const
{
	ptr_t p(clone_obj());
	bool modified(false);
	for (unsigned int i(0), n(p->size()); i < n; ++i) {
		FlightRestrictionTimeSlice& ts(p->operator[](i).as_flightrestriction());
		if (!ts.is_valid())
			continue;
		if (ts.get_condition()) {
			Condition::ptr_t pc(ts.get_condition()->simplify_bbox(bbox, ts.get_timeinterval()));
			if (pc) {
				ts.set_condition(pc);
				modified = true;
			}
		}
		modified = ts.get_restrictions().simplify_bbox(bbox, ts.get_timeinterval()) || modified;
	}
	if (modified)
		return p;
	return ptr_t();
}

FlightRestriction::ptr_t FlightRestriction::simplify_altrange(int32_t minalt, int32_t maxalt) const
{
	ptr_t p(clone_obj());
	bool modified(false);
	for (unsigned int i(0), n(p->size()); i < n; ++i) {
		FlightRestrictionTimeSlice& ts(p->operator[](i).as_flightrestriction());
		if (!ts.is_valid())
			continue;
		if (ts.get_condition()) {
			Condition::ptr_t pc(ts.get_condition()->simplify_altrange(minalt, maxalt, ts.get_timeinterval()));
			if (pc) {
				ts.set_condition(pc);
				modified = true;
			}
		}
		if (ts.get_type() == FlightRestrictionTimeSlice::type_forbidden)
			modified = ts.get_restrictions().simplify_altrange(minalt, maxalt, ts.get_timeinterval()) || modified;
	}
	if (modified)
		return p;
	return ptr_t();
}

FlightRestriction::ptr_t FlightRestriction::simplify_aircrafttype(const std::string& acfttype) const
{
	ptr_t p;
	for (unsigned int i(0), n(size()); i < n; ++i) {
		const FlightRestrictionTimeSlice& ts(operator[](i).as_flightrestriction());
		if (!ts.is_valid())
			continue;
		if (!ts.get_condition())
			continue;
		Condition::ptr_t pc(ts.get_condition()->simplify_aircrafttype(acfttype));
		if (!pc)
			continue;
		if (!p)
			p = clone_obj();
		FlightRestrictionTimeSlice& tsn(p->operator[](i).as_flightrestriction());
		if (!tsn.is_valid())
			continue;
		tsn.set_condition(pc);
	}
	return p;
}

FlightRestriction::ptr_t FlightRestriction::simplify_aircraftclass(const std::string& acftclass) const
{
	ptr_t p;
	for (unsigned int i(0), n(size()); i < n; ++i) {
		const FlightRestrictionTimeSlice& ts(operator[](i).as_flightrestriction());
		if (!ts.is_valid())
			continue;
		if (!ts.get_condition())
			continue;
		Condition::ptr_t pc(ts.get_condition()->simplify_aircraftclass(acftclass));
		if (!pc)
			continue;
		if (!p)
			p = clone_obj();
		FlightRestrictionTimeSlice& tsn(p->operator[](i).as_flightrestriction());
		if (!tsn.is_valid())
			continue;
		tsn.set_condition(pc);
	}
	return p;
}

FlightRestriction::ptr_t FlightRestriction::simplify_equipment(const std::string& equipment, Aircraft::pbn_t pbn) const
{
	ptr_t p;
	for (unsigned int i(0), n(size()); i < n; ++i) {
		const FlightRestrictionTimeSlice& ts(operator[](i).as_flightrestriction());
		if (!ts.is_valid())
			continue;
		if (!ts.get_condition())
			continue;
		Condition::ptr_t pc(ts.get_condition()->simplify_equipment(equipment, pbn));
		if (!pc)
			continue;
		if (!p)
			p = clone_obj();
		FlightRestrictionTimeSlice& tsn(p->operator[](i).as_flightrestriction());
		if (!tsn.is_valid())
			continue;
		tsn.set_condition(pc);
	}
	return p;
}

FlightRestriction::ptr_t FlightRestriction::simplify_typeofflight(char type_of_flight) const
{
	ptr_t p;
	for (unsigned int i(0), n(size()); i < n; ++i) {
		const FlightRestrictionTimeSlice& ts(operator[](i).as_flightrestriction());
		if (!ts.is_valid())
			continue;
		if (!ts.get_condition())
			continue;
		Condition::ptr_t pc(ts.get_condition()->simplify_typeofflight(type_of_flight));
		if (!pc)
			continue;
		if (!p)
			p = clone_obj();
		FlightRestrictionTimeSlice& tsn(p->operator[](i).as_flightrestriction());
		if (!tsn.is_valid())
			continue;
		tsn.set_condition(pc);
	}
	return p;
}

FlightRestriction::ptr_t FlightRestriction::simplify_mil(bool mil) const
{
	ptr_t p;
	for (unsigned int i(0), n(size()); i < n; ++i) {
		const FlightRestrictionTimeSlice& ts(operator[](i).as_flightrestriction());
		if (!ts.is_valid())
			continue;
		if (!ts.get_condition())
			continue;
		Condition::ptr_t pc(ts.get_condition()->simplify_mil(mil));
		if (!pc)
			continue;
		if (!p)
			p = clone_obj();
		FlightRestrictionTimeSlice& tsn(p->operator[](i).as_flightrestriction());
		if (!tsn.is_valid())
			continue;
		tsn.set_condition(pc);
	}
	return p;
}

FlightRestriction::ptr_t FlightRestriction::simplify_dep(const Link& arpt) const
{
	ptr_t p;
	for (unsigned int i(0), n(size()); i < n; ++i) {
		const FlightRestrictionTimeSlice& ts(operator[](i).as_flightrestriction());
		if (!ts.is_valid())
			continue;
		if (!ts.get_condition())
			continue;
		Condition::ptr_t pc(ts.get_condition()->simplify_dep(arpt, ts.get_timeinterval()));
		if (!pc)
			continue;
		if (!p)
			p = clone_obj();
		FlightRestrictionTimeSlice& tsn(p->operator[](i).as_flightrestriction());
		if (!tsn.is_valid())
			continue;
		tsn.set_condition(pc);
	}
	return p;
}

FlightRestriction::ptr_t FlightRestriction::simplify_dest(const Link& arpt) const
{
	ptr_t p;
	for (unsigned int i(0), n(size()); i < n; ++i) {
		const FlightRestrictionTimeSlice& ts(operator[](i).as_flightrestriction());
		if (!ts.is_valid())
			continue;
		if (!ts.get_condition())
			continue;
		Condition::ptr_t pc(ts.get_condition()->simplify_dest(arpt, ts.get_timeinterval()));
		if (!pc)
			continue;
		if (!p)
			p = clone_obj();
		FlightRestrictionTimeSlice& tsn(p->operator[](i).as_flightrestriction());
		if (!tsn.is_valid())
			continue;
		tsn.set_condition(pc);
	}
	return p;
}

FlightRestriction::ptr_t FlightRestriction::simplify_complexity(void) const
{
	reference();
	ptr_t p(const_cast<FlightRestriction *>(this));
	bool modified(false);
	{
		ptr_t p1(p->simplify_complexity_crossingpoints());
		if (p1) {
			p.swap(p1);
			modified = true;
		}
	}
	{
		ptr_t p1(p->simplify_complexity_crossingsegments());
		if (p1) {
			p.swap(p1);
			modified = true;
		}
	}	{
		ptr_t p1(p->simplify_complexity_closedairspace());
		if (p1) {
			p.swap(p1);
			modified = true;
		}
	}
	if (modified)
		return p;
	return ptr_t();
}

FlightRestriction::ptr_t FlightRestriction::simplify_conditionalavailability(const Graph& g, const ConditionalAvailability& condavail,
									     const TimeTableSpecialDateEval& tte, const timepair_t& tm) const
{
	ptr_t p;
	for (unsigned int i(0), n(size()); i < n; ++i) {
		const FlightRestrictionTimeSlice& ts(operator[](i).as_flightrestriction());
		if (!ts.is_valid())
			continue;
		if (!ts.get_condition())
			continue;
		Condition::ptr_t pc(ts.get_condition()->simplify_conditionalavailability(g, condavail, tte, tm));
		if (!pc)
			continue;
		if (!p)
			p = clone_obj();
		FlightRestrictionTimeSlice& tsn(p->operator[](i).as_flightrestriction());
		if (!tsn.is_valid())
			continue;
		tsn.set_condition(pc);
	}
	return p;
}

FlightRestrictionTimeSlice::FlightRestrictionTimeSlice(timetype_t starttime, timetype_t endtime)
	: IdentTimeSlice(starttime, endtime), m_type(type_invalid), m_procind(procind_invalid), m_enabled(false), m_trace(false)
{
}

timeset_t FlightRestrictionTimeSlice::timediscontinuities(void) const
{
	timeset_t r(IdentTimeSlice::timediscontinuities());
	timeset_t r2(get_timetable().timediscontinuities());
	constraintimediscontinuities(r2);
	r.insert(r2.begin(), r2.end());
	return r;
}

FlightRestrictionTimeSlice& FlightRestrictionTimeSlice::as_flightrestriction(void)
{
	if (this)
		return *this;
	return const_cast<FlightRestrictionTimeSlice&>(FlightRestriction::invalid_timeslice);
}

const FlightRestrictionTimeSlice& FlightRestrictionTimeSlice::as_flightrestriction(void) const
{
	if (this)
		return *this;
	return FlightRestriction::invalid_timeslice;
}

void FlightRestrictionTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<FlightRestrictionTimeSlice *>(this))->hibernate(gen);	
}

void FlightRestrictionTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<FlightRestrictionTimeSlice *>(this))->hibernate(gen);
}

const std::string& to_str(ADR::FlightRestrictionTimeSlice::type_t t)
{
	switch (t) {
	case ADR::FlightRestrictionTimeSlice::type_allowed:
	{
		static const std::string r("allowed");
		return r;
	}

	case ADR::FlightRestrictionTimeSlice::type_closed:
	{
		static const std::string r("closed");
		return r;
	}

	case ADR::FlightRestrictionTimeSlice::type_mandatory:
	{
		static const std::string r("mandatory");
		return r;
	}

	case ADR::FlightRestrictionTimeSlice::type_forbidden:
	{
		static const std::string r("forbidden");
		return r;
	}

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

const std::string& to_str(ADR::FlightRestrictionTimeSlice::procind_t p)
{
	switch (p) {
	case ADR::FlightRestrictionTimeSlice::procind_tfr:
	{
		static const std::string r("tfr");
		return r;
	}

	case ADR::FlightRestrictionTimeSlice::procind_raddct:
	{
		static const std::string r("raddct");
		return r;
	}

	case ADR::FlightRestrictionTimeSlice::procind_fradct:
	{
		static const std::string r("fradct");
		return r;
	}

	case ADR::FlightRestrictionTimeSlice::procind_fpr:
	{
		static const std::string r("fpr");
		return r;
	}

	case ADR::FlightRestrictionTimeSlice::procind_adcp:
	{
		static const std::string r("adcp");
		return r;
	}

	case ADR::FlightRestrictionTimeSlice::procind_adfltrule:
	{
		static const std::string r("adfltrule");
		return r;
	}

	case ADR::FlightRestrictionTimeSlice::procind_fltprop:
	{
		static const std::string r("fltprop");
		return r;
	}

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

char FlightRestrictionTimeSlice::get_type_char(type_t t)
{
	switch (t) {
	default:
	case type_invalid:
		return '-';

	case type_mandatory:
		return 'M';

	case type_allowed:
		return 'A';

	case type_forbidden:
		return 'F';

	case type_closed:
		return 'C';
	}
}

void FlightRestrictionTimeSlice::clone(void)
{
	if (get_condition())
		set_condition(get_condition()->clone());
	get_restrictions().clone();
}

bool FlightRestrictionTimeSlice::recompute(void)
{
	Rect newbbox(Point::invalid, Point::invalid);
	if (get_condition())
		get_condition()->add_bbox(newbbox, *this);
	get_restrictions().add_bbox(newbbox, *this);
	bool work(newbbox != m_bbox);
	m_bbox = newbbox;
	return work;
}

bool FlightRestrictionTimeSlice::recompute(Database& db)
{
	bool work(false);
	if (get_condition()) {
		Condition::ptr_t p(get_condition()->recompute(db, get_timeinterval()));
		if (p) {
			set_condition(p);
			work = true;
		}
	}
	return get_restrictions().recompute(db, get_timeinterval()) || work;
}

std::ostream& FlightRestrictionTimeSlice::print(std::ostream& os) const
{
	IdentTimeSlice::print(os);
	os << ' ' << get_type() << ' ' << get_procind();
	if (is_dct() || is_strict_dct() || is_unconditional() || is_routestatic() || is_mandatoryinbound() || is_mandatoryoutbound() ||
	    !is_enabled() || is_trace() || is_keep()) {
		os << " (";
		if (!is_enabled())
			os << '-';
		if (is_trace())
			os << 'T';
		{
			civmil_t civmil;
			if (is_strict_dct(civmil))
				os << 'D';
			else if (is_dct(civmil))
				os << 'd';
			else
				civmil = Condition::civmil_invalid;
			switch (civmil) {
			case Condition::civmil_civ:
				os << 'c';
				break;

			case Condition::civmil_mil:
				os << 'm';
				break;

			default:
				break;
			}
		}
		if (is_unconditional())
			os << 'U';
		if (is_routestatic())
			os << 'S';
		if (is_mandatoryinbound())
			os << 'I';
		if (is_mandatoryoutbound())
			os << 'O';
		if (is_keep())
			os << 'K';
		os << ')';
	}
	if (!get_instruction().empty())
		os << std::endl << "    instruction " << get_instruction();
	get_timetable().print(os, 4);
	if (get_condition())
		get_condition()->print(os, 4, *this);
	get_restrictions().print(os, 4, *this);
	{
		std::set<Link> dep, dest;
		if (is_deparr(dep, dest)) {
			for (std::set<Link>::const_iterator i(dep.begin()), e(dep.end()); i != e; ++i) {
				os << std::endl << "    VFR only airport dep " << *i;
				const AirportTimeSlice& tsa(i->get_obj()->operator()(*this).as_airport());
				if (tsa.is_valid())
					os << ' ' << tsa.get_ident();
			}
			for (std::set<Link>::const_iterator i(dest.begin()), e(dest.end()); i != e; ++i) {
				os << std::endl << "    VFR only airport arr " << *i;
				const AirportTimeSlice& tsa(i->get_obj()->operator()(*this).as_airport());
				if (tsa.is_valid())
					os << ' ' << tsa.get_ident();
			}
		}
	}
	{
		Link arpt;
		bool arr;
		double dist;
		dctconnpoints_t connpt;
		civmil_t civmil;
		if (is_deparr_dct(arpt, arr, dist, connpt, civmil)) {
			os << std::endl << "    Airport DCT limit " << (arr ? "arr" : "dep") << ' ' << arpt;
			const AirportTimeSlice& tsa(arpt.get_obj()->operator()(*this).as_airport());
			if (tsa.is_valid())
				os << ' ' << tsa.get_ident();
			os << " dctlimit " << dist;
			switch (civmil) {
			case Condition::civmil_civ:
				os << " CIV";
				break;

			case Condition::civmil_mil:
				os << " MIL";
				break;

			default:
				break;
			}
			for (dctconnpoints_t::const_iterator ci(connpt.begin()), ce(connpt.end()); ci != ce; ++ci) {
				os << std::endl << "      Conn " << ci->first;
				const PointIdentTimeSlice& tsp(ci->first.get_obj()->operator()(*this).as_point());
				if (tsp.is_valid())
					os << ' ' << tsp.get_ident();
				os << " alt " << ci->second.to_str();
			}
		}
	}
	{
		Link aspc;
		AltRange alt;
		double dist;
   		civmil_t civmil;
		if (is_enroute_dct(aspc, alt, dist, civmil)) {
			os << std::endl << "    Enroute DCT limit " << aspc;
			const AirspaceTimeSlice& tsa(aspc.get_obj()->operator()(*this).as_airspace());
			if (tsa.is_valid())
				os << ' ' << tsa.get_ident();
			os << ' ' << alt.to_str() << " dctlimit " << dist;
			switch (civmil) {
			case Condition::civmil_civ:
				os << " CIV";
				break;

			case Condition::civmil_mil:
				os << " MIL";
				break;

			default:
				break;
			}
		}
	}
	{
		std::vector<RuleSegment> cc(get_crossingcond());
		if (!cc.empty()) {
			bool subseq(false);
			os << std::endl << "    crossing cond: ";
			for (std::vector<RuleSegment>::const_iterator ci(cc.begin()), ce(cc.end()); ci != ce; ++ci) {
				if (subseq)
					os << " / ";
				subseq = true;
				os << ci->to_shortstr(get_starttime());
			}
		}
	}
	{
		std::vector<RuleSequence> m(get_mandatory());
		if (!m.empty()) {
			bool subseq(false);
			os << std::endl << "    mandatory: ";
			for (std::vector<RuleSequence>::const_iterator mi(m.begin()), me(m.end()); mi != me; ++mi) {
				if (subseq)
					os << " / ";
				subseq = true;
				os << mi->to_shortstr(get_starttime());
			}
		}
	}
	{
		std::vector<RuleSegment> fs(get_forbiddensegments());
		if (!fs.empty()) {
			bool subseq(false);
			os << std::endl << "    forbidden segments: ";
			for (std::vector<RuleSegment>::const_iterator si(fs.begin()), se(fs.end()); si != se; ++si) {
				if (subseq)
					os << " / ";
				subseq = true;
				os << si->to_shortstr(get_starttime());
			}
		}
	}
	return os;
}

bool FlightRestrictionTimeSlice::evaluate(RestrictionEval& tfrs, RestrictionResult& result) const
{
	if (true && !is_enabled())
		tfrs.message("Rule disabled", Message::type_info);
       	CondResult r(false);
	if (get_condition()) {
		if (is_trace()) {
			tfrs.message("Condition: " + get_condition()->to_str(tfrs.get_departuretime()),
				     Message::type_trace, tfrs.get_departuretime());
			r = get_condition()->evaluate_trace(tfrs, 0);
		} else {
			r = get_condition()->evaluate(tfrs);
		}
	}
	if (r.get_result() == boost::logic::indeterminate) {
		tfrs.message("Cannot evaluate condition", Message::type_warning, tfrs.get_departuretime());
		return true;
	}
	if (r.get_result() != true)
		return true;
	if (r.has_refloc() && r.get_refloc() < tfrs.wpt_size()) {
		const RestrictionEval::Waypoint& wpt(tfrs.wpt(r.get_refloc()));
		TimeTableEval tte(wpt.get_time_unix(), wpt.get_coord(), tfrs.get_specialdateeval());
		if (!get_timetable().is_inside(tte)) {
			if (is_trace())
				tfrs.message("Reference time " + tte.to_str() + " outside applicability window",
					     Message::type_trace, tte.get_time());
			return true;
		}
	}
	timetype_t tm(tfrs.get_departuretime());
	if (!is_inside(tm))
		tm = get_starttime();
	result = RestrictionResult(tfrs.get_currule(), tm, r.get_vertexset(), r.get_edgeset(), r.get_refloc());
	if (get_type() == type_closed) {
		if (!get_restrictions().empty()) 
		        tfrs.message("Rule closed for cruising has Tre elements", Message::type_warning, tfrs.get_departuretime());
		return !is_enabled();
	}
	bool ok(false);
	if (is_trace())
		ok = get_restrictions().evaluate_trace(tfrs, result, get_type() == type_forbidden);
	else
		ok = get_restrictions().evaluate(tfrs, result, get_type() == type_forbidden);
	if (ok)
		return true;
	if (is_trace())
		tfrs.message("Rule failed", Message::type_trace, tm);
	return !is_enabled();
}

BidirAltRange FlightRestrictionTimeSlice::evaluate_dct(DctParameters::Calc& dct, bool trace) const
{
	if (is_trace())
		trace = true;
	BidirAltRange r;
	if (!is_enabled()) {
		r = dct.get_alt();
		if (trace)
			dct.message("Result: " + r[0].to_str() + " / " + r[1].to_str() + " (disabled)", Message::type_trace, dct.get_time());
		return r;
	}
	r.set_empty();
	if (get_condition()) {
		if (trace) {
			std::ostringstream oss;
			oss << dct.get_ident(0) << ' ' << dct.get_ident(1) << ' '
			    << dct.get_alt(0).to_str() << " / " << dct.get_alt(1).to_str()
			    << " D" << std::fixed << std::setprecision(1) << dct.get_dist()
			    << " Condition: " + get_condition()->to_str(dct.get_time());
			dct.message(oss.str(), Message::type_trace, dct.get_time());
			r = get_condition()->evaluate_dct_trace(dct, 0);
		} else {
			r = get_condition()->evaluate_dct(dct);
		}
	}
	r.invert();
	if (false && trace)
		dct.message("Inverted Condition: " + r[0].to_str() + " / " + r[1].to_str(), Message::type_trace, dct.get_time());
	switch (get_type()) {
	case type_closed:
		if (!get_restrictions().empty()) 
		        dct.message("Rule closed for cruising has Tre elements", Message::type_warning, dct.get_time());
		break;

	case type_allowed:
	case type_forbidden:
	case type_mandatory:
	{
		BidirAltRange ar;
		if (trace)
			ar = get_restrictions().evaluate_dct_trace(dct, get_type() == type_forbidden);
		else
			ar = get_restrictions().evaluate_dct(dct, get_type() == type_forbidden);
		r |= ar;
		break;
	}

	default:
		r.set_full();
		break;
	}
	if (false && trace)
		dct.message("After Restrictions: " + r[0].to_str() + " / " + r[1].to_str(), Message::type_trace, dct.get_time());
	r &= dct.get_alt();
	if (trace)
		dct.message("Result: " + r[0].to_str() + " / " + r[1].to_str(), Message::type_trace, dct.get_time());
	return r;
}

void FlightRestrictionTimeSlice::get_dct_segments(DctSegments& seg) const
{
	get_restrictions().get_dct_segments(seg);
}

bool FlightRestrictionTimeSlice::is_unconditional_airspace(void) const
{
	// heuristic: if airspace crossing is forbidden and condition for it is crossing the airspace, treat as unconditional
	if (get_restrictions().size() != 1 || get_restrictions()[0].size() != 1)
		return false;
	Glib::RefPtr<const ConditionCrossingAirspace1> c(Glib::RefPtr<const ConditionCrossingAirspace1>::cast_dynamic(get_condition()));
	if (!c)
		return false;
	if (false && get_ident().substr(0, 6) == "EGD201")
		std::cerr << "Rule " << get_ident() << ": is_unconditional_airspace: Alt Valid: L"
			  << (c->get_alt().is_lower_valid() ? '+' : '-')
			  << " U" << (c->get_alt().is_upper_valid() ? '+' : '-') << std::endl;
	if (c->get_alt().is_lower_valid() || c->get_alt().is_upper_valid())
		return false;
	Glib::RefPtr<const RestrictionElementAirspace> re(Glib::RefPtr<const RestrictionElementAirspace>::cast_dynamic(get_restrictions()[0][0]));
	if (!re)
		return false;
	if (false && get_ident().substr(0, 6) == "EGD201")
		std::cerr << "Rule " << get_ident() << ": is_unconditional_airspace: Cond " << c->get_airspace()
			  << " Restriction " << re->get_airspace() << std::endl;
	return (UUID)c->get_airspace() == (UUID)re->get_airspace();
}

bool FlightRestrictionTimeSlice::is_unconditional(void) const
{
	if (!get_condition())
		return false;
	if (get_condition()->is_constant() && get_condition()->get_constvalue())
		return true;
	if (is_unconditional_airspace())
		return true;
	return false;
}

bool FlightRestrictionTimeSlice::is_dct(void) const
{
	civmil_t civmil;
	return is_dct(civmil);
}

bool FlightRestrictionTimeSlice::is_strict_dct(void) const
{
	civmil_t civmil;
	return is_strict_dct(civmil);
}

bool FlightRestrictionTimeSlice::is_dct(civmil_t& civmil) const
{
	civmil = Condition::civmil_invalid;
	if (!get_condition())
		return false;
	if (false && get_ident() == "LS1A")
		std::cerr << get_ident() << ": check condition DCT" << std::endl;
	if (false && !get_condition()->is_dct())
		return false;
	if (false && get_ident() == "LS1A")
		std::cerr << get_ident() << ": check condition valid DCT" << std::endl;
	if (!get_condition()->is_valid_dct(false, civmil))
		return false;
	if (false && get_ident() == "LS1A")
		std::cerr << get_ident() << ": check restriction valid DCT" << std::endl;
	if (!get_restrictions().is_valid_dct())
		return false;
	if (false && get_ident() == "LS1A")
		std::cerr << get_ident() << ": check is DCT" << std::endl;
	return true;
}

bool FlightRestrictionTimeSlice::is_strict_dct(civmil_t& civmil) const
{
	civmil = Condition::civmil_invalid;
	if (!get_condition())
		return false;
	if (false && get_ident() == "LS1A")
		std::cerr << get_ident() << ": check condition DCT" << std::endl;
	if (!get_condition()->is_dct())
		return false;
	if (false && get_ident() == "LS1A")
		std::cerr << get_ident() << ": check condition valid DCT" << std::endl;
	if (!get_condition()->is_valid_dct(false, civmil))
		return false;
	if (false && get_ident() == "LS1A")
		std::cerr << get_ident() << ": check restriction valid DCT" << std::endl;
	if (!get_restrictions().is_valid_dct())
		return false;
	if (false && get_ident() == "LS1A")
		std::cerr << get_ident() << ": check is DCT" << std::endl;
	return true;
}

bool FlightRestrictionTimeSlice::is_routestatic(void) const
{
	if (!get_condition())
		return true;
	return get_condition()->is_routestatic();
}

bool FlightRestrictionTimeSlice::is_routestatic(RuleSegment& seg) const
{
	if (!get_condition())
		return true;
	if (false) {
		Condition::routestatic_t rs(get_condition()->is_routestatic(seg));
		std::cerr << "is_routestatic: " << get_ident() << ": " << seg.to_shortstr(get_starttime())
			  << " -> " << (unsigned int)rs << std::endl;
		print(std::cerr) << std::endl;
		return rs != Condition::routestatic_nonstatic;
	}
	return get_condition()->is_routestatic(seg) != Condition::routestatic_nonstatic;
}

bool FlightRestrictionTimeSlice::is_keep(void) const
{
	if (!get_condition())
		return false;
	if (get_condition()->is_constant() && !get_condition()->get_constvalue())
		return false;
	switch (get_type()) {
	case type_mandatory:
	case type_forbidden:
		if (get_restrictions().empty())
			return false;
		break;

	case type_closed:
	case type_allowed:
		break;

	default:
		return false;
	}
	return true;
}

bool FlightRestrictionTimeSlice::is_deparr(std::set<Link>& dep, std::set<Link>& dest) const
{
	dep.clear();
	dest.clear();
	switch (get_type()) {
	case type_forbidden:
		break;

	default:
		return false;
	}
	if (!get_condition() || !get_restrictions().empty())
		return false;
	if (!get_condition()->is_deparr(dep, dest)) {
		dep.clear();
		dest.clear();
		return false;
	}
	return true;
}

bool FlightRestrictionTimeSlice::is_deparr_dct(Link& arpt, bool& arr, double& dist, dctconnpoints_t& connpt, civmil_t& civmil) const
{
	arpt = Link();
	arr = false;
	dist = std::numeric_limits<double>::quiet_NaN();
	connpt.clear();
	civmil = Condition::civmil_invalid;
	switch (get_type()) {
	case type_mandatory:
	case type_allowed:
		break;

	default:
		return false;
	}
	if (!get_condition())
		return false;
	if (!get_condition()->is_deparr_dct(arpt, arr, dist, civmil) ||
	    !get_restrictions().is_deparr_dct(arpt, arr, connpt)) {
		arpt = Link();
		arr = false;
		dist = std::numeric_limits<double>::quiet_NaN();
		connpt.clear();
		civmil = Condition::civmil_invalid;
		return false;
	}
	return true;
}

bool FlightRestrictionTimeSlice::is_deparr_dct(void) const
{
	Link arpt;
	bool arr;
	double dist;
	dctconnpoints_t connpt;
	civmil_t civmil;
	return is_deparr_dct(arpt, arr, dist, connpt, civmil);
}

bool FlightRestrictionTimeSlice::is_enroute_dct(Link& aspc, AltRange& alt, double& dist, civmil_t& civmil) const
{
	aspc = Link();
	alt = AltRange();
	dist = std::numeric_limits<double>::quiet_NaN();
	civmil = Condition::civmil_invalid;
	switch (get_type()) {
	case type_mandatory:
	case type_allowed:
		break;

	default:
		return false;
	}
	if (!get_condition())
		return false;
	if (!get_condition()->is_enroute_dct(aspc, alt, dist, civmil)) {
		aspc = Link();
		dist = std::numeric_limits<double>::quiet_NaN();
		civmil = Condition::civmil_invalid;
		return false;
	}
	return true;
}

std::vector<RuleSegment> FlightRestrictionTimeSlice::get_crossingcond(void) const
{
	std::vector<RuleSegment> r;
	Condition::const_ptr_t p(get_crossingcond(r));
	if (!p || !p->is_routestatic())
		return std::vector<RuleSegment>();
	return r;
}

Condition::const_ptr_t FlightRestrictionTimeSlice::get_crossingcond(std::vector<RuleSegment>& cc) const
{
	cc.clear();
	Condition::const_ptr_t p(get_condition());
	if (!p)
		return p;
	{
		std::vector<RuleSegment> r1;
		Condition::const_ptr_t p1(p->extract_crossingpoints(r1, get_timeinterval()));
		if (p1 && !r1.empty()) {
			cc.swap(r1);
			return p1;
		}
	}
	{
		std::vector<RuleSegment> r1;
		Condition::const_ptr_t p1(p->extract_crossingsegments(r1, get_timeinterval()));
		if (p1 && !r1.empty()) {
			cc.swap(r1);
			return p1;
		}
	}
	{
		std::vector<RuleSegment> r1;
		Condition::const_ptr_t p1(p->extract_crossingairspaces(r1));
		if (p1 && !r1.empty()) {
			cc.swap(r1);
			return p1;
		}
	}
	return p;
}

std::vector<RuleSequence> FlightRestrictionTimeSlice::get_mandatory(void) const
{
	if (get_type() != type_mandatory || !get_condition())
		return std::vector<RuleSequence>();
	return get_condition()->get_mandatory();
}

std::vector<RuleSegment> FlightRestrictionTimeSlice::get_forbiddensegments(void) const
{
	if (get_type() != type_forbidden && get_type() != type_closed)
		return std::vector<RuleSegment>();
	if (get_type() == type_forbidden && is_routestatic()) {
		std::vector<RuleSegment> segs;
		for (unsigned int i(0), n(get_restrictions().size()); i < n; ++i) {
			RuleSequence seq(get_restrictions()[i].get_rule());
			if (seq.size() != 1)
				continue;
			segs.push_back(seq.front());
		}
		return segs;
	}
	std::vector<RuleSegment> cc(get_crossingcond());
	if (cc.empty())
		return std::vector<RuleSegment>();
	if (get_type() == type_closed)
		return cc;
	std::vector<RuleSegment> segs;
	for (unsigned int i(0), n(get_restrictions().size()); i < n; ++i) {
		RuleSequence seq(get_restrictions()[i].get_rule());
		if (seq.size() != 1)
			continue;
		bool ok(false);
		for (std::vector<RuleSegment>::const_iterator ci(cc.begin()), ce(cc.end()); ci != ce; ++ci) {
			if (ci->get_type() == seq.front().get_type()) {
				switch (seq.front().get_type()) {
				case RuleSegment::type_point:
				case RuleSegment::type_airspace:
					ok = ci->get_uuid0() == seq.front().get_uuid0();
					// fall through

				case RuleSegment::type_airway:
				case RuleSegment::type_dct:
					ok = ok && ci->get_uuid1() == seq.front().get_uuid1()
						&& ci->get_airway_uuid() == seq.front().get_airway_uuid();
					break;

				default:
					break;
				}
			} else if (ci->get_type() == RuleSegment::type_point &&
				   (seq.front().get_type() == RuleSegment::type_airway ||
				    seq.front().get_type() == RuleSegment::type_dct)) {
				ok = ci->get_uuid0() == seq.front().get_uuid0() || ci->get_uuid0() == seq.front().get_uuid1();
			}
			if (!ok)
				continue;
			seq.front().get_alt().intersect(ci->get_alt());
			break;
		}
		if (!ok)
			continue;
		segs.push_back(seq.front());
	}
	return segs;
}
