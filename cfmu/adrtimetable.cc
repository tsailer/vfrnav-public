#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iomanip>
#include <sstream>

#include "adrtimetable.hh"
#include "adrhibernate.hh"
#include "adrunit.hh"

using namespace ADR;

const uint8_t TimePattern::daymask_sun;
const uint8_t TimePattern::daymask_mon;
const uint8_t TimePattern::daymask_tue;
const uint8_t TimePattern::daymask_wed;
const uint8_t TimePattern::daymask_thu;
const uint8_t TimePattern::daymask_fri;
const uint8_t TimePattern::daymask_sat;

TimePattern::TimePattern(void)
	: m_starttime(0), m_endtime(0), m_operator(operator_invalid), m_type(type_invalid), m_daymask(0)
{
}

TimePattern TimePattern::create_always(void)
{
	TimePattern t;
	t.set_operator(operator_set);
	t.set_type(type_weekday);
	t.set_starttime_min(0);
	t.set_endtime_min(24*60);
	t.set_daymask(daymask_all);
	return t;
}

TimePattern TimePattern::create_never(void)
{
	TimePattern t;
	t.set_operator(operator_set);
	t.set_type(type_weekday);
	t.set_starttime_min(0);
	t.set_endtime_min(24*60);
	t.set_daymask(0);
	return t;
}

