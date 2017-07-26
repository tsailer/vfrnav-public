#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iomanip>
#include "adrrestriction.hh"
#include "adrhibernate.hh"
#include "SunriseSunset.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

using namespace ADR;

// FIXME: check handling of AWY restriction / condition segments wrt intermediate points

CondResult& CondResult::operator&=(const CondResult& x)
{
	{
		set_t r;
		if (m_xngedgeinv && !x.m_xngedgeinv) {
			std::set_difference(x.m_xngedgeset.begin(), x.m_xngedgeset.end(),
					    m_xngedgeset.begin(), m_xngedgeset.end(),
					    std::inserter(r, r.begin()));
			m_xngedgeinv = false;
		} else if (!m_xngedgeinv && x.m_xngedgeinv) {
			std::set_difference(m_xngedgeset.begin(), m_xngedgeset.end(),
					    x.m_xngedgeset.begin(), x.m_xngedgeset.end(),
					    std::inserter(r, r.begin()));
		} else {
			std::set_intersection(m_xngedgeset.begin(), m_xngedgeset.end(),
					      x.m_xngedgeset.begin(), x.m_xngedgeset.end(),
					      std::inserter(r, r.begin()));
		}
		r.swap(m_xngedgeset);
	}
	m_result = m_result && x.m_result;
	if (!m_result) {
		m_vertexset.clear();
		m_edgeset.clear();
		clear_refloc();
	} else {
		m_vertexset.insert(x.m_vertexset.begin(), x.m_vertexset.end());
		m_edgeset.insert(x.m_edgeset.begin(), x.m_edgeset.end());
		if (x.has_refloc())
			set_refloc(x.get_refloc());
	}
	return *this;
}

CondResult& CondResult::operator|=(const CondResult& x)
{
	if (m_xngedgeinv && !x.m_xngedgeinv) {
		m_xngedgeset = x.m_xngedgeset;
		m_xngedgeinv = false;
	} else if (!m_xngedgeinv && x.m_xngedgeinv) {
	} else {
		m_xngedgeset.insert(x.m_xngedgeset.begin(), x.m_xngedgeset.end());
	}
	// determine smaller set of conditions
	if (!m_result) {
		if (!x.m_result) {
			m_vertexset.clear();
			m_edgeset.clear();
			clear_refloc();
		} else {
			m_vertexset = x.m_vertexset;
			m_edgeset = x.m_edgeset;
			clear_refloc();
			set_refloc(x.get_refloc());
		}
	} else if (!x.m_result) {
		// keep
	} else {
		if (x.m_vertexset.size() < m_vertexset.size())
			m_vertexset = x.m_vertexset;
		if (x.m_edgeset.size() < m_edgeset.size())
			m_edgeset = x.m_edgeset;
		if (x.has_refloc())
			set_refloc(x.get_refloc());
	}
	m_result = m_result || x.m_result;
	return *this;
}

CondResult CondResult::operator~(void)
{
	CondResult r(!m_result);
	r.m_xngedgeinv = !r.m_xngedgeinv;
	return r;
}

unsigned int CondResult::get_first(void) const
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

unsigned int CondResult::get_last(void) const
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

unsigned int CondResult::get_seqorder(unsigned int min) const
{
	set_t::const_iterator vi(m_vertexset.lower_bound(min >> 1)), ve(m_vertexset.end());
	set_t::const_iterator ei(m_edgeset.lower_bound((min - 1U) >> 1)), ee(m_edgeset.end());
	while (vi != ve && *vi * 2U + 1U <= min)
		++vi;
	while (ei != ee && *ei * 2U + 2U <= min)
		++ei;
	if (vi == ve) {
		if (ei == ee)
			return 0U;
		return *ei * 2U + 2U;
	}
	if (ei == ee)
		return *vi * 2U + 1U;
	return std::min(*vi * 2U + 1U, *ei * 2U + 2U);
}

bool CondResult::set_refloc(unsigned int rl)
{
	if (rl == ~0U)
		return false;
	bool s(!has_refloc() || rl < m_refloc);
	if (s)
		m_refloc = rl;
	return s;
}

SegmentsListTimeSlice::SegmentsListTimeSlice(timetype_t starttime, timetype_t endtime, const segments_t& segs)
	: m_time(timepair_t(starttime, endtime)), m_segments(segs)
{
}

bool SegmentsListTimeSlice::is_overlap(timetype_t tm0, timetype_t tm1) const
{
	if (!is_valid() || !(tm0 < tm1))
		return false;
	return tm1 > get_starttime() && tm0 < get_endtime();
}

timetype_t SegmentsListTimeSlice::get_overlap(timetype_t tm0, timetype_t tm1) const
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

Rect SegmentsListTimeSlice::get_bbox(void) const
{
	Rect bbox(Point::invalid, Point::invalid);
	for (segments_t::const_iterator i(m_segments.begin()), e(m_segments.end()); i != e; ++i) {
		const SegmentTimeSlice& ts(i->get_obj()->operator()(get_starttime()).as_segment());
		if (!ts.is_valid())
			continue;
		const PointIdentTimeSlice& tsp0(ts.get_start().get_obj()->operator()(get_starttime()).as_point());
		if (tsp0.is_valid() && !tsp0.get_coord().is_invalid()) {
			if (bbox.get_southwest().is_invalid() || bbox.get_northeast().is_invalid())
				bbox = Rect(tsp0.get_coord(), tsp0.get_coord());
			else
				bbox = bbox.add(tsp0.get_coord());
		}
		const PointIdentTimeSlice& tsp1(ts.get_end().get_obj()->operator()(get_starttime()).as_point());
		if (tsp1.is_valid() && !tsp1.get_coord().is_invalid()) {
			if (bbox.get_southwest().is_invalid() || bbox.get_northeast().is_invalid())
				bbox = Rect(tsp1.get_coord(), tsp1.get_coord());
			else
				bbox = bbox.add(tsp1.get_coord());
		}
	}
	return bbox;
}

void SegmentsListTimeSlice::recompute(Database& db, const Link& start, const Link& end, const Link& awy)
{
	m_segments.clear();
	set_endtime(get_starttime());
	if (awy.is_nil() || start.is_nil() || end.is_nil())
		return;
	Link lawy(awy);
	Link lstart(start);
	Link lend(end);
	lawy.load(db);
	lstart.load(db);
	lend.load(db);
	if (!lawy.get_obj() || !lstart.get_obj() || !lend.get_obj())
		return;
	lawy.get_obj()->link(db, ~0U);
	lstart.get_obj()->link(db, ~0U);
	lend.get_obj()->link(db, ~0U);
	Graph g;
	g.add(get_starttime(), lawy.get_obj());
	g.add(get_starttime(), lstart.get_obj());
	g.add(get_starttime(), lend.get_obj());
	Database::findresults_t r(db.find_dependson(lawy, Database::loadmode_link, get_starttime(), get_starttime() + 1,
						    Object::type_pointstart, Object::type_lineend, 0));
	for (Database::findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri)
		g.add(get_starttime(), ri->get_obj());
        Graph::vertex_descriptor v[2];
	{
		bool ok;
		boost::tie(v[0], ok) = g.find_vertex(lstart);
		if (!ok) {
			if (true)
				std::cerr << "SegmentsListTimeSlice::recompute: start point " << lstart << " not found for "
					  << Glib::TimeVal(get_starttime(), 0).as_iso8601() << std::endl;
			return;
		}
	}
	{
		bool ok;
		boost::tie(v[1], ok) = g.find_vertex(lend);
		if (!ok) {
			if (true)
				std::cerr << "SegmentsListTimeSlice::recompute: end point " << lend << " not found for "
					  << Glib::TimeVal(get_starttime(), 0).as_iso8601() << std::endl;
			return;
		}
	}
	Graph::UUIDEdgeFilter filter(g, lawy, GraphEdge::matchcomp_segment);
	typedef boost::filtered_graph<Graph, Graph::UUIDEdgeFilter> fg_t;
	fg_t fg(g, filter);
	std::vector<Graph::vertex_descriptor> predecessors(boost::num_vertices(fg), 0);
	Graph::TimeEdgeDistanceMap wmap(g, get_starttime());
	dijkstra_shortest_paths(fg, v[0], boost::weight_map(wmap).predecessor_map(&predecessors[0]));
	for (Graph::vertex_descriptor ve(v[1]);;) {
		if (ve == v[0])
			break;
		Graph::vertex_descriptor vb(predecessors[ve]);
		if (vb == ve) {
			m_segments.clear();
			if (true)
				std::cerr << "SegmentsListTimeSlice::recompute: path " << lstart << " -> " << lend
					  << " airway " << lawy << " not found for "
					  << Glib::TimeVal(get_starttime(), 0).as_iso8601() << std::endl;
			return;
		}
		Graph::out_edge_iterator e0, e1;
		for (boost::tie(e0, e1) = boost::out_edges(vb, g); e0 != e1; ++e0) {
			if (boost::target(*e0, g) != ve)
				continue;
			const GraphEdge& ee(g[*e0]);
			if (!ee.is_match(lawy))
				continue;
			m_segments.insert(m_segments.begin(), ee.get_routesegmentlink());
			break;
		}
		if (e0 == e1) {
			m_segments.clear();
			if (true)
				std::cerr << "SegmentsListTimeSlice::recompute: segment " << g[vb].get_uuid() << " -> " << g[ve].get_uuid()
					  << " airway " << lawy << " not found for "
					  << Glib::TimeVal(get_starttime(), 0).as_iso8601() << std::endl;
			return;
		}
		ve = vb;
	}
	if (m_segments.empty())
		return;
	set_endtime(std::numeric_limits<timetype_t>::max());
	for (segments_t::const_iterator i(m_segments.begin()), e(m_segments.end()); i != e; ++i) {
		const SegmentTimeSlice& ts(i->get_obj()->operator()(get_starttime()).as_segment());
		if (!ts.is_valid()) {
			std::cerr << "SegmentsListTimeSlice::recompute: cannot get timeslice of segment " << (*i)
				  << " for " << Glib::TimeVal(get_starttime(), 0).as_iso8601() << std::endl;
			m_segments.clear();
			set_endtime(get_starttime());
			return;
		}
		set_endtime(std::min(get_endtime(), ts.get_endtime()));
		const PointIdentTimeSlice& tss(ts.get_start().get_obj()->operator()(get_starttime()).as_point());
		const PointIdentTimeSlice& tse(ts.get_end().get_obj()->operator()(get_starttime()).as_point());
		if (!tss.is_valid() || !tse.is_valid()) {
			std::cerr << "SegmentsListTimeSlice::recompute: cannot get start/endpoint timeslice of segment " << (*i)
				  << " for " << Glib::TimeVal(get_starttime(), 0).as_iso8601() << std::endl;
			m_segments.clear();
			set_endtime(get_starttime());
			return;
		}
		set_endtime(std::min(get_endtime(), std::min(tss.get_endtime(), tse.get_endtime())));
	}
}

int SegmentsListTimeSlice::compare(const SegmentsListTimeSlice& x) const
{
	if (get_starttime() < x.get_starttime())
		return -1;
	if (x.get_starttime() < get_starttime())
		return 1;
	if (get_endtime() < x.get_endtime())
		return -1;
	if (x.get_endtime() < get_endtime())
		return 1;
	return 0;
}

int SegmentsListTimeSlice::compare_all(const SegmentsListTimeSlice& x) const
{
	{
		int c(compare(x));
		if (c)
			return c;
	}
	if (m_segments.size() < x.m_segments.size())
		return -1;
	if (x.m_segments.size() < m_segments.size())
		return 1;
	for (segments_t::size_type i(0), n(m_segments.size()); i < n; ++i) {
		int c(m_segments[i].compare(x.m_segments[i]));
		if (c)
			return c;
	}
	return 0;
}

std::ostream& SegmentsListTimeSlice::print(std::ostream& os, unsigned int indent) const
{
        os << std::endl << std::string(indent, ' ') << "Segment List "
	   << Glib::TimeVal(get_starttime(), 0).as_iso8601() << ' '
	   << Glib::TimeVal(get_endtime(), 0).as_iso8601();
	for (segments_t::const_iterator i(m_segments.begin()), e(m_segments.end()); i != e; ++i) {
		os << std::endl << std::string(indent + 2, ' ') << (*i) << ' ' << (i->is_backward() ? "backward" : "forward");
		const SegmentTimeSlice& ts(i->get_obj()->operator()(get_starttime()).as_segment());
		if (!ts.is_valid())
			continue;
		const IdentTimeSlice& tsr(ts.get_route().get_obj()->operator()(get_starttime()).as_ident());
		const PointIdentTimeSlice& tss(ts.get_start().get_obj()->operator()(get_starttime()).as_point());
		const PointIdentTimeSlice& tse(ts.get_end().get_obj()->operator()(get_starttime()).as_point());
		if (tsr.is_valid() && tss.is_valid() && tse.is_valid()) {
			if (i->is_backward())
				os << ' ' << tse.get_ident() << ' ' << tsr.get_ident() << ' ' << tss.get_ident();
			else
				os << ' ' << tss.get_ident() << ' ' << tsr.get_ident() << ' ' << tse.get_ident();
		}
	}
	return os;
}

int SegmentsList::compare(const SegmentsList& x) const
{
	for (const_iterator i0(begin()), e0(end()), i1(x.begin()), e1(x.end());; ++i0, ++i1) {
		if (i0 == e0)
			return (i1 == e1) ? 0 : -1;
		if (i1 == e1)
			return 1;
		int c(i0->compare_all(*i1));
		if (c)
			return c;
	}
}

Rect SegmentsList::get_bbox(const timepair_t& tm) const
{
	Rect bbox(Point::invalid, Point::invalid);
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		if (!i->is_overlap(tm))
			continue;
		if (bbox.get_southwest().is_invalid() || bbox.get_northeast().is_invalid())
			bbox = i->get_bbox();
		else
			bbox = bbox.add(i->get_bbox());
	}
	return bbox;
}

void SegmentsList::recompute(Database& db, const timepair_t& tm, const Link& start, const Link& end, const Link& awy)
{
	clear();
	for (timetype_t t(tm.first); t < tm.second; ) {
		SegmentsListTimeSlice seg(t, tm.second);
		seg.recompute(db, start, end, awy);
		if (!seg.is_valid()) {
			clear();
			return;
		}
		t = seg.get_endtime();
		insert(seg);
	}
}

std::ostream& SegmentsList::print(std::ostream& os, unsigned int indent) const
{
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		i->print(os, indent);
        return os;
}

RestrictionElement::RestrictionElement(const AltRange& alt)
	: m_refcount(1), m_alt(alt)
{
}

RestrictionElement::~RestrictionElement()
{
}

void RestrictionElement::reference(void) const
{
        g_atomic_int_inc(&m_refcount);
}

void RestrictionElement::unreference(void) const
{
        if (!g_atomic_int_dec_and_test(&m_refcount))
                return;
        delete this;
}

bool RestrictionElement::is_altrange(int32_t minalt, int32_t maxalt, const timepair_t& tm) const
{
	return get_altrange().is_overlap(minalt, maxalt);
}

CondResult RestrictionElement::evaluate_trace(RestrictionEval& tfrs, unsigned int altcnt, unsigned int seqcnt) const
{
	CondResult r(evaluate(tfrs));
	std::ostringstream oss;
	oss << "Restriction " << altcnt << '.' << seqcnt << " result " << (r.get_result() ? 'T' : (!r.get_result() ? 'F' : 'I'));
	Message msg(oss.str(), Message::type_tracetfe);
	msg.m_vertexset = r.get_vertexset();
	msg.m_edgeset = r.get_edgeset();
	tfrs.message(msg);
	return r;
}

BidirAltRange RestrictionElement::evaluate_dct(DctParameters::Calc& dct) const
{
	BidirAltRange x;
	x.set_empty();
	return x;
}

BidirAltRange RestrictionElement::evaluate_dct_trace(DctParameters::Calc& dct, unsigned int altcnt, unsigned int seqcnt) const
{
	BidirAltRange r(evaluate_dct(dct));
	std::ostringstream oss;
	oss << "Restriction " << altcnt << '.' << seqcnt << " result " << r[0].to_str() << " / " << r[1].to_str();
	Message msg(oss.str(), Message::type_tracetfe);
	dct.message(msg);
	return r;
}

void RestrictionElement::get_dct_segments(DctSegments& seg) const
{
}

std::string RestrictionElement::to_str(timetype_t tm) const
{
	return m_alt.to_str();
}

std::string RestrictionElement::to_shortstr(timetype_t tm) const
{
	return m_alt.to_shortstr();
}

RestrictionElement::ptr_t RestrictionElement::create(type_t t)
{
	switch (t) {
	case type_route:
	{
		ptr_t p(new RestrictionElementRoute());
		return p;
	}

	case type_point:
	{
		ptr_t p(new RestrictionElementPoint());
		return p;
	}

	case type_sidstar:
	{
		ptr_t p(new RestrictionElementSidStar());
		return p;
	}

	case type_airspace:
	{
		ptr_t p(new RestrictionElementAirspace());
		return p;
	}

	case type_invalid:
	default:
		return ptr_t();

	}
}

RestrictionElement::ptr_t RestrictionElement::create(ArchiveReadStream& ar)
{
	type_t t;
	ar.iouint8(t);
	ptr_t p(create(t));
	if (p)
		p->io(ar);
	return p;
}

RestrictionElement::ptr_t RestrictionElement::create(ArchiveReadBuffer& ar)
{
	type_t t;
	ar.iouint8(t);
	ptr_t p(create(t));
	if (p)
		p->io(ar);
	return p;
}

RestrictionElement::ptr_t RestrictionElement::create(ArchiveWriteStream& ar)
{
	return ptr_t();
}

RestrictionElement::ptr_t RestrictionElement::create(LinkLoader& ar)
{
	return ptr_t();
}

RestrictionElement::ptr_t RestrictionElement::create(DependencyGenerator<UUID::set_t>& ar)
{
	return ptr_t();
}

RestrictionElement::ptr_t RestrictionElement::create(DependencyGenerator<Link::LinkSet>& ar)
{
	return ptr_t();
}

RestrictionElementRoute::RestrictionElementRoute(const AltRange& alt, const Link& pt0, const Link& pt1, const Link& rte, const SegmentsList& segs)
	: RestrictionElement(alt), m_route(rte), m_segments(segs)
{
	m_point[0] = pt0;
	m_point[1] = pt1;
}

RestrictionElementRoute::ptr_t RestrictionElementRoute::clone(void) const
{
	return ptr_t(new RestrictionElementRoute(m_alt, m_point[0], m_point[1], m_route, m_segments));
}

RestrictionElementRoute::ptr_t RestrictionElementRoute::recompute(Database& db, const timepair_t& tm) const
{
	if (is_valid_dct()) {
		if (m_segments.empty())
			return ptr_t();
		return ptr_t(new RestrictionElementRoute(m_alt, m_point[0], m_point[1], m_route, SegmentsList()));
	}
	Glib::RefPtr<RestrictionElementRoute> p(new RestrictionElementRoute(m_alt, m_point[0], m_point[1], m_route, SegmentsList()));
	p->m_segments.recompute(db, tm, m_point[0], m_point[1], m_route);
	if (!p->m_segments.compare(m_segments))
		return ptr_t();
	return p;
}

void RestrictionElementRoute::add_bbox(Rect& bbox, const TimeSlice& ts) const
{
	for (unsigned int i = 0; i < 2; ++i) {
		const Object::const_ptr_t& pt(m_point[i].get_obj());
		for (unsigned int j(0), n(pt->size()); j < n; ++j) {
			const TimeSlice& ts1(pt->operator[](j));
			if (!ts1.is_overlap(ts))
				continue;
			const PointIdentTimeSlice& tsp(ts1.as_point());
			if (!tsp.is_valid())
				continue;
			if (tsp.get_coord().is_invalid())
				continue;
			if (bbox.get_southwest().is_invalid() || bbox.get_northeast().is_invalid())
				bbox = Rect(tsp.get_coord(), tsp.get_coord());
			else
				bbox = bbox.add(tsp.get_coord());
		}
	}
	{
		Rect bbox1(m_segments.get_bbox(ts.get_timeinterval()));
		if (!bbox1.get_southwest().is_invalid() && !bbox1.get_northeast().is_invalid()) {
			if (bbox.get_southwest().is_invalid() || bbox.get_northeast().is_invalid())
				bbox = bbox1;
			else
				bbox = bbox.add(bbox1);
		}
	}
}

bool RestrictionElementRoute::is_bbox(const Rect& bbox, const timepair_t& tm) const
{
	for (unsigned int i0(0), n0(m_point[0].get_obj()->size()); i0 < n0; ++i0) {
		const PointIdentTimeSlice& ts0(m_point[0].get_obj()->operator[](i0).as_point());
		if (!ts0.is_valid() || !ts0.is_overlap(tm))
			continue;
		if (bbox.is_inside(ts0.get_coord()))
			return true;
		for (unsigned int i1(0), n1(m_point[1].get_obj()->size()); i1 < n1; ++i1) {
			const PointIdentTimeSlice& ts1(m_point[1].get_obj()->operator[](i1).as_point());
			if (!ts1.is_valid() || !ts1.is_overlap(tm))
				continue;
			if (bbox.is_inside(ts1.get_coord()))
				return true;
			if (bbox.is_intersect(ts0.get_coord(), ts1.get_coord()))
				return true;
		}
	}
	{
		Rect bbox1(m_segments.get_bbox(tm));
		if (!bbox1.get_southwest().is_invalid() && !bbox1.get_northeast().is_invalid()) {
			if (bbox.is_intersect(bbox1))
				return true;
		}
	}
	return false;
}

CondResult RestrictionElementRoute::evaluate(RestrictionEval& tfrs) const
{
	if (is_valid_dct()) {
		CondResult r(false);
		for (unsigned int wptnr = 1U, n = tfrs.wpt_size(); wptnr < n; ++wptnr) {
			const RestrictionEval::Waypoint& wpt0(tfrs.wpt(wptnr - 1U));
			const RestrictionEval::Waypoint& wpt1(tfrs.wpt(wptnr));
			if (!wpt0.is_ifr() || !get_altrange().is_inside(wpt0.get_altitude()) ||
			    (wpt0.get_pathcode() != FPlanWaypoint::pathcode_directto && !wpt0.is_stay() &&
			     (wpt0.get_pathcode() != FPlanWaypoint::pathcode_none ||
			      (wptnr > 1U && wptnr + 1U < n))))
				continue;
			if (wpt0.get_ptobj()->get_uuid() == m_point[0] &&
			    wpt1.get_ptobj()->get_uuid() == m_point[1]) {
				r.set_result(true);
				r.get_edgeset().insert(wptnr - 1U);
			}
		}
		return r;
	}
	CondResult r(false);
	for (unsigned int wptnr = 1U, n = tfrs.wpt_size(); wptnr < n; ++wptnr) {
		const RestrictionEval::Waypoint& wpt0(tfrs.wpt(wptnr - 1U));
		if (!wpt0.is_ifr() || !get_altrange().is_inside(wpt0.get_altitude()))
			continue;
		if (wpt0.get_ptobj()->get_uuid() != m_point[0])
			continue;
		CondResult r1(true);
		unsigned int wptnre(wptnr);
		for (; wptnre < n; ++wptnre) {
			const RestrictionEval::Waypoint& wpt(tfrs.wpt(wptnre - 1U));
			const RestrictionEval::Waypoint& wpte(tfrs.wpt(wptnre));
			if (!wpt.is_ifr()) {
				r1.set_result(false);
				break;
			}
			if (!wpt.is_path_match(m_route)) {
				r1.set_result(false);
				break;
			}
			r1.get_edgeset().insert(wptnre - 1U);
			if (wpte.get_ptobj()->get_uuid() == m_point[1])
				break;
		}
		if (r1.get_result() && wptnre < n) {
			r.set_result(true);
			r.get_edgeset().insert(r1.get_edgeset().begin(), r1.get_edgeset().end());
			if (r.has_refloc())
				r.set_refloc(r1.get_refloc());
                }
	}
	return r;
}

RuleSegment RestrictionElementRoute::get_rule(void) const
{
	return RuleSegment(m_route.is_nil() ? RuleSegment::type_dct : RuleSegment::type_airway,
			   m_alt, m_point[0].get_obj(), m_point[1].get_obj(), m_route.get_obj(), m_segments);
}

BidirAltRange RestrictionElementRoute::evaluate_dct(DctParameters::Calc& dct) const
{
	BidirAltRange x;
	x.set(m_alt);
	if (!is_valid_dct()) {
		x.set_empty();
		return x;
	}
	for (unsigned int i = 0; i < 2; ++i) {
		if ((UUID)m_point[i] != dct.get_point(0)->get_uuid() ||
		    (UUID)m_point[!i] != dct.get_point(1)->get_uuid())
			x[i].set_empty();
	}
	return x;
}

