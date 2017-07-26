#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iomanip>
#include "tfr.hh"

// Simplify list of crossing point rules
// eg. EGLF1022A, EUEDMIL

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionAnd::extract_crossingpoints(std::vector<RuleWptAlt>& pts) const
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
				p = p->extract_crossingpoints(pts);
		}
		seq.push_back(p);
	}
	return simplify(seq);
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionSequence::extract_crossingpoints(std::vector<RuleWptAlt>& pts) const
{
	bool first(true);
	seq_t seq;
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p;
		if (first) {
			p = *i;
			if (p) {
				std::vector<RuleWptAlt> pts1;
				p = p->extract_crossingpoints(pts1);
				first = pts1.empty();
				if (!first)
					pts.insert(pts.end(), pts1.begin(), pts1.end());
			}
		}
		seq.push_back(p);
	}
	return simplify(seq);
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionCrossingPoint::extract_crossingpoints(std::vector<RuleWptAlt>& pts) const
{
	pts.push_back(RuleWptAlt(m_wpt, m_alt));
	ptr_t p(new ConditionConstant(m_childnum, true));
	return p;
}

std::vector<TrafficFlowRestrictions::RestrictionElement::ptr_t> TrafficFlowRestrictions::RestrictionElementPoint::clone_crossingpoints(const std::vector<RuleWptAlt>& pts) const
{
	std::vector<ptr_t> p;
	int32_t altl(std::numeric_limits<int32_t>::min()), altu(std::numeric_limits<int32_t>::max());
	if (m_alt.is_lower_valid())
		altl = m_alt.get_lower_alt();
	if (m_alt.is_upper_valid())
		altu = m_alt.get_upper_alt();
	AltRange::alt_t model(m_alt.get_lower_mode()), modeu(m_alt.get_upper_mode());
	for (std::vector<RuleWptAlt>::const_iterator pi(pts.begin()), pe(pts.end()); pi != pe; ++pi) {
		if (m_point != pi->get_wpt())
			continue;
		int32_t al(altl), au(altu);
		AltRange::alt_t ml(model), mu(modeu);
		if (pi->get_alt().is_lower_valid()) {
			al = std::max(al, pi->get_alt().get_lower_alt());
			if (ml == AltRange::alt_invalid)
				ml = pi->get_alt().get_lower_mode();
		}
		if (pi->get_alt().is_upper_valid()) {
			au = std::min(au, pi->get_alt().get_upper_alt());
			if (mu == AltRange::alt_invalid)
				mu = pi->get_alt().get_upper_mode();
		}
		if (al == std::numeric_limits<int32_t>::min()) {
			al = 0;
			ml = AltRange::alt_invalid;
		}
		if (au == std::numeric_limits<int32_t>::max())
			mu = AltRange::alt_invalid;
		ptr_t ppp(new RestrictionElementPoint(AltRange(al, ml, au, mu), m_point));
		p.push_back(ppp);
	}
	return p;
}

bool TrafficFlowRestrictions::RestrictionSequence::clone_crossingpoints(std::vector<RestrictionSequence>& seq, const std::vector<RuleWptAlt>& pts) const
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

bool TrafficFlowRestrictions::Restrictions::clone_crossingpoints(Restrictions& r, const std::vector<RuleWptAlt>& pts) const
{
        for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		if (!i->clone_crossingpoints(r.m_elements, pts))
			return false;
	}
	return true;
}

TrafficFlowRestrictions::TrafficFlowRule::ptr_t TrafficFlowRestrictions::TrafficFlowRule::simplify_complexity_crossingpoints(void) const
{
	if (get_codetype() != codetype_forbidden || !get_condition())
		return ptr_t();
	std::vector<RuleWptAlt> pts;
	Condition::ptr_t cond(get_condition()->extract_crossingpoints(pts));
	if (!cond || pts.empty() || !cond->is_routestatic())
		return ptr_t();
	Restrictions res;
	if (!get_restrictions().clone_crossingpoints(res, pts))
		return ptr_t();
	ptr_t p(new TrafficFlowRule());
	p->set_codetype(get_codetype());
	p->set_mid(get_mid());
	p->set_codeid(get_codeid());
	p->set_oprgoal(get_oprgoal());
	p->set_descr(get_descr());
	p->set_timesheet(get_timesheet());
	p->add_restrictions(res);
	p->set_condition(cond);
	if (false)
		std::cerr << "simplify_complexity_crossingpoints: simplified rule " << get_codeid() << std::endl
			  << "  oldcond:  " << get_condition()->to_str() << std::endl
			  << "  oldrestr: " << get_restrictions().to_str() << std::endl
			  << "  newcond:  " << p->get_condition()->to_str() << std::endl
			  << "  newrestr: " << p->get_restrictions().to_str() << std::endl;
	return p;
}


