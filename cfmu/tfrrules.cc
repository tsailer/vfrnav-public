#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iomanip>
#include "tfr.hh"
#include "SunriseSunset.h"

TrafficFlowRestrictions::TrafficFlowRule::TrafficFlowRule(void)
	: m_mid(~0ULL), m_refcount(1), m_codetype(codetype_invalid)
{
}

void TrafficFlowRestrictions::TrafficFlowRule::reference(void) const
{
        g_atomic_int_inc(&m_refcount);
}

void TrafficFlowRestrictions::TrafficFlowRule::unreference(void) const
{
        if (!g_atomic_int_dec_and_test(&m_refcount))
                return;
        delete this;
}

char TrafficFlowRestrictions::TrafficFlowRule::get_codetype_char(codetype_t t)
{
	switch (t) {
	default:
	case codetype_invalid:
		return '-';

	case codetype_mandatory:
		return 'M';

	case codetype_forbidden:
		return 'F';

	case codetype_closed:
		return 'C';
	}
}

bool TrafficFlowRestrictions::TrafficFlowRule::is_valid(void) const
{
	if (get_codetype() == codetype_invalid)
		return false;
	return true;
}

void TrafficFlowRestrictions::TrafficFlowRule::set_codetype(char x)
{
	static const codetype_t codetypes[] = {
		codetype_mandatory, codetype_forbidden, codetype_closed
	};

	for (unsigned int i = 0; i < sizeof(codetypes)/sizeof(codetypes[0]); ++i)
		if (x == get_codetype_char(codetypes[i])) {
			m_codetype = codetypes[i];
			return;
		}
	m_codetype = codetype_invalid;
}

void TrafficFlowRestrictions::TrafficFlowRule::set_codetype(const std::string& x)
{
	m_codetype = codetype_invalid;
	if (x.size() != 1)
		return;
	set_codetype(x[0]);
}

void TrafficFlowRestrictions::TrafficFlowRule::clone(void)
{
	if (m_condition) {
		Condition::ptr_t p(m_condition->clone());
		if (p)
			m_condition = p;
	}
	m_restrictions.clone();
}

bool TrafficFlowRestrictions::TrafficFlowRule::is_keep(void) const
{
	if (!get_condition())
		return false;
	if (get_condition()->is_constant() && !get_condition()->get_constvalue())
		return false;
	switch (get_codetype()) {
	case codetype_mandatory:
	case codetype_forbidden:
		if (get_restrictions().empty())
			return false;
		break;

	case codetype_closed:
		break;

	default:
		return false;
	}
	return true;
}

TrafficFlowRestrictions::TrafficFlowRule::ptr_t TrafficFlowRestrictions::TrafficFlowRule::simplify(void) const
{
	if (m_condition) {
		Condition::ptr_t p(m_condition->simplify());
		if (p) {
			ptr_t tr(new TrafficFlowRule(*this));
			tr->m_condition = p;
			return tr;
		}
	}
	return ptr_t();
}

TrafficFlowRestrictions::TrafficFlowRule::ptr_t TrafficFlowRestrictions::TrafficFlowRule::simplify_bbox(const Rect& bbox) const
{
	ptr_t tr(new TrafficFlowRule(*this));
	bool modified(false);
	if (m_condition) {
		Condition::ptr_t p(m_condition->simplify_bbox(bbox));
		if (p) {
			tr->m_condition = p;
			modified = true;
		}
	}
	modified = tr->m_restrictions.simplify_bbox(bbox) || modified;
	if (modified)
		return tr;
	return ptr_t();
}

TrafficFlowRestrictions::TrafficFlowRule::ptr_t TrafficFlowRestrictions::TrafficFlowRule::simplify_altrange(int32_t minalt, int32_t maxalt) const
{
	ptr_t tr(new TrafficFlowRule(*this));
	bool modified(false);
	if (m_condition) {
		Condition::ptr_t p(m_condition->simplify_altrange(minalt, maxalt));
		if (p) {
			tr->m_condition = p;
			modified = true;
		}
	}
	if (get_codetype() == codetype_forbidden)
		modified = tr->m_restrictions.simplify_altrange(minalt, maxalt) || modified;
	if (modified)
		return tr;
	return ptr_t();
}

TrafficFlowRestrictions::TrafficFlowRule::ptr_t TrafficFlowRestrictions::TrafficFlowRule::simplify_aircrafttype(const std::string& acfttype) const
{
	if (m_condition) {
		Condition::ptr_t p(m_condition->simplify_aircrafttype(acfttype));
		if (p) {
			ptr_t tr(new TrafficFlowRule(*this));
			tr->m_condition = p;
			return tr;
		}
	}
	return ptr_t();
}

TrafficFlowRestrictions::TrafficFlowRule::ptr_t TrafficFlowRestrictions::TrafficFlowRule::simplify_aircraftclass(const std::string& acftclass) const
{
	if (m_condition) {
		Condition::ptr_t p(m_condition->simplify_aircraftclass(acftclass));
		if (p) {
			ptr_t tr(new TrafficFlowRule(*this));
			tr->m_condition = p;
			return tr;
		}
	}
	return ptr_t();
}

TrafficFlowRestrictions::TrafficFlowRule::ptr_t TrafficFlowRestrictions::TrafficFlowRule::simplify_equipment(const std::string& equipment, Aircraft::pbn_t pbn) const
{
	if (m_condition) {
		Condition::ptr_t p(m_condition->simplify_equipment(equipment, pbn));
		if (p) {
			ptr_t tr(new TrafficFlowRule(*this));
			tr->m_condition = p;
			return tr;
		}
	}
	return ptr_t();
}

TrafficFlowRestrictions::TrafficFlowRule::ptr_t TrafficFlowRestrictions::TrafficFlowRule::simplify_typeofflight(char type_of_flight) const
{
	if (m_condition) {
		Condition::ptr_t p(m_condition->simplify_typeofflight(type_of_flight));
		if (p) {
			ptr_t tr(new TrafficFlowRule(*this));
			tr->m_condition = p;
			return tr;
		}
	}
	return ptr_t();
}

TrafficFlowRestrictions::TrafficFlowRule::ptr_t TrafficFlowRestrictions::TrafficFlowRule::simplify_mil(bool mil) const
{
	if (m_condition) {
		Condition::ptr_t p(m_condition->simplify_mil(mil));
		if (p) {
			ptr_t tr(new TrafficFlowRule(*this));
			tr->m_condition = p;
			return tr;
		}
	}
	return ptr_t();
}

TrafficFlowRestrictions::TrafficFlowRule::ptr_t TrafficFlowRestrictions::TrafficFlowRule::simplify_gat(bool gat) const
{
	if (m_condition) {
		Condition::ptr_t p(m_condition->simplify_gat(gat));
		if (p) {
			ptr_t tr(new TrafficFlowRule(*this));
			tr->m_condition = p;
			return tr;
		}
	}
	return ptr_t();
}

TrafficFlowRestrictions::TrafficFlowRule::ptr_t TrafficFlowRestrictions::TrafficFlowRule::simplify_dep(const std::string& ident, const Point& coord) const
{
	if (m_condition) {
		Condition::ptr_t p(m_condition->simplify_dep(ident, coord));
		if (p) {
			ptr_t tr(new TrafficFlowRule(*this));
			tr->m_condition = p;
			return tr;
		}
	}
	return ptr_t();
}

TrafficFlowRestrictions::TrafficFlowRule::ptr_t TrafficFlowRestrictions::TrafficFlowRule::simplify_dest(const std::string& ident, const Point& coord) const
{
	if (m_condition) {
		Condition::ptr_t p(m_condition->simplify_dest(ident, coord));
		if (p) {
			ptr_t tr(new TrafficFlowRule(*this));
			tr->m_condition = p;
			return tr;
		}
	}
	return ptr_t();
}

TrafficFlowRestrictions::TrafficFlowRule::ptr_t TrafficFlowRestrictions::TrafficFlowRule::simplify_complexity(void) const
{
	ptr_t p(simplify_complexity_crossingpoints());
	if (p)
		return p;
	p = simplify_complexity_crossingsegments();
	if (p)
		return p;
	p = simplify_complexity_closedairspace();
	if (p)
		return p;
	return ptr_t();
}

bool TrafficFlowRestrictions::TrafficFlowRule::evaluate(TrafficFlowRestrictions& tfrs, Result& result) const
{
	return evaluate(tfrs, result.m_rules);
}

bool TrafficFlowRestrictions::TrafficFlowRule::evaluate(TrafficFlowRestrictions& tfrs, std::vector<RuleResult>& result) const
{
	bool trace(tfrs.tracerules_exists(get_codeid()));
	bool disabled(tfrs.disabledrules_exists(get_codeid()));
	if (true && disabled)
		tfrs.message("Rule disabled", Message::type_info, get_codeid());
       	CondResult r(false);
	if (get_condition()) {
		if (trace) {
			tfrs.message("Condition: " + get_condition()->to_str(), Message::type_info, get_codeid());
			r = get_condition()->evaluate_trace(tfrs, CondResult::set_t(), this, 0);
		} else {
			r = get_condition()->evaluate(tfrs, CondResult::set_t());
		}
	}
	if (r.get_result() == boost::logic::indeterminate) {
		tfrs.message("Cannot evaluate condition", Message::type_warning, get_codeid());
		return true;
	}
	if (r.get_result() != true)
		return true;
	if (!r.get_reflocset().empty()) {
		unsigned int idx(*r.get_reflocset().begin());
		if (idx < tfrs.m_route.size()) {
			const FplWpt& wpt(tfrs.m_route[idx]);
			if (wpt.get_time_unix() > 48 * 60 * 60 &&
			    !m_timesheet.is_inside(wpt.get_time_unix(), wpt.get_coord())) {
				if (trace)
					tfrs.message("Reference time outside applicability window", Message::type_info, get_codeid());
				return true;
			}
		}
	}
	RuleResult ruleresult(get_codeid(), get_descr(), get_oprgoal(), get_condition() ? get_condition()->to_str() : "",
			      (RuleResult::codetype_t)get_codetype(), is_dct(), is_unconditional(), is_routestatic(),
			      is_mandatoryinbound(), disabled, r.get_vertexset(), r.get_edgeset(), r.get_reflocset());
	if (get_codetype() == codetype_closed) {
		if (!get_restrictions().empty()) 
		        tfrs.message("Rule closed for cruising has Tre elements", Message::type_warning, get_codeid());
		result.push_back(ruleresult);
		return disabled;
	}
	if (trace) {
		if (get_restrictions().evaluate_trace(tfrs, ruleresult, get_codetype() == codetype_forbidden, this))
			return true;
	} else {
		if (get_restrictions().evaluate(tfrs, ruleresult, get_codetype() == codetype_forbidden))
			return true;
	}
	if (get_condition()) {
		if (get_codetype() == codetype_mandatory && (true || get_codeid() == "EG2369A")) {
			// try to derive other mandatory options from the rule condition
			// I: R:EG2369A Condition: And{5}(XngAirspace{0,R}(F065-F999,EG/NAS),!Or{0}(XngPoint{0}(!A,DVR),XngPoint{1}(!A,LYD)))
			std::vector<RuleResult::Alternative> r(get_condition()->get_mandatory());
			for (std::vector<RuleResult::Alternative>::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri)
				ruleresult.add_element(*ri);
		}
		// record crossing condition
		{
			std::vector<RuleWptAlt> pts;
			std::vector<SegWptsAlt> segs;
			std::vector<Condition::airspacealt_t> aspcs;
			Condition::const_ptr_t p(get_condition());
			{
				Condition::const_ptr_t p1(p->extract_crossingpoints(pts));
				if (p1)
					p.swap(p1);
			}
			{
				Condition::const_ptr_t p1(p->extract_crossingsegments(segs));
				if (p1)
					p.swap(p1);
			}
			{
				Condition::const_ptr_t p1(p->extract_crossingairspaces(aspcs));
				if (p1)
					p.swap(p1);
			}
			if (p->is_routestatic()) {
				for (std::vector<RuleWptAlt>::const_iterator i(pts.begin()), e(pts.end()); i != e; ++i)
					ruleresult.add_crossingcond(RuleResult::Alternative::Sequence(*i));
				for (std::vector<SegWptsAlt>::const_iterator i(segs.begin()), e(segs.end()); i != e; ++i)
					ruleresult.add_crossingcond(RuleResult::Alternative::Sequence(*i));
				for (std::vector<Condition::airspacealt_t>::const_iterator i(aspcs.begin()), e(aspcs.end()); i != e; ++i)
					ruleresult.add_crossingcond(RuleResult::Alternative::Sequence(i->first, i->second));
			}
		}
	}
	result.push_back(ruleresult);
	return disabled;
}

bool TrafficFlowRestrictions::TrafficFlowRule::is_unconditional_airspace(void) const
{
	// heuristic: if airspace crossing is forbidden and condition for it is crossing the airspace, treat as unconditional
	if (get_restrictions().size() != 1 || get_restrictions()[0].size() != 1)
		return false;
	Glib::RefPtr<const ConditionCrossingAirspace1> c(Glib::RefPtr<const ConditionCrossingAirspace1>::cast_dynamic(get_condition()));
	if (!c)
		return false;
	if (false && get_codeid().substr(0, 6) == "EGD201")
		std::cerr << "Rule " << get_codeid() << ": is_unconditional_airspace: Alt Valid: L"
			  << (c->get_alt().is_lower_valid() ? '+' : '-')
			  << " U" << (c->get_alt().is_upper_valid() ? '+' : '-') << std::endl;
	if (c->get_alt().is_lower_valid() || c->get_alt().is_upper_valid())
		return false;
	Glib::RefPtr<const RestrictionElementAirspace> re(Glib::RefPtr<const RestrictionElementAirspace>::cast_dynamic(get_restrictions()[0][0]));
	if (!re)
		return false;
	TFRAirspace::const_ptr_t ca(c->get_airspace());
	if (!ca)
		return false;
	TFRAirspace::const_ptr_t rea(re->get_airspace());
	if (!rea)
		return false;
	if (false && get_codeid().substr(0, 6) == "EGD201")
		std::cerr << "Rule " << get_codeid() << ": is_unconditional_airspace: Cond "
			  << ca->get_ident() << '/' << AirspacesDb::Airspace::get_class_string(ca->get_bdryclass(), ca->get_typecode())
			  << ' ' << ca->get_lower_alt() << ".." << ca->get_upper_alt() << " Restriction "
			  << rea->get_ident() << '/' << AirspacesDb::Airspace::get_class_string(rea->get_bdryclass(), rea->get_typecode())
			  << ' ' << rea->get_lower_alt() << ".." << rea->get_upper_alt() << std::endl;
	return (rea->get_ident() == ca->get_ident() &&
		rea->get_typecode() == ca->get_typecode() &&
		rea->get_bdryclass() == ca->get_bdryclass() &&
		rea->get_lower_alt() == ca->get_lower_alt() &&
		rea->get_upper_alt() == ca->get_upper_alt());
}

bool TrafficFlowRestrictions::TrafficFlowRule::is_unconditional(void) const
{
	if (!get_condition())
		return false;
	if (get_condition()->is_constant() && get_condition()->get_constvalue())
		return true;
	if (is_unconditional_airspace())
		return true;
	return false;
}

bool TrafficFlowRestrictions::TrafficFlowRule::is_dct(void) const
{
	if (!get_condition())
		return false;
	if (false && get_codeid() == "LS1A")
		std::cerr << get_codeid() << ": check condition DCT" << std::endl;
	if (!get_condition()->is_dct())
		return false;
	if (false && get_codeid() == "LS1A")
		std::cerr << get_codeid() << ": check condition valid DCT" << std::endl;
	if (!get_condition()->is_valid_dct())
		return false;
	if (false && get_codeid() == "LS1A")
		std::cerr << get_codeid() << ": check restriction valid DCT" << std::endl;
	if (!get_restrictions().is_valid_dct())
		return false;
	if (false && get_codeid() == "LS1A")
		std::cerr << get_codeid() << ": check is DCT" << std::endl;
	return true;
}

bool TrafficFlowRestrictions::TrafficFlowRule::is_routestatic(void) const
{
	if (!get_condition())
		return true;
	return get_condition()->is_routestatic();
}

TrafficFlowRestrictions::DctParameters::AltRange TrafficFlowRestrictions::TrafficFlowRule::evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct, bool trace) const
{
	if (tfrs.tracerules_exists(get_codeid()))
		trace = true;
	bool disabled(tfrs.disabledrules_exists(get_codeid()));
       	DctParameters::AltRange r;
	r.set_empty();
	if (get_condition()) {
		if (trace) {
			std::ostringstream oss;
			oss << dct.get_ident(0) << ' ' << dct.get_ident(1) << ' ' << dct.get_alt(0).to_str() << " / " << dct.get_alt(1).to_str()
			    << " D" << std::fixed << std::setprecision(1) << dct.get_dist()
			    << " Condition: " + get_condition()->to_str();
			tfrs.message(oss.str(), Message::type_info, get_codeid());
			r = get_condition()->evaluate_dct_trace(tfrs, dct, this, 0);
		} else {
			r = get_condition()->evaluate_dct(tfrs, dct);
		}
	}
	r.invert();
	if (false && trace)
		tfrs.message("Inverted Condition: " + r[0].to_str() + " / " + r[1].to_str(), Message::type_info, get_codeid());
	switch (get_codetype()) {
	case codetype_closed:
		if (!get_restrictions().empty()) 
		        tfrs.message("Rule closed for cruising has Tre elements", Message::type_warning, get_codeid());
		break;

	case codetype_forbidden:
	case codetype_mandatory:
		if (trace) {
			r |= get_restrictions().evaluate_dct_trace(tfrs, dct, get_codetype() == codetype_forbidden, this);
		} else {
			r |= get_restrictions().evaluate_dct(tfrs, dct, get_codetype() == codetype_forbidden);
		}
		break;

	default:
		r.set_full();
		break;
	}
	if (false && trace)
		tfrs.message("After Restrictions: " + r[0].to_str() + " / " + r[1].to_str(), Message::type_info, get_codeid());
	r &= dct.get_alt();
	if (trace)
		tfrs.message("Result: " + r[0].to_str() + " / " + r[1].to_str(), Message::type_info, get_codeid());
	if (disabled)
		r.set_full();
	return r;
}

void TrafficFlowRestrictions::TrafficFlowRule::get_dct_segments(DctSegments& seg) const
{
	get_restrictions().get_dct_segments(seg);
}

TrafficFlowRestrictions::RuleResult TrafficFlowRestrictions::TrafficFlowRule::get_rule(void) const
{
	RuleResult r(get_codeid(), get_descr(), get_oprgoal(), get_condition() ? get_condition()->to_str() : "",
		     (RuleResult::codetype_t)get_codetype(), is_dct(), is_unconditional(), is_routestatic(), false);
	get_restrictions().set_ruleresult(r);
	return r;
}

