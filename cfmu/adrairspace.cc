#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iomanip>
#include <sstream>

#include "adr.hh"
#include "adrdb.hh"
#include "adrhibernate.hh"
#include "adrairspace.hh"

using namespace ADR;

Airspace::Airspace(const UUID& uuid)
	: objbase_t(uuid)
{
}

bool Airspace::is_inside(const TimeTableEval& tte, int32_t alt, const AltRange& altrange, const UUID& uuid) const
{
	const AirspaceTimeSlice& ts(operator()(tte.get_time()).as_airspace());
	if (!ts.is_valid())
		return false;
	return ts.is_inside(tte, alt, altrange, uuid);
}

bool Airspace::is_intersect(const TimeTableEval& tte, const Point& pt1, int32_t alt, const AltRange& altrange) const
{
	const AirspaceTimeSlice& ts(operator()(tte.get_time()).as_airspace());
	if (!ts.is_valid())
		return false;
	return ts.is_intersect(tte, pt1, alt, altrange);
}

bool Airspace::is_intersect(const TimeTableEval& tte, const Point& pt1, int32_t alt0, int32_t alt1, const AltRange& altrange) const
{
	const AirspaceTimeSlice& ts(operator()(tte.get_time()).as_airspace());
	if (!ts.is_valid())
		return false;
	return ts.is_intersect(tte, pt1, alt0, alt1, altrange);
}

IntervalSet<int32_t> Airspace::get_point_altitudes(const TimeTableEval& tte, const AltRange& altrange, const UUID& uuid) const
{
	const AirspaceTimeSlice& ts(operator()(tte.get_time()).as_airspace());
	if (!ts.is_valid())
		return IntervalSet<int32_t>();
	return ts.get_point_altitudes(tte, altrange, uuid);
}

IntervalSet<int32_t> Airspace::get_intersect_altitudes(const TimeTableEval& tte, const Point& pt1, const AltRange& altrange) const
{
	const AirspaceTimeSlice& ts(operator()(tte.get_time()).as_airspace());
	if (!ts.is_valid())
		return IntervalSet<int32_t>();
	return ts.get_intersect_altitudes(tte, pt1, altrange);
}

IntervalSet<int32_t> Airspace::get_point_intersect_altitudes(const TimeTableEval& tte, const Point& pt1, const AltRange& altrange, const UUID& uuid0, const UUID& uuid1) const
{
	const AirspaceTimeSlice& ts(operator()(tte.get_time()).as_airspace());
	if (!ts.is_valid())
		return IntervalSet<int32_t>();
	return ts.get_point_intersect_altitudes(tte, pt1, altrange, uuid0, uuid1);
}

std::vector<AirspaceTimeSlice::Trace> Airspace::trace_inside(const TimeTableEval& tte, int32_t alt, const AltRange& altrange, const UUID& uuid) const
{
	const AirspaceTimeSlice& ts(operator()(tte.get_time()).as_airspace());
	if (!ts.is_valid()) {
		std::vector<AirspaceTimeSlice::Trace> r;
		r.push_back(AirspaceTimeSlice::Trace(Object::ptr_t::cast_const(get_ptr()), AirspaceTimeSlice::Trace::nocomponent, AirspaceTimeSlice::Trace::reason_outsidetimeslice));
		return r;
	}
	return ts.trace_inside(tte, alt, altrange, uuid, Object::ptr_t::cast_const(get_ptr()));
}

std::vector<AirspaceTimeSlice::Trace> Airspace::trace_intersect(const TimeTableEval& tte, const Point& pt1, int32_t alt, const AltRange& altrange) const
{
	const AirspaceTimeSlice& ts(operator()(tte.get_time()).as_airspace());
	if (!ts.is_valid()) {
		std::vector<AirspaceTimeSlice::Trace> r;
		r.push_back(AirspaceTimeSlice::Trace(Object::ptr_t::cast_const(get_ptr()), AirspaceTimeSlice::Trace::nocomponent, AirspaceTimeSlice::Trace::reason_outsidetimeslice));
		return r;
	}
	return ts.trace_intersect(tte, pt1, alt, altrange, Object::ptr_t::cast_const(get_ptr()));
}

bool Airspace::is_altitude_overlap(int32_t alt0, int32_t alt1, const timepair_t& tm, const AltRange& altrange) const
{
	for (unsigned int i(0), n(size()); i < n; ++i) {
		const AirspaceTimeSlice& ts(operator[](i).as_airspace());
		if (!ts.is_valid() || !ts.is_overlap(tm))
			continue;
		if (ts.is_altitude_overlap(alt0, alt1, tm, altrange))
			return true;
	}
	return false;
}

const unsigned int AirspaceTimeSlice::Trace::nocomponent;