void RestrictionElementRoute::get_dct_segments(DctSegments& seg) const
{
	if (!is_valid_dct())
		return;
	if (!m_point[0].get_obj() || !m_point[1].get_obj())
		return;
	seg.add(DctSegments::DctSegment(m_point[0].get_obj(), m_point[1].get_obj()));
}

bool RestrictionElementRoute::is_deparr_dct(Link& arpt, bool& arr, dctconnpoints_t& connpt) const
{
	if (!is_valid_dct())
		return false;
	if (m_point[!!arr] != arpt)
		return false;
	IntervalSet<int32_t> ar(m_alt.get_interval(true));
	if (ar.is_empty())
		return true;
	std::pair<dctconnpoints_t::iterator,bool> ins(connpt.insert(dctconnpoints_t::value_type(m_point[!arr], ar)));
	if (!ins.second)
		ins.first->second |= ar;
	return true;
}

std::ostream& RestrictionElementRoute::print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const
{
	os << std::endl << std::string(indent, ' ') << (is_valid_dct() ? "DCT" : "Airway") << ' ' << get_altrange().to_str()
	   << std::endl << std::string(indent + 2, ' ') << "Start " << (UUID)m_point[0];
	{
		const PointIdentTimeSlice& tsp(m_point[0].get_obj()->operator()(ts).as_point());
		if (tsp.is_valid())
			os << ' ' << tsp.get_ident();
	}
	os << std::endl << std::string(indent + 2, ' ') << "End   " << (UUID)m_point[1];
	{
		const PointIdentTimeSlice& tsp(m_point[1].get_obj()->operator()(ts).as_point());
		if (tsp.is_valid())
			os << ' ' << tsp.get_ident();
	}
	if (is_valid_dct())
		return os;
	os << std::endl << std::string(indent + 2, ' ') << "Route " << (UUID)m_route;
	{
		const IdentTimeSlice& tsr(m_route.get_obj()->operator()(ts).as_ident());
		if (tsr.is_valid())
			os << ' ' << tsr.get_ident();
	}
	m_segments.print(os, indent + 2);
	return os;
}

std::string RestrictionElementRoute::to_str(timetype_t tm) const
{
	std::ostringstream oss;
	if (m_route.is_nil()) {
		oss << "DCT ";
	} else {
		const IdentTimeSlice& ts(m_route.get_obj()->operator()(tm).as_ident());
		if (ts.is_valid())
			oss << ts.get_ident() << ' ';
		else
			oss << "?""? ";
	}
	const PointIdentTimeSlice& ts0(m_point[0].get_obj()->operator()(tm).as_point());
	const PointIdentTimeSlice& ts1(m_point[1].get_obj()->operator()(tm).as_point());
	if (ts0.is_valid())
		oss << ts0.get_ident();
	else
		oss << "?""?";
	oss << ' ';
	if (ts1.is_valid())
		oss << ts1.get_ident();
	else
		oss << "?""?";
	oss << ' ' << RestrictionElement::to_str(tm);
	return oss.str();
}

std::string RestrictionElementRoute::to_shortstr(timetype_t tm) const
{
	std::ostringstream oss;
	if (m_route.is_nil()) {
		oss << "DCT ";
	} else {
		oss << "AWY ";
		const IdentTimeSlice& ts(m_route.get_obj()->operator()(tm).as_ident());
		if (ts.is_valid())
			oss << ts.get_ident() << ' ';
		else
			oss << "?""? ";
	}
	{
		std::string x(RestrictionElement::to_shortstr(tm));
		if (!x.empty())
			oss << x << ' ';
	}
	const PointIdentTimeSlice& ts0(m_point[0].get_obj()->operator()(tm).as_point());
	const PointIdentTimeSlice& ts1(m_point[1].get_obj()->operator()(tm).as_point());
	if (ts0.is_valid())
		oss << ts0.get_ident();
	else
		oss << "?""?";
	oss << ' ';
	if (ts1.is_valid())
		oss << ts1.get_ident();
	else
		oss << "?""?";
	return oss.str();
}

void RestrictionElementRoute::io(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void RestrictionElementRoute::io(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void RestrictionElementRoute::io(ArchiveWriteStream& ar) const
{
	const_cast<RestrictionElementRoute *>(this)->hibernate(ar);
}

void RestrictionElementRoute::io(LinkLoader& ar)
{
	hibernate(ar);
}

void RestrictionElementRoute::io(DependencyGenerator<UUID::set_t>& ar) const
{
	const_cast<RestrictionElementRoute *>(this)->hibernate(ar);
}

void RestrictionElementRoute::io(DependencyGenerator<Link::LinkSet>& ar) const
{
	const_cast<RestrictionElementRoute *>(this)->hibernate(ar);
}

RestrictionElementPoint::RestrictionElementPoint(const AltRange& alt, const Link& pt)
	: RestrictionElement(alt), m_point(pt)
{
}

RestrictionElementPoint::ptr_t RestrictionElementPoint::clone(void) const
{
	return ptr_t(new RestrictionElementPoint(m_alt, m_point));
}

void RestrictionElementPoint::add_bbox(Rect& bbox, const TimeSlice& ts) const
{
	const Object::const_ptr_t& pt(m_point.get_obj());
	for (unsigned int j(0), n(pt->size()); j < n; ++j) {
		const TimeSlice& ts1(pt->operator[](j));
		if (!ts1.is_overlap(ts))
			continue;
		const PointIdentTimeSlice& tsp(ts1.as_point());
		if (!tsp.is_valid())
			continue;
		if (tsp.get_coord().is_invalid())
			continue;
		if (bbox.get_southwest().is_invalid() || bbox.get_northeast().is_invalid())
			bbox = Rect(tsp.get_coord(), tsp.get_coord());
		else
			bbox = bbox.add(tsp.get_coord());
	}
}

bool RestrictionElementPoint::is_bbox(const Rect& bbox, const timepair_t& tm) const
{
	for (unsigned int i(0), n(m_point.get_obj()->size()); i < n; ++i) {
		const PointIdentTimeSlice& tsp(m_point.get_obj()->operator[](i).as_point());
		if (!tsp.is_valid() || !tsp.is_overlap(tm))
			continue;
		if (bbox.is_inside(tsp.get_coord()))
			return true;
	}
	return false;
}

CondResult RestrictionElementPoint::evaluate(RestrictionEval& tfrs) const
{
	CondResult r(false);
	for (unsigned int wptnr = 0U, n = tfrs.wpt_size(); wptnr < n; ++wptnr) {
		const RestrictionEval::Waypoint& wpt(tfrs.wpt(wptnr));
		if (!wpt.is_ifr() && (!wptnr || !tfrs.wpt(wptnr - 1U).is_ifr()))
			continue;
		// do not match points within a procedure
		// actually EH5500A only works if we match points within a procedure
		if (false && wptnr && (wpt.get_pathcode() == FPlanWaypoint::pathcode_sid ||
				       wpt.get_pathcode() == FPlanWaypoint::pathcode_star)) {
			const RestrictionEval::Waypoint& wptp(tfrs.wpt(wptnr - 1U));
			if (wptp.get_pathcode() == FPlanWaypoint::pathcode_sid ||
			    wptp.get_pathcode() == FPlanWaypoint::pathcode_star)
				continue;
		}
		if (!get_altrange().is_inside(wpt.get_altitude()) || wpt.get_ptobj()->get_uuid() != m_point)
			continue;
		r.set_result(true);
		r.get_vertexset().insert(wptnr);
	}
	return r;
}

RuleSegment RestrictionElementPoint::get_rule(void) const
{
	return RuleSegment(RuleSegment::type_point, m_alt, m_point.get_obj());
}

BidirAltRange RestrictionElementPoint::evaluate_dct(DctParameters::Calc& dct) const
{
	BidirAltRange x;
	x.set_empty();
	for (unsigned int i = 0; i < 2; ++i) {
		if ((UUID)m_point == dct.get_point(i)->get_uuid())
			x.set(m_alt);
	}
	return x;
}

std::ostream& RestrictionElementPoint::print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const
{
	os << std::endl << std::string(indent, ' ') << "Point " << get_altrange().to_str()
	   << std::endl << std::string(indent + 2, ' ') << (UUID)m_point;
	{
		const PointIdentTimeSlice& tsp(m_point.get_obj()->operator()(ts).as_point());
		if (tsp.is_valid())
			os << ' ' << tsp.get_ident();
	}
	return os;
}

std::string RestrictionElementPoint::to_str(timetype_t tm) const
{
	std::ostringstream oss;
	const PointIdentTimeSlice& ts(m_point.get_obj()->operator()(tm).as_point());
	if (ts.is_valid())
		oss << ts.get_ident();
	else
		oss << "?""?";
	oss << ' ' << RestrictionElement::to_str(tm);
	return oss.str();
}

std::string RestrictionElementPoint::to_shortstr(timetype_t tm) const
{
	std::ostringstream oss;
	oss << "POINT ";
	{
		std::string x(RestrictionElement::to_shortstr(tm));
		if (!x.empty())
			oss << x << ' ';
	}
	const PointIdentTimeSlice& ts(m_point.get_obj()->operator()(tm).as_point());
	if (ts.is_valid())
		oss << ts.get_ident();
	else
		oss << "?""?";
	return oss.str();
}

void RestrictionElementPoint::io(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void RestrictionElementPoint::io(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void RestrictionElementPoint::io(ArchiveWriteStream& ar) const
{
	const_cast<RestrictionElementPoint *>(this)->hibernate(ar);
}

void RestrictionElementPoint::io(LinkLoader& ar)
{
	hibernate(ar);
}

void RestrictionElementPoint::io(DependencyGenerator<UUID::set_t>& ar) const
{
	const_cast<RestrictionElementPoint *>(this)->hibernate(ar);
}

void RestrictionElementPoint::io(DependencyGenerator<Link::LinkSet>& ar) const
{
	const_cast<RestrictionElementPoint *>(this)->hibernate(ar);
}

RestrictionElementSidStar::RestrictionElementSidStar(const AltRange& alt, const Link& proc, bool star)
	: RestrictionElement(alt), m_proc(proc), m_star(star)
{
}

RestrictionElementSidStar::ptr_t RestrictionElementSidStar::clone(void) const
{
	return ptr_t(new RestrictionElementSidStar(m_alt, m_proc, m_star));
}

void RestrictionElementSidStar::add_bbox(Rect& bbox, const TimeSlice& ts) const
{
	const Object::const_ptr_t& instr(m_proc.get_obj());
	for (unsigned int i(0), n(instr->size()); i < n; ++i) {
		const TimeSlice& tsinstr1(instr->operator[](i));
		if (!tsinstr1.is_overlap(ts))
			continue;
		const StandardInstrumentTimeSlice& tsinstr(tsinstr1.as_sidstar());
		if (!tsinstr.is_valid())
			continue;
		const Object::const_ptr_t& arpt(tsinstr.get_airport().get_obj());
		for (unsigned int j(0), n(arpt->size()); j < n; ++j) {
			const TimeSlice& tsarpt1(arpt->operator[](j));
			if (!tsarpt1.is_overlap(ts))
				continue;
			const PointIdentTimeSlice& tsarpt(tsarpt1.as_point());
			if (!tsarpt.is_valid())
				continue;
			if (tsarpt.get_coord().is_invalid())
				continue;
			if (bbox.get_southwest().is_invalid() || bbox.get_northeast().is_invalid())
				bbox = Rect(tsarpt.get_coord(), tsarpt.get_coord());
			else
				bbox = bbox.add(tsarpt.get_coord());
		}
	}
}

bool RestrictionElementSidStar::is_bbox(const Rect& bbox, const timepair_t& tm) const
{
	for (unsigned int i0(0), n0(m_proc.get_obj()->size()); i0 < n0; ++i0) {
		const StandardInstrumentTimeSlice& tsp(m_proc.get_obj()->operator[](i0).as_sidstar());
		if (!tsp.is_valid() || !tsp.is_overlap(tm))
			continue;
		for (unsigned int i1(0), n1(tsp.get_airport().get_obj()->size()); i1 < n1; ++i1) {
			const AirportTimeSlice& tsa(tsp.get_airport().get_obj()->operator[](i1).as_airport());
			if (!tsa.is_valid() || !tsa.is_overlap(tm) || !tsa.is_overlap(tsp))
				continue;
			if (bbox.is_inside(tsa.get_coord()))
				return true;
		}
	}
	return false;
}

CondResult RestrictionElementSidStar::evaluate(RestrictionEval& tfrs) const
{
	CondResult r(false);
	for (unsigned int wptnr = 1U, n = tfrs.wpt_size(); wptnr < n; ++wptnr) {
		const RestrictionEval::Waypoint& wpt(tfrs.wpt(wptnr - 1));
		if (!wpt.is_ifr() || !get_altrange().is_inside(wpt.get_altitude()))
			continue;
		if (!wpt.is_path_match(m_proc))
			continue;
		r.set_result(true);
		r.get_edgeset().insert(wptnr - 1);
	}
	return r;
}

RuleSegment RestrictionElementSidStar::get_rule(void) const
{
	return RuleSegment(m_star ? RuleSegment::type_star : RuleSegment::type_sid,
			   m_alt, Object::const_ptr_t(), Object::const_ptr_t(), m_proc.get_obj());
}

std::ostream& RestrictionElementSidStar::print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const
{
	os << std::endl << std::string(indent, ' ') << (m_star ? "STAR" : "SID") << ' ' << get_altrange().to_str()
	   << std::endl << std::string(indent + 2, ' ') << (UUID)m_proc;
	{
		const StandardInstrumentTimeSlice& tsp(m_proc.get_obj()->operator()(ts).as_sidstar());
		if (tsp.is_valid()) {
			os << ' ' << tsp.get_ident() << std::endl << std::string(indent + 2, ' ') << (UUID)tsp.get_airport();
			const PointIdentTimeSlice& tsa(tsp.get_airport().get_obj()->operator()(ts).as_point());
			if (tsa.is_valid())
				os << ' ' << tsa.get_ident();
		}
	}
	return os;
}

std::string RestrictionElementSidStar::to_str(timetype_t tm) const
{
	std::ostringstream oss;
	oss << (m_star ? "STAR " : "SID ");
	const StandardInstrumentTimeSlice& ts(m_proc.get_obj()->operator()(tm).as_sidstar());
	if (!ts.is_valid()) {
		oss << "?""? ?""? " << RestrictionElement::to_str(tm);
		return oss.str();
	}
	oss << ts.get_ident() << ' ';
	const PointIdentTimeSlice& tsarpt(ts.get_airport().get_obj()->operator()(tm).as_point());
	if (!tsarpt.is_valid()) {
		oss << "?""? " << RestrictionElement::to_str(tm);
		return oss.str();
	}
	oss << tsarpt.get_ident() << ' ' << RestrictionElement::to_str(tm);
	return oss.str();
}

std::string RestrictionElementSidStar::to_shortstr(timetype_t tm) const
{
	std::ostringstream oss;
	oss << (m_star ? "STAR " : "SID ");
	const StandardInstrumentTimeSlice& ts(m_proc.get_obj()->operator()(tm).as_sidstar());
	if (!ts.is_valid()) {
		oss << "?""? ?""? " << RestrictionElement::to_str(tm);
		return oss.str();
	}
	oss << ts.get_ident() << ' ';
	const PointIdentTimeSlice& tsarpt(ts.get_airport().get_obj()->operator()(tm).as_point());
	if (!tsarpt.is_valid()) {
		oss << "?""? " << RestrictionElement::to_str(tm);
		return oss.str();
	}
	oss << tsarpt.get_ident();
	{
		std::string x(RestrictionElement::to_shortstr(tm));
		if (!x.empty())
			oss << ' ' << x;
	}
	return oss.str();
}

void RestrictionElementSidStar::io(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void RestrictionElementSidStar::io(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void RestrictionElementSidStar::io(ArchiveWriteStream& ar) const
{
	const_cast<RestrictionElementSidStar *>(this)->hibernate(ar);
}

void RestrictionElementSidStar::io(LinkLoader& ar)
{
	hibernate(ar);
}

void RestrictionElementSidStar::io(DependencyGenerator<UUID::set_t>& ar) const
{
	const_cast<RestrictionElementSidStar *>(this)->hibernate(ar);
}

void RestrictionElementSidStar::io(DependencyGenerator<Link::LinkSet>& ar) const
{
	const_cast<RestrictionElementSidStar *>(this)->hibernate(ar);
}

RestrictionElementAirspace::RestrictionElementAirspace(const AltRange& alt, const Link& aspc)
	: RestrictionElement(alt), m_airspace(aspc)
{
}

RestrictionElementAirspace::ptr_t RestrictionElementAirspace::clone(void) const
{
	return ptr_t(new RestrictionElementAirspace(m_alt, m_airspace));
}

void RestrictionElementAirspace::add_bbox(Rect& bbox, const TimeSlice& ts) const
{
	const Object::const_ptr_t& aspc(get_airspace().get_obj());
	for (unsigned int i(0), n(aspc->size()); i < n; ++i) {
		const TimeSlice& tsaspc1(aspc->operator[](i));
		if (!tsaspc1.is_overlap(ts))
			continue;
		const AirspaceTimeSlice& tsaspc(tsaspc1.as_airspace());
		if (!tsaspc.is_valid())
			continue;
		Rect bboxa;
		tsaspc.get_bbox(bboxa);
		if (bboxa.get_southwest().is_invalid() || bboxa.get_northeast().is_invalid())
			continue;
		if (bbox.get_southwest().is_invalid() || bbox.get_northeast().is_invalid())
			bbox = bboxa;
		else
			bbox = bbox.add(bboxa);
	}
}

bool RestrictionElementAirspace::is_bbox(const Rect& bbox, const timepair_t& tm) const
{
	for (unsigned int i(0), n(get_airspace().get_obj()->size()); i < n; ++i) {
		const AirspaceTimeSlice& tsa(get_airspace().get_obj()->operator[](i).as_airspace());
		if (!tsa.is_valid() || !tsa.is_overlap(tm))
			continue;
		Rect bboxa;
		tsa.get_bbox(bboxa);
		if (bbox.is_intersect(bboxa))
			return true;
	}
	return false;
}

bool RestrictionElementAirspace::is_altrange(int32_t minalt, int32_t maxalt, const timepair_t& tm) const
{
	Airspace::ptr_t p(Airspace::ptr_t::cast_dynamic(m_airspace.get_obj()));
	if (!p)
		return false;
	return p->is_altitude_overlap(minalt, maxalt, tm, get_altrange());
}

CondResult RestrictionElementAirspace::evaluate(RestrictionEval& tfrs) const
{
	CondResult r(false);
	for (unsigned int wptnr = 0U, n = tfrs.wpt_size(); wptnr < n; ++wptnr) {
		const RestrictionEval::Waypoint& wpt(tfrs.wpt(wptnr));
		if (!wpt.is_ifr() && (!wptnr || !tfrs.wpt(wptnr - 1U).is_ifr()))
			continue;
		if (false && wptnr) {
			const RestrictionEval::Waypoint& wpt0(tfrs.wpt(wptnr - 1U));
			if (wpt0.get_pathcode() == FPlanWaypoint::pathcode_sid ||
			    wpt0.get_pathcode() == FPlanWaypoint::pathcode_star)
				continue;
		}
		const AirspaceTimeSlice& ts(get_airspace().get_obj()->operator()(wpt.get_time_unix()).as_airspace());
		if (!ts.is_valid())
			continue;
		{
			TimeTableEval tte(wpt.get_time_unix(), wpt.get_coord(), tfrs.get_specialdateeval());
			if (ts.is_inside(tte, wpt.get_altitude(), get_altrange(), wpt.get_ptuuid())) {
				r.set_result(true);
				r.get_vertexset().insert(wptnr);
				continue;
			}
		}
		if (!wptnr)
			continue;
		{
			const RestrictionEval::Waypoint& wpt0(tfrs.wpt(wptnr - 1U));
			TimeTableEval tte(wpt0.get_time_unix(), wpt0.get_coord(), tfrs.get_specialdateeval());
			if (ts.is_intersect(tte, wpt.get_coord(), wpt0.get_altitude(), get_altrange())) {
				r.set_result(true);
				r.get_edgeset().insert(wptnr - 1U);
				continue;
			}
		}
	}
	return r;
}

RuleSegment RestrictionElementAirspace::get_rule(void) const
{
	return RuleSegment(RuleSegment::type_airspace, m_alt, m_airspace.get_obj());
}

std::ostream& RestrictionElementAirspace::print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const
{
	os << std::endl << std::string(indent, ' ') << "Airspace " << get_altrange().to_str()
	   << std::endl << std::string(indent + 2, ' ') << (UUID)m_airspace;
	{
		const AirspaceTimeSlice& tsa(m_airspace.get_obj()->operator()(ts).as_airspace());
		if (tsa.is_valid())
			os << ' ' << tsa.get_ident();
	}
	return os;
}

std::string RestrictionElementAirspace::to_str(timetype_t tm) const
{
	std::ostringstream oss;
	const AirspaceTimeSlice& ts(get_airspace().get_obj()->operator()(tm).as_airspace());
	if (!ts.is_valid()) {
		oss << "?""? " << RestrictionElement::to_str(tm);
		return oss.str();
	}
	oss << ts.get_ident() << '/' << ts.get_type() << ' ' << RestrictionElement::to_str(tm);
	return oss.str();
}

std::string RestrictionElementAirspace::to_shortstr(timetype_t tm) const
{
	std::ostringstream oss;
	oss << "AIRSPACE ";
	const AirspaceTimeSlice& ts(get_airspace().get_obj()->operator()(tm).as_airspace());
	if (ts.is_valid())
		oss << ts.get_ident() << '/' << ts.get_type();
	else
		oss << "?""?";
	{
		std::string x(RestrictionElement::to_shortstr(tm));
		if (!x.empty())
			oss << ' ' << x;
	}
	return oss.str();
}

void RestrictionElementAirspace::io(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void RestrictionElementAirspace::io(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void RestrictionElementAirspace::io(ArchiveWriteStream& ar) const
{
	const_cast<RestrictionElementAirspace *>(this)->hibernate(ar);
}

void RestrictionElementAirspace::io(LinkLoader& ar)
{
	hibernate(ar);
}

void RestrictionElementAirspace::io(DependencyGenerator<UUID::set_t>& ar) const
{
	const_cast<RestrictionElementAirspace *>(this)->hibernate(ar);
}

void RestrictionElementAirspace::io(DependencyGenerator<Link::LinkSet>& ar) const
{
	const_cast<RestrictionElementAirspace *>(this)->hibernate(ar);
}

RestrictionSequence::RestrictionSequence(void)
{
}

void RestrictionSequence::add(RestrictionElement::ptr_t e)
{
	m_elements.push_back(e);
}

void RestrictionSequence::clone(void)
{
	for (elements_t::iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		RestrictionElement::ptr_t p(*i);
		if (!p)
			continue;
		p = p->clone();
		if (!p)
			continue;
		*i = p;
	}
}

bool RestrictionSequence::recompute(Database& db, const timepair_t& tm)
{
	bool work(false);
	for (elements_t::iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		RestrictionElement::ptr_t p(*i);
		if (!p)
			continue;
		p = p->recompute(db, tm);
		if (!p)
			continue;
		*i = p;
		work = true;
	}
	return work;
}

void RestrictionSequence::add_bbox(Rect& bbox, const TimeSlice& ts) const
{
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		RestrictionElement::const_ptr_t p(*i);
		if (!p)
			continue;
		p->add_bbox(bbox, ts);
	}
}

bool RestrictionSequence::is_bbox(const Rect& bbox, const timepair_t& tm) const
{
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
		if ((*i)->is_bbox(bbox, tm))
			return true;
	return false;
}

bool RestrictionSequence::is_altrange(int32_t minalt, int32_t maxalt, const timepair_t& tm) const
{
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
		if ((*i)->is_altrange(minalt, maxalt, tm))
			return true;
	return false;
}

CondResult RestrictionSequence::evaluate(RestrictionEval& tfrs) const
{
	CondResult r(true);
	bool first(true);
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		if (first) {
			r = (*i)->evaluate(tfrs);
			first = false;
		} else {
			CondResult r1((*i)->evaluate(tfrs));
			if (r.get_last() > r1.get_first())
				r = CondResult(false);
			r &= r1;
		}
	}
	return r;
}

CondResult RestrictionSequence::evaluate_trace(RestrictionEval& tfrs, unsigned int altcnt) const
{
	CondResult r(true);
	bool first(true);
	unsigned int seqcnt(0);
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i, ++seqcnt) {
		if (first) {
			r = (*i)->evaluate_trace(tfrs, altcnt, seqcnt);
			first = false;
		} else {
			CondResult r1((*i)->evaluate_trace(tfrs, altcnt, seqcnt));
			if (r.get_last() > r1.get_first())
				r = CondResult(false);
			r &= r1;
		}
	}
	std::ostringstream oss;
	oss << "Restriction " << altcnt << " result " << (r.get_result() ? 'T' : (!r.get_result() ? 'F' : 'I'));
	Message msg(oss.str(), Message::type_tracetfe);
	msg.m_vertexset = r.get_vertexset();
	msg.m_edgeset = r.get_edgeset();
	tfrs.message(msg);
	return r;
}

RuleSequence RestrictionSequence::get_rule(void) const
{
	RuleSequence seq;
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		RuleSegment seg((*i)->get_rule());
		if (seg.get_type() != RuleSegment::type_invalid)
			seq.push_back(seg);
	}
	return seq;
}

bool RestrictionSequence::is_valid_dct(void) const
{
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
		if (!(*i)->is_valid_dct())
			return false;
	return true;
}

BidirAltRange RestrictionSequence::evaluate_dct(DctParameters::Calc& dct) const
{
	BidirAltRange x;
	x.set_full();
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		x &= (*i)->evaluate_dct(dct);
		if (x.is_empty())
			break;
	}
	return x;
}

BidirAltRange RestrictionSequence::evaluate_dct_trace(DctParameters::Calc& dct, unsigned int altcnt) const
{
	BidirAltRange x;
	x.set_full();
	unsigned int seqcnt(0);
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i, ++seqcnt) {
		x &= (*i)->evaluate_dct_trace(dct, altcnt, seqcnt);
		if (x.is_empty())
			break;
	}
	std::ostringstream oss;
	oss << "Restriction " << altcnt << " result " << x[0].to_str() << " / " << x[1].to_str();
	Message msg(oss.str(), Message::type_tracetfe);
	dct.message(msg);
	return x;	
}

void RestrictionSequence::get_dct_segments(DctSegments& seg) const
{
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)	
		(*i)->get_dct_segments(seg);
}

bool RestrictionSequence::is_deparr_dct(Link& arpt, bool& arr, RestrictionElement::dctconnpoints_t& connpt) const
{
	elements_t::const_iterator i(m_elements.begin()), e(m_elements.end());
	if (i == e)
		return false;
	if (!(*i)->is_deparr_dct(arpt, arr, connpt))
		return false;
	++i;
	if (i != e)
		return false;
	return true;
}

std::ostream& RestrictionSequence::print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const
{
	if (m_elements.empty())
		return os;
	os << std::endl << std::string(indent, ' ') << "RestrictionSequence";
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
		(*i)->print(os, indent + 2, ts);
	return os;
}

std::string RestrictionSequence::to_str(timetype_t tm) const
{
	std::ostringstream oss;
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
		if (*i)
			oss << ' ' << (*i)->to_str(tm);
	return oss.str().substr(1);
}

std::string RestrictionSequence::to_shortstr(timetype_t tm) const
{
	std::ostringstream oss;
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
		if (*i)
			oss << ' ' << (*i)->to_shortstr(tm);
	return oss.str().substr(1);
}

void RestrictionSequence::io(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void RestrictionSequence::io(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void RestrictionSequence::io(ArchiveWriteStream& ar) const
{
	const_cast<RestrictionSequence *>(this)->hibernate(ar);
}

void RestrictionSequence::io(LinkLoader& ar)
{
	hibernate(ar);
}

void RestrictionSequence::io(DependencyGenerator<UUID::set_t>& ar) const
{
	const_cast<RestrictionSequence *>(this)->hibernate(ar);
}

void RestrictionSequence::io(DependencyGenerator<Link::LinkSet>& ar) const
{
	const_cast<RestrictionSequence *>(this)->hibernate(ar);
}

Restrictions::Restrictions(void)
{
}

void Restrictions::clone(void)
{
	for (elements_t::iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
		i->clone();
}

bool Restrictions::recompute(Database& db, const timepair_t& tm)
{
	bool work(false);
	for (elements_t::iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
		work = i->recompute(db, tm) || work;
	return work;
}

void Restrictions::add_bbox(Rect& bbox, const TimeSlice& ts) const
{
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
		i->add_bbox(bbox, ts);
}

bool Restrictions::simplify_bbox(const Rect& bbox, const timepair_t& tm)
{
	bool modified(false);
	for (elements_t::iterator i(m_elements.begin()), e(m_elements.end()); i != e; ) {
		if (i->is_bbox(bbox, tm)) {
			++i;
			continue;
		}
		i = m_elements.erase(i);
		e = m_elements.end();
		modified = true;
	}
	return modified;
}

bool Restrictions::simplify_altrange(int32_t minalt, int32_t maxalt, const timepair_t& tm)
{
	bool modified(false);
	for (elements_t::iterator i(m_elements.begin()), e(m_elements.end()); i != e; ) {
		if (i->is_altrange(minalt, maxalt, tm)) {
			++i;
			continue;
		}
		i = m_elements.erase(i);
		e = m_elements.end();
		modified = true;
	}
	return modified;
}

bool Restrictions::evaluate(RestrictionEval& tfrs, RestrictionResult& result, bool forbidden) const
{
	bool ruleok(forbidden);
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		CondResult r(i->evaluate(tfrs));
		result.push_back(RestrictionSequenceResult(i->get_rule(), r.get_vertexset(), r.get_edgeset()));
		if (forbidden)
			ruleok = ruleok && !r.get_result();
		else
			ruleok = ruleok || r.get_result();
	}
	return ruleok;
}

bool Restrictions::evaluate_trace(RestrictionEval& tfrs, RestrictionResult& result, bool forbidden) const
{
	bool ruleok(forbidden);
	unsigned int altcnt(0);
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i, ++altcnt) {
		CondResult r(i->evaluate_trace(tfrs, altcnt));
		result.push_back(RestrictionSequenceResult(i->get_rule(), r.get_vertexset(), r.get_edgeset()));
		if (forbidden)
			ruleok = ruleok && !r.get_result();
		else
			ruleok = ruleok || r.get_result();
	}
	return ruleok;
}

bool Restrictions::is_valid_dct(void) const
{
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
		if (!i->is_valid_dct())
			return false;
	return true;
}

BidirAltRange Restrictions::evaluate_dct(DctParameters::Calc& dct, bool forbidden) const
{
	BidirAltRange x;
	x.set_empty();
	if (forbidden)
		x.set_full();
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		BidirAltRange x1(i->evaluate_dct(dct));
		if (forbidden)
			x &= ~x1;
		else
			x |= x1;
	}
	return x;
}

BidirAltRange Restrictions::evaluate_dct_trace(DctParameters::Calc& dct, bool forbidden) const
{
	BidirAltRange x;
	x.set_empty();
	if (forbidden)
		x.set_full();
	unsigned int altcnt(0);
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i, ++altcnt) {
		BidirAltRange x1(i->evaluate_dct_trace(dct, altcnt));
		if (forbidden)
			x &= ~x1;
		else
			x |= x1;
	}
	std::ostringstream oss;
	oss << "Restriction result " << x[0].to_str() << " / " << x[1].to_str();
	Message msg(oss.str(), Message::type_tracetfe);
	dct.message(msg);
	return x;
}

void Restrictions::get_dct_segments(DctSegments& seg) const
{
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)	
		i->get_dct_segments(seg);
}

