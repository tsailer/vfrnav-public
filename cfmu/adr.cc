#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/name_generator.hpp>
#include <iomanip>
#include <sstream>

#include "adr.hh"
#include "adrtimetable.hh"
#include "adrdb.hh"
#include "adrhibernate.hh"
#include "adraerodrome.hh"
#include "adrunit.hh"
#include "adrpoint.hh"
#include "adrroute.hh"
#include "adrairspace.hh"
#include "adrrestriction.hh"

using namespace ADR;

const UUID UUID::niluuid;

UUID::UUID(void)
{
	static_cast<boost::uuids::uuid&>(*this) = boost::uuids::nil_uuid();
}

UUID::UUID(const boost::uuids::uuid& uuid)
{
	static_cast<boost::uuids::uuid&>(*this) = uuid;
}

UUID::UUID(const boost::uuids::uuid& uuid, const std::string& t)
{
	boost::uuids::name_generator gen(uuid);
	static_cast<boost::uuids::uuid&>(*this) = gen(t);
}

UUID::UUID(uint32_t w0, uint32_t w1, uint32_t w2, uint32_t w3)
{
	begin()[ 0] = w0 >> 24;
	begin()[ 1] = w0 >> 16;
	begin()[ 2] = w0 >> 8;
	begin()[ 3] = w0;
	begin()[ 4] = w1 >> 24;
	begin()[ 5] = w1 >> 16;
	begin()[ 6] = w1 >> 8;
	begin()[ 7] = w1;
	begin()[ 8] = w2 >> 24;
	begin()[ 9] = w2 >> 16;
	begin()[10] = w2 >> 8;
	begin()[11] = w2;
	begin()[12] = w3 >> 24;
	begin()[13] = w3 >> 16;
	begin()[14] = w3 >> 8;
	begin()[15] = w3;
}

uint32_t UUID::get_word(unsigned int nr) const
{
	nr &= 3;
	nr <<= 2;
	return (begin()[nr] << 24) | (begin()[nr + 1] << 16) | (begin()[nr + 2] << 8) | begin()[nr + 3];
}

UUID UUID::from_str(const std::string& x)
{
	std::string::size_type pfx(x.compare(0, 9, "urn:uuid:") ? 0 : 9);
	if (x.size() <= pfx)
		return UUID();
	try {
		boost::uuids::string_generator gen;
		UUID u(gen(x.substr(pfx)));
		if (!u.is_nil())
			return u;
	} catch (const std::runtime_error& e) {
	}
	static const boost::uuids::uuid nsuuid = {
		0x5b, 0x20, 0xbd, 0x56,
		0x73, 0x8d,
		0x47, 0xcb,
		0xb1, 0xea,
		0x30, 0x0f, 0x31, 0x7e, 0x8b, 0x32
	};
	return from_str(nsuuid, x.substr(pfx));
}

UUID UUID::from_str(const UUID& uuid, const std::string& x)
{
	boost::uuids::name_generator gen(uuid);
	return gen(x);
}

UUID UUID::from_countryborder(const std::string& x)
{
	static const boost::uuids::uuid nsuuid = {
		0x00, 0xff, 0xf6, 0xf3,
		0xff, 0xf5,
		0x43, 0x74,
		0x96, 0x3b,
		0x0b, 0x8f, 0x11, 0x34, 0x22, 0xb9
	};
	return from_str(nsuuid, x);
}

std::string UUID::to_str(bool prefix) const
{
	std::ostringstream oss;
	if (prefix)
		oss << "urn:uuid:";
	oss << *this;
	return oss.str();
}

void UUID::dependencies(UUID::set_t& dep) const
{
	dep.insert(*this);
}

int UUID::compare(const UUID& x) const
{
	if (*this < x)
		return -1;
	if (x < *this)
		return 1;
	return 0;
}

Link::LinkSet::LinkSet(const base_t& x)
	: base_t(x), m_unlinked(false)
{
}

Link::LinkSet::LinkSet(const LinkSet& x)
	: base_t(x), m_unlinked(x.m_unlinked)
{
}

std::pair<Link::LinkSet::iterator,bool> Link::LinkSet::insert(const Link& link)
{
	m_unlinked = m_unlinked || !link.get_obj();
	std::pair<iterator,bool> r(base_t::insert(link));
	if (!r.second && !r.first->get_obj())
		const_cast<Link&>(*r.first).set_obj(link.get_obj());
	return r;
}

Link::Link(const UUID& uuid, const Glib::RefPtr<Object>& obj)
	: UUID(uuid), m_obj(obj)
{
}

Link::Link(const Glib::RefPtr<Object>& obj)
	: UUID(obj ? obj->get_uuid() : UUID::niluuid), m_obj(obj)
{
}

void Link::load(Database& db)
{
	if (m_obj)
		return;
	m_obj = Object::ptr_t::cast_const(db.load(*this));
}

void Link::dependencies(LinkSet& dep) const
{
	dep.insert(*this);
}

TimeSlice::TimeSlice(timetype_t starttime, timetype_t endtime)
	: m_time(timepair_t(starttime, endtime))
{
}

bool TimeSlice::is_overlap(uint64_t tm0, uint64_t tm1) const
{
	if (!is_valid() || !(tm0 < tm1))
		return false;
	return tm1 > get_starttime() && tm0 < get_endtime();
}

uint64_t TimeSlice::get_overlap(uint64_t tm0, uint64_t tm1) const
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

void TimeSlice::get_bbox(Rect& bbox) const
{
	bbox = Rect(Point::invalid, Point::invalid);
}

void TimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<TimeSlice *>(this))->hibernate(gen);	
}

void TimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<TimeSlice *>(this))->hibernate(gen);
}

void TimeSlice::constraintimediscontinuities(timeset_t& r) const
{
	while (!r.empty() && *r.begin() < get_starttime())
		r.erase(r.begin());
	while (!r.empty()) {
		timeset_t::iterator i(r.end());
		--i;
		if (*i <= get_endtime())
			break;
		r.erase(i);
	}
}

timeset_t TimeSlice::timediscontinuities(void) const
{
	timeset_t r;
	if (!is_valid())
		return r;
	r.insert(get_starttime());
	r.insert(get_endtime());
	Link::LinkSet dep;
	dependencies(dep);
	for (Link::LinkSet::const_iterator di(dep.begin()), de(dep.end()); di != de; ++di) {
		if (!di->get_obj())
			continue;
		timeset_t r1(di->get_obj()->timediscontinuities());
		r.insert(r1.begin(), r1.end());
	}
	constraintimediscontinuities(r);
	return r;
}

IdentTimeSlice& TimeSlice::as_ident(void)
{
	return const_cast<IdentTimeSlice&>(Object::invalid_identtimeslice);
}

const IdentTimeSlice& TimeSlice::as_ident(void) const
{
	return Object::invalid_identtimeslice;
}

PointIdentTimeSlice& TimeSlice::as_point(void)
{
	return const_cast<PointIdentTimeSlice&>(Object::invalid_pointidenttimeslice);
}

const PointIdentTimeSlice& TimeSlice::as_point(void) const
{
	return Object::invalid_pointidenttimeslice;
}

ElevPointIdentTimeSlice& TimeSlice::as_elevpoint(void)
{
	return const_cast<ElevPointIdentTimeSlice&>(Object::invalid_elevpointidenttimeslice);
}

const ElevPointIdentTimeSlice& TimeSlice::as_elevpoint(void) const
{
	return Object::invalid_elevpointidenttimeslice;
}

AirportTimeSlice& TimeSlice::as_airport(void)
{
	return const_cast<AirportTimeSlice&>(Airport::invalid_timeslice);
}

const AirportTimeSlice& TimeSlice::as_airport(void) const
{
	return Airport::invalid_timeslice;
}

AirportCollocationTimeSlice& TimeSlice::as_airportcollocation(void)
{
	return const_cast<AirportCollocationTimeSlice&>(AirportCollocation::invalid_timeslice);
}