const std::string& to_str(TimePattern::operator_t o)
{
	switch (o) {
	case TimePattern::operator_set:
	{
		static const std::string r("set");
		return r;
	}

	case TimePattern::operator_add:
	{
		static const std::string r("add");
		return r;
	}

	case TimePattern::operator_sub:
	{
		static const std::string r("sub");
		return r;
	}

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

const std::string& to_str(TimePattern::type_t t)
{
	switch (t) {
	case TimePattern::type_hol:
	{
		static const std::string r("hol");
		return r;
	}

	case TimePattern::type_busyfri:
	{
		static const std::string r("busyfri");
		return r;
	}

	case TimePattern::type_beforehol:
	{
		static const std::string r("beforehol");
		return r;
	}

	case TimePattern::type_afterhol:
	{
		static const std::string r("afterhol");
		return r;
	}

	case TimePattern::type_weekday:
	{
		static const std::string r("weekday");
		return r;
	}

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}


const std::string& TimePattern::get_wday_name(int wday)
{
	static const std::string daytbl[7] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
	if (wday < 0 || wday > 6) {
		static const std::string r("?");
		return r;
	}
	return daytbl[wday];
}

int TimePattern::get_wday_from_name(const std::string& wday)
{
	std::string wd(Glib::ustring(wday).uppercase());
	for (int w = 0; w < 7; ++w)
		if (get_wday_name(w) == wd)
			return w;
	return -1;
}

std::string TimePattern::get_daymask_string(uint8_t mask)
{
	mask &= 0x7f;
	if (!mask)
		return "";
	std::ostringstream oss;
	bool subseq(false);
	for (int wd = 0; wd < 7; ++wd) {
		if (!(mask & (1 << wd)))
			continue;
		if (subseq)
			oss << ' ';
		subseq = true;
		oss << get_wday_name(wd);
	}
	return oss.str();
}

std::ostream& TimePattern::print(std::ostream& os) const
{
	os << get_operator();
	{
		uint16_t t(get_starttime_min());
		uint16_t min(t);
		uint16_t hour(t / 60);
		min -= hour * 60;
		std::ostringstream oss;
		oss << std::setw(2) << std::setfill('0') << hour << ':'
		    << std::setw(2) << std::setfill('0') << min;
		os << ' ' << oss.str();
	}
	{
		uint16_t t(get_endtime_min());
		uint16_t min(t);
		uint16_t hour(t / 60);
		min -= hour * 60;
		std::ostringstream oss;
		oss << std::setw(2) << std::setfill('0') << hour << ':'
		    << std::setw(2) << std::setfill('0') << min;
		os << "..." << oss.str();
	}
	os << ' ' << get_type();
	if (get_type() != type_weekday)
		return os;
	{
		std::string x(get_daymask_string());
		if (!x.empty())
			os << ' ' << x;
	}
	return os;
}

bool TimePattern::is_inside(const TimeTableEval& tte) const
{
	{
		unsigned int d(tte.get_daytime());
		unsigned int s(get_starttime());
		unsigned int e(get_endtime());
		if (d < s)
			d += 24*60*60;
		if (e <= s)
			e += 24*60*60;
		if (d < s || d > e)
			return false;
	}
	switch (get_type()) {
	default:
		return false;

	case type_hol:
		return tte.is_hol();

	case type_busyfri:
		return tte.is_busyfri();

	case type_beforehol:
		return tte.is_beforehol();

	case type_afterhol:
		return tte.is_afterhol();

	case type_weekday:
		return is_wday(tte.get_wday());
	}
}

bool TimePattern::is_always(void) const
{
	if (get_type() != type_weekday || (daymask_all & ~get_daymask()))
		return false;
	unsigned int s(get_starttime());
	unsigned int e(get_endtime());
	if (e <= s)
		e += 24*60*60;
	return (e - s) >= 24*60*60;
}

bool TimePattern::is_never(void) const
{
	return get_type() == type_weekday && !(daymask_all & get_daymask());
}

TimeTableElement::TimeTableElement(void)
	: m_start(0), m_end(std::numeric_limits<timetype_t>::max()), m_exclude(false)
{
}

TimeTableElement TimeTableElement::create_always(void)
{
	TimeTableElement t;
	t.push_back(TimePattern::create_always());
	return t;
}

TimeTableElement TimeTableElement::create_never(void)
{
	TimeTableElement t;
	t.push_back(TimePattern::create_never());
	return t;
}

void TimeTableElement::never_if_empty(void)
{
	if (empty())
		return;
	TimePattern tp;
	tp.set_operator(TimePattern::operator_set);
	tp.set_type(TimePattern::type_weekday);
	tp.set_starttime_min(0);
	tp.set_endtime_min(0);
	tp.set_daymask(0);
	push_back(tp);
}

void TimeTableElement::kill_never(void)
{
	if (empty())
		return;
	for (size_type i(0), n(size()); i < n;) {
		if (!operator[](i).is_never() || operator[](i).get_operator() == TimePattern::operator_set) {
			++i;
			continue;
		}
		erase(begin() + i);
		n = size();
	}
	never_if_empty();
}

void TimeTableElement::kill_invalid(void)
{
	if (empty())
		return;
	for (size_type i(0), n(size()); i < n;) {
		if (!operator[](i).get_operator() < TimePattern::operator_invalid) {
			++i;
			continue;
		}
		erase(begin() + i);
		n = size();
	}
	never_if_empty();
}

void TimeTableElement::kill_leadingsub(void)
{
	if (empty())
		return;
	while (!empty()) {
		if (front().get_operator() != TimePattern::operator_sub) {
			front().set_operator(TimePattern::operator_set);
			return;
		}
		erase(begin());
	}
	never_if_empty();
}

void TimeTableElement::simplify_set(void)
{
	if (empty())
		return;
	for (size_type i(1), n(size()); i < n;) {
		if (operator[](i).get_operator() != TimePattern::operator_set) {
			++i;
			continue;
		}
		erase(begin() + i);
		n = size();
	}
	never_if_empty();
}

void TimeTableElement::simplify_always(void)
{
	if (empty())
		return;
	for (size_type n(size()), i(n); i > 0;) {
		--i;
		if (!operator[](i).is_always())
			continue;
		if (operator[](i).get_operator() == TimePattern::operator_sub) {
			erase(begin(), begin() + (i + 1));
			break;
		}
		erase(begin(), begin() + i);
		break;
	}
	kill_leadingsub();
	never_if_empty();
}

void TimeTableElement::simplify(void)
{
	kill_invalid();
	kill_never();
	kill_leadingsub();
	simplify_set();
	simplify_always();
}

void TimeTableElement::limit(timetype_t s, timetype_t e)
{
	m_start = std::max(m_start, s);
	m_end = std::min(m_end, e);
}

std::ostream& TimeTableElement::print(std::ostream& os, unsigned int indent) const
{
	if (true) {
		struct tm tmstart, tmend;
		time_t tstart(get_start()), tend(get_end());
		gmtime_r(&tstart, &tmstart);
		gmtime_r(&tend, &tmend);
		os << std::setw(4) << std::setfill('0') << (tmstart.tm_year + 1900) << '-'
		   << std::setw(2) << std::setfill('0') << (tmstart.tm_mon + 1) << '-'
		   << std::setw(2) << std::setfill('0') << tmstart.tm_mday << "..."
		   << std::setw(4) << std::setfill('0') << (tmend.tm_year + 1900) << '-'
		   << std::setw(2) << std::setfill('0') << (tmend.tm_mon + 1) << '-'
		   << std::setw(2) << std::setfill('0') << tmend.tm_mday;
	} else {
		os << get_start() << '-' << get_end();
	}
	os << " exclude " << (is_exclude() ? "yes" : "no");
	for (unsigned int i(0), n(size()); i < n; ++i)
		this->operator[](i).print(os << std::endl << std::string(indent, ' '));
	return os;
}

bool TimeTableElement::is_inside(const TimeTableEval& tte) const
{
	if (tte.get_time() < get_start() || tte.get_time() >= get_end())
		return false;
	bool val(empty());
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		switch (i->get_operator()) {
		case TimePattern::operator_set:
			val = i->is_inside(tte);
			break;

		case TimePattern::operator_add:
			val = val || i->is_inside(tte);
			break;

		case TimePattern::operator_sub:
			val = val && !i->is_inside(tte);
			break;

		default:
			break;
		}
	}
	return val;
}

timeset_t TimeTableElement::timediscontinuities(void) const
{
	timeset_t r;
	r.insert(get_start());
	r.insert(get_end());
	return r;
}

bool TimeTableElement::is_always(void) const
{
	switch (size()) {
	case 0:
		return true;

	case 1:
		return (!is_exclude() && front().is_always()) || (is_exclude() && front().is_never());

	default:
		return false;
	}
}

bool TimeTableElement::is_never(void) const
{
	switch (size()) {
	case 1:
		return (is_exclude() && front().is_always()) || (!is_exclude() && front().is_never());

	default:
		return false;
	}
}

bool TimeTableElement::convert(TimeTableWeekdayPattern& x) const
{
	x.set_start(get_start());
	x.set_end(get_end());
	if (empty()) {
		for (unsigned int wd = 0; wd < 7; ++wd)
			x[wd] = TimeTableWeekdayPattern::daytimeset_t::Intvl(0, 24*60);
		return true;
	}
	for (unsigned int wd = 0; wd < 7; ++wd)
		x[wd].set_empty();
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		if (i->get_type() != TimePattern::type_weekday)
			return false;
		uint16_t st(i->get_starttime_min());
		uint16_t et(i->get_endtime_min());
		if (et <= st)
			et += 24 * 60;
		switch (i->get_operator()) {
		case TimePattern::operator_set:
			for (unsigned int wd = 0; wd < 7; ++wd)
				if (i->is_wday(wd))
					x[wd] = TimeTableWeekdayPattern::daytimeset_t::Intvl(st, et);
			break;

		case TimePattern::operator_add:
			for (unsigned int wd = 0; wd < 7; ++wd)
				if (i->is_wday(wd))
					x[wd] |= TimeTableWeekdayPattern::daytimeset_t::Intvl(st, et);
			break;

		case TimePattern::operator_sub:
			for (unsigned int wd = 0; wd < 7; ++wd)
				if (i->is_wday(wd))
					x[wd] -= TimeTableWeekdayPattern::daytimeset_t::Intvl(st, et);
			break;

		default:
			break;
		}
	}
	return true;
}

TimeTable::TimeTable(void)
{
}

TimeTable TimeTable::create_always(void)
{
	TimeTable t;
	t.push_back(TimeTableElement::create_always());
	return t;
}

TimeTable TimeTable::create_never(void)
{
	TimeTable t;
	t.push_back(TimeTableElement::create_never());
	return t;
}

void TimeTable::simplify(void)
{
	for (size_type i(0), n(size()); i < n;) {
		{
			const TimeTableElement& tte(operator[](i));
			if (tte.get_end() <= tte.get_start()) {
				erase(begin() + i);
				n = size();
				continue;
			}
		}
		for (size_type j(i+1); j < n;) {
			TimeTableElement& tte0(operator[](i));
			TimeTableElement& tte1(operator[](j));
			if (tte0.get_start() != tte1.get_start() ||
			    tte0.get_end() != tte1.get_end() ||
			    tte0.is_exclude() != tte1.is_exclude()) {
				++j;
				continue;
			}
			if (!tte1.empty()) {
				if (tte1.front().get_operator() == TimePattern::operator_set)
					tte1.front().set_operator(TimePattern::operator_add);
				tte0.insert(tte0.end(), tte1.begin(), tte1.end());
			}
			erase(begin() + j);
			n = size();
		}
		++i;
	}
}

void TimeTable::limit(timetype_t s, timetype_t e)
{
	if (empty()) {
		if (e <= s)
			return;
		TimeTableElement tte;
		tte.set_start(s);
		tte.set_end(e);
		tte.set_exclude(false);
		TimePattern tp;
		tp.set_operator(TimePattern::operator_set);
		tp.set_type(TimePattern::type_weekday);
		tp.set_starttime_min(0);
		tp.set_endtime_min(24*60);
		tp.set_daymask(TimePattern::daymask_all);
		tte.push_back(tp);
		push_back(tte);
		return;
	}
	for (size_type i(0), n(size()); i < n;) {
		TimeTableElement& tte(operator[](i));
		tte.limit(s, e);
		if (tte.get_end() <= tte.get_start()) {
			erase(begin() + i);
			n = size();
			continue;
		}
		++i;
	}
}

void TimeTable::invert(void)
{
	for (iterator i(begin()), e(end()); i != e; ++i)
		i->set_exclude(!i->is_exclude());
}

std::ostream& TimeTable::print(std::ostream& os, unsigned int indent) const
{
	for (unsigned int i(0), n(size()); i < n; ++i)
		this->operator[](i).print(os << std::endl << std::string(indent, ' ') << "TimeTable[" << i << "] ", indent + 2);
	return os;
}

bool TimeTable::is_inside(const TimeTableEval& tte) const
{
	bool val(empty() || front().is_exclude());
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		if (i->is_exclude())
			val = val && !(i->is_inside(tte));
		else
			val = val || i->is_inside(tte);
	}
	return val;
}

