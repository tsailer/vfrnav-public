#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/filtered_graph.hpp>

#include <iomanip>

#include "adraupparse.hh"

using namespace ADR;

const std::string& to_str(ADR::ParseAUP::AUPObject::type_t t)
{
	switch (t) {
	case ADR::ParseAUP::AUPObject::type_invalid:
	{
		static const std::string r("invalid");
		return r;
	}

	case ADR::ParseAUP::AUPObject::type_routesegment:
	{
		static const std::string r("routesegment");
		return r;
	}

	case ADR::ParseAUP::AUPObject::type_route:
	{
		static const std::string r("route");
		return r;
	}

	case ADR::ParseAUP::AUPObject::type_navaid:
	{
		static const std::string r("navaid");
		return r;
	}

	case ADR::ParseAUP::AUPObject::type_designatedpoint:
	{
		static const std::string r("designatedpoint");
		return r;
	}

	case ADR::ParseAUP::AUPObject::type_airspace:
	{
		static const std::string r("airspace");
		return r;
	}

	default:
	{
		static const std::string r("?""?");
		return r;
	}
	}
}

ParseAUP::AUPTimeSlice::AUPTimeSlice(timetype_t starttime, timetype_t endtime)
	: m_time(timepair_t(starttime, endtime))
{
}

bool ParseAUP::AUPTimeSlice::is_overlap(uint64_t tm0, uint64_t tm1) const
{
	if (!is_valid() || !(tm0 < tm1))
		return false;
	return tm1 > get_starttime() && tm0 < get_endtime();
}

uint64_t ParseAUP::AUPTimeSlice::get_overlap(uint64_t tm0, uint64_t tm1) const
{
	if (!is_valid() || !(tm0 < tm1))
		return 0;
	if (tm0 > get_starttime()) {
		if (tm0 >= get_endtime())
			return 0;
		return std::min(get_endtime(), tm1) - tm0;
	}
	if (get_starttime() >= tm1)
		return 0;
	return std::min(get_endtime(), tm1) - get_starttime();
}

ParseAUP::AUPRouteSegmentTimeSlice& ParseAUP::AUPTimeSlice::as_routesegment(void)
{
	return const_cast<AUPRouteSegmentTimeSlice&>(AUPRouteSegment::invalid_timeslice);
}

const ParseAUP::AUPRouteSegmentTimeSlice& ParseAUP::AUPTimeSlice::as_routesegment(void) const
{
	return AUPRouteSegment::invalid_timeslice;
}

ParseAUP::AUPIdentTimeSlice& ParseAUP::AUPTimeSlice::as_ident(void)
{
	return const_cast<AUPIdentTimeSlice&>(AUPIdentTimeSlice::invalid_timeslice);
}

const ParseAUP::AUPIdentTimeSlice& ParseAUP::AUPTimeSlice::as_ident(void) const
{
	return AUPIdentTimeSlice::invalid_timeslice;
}

ParseAUP::AUPRouteTimeSlice& ParseAUP::AUPTimeSlice::as_route(void)
{
	return const_cast<AUPRouteTimeSlice&>(AUPRoute::invalid_timeslice);
}

const ParseAUP::AUPRouteTimeSlice& ParseAUP::AUPTimeSlice::as_route(void) const
{
	return AUPRoute::invalid_timeslice;
}

ParseAUP::AUPDesignatedPointTimeSlice& ParseAUP::AUPTimeSlice::as_designatedpoint(void)
{
	return const_cast<AUPDesignatedPointTimeSlice&>(AUPDesignatedPoint::invalid_timeslice);
}

const ParseAUP::AUPDesignatedPointTimeSlice& ParseAUP::AUPTimeSlice::as_designatedpoint(void) const
{
	return AUPDesignatedPoint::invalid_timeslice;
}

ParseAUP::AUPNavaidTimeSlice& ParseAUP::AUPTimeSlice::as_navaid(void)
{
	return const_cast<AUPNavaidTimeSlice&>(AUPNavaid::invalid_timeslice);
}

const ParseAUP::AUPNavaidTimeSlice& ParseAUP::AUPTimeSlice::as_navaid(void) const
{
	return AUPNavaid::invalid_timeslice;
}

ParseAUP::AUPAirspaceTimeSlice& ParseAUP::AUPTimeSlice::as_airspace(void)
{
	return const_cast<AUPAirspaceTimeSlice&>(AUPAirspace::invalid_timeslice);
}

const ParseAUP::AUPAirspaceTimeSlice& ParseAUP::AUPTimeSlice::as_airspace(void) const
{
	return AUPAirspace::invalid_timeslice;
}

std::ostream& ParseAUP::AUPTimeSlice::print(std::ostream& os) const
{
	if (get_starttime() == std::numeric_limits<uint64_t>::max()) {
		os << "unlimited           ";
	} else {
		Glib::TimeVal tv(get_starttime(), 0);
		os << tv.as_iso8601();
	}
	if (get_endtime() == std::numeric_limits<uint64_t>::max()) {
		os << " unlimited           ";
	} else {
		Glib::TimeVal tv(get_endtime(), 0);
		os << ' ' << tv.as_iso8601();
	}
	return os;
}

const ParseAUP::AUPTimeSlice ParseAUP::AUPObject::invalid_timeslice(0, 0);

ParseAUP::AUPObject::AUPObject(const std::string& id, const Link& link)
	: m_link(id, link), m_refcount(1)
{
}

ParseAUP::AUPObject::~AUPObject()
{
}

void ParseAUP::AUPObject::reference(void) const
{
        g_atomic_int_inc(&m_refcount);
}

void ParseAUP::AUPObject::unreference(void) const
{
        if (!g_atomic_int_dec_and_test(&m_refcount))
                return;
        delete this;
}

bool ParseAUP::AUPObject::empty(void) const
{
	if (this)
		return ts_empty();
	return true;
}

unsigned int ParseAUP::AUPObject::size(void) const
{
	if (this)
		return ts_size();
	return 0;
}

ParseAUP::AUPTimeSlice& ParseAUP::AUPObject::operator[](unsigned int idx)
{
	if (this)
		return ts_get(idx);
	return const_cast<ParseAUP::AUPTimeSlice&>(invalid_timeslice);
}

const ParseAUP::AUPTimeSlice& ParseAUP::AUPObject::operator[](unsigned int idx) const
{
	if (this)
		return ts_get(idx);
	return invalid_timeslice;
}

ParseAUP::AUPTimeSlice& ParseAUP::AUPObject::operator()(uint64_t tm)
{
	if (!this)
		return const_cast<ParseAUP::AUPTimeSlice&>(invalid_timeslice);
	for (unsigned int i(0), n(ts_size()); i < n; ++i) {
		ParseAUP::AUPTimeSlice& ts(ts_get(i));
		if (ts.is_inside(tm))
			return ts;
	}
	return const_cast<ParseAUP::AUPTimeSlice&>(invalid_timeslice);
}

const ParseAUP::AUPTimeSlice& ParseAUP::AUPObject::operator()(uint64_t tm) const
{
	if (!this)
		return invalid_timeslice;
	for (unsigned int i(0), n(ts_size()); i < n; ++i) {
		const ParseAUP::AUPTimeSlice& ts(ts_get(i));
		if (ts.is_inside(tm))
			return ts;
	}
	return invalid_timeslice;
}

ParseAUP::AUPTimeSlice& ParseAUP::AUPObject::operator()(uint64_t tm0, uint64_t tm1)
{
	if (!this)
		return const_cast<ParseAUP::AUPTimeSlice&>(invalid_timeslice);
	ParseAUP::AUPTimeSlice *tsr(&const_cast<ParseAUP::AUPTimeSlice&>(invalid_timeslice));
	uint64_t overlap(0);
	for (unsigned int i(0), n(ts_size()); i < n; ++i) {
		ParseAUP::AUPTimeSlice& ts(ts_get(i));
		uint64_t o(ts.get_overlap(tm0, tm1));
		if (o < overlap)
			continue;
		overlap = o;
		tsr = &ts;
	}
	return *tsr;
}

const ParseAUP::AUPTimeSlice& ParseAUP::AUPObject::operator()(uint64_t tm0, uint64_t tm1) const
{
	if (!this)
		return invalid_timeslice;
	const ParseAUP::AUPTimeSlice *tsr(&invalid_timeslice);
	uint64_t overlap(0);
	for (unsigned int i(0), n(ts_size()); i < n; ++i) {
		const ParseAUP::AUPTimeSlice& ts(ts_get(i));
		uint64_t o(ts.get_overlap(tm0, tm1));
		if (o < overlap)
			continue;
		overlap = o;
		tsr = &ts;
	}
	return *tsr;
}

ParseAUP::AUPTimeSlice& ParseAUP::AUPObject::operator()(const ParseAUP::AUPTimeSlice& ts)
{
	return this->operator()(ts.get_starttime(), ts.get_endtime());
}

const ParseAUP::AUPTimeSlice& ParseAUP::AUPObject::operator()(const ParseAUP::AUPTimeSlice& ts) const
{
	return this->operator()(ts.get_starttime(), ts.get_endtime());
}

ParseAUP::AUPTimeSlice& ParseAUP::AUPObject::ts_get(unsigned int idx)
{
	return const_cast<ParseAUP::AUPTimeSlice&>(invalid_timeslice);
}

const ParseAUP::AUPTimeSlice& ParseAUP::AUPObject::ts_get(unsigned int idx) const
{
	return invalid_timeslice;
}

std::ostream& ParseAUP::AUPObject::print(std::ostream& os) const
{
	os << get_id() << ' ' << to_str(get_type());
	if (!get_link().is_nil())
		os << ' ' << get_link();
	os << std::endl;
	for (unsigned int i(0), n(size()); i < n; ++i) {
		const AUPTimeSlice& ts(operator[](i));
		if (!ts.is_valid() && !ts.is_snapshot())
			continue;
		ts.print(os << "  ") << std::endl;
	}
	return os;
}

template<class TS>
const typename ParseAUP::AUPObjectImpl<TS>::TimeSliceImpl ParseAUP::AUPObjectImpl<TS>::invalid_timeslice(0, 0);

template<class TS>
ParseAUP::AUPObjectImpl<TS>::AUPObjectImpl(const std::string& id, const Link& link)
        : AUPObject(id, link)
{
}

template<class TS>
ParseAUP::AUPTimeSlice& ParseAUP::AUPObjectImpl<TS>::ts_get(unsigned int idx)
{
	if (this && idx < size())
		return m_timeslice[idx];
	return const_cast<TimeSliceImpl&>(invalid_timeslice);
}

template<class TS>
const ParseAUP::AUPTimeSlice& ParseAUP::AUPObjectImpl<TS>::ts_get(unsigned int idx) const
{
	if (this && idx < size())
		return m_timeslice[idx];
	return invalid_timeslice;
}

template<class TS>
class ParseAUP::AUPObjectImpl<TS>::SortTimeSlice {
public:
	bool operator()(const AUPTimeSlice& a, const AUPTimeSlice& b) const {
		if (a.get_starttime() < b.get_starttime())
			return true;
		if (a.get_starttime() > b.get_starttime())
			return false;
		return a.get_endtime() < b.get_endtime();
	}
};

template<class TS>
void ParseAUP::AUPObjectImpl<TS>::clean_timeslices(uint64_t cutoff)
{
	std::sort(m_timeslice.begin(), m_timeslice.end(), SortTimeSlice());
	if (false) {
		std::cerr << "Timeslices after sorting: " << size() << std::endl;
		for (typename timeslice_t::iterator i(m_timeslice.begin()), e(m_timeslice.end()); i != e; ++i)
			std::cerr << "  " << i->get_starttime() << ' ' << i->get_endtime() << std::endl;
	}
	for (typename timeslice_t::iterator i(m_timeslice.begin()), e(m_timeslice.end()); i != e; ) {
		typename timeslice_t::iterator i2(i);
		++i2;
		if (i->is_snapshot()) {
			i = i2;
			continue;
		}
		if (i2 != e && i->get_endtime() > i2->get_starttime() && !i2->is_snapshot()) {
			i->set_endtime(i2->get_starttime());
		}
		if (i->is_valid() && i->get_endtime() > cutoff) {
			i = i2;
			continue;
		}
		i = m_timeslice.erase(i);
		e = m_timeslice.end();
 
	}
	if (false) {
		std::cerr << "Timeslices after cleaning: " << size() << std::endl;
		for (typename timeslice_t::iterator i(m_timeslice.begin()), e(m_timeslice.end()); i != e; ++i)
			std::cerr << "  " << i->get_starttime() << ' ' << i->get_endtime() << std::endl;
	}
}

template<class TS>
void ParseAUP::AUPObjectImpl<TS>::add_timeslice(const TimeSliceImpl& ts)
{
	if (!ts.is_valid() && !ts.is_snapshot())
		return;
	if (false) {
		Glib::TimeVal tv0(ts.get_starttime(), 0);
		Glib::TimeVal tv1(ts.get_endtime(), 0);
		std::cerr << "add_timeslice: " << tv0.as_iso8601() << ' ' << tv1.as_iso8601()
			  << " (" << ts.get_starttime() << ' ' << ts.get_endtime() << ')' << std::endl;
	}
	if (!ts.is_snapshot()) {
		for (typename timeslice_t::iterator i(m_timeslice.begin()), e(m_timeslice.end()); i != e; ++i) {
			if (i->is_snapshot())
				continue;
			if (i->get_starttime() >= ts.get_starttime() && i->get_starttime() < ts.get_endtime())
				i->set_starttime(ts.get_endtime());
		}
	}
	m_timeslice.push_back(ts);
	clean_timeslices();
}