bool Restrictions::is_deparr_dct(Link& arpt, bool& arr, RestrictionElement::dctconnpoints_t& connpt) const
{
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)	
		if (!i->is_deparr_dct(arpt, arr, connpt))
			return false;
	return true;
}

std::ostream& Restrictions::print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const
{
	if (m_elements.empty())
		return os;
	os << std::endl << std::string(indent, ' ') << "Restrictions";
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
		i->print(os, indent + 2, ts);
	return os;
}

std::string Restrictions::to_str(timetype_t tm) const
{
	std::ostringstream oss;
	bool subseq(false);
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		if (subseq)
			oss << " / ";
		oss << i->to_str(tm);
		subseq = true;
	}
	return oss.str();
}

void Restrictions::io(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void Restrictions::io(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void Restrictions::io(ArchiveWriteStream& ar) const
{
	const_cast<Restrictions *>(this)->hibernate(ar);
}

void Restrictions::io(LinkLoader& ar)
{
	hibernate(ar);
}

void Restrictions::io(DependencyGenerator<UUID::set_t>& ar) const
{
	const_cast<Restrictions *>(this)->hibernate(ar);
}

void Restrictions::io(DependencyGenerator<Link::LinkSet>& ar) const
{
	const_cast<Restrictions *>(this)->hibernate(ar);
}

const bool Condition::ifr_refloc;

Condition::Condition(uint32_t childnum)
	: m_refcount(1), m_childnum(childnum)
{
}

Condition::~Condition()
{
}

void Condition::reference(void) const
{
        g_atomic_int_inc(&m_refcount);
}

void Condition::unreference(void) const
{
        if (!g_atomic_int_dec_and_test(&m_refcount))
                return;
        delete this;
}

Condition::ptr_t Condition::simplify(void) const
{
	return ptr_t();
}

Condition::ptr_t Condition::simplify_bbox(const Rect& bbox, const timepair_t& tm) const
{
	return ptr_t();
}

Condition::ptr_t Condition::simplify_altrange(int32_t minalt, int32_t maxalt, const timepair_t& tm) const
{
	return ptr_t();
}

Condition::ptr_t Condition::simplify_aircrafttype(const std::string& acfttype) const
{
	return ptr_t();
}

Condition::ptr_t Condition::simplify_aircraftclass(const std::string& acftclass) const
{
	return ptr_t();
}

Condition::ptr_t Condition::simplify_equipment(const std::string& equipment, Aircraft::pbn_t pbn) const
{
	return ptr_t();
}

Condition::ptr_t Condition::simplify_typeofflight(char type_of_flight) const
{
	return ptr_t();
}

Condition::ptr_t Condition::simplify_mil(bool mil) const
{
	return ptr_t();
}

Condition::ptr_t Condition::simplify_dep(const Link& arpt, const timepair_t& tm) const
{
	return ptr_t();
}

Condition::ptr_t Condition::simplify_dest(const Link& arpt, const timepair_t& tm) const
{
	return ptr_t();
}

Condition::ptr_t Condition::simplify_conditionalavailability(const Graph& g, const ConditionalAvailability& condavail,
							     const TimeTableSpecialDateEval& tsde, const timepair_t& tm) const
{
	return ptr_t();
}

std::string Condition::get_path(const Path *backptr) const
{
	std::string r;
	while (backptr) {
		std::ostringstream oss;
		if (backptr->get_cond())
			oss << backptr->get_cond()->get_childnum() << '.';
		else
			oss << "?.";
		r = oss.str() + r;
		backptr = backptr->get_back();
	}
	std::ostringstream oss;
	oss << r << get_childnum();
	return oss.str();
}

void Condition::trace(RestrictionEvalMessages& tfrs, const CondResult& r, const Path *backptr) const
{
	std::ostringstream oss;
	oss << "Condition " << get_path(backptr);
	if (r.has_refloc())
		oss << " RefLoc " << r.get_refloc();
	if (true && !r.get_xngedgeset().empty()) {
		oss << " Xng ";
		if (r.is_xngedgeinv())
			oss << '!';
		bool subseq(false);
		for (CondResult::set_t::const_iterator i(r.get_xngedgeset().begin()), e(r.get_xngedgeset().end()); i != e; ++i) {
			if (subseq)
				oss << ',';
			subseq = true;
			oss << *i;
		}
	}
	oss << " result " << (r.get_result() ? 'T' : (!r.get_result() ? 'F' : 'I'));
	Message msg(oss.str(), Message::type_tracetfe);
	msg.m_vertexset = r.get_vertexset();
	msg.m_edgeset = r.get_edgeset();
	tfrs.message(msg);
}

CondResult Condition::evaluate_trace(RestrictionEval& tfrs, const Path *backptr) const
{
	CondResult r(evaluate(tfrs));
	trace(tfrs, r, backptr);
	return r;
}

RuleSegment Condition::get_resultsequence(void) const
{
	return RuleSegment(RuleSegment::type_invalid);
}

void Condition::trace(RestrictionEvalMessages& tfrs, const BidirAltRange& r, const Path *backptr) const
{
	std::ostringstream oss;
	oss << "Condition " << get_path(backptr) << " result " << r[0].to_str() << '/' << r[1].to_str();
	Message msg(oss.str(), Message::type_tracetfe);
	tfrs.message(msg);
}

BidirAltRange Condition::evaluate_dct(DctParameters::Calc& dct) const
{
	BidirAltRange x;
	x.set_empty();
	return x;
}

BidirAltRange Condition::evaluate_dct_trace(DctParameters::Calc& dct, const Path *backptr) const
{
	BidirAltRange r(evaluate_dct(dct));
	trace(dct, r, backptr);
	return r;
}

std::vector<RuleSequence> Condition::get_mandatory(void) const
{
	return std::vector<RuleSequence>();
}

std::vector<RuleSequence> Condition::get_mandatory_seq(void) const
{
	return std::vector<RuleSequence>();
}

std::vector<RuleSegment> Condition::get_mandatory_or(void) const
{
	return std::vector<RuleSegment>();
}

Condition::ptr_t Condition::create(type_t t)
{
	switch (t) {
	case type_and:
	{
		ptr_t p(new ConditionAnd());
		return p;
	}

	case type_seq:
	{
		ptr_t p(new ConditionSequence());
		return p;
	}

	case type_constant:
	{
		ptr_t p(new ConditionConstant());
		return p;
	}

	case type_crossingairspace1:
	{
		ptr_t p(new ConditionCrossingAirspace1());
		return p;
	}

	case type_crossingairspace2:
	{
		ptr_t p(new ConditionCrossingAirspace2());
		return p;
	}

	case type_crossingdct:
	{
		ptr_t p(new ConditionCrossingDct());
		return p;
	}

	case type_crossingairway:
	{
		ptr_t p(new ConditionCrossingAirway());
		return p;
	}

	case type_crossingpoint:
	{
		ptr_t p(new ConditionCrossingPoint());
		return p;
	}

	case type_crossingdeparr:
	{
		ptr_t p(new ConditionDepArr());
		return p;
	}

	case type_crossingdeparrairspace:
	{
		ptr_t p(new ConditionDepArrAirspace());
		return p;
	}

	case type_crossingsidstar:
	{
		ptr_t p(new ConditionSidStar());
		return p;
	}

	case type_crossingairspaceactive:
	{
		ptr_t p(new ConditionCrossingAirspaceActive());
		return p;
	}

	case type_crossingairwayavailable:
	{
		ptr_t p(new ConditionCrossingAirwayAvailable());
		return p;
	}

	case type_dctlimit:
	{
		ptr_t p(new ConditionDctLimit());
		return p;
	}

	case type_aircraft:
	{
		ptr_t p(new ConditionAircraft());
		return p;
	}

	case type_flight:
	{
		ptr_t p(new ConditionFlight());
		return p;
	}

	case type_invalid:
	default:
		return ptr_t();

	}
}

Condition::ptr_t Condition::create(ArchiveReadStream& ar)
{
	type_t t;
	ar.iouint8(t);
	ptr_t p(create(t));
	if (false && !p) {
		std::ostringstream oss;
		oss << "Condition::create: invalid tag 0x" << std::hex << std::setfill('0') << std::setw(2) << (unsigned int)t;
		throw std::runtime_error(oss.str());
	}
	if (p)
		p->io(ar);
	return p;
}

Condition::ptr_t Condition::create(ArchiveReadBuffer& ar)
{
	type_t t;
	ar.iouint8(t);
	ptr_t p(create(t));
	if (false && !p) {
		std::ostringstream oss;
		oss << "Condition::create: invalid tag 0x" << std::hex << std::setfill('0') << std::setw(2) << (unsigned int)t;
		throw std::runtime_error(oss.str());
	}
	if (p)
		p->io(ar);
	return p;
}

Condition::ptr_t Condition::create(ArchiveWriteStream& ar)
{
	return ptr_t();
}

Condition::ptr_t Condition::create(LinkLoader& ar)
{
	return ptr_t();
}

Condition::ptr_t Condition::create(DependencyGenerator<UUID::set_t>& ar)
{
	return ptr_t();
}

Condition::ptr_t Condition::create(DependencyGenerator<Link::LinkSet>& ar)
{
	return ptr_t();
}

ConditionAnd::ConditionAnd(uint32_t childnum, bool resultinv)
	: Condition(childnum), m_inv(resultinv)
{
}

void ConditionAnd::add(const_ptr_t p, bool inv)
{
	m_cond.push_back(CondInv(p, inv));
}

bool ConditionAnd::is_refloc(void) const
{
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (!p)
			continue;
		if (p->is_refloc())
			return true;
	}
	return false;
}

bool ConditionAnd::is_constant(void) const
{
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (!p)
			continue;
		if (!p->is_constant())
			return false;
	}
	return true;
}

bool ConditionAnd::get_constvalue(void) const
{
	bool res(true);
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e && res; ++i) {
		const_ptr_t p(i->get_ptr());
		bool v(false);
		if (p)
			v = p->get_constvalue();
		if (i->is_inv())
			v = !v;
		res = res && v;
	}
	if (m_inv)
		res = !res;
	return res;
}

std::ostream& ConditionAnd::print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const
{
	os << std::endl << std::string(indent, ' ') << "Condition" << (m_inv ? "Or" : "And")<< '{' << get_childnum() << '}';
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		if (!m_inv ^ !i->is_inv()) {
			std::ostringstream oss;
			if (i->get_ptr())
				i->get_ptr()->print(oss, indent + 2, ts);
			else
				oss << " null";
			os << std::endl << std::string(indent + 2, ' ') << '!' << oss.str().substr(indent + 3);
			continue;
		}
		if (i->get_ptr()) {
			i->get_ptr()->print(os, indent + 2, ts);
			continue;
		}
		os << std::endl << std::string(indent + 2, ' ') << "null";
	}
	return os;
}

std::string ConditionAnd::to_str(timetype_t tm) const
{
	std::string r;
	{
		std::ostringstream oss;
		oss << (m_inv ? "Or" : "And") << '{' << get_childnum() << "}(";
		r = oss.str();
	}
	bool subseq(false);
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		if (subseq)
			r += ",";
		subseq = true;
		if (!m_inv ^ !i->is_inv())
			r += "!";
		const_ptr_t p(i->get_ptr());
		if (!p) {
			r += "null";
			continue;
		}
		r += p->to_str(tm);
	}
	r += ")";
	return r;
}

void ConditionAnd::add_bbox(Rect& bbox, const TimeSlice& ts) const
{
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (!p)
			continue;
		p->add_bbox(bbox, ts);
	}
}

ConditionAnd::ptr_t ConditionAnd::clone(void) const
{
	Glib::RefPtr<ConditionAnd> r(new ConditionAnd(get_childnum(), m_inv));
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (p)
			p = p->clone();
		r->add(p, i->is_inv());
	}
	return r;
}

ConditionAnd::ptr_t ConditionAnd::recompute(Database& db, const timepair_t& tm) const
{
	seq_t seq;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (p)
			p = p->recompute(db, tm);
		seq.push_back(p);
	}
	return simplify(seq);
}

ConditionAnd::ptr_t ConditionAnd::simplify(const seq_t& seq) const
{
	if (seq.size() != m_cond.size())
		throw std::runtime_error("ConditionAnd::simplify: internal error");
	Glib::RefPtr<ConditionAnd> r(new ConditionAnd(get_childnum(), m_inv));
	bool val(true), modified(false);
	for (unsigned int i = 0; i < m_cond.size() && val; ++i) {
		bool cst1(true), val1(false);
		const CondInv& c(m_cond[i]);
		const_ptr_t p(c.get_ptr());
		if (p) {
			const_ptr_t p1(seq[i]);
			if (p1) {
				modified = true;
				p = p1;
			}
			cst1 = p->is_constant();
			if (cst1)
				val1 = p->get_constvalue();
		}
		if (!cst1) {
			r->add(p, c.is_inv());
			continue;
		}
		modified = true;
		if (c.is_inv())
			val1 = !val1;
		val = val && val1;
	}
	if (val && !r->m_cond.empty()) {
		if (r->m_cond.size() == 1 && !m_inv == !r->m_cond[0].is_inv())
			return ptr_t::cast_const(r->m_cond[0].get_ptr());
		if (modified)
			return r;
		return ptr_t();
	}
	if (m_inv)
		val = !val;
	return ptr_t(new ConditionConstant(get_childnum(), val));
}

ConditionAnd::ptr_t ConditionAnd::simplify(void) const
{
	seq_t seq;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (p)
			p = p->simplify();
		seq.push_back(p);
	}
	return simplify(seq);
}

ConditionAnd::ptr_t ConditionAnd::simplify_bbox(const Rect& bbox, const timepair_t& tm) const
{
	seq_t seq;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (p)
			p = p->simplify_bbox(bbox, tm);
		seq.push_back(p);
	}
	return simplify(seq);
}

ConditionAnd::ptr_t ConditionAnd::simplify_altrange(int32_t minalt, int32_t maxalt, const timepair_t& tm) const
{
	seq_t seq;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (p)
			p = p->simplify_altrange(minalt, maxalt, tm);
		seq.push_back(p);
	}
	return simplify(seq);
}

ConditionAnd::ptr_t ConditionAnd::simplify_aircrafttype(const std::string& acfttype) const
{
	seq_t seq;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (p)
			p = p->simplify_aircrafttype(acfttype);
		seq.push_back(p);
	}
	return simplify(seq);
}

ConditionAnd::ptr_t ConditionAnd::simplify_aircraftclass(const std::string& acftclass) const
{
	seq_t seq;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (p)
			p = p->simplify_aircraftclass(acftclass);
		seq.push_back(p);
	}
	return simplify(seq);
}

ConditionAnd::ptr_t ConditionAnd::simplify_equipment(const std::string& equipment, Aircraft::pbn_t pbn) const
{
	seq_t seq;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (p)
			p = p->simplify_equipment(equipment, pbn);
		seq.push_back(p);
	}
	return simplify(seq);
}

ConditionAnd::ptr_t ConditionAnd::simplify_typeofflight(char type_of_flight) const
{
	seq_t seq;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (p)
			p = p->simplify_typeofflight(type_of_flight);
		seq.push_back(p);
	}
	return simplify(seq);
}

ConditionAnd::ptr_t ConditionAnd::simplify_mil(bool mil) const
{
	seq_t seq;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (p)
			p = p->simplify_mil(mil);
		seq.push_back(p);
	}
	return simplify(seq);
}

ConditionAnd::ptr_t ConditionAnd::simplify_dep(const Link& arpt, const timepair_t& tm) const
{
	seq_t seq;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (p)
			p = p->simplify_dep(arpt, tm);
		seq.push_back(p);
	}
	return simplify(seq);
}

ConditionAnd::ptr_t ConditionAnd::simplify_dest(const Link& arpt, const timepair_t& tm) const
{
	seq_t seq;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (p)
			p = p->simplify_dest(arpt, tm);
		seq.push_back(p);
	}
	return simplify(seq);
}

ConditionAnd::ptr_t ConditionAnd::simplify_conditionalavailability(const Graph& g, const ConditionalAvailability& condavail,
								   const TimeTableSpecialDateEval& tsde, const timepair_t& tm) const
{
	seq_t seq;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (p)
			p = p->simplify_conditionalavailability(g, condavail, tsde, tm);
		seq.push_back(p);
	}
	return simplify(seq);
}

CondResult ConditionAnd::evaluate(RestrictionEval& tfrs) const
{
	bool hasdctlim(false);
	CondResult r(!m_inv, !m_inv);
	if (m_inv) {
		// OR type
		for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
			const_ptr_t p(i->get_ptr());
			if (!p)
				continue;
			hasdctlim = hasdctlim || (p->get_type() == type_dctlimit);
			CondResult r1(p->evaluate(tfrs));
			if (!i->is_inv() && !boost::logic::indeterminate(r1.get_result()))
				r1 = ~r1;
			r |= r1;
		}
	} else {
		// AND type
		for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
			const_ptr_t p(i->get_ptr());
			if (!p)
				return CondResult(false);
			hasdctlim = hasdctlim || (p->get_type() == type_dctlimit);
			CondResult r1(p->evaluate(tfrs));
			if (i->is_inv() && !boost::logic::indeterminate(r1.get_result()))
				r1 = ~r1;
			r &= r1;
			if (r.get_result() == false)
				return CondResult(false);
		}
	}
	if (hasdctlim) {
		r.get_vertexset().clear();
		if (r.get_xngedgeset().empty()) {
			r.set_result(false);
			r.get_edgeset().clear();
		} else {
			r.get_edgeset() = r.get_xngedgeset();
		}
	}
	return r;
}