timeset_t TimeTable::timediscontinuities(void) const
{
	timeset_t r;
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		timeset_t r1(i->timediscontinuities());
		r.insert(r1.begin(), r1.end());
	}
	return r;
}

timepair_t TimeTable::timebounds(void) const
{
	timepair_t r(std::numeric_limits<timetype_t>::max(),
		     std::numeric_limits<timetype_t>::min());
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		if (i->get_start() >= i->get_end())
			continue;
		r.first = std::min(r.first, i->get_start());
		r.second = std::max(r.second, i->get_end());
	}
	return r;
}

bool TimeTable::is_never(void) const
{
	switch (size()) {
	case 1:
		return front().is_never();

	default:
		return false;
	}
}

bool TimeTable::convert(TimeTableWeekdayPattern& x) const
{
	if (empty() || front().is_exclude()) {
		for (unsigned int wd = 0; wd < 7; ++wd)
			x[wd] = TimeTableWeekdayPattern::daytimeset_t::Intvl(0, 24*60);
	} else {
		for (unsigned int wd = 0; wd < 7; ++wd)
			x[wd].set_empty();
	}
	if (empty()) {
		x.set_start(0);
		x.set_end(std::numeric_limits<timetype_t>::max());
		return true;
	}
	x.set_start(front().get_start());
	x.set_end(front().get_end());
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		TimeTableWeekdayPattern y;
		if (!i->convert(y))
			return false;
		if (i->is_exclude()) {
			for (unsigned int wd = 0; wd < 7; ++wd)
				x[wd] -= y[wd];
		} else {
			for (unsigned int wd = 0; wd < 7; ++wd)
				x[wd] |= y[wd];
		}
	}
	return true;
}

const TimeTableWeekdayPattern::daytimeset_t TimeTableWeekdayPattern::fullday(daytimeset_t::Intvl(0, 24*60));

TimeTableWeekdayPattern::TimeTableWeekdayPattern(timetype_t st, timetype_t en)
	: m_start(st), m_end(en)
{
}

