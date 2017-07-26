#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iomanip>

#include "adrgraph.hh"
#include "adrfplan.hh"

using namespace ADR;

GraphVertex::GraphVertex(timetype_t tm, const Object::const_ptr_t& obj)
	: m_obj(obj), m_tsindex(0), m_validident(false)
{
	if (!get_object())
		return;
	unsigned int tsidx(~0U);
	unsigned int n(get_object()->size());
	for (unsigned int i(0); i < n; ++i) {
		const PointIdentTimeSlice& p(get_object()->operator[](i).as_point());
		if (!p.is_valid())
			continue;
		if (!p.is_inside(tm))
			continue;
		tsidx = i;
		break;
	}
	if (tsidx >= n) {
		std::ostringstream oss;
		oss << "GraphVertex: object ";
		if (get_object())
			oss << get_object()->get_uuid();
		else
			oss << "NULL";
		oss << " has no valid timeslice for "
		    << Glib::TimeVal(tm, 0).as_iso8601();
		throw std::runtime_error(oss.str());
	}
	m_tsindex = tsidx;
	m_validident = is_ident_valid(get_ident());
	if (get_coord().is_invalid()) {
		std::ostringstream oss;
		oss << "GraphVertex: object ";
		if (get_object())
			oss << get_object()->get_uuid();
		else
			oss << "NULL";
		oss << " has an invalid coordinate for "
		    << Glib::TimeVal(tm, 0).as_iso8601();
		throw std::runtime_error(oss.str());
	}
}

GraphVertex::GraphVertex(const Object::const_ptr_t& obj)
	: m_obj(obj), m_tsindex(0), m_validident(false)
{
	if (!get_object())
		return;
	if (!get_object()->size()) {
		std::ostringstream oss;
		oss << "GraphVertex: object ";
		if (get_object())
			oss << get_object()->get_uuid();
		else
			oss << "NULL";
		oss << " has no valid timeslice";
		throw std::runtime_error(oss.str());
	}
	m_validident = is_ident_valid(get_ident());	
}

bool GraphVertex::is_airport(void) const
{
	if (!get_object())
		return false;
	return get_object()->is_airport();
}

bool GraphVertex::is_navaid(void) const
{
	if (!get_object())
		return false;
	return get_object()->is_navaid();
}

bool GraphVertex::is_intersection(void) const
{
	if (!get_object())
		return false;
	return get_object()->is_intersection();
}

bool GraphVertex::get(timetype_t tm, AirportsDb::Airport& el) const
{
	if (!get_object())
		return false;
	return get_object()->get(tm, el);
}

bool GraphVertex::get(timetype_t tm, NavaidsDb::Navaid& el) const
{
	if (!get_object())
		return false;
	return get_object()->get(tm, el);
}

bool GraphVertex::get(timetype_t tm, WaypointsDb::Waypoint& el) const
{
	if (!get_object())
		return false;
	return get_object()->get(tm, el);
}

bool GraphVertex::set(timetype_t tm, FPlanWaypoint& el) const
{
	if (!get_object())
		return false;
	return get_object()->set(tm, el);
}

bool GraphVertex::set(timetype_t tm, FPLWaypoint& el) const
{
	if (!set(tm, static_cast<FPlanWaypoint&>(el)))
		return false;
	el.set_ptobj(get_object());
	return true;
}

unsigned int GraphVertex::insert(timetype_t tm, FPlanRoute& route, uint32_t nr) const
{
	if (!get_object())
		return 0;
	return get_object()->insert(tm, route, nr);
}

const UUID& GraphVertex::get_uuid(void) const
{
	if (get_object())
		return get_object()->get_uuid();
	return UUID::niluuid;
}

const PointIdentTimeSlice& GraphVertex::get_timeslice(void) const
{
	if (!get_object())
		return Object::invalid_pointidenttimeslice;
	return get_object()->operator[](m_tsindex).as_point();
}

bool GraphVertex::is_ident_valid(const std::string& id)
{
	return id.size() >= 2 && !Engine::AirwayGraphResult::Vertex::is_ident_numeric(id);
}

const float GraphEdge::maxmetric = std::numeric_limits<float>::max();
const float GraphEdge::invalidmetric = std::numeric_limits<float>::quiet_NaN();
const boost::uuids::uuid GraphEdge::matchalluuid = {
	0xb7, 0xc9, 0xbe, 0x9a, 0x2e, 0xcb, 0x49, 0x03,
	0xb2, 0xd5, 0x58, 0xf9, 0xac, 0xbe, 0x36, 0x75
};
const UUID GraphEdge::matchall(matchalluuid);

GraphEdge::GraphEdge(timetype_t tm, const Object::const_ptr_t& obj, unsigned int levels)
	: m_obj(obj), m_dist(maxmetric), m_truetrack(0), m_metric(0), m_tsindex(0), m_routetsindex(0), m_levels(levels), m_solution(-1)
{
        const unsigned int pis(get_levels());
        if (pis) {
                m_metric = new float[pis];
                for (unsigned int pi = 0; pi < pis; ++pi)
                        m_metric[pi] = invalidmetric;
        }
	if (!get_object())
		return;
	unsigned int tsidx(~0U);
	unsigned int n(get_object()->size());
	bool comp(false);
	for (unsigned int i(0); i < n; ++i) {
		const TimeSlice& s(get_object()->operator[](i));
		if (!s.is_valid())
			continue;
		if (!s.is_inside(tm))
			continue;
		comp = !s.as_segment().is_valid();
		if (comp && !(s.as_route().is_valid() || s.as_sid().is_valid() || s.as_star().is_valid()))
			continue;
		tsidx = i;
		break;
	}
	if (tsidx >= n) {
		std::ostringstream oss;
		oss << "GraphEdge: object " << get_object()->get_uuid() << " has no valid timeslice for "
		    << Glib::TimeVal(tm, 0).as_iso8601();
		throw std::runtime_error(oss.str());
	}
	m_tsindex = tsidx & 0x7f;
	if (!comp) {
		const SegmentTimeSlice& s(get_timeslice());
		if (!s.get_route().get_obj() || !s.get_start().get_obj() || !s.get_end().get_obj()) {
			std::ostringstream oss;
			oss << "GraphEdge: object " << get_object()->get_uuid() << " has unlinked objects for "
			    << Glib::TimeVal(tm, 0).as_iso8601();
			throw std::runtime_error(oss.str());
		}
		try {
			GraphVertex vs(tm, s.get_start().get_obj());
			GraphVertex ve(tm, s.get_end().get_obj());
			m_truetrack = vs.get_coord().spheric_true_course(ve.get_coord());
			m_dist = vs.get_coord().spheric_distance_nmi(ve.get_coord());
		} catch (const std::exception& e) {
			std::ostringstream oss;
			oss << "GraphEdge: object " << get_object()->get_uuid() << " has invalid endpoint objects for "
			    << Glib::TimeVal(tm, 0).as_iso8601() << " (" << e.what() << ')';
			throw std::runtime_error(oss.str());
		}
		unsigned int tsidx(~0U);
		unsigned int nr(s.get_route().get_obj()->size());
		for (unsigned int i(0); i < nr; ++i) {
			const IdentTimeSlice& ts(s.get_route().get_obj()->operator[](i).as_ident());
			if (!ts.is_valid())
				continue;
			if (!ts.is_inside(tm))
				continue;
			tsidx = i;
			break;
		}
		if (tsidx >= nr) {
			std::ostringstream oss;
			oss << "GraphEdge: route object " << get_object()->get_uuid() << " has no valid timeslice for "
			    << Glib::TimeVal(tm, 0).as_iso8601();
			throw std::runtime_error(oss.str());
		}
		m_routetsindex = tsidx & 0x7f;
	}
}