CondResult ConditionAnd::evaluate_trace(RestrictionEval& tfrs, const Path *backptr) const
{
	Path path(backptr, this);
	bool hasdctlim(false);
	CondResult r(!m_inv, !m_inv);
	if (m_inv) {
		// OR type
		for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
			const_ptr_t p(i->get_ptr());
			if (!p)
				continue;
			hasdctlim = hasdctlim || (p->get_type() == type_dctlimit);
			CondResult r1(p->evaluate_trace(tfrs, &path));
			if (!i->is_inv() && !boost::logic::indeterminate(r1.get_result()))
				r1 = ~r1;
			r |= r1;
		}
	} else {
		// AND type
		for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
			const_ptr_t p(i->get_ptr());
			if (!p) {
				r = CondResult(false);
				break;
			}
			hasdctlim = hasdctlim || (p->get_type() == type_dctlimit);
			CondResult r1(p->evaluate_trace(tfrs, &path));
			if (i->is_inv() && !boost::logic::indeterminate(r1.get_result()))
				r1 = ~r1;
			r &= r1;
			if (r.get_result() == false) {
				r = CondResult(false);
				break;
			}
		}
	}
	if (hasdctlim) {
		r.get_vertexset().clear();
		if (r.get_xngedgeset().empty()) {
			r.set_result(false);
			r.get_edgeset().clear();
		} else {
			r.get_edgeset() = r.get_xngedgeset();
		}
	}
	trace(tfrs, r, backptr);
	return r;
}

Condition::routestatic_t ConditionAnd::is_routestatic(RuleSegment& seg) const
{
	routestatic_t ret(m_inv ? routestatic_staticfalse : routestatic_statictrue);
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (!p)
			continue;
		routestatic_t rs(p->is_routestatic(seg));
		if (i->is_inv()) {
			switch (rs) {
			case routestatic_staticfalse:
				rs = routestatic_statictrue;
				break;

			case routestatic_statictrue:
				rs = routestatic_staticfalse;
				break;

			default:
				break;
			}
		}
		switch (rs) {
		case routestatic_staticfalse:
			return m_inv ? routestatic_statictrue : routestatic_staticfalse;

		case routestatic_nonstatic:
			ret = routestatic_nonstatic;
			break;

		case routestatic_staticunknown:
			if (ret != routestatic_nonstatic)
				ret = routestatic_staticunknown;
			break;

		default:
			break;
		}
	}
	return ret;
}

bool ConditionAnd::is_routestatic(void) const
{
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (!p)
			continue;
		if (!p->is_routestatic())
			return false;
	}
	return true;
}

bool ConditionAnd::is_dct(void) const
{
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (!p)
			continue;
		if (p->is_dct())
			return true;
	}
	return false;
}

bool ConditionAnd::is_valid_dct(bool allowarrdep, civmil_t& civmil) const
{
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (!p)
			return false;
		{
			Glib::RefPtr<const ConditionFlight> pf(Glib::RefPtr<const ConditionFlight>::cast_dynamic(p));
			if (pf) {
				switch (pf->get_purpose()) {
				case ConditionFlight::purpose_all:
				case ConditionFlight::purpose_invalid:
					break;

				default:
					return false;
				}
				civmil_t civmil1(pf->get_civmil());
				if (i->is_inv()) {
					switch (civmil1) {
					case civmil_mil:
						civmil1 = civmil_civ;
						break;

					case civmil_civ:
						civmil1 = civmil_mil;
						break;

					default:
						break;
					}
				}
				if (civmil == civmil_invalid)
					civmil = civmil1;
				else if (civmil != civmil1 && civmil1 != civmil_invalid)
					return false;
				continue;
			}
		}
		if (m_inv != i->is_inv())
			return false;
		civmil_t civmil1(civmil_invalid);
		if (!p->is_valid_dct(allowarrdep, civmil1))
			return false;
		if (i->is_inv()) {
			switch (civmil1) {
			case civmil_mil:
				civmil1 = civmil_civ;
				break;

			case civmil_civ:
				civmil1 = civmil_mil;
				break;

			default:
				break;
			}
		}
		if (civmil == civmil_invalid)
			civmil = civmil1;
		else if (civmil != civmil1 && civmil1 != civmil_invalid)
			return false;
	}
	if (m_inv) {
		switch (civmil) {
		case civmil_mil:
			civmil = civmil_civ;
			break;

		case civmil_civ:
			civmil = civmil_mil;
			break;

		default:
			break;
		}
	}
	return true;
}

BidirAltRange ConditionAnd::evaluate_dct(DctParameters::Calc& dct) const
{
	if (m_inv) {
		// OR type
		BidirAltRange r;
		r.set_empty();
		for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
			const_ptr_t p(i->get_ptr());
			if (!p)
				continue;
			{
				Glib::RefPtr<const ConditionFlight> pf(Glib::RefPtr<const ConditionFlight>::cast_dynamic(p));
				if (pf)
					continue;
			}
			BidirAltRange r1(p->evaluate_dct(dct));
			if (!i->is_inv())
				r1.invert();
			r |= r1;
		}
		return r;
	}
	// AND type
	BidirAltRange r(dct.get_alt());
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (!p) {
			r.set_empty();
			break;
		}
		{
			Glib::RefPtr<const ConditionFlight> pf(Glib::RefPtr<const ConditionFlight>::cast_dynamic(p));
			if (pf)
				continue;
		}
		BidirAltRange r1(p->evaluate_dct(dct));
		if (i->is_inv())
			r1.invert();
		r &= r1;
		if (r.is_empty())
			break;
	}
	return r;
}

BidirAltRange ConditionAnd::evaluate_dct_trace(DctParameters::Calc& dct, const Path *backptr) const
{
	Path path(backptr, this);
	if (m_inv) {
		// OR type
		BidirAltRange r;
		r.set_empty();
		for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
			const_ptr_t p(i->get_ptr());
			if (!p)
				continue;
			{
				Glib::RefPtr<const ConditionFlight> pf(Glib::RefPtr<const ConditionFlight>::cast_dynamic(p));
				if (pf)
					continue;
			}
			BidirAltRange r1(p->evaluate_dct_trace(dct, &path));
			if (!i->is_inv())
				r1.invert();
			r |= r1;
		}
		trace(dct, r, backptr);
		return r;
	}
	// AND type
	BidirAltRange r(dct.get_alt());
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (!p) {
			r.set_empty();
			break;
		}
		{
			Glib::RefPtr<const ConditionFlight> pf(Glib::RefPtr<const ConditionFlight>::cast_dynamic(p));
			if (pf)
				continue;
		}
		BidirAltRange r1(p->evaluate_dct_trace(dct, &path));
		if (i->is_inv())
			r1.invert();
		r &= r1;
		if (r.is_empty())
			break;
	}
	trace(dct, r, backptr);
	return r;
}

bool ConditionAnd::is_deparr(std::set<Link>& dep, std::set<Link>& dest) const
{
	if (!m_inv)
		return false;
	bool ret(false);
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		if (!i->is_inv())
			return false;
		const_ptr_t p(i->get_ptr());
		if (!p)
			return false;
		if (!p->is_deparr(dep, dest))
			return false;
		ret = true;
	}
	return ret;
}

bool ConditionAnd::is_deparr_dct(Link& arpt, bool& arr, double& dist, civmil_t& civmil) const
{
	if (m_inv)
		return false;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (!p)
			return false;
		Link arpt1;
		bool arr1;
		double dist1(std::numeric_limits<double>::quiet_NaN());
		civmil_t civmil1(civmil_invalid);
		if (!p->is_deparr_dct(arpt1, arr1, dist1, civmil1))
			return false;
		if (i->is_inv()) {
			if (!arpt1.is_nil() || !std::isnan(dist1))
				return false;
			switch (civmil1) {
			case civmil_mil:
				civmil1 = civmil_civ;
				break;

			case civmil_civ:
				civmil1 = civmil_mil;
				break;

			default:
				break;
			}
		}
		if (arpt.is_nil()) {
			arpt = arpt1;
			arr = arr1;
		} else if (!arpt1.is_nil()) {
			if (arpt != arpt1 || arr != arr1)
				return false;
		}
		if (std::isnan(dist)) {
			dist = dist1;
		} else if (!std::isnan(dist1)) {
			dist = std::min(dist, dist1);
		}
		if (civmil == civmil_invalid)
			civmil = civmil1;
		else if (civmil != civmil1 && civmil1 != civmil_invalid)
			return false;
	}
	return true;
}

bool ConditionAnd::is_enroute_dct(Link& aspc, AltRange& alt, double& dist, civmil_t& civmil) const
{
	if (m_inv)
		return false;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (!p)
			return false;
		Link aspc1;
		AltRange alt1;
		double dist1(std::numeric_limits<double>::quiet_NaN());
		civmil_t civmil1(civmil_invalid);
		if (!p->is_enroute_dct(aspc1, alt1, dist1, civmil1))
			return false;
		if (i->is_inv()) {
			if (!aspc1.is_nil() || !std::isnan(dist1))
				return false;
			switch (civmil1) {
			case civmil_mil:
				civmil1 = civmil_civ;
				break;

			case civmil_civ:
				civmil1 = civmil_mil;
				break;

			default:
				break;
			}
		}
		if (aspc.is_nil()) {
			aspc = aspc1;
			alt = alt1;
		} else if (!aspc1.is_nil()) {
			if (aspc != aspc1)
				return false;
			alt.intersect(alt1);
		}
		if (std::isnan(dist)) {
			dist = dist1;
		} else if (!std::isnan(dist1)) {
			dist = std::min(dist, dist1);
		}
		if (civmil == civmil_invalid)
			civmil = civmil1;
		else if (civmil != civmil1 && civmil1 != civmil_invalid)
			return false;
	}
	return true;
}

std::vector<RuleSequence> ConditionAnd::get_mandatory(void) const
{
	if (m_inv)
		// OR type
		return std::vector<RuleSequence>();
	// AND type
	std::vector<RuleSequence> r;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		if (!i->is_inv())
			continue;
		const_ptr_t p(i->get_ptr());
		if (!p)
			continue;
		{
			RuleSegment seg(p->get_resultsequence());
			if (seg.is_valid()) {
				r.push_back(RuleSequence());
				r.back().push_back(seg);
			}
		}
		{
			std::vector<RuleSegment> seq(p->get_mandatory_or());
			for (std::vector<RuleSegment>::const_iterator si(seq.begin()), se(seq.end()); si != se; ++si) {
				if (!si->is_valid())
					continue;
				r.push_back(RuleSequence());
				r.back().push_back(*si);
			}
		}
		{
			std::vector<RuleSequence> alt(p->get_mandatory_seq());
			for (std::vector<RuleSequence>::const_iterator ai(alt.begin()), ae(alt.end()); ai != ae; ++ai) {
				r.push_back(*ai);
			}
		}
	}
	return r;
}

std::vector<RuleSegment> ConditionAnd::get_mandatory_or(void) const
{
	if (!m_inv)
		// AND type
		return std::vector<RuleSegment>();
	// OR type
	std::vector<RuleSegment> r;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		if (!i->is_inv())
			return std::vector<RuleSegment>();
		const_ptr_t p(i->get_ptr());
		if (!p)
			return std::vector<RuleSegment>();
	        r.push_back(p->get_resultsequence());
		if (!r.back().is_valid())
			return std::vector<RuleSegment>();
	}
	return r;
}

Condition::forbiddenpoint_t ConditionAnd::evaluate_forbidden_point(const UUID& pt, const UUID& dep, const UUID& dest,
								   const TimeTableEval& tte, const Point& v1c, 
								   const UUID& v0, const UUID& v1, const UUID& awy, int32_t alt) const
{
	static const forbiddenpoint_t samesegret[2][4] = {
		{ forbiddenpoint_false, forbiddenpoint_falsesamesegbefore, forbiddenpoint_falsesamesegat, forbiddenpoint_falsesamesegafter },
		{ forbiddenpoint_true, forbiddenpoint_truesamesegbefore, forbiddenpoint_truesamesegat, forbiddenpoint_truesamesegafter }
	};
	bool r(!m_inv);
	unsigned int sameseg(0);
	bool hasotherseg(false);
	if (m_inv) {
		// OR type
		for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
			const_ptr_t p(i->get_ptr());
			if (!p)
				continue;
			forbiddenpoint_t r1(p->evaluate_forbidden_point(pt, dep, dest, tte, v1c, v0, v1, awy, alt));
			if (false)
				std::cerr << "ConditionAnd::evaluate_forbidden_point: " << (i - m_cond.begin()) << " result " << r1 << std::endl;
			switch (r1) {
			case forbiddenpoint_truesamesegbefore:
			case forbiddenpoint_falsesamesegbefore:
				sameseg = std::max(sameseg, 1U);
				break;

			case forbiddenpoint_truesamesegat:
			case forbiddenpoint_falsesamesegat:
				sameseg = std::max(sameseg, 2U);
				break;

			case forbiddenpoint_truesamesegafter:
			case forbiddenpoint_falsesamesegafter:
				sameseg = std::max(sameseg, 3U);
				break;

			case forbiddenpoint_trueotherseg:
			case forbiddenpoint_falseotherseg:
				hasotherseg = true;
				break;

			default:
				break;
			}
			switch (r1) {
			case forbiddenpoint_true:
			case forbiddenpoint_truesamesegbefore:
			case forbiddenpoint_truesamesegat:
			case forbiddenpoint_truesamesegafter:
			case forbiddenpoint_trueotherseg:
				r = r || i->is_inv();
				break;

			case forbiddenpoint_false:
			case forbiddenpoint_falsesamesegbefore:
			case forbiddenpoint_falsesamesegat:
			case forbiddenpoint_falsesamesegafter:
			case forbiddenpoint_falseotherseg:
				r = r || !i->is_inv();
				break;

			default:
				return forbiddenpoint_invalid;
			}
		}
	} else {
		// AND type
		for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
			const_ptr_t p(i->get_ptr());
			if (!p) {
				r = false;
				continue;
			}
			forbiddenpoint_t r1(p->evaluate_forbidden_point(pt, dep, dest, tte, v1c, v0, v1, awy, alt));
			if (false)
				std::cerr << "ConditionAnd::evaluate_forbidden_point: " << (i - m_cond.begin()) << " result " << r1 << std::endl;
			switch (r1) {
			case forbiddenpoint_truesamesegbefore:
			case forbiddenpoint_falsesamesegbefore:
				sameseg = std::max(sameseg, 1U);
				break;

			case forbiddenpoint_truesamesegat:
			case forbiddenpoint_falsesamesegat:
				sameseg = std::max(sameseg, 2U);
				break;

			case forbiddenpoint_truesamesegafter:
			case forbiddenpoint_falsesamesegafter:
				sameseg = std::max(sameseg, 3U);
				break;

			case forbiddenpoint_trueotherseg:
			case forbiddenpoint_falseotherseg:
				hasotherseg = true;
				break;

			default:
				break;
			}
			switch (r1) {
			case forbiddenpoint_true:
			case forbiddenpoint_truesamesegbefore:
			case forbiddenpoint_truesamesegat:
			case forbiddenpoint_truesamesegafter:
			case forbiddenpoint_trueotherseg:
				r = r && !i->is_inv();
				break;

			case forbiddenpoint_false:
			case forbiddenpoint_falsesamesegbefore:
			case forbiddenpoint_falsesamesegat:
			case forbiddenpoint_falsesamesegafter:
			case forbiddenpoint_falseotherseg:
				r = r && i->is_inv();
				break;

			default:
				return forbiddenpoint_invalid;
			}
		}
	}
	return samesegret[r][sameseg];
}

void ConditionAnd::io(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void ConditionAnd::io(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void ConditionAnd::io(ArchiveWriteStream& ar) const
{
	const_cast<ConditionAnd *>(this)->hibernate(ar);
}

void ConditionAnd::io(LinkLoader& ar)
{
	hibernate(ar);
}

void ConditionAnd::io(DependencyGenerator<UUID::set_t>& ar) const
{
	const_cast<ConditionAnd *>(this)->hibernate(ar);
}

void ConditionAnd::io(DependencyGenerator<Link::LinkSet>& ar) const
{
	const_cast<ConditionAnd *>(this)->hibernate(ar);
}

ConditionSequence::ConditionSequence(uint32_t childnum)
	: Condition(childnum)
{
}

void ConditionSequence::add(const_ptr_t p)
{
	m_seq.push_back(p);
}

bool ConditionSequence::is_refloc(void) const
{
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (!p)
			continue;
		if (p->is_refloc())
			return true;
	}
	return false;
}

bool ConditionSequence::is_constant(void) const
{
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (!p)
			continue;
		if (!p->is_constant())
			return false;
	}
	return true;
}

bool ConditionSequence::get_constvalue(void) const
{
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (!p)
			return false;
		if (!p->get_constvalue())
			return false;
	}
	return true;
}

std::ostream& ConditionSequence::print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const
{
	os << std::endl << std::string(indent, ' ') << "ConditionSequence{" << get_childnum() << '}';
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		if (*i) {
			(*i)->print(os, indent + 2, ts);
			continue;
		}
		os << std::endl << std::string(indent + 2, ' ') << "null";
	}
	return os;
}

std::string ConditionSequence::to_str(timetype_t tm) const
{
	std::string r;
	{
		std::ostringstream oss;
		oss << "Seq{" << get_childnum() << "}(";
		r = oss.str();
	}
	bool subseq(false);
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		if (subseq)
			r += ",";
		subseq = true;
		const_ptr_t p(*i);
		if (!p) {
			r += "null";
			continue;
		}
		r += p->to_str(tm);
	}
	r += ")";
	return r;
}

void ConditionSequence::add_bbox(Rect& bbox, const TimeSlice& ts) const
{
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (!p)
			continue;
		p->add_bbox(bbox, ts);
	}
}

ConditionSequence::ptr_t ConditionSequence::clone(void) const
{
	Glib::RefPtr<ConditionSequence> r(new ConditionSequence(get_childnum()));
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (p)
			p = p->clone();
		r->add(p);
	}
	return r;
}

ConditionSequence::ptr_t ConditionSequence::recompute(Database& db, const timepair_t& tm) const
{
	seq_t seq;
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (p)
			p = p->recompute(db, tm);
		seq.push_back(p);
	}
	return simplify(seq);
}

ConditionSequence::ptr_t ConditionSequence::simplify(const seq_t& seq) const
{
	if (seq.size() != m_seq.size())
		throw std::runtime_error("ConditionSequence::simplify: internal error");
	Glib::RefPtr<ConditionSequence> r(new ConditionSequence(get_childnum()));
	bool val(true), modified(false);
	for (unsigned int i = 0; i < m_seq.size() && val; ++i) {
		const_ptr_t p(m_seq[i]);
		bool cst1(true), val1(false);
		if (p) {
			const_ptr_t p1(seq[i]);
			if (p1) {
				modified = true;
				p = p1;
			}
			cst1 = p->is_constant();
			if (cst1)
				val1 = p->get_constvalue();
		}
		if (!cst1) {
			r->add(p);
			continue;
		}
		modified = true;
		val = val && val1;
	}
	if (val && !r->m_seq.empty()) {
		if (r->m_seq.size() == 1)
			return ptr_t::cast_const(r->m_seq[0]);
		if (modified)
			return r;
		return ptr_t();
	}
	return ptr_t(new ConditionConstant(get_childnum(), val));
}

ConditionSequence::ptr_t ConditionSequence::simplify(void) const
{
	seq_t seq;
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (p)
			p = p->simplify();
		seq.push_back(p);
	}
	return simplify(seq);
}

ConditionSequence::ptr_t ConditionSequence::simplify_bbox(const Rect& bbox, const timepair_t& tm) const
{
	seq_t seq;
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (p)
			p = p->simplify_bbox(bbox, tm);
		seq.push_back(p);
	}
	return simplify(seq);
}

ConditionSequence::ptr_t ConditionSequence::simplify_altrange(int32_t minalt, int32_t maxalt, const timepair_t& tm) const
{
	seq_t seq;
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (p)
			p = p->simplify_altrange(minalt, maxalt, tm);
		seq.push_back(p);
	}
	return simplify(seq);
}

ConditionSequence::ptr_t ConditionSequence::simplify_aircrafttype(const std::string& acfttype) const
{
	seq_t seq;
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (p)
			p = p->simplify_aircrafttype(acfttype);
		seq.push_back(p);
	}
	return simplify(seq);
}

ConditionSequence::ptr_t ConditionSequence::simplify_aircraftclass(const std::string& acftclass) const
{
	seq_t seq;
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (p)
			p = p->simplify_aircraftclass(acftclass);
		seq.push_back(p);
	}
	return simplify(seq);
}

ConditionSequence::ptr_t ConditionSequence::simplify_equipment(const std::string& equipment, Aircraft::pbn_t pbn) const
{
	seq_t seq;
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (p)
			p = p->simplify_equipment(equipment, pbn);
		seq.push_back(p);
	}
	return simplify(seq);
}

ConditionSequence::ptr_t ConditionSequence::simplify_typeofflight(char type_of_flight) const
{
	seq_t seq;
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (p)
			p = p->simplify_typeofflight(type_of_flight);
		seq.push_back(p);
	}
	return simplify(seq);
}

ConditionSequence::ptr_t ConditionSequence::simplify_mil(bool mil) const
{
	seq_t seq;
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (p)
			p = p->simplify_mil(mil);
		seq.push_back(p);
	}
	return simplify(seq);
}

ConditionSequence::ptr_t ConditionSequence::simplify_dep(const Link& arpt, const timepair_t& tm) const
{
	seq_t seq;
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (p)
			p = p->simplify_dep(arpt, tm);
		seq.push_back(p);
	}
	return simplify(seq);
}

ConditionSequence::ptr_t ConditionSequence::simplify_dest(const Link& arpt, const timepair_t& tm) const
{
	seq_t seq;
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (p)
			p = p->simplify_dest(arpt, tm);
		seq.push_back(p);
	}
	return simplify(seq);
}

ConditionSequence::ptr_t ConditionSequence::simplify_conditionalavailability(const Graph& g, const ConditionalAvailability& condavail,
									     const TimeTableSpecialDateEval& tsde, const timepair_t& tm) const
{
	seq_t seq;
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (p)
			p = p->simplify_conditionalavailability(g, condavail, tsde, tm);
		seq.push_back(p);
	}
	return simplify(seq);
}

CondResult ConditionSequence::evaluate(RestrictionEval& tfrs) const
{
	CondResult r(true);
	bool first(true);
	unsigned int seq(0U);
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (!p)
			return CondResult(false);
		if (first) {
			r = p->evaluate(tfrs);
			first = false;
			seq = r.get_seqorder();
		} else {
			CondResult r1(p->evaluate(tfrs));
			unsigned int seq1(r1.get_seqorder(seq));
			if (seq1 <= seq)
				return CondResult(false);
			r &= r1;
			seq = seq1;
		}
		if (r.get_result() == false)
			return CondResult(false);
	}
	r.get_xngedgeset().clear();
	return r;
}

CondResult ConditionSequence::evaluate_trace(RestrictionEval& tfrs, const Path *backptr) const
{
	Path path(backptr, this);
	CondResult r(true);
	bool first(true);
	unsigned int seq(0U);
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (!p) {
			r = CondResult(false);
			break;
		}
		if (first) {
			r = p->evaluate_trace(tfrs, &path);
			first = false;
			seq = r.get_seqorder();
		} else {
			CondResult r1(p->evaluate_trace(tfrs, &path));
			unsigned int seq1(r1.get_seqorder(seq));
			if (seq1 <= seq) {
				r = CondResult(false);
				break;
			}
			r &= r1;
			seq = seq1;
		}
		if (r.get_result() == false) {
			r = CondResult(false);
			break;
		}
	}
	r.get_xngedgeset().clear();
	trace(tfrs, r, backptr);
	return r;
}

Condition::routestatic_t ConditionSequence::is_routestatic(RuleSegment& seg) const
{
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (!p)
			continue;
		if (p->is_routestatic(seg) == routestatic_nonstatic)
			return routestatic_nonstatic;
	}
	return routestatic_staticunknown;
}

bool ConditionSequence::is_routestatic(void) const
{
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (!p)
			continue;
		if (!p->is_routestatic())
			return false;
	}
	return true;
}

std::vector<RuleSequence> ConditionSequence::get_mandatory_seq(void) const
{
	std::vector<RuleSequence> r;
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (!p)
			return std::vector<RuleSequence>();
		{
			RuleSegment seg(p->get_resultsequence());
			if (seg.is_valid()) {
				if (r.empty())
					r.push_back(RuleSequence());
				for (std::vector<RuleSequence>::iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
					ri->push_back(seg);
				}
			}
		}
		{
			std::vector<RuleSegment> seq(p->get_mandatory_or());
			std::vector<RuleSequence> rnew;
			for (std::vector<RuleSegment>::const_iterator si(seq.begin()), se(seq.end()); si != se; ++si) {
				if (!si->is_valid())
					continue;
				if (r.empty()) {
					rnew.push_back(RuleSequence());
					rnew.back().push_back(*si);
					continue;
				}
				for (std::vector<RuleSequence>::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
					rnew.push_back(*ri);
					rnew.back().push_back(*si);
				}
			}
			r.swap(rnew);
		}
	}
	return r;
}