const AirportCollocationTimeSlice& TimeSlice::as_airportcollocation(void) const
{
	return AirportCollocation::invalid_timeslice;
}

OrganisationAuthorityTimeSlice& TimeSlice::as_organisationauthority(void)
{
	return const_cast<OrganisationAuthorityTimeSlice&>(OrganisationAuthority::invalid_timeslice);
}

const OrganisationAuthorityTimeSlice& TimeSlice::as_organisationauthority(void) const
{
	return OrganisationAuthority::invalid_timeslice;
}

AirTrafficManagementServiceTimeSlice& TimeSlice::as_airtrafficmanagementservice(void)
{
	return const_cast<AirTrafficManagementServiceTimeSlice&>(AirTrafficManagementService::invalid_timeslice);
}

const AirTrafficManagementServiceTimeSlice& TimeSlice::as_airtrafficmanagementservice(void) const
{
	return AirTrafficManagementService::invalid_timeslice;
}

UnitTimeSlice& TimeSlice::as_unit(void)
{
	return const_cast<UnitTimeSlice&>(Unit::invalid_timeslice);
}

const UnitTimeSlice& TimeSlice::as_unit(void) const
{
	return Unit::invalid_timeslice;
}

SpecialDateTimeSlice& TimeSlice::as_specialdate(void)
{
	return const_cast<SpecialDateTimeSlice&>(SpecialDate::invalid_timeslice);
}

const SpecialDateTimeSlice& TimeSlice::as_specialdate(void) const
{
	return SpecialDate::invalid_timeslice;
}

NavaidTimeSlice& TimeSlice::as_navaid(void)
{
	return const_cast<NavaidTimeSlice&>(Navaid::invalid_timeslice);
}

const NavaidTimeSlice& TimeSlice::as_navaid(void) const
{
	return Navaid::invalid_timeslice;
}

DesignatedPointTimeSlice& TimeSlice::as_designatedpoint(void)
{
	return const_cast<DesignatedPointTimeSlice&>(DesignatedPoint::invalid_timeslice);
}

const DesignatedPointTimeSlice& TimeSlice::as_designatedpoint(void) const
{
	return DesignatedPoint::invalid_timeslice;
}

AngleIndicationTimeSlice& TimeSlice::as_angleindication(void)
{
	return const_cast<AngleIndicationTimeSlice&>(AngleIndication::invalid_timeslice);
}

const AngleIndicationTimeSlice& TimeSlice::as_angleindication(void) const
{
	return AngleIndication::invalid_timeslice;
}

DistanceIndicationTimeSlice& TimeSlice::as_distanceindication(void)
{
	return const_cast<DistanceIndicationTimeSlice&>(DistanceIndication::invalid_timeslice);
}

const DistanceIndicationTimeSlice& TimeSlice::as_distanceindication(void) const
{
	return DistanceIndication::invalid_timeslice;
}

SegmentTimeSlice& TimeSlice::as_segment(void)
{
	return const_cast<SegmentTimeSlice&>(Object::invalid_segmenttimeslice);
}

const SegmentTimeSlice& TimeSlice::as_segment(void) const
{
	return Object::invalid_segmenttimeslice;
}

StandardInstrumentTimeSlice& TimeSlice::as_sidstar(void)
{
	return const_cast<StandardInstrumentTimeSlice&>(Object::invalid_standardinstrumenttimeslice);
}

const StandardInstrumentTimeSlice& TimeSlice::as_sidstar(void) const
{
	return Object::invalid_standardinstrumenttimeslice;
}

StandardInstrumentDepartureTimeSlice& TimeSlice::as_sid(void)
{
	return const_cast<StandardInstrumentDepartureTimeSlice&>(StandardInstrumentDeparture::invalid_timeslice);
}

const StandardInstrumentDepartureTimeSlice& TimeSlice::as_sid(void) const
{
	return StandardInstrumentDeparture::invalid_timeslice;
}

StandardInstrumentArrivalTimeSlice& TimeSlice::as_star(void)
{
	return const_cast<StandardInstrumentArrivalTimeSlice&>(StandardInstrumentArrival::invalid_timeslice);
}

const StandardInstrumentArrivalTimeSlice& TimeSlice::as_star(void) const
{
	return StandardInstrumentArrival::invalid_timeslice;
}

DepartureLegTimeSlice& TimeSlice::as_departureleg(void)
{
	return const_cast<DepartureLegTimeSlice&>(DepartureLeg::invalid_timeslice);
}

const DepartureLegTimeSlice& TimeSlice::as_departureleg(void) const
{
	return DepartureLeg::invalid_timeslice;
}

ArrivalLegTimeSlice& TimeSlice::as_arrivalleg(void)
{
	return const_cast<ArrivalLegTimeSlice&>(ArrivalLeg::invalid_timeslice);
}

const ArrivalLegTimeSlice& TimeSlice::as_arrivalleg(void) const
{
	return ArrivalLeg::invalid_timeslice;
}

RouteTimeSlice& TimeSlice::as_route(void)
{
	return const_cast<RouteTimeSlice&>(Route::invalid_timeslice);
}

const RouteTimeSlice& TimeSlice::as_route(void) const
{
	return Route::invalid_timeslice;
}

RouteSegmentTimeSlice& TimeSlice::as_routesegment(void)
{
	return const_cast<RouteSegmentTimeSlice&>(RouteSegment::invalid_timeslice);
}

const RouteSegmentTimeSlice& TimeSlice::as_routesegment(void) const
{
	return RouteSegment::invalid_timeslice;
}

StandardLevelColumnTimeSlice& TimeSlice::as_standardlevelcolumn(void)
{
	return const_cast<StandardLevelColumnTimeSlice&>(StandardLevelColumn::invalid_timeslice);
}

const StandardLevelColumnTimeSlice& TimeSlice::as_standardlevelcolumn(void) const
{
	return StandardLevelColumn::invalid_timeslice;
}

StandardLevelTableTimeSlice& TimeSlice::as_standardleveltable(void)
{
	return const_cast<StandardLevelTableTimeSlice&>(StandardLevelTable::invalid_timeslice);
}

const StandardLevelTableTimeSlice& TimeSlice::as_standardleveltable(void) const
{
	return StandardLevelTable::invalid_timeslice;
}

AirspaceTimeSlice& TimeSlice::as_airspace(void)
{
	return const_cast<AirspaceTimeSlice&>(Airspace::invalid_timeslice);
}

const AirspaceTimeSlice& TimeSlice::as_airspace(void) const
{
	return Airspace::invalid_timeslice;
}

FlightRestrictionTimeSlice& TimeSlice::as_flightrestriction(void)
{
	return const_cast<FlightRestrictionTimeSlice&>(FlightRestriction::invalid_timeslice);
}

const FlightRestrictionTimeSlice& TimeSlice::as_flightrestriction(void) const
{
	return FlightRestriction::invalid_timeslice;
}


std::ostream& TimeSlice::print(std::ostream& os) const
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

const TimeSlice Object::invalid_timeslice(0, 0);
const IdentTimeSlice Object::invalid_identtimeslice(0, 0);
const PointIdentTimeSlice Object::invalid_pointidenttimeslice(0, 0);
const ElevPointIdentTimeSlice Object::invalid_elevpointidenttimeslice(0, 0);
const SegmentTimeSlice Object::invalid_segmenttimeslice(0, 0);
const StandardInstrumentTimeSlice Object::invalid_standardinstrumenttimeslice(0, 0);

Object::Object(const UUID& uuid)
	: m_uuid(uuid), m_modified(0), m_refcount(1), m_dirty(false)
{
}

Object::~Object()
{
}

void Object::reference(void) const
{
        g_atomic_int_inc(&m_refcount);
}

void Object::unreference(void) const
{
        if (!g_atomic_int_dec_and_test(&m_refcount))
                return;
        delete this;
}

bool Object::empty(void) const
{
	if (this)
		return ts_empty();
	return true;
}

unsigned int Object::size(void) const
{
	if (this)
		return ts_size();
	return 0;
}