TimeTableElement TimeTableWeekdayPattern::get_timetableelement(void) const
{
	TimeTableElement tte;
	tte.set_start(get_start());
	tte.set_end(get_end());
	tte.set_exclude(false);
	uint8_t tmask(0x7f);
	for (unsigned int wd = 0; wd < 7; ++wd) {
		uint8_t mask(1 << wd);
		if (!(mask & tmask))
			continue;
		uint8_t cmask(mask);
		const daytimeset_t& dts(operator[](wd));
		for (unsigned int wd1 = wd + 1; wd1 < 7; ++wd1) {
			uint8_t mask1(1 << wd1);
			if (!(mask1 & tmask))
				continue;
			if (dts != operator[](wd1))
				continue;
			cmask |= mask1;
		}
		tmask &= ~cmask;
		for (daytimeset_t::const_iterator dtsi(dts.begin()), dtse(dts.end()); dtsi != dtse; ++dtsi) {
			TimePattern tp;
			tp.set_operator(TimePattern::operator_add);
			tp.set_type(TimePattern::type_weekday);
			tp.set_starttime_min(dtsi->get_lower());
			tp.set_endtime_min(dtsi->get_upper());
			tp.set_daymask(cmask);
			tte.push_back(tp);
		}
	}
	if (tte.empty()) {
		TimePattern tp;
		tp.set_operator(TimePattern::operator_set);
		tp.set_type(TimePattern::type_weekday);
		tp.set_starttime_min(0);
		tp.set_endtime_min(0);
		tp.set_daymask(0);
		tte.push_back(tp);
	}
	tte.front().set_operator(TimePattern::operator_set);
	return tte;
}

TimeTable TimeTableWeekdayPattern::get_timetable(void) const
{
	TimeTable tt;
	tt.push_back(get_timetableelement());
	return tt;
}

bool TimeTableWeekdayPattern::operator==(const TimeTableWeekdayPattern& x) const
{
	return startend_eq(x) && daytime_eq(x);
}

bool TimeTableWeekdayPattern::startend_eq(const TimeTableWeekdayPattern& x) const
{
	return get_start() == x.get_start() && get_end() == x.get_end();
}

bool TimeTableWeekdayPattern::daytime_eq(const TimeTableWeekdayPattern& x) const
{
	for (unsigned int wd = 0; wd < 7; ++wd)
		if (operator[](wd) != x[wd])
			return false;
	return true;
}

bool TimeTableWeekdayPattern::merge_adjacent(const TimeTableWeekdayPattern& x)
{
	if (get_start() >= get_end())
		return false;
	if (x.get_start() >= x.get_end())
		return false;
	if (get_start() > x.get_end() || x.get_start() > get_end())
		return false;
	if (!daytime_eq(x))
		return false;
	set_start(std::min(get_start(), x.get_start()));
	set_end(std::max(get_end(), x.get_end()));
	return true;
}

bool TimeTableWeekdayPattern::merge_sametime(const TimeTableWeekdayPattern& x)
{
	if (get_start() >= get_end())
		return false;
	if (!startend_eq(x))
		return false;
	for (unsigned int wd = 0; wd < 7; ++wd)
		operator[](wd) |= x[wd];
	return true;
}

bool TimeTableWeekdayPattern::is_never(void) const
{
	for (unsigned int i = 0; i < 7; ++i)
		if (!operator[](i).is_empty())
			return false;
	return true;
}

bool TimeTableWeekdayPattern::is_h24(void) const
{
	for (unsigned int i = 0; i < 7; ++i) {
		const daytimeset_t& dts(operator[](i));
		if (dts.is_empty())
			return false;
		const daytimeset_t::Intvl& di(*dts.begin());
		if (di.get_lower() > 0 || di.get_upper() < 24*60)
			return false;
	}
	return true;
}

TimeTableWeekdayPattern& TimeTableWeekdayPattern::invert(void)
{
	for (unsigned int i = 0; i < 7; ++i) {
		daytimeset_t& dts(operator[](i));
		dts = fullday & ~dts;
	}
	return *this;
}

TimeTableWeekdayPattern& TimeTableWeekdayPattern::operator&=(const TimeTableWeekdayPattern& t)
{
	if (!startend_eq(t))
		throw std::runtime_error("TimeTableWeekdayPattern::operator&=: start/end time not same");
	for (unsigned int i = 0; i < 7; ++i)
		operator[](i) &= t[i];
	return *this;
}

TimeTableWeekdayPattern& TimeTableWeekdayPattern::operator|=(const TimeTableWeekdayPattern& t)
{
	if (!startend_eq(t))
		throw std::runtime_error("TimeTableWeekdayPattern::operator|=: start/end time not same");
	for (unsigned int i = 0; i < 7; ++i)
		operator[](i) |= t[i];
	return *this;
}

void TimeTableWeekdayPattern::set_never(void)
{
	for (unsigned int i = 0; i < 7; ++i)
		operator[](i).set_empty();
}

void TimeTableWeekdayPattern::set_h24(void)
{
	for (unsigned int i = 0; i < 7; ++i)
		operator[](i) = fullday;
}

std::string TimeTableWeekdayPattern::to_str(void) const
{
	std::ostringstream oss;
	if (true) {
		struct tm tmstart, tmend;
		time_t tstart(get_start()), tend(get_end());
		gmtime_r(&tstart, &tmstart);
		gmtime_r(&tend, &tmend);
		oss << std::setw(4) << std::setfill('0') << (tmstart.tm_year + 1900) << '-'
		    << std::setw(2) << std::setfill('0') << (tmstart.tm_mon + 1) << '-'
		    << std::setw(2) << std::setfill('0') << tmstart.tm_mday << "..."
		    << std::setw(4) << std::setfill('0') << (tmend.tm_year + 1900) << '-'
		    << std::setw(2) << std::setfill('0') << (tmend.tm_mon + 1) << '-'
		    << std::setw(2) << std::setfill('0') << tmend.tm_mday;
	} else {
		oss << get_start() << '-' << get_end();
	}
	uint8_t tmask(0x7f);
	for (unsigned int wd = 0; wd < 7; ++wd) {
		uint8_t mask(1 << wd);
		if (!(mask & tmask))
			continue;
		oss << ' ' << TimePattern::get_wday_name(wd);
		uint8_t cmask(mask);
		const daytimeset_t& dts(operator[](wd));
		for (unsigned int wd1 = wd + 1; wd1 < 7; ++wd1) {
			uint8_t mask1(1 << wd1);
			if (!(mask1 & tmask))
				continue;
			if (dts != operator[](wd1))
				continue;
			cmask |= mask1;
			oss << ' ' << TimePattern::get_wday_name(wd1);
		}
		tmask &= ~cmask;
		oss << ':' << dts.to_str();
	}
	return oss.str();
}