GraphEdge::GraphEdge(unsigned int levels, float dist, float truetrack)
	: m_dist(dist), m_truetrack(truetrack), m_metric(0), m_tsindex(0), m_routetsindex(0), m_levels(levels), m_solution(-1)
{
        const unsigned int pis(get_levels());
        if (pis) {
                m_metric = new float[pis];
                for (unsigned int pi = 0; pi < pis; ++pi)
                        m_metric[pi] = invalidmetric;
        }
}

GraphEdge::GraphEdge(const GraphEdge& x)
	: m_obj(x.m_obj), m_dist(x.m_dist), m_truetrack(x.m_truetrack), m_metric(0), m_tsindex(x.m_tsindex), m_routetsindex(x.m_routetsindex),
	  m_levels(x.m_levels), m_solution(x.m_solution)
{
        const unsigned int pis(get_levels());
        if (pis) {
                m_metric = new float[pis];
                for (unsigned int pi = 0; pi < pis; ++pi)
                        m_metric[pi] = x.m_metric[pi];
        }
}

GraphEdge::~GraphEdge()
{
	delete[] m_metric;
	m_metric = 0;
}

GraphEdge& GraphEdge::operator=(const GraphEdge& x)
{
	delete[] m_metric;
	m_metric = 0;
	m_obj = x.m_obj;
	m_tsindex = x.m_tsindex;
	m_routetsindex = x.m_routetsindex;
	m_levels = x.m_levels;
	m_solution = x.m_solution;
	m_dist = x.m_dist;
	m_truetrack = x.m_truetrack;
	const unsigned int pis(get_levels());
	if (pis) {
		m_metric = new float[pis];
		for (unsigned int pi = 0; pi < pis; ++pi)
			m_metric[pi] = x.m_metric[pi];
	}
	return *this;
}

void GraphEdge::swap(GraphEdge& x)
{
	std::swap(m_metric, x.m_metric);
	m_obj.swap(x.m_obj);
	std::swap(m_tsindex, x.m_tsindex);
	std::swap(m_routetsindex, x.m_routetsindex);
	std::swap(m_levels, x.m_levels);
	std::swap(m_solution, x.m_solution);
	std::swap(m_dist, x.m_dist);
	std::swap(m_truetrack, x.m_truetrack);
}

bool GraphEdge::is_awy(bool allowcomposite) const
{
	const TimeSlice& ts(get_object()->operator[](get_tsindex()));
	if (ts.as_routesegment().is_valid())
		return true;
	if (!allowcomposite)
		return false;
	return ts.as_route().is_valid();
}

bool GraphEdge::is_sid(bool allowcomposite) const
{
	const TimeSlice& ts(get_object()->operator[](get_tsindex()));
	if (ts.as_departureleg().is_valid())
		return true;
	if (!allowcomposite)
		return false;
	return ts.as_sid().is_valid();
}

bool GraphEdge::is_star(bool allowcomposite) const
{
	const TimeSlice& ts(get_object()->operator[](get_tsindex()));
	if (ts.as_arrivalleg().is_valid())
		return true;
	if (!allowcomposite)
		return false;
	return ts.as_star().is_valid();
}

bool GraphEdge::is_composite(void) const
{
	const TimeSlice& ts(get_object()->operator[](get_tsindex()));
	return ts.as_route().is_valid() || ts.as_sid().is_valid() || ts.as_star().is_valid();
}

const SegmentTimeSlice& GraphEdge::get_timeslice(void) const
{
	return get_object()->operator[](get_tsindex()).as_segment();
}

const Object::const_ptr_t& GraphEdge::get_route_object(bool allowcomposite) const
{
	static const Object::const_ptr_t nullobjptr;
	const TimeSlice& ts(get_object()->operator[](get_tsindex()));
	{
		const SegmentTimeSlice& tss(ts.as_segment());
		if (tss.is_valid())
			return tss.get_route().get_const_obj();
	}
	if (!allowcomposite || !ts.as_ident().is_valid())
		return nullobjptr;
	return get_object();
}

const IdentTimeSlice& GraphEdge::get_route_timeslice(bool allowcomposite) const
{
	const TimeSlice& ts(get_object()->operator[](get_tsindex()));
	{
		const SegmentTimeSlice& tss(ts.as_segment());
		if (tss.is_valid())
			return tss.get_route().get_obj()->operator[](get_routetsindex()).as_ident();
	}
	if (!allowcomposite)
		return Object::invalid_identtimeslice;
	return get_object()->operator[](get_tsindex()).as_ident();
}

RouteSegmentLink GraphEdge::get_routesegmentlink(void) const
{
	return RouteSegmentLink(Object::ptr_t::cast_const(get_object()), is_backward());
}

void GraphEdge::resize(unsigned int levels)
{
	const unsigned int pisold(get_levels());
	m_levels = levels;
	const unsigned int pis(get_levels());
	float *mold(0);
	if (pis)
		mold = new float[pis];
	std::swap(m_metric, mold);
	unsigned int i(0);
	for (unsigned int n(std::min(pis, pisold)); i < n; ++i)
		m_metric[i] = mold[i];
	for (; i < pis; ++i)
		m_metric[i] = invalidmetric;
	delete[] mold;
}

void GraphEdge::set_metric(unsigned int pi, float metric)
{
	if (pi >= get_levels())
		return;
	m_metric[pi] = metric;
}

float GraphEdge::get_metric(unsigned int pi) const
{
	if (pi >= get_levels())
		return invalidmetric;
	return m_metric[pi];
}

void GraphEdge::set_all_metric(float metric)
{
	const unsigned int pis(get_levels());
	for (unsigned int pi = 0; pi < pis; ++pi)
		m_metric[pi] = metric;
}

void GraphEdge::scale_metric(float scale, float offset)
{
	const unsigned int pis(get_levels());
	for (unsigned int pi = 0; pi < pis; ++pi)
		if (!Graph::is_metric_invalid(m_metric[pi]))
			m_metric[pi] = m_metric[pi] * scale + offset;
}

IntervalSet<int32_t> GraphEdge::get_altrange(const ConditionalAvailability& condavail, const TimeTableEval& tte) const
{
	timetype_t tuntil;
	return get_altrange(condavail, tte, tuntil);
}

