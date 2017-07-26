#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iomanip>
#include "adrrestriction.hh"

using namespace ADR;

// Simplify list of crossing point rules
// eg. EGLF1022A, EUEDMIL

Condition::ptr_t ConditionAnd::extract_crossingpoints(std::vector<RuleSegment>& pts, const timepair_t& tm, const Object::const_ptr_t& aspc) const
{
	// AND's are restricted to AND(awycrossing,something else)
	if (!is_inv()) {
		if (aspc || m_cond.size() != 2)
			return ptr_t();
		if (m_cond[0].is_inv() || m_cond[1].is_inv())
			return ptr_t();
		if (!m_cond[0].get_ptr() || !m_cond[1].get_ptr())
			return ptr_t();
		for (unsigned int i = 0; i < 2; ++i) {
			std::vector<RuleSegment> seg;
			seq_t seq;
			seq.resize(2, const_ptr_t());
			seq[i] = m_cond[i].get_ptr()->extract_crossingairspaces(seg);
			if (seg.size() == 1 && seg.front().is_airspace() && seg.front().get_wpt0()) {
				std::vector<RuleSegment> pts1;
				seq[!i] =  m_cond[!i].get_ptr()->extract_crossingpoints(pts1, tm, seg.front().get_wpt0());
				for (std::vector<RuleSegment>::iterator i(pts1.begin()), e(pts1.end()); i != e; ++i)
					i->get_alt().intersect(seg.front().get_alt());
				pts.insert(pts.end(), pts1.begin(), pts1.end());
				return simplify(seq);
			}
		}
		return ptr_t();
	}
	seq_t seq;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p;
		if (i->is_inv()) {
			p = i->get_ptr();
			if (p)
				p = p->extract_crossingpoints(pts, tm, aspc);
		}
		seq.push_back(p);
	}
	return simplify(seq);
}

Condition::ptr_t ConditionSequence::extract_crossingpoints(std::vector<RuleSegment>& pts, const timepair_t& tm, const Object::const_ptr_t& aspc) const
{
	if (aspc)
		return ptr_t();
	bool first(true);
	seq_t seq;
	std::vector<RuleSegment> pts1;
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p;
		if (first) {
			p = *i;
			if (p) {
				p = p->extract_crossingpoints(pts1, tm, aspc);
				first = pts1.empty();
				if (first && !(*i)->is_routestatic())
					return ptr_t();
			}
		} else if (!(*i)->is_routestatic()) {
			return ptr_t();
		}
		seq.push_back(p);
	}
	pts.insert(pts.end(), pts1.begin(), pts1.end());
	return simplify(seq);
}

Condition::ptr_t ConditionCrossingPoint::extract_crossingpoints(std::vector<RuleSegment>& pts, const timepair_t& tm, const Object::const_ptr_t& aspc) const
{
	if (!m_wpt.get_obj())
		return ptr_t();
	if (aspc) {
		timeset_t tdisc(aspc->timediscontinuities());
		{
			timeset_t tdisc1(m_wpt.get_obj()->timediscontinuities());
			tdisc.insert(tdisc1.begin(), tdisc1.end());
		}
		tdisc.insert(tm.first);
		IntervalSet<int32_t> alt;
		for (timeset_t::const_iterator ti(tdisc.begin()), te(tdisc.end()); ti != te; ++ti) {
			if (*ti < tm.first)
				continue;
			if (*ti >= tm.second)
				break;
			const PointIdentTimeSlice& tsp(m_wpt.get_obj()->operator()(*ti).as_point());
			if (!tsp.is_valid() || tsp.get_coord().is_invalid())
				return ptr_t();
			const AirspaceTimeSlice& ts(aspc->operator()(*ti).as_airspace());
			if (!ts.is_valid())
				continue;
			TimeTableSpecialDateEval ttsde;
			TimeTableEval tte(*ti, tsp.get_coord(), ttsde);
			IntervalSet<int32_t> alt1(ts.get_point_altitudes(tte, AltRange(), m_wpt.get_obj()->get_uuid()));
			if (alt1.is_empty())
				return ptr_t();
			if (alt.is_empty())
				alt = alt1;
			else if (alt != alt1)
				return ptr_t();
		}
		if (alt.is_empty())
			return ptr_t();
		// FIXME: RuleSegment should probably use intervals
		AltRange ar;
		unsigned int arcnt(0);
		for (IntervalSet<int32_t>::const_iterator i(alt.begin()), e(alt.end()); i != e; ++i, ++arcnt)
			ar = AltRange(i->get_lower(), AltRange::alt_std, i->get_upper() - 1, AltRange::alt_std);
		if (arcnt != 1)
			return ptr_t();
		ar.intersect(m_alt);
		pts.push_back(RuleSegment(RuleSegment::type_point, ar, m_wpt.get_obj()));
		ptr_t p(new ConditionConstant(m_childnum, true));
		return p;
	}
	pts.push_back(RuleSegment(RuleSegment::type_point, m_alt, m_wpt.get_obj()));
	ptr_t p(new ConditionConstant(m_childnum, true));
	return p;
}