TimeSlice& Object::operator[](unsigned int idx)
{
	if (this)
		return ts_get(idx);
	return const_cast<TimeSlice&>(invalid_timeslice);
}

const TimeSlice& Object::operator[](unsigned int idx) const
{
	if (this)
		return ts_get(idx);
	return invalid_timeslice;
}

TimeSlice& Object::operator()(uint64_t tm)
{
	if (!this)
		return const_cast<TimeSlice&>(invalid_timeslice);
	for (unsigned int i(0), n(ts_size()); i < n; ++i) {
		TimeSlice& ts(ts_get(i));
		if (ts.is_inside(tm))
			return ts;
	}
	return const_cast<TimeSlice&>(invalid_timeslice);
}

const TimeSlice& Object::operator()(uint64_t tm) const
{
	if (!this)
		return invalid_timeslice;
	for (unsigned int i(0), n(ts_size()); i < n; ++i) {
		const TimeSlice& ts(ts_get(i));
		if (ts.is_inside(tm))
			return ts;
	}
	return invalid_timeslice;
}

TimeSlice& Object::operator()(uint64_t tm0, uint64_t tm1)
{
	if (!this)
		return const_cast<TimeSlice&>(invalid_timeslice);
	TimeSlice *tsr(&const_cast<TimeSlice&>(invalid_timeslice));
	uint64_t overlap(0);
	for (unsigned int i(0), n(ts_size()); i < n; ++i) {
		TimeSlice& ts(ts_get(i));
		uint64_t o(ts.get_overlap(tm0, tm1));
		if (o < overlap)
			continue;
		overlap = o;
		tsr = &ts;
	}
	return *tsr;
}

const TimeSlice& Object::operator()(uint64_t tm0, uint64_t tm1) const
{
	if (!this)
		return invalid_timeslice;
	const TimeSlice *tsr(&invalid_timeslice);
	uint64_t overlap(0);
	for (unsigned int i(0), n(ts_size()); i < n; ++i) {
		const TimeSlice& ts(ts_get(i));
		uint64_t o(ts.get_overlap(tm0, tm1));
		if (o < overlap)
			continue;
		overlap = o;
		tsr = &ts;
	}
	return *tsr;
}

TimeSlice& Object::operator()(const TimeSlice& ts)
{
	return this->operator()(ts.get_starttime(), ts.get_endtime());
}

const TimeSlice& Object::operator()(const TimeSlice& ts) const
{
	return this->operator()(ts.get_starttime(), ts.get_endtime());
}

TimeSlice& Object::ts_get(unsigned int idx)
{
	return const_cast<TimeSlice&>(invalid_timeslice);
}

const TimeSlice& Object::ts_get(unsigned int idx) const
{
	return invalid_timeslice;
}

bool Object::is_unlinked(void) const
{
	Link::LinkSet dep;
	dependencies(dep);
	return dep.has_unlinked();
}

timeset_t Object::timediscontinuities(void) const
{
	timeset_t r;
	for (unsigned int i(0), n(size()); i < n; ++i) {
		const TimeSlice& ts(operator[](i));
		timeset_t r1(ts.timediscontinuities());
		r.insert(r1.begin(), r1.end());
	}
	return r;
}

void Object::recompute(void)
{
	for (unsigned int i(0), n(size()); i < n; ++i) {
		TimeSlice& ts(operator[](i));
		if (!ts.is_valid())
			continue;
		if (!ts.recompute())
			continue;
		set_modified();
		set_dirty();
	}
}

void Object::recompute(TopoDb30& topodb)
{
	for (unsigned int i(0), n(size()); i < n; ++i) {
		TimeSlice& ts(operator[](i));
		if (!ts.is_valid())
			continue;
		if (!ts.recompute(topodb))
			continue;
		set_modified();
		set_dirty();
	}
}

void Object::recompute(Database& db)
{
	for (unsigned int i(0), n(size()); i < n; ++i) {
		TimeSlice& ts(operator[](i));
		if (!ts.is_valid())
			continue;
		if (!ts.recompute(db))
			continue;
		set_modified();
		set_dirty();
		// recompute bounding boxes as well
		ts.recompute();
	}
}

const std::string& Object::get_type_name(type_t t)
{
	if ((ADR::Object::type_t)(t & ~0x0f) == ADR::Object::type_airport) {
		static const std::string r("airport");
		return r;
	}
	return get_long_type_name(t);
}

const std::string& Object::get_long_type_name(type_t t)
{
	switch (t) {
	case ADR::Object::type_invalid:
	{
		static const std::string r("invalid");
		return r;
	}

	case ADR::Object::type_airportcollocation:
	{
		static const std::string r("airportcollocation");
		return r;
	}

	case ADR::Object::type_organisationauthority:
	{
		static const std::string r("organisationauthority");
		return r;
	}

	case ADR::Object::type_airtrafficmanagementservice:
	{
		static const std::string r("airtrafficmanagementservice");
		return r;
	}

	case ADR::Object::type_unit:
	{
		static const std::string r("unit");
		return r;
	}

	case ADR::Object::type_specialdate:
	{
		static const std::string r("specialdate");
		return r;
	}

	case ADR::Object::type_angleindication:
	{
		static const std::string r("angleindication");
		return r;
	}

	case ADR::Object::type_distanceindication:
	{
		static const std::string r("distanceindication");
		return r;
	}

	case ADR::Object::type_sid:
	{
		static const std::string r("sid");
		return r;
	}

	case ADR::Object::type_star:
	{
		static const std::string r("star");
		return r;
	}

	case ADR::Object::type_route:
	{
		static const std::string r("route");
		return r;
	}

	case ADR::Object::type_standardlevelcolumn:
	{
		static const std::string r("standardlevelcolumn");
		return r;
	}

	case ADR::Object::type_standardleveltable:
	{
		static const std::string r("standardleveltable");
		return r;
	}

	case ADR::Object::type_flightrestriction:
	{
		static const std::string r("flightrestriction");
		return r;
	}

	case ADR::Object::type_navaid:
	{
		static const std::string r("navaid");
		return r;
	}

	case ADR::Object::type_designatedpoint:
	{
		static const std::string r("designatedpoint");
		return r;
	}

	case ADR::Object::type_departureleg:
	{
		static const std::string r("departureleg");
		return r;
	}

	case ADR::Object::type_arrivalleg:
	{
		static const std::string r("arrivalleg");
		return r;
	}

	case ADR::Object::type_routesegment:
	{
		static const std::string r("routesegment");
		return r;
	}

	case ADR::Object::type_airspace:
	{
		static const std::string r("airspace");
		return r;
	}

	case ADR::Object::type_airport:
	{
		static const std::string r("airport");
		return r;
	}

	case ADR::Object::type_airport + AirportTimeSlice::flag_civ:
	{
		static const std::string r("airport(civ)");
		return r;
	}

	case ADR::Object::type_airport + AirportTimeSlice::flag_mil:
	{
		static const std::string r("airport(mil)");
		return r;
	}

	case ADR::Object::type_airport + AirportTimeSlice::flag_mil + AirportTimeSlice::flag_civ:
	{
		static const std::string r("airport(civ,mil)");
		return r;
	}

	case ADR::Object::type_airport + AirportTimeSlice::flag_depifr:
	{
		static const std::string r("airport(depifr)");
		return r;
	}

	case ADR::Object::type_airport + AirportTimeSlice::flag_depifr + AirportTimeSlice::flag_civ:
	{
		static const std::string r("airport(depifr,civ)");
		return r;
	}

	case ADR::Object::type_airport + AirportTimeSlice::flag_depifr + AirportTimeSlice::flag_mil:
	{
		static const std::string r("airport(depifr,mil)");
		return r;
	}

	case ADR::Object::type_airport + AirportTimeSlice::flag_depifr + AirportTimeSlice::flag_mil + AirportTimeSlice::flag_civ:
	{
		static const std::string r("airport(depifr,civ,mil)");
		return r;
	}

	case ADR::Object::type_airport + AirportTimeSlice::flag_arrifr:
	{
		static const std::string r("airport(arrifr)");
		return r;
	}

	case ADR::Object::type_airport + AirportTimeSlice::flag_arrifr + AirportTimeSlice::flag_civ:
	{
		static const std::string r("airport(arrifr,civ)");
		return r;
	}

	case ADR::Object::type_airport + AirportTimeSlice::flag_arrifr + AirportTimeSlice::flag_mil:
	{
		static const std::string r("airport(arrifr,mil)");
		return r;
	}

	case ADR::Object::type_airport + AirportTimeSlice::flag_arrifr + AirportTimeSlice::flag_mil + AirportTimeSlice::flag_civ:
	{
		static const std::string r("airport(arrifr,civ,mil)");
		return r;
	}

	case ADR::Object::type_airport + AirportTimeSlice::flag_arrifr + AirportTimeSlice::flag_depifr:
	{
		static const std::string r("airport(arrifr,depifr)");
		return r;
	}

	case ADR::Object::type_airport + AirportTimeSlice::flag_arrifr + AirportTimeSlice::flag_depifr + AirportTimeSlice::flag_civ:
	{
		static const std::string r("airport(arrifr,depifr,civ)");
		return r;
	}

	case ADR::Object::type_airport + AirportTimeSlice::flag_arrifr + AirportTimeSlice::flag_depifr + AirportTimeSlice::flag_mil:
	{
		static const std::string r("airport(arrifr,depifr,mil)");
		return r;
	}

	case ADR::Object::type_airport + AirportTimeSlice::flag_arrifr + AirportTimeSlice::flag_depifr + AirportTimeSlice::flag_mil + AirportTimeSlice::flag_civ:
	{
		static const std::string r("airport(arrifr,depifr,civ,mil)");
		return r;
	}

	default:
	{
		static const std::string r("?""?");
		return r;
	}
	}
}