IntervalSet<int32_t> GraphEdge::get_altrange(const ConditionalAvailability& condavail, const TimeTableEval& tte, timetype_t& tuntil) const
{
	tuntil = std::numeric_limits<timetype_t>::max();
	IntervalSet<int32_t> r;
	r.set_empty();
	const SegmentTimeSlice& seg(get_object()->operator()(tte.get_time()).as_segment());
	if (!seg.is_valid())
		return r;
	tuntil = std::min(tuntil, seg.get_endtime());
	const DepartureLegTimeSlice& dep(seg.as_departureleg());
	const ArrivalLegTimeSlice& arr(seg.as_arrivalleg());
	const RouteSegmentTimeSlice& rte(seg.as_routesegment());
	if (!dep.is_valid() && !arr.is_valid() && !rte.is_valid())
		return r;
	if (is_backward() && !rte.is_valid())
		return r;
	r.set_full();
	if (false) {
		int32_t minelev(0);
		if (seg.is_terrainelev_valid())
			minelev = seg.get_terrainelev();
		if (seg.is_corridor5elev_valid())
			minelev = std::max(minelev, seg.get_corridor5elev());
		if (minelev > 5000)
			minelev += 1000;
		minelev += 1000;
		r = IntervalSet<int32_t>(IntervalSet<int32_t>::Intvl(minelev, std::numeric_limits<int32_t>::max()));
	}
	if (!rte.is_valid() || rte.get_availability().empty()) {
		r &= seg.get_altrange().get_interval();
		return r;
	}
	IntervalSet<int32_t> r1;
	bool hascond(false);
	for (RouteSegmentTimeSlice::availability_t::const_iterator ai(rte.get_availability().begin()), ae(rte.get_availability().end()); ai != ae; ++ai) {
		const RouteSegmentTimeSlice::Availability& a(*ai);
		if ((is_backward() && !a.is_backward()) || (is_forward() && !a.is_forward()))
			continue;
		switch (a.get_status()) {
		case RouteSegmentTimeSlice::Availability::status_closed:
		default:
			continue;

		case RouteSegmentTimeSlice::Availability::status_open:
			break;

		case RouteSegmentTimeSlice::Availability::status_conditional:
			hascond = true;
			// FIXME: should probably be 2
			if (a.get_cdr() >= 3)
				continue;
			break;
		}
		if (!a.get_timetable().is_inside(tte))
			continue;
		r1 |= a.get_altrange().get_interval();
	}
	if (hascond) {
		AUPCDR::const_ptr_t p(AUPCDR::const_ptr_t::cast_dynamic(condavail.find(get_uuid(), tte.get_time())));
		if (p) {
			tuntil = std::min(tuntil, p->get_endtime());
			for (AUPCDR::availability_t::const_iterator ai(p->get_availability().begin()), ae(p->get_availability().end()); ai != ae; ++ai) {
				if ((is_backward() && ai->is_backward()) || (is_forward() && ai->is_forward()))
					continue;
				if (ai->get_cdr() >= 2)
					r1 &= ~ai->get_altrange().get_interval();
				else
					r1 |= ai->get_altrange().get_interval();
				if (true)
					std::cerr << "AUP: Segment " << get_uuid() << ' ' << ai->get_altrange().to_str()
						  << " CDR " << ai->get_cdr() << std::endl;
			}
		}
	}
	r &= r1;
	return r;
}

void GraphEdge::set_metric_seg(const ConditionalAvailability& condavail, const TimeTableEval& tte, int level, int levelinc)
{
	if (true) {
		IntervalSet<int32_t> r(get_altrange(condavail, tte));
		if (r.is_empty()) {
			clear_metric();
			return;
		}
		const unsigned int pis(get_levels());
		int lvl(level);
		for (unsigned int pi = 0; pi < pis; ++pi, lvl += levelinc) {
			if (r.is_inside(lvl))
				set_metric(pi, get_dist());
			else
				clear_metric(pi);
		}
		return;
	}
	const SegmentTimeSlice& seg(get_timeslice());
	if (!seg.is_valid()) {
		clear_metric();
		return;
	}
	const DepartureLegTimeSlice& dep(seg.as_departureleg());
	const ArrivalLegTimeSlice& arr(seg.as_arrivalleg());
	const RouteSegmentTimeSlice& rte(seg.as_routesegment());
	if (!dep.is_valid() && !arr.is_valid() && !rte.is_valid()) {
		clear_metric();
		return;
	}
	if (is_backward() && !rte.is_valid()) {
		clear_metric();
		return;
	}
	int32_t minelev(0);
	if (false) {
		if (seg.is_terrainelev_valid())
			minelev = seg.get_terrainelev();
		if (seg.is_corridor5elev_valid())
			minelev = std::max(minelev, seg.get_corridor5elev());
		if (minelev > 5000)
			minelev += 1000;
		minelev += 1000;
	}
	const unsigned int pis(get_levels());
	if (!rte.is_valid() || rte.get_availability().empty()) {
		for (unsigned int pi = 0; pi < pis; ++pi, level += levelinc) {
			if (level < minelev || !seg.get_altrange().is_inside(level)) {
				clear_metric(pi);
				continue;
			}
			set_metric(pi, get_dist());
		}
		return;
	}
	clear_metric();
	bool pcdrloaded(false);
	AUPCDR::const_ptr_t pcdr;
	for (RouteSegmentTimeSlice::availability_t::const_iterator ai(rte.get_availability().begin()), ae(rte.get_availability().end()); ai != ae; ++ai) {
		const RouteSegmentTimeSlice::Availability& a(*ai);
		if ((is_backward() && !a.is_backward()) || (is_forward() && !a.is_forward()))
			continue;
		switch (a.get_status()) {
		case RouteSegmentTimeSlice::Availability::status_closed:
		default:
			continue;

		case RouteSegmentTimeSlice::Availability::status_open:
			break;

		case RouteSegmentTimeSlice::Availability::status_conditional:
		{
			unsigned int cdr(a.get_cdr());
			if (!pcdrloaded) {
				pcdr = AUPCDR::const_ptr_t::cast_dynamic(condavail.find(get_uuid(), tte.get_time()));
				pcdrloaded = true;
			}
			if (pcdr) {
				for (AUPCDR::availability_t::const_iterator ai(pcdr->get_availability().begin()), ae(pcdr->get_availability().end()); ai != ae; ++ai) {
					if (ai->get_altrange() != a.get_altrange() || ai->is_forward() != a.is_forward())
						continue;
					cdr = ai->get_cdr();
					if (true)
						std::cerr << "AUP: Segment " << get_uuid() << ' ' << a.get_altrange().to_str() << " CDR " << cdr << std::endl;
					break;
				}
			}
			if (cdr >= 3)
				continue;
			break;
		}
		}
		if (!a.get_timetable().is_inside(tte))
			continue;
		int lvl(level);
		for (unsigned int pi = 0; pi < pis; ++pi, lvl += levelinc) {
			if (lvl < minelev || !a.get_altrange().is_inside(lvl))
				continue;
			set_metric(pi, get_dist());
		}
	}
}

