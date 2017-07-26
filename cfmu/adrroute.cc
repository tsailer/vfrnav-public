#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iomanip>
#include <sstream>

#include "adr.hh"
#include "adrdb.hh"
#include "adrhibernate.hh"
#include "adrroute.hh"

using namespace ADR;

SegmentTimeSlice::SegmentTimeSlice(timetype_t starttime, timetype_t endtime)
	: TimeSlice(starttime, endtime), m_terrainelev(ElevPointIdentTimeSlice::invalid_elev),
	  m_corridor5elev(ElevPointIdentTimeSlice::invalid_elev)
{
}

SegmentTimeSlice& SegmentTimeSlice::as_segment(void)
{
	if (this)
		return *this;
	return const_cast<SegmentTimeSlice&>(Object::invalid_segmenttimeslice);
}

const SegmentTimeSlice& SegmentTimeSlice::as_segment(void) const
{
	if (this)
		return *this;
	return Object::invalid_segmenttimeslice;
}

void SegmentTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<SegmentTimeSlice *>(this))->hibernate(gen);	
}

void SegmentTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<SegmentTimeSlice *>(this))->hibernate(gen);
}

bool SegmentTimeSlice::get(uint64_t tm, AirwaysDb::Airway& el) const
{
	if (!is_inside(tm))
		return false;
	if (!get_route().get_obj() ||
	    !get_start().get_obj() ||
	    !get_end().get_obj())
		return false;
	const PointIdentTimeSlice& pts(get_start().get_obj()->operator()(*this).as_point());
	const PointIdentTimeSlice& pte(get_end().get_obj()->operator()(*this).as_point());
	const IdentTimeSlice& id(get_route().get_obj()->operator()(*this).as_ident());
	if (!pts.is_valid() || !pte.is_valid() || !id.is_valid())
		return false;
	el.set_begin_coord(pts.get_coord());
	el.set_begin_name(pts.get_ident());
	el.set_end_coord(pte.get_coord());
	el.set_end_name(pte.get_ident());
	el.set_name(id.get_ident());
	if (is_terrainelev_valid())
		el.set_terrain_elev(get_terrainelev());
	if (is_corridor5elev_valid())
		el.set_corridor5_elev(get_corridor5elev());
	el.set_base_level(is_lower_valid() ? (get_lower_alt() + 99) / 100 : 0);
	el.set_top_level(is_upper_valid() ? get_upper_alt() / 100 : 999);
	el.set_type((el.get_base_level() >= 245) ? AirwaysDb::Airway::airway_high : 
		    ((el.get_top_level() > 245) ? AirwaysDb::Airway::airway_both : AirwaysDb::Airway::airway_low));
	return true;
}

bool SegmentTimeSlice::recompute(void)
{
	bool work(TimeSlice::recompute());
	if (!get_start().get_obj()) {
		std::ostringstream oss;
		oss << "SegmentTimeSlice::recompute: unlinked object " << get_start();
		throw std::runtime_error(oss.str());
	}
	if (!get_end().get_obj()) {
		std::ostringstream oss;
		oss << "SegmentTimeSlice::recompute: unlinked object " << get_end();
		throw std::runtime_error(oss.str());
	}
	const PointIdentTimeSlice& pts(get_start().get_obj()->operator()(*this).as_point());
	if (!pts.is_valid()) {
		std::ostringstream oss;
		oss << "SegmentTimeSlice::recompute: object " << get_start() << " not a point";
		throw std::runtime_error(oss.str());
	}
	const PointIdentTimeSlice& pte(get_end().get_obj()->operator()(*this).as_point());
	if (!pte.is_valid()) {
		std::ostringstream oss;
		oss << "SegmentTimeSlice::recompute: object " << get_end() << " not a point";
		throw std::runtime_error(oss.str());
	}
	Rect bbox(pts.get_coord(), pts.get_coord());
	bbox = bbox.add(pte.get_coord());
	work = work || bbox != m_bbox;
	m_bbox = bbox;
	return work;
}