Condition::forbiddenpoint_t ConditionSequence::evaluate_forbidden_point(const UUID& pt, const UUID& dep, const UUID& dest,
									const TimeTableEval& tte, const Point& v1c, 
									const UUID& v0, const UUID& v1, const UUID& awy, int32_t alt) const
{
	static const forbiddenpoint_t samesegret[2][4] = {
		{ forbiddenpoint_false, forbiddenpoint_falsesamesegbefore, forbiddenpoint_falsesamesegat, forbiddenpoint_falsesamesegafter },
		{ forbiddenpoint_true, forbiddenpoint_truesamesegbefore, forbiddenpoint_truesamesegat, forbiddenpoint_truesamesegafter }
	};
	bool r(true);
	unsigned int sameseg(0);
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (!p)
			return forbiddenpoint_invalid;
		forbiddenpoint_t r1(p->evaluate_forbidden_point(pt, dep, dest, tte, v1c, v0, v1, awy, alt));
		if (false)
			std::cerr << "ConditionSequence::evaluate_forbidden_point: " << (i - m_seq.begin()) << " result " << r1 << std::endl;
		switch (r1) {
		case forbiddenpoint_truesamesegbefore:
		case forbiddenpoint_falsesamesegbefore:
			if (sameseg > 1)
				return forbiddenpoint_invalid;
			sameseg = 1;
			break;

		case forbiddenpoint_truesamesegat:
		case forbiddenpoint_falsesamesegat:
			if (sameseg > 2)
				return forbiddenpoint_invalid;
			sameseg = 2;
			break;

		case forbiddenpoint_truesamesegafter:
		case forbiddenpoint_falsesamesegafter:
			if (sameseg > 3)
				return forbiddenpoint_invalid;
			sameseg = 3;
			break;

		default:
			break;
		}
		switch (r1) {
		case forbiddenpoint_true:
		case forbiddenpoint_truesamesegbefore:
		case forbiddenpoint_truesamesegat:
		case forbiddenpoint_truesamesegafter:
			break;

		case forbiddenpoint_false:
		case forbiddenpoint_falsesamesegbefore:
		case forbiddenpoint_falsesamesegat:
		case forbiddenpoint_falsesamesegafter:
			r = false;
			break;

		default:
			return forbiddenpoint_invalid;
		}
	}
	return samesegret[r][sameseg];
}