void GraphEdge::set_metric_dct(int level, int levelinc, int32_t terrainelev, int32_t corridor5elev)
{
	int32_t minelev(std::max(terrainelev, corridor5elev));
	if (minelev > 5000)
		minelev += 1000;
	minelev += 1000;
	const unsigned int pis(get_levels());
	for (unsigned int pi = 0; pi < pis; ++pi, level += levelinc) {
		if (level < minelev) {
			clear_metric(pi);
			continue;
		}
		set_metric(pi, get_dist());
	}
}

bool GraphEdge::is_valid(unsigned int pi) const
{
	if (pi >= get_levels())
		return false;
	return !Graph::is_metric_invalid(m_metric[pi]);
}

bool GraphEdge::is_valid(void) const
{
	for (unsigned int pi(0), pis(get_levels()); pi < pis; ++pi)
		if (!Graph::is_metric_invalid(m_metric[pi]))
			return true;
	return false;
}

const UUID& GraphEdge::get_uuid(void) const
{
	if (get_object())
		return get_object()->get_uuid();
	return UUID::niluuid;
}

const UUID& GraphEdge::get_route_uuid(bool allowcomposite) const
{
	Object::const_ptr_t p(get_route_object(allowcomposite));
	if (!p)
		return UUID::niluuid;
	return p->get_uuid();
}

const std::string& GraphEdge::get_route_ident_or_dct(bool allowcomposite) const
{
	static const std::string dct("DCT");
	static const std::string invalid("?""?");
	if (!get_object())
		return dct;
	const std::string& id(get_route_ident(allowcomposite));
	if (!id.empty())
		return id;
	return invalid;
}

FPlanWaypoint::pathcode_t GraphEdge::get_pathcode(void) const
{
	if (is_dct())
		return FPlanWaypoint::pathcode_directto;
	if (is_awy(true))
		return FPlanWaypoint::pathcode_airway;
	if (is_sid(true))
		return FPlanWaypoint::pathcode_sid;
	if (is_star(true))
		return FPlanWaypoint::pathcode_star;
	return FPlanWaypoint::pathcode_none;
}

bool GraphEdge::is_match(const Object::const_ptr_t& p) const
{
	if (!p)
		return !get_object();
	if (p->get_uuid() == matchall)
		return true;
	if (!get_object())
		return false;
	if (p->get_uuid() == get_object()->get_uuid())
		return true;
	const SegmentTimeSlice& seg(get_object()->operator[](get_tsindex()).as_segment());
	if (!seg.is_valid())
		return false;
	return p->get_uuid() == seg.get_route();
}

bool GraphEdge::is_match(const UUID& uuid) const
{
	if (uuid == matchall)
		return true;
	if (!get_object())
		return uuid.is_nil();
	if (uuid == get_object()->get_uuid())
		return true;
	const SegmentTimeSlice& seg(get_object()->operator[](get_tsindex()).as_segment());
	if (!seg.is_valid())
		return false;
	return uuid == seg.get_route();
}

bool GraphEdge::is_match(matchcomp_t mc) const
{
	switch (mc) {
	case matchcomp_segment:
		return !is_composite();

	case matchcomp_composite:
		return is_composite();

	case matchcomp_route:
		return !is_noroute();

	default:
		return true;
	}
}

Object::ptr_t GraphEdge::get_matchall(void)
{
	return Object::ptr_t(new ObjectImpl<TimeSlice,Object::type_invalid>(matchall));
}

bool GraphEdge::is_overlap(int32_t alt0, int32_t alt1) const
{
	const SegmentTimeSlice& seg(get_timeslice());
	if (!seg.is_valid())
		return true;
	const RouteSegmentTimeSlice& rte(seg.as_routesegment());
	if (!rte.is_valid() || rte.get_availability().empty())
		return seg.get_altrange().is_overlap(alt0, alt1);
	for (RouteSegmentTimeSlice::availability_t::const_iterator ai(rte.get_availability().begin()), ae(rte.get_availability().end()); ai != ae; ++ai) {
		const RouteSegmentTimeSlice::Availability& a(*ai);
		if ((is_backward() && !a.is_backward()) || (is_forward() && !a.is_forward()))
			continue;
		switch (a.get_status()) {
		case RouteSegmentTimeSlice::Availability::status_closed:
		default:
			continue;

		case RouteSegmentTimeSlice::Availability::status_open:
			break;

		case RouteSegmentTimeSlice::Availability::status_conditional:
			if (a.get_cdr() >= 3)
				continue;
			break;
		}
		if (a.get_altrange().is_overlap(alt0, alt1))
			return true;
	}
	return false;
}

bool GraphEdge::is_inside(int32_t alt0, int32_t alt1) const
{
	const SegmentTimeSlice& seg(get_timeslice());
	if (!seg.is_valid())
		return true;
	const RouteSegmentTimeSlice& rte(seg.as_routesegment());
	if (!rte.is_valid() || rte.get_availability().empty())
		return seg.get_altrange().is_inside(alt0, alt1);
	for (RouteSegmentTimeSlice::availability_t::const_iterator ai(rte.get_availability().begin()), ae(rte.get_availability().end()); ai != ae; ++ai) {
		const RouteSegmentTimeSlice::Availability& a(*ai);
		if ((is_backward() && !a.is_backward()) || (is_forward() && !a.is_forward()))
			continue;
		switch (a.get_status()) {
		case RouteSegmentTimeSlice::Availability::status_closed:
		default:
			continue;

		case RouteSegmentTimeSlice::Availability::status_open:
			break;

		case RouteSegmentTimeSlice::Availability::status_conditional:
			if (a.get_cdr() >= 3)
				continue;
			break;
		}
		if (a.get_altrange().is_inside(alt0, alt1))
			return true;
	}
	return false;
}

bool GraphEdge::is_inside(int32_t alt0, int32_t alt1, timetype_t tm, const TimeTableSpecialDateEval& ttsde) const
{
	const SegmentTimeSlice& seg(get_timeslice());
	if (!seg.is_valid())
		return true;
	const RouteSegmentTimeSlice& rte(seg.as_routesegment());
	if (!rte.is_valid() || rte.get_availability().empty())
		return seg.get_altrange().is_inside(alt0, alt1);
	Point pt;
	pt.set_invalid();
	{
		const PointIdentTimeSlice& ts((is_backward() ? seg.get_end().get_obj() : seg.get_start().get_obj())->operator()(tm).as_point());
		if (ts.is_valid())
			pt = ts.get_coord();
	}
	TimeTableEval tte(tm, pt, ttsde);
	for (RouteSegmentTimeSlice::availability_t::const_iterator ai(rte.get_availability().begin()), ae(rte.get_availability().end()); ai != ae; ++ai) {
		const RouteSegmentTimeSlice::Availability& a(*ai);
		if ((is_backward() && !a.is_backward()) || (is_forward() && !a.is_forward()))
			continue;
		switch (a.get_status()) {
		case RouteSegmentTimeSlice::Availability::status_closed:
		default:
			continue;

		case RouteSegmentTimeSlice::Availability::status_open:
			break;

		case RouteSegmentTimeSlice::Availability::status_conditional:
			if (a.get_cdr() >= 3)
				continue;
			break;
		}
		if (a.get_altrange().is_inside(alt0, alt1) && a.get_timetable().is_inside(tte))
			return true;
	}
	return false;
}