Object::ptr_t Object::create(type_t t, const UUID& uuid)
{
	switch (t) {
	case type_invalid:
	{
		ptr_t p(new ObjectImpl<TimeSlice,Object::type_invalid>(uuid));
		return p;
	}

	case type_airportcollocation:
	{
		ptr_t p(new AirportCollocation(uuid));
		return p;
	}

	case type_organisationauthority:
	{
		ptr_t p(new OrganisationAuthority(uuid));
		return p;
	}

	case type_airtrafficmanagementservice:
	{
		ptr_t p(new AirTrafficManagementService(uuid));
		return p;
	}

	case type_unit:
	{
		ptr_t p(new Unit(uuid));
		return p;
	}

	case type_specialdate:
	{
		ptr_t p(new SpecialDate(uuid));
		return p;
	}

	case type_angleindication:
	{
		ptr_t p(new AngleIndication(uuid));
		return p;
	}

	case type_distanceindication:
	{
		ptr_t p(new DistanceIndication(uuid));
		return p;
	}

	case type_sid:
	{
		ptr_t p(new StandardInstrumentDeparture(uuid));
		return p;
	}

	case type_star:
	{
		ptr_t p(new StandardInstrumentArrival(uuid));
		return p;
	}

	case type_route:
	{
		ptr_t p(new Route(uuid));
		return p;
	}

	case type_standardlevelcolumn:
	{
		ptr_t p(new StandardLevelColumn(uuid));
		return p;
	}

	case type_standardleveltable:
	{
		ptr_t p(new StandardLevelTable(uuid));
		return p;
	}

	case type_flightrestriction:
	{
		ptr_t p(new FlightRestriction(uuid));
		return p;
	}

	case type_navaid:
	{
		ptr_t p(new Navaid(uuid));
		return p;
	}

	case type_designatedpoint:
	{
		ptr_t p(new DesignatedPoint(uuid));
		return p;
	}

	case type_departureleg:
	{
		ptr_t p(new DepartureLeg(uuid));
		return p;
	}

	case type_arrivalleg:
	{
		ptr_t p(new ArrivalLeg(uuid));
		return p;
	}

	case type_routesegment:
	{
		ptr_t p(new RouteSegment(uuid));
		return p;
	}

	case type_airspace:
	{
		ptr_t p(new Airspace(uuid));
		return p;
	}

	default:
	{
		if ((type_t)(t & ~0x0f) == type_airport) {
			ptr_t p(new Airport(uuid));
			return p;
		}
		std::ostringstream oss;
		oss << "Object::create: invalid type " << to_str(t) << " (" << (unsigned int)t << ')';
		throw std::runtime_error(oss.str());
	}
	}
	return ptr_t();
}

Object::ptr_t Object::create(ArchiveReadStream& ar, const UUID& uuid)
{
	type_t t;
	ar.iouint8(t);
	ptr_t p(create(t, uuid));
	p->load(ar);
	return p;
}

Object::ptr_t Object::create(ArchiveReadBuffer& ar, const UUID& uuid)
{
	type_t t;
	ar.iouint8(t);
	ptr_t p(create(t, uuid));
	p->load(ar);
	return p;
}

std::pair<uint64_t,uint64_t> Object::get_timebounds(void) const
{
	std::pair<uint64_t,uint64_t> r(std::numeric_limits<uint64_t>::max(),
				       std::numeric_limits<uint64_t>::min());
	if (!this)
		return r;
	for (unsigned int i(0), n(ts_size()); i < n; ++i) {
		const TimeSlice& ts(ts_get(i));
		if (!ts.is_valid())
			continue;
		r.first = std::min(r.first, ts.get_starttime());
		r.second = std::max(r.second, ts.get_endtime());
	}
	return r;
}

IntervalSet<uint64_t> Object::get_timeintervals(void) const
{
	IntervalSet<uint64_t> r;
	if (!this)
		return r;
	for (unsigned int i(0), n(ts_size()); i < n; ++i) {
		const TimeSlice& ts(ts_get(i));
		if (!ts.is_valid())
			continue;
		r |= IntervalSet<uint64_t>::Intvl(ts.get_starttime(),
						  ts.get_endtime());
	}
	return r;
}

bool Object::has_overlap(timetype_t tm0, timetype_t tm1) const
{
	for (unsigned int i(0), n(ts_size()); i < n; ++i) {
		const TimeSlice& ts(ts_get(i));
		if (!ts.is_valid())
			continue;
		if (ts.is_overlap(tm0, tm1))
			return true;
	}
	return false;
}

bool Object::has_nonoverlap(timetype_t tm0, timetype_t tm1) const
{
	for (unsigned int i(0), n(ts_size()); i < n; ++i) {
		const TimeSlice& ts(ts_get(i));
		if (!ts.is_valid())
			return true;
		if (!ts.is_overlap(tm0, tm1))
			return true;
	}
	return false;
}

void Object::set_modified(uint64_t m)
{
	m_modified = m;
	set_dirty();
}

void Object::set_modified(void)
{
	Glib::TimeVal tv;
	tv.assign_current_time();
	set_modified(tv.tv_sec < 0 ? 0 : tv.tv_sec);
}

void Object::get_bbox(Rect& bbox) const
{
	const unsigned int n(size());
	if (!n) {
		bbox = Rect(Point::invalid, Point::invalid);
		return;
	}
	Rect bbox1;
	operator[](0).get_bbox(bbox1);
	for (unsigned int i(1); i < n; ++i) {
		Rect bbox2;
		operator[](i).get_bbox(bbox2);
		bbox1 = bbox1.add(bbox2);
	}
	bbox = bbox1;
}

bool Object::set(uint64_t tm, FPlanWaypoint& el) const
{
	{
		AirportsDb::Airport elx;
		if (get(tm, elx)) {
			el.set(elx);
			return true;
		}
	}
	{
		NavaidsDb::Navaid elx;
		if (get(tm, elx)) {
			el.set(elx);
			return true;
		}
	}
	{
		WaypointsDb::Waypoint elx;
		if (get(tm, elx)) {
			el.set(elx);
			return true;
		}
	}
	return false;
}