void ConditionSequence::io(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void ConditionSequence::io(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void ConditionSequence::io(ArchiveWriteStream& ar) const
{
	const_cast<ConditionSequence *>(this)->hibernate(ar);
}

void ConditionSequence::io(LinkLoader& ar)
{
	hibernate(ar);
}

void ConditionSequence::io(DependencyGenerator<UUID::set_t>& ar) const
{
	const_cast<ConditionSequence *>(this)->hibernate(ar);
}

void ConditionSequence::io(DependencyGenerator<Link::LinkSet>& ar) const
{
	const_cast<ConditionSequence *>(this)->hibernate(ar);
}

ConditionConstant::ConditionConstant(uint32_t childnum, bool val)
	: Condition(childnum), m_value(val)
{
}

std::ostream& ConditionConstant::print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const
{
	os << std::endl << std::string(indent, ' ') << "ConditionConstant{" << get_childnum() << "} " << (m_value ? "true" : "false");
	return os;
}

std::string ConditionConstant::to_str(timetype_t tm) const
{
	std::ostringstream oss;
	oss << "Const{" << get_childnum() << "}(" << (m_value ? 'T' : 'F') << ')';
	return oss.str();
}

Condition::ptr_t ConditionConstant::clone(void) const
{
	return ptr_t(new ConditionConstant(m_childnum, m_value));
}

CondResult ConditionConstant::evaluate(RestrictionEval& tfrs) const
{
	return CondResult(m_value);
}

BidirAltRange ConditionConstant::evaluate_dct(DctParameters::Calc& dct) const
{
	BidirAltRange x(dct.get_alt());
	if (!m_value)
		x.set_empty();
	return x;
}

void ConditionConstant::io(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void ConditionConstant::io(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void ConditionConstant::io(ArchiveWriteStream& ar) const
{
	const_cast<ConditionConstant *>(this)->hibernate(ar);
}

void ConditionConstant::io(LinkLoader& ar)
{
	hibernate(ar);
}

void ConditionConstant::io(DependencyGenerator<UUID::set_t>& ar) const
{
	const_cast<ConditionConstant *>(this)->hibernate(ar);
}

void ConditionConstant::io(DependencyGenerator<Link::LinkSet>& ar) const
{
	const_cast<ConditionConstant *>(this)->hibernate(ar);
}

ConditionAltitude::ConditionAltitude(uint32_t childnum, const AltRange& alt)
	: Condition(childnum), m_alt(alt)
{
}

ConditionAltitude::ptr_t ConditionAltitude::simplify_altrange(int32_t minalt, int32_t maxalt, const timepair_t& tm) const
{
	if (minalt > maxalt || 
	    (m_alt.is_lower_valid() && maxalt < m_alt.get_lower_alt()) ||
	    (m_alt.is_upper_valid() && minalt > m_alt.get_upper_alt()))
		return ptr_t(new ConditionConstant(get_childnum(), false));
	return ptr_t();
}

bool ConditionAltitude::check_alt(int32_t alt) const
{
	return m_alt.is_inside(alt);
}

ConditionCrossingAirspace1::ConditionCrossingAirspace1(uint32_t childnum, const AltRange& alt, const Link& aspc, bool refloc)
	: ConditionAltitude(childnum, alt), m_airspace(aspc), m_refloc(refloc)
{
}

std::ostream& ConditionCrossingAirspace1::print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const
{
	os << std::endl << std::string(indent, ' ') << "ConditionCrossingAirspace1{" << get_childnum() << "} " << to_altstr() << (m_refloc ? " RefLoc" : "")
	   << std::endl << std::string(indent + 2, ' ') << (UUID)m_airspace;
        {
		const AirspaceTimeSlice& tsa(m_airspace.get_obj()->operator()(ts).as_airspace());
                if (tsa.is_valid())
                        os << ' ' << tsa.get_ident();
        }
	return os;
}

std::string ConditionCrossingAirspace1::to_str(timetype_t tm) const
{
	std::ostringstream oss;
	oss << "XngAirspace{" << get_childnum();
	if (m_refloc)
		oss << ",R";
	oss << "}(" << to_altstr() << ',';
	const AirspaceTimeSlice& ts(m_airspace.get_obj()->operator()(tm).as_airspace());
	if (ts.is_valid())
		oss << ts.get_ident() << '/' << ts.get_type();
	else
		oss << "?""?";
	oss << ')';
	return oss.str();
}

void ConditionCrossingAirspace1::add_bbox(Rect& bbox, const TimeSlice& ts) const
{
	const Object::const_ptr_t& aspc(m_airspace.get_obj());
	for (unsigned int i(0), n(aspc->size()); i < n; ++i) {
		const TimeSlice& tsaspc1(aspc->operator[](i));
		if (!tsaspc1.is_overlap(ts))
			continue;
		const AirspaceTimeSlice& tsaspc(tsaspc1.as_airspace());
		if (!tsaspc.is_valid())
			continue;
		Rect bboxa;
		tsaspc.get_bbox(bboxa);
		if (bboxa.get_southwest().is_invalid() || bboxa.get_northeast().is_invalid())
			continue;
		if (bbox.get_southwest().is_invalid() || bbox.get_northeast().is_invalid())
			bbox = bboxa;
		else
			bbox = bbox.add(bboxa);
	}
}

Condition::ptr_t ConditionCrossingAirspace1::clone(void) const
{
	return ptr_t(new ConditionCrossingAirspace1(m_childnum, m_alt, m_airspace, m_refloc));
}

ConditionCrossingAirspace1::ptr_t ConditionCrossingAirspace1::simplify_bbox(const Rect& bbox, const timepair_t& tm) const
{
	for (unsigned int i(0), n(m_airspace.get_obj()->size()); i < n; ++i) {
		const AirspaceTimeSlice& tsa(m_airspace.get_obj()->operator[](i).as_airspace());
		if (!tsa.is_valid() || !tsa.is_overlap(tm))
			continue;
		Rect bboxa;
		tsa.get_bbox(bboxa);
		if (bbox.is_intersect(bboxa))
			return ptr_t();
	}
	return ptr_t(new ConditionConstant(get_childnum(), false));
}

CondResult ConditionCrossingAirspace1::evaluate(RestrictionEval& tfrs) const
{
	// exclude SID and STAR leg
	// otherwise, -(FPL-ABCDE-IG -1P28R/L -SDFGLOR/S -LGST1005 -N0138F090 DCT XAVIS A10 ALIKI A14 MIL B34 BADEL/N0130F120 B34 DDM G85 KOR L613 YNN L604 ELBAK ELBAK1J -LATI0345 -PBN/B2 DOF/140206 ) triggers an error
	CondResult r(false);
	for (unsigned int wptnr = 2U, n = tfrs.wpt_size(); wptnr + 1U < n; ++wptnr) {
		const RestrictionEval::Waypoint& wpt0(tfrs.wpt(wptnr - 1U));
		const RestrictionEval::Waypoint& wpt1(tfrs.wpt(wptnr));
		if (!wpt0.is_ifr())
			continue;
		if (false && (wpt0.get_pathcode() == FPlanWaypoint::pathcode_sid ||
			      wpt0.get_pathcode() == FPlanWaypoint::pathcode_star))
			continue;
		const AirspaceTimeSlice& ts(get_airspace().get_obj()->operator()(wpt0.get_time_unix()).as_airspace());
		if (!ts.is_valid())
			continue;
		if (!ifr_refloc || !m_refloc || wpt0.is_ifr()) {
			TimeTableEval tte(wpt0.get_time_unix(), wpt0.get_coord(), tfrs.get_specialdateeval());
			if (ts.is_inside(tte, wpt0.get_altitude(), get_alt(), wpt0.get_ptuuid())) {
				r.set_result(true);
				r.get_vertexset().insert(wptnr - 1U);
				if (wptnr >= 3U && wptnr <= n)
					r.get_xngedgeset().insert(wptnr - 2U);
				r.get_xngedgeset().insert(wptnr - 1U);
				if (m_refloc)
					r.set_refloc(wptnr - 1U);
				//std::cerr << wpt0.get_ident() << " inside " << ts.get_ident() << std::endl;
				continue;
			}
		}
		if (!ifr_refloc ||  !m_refloc || wpt1.is_ifr()) {
			TimeTableEval tte(wpt1.get_time_unix(), wpt1.get_coord(), tfrs.get_specialdateeval());
			if (ts.is_inside(tte, wpt1.get_altitude(), get_alt(), wpt1.get_ptuuid())) {
				r.set_result(true);
				r.get_vertexset().insert(wptnr);
				r.get_xngedgeset().insert(wptnr - 1U);
				if (wptnr + 2U < n)
					r.get_xngedgeset().insert(wptnr);
				if (m_refloc)
					r.set_refloc(wptnr);
				//std::cerr << wpt1.get_ident() << " inside " << ts.get_ident() << std::endl;
				continue;
			}
		}
		if (!ifr_refloc ||  !m_refloc || wpt0.is_ifr() || wpt1.is_ifr()) {
			TimeTableEval tte(wpt0.get_time_unix(), wpt0.get_coord(), tfrs.get_specialdateeval());
			TimeTableEval tteh(wpt0.get_time_unix() + (wpt1.get_time_unix() - wpt0.get_time_unix()) / 2,
					   wpt0.get_coord().halfway(wpt1.get_coord()), tfrs.get_specialdateeval());
			if (ts.is_intersect(tte, wpt1.get_coord(), std::min(wpt0.get_altitude(), wpt1.get_altitude()),
					    std::max(wpt0.get_altitude(), wpt1.get_altitude()), get_alt()) ||
			    ts.is_inside(tteh, wpt0.get_altitude() + (wpt1.get_altitude() - wpt0.get_altitude()) / 2, get_alt())) {
				r.set_result(true);
				r.get_edgeset().insert(wptnr - 1U);
				r.get_xngedgeset().insert(wptnr - 1U);
				if (m_refloc)
					r.set_refloc(wptnr);
				//std::cerr << wpt0.get_ident() << '-' << wpt1.get_ident()
				//	  << " inside " << ts.get_ident() << std::endl;
				continue;
			}
		}
	}
	return r;
}

Condition::routestatic_t ConditionCrossingAirspace1::is_routestatic(RuleSegment& seg) const
{
	if (seg.get_type() != RuleSegment::type_airspace || seg.get_uuid0() != get_airspace())
		return routestatic_nonstatic;
	AltRange ar(seg.get_alt());
	ar.intersect(get_alt());
	seg.get_alt() = ar;
	if (ar.is_empty())
		return routestatic_staticfalse;
	return routestatic_statictrue;
}

bool ConditionCrossingAirspace1::is_enroute_dct(Link& aspc, AltRange& alt, double& dist, civmil_t& civmil) const
{
	aspc = get_airspace();
	alt = m_alt;
	return true;
}

BidirAltRange ConditionCrossingAirspace1::evaluate_dct(DctParameters::Calc& dct) const
{
	BidirAltRange::set_t r;
	r.set_empty();
	Airspace::const_ptr_t aspc(Airspace::ptr_t::cast_dynamic(get_airspace().get_obj()));
	if (!aspc)
		return BidirAltRange(r, r);
	if (dct.is_airport())
		return BidirAltRange(r, r);
	r |= aspc->get_point_intersect_altitudes(dct.get_tte(0), dct.get_coord(1), get_alt(), dct.get_uuid(0), dct.get_uuid(1));
	return BidirAltRange(r, r);
}

Condition::forbiddenpoint_t ConditionCrossingAirspace1::evaluate_forbidden_point(const UUID& pt, const UUID& dep, const UUID& dest,
										 const TimeTableEval& tte, const Point& v1c, 
										 const UUID& v0, const UUID& v1, const UUID& awy, int32_t alt) const
{
	const AirspaceTimeSlice& ts(get_airspace().get_obj()->operator()(tte.get_time()).as_airspace());
	if (!ts.is_valid())
		return forbiddenpoint_invalid;
	TimeTableEval tte1(tte.get_time(), v1c, tte.get_specialdateeval());
	if (!(ts.is_inside(tte, alt, get_alt(), v0) ||
	      ts.is_inside(tte1, alt, get_alt(), v1) ||
	      ts.is_intersect(tte, v1c, alt, get_alt())))
		return forbiddenpoint_invalid;
	return (pt == v0) ? forbiddenpoint_truesamesegafter : (pt == v1) ? forbiddenpoint_truesamesegbefore : forbiddenpoint_trueotherseg;
}

void ConditionCrossingAirspace1::io(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void ConditionCrossingAirspace1::io(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void ConditionCrossingAirspace1::io(ArchiveWriteStream& ar) const
{
	const_cast<ConditionCrossingAirspace1 *>(this)->hibernate(ar);
}

void ConditionCrossingAirspace1::io(LinkLoader& ar)
{
	hibernate(ar);
}

void ConditionCrossingAirspace1::io(DependencyGenerator<UUID::set_t>& ar) const
{
	const_cast<ConditionCrossingAirspace1 *>(this)->hibernate(ar);
}

void ConditionCrossingAirspace1::io(DependencyGenerator<Link::LinkSet>& ar) const
{
	const_cast<ConditionCrossingAirspace1 *>(this)->hibernate(ar);
}

constexpr float ConditionCrossingAirspace2::fuzzy_dist;
const unsigned int ConditionCrossingAirspace2::fuzzy_exponent;

ConditionCrossingAirspace2::ConditionCrossingAirspace2(uint32_t childnum, const AltRange& alt, const Link& aspc0, const Link& aspc1, bool refloc)
	: ConditionAltitude(childnum, alt), m_refloc(refloc)
{
	m_airspace[0] = aspc0;
	m_airspace[1] = aspc1;
}

uint8_t ConditionCrossingAirspace2::rev8(uint8_t x)
{
        x = ((x >> 4) & 0x0F) | ((x << 4) & 0xF0);
        x = ((x >> 2) & 0x33) | ((x << 2) & 0xCC);
        x = ((x >> 1) & 0x55) | ((x << 1) & 0xAA);
        return x;
}

std::ostream& ConditionCrossingAirspace2::print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const
{
	os << std::endl << std::string(indent, ' ') << "ConditionCrossingAirspace2{" << get_childnum() << "} " << to_altstr() << (m_refloc ? " RefLoc" : "")
	   << std::endl << std::string(indent + 2, ' ') << (UUID)m_airspace[0];
        {
		const AirspaceTimeSlice& tsa(m_airspace[0].get_obj()->operator()(ts).as_airspace());
                if (tsa.is_valid())
                        os << ' ' << tsa.get_ident();
        }
	os << std::endl << std::string(indent + 2, ' ') << (UUID)m_airspace[1];
        {
		const AirspaceTimeSlice& tsa(m_airspace[1].get_obj()->operator()(ts).as_airspace());
                if (tsa.is_valid())
                        os << ' ' << tsa.get_ident();
        }
	return os;
}

std::string ConditionCrossingAirspace2::to_str(timetype_t tm) const
{
	std::ostringstream oss;
	oss << "XngAirspace2{" << get_childnum();
	if (m_refloc)
		oss << ",R";
	oss << "}(" << to_altstr() << ',';
	const AirspaceTimeSlice& ts0(m_airspace[0].get_obj()->operator()(tm).as_airspace());
	if (ts0.is_valid())
		oss << ts0.get_ident() << '/' << ts0.get_type();
	else
		oss << "null";
	oss << ',';
	const AirspaceTimeSlice& ts1(m_airspace[1].get_obj()->operator()(tm).as_airspace());
	if (ts1.is_valid())
		oss << ts1.get_ident() << '/' << ts1.get_type();
	else
		oss << "null";
	oss << ')';
	return oss.str();
}

void ConditionCrossingAirspace2::add_bbox(Rect& bbox, const TimeSlice& ts) const
{
	for (unsigned int a(0); a < 2; ++a) {
		const Object::const_ptr_t& aspc(m_airspace[a].get_obj());
		for (unsigned int i(0), n(aspc->size()); i < n; ++i) {
			const TimeSlice& tsaspc1(aspc->operator[](i));
			if (!tsaspc1.is_overlap(ts))
				continue;
			const AirspaceTimeSlice& tsaspc(tsaspc1.as_airspace());
			if (!tsaspc.is_valid())
				continue;
			Rect bboxa;
			tsaspc.get_bbox(bboxa);
			if (bboxa.get_southwest().is_invalid() || bboxa.get_northeast().is_invalid())
				continue;
			if (bbox.get_southwest().is_invalid() || bbox.get_northeast().is_invalid())
				bbox = bboxa;
			else
				bbox = bbox.add(bboxa);
		}
	}
}

Condition::ptr_t ConditionCrossingAirspace2::clone(void) const
{
	return ptr_t(new ConditionCrossingAirspace2(m_childnum, m_alt, m_airspace[0], m_airspace[1], m_refloc));
}

ConditionCrossingAirspace2::ptr_t ConditionCrossingAirspace2::simplify_bbox(const Rect& bbox, const timepair_t& tm) const
{
	for (unsigned int j(0); j < 2; ++j) {
		for (unsigned int i(0), n(m_airspace[j].get_obj()->size()); i < n; ++i) {
			const AirspaceTimeSlice& tsa(m_airspace[j].get_obj()->operator[](i).as_airspace());
			if (!tsa.is_valid() || !tsa.is_overlap(tm))
				continue;
			Rect bboxa;
			tsa.get_bbox(bboxa);
			if (bbox.is_intersect(bboxa))
				return ptr_t();
		}
	}
	return ptr_t(new ConditionConstant(get_childnum(), false));
}

CondResult ConditionCrossingAirspace2::evaluate(RestrictionEval& tfrs) const
{
	CondResult r(false);
	for (unsigned int wptnr = 2U, n = tfrs.wpt_size(); wptnr + 1U < n; ++wptnr) {
		const RestrictionEval::Waypoint& wpt0(tfrs.wpt(wptnr - 1U));
		const RestrictionEval::Waypoint& wpt1(tfrs.wpt(wptnr));
		if (!wpt0.is_ifr())
			continue;
		if (false && (wpt0.get_pathcode() == FPlanWaypoint::pathcode_sid ||
			      wpt0.get_pathcode() == FPlanWaypoint::pathcode_star))
			continue;
		if (ifr_refloc && m_refloc && !(wpt0.is_ifr() && wpt1.is_ifr()))
			continue;
		const AirspaceTimeSlice& ts0(m_airspace[0].get_obj()->operator()(wpt0.get_time_unix()).as_airspace());
		if (!ts0.is_valid())
			continue;
		const AirspaceTimeSlice& ts1(m_airspace[1].get_obj()->operator()(wpt1.get_time_unix()).as_airspace());
		if (!ts1.is_valid())
			continue;
		{
			TimeTableEval tte0(wpt0.get_time_unix(), wpt0.get_coord(), tfrs.get_specialdateeval());
			TimeTableEval tte1(wpt1.get_time_unix(), wpt1.get_coord(), tfrs.get_specialdateeval());
			bool inside0(ts0.is_inside(tte0, wpt0.get_altitude(), get_alt(), wpt0.get_ptuuid()));
			bool inside1(ts1.is_inside(tte1, wpt1.get_altitude(), get_alt(), wpt1.get_ptuuid()));
			if (!inside0 && !inside1)
				continue;
			int32_t alt0(std::min(wpt0.get_altitude(), wpt1.get_altitude()));
			int32_t alt1(std::max(wpt0.get_altitude(), wpt1.get_altitude()));
			bool intersect0(ts0.is_intersect(tte1, wpt0.get_coord(), alt0, alt1, get_alt()));
			bool intersect1(ts1.is_intersect(tte0, wpt1.get_coord(), alt0, alt1, get_alt()));
			if (!((inside0 && inside1) || (inside0 && intersect1) || (intersect0 && inside1)))
				continue;
		}
		if (fuzzy_exponent && fuzzy_dist > 0) {
			// fuzzy match
			bool ok(false);
			for (unsigned int fm = 1U << fuzzy_exponent; !ok;) {
				if (!fm)
					break;
				--fm;
				float crs(rev8(fm) * (360.0 / 256.0));
				TimeTableEval tte0(wpt0.get_time_unix(), wpt0.get_coord().simple_course_distance_nmi(crs, fuzzy_dist), tfrs.get_specialdateeval());
				TimeTableEval tte1(wpt1.get_time_unix(), wpt1.get_coord().simple_course_distance_nmi(crs, fuzzy_dist), tfrs.get_specialdateeval());
				ok = !ts0.is_inside(tte0, wpt0.get_altitude(), get_alt()) || !ts1.is_inside(tte1, wpt1.get_altitude(), get_alt());
			}
			if (ok)
				continue;
		}
		r.set_result(true);
		r.get_edgeset().insert(wptnr - 1U);
		r.get_xngedgeset().insert(wptnr - 1U);
		if (m_refloc)
			r.set_refloc(wptnr - 1U);
	}
	return r;
}

BidirAltRange ConditionCrossingAirspace2::evaluate_dct(DctParameters::Calc& dct) const
{
	BidirAltRange r;
	r.set_empty();
	Airspace::const_ptr_t aspc0(Airspace::ptr_t::cast_dynamic(m_airspace[0].get_obj()));
	Airspace::const_ptr_t aspc1(Airspace::ptr_t::cast_dynamic(m_airspace[1].get_obj()));
	if (!aspc0 || !aspc1)
		return r;
	if (dct.is_airport())
		return r;
	for (unsigned int i = 0; i < 2; ++i) {
		BidirAltRange::set_t inside0(aspc0->get_point_altitudes(dct.get_tte(i), get_alt(), dct.get_uuid(i)));
		BidirAltRange::set_t inside1(aspc1->get_point_altitudes(dct.get_tte(!i), get_alt(), dct.get_uuid(!i)));
		if (inside0.is_empty() && inside1.is_empty()) {
			r[i].set_empty();
			continue;
		}
		BidirAltRange::set_t intersect0(aspc0->get_point_intersect_altitudes(dct.get_tte(!i), dct.get_coord(i), get_alt(), dct.get_uuid(!i), dct.get_uuid(i)));
		BidirAltRange::set_t intersect1(aspc1->get_point_intersect_altitudes(dct.get_tte(i), dct.get_coord(!i), get_alt(), dct.get_uuid(i), dct.get_uuid(!i)));
		r[i] = (inside0 & inside1) | (inside0 & intersect1) | (intersect0 & inside1);
		if (r[i].is_empty())
			continue;
		if (!fuzzy_exponent || !(fuzzy_dist > 0))
			continue;
		// fuzzy match
		for (unsigned int fm = 1U << fuzzy_exponent;;) {
			if (!fm)
				break;
			--fm;
			float crs(rev8(fm) * (360.0 / 256.0));
			r[i] &= aspc0->get_point_altitudes(dct.get_tte(i, crs, fuzzy_dist), get_alt()) &
				aspc1->get_point_altitudes(dct.get_tte(!i, crs, fuzzy_dist), get_alt());
		}
	}
	return r;
}

Condition::forbiddenpoint_t ConditionCrossingAirspace2::evaluate_forbidden_point(const UUID& pt, const UUID& dep, const UUID& dest,
										 const TimeTableEval& tte, const Point& v1c, 
										 const UUID& v0, const UUID& v1, const UUID& awy, int32_t alt) const
{
 	const AirspaceTimeSlice& ts0(m_airspace[0].get_obj()->operator()(tte.get_time()).as_airspace());
	if (!ts0.is_valid())
		return forbiddenpoint_invalid;
	const AirspaceTimeSlice& ts1(m_airspace[1].get_obj()->operator()(tte.get_time()).as_airspace());
	if (!ts1.is_valid())
		return forbiddenpoint_invalid;
	TimeTableEval tte1(tte.get_time(), v1c, tte.get_specialdateeval());
	bool inside0(ts0.is_inside(tte, alt, get_alt(), v0));
	bool inside1(ts1.is_inside(tte1, alt, get_alt(), v1));
	if (!inside0 && !inside1)
		return forbiddenpoint_invalid;
	bool intersect0(ts0.is_intersect(tte1, tte.get_point(), alt, get_alt()));
	bool intersect1(ts1.is_intersect(tte, v1c, alt, get_alt()));
	if (!((inside0 && inside1) || (inside0 && intersect1) || (intersect0 && inside1)))
		return forbiddenpoint_invalid;
	return (pt == v0) ? forbiddenpoint_truesamesegafter : (pt == v1) ? forbiddenpoint_truesamesegbefore : forbiddenpoint_trueotherseg;
}

void ConditionCrossingAirspace2::io(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void ConditionCrossingAirspace2::io(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void ConditionCrossingAirspace2::io(ArchiveWriteStream& ar) const
{
	const_cast<ConditionCrossingAirspace2 *>(this)->hibernate(ar);
}

void ConditionCrossingAirspace2::io(LinkLoader& ar)
{
	hibernate(ar);
}

void ConditionCrossingAirspace2::io(DependencyGenerator<UUID::set_t>& ar) const
{
	const_cast<ConditionCrossingAirspace2 *>(this)->hibernate(ar);
}

void ConditionCrossingAirspace2::io(DependencyGenerator<Link::LinkSet>& ar) const
{
	const_cast<ConditionCrossingAirspace2 *>(this)->hibernate(ar);
}

ConditionCrossingDct::ConditionCrossingDct(uint32_t childnum, const AltRange& alt, const Link& wpt0, const Link& wpt1, bool refloc)
	: ConditionAltitude(childnum, alt), m_refloc(refloc)
{
	m_wpt[0] = wpt0;
	m_wpt[1] = wpt1;
}

std::ostream& ConditionCrossingDct::print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const
{
	os << std::endl << std::string(indent, ' ') << "ConditionCrossingDct{" << get_childnum() << "} " << to_altstr() << (m_refloc ? " RefLoc" : "")
	   << std::endl << std::string(indent + 2, ' ') << "Start " << (UUID)m_wpt[0];
        {
		const PointIdentTimeSlice& tsp(m_wpt[0].get_obj()->operator()(ts).as_point());
		if (tsp.is_valid())
			os << ' ' << tsp.get_ident();
        }
	os << std::endl << std::string(indent + 2, ' ') << "End   " << (UUID)m_wpt[1];
        {
		const PointIdentTimeSlice& tsp(m_wpt[1].get_obj()->operator()(ts).as_point());
		if (tsp.is_valid())
			os << ' ' << tsp.get_ident();
        }
	return os;
}

std::string ConditionCrossingDct::to_str(timetype_t tm) const
{
	std::ostringstream oss;
	oss << "XngDct{" << get_childnum();
	if (m_refloc)
		oss << ",R";
	const PointIdentTimeSlice& ts0(m_wpt[0].get_obj()->operator()(tm).as_point());
	const PointIdentTimeSlice& ts1(m_wpt[1].get_obj()->operator()(tm).as_point());
	oss << "}(" << to_altstr() << ',';
	if (ts0.is_valid())
		oss << ts0.get_ident();
	else
		oss << "?""?";
	oss << ',';
	if (ts1.is_valid())
		oss << ts1.get_ident();
	else
		oss << "?""?";
	oss << ')';
	return oss.str();
}

void ConditionCrossingDct::add_bbox(Rect& bbox, const TimeSlice& ts) const
{
	for (unsigned int i = 0; i < 2; ++i) {
		const Object::const_ptr_t& pt(m_wpt[i].get_obj());
		for (unsigned int j(0), n(pt->size()); j < n; ++j) {
			const TimeSlice& ts1(pt->operator[](j));
			if (!ts1.is_overlap(ts))
				continue;
			const PointIdentTimeSlice& tsp(ts1.as_point());
			if (!tsp.is_valid())
				continue;
			if (tsp.get_coord().is_invalid())
				continue;
			if (bbox.get_southwest().is_invalid() || bbox.get_northeast().is_invalid())
				bbox = Rect(tsp.get_coord(), tsp.get_coord());
			else
				bbox = bbox.add(tsp.get_coord());
		}
	}	
}

Condition::ptr_t ConditionCrossingDct::clone(void) const
{
	return ptr_t(new ConditionCrossingDct(m_childnum, m_alt, m_wpt[0], m_wpt[1], m_refloc));
}

ConditionCrossingDct::ptr_t ConditionCrossingDct::simplify_bbox(const Rect& bbox, const timepair_t& tm) const
{
	for (unsigned int i0(0), n0(m_wpt[0].get_obj()->size()); i0 < n0; ++i0) {
		const PointIdentTimeSlice& ts0(m_wpt[0].get_obj()->operator[](i0).as_point());
		if (!ts0.is_valid() || !ts0.is_overlap(tm))
			continue;
		if (bbox.is_inside(ts0.get_coord()))
			return ptr_t();
		for (unsigned int i1(0), n1(m_wpt[1].get_obj()->size()); i1 < n1; ++i1) {
			const PointIdentTimeSlice& ts1(m_wpt[1].get_obj()->operator[](i1).as_point());
			if (!ts1.is_valid() || !ts1.is_overlap(tm))
				continue;
			if (bbox.is_inside(ts1.get_coord()))
				return ptr_t();
			if (bbox.is_intersect(ts0.get_coord(), ts1.get_coord()))
				return ptr_t();
		}
	}
	return ptr_t(new ConditionConstant(get_childnum(), false));
}

CondResult ConditionCrossingDct::evaluate(RestrictionEval& tfrs) const
{
	CondResult r(false);
	for (unsigned int wptnr = 1U, n = tfrs.wpt_size(); wptnr < n; ++wptnr) {
		const RestrictionEval::Waypoint& wpt0(tfrs.wpt(wptnr - 1U));
		const RestrictionEval::Waypoint& wpt1(tfrs.wpt(wptnr));
		if (!wpt0.is_ifr() || !get_alt().is_inside(wpt0.get_altitude()) ||
		    (wpt0.get_pathcode() != FPlanWaypoint::pathcode_directto && !wpt0.is_stay() &&
		     (wpt0.get_pathcode() != FPlanWaypoint::pathcode_none ||
		      (wptnr > 1U && wptnr + 1U < n))))
			continue;
		if (wpt0.get_ptobj()->get_uuid() == m_wpt[0] &&
		    wpt1.get_ptobj()->get_uuid() == m_wpt[1]) {
			r.set_result(true);
			r.get_edgeset().insert(wptnr - 1U);
			r.get_xngedgeset().insert(wptnr - 1U);
			if (m_refloc)
				r.set_refloc(wptnr - 1U);
		}
	}
	return r;
}

RuleSegment ConditionCrossingDct::get_resultsequence(void) const
{
	return RuleSegment(RuleSegment::type_dct, m_alt, m_wpt[0].get_obj(), m_wpt[1].get_obj());
}

Condition::routestatic_t ConditionCrossingDct::is_routestatic(RuleSegment& seg) const
{
	if (seg.get_type() != RuleSegment::type_dct || (UUID)m_wpt[0] != seg.get_uuid0() || (UUID)m_wpt[1] != seg.get_uuid1())
		return routestatic_nonstatic;
	AltRange ar(seg.get_alt());
	ar.intersect(get_alt());
	seg.get_alt() = ar;
	if (ar.is_empty())
		return routestatic_staticfalse;
	return routestatic_statictrue;
}

BidirAltRange ConditionCrossingDct::evaluate_dct(DctParameters::Calc& dct) const
{
	BidirAltRange r;
	r.set_empty();
	for (unsigned int i = 0; i < 2; ++i) {
		if ((UUID)m_wpt[0] != dct.get_point(i)->get_uuid() || (UUID)m_wpt[1] != dct.get_point(!i)->get_uuid())
			continue;
		r[i] = BidirAltRange::set_t::Intvl(m_alt.get_lower_alt(), m_alt.get_upper_alt());
	}
	return r;
}

Condition::forbiddenpoint_t ConditionCrossingDct::evaluate_forbidden_point(const UUID& pt, const UUID& dep, const UUID& dest,
									   const TimeTableEval& tte, const Point& v1c, 
									   const UUID& v0, const UUID& v1, const UUID& awy, int32_t alt) const
{
	if ((pt == m_wpt[0] && pt == v1) || (pt == m_wpt[1] && pt == v0))
		return forbiddenpoint_invalid;
	if (awy.is_nil() && v0 == m_wpt[0] && v1 == m_wpt[1] && get_alt().is_inside(alt))
		return (pt == m_wpt[0]) ? forbiddenpoint_truesamesegafter : (pt == m_wpt[1]) ? forbiddenpoint_truesamesegbefore : forbiddenpoint_trueotherseg;
	return (pt == m_wpt[0]) ? forbiddenpoint_falsesamesegafter : (pt == m_wpt[1]) ? forbiddenpoint_falsesamesegbefore : forbiddenpoint_falseotherseg;
}

void ConditionCrossingDct::io(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void ConditionCrossingDct::io(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void ConditionCrossingDct::io(ArchiveWriteStream& ar) const
{
	const_cast<ConditionCrossingDct *>(this)->hibernate(ar);
}

void ConditionCrossingDct::io(LinkLoader& ar)
{
	hibernate(ar);
}

void ConditionCrossingDct::io(DependencyGenerator<UUID::set_t>& ar) const
{
	const_cast<ConditionCrossingDct *>(this)->hibernate(ar);
}

void ConditionCrossingDct::io(DependencyGenerator<Link::LinkSet>& ar) const
{
	const_cast<ConditionCrossingDct *>(this)->hibernate(ar);
}

ConditionCrossingAirway::ConditionCrossingAirway(uint32_t childnum, const AltRange& alt, const Link& wpt0, const Link& wpt1,
						 const Link& awy, bool refloc, const SegmentsList& segs)
	: ConditionCrossingDct(childnum, alt, wpt0, wpt1, refloc), m_awy(awy), m_segments(segs)
{
}

std::ostream& ConditionCrossingAirway::print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const
{
	os << std::endl << std::string(indent, ' ') << "ConditionCrossingAirway{" << get_childnum() << "} " << to_altstr() << (m_refloc ? " RefLoc" : "")
	   << std::endl << std::string(indent + 2, ' ') << "Start " << (UUID)m_wpt[0];
        {
		const PointIdentTimeSlice& tsp(m_wpt[0].get_obj()->operator()(ts).as_point());
		if (tsp.is_valid())
			os << ' ' << tsp.get_ident();
        }
	os << std::endl << std::string(indent + 2, ' ') << "End   " << (UUID)m_wpt[1];
        {
		const PointIdentTimeSlice& tsp(m_wpt[1].get_obj()->operator()(ts).as_point());
		if (tsp.is_valid())
			os << ' ' << tsp.get_ident();
        }
	os << std::endl << std::string(indent + 2, ' ') << "Route " << (UUID)m_awy;
        {
		const RouteTimeSlice& tsa(m_awy.get_obj()->operator()(ts).as_route());
		if (tsa.is_valid())
			os << ' ' << tsa.get_ident();
        }
	m_segments.print(os, indent + 2);
	return os;
}

std::string ConditionCrossingAirway::to_str(timetype_t tm) const
{
	std::ostringstream oss;
	oss << "XngAwy{" << get_childnum();
	if (m_refloc)
		oss << ",R";
	oss << "}(" << to_altstr() << ',';
	const PointIdentTimeSlice& ts0(m_wpt[0].get_obj()->operator()(tm).as_point());
	const PointIdentTimeSlice& ts1(m_wpt[1].get_obj()->operator()(tm).as_point());
	const IdentTimeSlice& tsa(m_awy.get_obj()->operator()(tm).as_ident());
	if (ts0.is_valid())
		oss << ts0.get_ident();
	else
		oss << "?""?";
	oss << ',';
	if (ts1.is_valid())
		oss << ts1.get_ident();
	else
		oss << "?""?";
	oss << ',';
	if (tsa.is_valid())
		oss << tsa.get_ident();
	else
		oss << "?""?";
	oss << ')';
	return oss.str();
}

void ConditionCrossingAirway::add_bbox(Rect& bbox, const TimeSlice& ts) const
{
	ConditionCrossingDct::add_bbox(bbox, ts);
	{
		Rect bbox1(m_segments.get_bbox(ts.get_timeinterval()));
		if (!bbox1.get_southwest().is_invalid() && !bbox1.get_northeast().is_invalid()) {
			if (bbox.get_southwest().is_invalid() || bbox.get_northeast().is_invalid())
				bbox = bbox1;
			else
				bbox = bbox.add(bbox1);
		}
	}
}

Condition::ptr_t ConditionCrossingAirway::clone(void) const
{
	return ptr_t(new ConditionCrossingAirway(m_childnum, m_alt, m_wpt[0], m_wpt[1], m_awy, m_refloc, m_segments));
}

Condition::ptr_t ConditionCrossingAirway::recompute(Database& db, const timepair_t& tm) const
{
	Glib::RefPtr<ConditionCrossingAirway> p(new ConditionCrossingAirway(m_childnum, m_alt, m_wpt[0], m_wpt[1], m_awy, m_refloc, SegmentsList()));
	p->m_segments.recompute(db, tm, m_wpt[0], m_wpt[1], m_awy);
	if (!p->m_segments.compare(m_segments))
		return ptr_t();
	return p;
}

ConditionCrossingAirway::ptr_t ConditionCrossingAirway::simplify_bbox(const Rect& bbox, const timepair_t& tm) const
{
	{
		Rect bbox1(m_segments.get_bbox(tm));
		if (!bbox1.get_southwest().is_invalid() && !bbox1.get_northeast().is_invalid()) {
			if (bbox.is_intersect(bbox1))
				return ptr_t();
		}
	}
	return ConditionCrossingDct::simplify_bbox(bbox, tm);
}

CondResult ConditionCrossingAirway::evaluate(RestrictionEval& tfrs) const
{
	CondResult r(false);
	for (unsigned int wptnr = 1U, n = tfrs.wpt_size(); wptnr < n; ++wptnr) {
		const RestrictionEval::Waypoint& wpt0(tfrs.wpt(wptnr - 1U));
		if (!wpt0.is_ifr() || !get_alt().is_inside(wpt0.get_altitude()))
			continue;
		if (wpt0.get_ptobj()->get_uuid() != m_wpt[0])
			continue;
		CondResult r1(true);
		unsigned int wptnre(wptnr);
		for (; wptnre < n; ++wptnre) {
			const RestrictionEval::Waypoint& wpt(tfrs.wpt(wptnre - 1U));
			const RestrictionEval::Waypoint& wpte(tfrs.wpt(wptnre));
			if (!wpt.is_ifr()) {
				r1.set_result(false);
				break;
			}
			if (!wpt.is_path_match(m_awy)) {
				r1.set_result(false);
				break;
			}
			r1.get_edgeset().insert(wptnre - 1U);
			r1.get_xngedgeset().insert(wptnre - 1U);
			if (m_refloc)
				r1.set_refloc(wptnre - 1U);
			if (wpte.get_ptobj()->get_uuid() == m_wpt[1])
				break;
		}
		if (r1.get_result() && wptnre < n) {
			r.set_result(true);
			r.get_edgeset().insert(r1.get_edgeset().begin(), r1.get_edgeset().end());
			r.get_xngedgeset().insert(r1.get_xngedgeset().begin(), r1.get_xngedgeset().end());
			if (r.has_refloc())
				r.set_refloc(r1.get_refloc());
                }
	}
	return r;
}

RuleSegment ConditionCrossingAirway::get_resultsequence(void) const
{
	return RuleSegment(RuleSegment::type_dct, m_alt, m_wpt[0].get_obj(), m_wpt[1].get_obj(), m_awy.get_obj(), m_segments);
}

Condition::routestatic_t ConditionCrossingAirway::is_routestatic(RuleSegment& seg) const
{
	if (seg.get_type() != RuleSegment::type_airway || (UUID)m_wpt[0] != seg.get_uuid0() ||
	    (UUID)m_wpt[1] != seg.get_uuid1() || (UUID)m_awy != seg.get_airway_uuid())
		return routestatic_nonstatic;
	AltRange ar(seg.get_alt());
	ar.intersect(get_alt());
	seg.get_alt() = ar;
	if (ar.is_empty())
		return routestatic_staticfalse;
	return routestatic_statictrue;
}

Condition::forbiddenpoint_t ConditionCrossingAirway::evaluate_forbidden_point(const UUID& pt, const UUID& dep, const UUID& dest,
									      const TimeTableEval& tte, const Point& v1c, 
									      const UUID& v0, const UUID& v1, const UUID& awy, int32_t alt) const
{
	unsigned int idx(0);
	if ((pt == m_wpt[0] && pt == v1) || (pt == m_wpt[1] && pt == v0))
		return forbiddenpoint_invalid;
	if (awy.is_nil() || awy != m_awy || !get_alt().is_inside(alt)) {
		if (v0 == m_wpt[0] && v1 == m_wpt[1])
		    return (pt == m_wpt[0]) ? forbiddenpoint_falsesamesegafter :
			    (pt == m_wpt[1]) ? forbiddenpoint_falsesamesegbefore :
			    forbiddenpoint_falseotherseg;
		return forbiddenpoint_false;
	}
	if (v0 == m_wpt[0] && v1 == m_wpt[1])
		return (pt == m_wpt[0]) ? forbiddenpoint_truesamesegafter :
			(pt == m_wpt[1]) ? forbiddenpoint_truesamesegbefore :
			forbiddenpoint_trueotherseg;
	// need to abort here; if this airway condition describes a multi segment crossing, it may still be true; eg. ES5572A
	return forbiddenpoint_invalid;
}

void ConditionCrossingAirway::io(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void ConditionCrossingAirway::io(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void ConditionCrossingAirway::io(ArchiveWriteStream& ar) const
{
	const_cast<ConditionCrossingAirway *>(this)->hibernate(ar);
}

void ConditionCrossingAirway::io(LinkLoader& ar)
{
	hibernate(ar);
}

void ConditionCrossingAirway::io(DependencyGenerator<UUID::set_t>& ar) const
{
	const_cast<ConditionCrossingAirway *>(this)->hibernate(ar);
}

void ConditionCrossingAirway::io(DependencyGenerator<Link::LinkSet>& ar) const
{
	const_cast<ConditionCrossingAirway *>(this)->hibernate(ar);
}

ConditionCrossingPoint::ConditionCrossingPoint(uint32_t childnum, const AltRange& alt, const Link& wpt, bool refloc)
	: ConditionAltitude(childnum, alt), m_wpt(wpt), m_refloc(refloc)
{
}

std::ostream& ConditionCrossingPoint::print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const
{
	os << std::endl << std::string(indent, ' ') << "ConditionCrossingPoint{" << get_childnum() << "} " << to_altstr() << (m_refloc ? " RefLoc" : "")
	   << std::endl << std::string(indent + 2, ' ') << (UUID)m_wpt;
	{
		const PointIdentTimeSlice& tsp(m_wpt.get_obj()->operator()(ts).as_point());
		if (tsp.is_valid())
			os << ' ' << tsp.get_ident();
	}
	return os;
}

std::string ConditionCrossingPoint::to_str(timetype_t tm) const
{
	std::ostringstream oss;
	oss << "XngPoint{" << get_childnum();
	if (m_refloc)
		oss << ",R";
	oss << "}(" << to_altstr() << ',';
	const PointIdentTimeSlice& ts(m_wpt.get_obj()->operator()(tm).as_point());
	if (ts.is_valid())
		oss << ts.get_ident();
	else
		oss << "?""?";
	oss << ')';
	return oss.str();
}

void ConditionCrossingPoint::add_bbox(Rect& bbox, const TimeSlice& ts) const
{
	const Object::const_ptr_t& pt(m_wpt.get_obj());
	for (unsigned int j(0), n(pt->size()); j < n; ++j) {
		const TimeSlice& ts1(pt->operator[](j));
		if (!ts1.is_overlap(ts))
			continue;
		const PointIdentTimeSlice& tsp(ts1.as_point());
		if (!tsp.is_valid())
			continue;
		if (tsp.get_coord().is_invalid())
			continue;
		if (bbox.get_southwest().is_invalid() || bbox.get_northeast().is_invalid())
			bbox = Rect(tsp.get_coord(), tsp.get_coord());
		else
			bbox = bbox.add(tsp.get_coord());
	}
}

Condition::ptr_t ConditionCrossingPoint::clone(void) const
{
	return ptr_t(new ConditionCrossingPoint(m_childnum, m_alt, m_wpt, m_refloc));
}

ConditionCrossingPoint::ptr_t ConditionCrossingPoint::simplify_bbox(const Rect& bbox, const timepair_t& tm) const
{
	for (unsigned int i(0), n(m_wpt.get_obj()->size()); i < n; ++i) {
		const PointIdentTimeSlice& tsp(m_wpt.get_obj()->operator[](i).as_point());
		if (!tsp.is_valid() || !tsp.is_overlap(tm))
			continue;
		if (bbox.is_inside(tsp.get_coord()))
			return ptr_t();
	}
	return ptr_t(new ConditionConstant(get_childnum(), false));
}

CondResult ConditionCrossingPoint::evaluate(RestrictionEval& tfrs) const
{
	CondResult r(false);
	for (unsigned int wptnr = 0U, n = tfrs.wpt_size(); wptnr < n; ++wptnr) {
		const RestrictionEval::Waypoint& wpt(tfrs.wpt(wptnr));
 		if (!wpt.is_ifr() && (!wptnr || !tfrs.wpt(wptnr - 1U).is_ifr()))
			continue;
		// do not match points within a procedure
		// actually EH5500A only works if we match points within a procedure
		if (false && wptnr && (wpt.get_pathcode() == FPlanWaypoint::pathcode_sid ||
				       wpt.get_pathcode() == FPlanWaypoint::pathcode_star)) {
			const RestrictionEval::Waypoint& wptp(tfrs.wpt(wptnr - 1U));
			if (wptp.get_pathcode() == FPlanWaypoint::pathcode_sid ||
			    wptp.get_pathcode() == FPlanWaypoint::pathcode_star)
				continue;
		}
		if (!get_alt().is_inside(wpt.get_altitude()) || wpt.get_ptobj()->get_uuid() != m_wpt)
			continue;
		if (ifr_refloc && m_refloc && !wpt.is_ifr())
			continue;
		r.set_result(true);
		r.get_vertexset().insert(wptnr);
		if (wptnr)
			r.get_xngedgeset().insert(wptnr - 1U);
		r.get_xngedgeset().insert(wptnr);
		if (m_refloc)
			r.set_refloc(wptnr);
	}
	return r;
}

RuleSegment ConditionCrossingPoint::get_resultsequence(void) const
{
	return RuleSegment(RuleSegment::type_point, m_alt, m_wpt.get_obj());
}

Condition::routestatic_t ConditionCrossingPoint::is_routestatic(RuleSegment& seg) const
{
	switch (seg.get_type()) {
	case RuleSegment::type_point:
		if ((UUID)m_wpt != seg.get_uuid0())
			return routestatic_nonstatic;
		break;

	case RuleSegment::type_dct:
	case RuleSegment::type_airway:
		if ((UUID)m_wpt != seg.get_uuid0() && (UUID)m_wpt != seg.get_uuid1())
			return routestatic_nonstatic;
		break;

	default:
		return routestatic_nonstatic;
	}
	AltRange ar(seg.get_alt());
	ar.intersect(get_alt());
	seg.get_alt() = ar;
	if (ar.is_empty())
		return routestatic_staticfalse;
	return routestatic_statictrue;
}

BidirAltRange ConditionCrossingPoint::evaluate_dct(DctParameters::Calc& dct) const
{
	BidirAltRange r;
	r.set_empty();
	for (unsigned int i = 0; i < 2; ++i) {
		if ((UUID)m_wpt != dct.get_point(i)->get_uuid())
			continue;
		r.set(m_alt);
		break;
	}
	return r;
}

Condition::forbiddenpoint_t ConditionCrossingPoint::evaluate_forbidden_point(const UUID& pt, const UUID& dep, const UUID& dest,
									     const TimeTableEval& tte, const Point& v1c, 
									     const UUID& v0, const UUID& v1, const UUID& awy, int32_t alt) const
{
	if ((v0 == m_wpt || v1 == m_wpt) && get_alt().is_inside(alt))
		return (pt == m_wpt) ? forbiddenpoint_truesamesegat : forbiddenpoint_trueotherseg;
	return (pt == m_wpt) ? forbiddenpoint_falsesamesegat : forbiddenpoint_falseotherseg;
}

void ConditionCrossingPoint::io(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void ConditionCrossingPoint::io(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void ConditionCrossingPoint::io(ArchiveWriteStream& ar) const
{
	const_cast<ConditionCrossingPoint *>(this)->hibernate(ar);
}

void ConditionCrossingPoint::io(LinkLoader& ar)
{
	hibernate(ar);
}

void ConditionCrossingPoint::io(DependencyGenerator<UUID::set_t>& ar) const
{
	const_cast<ConditionCrossingPoint *>(this)->hibernate(ar);
}

void ConditionCrossingPoint::io(DependencyGenerator<Link::LinkSet>& ar) const
{
	const_cast<ConditionCrossingPoint *>(this)->hibernate(ar);
}

ConditionDepArr::ConditionDepArr(uint32_t childnum, const Link& airport, bool arr, bool refloc)
	: Condition(childnum), m_airport(airport), m_arr(arr), m_refloc(refloc)
{
}

std::ostream& ConditionDepArr::print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const
{
	os << std::endl << std::string(indent, ' ') << "Condition" << (m_arr ? "Arrival" : "Departure") << '{' << get_childnum() << '}' << (m_refloc ? " RefLoc" : "")
	   << std::endl << std::string(indent + 2, ' ') << (UUID)m_airport;
	{
		const AirportTimeSlice& tsa(m_airport.get_obj()->operator()(ts).as_airport());
		if (tsa.is_valid())
			os << ' ' << tsa.get_ident();
	}
	return os;
}

std::string ConditionDepArr::to_str(timetype_t tm) const
{
	std::ostringstream oss;
	oss << (m_arr ? "Dest" : "Dep") << '{' << get_childnum();
	if (m_refloc)
		oss << ",R";
	oss << "}(";
	const AirportTimeSlice& ts(m_airport.get_obj()->operator()(tm).as_airport());
	if (ts.is_valid())
		oss << ts.get_ident();
	else
		oss << "?""?";
	oss << ')';
	return oss.str();
}

void ConditionDepArr::add_bbox(Rect& bbox, const TimeSlice& ts) const
{
	const Object::const_ptr_t& pt(m_airport.get_obj());
	for (unsigned int j(0), n(pt->size()); j < n; ++j) {
		const TimeSlice& ts1(pt->operator[](j));
		if (!ts1.is_overlap(ts))
			continue;
		const PointIdentTimeSlice& tsp(ts1.as_point());
		if (!tsp.is_valid())
			continue;
		if (tsp.get_coord().is_invalid())
			continue;
		if (bbox.get_southwest().is_invalid() || bbox.get_northeast().is_invalid())
			bbox = Rect(tsp.get_coord(), tsp.get_coord());
		else
			bbox = bbox.add(tsp.get_coord());
	}
}

Condition::ptr_t ConditionDepArr::clone(void) const
{
	return ptr_t(new ConditionDepArr(m_childnum, m_airport, m_arr, m_refloc));
}

ConditionDepArr::ptr_t ConditionDepArr::simplify_bbox(const Rect& bbox, const timepair_t& tm) const
{
	for (unsigned int i(0), n(m_airport.get_obj()->size()); i < n; ++i) {
		const AirportTimeSlice& tsa(m_airport.get_obj()->operator[](i).as_airport());
		if (!tsa.is_valid() || !tsa.is_overlap(tm))
			continue;
		if (bbox.is_inside(tsa.get_coord()))
			return ptr_t();
	}
	return ptr_t(new ConditionConstant(get_childnum(), false));
}

ConditionDepArr::ptr_t ConditionDepArr::simplify_dep(const Link& arpt, const timepair_t& tm) const
{
	if (m_arr)
		return ptr_t();
	bool x((UUID)arpt == (UUID)m_airport);
	if (x && m_refloc)
		return ptr_t();
	return ptr_t(new ConditionConstant(get_childnum(), x));
}

ConditionDepArr::ptr_t ConditionDepArr::simplify_dest(const Link& arpt, const timepair_t& tm) const
{
	if (!m_arr)
		return ptr_t();
	bool x((UUID)arpt == (UUID)m_airport);
	if (x && m_refloc)
		return ptr_t();
	return ptr_t(new ConditionConstant(get_childnum(), x));
}

CondResult ConditionDepArr::evaluate(RestrictionEval& tfrs) const
{
	unsigned int idx(m_arr ? tfrs.wpt_size() - 1 : 0);
	const RestrictionEval::Waypoint& wpt(tfrs.wpt(idx));
	if (m_refloc && !wpt.is_ifr())
		return CondResult(false);
	if (wpt.get_ptobj() && wpt.get_ptobj()->get_uuid() == m_airport) {
		CondResult r(true);
		r.get_vertexset().insert(idx);
		r.get_xngedgeset().insert(m_arr ? tfrs.wpt_size() - 2 : 0);
		if (m_refloc)
			r.set_refloc(idx);
		return r;
	}
	return CondResult(false);
}

bool ConditionDepArr::is_deparr(std::set<Link>& dep, std::set<Link>& dest) const
{
	if (m_arr)
		dest.insert(m_airport);
	else
		dep.insert(m_airport);
	return true;
}

bool ConditionDepArr::is_deparr_dct(Link& arpt, bool& arr, double& dist, civmil_t& civmil) const
{
	arpt = m_airport;
	arr = m_arr;
	return true;
}

Condition::forbiddenpoint_t ConditionDepArr::evaluate_forbidden_point(const UUID& pt, const UUID& dep, const UUID& dest,
								      const TimeTableEval& tte, const Point& v1c, 
								      const UUID& v0, const UUID& v1, const UUID& awy, int32_t alt) const
{
	if (m_airport == (m_arr ? dest : dep))
		return forbiddenpoint_true;
	return forbiddenpoint_false;
}

void ConditionDepArr::io(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void ConditionDepArr::io(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void ConditionDepArr::io(ArchiveWriteStream& ar) const
{
	const_cast<ConditionDepArr *>(this)->hibernate(ar);
}

void ConditionDepArr::io(LinkLoader& ar)
{
	hibernate(ar);
}

void ConditionDepArr::io(DependencyGenerator<UUID::set_t>& ar) const
{
	const_cast<ConditionDepArr *>(this)->hibernate(ar);
}

void ConditionDepArr::io(DependencyGenerator<Link::LinkSet>& ar) const
{
	const_cast<ConditionDepArr *>(this)->hibernate(ar);
}

ConditionDepArrAirspace::ConditionDepArrAirspace(uint32_t childnum, const Link& aspc, bool arr, bool refloc)
	: Condition(childnum), m_airspace(aspc), m_arr(arr), m_refloc(refloc)
{
}

std::ostream& ConditionDepArrAirspace::print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const
{
	os << std::endl << std::string(indent, ' ') << "Condition" << (m_arr ? "Arrival" : "Departure") << "Airspace{" << get_childnum() << '}' << (m_refloc ? " RefLoc" : "")
	   << std::endl << std::string(indent + 2, ' ') << (UUID)m_airspace;
	{
		const AirspaceTimeSlice& tsa(m_airspace.get_obj()->operator()(ts).as_airspace());
		if (tsa.is_valid())
			os << ' ' << tsa.get_ident();
	}
	return os;
}

std::string ConditionDepArrAirspace::to_str(timetype_t tm) const
{
	std::ostringstream oss;
	oss << (m_arr ? "DestAirspace" : "DepAirspace") << '{' << get_childnum();
	if (m_refloc)
		oss << ",R";
	oss << "}(";
	const AirspaceTimeSlice& ts(m_airspace.get_obj()->operator()(tm).as_airspace());
	if (ts.is_valid())
		oss << ts.get_ident() << '/' << ts.get_type();
	else
		oss << "?""?";
	oss << ')';
	return oss.str();
}

void ConditionDepArrAirspace::add_bbox(Rect& bbox, const TimeSlice& ts) const
{
	const Object::const_ptr_t& aspc(m_airspace.get_obj());
	for (unsigned int i(0), n(aspc->size()); i < n; ++i) {
		const TimeSlice& tsaspc1(aspc->operator[](i));
		if (!tsaspc1.is_overlap(ts))
			continue;
		const AirspaceTimeSlice& tsaspc(tsaspc1.as_airspace());
		if (!tsaspc.is_valid())
		        continue;
		Rect bboxa;
		tsaspc.get_bbox(bboxa);
		if (bboxa.get_southwest().is_invalid() || bboxa.get_northeast().is_invalid())
			continue;
		if (bbox.get_southwest().is_invalid() || bbox.get_northeast().is_invalid())
			bbox = bboxa;
		else
			bbox = bbox.add(bboxa);
	}
}

Condition::ptr_t ConditionDepArrAirspace::clone(void) const
{
	return ptr_t(new ConditionDepArrAirspace(m_childnum, m_airspace, m_arr, m_refloc));
}

ConditionDepArrAirspace::ptr_t ConditionDepArrAirspace::simplify_bbox(const Rect& bbox, const timepair_t& tm) const
{
	for (unsigned int i(0), n(m_airspace.get_obj()->size()); i < n; ++i) {
		const AirspaceTimeSlice& tsa(m_airspace.get_obj()->operator[](i).as_airspace());
		if (!tsa.is_valid() || !tsa.is_overlap(tm))
			continue;
		Rect bboxa;
		tsa.get_bbox(bboxa);
		if (bbox.is_intersect(bboxa))
			return ptr_t();
	}
	return ptr_t(new ConditionConstant(get_childnum(), false));
}

ConditionDepArrAirspace::ptr_t ConditionDepArrAirspace::simplify_dep(const Link& arpt, const timepair_t& tm) const
{
	if (m_arr || !arpt.get_obj())
		return ptr_t();
	for (unsigned int i0(0), n0(arpt.get_obj()->size()); i0 < n0; ++i0) {
		const PointIdentTimeSlice& tsp(arpt.get_obj()->operator[](i0).as_point());
		if (!tsp.is_valid() || !tsp.is_overlap(tm))
			continue;
		for (unsigned int i1(0), n1(m_airspace.get_obj()->size()); i1 < n1; ++i1) {
			const AirspaceTimeSlice& tsa(m_airspace.get_obj()->operator[](i1).as_airspace());
			if (!tsa.is_valid() || !tsa.is_overlap(tm))
				continue;
			Rect bboxa;
			tsa.get_bbox(bboxa);
			if (bboxa.is_inside(tsp.get_coord()))
				return ptr_t();
		}
	}
	return ptr_t(new ConditionConstant(get_childnum(), false));
}

ConditionDepArrAirspace::ptr_t ConditionDepArrAirspace::simplify_dest(const Link& arpt, const timepair_t& tm) const
{
	if (!m_arr || !arpt.get_obj())
		return ptr_t();
	for (unsigned int i0(0), n0(arpt.get_obj()->size()); i0 < n0; ++i0) {
		const PointIdentTimeSlice& tsp(arpt.get_obj()->operator[](i0).as_point());
		if (!tsp.is_valid() || !tsp.is_overlap(tm))
			continue;
		for (unsigned int i1(0), n1(m_airspace.get_obj()->size()); i1 < n1; ++i1) {
			const AirspaceTimeSlice& tsa(m_airspace.get_obj()->operator[](i1).as_airspace());
			if (!tsa.is_valid() || !tsa.is_overlap(tm))
				continue;
			Rect bboxa;
			tsa.get_bbox(bboxa);
			if (bboxa.is_inside(tsp.get_coord()))
				return ptr_t();
		}
	}
	return ptr_t(new ConditionConstant(get_childnum(), false));
}

CondResult ConditionDepArrAirspace::evaluate(RestrictionEval& tfrs) const
{
	unsigned int idx(m_arr ? tfrs.wpt_size() - 1 : 0);
	const RestrictionEval::Waypoint& wpt(tfrs.wpt(idx));
	if (m_refloc && !wpt.is_ifr())
		return CondResult(false);
	const AirspaceTimeSlice& ts(m_airspace.get_obj()->operator()(wpt.get_time_unix()).as_airspace());
	if (!ts.is_valid())
		return CondResult(false);
	TimeTableEval tte(wpt.get_time_unix(), wpt.get_coord(), tfrs.get_specialdateeval());
	if (!ts.is_inside(tte, AltRange::altignore))
		return CondResult(false);
	CondResult r(true);
	r.get_vertexset().insert(idx);
	r.get_xngedgeset().insert(m_arr ? tfrs.wpt_size() - 2 : 0);
	if (m_refloc)
		r.set_refloc(idx);
	return r;
}

void ConditionDepArrAirspace::io(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void ConditionDepArrAirspace::io(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void ConditionDepArrAirspace::io(ArchiveWriteStream& ar) const
{
	const_cast<ConditionDepArrAirspace *>(this)->hibernate(ar);
}

void ConditionDepArrAirspace::io(LinkLoader& ar)
{
	hibernate(ar);
}

void ConditionDepArrAirspace::io(DependencyGenerator<UUID::set_t>& ar) const
{
	const_cast<ConditionDepArrAirspace *>(this)->hibernate(ar);
}

void ConditionDepArrAirspace::io(DependencyGenerator<Link::LinkSet>& ar) const
{
	const_cast<ConditionDepArrAirspace *>(this)->hibernate(ar);
}

ConditionSidStar::ConditionSidStar(uint32_t childnum, const Link& proc, bool star, bool refloc)
	: Condition(childnum), m_proc(proc), m_star(star), m_refloc(refloc)
{
}

std::ostream& ConditionSidStar::print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const
{
	os << std::endl << std::string(indent, ' ') << "Condition" << (m_star ? "STAR" : "SID") << '{' << get_childnum() << '}' << (m_refloc ? " RefLoc" : "")
	   << std::endl << std::string(indent + 2, ' ') << (UUID)m_proc;
	{
		const StandardInstrumentTimeSlice& tsp(m_proc.get_obj()->operator()(ts).as_sidstar());
		if (tsp.is_valid()) {
			os << ' ' << tsp.get_ident() << std::string(indent + 2, ' ') << (UUID)tsp.get_airport();
			const PointIdentTimeSlice& tsa(tsp.get_airport().get_obj()->operator()(ts).as_point());
			if (tsa.is_valid())
				os << ' ' << tsa.get_ident();
		}
	}
	return os;
}

std::string ConditionSidStar::to_str(timetype_t tm) const
{
	std::ostringstream oss;
	oss << (m_star ? "Star" : "Sid") << '{' << get_childnum() << "}(";
	const StandardInstrumentTimeSlice& ts(m_proc.get_obj()->operator()(tm).as_sidstar());
	const AirportTimeSlice& tsa(ts.get_airport().get_obj()->operator()(tm).as_airport());
	if (tsa.is_valid())
		oss << tsa.get_ident();
	else
		oss << "?""?";
	oss << ',';
	if (ts.is_valid())
		oss << ts.get_ident();
	else
		oss << "?""?";
	oss << ')';
	return oss.str();
}

void ConditionSidStar::add_bbox(Rect& bbox, const TimeSlice& ts) const
{
	const Object::const_ptr_t& instr(m_proc.get_obj());
	for (unsigned int i(0), n(instr->size()); i < n; ++i) {
		const TimeSlice& tsinstr1(instr->operator[](i));
		if (!tsinstr1.is_overlap(ts))
			continue;
		const StandardInstrumentTimeSlice& tsinstr(tsinstr1.as_sidstar());
		if (!tsinstr.is_valid())
			continue;
		const Object::const_ptr_t& arpt(tsinstr.get_airport().get_obj());
		for (unsigned int j(0), n(arpt->size()); j < n; ++j) {
			const TimeSlice& tsarpt1(arpt->operator[](j));
			if (!tsarpt1.is_overlap(ts))
				continue;
			const PointIdentTimeSlice& tsarpt(tsarpt1.as_point());
			if (!tsarpt.is_valid())
				continue;
			if (tsarpt.get_coord().is_invalid())
				continue;
			if (bbox.get_southwest().is_invalid() || bbox.get_northeast().is_invalid())
				bbox = Rect(tsarpt.get_coord(), tsarpt.get_coord());
			else
				bbox = bbox.add(tsarpt.get_coord());
		}
	}
}

Condition::ptr_t ConditionSidStar::clone(void) const
{
	return ptr_t(new ConditionSidStar(m_childnum, m_proc, m_star, m_refloc));
}

ConditionDepArr::ptr_t ConditionSidStar::simplify_bbox(const Rect& bbox, const timepair_t& tm) const
{
	for (unsigned int i0(0), n0(m_proc.get_obj()->size()); i0 < n0; ++i0) {
		const StandardInstrumentTimeSlice& tsp(m_proc.get_obj()->operator[](i0).as_sidstar());
		if (!tsp.is_valid() || !tsp.is_overlap(tm))
			continue;
		for (unsigned int i1(0), n1(tsp.get_airport().get_obj()->size()); i1 < n1; ++i1) {
			const AirportTimeSlice& tsa(tsp.get_airport().get_obj()->operator[](i1).as_airport());
			if (!tsa.is_valid() || !tsa.is_overlap(tm) || !tsa.is_overlap(tsp))
				continue;
			if (bbox.is_inside(tsa.get_coord()))
				return ptr_t();
		}
	}
	return ptr_t(new ConditionConstant(get_childnum(), false));
}

ConditionSidStar::ptr_t ConditionSidStar::simplify_dep(const Link& arpt, const timepair_t& tm) const
{
	if (m_star)
		return ptr_t();
	for (unsigned int i(0), n(m_proc.get_obj()->size()); i < n; ++i) {
		const StandardInstrumentTimeSlice& tsp(m_proc.get_obj()->operator[](i).as_sidstar());
		if (!tsp.is_valid() || !tsp.is_overlap(tm))
			continue;
		if (tsp.get_airport() == (UUID)arpt)
			return ptr_t();
	}
	return ptr_t(new ConditionConstant(get_childnum(), false));
}

ConditionSidStar::ptr_t ConditionSidStar::simplify_dest(const Link& arpt, const timepair_t& tm) const
{
	if (!m_star)
		return ptr_t();
	for (unsigned int i(0), n(m_proc.get_obj()->size()); i < n; ++i) {
		const StandardInstrumentTimeSlice& tsp(m_proc.get_obj()->operator[](i).as_sidstar());
		if (!tsp.is_valid() || !tsp.is_overlap(tm))
			continue;
		if (tsp.get_airport() == (UUID)arpt)
			return ptr_t();
	}
	return ptr_t(new ConditionConstant(get_childnum(), false));
}

CondResult ConditionSidStar::evaluate(RestrictionEval& tfrs) const
{
	unsigned int idx(m_star ? tfrs.wpt_size() - 1 : 0);
	unsigned int idxe(m_star ? tfrs.wpt_size() - 2 : 0);
	const RestrictionEval::Waypoint& wpt(tfrs.wpt(idx));
	const RestrictionEval::Waypoint& wpte(tfrs.wpt(idxe));
	if (!wpt.is_ifr())
		return CondResult(false);
	if (!wpte.is_path_match(m_proc))
		return CondResult(false);
	CondResult r(true);
	if (m_star) {
		r.get_vertexset().insert(idx);
		r.get_vertexset().insert(idxe);
		r.get_edgeset().insert(idxe);
		r.get_xngedgeset().insert(idxe);
		r.set_refloc(idxe);
		while (idxe) {
			--idxe;
			const RestrictionEval::Waypoint& wpte(tfrs.wpt(idxe));
			if (!wpte.is_path_match(m_proc))
				break;
			r.get_vertexset().insert(idxe);
			r.get_edgeset().insert(idxe);
			r.get_xngedgeset().insert(idxe);
			r.set_refloc(idxe);
		}
	} else {
		r.get_vertexset().insert(idxe);
		r.get_vertexset().insert(idxe + 1);
		r.get_edgeset().insert(idxe);
		r.get_xngedgeset().insert(idxe);
		r.set_refloc(idxe);
		for (;;) {
			++idxe;
			if (idxe + 1 >= tfrs.wpt_size())
				break;
			const RestrictionEval::Waypoint& wpte(tfrs.wpt(idxe));
			if (!wpte.is_path_match(m_proc))
				break;
			r.get_vertexset().insert(idxe + 1);
			r.get_edgeset().insert(idxe);
			r.get_xngedgeset().insert(idxe);
		}
	}
	return r;
}

void ConditionSidStar::io(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void ConditionSidStar::io(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void ConditionSidStar::io(ArchiveWriteStream& ar) const
{
	const_cast<ConditionSidStar *>(this)->hibernate(ar);
}

void ConditionSidStar::io(LinkLoader& ar)
{
	hibernate(ar);
}

void ConditionSidStar::io(DependencyGenerator<UUID::set_t>& ar) const
{
	const_cast<ConditionSidStar *>(this)->hibernate(ar);
}

void ConditionSidStar::io(DependencyGenerator<Link::LinkSet>& ar) const
{
	const_cast<ConditionSidStar *>(this)->hibernate(ar);
}

ConditionCrossingAirspaceActive::ConditionCrossingAirspaceActive(uint32_t childnum, const Link& aspc)
	: Condition(childnum), m_airspace(aspc)
{
}

std::ostream& ConditionCrossingAirspaceActive::print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const
{
	os << std::endl << std::string(indent, ' ') << "ConditionCrossingAirspaceActive{" << get_childnum() << '}'
	   << std::endl << std::string(indent + 2, ' ') << (UUID)m_airspace;
	{
		const AirspaceTimeSlice& tsa(m_airspace.get_obj()->operator()(ts).as_airspace());
		if (tsa.is_valid())
			os << ' ' << tsa.get_ident();
	}
	return os;
}

std::string ConditionCrossingAirspaceActive::to_str(timetype_t tm) const
{
	std::ostringstream oss;
	oss << "AirspaceAct{" << get_childnum() << "}(";
	const AirspaceTimeSlice& ts(m_airspace.get_obj()->operator()(tm).as_airspace());
	if (ts.is_valid())
		oss << ts.get_ident() << '/' << ts.get_type();
	else
		oss << "?""?";
	oss << ')';
	return oss.str();
}

Condition::ptr_t ConditionCrossingAirspaceActive::clone(void) const
{
	return ptr_t(new ConditionCrossingAirspaceActive(m_childnum, m_airspace));
}

ConditionCrossingAirspaceActive::ptr_t ConditionCrossingAirspaceActive::simplify_conditionalavailability(const Graph& g, const ConditionalAvailability& condavail,
													 const TimeTableSpecialDateEval& tsde, const timepair_t& tm) const
{
	ConditionalAvailability::results_t r(condavail.find(m_airspace, tm.first, tm.second));
	bool ok(false);
	for (ConditionalAvailability::results_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
		AUPRSA::const_ptr_t p(AUPRSA::const_ptr_t::cast_dynamic(*ri));
		if (!p || p->get_obj() != m_airspace)
			continue;
		if (p->get_activation().get_status() != AUPRSA::Activation::status_active)
			continue;
		ok = true;
		break;
	}
	if (false) {
		const AirspaceTimeSlice& ts(m_airspace.get_obj()->operator()(tm.first).as_airspace());
		std::cerr << "Airspace Active: " << m_airspace << ' ' << ts.get_ident() << ' '
			  << Glib::TimeVal(tm.first, 0).as_iso8601() << ".." << Glib::TimeVal(tm.second, 0).as_iso8601()
			  << ' ' << (ok ? "active" : "inactive") << std::endl;
	}
	if (false && !ok) {
		timeset_t tdisc(m_airspace.get_obj()->timediscontinuities());
		tdisc.insert(tm.first);
		tdisc.insert(tm.second);
		for (timeset_t::iterator ti(tdisc.begin()); ti != tdisc.end(); ++ti) {
			if (*ti < tm.first)
				continue;
			if (*ti >= tm.second)
				break;
			const AirspaceTimeSlice& ts(m_airspace.get_obj()->operator()(*ti).as_airspace());
			if (!ts.is_valid())
				continue;
			TimeTableEval tte(*ti, Point::invalid, tsde);
			if (!ts.get_timetable().is_inside(tte))
				continue;
			ok = true;
			break;
		}
	}
	return ptr_t(new ConditionConstant(get_childnum(), ok));
}

CondResult ConditionCrossingAirspaceActive::evaluate(RestrictionEval& tfrs) const
{
	AUPRSA::const_ptr_t p(AUPRSA::const_ptr_t::cast_dynamic(tfrs.get_condavail().find(m_airspace, tfrs.get_departuretime())));
	bool ok(p && p->get_obj() == m_airspace && p->get_activation().get_status() == AUPRSA::Activation::status_active);
	if (false && !ok) {
		const AirspaceTimeSlice& ts(m_airspace.get_obj()->operator()(tfrs.get_departuretime()).as_airspace());
		if (ts.is_valid()) {
			TimeTableEval tte(tfrs.get_departuretime(), Point::invalid, tfrs.get_specialdateeval());
			ok = ts.get_timetable().is_inside(tte);
		}
	}
	if (true) {
		std::ostringstream oss;
		oss << "Airspace Active: " << m_airspace;
		{
			const AirspaceTimeSlice& ts(m_airspace.get_obj()->operator()(tfrs.get_departuretime()).as_airspace());
			if (ts.is_valid())
				oss << ' ' << ts.get_ident();
		}
		oss << ' ' << Glib::TimeVal(tfrs.get_departuretime(), 0).as_iso8601() << " result " << (ok ? "ok" : "NOT ok");
		Message msg(oss.str(), Message::type_trace);
		tfrs.message(msg);
		if (true)
			std::cerr << oss.str() << std::endl;
	}
	return CondResult(ok);
}

void ConditionCrossingAirspaceActive::io(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void ConditionCrossingAirspaceActive::io(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void ConditionCrossingAirspaceActive::io(ArchiveWriteStream& ar) const
{
	const_cast<ConditionCrossingAirspaceActive *>(this)->hibernate(ar);
}

void ConditionCrossingAirspaceActive::io(LinkLoader& ar)
{
	hibernate(ar);
}

void ConditionCrossingAirspaceActive::io(DependencyGenerator<UUID::set_t>& ar) const
{
	const_cast<ConditionCrossingAirspaceActive *>(this)->hibernate(ar);
}

void ConditionCrossingAirspaceActive::io(DependencyGenerator<Link::LinkSet>& ar) const
{
	const_cast<ConditionCrossingAirspaceActive *>(this)->hibernate(ar);
}

ConditionCrossingAirwayAvailable::ConditionCrossingAirwayAvailable(uint32_t childnum, const AltRange& alt, const Link& wpt0, const Link& wpt1,
								   const Link& awy)
	: ConditionAltitude(childnum, alt), m_awy(awy)
{
	m_wpt[0] = wpt0;
	m_wpt[1] = wpt1;
}

std::ostream& ConditionCrossingAirwayAvailable::print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const
{
	os << std::endl << std::string(indent, ' ') << "ConditionCrossingAirwayAvailable{" << get_childnum() << "} " << to_altstr()
	   << std::endl << std::string(indent + 2, ' ') << "Start " << (UUID)m_wpt[0];
        {
		const PointIdentTimeSlice& tsp(m_wpt[0].get_obj()->operator()(ts).as_point());
		if (tsp.is_valid())
			os << ' ' << tsp.get_ident();
        }
	os << std::endl << std::string(indent + 2, ' ') << "End   " << (UUID)m_wpt[1];
        {
		const PointIdentTimeSlice& tsp(m_wpt[1].get_obj()->operator()(ts).as_point());
		if (tsp.is_valid())
			os << ' ' << tsp.get_ident();
        }
	os << std::endl << std::string(indent + 2, ' ') << "Route " << (UUID)m_awy;
        {
		const RouteTimeSlice& tsa(m_awy.get_obj()->operator()(ts).as_route());
		if (tsa.is_valid())
			os << ' ' << tsa.get_ident();
        }
	return os;
}

std::string ConditionCrossingAirwayAvailable::to_str(timetype_t tm) const
{
	std::ostringstream oss;
	oss << "AwyAvbl{" << get_childnum() << "}(" << to_altstr() << ',';
	const PointIdentTimeSlice& ts0(m_wpt[0].get_obj()->operator()(tm).as_point());
	const PointIdentTimeSlice& ts1(m_wpt[1].get_obj()->operator()(tm).as_point());
	const IdentTimeSlice& tsa(m_awy.get_obj()->operator()(tm).as_ident());
	if (ts0.is_valid())
		oss << ts0.get_ident();
	else
		oss << "?""?";
	oss << ',';
	if (ts1.is_valid())
		oss << ts1.get_ident();
	else
		oss << "?""?";
	oss << ',';
	if (tsa.is_valid())
		oss << tsa.get_ident();
	else
		oss << "?""?";
	oss << ')';
	return oss.str();
}

Condition::ptr_t ConditionCrossingAirwayAvailable::clone(void) const
{
	return ptr_t(new ConditionCrossingAirwayAvailable(m_childnum, m_alt, m_wpt[0], m_wpt[1], m_awy));
}

int ConditionCrossingAirwayAvailable::compute_crossing_band(IntervalSet<int32_t>& r, const Graph& g, const ConditionalAvailability& condavail,
							    const TimeTableSpecialDateEval& tsde, timetype_t tm, timetype_t& tuntil) const
{
	tuntil = std::numeric_limits<timetype_t>::max();
	r.set_empty();
	Graph::vertex_descriptor v[2];
	for (unsigned int i(0); i < 2; ++i) {
		bool ok;
		boost::tie(v[i], ok) = g.find_vertex(m_wpt[i]);
		if (ok)
			continue;
		return -1 - i;
	}
	Graph::UUIDEdgeFilter filter(g, m_awy, GraphEdge::matchcomp_segment);
	typedef boost::filtered_graph<Graph, Graph::UUIDEdgeFilter> fg_t;
	fg_t fg(g, filter);
	r.set_full();
	std::vector<Graph::vertex_descriptor> predecessors(boost::num_vertices(fg), 0);
	Graph::TimeEdgeDistanceMap wmap(g, tm);
	dijkstra_shortest_paths(fg, v[0], boost::weight_map(wmap).predecessor_map(&predecessors[0]));
	if (predecessors[v[1]] == v[1])
		return -3;
	{
		Graph::vertex_descriptor vs(v[1]);
		while (vs != v[0]) {
			Graph::vertex_descriptor us(predecessors[vs]);
			if (us == vs)
				break;
			IntervalSet<int32_t> rs;
			rs.set_empty();
			fg_t::out_edge_iterator e0, e1;
			for(boost::tie(e0, e1) = boost::out_edges(us, fg); e0 != e1; ++e0) {
				if (boost::target(*e0, fg) != vs)
					continue;
				const PointIdentTimeSlice& tsu(fg[us].get_object()->operator()(tm).as_point());
				TimeTableEval tte(tm, tsu.get_coord(), tsde);
				timetype_t tuntil1;
				rs |= fg[*e0].get_altrange(condavail, tte, tuntil1);
				tuntil = std::min(tuntil, tuntil1);
			}
			r &= rs;
			vs = us;
		}
	}
	return 0;
}

ConditionCrossingAirwayAvailable::ptr_t ConditionCrossingAirwayAvailable::simplify_conditionalavailability(const Graph& g, const ConditionalAvailability& condavail,
													   const TimeTableSpecialDateEval& tsde, const timepair_t& tm) const
{
	// get time discontinuities
	timeset_t tdisc(m_awy.get_obj()->timediscontinuities());
	for (unsigned int i(0); i < 2; ++i) {
		timeset_t tdisc1(m_wpt[i].get_obj()->timediscontinuities());
		tdisc.insert(tdisc1.begin(), tdisc1.end());
	}
	tdisc.insert(tm.first);
	tdisc.insert(tm.second);
	IntervalSet<int32_t> r;
	r.set_full();
	IntervalSet<int32_t> ralt(get_alt().get_interval());
	for (timeset_t::iterator ti(tdisc.begin()); ti != tdisc.end(); ++ti) {
		if (*ti < tm.first)
			continue;
		if (*ti >= tm.second)
			break;
		timeset_t::iterator ti2(ti);
		++ti2;
		if (ti2 == tdisc.end())
			break;
		timetype_t tuntil;
		IntervalSet<int32_t> r1;
		int err(compute_crossing_band(r1, g, condavail, tsde, *ti, tuntil));
		if (tuntil > *ti && tuntil < *ti2)
			tdisc.insert(tuntil);
		if (true) {
			switch (err) {
			case -1:
			case -2:
				std::cerr << "Cannot evaluate " << to_str(*ti) << " due to missing graph vertex " << m_wpt[-1-err] << std::endl;
				break;

			case -3:
				std::cerr << "Cannot evaluate " << to_str(*ti) << " due to no path from " << m_wpt[0]
					  << " to " << m_wpt[1] << " on airway " << m_awy << std::endl;
				break;

			default:
				break;
			}
		}
		r &= r1;
		if (true)
			std::cerr << "Airway Available: " << m_wpt[0] << ' ' << m_wpt[1] << ' ' << m_awy
				  << " (" << to_str(*ti) << ") requested alt " << ralt.to_str()
				  << " time " << Glib::TimeVal(*ti, 0).as_iso8601() << " alt " << r1.to_str()
				  << " available alt " << r.to_str() << std::endl;
	}
	bool ok(ralt == (r & ralt));
	return ptr_t(new ConditionConstant(get_childnum(), ok));
}

CondResult ConditionCrossingAirwayAvailable::evaluate(RestrictionEval& tfrs) const
{
	timetype_t tm(tfrs.get_departuretime());
	if (!tfrs.get_graph()) {
		std::ostringstream oss;
		oss << "Cannot evaluate " << to_str(tm) << " due to missing graph";
		Message msg(oss.str(), Message::type_warning);
		tfrs.message(msg);
		return CondResult(false);
	}
	IntervalSet<int32_t> r;
	const Graph& g(*tfrs.get_graph());
	timetype_t tuntil;
	int err(compute_crossing_band(r, g, tfrs.get_condavail(), tfrs.get_specialdateeval(), tm, tuntil));
	if (true) {
		switch (err) {
		case -1:
		case -2:
		{
			std::ostringstream oss;
			oss << "Cannot evaluate " << to_str(tm) << " due to missing graph vertex " << m_wpt[-1-err];
			Message msg(oss.str(), Message::type_warning);
			tfrs.message(msg);
			break;
		}

		case -3:
		{
			std::ostringstream oss;
			oss << "Cannot evaluate " << to_str(tm) << " due to no path from " << m_wpt[0]
			    << " to " << m_wpt[1] << " on airway " << m_awy;
			Message msg(oss.str(), Message::type_warning);
			tfrs.message(msg);
			break;
		}

		default:
			break;
		}
	}
	IntervalSet<int32_t> ralt(get_alt().get_interval());
	bool ok(ralt == (r & ralt));
	if (true) {
		std::ostringstream oss;
		oss << "Airway Available: " << m_wpt[0] << ' ' << m_wpt[1] << ' ' << m_awy
		    << " (" << to_str(tm) << ") requested alt " << ralt.to_str()
		    << " available alt " << r.to_str() << " result " << (ok ? "ok" : "NOT ok");
		Message msg(oss.str(), Message::type_trace);
		tfrs.message(msg);
		if (true)
			std::cerr << oss.str() << std::endl;
	}
	return CondResult(ok);
}

void ConditionCrossingAirwayAvailable::io(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void ConditionCrossingAirwayAvailable::io(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void ConditionCrossingAirwayAvailable::io(ArchiveWriteStream& ar) const
{
	const_cast<ConditionCrossingAirwayAvailable *>(this)->hibernate(ar);
}

void ConditionCrossingAirwayAvailable::io(LinkLoader& ar)
{
	hibernate(ar);
}

void ConditionCrossingAirwayAvailable::io(DependencyGenerator<UUID::set_t>& ar) const
{
	const_cast<ConditionCrossingAirwayAvailable *>(this)->hibernate(ar);
}

void ConditionCrossingAirwayAvailable::io(DependencyGenerator<Link::LinkSet>& ar) const
{
	const_cast<ConditionCrossingAirwayAvailable *>(this)->hibernate(ar);
}

ConditionDctLimit::ConditionDctLimit(uint32_t childnum, double limit)
	: Condition(childnum), m_dctlimit(limit)
{
}

std::ostream& ConditionDctLimit::print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const
{
	os << std::endl << std::string(indent, ' ') << "ConditionDctLimit{" << get_childnum() << "} dctlimit " << m_dctlimit;
	return os;
}

std::string ConditionDctLimit::to_str(timetype_t tm) const
{
	std::ostringstream oss;
	oss << "DctLimit{" << get_childnum() << "}(" << m_dctlimit << ')';
	return oss.str();
}

Condition::ptr_t ConditionDctLimit::clone(void) const
{
	return ptr_t(new ConditionDctLimit(m_childnum, m_dctlimit));
}

CondResult ConditionDctLimit::evaluate(RestrictionEval& tfrs) const
{
	// FIXME
	CondResult r(false);
	for (unsigned int wptnr = 1U, n = tfrs.wpt_size(); wptnr < n; ++wptnr) {
		const RestrictionEval::Waypoint& wpt0(tfrs.wpt(wptnr - 1));
		if (!wpt0.is_ifr() || (wpt0.get_pathcode() != FPlanWaypoint::pathcode_directto && !wpt0.is_stay()))
			continue;
		const RestrictionEval::Waypoint& wpt1(tfrs.wpt(wptnr));
		if (wpt0.get_coord().spheric_distance_nmi_dbl(wpt1.get_coord()) <= m_dctlimit)
			continue;
		r.set_result(true);
		r.get_edgeset().insert(wptnr - 1);
		r.get_xngedgeset().insert(wptnr - 1);
 	}
	return r;
}

BidirAltRange ConditionDctLimit::evaluate_dct(DctParameters::Calc& dct) const
{
	BidirAltRange r(dct.get_alt());
	if (dct.get_dist() <= m_dctlimit)
		r.set_empty();
	return r;
}

bool ConditionDctLimit::is_deparr_dct(Link& arpt, bool& arr, double& dist, civmil_t& civmil) const
{
	dist = m_dctlimit;
	return true;
}

bool ConditionDctLimit::is_enroute_dct(Link& aspc, AltRange& alt, double& dist, civmil_t& civmil) const
{
	dist = m_dctlimit;
	return true;
}

void ConditionDctLimit::io(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void ConditionDctLimit::io(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void ConditionDctLimit::io(ArchiveWriteStream& ar) const
{
	const_cast<ConditionDctLimit *>(this)->hibernate(ar);
}

void ConditionDctLimit::io(LinkLoader& ar)
{
	hibernate(ar);
}

void ConditionDctLimit::io(DependencyGenerator<UUID::set_t>& ar) const
{
	const_cast<ConditionDctLimit *>(this)->hibernate(ar);
}

void ConditionDctLimit::io(DependencyGenerator<Link::LinkSet>& ar) const
{
	const_cast<ConditionDctLimit *>(this)->hibernate(ar);
}

ConditionAircraft::ConditionAircraft(uint32_t childnum, const std::string& icaotype, uint8_t engines, acfttype_t acfttype, 
				     engine_t engine, navspec_t navspec, vertsep_t vertsep)
	: Condition(childnum), m_icaotype(icaotype), m_engines(engines), m_acfttype(acfttype), m_engine(engine), m_navspec(navspec), m_vertsep(vertsep)
{
}

std::ostream& ConditionAircraft::print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const
{
	os << std::endl << std::string(indent, ' ') << "ConditionAircraft{" << get_childnum() << '}';
	if (!get_icaotype().empty())
		os << " icaotype " << get_icaotype();
	if (get_acfttype() != acfttype_invalid)
		os << " type " << (char)get_acfttype();
	if (get_engine() != engine_invalid)
		os << " engine " << (char)get_engine();
	if (get_engines())
		os << " engines " << (unsigned int)get_engines();
	if (get_navspec() != navspec_invalid)
		os << " PRNAV";
	if (get_vertsep() != vertsep_invalid)
		os << " RVSM";
	return os;
}

std::string ConditionAircraft::to_str(timetype_t tm) const
{
	std::ostringstream oss;
	oss << "Aircraft{" << get_childnum() << '}';
	char sep('(');
	if (!get_icaotype().empty()) {
		oss << sep << get_icaotype();
		sep = ',';
	}
	if (get_acfttype() != acfttype_invalid) {
		oss << sep << (char)get_acfttype();
		sep = ',';
	}
	if (get_engine() != engine_invalid) {
		oss << sep << (char)get_engine();
		sep = ',';
	}
	if (get_engines()) {
		oss << sep << (unsigned int)get_engines();
		sep = ',';
	}
	if (get_navspec() != navspec_invalid) {
		oss << sep << "PRNAV";
		sep = ',';
	}
	if (get_vertsep() != vertsep_invalid) {
		oss << sep << "RVSM";
		sep = ',';
	}
	if (sep != '(')
		oss << ')';
	return oss.str();
}

Condition::ptr_t ConditionAircraft::clone(void) const
{
	return ptr_t(new ConditionAircraft(m_childnum, m_icaotype, m_engines, m_acfttype, m_engine, m_navspec, m_vertsep));
}

ConditionAircraft::ptr_t ConditionAircraft::simplify_aircrafttype(const std::string& acfttype) const
{
	bool val(m_icaotype.empty() || m_icaotype == acfttype);	
	if (!val || (m_engines == engine_invalid && m_acfttype == acfttype_invalid &&
		     !m_engine && m_navspec == navspec_invalid && m_vertsep == vertsep_invalid))
		return ptr_t(new ConditionConstant(get_childnum(), val));
	return ptr_t(new ConditionAircraft(get_childnum(), "", m_engines, m_acfttype, m_engine, m_navspec, m_vertsep));
}

ConditionAircraft::ptr_t ConditionAircraft::simplify_aircraftclass(const std::string& acftclass) const
{
	bool val(true);	
	if (acftclass.size() < 1)
		val = false;
	else
		val = val && (m_acfttype == acfttype_invalid || m_acfttype == std::toupper(acftclass[0]));
	if (acftclass.size() < 3)
		val = false;
	else
		val = val && (m_engine == engine_invalid || m_engine == std::toupper(acftclass[2]));
	if (get_engines() && (acftclass.size() < 2 || acftclass[1] != (char)('0' + get_engines())))
		val = false;
	if (!val || (m_icaotype.empty() && m_navspec == navspec_invalid && m_vertsep == vertsep_invalid))
		return ptr_t(new ConditionConstant(get_childnum(), val));
	return ptr_t(new ConditionAircraft(get_childnum(), m_icaotype, 0, acfttype_invalid, engine_invalid, m_navspec, m_vertsep));
}

ConditionAircraft::ptr_t ConditionAircraft::simplify_equipment(const std::string& equipment, Aircraft::pbn_t pbn) const
{
	bool val(true);	
	switch (m_navspec) {
	case navspec_rnav1:
		if (!(pbn & Aircraft::pbn_rnav))
			val = false;
		break;

	default:
		break;
	}
	switch (m_vertsep) {
	case vertsep_rvsm:
		val = val && equipment.find('W') != std::string::npos;
		break;

	default:
		break;
	}
	if (!val || (m_icaotype.empty() && m_engines == engine_invalid && m_acfttype == acfttype_invalid && !m_engine))
		return ptr_t(new ConditionConstant(get_childnum(), val));
	return ptr_t(new ConditionAircraft(get_childnum(), m_icaotype, m_engines, m_acfttype, m_engine, navspec_invalid, vertsep_invalid));
}

CondResult ConditionAircraft::evaluate(RestrictionEval& tfrs) const
{
	bool val(true);
	val = val && (get_icaotype().empty() || get_icaotype() == tfrs.get_aircrafttype());
	std::string acftclass(Aircraft::get_aircrafttypeclass(tfrs.get_aircrafttype()));
	val = val && (get_acfttype() == acfttype_invalid || (acftclass.size() >= 1 && std::toupper(acftclass[0]) == get_acfttype()));
	val = val && (get_engine() == engine_invalid || (acftclass.size() >= 3 && std::toupper(acftclass[2]) == get_engine()));
	val = val && (!get_engines() || (acftclass.size() >= 2 && acftclass[1] == (char)('0' + get_engines())));
	val = val && (get_navspec() == navspec_invalid || (tfrs.get_pbn() & Aircraft::pbn_rnav));
	val = val && (get_vertsep() == vertsep_invalid || tfrs.get_equipment().find('W') != std::string::npos);
	return CondResult(val);
}

void ConditionAircraft::io(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void ConditionAircraft::io(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void ConditionAircraft::io(ArchiveWriteStream& ar) const
{
	const_cast<ConditionAircraft *>(this)->hibernate(ar);
}

void ConditionAircraft::io(LinkLoader& ar)
{
	hibernate(ar);
}

void ConditionAircraft::io(DependencyGenerator<UUID::set_t>& ar) const
{
	const_cast<ConditionAircraft *>(this)->hibernate(ar);
}

void ConditionAircraft::io(DependencyGenerator<Link::LinkSet>& ar) const
{
	const_cast<ConditionAircraft *>(this)->hibernate(ar);
}

ConditionFlight::ConditionFlight(uint32_t childnum, civmil_t civmil, purpose_t purpose)
	:  Condition(childnum), m_civmil(civmil), m_purpose(purpose)
{
}

std::ostream& ConditionFlight::print(std::ostream& os, unsigned int indent, const TimeSlice& ts) const
{
	os << std::endl << std::string(indent, ' ') << "ConditionFlight{" << get_childnum() << '}';
	if (get_civmil() != civmil_invalid)
		os << ' ' << (char)get_civmil();
	if (get_purpose() != purpose_invalid)
		os << ' ' << (char)get_purpose();
	return os;
}

std::string ConditionFlight::to_str(timetype_t tm) const
{
	std::ostringstream oss;
	oss << "Flight{" << get_childnum() << '}';
	char sep('(');
	if (get_civmil() != civmil_invalid) {
		oss << sep << (char)get_civmil();
		sep = ',';
	}
	if (get_purpose() != purpose_invalid) {
		oss << sep << (char)get_purpose();
		sep = ',';
	}
	if (sep != '(')
		oss << ')';
	return oss.str();
}

Condition::ptr_t ConditionFlight::clone(void) const
{
	return ptr_t(new ConditionFlight(m_childnum, m_civmil, m_purpose));
}

ConditionFlight::ptr_t ConditionFlight::simplify_typeofflight(char type_of_flight) const
{
	bool val(false);
	switch (get_purpose()) {
	case purpose_all:
	case purpose_invalid:
		val = true;
		break;

	case purpose_scheduled:
	case purpose_nonscheduled:
	case purpose_private:
		val = type_of_flight == std::toupper(type_of_flight);
		break;

	case purpose_participant:
	case purpose_airtraining:
	case purpose_airwork:
	default:
		val = false;
		break;
	}
	if (get_civmil() == civmil_mil)
		val = false;
	if (!val || get_civmil() == civmil_invalid)
		return ptr_t(new ConditionConstant(get_childnum(), val));
	return ptr_t(new ConditionFlight(get_childnum(), get_civmil(), purpose_invalid));
}

ConditionFlight::ptr_t ConditionFlight::simplify_mil(bool mil) const
{
	switch (get_purpose()) {
	case purpose_all:
	case purpose_invalid:
		return ptr_t(new ConditionConstant(get_childnum(), get_civmil() != (mil ? civmil_civ : civmil_mil)));

	default:
		if (get_civmil() == (mil ? civmil_civ : civmil_mil))
			return ptr_t(new ConditionConstant(get_childnum(), false));
		return ptr_t(new ConditionFlight(get_childnum(), civmil_invalid, get_purpose()));
	}
}

CondResult ConditionFlight::evaluate(RestrictionEval& tfrs) const
{
	if (get_civmil() == civmil_mil)
		return CondResult(false);
	CondResult r(false);
	char type_of_flight(std::toupper(tfrs.get_flighttype()));
	switch (get_purpose()) {
	case purpose_all:
	case purpose_invalid:
		r.set_result(true);
		break;

	case purpose_scheduled:
	case purpose_nonscheduled:
	case purpose_private:
		r.set_result(type_of_flight == get_purpose());
		break;

	case purpose_participant:
	case purpose_airtraining:
	case purpose_airwork:
	default:
		r.set_result(false);
		break;
	}
	return r;
}

bool ConditionFlight::is_deparr_dct(Link& arpt, bool& arr, double& dist, civmil_t& civmil) const
{
	switch (get_purpose()) {
	case purpose_all:
	case purpose_invalid:
		civmil = get_civmil();
		return true;

	default:
		return false;
	}
}

bool ConditionFlight::is_enroute_dct(Link& aspc, AltRange& alt, double& dist, civmil_t& civmil) const
{
	switch (get_purpose()) {
	case purpose_all:
	case purpose_invalid:
		civmil = get_civmil();
		return true;

	default:
		return false;
	}
}

void ConditionFlight::io(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void ConditionFlight::io(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void ConditionFlight::io(ArchiveWriteStream& ar) const
{
	const_cast<ConditionFlight *>(this)->hibernate(ar);
}

void ConditionFlight::io(LinkLoader& ar)
{
	hibernate(ar);
}

void ConditionFlight::io(DependencyGenerator<UUID::set_t>& ar) const
{
	const_cast<ConditionFlight *>(this)->hibernate(ar);
}

void ConditionFlight::io(DependencyGenerator<Link::LinkSet>& ar) const
{
	const_cast<ConditionFlight *>(this)->hibernate(ar);
}