void TrafficFlowRestrictions::TrafficFlowRules::simplify(void)
{
	for (iterator i(begin()), e(end()); i != e; ) {
		if (*i) {
			if (false && (*i)->get_condition())
				std::cerr << "simplify: cond before: " << (*i)->get_condition()->to_str() << std::endl;
			TrafficFlowRule::ptr_t p((*i)->simplify());
			if (p)
				*i = p;
			if (false && (*i)->get_condition())
				std::cerr << "simplify: cond after: " << (*i)->get_condition()->to_str() << std::endl;
			if ((*i)->is_keep()) {
				++i;
				continue;
			}
		}
		i = erase(i);
		e = end();
	}
}

void TrafficFlowRestrictions::TrafficFlowRules::simplify_bbox(const Rect& bbox)
{
	for (iterator i(begin()), e(end()); i != e; ) {
		if (*i) {
			TrafficFlowRule::ptr_t p((*i)->simplify_bbox(bbox));
			if (p)
				*i = p;
			if ((*i)->is_keep()) {
				++i;
				continue;
			}
		}
		i = erase(i);
		e = end();
	}
}

void TrafficFlowRestrictions::TrafficFlowRules::simplify_altrange(int32_t minalt, int32_t maxalt)
{
	for (iterator i(begin()), e(end()); i != e; ) {
		if (*i) {
			TrafficFlowRule::ptr_t p((*i)->simplify_altrange(minalt, maxalt));
			if (p)
				*i = p;
			if ((*i)->is_keep()) {
				++i;
				continue;
			}
		}
		i = erase(i);
		e = end();
	}
}

void TrafficFlowRestrictions::TrafficFlowRules::simplify_aircrafttype(const std::string& acfttype)
{
	for (iterator i(begin()), e(end()); i != e; ) {
		if (*i) {
			TrafficFlowRule::ptr_t p((*i)->simplify_aircrafttype(acfttype));
			if (p)
				*i = p;
			if ((*i)->is_keep()) {
				++i;
				continue;
			}
		}
		i = erase(i);
		e = end();
	}
}

void TrafficFlowRestrictions::TrafficFlowRules::simplify_aircraftclass(const std::string& acftclass)
{
	for (iterator i(begin()), e(end()); i != e; ) {
		if (*i) {
			TrafficFlowRule::ptr_t p((*i)->simplify_aircraftclass(acftclass));
			if (p)
				*i = p;
			if ((*i)->is_keep()) {
				++i;
				continue;
			}
		}
		i = erase(i);
		e = end();
	}
}

void TrafficFlowRestrictions::TrafficFlowRules::simplify_equipment(const std::string& equipment, Aircraft::pbn_t pbn)
{
	for (iterator i(begin()), e(end()); i != e; ) {
		if (*i) {
			TrafficFlowRule::ptr_t p((*i)->simplify_equipment(equipment, pbn));
			if (p)
				*i = p;
			if ((*i)->is_keep()) {
				++i;
				continue;
			}
		}
		i = erase(i);
		e = end();
	}
}

void TrafficFlowRestrictions::TrafficFlowRules::simplify_typeofflight(char type_of_flight)
{
	for (iterator i(begin()), e(end()); i != e; ) {
		if (*i) {
			TrafficFlowRule::ptr_t p((*i)->simplify_typeofflight(type_of_flight));
			if (p)
				*i = p;
			if ((*i)->is_keep()) {
				++i;
				continue;
			}
		}
		i = erase(i);
		e = end();
	}
}

void TrafficFlowRestrictions::TrafficFlowRules::simplify_mil(bool mil)
{
	for (iterator i(begin()), e(end()); i != e; ) {
		if (*i) {
			TrafficFlowRule::ptr_t p((*i)->simplify_mil(mil));
			if (p)
				*i = p;
			if ((*i)->is_keep()) {
				++i;
				continue;
			}
		}
		i = erase(i);
		e = end();
	}
}

void TrafficFlowRestrictions::TrafficFlowRules::simplify_gat(bool gat)
{
	for (iterator i(begin()), e(end()); i != e; ) {
		if (*i) {
			TrafficFlowRule::ptr_t p((*i)->simplify_gat(gat));
			if (p)
				*i = p;
			if ((*i)->is_keep()) {
				++i;
				continue;
			}
		}
		i = erase(i);
		e = end();
	}
}

void TrafficFlowRestrictions::TrafficFlowRules::simplify_dep(const std::string& ident, const Point& coord)
{
	for (iterator i(begin()), e(end()); i != e; ) {
		if (*i) {
			TrafficFlowRule::ptr_t p((*i)->simplify_dep(ident, coord));
			if (p)
				*i = p;
			if ((*i)->is_keep()) {
				++i;
				continue;
			}
		}
		i = erase(i);
		e = end();
	}
}

void TrafficFlowRestrictions::TrafficFlowRules::simplify_dest(const std::string& ident, const Point& coord)
{
	for (iterator i(begin()), e(end()); i != e; ) {
		if (*i) {
			TrafficFlowRule::ptr_t p((*i)->simplify_dest(ident, coord));
			if (p)
				*i = p;
			if ((*i)->is_keep()) {
				++i;
				continue;
			}
		}
		i = erase(i);
		e = end();
	}
}

void TrafficFlowRestrictions::TrafficFlowRules::simplify_complexity(void)
{
	for (iterator i(begin()), e(end()); i != e; ) {
		if (*i) {
			TrafficFlowRule::ptr_t p((*i)->simplify_complexity());
			if (p)
				*i = p;
			if ((*i)->is_keep()) {
				++i;
				continue;
			}
		}
		i = erase(i);
		e = end();
	}
}

bool TrafficFlowRestrictions::TrafficFlowRules::evaluate(TrafficFlowRestrictions& tfrs, Result& result) const
{
	return evaluate(tfrs, result.m_rules);
}

bool TrafficFlowRestrictions::TrafficFlowRules::evaluate(TrafficFlowRestrictions& tfrs, std::vector<RuleResult>& result) const
{
	bool ret(true);
	unsigned int nrrules(0);
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		if (!*i)
			continue;
		++nrrules;
		bool r((*i)->evaluate(tfrs, result));
		ret = r && ret;
		if (false)
			std::cerr << "Rule " << (*i)->get_codeid() << ": " << (r ? "OK" : "FAIL") << std::endl;
	}
	if (true) {
		std::ostringstream oss;
		oss << "Rules processed: " << nrrules << " Result: " << (ret ? "OK" : "FAIL");
		tfrs.message(oss.str(), Message::type_info);
	}
	return ret;
}

TrafficFlowRestrictions::DctParameters::AltRange TrafficFlowRestrictions::TrafficFlowRules::evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct, bool trace) const
{
       	DctParameters::AltRange r(dct.get_alt());
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		if (!*i)
			continue;
		r &= (*i)->evaluate_dct(tfrs, dct, trace);
		if (r.is_empty())
			break;
	}
	if (trace) {
		std::ostringstream oss;
		oss << dct.get_ident(0) << ' ' << dct.get_ident(1)
		    << " D" << std::fixed << std::setprecision(1) << dct.get_dist()
		    << " result " << r[0].to_str() << " / " << r[1].to_str();
		tfrs.message(oss.str(), Message::type_info);
	}
	return r;
}

void TrafficFlowRestrictions::TrafficFlowRules::get_dct_segments(DctSegments& seg) const
{
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		if (!*i)
			continue;
		(*i)->get_dct_segments(seg);
	}
}

std::vector<TrafficFlowRestrictions::RuleResult> TrafficFlowRestrictions::TrafficFlowRules::find_rules(const std::string& name, bool exact) const
{
	std::vector<TrafficFlowRestrictions::RuleResult> r;
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		if (!*i)
			continue;
		if (exact) {
			if ((*i)->get_codeid() != name)
				continue;
		} else {
			if ((*i)->get_codeid().compare(0, name.size(), name))
				continue;
		}
		r.push_back((*i)->get_rule());
	}
	return r;
}

TrafficFlowRestrictions::Timesheet::Element::Element(void)
	: m_timewef(~0U), m_timetil(~0U), m_monwef(~0U), m_montil(~0U), m_daywef(~0U), m_daytil(~0U),
	  m_day(~0U), m_timeref(timeref_invalid), m_sunrise(false), m_sunset(false)
{
}