unsigned int Object::insert(uint64_t tm, FPlanRoute& route, uint32_t nr) const
{
	{
		AirportsDb::Airport elx;
		if (get(tm, elx))
			return route.insert(elx, nr);
	}
	{
		NavaidsDb::Navaid elx;
		if (get(tm, elx))
			return route.insert(elx, nr);
	}
	{
		WaypointsDb::Waypoint elx;
		if (get(tm, elx))
			return route.insert(elx, nr);
	}
	return false;
}

std::ostream& Object::print(std::ostream& os) const
{
	os << get_uuid() << ' ' << get_long_type_name();
	{
		Rect bbox;
		get_bbox(bbox);
		if (!bbox.get_southwest().is_invalid() && !bbox.get_northeast().is_invalid())
			os << ' ' << bbox.get_southwest().get_lat_str2()
			   << ' ' << bbox.get_southwest().get_lon_str2()
			   << ' ' << bbox.get_northeast().get_lat_str2()
			   << ' ' << bbox.get_northeast().get_lon_str2();
	}
	{
		Glib::TimeVal tv(get_modified(), 0);
		os << ' ' << tv.as_iso8601();
	}
	if (is_dirty())
		os << " D";
	os << std::endl;
	for (unsigned int i(0), n(size()); i < n; ++i) {
		const TimeSlice& ts(operator[](i));
		if (false) {
			Glib::TimeVal tv0(ts.get_starttime(), 0);
			Glib::TimeVal tv1(ts.get_endtime(), 0);
			std::cerr << "  [" << i << '/' << n << "]: " << tv0.as_iso8601() << ' ' << tv1.as_iso8601()
				  << " (" << ts.get_starttime() << ' ' << ts.get_endtime() << ')' << std::endl;
		}
		if (!ts.is_valid())
			continue;
		ts.print(os << "  ") << std::endl;
	}
	return os;
}

template<class TS, int TYP>
const typename ObjectImpl<TS,TYP>::TimeSliceImpl ObjectImpl<TS,TYP>::invalid_timeslice(0, 0);

template<class TS, int TYP>
ObjectImpl<TS,TYP>::ObjectImpl(const UUID& uuid)
	: Object(uuid)
{
}

template<class TS, int TYP>
void ObjectImpl<TS,TYP>::clone_impl(const ptr_t& p) const
{
	p->m_modified = m_modified;
	p->m_dirty = m_dirty;
	p->m_timeslice = m_timeslice;
	for (typename timeslice_t::iterator i(p->m_timeslice.begin()), e(p->m_timeslice.end()); i != e; ++i) {
		i->clone();
	}
}

template<class TS, int TYP>
void ObjectImpl<TS,TYP>::link(Database& db, unsigned int depth)
{
	LinkLoader ll(db, depth);
	for (typename timeslice_t::iterator i(m_timeslice.begin()), e(m_timeslice.end()); i != e; ++i) {
		i->hibernate(ll);
	}
}

template<class TS, int TYP>
void ObjectImpl<TS,TYP>::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<objtype_t *>(this))->hibernate(gen);
}

template<class TS, int TYP>
void ObjectImpl<TS,TYP>::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<objtype_t *>(this))->hibernate(gen);
}

template<class TS, int TYP>
void ObjectImpl<TS,TYP>::load(ArchiveReadStream& ar)
{
	hibernate(ar);
}

template<class TS, int TYP>
void ObjectImpl<TS,TYP>::load(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

template<class TS, int TYP>
void ObjectImpl<TS,TYP>::save(ArchiveWriteStream& ar) const
{
	type_t t(get_type());
	ar.iouint8(t);
	(const_cast<objtype_t *>(this))->hibernate(ar);
}

template<class TS, int TYP>
TimeSlice& ObjectImpl<TS,TYP>::ts_get(unsigned int idx)
{
	if (this && idx < size())
		return m_timeslice[idx];
	return const_cast<TimeSliceImpl&>(invalid_timeslice);
}

template<class TS, int TYP>
const TimeSlice& ObjectImpl<TS,TYP>::ts_get(unsigned int idx) const
{
	if (this && idx < size())
		return m_timeslice[idx];
	return invalid_timeslice;
}


template<class TS, int TYP>
class ObjectImpl<TS,TYP>::SortTimeSlice {
public:
	bool operator()(const TimeSlice& a, const TimeSlice& b) const {
		if (a.get_starttime() < b.get_starttime())
			return true;
		if (a.get_starttime() > b.get_starttime())
			return false;
		return a.get_endtime() < b.get_endtime();
	}
};


template<class TS, int TYP>
void ObjectImpl<TS,TYP>::clean_timeslices(uint64_t cutoff)
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
		if (i2 != e && i->get_endtime() > i2->get_starttime()) {
			i->set_endtime(i2->get_starttime());
			set_modified();
		}
		if (i->is_valid() && i->get_endtime() > cutoff) {
			i = i2;
			continue;
		}
		i = m_timeslice.erase(i);
		e = m_timeslice.end();
		set_modified();
		set_dirty();
	}
	if (false) {
		std::cerr << "Timeslices after cleaning: " << size() << std::endl;
		for (typename timeslice_t::iterator i(m_timeslice.begin()), e(m_timeslice.end()); i != e; ++i)
			std::cerr << "  " << i->get_starttime() << ' ' << i->get_endtime() << std::endl;
	}
}

template<class TS, int TYP>
void ObjectImpl<TS,TYP>::add_timeslice(const TimeSliceImpl& ts)
{
	if (!ts.is_valid())
		return;
	if (false) {
		Glib::TimeVal tv0(ts.get_starttime(), 0);
		Glib::TimeVal tv1(ts.get_endtime(), 0);
		std::cerr << "add_timeslice: " << tv0.as_iso8601() << ' ' << tv1.as_iso8601()
			  << " (" << ts.get_starttime() << ' ' << ts.get_endtime() << ')' << std::endl;
	}
	for (typename timeslice_t::iterator i(m_timeslice.begin()), e(m_timeslice.end()); i != e; ++i)
		if (i->get_starttime() >= ts.get_starttime() && i->get_starttime() < ts.get_endtime())
			i->set_starttime(ts.get_endtime());
	m_timeslice.push_back(ts);
	set_modified();
	set_dirty();
	clean_timeslices();
}

template<class TS, int TYP>
Object::ptr_t ObjectImpl<TS,TYP>::simplify_time(timetype_t tm0, timetype_t tm1) const
{
	if (!has_nonoverlap(tm0, tm1))
		return ptr_t();
	ObjectImpl<TS,TYP>::ptr_t p(clone_obj());
	for (typename timeslice_t::iterator i(p->m_timeslice.begin()), e(p->m_timeslice.end()); i != e;) {
		if (i->is_overlap(tm0, tm1)) {
			++i;
			continue;
		}
		i = p->m_timeslice.erase(i);
		e = p->m_timeslice.end();
	}
	return p;
}

IdentTimeSlice::IdentTimeSlice(timetype_t starttime, timetype_t endtime)
	: TimeSlice(starttime, endtime)
{
}

IdentTimeSlice& IdentTimeSlice::as_ident(void)
{
	if (this)
		return *this;
	return const_cast<IdentTimeSlice&>(Object::invalid_identtimeslice);
}

const IdentTimeSlice& IdentTimeSlice::as_ident(void) const
{
	if (this)
		return *this;
	return Object::invalid_identtimeslice;
}

void IdentTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<IdentTimeSlice *>(this))->hibernate(gen);	
}

void IdentTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<IdentTimeSlice *>(this))->hibernate(gen);
}

std::ostream& IdentTimeSlice::print(std::ostream& os) const
{
	TimeSlice::print(os);
	return os << " ident " << get_ident();
}

PointIdentTimeSlice::PointIdentTimeSlice(timetype_t starttime, timetype_t endtime)
	: IdentTimeSlice(starttime, endtime)
{
	m_coord.set_invalid();
}

PointIdentTimeSlice& PointIdentTimeSlice::as_point(void)
{
	if (this)
		return *this;
	return const_cast<PointIdentTimeSlice&>(Object::invalid_pointidenttimeslice);
}