namespace {

static bool compute_profile_1(TopoDb30& topodb, std::vector<TopoDb30::Profile>& profs, const Point& p0, const Point& p1,
			      unsigned int itercnt = 0)
{
        TopoDb30::Profile prof(topodb.get_profile(p0, p1, 5));
        if (!prof.empty()) {
                profs.push_back(prof);
                return true;
        }
        if (itercnt >= 8) {
                std::cerr << "compute_profile: aborting due to iteration count" << std::endl;
                return false;
        }
        // fixme: should half according to great circle distance
        Point pm(p0.halfway(p1));
        ++itercnt;
	return (compute_profile_1(topodb, profs, p0, pm, itercnt) &&
		compute_profile_1(topodb, profs, pm, p1, itercnt));
}

};

SegmentTimeSlice::profile_t SegmentTimeSlice::compute_profile(TopoDb30& topodb, const Point& p0, const Point& p1)
{
	profile_t p(ElevPointIdentTimeSlice::invalid_elev, ElevPointIdentTimeSlice::invalid_elev);
	std::vector<TopoDb30::Profile> profs;
	if (!compute_profile_1(topodb, profs, p0, p1))
		return p;
	{
		TopoDb30::elev_t elev(std::numeric_limits<TopoDb30::elev_t>::min());
		for (std::vector<TopoDb30::Profile>::const_iterator psi(profs.begin()), pse(profs.end()); psi != pse; ++psi) {
			const TopoDb30::Profile& prof(*psi);
			for (TopoDb30::Profile::const_iterator i(prof.begin()), e(prof.end()); i != e; ++i) {
				TopoDb30::elev_t el(i->get_elev());
				if (el == TopoDb30::ocean)
					el = 0;
				if (el != TopoDb30::nodata)
					elev = std::max(elev, el);
			}
		}
		if (elev != std::numeric_limits<TopoDb30::elev_t>::min())
			p.first = elev * Point::m_to_ft;
	}
	{
		TopoDb30::elev_t elev(std::numeric_limits<TopoDb30::elev_t>::min());
		for (std::vector<TopoDb30::Profile>::const_iterator psi(profs.begin()), pse(profs.end()); psi != pse; ++psi) {
			const TopoDb30::Profile& prof(*psi);
			TopoDb30::elev_t el(prof.get_maxelev());
			if (el == TopoDb30::ocean)
				el = 0;
			if (el != TopoDb30::nodata)
				elev = std::max(elev, el);
		}
		if (elev != std::numeric_limits<TopoDb30::elev_t>::min())
			p.second = elev * Point::m_to_ft;
	}
	return p;
}

SegmentTimeSlice::profile_t SegmentTimeSlice::compute_profile(TopoDb30& topodb) const
{
	if (!get_start().get_obj())
		return profile_t(ElevPointIdentTimeSlice::invalid_elev, ElevPointIdentTimeSlice::invalid_elev);
	if (!get_end().get_obj())
		return profile_t(ElevPointIdentTimeSlice::invalid_elev, ElevPointIdentTimeSlice::invalid_elev);
	const PointIdentTimeSlice& pts(get_start().get_obj()->operator()(*this).as_point());
	if (!pts.is_valid())
		return profile_t(ElevPointIdentTimeSlice::invalid_elev, ElevPointIdentTimeSlice::invalid_elev);
	const PointIdentTimeSlice& pte(get_end().get_obj()->operator()(*this).as_point());
	if (!pte.is_valid())
		return profile_t(ElevPointIdentTimeSlice::invalid_elev, ElevPointIdentTimeSlice::invalid_elev);
	return compute_profile(topodb, pts.get_coord(), pts.get_coord());
}

bool SegmentTimeSlice::recompute(TopoDb30& topodb)
{
	bool work(TimeSlice::recompute(topodb));
	if (!is_terrainelev_valid() || !is_corridor5elev_valid()) {
		profile_t p(compute_profile(topodb));
		if (!is_terrainelev_valid() && p.first != ElevPointIdentTimeSlice::invalid_elev) {
			set_terrainelev(p.first);
			work = true;
		}
		if (!is_corridor5elev_valid() && p.second != ElevPointIdentTimeSlice::invalid_elev) {
			set_corridor5elev(p.second);
			work = true;
		}
	}
	return work;
}