const std::string& to_str(ADR::AirspaceTimeSlice::Trace::reason_t r)
{
	switch (r) {
	case ADR::AirspaceTimeSlice::Trace::reason_inside:
	{
		static const std::string r("inside");
		return r;
	}

	case ADR::AirspaceTimeSlice::Trace::reason_intersect:
	{
		static const std::string r("intersect");
		return r;
	}

	case ADR::AirspaceTimeSlice::Trace::reason_outside:
	{
		static const std::string r("outside");
		return r;
	}

	case ADR::AirspaceTimeSlice::Trace::reason_outsidebbox:
	{
		static const std::string r("outsidebbox");
		return r;
	}

	case ADR::AirspaceTimeSlice::Trace::reason_nointersect:
	{
		static const std::string r("nointersect");
		return r;
	}

	case ADR::AirspaceTimeSlice::Trace::reason_border:
	{
		static const std::string r("border");
		return r;
	}

	case ADR::AirspaceTimeSlice::Trace::reason_outsidetime:
	{
		static const std::string r("outsidetime");
		return r;
	}

	case ADR::AirspaceTimeSlice::Trace::reason_outsidetimeslice:
	{
		static const std::string r("outsidetimeslice");
		return r;
	}

	case ADR::AirspaceTimeSlice::Trace::reason_altrange:
	{
		static const std::string r("altrange");
		return r;
	}

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

const uint16_t AirspaceTimeSlice::Component::PointLink::exterior = std::numeric_limits<uint16_t>::max();

AirspaceTimeSlice::Component::PointLink::PointLink(const Link& l, uint16_t poly, uint16_t ring, uint32_t index)
	: m_link(l), m_poly(poly), m_ring(ring), m_index(index)
{
}

AirspaceTimeSlice::Component::Component(void)
	: m_gndelevmin(ElevPointIdentTimeSlice::invalid_elev),
	  m_gndelevmax(ElevPointIdentTimeSlice::invalid_elev), m_flags(operator_invalid)
{
}

const std::string& to_str(ADR::AirspaceTimeSlice::Component::operator_t o)
{
	switch (o) {
	case ADR::AirspaceTimeSlice::Component::operator_base:
	{
		static const std::string r("base");
		return r;
	}

	case ADR::AirspaceTimeSlice::Component::operator_union:
	{
		static const std::string r("union");
		return r;
	}

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

bool AirspaceTimeSlice::Component::is_pointlink(const UUID& uuid) const
{
	if (uuid.is_nil())
		return false;
	for (pointlink_t::const_iterator pi(get_pointlink().begin()), pe(get_pointlink().end()); pi != pe; ++pi) {
		if (uuid == pi->get_link())
			return true;
	}
	return false;
}

MultiPolygonHole AirspaceTimeSlice::Component::get_full_poly(const TimeSlice& ts) const
{
	MultiPolygonHole poly(get_poly());
	if (!poly.empty())
		poly.geos_make_valid();
	if (get_airspace().is_nil())
		return poly;
	const Object::const_ptr_t& aspc(get_airspace().get_obj());
	if (!aspc) {
		std::ostringstream oss;
		oss << "AirspaceTimeSlice::Component::get_full_poly: unlinked airspace " << get_airspace();
		throw std::runtime_error(oss.str());
	}
	unsigned int tsidx(~0U), n(aspc->size());
	uint64_t bestolap(0);
	for (unsigned int i(0); i < n; ++i) {
		const TimeSlice& ts1(aspc->operator[](i));
		uint64_t olap(ts1.get_overlap(ts));
		if (olap <= bestolap)
			continue;
		const AirspaceTimeSlice& as(ts1.as_airspace());
		if (!as.is_valid())
			continue;
		tsidx = i;
	}
	if (tsidx >= n) {
		std::ostringstream oss;
		oss << "AirspaceTimeSlice::Component::get_full_poly: airspace " << get_airspace() << " has no suitable timeslice";
		throw std::runtime_error(oss.str());
	}
	const AirspaceTimeSlice& as(aspc->operator[](tsidx).as_airspace());
	MultiPolygonHole mp(as.get_full_poly());
	if (poly.empty())
		poly.swap(mp);
	else
		poly.geos_union(mp);
	return poly;
}

MultiPolygonHole AirspaceTimeSlice::Component::get_full_poly(uint64_t tm) const
{
	MultiPolygonHole poly(get_poly());
	if (!poly.empty())
		poly.geos_make_valid();
	if (get_airspace().is_nil())
		return poly;
	const Object::const_ptr_t& aspc(get_airspace().get_obj());
	if (!aspc) {
		std::ostringstream oss;
		oss << "AirspaceTimeSlice::Component::get_full_poly: unlinked airspace " << get_airspace();
		throw std::runtime_error(oss.str());
	}
	const AirspaceTimeSlice& as(aspc->operator()(tm).as_airspace());
	if (!as.is_valid()) {
		std::ostringstream oss;
		oss << "AirspaceTimeSlice::Component::get_full_poly: airspace " << get_airspace() << " has no suitable timeslice";
		throw std::runtime_error(oss.str());
	}
	MultiPolygonHole mp(as.get_full_poly());
	if (poly.empty())
		poly.swap(mp);
	else
		poly.geos_union(mp);
	return poly;
}

bool AirspaceTimeSlice::Component::is_inside(const Point& pt, const timepair_t& tint, int32_t alt, const AltRange& altrange, const UUID& uuid) const
{
	AltRange ar(altrange);
	if (is_poly() || !is_full_geometry())
		ar.merge(m_altrange);
	if (is_poly()) {
		if (!ar.is_inside(alt))
			return false;
		if (is_pointlink(uuid))
			return false;
		if (!get_poly().windingnumber(pt))
			return false;
		return true;
	}
	for (unsigned int i(0), n(get_airspace().get_obj()->size()); i < n; ++i) {
		const AirspaceTimeSlice& ts(get_airspace().get_obj()->operator[](i).as_airspace());
		if (!ts.is_valid() || !ts.is_overlap(tint))
			continue;
		if (ts.is_inside(pt, alt, ar, uuid))
			return true;
	}
	return false;
}

bool AirspaceTimeSlice::Component::is_inside(const TimeTableEval& tte, int32_t alt, const AltRange& altrange, const UUID& uuid) const
{
	AltRange ar(altrange);
	if (is_poly() || !is_full_geometry())
		ar.merge(m_altrange);
	if (is_poly()) {
		if (!ar.is_inside(alt))
			return false;
		if (is_pointlink(uuid))
			return false;
		if (!get_poly().windingnumber(tte.get_point()))
			return false;
		return true;
	}
	const AirspaceTimeSlice& ts(get_airspace().get_obj()->operator()(tte.get_time()).as_airspace());
	if (!ts.is_valid())
		return false;
	return ts.is_inside(tte, alt, ar, uuid);
}

bool AirspaceTimeSlice::Component::is_intersect(const TimeTableEval& tte, const Point& pt1, int32_t alt, const AltRange& altrange) const
{
	AltRange ar(altrange);
	if (is_poly() || !is_full_geometry())
		ar.merge(m_altrange);
	if (is_poly()) {
		if (!ar.is_inside(alt))
			return false;
		if (!get_poly().is_strict_intersection(tte.get_point(), pt1))
			return false;
		return true;
	}
	const AirspaceTimeSlice& ts(get_airspace().get_obj()->operator()(tte.get_time()).as_airspace());
	if (!ts.is_valid())
		return false;
	return ts.is_intersect(tte, pt1, alt, ar);
}

bool AirspaceTimeSlice::Component::is_intersect(const TimeTableEval& tte, const Point& pt1, int32_t alt0, int32_t alt1, const AltRange& altrange) const
{
	AltRange ar(altrange);
	if (is_poly() || !is_full_geometry())
		ar.merge(m_altrange);
	if (is_poly()) {
		if (!ar.is_overlap(alt0, alt1))
			return false;
		if (!get_poly().is_strict_intersection(tte.get_point(), pt1))
			return false;
		return true;
	}
	const AirspaceTimeSlice& ts(get_airspace().get_obj()->operator()(tte.get_time()).as_airspace());
	if (!ts.is_valid())
		return false;
	return ts.is_intersect(tte, pt1, alt0, alt1, ar);
}

IntervalSet<int32_t> AirspaceTimeSlice::Component::get_point_altitudes(const TimeTableEval& tte, const AltRange& altrange, const UUID& uuid) const
{
	AltRange ar(altrange);
	if (is_poly() || !is_full_geometry())
		ar.merge(m_altrange);
	if (is_poly()) {
		if (is_pointlink(uuid))
			return IntervalSet<int32_t>();
		if (!get_poly().windingnumber(tte.get_point()))
			return IntervalSet<int32_t>();
		return ar.get_interval();
	}
	const AirspaceTimeSlice& ts(get_airspace().get_obj()->operator()(tte.get_time()).as_airspace());
	if (!ts.is_valid())
		return ar.get_interval();
	return ts.get_point_altitudes(tte, ar, uuid);
}

IntervalSet<int32_t> AirspaceTimeSlice::Component::get_intersect_altitudes(const TimeTableEval& tte, const Point& pt1, const AltRange& altrange) const
{
	AltRange ar(altrange);
	if (is_poly() || !is_full_geometry())
		ar.merge(m_altrange);
	if (is_poly()) {
		if (!get_poly().is_strict_intersection(tte.get_point(), pt1))
			return IntervalSet<int32_t>();
		return ar.get_interval();
	}
	const AirspaceTimeSlice& ts(get_airspace().get_obj()->operator()(tte.get_time()).as_airspace());
	if (!ts.is_valid())
		return IntervalSet<int32_t>();
	return ts.get_intersect_altitudes(tte, pt1, ar);
}

IntervalSet<int32_t> AirspaceTimeSlice::Component::get_point_intersect_altitudes(const TimeTableEval& tte, const Point& pt1, const AltRange& altrange, const UUID& uuid0, const UUID& uuid1) const
{
	AltRange ar(altrange);
	if (is_poly() || !is_full_geometry())
		ar.merge(m_altrange);
	if (is_poly()) {
		bool onborder0(is_pointlink(uuid0));
		bool onborder1(is_pointlink(uuid1));
		if (!(get_poly().windingnumber(tte.get_point()) && !onborder0) &&
		    !(get_poly().windingnumber(pt1) && !onborder1) &&
		    !(get_poly().is_strict_intersection(tte.get_point(), pt1) ||
		      (onborder0 && onborder1 && get_poly().windingnumber(tte.get_point().halfway(pt1)))))
			return IntervalSet<int32_t>();
		if (false) {
			get_poly().print(std::cerr << "AirspaceTimeSlice::Component::get_point_intersect_altitudes: match:"
					 << std::endl << "  ") << std::endl;
			if (get_poly().windingnumber(tte.get_point()) && !is_pointlink(uuid0))
				std::cerr << "  First Point " << tte.get_point().get_lat_str2() << ' ' << tte.get_point().get_lon_str2()
					  << ' ' << uuid0 << " is inside" << std::endl;
			if (get_poly().windingnumber(pt1) && !is_pointlink(uuid1))
				std::cerr << "  Second Point " << pt1.get_lat_str2() << ' ' << pt1.get_lon_str2()
					  << ' ' << uuid1 << " is inside" << std::endl;
			if (get_poly().is_strict_intersection(tte.get_point(), pt1))
				std::cerr << "  Line " << tte.get_point().get_lat_str2() << ' ' << tte.get_point().get_lon_str2()
					  << " to " << pt1.get_lat_str2() << ' ' << pt1.get_lon_str2() << " intersects" << std::endl;
		}
		return ar.get_interval();
	}
	const AirspaceTimeSlice& ts(get_airspace().get_obj()->operator()(tte.get_time()).as_airspace());
	if (!ts.is_valid())
		return IntervalSet<int32_t>();
	return ts.get_point_intersect_altitudes(tte, pt1, ar, uuid0, uuid1);
}

std::vector<AirspaceTimeSlice::Trace> AirspaceTimeSlice::Component::trace_inside(const TimeTableEval& tte, int32_t alt, const AltRange& altrange, const UUID& uuid, const Object::ptr_t& aspc, unsigned int comp) const
{
	AltRange ar(altrange);
	if (is_poly() || !is_full_geometry())
		ar.merge(m_altrange);
	if (is_poly()) {
		if (!ar.is_inside(alt)) {
			std::vector<AirspaceTimeSlice::Trace> r;
			r.push_back(Trace(aspc, comp, Trace::reason_altrange));
			return r;
		}
		if (is_pointlink(uuid)) {
			std::vector<AirspaceTimeSlice::Trace> r;
			r.push_back(Trace(aspc, comp, Trace::reason_border));
			return r;
		}
		if (!get_poly().windingnumber(tte.get_point())) {
			std::vector<AirspaceTimeSlice::Trace> r;
			r.push_back(Trace(aspc, comp, Trace::reason_outside));
			return r;
		}
		std::vector<AirspaceTimeSlice::Trace> r;
		r.push_back(Trace(aspc, comp, Trace::reason_inside));
		return r;
	}
	const AirspaceTimeSlice& ts(get_airspace().get_obj()->operator()(tte.get_time()).as_airspace());
	if (!ts.is_valid()) {
		std::vector<AirspaceTimeSlice::Trace> r;
		r.push_back(Trace(aspc, comp, Trace::reason_outsidetimeslice));
		return r;
	}
	return ts.trace_inside(tte, alt, ar, uuid, get_airspace().get_obj());
}

std::vector<AirspaceTimeSlice::Trace> AirspaceTimeSlice::Component::trace_intersect(const TimeTableEval& tte, const Point& pt1, int32_t alt, const AltRange& altrange, const Object::ptr_t& aspc, unsigned int comp) const
{
	AltRange ar(altrange);
	if (is_poly() || !is_full_geometry())
		ar.merge(m_altrange);
	if (is_poly()) {
		if (!ar.is_inside(alt)) {
			std::vector<AirspaceTimeSlice::Trace> r;
			r.push_back(Trace(aspc, comp, Trace::reason_altrange));
			return r;
		}
		if (!get_poly().is_strict_intersection(tte.get_point(), pt1)) {
			std::vector<AirspaceTimeSlice::Trace> r;
			r.push_back(Trace(aspc, comp, Trace::reason_nointersect));
			return r;
		}
		std::vector<AirspaceTimeSlice::Trace> r;
		r.push_back(Trace(aspc, comp, Trace::reason_intersect));
		return r;
	}
	const AirspaceTimeSlice& ts(get_airspace().get_obj()->operator()(tte.get_time()).as_airspace());
	if (!ts.is_valid()) {
		std::vector<AirspaceTimeSlice::Trace> r;
		r.push_back(Trace(aspc, comp, Trace::reason_outsidetimeslice));
		return r;
	}
	return ts.trace_intersect(tte, pt1, alt, ar, get_airspace().get_obj());
}

bool AirspaceTimeSlice::Component::is_altitude_overlap(int32_t alt0, int32_t alt1, const timepair_t& tm, const AltRange& altrange) const
{
	AltRange ar(altrange);
	if (is_poly() || !is_full_geometry())
		ar.merge(m_altrange);
	if (is_poly())
		return ar.is_overlap(alt0, alt1);
	Airspace::ptr_t p(Airspace::ptr_t::cast_dynamic(get_airspace().get_obj()));
	if (!p)
		return true;
	return p->is_altitude_overlap(alt0, alt1, tm, ar);
}

bool AirspaceTimeSlice::Component::recompute(const TimeSlice& ts)
{
	bool work(false);
	// extract point coordinates from linked points
	for (pointlink_t::const_iterator i(m_pointlink.begin()), e(m_pointlink.end()); i != e; ++i) {
		if (!i->get_link().get_obj()) {
			std::ostringstream oss;
			oss << "AirspaceTimeSlice::Component::recompute: unlinked object " << i->get_link();
			throw std::runtime_error(oss.str());
		}
		const PointIdentTimeSlice& pt(i->get_link().get_obj()->operator()(ts).as_point());
		if (!pt.is_valid()) {
			std::ostringstream oss;
			oss << "AirspaceTimeSlice::Component::recompute: object " << i->get_link() << " not a point";
			throw std::runtime_error(oss.str());
		}
		if (i->get_poly() >= m_poly.size())
			throw std::runtime_error("PointLink poly out of range");
		PolygonHole& ph(m_poly[i->get_poly()]);
		PolygonSimple *ps(0);
		if (i->is_exterior()) {
			ps = &ph.get_exterior();
		} else {
			if (i->get_ring() >= ph.get_nrinterior())
				throw std::runtime_error("PointLink ring out of range");
			ps = &ph[i->get_ring()];
		}
		if (i->get_index() >= ps->size())
			throw std::runtime_error("PointLink index out of range");
		Point& ppt((*ps)[i->get_index()]);
		work = work || ppt != pt.get_coord();
		ppt = pt.get_coord();
	}
	// ensure correct orientation
	for (unsigned int pi(0), pn(m_poly.size()); pi < pn; ++pi) {
		PolygonHole& ph(m_poly[pi]);
		if (!ph.get_exterior().is_counterclockwise()) {
			ph.get_exterior().reverse();
			work = true;
			unsigned int sz(ph.get_exterior().size());
			for (pointlink_t::iterator i(m_pointlink.begin()), e(m_pointlink.end()); i != e; ++i) {
				if (i->get_poly() != pi || !i->is_exterior())
					continue;
				i->set_index(sz - 1 - i->get_index());
			}
		}
		for (unsigned int r(0), n(ph.get_nrinterior()); r < n; ++r) {
			PolygonSimple& ps(ph[r]);
			if (!ps.is_counterclockwise())
				continue;
			ps.reverse();
			work = true;
			unsigned int sz(ps.size());
			for (pointlink_t::iterator i(m_pointlink.begin()), e(m_pointlink.end()); i != e; ++i) {
				if (i->get_poly() != pi || i->get_ring() != r)
					continue;
				i->set_index(sz - 1 - i->get_index());
			}
		}
	}
	return work;
}

bool AirspaceTimeSlice::Component::recompute(const TimeSlice& ts, TopoDb30& topodb)
{
	bool work(false);
	if (!is_gndelevmin_valid() || !is_gndelevmax_valid()) {
		elev_t min(std::numeric_limits<elev_t>::max());
		elev_t max(std::numeric_limits<elev_t>::min());
		if (is_poly()) {
			TopoDb30::minmax_elev_t mme(topodb.get_minmax_elev(get_poly()));
			if (mme.first == TopoDb30::ocean)
				mme.first = 0;
			if (mme.second == TopoDb30::ocean)
				mme.second = 0;
			if (mme.first != TopoDb30::nodata) {
				elev_t e(Point::round<elev_t,float>(mme.first * Point::m_to_ft));
				min = std::min(min, e);
				max = std::max(max, e);
			}
			if (mme.second != TopoDb30::nodata) {
				elev_t e(Point::round<elev_t,float>(mme.second * Point::m_to_ft));
				max = std::max(max, e);
			}
		}
		const Object::const_ptr_t& aspc(get_airspace().get_obj());
		for (unsigned int i(0), n(aspc->size()); i < n; ++i) {
			const TimeSlice& ts1(aspc->operator[](i));
			if (!ts1.is_overlap(ts))
				continue;
			const AirspaceTimeSlice& as(ts1.as_airspace());
			if (!as.is_valid())
				continue;
			elev_t e(as.get_gndelevmin());
			if (e != ElevPointIdentTimeSlice::invalid_elev) {
				min = std::min(min, e);
				max = std::max(max, e);
			}
			e = as.get_gndelevmax();
			if (e != ElevPointIdentTimeSlice::invalid_elev) {
				min = std::min(min, e);
				max = std::max(max, e);
			}
		}
		if (min <= max) {
			work = true;
			set_gndelevmin(min);
			set_gndelevmax(max);
		}
	}
	return work;
}

std::ostream& AirspaceTimeSlice::Component::print(std::ostream& os, const TimeSlice& ts) const
{
	os << ' ' << get_operator()
	   << " altitude " << m_altrange.to_str()
	   << ' ' << (is_full_geometry() ? "full" : "horiz");
	if (is_gndelevmin_valid() && is_gndelevmax_valid())
		os << " gndelev " << get_gndelevmin() << "..." << get_gndelevmax();
	if (!get_airspace().is_nil()) {
		os << std::endl << "      airspace " << get_airspace();
		{
			const IdentTimeSlice& id(get_airspace().get_obj()->operator()(ts).as_ident());
			if (id.is_valid())
				os << ' ' << id.get_ident();
		}
	}
	if (!m_poly.empty())
		m_poly.print(os << std::endl << "      ");
	const pointlink_t& pl(get_pointlink());
	for (pointlink_t::const_iterator i(pl.begin()), e(pl.end()); i != e; ++i) {
		os << std::endl << "      point " << i->get_link();
		{
			const IdentTimeSlice& id(i->get_link().get_obj()->operator()(ts).as_ident());
			if (id.is_valid())
				os << ' ' << id.get_ident();
		}
		os << " ring " << i->get_ring() << " index " << i->get_index();
	}
	return os;
}

AirspaceTimeSlice::AirspaceTimeSlice(timetype_t starttime, timetype_t endtime)
	: IdentTimeSlice(starttime, endtime), m_type(type_invalid), m_flags(localtype_none)
{
}

AirspaceTimeSlice& AirspaceTimeSlice::as_airspace(void)
{
	if (this)
		return *this;
	return const_cast<AirspaceTimeSlice&>(Airspace::invalid_timeslice);
}

const AirspaceTimeSlice& AirspaceTimeSlice::as_airspace(void) const
{
	if (this)
		return *this;
	return Airspace::invalid_timeslice;
}

void AirspaceTimeSlice::dependencies(UUID::set_t& dep) const
{
	DependencyGenerator<UUID::set_t> gen(dep);
	(const_cast<AirspaceTimeSlice *>(this))->hibernate(gen);	
}

void AirspaceTimeSlice::dependencies(Link::LinkSet& dep) const
{
	DependencyGenerator<Link::LinkSet> gen(dep);
	(const_cast<AirspaceTimeSlice *>(this))->hibernate(gen);
}

const std::string& to_str(ADR::AirspaceTimeSlice::type_t t)
{
	switch (t) {
	case ADR::AirspaceTimeSlice::type_atz:
	{
		static const std::string r("atz");
		return r;
	}

	case ADR::AirspaceTimeSlice::type_cba:
	{
		static const std::string r("cba");
		return r;
	}

	case ADR::AirspaceTimeSlice::type_cta:
	{
		static const std::string r("cta");
		return r;
	}

	case ADR::AirspaceTimeSlice::type_cta_p:
	{
		static const std::string r("cta_p");
		return r;
	}

	case ADR::AirspaceTimeSlice::type_ctr:
	{
		static const std::string r("ctr");
		return r;
	}

	case ADR::AirspaceTimeSlice::type_d:
	{
		static const std::string r("d");
		return r;
	}

	case ADR::AirspaceTimeSlice::type_d_other:
	{
		static const std::string r("d_other");
		return r;
	}

	case ADR::AirspaceTimeSlice::type_fir:
	{
		static const std::string r("fir");
		return r;
	}

	case ADR::AirspaceTimeSlice::type_fir_p:
	{
		static const std::string r("fir_p");
		return r;
	}

	case ADR::AirspaceTimeSlice::type_nas:
	{
		static const std::string r("nas");
		return r;
	}

	case ADR::AirspaceTimeSlice::type_oca:
	{
		static const std::string r("oca");
		return r;
	}

	case ADR::AirspaceTimeSlice::type_adr_auag:
	{
		static const std::string r("adr_auag");
		return r;
	}

	case ADR::AirspaceTimeSlice::type_adr_fab:
	{
		static const std::string r("adr_fab");
		return r;
	}

	case ADR::AirspaceTimeSlice::type_p:
	{
		static const std::string r("p");
		return r;
	}

	case ADR::AirspaceTimeSlice::type_part:
	{
		static const std::string r("part");
		return r;
	}

	case ADR::AirspaceTimeSlice::type_r:
	{
		static const std::string r("r");
		return r;
	}

	case ADR::AirspaceTimeSlice::type_sector:
	{
		static const std::string r("sector");
		return r;
	}

	case ADR::AirspaceTimeSlice::type_sector_c:
	{
		static const std::string r("sector_c");
		return r;
	}

	case ADR::AirspaceTimeSlice::type_tma:
	{
		static const std::string r("tma");
		return r;
	}

	case ADR::AirspaceTimeSlice::type_tra:
	{
		static const std::string r("tra");
		return r;
	}

	case ADR::AirspaceTimeSlice::type_tsa:
	{
		static const std::string r("tsa");
		return r;
	}

	case ADR::AirspaceTimeSlice::type_uir:
	{
		static const std::string r("uir");
		return r;
	}

	case ADR::AirspaceTimeSlice::type_uta:
	{
		static const std::string r("uta");
		return r;
	}

	case ADR::AirspaceTimeSlice::type_border:
	{
		static const std::string r("border");
		return r;
	}

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

const std::string& to_str(ADR::AirspaceTimeSlice::localtype_t t)
{
	switch (t) {
	case ADR::AirspaceTimeSlice::localtype_none:
	{
		static const std::string r("none");
		return r;
	}

	case ADR::AirspaceTimeSlice::localtype_mra:
	{
		static const std::string r("mra");
		return r;
	}

	case ADR::AirspaceTimeSlice::localtype_mta:
	{
		static const std::string r("mta");
		return r;
	}

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

MultiPolygonHole AirspaceTimeSlice::get_full_poly(void) const
{
	MultiPolygonHole poly;
	for (components_t::const_iterator ci(m_components.begin()), ce(m_components.end()); ci != ce; ++ci) {
		MultiPolygonHole mp(ci->get_full_poly(*this));
		switch (ci->get_operator()) {
		case Component::operator_base:
			poly.swap(mp);
			break;

		case Component::operator_union:
			poly.geos_union(mp);
			break;

		default:
			throw std::runtime_error("AirspaceTimeSlice::get_full_poly: invalid component operator " + to_str(ci->get_operator()));
			break;
		}
	}
	return poly;
}

MultiPolygonHole AirspaceTimeSlice::get_full_poly(uint64_t tm) const
{
	if (!is_inside(tm))
		throw std::runtime_error("AirspaceTimeSlice::get_full_poly: time not within timeslice");
	MultiPolygonHole poly;
	for (components_t::const_iterator ci(m_components.begin()), ce(m_components.end()); ci != ce; ++ci) {
		MultiPolygonHole mp(ci->get_full_poly(tm));
		switch (ci->get_operator()) {
		case Component::operator_base:
			poly.swap(mp);
			break;

		case Component::operator_union:
			poly.geos_union(mp);
			break;

		default:
			throw std::runtime_error("AirspaceTimeSlice::get_full_poly: invalid component operator " + to_str(ci->get_operator()));
			break;
		}
	}
	return poly;
}

char AirspaceTimeSlice::get_airspace_bdryclass(type_t t)
{
	switch (t) {
	case ADR::AirspaceTimeSlice::type_atz:
		return AirspacesDb::Airspace::bdryclass_ead_ead;

	case ADR::AirspaceTimeSlice::type_cba:
		return AirspacesDb::Airspace::bdryclass_ead_cba;

	case ADR::AirspaceTimeSlice::type_cta:
		return AirspacesDb::Airspace::bdryclass_ead_cta;

	case ADR::AirspaceTimeSlice::type_cta_p:
		return AirspacesDb::Airspace::bdryclass_ead_cta_p;

	case ADR::AirspaceTimeSlice::type_ctr:
		return AirspacesDb::Airspace::bdryclass_ead_ctr;

	case ADR::AirspaceTimeSlice::type_d:
		return AirspacesDb::Airspace::bdryclass_ead_d;

	case ADR::AirspaceTimeSlice::type_d_other:
		return AirspacesDb::Airspace::bdryclass_ead_d_other;

	case ADR::AirspaceTimeSlice::type_fir:
		return AirspacesDb::Airspace::bdryclass_ead_fir;

	case ADR::AirspaceTimeSlice::type_fir_p:
		return AirspacesDb::Airspace::bdryclass_ead_fir_p;

	case ADR::AirspaceTimeSlice::type_nas:
		return AirspacesDb::Airspace::bdryclass_ead_nas;

	case ADR::AirspaceTimeSlice::type_oca:
		return AirspacesDb::Airspace::bdryclass_ead_oca;

	case ADR::AirspaceTimeSlice::type_adr_auag:
		return AirspacesDb::Airspace::bdryclass_ead_ead;

	case ADR::AirspaceTimeSlice::type_adr_fab:
		return AirspacesDb::Airspace::bdryclass_ead_ead;

	case ADR::AirspaceTimeSlice::type_p:
		return AirspacesDb::Airspace::bdryclass_ead_p;

	case ADR::AirspaceTimeSlice::type_part:
		return AirspacesDb::Airspace::bdryclass_ead_part;

	case ADR::AirspaceTimeSlice::type_r:
		return AirspacesDb::Airspace::bdryclass_ead_r;

	case ADR::AirspaceTimeSlice::type_sector:
		return AirspacesDb::Airspace::bdryclass_ead_sector;

	case ADR::AirspaceTimeSlice::type_sector_c:
		return AirspacesDb::Airspace::bdryclass_ead_sector_c;

	case ADR::AirspaceTimeSlice::type_tma:
		return AirspacesDb::Airspace::bdryclass_ead_tma;

	case ADR::AirspaceTimeSlice::type_tra:
		return AirspacesDb::Airspace::bdryclass_ead_tra;

	case ADR::AirspaceTimeSlice::type_tsa:
		return AirspacesDb::Airspace::bdryclass_ead_tsa;

	case ADR::AirspaceTimeSlice::type_uir:
		return AirspacesDb::Airspace::bdryclass_ead_uir;

	case ADR::AirspaceTimeSlice::type_uta:
		return AirspacesDb::Airspace::bdryclass_ead_uta;

	case ADR::AirspaceTimeSlice::type_border:
		return AirspacesDb::Airspace::bdryclass_ead_political;

	default:
		return 0;
	}
}

bool AirspaceTimeSlice::get(uint64_t tm, AirspacesDb::Airspace& el) const
{
	if (!is_inside(tm))
		return false;
      	el.set_icao(get_ident());
      	el.set_ident(get_ident());
	{
		std::ostringstream oss;
		get_timetable().print(oss, 0);
		el.set_efftime(oss.str());
	}
	{
		std::string note;
		if (is_icao())
			note += " ICAO";
		if (is_flexibleuse())
			note += " Flexible Use";
		if (is_level(1))
			note += " Level1";
		if (is_level(2))
			note += " Level1";
		if (is_level(3))
			note += " Level3";
		if (!note.empty())
			el.set_note(note.substr(1));
	}
	// Polygon and Components
	{
		MultiPolygonHole poly;
		uint8_t altlwrflag(AirspacesDb::Airspace::altflag_unkn);
		uint8_t altuprflag(AirspacesDb::Airspace::altflag_unkn);
		int32_t altlwr(std::numeric_limits<int32_t>::max());
		int32_t altupr(0);
		for (components_t::const_iterator ci(m_components.begin()), ce(m_components.end()); ci != ce; ++ci) {
			switch (ci->get_lower_mode()) {
			case AltRange::alt_qnh:
				altlwrflag &= ~(AirspacesDb::Airspace::altflag_agl |
						AirspacesDb::Airspace::altflag_fl |
						AirspacesDb::Airspace::altflag_unkn);
				altlwr = std::min(altlwr, ci->get_lower_alt());
				break;

			case AltRange::alt_std:
				altlwrflag &= ~(AirspacesDb::Airspace::altflag_agl |
						AirspacesDb::Airspace::altflag_unkn);
				altlwrflag |= AirspacesDb::Airspace::altflag_fl;
				altlwr = std::min(altlwr, ci->get_lower_alt());
				break;

			case AltRange::alt_height:
				altlwrflag &= ~(AirspacesDb::Airspace::altflag_fl |
						AirspacesDb::Airspace::altflag_unkn);
				altlwrflag |= AirspacesDb::Airspace::altflag_agl;
				altlwr = std::min(altlwr, ci->get_lower_alt());
				break;

			default:
				break;
			}
			switch (ci->get_upper_mode()) {
			case AltRange::alt_qnh:
				altuprflag &= ~(AirspacesDb::Airspace::altflag_agl |
						AirspacesDb::Airspace::altflag_fl |
						AirspacesDb::Airspace::altflag_unkn);
				altupr = std::min(altupr, ci->get_upper_alt());
				break;

			case AltRange::alt_std:
				altuprflag &= ~(AirspacesDb::Airspace::altflag_agl |
						AirspacesDb::Airspace::altflag_unkn);
				altuprflag |= AirspacesDb::Airspace::altflag_fl;
				altupr = std::min(altupr, ci->get_upper_alt());
				break;

			case AltRange::alt_height:
				altuprflag &= ~(AirspacesDb::Airspace::altflag_fl |
						AirspacesDb::Airspace::altflag_unkn);
				altuprflag |= AirspacesDb::Airspace::altflag_agl;
				altupr = std::min(altupr, ci->get_upper_alt());
				break;

			default:
				break;
			}
			if (ci->is_poly()) {
				MultiPolygonHole mp(ci->get_poly());
				mp.geos_make_valid();
				switch (ci->get_operator()) {
				case Component::operator_base:
					poly.swap(mp);
					break;

				case Component::operator_union:
					if (poly.empty()) {
						poly.swap(mp);
						break;
					}
					poly.geos_union(mp);
					break;

				default:
					return false;
				}
			}
			if (ci->get_airspace().is_nil())
				continue;
			const AirspaceTimeSlice& as(ci->get_airspace().get_obj()->operator()(tm).as_airspace());
			if (!as.is_valid())
				return false;
			AirspacesDb::Airspace::Component comp;
			comp.set_icao(as.get_ident());
			comp.set_bdryclass(as.get_airspace_bdryclass());
			comp.set_typecode(AirspacesDb::Airspace::typecode_ead);
			switch (ci->get_operator()) {
			case Component::operator_base:
				comp.set_operator(AirspacesDb::Airspace::Component::operator_set);
				break;

			case Component::operator_union:
				comp.set_operator(AirspacesDb::Airspace::Component::operator_union);
				break;

			default:
				return false;
			}
			el.add_component(comp);
		}
		el.set_polygon(poly);
		el.compute_segments_from_poly();
		if (altlwrflag & AirspacesDb::Airspace::altflag_unkn)
			altlwr = 0;
		else if (altlwrflag & AirspacesDb::Airspace::altflag_agl && !altlwr)
			altlwrflag |= AirspacesDb::Airspace::altflag_gnd;
		else if (altlwr == std::numeric_limits<int32_t>::max())
			altlwrflag |= AirspacesDb::Airspace::altflag_unlim;
		if (altuprflag & AirspacesDb::Airspace::altflag_unkn)
			altupr = std::numeric_limits<int32_t>::max();
		else if (altuprflag & AirspacesDb::Airspace::altflag_agl && !altupr)
			altuprflag |= AirspacesDb::Airspace::altflag_gnd;
		else if (altupr == std::numeric_limits<int32_t>::max())
			altuprflag |= AirspacesDb::Airspace::altflag_unlim;
		el.set_altlwr(altlwr);
		el.set_altupr(altupr);
		el.set_altlwrflags(altlwrflag);
		el.set_altuprflags(altuprflag);
	}
	el.set_polygon(get_full_poly(tm));
	{
		AirspaceTimeSlice::elev_t e(get_gndelevmin());
		if (e != ElevPointIdentTimeSlice::invalid_elev)
			el.set_gndelevmin(e);
	}
	{
		AirspaceTimeSlice::elev_t e(get_gndelevmax());
		if (e != ElevPointIdentTimeSlice::invalid_elev)
			el.set_gndelevmax(e);
	}
	return true;
}

bool AirspaceTimeSlice::is_inside(const Point& pt, int32_t alt, const AltRange& altrange, const UUID& uuid) const
{
	if (!m_bbox.is_inside(pt))
		return false;
	bool val(false);
	for (components_t::const_iterator ci(m_components.begin()), ce(m_components.end()); ci != ce; ++ci) {
		switch (ci->get_operator()) {
		case Component::operator_base:
			val = ci->is_inside(pt, get_timeinterval(), alt, altrange, uuid);
			break;

		case Component::operator_union:
			val = val || ci->is_inside(pt, get_timeinterval(), alt, altrange, uuid);
			break;

		default:
			break;
		}
	}
	return val;
}

bool AirspaceTimeSlice::is_inside(const TimeTableEval& tte, int32_t alt, const AltRange& altrange, const UUID& uuid) const
{
	if (!m_bbox.is_inside(tte.get_point()))
		return false;
	if (!m_timetable.is_inside(tte))
		return false;
	bool val(false);
	for (components_t::const_iterator ci(m_components.begin()), ce(m_components.end()); ci != ce; ++ci) {
		switch (ci->get_operator()) {
		case Component::operator_base:
			val = ci->is_inside(tte, alt, altrange, uuid);
			break;

		case Component::operator_union:
			val = val || ci->is_inside(tte, alt, altrange, uuid);
			break;

		default:
			break;
		}
	}
	return val;
}

bool AirspaceTimeSlice::is_intersect(const TimeTableEval& tte, const Point& pt1, int32_t alt, const AltRange& altrange) const
{
	if (!m_bbox.is_inside(tte.get_point()) &&
	    !m_bbox.is_inside(pt1) &&
	    !m_bbox.is_intersect(tte.get_point(), pt1))
		return false;
	if (!m_timetable.is_inside(tte))
		return false;
	bool val(false);
	for (components_t::const_iterator ci(m_components.begin()), ce(m_components.end()); ci != ce; ++ci) {
		switch (ci->get_operator()) {
		case Component::operator_base:
			val = ci->is_intersect(tte, pt1, alt, altrange);
			break;

		case Component::operator_union:
			val = val || ci->is_intersect(tte, pt1, alt, altrange);
			break;

		default:
			break;
		}
	}
	return val;
}

bool AirspaceTimeSlice::is_intersect(const TimeTableEval& tte, const Point& pt1, int32_t alt0, int32_t alt1, const AltRange& altrange) const
{
	if (!m_bbox.is_inside(tte.get_point()) &&
	    !m_bbox.is_inside(pt1) &&
	    !m_bbox.is_intersect(tte.get_point(), pt1))
		return false;
	if (!m_timetable.is_inside(tte))
		return false;
	bool val(false);
	for (components_t::const_iterator ci(m_components.begin()), ce(m_components.end()); ci != ce; ++ci) {
		switch (ci->get_operator()) {
		case Component::operator_base:
			val = ci->is_intersect(tte, pt1, alt0, alt1, altrange);
			break;

		case Component::operator_union:
			val = val || ci->is_intersect(tte, pt1, alt0, alt1, altrange);
			break;

		default:
			break;
		}
	}
	return val;
}

IntervalSet<int32_t> AirspaceTimeSlice::get_point_altitudes(const TimeTableEval& tte, const AltRange& altrange, const UUID& uuid) const
{
	if (!m_bbox.is_inside(tte.get_point()))
		return IntervalSet<int32_t>();
	if (!m_timetable.is_inside(tte))
		return IntervalSet<int32_t>();
	IntervalSet<int32_t> val;
	for (components_t::const_iterator ci(m_components.begin()), ce(m_components.end()); ci != ce; ++ci) {
		IntervalSet<int32_t> val1(ci->get_point_altitudes(tte, altrange, uuid));
		switch (ci->get_operator()) {
		case Component::operator_base:
			val = val1;
			break;

		case Component::operator_union:
			val |= val1;
			break;

		// subtract: val &= ~val1;
		// intersect: val &= val1;

		default:
			break;
		}
	}
	return val;
}

IntervalSet<int32_t> AirspaceTimeSlice::get_intersect_altitudes(const TimeTableEval& tte, const Point& pt1, const AltRange& altrange) const
{
	if (!m_bbox.is_inside(tte.get_point()) &&
	    !m_bbox.is_inside(pt1) &&
	    !m_bbox.is_intersect(tte.get_point(), pt1))
		return IntervalSet<int32_t>();
	if (!m_timetable.is_inside(tte))
		return IntervalSet<int32_t>();
	IntervalSet<int32_t> val;
	for (components_t::const_iterator ci(m_components.begin()), ce(m_components.end()); ci != ce; ++ci) {
		IntervalSet<int32_t> val1(ci->get_intersect_altitudes(tte, pt1, altrange));
		switch (ci->get_operator()) {
		case Component::operator_base:
			val = val1;
			break;

		case Component::operator_union:
			val |= val1;
			break;

		// subtract: val &= ~val1;
		// intersect: val &= val1;

		default:
			break;
		}
	}
	return val;
}

IntervalSet<int32_t> AirspaceTimeSlice::get_point_intersect_altitudes(const TimeTableEval& tte, const Point& pt1, const AltRange& altrange, const UUID& uuid0, const UUID& uuid1) const
{
	if (!m_bbox.is_inside(tte.get_point()) &&
	    !m_bbox.is_inside(pt1) &&
	    !m_bbox.is_intersect(tte.get_point(), pt1))
		return IntervalSet<int32_t>();
	if (!m_timetable.is_inside(tte))
		return IntervalSet<int32_t>();
	IntervalSet<int32_t> val;
	for (components_t::const_iterator ci(m_components.begin()), ce(m_components.end()); ci != ce; ++ci) {
		IntervalSet<int32_t> val1(ci->get_point_intersect_altitudes(tte, pt1, altrange, uuid0, uuid1));
		switch (ci->get_operator()) {
		case Component::operator_base:
			val = val1;
			break;

		case Component::operator_union:
			val |= val1;
			break;

		// subtract: val &= ~val1;
		// intersect: val &= val1;

		default:
			break;
		}
	}
	return val;
}

std::vector<AirspaceTimeSlice::Trace> AirspaceTimeSlice::trace_inside(const TimeTableEval& tte, int32_t alt, const AltRange& altrange, const UUID& uuid, const Object::ptr_t& aspc) const
{
	if (!m_bbox.is_inside(tte.get_point())) {
		std::vector<AirspaceTimeSlice::Trace> r;
		r.push_back(Trace(aspc, Trace::nocomponent, Trace::reason_outsidebbox));
		return r;
	}
	if (!m_timetable.is_inside(tte)) {
		std::vector<AirspaceTimeSlice::Trace> r;
		r.push_back(Trace(aspc, Trace::nocomponent, Trace::reason_outsidetime));
		return r;
	}
	bool val(false);
	std::vector<AirspaceTimeSlice::Trace> r;
	for (components_t::const_iterator cb(m_components.begin()), ci(cb), ce(m_components.end()); ci != ce; ++ci) {
		switch (ci->get_operator()) {
		case Component::operator_base:
		{
			std::vector<AirspaceTimeSlice::Trace> r1(ci->trace_inside(tte, alt, altrange, uuid, aspc, ci - cb));
			r.insert(r.end(), r1.begin(), r1.end());
			val = r1.back();
			break;
		}

		case Component::operator_union:
		{
			std::vector<AirspaceTimeSlice::Trace> r1(ci->trace_inside(tte, alt, altrange, uuid, aspc, ci - cb));
			r.insert(r.end(), r1.begin(), r1.end());
			val = val || r1.back();
			break;
		}

		default:
			break;
		}
	}
	r.push_back(Trace(aspc, Trace::nocomponent, val ? Trace::reason_inside : Trace::reason_outside));
	return r;
}

std::vector<AirspaceTimeSlice::Trace> AirspaceTimeSlice::trace_intersect(const TimeTableEval& tte, const Point& pt1, int32_t alt, const AltRange& altrange, const Object::ptr_t& aspc) const
{
	if (!m_bbox.is_inside(tte.get_point()) &&
	    !m_bbox.is_inside(pt1) &&
	    !m_bbox.is_intersect(tte.get_point(), pt1)) {
		std::vector<AirspaceTimeSlice::Trace> r;
		r.push_back(Trace(aspc, Trace::nocomponent, Trace::reason_outsidebbox));
		return r;
	}
	if (!m_timetable.is_inside(tte)) {
		std::vector<AirspaceTimeSlice::Trace> r;
		r.push_back(Trace(aspc, Trace::nocomponent, Trace::reason_outsidetime));
		return r;
	}
	bool val(false);
	std::vector<AirspaceTimeSlice::Trace> r;
	for (components_t::const_iterator cb(m_components.begin()), ci(cb), ce(m_components.end()); ci != ce; ++ci) {
		switch (ci->get_operator()) {
		case Component::operator_base:
		{
			std::vector<AirspaceTimeSlice::Trace> r1(ci->trace_intersect(tte, pt1, alt, altrange, aspc, ci - cb));
			r.insert(r.end(), r1.begin(), r1.end());
			val = r1.back();
			break;
		}

		case Component::operator_union:
		{
			std::vector<AirspaceTimeSlice::Trace> r1(ci->trace_intersect(tte, pt1, alt, altrange, aspc, ci - cb));
			r.insert(r.end(), r1.begin(), r1.end());
			val = val || r1.back();
			break;
		}

		default:
			break;
		}
	}
	r.push_back(Trace(aspc, Trace::nocomponent, val ? Trace::reason_intersect : Trace::reason_nointersect));
	return r;
}

bool AirspaceTimeSlice::is_altitude_overlap(int32_t alt0, int32_t alt1, const timepair_t& tm, const AltRange& altrange) const
{
	for (components_t::const_iterator ci(m_components.begin()), ce(m_components.end()); ci != ce; ++ci) {
		if (ci->is_altitude_overlap(alt0, alt1, tm, altrange))
			return true;
	}
	return false;
}

bool AirspaceTimeSlice::recompute(void)
{
	bool work(IdentTimeSlice::recompute()), first(true);
	Rect newbbox(Point::invalid, Point::invalid);
	for (components_t::iterator ci(m_components.begin()), ce(m_components.end()); ci != ce; ++ci) {
		work = ci->recompute(*this) || work;
		if (ci->is_poly()) {
			Rect bbox(ci->get_poly().get_bbox());
			if (first) {
				newbbox = bbox;
				first = false;
			} else {
				newbbox = newbbox.add(bbox.get_southwest());
				newbbox = newbbox.add(bbox.get_northeast());
			}
		}
		const Object::const_ptr_t& aspc(ci->get_airspace().get_obj());
		for (unsigned int i(0), n(aspc->size()); i < n; ++i) {
			const TimeSlice& ts(aspc->operator[](i));
			if (!ts.is_overlap(*this))
				continue;
			const AirspaceTimeSlice& as(ts.as_airspace());
			if (!as.is_valid())
				continue;
			if (first) {
				as.get_bbox(newbbox);
				first = false;
			} else {
				Rect bbox;
				as.get_bbox(bbox);
				newbbox = newbbox.add(bbox);
			}
		}
	}
	work = work || newbbox != m_bbox;
	m_bbox = newbbox;
	return work;
}

bool AirspaceTimeSlice::recompute(TopoDb30& topodb)
{
	bool work(IdentTimeSlice::recompute(topodb));
	for (components_t::iterator ci(m_components.begin()), ce(m_components.end()); ci != ce; ++ci)
		work = ci->recompute(*this, topodb) || work;
	return work;
}

AirspaceTimeSlice::elev_t AirspaceTimeSlice::get_gndelevmin(void) const
{
	elev_t e(std::numeric_limits<elev_t>::max());
	for (components_t::const_iterator ci(m_components.begin()), ce(m_components.end()); ci != ce; ++ci)
		if (ci->is_gndelevmin_valid())
			e = std::min(e, ci->get_gndelevmin());
	if (e != std::numeric_limits<elev_t>::max())
		return e;
	return ElevPointIdentTimeSlice::invalid_elev;
}

AirspaceTimeSlice::elev_t AirspaceTimeSlice::get_gndelevmax(void) const
{
	elev_t e(std::numeric_limits<elev_t>::min());
	for (components_t::const_iterator ci(m_components.begin()), ce(m_components.end()); ci != ce; ++ci)
		if (ci->is_gndelevmax_valid())
			e = std::max(e, ci->get_gndelevmax());
	if (e != std::numeric_limits<elev_t>::min())
		return e;
	return ElevPointIdentTimeSlice::invalid_elev;
}

std::ostream& AirspaceTimeSlice::print(std::ostream& os) const
{
        IdentTimeSlice::print(os);
	os << ' ' << get_type() << ' ' << get_localtype() << (is_icao() ? " icao" : "")
	   << (is_flexibleuse() ? " flexibleuse" : "") << (is_level(1) ? " level1" : "")
	   << (is_level(2) ? " level2" : "") << (is_level(3) ? " level3" : "");
	get_timetable().print(os, 4);
	for (components_t::const_iterator ci(m_components.begin()), ce(m_components.end()); ci != ce; ++ci)
		ci->print(os << std::endl << "    component", *this);
	return os;
}