const PointIdentTimeSlice& PointIdentTimeSlice::as_point(void) const
{
	if (this)
		return *this;
	return Object::invalid_pointidenttimeslice;
}

void PointIdentTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<PointIdentTimeSlice *>(this))->hibernate(gen);	
}

void PointIdentTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<PointIdentTimeSlice *>(this))->hibernate(gen);
}

std::ostream& PointIdentTimeSlice::print(std::ostream& os) const
{
        IdentTimeSlice::print(os);
	os << " coord ";
	if (!get_coord().is_invalid())
		os << get_coord().get_lat_str2() << ' ' << get_coord().get_lon_str2();
	else
		os << "nil";
	return os;
}

void PointIdentTimeSlice::get_bbox(Rect& bbox) const
{
	bbox = Rect(get_coord(), get_coord());
}

const ElevPointIdentTimeSlice::elev_t ElevPointIdentTimeSlice::invalid_elev = std::numeric_limits<elev_t>::min();

ElevPointIdentTimeSlice::ElevPointIdentTimeSlice(timetype_t starttime, timetype_t endtime)
	: PointIdentTimeSlice(starttime, endtime), m_elev(invalid_elev)
{
}

ElevPointIdentTimeSlice& ElevPointIdentTimeSlice::as_elevpoint(void)
{
	if (this)
		return *this;
	return const_cast<ElevPointIdentTimeSlice&>(Object::invalid_elevpointidenttimeslice);
}

const ElevPointIdentTimeSlice& ElevPointIdentTimeSlice::as_elevpoint(void) const
{
	if (this)
		return *this;
	return Object::invalid_elevpointidenttimeslice;
}

void ElevPointIdentTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<ElevPointIdentTimeSlice *>(this))->hibernate(gen);	
}

void ElevPointIdentTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<ElevPointIdentTimeSlice *>(this))->hibernate(gen);
}

bool ElevPointIdentTimeSlice::recompute(TopoDb30& topodb)
{
	bool work(PointIdentTimeSlice::recompute(topodb));
	if (!is_elev_valid() && !get_coord().is_invalid()) {
		TopoDb30::elev_t e(topodb.get_elev(get_coord()));
		if (e != TopoDb30::nodata) {
			if (e == TopoDb30::ocean)
				e = 0;
			set_elev(Point::round<elev_t,float>(e * Point::m_to_ft));
			work = true;
		}
	}
	return work;
}

std::ostream& ElevPointIdentTimeSlice::print(std::ostream& os) const
{
	PointIdentTimeSlice::print(os);
	if (is_elev_valid())
		os << ' ' << get_elev() << "ft";
	return os;
}

const int32_t AltRange::altmax = std::numeric_limits<int32_t>::max();
const int32_t AltRange::altignore = std::numeric_limits<int32_t>::min() + 1;
const int32_t AltRange::altinvalid = std::numeric_limits<int32_t>::min();

AltRange::AltRange(int32_t lwr, alt_t lwrmode, int32_t upr, alt_t uprmode)
	: m_lwralt(lwr), m_upralt(upr), m_mode((lwrmode & 0x0f) | ((uprmode & 0x0f) << 4))
{
}
	