Condition::ptr_t ConditionDepArr::extract_crossingpoints(std::vector<RuleSegment>& pts, const timepair_t& tm, const Object::const_ptr_t& aspc) const
{
	if (!m_airport.get_obj())
		return ptr_t();
	if (aspc) {
		timeset_t tdisc(aspc->timediscontinuities());
		{
			timeset_t tdisc1(m_airport.get_obj()->timediscontinuities());
			tdisc.insert(tdisc1.begin(), tdisc1.end());
		}
		tdisc.insert(tm.first);
		IntervalSet<int32_t> alt;
		for (timeset_t::const_iterator ti(tdisc.begin()), te(tdisc.end()); ti != te; ++ti) {
			if (*ti < tm.first)
				continue;
			if (*ti >= tm.second)
				break;
			const PointIdentTimeSlice& tsp(m_airport.get_obj()->operator()(*ti).as_point());
			if (!tsp.is_valid() || tsp.get_coord().is_invalid())
				return ptr_t();
			const AirspaceTimeSlice& ts(aspc->operator()(*ti).as_airspace());
			if (!ts.is_valid())
				continue;
			TimeTableSpecialDateEval ttsde;
			TimeTableEval tte(*ti, tsp.get_coord(), ttsde);
			IntervalSet<int32_t> alt1(ts.get_point_altitudes(tte, AltRange(), m_airport.get_obj()->get_uuid()));
			if (alt1.is_empty())
				return ptr_t();
			if (alt.is_empty())
				alt = alt1;
			else if (alt != alt1)
				return ptr_t();
		}
		if (alt.is_empty())
			return ptr_t();
		// FIXME: RuleSegment should probably use intervals
		AltRange ar;
		unsigned int arcnt(0);
		for (IntervalSet<int32_t>::const_iterator i(alt.begin()), e(alt.end()); i != e; ++i, ++arcnt)
			ar = AltRange(i->get_lower(), AltRange::alt_std, i->get_upper() - 1, AltRange::alt_std);
		if (arcnt != 1)
			return ptr_t();
		pts.push_back(RuleSegment(RuleSegment::type_point, ar, m_airport.get_obj()));
		ptr_t p(new ConditionConstant(m_childnum, true));
		return p;
	}
	pts.push_back(RuleSegment(RuleSegment::type_point, AltRange(), m_airport.get_obj()));
	ptr_t p(new ConditionConstant(m_childnum, true));
	return p;
}

std::vector<RestrictionElement::ptr_t> RestrictionElementPoint::clone_crossingpoints(const std::vector<RuleSegment>& pts) const
{
	std::vector<ptr_t> p;
	for (std::vector<RuleSegment>::const_iterator pi(pts.begin()), pe(pts.end()); pi != pe; ++pi) {
		if (!pi->is_point())
			continue;
		if (m_point != pi->get_uuid0())
			continue;
		AltRange ar(m_alt);
		ar.intersect(pi->get_alt());
		ptr_t ppp(new RestrictionElementPoint(ar, m_point));
		p.push_back(ppp);
	}
	return p;
}