const std::string& TrafficFlowRestrictions::Timesheet::Element::get_day_string(unsigned int d)
{
	switch (d) {
	case 0:
	{
		static const std::string r("SUN");
		return r;
	}

	case 1:
	{
		static const std::string r("MON");
		return r;
	}

	case 2:
	{
		static const std::string r("TUE");
		return r;
	}

	case 3:
	{
		static const std::string r("WED");
		return r;
	}

	case 4:
	{
		static const std::string r("THU");
		return r;
	}

	case 5:
	{
		static const std::string r("FRI");
		return r;
	}

	case 6:
	{
		static const std::string r("SAT");
		return r;
	}

	case 7:
	{
		static const std::string r("WD");
		return r;
	}

	case 8:
	{
		static const std::string r("LH");
		return r;
	}

	case 9:
	{
		static const std::string r("ANY");
		return r;
	}

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}
		
const std::string& TrafficFlowRestrictions::Timesheet::Element::get_timeref_string(timeref_t t)
{
	switch (t) {
	case timeref_utc:
	{
		static const std::string r("UTC");
		return r;
	}

	case timeref_utcw:
	{
		static const std::string r("UTCW");
		return r;
	}

	case timeref_invalid:
	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

void TrafficFlowRestrictions::Timesheet::Element::set_datewef(const std::string& x)
{
	if (x.size() != 5 || !std::isdigit(x[0])  || !std::isdigit(x[1]) ||
	    x[2] != '-' || !std::isdigit(x[3])  || !std::isdigit(x[4])) {
		m_monwef = ~0U;
		m_daywef = ~0U;
		return;
	}
	m_daywef = (x[0] - '0') * 10 + (x[1] - '0');
	m_monwef = (x[3] - '0') * 10 + (x[4] - '0') - 1;
}

void TrafficFlowRestrictions::Timesheet::Element::set_datetil(const std::string& x)
{
	if (x.size() != 5 || !std::isdigit(x[0])  || !std::isdigit(x[1]) ||
	    x[2] != '-' || !std::isdigit(x[3])  || !std::isdigit(x[4])) {
		m_montil = ~0U;
		m_daytil = ~0U;
		return;
	}
	m_daytil = (x[0] - '0') * 10 + (x[1] - '0');
	m_montil = (x[3] - '0') * 10 + (x[4] - '0') - 1;
}

void TrafficFlowRestrictions::Timesheet::Element::set_timewef(const std::string& x)
{
	if (x.size() != 5 || !std::isdigit(x[0])  || !std::isdigit(x[1]) ||
	    x[2] != ':' || !std::isdigit(x[3])  || !std::isdigit(x[4])) {
		m_timewef = ~0U;
		return;
	}
	m_timewef = (x[0] - '0') * 36000 + (x[1] - '0') * 3600 + (x[3] - '0') * 600 + (x[4] - '0') * 60;
}

void TrafficFlowRestrictions::Timesheet::Element::set_timetil(const std::string& x)
{
	if (x.size() != 5 || !std::isdigit(x[0])  || !std::isdigit(x[1]) ||
	    x[2] != ':' || !std::isdigit(x[3])  || !std::isdigit(x[4])) {
		m_timetil = ~0U;
		return;
	}
	m_timetil = (x[0] - '0') * 36000 + (x[1] - '0') * 3600 + (x[3] - '0') * 600 + (x[4] - '0') * 60;
}

void TrafficFlowRestrictions::Timesheet::Element::set_day(const std::string& x)
{
	for (unsigned int i = 0; i < 10; ++i)
		if (get_day_string(i) == x) {
			m_day = i;
			return;
		}
	m_day = ~0U;
}

void TrafficFlowRestrictions::Timesheet::Element::set_timeref(const std::string& x)
{
	for (timeref_t i = timeref_first; i < timeref_invalid; i = (timeref_t)(i + 1))
		if (get_timeref_string(i) == x) {
			m_timeref = i;
			return;
		}
	m_timeref = timeref_invalid;
}

bool TrafficFlowRestrictions::Timesheet::Element::is_valid(void) const
{
	return (get_timeref() < timeref_invalid &&
		(is_sunrise() || (get_timewef() >= 0 && get_timewef() < 24*60*60)) &&
		(is_sunset() || (get_timetil() >= 0 && get_timetil() < 24*60*60)) &&
		get_monwef() < 12 &&
		get_montil() < 12 &&
		get_daywef() >= 1 && get_daywef() <= 31 &&
		get_daytil() >= 1 && get_daytil() <= 31 &&
		get_day() < 10);
}

bool TrafficFlowRestrictions::Timesheet::Element::is_inside(time_t t, const Point& pt) const
{
	if (!is_valid())
		return false;
	struct tm tm;
	if (!gmtime_r(&t, &tm))
		return false;
	switch (get_day()) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
		if (tm.tm_wday != get_day())
			return false;
		break;

	case 7:
		if (tm.tm_wday < 1 || tm.tm_wday > 5)
			return false;
		break;

	case 8:
		if (false && tm.tm_wday != 0)
			return false;
		break;

	default:
		break;
	}
	{
		int mon(tm.tm_mon + 1);
		if (get_monwef() > get_montil() || (get_monwef() == get_montil() && get_daywef() > get_daytil())) {
			if ((mon > get_montil() || (mon == get_montil() && tm.tm_mday > get_daytil())) &&
			    (mon < get_monwef() || (mon == get_monwef() && tm.tm_mday < get_daywef())))
				return false;
		} else {
			if ((mon < get_monwef() || (mon == get_monwef() && tm.tm_mday < get_daywef())) ||
			    (mon > get_montil() || (mon == get_montil() && tm.tm_mday > get_daytil())))
				return false;
		}
	}
	int32_t twef(get_timewef()), ttil(get_timetil());
	switch (get_timeref()) {
	case timeref_utcw:
		if (tm.tm_isdst)
			break;
		if (!is_sunrise())
			twef += 3600;
		if (!is_sunset())
			ttil += 3600;
		break;

	case timeref_utc:
		break;

	default:
		return false;
	}
	if (is_sunrise() || is_sunset()) {
                double sr, ss;
                int r = SunriseSunset::civil_twilight(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, pt, sr, ss);
                if (r) {
			if (r < 0)
				return false;
			return true;
                }
		if (is_sunrise())
			twef += sr * 3600;
		if (is_sunset())
			ttil += ss * 3600;
	}
	while (twef < 0)
		twef += 24 * 60 * 60;
	while (twef >= 24 * 60 * 60)
		twef -= 24 * 60 * 60;
	while (ttil < 0)
		ttil += 24 * 60 * 60;
	while (ttil >= 24 * 60 * 60)
		ttil -= 24 * 60 * 60;
	int32_t tt(tm.tm_sec + tm.tm_min * 60 + tm.tm_hour * 60 * 60);
	if (twef > ttil) {
		if (tt > ttil && tt < twef)
			return false;
	} else {
		if (tt < twef || tt > ttil)
			return false;
	}
	return true;
}

std::string TrafficFlowRestrictions::Timesheet::Element::to_ustr(void) const
{
	std::ostringstream oss;
	oss << get_timeref_string() << ' ' << get_day_string() << ' '
	    << std::setfill('0') << std::setw(2) << get_daywef() << '-'
	    << std::setfill('0') << std::setw(2) << (get_monwef() + 1) << ' ';
	if (is_sunrise()) {
		oss << "SR";
		if (get_timewef())
			oss << std::showpos << (get_timewef() / 60) << ' ';
	} else {
		unsigned int x(get_timewef() / 60);
		oss << std::setfill('0') << std::setw(2) << (x / 60) << ':'
		    << std::setfill('0') << std::setw(2) << (x % 60) << ' ';
	}
	oss << std::setfill('0') << std::setw(2) << get_daytil() << '-'
	    << std::setfill('0') << std::setw(2) << (get_montil() + 1) << ' ';
	if (is_sunset()) {
		oss << "SS";
		if (get_timetil())
			oss << std::showpos << (get_timetil() / 60) << ' ';
	} else {
		unsigned int x(get_timetil() / 60);
		oss << std::setfill('0') << std::setw(2) << (x / 60) << ':'
		    << std::setfill('0') << std::setw(2) << (x % 60) << ' ';
	}
	return oss.str();
}

TrafficFlowRestrictions::Timesheet::Timesheet(void)
	: m_workhr(workhr_invalid)
{
}

const std::string& TrafficFlowRestrictions::Timesheet::get_workhr_string(workhr_t x)
{
	switch (x) {
	case workhr_h24:
	{
		static const std::string r("H24");
		return r;
	}

	case workhr_timesheet:
	{
		static const std::string r("TIMSH");
		return r;
	}

	case workhr_invalid:
	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

void TrafficFlowRestrictions::Timesheet::set_workhr(const std::string& x)
{
	for (workhr_t h = workhr_first; h < workhr_invalid; h = (workhr_t)(h + 1))
		if (get_workhr_string(h) == x) {
			m_workhr = h;
			return;
		}
	m_workhr = workhr_invalid;
}

bool TrafficFlowRestrictions::Timesheet::is_valid(void) const
{
	if (get_workhr() >= workhr_invalid)
		return false;
	// H24 with timesheets means that always except when the timesheet matches
	if ((get_workhr() == workhr_timesheet) && m_elements.empty())
		return false;
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
		if (!i->is_valid())
			return false;
	return true;
}

bool TrafficFlowRestrictions::Timesheet::is_inside(time_t t, const Point& pt) const
{
	switch (get_workhr()) {
	case workhr_h24:
		return true;

	case workhr_timesheet:
		for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
			if (i->is_inside(t, pt))
				return true;
		return false;

	case workhr_invalid:
	default:
		return false;
	}
}

std::string TrafficFlowRestrictions::Timesheet::to_str(void) const
{
	std::ostringstream oss;
	oss << get_workhr_string();
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
		oss << ", " << i->to_ustr();
	return oss.str();
}

TrafficFlowRestrictions::AltRange::AltRange(int32_t lwr, alt_t lwrmode, int32_t upr, alt_t uprmode)
	: m_lwralt(lwr), m_upralt(upr), m_lwrmode(lwrmode), m_uprmode(uprmode)
{
}
	
const std::string& TrafficFlowRestrictions::AltRange::get_mode_string(alt_t x)
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

	case alt_invalid:
	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

void TrafficFlowRestrictions::AltRange::set_lower_mode(const std::string& x)
{
	for (alt_t a(alt_first); a < alt_invalid; a = (alt_t)(a + 1))
		if (get_mode_string(a) == x) {
			set_lower_mode(a);
			return;
		}
	set_lower_mode(alt_invalid);
}

void TrafficFlowRestrictions::AltRange::set_upper_mode(const std::string& x)
{
	for (alt_t a(alt_first); a < alt_invalid; a = (alt_t)(a + 1))
		if (get_mode_string(a) == x) {
			set_upper_mode(a);
			return;
		}
	set_upper_mode(alt_invalid);
}

bool TrafficFlowRestrictions::AltRange::is_lower_valid(void) const
{
	return get_lower_mode() < alt_invalid;
}

bool TrafficFlowRestrictions::AltRange::is_upper_valid(void) const
{
	return get_upper_mode() < alt_invalid;
}

bool TrafficFlowRestrictions::AltRange::is_valid(void) const
{
	return is_lower_valid() && is_upper_valid();
}

int32_t TrafficFlowRestrictions::AltRange::get_lower_alt_if_valid(void) const
{
	if (is_lower_valid())
		return get_lower_alt();
	return std::numeric_limits<int32_t>::min();
}

int32_t TrafficFlowRestrictions::AltRange::get_upper_alt_if_valid(void) const
{
	if (is_upper_valid())
		return get_upper_alt();
	return std::numeric_limits<int32_t>::min();
}

std::string TrafficFlowRestrictions::AltRange::to_str(void) const
{
	if (!(is_lower_valid() || is_upper_valid()))
		return "!A";
	std::ostringstream oss;
	if (is_lower_valid()) {
		switch (get_lower_mode()) {
		case alt_qnh:
			oss << 'A';
			break;

		case alt_std:
			oss << 'F';
			break;

		case alt_height:
			oss << 'H';
			break;

		default:
			oss << 'X';
			break;
		}
		oss << std::setw(3) << std::setfill('0') << (std::min(get_lower_alt(), (int32_t)99900) / 100);
	}
	oss << '-';
	if (is_upper_valid()) {
		switch (get_upper_mode()) {
		case alt_qnh:
			oss << 'A';
			break;

		case alt_std:
			oss << 'F';
			break;

		case alt_height:
			oss << 'H';
			break;

		default:
			oss << 'X';
			break;
		}
		oss << std::setw(3) << std::setfill('0') << ((std::min(get_upper_alt(), (int32_t)99900) + 99) / 100);
	}
	return oss.str();
}

bool TrafficFlowRestrictions::TFRAirspace::PointCache::operator<(const PointCache& x) const
{
	if (m_point.get_lat() < x.m_point.get_lat())
		return true;
	if (x.m_point.get_lat() < m_point.get_lat())
		return false;
	return m_point.get_lon() < x.m_point.get_lon();
}

bool TrafficFlowRestrictions::TFRAirspace::LineCache::operator<(const LineCache& x) const
{
	if (m_point0.get_lat() < x.m_point0.get_lat())
		return true;
	if (x.m_point0.get_lat() < m_point0.get_lat())
		return false;
	if (m_point0.get_lon() < x.m_point0.get_lon())
		return true;
	if (x.m_point0.get_lon() < m_point0.get_lon())
		return false;
	if (m_point1.get_lat() < x.m_point1.get_lat())
		return true;
	if (x.m_point1.get_lat() < m_point1.get_lat())
		return false;
	return m_point1.get_lon() < x.m_point1.get_lon();
}

TrafficFlowRestrictions::TFRAirspace::TFRAirspace(const std::string& id, const std::string& type, const MultiPolygonHole& poly, const Rect& bbox,
						  char bc, uint8_t tc, int32_t altlwr, int32_t altupr)
	: m_refcount(1), m_id(id), m_type(type), m_poly(poly), m_bbox(bbox), m_lwralt(altlwr), m_upralt(altupr), m_bdryclass(bc), m_typecode(tc)
{
}

void TrafficFlowRestrictions::TFRAirspace::reference(void) const
{
        g_atomic_int_inc(&m_refcount);
}

void TrafficFlowRestrictions::TFRAirspace::unreference(void) const
{
        if (!g_atomic_int_dec_and_test(&m_refcount))
                return;
        delete this;
}

void TrafficFlowRestrictions::TFRAirspace::add_component(const_ptr_t aspc, AirspacesDb::Airspace::Component::operator_t oper)
{
	m_comps.push_back(comp_t(aspc, oper));
}

bool TrafficFlowRestrictions::TFRAirspace::is_inside(const Point& coord, int32_t alt, int32_t altlwr, int32_t altupr) const
{
	if (m_comps.empty()) {
		if (altlwr == std::numeric_limits<int32_t>::min())
			altlwr = m_lwralt;
		if (altupr == std::numeric_limits<int32_t>::min())
			altupr = m_upralt;
		bool val(alt >= altlwr && alt <= altupr);
		val = val && m_bbox.is_inside(coord);
		if (val && true) {
			pointcache_t::const_iterator i(m_pointcache.find(PointCache(coord)));
			if (i == m_pointcache.end()) {
				val = !!m_poly.windingnumber(coord);
				m_pointcache.insert(PointCache(coord, val));
			} else {
				val = i->is_inside();
			}
		} else {
			val = val && m_poly.windingnumber(coord);
		}
		if (false && !get_ident().compare(0, 4, "LSAZ")) {
			std::cerr << "is_inside: " << get_ident() << ": " << coord
				  << " F" << std::setw(3) << std::setfill('0') << (alt / 100)
				  << " F" << std::setw(3) << std::setfill('0') << (altlwr / 100)
				  << "...F" << std::setw(3) << std::setfill('0') << (altupr / 100)
				  << " (F" << std::setw(3) << std::setfill('0') << (m_lwralt / 100)
				  << "...F" << std::setw(3) << std::setfill('0') << (m_upralt / 100)
				  << "): result " << (val ? 'T' : 'F') << std::endl;
		}
		return val;
	}
	bool val(false);
	for (comps_t::const_iterator ci(m_comps.begin()), ce(m_comps.end()); ci != ce; ++ci) {
		switch (ci->second) {
		default:
		case AirspacesDb::Airspace::Component::operator_set:
			val = ci->first->is_inside(coord, alt, altlwr, altupr);
			break;

		case AirspacesDb::Airspace::Component::operator_union:
			val = val || ci->first->is_inside(coord, alt, altlwr, altupr);
			break;

		case AirspacesDb::Airspace::Component::operator_subtract:
			val = val && !ci->first->is_inside(coord, alt, altlwr, altupr);
			break;

		case AirspacesDb::Airspace::Component::operator_intersect:
			val = val && ci->first->is_inside(coord, alt, altlwr, altupr);
			break;
		}
	}
	if (false && !get_ident().compare(0, 4, "LSAZ")) {
		std::cerr << "is_inside(C): " << get_ident() << ": " << coord
			  << " F" << std::setw(3) << std::setfill('0') << (alt / 100)
			  << " F" << std::setw(3) << std::setfill('0') << (altlwr / 100)
			  << "...F" << std::setw(3) << std::setfill('0') << (altupr / 100)
			  << " (F" << std::setw(3) << std::setfill('0') << (m_lwralt / 100)
			  << "...F" << std::setw(3) << std::setfill('0') << (m_upralt / 100)
			  << "): result " << (val ? 'T' : 'F') << std::endl;;
	}
	return val;
}

bool TrafficFlowRestrictions::TFRAirspace::is_intersection(const Point& la, const Point& lb, int32_t alt, int32_t altlwr, int32_t altupr) const
{
	if (m_comps.empty()) {
		if (altlwr == std::numeric_limits<int32_t>::min())
			altlwr = m_lwralt;
		if (altupr == std::numeric_limits<int32_t>::min())
			altupr = m_upralt;
		bool val(alt >= altlwr && alt <= altupr);
		if (val && la.get_lat() > m_bbox.get_north() && lb.get_lat() > m_bbox.get_north())
			val = false;
		if (val && la.get_lat() < m_bbox.get_south() && lb.get_lat() < m_bbox.get_south())
			val = false;
		if (val) {
			Point::coord_t lalon(la.get_lon() - m_bbox.get_west());
			Point::coord_t lblon(lb.get_lon() - m_bbox.get_west());
			Point::coord_t w(m_bbox.get_east() - m_bbox.get_west());
			if (lalon < 0 && lblon < 0)
				val = false;
			if (lalon > w && lblon > w)
				val = false;
		}
		if (val && true) {
			linecache_t::const_iterator i(m_linecache.find(LineCache(la, lb)));
			if (i == m_linecache.end()) {
				val = !!m_poly.is_intersection(la, lb);
				m_linecache.insert(LineCache(la, lb, val));
			} else {
				val = i->is_intersect();
			}
		} else {
			val = val && m_poly.is_intersection(la, lb);
		}
		if (false && !get_ident().compare(0, 4, "LSAZ")) {
			std::cerr << "is_intersection: " << get_ident() << ": " << la << " -> " << lb
				  << " F" << std::setw(3) << std::setfill('0') << (alt / 100)
				  << " F" << std::setw(3) << std::setfill('0') << (altlwr / 100)
				  << "...F" << std::setw(3) << std::setfill('0') << (altupr / 100)
				  << " (F" << std::setw(3) << std::setfill('0') << (m_lwralt / 100)
				  << "...F" << std::setw(3) << std::setfill('0') << (m_upralt / 100)
				  << "): result " << (val ? 'T' : 'F') << std::endl;
		}	
		return val;
	}
	bool val(false);
	for (comps_t::const_iterator ci(m_comps.begin()), ce(m_comps.end()); ci != ce; ++ci) {
		switch (ci->second) {
		default:
		case AirspacesDb::Airspace::Component::operator_set:
			val = ci->first->is_intersection(la, lb, alt, altlwr, altupr);
			break;

		case AirspacesDb::Airspace::Component::operator_union:
			val = val || ci->first->is_intersection(la, lb, alt, altlwr, altupr);
			break;

		case AirspacesDb::Airspace::Component::operator_subtract:
			val = val && !ci->first->is_intersection(la, lb, alt, altlwr, altupr);
			break;

		case AirspacesDb::Airspace::Component::operator_intersect:
			val = val && ci->first->is_intersection(la, lb, alt, altlwr, altupr);
			break;
		}
	}
	if (false && !get_ident().compare(0, 4, "LSAZ")) {
		std::cerr << "is_intersection(C): " << get_ident() << ": " << la << " -> " << lb
			  << " F" << std::setw(3) << std::setfill('0') << (alt / 100)
			  << " F" << std::setw(3) << std::setfill('0') << (altlwr / 100)
			  << "...F" << std::setw(3) << std::setfill('0') << (altupr / 100)
			  << " (F" << std::setw(3) << std::setfill('0') << (m_lwralt / 100)
			  << "...F" << std::setw(3) << std::setfill('0') << (m_upralt / 100)
			  << "): result " << (val ? 'T' : 'F') << std::endl;
	}
	return val;
}

IntervalSet<int32_t> TrafficFlowRestrictions::TFRAirspace::get_altrange(const Point& coord, int32_t altlwr, int32_t altupr) const
{
	IntervalSet<int32_t> r;
	r.set_empty();
	if (m_comps.empty()) {
		if (!m_bbox.is_inside(coord) || !m_poly.windingnumber(coord))
			return r;
		if (altlwr == std::numeric_limits<int32_t>::min())
			altlwr = m_lwralt;
		if (altupr == std::numeric_limits<int32_t>::min())
			altupr = m_upralt;
		r = IntervalSet<int32_t>(IntervalSet<int32_t>::Intvl(altlwr, altupr+1));
		return r;
	}
	for (comps_t::const_iterator ci(m_comps.begin()), ce(m_comps.end()); ci != ce; ++ci) {
		IntervalSet<int32_t> r1(ci->first->get_altrange(coord, altlwr, altupr));
		switch (ci->second) {
		default:
		case AirspacesDb::Airspace::Component::operator_set:
			r = r1;
			break;

		case AirspacesDb::Airspace::Component::operator_union:
			r |= r1;
			break;

		case AirspacesDb::Airspace::Component::operator_subtract:
			r &= ~r1;
			break;

		case AirspacesDb::Airspace::Component::operator_intersect:
			r &= r1;
			break;
		}
	}
	return r;
}

IntervalSet<int32_t> TrafficFlowRestrictions::TFRAirspace::get_altrange(const Point& coord0, const Point& coord1, int32_t altlwr, int32_t altupr) const
{
	IntervalSet<int32_t> r;
	r.set_empty();
	if (m_comps.empty()) {
		if (!((m_bbox.is_inside(coord0) && m_poly.windingnumber(coord0)) ||
		      (m_bbox.is_inside(coord1) && m_poly.windingnumber(coord1)) ||
		      is_intersection(coord0, coord1)))
			return r;
		if (altlwr == std::numeric_limits<int32_t>::min())
			altlwr = m_lwralt;
		if (altupr == std::numeric_limits<int32_t>::min())
			altupr = m_upralt;
		r = IntervalSet<int32_t>(IntervalSet<int32_t>::Intvl(altlwr, altupr+1));
		return r;
	}
	for (comps_t::const_iterator ci(m_comps.begin()), ce(m_comps.end()); ci != ce; ++ci) {
		IntervalSet<int32_t> r1(ci->first->get_altrange(coord0, coord1, altlwr, altupr));
		switch (ci->second) {
		default:
		case AirspacesDb::Airspace::Component::operator_set:
			r = r1;
			break;

		case AirspacesDb::Airspace::Component::operator_union:
			r |= r1;
			break;

		case AirspacesDb::Airspace::Component::operator_subtract:
			r &= ~r1;
			break;

		case AirspacesDb::Airspace::Component::operator_intersect:
			r &= r1;
			break;
		}
	}
	return r;
}

void TrafficFlowRestrictions::TFRAirspace::get_boundingalt(int32_t& lwralt, int32_t& upralt) const
{
	if (m_comps.empty()) {
		lwralt = std::min(lwralt, m_lwralt);
		upralt = std::max(upralt, m_upralt);
		return;
	}
	for (comps_t::const_iterator ci(m_comps.begin()), ce(m_comps.end()); ci != ce; ++ci)
		ci->first->get_boundingalt(lwralt, upralt);
}

TrafficFlowRestrictions::RestrictionElement::RestrictionElement(const AltRange& alt)
	: m_refcount(1), m_alt(alt)
{
	if (!m_alt.is_lower_valid()) {
		m_alt.set_lower_mode(AltRange::alt_std);
		m_alt.set_lower_alt(0);
	}
	if (!m_alt.is_upper_valid()) {
		m_alt.set_upper_mode(AltRange::alt_std);
		m_alt.set_upper_alt(std::numeric_limits<int32_t>::max());
	}
}

TrafficFlowRestrictions::RestrictionElement::~RestrictionElement()
{
}

void TrafficFlowRestrictions::RestrictionElement::reference(void) const
{
        g_atomic_int_inc(&m_refcount);
}

void TrafficFlowRestrictions::RestrictionElement::unreference(void) const
{
        if (!g_atomic_int_dec_and_test(&m_refcount))
                return;
        delete this;
}

bool TrafficFlowRestrictions::RestrictionElement::is_altrange(int32_t minalt, int32_t maxalt) const
{
	if (minalt > maxalt)
		return false;
	if (m_alt.is_lower_valid() && maxalt < m_alt.get_lower_alt())
		return false;
	if (m_alt.is_upper_valid() && minalt > m_alt.get_upper_alt())
		return false;
	return true;
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::RestrictionElement::evaluate_trace(TrafficFlowRestrictions& tfrs, const TrafficFlowRule *rule,
												unsigned int altcnt, unsigned int seqcnt) const
{
	CondResult r(evaluate(tfrs));
	std::ostringstream oss;
	oss << "Restriction " << altcnt << '.' << seqcnt << " result " << (r.get_result() ? 'T' : (!r.get_result() ? 'F' : 'I'));
	Message msg(oss.str(), Message::type_tracetfe, rule->get_codeid());
	msg.m_vertexset = r.get_vertexset();
	msg.m_edgeset = r.get_edgeset();
	tfrs.message(msg);
	return r;
}

TrafficFlowRestrictions::DctParameters::AltRange TrafficFlowRestrictions::RestrictionElement::evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct) const
{
	DctParameters::AltRange x;
	x.set_empty();
	return x;
}

TrafficFlowRestrictions::DctParameters::AltRange TrafficFlowRestrictions::RestrictionElement::evaluate_dct_trace(TrafficFlowRestrictions& tfrs, const DctParameters& dct,
														      const TrafficFlowRule *rule, unsigned int altcnt, unsigned int seqcnt) const
{
	DctParameters::AltRange r(evaluate_dct(tfrs, dct));
	std::ostringstream oss;
	oss << "Restriction " << altcnt << '.' << seqcnt << " result " << r[0].to_str() << " / " << r[1].to_str();
	Message msg(oss.str(), Message::type_tracetfe, rule->get_codeid());
	tfrs.message(msg);
	return r;
}

void TrafficFlowRestrictions::RestrictionElement::get_dct_segments(DctSegments& seg) const
{
}

std::string TrafficFlowRestrictions::RestrictionElement::to_str(void) const
{
	return m_alt.to_str();
}

TrafficFlowRestrictions::RestrictionElementRoute::RestrictionElementRoute(const AltRange& alt, const RuleWpt& pt0, const RuleWpt& pt1,
									  const std::string& rteid, match_t m)
	: RestrictionElement(alt), m_rteid(rteid), m_match(m)
{
	m_point[0] = pt0;
	m_point[1] = pt1;
}

TrafficFlowRestrictions::RestrictionElementRoute::ptr_t TrafficFlowRestrictions::RestrictionElementRoute::clone(void) const
{
	return ptr_t(new RestrictionElementRoute(m_alt, m_point[0], m_point[1], m_rteid, m_match));
}

bool TrafficFlowRestrictions::RestrictionElementRoute::is_bbox(const Rect& bbox) const
{
	return bbox.is_inside(m_point[0].get_coord()) || bbox.is_inside(m_point[1].get_coord()) ||
		bbox.is_intersect(m_point[0].get_coord(), m_point[1].get_coord());
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::RestrictionElementRoute::evaluate(TrafficFlowRestrictions& tfrs) const
{
	Graph::vertex_descriptor u(tfrs.get_vertexdesc(m_point[0]));
	Graph::vertex_descriptor v(tfrs.get_vertexdesc(m_point[1]));
	if (u == TrafficFlowRestrictions::invalid_vertex_descriptor ||
	    v == TrafficFlowRestrictions::invalid_vertex_descriptor)
		return CondResult(false);
	const Fpl& fpl(tfrs.get_route());
	switch (m_match) {
	case match_awy:
	{
		CondResult r(false);
		for (unsigned int wptnr = 1U; wptnr < fpl.size(); ++wptnr) {
			const FplWpt& wpt(fpl[wptnr - 1U]);
			if (wpt.get_vertex_descriptor() != u || !wpt.within(m_alt))
				continue;
			CondResult r1(true);
			for (unsigned int wptnre(wptnr); wptnre < fpl.size(); ++wptnre) {
				const FplWpt& wpt(fpl[wptnre - 1U]);
				const FplWpt& wpte(fpl[wptnre]);
				if (!wpt.is_ifr() ||
				    wpt.get_pathcode() != FPlanWaypoint::pathcode_airway ||
				    wpt.get_pathname() != m_rteid) {
					r1.set_result(false);
					break;
				}
				r1.get_edgeset().insert(wptnre - 1U);
				if (wpte.get_vertex_descriptor() == v)
					break;
			}
			if (r1.get_result()) {
				r.set_result(true);
				r.get_edgeset().insert(r1.get_edgeset().begin(), r1.get_edgeset().end());
				r.get_reflocset().insert(r1.get_reflocset().begin(), r1.get_reflocset().end());
			}
		}
		return r;
	}

	case match_dct:
	{
		CondResult r(false);
		for (unsigned int wptnr = 1U; wptnr < fpl.size(); ++wptnr) {
			const FplWpt& wpt0(fpl[wptnr - 1U]);
			const FplWpt& wpt1(fpl[wptnr]);
			if (!wpt0.is_ifr() || !wpt0.within(m_alt) ||
			    (wpt0.get_pathcode() != FPlanWaypoint::pathcode_directto &&
			     (wpt0.get_pathcode() != FPlanWaypoint::pathcode_none ||
			      (wptnr > 1U && wptnr + 1U < fpl.size()))))
				continue;
			if (wpt0.get_vertex_descriptor() == u &&
			    wpt1.get_vertex_descriptor() == v) {
				r.set_result(true);
				r.get_edgeset().insert(wptnr - 1U);
			}
		}
		return r;
	}

	case match_all:
	{
		CondResult r(false);
		for (unsigned int wptnr = 1U; wptnr < fpl.size(); ++wptnr) {
			const FplWpt& wpt0(fpl[wptnr - 1U]);
			const FplWpt& wpt1(fpl[wptnr]);
			if (!wpt0.is_ifr() || !wpt0.within(m_alt))
				continue;
			if (wpt0.get_vertex_descriptor() == u &&
			    wpt1.get_vertex_descriptor() == v) {
				r.set_result(true);
				r.get_edgeset().insert(wptnr - 1U);
			}
		}
		return r;
	}

	default:
		return CondResult(false);
	}
}

TrafficFlowRestrictions::RuleResult::Alternative::Sequence TrafficFlowRestrictions::RestrictionElementRoute::get_rule(void) const
{
	int32_t altlwr(m_alt.get_lower_alt()), altupr(m_alt.get_upper_alt());
	if (!m_alt.is_lower_valid())
		altlwr = 0;
	if (!m_alt.is_upper_valid())
		altupr = std::numeric_limits<int32_t>::max();
	return RuleResult::Alternative::Sequence(m_match == match_awy ? RuleResult::Alternative::Sequence::type_airway
						 : RuleResult::Alternative::Sequence::type_dct,
						 RuleResult::Alternative::Sequence::RulePoint(m_point[0]),
						 RuleResult::Alternative::Sequence::RulePoint(m_point[1]),
						 altlwr, altupr, m_rteid, "", 0, m_alt.is_lower_valid(), m_alt.is_upper_valid());
}

TrafficFlowRestrictions::DctParameters::AltRange TrafficFlowRestrictions::RestrictionElementRoute::evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct) const
{
	DctParameters::AltRange x;
	x.set(m_alt);
	if (m_match != match_dct) {
		x.set_empty();
		return x;
	}
	for (unsigned int i = 0; i < 2; ++i) {
		if (m_point[i].get_ident() != dct.get_ident(0) ||
		    m_point[!i].get_ident() != dct.get_ident(1) ||
		    m_point[i].get_coord().spheric_distance_nmi_dbl(dct.get_coord(0)) > 0.1 ||
		    m_point[!i].get_coord().spheric_distance_nmi_dbl(dct.get_coord(1)) > 0.1)
			x[i].set_empty();
	}
	return x;
}

void TrafficFlowRestrictions::RestrictionElementRoute::get_dct_segments(DctSegments& seg) const
{
	if (m_match != match_dct)
		return;
	seg.add(DctSegments::DctSegment(m_point[0], m_point[1]));
}

std::string TrafficFlowRestrictions::RestrictionElementRoute::to_str(void) const
{
	std::ostringstream oss;
	switch (m_match) {
	case match_dct:
		oss << "DCT ";
		break;

	case match_awy:
		oss << m_rteid << ' ';
		break;

	default:
		break;
	}
	oss << m_point[0].get_ident() << ' ' << m_point[1].get_ident() << ' ' << RestrictionElement::to_str();
	return oss.str();
}

TrafficFlowRestrictions::RestrictionElementPoint::RestrictionElementPoint(const AltRange& alt, const RuleWpt& pt)
	: RestrictionElement(alt), m_point(pt)
{
}

TrafficFlowRestrictions::RestrictionElementPoint::ptr_t TrafficFlowRestrictions::RestrictionElementPoint::clone(void) const
{
	return ptr_t(new RestrictionElementPoint(m_alt, m_point));
}

bool TrafficFlowRestrictions::RestrictionElementPoint::is_bbox(const Rect& bbox) const
{
	return bbox.is_inside(m_point.get_coord());
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::RestrictionElementPoint::evaluate(TrafficFlowRestrictions& tfrs) const
{
	Graph::vertex_descriptor u(tfrs.get_vertexdesc(m_point));
	if (u == TrafficFlowRestrictions::invalid_vertex_descriptor)
		return CondResult(false);
	const Fpl& fpl(tfrs.get_route());
	CondResult r(false);
	for (unsigned int wptnr = 0U; wptnr < fpl.size(); ++wptnr) {
		const FplWpt& wpt(fpl[wptnr]);
		if (!wpt.is_ifr() && (!wptnr || !fpl[wptnr - 1U].is_ifr()))
			continue;
		if (wpt.get_vertex_descriptor() != u || !wpt.within(m_alt))
			continue;
		r.set_result(true);
		r.get_vertexset().insert(wptnr);
	}
	return r;
}

TrafficFlowRestrictions::RuleResult::Alternative::Sequence TrafficFlowRestrictions::RestrictionElementPoint::get_rule(void) const
{
	int32_t altlwr(m_alt.get_lower_alt()), altupr(m_alt.get_upper_alt());
	if (!m_alt.is_lower_valid())
		altlwr = 0;
	if (!m_alt.is_upper_valid())
		altupr = std::numeric_limits<int32_t>::max();
	return RuleResult::Alternative::Sequence(RuleResult::Alternative::Sequence::type_point,
						 RuleResult::Alternative::Sequence::RulePoint(m_point),
						 RuleResult::Alternative::Sequence::RulePoint(),
						 altlwr, altupr, "", "", 0, m_alt.is_lower_valid(), m_alt.is_upper_valid());
}

TrafficFlowRestrictions::DctParameters::AltRange TrafficFlowRestrictions::RestrictionElementPoint::evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct) const
{
	DctParameters::AltRange x;
	x.set_empty();
	for (unsigned int i = 0; i < 2; ++i) {
		if (m_point.get_ident() == dct.get_ident(i) &&
		    m_point.get_coord().spheric_distance_nmi_dbl(dct.get_coord(i)) <= 0.1)
			x.set(m_alt);
	}
	return x;
}

std::string TrafficFlowRestrictions::RestrictionElementPoint::to_str(void) const
{
	std::ostringstream oss;
	oss << m_point.get_ident() << ' ' << RestrictionElement::to_str();
	return oss.str();
}

TrafficFlowRestrictions::RestrictionElementSidStar::RestrictionElementSidStar(const AltRange& alt, const std::string& procid, const RuleWpt& arpt,
									      bool star)
	: RestrictionElement(alt), m_procid(procid), m_arpt(arpt), m_star(star)
{
}

TrafficFlowRestrictions::RestrictionElementSidStar::ptr_t TrafficFlowRestrictions::RestrictionElementSidStar::clone(void) const
{
	return ptr_t(new RestrictionElementSidStar(m_alt, m_procid, m_arpt, m_star));
}

bool TrafficFlowRestrictions::RestrictionElementSidStar::is_bbox(const Rect& bbox) const
{
	return bbox.is_inside(m_arpt.get_coord());	
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::RestrictionElementSidStar::evaluate(TrafficFlowRestrictions& tfrs) const
{
	const Fpl& fpl(tfrs.get_route());
	if (fpl.size() < 2)
		return CondResult(false);
	const FplWpt& wpt(m_star ? fpl.back() : fpl.front());
	if (m_arpt.get_ident() != wpt.get_icao())
		return CondResult(false);
	CondResult r(true);
	if (m_star) {
		const FplWpt& wpt1(fpl[fpl.size() - 2U]);
		bool ok(wpt1.get_pathcode() == FPlanWaypoint::pathcode_star && wpt1.get_pathname() == m_procid);
		if (!ok && ((wpt1.get_pathcode() == FPlanWaypoint::pathcode_star && wpt1.get_pathname().empty()) ||
			    wpt1.get_pathcode() == FPlanWaypoint::pathcode_none || wpt1.get_pathcode() == FPlanWaypoint::pathcode_directto)) {
			std::string pterm(m_procid);
			std::string::size_type n(pterm.find_first_of("0123456789"));
			if (n != std::string::npos)
				pterm.erase(n);
			if (pterm.size() >= 2 && (pterm == wpt1.get_icao() || (wpt1.get_icao().empty() && pterm == wpt1.get_name())))
				ok = true;
		}
		if (!ok)
			return CondResult(false);
		if (!wpt1.within(m_alt))
			return CondResult(false);
		r.get_vertexset().insert(fpl.size() - 1U);
		r.get_vertexset().insert(fpl.size() - 2U);
	} else {
		bool ok(wpt.get_pathcode() == FPlanWaypoint::pathcode_sid && wpt.get_pathname() == m_procid);
                if (!ok && ((wpt.get_pathcode() == FPlanWaypoint::pathcode_sid && wpt.get_pathname().empty()) ||
			    wpt.get_pathcode() == FPlanWaypoint::pathcode_none || wpt.get_pathcode() == FPlanWaypoint::pathcode_directto)) {
			const FplWpt& wpt1(fpl[1U]);
			std::string pterm(m_procid);
			std::string::size_type n(pterm.find_first_of("0123456789"));
			if (n != std::string::npos)
				pterm.erase(n);
			if (pterm.size() >= 2 && (pterm == wpt1.get_icao() || (wpt1.get_icao().empty() && pterm == wpt1.get_name())))
				ok = true;
		}
 		if (!ok)
			return CondResult(false);
		const FplWpt& wpt1(fpl[1U]);
		if (!wpt1.within(m_alt))
			return CondResult(false);
		r.get_vertexset().insert(0U);
	}
	return r;
}

TrafficFlowRestrictions::RuleResult::Alternative::Sequence TrafficFlowRestrictions::RestrictionElementSidStar::get_rule(void) const
{
	int32_t altlwr(m_alt.get_lower_alt()), altupr(m_alt.get_upper_alt());
	if (!m_alt.is_lower_valid())
		altlwr = 0;
	if (!m_alt.is_upper_valid())
		altupr = std::numeric_limits<int32_t>::max();
	return RuleResult::Alternative::Sequence(m_star ? RuleResult::Alternative::Sequence::type_star
						 : RuleResult::Alternative::Sequence::type_sid,
						 RuleResult::Alternative::Sequence::RulePoint(),
						 RuleResult::Alternative::Sequence::RulePoint(),
						 altlwr, altupr, m_procid, m_arpt.get_ident(), 0, m_alt.is_lower_valid(), m_alt.is_upper_valid());
}

std::string TrafficFlowRestrictions::RestrictionElementSidStar::to_str(void) const
{
	std::ostringstream oss;
	oss << (m_star ? "STAR " : "SID ") << m_procid << ' ' << m_arpt.get_ident() << ' ' << RestrictionElement::to_str();
	return oss.str();
}

TrafficFlowRestrictions::RestrictionElementAirspace::RestrictionElementAirspace(const AltRange& alt, TFRAirspace::const_ptr_t aspc)
	: RestrictionElement(alt), m_airspace(aspc)
{
	if (!alt.is_lower_valid() && m_airspace) {
		m_alt.set_lower_mode(AltRange::alt_std);
		m_alt.set_lower_alt(m_airspace->get_lower_alt());
	}
	if (!alt.is_upper_valid()) {
		m_alt.set_upper_mode(AltRange::alt_std);
		m_alt.set_upper_alt(m_airspace->get_upper_alt());
	}
}

TrafficFlowRestrictions::RestrictionElementAirspace::ptr_t TrafficFlowRestrictions::RestrictionElementAirspace::clone(void) const
{
	return ptr_t(new RestrictionElementAirspace(m_alt, m_airspace));
}

bool TrafficFlowRestrictions::RestrictionElementAirspace::is_bbox(const Rect& bbox) const
{
	if (!m_airspace)
		return false;
	return bbox.is_intersect(m_airspace->get_bbox());
}

bool TrafficFlowRestrictions::RestrictionElementAirspace::is_altrange(int32_t minalt, int32_t maxalt) const
{
	if (minalt > maxalt)
		return false;
	int32_t lwralt(m_alt.get_lower_alt());
	int32_t upralt(m_alt.get_upper_alt());
	if (!m_alt.is_valid()) {
		if (!m_airspace)
			return false;
		int32_t lwralt1(std::numeric_limits<int32_t>::max());
		int32_t upralt1(std::numeric_limits<int32_t>::min());
		m_airspace->get_boundingalt(lwralt1, upralt1);
		if (!m_alt.is_lower_valid())
			lwralt = lwralt1;
		if (!m_alt.is_upper_valid())
			upralt = upralt1;
	}
	return !(maxalt < lwralt || minalt > upralt);
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::RestrictionElementAirspace::evaluate(TrafficFlowRestrictions& tfrs) const
{
	if (!m_airspace)
		return CondResult(false);
	const Fpl& fpl(tfrs.get_route());
	CondResult r(false);
	for (unsigned int wptnr = 0U; wptnr < fpl.size(); ++wptnr) {
		const FplWpt& wpt(fpl[wptnr]);
		if (!wpt.is_ifr() && (!wptnr || !fpl[wptnr - 1U].is_ifr()))
			continue;
		const Point& pt(wpt.get_coord());
		if (m_airspace->is_inside(pt, wpt.get_altitude(), m_alt.get_lower_alt_if_valid(), m_alt.get_upper_alt_if_valid())) {
			r.set_result(true);
			r.get_vertexset().insert(wptnr);
			continue;
		}
		if (!wptnr)
			continue;
		const FplWpt& wpt0(fpl[wptnr - 1U]);
		const Point& pt0(wpt0.get_coord());
		if (m_airspace->is_intersection(pt0, pt, wpt0.get_altitude(), m_alt.get_lower_alt_if_valid(), m_alt.get_upper_alt_if_valid())) {
			r.set_result(true);
			r.get_edgeset().insert(wptnr - 1U);
			continue;
		}
	}
	return r;
}

TrafficFlowRestrictions::RuleResult::Alternative::Sequence TrafficFlowRestrictions::RestrictionElementAirspace::get_rule(void) const
{
	int32_t altlwr(m_alt.get_lower_alt()), altupr(m_alt.get_upper_alt());
	if (!m_airspace) {
		if (!m_alt.is_lower_valid())
			altlwr = 0;
		if (!m_alt.is_upper_valid())
			altupr = std::numeric_limits<int32_t>::max();
		return RuleResult::Alternative::Sequence(RuleResult::Alternative::Sequence::type_airspace,
							 RuleResult::Alternative::Sequence::RulePoint(),
							 RuleResult::Alternative::Sequence::RulePoint(),
							 altlwr, altupr, "", "", 0, m_alt.is_lower_valid(), m_alt.is_upper_valid());
	}
	if (!m_alt.is_valid()) {
		int32_t altlwr1(std::numeric_limits<int32_t>::max());
		int32_t altupr1(std::numeric_limits<int32_t>::min());
		m_airspace->get_boundingalt(altlwr1, altupr1);
		if (!m_alt.is_lower_valid())
			altlwr = altlwr1;
		if (!m_alt.is_upper_valid())
			altupr = altupr1;
	}
	return RuleResult::Alternative::Sequence(RuleResult::Alternative::Sequence::type_airspace,
						 RuleResult::Alternative::Sequence::RulePoint(),
						 RuleResult::Alternative::Sequence::RulePoint(),
						 altlwr, altupr, m_airspace->get_ident(), "",
						 m_airspace->get_bdryclass(), m_alt.is_lower_valid(), m_alt.is_upper_valid());
}

std::string TrafficFlowRestrictions::RestrictionElementAirspace::to_str(void) const
{
	std::ostringstream oss;
	if (m_airspace)
		oss << m_airspace->get_ident() << '/' << m_airspace->get_type();
	else
		oss << "?ASPC";
	oss << ' ' << RestrictionElement::to_str();
	return oss.str();
}

TrafficFlowRestrictions::RestrictionSequence::RestrictionSequence(void)
{
}

void TrafficFlowRestrictions::RestrictionSequence::add(RestrictionElement::ptr_t e)
{
	m_elements.push_back(e);
}

void TrafficFlowRestrictions::RestrictionSequence::clone(void)
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

bool TrafficFlowRestrictions::RestrictionSequence::is_bbox(const Rect& bbox) const
{
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
		if ((*i)->is_bbox(bbox))
			return true;
	return false;
}

bool TrafficFlowRestrictions::RestrictionSequence::is_altrange(int32_t minalt, int32_t maxalt) const
{
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
		if ((*i)->is_altrange(minalt, maxalt))
			return true;
	return false;
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::RestrictionSequence::evaluate(TrafficFlowRestrictions& tfrs) const
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

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::RestrictionSequence::evaluate_trace(TrafficFlowRestrictions& tfrs, const TrafficFlowRule *rule, unsigned int altcnt) const
{
	CondResult r(true);
	bool first(true);
	unsigned int seqcnt(0);
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i, ++seqcnt) {
		if (first) {
			r = (*i)->evaluate_trace(tfrs, rule, altcnt, seqcnt);
			first = false;
		} else {
			CondResult r1((*i)->evaluate_trace(tfrs, rule, altcnt, seqcnt));
			if (r.get_last() > r1.get_first())
				r = CondResult(false);
			r &= r1;
		}
	}
	std::ostringstream oss;
	oss << "Restriction " << altcnt << " result " << (r.get_result() ? 'T' : (!r.get_result() ? 'F' : 'I'));
	Message msg(oss.str(), Message::type_tracetfe, rule->get_codeid());
	msg.m_vertexset = r.get_vertexset();
	msg.m_edgeset = r.get_edgeset();
	tfrs.message(msg);
	return r;
}

TrafficFlowRestrictions::RuleResult::Alternative TrafficFlowRestrictions::RestrictionSequence::get_rule(void) const
{
	RuleResult::Alternative alt;
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		RuleResult::Alternative::Sequence seq1((*i)->get_rule());
		if (seq1.get_type() != RuleResult::Alternative::Sequence::type_invalid)
		        alt.add_sequence(seq1);
	}
	return alt;
}

bool TrafficFlowRestrictions::RestrictionSequence::is_valid_dct(void) const
{
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
		if (!(*i)->is_valid_dct())
			return false;
	return true;
}

TrafficFlowRestrictions::DctParameters::AltRange TrafficFlowRestrictions::RestrictionSequence::evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct) const
{
	DctParameters::AltRange x;
	x.set_full();
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		x &= (*i)->evaluate_dct(tfrs, dct);
		if (x.is_empty())
			break;
	}
	return x;
}

TrafficFlowRestrictions::DctParameters::AltRange TrafficFlowRestrictions::RestrictionSequence::evaluate_dct_trace(TrafficFlowRestrictions& tfrs, const DctParameters& dct,
														  const TrafficFlowRule *rule, unsigned int altcnt) const
{
	DctParameters::AltRange x;
	x.set_full();
	unsigned int seqcnt(0);
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i, ++seqcnt) {
		x &= (*i)->evaluate_dct_trace(tfrs, dct, rule, altcnt, seqcnt);
		if (x.is_empty())
			break;
	}
	std::ostringstream oss;
	oss << "Restriction " << altcnt << " result " << x[0].to_str() << " / " << x[1].to_str();
	Message msg(oss.str(), Message::type_tracetfe, rule->get_codeid());
	tfrs.message(msg);
	return x;	
}

void TrafficFlowRestrictions::RestrictionSequence::get_dct_segments(DctSegments& seg) const
{
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)	
		(*i)->get_dct_segments(seg);
}

std::string TrafficFlowRestrictions::RestrictionSequence::to_str(void) const
{
	std::ostringstream oss;
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
		if (*i)
			oss << ' ' << (*i)->to_str();
	return oss.str().substr(1);
}

TrafficFlowRestrictions::Restrictions::Restrictions(void)
{
}

void TrafficFlowRestrictions::Restrictions::clone(void)
{
	for (elements_t::iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
		i->clone();
}

bool TrafficFlowRestrictions::Restrictions::simplify_bbox(const Rect& bbox)
{
	bool modified(false);
	for (elements_t::iterator i(m_elements.begin()), e(m_elements.end()); i != e; ) {
		if (i->is_bbox(bbox)) {
			++i;
			continue;
		}
		i = m_elements.erase(i);
		e = m_elements.end();
		modified = true;
	}
	return modified;
}

bool TrafficFlowRestrictions::Restrictions::simplify_altrange(int32_t minalt, int32_t maxalt)
{
	bool modified(false);
	for (elements_t::iterator i(m_elements.begin()), e(m_elements.end()); i != e; ) {
		if (i->is_altrange(minalt, maxalt)) {
			++i;
			continue;
		}
		i = m_elements.erase(i);
		e = m_elements.end();
		modified = true;
	}
	return modified;
}

bool TrafficFlowRestrictions::Restrictions::evaluate(TrafficFlowRestrictions& tfrs, RuleResult& result, bool forbidden) const
{
	bool ruleok(forbidden);
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		RuleResult::Alternative alt(i->get_rule());
		CondResult r(i->evaluate(tfrs));
		alt.m_vertexset = r.get_vertexset();
		alt.m_edgeset = r.get_edgeset();
		result.add_element(alt);
		if (forbidden)
			ruleok = ruleok && !r.get_result();
		else
			ruleok = ruleok || r.get_result();
	}
	return ruleok;
}

bool TrafficFlowRestrictions::Restrictions::evaluate_trace(TrafficFlowRestrictions& tfrs, RuleResult& result, bool forbidden, const TrafficFlowRule *rule) const
{
	bool ruleok(forbidden);
	unsigned int altcnt(0);
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i, ++altcnt) {
		RuleResult::Alternative alt(i->get_rule());
		CondResult r(i->evaluate_trace(tfrs, rule, altcnt));
		alt.m_vertexset = r.get_vertexset();
		alt.m_edgeset = r.get_edgeset();
		result.add_element(alt);
		if (forbidden)
			ruleok = ruleok && !r.get_result();
		else
			ruleok = ruleok || r.get_result();
	}
	return ruleok;
}