std::ostream& SegmentTimeSlice::print(std::ostream& os) const
{
	TimeSlice::print(os);
	os << " altitude " << m_altrange.to_str();
	if (is_terrainelev_valid())
		os << " terrain " << get_terrainelev();
	if (is_corridor5elev_valid())
		os << " corridor5 " << get_corridor5elev();
	os << std::endl << "    route " << get_route();
	{
		const IdentTimeSlice& id(get_route().get_obj()->operator()(*this).as_ident());
		if (id.is_valid())
			os << ' ' << id.get_ident();
	}
	os << std::endl << "    start " << get_start();
	{
		const IdentTimeSlice& id(get_start().get_obj()->operator()(*this).as_ident());
		if (id.is_valid())
			os << ' ' << id.get_ident();
	}
	os << std::endl << "    end " << get_end();
	{
		const IdentTimeSlice& id(get_end().get_obj()->operator()(*this).as_ident());
		if (id.is_valid())
			os << ' ' << id.get_ident();
	}
	return os;
}

StandardInstrumentTimeSlice::StandardInstrumentTimeSlice(timetype_t starttime, timetype_t endtime)
	: IdentTimeSlice(starttime, endtime), m_status(status_invalid)
{
}

timeset_t StandardInstrumentTimeSlice::timediscontinuities(void) const
{
	timeset_t r(IdentTimeSlice::timediscontinuities());
	timeset_t r2(get_timetable().timediscontinuities());
	constraintimediscontinuities(r2);
	r.insert(r2.begin(), r2.end());
	return r;
}

StandardInstrumentTimeSlice& StandardInstrumentTimeSlice::as_sidstar(void)
{
	if (this)
		return *this;
	return const_cast<StandardInstrumentTimeSlice&>(Object::invalid_standardinstrumenttimeslice);
}

const StandardInstrumentTimeSlice& StandardInstrumentTimeSlice::as_sidstar(void) const
{
	if (this)
		return *this;
	return Object::invalid_standardinstrumenttimeslice;
}

void StandardInstrumentTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<StandardInstrumentTimeSlice *>(this))->hibernate(gen);	
}

void StandardInstrumentTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<StandardInstrumentTimeSlice *>(this))->hibernate(gen);
}

const std::string& to_str(ADR::StandardInstrumentTimeSlice::status_t s)
{
	switch (s) {
	case ADR::StandardInstrumentTimeSlice::status_usable:
	{
		static const std::string r("usable");
		return r;
	}

	case ADR::StandardInstrumentTimeSlice::status_atcdiscr:
	{
		static const std::string r("atcdiscretion");
		return r;
	}

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

std::ostream& StandardInstrumentTimeSlice::print(std::ostream& os) const
{
	IdentTimeSlice::print(os);
	get_timetable().print(os << ' ' << get_status(), 4);
	os << std::endl << "    airport " << get_airport();
	{
		const IdentTimeSlice& id(get_airport().get_obj()->operator()(*this).as_ident());
		if (id.is_valid())
			os << ' ' << id.get_ident();
	}
	for (connpoints_t::const_iterator i(m_connpoints.begin()), e(m_connpoints.end()); i != e; ++i) {
		os << std::endl << "    connection point " << *i;
		{
			const IdentTimeSlice& id(i->get_obj()->operator()(*this).as_ident());
			if (id.is_valid())
				os << ' ' << id.get_ident();
		}
	}
	return os;
}

StandardInstrumentDeparture::StandardInstrumentDeparture(const UUID& uuid)
	: objbase_t(uuid)
{
}

StandardInstrumentDepartureTimeSlice::StandardInstrumentDepartureTimeSlice(timetype_t starttime, timetype_t endtime)
	: StandardInstrumentTimeSlice(starttime, endtime)
{
}

StandardInstrumentDepartureTimeSlice& StandardInstrumentDepartureTimeSlice::as_sid(void)
{
	if (this)
		return *this;
	return const_cast<StandardInstrumentDepartureTimeSlice&>(StandardInstrumentDeparture::invalid_timeslice);
}

const StandardInstrumentDepartureTimeSlice& StandardInstrumentDepartureTimeSlice::as_sid(void) const
{
	if (this)
		return *this;
	return StandardInstrumentDeparture::invalid_timeslice;
}

void StandardInstrumentDepartureTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<StandardInstrumentDepartureTimeSlice *>(this))->hibernate(gen);	
}

void StandardInstrumentDepartureTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<StandardInstrumentDepartureTimeSlice *>(this))->hibernate(gen);
}

std::ostream& StandardInstrumentDepartureTimeSlice::print(std::ostream& os) const
{
	StandardInstrumentTimeSlice::print(os);
	os << " instruction " << get_instruction();
	return os;
}