bool RestrictionSequence::clone_crossingpoints(std::vector<RestrictionSequence>& seq, const std::vector<RuleSegment>& pts) const
{
	bool subseq(false);
        for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		RestrictionElement::const_ptr_t p(*i);
		if (!p)
			continue;
		std::vector<RestrictionElement::ptr_t> pp(p->clone_crossingpoints(pts));
		if (pp.empty())
			return false;
		if (subseq)
			return false;
		subseq = true;
		for (std::vector<RestrictionElement::ptr_t>::const_iterator pi(pp.begin()), pe(pp.end()); pi != pe; ++pi) {
			seq.push_back(RestrictionSequence());
			seq.back().m_elements.push_back(*pi);
		}
	}
	return true;
}

bool Restrictions::clone_crossingpoints(Restrictions& r, const std::vector<RuleSegment>& pts) const
{
        for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		if (!i->clone_crossingpoints(r.m_elements, pts))
			return false;
	}
	return true;
}

FlightRestriction::ptr_t FlightRestriction::simplify_complexity_crossingpoints(void) const
{
	ptr_t p(clone_obj());
	bool modified(false);
	for (unsigned int i(0), n(p->size()); i < n; ++i) {
		FlightRestrictionTimeSlice& ts(p->operator[](i).as_flightrestriction());
		if (!ts.is_valid())
			continue;
		if (ts.get_type() != FlightRestrictionTimeSlice::type_forbidden || !ts.get_condition())
			continue;
		std::vector<RuleSegment> pts;
		Condition::ptr_t cond(ts.get_condition()->extract_crossingpoints(pts, ts.get_timeinterval()));
		if (!cond || pts.empty() || !cond->is_routestatic())
			continue;
		Restrictions res;
		if (!ts.get_restrictions().clone_crossingpoints(res, pts))
			continue;
		ts.set_condition(cond);
		ts.set_restrictions(res);
		modified = true;
	}
	if (modified)
		return p;
	return ptr_t();
}

// Simplify list of crossing segment rules
// eg. LS2521A

Condition::ptr_t ConditionAnd::extract_crossingsegments(std::vector<RuleSegment>& segs, const timepair_t& tm, const Object::const_ptr_t& aspc) const
{
	// AND's are restricted to AND(awycrossing,something else)
	if (!is_inv()) {
		if (aspc || m_cond.size() != 2)
			return ptr_t();
		if (m_cond[0].is_inv() || m_cond[1].is_inv())
			return ptr_t();
		if (!m_cond[0].get_ptr() || !m_cond[1].get_ptr())
			return ptr_t();
		for (unsigned int i = 0; i < 2; ++i) {
			std::vector<RuleSegment> seg;
			m_cond[i].get_ptr()->extract_crossingairspaces(seg);
			if (seg.size() == 1 && seg.front().is_airspace() && seg.front().get_wpt0()) {
				seq_t seq;
				seq.resize(2, const_ptr_t());
				std::vector<RuleSegment> segs1;
				seq[!i] =  m_cond[!i].get_ptr()->extract_crossingsegments(segs1, tm, seg.front().get_wpt0());
				for (std::vector<RuleSegment>::iterator i(segs1.begin()), e(segs1.end()); i != e; ++i)
					i->get_alt().intersect(seg.front().get_alt());
				segs.insert(segs.end(), segs1.begin(), segs1.end());
				return simplify(seq);
			}
		}
		return ptr_t();
	}
	seq_t seq;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p;
		if (i->is_inv()) {
			p = i->get_ptr();
			if (p)
				p = p->extract_crossingsegments(segs, tm, aspc);
		}
		seq.push_back(p);
	}
	return simplify(seq);
}

Condition::ptr_t ConditionSequence::extract_crossingsegments(std::vector<RuleSegment>& segs, const timepair_t& tm, const Object::const_ptr_t& aspc) const
{
	if (aspc)
		return ptr_t();
	bool first(true);
	seq_t seq;
	std::vector<RuleSegment> segs1;
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p;
		if (first) {
			p = *i;
			if (p) {
				p = p->extract_crossingsegments(segs1, tm, aspc);
				first = segs1.empty();
				if (first && !(*i)->is_routestatic())
					return ptr_t();
			}
		} else if (!(*i)->is_routestatic()) {
			return ptr_t();
		}
		seq.push_back(p);
	}
	segs.insert(segs.end(), segs1.begin(), segs1.end());
	return simplify(seq);
}