bool TrafficFlowRestrictions::Restrictions::is_valid_dct(void) const
{
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
		if (!i->is_valid_dct())
			return false;
	return true;
}

TrafficFlowRestrictions::DctParameters::AltRange TrafficFlowRestrictions::Restrictions::evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct, bool forbidden) const
{
	DctParameters::AltRange x;
	x.set_empty();
	if (forbidden)
		x.set_full();
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		DctParameters::AltRange x1(i->evaluate_dct(tfrs, dct));
		if (forbidden)
			x &= ~x1;
		else
			x |= x1;
	}
	return x;
}

TrafficFlowRestrictions::DctParameters::AltRange TrafficFlowRestrictions::Restrictions::evaluate_dct_trace(TrafficFlowRestrictions& tfrs, const DctParameters& dct, bool forbidden, const TrafficFlowRule *rule) const
{
	DctParameters::AltRange x;
	x.set_empty();
	if (forbidden)
		x.set_full();
	unsigned int altcnt(0);
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i, ++altcnt) {
		DctParameters::AltRange x1(i->evaluate_dct_trace(tfrs, dct, rule, altcnt));
		if (forbidden)
			x &= ~x1;
		else
			x |= x1;
	}
	std::ostringstream oss;
	oss << "Restriction result " << x[0].to_str() << " / " << x[1].to_str();
	Message msg(oss.str(), Message::type_tracetfe, rule->get_codeid());
	tfrs.message(msg);
	return x;
}