// Simplify list of crossing segment rules
// eg. LS2521A

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionAnd::extract_crossingsegments(std::vector<SegWptsAlt>& segs) const
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
				p = p->extract_crossingsegments(segs);
		}
		seq.push_back(p);
	}
	return simplify(seq);
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionSequence::extract_crossingsegments(std::vector<SegWptsAlt>& segs) const
{
	bool first(true);
	seq_t seq;
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p;
		if (first) {
			p = *i;
			if (p) {
				std::vector<SegWptsAlt> segs1;
				p = p->extract_crossingsegments(segs1);
				first = segs1.empty();
				if (!first)
					segs.insert(segs.end(), segs1.begin(), segs1.end());
			}
		}
		seq.push_back(p);
	}
	return simplify(seq);
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionCrossingDct::extract_crossingsegments(std::vector<SegWptsAlt>& segs) const
{
	segs.push_back(SegWptsAlt(m_wpt0, m_wpt1, m_alt));
	ptr_t p(new ConditionConstant(m_childnum, true));
	return p;
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionCrossingAirway::extract_crossingsegments(std::vector<SegWptsAlt>& segs) const
{
	if (m_awyname.empty())
		return ptr_t();
	segs.push_back(SegWptsAlt(m_wpt0, m_wpt1, m_alt, m_awyname));
	ptr_t p(new ConditionConstant(m_childnum, true));
	return p;
}

std::vector<TrafficFlowRestrictions::RestrictionElement::ptr_t> TrafficFlowRestrictions::RestrictionElementRoute::clone_crossingsegments(const std::vector<SegWptsAlt>& segs) const
{
	std::vector<ptr_t> p;
	int32_t altl(std::numeric_limits<int32_t>::min()), altu(std::numeric_limits<int32_t>::max());
	if (m_alt.is_lower_valid())
		altl = m_alt.get_lower_alt();
	if (m_alt.is_upper_valid())
		altu = m_alt.get_upper_alt();
	AltRange::alt_t model(m_alt.get_lower_mode()), modeu(m_alt.get_upper_mode());
	for (std::vector<SegWptsAlt>::const_iterator pi(segs.begin()), pe(segs.end()); pi != pe; ++pi) {
		if (m_point[0] != pi->get_wpt0() || m_point[1] != pi->get_wpt1())
			continue;
		if (pi->is_dct()) {
			if (m_match != match_all && m_match != match_dct)
				continue;
		} else {
			if (m_match != match_all && m_match != match_awy)
				continue;
			if (m_rteid != pi->get_airway())
				continue;
		}
		int32_t al(altl), au(altu);
		AltRange::alt_t ml(model), mu(modeu);
		if (pi->get_alt().is_lower_valid()) {
			al = std::max(al, pi->get_alt().get_lower_alt());
			if (ml == AltRange::alt_invalid)
				ml = pi->get_alt().get_lower_mode();
		}
		if (pi->get_alt().is_upper_valid()) {
			au = std::min(au, pi->get_alt().get_upper_alt());
			if (mu == AltRange::alt_invalid)
				mu = pi->get_alt().get_upper_mode();
		}
		if (al == std::numeric_limits<int32_t>::min()) {
			al = 0;
			ml = AltRange::alt_invalid;
		}
		if (au == std::numeric_limits<int32_t>::max())
			mu = AltRange::alt_invalid;
		ptr_t ppp(new RestrictionElementRoute(AltRange(al, ml, au, mu), m_point[0], m_point[1], m_rteid, pi->is_dct() ? match_dct : match_awy));
		p.push_back(ppp);
	}
	return p;
}

bool TrafficFlowRestrictions::RestrictionSequence::clone_crossingsegments(std::vector<RestrictionSequence>& seq, const std::vector<SegWptsAlt>& segs) const
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

bool TrafficFlowRestrictions::Restrictions::clone_crossingsegments(Restrictions& r, const std::vector<SegWptsAlt>& segs) const
{
        for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		if (!i->clone_crossingsegments(r.m_elements, segs))
			return false;
	}
	return true;
}