TimeTableAnd::TimeTableAnd()
{
}

void TimeTableAnd::simplify(void)
{
	if (false)
		print(std::cerr << "TimeTableAnd::simplify: entry", 2) << std::endl;
	// first pass: consolidate weekday patterns
	{
		TimeTableAnd z;
		for (;;) {
			bool subseq(false);
			TimeTableWeekdayPattern x;
			for (iterator i(begin()), e(end()); i != e;) {
				if (!subseq) {
					subseq = i->convert(x);
					if (!subseq) {
						++i;
						continue;
					}
					i = erase(i);
					e = end();
					continue;
				}
				TimeTableWeekdayPattern y;
				if (!i->convert(y) || y.get_start() != x.get_start() || y.get_end() != x.get_end()) {
					++i;
					continue;
				}
				for (unsigned int wd = 0; wd < 7; ++wd)
					x[wd] &= y[wd];
				i = erase(i);
				e = end();
			}
			if (!subseq)
				break;
			z.push_back(x.get_timetable());
		}
		insert(begin(), z.begin(), z.end());
	}
	// non-weekday pattern simplifications
	for (iterator i(begin()), e(end()); i != e; ++i)
		i->simplify();
	timepair_t b(timebounds());
	for (size_type i(0), n(size()); i < n; ) {
		if (operator[](i).size() != 1 || !operator[](i).front().is_always()) {
			++i;
			continue;
		}
		if (n < 2)
			break;
		erase(begin() + i);
		n = size();
	}
	if (false)
		print(std::cerr << "TimeTableAnd::simplify: exit", 2) << std::endl;
}

void TimeTableAnd::limit(timetype_t s, timetype_t e)
{
	for (iterator i(begin()), en(end()); i != en; ++i)
		i->limit(s, e);
}

std::ostream& TimeTableAnd::print(std::ostream& os, unsigned int indent) const
{
	for (unsigned int i(0), n(size()); i < n; ++i)
		this->operator[](i).print(os << std::endl << std::string(indent, ' ') << "TimeTableAnd[" << i << "] ", indent + 2);
	return os;
}

bool TimeTableAnd::is_inside(const TimeTableEval& tte) const
{
	bool val(true);
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		val = val && i->is_inside(tte);
	return val;
}

timeset_t TimeTableAnd::timediscontinuities(void) const
{
	timeset_t r;
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		timeset_t r1(i->timediscontinuities());
		r.insert(r1.begin(), r1.end());
	}
	return r;
}

timepair_t TimeTableAnd::timebounds(void) const
{
	timepair_t r(std::numeric_limits<timetype_t>::max(),
		     std::numeric_limits<timetype_t>::min());
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		timepair_t r1(i->timebounds());
		if (r1.first >= r1.second)
			continue;
		r.first = std::min(r.first, r1.first);
		r.second = std::max(r.second, r1.second);
	}
	return r;
}

bool TimeTableAnd::is_never(void) const
{
	if (empty())
		return true;
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		if (i->is_never())
			return true;
	return false;
}

bool TimeTableAnd::convert(TimeTableWeekdayPattern& x) const
{
	if (empty()) {
		x = TimeTableWeekdayPattern();
		return true;
	}
	bool first(true);
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		if (i->is_never()) {
			x = TimeTableWeekdayPattern();
			return true;
		}
		if (first) {
			if (!i->convert(x))
				return false;
			first = false;
			continue;
		}
		TimeTableWeekdayPattern y;
		if (!i->convert(y) ||
		    x.get_start() != y.get_start() || x.get_end() != y.get_end())
			return false;
		x &= y;
	}
	return !first;
}

TimeTableOr::TimeTableOr()
{
}

TimeTableOr TimeTableOr::create_always(void)
{
	return TimeTableOr();
}


TimeTableOr TimeTableOr::create_never(void)
{
	TimeTableOr t;
	t.push_back(TimeTableAnd());
	return t;
}

void TimeTableOr::simplify(bool merge_adj)
{
	if (false)
		print(std::cerr << "TimeTableOr::simplify: enter"
		      << (merge_adj ? " (merge adjacent)" : ""), 2) << std::endl;
	if (empty())
		return;
	for (iterator i(begin()), e(end()); i != e;) {
		i->simplify();
		if (!i->is_never()) {
			++i;
			continue;
		}
		i = erase(i);
		e = end();
	}
	if (empty()) {
		push_back(TimeTableAnd());
		return;
	}
	if (false) {
		for (size_type i(0), n(size()); i < n; ++i) {
			std::cerr << "TimeTableOr[" << i << "]: size " << operator[](i).size() << std::endl;
			if (operator[](i).size() != 1)
				continue;
			TimeTableWeekdayPattern x;
			bool xok(operator[](i).front().convert(x));
			std::cerr << "TimeTableOr[" << i << "][0]: wdpattern " << (xok ? 'Y' : 'N');
			if (xok)
				std::cerr << ' ' << x.to_str();
			std::cerr << std::endl;
		}
	}
	typedef std::vector<TimeTableWeekdayPattern> wdp_t;
	wdp_t wdp;
	for (iterator i(begin()), e(end()); i != e;) {
		TimeTableWeekdayPattern x;
		if (!i->convert(x)) {
			++i;
			continue;
		}
		if (x.get_start() < x.get_end())
			wdp.push_back(x);
		i = erase(i);
		e = end();
	}
	if (empty() && wdp.empty()) {
		push_back(TimeTableAnd());
		return;
	}
	for (wdp_t::size_type i0(0), n(wdp.size()); i0 != n; ++i0) {
		for (wdp_t::size_type i1(i0 + 1); i1 != n; ) {
			if (!wdp[i0].merge_sametime(wdp[i1])) {
				++i1;
				continue;
			}
			wdp.erase(wdp.begin() + i1);
			n = wdp.size();
		}
	}
	if (merge_adj) {
		for (wdp_t::size_type i0(0), n(wdp.size()); i0 != n; ++i0) {
			for (wdp_t::size_type i1(i0 + 1); i1 != n; ) {
				if (!wdp[i0].merge_adjacent(wdp[i1])) {
					++i1;
					continue;
				}
				wdp.erase(wdp.begin() + i1);
				n = wdp.size();
			}
		}
	}
	for (wdp_t::const_iterator i(wdp.begin()), e(wdp.end()); i != e; ++i) {
		push_back(TimeTableAnd());
		back().push_back(i->get_timetable());
	}
	if (false)
		print(std::cerr << "TimeTableOr::simplify: exit", 2) << std::endl;
}