void TrafficFlowRestrictions::Restrictions::get_dct_segments(DctSegments& seg) const
{
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)	
		i->get_dct_segments(seg);
}

void TrafficFlowRestrictions::Restrictions::set_ruleresult(RuleResult& result) const
{
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		RuleResult::Alternative alt(i->get_rule());
		result.add_element(alt);
	}
}

std::string TrafficFlowRestrictions::Restrictions::to_str(void) const
{
	std::ostringstream oss;
	bool subseq(false);
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		if (subseq)
			oss << " / ";
		oss << i->to_str();
		subseq = true;
	}
	return oss.str();
}

const bool TrafficFlowRestrictions::Condition::ifr_refloc;

TrafficFlowRestrictions::Condition::Condition(unsigned int childnum)
	: m_refcount(1), m_childnum(childnum)
{
}

TrafficFlowRestrictions::Condition::~Condition()
{
}

void TrafficFlowRestrictions::Condition::reference(void) const
{
        g_atomic_int_inc(&m_refcount);
}

void TrafficFlowRestrictions::Condition::unreference(void) const
{
        if (!g_atomic_int_dec_and_test(&m_refcount))
                return;
        delete this;
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::Condition::simplify(void) const
{
	return ptr_t();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::Condition::simplify_bbox(const Rect& bbox) const
{
	return ptr_t();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::Condition::simplify_altrange(int32_t minalt, int32_t maxalt) const
{
	return ptr_t();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::Condition::simplify_aircrafttype(const std::string& acfttype) const
{
	return ptr_t();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::Condition::simplify_aircraftclass(const std::string& acftclass) const
{
	return ptr_t();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::Condition::simplify_equipment(const std::string& equipment, Aircraft::pbn_t pbn) const
{
	return ptr_t();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::Condition::simplify_typeofflight(char type_of_flight) const
{
	return ptr_t();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::Condition::simplify_mil(bool mil) const
{
	return ptr_t();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::Condition::simplify_gat(bool gat) const
{
	return ptr_t();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::Condition::simplify_dep(const std::string& ident, const Point& coord) const
{
	return ptr_t();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::Condition::simplify_dest(const std::string& ident, const Point& coord) const
{
	return ptr_t();
}

std::string TrafficFlowRestrictions::Condition::get_path(const Path *backptr) const
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

void TrafficFlowRestrictions::Condition::trace(TrafficFlowRestrictions& tfrs, const CondResult& r, const TrafficFlowRule *rule, const Path *backptr) const
{
	std::ostringstream oss;
	oss << "Condition " << get_path(backptr);
	{
		bool subseq(false);
		for (CondResult::set_t::const_iterator i(r.get_reflocset().begin()), e(r.get_reflocset().end()); i != e; ++i) {
			if (subseq) {
				oss << ',';
			} else {
				oss << " RefLoc ";
				subseq = true;
			}
			oss << *i;
		}
	}
	oss << " result " << (r.get_result() ? 'T' : (!r.get_result() ? 'F' : 'I'));
	Message msg(oss.str(), Message::type_tracetfe, rule->get_codeid());
	msg.m_vertexset = r.get_vertexset();
	msg.m_edgeset = r.get_edgeset();
	tfrs.message(msg);
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::Condition::evaluate_trace(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset,
										       const TrafficFlowRule *rule, const Path *backptr) const
{
	CondResult r(evaluate(tfrs, reflocset));
	trace(tfrs, r, rule, backptr);
	return r;
}

TrafficFlowRestrictions::RuleResult::Alternative::Sequence TrafficFlowRestrictions::Condition::get_resultsequence(void) const
{
	return RuleResult::Alternative::Sequence(RuleResult::Alternative::Sequence::type_invalid);
}

void TrafficFlowRestrictions::Condition::trace(TrafficFlowRestrictions& tfrs, const DctParameters::AltRange& r, const TrafficFlowRule *rule, const Path *backptr) const
{
	std::ostringstream oss;
	oss << "Condition " << get_path(backptr) << " result " << r[0].to_str() << '/' << r[1].to_str();
	Message msg(oss.str(), Message::type_tracetfe, rule->get_codeid());
	tfrs.message(msg);
}

TrafficFlowRestrictions::DctParameters::AltRange TrafficFlowRestrictions::Condition::evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct) const
{
	DctParameters::AltRange x;
	x.set_empty();
	return x;
}

TrafficFlowRestrictions::DctParameters::AltRange TrafficFlowRestrictions::Condition::evaluate_dct_trace(TrafficFlowRestrictions& tfrs, const DctParameters& dct,
													const TrafficFlowRule *rule, const Path *backptr) const
{
	DctParameters::AltRange r(evaluate_dct(tfrs, dct));
	trace(tfrs, r, rule, backptr);
	return r;
}

std::vector<TrafficFlowRestrictions::RuleResult::Alternative> TrafficFlowRestrictions::Condition::get_mandatory(void) const
{
	return std::vector<RuleResult::Alternative>();
}

std::vector<TrafficFlowRestrictions::RuleResult::Alternative> TrafficFlowRestrictions::Condition::get_mandatory_seq(void) const
{
	return std::vector<RuleResult::Alternative>();
}

std::vector<TrafficFlowRestrictions::RuleResult::Alternative::Sequence> TrafficFlowRestrictions::Condition::get_mandatory_or(void) const
{
	return std::vector<RuleResult::Alternative::Sequence>();
}

TrafficFlowRestrictions::ConditionAnd::ConditionAnd(unsigned int childnum, bool resultinv)
	: Condition(childnum), m_inv(resultinv)
{
}

void TrafficFlowRestrictions::ConditionAnd::add(const_ptr_t p, bool inv)
{
	m_cond.push_back(CondInv(p, inv));
}

bool TrafficFlowRestrictions::ConditionAnd::is_refloc(void) const
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

bool TrafficFlowRestrictions::ConditionAnd::is_constant(void) const
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

bool TrafficFlowRestrictions::ConditionAnd::get_constvalue(void) const
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

std::string TrafficFlowRestrictions::ConditionAnd::to_str(void) const
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
		r += p->to_str();
	}
	r += ")";
	return r;
}

TrafficFlowRestrictions::ConditionAnd::ptr_t TrafficFlowRestrictions::ConditionAnd::clone(void) const
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

TrafficFlowRestrictions::ConditionAnd::ptr_t TrafficFlowRestrictions::ConditionAnd::simplify(const seq_t& seq) const
{
	if (seq.size() != m_cond.size())
		throw std::runtime_error("TrafficFlowRestrictions::ConditionAnd::simplify: internal error");
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

TrafficFlowRestrictions::ConditionAnd::ptr_t TrafficFlowRestrictions::ConditionAnd::simplify(void) const
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

TrafficFlowRestrictions::ConditionAnd::ptr_t TrafficFlowRestrictions::ConditionAnd::simplify_bbox(const Rect& bbox) const
{
	seq_t seq;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (p)
			p = p->simplify_bbox(bbox);
		seq.push_back(p);
	}
	return simplify(seq);
}

TrafficFlowRestrictions::ConditionAnd::ptr_t TrafficFlowRestrictions::ConditionAnd::simplify_altrange(int32_t minalt, int32_t maxalt) const
{
	seq_t seq;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (p)
			p = p->simplify_altrange(minalt, maxalt);
		seq.push_back(p);
	}
	return simplify(seq);
}

TrafficFlowRestrictions::ConditionAnd::ptr_t TrafficFlowRestrictions::ConditionAnd::simplify_aircrafttype(const std::string& acfttype) const
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

TrafficFlowRestrictions::ConditionAnd::ptr_t TrafficFlowRestrictions::ConditionAnd::simplify_aircraftclass(const std::string& acftclass) const
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

TrafficFlowRestrictions::ConditionAnd::ptr_t TrafficFlowRestrictions::ConditionAnd::simplify_equipment(const std::string& equipment, Aircraft::pbn_t pbn) const
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

TrafficFlowRestrictions::ConditionAnd::ptr_t TrafficFlowRestrictions::ConditionAnd::simplify_typeofflight(char type_of_flight) const
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

TrafficFlowRestrictions::ConditionAnd::ptr_t TrafficFlowRestrictions::ConditionAnd::simplify_mil(bool mil) const
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

TrafficFlowRestrictions::ConditionAnd::ptr_t TrafficFlowRestrictions::ConditionAnd::simplify_gat(bool gat) const
{
	seq_t seq;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (p)
			p = p->simplify_gat(gat);
		seq.push_back(p);
	}
	return simplify(seq);
}

TrafficFlowRestrictions::ConditionAnd::ptr_t TrafficFlowRestrictions::ConditionAnd::simplify_dep(const std::string& ident, const Point& coord) const
{
	seq_t seq;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (p)
			p = p->simplify_dep(ident, coord);
		seq.push_back(p);
	}
	return simplify(seq);
}

TrafficFlowRestrictions::ConditionAnd::ptr_t TrafficFlowRestrictions::ConditionAnd::simplify_dest(const std::string& ident, const Point& coord) const
{
	seq_t seq;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (p)
			p = p->simplify_dest(ident, coord);
		seq.push_back(p);
	}
	return simplify(seq);
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::ConditionAnd::evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const
{
	if (m_inv) {
		// OR type
		CondResult r(false);
		r.get_reflocset().insert(reflocset.begin(), reflocset.end());
		for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
			const_ptr_t p(i->get_ptr());
			if (!p)
				continue;
			CondResult r1(p->evaluate(tfrs, r.get_reflocset()));
			if (!i->is_inv() && !boost::logic::indeterminate(r1.get_result()))
				r1.set_result(!r1.get_result());
			r |= r1;
		}
		return r;
	}
	// AND type
	CondResult r(true);
	r.get_reflocset().insert(reflocset.begin(), reflocset.end());
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (!p)
			return CondResult(false);
		CondResult r1(p->evaluate(tfrs, r.get_reflocset()));
		if (i->is_inv() && !boost::logic::indeterminate(r1.get_result()))
			r1.set_result(!r1.get_result());
		r &= r1;
		if (r.get_result() == false)
			return CondResult(false);
	}
	return r;
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::ConditionAnd::evaluate_trace(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset,
											  const TrafficFlowRule *rule, const Path *backptr) const
{
	Path path(backptr, this);
	if (m_inv) {
		// OR type
		CondResult r(false);
		r.get_reflocset().insert(reflocset.begin(), reflocset.end());
		for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
			const_ptr_t p(i->get_ptr());
			if (!p)
				continue;
			CondResult r1(p->evaluate_trace(tfrs, r.get_reflocset(), rule, &path));
			if (!i->is_inv() && !boost::logic::indeterminate(r1.get_result()))
				r1.set_result(!r1.get_result());
			r |= r1;
		}
		trace(tfrs, r, rule, backptr);
		return r;
	}
	// AND type
	CondResult r(true);
	r.get_reflocset().insert(reflocset.begin(), reflocset.end());
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (!p) {
			r = CondResult(false);
			break;
		}
		CondResult r1(p->evaluate_trace(tfrs, r.get_reflocset(), rule, &path));
		if (i->is_inv() && !boost::logic::indeterminate(r1.get_result()))
			r1.set_result(!r1.get_result());
		r &= r1;
		if (r.get_result() == false) {
			r = CondResult(false);
			break;
		}
	}
	trace(tfrs, r, rule, backptr);
	return r;
}

bool TrafficFlowRestrictions::ConditionAnd::is_routestatic(void) const
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

bool TrafficFlowRestrictions::ConditionAnd::is_dct(void) const
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

bool TrafficFlowRestrictions::ConditionAnd::is_valid_dct(void) const
{
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (!p)
			return false;;
		if (!p->is_valid_dct())
			return false;
	}
	return true;
}

TrafficFlowRestrictions::DctParameters::AltRange TrafficFlowRestrictions::ConditionAnd::evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct) const
{
	if (m_inv) {
		// OR type
		DctParameters::AltRange r;
		r.set_empty();
		for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
			const_ptr_t p(i->get_ptr());
			if (!p)
				continue;
			DctParameters::AltRange r1(p->evaluate_dct(tfrs, dct));
			if (!i->is_inv())
				r1.invert();
			r |= r1;
		}
		return r;
	}
	// AND type
	DctParameters::AltRange r(dct.get_alt());
	//r.set_full();
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (!p) {
			r.set_empty();
			break;
		}
		DctParameters::AltRange r1(p->evaluate_dct(tfrs, dct));
		if (i->is_inv())
			r1.invert();
		r &= r1;
		if (r.is_empty())
			break;
	}
	return r;
}

TrafficFlowRestrictions::DctParameters::AltRange TrafficFlowRestrictions::ConditionAnd::evaluate_dct_trace(TrafficFlowRestrictions& tfrs, const DctParameters& dct,
													   const TrafficFlowRule *rule, const Path *backptr) const
{
	Path path(backptr, this);
	if (m_inv) {
		// OR type
		DctParameters::AltRange r;
		r.set_empty();
		for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
			const_ptr_t p(i->get_ptr());
			if (!p)
				continue;
			DctParameters::AltRange r1(p->evaluate_dct_trace(tfrs, dct, rule, &path));
			if (!i->is_inv())
				r1.invert();
			r |= r1;
		}
		trace(tfrs, r, rule, backptr);
		return r;
	}
	// AND type
	DctParameters::AltRange r(dct.get_alt());
	//r.set_full();
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		const_ptr_t p(i->get_ptr());
		if (!p) {
			r.set_empty();
			break;
		}
		DctParameters::AltRange r1(p->evaluate_dct_trace(tfrs, dct, rule, &path));
		if (i->is_inv())
			r1.invert();
		r &= r1;
		if (r.is_empty())
			break;
	}
	trace(tfrs, r, rule, backptr);
	return r;
}