IntervalSet<int32_t> GraphEdge::get_altrange(const SegmentTimeSlice& seg) const
{
	IntervalSet<int32_t> r;
	r.set_empty();
	if (!seg.is_valid())
		return r;
	const RouteSegmentTimeSlice& rte(seg.as_routesegment());
	if (!rte.is_valid() || rte.get_availability().empty()) {
		r |= seg.get_altrange().get_interval();
		return r;
	}
	for (RouteSegmentTimeSlice::availability_t::const_iterator ai(rte.get_availability().begin()), ae(rte.get_availability().end()); ai != ae; ++ai) {
		const RouteSegmentTimeSlice::Availability& a(*ai);
		if ((is_backward() && !a.is_backward()) || (is_forward() && !a.is_forward()))
			continue;
		switch (a.get_status()) {
		case RouteSegmentTimeSlice::Availability::status_closed:
		default:
			continue;

		case RouteSegmentTimeSlice::Availability::status_open:
			break;

		case RouteSegmentTimeSlice::Availability::status_conditional:
			if (a.get_cdr() >= 3)
				continue;
			break;
		}
		r |= a.get_altrange().get_interval();
	}
	return r;
}

IntervalSet<int32_t> GraphEdge::get_altrange(void) const
{
	return get_altrange(get_timeslice());
}

IntervalSet<int32_t> GraphEdge::get_altrange(timetype_t tm) const
{
	return get_altrange(get_object()->operator()(tm).as_segment());
}

const Graph::vertex_descriptor Graph::invalid_vertex_descriptor;
const Graph::objectset_t Graph::empty_objectset;
const bool Graph::restartedgescan;

void Graph::swap(Graph& x)
{
	base_t::swap(x);
	std::swap(m_nameindex, x.m_nameindex);
	std::swap(m_dependson, x.m_dependson);
	std::swap(m_vertexmap, x.m_vertexmap);
}

void Graph::clear(void)
{
	base_t::clear();
	m_nameindex.clear();
	m_dependson.clear();
	m_vertexmap.clear();
}

Graph::findresult_t Graph::find_ident(const std::string& ident) const
{
	nameindex_t::const_iterator i(m_nameindex.find(ident));
	if (i == m_nameindex.end())
		return findresult_t(empty_objectset.begin(), empty_objectset.end());
	return findresult_t(i->second.begin(), i->second.end());
}

Graph::findresult_t Graph::find_dependson(const Object::const_ptr_t& p) const
{
	dependson_t::const_iterator i(m_dependson.find(p));
	if (i == m_dependson.end())
		return findresult_t(empty_objectset.begin(), empty_objectset.end());
	return findresult_t(i->second.begin(), i->second.end());
}

Graph::findresult_t Graph::find_dependson(const UUID& u) const
{
	Object::ptr_t p(new ObjectImpl<TimeSlice,Object::type_invalid>(u));
	return find_dependson(p);
}

bool Graph::add_dep(const Object::const_ptr_t& p, const Object::const_ptr_t& pdep)
{
	if (!p || !pdep)
		return false;
	std::pair<dependson_t::iterator,bool> ins(m_dependson.insert(dependson_t::value_type(pdep, objectset_t())));
	std::pair<objectset_t::iterator,bool> ins2(ins.first->second.insert(p));
	return ins2.second;
}