StandardInstrumentArrival::StandardInstrumentArrival(const UUID& uuid)
	: objbase_t(uuid)
{
}

StandardInstrumentArrivalTimeSlice::StandardInstrumentArrivalTimeSlice(timetype_t starttime, timetype_t endtime)
	: StandardInstrumentTimeSlice(starttime, endtime)
{
}

StandardInstrumentArrivalTimeSlice& StandardInstrumentArrivalTimeSlice::as_star(void)
{
	if (this)
		return *this;
	return const_cast<StandardInstrumentArrivalTimeSlice&>(StandardInstrumentArrival::invalid_timeslice);
}

const StandardInstrumentArrivalTimeSlice& StandardInstrumentArrivalTimeSlice::as_star(void) const
{
	if (this)
		return *this;
	return StandardInstrumentArrival::invalid_timeslice;
}

void StandardInstrumentArrivalTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<StandardInstrumentArrivalTimeSlice *>(this))->hibernate(gen);	
}

void StandardInstrumentArrivalTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<StandardInstrumentArrivalTimeSlice *>(this))->hibernate(gen);
}

std::ostream& StandardInstrumentArrivalTimeSlice::print(std::ostream& os) const
{
	StandardInstrumentTimeSlice::print(os);
	os << " instruction " << get_instruction() << std::endl << "    IAF " << get_iaf();
	{
		const IdentTimeSlice& id(get_iaf().get_obj()->operator()(*this).as_ident());
		if (id.is_valid())
			os << ' ' << id.get_ident();
	}
	return os;
}

DepartureLeg::DepartureLeg(const UUID& uuid)
	: objbase_t(uuid)
{
}

DepartureLegTimeSlice::DepartureLegTimeSlice(timetype_t starttime, timetype_t endtime)
	: SegmentTimeSlice(starttime, endtime)
{
}

DepartureLegTimeSlice& DepartureLegTimeSlice::as_departureleg(void)
{
	if (this)
		return *this;
	return const_cast<DepartureLegTimeSlice&>(DepartureLeg::invalid_timeslice);
}

const DepartureLegTimeSlice& DepartureLegTimeSlice::as_departureleg(void) const
{
	if (this)
		return *this;
	return DepartureLeg::invalid_timeslice;
}

void DepartureLegTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<DepartureLegTimeSlice *>(this))->hibernate(gen);	
}

void DepartureLegTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<DepartureLegTimeSlice *>(this))->hibernate(gen);
}

ArrivalLeg::ArrivalLeg(const UUID& uuid)
	: objbase_t(uuid)
{
}

ArrivalLegTimeSlice::ArrivalLegTimeSlice(timetype_t starttime, timetype_t endtime)
	: SegmentTimeSlice(starttime, endtime)
{
}

ArrivalLegTimeSlice& ArrivalLegTimeSlice::as_arrivalleg(void)
{
	if (this)
		return *this;
	return const_cast<ArrivalLegTimeSlice&>(ArrivalLeg::invalid_timeslice);
}

const ArrivalLegTimeSlice& ArrivalLegTimeSlice::as_arrivalleg(void) const
{
	if (this)
		return *this;
	return ArrivalLeg::invalid_timeslice;
}

void ArrivalLegTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<ArrivalLegTimeSlice *>(this))->hibernate(gen);	
}

void ArrivalLegTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<ArrivalLegTimeSlice *>(this))->hibernate(gen);
}

Route::Route(const UUID& uuid)
	: objbase_t(uuid)
{
}

RouteTimeSlice::RouteTimeSlice(timetype_t starttime, timetype_t endtime)
	: IdentTimeSlice(starttime, endtime)
{
}

RouteTimeSlice& RouteTimeSlice::as_route(void)
{
	if (this)
		return *this;
	return const_cast<RouteTimeSlice&>(Route::invalid_timeslice);
}

const RouteTimeSlice& RouteTimeSlice::as_route(void) const
{
	if (this)
		return *this;
	return Route::invalid_timeslice;
}

void RouteTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<RouteTimeSlice *>(this))->hibernate(gen);	
}

void RouteTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<RouteTimeSlice *>(this))->hibernate(gen);
}

std::ostream& RouteTimeSlice::print(std::ostream& os) const
{
	IdentTimeSlice::print(os);
	return os;
}