ParseAUP::AUPRouteSegmentTimeSlice::Availability::operator AUPCDR::Availability(void) const
{
	AUPCDR::Availability a;
	a.set_altrange(get_altrange());
	for (hostairspaces_t::const_iterator i(get_hostairspaces().begin()), e(get_hostairspaces().end()); i != e; ++i) {
		if (i->get_link().is_nil())
			continue;
		a.get_hostairspaces().push_back(i->get_link());
	}
	a.set_cdr(get_cdr());
	return a;
}

std::ostream& ParseAUP::AUPRouteSegmentTimeSlice::Availability::print(std::ostream& os, const timepair_t& tp) const
{
	os << " altitude " << m_altrange.to_str();
	if (get_cdr())
		os << " cdr" << get_cdr();
	for (unsigned int i(0), n(m_hostairspaces.size()); i < n; ++i) {
		const AUPLink& l(m_hostairspaces[i]);
		os << std::endl << "      host airspace " << l.get_id();
		if (l.get_link().is_nil())
			continue;
		os << ' ' << l.get_link();
		const AirspaceTimeSlice& ts(l.get_link().get_obj()->operator()(tp.first).as_airspace());
		if (!ts.is_valid())
			continue;
		os << ' ' << ts.get_ident() << ' ' << ts.get_type();
	}
	return os;
}

ParseAUP::AUPRouteSegmentTimeSlice::AUPRouteSegmentTimeSlice(timetype_t starttime, timetype_t endtime)
	: AUPTimeSlice(starttime, endtime)
{
}

ParseAUP::AUPRouteSegmentTimeSlice& ParseAUP::AUPRouteSegmentTimeSlice::as_routesegment(void)
{
	if (this)
		return *this;
	return const_cast<AUPRouteSegmentTimeSlice&>(AUPRouteSegment::invalid_timeslice);
}

const ParseAUP::AUPRouteSegmentTimeSlice& ParseAUP::AUPRouteSegmentTimeSlice::as_routesegment(void) const
{
	if (this)
		return *this;
	return AUPRouteSegment::invalid_timeslice;
}

std::ostream& ParseAUP::AUPRouteSegmentTimeSlice::print(std::ostream& os) const
{
	AUPTimeSlice::print(os);
	os << std::endl << "    start " << get_start().get_id();
	if (!get_start().get_link().is_nil()) {
		os << ' ' << get_start().get_link();
		const PointIdentTimeSlice& ts(get_start().get_link().get_obj()->operator()(get_starttime()).as_point());
		if (ts.is_valid())
			os << ' ' << ts.get_ident();
	}
	os << std::endl << "    end   " << get_end().get_id();
	if (!get_end().get_link().is_nil()) {
		os << ' ' << get_end().get_link();
		const PointIdentTimeSlice& ts(get_end().get_link().get_obj()->operator()(get_starttime()).as_point());
		if (ts.is_valid())
			os << ' ' << ts.get_ident();
	}
	os << std::endl << "    route " << get_route().get_id();
	if (!get_route().get_link().is_nil()) {
		os << ' ' << get_route().get_link();
		const RouteTimeSlice& ts(get_route().get_link().get_obj()->operator()(get_starttime()).as_route());
		if (ts.is_valid())
			os << ' ' << ts.get_ident();
	}
	for (unsigned int i(0), n(m_availability.size()); i < n; ++i)
		m_availability[i].print(os << std::endl << "    Availability[" << i << ']', get_timeinterval());
	for (unsigned int i(0), n(m_segments.size()); i < n; ++i)
		os << std::endl << "    segment " << m_segments[i] << (m_segments[i].is_forward() ? " forward" : " backward");
	return os;
}

ParseAUP::AUPRouteSegment::AUPRouteSegment(const std::string& id, const Link& link)
	: objbase_t(id, link)
{
}

template class ParseAUP::AUPObjectImpl<ParseAUP::AUPRouteSegmentTimeSlice>;

ParseAUP::AUPIdentTimeSlice::AUPIdentTimeSlice(timetype_t starttime, timetype_t endtime)
	: AUPTimeSlice(starttime, endtime)
{
}

ParseAUP::AUPIdentTimeSlice& ParseAUP::AUPIdentTimeSlice::as_ident(void)
{
	if (this)
		return *this;
	return const_cast<AUPIdentTimeSlice&>(AUPIdentTimeSlice::invalid_timeslice);
}

const ParseAUP::AUPIdentTimeSlice& ParseAUP::AUPIdentTimeSlice::as_ident(void) const
{
	if (this)
		return *this;
	return AUPIdentTimeSlice::invalid_timeslice;
}

std::ostream& ParseAUP::AUPIdentTimeSlice::print(std::ostream& os) const
{
	AUPTimeSlice::print(os);
	return os << " ident " << get_ident();
}

const ParseAUP::AUPIdentTimeSlice ParseAUP::AUPIdentTimeSlice::invalid_timeslice;

ParseAUP::AUPRouteTimeSlice::AUPRouteTimeSlice(timetype_t starttime, timetype_t endtime)
	: AUPIdentTimeSlice(starttime, endtime)
{
}

ParseAUP::AUPRouteTimeSlice& ParseAUP::AUPRouteTimeSlice::as_route(void)
{
	if (this)
		return *this;
	return const_cast<AUPRouteTimeSlice&>(AUPRoute::invalid_timeslice);
}

const ParseAUP::AUPRouteTimeSlice& ParseAUP::AUPRouteTimeSlice::as_route(void) const
{
	if (this)
		return *this;
	return AUPRoute::invalid_timeslice;
}

std::ostream& ParseAUP::AUPRouteTimeSlice::print(std::ostream& os) const
{
	AUPIdentTimeSlice::print(os);
	return os;
}

ParseAUP::AUPRoute::AUPRoute(const std::string& id, const Link& link)
	: objbase_t(id, link)
{
}

template class ParseAUP::AUPObjectImpl<ParseAUP::AUPRouteTimeSlice>;

ParseAUP::AUPDesignatedPointTimeSlice::AUPDesignatedPointTimeSlice(timetype_t starttime, timetype_t endtime)
	: AUPIdentTimeSlice(starttime, endtime), m_type(ADR::DesignatedPointTimeSlice::type_invalid)
{
}

ParseAUP::AUPDesignatedPointTimeSlice& ParseAUP::AUPDesignatedPointTimeSlice::as_designatedpoint(void)
{
	if (this)
		return *this;
	return const_cast<AUPDesignatedPointTimeSlice&>(AUPDesignatedPoint::invalid_timeslice);
}

const ParseAUP::AUPDesignatedPointTimeSlice& ParseAUP::AUPDesignatedPointTimeSlice::as_designatedpoint(void) const
{
	if (this)
		return *this;
	return AUPDesignatedPoint::invalid_timeslice;
}

std::ostream& ParseAUP::AUPDesignatedPointTimeSlice::print(std::ostream& os) const
{
	AUPIdentTimeSlice::print(os);
	return os << " type " << get_type();
}

ParseAUP::AUPDesignatedPoint::AUPDesignatedPoint(const std::string& id, const Link& link)
	: objbase_t(id, link)
{
}

template class ParseAUP::AUPObjectImpl<ParseAUP::AUPDesignatedPointTimeSlice>;

ParseAUP::AUPNavaidTimeSlice::AUPNavaidTimeSlice(timetype_t starttime, timetype_t endtime)
	: AUPIdentTimeSlice(starttime, endtime)
{
}

ParseAUP::AUPNavaidTimeSlice& ParseAUP::AUPNavaidTimeSlice::as_navaid(void)
{
	if (this)
		return *this;
	return const_cast<AUPNavaidTimeSlice&>(AUPNavaid::invalid_timeslice);
}

const ParseAUP::AUPNavaidTimeSlice& ParseAUP::AUPNavaidTimeSlice::as_navaid(void) const
{
	if (this)
		return *this;
	return AUPNavaid::invalid_timeslice;
}

std::ostream& ParseAUP::AUPNavaidTimeSlice::print(std::ostream& os) const
{
	AUPIdentTimeSlice::print(os);
	return os;
}

ParseAUP::AUPNavaid::AUPNavaid(const std::string& id, const Link& link)
	: objbase_t(id, link)
{
}

template class ParseAUP::AUPObjectImpl<ParseAUP::AUPNavaidTimeSlice>;

ParseAUP::AUPAirspaceTimeSlice::Activation::operator AUPRSA::Activation(void) const
{
	AUPRSA::Activation a;
	a.set_altrange(get_altrange());
	for (hostairspaces_t::const_iterator i(get_hostairspaces().begin()), e(get_hostairspaces().end()); i != e; ++i) {
		if (i->get_link().is_nil())
			continue;
		a.get_hostairspaces().push_back(i->get_link());
	}
	a.set_status(get_status());
	return a;
}

std::ostream& ParseAUP::AUPAirspaceTimeSlice::Activation::print(std::ostream& os, const timepair_t& tp) const
{
	os << " altitude " << m_altrange.to_str() << ' ' << get_status();
	for (unsigned int i(0), n(m_hostairspaces.size()); i < n; ++i) {
		const AUPLink& l(m_hostairspaces[i]);
		os << std::endl << "      host airspace " << l.get_id();
		if (l.get_link().is_nil())
			continue;
		os << ' ' << l.get_link();
		const AirspaceTimeSlice& ts(l.get_link().get_obj()->operator()(tp.first).as_airspace());
		if (!ts.is_valid())
			continue;
		os << ' ' << ts.get_ident() << ' ' << ts.get_type();
	}
	return os;
}

ParseAUP::AUPAirspaceTimeSlice::AUPAirspaceTimeSlice(timetype_t starttime, timetype_t endtime)
	: AUPIdentTimeSlice(starttime, endtime), m_type(ADR::AirspaceTimeSlice::type_invalid), m_flags(0)
{
}

ParseAUP::AUPAirspaceTimeSlice& ParseAUP::AUPAirspaceTimeSlice::as_airspace(void)
{
	if (this)
		return *this;
	return const_cast<AUPAirspaceTimeSlice&>(AUPAirspace::invalid_timeslice);
}

const ParseAUP::AUPAirspaceTimeSlice& ParseAUP::AUPAirspaceTimeSlice::as_airspace(void) const
{
	if (this)
		return *this;
	return AUPAirspace::invalid_timeslice;
}

std::ostream& ParseAUP::AUPAirspaceTimeSlice::print(std::ostream& os) const
{
	AUPIdentTimeSlice::print(os);
	os << ' ' << get_type() << (is_icao() ? " icao" : "") << (is_level(1) ? " level1" : "")
	   << (is_level(2) ? " level2" : "") << (is_level(3) ? " level3" : "");
	m_activation.print(os, get_timeinterval());
	return os;
}

ParseAUP::AUPAirspace::AUPAirspace(const std::string& id, const Link& link)
	: objbase_t(id, link)
{
}

template class ParseAUP::AUPObjectImpl<ParseAUP::AUPAirspaceTimeSlice>;

ParseAUP::Node::Node(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: m_parser(parser), m_tagname(tagname), m_refcount(1), m_childnum(childnum), m_childcount(0)
{
	for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
		if (i->name != "gml:id")
			continue;
		m_id = i->value;
	}
}

ParseAUP::Node::~Node()
{
}

void ParseAUP::Node::reference(void) const
{
	g_atomic_int_inc(&m_refcount);
}

void ParseAUP::Node::unreference(void) const
{
	if (!g_atomic_int_dec_and_test(&m_refcount))
		return;
	delete this;
}

void ParseAUP::Node::error(const std::string& text) const
{
	std::string text1(text);
	if (!get_id().empty())
		text1 += " [" + get_id() + "]";
	m_parser.error(text1);
}

void ParseAUP::Node::error(const_ptr_t child, const std::string& text) const
{
	if (!child) {
		error(text);
		return;
	}
	std::ostringstream oss;
	oss << child->get_element_name();
	if (get_childcount() != 1)
		oss << '[' << ((int)get_childcount() - 1) << ']';
	std::string text1(text);
	if (!get_id().empty())
		text1 += " [" + get_id() + "]";
	m_parser.error(oss.str(), text1);
}

void ParseAUP::Node::warning(const std::string& text) const
{
	std::string text1(text);
	if (!get_id().empty())
		text1 += " [" + get_id() + "]";
	m_parser.warning(text1);
}

void ParseAUP::Node::warning(const_ptr_t child, const std::string& text) const
{
	if (!child) {
		warning(text);
		return;
	}
	std::ostringstream oss;
	oss << child->get_element_name();
	if (get_childcount() != 1)
		oss << '[' << ((int)get_childcount() - 1) << ']';
	std::string text1(text);
	if (!get_id().empty())
		text1 += " [" + get_id() + "]";
	m_parser.warning(oss.str(), text1);
}

void ParseAUP::Node::info(const std::string& text) const
{
	std::string text1(text);
	if (!get_id().empty())
		text1 += " [" + get_id() + "]";
	m_parser.info(text1);
}

void ParseAUP::Node::info(const_ptr_t child, const std::string& text) const
{
	if (!child) {
		info(text);
		return;
	}
	std::ostringstream oss;
	oss << child->get_element_name();
	if (get_childcount() != 1)
		oss << '[' << ((int)get_childcount() - 1) << ']';
	std::string text1(text);
	if (!get_id().empty())
		text1 += " [" + get_id() + "]";
	m_parser.info(oss.str(), text1);
}

void ParseAUP::Node::on_characters(const Glib::ustring& characters)
{
}