Condition::ptr_t ConditionCrossingDct::extract_crossingsegments(std::vector<RuleSegment>& segs, const timepair_t& tm, const Object::const_ptr_t& aspc) const
{
	if (!m_wpt[0].get_obj() || !m_wpt[1].get_obj())
		return ptr_t();
	if (aspc) {
		timeset_t tdisc(aspc->timediscontinuities());
		for (unsigned int i = 0; i < 2; ++i) {
			timeset_t tdisc1(m_wpt[i].get_obj()->timediscontinuities());
			tdisc.insert(tdisc1.begin(), tdisc1.end());
		}
		tdisc.insert(tm.first);
		IntervalSet<int32_t> alt;
		for (timeset_t::const_iterator ti(tdisc.begin()), te(tdisc.end()); ti != te; ++ti) {
			if (*ti < tm.first)
				continue;
			if (*ti >= tm.second)
				break;
			const PointIdentTimeSlice& tsp0(m_wpt[0].get_obj()->operator()(*ti).as_point());
			if (!tsp0.is_valid() || tsp0.get_coord().is_invalid())
				return ptr_t();
			const PointIdentTimeSlice& tsp1(m_wpt[1].get_obj()->operator()(*ti).as_point());
			if (!tsp1.is_valid() || tsp1.get_coord().is_invalid())
				return ptr_t();
			const AirspaceTimeSlice& ts(aspc->operator()(*ti).as_airspace());
			if (!ts.is_valid())
				continue;
			TimeTableSpecialDateEval ttsde;
			TimeTableEval tte(*ti, tsp0.get_coord(), ttsde);
			IntervalSet<int32_t> alt1(ts.get_point_intersect_altitudes(tte, tsp1.get_coord(), AltRange(),
										   m_wpt[0].get_obj()->get_uuid(), m_wpt[1].get_obj()->get_uuid()));
			if (alt1.is_empty())
				return ptr_t();
			if (alt.is_empty())
				alt = alt1;
			else if (alt != alt1)
				return ptr_t();
		}
		if (alt.is_empty())
			return ptr_t();
		// FIXME: RuleSegment should probably use intervals
		AltRange ar;
		unsigned int arcnt(0);
		for (IntervalSet<int32_t>::const_iterator i(alt.begin()), e(alt.end()); i != e; ++i, ++arcnt)
			ar = AltRange(i->get_lower(), AltRange::alt_std, i->get_upper() - 1, AltRange::alt_std);
		if (arcnt != 1)
			return ptr_t();
		ar.intersect(m_alt);
		segs.push_back(RuleSegment(RuleSegment::type_dct, ar, m_wpt[0].get_obj(), m_wpt[1].get_obj()));
		ptr_t p(new ConditionConstant(m_childnum, true));
		return p;
	}
	segs.push_back(RuleSegment(RuleSegment::type_dct, m_alt, m_wpt[0].get_obj(), m_wpt[1].get_obj()));
	ptr_t p(new ConditionConstant(m_childnum, true));
	return p;
}