RouteSegment::RouteSegment(const UUID& uuid)
	: objbase_t(uuid)
{
}

RouteSegmentTimeSlice::Availability::Availability(void)
	: m_flags(status_invalid << 0)
{
}

std::ostream& RouteSegmentTimeSlice::Availability::print(std::ostream& os, const TimeSlice& ts) const
{
	os << " altitude " << m_altrange.to_str()
	   << ' ' << get_status() << (is_forward() ? " forward" : "") << (is_backward() ? " backward" : "");
	if (get_cdr())
		os << " cdr" << get_cdr();
	os << std::endl << "      levels " << get_levels();
	{
		const IdentTimeSlice& id(get_levels().get_obj()->operator()(ts).as_ident());
		if (id.is_valid())
			os << ' ' << id.get_ident();
	}
       	get_timetable().print(os, 6);
	return os;
}

RouteSegmentTimeSlice::Level::Level(void)
{
}

std::ostream& RouteSegmentTimeSlice::Level::print(std::ostream& os, const TimeSlice& ts) const
{
	os << " altitude " << m_altrange.to_str();
       	get_timetable().print(os, 6);
	return os;
}

RouteSegmentTimeSlice::RouteSegmentTimeSlice(timetype_t starttime, timetype_t endtime)
	: SegmentTimeSlice(starttime, endtime)
{
}

timeset_t RouteSegmentTimeSlice::timediscontinuities(void) const
{
	timeset_t r(SegmentTimeSlice::timediscontinuities());
	timeset_t r2;
	for (availability_t::const_iterator i(get_availability().begin()), e(get_availability().end()); i != e; ++i) {
		timeset_t r3(i->get_timetable().timediscontinuities());
		r2.insert(r3.begin(), r3.end());
	}
	for (levels_t::const_iterator i(get_levels().begin()), e(get_levels().end()); i != e; ++i) {
		timeset_t r3(i->get_timetable().timediscontinuities());
		r2.insert(r3.begin(), r3.end());
	}
	constraintimediscontinuities(r2);
	r.insert(r2.begin(), r2.end());
	return r;
}

RouteSegmentTimeSlice& RouteSegmentTimeSlice::as_routesegment(void)
{
	if (this)
		return *this;
	return const_cast<RouteSegmentTimeSlice&>(RouteSegment::invalid_timeslice);
}

const RouteSegmentTimeSlice& RouteSegmentTimeSlice::as_routesegment(void) const
{
	if (this)
		return *this;
	return RouteSegment::invalid_timeslice;
}

void RouteSegmentTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<RouteSegmentTimeSlice *>(this))->hibernate(gen);	
}

void RouteSegmentTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<RouteSegmentTimeSlice *>(this))->hibernate(gen);
}

bool RouteSegmentTimeSlice::is_forward(void) const
{
	for (availability_t::const_iterator ai(get_availability().begin()), ae(get_availability().end()); ai != ae; ++ai) {
		const Availability& a(*ai);
		if (!a.is_forward())
			continue;
		switch (a.get_status()) {
		case Availability::status_closed:
		default:
			continue;

		case Availability::status_open:
			break;

		case Availability::status_conditional:
			if (a.get_cdr() >= 3)
				continue;
			break;
		}
		return true;
	}
	return false;
}

bool RouteSegmentTimeSlice::is_backward(void) const
{
	for (availability_t::const_iterator ai(get_availability().begin()), ae(get_availability().end()); ai != ae; ++ai) {
		const Availability& a(*ai);
		if (!a.is_backward())
			continue;
		switch (a.get_status()) {
		case Availability::status_closed:
		default:
			continue;

		case Availability::status_open:
			break;

		case Availability::status_conditional:
			if (a.get_cdr() >= 3)
				continue;
			break;
		}
		return true;
	}
	return false;
}