std::vector<TrafficFlowRestrictions::RuleResult::Alternative> TrafficFlowRestrictions::ConditionAnd::get_mandatory(void) const
{
	if (m_inv)
		// OR type
		return std::vector<RuleResult::Alternative>();
	// AND type
	std::vector<RuleResult::Alternative> r;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		if (!i->is_inv())
			continue;
		const_ptr_t p(i->get_ptr());
		if (!p)
			continue;
		{
			RuleResult::Alternative::Sequence seq(p->get_resultsequence());
			if (seq.is_valid()) {
				r.push_back(RuleResult::Alternative());
				r.back().add_sequence(seq);
			}
		}
		{
			std::vector<RuleResult::Alternative::Sequence> seq(p->get_mandatory_or());
			for (std::vector<RuleResult::Alternative::Sequence>::const_iterator si(seq.begin()), se(seq.end()); si != se; ++si) {
				if (!si->is_valid())
					continue;
				r.push_back(RuleResult::Alternative());
				r.back().add_sequence(*si);
			}
		}
		{
			std::vector<RuleResult::Alternative> alt(p->get_mandatory_seq());
			for (std::vector<RuleResult::Alternative>::const_iterator ai(alt.begin()), ae(alt.end()); ai != ae; ++ai) {
				r.push_back(*ai);
			}
		}
	}
	return r;
}

std::vector<TrafficFlowRestrictions::RuleResult::Alternative::Sequence> TrafficFlowRestrictions::ConditionAnd::get_mandatory_or(void) const
{
	if (!m_inv)
		// AND type
		return std::vector<RuleResult::Alternative::Sequence>();
	// OR type
	std::vector<RuleResult::Alternative::Sequence> r;
	for (cond_t::const_iterator i(m_cond.begin()), e(m_cond.end()); i != e; ++i) {
		if (!i->is_inv())
			return std::vector<RuleResult::Alternative::Sequence>();
		const_ptr_t p(i->get_ptr());
		if (!p)
			return std::vector<RuleResult::Alternative::Sequence>();
	        r.push_back(p->get_resultsequence());
		if (!r.back().is_valid())
			return std::vector<RuleResult::Alternative::Sequence>();
	}
	return r;
}

TrafficFlowRestrictions::ConditionSequence::ConditionSequence(unsigned int childnum)
	: Condition(childnum)
{
}

void TrafficFlowRestrictions::ConditionSequence::add(const_ptr_t p)
{
	m_seq.push_back(p);
}

bool TrafficFlowRestrictions::ConditionSequence::is_refloc(void) const
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

bool TrafficFlowRestrictions::ConditionSequence::is_constant(void) const
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

bool TrafficFlowRestrictions::ConditionSequence::get_constvalue(void) const
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

std::string TrafficFlowRestrictions::ConditionSequence::to_str(void) const
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
		r += p->to_str();
	}
	r += ")";
	return r;
}

TrafficFlowRestrictions::ConditionSequence::ptr_t TrafficFlowRestrictions::ConditionSequence::clone(void) const
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

TrafficFlowRestrictions::ConditionSequence::ptr_t TrafficFlowRestrictions::ConditionSequence::simplify(const seq_t& seq) const
{
	if (seq.size() != m_seq.size())
		throw std::runtime_error("TrafficFlowRestrictions::ConditionSequence::simplify: internal error");
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

TrafficFlowRestrictions::ConditionSequence::ptr_t TrafficFlowRestrictions::ConditionSequence::simplify(void) const
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

TrafficFlowRestrictions::ConditionSequence::ptr_t TrafficFlowRestrictions::ConditionSequence::simplify_bbox(const Rect& bbox) const
{
	seq_t seq;
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (p)
			p = p->simplify_bbox(bbox);
		seq.push_back(p);
	}
	return simplify(seq);
}

TrafficFlowRestrictions::ConditionSequence::ptr_t TrafficFlowRestrictions::ConditionSequence::simplify_altrange(int32_t minalt, int32_t maxalt) const
{
	seq_t seq;
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (p)
			p = p->simplify_altrange(minalt, maxalt);
		seq.push_back(p);
	}
	return simplify(seq);
}

TrafficFlowRestrictions::ConditionSequence::ptr_t TrafficFlowRestrictions::ConditionSequence::simplify_aircrafttype(const std::string& acfttype) const
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

TrafficFlowRestrictions::ConditionSequence::ptr_t TrafficFlowRestrictions::ConditionSequence::simplify_aircraftclass(const std::string& acftclass) const
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

TrafficFlowRestrictions::ConditionSequence::ptr_t TrafficFlowRestrictions::ConditionSequence::simplify_equipment(const std::string& equipment, Aircraft::pbn_t pbn) const
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

TrafficFlowRestrictions::ConditionSequence::ptr_t TrafficFlowRestrictions::ConditionSequence::simplify_typeofflight(char type_of_flight) const
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

TrafficFlowRestrictions::ConditionSequence::ptr_t TrafficFlowRestrictions::ConditionSequence::simplify_mil(bool mil) const
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

TrafficFlowRestrictions::ConditionSequence::ptr_t TrafficFlowRestrictions::ConditionSequence::simplify_gat(bool gat) const
{
	seq_t seq;
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (p)
			p = p->simplify_gat(gat);
		seq.push_back(p);
	}
	return simplify(seq);
}

TrafficFlowRestrictions::ConditionSequence::ptr_t TrafficFlowRestrictions::ConditionSequence::simplify_dep(const std::string& ident, const Point& coord) const
{
	seq_t seq;
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (p)
			p = p->simplify_dep(ident, coord);
		seq.push_back(p);
	}
	return simplify(seq);
}

TrafficFlowRestrictions::ConditionSequence::ptr_t TrafficFlowRestrictions::ConditionSequence::simplify_dest(const std::string& ident, const Point& coord) const
{
	seq_t seq;
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (p)
			p = p->simplify_dest(ident, coord);
		seq.push_back(p);
	}
	return simplify(seq);
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::ConditionSequence::evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const
{
	CondResult r(true);
	bool first(true);
	r.get_reflocset().insert(reflocset.begin(), reflocset.end());
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (!p)
			return CondResult(false);
		if (first) {
			r = p->evaluate(tfrs, r.get_reflocset());
			first = false;
		} else {
			CondResult r1(p->evaluate(tfrs, r.get_reflocset()));
			if (r.get_first() > r1.get_first())
				return CondResult(false);
			r &= r1;
		}
		if (r.get_result() == false)
			return CondResult(false);
	}
	return r;
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::ConditionSequence::evaluate_trace(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset,
											       const TrafficFlowRule *rule, const Path *backptr) const
{
	Path path(backptr, this);
	CondResult r(true);
	bool first(true);
	r.get_reflocset().insert(reflocset.begin(), reflocset.end());
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (!p) {
			r = CondResult(false);
			break;
		}
		if (first) {
			r = p->evaluate_trace(tfrs, r.get_reflocset(), rule, &path);
			first = false;
		} else {
			CondResult r1(p->evaluate_trace(tfrs, r.get_reflocset(), rule, &path));
			if (r.get_first() > r1.get_first()) {
				r = CondResult(false);
				break;
			}
			r &= r1;
		}
		if (r.get_result() == false) {
			r = CondResult(false);
			break;
		}
	}
	trace(tfrs, r, rule, backptr);
	return r;
}

bool TrafficFlowRestrictions::ConditionSequence::is_routestatic(void) const
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

std::vector<TrafficFlowRestrictions::RuleResult::Alternative> TrafficFlowRestrictions::ConditionSequence::get_mandatory_seq(void) const
{
	std::vector<RuleResult::Alternative> r;
	for (seq_t::const_iterator i(m_seq.begin()), e(m_seq.end()); i != e; ++i) {
		const_ptr_t p(*i);
		if (!p)
			return std::vector<RuleResult::Alternative>();
		{
			RuleResult::Alternative::Sequence seq(p->get_resultsequence());
			if (seq.is_valid()) {
				if (r.empty())
					r.push_back(RuleResult::Alternative());
				for (std::vector<RuleResult::Alternative>::iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
					ri->add_sequence(seq);
				}
			}
		}
		{
			std::vector<RuleResult::Alternative::Sequence> seq(p->get_mandatory_or());
			std::vector<RuleResult::Alternative> rnew;
			for (std::vector<RuleResult::Alternative::Sequence>::const_iterator si(seq.begin()), se(seq.end()); si != se; ++si) {
				if (!si->is_valid())
					continue;
				if (r.empty()) {
					rnew.push_back(RuleResult::Alternative());
					rnew.back().add_sequence(*si);
					continue;
				}
				for (std::vector<RuleResult::Alternative>::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
					rnew.push_back(*ri);
					rnew.back().add_sequence(*si);
				}
			}
			r.swap(rnew);
		}
	}
	return r;
}

TrafficFlowRestrictions::ConditionConstant::ConditionConstant(unsigned int childnum, bool val)
	: Condition(childnum), m_value(val)
{
}