unsigned int Graph::add(timetype_t tm, const Object::const_ptr_t& p, unsigned int levels)
{
	if (!p || p->empty() || p->get_uuid().is_nil())
		return 0;
	const TimeSlice& ts(p->operator()(tm));
	if (!ts.is_valid())
		return 0;
	unsigned int ret(0);
	bool retifname(false);
	// check if point object
	if (ts.as_point().is_valid()) {
		std::pair<vertexmap_t::iterator,bool> ins(m_vertexmap.insert(vertexmap_t::value_type(p, boost::num_vertices(*this))));
		if (false)
			std::cerr << "Graph::add: point " << ts.as_ident().get_ident() << (ins.second ? "" : " (duplicate)") << std::endl;
		if (!ins.second)
			return ret;
		ins.first->second = boost::add_vertex(GraphVertex(tm, p), *this);
		++ret;
	} else if (ts.as_segment().is_valid()) {
		const SegmentTimeSlice& tss(ts.as_segment());
		if (false)
			std::cerr << "Graph::add: route segment " << tss.get_route().get_obj()->operator()(tm).as_ident().get_ident()
				  << ' ' << tss.get_start().get_obj()->operator()(tm).as_ident().get_ident()
				  << ' ' << tss.get_end().get_obj()->operator()(tm).as_ident().get_ident() << std::endl;
		if (!tss.get_route().get_obj() || !tss.get_start().get_obj() || !tss.get_end().get_obj())
			return ret;
		ret += add(tm, tss.get_route().get_obj(), levels);
		ret += add(tm, tss.get_start().get_obj(), levels);
		ret += add(tm, tss.get_end().get_obj(), levels);
		std::pair<vertex_descriptor,bool> v0(find_vertex(tss.get_start().get_obj()));
		std::pair<vertex_descriptor,bool> v1(find_vertex(tss.get_end().get_obj()));
		if (!v0.second || !v1.second)
			return ret;
		GraphEdge edge(tm, p, levels);
		std::pair<edge_descriptor, bool> ef, eb;
		ef.second = tss.is_forward();
		eb.second = tss.is_backward();
		if (ef.second) {
			out_edge_iterator e0, e1;
			for (boost::tie(e0, e1) = boost::out_edges(v0.first, *this); e0 != e1; ++e0) {
				if (boost::target(*e0, *this) != v1.first)
					continue;
				if ((*this)[*e0].get_uuid() != p->get_uuid())
					continue;
				ef.second = false;
				break;
			}
			if (ef.second) {
				ef = boost::add_edge(v0.first, v1.first, edge, *this);
				if (ef.second) {
					GraphEdge& ee((*this)[ef.first]);
					ee.set_forward();
				}
			}
		}
		if (eb.second) {
			out_edge_iterator e0, e1;
			for (boost::tie(e0, e1) = boost::out_edges(v1.first, *this); e0 != e1; ++e0) {
				if (boost::target(*e0, *this) != v0.first)
					continue;
				if ((*this)[*e0].get_uuid() != p->get_uuid())
					continue;
				eb.second = false;
				break;
			}
			if (eb.second) {
				eb = boost::add_edge(v1.first, v0.first, edge, *this);
				if (eb.second) {
					GraphEdge& ee((*this)[eb.first]);
					ee.set_backward();
				}
			}
		}
		if (ef.second || eb.second)
			++ret;
		add_dep(p, tss.get_route().get_obj());
		add_dep(p, tss.get_start().get_obj());
		add_dep(p, tss.get_end().get_obj());
	} else if (ts.as_sid().is_valid()) {
		const StandardInstrumentDepartureTimeSlice& tss(ts.as_sid());
		if (false)
			std::cerr << "Graph::add: SID " << tss.get_ident() << std::endl;
		if (tss.get_ident().empty())
			return ret;
		if (!tss.get_airport().get_obj())
			return ret;
		for (StandardInstrumentDepartureTimeSlice::connpoints_t::const_iterator ci(tss.get_connpoints().begin()), ce(tss.get_connpoints().end()); ci != ce; ++ci)
			if (!ci->get_obj())
				return ret;
		ret += add(tm, tss.get_airport().get_obj());
		add_dep(p, tss.get_airport().get_obj());
		for (StandardInstrumentDepartureTimeSlice::connpoints_t::const_iterator ci(tss.get_connpoints().begin()), ce(tss.get_connpoints().end()); ci != ce; ++ci) {
			ret += add(tm, ci->get_obj());
			add_dep(p, ci->get_obj());
		}
		retifname = true;
	} else if (ts.as_star().is_valid()) {
		const StandardInstrumentArrivalTimeSlice& tss(ts.as_star());
		if (false)
			std::cerr << "Graph::add: STAR " << tss.get_ident() << std::endl;
		if (tss.get_ident().empty())
			return ret;
		if (!tss.get_airport().get_obj())
			return ret;
		for (StandardInstrumentArrivalTimeSlice::connpoints_t::const_iterator ci(tss.get_connpoints().begin()), ce(tss.get_connpoints().end()); ci != ce; ++ci)
			if (!ci->get_obj())
				return ret;
		if (false && !tss.get_iaf().get_obj())
			return ret;
		ret += add(tm, tss.get_airport().get_obj());
		add_dep(p, tss.get_airport().get_obj());
		for (StandardInstrumentArrivalTimeSlice::connpoints_t::const_iterator ci(tss.get_connpoints().begin()), ce(tss.get_connpoints().end()); ci != ce; ++ci) {
			ret += add(tm, ci->get_obj());
			add_dep(p, ci->get_obj());
		}
		if (tss.get_iaf().get_obj()) {
			ret += add(tm, tss.get_iaf().get_obj());
			add_dep(p, tss.get_iaf().get_obj());
		}
		retifname = true;
	} else if (ts.as_route().is_valid()) {
		const RouteTimeSlice& tss(ts.as_route());
		if (false)
			std::cerr << "Graph::add: route " << tss.get_ident() << std::endl;
		if (tss.get_ident().empty())
			return ret;
		retifname = true;
	} else {
		if (false) {
			if (ts.as_ident().is_valid())
				std::cerr << "Graph::add: discarding unknown " << ts.as_ident().get_ident() << std::endl;
			else
				std::cerr << "Graph::add: discarding unknown ?""?" << std::endl;
		}
		return ret;
	}
	{
		const std::string& id(ts.as_ident().get_ident());
		if (!id.empty()) {
			std::pair<nameindex_t::iterator,bool> ins(m_nameindex.insert(nameindex_t::value_type(id, objectset_t())));
			std::pair<objectset_t::iterator,bool> ins2(ins.first->second.insert(p));
			if (retifname && ins2.second)
				++ret;
		}
	}
	return ret;
}