Condition::ptr_t ConditionCrossingAirway::extract_crossingsegments(std::vector<RuleSegment>& segs, const timepair_t& tm, const Object::const_ptr_t& aspc) const
{
	if (!m_wpt[0].get_obj() || !m_wpt[1].get_obj() || !m_awy.get_obj())
		return ptr_t();
	if (aspc) {
		timeset_t tdisc(aspc->timediscontinuities());
		for (SegmentsList::const_iterator i(m_segments.begin()), e(m_segments.end()); i != e; ++i) {
			const SegmentsListTimeSlice& ts(*i);
			if (!ts.is_overlap(tm))
				continue;
			if (ts.get_segments().empty())
				return ptr_t();
			tdisc.insert(ts.get_starttime());
			tdisc.insert(ts.get_endtime());
			for (SegmentsListTimeSlice::segments_t::const_iterator si(ts.get_segments().begin()), se(ts.get_segments().end()); si != se; ++si) {
				timeset_t tdisc1(si->get_obj()->timediscontinuities());
				for (timeset_t::const_iterator ti(tdisc1.begin()), te(tdisc1.end()); ti != te; ++ti) {
					if (*ti < ts.get_starttime())
						continue;
					if (*ti >= ts.get_endtime())
						break;
					tdisc.insert(*ti);
				}
			}
		}
		tdisc.insert(tm.first);
		IntervalSet<int32_t> alt;
		for (timeset_t::const_iterator ti(tdisc.begin()), te(tdisc.end()); ti != te; ++ti) {
			if (*ti < tm.first)
				continue;
			if (*ti >= tm.second)
				break;
			const AirspaceTimeSlice& ts(aspc->operator()(*ti).as_airspace());
			if (!ts.is_valid())
				continue;
			for (SegmentsList::const_iterator sli(m_segments.begin()), sle(m_segments.end()); sli != sle; ++sli) {
				if (!sli->is_inside(*ti))
					continue;
				IntervalSet<int32_t> alt1;
				for (SegmentsListTimeSlice::segments_t::const_iterator si(sli->get_segments().begin()), se(sli->get_segments().end()); si != se; ++si) {
					const SegmentTimeSlice& tss(si->get_obj()->operator()(*ti).as_segment());
					if (!tss.is_valid())
						return ptr_t();
					const PointIdentTimeSlice& tsp0(tss.get_start().get_obj()->operator()(*ti).as_point());
					if (!tsp0.is_valid() || tsp0.get_coord().is_invalid())
						return ptr_t();
					const PointIdentTimeSlice& tsp1(tss.get_end().get_obj()->operator()(*ti).as_point());
					if (!tsp1.is_valid() || tsp1.get_coord().is_invalid())
						return ptr_t();
					TimeTableSpecialDateEval ttsde;
					TimeTableEval tte(*ti, tsp0.get_coord(), ttsde);
					IntervalSet<int32_t> alt2(ts.get_point_intersect_altitudes(tte, tsp1.get_coord(), AltRange(),
												   tss.get_start().get_obj()->get_uuid(),
												   tss.get_end().get_obj()->get_uuid()));
					if (alt2.is_empty())
						return ptr_t();
					if (alt1.is_empty())
						alt1 = alt2;
					else if (alt1 != alt2)
						return ptr_t();
				}
				if (alt1.is_empty())
					return ptr_t();
				if (alt.is_empty())
					alt = alt1;
				else if (alt != alt1)
					return ptr_t();
			}
		}
		if (alt.is_empty())
			return ptr_t();
		// FIXME: RuleSegment should probably use intervals
		AltRange ar;
		unsigned int arcnt(0);
		for (IntervalSet<int32_t>::const_iterator i(alt.begin()), e(alt.end()); i != e; ++i, ++arcnt)
			ar = AltRange(i->get_lower(), AltRange::alt_std, i->get_upper() - 1, AltRange::alt_std);
		if (arcnt != 1)
			return ptr_t();
		ar.intersect(m_alt);
		segs.push_back(RuleSegment(RuleSegment::type_airway, ar, m_wpt[0].get_obj(), m_wpt[1].get_obj(), m_awy.get_obj()));
		ptr_t p(new ConditionConstant(m_childnum, true));
		return p;
	}
	segs.push_back(RuleSegment(RuleSegment::type_airway, m_alt, m_wpt[0].get_obj(), m_wpt[1].get_obj(), m_awy.get_obj()));
	ptr_t p(new ConditionConstant(m_childnum, true));
	return p;
}

std::vector<RestrictionElement::ptr_t> RestrictionElementRoute::clone_crossingsegments(const std::vector<RuleSegment>& segs) const
{
	std::vector<ptr_t> p;
	for (std::vector<RuleSegment>::const_iterator pi(segs.begin()), pe(segs.end()); pi != pe; ++pi) {
		if (m_point[0] != pi->get_uuid0() || m_point[1] != pi->get_uuid1())
			continue;
		if (pi->is_dct()) {
			if (!is_valid_dct())
				continue;
		} else if (pi->is_airway()) {
			if (is_valid_dct())
				continue;
			if (m_route != pi->get_airway_uuid())
				continue;
		} else {
			continue;
		}
		AltRange ar(m_alt);
		ar.intersect(pi->get_alt());
		ptr_t ppp(new RestrictionElementRoute(ar, m_point[0], m_point[1], m_route));
		p.push_back(ppp);
	}
	return p;
}