TrafficFlowRestrictions::TrafficFlowRule::ptr_t TrafficFlowRestrictions::TrafficFlowRule::simplify_complexity_crossingsegments(void) const
{
	if (get_codetype() != codetype_forbidden || !get_condition())
		return ptr_t();
	std::vector<SegWptsAlt> segs;
	Condition::ptr_t cond(get_condition()->extract_crossingsegments(segs));
	if (!cond || segs.empty() || !cond->is_routestatic())
		return ptr_t();
	Restrictions res;
	if (!get_restrictions().clone_crossingsegments(res, segs))
		return ptr_t();
	ptr_t p(new TrafficFlowRule());
	p->set_codetype(get_codetype());
	p->set_mid(get_mid());
	p->set_codeid(get_codeid());
	p->set_oprgoal(get_oprgoal());
	p->set_descr(get_descr());
	p->set_timesheet(get_timesheet());
	p->add_restrictions(res);
	p->set_condition(cond);
	if (false)
		std::cerr << "simplify_complexity_crossingsegments: simplified rule " << get_codeid() << std::endl
			  << "  oldcond:  " << get_condition()->to_str() << std::endl
			  << "  oldrestr: " << get_restrictions().to_str() << std::endl
			  << "  newcond:  " << p->get_condition()->to_str() << std::endl
			  << "  newrestr: " << p->get_restrictions().to_str() << std::endl;
	return p;
}

// mandatory crossing points

bool TrafficFlowRestrictions::RestrictionElementRoute::is_mandatoryinbound(const std::vector<RuleWptAlt>& pts) const
{
	for (std::vector<RuleWptAlt>::const_iterator pi(pts.begin()), pe(pts.end()); pi != pe; ++pi) {
		if (!m_point[1].is_same(pi->get_wpt()))
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

bool TrafficFlowRestrictions::RestrictionSequence::is_mandatoryinbound(const std::vector<RuleWptAlt>& pts) const
{
	if (m_elements.empty() || !m_elements.back())
		return false;
	return m_elements.back()->is_mandatoryinbound(pts);
}

bool TrafficFlowRestrictions::Restrictions::is_mandatoryinbound(const std::vector<RuleWptAlt>& pts) const
{
	if (m_elements.empty())
		return false;
        for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
		if (!i->is_mandatoryinbound(pts))
			return false;
	return true;
}

bool TrafficFlowRestrictions::TrafficFlowRule::is_mandatoryinbound(void) const
{
	if (get_codetype() != codetype_mandatory || !get_condition())
		return false;
	std::vector<RuleWptAlt> pts;
	Condition::ptr_t cond(get_condition()->extract_crossingpoints(pts));
	if (pts.empty())
		return false;
	return get_restrictions().is_mandatoryinbound(pts);
}

// Simplify closed levels

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionAnd::extract_crossingairspaces(std::vector<airspacealt_t>& aspcs) const
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

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionSequence::extract_crossingairspaces(std::vector<airspacealt_t>& aspcs) const
{
	bool first(true);
	seq_t seq;
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p;
		if (first) {
			p = *i;
			if (p) {
				std::vector<airspacealt_t> aspcs1;
				p = p->extract_crossingairspaces(aspcs1);
				first = aspcs1.empty();
				if (!first)
					aspcs.insert(aspcs.end(), aspcs1.begin(), aspcs1.end());
			}
		}
		seq.push_back(p);
	}
	return simplify(seq);
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionCrossingAirspace1::extract_crossingairspaces(std::vector<airspacealt_t>& aspcs) const
{
	aspcs.push_back(airspacealt_t(m_airspace, m_alt));
	ptr_t p(new ConditionConstant(m_childnum, true));
	return p;
}

TrafficFlowRestrictions::TrafficFlowRule::ptr_t TrafficFlowRestrictions::TrafficFlowRule::simplify_complexity_closedairspace(void) const
{
	if (get_codetype() != codetype_closed || !get_condition() || !get_restrictions().empty())
		return ptr_t();
	std::vector<Condition::airspacealt_t> aspcs;
	Condition::ptr_t cond(get_condition()->extract_crossingairspaces(aspcs));
	if (!cond || aspcs.empty() || !cond->is_routestatic())
		return ptr_t();
	Restrictions res;
	for (std::vector<Condition::airspacealt_t>::const_iterator ai(aspcs.begin()), ae(aspcs.end()); ai != ae; ++ai) {
		RestrictionSequence seq;
		RestrictionElement::ptr_t p(new RestrictionElementAirspace(ai->second, ai->first));
		seq.add(p);
		res.add(seq);
	}
	ptr_t p(new TrafficFlowRule());
	p->set_codetype(codetype_forbidden);
	p->set_mid(get_mid());
	p->set_codeid(get_codeid());
	p->set_oprgoal(get_oprgoal());
	p->set_descr(get_descr());
	p->set_timesheet(get_timesheet());
	p->add_restrictions(res);
	p->set_condition(cond);
	if (false)
		std::cerr << "simplify_complexity_closedairspace: simplified rule " << get_codeid() << std::endl
			  << "  oldcond:  " << get_condition()->to_str() << std::endl
			  << "  oldrestr: " << get_restrictions().to_str() << std::endl
			  << "  newcond:  " << p->get_condition()->to_str() << std::endl
			  << "  newrestr: " << p->get_restrictions().to_str() << std::endl;
	return p;
}