void TimeTableOr::limit(timetype_t s, timetype_t e)
{
	if (empty()) {
		TimeTableAnd t;
		t.push_back(TimeTable::create_always());
		push_back(t);
	}
	for (iterator i(begin()), en(end()); i != en; ++i)
		i->limit(s, e);
}

void TimeTableOr::split(const timeset_t& s)
{
	for (iterator i(begin()), e(end()); i != e;) {
		timepair_t b(i->timebounds());
		iterator i0(i);
		++i;
		timeset_t::const_iterator si(s.upper_bound(b.first));
		if (si == s.end() || *si >= b.second)
			continue;
		i = insert(i, *i0);
		e = end();
		i0 = i;
		--i0;
		i0->limit(b.first, *si);
		i->limit(*si, b.second);
	}
}

bool TimeTableOr::is_always(void) const
{
	return empty();
}

bool TimeTableOr::is_never(void) const
{
	if (empty())
		return false;
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		if (!i->is_never())
			return false;
	return true;
}

bool TimeTableOr::convert(TimeTableWeekdayPattern& x) const
{
	if (empty()) {
		x = TimeTableWeekdayPattern();
		x.set_h24();
		return true;
	}
	bool first(true);
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		if (i->is_never())
			continue;
		if (first) {
			if (!i->convert(x))
				return false;
			first = false;
			continue;
		}
		TimeTableWeekdayPattern y;
		if (!i->convert(y) ||
		    x.get_start() != y.get_start() || x.get_end() != y.get_end())
			return false;
		x |= y;
	}
	return !first;
}

TimeTableOr& TimeTableOr::invert(void)
{
	if (empty()) {
		push_back(TimeTableAnd());
		return *this;
	}
	for (iterator i(begin()), e(end()); i != e;) {
		if (!i->is_never()) {
			++i;
			continue;
		}
		i = erase(i);
		e = end();
	}
	if (empty())
		return *this;
	// !(a0*a1+b0*b1) = !(a0*a1)*!(b0*b1) = (!a0+!a1)*(!b0+!b1) = !a0*!b0+!a0*!b1+!a1*!b0+!a1*!b1
	typedef std::vector<TimeTableAnd::size_type> cntrs_t;
	cntrs_t cntrs(size(), 0);
	TimeTableOr t;
	for (;;) {
		cntrs_t::size_type ic, nc(cntrs.size());
		// product of inverted elements of each term
		t.push_back(TimeTableAnd());
		for (ic = 0; ic < nc; ++ic) {
			t.back().push_back(operator[](ic)[cntrs[ic]]);
			t.back().back().invert();
		}
		// increment counter
		for (ic = 0; ic < nc; ++ic) {
			++cntrs[ic];
			if (cntrs[ic] < operator[](ic).size())
				break;
			cntrs[ic] = 0;
		}
		if (ic >= nc)
			break;
	}
	swap(t);
	return *this;
}

TimeTableOr& TimeTableOr::operator&=(const TimeTableOr& tt)
{
	if (tt.empty())
		return *this;
	if (empty()) {
		*this = tt;
		return *this;
	}
	TimeTableOr t;
	for (const_iterator i0(begin()), e0(end()); i0 != e0; ++i0) {
		if (i0->is_never())
			continue;
		for (const_iterator i1(tt.begin()), e1(tt.end()); i1 != e1; ++i1) {
			if (i1->is_never())
				continue;
			t.push_back(*i0);
			t.back().insert(t.back().end(), i1->begin(), i1->end());
		}
	}
	swap(t);
	if (empty())
		push_back(TimeTableAnd());		
	return *this;
}

TimeTableOr& TimeTableOr::operator|=(const TimeTableOr& tt)
{
	if (empty() || tt.empty()) {
		clear();
		return *this;
	}
	insert(end(), tt.begin(), tt.end());
	return *this;
}

TimeTableOr& TimeTableOr::operator&=(const TimeTable& tt)
{
	if (tt.is_never()) {
		clear();
		push_back(TimeTableAnd());
		return *this;
	}
	if (empty()) {
		push_back(TimeTableAnd());
		back().push_back(tt);
		return *this;
	}
	for (iterator i(begin()), e(end()); i != e; ++i) {
		if (i->is_never())
			continue;
		i->push_back(tt);
	}
	if (empty())
		push_back(TimeTableAnd());
	return *this;
}

TimeTableOr& TimeTableOr::operator|=(const TimeTable& tt)
{
	if (empty() || tt.is_never())
		return *this;
	push_back(TimeTableAnd());
	back().push_back(tt);
	return *this;
}

std::ostream& TimeTableOr::print(std::ostream& os, unsigned int indent) const
{
	for (unsigned int i(0), n(size()); i < n; ++i)
		this->operator[](i).print(os << std::endl << std::string(indent, ' ')
					  << "TimeTableOr[" << i << "] ", indent + 2);
	return os;
}