bool RestrictionSequence::clone_crossingsegments(std::vector<RestrictionSequence>& seq, const std::vector<RuleSegment>& segs) const
{
	bool subseq(false);
        for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		RestrictionElement::const_ptr_t p(*i);
		if (!p)
			continue;
		std::vector<RestrictionElement::ptr_t> pp(p->clone_crossingsegments(segs));
		if (pp.empty())
			return false;
		if (subseq)
			return false;
		subseq = true;
		for (std::vector<RestrictionElement::ptr_t>::const_iterator pi(pp.begin()), pe(pp.end()); pi != pe; ++pi) {
			seq.push_back(RestrictionSequence());
			seq.back().m_elements.push_back(*pi);
		}
	}
	return true;
}

bool Restrictions::clone_crossingsegments(Restrictions& r, const std::vector<RuleSegment>& segs) const
{
        for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		if (!i->clone_crossingsegments(r.m_elements, segs))
			return false;
	}
	return true;
}

FlightRestriction::ptr_t FlightRestriction::simplify_complexity_crossingsegments(void) const
{
	ptr_t p(clone_obj());
	bool modified(false);
	for (unsigned int i(0), n(p->size()); i < n; ++i) {
		FlightRestrictionTimeSlice& ts(p->operator[](i).as_flightrestriction());
		if (!ts.is_valid())
			continue;
		if (ts.get_type() != FlightRestrictionTimeSlice::type_forbidden || !ts.get_condition())
			continue;
		std::vector<RuleSegment> segs;
		Condition::ptr_t cond(ts.get_condition()->extract_crossingsegments(segs, ts.get_timeinterval()));
		if (!cond || segs.empty() || !cond->is_routestatic())
			continue;
		Restrictions res;
		if (!ts.get_restrictions().clone_crossingsegments(res, segs))
			continue;
		ts.set_condition(cond);
		ts.set_restrictions(res);
		modified = true;
	}
	if (modified)
		return p;
	return ptr_t();
}

// mandatory crossing points

bool RestrictionElementRoute::is_mandatoryinbound(const timepair_t& tint, const std::vector<RuleSegment>& pts) const
{
	for (std::vector<RuleSegment>::const_iterator pi(pts.begin()), pe(pts.end()); pi != pe; ++pi) {
		if (!pi->is_point())
			continue;
		if (m_point[1] != pi->get_uuid0())
			continue;
		if (pi->get_alt().is_lower_valid() &&
		    (!m_alt.is_lower_valid() || pi->get_alt().get_lower_alt() > m_alt.get_lower_alt()))
			continue;
		if (pi->get_alt().is_upper_valid() &&
		    (!m_alt.is_upper_valid() || pi->get_alt().get_upper_alt() < m_alt.get_upper_alt()))
			continue;
		return true;
	}
	return false;
}

bool RestrictionElementSidStar::is_mandatoryinbound(const timepair_t& tint, const std::vector<RuleSegment>& pts) const
{
	if (!m_star)
		return false;
	for (std::vector<RuleSegment>::const_iterator pi(pts.begin()), pe(pts.end()); pi != pe; ++pi) {
		if (!pi->is_point())
			continue;
		{
			bool match(false), matchok(true);
			for (unsigned int i(0), n(m_proc.get_obj()->size()); i < n; ++i) {
				const StandardInstrumentTimeSlice& ts(m_proc.get_obj()->operator[](i).as_sidstar());
				if (!ts.is_valid() || !ts.is_overlap(tint))
					continue;
				if (ts.get_airport() == pi->get_uuid0())
					match = true;
				else
					matchok = false;
			}
			if (!(match && matchok))
				continue;
		}
		if (pi->get_alt().is_lower_valid() &&
		    (!m_alt.is_lower_valid() || pi->get_alt().get_lower_alt() > m_alt.get_lower_alt()))
			continue;
		if (pi->get_alt().is_upper_valid() &&
		    (!m_alt.is_upper_valid() || pi->get_alt().get_upper_alt() < m_alt.get_upper_alt()))
			continue;
		return true;
	}
	return false;
}

bool RestrictionSequence::is_mandatoryinbound(const timepair_t& tint, const std::vector<RuleSegment>& pts) const
{
	if (m_elements.empty() || !m_elements.back())
		return false;
	return m_elements.back()->is_mandatoryinbound(tint, pts);
}