const std::string& AltRange::get_mode_string(alt_t x)
{
	switch (x) {
	case alt_qnh:
	{
		static const std::string r("ALT");
		return r;
	}

	case alt_std:
	{
		static const std::string r("STD");
		return r;
	}

	case alt_height:
	{
		static const std::string r("HEI");
		return r;
	}

	case alt_floor:
	{
		static const std::string r("FLOOR");
		return r;
	}

	case alt_ceiling:
	{
		static const std::string r("CEIL");
		return r;
	}

	case alt_invalid:
	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

void AltRange::set_lower_mode(const std::string& x)
{
	for (alt_t a(alt_first); a < alt_invalid; a = (alt_t)(a + 1))
		if (get_mode_string(a) == x) {
			set_lower_mode(a);
			return;
		}
	set_lower_mode(alt_invalid);
}

void AltRange::set_upper_mode(const std::string& x)
{
	for (alt_t a(alt_first); a < alt_invalid; a = (alt_t)(a + 1))
		if (get_mode_string(a) == x) {
			set_upper_mode(a);
			return;
		}
	set_upper_mode(alt_invalid);
}

bool AltRange::is_lower_valid(void) const
{
	return get_lower_mode() < alt_invalid;
}

bool AltRange::is_upper_valid(void) const
{
	return get_upper_mode() < alt_invalid;
}

bool AltRange::is_valid(void) const
{
	return is_lower_valid() && is_upper_valid();
}

int32_t AltRange::get_lower_alt_if_valid(void) const
{
	if (is_lower_valid())
		return get_lower_alt();
	return altinvalid;
}

int32_t AltRange::get_upper_alt_if_valid(void) const
{
	if (is_upper_valid())
		return get_upper_alt();
	return altinvalid;
}

bool AltRange::operator==(const AltRange& x) const
{
	return (get_upper_alt() == x.get_upper_alt() &&
		get_lower_alt() == x.get_lower_alt() &&
		get_upper_mode() == x.get_upper_mode() &&
		get_lower_mode() == x.get_lower_mode());
}

bool AltRange::is_inside(int32_t alt) const
{
	if (alt == altinvalid)
		return false;
	if (alt == altignore)
		return true;
	switch (get_lower_mode()) {
	case alt_qnh:
	case alt_std:
	case alt_height:
	case alt_floor:
		if (get_lower_alt() > alt)
			return false;
		break;

	default:
		break;
	}
	switch (get_upper_mode()) {
	case alt_qnh:
	case alt_std:
	case alt_height:
	case alt_ceiling:
		if (get_upper_alt() < alt)
			return false;
		break;

	default:
		break;
	}
	return true;
}

bool AltRange::is_inside(int32_t alt0, int32_t alt1) const
{
	if (alt0 > alt1)
		return false;
	if (alt0 == altinvalid || alt1 == altinvalid)
		return false;
	if (alt0 == altignore || alt1 == altignore)
		return true;
	switch (get_lower_mode()) {
	case alt_qnh:
	case alt_std:
	case alt_height:
	case alt_floor:
		if (get_lower_alt() > alt0)
			return false;
		break;

	default:
		break;
	}
	switch (get_upper_mode()) {
	case alt_qnh:
	case alt_std:
	case alt_height:
	case alt_ceiling:
		if (get_upper_alt() < alt1)
			return false;
		break;

	default:
		break;
	}
	return true;
}

bool AltRange::is_overlap(int32_t alt0, int32_t alt1) const
{
	if (alt0 > alt1)
		return false;
	if (alt0 == altinvalid && alt1 == altinvalid)
		return true;
	if (alt0 == altinvalid || alt1 == altinvalid)
		return false;
	if (is_lower_valid() && get_lower_alt() > alt1)
		return false;
	if (is_upper_valid() && get_upper_alt() < alt0)
		return false;
	return true;
}

bool AltRange::is_empty(void) const
{
	switch (get_lower_mode()) {
	case alt_qnh:
	case alt_std:
	case alt_height:
	case alt_floor:
		switch (get_upper_mode()) {
		case alt_qnh:
		case alt_std:
		case alt_height:
		case alt_ceiling:
			return get_lower_alt() > get_upper_alt();

		default:
			break;
		}
		break;

	default:
		break;
	}
	return false;
}

void AltRange::merge(const AltRange& r)
{
	switch (get_lower_mode()) {
	case alt_qnh:
	case alt_std:
	case alt_height:
		break;

	case alt_floor:
		switch (r.get_lower_mode()) {
		case alt_qnh:
		case alt_std:
		case alt_height:
			set_lower_mode(r.get_lower_mode());
			set_lower_alt(std::max(r.get_lower_alt(), get_lower_alt()));
			break;

		default:
			break;
		}
		break;

	case alt_ceiling:
		switch (r.get_upper_mode()) {
		case alt_qnh:
		case alt_std:
		case alt_height:
			set_lower_mode(r.get_upper_mode());
			set_lower_alt(std::min(r.get_upper_alt(), get_lower_alt()));
			break;

		default:
			break;
		}
		break;

	default:
		switch (r.get_lower_mode()) {
		case alt_qnh:
		case alt_std:
		case alt_height:
			set_lower_mode(r.get_lower_mode());
			set_lower_alt(r.get_lower_alt());
			break;

		default:
			break;
		}
		break;
	}
	switch (get_upper_mode()) {
	case alt_qnh:
	case alt_std:
	case alt_height:
		break;

	case alt_floor:
		switch (r.get_lower_mode()) {
		case alt_qnh:
		case alt_std:
		case alt_height:
			set_upper_mode(r.get_lower_mode());
			set_upper_alt(std::max(r.get_lower_alt(), get_upper_alt()));
			break;

		default:
			break;
		}
		break;

	case alt_ceiling:
		switch (r.get_upper_mode()) {
		case alt_qnh:
		case alt_std:
		case alt_height:
			set_upper_mode(r.get_upper_mode());
			set_upper_alt(std::min(r.get_upper_alt(), get_upper_alt()));
			break;

		default:
			break;
		}
		break;

	default:
		switch (r.get_upper_mode()) {
		case alt_qnh:
		case alt_std:
		case alt_height:
			set_upper_mode(r.get_upper_mode());
			set_upper_alt(r.get_upper_alt());
			break;

		default:
			break;
		}
		break;
	}
}

void AltRange::intersect(const AltRange& r)
{
	switch (get_lower_mode()) {
	case alt_qnh:
	case alt_std:
	case alt_height:
	case alt_floor:
		switch (r.get_lower_mode()) {
		case alt_floor:
			set_lower_mode(alt_floor);
			// fall through

		case alt_qnh:
		case alt_std:
		case alt_height:
			set_lower_alt(std::max(get_lower_alt(), r.get_lower_alt()));
			break;

		default:
			break;
		}
		break;

	default:
		switch (r.get_lower_mode()) {
		case alt_qnh:
		case alt_std:
		case alt_height:
		case alt_floor:
			set_lower_mode(r.get_lower_mode());
			set_lower_alt(r.get_lower_alt());
			break;

		default:
			set_lower_mode(alt_invalid);
			break;
		}
		break;
	}
	switch (get_upper_mode()) {
	case alt_qnh:
	case alt_std:
	case alt_height:
	case alt_ceiling:
		switch (r.get_upper_mode()) {
		case alt_ceiling:
			set_upper_mode(alt_ceiling);
			// fall through

		case alt_qnh:
		case alt_std:
		case alt_height:
			set_upper_alt(std::min(get_upper_alt(), r.get_upper_alt()));
			break;

		default:
			break;
		}
		break;

	default:
		switch (r.get_upper_mode()) {
		case alt_qnh:
		case alt_std:
		case alt_height:
		case alt_ceiling:
			set_upper_mode(r.get_upper_mode());
			set_upper_alt(r.get_upper_alt());
			break;

		default:
			set_upper_mode(alt_invalid);
			break;
		}
		break;
	}
}

std::string AltRange::to_str(void) const
{
	if (!(is_lower_valid() || is_upper_valid()))
		return "!A";
	std::ostringstream oss;
	if (is_lower_valid()) {
		int alt(get_lower_alt() / 100);
		switch (get_lower_mode()) {
		case alt_qnh:
			if (alt > 999) {
				oss << "UNL";
				break;
			}
			if (alt <= 0) {
				oss << "MSL";
				break;
			}
			oss << 'A' << std::setw(3) << std::setfill('0') << alt;
			break;

		case alt_std:
			if (alt > 999) {
				oss << "UNL";
				break;
			}
			if (alt <= 0) {
				oss << "MSL";
				break;
			}
			oss << 'F' << std::setw(3) << std::setfill('0') << alt;
			break;

		case alt_height:
			if (alt > 999) {
				oss << "UNL";
				break;
			}
			if (alt <= 0) {
				oss << "GND";
				break;
			}
			oss << 'H' << std::setw(3) << std::setfill('0') << alt;
			break;

		case alt_floor:
			oss << "FLOOR";
			if (alt <= 0)
				break;
			if (alt > 999) {
				oss << "/UNL";
				break;
			}
			{
				std::ostringstream oss2;
				oss2 << std::setw(3) << std::setfill('0') << alt;
				oss << "/F" << oss2.str();
			}
			break;

		case alt_ceiling:
			oss << "CEIL";
			if (alt <= 0)
				break;
			if (alt > 999) {
				oss << "/UNL";
				break;
			}
			{
				std::ostringstream oss2;
				oss2 << std::setw(3) << std::setfill('0') << alt;
				oss << "/F" << oss2.str();
			}
			break;

		default:
			oss << 'X' << std::setw(3) << std::setfill('0') << alt;
			break;
		}
	}
	oss << '-';
	if (is_upper_valid()) {
		int alt((std::min(get_upper_alt(), std::numeric_limits<int32_t>::max() - 99) + 99) / 100);
		switch (get_upper_mode()) {
		case alt_qnh:
			if (alt > 999) {
				oss << "UNL";
				break;
			}
			if (alt <= 0) {
				oss << "MSL";
				break;
			}
			oss << 'A' << std::setw(3) << std::setfill('0') << alt;
			break;

		case alt_std:
			if (alt > 999) {
				oss << "UNL";
				break;
			}
			if (alt <= 0) {
				oss << "MSL";
				break;
			}
			oss << 'F' << std::setw(3) << std::setfill('0') << alt;
			break;

		case alt_height:
			if (alt > 999) {
				oss << "UNL";
				break;
			}
			if (alt <= 0) {
				oss << "GND";
				break;
			}
			oss << 'H' << std::setw(3) << std::setfill('0') << alt;
			break;

		case alt_floor:
			oss << "FLOOR";
			if (alt <= 0) {
				oss << "/MSL";
				break;
			}
			if (alt > 999)
				break;
			{
				std::ostringstream oss2;
				oss2 << std::setw(3) << std::setfill('0') << alt;
				oss << "/F" << oss2.str();
			}
			break;

		case alt_ceiling:
			oss << "CEIL";
			if (alt <= 0) {
				oss << "/MSL";
				break;
			}
			if (alt > 999)
				break;
			{
				std::ostringstream oss2;
				oss2 << std::setw(3) << std::setfill('0') << alt;
				oss << "/F" << oss2.str();
			}
			break;

		default:
			oss << 'X' << std::setw(3) << std::setfill('0') << alt;
			break;
		}
	}
	return oss.str();
}

std::string AltRange::to_shortstr(void) const
{
	if (!(is_lower_valid() || is_upper_valid()))
		return "";
	std::ostringstream oss;
	if (is_lower_valid()) {
		int alt(get_lower_alt() / 100);
		switch (get_lower_mode()) {
		case alt_qnh:
			if (alt > 999) {
				oss << "UNL";
				break;
			}
			if (alt <= 0) {
				oss << "MSL";
				break;
			}
			oss << 'A' << std::setw(3) << std::setfill('0') << alt;
			break;

		case alt_std:
			if (alt > 999) {
				oss << "UNL";
				break;
			}
			if (alt <= 0) {
				oss << "MSL";
				break;
			}
			oss << 'F' << std::setw(3) << std::setfill('0') << alt;
			break;

		case alt_height:
			if (alt > 999) {
				oss << "UNL";
				break;
			}
			if (alt <= 0) {
				oss << "GND";
				break;
			}
			oss << 'H' << std::setw(3) << std::setfill('0') << alt;
			break;

		case alt_floor:
			oss << "FLOOR";
			if (alt <= 0)
				break;
			if (alt > 999) {
				oss << "/UNL";
				break;
			}
			{
				std::ostringstream oss2;
				oss2 << std::setw(3) << std::setfill('0') << alt;
				oss << "/F" << oss2.str();
			}
			break;

		case alt_ceiling:
			oss << "CEIL";
			if (alt <= 0)
				break;
			if (alt > 999) {
				oss << "/UNL";
				break;
			}
			{
				std::ostringstream oss2;
				oss2 << std::setw(3) << std::setfill('0') << alt;
				oss << "/F" << oss2.str();
			}
			break;

		default:
			break;
		}
	}
	oss << "..";
	if (is_upper_valid()) {
		int alt((std::min(get_upper_alt(), std::numeric_limits<int32_t>::max() - 99) + 99) / 100);
		switch (get_upper_mode()) {
		case alt_qnh:
			if (alt > 999) {
				oss << "UNL";
				break;
			}
			if (alt <= 0) {
				oss << "MSL";
				break;
			}
			oss << 'A' << std::setw(3) << std::setfill('0') << alt;
			break;

		case alt_std:
			if (alt > 999) {
				oss << "UNL";
				break;
			}
			if (alt <= 0) {
				oss << "MSL";
				break;
			}
			oss << 'F' << std::setw(3) << std::setfill('0') << alt;
			break;

		case alt_height:
			if (alt > 999) {
				oss << "UNL";
				break;
			}
			if (alt <= 0) {
				oss << "GND";
				break;
			}
			oss << 'H' << std::setw(3) << std::setfill('0') << alt;
			break;

		case alt_floor:
			oss << "FLOOR";
			if (alt <= 0) {
				oss << "/MSL";
				break;
			}
			if (alt > 999)
				break;
			{
				std::ostringstream oss2;
				oss2 << std::setw(3) << std::setfill('0') << alt;
				oss << "/F" << oss2.str();
			}
			break;

		case alt_ceiling:
			oss << "CEIL";
			if (alt <= 0) {
				oss << "/MSL";
				break;
			}
			if (alt > 999)
				break;
			{
				std::ostringstream oss2;
				oss2 << std::setw(3) << std::setfill('0') << alt;
				oss << "/F" << oss2.str();
			}
			break;

		default:
			break;
		}
	}
	return oss.str();
}

IntervalSet<int32_t> AltRange::get_interval(bool invisfull) const
{
	int32_t l(0), u(std::numeric_limits<int32_t>::max());
	switch (get_lower_mode()) {
	case alt_qnh:
	case alt_std:
	case alt_height:
	//case alt_floor:
		l = get_lower_alt();
		break;

	case alt_floor:
	case alt_ceiling:
		break;

	default:
		if (invisfull)
			break;
		return IntervalSet<int32_t>();
	}
	switch (get_upper_mode()) {
	case alt_qnh:
	case alt_std:
	case alt_height:
	//case alt_ceiling:
		u = get_upper_alt();
		if (u != std::numeric_limits<int32_t>::max())
			++u;
		break;

	case alt_floor:
	case alt_ceiling:
		break;

	default:
		if (invisfull)
			break;
		return IntervalSet<int32_t>();
	}
	if (l >= u)
		return IntervalSet<int32_t>();
	return IntervalSet<int32_t>(IntervalSet<int32_t>::Intvl(l, u));
}

BidirAltRange::BidirAltRange(const set_t& set0, const set_t& set1)
{
	m_set[0] = set0;
	m_set[1] = set1;
}

bool BidirAltRange::is_inside(unsigned int index, type_t alt) const
{
	return m_set[!!index].is_inside(alt);
}

bool BidirAltRange::is_empty(void) const
{
	return m_set[0].is_empty() && m_set[1].is_empty();
}

void BidirAltRange::set_empty(void)
{
	m_set[0].set_empty();
	m_set[1].set_empty();
}

void BidirAltRange::set_full(void)
{
	m_set[0].set_full();
	m_set[1].set_full();
}

void BidirAltRange::set_interval(type_t alt0, type_t alt1)
{
	m_set[0] = m_set[1] = set_t::Intvl(alt0, alt1);
}

void BidirAltRange::set(const AltRange& ar)
{
	type_t l(std::numeric_limits<int32_t>::min());
	type_t u(std::numeric_limits<int32_t>::max());
	switch (ar.get_lower_mode()) {
	case AltRange::alt_qnh:
	case AltRange::alt_std:
	case AltRange::alt_height:
		l = ar.get_lower_alt();
		break;

	default:
		break;
	}
	switch (ar.get_upper_mode()) {
	case AltRange::alt_qnh:
	case AltRange::alt_std:
	case AltRange::alt_height:
		u = ar.get_upper_alt();
		if (u != std::numeric_limits<int32_t>::max())
			++u;
		break;

	default:
		break;
	}
	m_set[0] = m_set[1] = set_t::Intvl(l, u);
}

void BidirAltRange::invert(void)
{
	m_set[0] = ~m_set[0];
	m_set[1] = ~m_set[1];
}

void BidirAltRange::swapdir(void)
{
	m_set[0].swap(m_set[1]);
}

BidirAltRange& BidirAltRange::operator&=(const BidirAltRange& x)
{
	m_set[0] &= x.m_set[0];
	m_set[1] &= x.m_set[1];
	return *this;
}

BidirAltRange& BidirAltRange::operator|=(const BidirAltRange& x)
{
	m_set[0] |= x.m_set[0];
	m_set[1] |= x.m_set[1];
	return *this;
}

BidirAltRange& BidirAltRange::operator^=(const BidirAltRange& x)
{
	m_set[0] ^= x.m_set[0];
	m_set[1] ^= x.m_set[1];
	return *this;
}

BidirAltRange& BidirAltRange::operator-=(const BidirAltRange& x)
{
	m_set[0] -= x.m_set[0];
	m_set[1] -= x.m_set[1];
	return *this;
}

BidirAltRange BidirAltRange::operator~(void) const
{
	return BidirAltRange(~m_set[0], ~m_set[1]);
}

int BidirAltRange::compare(const BidirAltRange& x) const
{
	int c(m_set[0].compare(x.m_set[0]));
	if (c)
		return c;
	return m_set[1].compare(x.m_set[1]);
}

std::ostream& BidirAltRange::print(std::ostream& os, unsigned int indent) const
{
	return os << std::endl << std::string(indent, ' ') << "Forward:  " << m_set[0].to_str()
		  << std::endl << std::string(indent, ' ') << "Backward: " << m_set[1].to_str();
}


template class ObjectImpl<TimeSlice,Object::type_invalid>;
template class ObjectImpl<AirportTimeSlice,Object::type_airport>;
template class ObjectImpl<AirportCollocationTimeSlice,Object::type_airportcollocation>;
template class ObjectImpl<OrganisationAuthorityTimeSlice,Object::type_organisationauthority>;
template class ObjectImpl<AirTrafficManagementServiceTimeSlice,Object::type_airtrafficmanagementservice>;
template class ObjectImpl<UnitTimeSlice,Object::type_unit>;
template class ObjectImpl<SpecialDateTimeSlice,Object::type_specialdate>;
template class ObjectImpl<NavaidTimeSlice,Object::type_navaid>;
template class ObjectImpl<DesignatedPointTimeSlice,Object::type_designatedpoint>;
template class ObjectImpl<AngleIndicationTimeSlice,Object::type_angleindication>;
template class ObjectImpl<DistanceIndicationTimeSlice,Object::type_distanceindication>;
template class ObjectImpl<StandardInstrumentArrivalTimeSlice,Object::type_star>;
template class ObjectImpl<StandardInstrumentDepartureTimeSlice,Object::type_sid>;
template class ObjectImpl<DepartureLegTimeSlice,Object::type_departureleg>;
template class ObjectImpl<ArrivalLegTimeSlice,Object::type_arrivalleg>;
template class ObjectImpl<RouteTimeSlice,Object::type_route>;
template class ObjectImpl<RouteSegmentTimeSlice,Object::type_routesegment>;
template class ObjectImpl<StandardLevelColumnTimeSlice,Object::type_standardlevelcolumn>;
template class ObjectImpl<StandardLevelTableTimeSlice,Object::type_standardleveltable>;
template class ObjectImpl<AirspaceTimeSlice,Object::type_airspace>;
template class ObjectImpl<FlightRestrictionTimeSlice,Object::type_flightrestriction>;