unsigned int Graph::add(const Object::const_ptr_t& p, unsigned int levels)
{
	if (!p || p->empty() || p->get_uuid().is_nil())
		return 0;
	const TimeSlice& ts(p->operator[](0));
	if (!ts.is_valid())
		return 0;
	unsigned int ret(0);
	bool retifname(false);
	// check if point object
	if (ts.as_point().is_valid()) {
		std::pair<vertexmap_t::iterator,bool> ins(m_vertexmap.insert(vertexmap_t::value_type(p, boost::num_vertices(*this))));
		if (false)
			std::cerr << "Graph::add: point " << ts.as_ident().get_ident() << (ins.second ? "" : " (duplicate)") << std::endl;
		if (!ins.second)
			return ret;
		ins.first->second = boost::add_vertex(GraphVertex(p), *this);
		++ret;
	} else if (ts.as_segment().is_valid()) {
		for (unsigned int i(0), n(p->size()); i < n; ++i) {
			const SegmentTimeSlice& tss(p->operator[](i).as_segment());
			if (!tss.is_valid())
				continue;
			if (false)
				std::cerr << "Graph::add: route segment " << tss.get_route().get_obj()->operator()(tss).as_ident().get_ident()
					  << ' ' << tss.get_start().get_obj()->operator()(tss).as_ident().get_ident()
					  << ' ' << tss.get_end().get_obj()->operator()(tss).as_ident().get_ident() << std::endl;
			if (!tss.get_route().get_obj() || !tss.get_start().get_obj() || !tss.get_end().get_obj())
				continue;;
			ret += add(tss.get_route().get_obj(), levels);
			ret += add(tss.get_start().get_obj(), levels);
			ret += add(tss.get_end().get_obj(), levels);
			std::pair<vertex_descriptor,bool> v0(find_vertex(tss.get_start().get_obj()));
			std::pair<vertex_descriptor,bool> v1(find_vertex(tss.get_end().get_obj()));
			if (!v0.second || !v1.second)
				continue;
			// find suitable time where both endpoints are defined
			timetype_t tm;
			{
				bool tmset(false);
				for (unsigned int i0(0), n0(tss.get_start().get_obj()->size()); i0 < n0; ++i0) {
					const TimeSlice& ts0(tss.get_start().get_obj()->operator[](i0));
					if (!ts0.is_overlap(tss))
						continue;
					for (unsigned int i1(0), n1(tss.get_end().get_obj()->size()); i1 < n1; ++i1) {
						const TimeSlice& ts1(tss.get_end().get_obj()->operator[](i1));
						if (!ts1.is_overlap(tss) || !ts1.is_overlap(ts0))
							continue;
						timetype_t t0(std::max(std::max(ts0.get_starttime(), ts1.get_starttime()), tss.get_starttime()));
						timetype_t t1(std::min(std::min(ts0.get_endtime(), ts1.get_endtime()), tss.get_endtime()));
						if (t1 <= t0)
							continue;
						tm = t0;
						tmset = true;
						break;
					}
					if (tmset)
						break;
				}
				if (!tmset)
					continue;
			}
			GraphEdge edge(tm, p, levels);
			std::pair<edge_descriptor, bool> ef, eb;
			ef.second = tss.is_forward();
			eb.second = tss.is_backward();
			if (ef.second) {
				out_edge_iterator e0, e1;
				for (boost::tie(e0, e1) = boost::out_edges(v0.first, *this); e0 != e1; ++e0) {
					if (boost::target(*e0, *this) != v1.first)
						continue;
					if ((*this)[*e0].get_uuid() != p->get_uuid())
						continue;
					ef.second = false;
					break;
				}
				if (ef.second) {
					ef = boost::add_edge(v0.first, v1.first, edge, *this);
					if (ef.second) {
						GraphEdge& ee((*this)[ef.first]);
						ee.set_forward();
					}
				}
			}
			if (eb.second) {
				out_edge_iterator e0, e1;
				for (boost::tie(e0, e1) = boost::out_edges(v1.first, *this); e0 != e1; ++e0) {
					if (boost::target(*e0, *this) != v0.first)
						continue;
					if ((*this)[*e0].get_uuid() != p->get_uuid())
						continue;
					eb.second = false;
					break;
				}
				if (eb.second) {
					eb = boost::add_edge(v1.first, v0.first, edge, *this);
					if (eb.second) {
						GraphEdge& ee((*this)[eb.first]);
						ee.set_backward();
					}
				}
			}
			if (ef.second || eb.second)
				++ret;
			add_dep(p, tss.get_route().get_obj());
			add_dep(p, tss.get_start().get_obj());
			add_dep(p, tss.get_end().get_obj());
		}
	} else if (ts.as_sid().is_valid()) {
		for (unsigned int i(0), n(p->size()); i < n; ++i) {
			const StandardInstrumentDepartureTimeSlice& tss(p->operator[](i).as_sid());
			if (!tss.is_valid())
				continue;
			if (false)
				std::cerr << "Graph::add: SID " << tss.get_ident() << std::endl;
			if (tss.get_ident().empty())
				continue;
			if (!tss.get_airport().get_obj())
				continue;
			{
				StandardInstrumentDepartureTimeSlice::connpoints_t::const_iterator ci(tss.get_connpoints().begin()), ce(tss.get_connpoints().end());
				for (; ci != ce; ++ci)
					if (!ci->get_obj())
						break;
				if (ci != ce)
					continue;
			}
			ret += add(tss.get_airport().get_obj());
			add_dep(p, tss.get_airport().get_obj());
			for (StandardInstrumentDepartureTimeSlice::connpoints_t::const_iterator ci(tss.get_connpoints().begin()), ce(tss.get_connpoints().end()); ci != ce; ++ci) {
				ret += add(ci->get_obj());
				add_dep(p, ci->get_obj());
			}
		}
		retifname = true;
	} else if (ts.as_star().is_valid()) {
		for (unsigned int i(0), n(p->size()); i < n; ++i) {
			const StandardInstrumentArrivalTimeSlice& tss(p->operator[](i).as_star());
			if (!tss.is_valid())
				continue;
			if (false)
				std::cerr << "Graph::add: STAR " << tss.get_ident() << std::endl;
			if (tss.get_ident().empty())
				continue;
			if (!tss.get_airport().get_obj())
				continue;
			{
				StandardInstrumentArrivalTimeSlice::connpoints_t::const_iterator ci(tss.get_connpoints().begin()), ce(tss.get_connpoints().end());
				for (; ci != ce; ++ci)
					if (!ci->get_obj())
						break;
				if (ci != ce)
					continue;
			}
			if (false && !tss.get_iaf().get_obj())
				continue;
			ret += add(tss.get_airport().get_obj());
			add_dep(p, tss.get_airport().get_obj());
			for (StandardInstrumentArrivalTimeSlice::connpoints_t::const_iterator ci(tss.get_connpoints().begin()), ce(tss.get_connpoints().end()); ci != ce; ++ci) {
				ret += add(ci->get_obj());
				add_dep(p, ci->get_obj());
			}
			if (tss.get_iaf().get_obj()) {
				ret += add(tss.get_iaf().get_obj());
				add_dep(p, tss.get_iaf().get_obj());
			}
		}
		retifname = true;
	} else if (ts.as_route().is_valid()) {
		for (unsigned int i(0), n(p->size()); i < n; ++i) {
			const RouteTimeSlice& tss(p->operator[](i).as_route());
			if (!tss.is_valid())
				continue;
			if (false)
				std::cerr << "Graph::add: route " << tss.get_ident() << std::endl;
			if (tss.get_ident().empty())
				continue;
		}
		retifname = true;
	} else {
		if (false) {
			if (ts.as_ident().is_valid())
				std::cerr << "Graph::add: discarding unknown " << ts.as_ident().get_ident() << std::endl;
			else
				std::cerr << "Graph::add: discarding unknown ?""?" << std::endl;
		}
		return ret;
	}
	{
		for (unsigned int i(0), n(p->size()); i < n; ++i) {
			const std::string& id(p->operator[](i).as_ident().get_ident());
			if (id.empty())
				continue;
			std::pair<nameindex_t::iterator,bool> ins(m_nameindex.insert(nameindex_t::value_type(id, objectset_t())));
			std::pair<objectset_t::iterator,bool> ins2(ins.first->second.insert(p));
			if (retifname && ins2.second)
				++ret;
		}
	}
	return ret;
}

bool Graph::is_valid_connection_precheck(unsigned int piu, unsigned int piv, edge_descriptor e) const
{
	const GraphEdge& ee((*this)[e]);
	const unsigned int pis(ee.get_levels());
	if (piu >= pis) {
		if (piv >= pis)
			return false;
		// check that the destination level is part of the SID
		if (!ee.is_valid(piv))
			return false;
	} else if (piv >= pis) {
		// check that the originating level is part of the STAR
		if (!ee.is_valid(piu))
			return false;
	} else {
		if (!ee.is_valid(piu))
			return false;
	}
	return true;
}

bool Graph::is_valid_connection_precheck(vertex_descriptor u, unsigned int piu, vertex_descriptor v, unsigned int piv, edge_descriptor e) const
{
	if (boost::source(e, *this) != u || boost::target(e, *this) != v)
		return false;
	return is_valid_connection(piu, piv, e);
}

bool Graph::is_valid_connection(unsigned int piu, unsigned int piv, edge_descriptor e) const
{
	static const bool allow_awychange = false;
	const GraphEdge& ee((*this)[e]);
	const unsigned int pis(ee.get_levels());
	if (piu >= pis) {
		if (piv >= pis)
			return false;
		// check that the destination level is part of the SID
		if (!ee.is_valid(piv))
			return false;
	} else if (piv >= pis) {
		// check that the originating level is part of the STAR
		if (!ee.is_valid(piu))
			return false;
	} else {
		if (!ee.is_valid(piu))
			return false;
		if (piu == piv)
			return true;
		// check that level changes followed by DCT edges do not violate DCT level restrictions
		unsigned int pi0(piu), pi1(piv);
		if (pi0 > pi1)
			std::swap(pi0, pi1);
		for (; pi0 <= pi1; ++pi0) {
			if (ee.is_valid(pi0))
				continue;
			if (!ee.is_dct() && (allow_awychange && !ee.is_awy()))
				return false;
			vertex_descriptor u(boost::source(e, *this));
			vertex_descriptor v(boost::target(e, *this));
			out_edge_iterator e0, e1;
			for (boost::tie(e0, e1) = boost::out_edges(u, *this); e0 != e1; ++e0) {
				if (boost::target(*e0, *this) != v)
					continue;
				const GraphEdge& eee((*this)[*e0]);
				if (allow_awychange) {
					if (!(ee.is_dct() && eee.is_dct()) && !eee.is_awy())
						continue;
				} else {
					if (!eee.is_dct() && !eee.is_awy())
						continue;
				}
				if (eee.is_valid(pi0))
					break;
			}
			if (e0 == e1)
				return false;
		}
	}
	return true;
}