bool TimeTableOr::is_inside(const TimeTableEval& tte) const
{
	bool val(empty());
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		val = val || i->is_inside(tte);
	return val;
}

timeset_t TimeTableOr::timediscontinuities(void) const
{
	timeset_t r;
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		timeset_t r1(i->timediscontinuities());
		r.insert(r1.begin(), r1.end());
	}
	return r;
}

TimeTableEval::TimeTableEval(uint64_t tm, const Point& pt, const TimeTableSpecialDateEval& ttsde)
	: m_specialeval(ttsde), m_pt(pt), m_time(tm), m_special(0)
{
	time_t t(m_time);
	gmtime_r(&t, &m_tm);
}

bool TimeTableEval::is_busyfri(void) const
{
	if (!(m_special & 0x02)) {
		if (m_specialeval.is_specialday(*this, SpecialDateTimeSlice::type_busyfri))
			m_special |= 0x01;
		m_special |= 0x02;
	}
	return !!(m_special & 0x01);
}

bool TimeTableEval::is_hol(void) const
{
	if (!(m_special & 0x08)) {
		if (m_specialeval.is_specialday(*this, SpecialDateTimeSlice::type_hol))
			m_special |= 0x04;
		m_special |= 0x08;
	}
	return !!(m_special & 0x04);
}

bool TimeTableEval::is_beforehol(void) const
{
	if (!(m_special & 0x20)) {
		TimeTableEval tte(get_time() + (24 * 60 * 60), get_point(), m_specialeval);
		if (m_specialeval.is_specialday(tte, SpecialDateTimeSlice::type_hol))
			m_special |= 0x10;
		m_special |= 0x20;
	}
	return !!(m_special & 0x10);
}

bool TimeTableEval::is_afterhol(void) const
{
	if (!(m_special & 0x80)) {
		TimeTableEval tte(get_time() - (24 * 60 * 60), get_point(), m_specialeval);
		if (m_specialeval.is_specialday(tte, SpecialDateTimeSlice::type_hol))
			m_special |= 0x40;
		m_special |= 0x80;
	}
	return !!(m_special & 0x40);
}

std::string TimeTableEval::to_str(void) const
{
	std::ostringstream oss;
	oss << TimePattern::get_wday_name(get_wday()) << ' '
	    << std::setw(4) << std::setfill('0') << get_year() << '-'
	    << std::setw(2) << std::setfill('0') << get_mon() << '-'
	    << std::setw(2) << std::setfill('0') << get_mday() << ' '
	    << std::setw(2) << std::setfill('0') << get_hour() << ':'
	    << std::setw(2) << std::setfill('0') << get_min() << ':'
	    << std::setw(2) << std::setfill('0') << get_sec();
	return oss.str();
}

PointPair::PointPair(const Link& pt0, const Link& pt1)
{
	m_pt[0] = pt0;
	m_pt[1] = pt1;
}

const UUID& PointPair::get_uuid(unsigned int index) const
{
	const Link& l(get_point(index));
	if (!l.get_obj())
		return UUID::niluuid;
	return l.get_obj()->get_uuid();
}