bool Restrictions::is_mandatoryinbound(const timepair_t& tint, const std::vector<RuleSegment>& pts) const
{
	if (m_elements.empty())
		return false;
        for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
		if (!i->is_mandatoryinbound(tint, pts))
			return false;
	return true;
}

bool FlightRestrictionTimeSlice::is_mandatoryinbound(void) const
{
	if (get_type() != type_mandatory || !get_condition())
		return false;
	std::vector<RuleSegment> pts;
	Condition::ptr_t cond(get_condition()->extract_crossingpoints(pts, get_timeinterval()));
	if (pts.empty())
		return false;
	if (cond && !cond->is_routestatic())
		return false;
	return get_restrictions().is_mandatoryinbound(get_timeinterval(), pts);
}

bool RestrictionElementRoute::is_mandatoryoutbound(const timepair_t& tint, const std::vector<RuleSegment>& pts) const
{
	for (std::vector<RuleSegment>::const_iterator pi(pts.begin()), pe(pts.end()); pi != pe; ++pi) {
		if (!pi->is_point())
			continue;
		if (m_point[0] != pi->get_uuid0())
			continue;
		if (pi->get_alt().is_lower_valid() &&
		    (!m_alt.is_lower_valid() || pi->get_alt().get_lower_alt() > m_alt.get_lower_alt()))
			continue;
		if (pi->get_alt().is_upper_valid() &&
		    (!m_alt.is_upper_valid() || pi->get_alt().get_upper_alt() < m_alt.get_upper_alt()))
			continue;
		return true;
	}
	return false;
}

bool RestrictionElementSidStar::is_mandatoryoutbound(const timepair_t& tint, const std::vector<RuleSegment>& pts) const
{
	if (m_star)
		return false;
	for (std::vector<RuleSegment>::const_iterator pi(pts.begin()), pe(pts.end()); pi != pe; ++pi) {
		if (!pi->is_point())
			continue;
		{
			bool match(false), matchok(true);
			for (unsigned int i(0), n(m_proc.get_obj()->size()); i < n; ++i) {
				const StandardInstrumentTimeSlice& ts(m_proc.get_obj()->operator[](i).as_sidstar());
				if (!ts.is_valid() || !ts.is_overlap(tint))
					continue;
				if (ts.get_airport() == pi->get_uuid0())
					match = true;
				else
					matchok = false;
			}
			if (!(match && matchok))
				continue;
		}
		if (pi->get_alt().is_lower_valid() &&
		    (!m_alt.is_lower_valid() || pi->get_alt().get_lower_alt() > m_alt.get_lower_alt()))
			continue;
		if (pi->get_alt().is_upper_valid() &&
		    (!m_alt.is_upper_valid() || pi->get_alt().get_upper_alt() < m_alt.get_upper_alt()))
			continue;
		return true;
	}
	return false;
}

bool RestrictionSequence::is_mandatoryoutbound(const timepair_t& tint, const std::vector<RuleSegment>& pts) const
{
	if (m_elements.empty() || !m_elements.back())
		return false;
	return m_elements.back()->is_mandatoryoutbound(tint, pts);
}

bool Restrictions::is_mandatoryoutbound(const timepair_t& tint, const std::vector<RuleSegment>& pts) const
{
	if (m_elements.empty())
		return false;
        for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
		if (!i->is_mandatoryoutbound(tint, pts))
			return false;
	return true;
}

bool FlightRestrictionTimeSlice::is_mandatoryoutbound(void) const
{
	if (get_type() != type_mandatory || !get_condition())
		return false;
	std::vector<RuleSegment> pts;
	Condition::ptr_t cond(get_condition()->extract_crossingpoints(pts, get_timeinterval()));
	if (pts.empty())
		return false;
	if (cond && !cond->is_routestatic())
		return false;
	return get_restrictions().is_mandatoryoutbound(get_timeinterval(), pts);
}

// Simplify closed levels