bool Graph::is_valid_connection(vertex_descriptor u, unsigned int piu, vertex_descriptor v, unsigned int piv, edge_descriptor e) const
{
	if (boost::source(e, *this) != u || boost::target(e, *this) != v)
		return false;
	return is_valid_connection(piu, piv, e);
}

std::pair<Graph::edge_descriptor,bool> Graph::find_edge(vertex_descriptor u, vertex_descriptor v, const Object::const_ptr_t& p) const
{
	out_edge_iterator e0, e1;
	for (boost::tie(e0, e1) = boost::out_edges(u, *this); e0 != e1; ++e0) {
		if (boost::target(*e0, *this) != v)
			continue;
		const GraphEdge& ee((*this)[*e0]);
		if (!ee.is_match(p))
			continue;
		return std::pair<edge_descriptor,bool>(*e0, true);
	}
	return std::pair<edge_descriptor,bool>(edge_descriptor(), false);
}

std::pair<Graph::edge_descriptor,bool> Graph::find_edge(vertex_descriptor u, vertex_descriptor v, const UUID& uuid) const
{
	out_edge_iterator e0, e1;
	for (boost::tie(e0, e1) = boost::out_edges(u, *this); e0 != e1; ++e0) {
		if (boost::target(*e0, *this) != v)
			continue;
		const GraphEdge& ee((*this)[*e0]);
		if (!ee.is_match(uuid))
			continue;
		return std::pair<edge_descriptor,bool>(*e0, true);
	}
	return std::pair<edge_descriptor,bool>(edge_descriptor(), false);
}

std::pair<Graph::vertex_descriptor,bool> Graph::find_vertex(const Object::const_ptr_t& p) const
{
	vertexmap_t::const_iterator i(m_vertexmap.find(p));
	if (i == m_vertexmap.end())
		return std::pair<vertex_descriptor,bool>(~0U, false);
	return std::pair<vertex_descriptor,bool>(i->second, true);
}

std::pair<Graph::vertex_descriptor,bool> Graph::find_vertex(const UUID& uuid) const
{
	Object::ptr_t p(new ObjectImpl<TimeSlice,Object::type_invalid>(uuid));
	return find_vertex(p);
}

unsigned int Graph::kill_empty_edges(void)
{
	unsigned int work(0);
	Graph::vertex_iterator vi, ve;
	for (boost::tie(vi, ve) = boost::vertices(*this); vi != ve; ++vi) {
		for (;;) {
			bool done(true);
			Graph::out_edge_iterator e0, e1;
			for (boost::tie(e0, e1) = boost::out_edges(*vi, *this); e0 != e1;) {
				Graph::out_edge_iterator ex(e0);
				++e0;
				GraphEdge& ee((*this)[*ex]);
				if (ee.is_valid())
					continue;
				boost::remove_edge(ex, *this);
				++work;
				if (restartedgescan) {
					done = false;
					break;
				}
			}
			if (done)
				break;
		}
	}
	return work;
}

std::ostream& Graph::print(std::ostream& os) const
{
        Graph::edge_iterator e0, e1;
        for (boost::tie(e0, e1) = boost::edges(*this); e0 != e1; ++e0) {
                const GraphEdge& ee((*this)[*e0]);
		vertex_descriptor u(boost::source(*e0, *this));
		vertex_descriptor v(boost::target(*e0, *this));
		const GraphVertex uu((*this)[u]);
		const GraphVertex vv((*this)[v]);
		{
			std::ostringstream ossd, osstt;
			ossd << std::fixed << std::setprecision(1) << ee.get_dist();
			osstt << std::fixed << std::setprecision(0) << ee.get_truetrack();
			os << uu.get_ident() << '(' << u << ")->"
			   << vv.get_ident() << '(' << v << "): "
			   << ee.get_route_ident_or_dct(true);
			if (false) {
				const SegmentTimeSlice& tsseg(ee.get_timeslice());
				if (tsseg.is_valid()) {
					const PointIdentTimeSlice& tss(tsseg.get_start().get_obj()->operator()(tsseg).as_point());
					const PointIdentTimeSlice& tse(tsseg.get_end().get_obj()->operator()(tsseg).as_point());
					os << " [" << tss.get_ident() << "->" << tse.get_ident() << ']';
				}
			}
			os << " (";
			if (ee.is_dct())
				os << 'D';
			if (ee.is_awy(true))
				os << 'A';
			if (ee.is_sid(true))
				os << 'd';
			if (ee.is_star(true))
				os << 'a';
			if (ee.is_composite())
				os << 'C';
			if (ee.is_forward())
				os << 'F';
			if (ee.is_backward())
				os << 'B';
			if (ee.is_noroute())
				os << '-';
			os << ") D" << ossd.str()
			   << " T" << osstt.str();
		}
		if (ee.is_solution())
			os << " S" << ee.get_solution();
		const unsigned int pis(ee.get_levels());
		if (pis)
			os << " |";
		for (unsigned int i = 0; i < pis; ++i) {
			os << ' ';
			if (ee.is_valid(i)) {
				os << ee.get_metric(i);
				continue;
			}
			os << "--";
		}
		os << std::endl;
	}
	return os;
}

namespace ADR {

Graph::TimeEdgeDistanceMap::value_type get(const Graph::TimeEdgeDistanceMap& m, Graph::TimeEdgeDistanceMap::key_type key)
{
	const GraphVertex& v0(m.get_graph()[boost::source(key, m.get_graph())]);
	const GraphVertex& v1(m.get_graph()[boost::target(key, m.get_graph())]);
	const PointIdentTimeSlice& ts0(v0.get_object()->operator()(m.get_time()).as_point());
	const PointIdentTimeSlice& ts1(v1.get_object()->operator()(m.get_time()).as_point());
	if (ts0.is_valid() && ts1.is_valid() && !ts0.get_coord().is_invalid() && !ts1.get_coord().is_invalid())
		return ts0.get_coord().spheric_distance_nmi_dbl(ts1.get_coord());
	return std::numeric_limits<double>::max();
}

#if 0
void put(const Graph::TimeEdgeDistanceMap& m, Graph::TimeEdgeDistanceMap::key_type key, const Graph::TimeEdgeDistanceMap::value_type& value)
{
}

Graph::TimeEdgeDistanceMap::reference at(const Graph::TimeEdgeDistanceMap& m, Graph::TimeEdgeDistanceMap::key_type key)
{
}
#endif

};