const std::string& to_str(RouteSegmentTimeSlice::Availability::status_t s)
{
	switch (s) {
	case ADR::RouteSegmentTimeSlice::Availability::status_closed:
	{
		static const std::string r("closed");
		return r;
	}

	case ADR::RouteSegmentTimeSlice::Availability::status_open:
	{
		static const std::string r("open");
		return r;
	}

	case ADR::RouteSegmentTimeSlice::Availability::status_conditional:
	{
		static const std::string r("conditional");
		return r;
	}

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

std::ostream& RouteSegmentTimeSlice::print(std::ostream& os) const
{
	SegmentTimeSlice::print(os);
	for (unsigned int i(0), n(m_availability.size()); i < n; ++i)
	        m_availability[i].print(os << std::endl << "    Availability[" << i << ']', *this);
	for (unsigned int i(0), n(m_levels.size()); i < n; ++i)
	        m_levels[i].print(os << std::endl << "    Level[" << i << ']', *this);
	return os;
}

StandardLevelColumn::StandardLevelColumn(const UUID& uuid)
	: objbase_t(uuid)
{
}

StandardLevelColumnTimeSlice::StandardLevelColumnTimeSlice(timetype_t starttime, timetype_t endtime)
	: TimeSlice(starttime, endtime), m_series(series_invalid)
{
}

StandardLevelColumnTimeSlice& StandardLevelColumnTimeSlice::as_standardlevelcolumn(void)
{
	if (this)
		return *this;
	return const_cast<StandardLevelColumnTimeSlice&>(StandardLevelColumn::invalid_timeslice);
}

const StandardLevelColumnTimeSlice& StandardLevelColumnTimeSlice::as_standardlevelcolumn(void) const
{
	if (this)
		return *this;
	return StandardLevelColumn::invalid_timeslice;
}

void StandardLevelColumnTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<StandardLevelColumnTimeSlice *>(this))->hibernate(gen);	
}

void StandardLevelColumnTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<StandardLevelColumnTimeSlice *>(this))->hibernate(gen);
}

const std::string& to_str(StandardLevelColumnTimeSlice::series_t s)
{
	switch (s) {
	case ADR::StandardLevelColumnTimeSlice::series_even:
	{
		static const std::string r("even");
		return r;
	}

	case ADR::StandardLevelColumnTimeSlice::series_odd:
	{
		static const std::string r("odd");
		return r;
	}

	case ADR::StandardLevelColumnTimeSlice::series_unidirectional:
	{
		static const std::string r("unidirectional");
		return r;
	}

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

std::ostream& StandardLevelColumnTimeSlice::print(std::ostream& os) const
{
	TimeSlice::print(os);
	os << ' ' << get_series() << std::endl << "    leveltable " << get_leveltable();
	{
		const IdentTimeSlice& id(get_leveltable().get_obj()->operator()(*this).as_ident());
		if (id.is_valid())
			os << ' ' << id.get_ident();
	}
	return os;
}

StandardLevelTable::StandardLevelTable(const UUID& uuid)
	: objbase_t(uuid)
{
}

StandardLevelTableTimeSlice::StandardLevelTableTimeSlice(timetype_t starttime, timetype_t endtime)
	: IdentTimeSlice(starttime, endtime)
{
}

StandardLevelTableTimeSlice& StandardLevelTableTimeSlice::as_standardleveltable(void)
{
	if (this)
		return *this;
	return const_cast<StandardLevelTableTimeSlice&>(StandardLevelTable::invalid_timeslice);
}

const StandardLevelTableTimeSlice& StandardLevelTableTimeSlice::as_standardleveltable(void) const
{
	if (this)
		return *this;
	return StandardLevelTable::invalid_timeslice;
}

void StandardLevelTableTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<StandardLevelTableTimeSlice *>(this))->hibernate(gen);	
}

void StandardLevelTableTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<StandardLevelTableTimeSlice *>(this))->hibernate(gen);
}

std::ostream& StandardLevelTableTimeSlice::print(std::ostream& os) const
{
	IdentTimeSlice::print(os);
	os << " standard ICAO " << (is_standardicao() ? "yes" : "no");
	return os;
}

RouteSegmentLink::RouteSegmentLink(const UUID& uuid, const Glib::RefPtr<Object>& obj, bool backward)
	: Link(uuid, obj), m_backward(backward)
{
}

RouteSegmentLink::RouteSegmentLink(const Glib::RefPtr<Object>& obj, bool backward)
	: Link(obj), m_backward(backward)
{
}

int RouteSegmentLink::compare(const RouteSegmentLink& x) const
{
	int c(Link::compare(x));
	if (c)
		return c;
	if (is_forward() && x.is_backward())
		return -1;
	if (is_backward() && x.is_forward())
		return 1;
	return 0;
}