std::pair<double,double> PointPair::dist(void) const
{
	std::pair<double,double> r(std::numeric_limits<double>::max(), std::numeric_limits<double>::min());
	for (unsigned int i0(0), n0(get_point(0).get_obj()->size()); i0 < n0; ++i0) {
		const PointIdentTimeSlice& ts0(get_point(0).get_obj()->operator[](i0).as_point());
		if (!ts0.is_valid())
			continue;
		if (ts0.get_coord().is_invalid())
			continue;
		for (unsigned int i1(0), n1(get_point(1).get_obj()->size()); i1 < n1; ++i1) {
			const PointIdentTimeSlice& ts1(get_point(1).get_obj()->operator[](i1).as_point());
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

void PointPair::swapdir(void)
{
	std::swap(m_pt[0], m_pt[1]);
}

Rect PointPair::get_bbox(void) const
{
	Rect bbox(Point::invalid, Point::invalid);
	bool first(true);
	for (unsigned int i = 0; i < 2; ++i) {
		const Link& l(get_point(i));
		if (!l.get_obj())
			continue;
		if (first) {
			l.get_obj()->get_bbox(bbox);
			first = false;
			continue;
		}
		Rect bbox1;
		l.get_obj()->get_bbox(bbox1);
		bbox = bbox.add(bbox1);
	}
	return bbox;
}

int PointPair::compare(const PointPair& x) const
{
	int c(get_uuid(0).compare(x.get_uuid(0)));
	if (c)
		return c;
	return get_uuid(1).compare(x.get_uuid(1));
}

void PointPair::link(Database& db, unsigned int depth)
{
	LinkLoader ll(db, depth);
	hibernate(ll);
}

std::ostream& PointPair::print(std::ostream& os, unsigned int indent, timetype_t tm) const
{
	for (unsigned int i = 0; i < 2; ++i) {
		os << std::endl << std::string(indent, ' ') << get_point(i);
		const IdentTimeSlice& id(get_point(i).get_obj()->operator()(tm).as_ident());
		if (id.is_valid())
			os << ' ' << id.get_ident();
	}
	return os;
}

DctLeg::Alt::Alt(const BidirAltRange& ar, const TimeTableOr& tt)
	: m_altrange(ar), m_timetable(tt)
{
}

void DctLeg::Alt::swapdir(void)
{
	m_altrange.swapdir();
}

std::ostream& DctLeg::Alt::print(std::ostream& os, unsigned int indent) const
{
	m_altrange.print(os, indent);
	m_timetable.print(os, indent);
	return os;
}

DctLeg::DctLeg(const Link& pt0, const Link& pt1)
	: PointPair(pt0, pt1)
{
}

void DctLeg::add(altset_t& as, const Alt& a)
{
	std::pair<altset_t::iterator,bool> ins(as.insert(a));
	if (ins.second)
		return;
	Alt& aa(const_cast<Alt&>(*ins.first));
	aa.get_timetable().insert(aa.get_timetable().end(), a.get_timetable().begin(), a.get_timetable().end());
}

void DctLeg::clear_empty(altset_t& as)
{
	for (altset_t::iterator i(as.begin()), e(as.end()); i != e;) {
		if (!i->is_empty()) {
			++i;
			continue;
		}
		altset_t::iterator i1(i);
		++i;
		as.erase(i1);
	}
}

BidirAltRange DctLeg::get_altrange(const TimeTableEval& tte) const
{
	BidirAltRange ar;
	for (altset_t::const_iterator i(m_altset.begin()), e(m_altset.end()); i != e; ++i) {
		if (!i->get_timetable().is_inside(tte))
			continue;
		ar |= i->get_altrange();
	}
	return ar;
}

void DctLeg::swapdir(void)
{
        PointPair::swapdir();
	altset_t x;
	for (altset_t::const_iterator i(m_altset.begin()), e(m_altset.end()); i != e; ++i) {
		Alt a(*i);
		a.swapdir();
		x.insert(a);
	}
	m_altset.swap(x);
}

void DctLeg::simplify(altset_t& as)
{
	altset_t x;
	for (altset_t::const_iterator i(as.begin()), e(as.end()); i != e; ++i) {
		Alt a(*i);
		a.simplify();
		if (a.get_timetable().is_never())
			continue;
		x.insert(a);
	}
	as.swap(x);
}

void DctLeg::fix_overlap(altset_t& as)
{
	typedef std::pair<BidirAltRange,TimeTableWeekdayPattern> altwdp_t;
	typedef std::vector<altwdp_t> altwdpvec_t;
	altwdpvec_t wdpvec;
	// convert to vector of BidirAltRanges / TimeTableWeekdayPatterns
	for (altset_t::iterator i(as.begin()), e(as.end()); i != e;) {
		{
			altwdp_t aw;
			if (i->get_timetable().convert(aw.second)) {
				if (!aw.second.is_never()) {
					aw.first = i->get_altrange();
					wdpvec.push_back(aw);
				}
				altset_t::iterator i1(i);
				++i;
				as.erase(i1);
				continue;
			}
		}
		TimeTableOr& tt(const_cast<TimeTableOr&>(i->get_timetable()));
		for (TimeTableOr::iterator ti(tt.begin()), te(tt.end()); ti != te; ) {
			altwdp_t aw;
			if (ti->convert(aw.second)) {
				if (!aw.second.is_never()) {
					aw.first = i->get_altrange();
					wdpvec.push_back(aw);
				}
				tt.erase(ti);
				continue;
			}
			++ti;
		}
		if (tt.empty()) {
			altset_t::iterator i1(i);
			++i;
			as.erase(i1);
			continue;
		}
		++i;
	}
	// disentangle altitudes
	for (;;) {
		bool work(false);
		for (altwdpvec_t::size_type i0(0), n(wdpvec.size()); i0 < n; ++i0) {
			for (altwdpvec_t::size_type i1(i0 + 1); i1 < n; ++i1) {
				if (!wdpvec[i0].second.startend_eq(wdpvec[i1].second))
					continue;
				if ((wdpvec[i0].first & wdpvec[i1].first).is_empty())
					continue;
				TimeTableWeekdayPattern tp(wdpvec[i0].second & wdpvec[i1].second);
				if (tp.is_never())
					continue;
				BidirAltRange ar(wdpvec[i0].first | wdpvec[i1].first);
				wdpvec.push_back(altwdp_t(wdpvec[i0].first & ~wdpvec[i1].first,
							  wdpvec[i0].second & ~wdpvec[i1].second));
				wdpvec[i1] = altwdp_t(wdpvec[i1].first & ~wdpvec[i0].first,
						      wdpvec[i1].second & ~wdpvec[i0].second);
				wdpvec[i0] = altwdp_t(ar, tp);
				work = true;
			}
		}
		if (!work)
			break;
	}
	// merge sametime
	for (altwdpvec_t::size_type i0(0), n(wdpvec.size()); i0 < n; ++i0) {
		for (altwdpvec_t::size_type i1(i0 + 1); i1 < n; ) {
			if (wdpvec[i0].second != wdpvec[i1].second) {
				++i1;
				continue;
			}
			wdpvec[i0].first |= wdpvec[i1].first;
			wdpvec.erase(wdpvec.begin() + i1);
			n = wdpvec.size();
		}
	}
       	// convert back
	for (altwdpvec_t::iterator i(wdpvec.begin()), e(wdpvec.end()); i != e; ++i) {
		if (i->second.is_never())
			continue;
		TimeTableOr tt;
		tt.push_back(TimeTableAnd());
		tt.back().push_back(i->second.get_timetable());
		add(as, Alt(i->first, tt));
	}
}

void DctLeg::load(ArchiveReadStream& ar)
{
	hibernate(ar);
}

void DctLeg::load(ArchiveReadBuffer& ar)
{
	hibernate(ar);
}

void DctLeg::save(ArchiveWriteStream& ar) const
{
	(const_cast<DctLeg *>(this))->hibernate(ar);
}

void DctLeg::link(Database& db, unsigned int depth)
{
	LinkLoader ll(db, depth);
	hibernate(ll);
}

std::ostream& DctLeg::print(std::ostream& os, unsigned int indent, timetype_t tm) const
{
	PointPair::print(os, indent, tm);
	for (altset_t::const_iterator i(m_altset.begin()), e(m_altset.end()); i != e; ++i)
		i->print(os, indent + 2);
	return os;
}