std::string TrafficFlowRestrictions::ConditionConstant::to_str(void) const
{
	std::ostringstream oss;
	oss << "Const{" << get_childnum() << "}(" << (m_value ? 'T' : 'F') << ')';
	return oss.str();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionConstant::clone(void) const
{
	return ptr_t(new ConditionConstant(m_childnum, m_value));
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::ConditionConstant::evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const
{
	return CondResult(m_value);
}

TrafficFlowRestrictions::DctParameters::AltRange TrafficFlowRestrictions::ConditionConstant::evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct) const
{
	DctParameters::AltRange x(dct.get_alt());
	if (!m_value)
		x.set_empty();
	return x;
}

TrafficFlowRestrictions::ConditionAltitude::ConditionAltitude(unsigned int childnum, const AltRange& alt)
	: Condition(childnum), m_alt(alt)
{
}

TrafficFlowRestrictions::ConditionAltitude::ptr_t TrafficFlowRestrictions::ConditionAltitude::simplify_altrange(int32_t minalt, int32_t maxalt) const
{
	if (minalt > maxalt || 
	    (m_alt.is_lower_valid() && maxalt < m_alt.get_lower_alt()) ||
	    (m_alt.is_upper_valid() && minalt > m_alt.get_upper_alt()))
		return ptr_t(new ConditionConstant(get_childnum(), false));
	return ptr_t();
}

bool TrafficFlowRestrictions::ConditionAltitude::check_alt(int32_t alt) const
{
	return !m_alt.is_valid() || (alt >= m_alt.get_lower_alt() && alt <= m_alt.get_upper_alt());
}

TrafficFlowRestrictions::ConditionCrossingAirspace1::ConditionCrossingAirspace1(unsigned int childnum, const AltRange& alt, TFRAirspace::const_ptr_t aspc, bool refloc)
	: ConditionAltitude(childnum, alt), m_airspace(aspc), m_refloc(refloc)
{
}

std::string TrafficFlowRestrictions::ConditionCrossingAirspace1::to_str(void) const
{
	std::ostringstream oss;
	oss << "XngAirspace{" << get_childnum();
	if (m_refloc)
		oss << ",R";
	oss << "}(" << to_altstr() << ',';
	if (m_airspace)
		oss << m_airspace->get_ident() << '/' << m_airspace->get_type();
	else
		oss << "null";
	oss << ')';
	return oss.str();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionCrossingAirspace1::clone(void) const
{
	return ptr_t(new ConditionCrossingAirspace1(m_childnum, m_alt, m_airspace, m_refloc));
}

TrafficFlowRestrictions::ConditionCrossingAirspace1::ptr_t TrafficFlowRestrictions::ConditionCrossingAirspace1::simplify_bbox(const Rect& bbox) const
{
	if (!m_airspace || !bbox.is_intersect(m_airspace->get_bbox()))
		return ptr_t(new ConditionConstant(get_childnum(), false));
	return ptr_t();
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::ConditionCrossingAirspace1::evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const
{
	if (!m_airspace)
		return CondResult(false);
	const Fpl& fpl(tfrs.get_route());
	CondResult r(false);
	bool refloc(m_refloc);
	for (unsigned int wptnr = 1U; wptnr < fpl.size(); ++wptnr) {
		const FplWpt& wpt0(fpl[wptnr - 1U]);
		const FplWpt& wpt1(fpl[wptnr]);
		if (!wpt0.is_ifr())
			continue;
		const Point& pt0(wpt0.get_coord());
		const Point& pt1(wpt1.get_coord());
		if ((!ifr_refloc || !m_refloc || wpt0.is_ifr()) &&
		    m_airspace->is_inside(pt0, wpt0.get_altitude(), m_alt.get_lower_alt_if_valid(), m_alt.get_upper_alt_if_valid())) {
			r.set_result(true);
			r.get_vertexset().insert(wptnr - 1U);
			if (refloc) {
				r.get_reflocset().insert(wptnr - 1U);
				refloc = false;
			}
			continue;
		}
		if ((!ifr_refloc ||  !m_refloc || wpt1.is_ifr()) &&
		    m_airspace->is_inside(pt1, wpt1.get_altitude(), m_alt.get_lower_alt_if_valid(), m_alt.get_upper_alt_if_valid())) {
			r.set_result(true);
			r.get_vertexset().insert(wptnr);
			if (refloc) {
				r.get_reflocset().insert(wptnr);
				refloc = false;
			}
			continue;
		}
		if ((!ifr_refloc ||  !m_refloc || wpt0.is_ifr() || wpt1.is_ifr()) &&
		    m_airspace->is_intersection(pt0, pt1, wpt1.get_altitude(), m_alt.get_lower_alt_if_valid(), m_alt.get_upper_alt_if_valid())) {
			r.set_result(true);
			r.get_edgeset().insert(wptnr - 1U);
			if (refloc) {
				r.get_reflocset().insert(wptnr);
				refloc = false;
			}
			continue;
		}
	}
	return r;
}

TrafficFlowRestrictions::DctParameters::AltRange TrafficFlowRestrictions::ConditionCrossingAirspace1::evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct) const
{
	DctParameters::AltRange::set_t r;
	r.set_empty();
	if (!m_airspace)
		return DctParameters::AltRange(r, r);
	r |= m_airspace->get_altrange(dct.get_coord(0), dct.get_coord(1), m_alt.get_lower_alt_if_valid(), m_alt.get_upper_alt_if_valid());
	return DctParameters::AltRange(r, r);
}

constexpr float TrafficFlowRestrictions::ConditionCrossingAirspace2::fuzzy_dist;
const unsigned int TrafficFlowRestrictions::ConditionCrossingAirspace2::fuzzy_exponent;

TrafficFlowRestrictions::ConditionCrossingAirspace2::ConditionCrossingAirspace2(unsigned int childnum, const AltRange& alt, TFRAirspace::const_ptr_t aspc0, TFRAirspace::const_ptr_t aspc1, bool refloc)
	: ConditionAltitude(childnum, alt), m_airspace0(aspc0), m_airspace1(aspc1), m_refloc(refloc)
{
}

uint8_t TrafficFlowRestrictions::ConditionCrossingAirspace2::rev8(uint8_t x)
{
        x = ((x >> 4) & 0x0F) | ((x << 4) & 0xF0);
        x = ((x >> 2) & 0x33) | ((x << 2) & 0xCC);
        x = ((x >> 1) & 0x55) | ((x << 1) & 0xAA);
        return x;
}

std::string TrafficFlowRestrictions::ConditionCrossingAirspace2::to_str(void) const
{
	std::ostringstream oss;
	oss << "XngAirspace2{" << get_childnum();
	if (m_refloc)
		oss << ",R";
	oss << "}(" << to_altstr() << ',';
	if (m_airspace0)
		oss << m_airspace0->get_ident() << '/' << m_airspace0->get_type();
	else
		oss << "null";
	oss << ',';
	if (m_airspace1)
		oss << m_airspace1->get_ident() << '/' << m_airspace1->get_type();
	else
		oss << "null";
	oss << ')';
	return oss.str();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionCrossingAirspace2::clone(void) const
{
	return ptr_t(new ConditionCrossingAirspace2(m_childnum, m_alt, m_airspace0, m_airspace1, m_refloc));
}

TrafficFlowRestrictions::ConditionCrossingAirspace2::ptr_t TrafficFlowRestrictions::ConditionCrossingAirspace2::simplify_bbox(const Rect& bbox) const
{
	if (!m_airspace0 || !m_airspace1 || !(bbox.is_intersect(m_airspace0->get_bbox()) || bbox.is_intersect(m_airspace1->get_bbox())))
		return ptr_t(new ConditionConstant(get_childnum(), false));
	return ptr_t();
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::ConditionCrossingAirspace2::evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const
{
	if (!m_airspace0 || !m_airspace1)
		return CondResult(false);
	const Fpl& fpl(tfrs.get_route());
	CondResult r(false);
	bool refloc(m_refloc);
	for (unsigned int wptnr = 1U; wptnr < fpl.size(); ++wptnr) {
		const FplWpt& wpt0(fpl[wptnr - 1U]);
		const FplWpt& wpt1(fpl[wptnr]);
		if (!wpt0.is_ifr())
			continue;
		if (ifr_refloc && m_refloc && !(wpt0.is_ifr() && wpt1.is_ifr()))
			continue;
		const Point& pt0(wpt0.get_coord());
		const Point& pt1(wpt1.get_coord());
		if (!m_airspace0->is_inside(pt0, wpt0.get_altitude(), m_alt.get_lower_alt_if_valid(), m_alt.get_upper_alt_if_valid()) ||
		    !m_airspace1->is_inside(pt1, wpt1.get_altitude(), m_alt.get_lower_alt_if_valid(), m_alt.get_upper_alt_if_valid()))
			continue;
		if (fuzzy_exponent && fuzzy_dist > 0) {
			// fuzzy match
			bool ok(false);
			for (unsigned int fm = 1U << fuzzy_exponent; !ok;) {
				if (!fm)
					break;
				--fm;
				float crs(rev8(fm) * (360.0 / 256.0));
				Point ptx0(pt0.simple_course_distance_nmi(crs, fuzzy_dist));
				Point ptx1(pt1.simple_course_distance_nmi(crs, fuzzy_dist));
				ok = !m_airspace0->is_inside(ptx0, wpt0.get_altitude(), m_alt.get_lower_alt_if_valid(), m_alt.get_upper_alt_if_valid()) ||
					!m_airspace1->is_inside(ptx1, wpt1.get_altitude(), m_alt.get_lower_alt_if_valid(), m_alt.get_upper_alt_if_valid());
			}
			if (ok)
				continue;
		}
		r.set_result(true);
		r.get_edgeset().insert(wptnr - 1U);
		if (refloc) {
			r.get_reflocset().insert(wptnr - 1U);
			refloc = false;
		}
	}
	return r;
}

TrafficFlowRestrictions::DctParameters::AltRange TrafficFlowRestrictions::ConditionCrossingAirspace2::evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct) const
{
	DctParameters::AltRange r;
	r.set_empty();
	if (!m_airspace0 || !m_airspace1)
		return r;
	for (unsigned int i = 0; i < 2; ++i) {
		r[i] = m_airspace0->get_altrange(dct.get_coord(i), m_alt.get_lower_alt_if_valid(), m_alt.get_upper_alt_if_valid()) &
			m_airspace1->get_altrange(dct.get_coord(!i), m_alt.get_lower_alt_if_valid(), m_alt.get_upper_alt_if_valid());
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
			Point ptx0(dct.get_coord(i).simple_course_distance_nmi(crs, fuzzy_dist));
			Point ptx1(dct.get_coord(!i).simple_course_distance_nmi(crs, fuzzy_dist));
			r[i] &= m_airspace0->get_altrange(ptx0, m_alt.get_lower_alt_if_valid(), m_alt.get_upper_alt_if_valid()) &
				m_airspace1->get_altrange(ptx1, m_alt.get_lower_alt_if_valid(), m_alt.get_upper_alt_if_valid());
		}
	}
	return r;
}

TrafficFlowRestrictions::ConditionCrossingDct::ConditionCrossingDct(unsigned int childnum, const AltRange& alt, const RuleWpt& wpt0, const RuleWpt& wpt1, bool refloc)
	: ConditionAltitude(childnum, alt), m_wpt0(wpt0), m_wpt1(wpt1), m_refloc(refloc)
{
}

std::string TrafficFlowRestrictions::ConditionCrossingDct::to_str(void) const
{
	std::ostringstream oss;
	oss << "XngDct{" << get_childnum();
	if (m_refloc)
		oss << ",R";
	oss << "}(" << to_altstr() << ',' << m_wpt0.get_ident() << ',' << m_wpt1.get_ident() << ')';
	return oss.str();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionCrossingDct::clone(void) const
{
	return ptr_t(new ConditionCrossingDct(m_childnum, m_alt, m_wpt0, m_wpt1, m_refloc));
}

TrafficFlowRestrictions::ConditionCrossingDct::ptr_t TrafficFlowRestrictions::ConditionCrossingDct::simplify_bbox(const Rect& bbox) const
{
	if (!(bbox.is_inside(m_wpt0.get_coord()) || bbox.is_inside(m_wpt1.get_coord()) || bbox.is_intersect(m_wpt0.get_coord(), m_wpt1.get_coord())))
		return ptr_t(new ConditionConstant(get_childnum(), false));
	return ptr_t();
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::ConditionCrossingDct::evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const
{
	Graph::vertex_descriptor u(tfrs.get_vertexdesc(m_wpt0));
	Graph::vertex_descriptor v(tfrs.get_vertexdesc(m_wpt1));
	if (u == TrafficFlowRestrictions::invalid_vertex_descriptor ||
	    v == TrafficFlowRestrictions::invalid_vertex_descriptor)
		return CondResult(false);
	const Fpl& fpl(tfrs.get_route());
	CondResult r(false);
	for (unsigned int wptnr = 1U; wptnr < fpl.size(); ++wptnr) {
		const FplWpt& wpt0(fpl[wptnr - 1U]);
		const FplWpt& wpt1(fpl[wptnr]);
		// accept none instead of directto for first and last leg
		if (!wpt0.is_ifr() || !wpt0.within(m_alt) ||
		    (wpt0.get_pathcode() != FPlanWaypoint::pathcode_directto &&
		     (wpt0.get_pathcode() != FPlanWaypoint::pathcode_none ||
		      (wptnr > 1U && wptnr + 1U < fpl.size()))))
			continue;
		if (ifr_refloc && m_refloc && !wpt0.is_ifr())
			continue;
		if (wpt0.get_vertex_descriptor() == u &&
		    wpt1.get_vertex_descriptor() == v) {
			r.set_result(true);
			r.get_edgeset().insert(wptnr - 1U);
			if (m_refloc)
				r.get_reflocset().insert(wptnr);
		}
	}
	return r;
}

TrafficFlowRestrictions::RuleResult::Alternative::Sequence TrafficFlowRestrictions::ConditionCrossingDct::get_resultsequence(void) const
{
	int32_t altlwr(m_alt.get_lower_alt()), altupr(m_alt.get_upper_alt());
        if (!m_alt.is_lower_valid())
                altlwr = 0;
        if (!m_alt.is_upper_valid())
                altupr = std::numeric_limits<int32_t>::max();
	return RuleResult::Alternative::Sequence(RuleResult::Alternative::Sequence::type_dct,
                                                 RuleResult::Alternative::Sequence::RulePoint(m_wpt0),
                                                 RuleResult::Alternative::Sequence::RulePoint(m_wpt1),
                                                 altlwr, altupr, "", "", 0, m_alt.is_lower_valid(), m_alt.is_upper_valid());
}

TrafficFlowRestrictions::DctParameters::AltRange TrafficFlowRestrictions::ConditionCrossingDct::evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct) const
{
	DctParameters::AltRange r;
	r.set_empty();
	for (unsigned int i = 0; i < 2; ++i) {
		if (m_wpt0.get_ident() != dct.get_ident(i) || m_wpt1.get_ident() != dct.get_ident(!i) ||
		    m_wpt0.get_coord().spheric_distance_nmi_dbl(dct.get_coord(i)) > 0.1 ||
		    m_wpt1.get_coord().spheric_distance_nmi_dbl(dct.get_coord(!i)) > 0.1)
			continue;
		r[i] = DctParameters::AltRange::set_t::Intvl(m_alt.get_lower_alt(), m_alt.get_upper_alt());
	}
	return r;
}

TrafficFlowRestrictions::ConditionCrossingAirway::ConditionCrossingAirway(unsigned int childnum, const AltRange& alt, const RuleWpt& wpt0, const RuleWpt& wpt1,
									  const std::string& awyname, bool refloc)
	: ConditionCrossingDct(childnum, alt, wpt0, wpt1, refloc), m_awyname(awyname)
{
}

std::string TrafficFlowRestrictions::ConditionCrossingAirway::to_str(void) const
{
	std::ostringstream oss;
	oss << "XngAwy{" << get_childnum();
	if (m_refloc)
		oss << ",R";
	oss << "}(" << to_altstr() << ',' << m_wpt0.get_ident() << ',' << m_wpt1.get_ident() << ',' << m_awyname << ')';
	return oss.str();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionCrossingAirway::clone(void) const
{
	return ptr_t(new ConditionCrossingAirway(m_childnum, m_alt, m_wpt0, m_wpt1, m_awyname, m_refloc));
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::ConditionCrossingAirway::evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const
{
	Graph::vertex_descriptor u(tfrs.get_vertexdesc(m_wpt0));
	Graph::vertex_descriptor v(tfrs.get_vertexdesc(m_wpt1));
	if (u == TrafficFlowRestrictions::invalid_vertex_descriptor ||
	    v == TrafficFlowRestrictions::invalid_vertex_descriptor)
		return CondResult(false);
	const Fpl& fpl(tfrs.get_route());
	CondResult r(false);
	for (unsigned int wptnr = 1U; wptnr < fpl.size(); ++wptnr) {
		const FplWpt& wpt(fpl[wptnr - 1U]);
		if (wpt.get_vertex_descriptor() != u || !wpt.within(m_alt))
			continue;
		if (ifr_refloc && m_refloc && !wpt.is_ifr())
			continue;
		CondResult r1(true);
		bool refloc(m_refloc);
		for (unsigned int wptnre(wptnr); wptnre < fpl.size(); ++wptnre) {
			const FplWpt& wpt(fpl[wptnre - 1U]);
			const FplWpt& wpte(fpl[wptnre]);
			if (!wpt.is_ifr() ||
			    wpt.get_pathcode() != FPlanWaypoint::pathcode_airway ||
			    wpt.get_pathname() != m_awyname) {
				r1.set_result(false);
				break;
			}
			r1.get_edgeset().insert(wptnre - 1U);
			if (refloc) {
				r1.get_reflocset().insert(wptnre - 1U);
				refloc = false;
			}
			if (wpte.get_vertex_descriptor() == v)
				break;
		}
		if (r1.get_result()) {
			r.set_result(true);
			r.get_edgeset().insert(r1.get_edgeset().begin(), r1.get_edgeset().end());
			if (r.get_reflocset().empty())
				r.get_reflocset().insert(r1.get_reflocset().begin(), r1.get_reflocset().end());
		}
	}
	return r;
}

TrafficFlowRestrictions::RuleResult::Alternative::Sequence TrafficFlowRestrictions::ConditionCrossingAirway::get_resultsequence(void) const
{
	int32_t altlwr(m_alt.get_lower_alt()), altupr(m_alt.get_upper_alt());
        if (!m_alt.is_lower_valid())
                altlwr = 0;
        if (!m_alt.is_upper_valid())
                altupr = std::numeric_limits<int32_t>::max();
	return RuleResult::Alternative::Sequence(RuleResult::Alternative::Sequence::type_airway,
                                                 RuleResult::Alternative::Sequence::RulePoint(m_wpt0),
                                                 RuleResult::Alternative::Sequence::RulePoint(m_wpt1),
                                                 altlwr, altupr, m_awyname, "", 0, m_alt.is_lower_valid(), m_alt.is_upper_valid());
}

TrafficFlowRestrictions::ConditionCrossingPoint::ConditionCrossingPoint(unsigned int childnum, const AltRange& alt, const RuleWpt& wpt, bool refloc)
	: ConditionAltitude(childnum, alt), m_wpt(wpt), m_refloc(refloc)
{
}

std::string TrafficFlowRestrictions::ConditionCrossingPoint::to_str(void) const
{
	std::ostringstream oss;
	oss << "XngPoint{" << get_childnum();
	if (m_refloc)
		oss << ",R";
	oss << "}(" << to_altstr() << ',' << m_wpt.get_ident() << ')';
	return oss.str();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionCrossingPoint::clone(void) const
{
	return ptr_t(new ConditionCrossingPoint(m_childnum, m_alt, m_wpt, m_refloc));
}

TrafficFlowRestrictions::ConditionCrossingPoint::ptr_t TrafficFlowRestrictions::ConditionCrossingPoint::simplify_bbox(const Rect& bbox) const
{
	if (!bbox.is_inside(m_wpt.get_coord()))
		return ptr_t(new ConditionConstant(get_childnum(), false));
	return ptr_t();
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::ConditionCrossingPoint::evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const
{
	Graph::vertex_descriptor u(tfrs.get_vertexdesc(m_wpt));
	if (u == TrafficFlowRestrictions::invalid_vertex_descriptor)
		return CondResult(false);
	const Fpl& fpl(tfrs.get_route());
	CondResult r(false);
	bool refloc(m_refloc);
	for (unsigned int wptnr = 0U; wptnr < fpl.size(); ++wptnr) {
		const FplWpt& wpt(fpl[wptnr]);
		if (!wpt.is_ifr() && (!wptnr || !fpl[wptnr - 1U].is_ifr()))
			continue;
		if (wpt.get_vertex_descriptor() != u || !wpt.within(m_alt))
			continue;
		if (ifr_refloc && m_refloc && !wpt.is_ifr())
			continue;
		r.set_result(true);
		r.get_vertexset().insert(wptnr);
		if (refloc) {
			r.get_reflocset().insert(wptnr);
			refloc = false;
		}
	}
	return r;
}

TrafficFlowRestrictions::RuleResult::Alternative::Sequence TrafficFlowRestrictions::ConditionCrossingPoint::get_resultsequence(void) const
{
	int32_t altlwr(m_alt.get_lower_alt()), altupr(m_alt.get_upper_alt());
        if (!m_alt.is_lower_valid())
                altlwr = 0;
        if (!m_alt.is_upper_valid())
                altupr = std::numeric_limits<int32_t>::max();
	return RuleResult::Alternative::Sequence(RuleResult::Alternative::Sequence::type_point,
                                                 RuleResult::Alternative::Sequence::RulePoint(m_wpt),
                                                 RuleResult::Alternative::Sequence::RulePoint(m_wpt),
                                                 altlwr, altupr, "", "", 0, m_alt.is_lower_valid(), m_alt.is_upper_valid());
}

TrafficFlowRestrictions::DctParameters::AltRange TrafficFlowRestrictions::ConditionCrossingPoint::evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct) const
{
	DctParameters::AltRange r;
	r.set_empty();
	for (unsigned int i = 0; i < 2; ++i) {
		if (m_wpt.get_ident() != dct.get_ident(i) ||
		    m_wpt.get_coord().spheric_distance_nmi_dbl(dct.get_coord(i)) > 0.1)
			continue;
		r.set(m_alt);
		break;
	}
	return r;
}

TrafficFlowRestrictions::ConditionDepArr::ConditionDepArr(unsigned int childnum, const RuleWpt& airport, bool arr, bool refloc)
	: Condition(childnum), m_airport(airport), m_arr(arr), m_refloc(refloc)
{
}

std::string TrafficFlowRestrictions::ConditionDepArr::to_str(void) const
{
	std::ostringstream oss;
	oss << (m_arr ? "Dest" : "Dep") << '{' << get_childnum();
	if (m_refloc)
		oss << ",R";
	oss << "}(" << m_airport.get_ident() << ')';
	return oss.str();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionDepArr::clone(void) const
{
	return ptr_t(new ConditionDepArr(m_childnum, m_airport, m_arr, m_refloc));
}

TrafficFlowRestrictions::ConditionDepArr::ptr_t TrafficFlowRestrictions::ConditionDepArr::simplify_bbox(const Rect& bbox) const
{
	if (!m_airport.get_coord().is_invalid() && !bbox.is_inside(m_airport.get_coord()))
		return ptr_t(new ConditionConstant(get_childnum(), false));
	return ptr_t();
}

TrafficFlowRestrictions::ConditionDepArr::ptr_t TrafficFlowRestrictions::ConditionDepArr::simplify_dep(const std::string& ident, const Point& coord) const
{
	if (m_arr)
		return ptr_t();
	bool x(ident == m_airport.get_ident());
	if (x && m_refloc)
		return ptr_t();
	return ptr_t(new ConditionConstant(get_childnum(), x));
}

TrafficFlowRestrictions::ConditionDepArr::ptr_t TrafficFlowRestrictions::ConditionDepArr::simplify_dest(const std::string& ident, const Point& coord) const
{
	if (!m_arr)
		return ptr_t();
	bool x(ident == m_airport.get_ident());
	if (x && m_refloc)
		return ptr_t();
	return ptr_t(new ConditionConstant(get_childnum(), x));
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::ConditionDepArr::evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const
{
	const Fpl& fpl(tfrs.get_route());
	const FplWpt& wpt(m_arr ? fpl.back() : fpl.front());
	if (ifr_refloc && m_refloc && !wpt.is_ifr())
		return CondResult(false);
	if (m_airport.get_ident() == wpt.get_icao()) {
		CondResult r(true);
		r.get_vertexset().insert(m_arr ? fpl.size() - 1U : 0U);
		if (m_refloc)
			r.get_reflocset().insert(m_arr ? fpl.size() - 1U : 0U);
		return r;
	}
	return CondResult(false);
}

TrafficFlowRestrictions::ConditionDepArrAirspace::ConditionDepArrAirspace(unsigned int childnum, TFRAirspace::const_ptr_t aspc, bool arr, bool refloc)
	: Condition(childnum), m_airspace(aspc), m_arr(arr), m_refloc(refloc)
{
}

std::string TrafficFlowRestrictions::ConditionDepArrAirspace::to_str(void) const
{
	std::ostringstream oss;
	oss << (m_arr ? "DestAirspace" : "DepAirspace") << '{' << get_childnum();
	if (m_refloc)
		oss << ",R";
	oss << "}(";
	if (m_airspace)
		oss << m_airspace->get_ident() << '/' << m_airspace->get_type();
	else
		oss << "null";
	oss << ')';
	return oss.str();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionDepArrAirspace::clone(void) const
{
	return ptr_t(new ConditionDepArrAirspace(m_childnum, m_airspace, m_arr, m_refloc));
}

TrafficFlowRestrictions::ConditionDepArrAirspace::ptr_t TrafficFlowRestrictions::ConditionDepArrAirspace::simplify_bbox(const Rect& bbox) const
{
	if (!m_airspace || !bbox.is_intersect(m_airspace->get_bbox()))
		return ptr_t(new ConditionConstant(get_childnum(), false));
	return ptr_t();
}

TrafficFlowRestrictions::ConditionDepArrAirspace::ptr_t TrafficFlowRestrictions::ConditionDepArrAirspace::simplify_dep(const std::string& ident, const Point& coord) const
{
	if (m_arr)
		return ptr_t();
	bool x(m_airspace && m_airspace->get_bbox().is_inside(coord) && m_airspace->get_poly().windingnumber(coord));
	if (x && m_refloc)
		return ptr_t();
	return ptr_t(new ConditionConstant(get_childnum(), x));
}

TrafficFlowRestrictions::ConditionDepArrAirspace::ptr_t TrafficFlowRestrictions::ConditionDepArrAirspace::simplify_dest(const std::string& ident, const Point& coord) const
{
	if (!m_arr)
		return ptr_t();
	bool x(m_airspace && m_airspace->get_bbox().is_inside(coord) && m_airspace->get_poly().windingnumber(coord));
	if (x && m_refloc)
		return ptr_t();
	return ptr_t(new ConditionConstant(get_childnum(), x));
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::ConditionDepArrAirspace::evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const
{
	if (!m_airspace)
		return CondResult(false);
	const Fpl& fpl(tfrs.get_route());
	const FplWpt& wpt(m_arr ? fpl.back() : fpl.front());
	if (ifr_refloc && m_refloc && !wpt.is_ifr())
		return CondResult(false);
	const MultiPolygonHole& poly(m_airspace->get_poly());
	const Rect& bbox(m_airspace->get_bbox());
	// Rect bbox(poly.get_bbox());
	if (!wpt.is_ifr() && (!m_arr || !fpl[fpl.size() - 2U].is_ifr()))
		return CondResult(false);
	const Point& pt(wpt.get_coord());
	if (!bbox.is_inside(pt) || !poly.windingnumber(pt))
		return CondResult(false);
	CondResult r(true);
	r.get_vertexset().insert(m_arr ? fpl.size() - 1U : 0U);
	if (m_refloc)
		r.get_reflocset().insert(m_arr ? fpl.size() - 1U : 0U);
	return r;
}

TrafficFlowRestrictions::ConditionSidStar::ConditionSidStar(unsigned int childnum, const RuleWpt& airport, const std::string& procid, bool star, bool refloc)
	: Condition(childnum), m_airport(airport), m_procid(procid), m_star(star), m_refloc(refloc)
{
}

std::string TrafficFlowRestrictions::ConditionSidStar::to_str(void) const
{
	std::ostringstream oss;
	oss << (m_star ? "Star" : "Sid") << '{' << get_childnum()
	    << "}(" << m_airport.get_ident() << ',' << m_procid << ')';
	return oss.str();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionSidStar::clone(void) const
{
	return ptr_t(new ConditionSidStar(m_childnum, m_airport, m_procid, m_star, m_refloc));
}

TrafficFlowRestrictions::ConditionSidStar::ptr_t TrafficFlowRestrictions::ConditionSidStar::simplify_dep(const std::string& ident, const Point& coord) const
{
	if (m_star || ident == m_airport.get_ident())
		return ptr_t();
	return ptr_t(new ConditionConstant(get_childnum(), false));
}

TrafficFlowRestrictions::ConditionSidStar::ptr_t TrafficFlowRestrictions::ConditionSidStar::simplify_dest(const std::string& ident, const Point& coord) const
{
	if (!m_star || ident == m_airport.get_ident())
		return ptr_t();
	return ptr_t(new ConditionConstant(get_childnum(), false));
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::ConditionSidStar::evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const
{
	const Fpl& fpl(tfrs.get_route());
	if (fpl.size() < 2)
		return CondResult(false);
	const FplWpt& wpt(m_star ? fpl.back() : fpl.front());
	if (ifr_refloc && m_refloc && !wpt.is_ifr())
		return CondResult(false);
	if (m_airport.get_ident() != wpt.get_icao())
		return CondResult(false);
	CondResult r(true);
	if (m_star) {
		const FplWpt& wpt1(fpl[fpl.size() - 2U]);
		bool ok(wpt1.get_pathcode() == FPlanWaypoint::pathcode_star && wpt1.get_pathname() == m_procid);
		if (!ok && ((wpt1.get_pathcode() == FPlanWaypoint::pathcode_star && wpt1.get_pathname().empty()) ||
			    wpt1.get_pathcode() == FPlanWaypoint::pathcode_none || wpt1.get_pathcode() == FPlanWaypoint::pathcode_directto)) {
			std::string pterm(m_procid);
			std::string::size_type n(pterm.find_first_of("0123456789"));
			if (n != std::string::npos)
				pterm.erase(n);
			if (pterm.size() >= 2 && (pterm == wpt1.get_icao() || (wpt1.get_icao().empty() && pterm == wpt1.get_name())))
				ok = true;
		}
		if (!ok)
			return CondResult(false);
		r.get_vertexset().insert(fpl.size() - 1U);
		r.get_vertexset().insert(fpl.size() - 2U);
		if (m_refloc)
			r.get_reflocset().insert(fpl.size() - 1U);
	} else {
		bool ok(wpt.get_pathcode() == FPlanWaypoint::pathcode_sid && wpt.get_pathname() == m_procid);
		if (!ok && ((wpt.get_pathcode() == FPlanWaypoint::pathcode_sid && wpt.get_pathname().empty()) ||
			    wpt.get_pathcode() == FPlanWaypoint::pathcode_none || wpt.get_pathcode() == FPlanWaypoint::pathcode_directto)) {
			const FplWpt& wpt1(fpl[1U]);
			std::string pterm(m_procid);
			std::string::size_type n(pterm.find_first_of("0123456789"));
			if (n != std::string::npos)
				pterm.erase(n);
			if (pterm.size() >= 2 && (pterm == wpt1.get_icao() || (wpt1.get_icao().empty() && pterm == wpt1.get_name())))
				ok = true;
		}
		if (!ok)
			return CondResult(false);
		r.get_vertexset().insert(0U);
		if (m_refloc)
			r.get_reflocset().insert(0U);
	}
	return r;
}

TrafficFlowRestrictions::ConditionCrossingAirspaceActive::ConditionCrossingAirspaceActive(unsigned int childnum, TFRAirspace::const_ptr_t aspc)
	: Condition(childnum), m_airspace(aspc)
{
}

std::string TrafficFlowRestrictions::ConditionCrossingAirspaceActive::to_str(void) const
{
	std::ostringstream oss;
	oss << "AirspaceAct{" << get_childnum()
	    << "}(";
	if (m_airspace)
		oss << m_airspace->get_ident() << '/' << m_airspace->get_type();
	else
		oss << "null";
	oss << ')';
	return oss.str();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionCrossingAirspaceActive::clone(void) const
{
	return ptr_t(new ConditionCrossingAirspaceActive(m_childnum, m_airspace));
}

// FIXME: currently assume the airspace is always active
TrafficFlowRestrictions::ConditionCrossingAirspaceActive::ptr_t TrafficFlowRestrictions::ConditionCrossingAirspaceActive::simplify(void) const
{
	return ptr_t(new ConditionConstant(get_childnum(), true));
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::ConditionCrossingAirspaceActive::evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const
{
	return CondResult(true);
}

TrafficFlowRestrictions::ConditionCrossingAirwayAvailable::ConditionCrossingAirwayAvailable(unsigned int childnum, const AltRange& alt, const RuleWpt& wpt0, const RuleWpt& wpt1,
											    const std::string& awyname)
	: ConditionAltitude(childnum, alt), m_wpt0(wpt0), m_wpt1(wpt1), m_awyname(awyname)
{
}

std::string TrafficFlowRestrictions::ConditionCrossingAirwayAvailable::to_str(void) const
{
	std::ostringstream oss;
	oss << "AwyAvbl{" << get_childnum() << "}(" << to_altstr() << ','
	    << m_wpt0.get_ident() << ',' << m_wpt1.get_ident() << ',' << m_awyname << ')';
	return oss.str();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionCrossingAirwayAvailable::clone(void) const
{
	return ptr_t(new ConditionCrossingAirwayAvailable(m_childnum, m_alt, m_wpt0, m_wpt1, m_awyname));
}

// FIXME: currently assume the airway is not available
TrafficFlowRestrictions::ConditionCrossingAirwayAvailable::ptr_t TrafficFlowRestrictions::ConditionCrossingAirwayAvailable::simplify(void) const
{
	return ptr_t(new ConditionConstant(get_childnum(), false));
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::ConditionCrossingAirwayAvailable::evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const
{
	return CondResult(false);
}

TrafficFlowRestrictions::ConditionDctLimit::ConditionDctLimit(unsigned int childnum, double limit)
	: Condition(childnum), m_dctlimit(limit)
{
}

std::string TrafficFlowRestrictions::ConditionDctLimit::to_str(void) const
{
	std::ostringstream oss;
	oss << "DctLimit{" << get_childnum() << "}(" << m_dctlimit << ')';
	return oss.str();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionDctLimit::clone(void) const
{
	return ptr_t(new ConditionDctLimit(m_childnum, m_dctlimit));
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::ConditionDctLimit::evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const
{
	const Fpl& fpl(tfrs.get_route());
	CondResult r(false);
	for (CondResult::set_t::const_iterator ri(reflocset.begin()), re(reflocset.end()); ri != re; ++ri) {
		unsigned int wptnr(*ri);
		if (wptnr + 1U >= fpl.size())
			continue;
		const FplWpt& wpt0(fpl[wptnr]);
		const FplWpt& wpt1(fpl[wptnr + 1U]);
		if (!wpt0.is_ifr() || wpt0.get_pathcode() != FPlanWaypoint::pathcode_directto)
			continue;
		if (wpt0.get_coord().spheric_distance_nmi_dbl(wpt1.get_coord()) <= m_dctlimit)
			continue;
		r.set_result(true);
		r.get_edgeset().insert(wptnr);
	}
	return r;
}

TrafficFlowRestrictions::DctParameters::AltRange TrafficFlowRestrictions::ConditionDctLimit::evaluate_dct(TrafficFlowRestrictions& tfrs, const DctParameters& dct) const
{
	DctParameters::AltRange r(dct.get_alt());
	if (dct.get_dist() <= m_dctlimit)
		r.set_empty();
	return r;
}

TrafficFlowRestrictions::ConditionPropulsion::ConditionPropulsion(unsigned int childnum, kind_t kind, propulsion_t prop, unsigned int nrengines)
	: Condition(childnum), m_kind(kind), m_propulsion(prop), m_nrengines(nrengines)
{
}

std::string TrafficFlowRestrictions::ConditionPropulsion::to_str(void) const
{
	std::ostringstream oss;
	oss << "Propulsion{" << get_childnum() << "}(" << m_kind << ','
	    << m_propulsion << ',' << m_nrengines << ')';
	return oss.str();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionPropulsion::clone(void) const
{
	return ptr_t(new ConditionPropulsion(m_childnum, m_kind, m_propulsion, m_nrengines));
}

TrafficFlowRestrictions::ConditionPropulsion::ptr_t TrafficFlowRestrictions::ConditionPropulsion::simplify_aircraftclass(const std::string& acftclass) const
{
	bool val(true);	
	if (acftclass.size() < 1) {
		val = false;
	} else {
		switch (std::toupper(acftclass[0])) {
		case 'L':
			val = val && (m_kind == kind_any || m_kind == kind_landplane);
			break;

		case 'S':
			val = val && (m_kind == kind_any || m_kind == kind_seaplane);
			break;

		case 'A':
			val = val && (m_kind == kind_any || m_kind == kind_amphibian);
			break;

		case 'H':
			val = val && (m_kind == kind_any || m_kind == kind_helicopter);
			break;

		case 'G':
			val = val && (m_kind == kind_any || m_kind == kind_gyrocopter);
			break;

		case 'T':
			val = val && (m_kind == kind_any || m_kind == kind_tiltwing);
			break;

		default:
			val = val && (m_kind == kind_any);
			break;
		}
	}
	if (acftclass.size() < 3) {
		val = false;
	} else {
		switch (std::toupper(acftclass[2])) {
		case 'P':
			val = val && (m_propulsion == propulsion_piston);
			break;

		case 'T':
			val = val && (m_propulsion == propulsion_turboprop);
			break;

		case 'J':
			val = val && (m_propulsion == propulsion_jet);
			break;

		default:
			val = false;
			break;
		}
	}
	if (m_nrengines && (acftclass.size() < 2 || acftclass[1] != ('0' + m_nrengines)))
		val = false;
	return ptr_t(new ConditionConstant(get_childnum(), val));
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::ConditionPropulsion::evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const
{
	CondResult r(true);	
	if (tfrs.get_aircrafttypeclass().size() < 1) {
		r.set_result(false);
	} else {
		switch (std::toupper(tfrs.get_aircrafttypeclass()[0])) {
		case 'L':
			if (!(m_kind == kind_any || m_kind == kind_landplane))
				r.set_result(false);
			break;

		case 'S':
			if (!(m_kind == kind_any || m_kind == kind_seaplane))
				r.set_result(false);
			break;

		case 'A':
			if (!(m_kind == kind_any || m_kind == kind_amphibian))
				r.set_result(false);
			break;

		case 'H':
			if (!(m_kind == kind_any || m_kind == kind_helicopter))
				r.set_result(false);
			break;

		case 'G':
			if (!(m_kind == kind_any || m_kind == kind_gyrocopter))
				r.set_result(false);
			break;

		case 'T':
			if (!(m_kind == kind_any || m_kind == kind_tiltwing))
				r.set_result(false);
			break;

		default:
			if (!(m_kind == kind_any))
				r.set_result(false);
			break;
		}
	}
	if (tfrs.get_aircrafttypeclass().size() < 3) {
		r.set_result(false);
	} else {
		switch (std::toupper(tfrs.get_aircrafttypeclass()[2])) {
		case 'P':
			if (!(m_propulsion == propulsion_piston))
				r.set_result(false);
			break;

		case 'T':
			if (!(m_propulsion == propulsion_turboprop))
				r.set_result(false);
			break;

		case 'J':
			if (!(m_propulsion == propulsion_jet))
				r.set_result(false);
			break;

		default:
			r.set_result(false);
			break;
		}
	}
	if (m_nrengines && (tfrs.get_aircrafttypeclass().size() < 2 || tfrs.get_aircrafttypeclass()[1] != ('0' + m_nrengines)))
		r.set_result(false);
	return r;
}

TrafficFlowRestrictions::ConditionAircraftType::ConditionAircraftType(unsigned int childnum, const std::string& type)
	: Condition(childnum), m_type(type)
{
}

std::string TrafficFlowRestrictions::ConditionAircraftType::to_str(void) const
{
	std::ostringstream oss;
	oss << "Acft{" << get_childnum() << "}(" << m_type << ')';
	return oss.str();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionAircraftType::clone(void) const
{
	return ptr_t(new ConditionAircraftType(m_childnum, m_type));
}

TrafficFlowRestrictions::ConditionAircraftType::ptr_t TrafficFlowRestrictions::ConditionAircraftType::simplify_aircrafttype(const std::string& acfttype) const
{
	return ptr_t(new ConditionConstant(get_childnum(), acfttype == m_type));
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::ConditionAircraftType::evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const
{
	return CondResult(tfrs.get_aircrafttype() == m_type);
}

TrafficFlowRestrictions::ConditionEquipment::ConditionEquipment(unsigned int childnum, equipment_t e)
	: Condition(childnum), m_equipment(e)
{
}

std::string TrafficFlowRestrictions::ConditionEquipment::to_str(void) const
{
	std::ostringstream oss;
	oss << "Equipment{" << get_childnum() << "}(";
	if (m_equipment == equipment_invalid)
		oss << '?';
	else
		oss << (char)m_equipment;
	oss << ')';
	return oss.str();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionEquipment::clone(void) const
{
	return ptr_t(new ConditionEquipment(m_childnum, m_equipment));
}

TrafficFlowRestrictions::ConditionEquipment::ptr_t TrafficFlowRestrictions::ConditionEquipment::simplify_equipment(const std::string& equipment, Aircraft::pbn_t pbn) const
{
	bool val(false);
	switch (m_equipment) {
	default:
		val = false;
		break;

	case equipment_rnav:
		val = !!(pbn & Aircraft::pbn_rnav);
		break;

	case equipment_rvsm:
		val = equipment.find('W') != std::string::npos;
		break;

	case equipment_833:
		val = equipment.find('Y') != std::string::npos;
		break;

	case equipment_mls:
		val = equipment.find('K') != std::string::npos;
		break;

	case equipment_ils:
		val = equipment.find('L') != std::string::npos;
		break;
	}
	return ptr_t(new ConditionConstant(get_childnum(), val));	
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::ConditionEquipment::evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const
{
	CondResult r(false);
	switch (m_equipment) {
	default:
		r.set_result(false);
		break;

	case equipment_rnav:
		r.set_result(!!(tfrs.get_pbn() & Aircraft::pbn_rnav));
		break;

	case equipment_rvsm:
		r.set_result(tfrs.get_equipment().find('W') != std::string::npos);
		break;

	case equipment_833:
		r.set_result(tfrs.get_equipment().find('Y') != std::string::npos);
		break;

	case equipment_mls:
		r.set_result(tfrs.get_equipment().find('K') != std::string::npos);
		break;

	case equipment_ils:
		r.set_result(tfrs.get_equipment().find('L') != std::string::npos);
		break;
	}
	return r;	
}

TrafficFlowRestrictions::ConditionFlight::ConditionFlight(unsigned int childnum, flight_t f)
	: Condition(childnum), m_flight(f)
{
}

std::string TrafficFlowRestrictions::ConditionFlight::to_str(void) const
{
	std::ostringstream oss;
	oss << "Flight{" << get_childnum() << "}(" << m_flight << ')';
	return oss.str();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionFlight::clone(void) const
{
	return ptr_t(new ConditionFlight(m_childnum, m_flight));
}

TrafficFlowRestrictions::ConditionFlight::ptr_t TrafficFlowRestrictions::ConditionFlight::simplify_typeofflight(char type_of_flight) const
{
	bool val(false);
	type_of_flight = std::toupper(type_of_flight);
	switch (m_flight) {
	default:
		val = false;
		break;

	case flight_scheduled:
		val = type_of_flight == 'S';
		break;

	case flight_nonscheduled:
		val = type_of_flight == 'N';
		break;

	case flight_other:
		val = type_of_flight != 'S' && type_of_flight != 'N';
		break;
	}
	return ptr_t(new ConditionConstant(get_childnum(), val));	
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::ConditionFlight::evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const
{
	CondResult r(false);
	char type_of_flight(std::toupper(tfrs.get_typeofflight()));
	switch (m_flight) {
	default:
		r.set_result(false);
		break;

	case flight_scheduled:
		r.set_result(type_of_flight == 'S');
		break;

	case flight_nonscheduled:
		r.set_result(type_of_flight == 'N');
		break;

	case flight_other:
		r.set_result(type_of_flight != 'S' && type_of_flight != 'N');
		break;
	}
	return r;	
}

TrafficFlowRestrictions::ConditionCivMil::ConditionCivMil(unsigned int childnum, civmil_t cm)
	: Condition(childnum), m_civmil(cm)
{
}

std::string TrafficFlowRestrictions::ConditionCivMil::to_str(void) const
{
	std::ostringstream oss;
	oss << "Mil{" << get_childnum() << "}(" << (m_civmil == civmil_mil ? "Mil" : "Civ") << ')';
	return oss.str();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionCivMil::clone(void) const
{
	return ptr_t(new ConditionCivMil(m_childnum, m_civmil));
}

TrafficFlowRestrictions::ConditionCivMil::ptr_t TrafficFlowRestrictions::ConditionCivMil::simplify_mil(bool mil) const
{
	return ptr_t(new ConditionConstant(get_childnum(), m_civmil == (mil ? civmil_mil : civmil_civ)));	
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::ConditionCivMil::evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const
{
	return CondResult(m_civmil == civmil_civ);
}

TrafficFlowRestrictions::ConditionGeneralAviation::ConditionGeneralAviation(unsigned int childnum)
	: Condition(childnum)
{
}

std::string TrafficFlowRestrictions::ConditionGeneralAviation::to_str(void) const
{
	std::ostringstream oss;
	oss << "Gat{" << get_childnum() << "}()";
	return oss.str();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::ConditionGeneralAviation::clone(void) const
{
	return ptr_t(new ConditionGeneralAviation(m_childnum));
}

TrafficFlowRestrictions::ConditionGeneralAviation::ptr_t TrafficFlowRestrictions::ConditionGeneralAviation::simplify_gat(bool gat) const
{
	return ptr_t(new ConditionConstant(get_childnum(), gat));	
}

TrafficFlowRestrictions::CondResult TrafficFlowRestrictions::ConditionGeneralAviation::evaluate(TrafficFlowRestrictions& tfrs, const CondResult::set_t& reflocset) const
{
	return CondResult(true);
}