Condition::ptr_t ConditionAnd::extract_crossingairspaces(std::vector<RuleSegment>& aspcs) const
{
	// only OR's
	if (!is_inv())
		return ptr_t();
	seq_t seq;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p;
		if (i->is_inv()) {
			p = i->get_ptr();
			if (p)
				p = p->extract_crossingairspaces(aspcs);
		}
		seq.push_back(p);
	}
	return simplify(seq);
}

Condition::ptr_t ConditionSequence::extract_crossingairspaces(std::vector<RuleSegment>& aspcs) const
{
	bool first(true);
	seq_t seq;
	std::vector<RuleSegment> aspcs1;
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p;
		if (first) {
			p = *i;
			if (p) {
				p = p->extract_crossingairspaces(aspcs1);
				first = aspcs1.empty();
				if (first && !(*i)->is_routestatic())
					return ptr_t();
			}
		} else if (!(*i)->is_routestatic()) {
			return ptr_t();
		}
		seq.push_back(p);
	}
	aspcs.insert(aspcs.end(), aspcs1.begin(), aspcs1.end());
	return simplify(seq);
}

Condition::ptr_t ConditionCrossingAirspace1::extract_crossingairspaces(std::vector<RuleSegment>& aspcs) const
{
	if (!m_airspace.get_obj())
		return ptr_t();
	aspcs.push_back(RuleSegment(RuleSegment::type_airspace, m_alt, m_airspace.get_obj()));
	ptr_t p(new ConditionConstant(m_childnum, true));
	return p;
}

FlightRestriction::ptr_t FlightRestriction::simplify_complexity_closedairspace(void) const
{
	ptr_t p(clone_obj());
	bool modified(false);
	for (unsigned int i(0), n(p->size()); i < n; ++i) {
		FlightRestrictionTimeSlice& ts(p->operator[](i).as_flightrestriction());
		if (!ts.is_valid())
			continue;
		if (ts.get_type() != FlightRestrictionTimeSlice::type_closed || !ts.get_condition() || !ts.get_restrictions().empty())
			continue;
		std::vector<RuleSegment> aspcs;
		Condition::ptr_t cond(ts.get_condition()->extract_crossingairspaces(aspcs));
		if (!cond || aspcs.empty() || !cond->is_routestatic())
			continue;
		Restrictions res;
		for (std::vector<RuleSegment>::const_iterator ai(aspcs.begin()), ae(aspcs.end()); ai != ae; ++ai) {
			RestrictionSequence seq;
			seq.add(ai->get_restrictionelement());
			res.add(seq);
		}
		ts.set_type(FlightRestrictionTimeSlice::type_forbidden);
		ts.set_condition(cond);
		ts.set_restrictions(res);
		modified = true;
	}
	if (modified)
		return p;
	return ptr_t();
}

const std::string& to_str(ADR::Condition::forbiddenpoint_t fp)
{
	switch (fp) {
	case ADR::Condition::forbiddenpoint_true:
	{
		static const std::string r("true");
		return r;
	}

	case ADR::Condition::forbiddenpoint_truesamesegbefore:
	{
		static const std::string r("truesamesegbefore");
		return r;
	}

	case ADR::Condition::forbiddenpoint_truesamesegat:
	{
		static const std::string r("truesamesegat");
		return r;
	}

	case ADR::Condition::forbiddenpoint_truesamesegafter:
	{
		static const std::string r("truesamesegafter");
		return r;
	}

	case ADR::Condition::forbiddenpoint_trueotherseg:
	{
		static const std::string r("trueotherseg");
		return r;
	}

	case ADR::Condition::forbiddenpoint_false:
	{
		static const std::string r("false");
		return r;
	}

	case ADR::Condition::forbiddenpoint_falsesamesegbefore:
	{
		static const std::string r("falsesamesegbefore");
		return r;
	}

		case ADR::Condition::forbiddenpoint_falsesamesegat:
	{
		static const std::string r("falsesamesegat");
		return r;
	}

	case ADR::Condition::forbiddenpoint_falsesamesegafter:
	{
		static const std::string r("falsesamesegafter");
		return r;
	}

	case ADR::Condition::forbiddenpoint_falseotherseg:
	{
		static const std::string r("falseotherseg");
		return r;
	}

	case ADR::Condition::forbiddenpoint_invalid:
	{
		static const std::string r("invalid");
		return r;
	}

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}