void ParseAUP::Node::on_subelement(const ptr_t& el)
{
	++m_childcount;
	{
		NodeText::ptr_t elx(NodeText::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeLink::ptr_t elx(NodeLink::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeIgnore::ptr_t elx(NodeIgnore::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeADRMessageMember::ptr_t elx(NodeADRMessageMember::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMExtensionBase::ptr_t elx(NodeAIXMExtensionBase::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMExtension::ptr_t elx(NodeAIXMExtension::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMAirspaceActivation::ptr_t elx(NodeAIXMAirspaceActivation::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMActivation::ptr_t elx(NodeAIXMActivation::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeGMLValidTime::ptr_t elx(NodeGMLValidTime::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeGMLBeginPosition::ptr_t elx(NodeGMLBeginPosition::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeGMLEndPosition::ptr_t elx(NodeGMLEndPosition::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeGMLTimePeriod::ptr_t elx(NodeGMLTimePeriod::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeGMLTimePosition::ptr_t elx(NodeGMLTimePosition::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeGMLTimeInstant::ptr_t elx(NodeGMLTimeInstant::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMEnRouteSegmentPoint::ptr_t elx(NodeAIXMEnRouteSegmentPoint::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMStart::ptr_t elx(NodeAIXMStart::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMEnd::ptr_t elx(NodeAIXMEnd::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMAirspaceLayer::ptr_t elx(NodeAIXMAirspaceLayer::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMLevels::ptr_t elx(NodeAIXMLevels::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMAvailabilityBase::ptr_t elx(NodeAIXMAvailabilityBase::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMAvailability::ptr_t elx(NodeAIXMAvailability::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMAnyTimeSlice::ptr_t elx(NodeAIXMAnyTimeSlice::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMTimeSlice::ptr_t elx(NodeAIXMTimeSlice::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMObject::ptr_t elx(NodeAIXMObject::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAllocations::ptr_t elx(NodeAllocations::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeData::ptr_t elx(NodeData::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	error(el, "Element " + get_element_name() + " does not support unknown subelements");
}

void ParseAUP::Node::on_subelement(const NodeIgnore::ptr_t& el)
{
}

void ParseAUP::Node::on_subelement(const NodeText::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support text subelements");
}

void ParseAUP::Node::on_subelement(const NodeLink::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support link subelements");
}

void ParseAUP::Node::on_subelement(const Glib::RefPtr<NodeAIXMExtensionBase>& el)
{
	error(el, "Element " + get_element_name() + " does not support AIXM Extension subelements");
}

void ParseAUP::Node::on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMExtension::get_static_element_name() + " subelements");
}

void ParseAUP::Node::on_subelement(const Glib::RefPtr<NodeAIXMAirspaceActivation>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMAirspaceActivation::get_static_element_name() + " subelements");
}

void ParseAUP::Node::on_subelement(const Glib::RefPtr<NodeAIXMActivation>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMActivation::get_static_element_name() + " subelements");
}

void ParseAUP::Node::on_subelement(const Glib::RefPtr<NodeGMLValidTime>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeGMLValidTime::get_static_element_name() + " subelements");
}

void ParseAUP::Node::on_subelement(const Glib::RefPtr<NodeGMLBeginPosition>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeGMLBeginPosition::get_static_element_name() + " subelements");
}

void ParseAUP::Node::on_subelement(const Glib::RefPtr<NodeGMLEndPosition>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeGMLEndPosition::get_static_element_name() + " subelements");
}

void ParseAUP::Node::on_subelement(const Glib::RefPtr<NodeGMLTimePeriod>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeGMLTimePeriod::get_static_element_name() + " subelements");
}

void ParseAUP::Node::on_subelement(const Glib::RefPtr<NodeGMLTimePosition>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeGMLTimePosition::get_static_element_name() + " subelements");
}

void ParseAUP::Node::on_subelement(const Glib::RefPtr<NodeGMLTimeInstant>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeGMLTimeInstant::get_static_element_name() + " subelements");
}

void ParseAUP::Node::on_subelement(const Glib::RefPtr<NodeAIXMEnRouteSegmentPoint>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMEnRouteSegmentPoint::get_static_element_name() + " subelements");
}

void ParseAUP::Node::on_subelement(const Glib::RefPtr<NodeAIXMStart>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMStart::get_static_element_name() + " subelements");
}

void ParseAUP::Node::on_subelement(const Glib::RefPtr<NodeAIXMEnd>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMEnd::get_static_element_name() + " subelements");
}

void ParseAUP::Node::on_subelement(const Glib::RefPtr<NodeAIXMAirspaceLayer>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMAirspaceLayer::get_static_element_name() + " subelements");
}

void ParseAUP::Node::on_subelement(const Glib::RefPtr<NodeAIXMLevels>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMLevels::get_static_element_name() + " subelements");
}

void ParseAUP::Node::on_subelement(const Glib::RefPtr<NodeAIXMAvailabilityBase>& el)
{
	error(el, "Element " + get_element_name() + " does not support AIXM Availability subelements");
}

void ParseAUP::Node::on_subelement(const Glib::RefPtr<NodeAIXMAvailability>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMAvailability::get_static_element_name() + " subelements");
}

void ParseAUP::Node::on_subelement(const Glib::RefPtr<NodeAIXMAnyTimeSlice>& el)
{
	error(el, "Element " + get_element_name() + " does not support aixm time slice subelements");
}

void ParseAUP::Node::on_subelement(const Glib::RefPtr<NodeAIXMTimeSlice>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMTimeSlice::get_static_element_name() + " subelements");
}

void ParseAUP::Node::on_subelement(const Glib::RefPtr<NodeAIXMObject>& el)
{
	error(el, "Element " + get_element_name() + " does not support aixm object subelements");
}

void ParseAUP::Node::on_subelement(const Glib::RefPtr<NodeADRMessageMember>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeADRMessageMember::get_static_element_name() + " subelements");
}

void ParseAUP::Node::on_subelement(const Glib::RefPtr<NodeAllocations>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeCDRAllocations::get_static_element_name() + " / " + NodeRSAAllocations::get_static_element_name() + " subelements");
}

void ParseAUP::Node::on_subelement(const Glib::RefPtr<NodeData>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeData::get_static_element_name() + " subelements");
}

ParseAUP::NodeIgnore::NodeIgnore(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties, const std::string& name)
	: Node(parser, tagname, childnum, properties), m_elementname(name)
{
}

void ParseAUP::NodeNoIgnore::on_subelement(const NodeIgnore::ptr_t& el)
{
	if (true)
		std::cerr << "ADR Parser: Element " + get_element_name() + " does not support ignored subelements" << std::endl;
	error("Element " + get_element_name() + " does not support ignored subelements");
}

ParseAUP::NodeText::NodeText(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties, const std::string& name)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_factor(1), m_factorl(1), m_texttype(find_type(name))
{
	check_attr(properties);
}

ParseAUP::NodeText::NodeText(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties, texttype_t tt)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_factor(1), m_factorl(1), m_texttype(tt)
{
	check_attr(properties);
}

void ParseAUP::NodeText::check_attr(const AttributeList& properties)
{
	switch (get_type()) {
	case tt_aixmupperlimit:
	case tt_aixmlowerlimit:
		for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
			if (i->name != "uom")
				continue;
			std::string val(Glib::ustring(i->value).uppercase());
			if (val == "FT") {
				m_factor = 1;
				m_factorl = 1;
			} else if (val == "FL") {
				m_factor = 100;
				m_factorl = 100;
			} else {
				error(get_element_name() + ": unknown unit of measure: " + val);
			}
		}
		break;

	default:
		break;
	}
}

void ParseAUP::NodeText::on_characters(const Glib::ustring& characters)
{
	m_text += characters;
}

const std::string& ParseAUP::NodeText::get_static_element_name(texttype_t tt)
{
	switch (tt) {
	case tt_adrconditionalroutetype:
	{
		static const std::string r("adr:conditionalRouteType");
		return r;
	}

	case tt_adrlevel1:
	{
		static const std::string r("adr:level1");
		return r;
	}

	case tt_aixmdesignator:
	{
		static const std::string r("aixm:designator");
		return r;
	}

	case tt_aixmdesignatoricao:
	{
		static const std::string r("aixm:designatorICAO");
		return r;
	}

	case tt_aixmdesignatornumber:
	{
		static const std::string r("aixm:designatorNumber");
		return r;
	}

	case tt_aixmdesignatorprefix:
	{
		static const std::string r("aixm:designatorPrefix");
		return r;
	}

	case tt_aixmdesignatorsecondletter:
	{
		static const std::string r("aixm:designatorSecondLetter");
		return r;
	}

	case tt_aixminterpretation:
	{
		static const std::string r("aixm:interpretation");
		return r;
	}

	case tt_aixmlowerlimit:
	{
		static const std::string r("aixm:lowerLimit");
		return r;
	}

	case tt_aixmlowerlimitreference:
	{
		static const std::string r("aixm:lowerLimitReference");
		return r;
	}

	case tt_aixmmultipleidentifier:
	{
		static const std::string r("aixm:multipleIdentifier");
		return r;
	}

	case tt_aixmstatus:
	{
		static const std::string r("aixm:status");
		return r;
	}

	case tt_aixmtype:
	{
		static const std::string r("aixm:type");
		return r;
	}

	case tt_aixmupperlimit:
	{
		static const std::string r("aixm:upperLimit");
		return r;
	}

	case tt_aixmupperlimitreference:
	{
		static const std::string r("aixm:upperLimitReference");
		return r;
	}

	case tt_requestid:
	{
		static const std::string r("requestId");
		return r;
	}

	case tt_requestreceptiontime:
	{
		static const std::string r("requestReceptionTime");
		return r;
	}

	case tt_sendtime:
	{
		static const std::string r("sendTime");
		return r;
	}

	case tt_status:
	{
		static const std::string r("status");
		return r;
	}

	default:
	case tt_invalid:
	{
		static const std::string r("?");
		return r;
	}
	}
}

void ParseAUP::NodeText::check_type_order(void)
{
	for (texttype_t tt(tt_first); tt < tt_invalid; ) {
		texttype_t ttp(tt);
		tt = (texttype_t)(tt + 1);
		if (!(tt < tt_invalid))
			break;
		if (get_static_element_name(ttp) < get_static_element_name(tt))
			continue;
		throw std::runtime_error("NodeText ordering error: " + get_static_element_name(ttp) +
					 " >= " + get_static_element_name(tt));
	}
}

ParseAUP::NodeText::texttype_t ParseAUP::NodeText::find_type(const std::string& name)
{
	if (false) {
		for (texttype_t tt(tt_first); tt < tt_invalid; tt = (texttype_t)(tt + 1))
			if (name == get_static_element_name(tt))
				return tt;
		return tt_invalid;
	}
	texttype_t tb(tt_first), tt(tt_invalid);
	for (;;) {
		texttype_t tm((texttype_t)((tb + tt) >> 1));
		if (tm == tb) {
			if (name == get_static_element_name(tb))
				return tb;
			return tt_invalid;
		}
		int c(name.compare(get_static_element_name(tm)));
		if (!c)
			return tm;
		if (c < 0)
			tt = tm;
		else
			tb = tm;
	}
}

long ParseAUP::NodeText::get_long(void) const
{
	const char *cp(m_text.c_str());
	char *ep;
	long v(strtol(cp, &ep, 10));
	if (!*ep && ep != cp)
		return v * (long)m_factorl;
	error("invalid signed number " + get_text());
	return 0;
}

unsigned long ParseAUP::NodeText::get_ulong(void) const
{
	const char *cp(m_text.c_str());
	char *ep;
	unsigned long v(strtoul(cp, &ep, 10));
	if (!*ep && ep != cp)
		return v * m_factorl;
	error("invalid unsigned number " + get_text());
	return 0;
}

double ParseAUP::NodeText::get_double(void) const
{
	const char *cp(m_text.c_str());
	char *ep;
	double v(strtod(cp, &ep));
	if (!*ep && ep != cp)
		return v * m_factor;
	error("invalid floating point number " + get_text());
	return std::numeric_limits<double>::quiet_NaN();
}

Point::coord_t ParseAUP::NodeText::get_lat(void) const
{
	Point pt;
	if (!(pt.set_str(get_text()) & Point::setstr_lat)) {
		error("invalid latitude " + get_text());
		pt.set_invalid();
	}
	return pt.get_lat();
}

Point::coord_t ParseAUP::NodeText::get_lon(void) const
{
	Point pt;
	if (!(pt.set_str(get_text()) & Point::setstr_lon)) {
		error("invalid longitude " + get_text());
		pt.set_invalid();
	}
	return pt.get_lon();
}

Point ParseAUP::NodeText::get_coord_deg(void) const
{
	Point pt;
	pt.set_invalid();
	const char *cp(get_text().c_str());
	char *cp1;
	double lat(strtod(cp, &cp1));
	if (cp1 == cp || !cp1 || *cp1 != ' ')
		return pt;
	cp = cp1 + 1;
	double lon(strtod(cp, &cp1));
	if (cp1 == cp || !cp1 || (*cp1 && *cp1 != ' '))
		return pt;
	pt.set_lat_deg_dbl(lat);
	pt.set_lon_deg_dbl(lon);
	return pt;
}

std::pair<Point,int32_t> ParseAUP::NodeText::get_coord_3d(void) const
{
	std::pair<Point,int32_t> r;
	r.first.set_invalid();
	r.second = std::numeric_limits<int32_t>::min();
	const char *cp(get_text().c_str());
	char *cp1;
	double lat(strtod(cp, &cp1));
	if (cp1 == cp || !cp1 || *cp1 != ' ')
		return r;
	cp = cp1 + 1;
	double lon(strtod(cp, &cp1));
	if (cp1 == cp || !cp1 || (*cp1 && *cp1 != ' '))
		return r;
	r.first.set_lat_deg_dbl(lat);
	r.first.set_lon_deg_dbl(lon);
	if (!*cp1)
		return r;
	cp = cp1 + 1;
	double alt(strtod(cp, &cp1));
	if (cp1 == cp || !cp1 || (*cp1 && *cp1 != ' '))
		return r;
	r.second = Point::round<int32_t,double>(alt * Point::m_to_ft_dbl);
	return r;
}

UUID ParseAUP::NodeText::get_uuid(void) const
{
	return UUID::from_str(get_text());
}

int ParseAUP::NodeText::get_timehhmm(void) const
{
	const char *cp(get_text().c_str());
	char *cp1;
	int t(strtoul(cp, &cp1, 10) * 60);
	if (cp1 == cp || !cp1 || *cp1 != ':')
		return -1;
	cp = cp1 + 1;
	t += strtoul(cp, &cp1, 10);
	if (cp1 == cp || !cp1 || (*cp1 && *cp1 != ' '))
		return -1;
	return t;
}

timetype_t ParseAUP::NodeText::get_dateyyyymmdd(void) const
{
	const char *cp(get_text().c_str());
	char *cp1;
	struct tm tm;
	memset(&tm, 0, sizeof(tm));
	tm.tm_year = strtoul(cp, &cp1, 10) - 1900;
	if (cp1 == cp || !cp1 || *cp1 != '-')
		return std::numeric_limits<timetype_t>::max();
	cp = cp1 + 1;
	tm.tm_mon = strtoul(cp, &cp1, 10) - 1;
	if (cp1 == cp || !cp1 || *cp1 != '-')
		return std::numeric_limits<timetype_t>::max();
	cp = cp1 + 1;
	tm.tm_mday = strtoul(cp, &cp1, 10);
	if (cp1 == cp || !cp1 || (*cp1 && *cp1 != ' '))
		return std::numeric_limits<timetype_t>::max();
	time_t t(timegm(&tm));
	if (t != (time_t)-1)
		return t;
	return std::numeric_limits<timetype_t>::max();
}

int ParseAUP::NodeText::get_daynr(void) const
{
	if (get_text() == "SUN")
		return 0;
	if (get_text() == "MON")
		return 1;
	if (get_text() == "TUE")
		return 2;
	if (get_text() == "WED")
		return 3;
	if (get_text() == "THU")
		return 4;
	if (get_text() == "FRI")
		return 5;
	if (get_text() == "SAT")
		return 6;
	return -1;
}

int ParseAUP::NodeText::get_yesno(void) const
{
	if (get_text() == "YES")
		return 1;
	if (get_text() == "NO")
		return 0;
	return -1;
}

void ParseAUP::NodeText::update(AltRange& ar)
{
	switch (get_type()) {
	case tt_aixmupperlimit:
		if (get_text() == "GND") {
			ar.set_upper_alt(0);
			ar.set_upper_mode(AltRange::alt_height);
		} else if (get_text() == "FLOOR") {
			ar.set_upper_alt(0);
			ar.set_upper_mode(AltRange::alt_floor);
		} else if (get_text() == "CEILING") {
			ar.set_upper_alt(AltRange::altmax);
			ar.set_upper_mode(AltRange::alt_ceiling);
		} else if (get_text() == "UNL") {
			ar.set_upper_alt(AltRange::altmax);
		} else {
			ar.set_upper_alt(get_long());
		}
		break;

	case tt_aixmlowerlimit:
		if (get_text() == "GND") {
			ar.set_lower_alt(0);
			ar.set_lower_mode(AltRange::alt_height);
		} else if (get_text() == "FLOOR") {
			ar.set_lower_alt(0);
			ar.set_lower_mode(AltRange::alt_floor);
		} else if (get_text() == "CEILING") {
			ar.set_lower_alt(AltRange::altmax);
			ar.set_lower_mode(AltRange::alt_ceiling);
		} else if (get_text() == "UNL") {
			ar.set_lower_alt(AltRange::altmax);
		} else {
			ar.set_lower_alt(get_long());
		}
		break;

	case tt_aixmupperlimitreference:
		if ((ar.get_upper_mode() == AltRange::alt_height && !ar.get_upper_alt()) ||
		    ar.get_upper_mode() == AltRange::alt_floor ||
		    ar.get_upper_mode() == AltRange::alt_ceiling)
			break;
		if (get_text() == "STD")
			ar.set_upper_mode(AltRange::alt_std);
		else if (get_text() == "MSL")
			ar.set_upper_mode(AltRange::alt_qnh);
		else
			error(get_element_name() + ": unknown upper limit reference " + get_text());
		break;

	case tt_aixmlowerlimitreference:
		if ((ar.get_lower_mode() == AltRange::alt_height && !ar.get_lower_alt()) ||
		    ar.get_lower_mode() == AltRange::alt_floor ||
		    ar.get_lower_mode() == AltRange::alt_ceiling)
			break;
		if (get_text() == "STD")
			ar.set_lower_mode(AltRange::alt_std);
		else if (get_text() == "MSL")
			ar.set_lower_mode(AltRange::alt_qnh);
		else
			error(get_element_name() + ": unknown lower limit reference " + get_text());
		break;

	default:
		error(get_element_name() + ": cannot update altrange with " + get_text());
		break;
	}
}

ParseAUP::NodeLink::NodeLink(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties, const std::string& name)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_linktype(find_type(name))
{
	process_attr(properties);
}

ParseAUP::NodeLink::NodeLink(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties, linktype_t lt)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_linktype(lt)
{
	process_attr(properties);
}

void ParseAUP::NodeLink::process_attr(const AttributeList& properties)
{
	for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
		if (i->name != "xlink:href")
			continue;
		if (!i->value.compare(0, 23, "urn:uuid:#xpointer(id('")) {
			m_id = i->value.substr(23);
			std::string::size_type i(m_id.size());
			if (i > 0) {
				if (m_id[i - 1] == ')')
					--i;
				else
					i = 0;
			}
			if (i > 0) {
				if (m_id[i - 1] == ')')
					--i;
				else
					i = 0;
			}
			if (i > 0) {
				if (m_id[i - 1] == '\'')
					--i;
				else
					i = 0;
			}
			m_id.erase(i);
		}
		if (!m_id.empty()) {
			m_uuid = UUID::from_str(i->value);
			if (m_uuid.is_nil())
				warning(get_element_name() + " invalid UUID " + i->value);
		}
	}
}

const std::string& ParseAUP::NodeLink::get_static_element_name(linktype_t lt)
{
	switch (lt) {
	case lt_adrhostairspace:
	{
		static const std::string r("adr:hostAirspace");
		return r;
	}

	case lt_aixmpointchoiceairportreferencepoint:
	{
		static const std::string r("aixm:pointChoice_airportReferencePoint");
		return r;
	}

	case lt_aixmpointchoicefixdesignatedpoint:
	{
		static const std::string r("aixm:pointChoice_fixDesignatedPoint");
		return r;
	}

	case lt_aixmpointchoicenavaidsystem:
	{
		static const std::string r("aixm:pointChoice_navaidSystem");
		return r;
	}

	case lt_aixmrouteformed:
	{
		static const std::string r("aixm:routeFormed");
		return r;
	}

	default:
	case lt_invalid:
	{
		static const std::string r("?");
		return r;
	}
	}
}

void ParseAUP::NodeLink::check_type_order(void)
{
	for (linktype_t lt(lt_first); lt < lt_invalid; ) {
		linktype_t ltp(lt);
		lt = (linktype_t)(lt + 1);
		if (!(lt < lt_invalid))
			break;
		if (get_static_element_name(ltp) < get_static_element_name(lt))
			continue;
		throw std::runtime_error("NodeLink ordering error: " + get_static_element_name(ltp) +
					 " >= " + get_static_element_name(lt));
	}
}

ParseAUP::NodeLink::linktype_t ParseAUP::NodeLink::find_type(const std::string& name)
{
	if (false) {
		for (linktype_t lt(lt_first); lt < lt_invalid; lt = (linktype_t)(lt + 1))
			if (name == get_static_element_name(lt))
				return lt;
		return lt_invalid;
	}
	linktype_t lb(lt_first), lt(lt_invalid);
	for (;;) {
		linktype_t lm((linktype_t)((lb + lt) >> 1));
		if (lm == lb) {
			if (name == get_static_element_name(lb))
				return lb;
			return lt_invalid;
		}
		int c(name.compare(get_static_element_name(lm)));
		if (!c)
			return lm;
		if (c < 0)
			lt = lm;
		else
			lb = lm;
	}
}

ParseAUP::NodeAIXMExtensionBase::NodeAIXMExtensionBase(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

ParseAUP::NodeAIXMAvailabilityBase::NodeAIXMAvailabilityBase(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

const std::string ParseAUP::NodeADRAirspaceExtension::elementname("adr:AirspaceExtension");

ParseAUP::NodeADRAirspaceExtension::NodeADRAirspaceExtension(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMExtensionBase(parser, tagname, childnum, properties), m_level1(false)
{
}

void ParseAUP::NodeADRAirspaceExtension::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_adrlevel1:
	{
		int b(el->get_yesno());
		if (b == -1)
			error(get_static_element_name() + ": invalid level1 " + el->get_text());
		else
			m_level1 = !!b;
		break;
	}

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

const std::string ParseAUP::NodeADRAirspaceActivationExtension::elementname("adr:AirspaceActivationExtension");

ParseAUP::NodeADRAirspaceActivationExtension::NodeADRAirspaceActivationExtension(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMExtensionBase(parser, tagname, childnum, properties)
{
}

void ParseAUP::NodeADRAirspaceActivationExtension::on_subelement(const NodeLink::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_adrhostairspace:
		m_hostairspaces.push_back(AUPLink(el->get_id()));
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

const std::string ParseAUP::NodeADRRouteAvailabilityExtension::elementname("adr:RouteAvailabilityExtension");

ParseAUP::NodeADRRouteAvailabilityExtension::NodeADRRouteAvailabilityExtension(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMExtensionBase(parser, tagname, childnum, properties), m_cdr(0)
{
}

void ParseAUP::NodeADRRouteAvailabilityExtension::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_adrconditionalroutetype:
		if (el->get_text() == "CDR_1")
			m_cdr = 1;
		else if (el->get_text() == "CDR_2")
			m_cdr = 2;
		else if (el->get_text() == "CDR_3")
			m_cdr = 3;
		else
			error(get_static_element_name() + ": unknown conditional route type " + el->get_element_name());
		break;

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

void ParseAUP::NodeADRRouteAvailabilityExtension::on_subelement(const NodeLink::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_adrhostairspace:
		m_hostairspaces.push_back(AUPLink(el->get_id()));
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

const std::string ParseAUP::NodeAIXMExtension::elementname("aixm:extension");

ParseAUP::NodeAIXMExtension::NodeAIXMExtension(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseAUP::NodeAIXMExtension::on_subelement(const Glib::RefPtr<NodeAIXMExtensionBase>& el)
{
	if (!el)
		return;
	if (m_el)
		error(get_static_element_name() + ": multiple subelements");
	m_el = el;
}

const std::string ParseAUP::NodeAIXMAirspaceActivation::elementname("aixm:AirspaceActivation");

ParseAUP::NodeAIXMAirspaceActivation::NodeAIXMAirspaceActivation(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseAUP::NodeAIXMAirspaceActivation::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmstatus:
		if (el->get_text() == "ACTIVE")
			m_activation.set_status(AUPRSA::Activation::status_active);
		else
			error(get_static_element_name() + ": invalid status " + el->get_text());
		break;

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

void ParseAUP::NodeAIXMAirspaceActivation::on_subelement(const Glib::RefPtr<NodeAIXMLevels>& el)
{
	if (el)
		m_activation.set_altrange(el->get_altrange());
}

void ParseAUP::NodeAIXMAirspaceActivation::on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el)
{
	if (!el)
		return;
	NodeADRAirspaceActivationExtension::ptr_t elx(el->get<NodeADRAirspaceActivationExtension>());
	if (!elx) {
		error(get_static_element_name() + ": invalid activation type " + el->get_element_name());
		return;
	}
	m_activation.set_hostairspaces(elx->get_hostairspaces());
}

const std::string ParseAUP::NodeAIXMActivation::elementname("aixm:activation");

ParseAUP::NodeAIXMActivation::NodeAIXMActivation(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseAUP::NodeAIXMActivation::on_subelement(const Glib::RefPtr<NodeAIXMAirspaceActivation>& el)
{
	if (!el)
		return;
	if (m_el)
		error(get_static_element_name() + ": multiple subelements");
	m_el = el;
}

const std::string ParseAUP::NodeGMLValidTime::elementname("gml:validTime");

ParseAUP::NodeGMLValidTime::NodeGMLValidTime(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_start(0), m_end(std::numeric_limits<long>::max())
{
}

void ParseAUP::NodeGMLValidTime::on_subelement(const NodeGMLTimePeriod::ptr_t& el)
{
	if (el) {
		m_start = el->get_start();
		m_end = el->get_end();
	}
}

void ParseAUP::NodeGMLValidTime::on_subelement(const Glib::RefPtr<NodeGMLTimeInstant>& el)
{
	if (el)
		m_start = m_end = el->get_time();
}

const long ParseAUP::NodeGMLTimePosition::invalid = std::numeric_limits<long>::min();
const long ParseAUP::NodeGMLTimePosition::unknown = std::numeric_limits<long>::min() + 1;

const std::string ParseAUP::NodeGMLTimePosition::elementname("gml:timePosition");

ParseAUP::NodeGMLTimePosition::NodeGMLTimePosition(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_unknown(false)
{
	for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
		if (i->name != "indeterminatePosition")
			continue;
		m_unknown = i->value == "unknown";
	}
}

void ParseAUP::NodeGMLTimePosition::on_characters(const Glib::ustring& characters)
{
	m_text += characters;
}

long ParseAUP::NodeGMLTimePosition::get_time(void) const
{
	if (m_unknown)
		return unknown;
	Glib::TimeVal tv;
	if (!tv.assign_from_iso8601(m_text + "Z"))
		return invalid;
	return tv.tv_sec;
}

const std::string ParseAUP::NodeGMLBeginPosition::elementname("gml:beginPosition");

ParseAUP::NodeGMLBeginPosition::NodeGMLBeginPosition(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeGMLTimePosition(parser, tagname, childnum, properties)
{
}

long ParseAUP::NodeGMLBeginPosition::get_time(void) const
{
	long t(NodeGMLTimePosition::get_time());
	if (t == invalid)
		t = 0;
	else if (t < 0 || t == unknown)
		t = 0;
	return t;
}

const std::string ParseAUP::NodeGMLEndPosition::elementname("gml:endPosition");

ParseAUP::NodeGMLEndPosition::NodeGMLEndPosition(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeGMLTimePosition(parser, tagname, childnum, properties)
{
}

long ParseAUP::NodeGMLEndPosition::get_time(void) const
{
	long t(NodeGMLTimePosition::get_time());
	if (t == invalid)
		t = 0;
	else if (t < 0 || t == unknown)
		t = std::numeric_limits<long>::max();
	return t;
}

const std::string ParseAUP::NodeGMLTimePeriod::elementname("gml:TimePeriod");

ParseAUP::NodeGMLTimePeriod::NodeGMLTimePeriod(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_start(0), m_end(std::numeric_limits<long>::max())
{
}

void ParseAUP::NodeGMLTimePeriod::on_subelement(const NodeGMLBeginPosition::ptr_t& el)
{
	if (el)
		m_start = el->get_time();
}

void ParseAUP::NodeGMLTimePeriod::on_subelement(const NodeGMLEndPosition::ptr_t& el)
{
	if (el)
		m_end = el->get_time();
}

const std::string ParseAUP::NodeGMLTimeInstant::elementname("gml:TimeInstant");

ParseAUP::NodeGMLTimeInstant::NodeGMLTimeInstant(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_time(NodeGMLTimePosition::invalid)
{
}

void ParseAUP::NodeGMLTimeInstant::on_subelement(const NodeGMLTimePosition::ptr_t& el)
{
	if (el)
		m_time = el->get_time();
}

const std::string ParseAUP::NodeAIXMEnRouteSegmentPoint::elementname("aixm:EnRouteSegmentPoint");

ParseAUP::NodeAIXMEnRouteSegmentPoint::NodeAIXMEnRouteSegmentPoint(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseAUP::NodeAIXMEnRouteSegmentPoint::on_subelement(const NodeLink::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_aixmpointchoicenavaidsystem:
		m_id = el->get_id();
		break;

	case NodeLink::lt_aixmpointchoicefixdesignatedpoint:
		m_id = el->get_id();
		break;

	case NodeLink::lt_aixmpointchoiceairportreferencepoint:
		m_id = el->get_id();
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

const std::string ParseAUP::NodeAIXMStart::elementname("aixm:start");

ParseAUP::NodeAIXMStart::NodeAIXMStart(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseAUP::NodeAIXMStart::on_subelement(const Glib::RefPtr<NodeAIXMEnRouteSegmentPoint>& el)
{
	if (el)
		m_id = el->get_id();
}

const std::string ParseAUP::NodeAIXMEnd::elementname("aixm:end");

ParseAUP::NodeAIXMEnd::NodeAIXMEnd(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseAUP::NodeAIXMEnd::on_subelement(const Glib::RefPtr<NodeAIXMEnRouteSegmentPoint>& el)
{
	if (el)
		m_id = el->get_id();
}

const std::string ParseAUP::NodeAIXMAirspaceLayer::elementname("aixm:AirspaceLayer");

ParseAUP::NodeAIXMAirspaceLayer::NodeAIXMAirspaceLayer(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseAUP::NodeAIXMAirspaceLayer::on_subelement(const Glib::RefPtr<NodeText>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmupperlimit:
	case NodeText::tt_aixmlowerlimit:
	case NodeText::tt_aixmupperlimitreference:
	case NodeText::tt_aixmlowerlimitreference:
		el->update(m_altrange);
		break;

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

const std::string ParseAUP::NodeAIXMLevels::elementname("aixm:levels");

ParseAUP::NodeAIXMLevels::NodeAIXMLevels(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseAUP::NodeAIXMLevels::on_subelement(const Glib::RefPtr<NodeAIXMAirspaceLayer>& el)
{
	if (el)
		m_altrange = el->get_altrange();
}

const std::string ParseAUP::NodeAIXMRouteAvailability::elementname("aixm:RouteAvailability");

ParseAUP::NodeAIXMRouteAvailability::NodeAIXMRouteAvailability(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAvailabilityBase(parser, tagname, childnum, properties)
{
}

void ParseAUP::NodeAIXMRouteAvailability::on_subelement(const Glib::RefPtr<NodeAIXMLevels>& el)
{
	if (el)
		m_availability.set_altrange(el->get_altrange());
}

void ParseAUP::NodeAIXMRouteAvailability::on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el)
{
	if (!el)
		return;
	NodeADRRouteAvailabilityExtension::ptr_t elx(el->get<NodeADRRouteAvailabilityExtension>());
	if (!elx) {
		if (el->get_el())
			error(get_static_element_name() + ": invalid extension type " + el->get_el()->get_element_name());
		else
			error(get_static_element_name() + ": no extension");
		return;
	}
	m_availability.set_cdr(elx->get_cdr());
	m_availability.set_hostairspaces(elx->get_hostairspaces());
}

const std::string ParseAUP::NodeAIXMAvailability::elementname("aixm:availability");

ParseAUP::NodeAIXMAvailability::NodeAIXMAvailability(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseAUP::NodeAIXMAvailability::on_subelement(const Glib::RefPtr<NodeAIXMAvailabilityBase>& el)
{
	if (!el)
		return;
	if (m_el)
		error(get_static_element_name() + ": multiple subelements");
	m_el = el;
}

ParseAUP::NodeAIXMAnyTimeSlice::NodeAIXMAnyTimeSlice(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_validstart(0), m_validend(std::numeric_limits<long>::max()),
	  m_featurestart(0), m_featureend(std::numeric_limits<long>::max()), m_interpretation(interpretation_invalid)
{
}

void ParseAUP::NodeAIXMAnyTimeSlice::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixminterpretation:
		if (el->get_text() == "BASELINE")
			m_interpretation = interpretation_baseline;
		else if (el->get_text() == "PERMDELTA")
			m_interpretation = interpretation_permdelta;
		else if (el->get_text() == "TEMPDELTA")
			m_interpretation = interpretation_tempdelta;
		else if (el->get_text() == "SNAPSHOT")
			m_interpretation = interpretation_snapshot;
		else
			error("aixm TimeSlice: invalid interpretation: " + el->get_text());
		break;

	default:
		error("aixm TimeSlice: unknown text " + el->get_element_name());
		break;
	}
}

void ParseAUP::NodeAIXMAnyTimeSlice::on_subelement(const NodeGMLValidTime::ptr_t& el)
{
	if (el) {
		m_validstart = el->get_start();
		m_validend = el->get_end();
		AUPTimeSlice& ts(get_tsbase());
		if (&ts) {
			uint64_t start(m_validstart), end(m_validend);
			if (m_validstart < 0)
				start = 0;
			else if (m_validstart == std::numeric_limits<long>::max())
				start = std::numeric_limits<uint64_t>::max();
			if (m_validend < 0)
				end = 0;
			else if (m_validend == std::numeric_limits<long>::max())
				end = std::numeric_limits<uint64_t>::max();
			ts.set_starttime(start);
			ts.set_endtime(end);
		}
	}
}

const std::string ParseAUP::NodeAIXMRouteSegmentTimeSlice::elementname("aixm:RouteSegmentTimeSlice");

ParseAUP::NodeAIXMRouteSegmentTimeSlice::NodeAIXMRouteSegmentTimeSlice(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAnyTimeSlice(parser, tagname, childnum, properties)
{
}

void ParseAUP::NodeAIXMRouteSegmentTimeSlice::on_subelement(const Glib::RefPtr<NodeLink>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_aixmrouteformed:
		m_timeslice.get_route().set_id(el->get_id());
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

void ParseAUP::NodeAIXMRouteSegmentTimeSlice::on_subelement(const Glib::RefPtr<NodeAIXMStart>& el)
{
	if (!el)
		return;
	m_timeslice.get_start().set_id(el->get_id());
}

void ParseAUP::NodeAIXMRouteSegmentTimeSlice::on_subelement(const Glib::RefPtr<NodeAIXMEnd>& el)
{
	if (!el)
		return;
	m_timeslice.get_end().set_id(el->get_id());
}

void ParseAUP::NodeAIXMRouteSegmentTimeSlice::on_subelement(const Glib::RefPtr<NodeAIXMAvailability>& el)
{
	if (!el)
		return;
	NodeAIXMRouteAvailability::ptr_t elx(el->get<NodeAIXMRouteAvailability>());
	if (!elx) {
		error(get_static_element_name() + ": invalid availability type " + el->get_element_name());
		return;
	}
	m_timeslice.get_availability().push_back(elx->get_availability());
}

const std::string ParseAUP::NodeAIXMRouteTimeSlice::elementname("aixm:RouteTimeSlice");

ParseAUP::NodeAIXMRouteTimeSlice::NodeAIXMRouteTimeSlice(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAnyTimeSlice(parser, tagname, childnum, properties)
{
}

void ParseAUP::NodeAIXMRouteTimeSlice::on_subelement(const Glib::RefPtr<NodeText>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmdesignatorprefix:
		m_desigprefix = el->get_text();
		break;

	case NodeText::tt_aixmdesignatorsecondletter:
		m_desigsecondletter = el->get_text();
		break;

	case NodeText::tt_aixmdesignatornumber:
		m_designumber = el->get_text();
		break;

	case NodeText::tt_aixmmultipleidentifier:
		m_multiid = el->get_text();
		break;

	default:
		NodeAIXMAnyTimeSlice::on_subelement(el);
		break;
	}
	m_timeslice.set_ident(m_desigprefix + m_desigsecondletter + m_designumber + m_multiid);
}

const std::string ParseAUP::NodeAIXMDesignatedPointTimeSlice::elementname("aixm:DesignatedPointTimeSlice");

ParseAUP::NodeAIXMDesignatedPointTimeSlice::NodeAIXMDesignatedPointTimeSlice(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAnyTimeSlice(parser, tagname, childnum, properties)
{
}

void ParseAUP::NodeAIXMDesignatedPointTimeSlice::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmtype:
		if (el->get_text() == "ICAO")
			m_timeslice.set_type(ADR::DesignatedPointTimeSlice::type_icao);
		else if (el->get_text() == "TERMINAL")
			m_timeslice.set_type(ADR::DesignatedPointTimeSlice::type_terminal);
		else if (el->get_text() == "COORD")
			m_timeslice.set_type(ADR::DesignatedPointTimeSlice::type_coord);
		else if (el->get_text() == "OTHER:__ADR__BOUNDARY")
			m_timeslice.set_type(ADR::DesignatedPointTimeSlice::type_adrboundary);
		else if (el->get_text() == "OTHER:__ADR__REFERENCE")
			m_timeslice.set_type(ADR::DesignatedPointTimeSlice::type_adrreference);
		else
			error(get_static_element_name() + ": unknown " + el->get_element_name() + " " + el->get_text());
		break;

	case NodeText::tt_aixmdesignator:
		m_timeslice.set_ident(el->get_text());
		break;

	default:
		NodeAIXMAnyTimeSlice::on_subelement(el);
		break;
	}
}

const std::string ParseAUP::NodeAIXMNavaidTimeSlice::elementname("aixm:NavaidTimeSlice");

ParseAUP::NodeAIXMNavaidTimeSlice::NodeAIXMNavaidTimeSlice(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAnyTimeSlice(parser, tagname, childnum, properties)
{
}

void ParseAUP::NodeAIXMNavaidTimeSlice::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmdesignator:
		m_timeslice.set_ident(el->get_text());
		break;

	default:
		NodeAIXMAnyTimeSlice::on_subelement(el);
		break;
	}
}

const std::string ParseAUP::NodeAIXMAirspaceTimeSlice::elementname("aixm:AirspaceTimeSlice");

ParseAUP::NodeAIXMAirspaceTimeSlice::NodeAIXMAirspaceTimeSlice(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAnyTimeSlice(parser, tagname, childnum, properties)
{
}

void ParseAUP::NodeAIXMAirspaceTimeSlice::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmtype:
		if (el->get_text() == "ATZ")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_atz);
		else if (el->get_text() == "CBA")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_cba);
		else if (el->get_text() == "CTA")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_cta);
		else if (el->get_text() == "CTA_P")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_cta_p);
		else if (el->get_text() == "CTR")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_ctr);
		else if (el->get_text() == "D")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_d);
		else if (el->get_text() == "D_OTHER")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_d_other);
		else if (el->get_text() == "FIR")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_fir);
		else if (el->get_text() == "FIR_P")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_fir_p);
		else if (el->get_text() == "NAS")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_nas);
		else if (el->get_text() == "OCA")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_oca);
		else if (el->get_text() == "OTHER:__ADR__AUAG")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_adr_auag);
		else if (el->get_text() == "OTHER:__ADR__FAB")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_adr_fab);
		else if (el->get_text() == "P")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_p);
		else if (el->get_text() == "PART")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_part);
		else if (el->get_text() == "R")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_r);
		else if (el->get_text() == "SECTOR")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_sector);
		else if (el->get_text() == "SECTOR_C")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_sector_c);
		else if (el->get_text() == "TMA")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_tma);
		else if (el->get_text() == "TRA")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_tra);
		else if (el->get_text() == "TSA")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_tsa);
		else if (el->get_text() == "UIR")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_uir);
		else if (el->get_text() == "UTA")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_uta);
		else
			error(get_static_element_name() + ": unknown type " + el->get_text());
		break;

	case NodeText::tt_aixmdesignator:
		m_timeslice.set_ident(el->get_text());
		break;

	case NodeText::tt_aixmdesignatoricao:
	{
		int b(el->get_yesno());
		if (b == -1) {
			error(get_static_element_name() + "/" + el->get_element_name() + ": invalid boolean " + el->get_text());
			break;
		}
		m_timeslice.set_icao(!!b);
		break;
	}

	default:
		NodeAIXMAnyTimeSlice::on_subelement(el);
		break;
	}
}

void ParseAUP::NodeAIXMAirspaceTimeSlice::on_subelement(const Glib::RefPtr<NodeAIXMActivation>& el)
{
	if (!el)
		return;
	NodeAIXMAirspaceActivation::ptr_t elx(el->get<NodeAIXMAirspaceActivation>());
	if (!elx) {
		if (el->get_el())
			error(get_static_element_name() + ": invalid activation type " + el->get_el()->get_element_name());
		else
			error(get_static_element_name() + ": no activation");
		return;
	}
	m_timeslice.set_activation(elx->get_activation());
}

void ParseAUP::NodeAIXMAirspaceTimeSlice::on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el)
{
	if (!el)
		return;
	NodeADRAirspaceExtension::ptr_t elx(el->get<NodeADRAirspaceExtension>());
	if (!elx) {
		if (el->get_el())
			error(get_static_element_name() + ": invalid extension type " + el->get_el()->get_element_name());
		else
			error(get_static_element_name() + ": no extension");
		return;
	}
	m_timeslice.set_level(1, elx->is_level1());
}

const std::string ParseAUP::NodeAIXMTimeSlice::elementname("aixm:timeSlice");

ParseAUP::NodeAIXMTimeSlice::NodeAIXMTimeSlice(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseAUP::NodeAIXMTimeSlice::on_subelement(const NodeAIXMAnyTimeSlice::ptr_t& el)
{
	if (el)
		m_nodes.push_back(el);
}



ParseAUP::NodeAIXMObject::NodeAIXMObject(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

template <typename Obj> 
ParseAUP::NodeAIXMObjectImpl<Obj>::NodeAIXMObjectImpl(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObject(parser, tagname, childnum, properties), m_obj(new Object())
{
	m_obj->set_id(m_id);
}

template <typename Obj> 
void ParseAUP::NodeAIXMObjectImpl<Obj>::on_subelement(const NodeAIXMTimeSlice::ptr_t& el)
{
	if (!el)
		return;
	for (unsigned int i(0), n(el->size()); i < n; ++i) {
		if (!el->operator[](i))
			continue;
		typedef typename Obj::TimeSliceImpl ts_t;
		ts_t *ts(dynamic_cast<ts_t *>(&(el->operator[](i)->get_tsbase())));
		if (!ts) {
			error("timeSlice: invalid subtype");
			continue;
		}
		m_obj->add_timeslice(*ts);
	}
}

const std::string ParseAUP::NodeAIXMRouteSegment::elementname("aixm:RouteSegment");

ParseAUP::NodeAIXMRouteSegment::NodeAIXMRouteSegment(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObjectImpl<AUPRouteSegment>(parser, tagname, childnum, properties)
{
}

const std::string ParseAUP::NodeAIXMRoute::elementname("aixm:Route");

ParseAUP::NodeAIXMRoute::NodeAIXMRoute(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObjectImpl<AUPRoute>(parser, tagname, childnum, properties)
{
}

const std::string ParseAUP::NodeAIXMDesignatedPoint::elementname("aixm:DesignatedPoint");

ParseAUP::NodeAIXMDesignatedPoint::NodeAIXMDesignatedPoint(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObjectImpl<AUPDesignatedPoint>(parser, tagname, childnum, properties)
{
}

const std::string ParseAUP::NodeAIXMNavaid::elementname("aixm:Navaid");

ParseAUP::NodeAIXMNavaid::NodeAIXMNavaid(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObjectImpl<AUPNavaid>(parser, tagname, childnum, properties)
{
}

const std::string ParseAUP::NodeAIXMAirspace::elementname("aixm:Airspace");

ParseAUP::NodeAIXMAirspace::NodeAIXMAirspace(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObjectImpl<AUPAirspace>(parser, tagname, childnum, properties)
{
}

const std::string ParseAUP::NodeADRMessageMember::elementname("adrmsg:hasMember");

ParseAUP::NodeADRMessageMember::NodeADRMessageMember(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseAUP::NodeADRMessageMember::on_subelement(const NodeAIXMObject::ptr_t& el)
{
	if (el)
		m_obj = el;
}

ParseAUP::NodeAllocations::NodeAllocations(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseAUP::NodeAllocations::on_subelement(const NodeADRMessageMember::ptr_t& el)
{
	if (!el)
		return;
	NodeAIXMObject::ptr_t aixmobj(el->get_obj());
	if (!aixmobj)
		return;
	AUPObject::ptr_t p(aixmobj->get_obj());
	if (!p) {
		error("NodeAllocations: no object");
		return;
	}
	if (false)
		std::cerr << "Adding object ID " << p->get_id() << " refcnt " << p->get_refcount() << std::endl;
	std::pair<objects_t::iterator,bool> ins(m_parser.m_objects.insert(p));
	if (!ins.second)
		error("NodeAllocations: duplicate ID " + p->get_id());
}

const std::string ParseAUP::NodeCDRAllocations::elementname("cdrOpeningsClosures");

ParseAUP::NodeCDRAllocations::NodeCDRAllocations(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAllocations(parser, tagname, childnum, properties)
{
}

const std::string ParseAUP::NodeRSAAllocations::elementname("rsaAllocations");

ParseAUP::NodeRSAAllocations::NodeRSAAllocations(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAllocations(parser, tagname, childnum, properties)
{
}

ParseAUP::NodeData::NodeData(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

const std::string ParseAUP::NodeData::elementname("data");

void ParseAUP::NodeData::on_subelement(const NodeAllocations::ptr_t& el)
{
}

ParseAUP::NodeAirspaceReply::NodeAirspaceReply(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseAUP::NodeAirspaceReply::on_subelement(const Glib::RefPtr<NodeText>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_sendtime:
		m_sendtime = el->get_text();
		break;

	case NodeText::tt_status:
		m_status = el->get_text();
		break;

	case NodeText::tt_requestid:
		m_requestid = el->get_text();
		break;

	case NodeText::tt_requestreceptiontime:
		m_requesttime = el->get_text();
		break;

	default:
		error(get_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

void ParseAUP::NodeAirspaceReply::on_subelement(const NodeData::ptr_t& el)
{
}

const std::string ParseAUP::NodeAirspaceEAUPCDRReply::elementname("airspace:EAUPCDRReply");

ParseAUP::NodeAirspaceEAUPCDRReply::NodeAirspaceEAUPCDRReply(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAirspaceReply(parser, tagname, childnum, properties)
{
}

const std::string ParseAUP::NodeAirspaceEAUPRSAReply::elementname("airspace:EAUPRSAReply");

ParseAUP::NodeAirspaceEAUPRSAReply::NodeAirspaceEAUPRSAReply(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAirspaceReply(parser, tagname, childnum, properties)
{
}

const bool ParseAUP::tracestack;

ParseAUP::ParseAUP(Database& db, bool verbose)
	: m_db(db), m_errorcnt(0), m_warncnt(0), m_verbose(verbose)
{
	NodeText::check_type_order();
	NodeLink::check_type_order();
	// create factory table
	m_factory[NodeADRMessageMember::get_static_element_name()] = NodeADRMessageMember::create;
	m_factory[NodeADRAirspaceExtension::get_static_element_name()] = NodeADRAirspaceExtension::create;
	m_factory[NodeADRAirspaceActivationExtension::get_static_element_name()] = NodeADRAirspaceActivationExtension::create;
	m_factory[NodeADRRouteAvailabilityExtension::get_static_element_name()] = NodeADRRouteAvailabilityExtension::create;
	m_factory[NodeAIXMExtension::get_static_element_name()] = NodeAIXMExtension::create;
	m_factory[NodeAIXMAirspaceActivation::get_static_element_name()] = NodeAIXMAirspaceActivation::create;
	m_factory[NodeAIXMActivation::get_static_element_name()] = NodeAIXMActivation::create;
	m_factory[NodeGMLValidTime::get_static_element_name()] = NodeGMLValidTime::create;
	m_factory[NodeGMLTimePosition::get_static_element_name()] = NodeGMLTimePosition::create;
	m_factory[NodeGMLBeginPosition::get_static_element_name()] = NodeGMLBeginPosition::create;
	m_factory[NodeGMLEndPosition::get_static_element_name()] = NodeGMLEndPosition::create;
	m_factory[NodeGMLTimePeriod::get_static_element_name()] = NodeGMLTimePeriod::create;
	m_factory[NodeGMLTimeInstant::get_static_element_name()] = NodeGMLTimeInstant::create;
	m_factory[NodeAIXMEnRouteSegmentPoint::get_static_element_name()] = NodeAIXMEnRouteSegmentPoint::create;
	m_factory[NodeAIXMStart::get_static_element_name()] = NodeAIXMStart::create;
	m_factory[NodeAIXMEnd::get_static_element_name()] = NodeAIXMEnd::create;
	m_factory[NodeAIXMAirspaceLayer::get_static_element_name()] = NodeAIXMAirspaceLayer::create;
	m_factory[NodeAIXMLevels::get_static_element_name()] = NodeAIXMLevels::create;
	m_factory[NodeAIXMRouteAvailability::get_static_element_name()] = NodeAIXMRouteAvailability::create;
	m_factory[NodeAIXMAvailability::get_static_element_name()] = NodeAIXMAvailability::create;
	m_factory[NodeAIXMRouteSegmentTimeSlice::get_static_element_name()] = NodeAIXMRouteSegmentTimeSlice::create;
	m_factory[NodeAIXMRouteTimeSlice::get_static_element_name()] = NodeAIXMRouteTimeSlice::create;
	m_factory[NodeAIXMDesignatedPointTimeSlice::get_static_element_name()] = NodeAIXMDesignatedPointTimeSlice::create;
	m_factory[NodeAIXMNavaidTimeSlice::get_static_element_name()] = NodeAIXMNavaidTimeSlice::create;
	m_factory[NodeAIXMAirspaceTimeSlice::get_static_element_name()] = NodeAIXMAirspaceTimeSlice::create;
	m_factory[NodeAIXMTimeSlice::get_static_element_name()] = NodeAIXMTimeSlice::create;
	m_factory[NodeAIXMRouteSegment::get_static_element_name()] = NodeAIXMRouteSegment::create;
	m_factory[NodeAIXMRoute::get_static_element_name()] = NodeAIXMRoute::create;
	m_factory[NodeAIXMDesignatedPoint::get_static_element_name()] = NodeAIXMDesignatedPoint::create;
	m_factory[NodeAIXMNavaid::get_static_element_name()] = NodeAIXMNavaid::create;
	m_factory[NodeAIXMAirspace::get_static_element_name()] = NodeAIXMAirspace::create;
	m_factory[NodeCDRAllocations::get_static_element_name()] = NodeCDRAllocations::create;
	m_factory[NodeRSAAllocations::get_static_element_name()] = NodeRSAAllocations::create;
	m_factory[NodeData::get_static_element_name()] = NodeData::create;
	m_factory[NodeAirspaceEAUPCDRReply::get_static_element_name()] = NodeAirspaceEAUPCDRReply::create;
	m_factory[NodeAirspaceEAUPRSAReply::get_static_element_name()] = NodeAirspaceEAUPRSAReply::create;
 	// shorthand namespace table
	m_namespaceshort["eurocontrol/cfmu/b2b/AirspaceServices"] = "airspace";
	m_namespaceshort["http://www.eurocontrol.int/cfmu/b2b/ADRMessage"] = "adrmsg";
	m_namespaceshort["http://www.aixm.aero/schema/5.1/extensions/ADR"] = "adr";
	m_namespaceshort["http://www.opengis.net/gml/3.2"] = "gml";
	m_namespaceshort["http://www.aixm.aero/schema/5.1"] = "aixm";
	m_namespaceshort["http://www.w3.org/1999/xlink"] = "xlink";
}

ParseAUP::~ParseAUP()
{
}

void ParseAUP::error(const std::string& text)
{
	++m_errorcnt;
	std::cerr << "E: " << text << std::endl;
}

void ParseAUP::error(const std::string& child, const std::string& text)
{
	++m_errorcnt;
	std::cerr << "E: " << child << ": " << text << std::endl;
}

void ParseAUP::warning(const std::string& text)
{
	++m_warncnt;
	std::cerr << "W: " << text << std::endl;
}

void ParseAUP::warning(const std::string& child, const std::string& text)
{
	++m_warncnt;
	std::cerr << "W: " << child << ": " << text << std::endl;
}

void ParseAUP::info(const std::string& text)
{
	std::cerr << "I: " << text << std::endl;
}

void ParseAUP::info(const std::string& child, const std::string& text)
{
	std::cerr << "I: " << child << ": " << text << std::endl;
}

void ParseAUP::on_start_document(void)
{
	m_parsestack.clear();
}

void ParseAUP::on_end_document(void)
{
	if (!m_parsestack.empty()) {
		if (true)
			std::cerr << "ADR Loader: parse stack not empty at end of document" << std::endl;
		error("parse stack not empty at end of document");
	}
}

void ParseAUP::on_start_element(const Glib::ustring& name, const AttributeList& properties)
{
	std::string name1(name);
	// handle namespace
	for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
		if (i->name.compare(0, 6, "xmlns:"))
			continue;
		std::string ns(i->name.substr(6));
		std::string val(i->value);
		if (false)
			std::cerr << "(Re)defining namespace " << ns << " value "  << val << std::endl;
		std::pair<namespacemap_t::iterator,bool> ins(m_namespacemap.insert(std::make_pair(ns, val)));
		if (ins.second)
			continue;
		if (ins.first->second != val)
			warning("Redefining namespace " + ns + " to " + val);
		ins.first->second = val;
	}
	{
		std::string::size_type n(name1.find(':'));
		if (n != std::string::npos) {
			std::string ns(name1.substr(0, n));
			namespacemap_t::const_iterator i(m_namespacemap.find(ns));
			if (i == m_namespacemap.end()) {
				error("Namespace " + ns + " not found");
			} else {
				namespacemap_t::const_iterator j(m_namespaceshort.find(i->second));
				if (j == m_namespaceshort.end()) {
					error("Namespace " + i->second + " unknown");
				} else {
					name1 = j->second + name1.substr(n);
					if (false)
						std::cerr << "Translated namespace " << name << " -> " << name1 << std::endl;
				}
			}
		}
	}
	AttributeList properties1(properties);
	for (AttributeList::iterator pi(properties1.begin()), pe(properties1.end()); pi != pe; ++pi) {
		std::string name1(pi->name);
		std::string::size_type n(name1.find(':'));
		if (n == std::string::npos)
			continue;
		std::string ns(name1.substr(0, n));
		if (ns == "xmlns")
			continue;
		namespacemap_t::const_iterator i(m_namespacemap.find(ns));
		if (i == m_namespacemap.end()) {
			error("Namespace " + ns + " not found");
			continue;
		}
		namespacemap_t::const_iterator j(m_namespaceshort.find(i->second));
		if (j == m_namespaceshort.end()) {
			error("Namespace " + i->second + " unknown");
			continue;
		}
		name1 = j->second + name1.substr(n);
		pi->name = name1;
	}
	unsigned int childnum(0);
	if (m_parsestack.empty()) {
		if (name1 == NodeAirspaceEAUPCDRReply::get_static_element_name()) {
			if (tracestack)
				std::cerr << ">> " << name << " (" << NodeAirspaceEAUPCDRReply::get_static_element_name() << ')' << std::endl;
			try {
				Node::ptr_t p(new NodeAirspaceEAUPCDRReply(*this, name, childnum, properties1));
				m_parsestack.push_back(p);
			} catch (const std::runtime_error& e) {
				error(std::string("Cannot create ADRMessage node: ") + e.what());
				throw;
			}
			return;
		}
		if (name1 == NodeAirspaceEAUPRSAReply::get_static_element_name()) {
			if (tracestack)
				std::cerr << ">> " << name << " (" << NodeAirspaceEAUPRSAReply::get_static_element_name() << ')' << std::endl;
			try {
				Node::ptr_t p(new NodeAirspaceEAUPRSAReply(*this, name, childnum, properties1));
				m_parsestack.push_back(p);
			} catch (const std::runtime_error& e) {
				error(std::string("Cannot create ADRMessage node: ") + e.what());
				throw;
			}
			return;
		}
		error("document must begin with " + NodeAirspaceEAUPCDRReply::get_static_element_name() +
		      " or " + NodeAirspaceEAUPRSAReply::get_static_element_name());
	}
	childnum = m_parsestack.back()->get_childcount();
	if (m_parsestack.back()->is_ignore()) {
		if (tracestack)
			std::cerr << ">> ignore: " << name << " (" << name1 << ')' << std::endl;
		try {
			Node::ptr_t p(new NodeIgnore(*this, name, childnum, properties1, name1));
			m_parsestack.push_back(p);
		} catch (const std::runtime_error& e) {
			error(std::string("Cannot create Ignore node: ") + e.what());
			throw;
		}
		return;
	}
	// Node factory
	{
		NodeText::texttype_t tt(NodeText::find_type(name1));
		if (tt != NodeText::tt_invalid) {
			if (tracestack)
				std::cerr << ">> " << name << " (" << NodeText::get_static_element_name(tt) << ')' << std::endl;
			try {
				Node::ptr_t p(new NodeText(*this, name, childnum, properties1, tt));
				m_parsestack.push_back(p);
			} catch (const std::runtime_error& e) {
				error(std::string("Cannot create Text ") + NodeText::get_static_element_name(tt) + " node: " + e.what());
				throw;
			}
			return;
		}
	}
	{
		NodeLink::linktype_t lt(NodeLink::find_type(name1));
		if (lt != NodeLink::lt_invalid) {
			if (tracestack)
				std::cerr << ">> " << name << " (" << NodeLink::get_static_element_name(lt) << ')' << std::endl;
			try {
				Node::ptr_t p(new NodeLink(*this, name, childnum, properties1, lt));
				m_parsestack.push_back(p);
			} catch (const std::runtime_error& e) {
				error(std::string("Cannot create Link ") + NodeLink::get_static_element_name(lt) + " node: " + e.what());
				throw;
			}
			return;
		}
	}
	{
		factory_t::const_iterator fi(m_factory.find(name1));
		if (fi != m_factory.end()) {
			Node::ptr_t p;
			try {
				p = (fi->second)(*this, name, childnum, properties1);
				m_parsestack.push_back(p);
			} catch (const std::runtime_error& e) {
				error(std::string("Cannot create ") + name1 + " node: " + e.what());
				throw;
			}
			if (tracestack)
				std::cerr << ">> " << name << " (" << p->get_element_name() << ')' << std::endl;
			return;
		}
	}
	// finally: ignoring element
	{
		if (tracestack)
			std::cerr << ">> unknown: " << name << std::endl;
		try {
			Node::ptr_t p(new NodeIgnore(*this, name, childnum, properties1, name1));
			m_parsestack.push_back(p);
		} catch (const std::runtime_error& e) {
			error(std::string("Cannot create ADRMessage node: ") + e.what());
			throw;
		}
		return;
	}
}

void ParseAUP::on_end_element(const Glib::ustring& name)
{
	if (m_parsestack.empty())
		error("parse stack underflow on end element");
	Node::ptr_t p(m_parsestack.back());
	m_parsestack.pop_back();
	if (tracestack)
		std::cerr << "<< " << p->get_element_name() << std::endl;
	if (p->get_tagname() != (std::string)name)
		error("end element name " + name + " does not match expected name " + p->get_tagname());
	if (!m_parsestack.empty()) {
		try {
			m_parsestack.back()->on_subelement(p);
		} catch (const std::runtime_error& e) {
			error(std::string("Error in on_subelement method of ") + m_parsestack.back()->get_element_name() + ": " + e.what());
			throw;
		}
		return;
	}
	{
		NodeAirspaceReply::ptr_t px(NodeAirspaceReply::ptr_t::cast_dynamic(p));
		if (!px)
			error("top node is not an " + NodeAirspaceEAUPCDRReply::get_static_element_name() +
			      " or an " + NodeAirspaceEAUPRSAReply::get_static_element_name());
	}
}

void ParseAUP::on_characters(const Glib::ustring& characters)
{
	if (m_parsestack.empty())
		return;
	try {
		m_parsestack.back()->on_characters(characters);
	} catch (const std::runtime_error& e) {
		error(std::string("Error in on_characters method of ") + m_parsestack.back()->get_element_name() + ": " + e.what());
		throw;
	}
}

void ParseAUP::on_comment(const Glib::ustring& text)
{
}

void ParseAUP::on_warning(const Glib::ustring& text)
{
	warning("Parser: Warning: " + text);
}

void ParseAUP::on_error(const Glib::ustring& text)
{
	warning("Parser: Error: " + text);
}

void ParseAUP::on_fatal_error(const Glib::ustring& text)
{
	error("Parser: Fatal Error: " + text);
}

void ParseAUP::process(bool verbose)
{
	for (objects_t::iterator oi(m_objects.begin()), oe(m_objects.end()); oi != oe; ) {
		objects_t::iterator oii(oi);
		++oi;
		const AUPObject::ptr_t& p(*oii);
		if (!p) {
			m_objects.erase(oii);
			continue;
		}
		const AUPAirspace::ptr_t& pa(AUPAirspace::ptr_t::cast_dynamic(p));
		if (!pa)
			continue;
		std::string id;
		ADR::AirspaceTimeSlice::type_t typ(ADR::AirspaceTimeSlice::type_invalid);
		unsigned int i(0), n(pa->size());
		for (; i < n; ++i) {
			const AUPAirspaceTimeSlice& ts(pa->operator[](i).as_airspace());
			if (!ts.is_snapshot())
				continue;
			id = ts.get_ident();
			typ = ts.get_type();
			ADR::Database::findresults_t r(get_db().find_by_ident(ts.get_ident(), ADR::Database::comp_exact, 0,
									      ADR::Database::loadmode_link, ts.get_starttime(),
									      ts.get_endtime() + 1, ADR::Object::type_airspace,
									      ADR::Object::type_airspace));
			for (ADR::Database::findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
				const ADR::AirspaceTimeSlice& tsa(ri->get_obj()->operator()(ts.get_starttime()).as_airspace());
				if (!tsa.is_valid())
					continue;
				if (tsa.get_ident() != ts.get_ident())
					continue;
				if (ts.get_type() != ADR::AirspaceTimeSlice::type_invalid &&
				    tsa.get_type() != ts.get_type())
					continue;
				if (!pa->get_link().is_nil())
					warning("Airspace " + ts.get_ident() + " has multiple matches");
				pa->set_link(*ri);
				if (typ == ADR::AirspaceTimeSlice::type_invalid)
					typ = tsa.get_type();
			}
			break;
		}
		if (pa->get_link().is_nil()) {
			warning("Airspace " + pa->get_id() + " [" + id + "] not found");
			if (true && verbose)
				pa->print(std::cerr) << std::endl;
			m_objects.erase(oii);
			continue;
		}
		for (i = 0; i < n; ++i) {
			AUPAirspaceTimeSlice& ts(pa->operator[](i).as_airspace());
			if (!ts.is_valid() && !ts.is_snapshot())
				continue;
			if (ts.get_ident().empty())
				ts.set_ident(id);
			if (ts.get_type() == ADR::AirspaceTimeSlice::type_invalid)
				ts.set_type(typ);
		}
	}
	for (objects_t::iterator oi(m_objects.begin()), oe(m_objects.end()); oi != oe; ++oi) {
		const AUPAirspace::ptr_t& pa(AUPAirspace::ptr_t::cast_dynamic(*oi));
		if (!pa)
			continue;
		std::cerr << "Airspace " << pa->get_link() << std::endl;
		for (unsigned int i(0), n(pa->size()); i < n; ++i) {
			AUPAirspaceTimeSlice& ts(pa->operator[](i).as_airspace());
			if (!ts.is_valid() && !ts.is_snapshot())
				continue;
			AUPAirspaceTimeSlice::Activation& act(ts.get_activation());
			AUPAirspaceTimeSlice::Activation::hostairspaces_t& ha(act.get_hostairspaces());
			for (AUPAirspaceTimeSlice::Activation::hostairspaces_t::iterator i(ha.begin()), e(ha.end()); i != e; ++i) {
				AUPObject::ptr_t p(new AUPObject(i->get_id()));
				objects_t::const_iterator oi(m_objects.find(p));
				if (oi == m_objects.end()) {
					warning("Host Airspace ID " + i->get_id() + " not found");
				} else {
					i->set_link((*oi)->get_link());
				}
			}
		}
		if (true && verbose)
			pa->print(std::cout) << std::endl;
	}
	for (objects_t::iterator oi(m_objects.begin()), oe(m_objects.end()); oi != oe; ) {
		objects_t::iterator oii(oi);
		++oi;
		const AUPRouteSegment::ptr_t& ps(AUPRouteSegment::ptr_t::cast_dynamic(*oii));
		if (!ps)
			continue;
		AUPLink route, start, end;
		AUPRouteSegmentTimeSlice::segments_t segs;
		unsigned int i(0), n(ps->size());
		for (; i < n; ++i) {
			const AUPRouteSegmentTimeSlice& ts(ps->operator[](i).as_routesegment());
			if (!ts.is_snapshot())
				continue;
			std::string routeident, startident, endident;
			std::string routeid, startid, endid;
			ADR::timetype_t routetm(0), starttm(0), endtm(0);
			{
				AUPObject::ptr_t p(new AUPObject(ts.get_route().get_id()));
				objects_t::const_iterator i(m_objects.find(p));
				if (i == m_objects.end()) {
					warning("Route ID " + ts.get_route().get_id() + " not found");
				} else {
					routeid = ts.get_route().get_id();
					for (unsigned int j(0), n((*i)->size()); j < n; ++j) {
						const AUPRouteTimeSlice& ts((*i)->operator[](j).as_route());
						if (!ts.is_snapshot())
							continue;
						routeident = ts.get_ident();
						routetm = ts.get_starttime();
						break;
					}
				}
			}
			{
				AUPObject::ptr_t p(new AUPObject(ts.get_start().get_id()));
				objects_t::const_iterator i(m_objects.find(p));
				if (i == m_objects.end()) {
					warning("Start ID " + ts.get_start().get_id() + " not found");
				} else {
					startid = ts.get_start().get_id();
					for (unsigned int j(0), n((*i)->size()); j < n; ++j) {
						const AUPIdentTimeSlice& ts((*i)->operator[](j).as_ident());
						if (!ts.is_snapshot())
							continue;
						startident = ts.get_ident();
						starttm = ts.get_starttime();
						break;
					}
				}
			}
			{
				AUPObject::ptr_t p(new AUPObject(ts.get_end().get_id()));
				objects_t::const_iterator i(m_objects.find(p));
				if (i == m_objects.end()) {
					warning("End ID " + ts.get_end().get_id() + " not found");
				} else {
					endid = ts.get_end().get_id();
					for (unsigned int j(0), n((*i)->size()); j < n; ++j) {
						const AUPIdentTimeSlice& ts((*i)->operator[](j).as_ident());
						if (!ts.is_snapshot())
							continue;
						endident = ts.get_ident();
						endtm = ts.get_starttime();
						break;
					}
				}
			}
			if (routeident.empty() || startident.empty() || endident.empty())
				continue;
			ADR::Database::findresults_t r(get_db().find_by_ident(routeident, ADR::Database::comp_exact, 0,
									      ADR::Database::loadmode_link, routetm,
									      routetm + 1, ADR::Object::type_route,
									      ADR::Object::type_route));
			for (ADR::Database::findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
				const ADR::RouteTimeSlice& tsr(ri->get_obj()->operator()(routetm).as_route());
				if (!tsr.is_valid())
					continue;
				if (tsr.get_ident() != routeident)
					continue;
				ADR::Link lstart, lend;
				AUPSegmentLink lseg;
				ADR::Database::findresults_t rs(get_db().find_dependson((*ri), ADR::Database::loadmode_link, std::min(starttm, endtm),
											std::max(starttm, endtm) + 1, ADR::Object::type_routesegment,
											ADR::Object::type_routesegment));
				for (ADR::Database::findresults_t::const_iterator rsi(rs.begin()), rse(rs.end()); rsi != rse; ++rsi) {
					bool sf, sb, ef, eb;
					{
						const ADR::RouteSegmentTimeSlice& tsr(rsi->get_obj()->operator()(starttm).as_routesegment());
						if (!tsr.is_valid())
							continue;
						const ADR::PointIdentTimeSlice& ts0(tsr.get_start().get_obj()->operator()(starttm).as_point());
						sf = ts0.is_valid() && ts0.get_ident() == startident;
						if (sf)
							lstart = tsr.get_start();
						const ADR::PointIdentTimeSlice& ts1(tsr.get_end().get_obj()->operator()(starttm).as_point());
						sb = ts1.is_valid() && ts1.get_ident() == startident;
						if (sb)
							lstart = tsr.get_end();
					}
					{
						const ADR::RouteSegmentTimeSlice& tsr(rsi->get_obj()->operator()(endtm).as_routesegment());
						if (!tsr.is_valid())
							continue;
						const ADR::PointIdentTimeSlice& ts0(tsr.get_start().get_obj()->operator()(endtm).as_point());
						eb = ts0.is_valid() && ts0.get_ident() == endident;
						if (eb)
							lend = tsr.get_start();
						const ADR::PointIdentTimeSlice& ts1(tsr.get_end().get_obj()->operator()(endtm).as_point());
						ef = ts1.is_valid() && ts1.get_ident() == endident;
						if (ef)
							lend = tsr.get_end();
					}
					if ((sf && ef) || (sb && eb))
						lseg = AUPSegmentLink(*rsi, (sb && eb));
				}
				if (lstart.is_nil() || lend.is_nil())
					continue;
				segs.clear();
				if (lseg.is_nil()) {
					Graph g;
					g.add(ts.get_starttime(), ri->get_obj());
					g.add(ts.get_starttime(), lstart.get_obj());
					g.add(ts.get_starttime(), lend.get_obj());
					// fixme: should force both edges to be created
					for (ADR::Database::findresults_t::const_iterator rsi(rs.begin()), rse(rs.end()); rsi != rse; ++rsi)
						g.add(ts.get_starttime(), rsi->get_obj());
					Graph::vertex_descriptor u, v;
					bool ok(false);
					{
						boost::tie(u, ok) = g.find_vertex(lstart);
						if (!ok) {
							std::ostringstream oss;
							oss << "Cannot find vertex " << lstart;
							warning(oss.str());
						}
					}
					{
						bool ok1;
						boost::tie(v, ok1) = g.find_vertex(lend);
						if (!ok1) {
							std::ostringstream oss;
							oss << "Cannot find vertex " << lend;
							warning(oss.str());
						}
						ok = ok && ok1;
					}
					if (ok) {
						ADR::Graph::UUIDEdgeFilter filter(g, *ri, ADR::GraphEdge::matchcomp_all);
						typedef boost::filtered_graph<ADR::Graph, ADR::Graph::UUIDEdgeFilter> fg_t;
						fg_t fg(g, filter);
						std::vector<Graph::vertex_descriptor> predecessors(boost::num_vertices(g), 0);
						dijkstra_shortest_paths(fg, u, boost::weight_map(boost::get(&ADR::GraphEdge::m_dist, g)).predecessor_map(&predecessors[0]));
						if (predecessors[v] == v) {
							std::ostringstream oss;
							oss << "Cannot find route from " << lstart << " to " << lend;
							warning(oss.str());
							if (!false)
								g.print(std::cerr << oss.str() << std::endl) << std::endl;
						}
						for (;;) {
							if (u == v)
								break;
							Graph::vertex_descriptor uu(predecessors[v]);
							if (uu == v)
								break;
							fg_t::out_edge_iterator e0, e1;
							for (boost::tie(e0, e1) = boost::out_edges(uu, fg); e0 != e1; ++e0)
								if (boost::target(*e0, fg) == v)
									break;
							if (e0 == e1) {
								warning("cannot find solution edge");
								v = uu;
								continue;
							}
							GraphEdge& ee(fg[*e0]);
							const Object::const_ptr_t& p(ee.get_object());
							if (!p) {
								warning("solution edge has no object");
								v = uu;
								continue;
							}
							segs.insert(segs.begin(), AUPSegmentLink(Link(Object::ptr_t::cast_const(p)), ee.is_backward()));
							v = uu;
						}
					}
				} else {
					segs.push_back(lseg);
				}
				if (!route.get_link().is_nil())
					warning("Duplicate route " + startident + " " + routeident + " " + endident);
				route = AUPLink(routeid, *ri);
				start = AUPLink(startid, lstart);
				end = AUPLink(endid, lend);
			}
		}
		if (route.get_link().is_nil()) {
			warning("Route Segment " + ps->get_id() + " [" + start.get_id() + " " + route.get_id() + " " + end.get_id() + "] not found");
			if (true && verbose)
				ps->print(std::cerr) << std::endl;
			m_objects.erase(oii);
			continue;
		}
		if (segs.size() != 1) {
			std::ostringstream oss;
			oss << "Route " << ps->get_id() << " has " << segs.size() << " segments";
			warning(oss.str());
		}
		for (i = 0; i < n; ++i) {
			AUPRouteSegmentTimeSlice& ts(ps->operator[](i).as_routesegment());
			if (!ts.is_valid() && !ts.is_snapshot())
				continue;
			if (ts.get_route().get_id().empty())
				ts.get_route().set_id(route.get_id());
			if (ts.get_start().get_id().empty())
				ts.get_start().set_id(start.get_id());
			if (ts.get_end().get_id().empty())
				ts.get_end().set_id(end.get_id());
			if (ts.get_route().get_link().is_nil())
				ts.get_route().set_link(route.get_link());
			if (ts.get_start().get_link().is_nil())
				ts.get_start().set_link(start.get_link());
			if (ts.get_end().get_link().is_nil())
				ts.get_end().set_link(end.get_link());
			if (ts.get_segments().empty())
				ts.set_segments(segs);
			AUPRouteSegmentTimeSlice::availability_t& av(ts.get_availability());
			for (AUPRouteSegmentTimeSlice::availability_t::iterator ai(av.begin()), ae(av.end()); ai != ae; ++ai) {
				AUPRouteSegmentTimeSlice::Availability::hostairspaces_t& ha(ai->get_hostairspaces());
				for (AUPRouteSegmentTimeSlice::Availability::hostairspaces_t::iterator i(ha.begin()), e(ha.end()); i != e; ++i) {
					AUPObject::ptr_t p(new AUPObject(i->get_id()));
					objects_t::const_iterator oi(m_objects.find(p));
					if (oi == m_objects.end()) {
						warning("Host Airspace ID " + i->get_id() + " not found");
					} else {
						i->set_link((*oi)->get_link());
					}
				}
			}
		}
		if (true && verbose)
			ps->print(std::cout) << std::endl;
	}
}

void ParseAUP::save(AUPDatabase& aupdb)
{
	for (objects_t::iterator oi(m_objects.begin()), oe(m_objects.end()); oi != oe; ++oi) {
		const AUPAirspace::ptr_t& pa(AUPAirspace::ptr_t::cast_dynamic(*oi));
		if (pa) {
			for (unsigned int i(0), n(pa->size()); i < n; ++i) {
				const AUPAirspaceTimeSlice& ts(pa->operator[](i).as_airspace());
				if (!ts.is_valid())
					continue;
				if (ts.get_activation().get_status() == AUPRSA::Activation::status_invalid)
					continue;
				AUPRSA::ptr_t p(new AUPRSA(pa->get_link(), ts.get_starttime(), ts.get_endtime()));
				p->set_activation(ts.get_activation());
				p->set_airspacetype(ts.get_type());
				p->set_icao(ts.is_icao());
				for (unsigned int j(1); j <= 3; ++j)
					p->set_level(j, ts.is_level(j));
				aupdb.save(p);
			}
		}
		const AUPRouteSegment::ptr_t& ps(AUPRouteSegment::ptr_t::cast_dynamic(*oi));
		if (ps) {
			for (unsigned int i(0), n(ps->size()); i < n; ++i) {
				const AUPRouteSegmentTimeSlice& ts(ps->operator[](i).as_routesegment());
				if (!ts.is_valid())
					continue;
				if (ts.get_availability().empty() || ts.get_segments().empty())
					continue;
				AUPCDR::ptr_t p(new AUPCDR(Link(), ts.get_starttime(), ts.get_endtime()));
				for (AUPRouteSegmentTimeSlice::availability_t::const_iterator i(ts.get_availability().begin()), e(ts.get_availability().end()); i != e; ++i)
					p->get_availability().push_back(*i);
				for (AUPRouteSegmentTimeSlice::segments_t::const_iterator i(ts.get_segments().begin()), e(ts.get_segments().end()); i != e; ++i) {
					p->set_obj(*i);
					for (AUPCDR::availability_t::iterator ai(p->get_availability().begin()), ae(p->get_availability().begin()); ai != ae; ++ai)
						ai->set_forward(i->is_forward());
					aupdb.save(p);
				}
			}
		}
	}
}

#include "sysdeps.h"
#include "getopt.h"

int main(int argc, char *argv[])
{
	static struct option long_options[] = {
		{ "dir", required_argument, 0, 'd' },
		{ "aupdir", required_argument, 0, 'a' },
		{ "verbose", no_argument, 0, 'v' },
		{0, 0, 0, 0}
	};
        Glib::ustring db_dir(".");
	Glib::ustring aupdb_dir;
	bool verbose(false);
	int c, err(0);

	while ((c = getopt_long(argc, argv, "d:a:v", long_options, 0)) != EOF) {
		switch (c) {
		case 'd':
			if (optarg)
				db_dir = optarg;
			break;

		case 'a':
			if (optarg)
				aupdb_dir = optarg;
			break;

		case 'v':
			verbose = true;
			break;

		default:
			err++;
			break;
		}
	}
	if (err) {
		std::cerr << "usage: adraupimport [-d <dir>] [-a <aupdir>] [-v]" << std::endl;
		return EX_USAGE;
	}
	try {
		ADR::Database db(db_dir);
		ADR::AUPDatabase aupdb(aupdb_dir.empty() ? db_dir : aupdb_dir);
		// try Write Ahead Logging mode
		aupdb.set_wal(true);
		unsigned int err(0), warn(0);
		{
			ADR::ParseAUP parser(db, verbose);
			parser.set_validate(false); // Do not validate, we do not have a DTD
			parser.set_substitute_entities(true);
			for (; optind < argc; optind++) {
				std::cerr << "Parsing file " << argv[optind] << std::endl;
				parser.parse_file(argv[optind]);
			}
			err += parser.get_errorcnt();
			warn += parser.get_warncnt();
			std::cerr << "Import: " << err << " errors, " << warn << " warnings" << std::endl;
			parser.process(verbose);
			err += parser.get_errorcnt();
			warn += parser.get_warncnt();
			parser.save(aupdb);
		}
		std::cerr << "Processing: " << err << " errors, " << warn << " warnings" << std::endl;
		// disable WAL mode - read only databases are not possible with WAL
		aupdb.set_wal(false);
	} catch (const xmlpp::exception& ex) {
		std::cerr << "libxml++ exception: " << ex.what() << std::endl;
		return EX_DATAERR;
	} catch (const std::exception& ex) {
		std::cerr << "exception: " << ex.what() << std::endl;
		return EX_DATAERR;
	}
	return EX_OK;
}
