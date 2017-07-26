#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "interval.hh"

#include <algorithm>
#include <sstream>
#include <type_traits>

template <typename T>
Interval<T>::Interval(void)
	: m_lower(std::numeric_limits<type_t>::min()), m_upper(std::numeric_limits<type_t>::max())
{
}

template <typename T>
Interval<T>::Interval(type_t val)
	: m_lower(val), m_upper(val + 1)
{
}

template <typename T>
Interval<T>::Interval(type_t val0, type_t val1)
	: m_lower(val0), m_upper(val1)
{
}

template <typename T>
bool Interval<T>::is_empty(void) const
{
	return m_lower >= m_upper;
}

template <typename T>
bool Interval<T>::is_full(void) const
{
	return m_lower == std::numeric_limits<type_t>::min() &&
		m_upper == std::numeric_limits<type_t>::max();
}
template <typename T>
bool Interval<T>::is_inside(type_t x) const
{
	return x >= m_lower && x < m_upper;
}

template <typename T>
bool Interval<T>::is_overlap(const Interval& x) const
{
	if (is_empty() || x.is_empty())
		return false;
	return x.get_upper() > get_lower() && x.get_lower() < get_upper();
}

template <typename T>
bool Interval<T>::is_adjacent(const Interval& x) const
{
	if (is_empty() || x.is_empty())
		return false;
	return x.get_upper() == get_lower() || get_upper() == x.get_lower();
}

template <typename T>
bool Interval<T>::is_overlap_or_adjacent(const Interval& x) const
{
	if (is_empty() || x.is_empty())
		return false;
	return x.get_upper() >= get_lower() && x.get_lower() <= get_upper();
}

template <typename T>
void Interval<T>::set_empty(void)
{
	m_lower = m_upper = std::numeric_limits<type_t>::min();
}

template <typename T>
void Interval<T>::set_full(void)
{
	m_lower = std::numeric_limits<type_t>::min();
	m_upper = std::numeric_limits<type_t>::max();
}

template <typename T>
std::string Interval<T>::to_str(void) const
{
	std::ostringstream oss;
	oss << '[' << m_lower << ',' << m_upper << ')';
	return oss.str();
}

template <typename T>
int Interval<T>::compare(const Interval& x) const
{
	if (m_lower < x.m_lower)
		return -1;
	if (x.m_lower < m_lower)
		return 1;
	if (m_upper < x.m_upper)
		return -1;
	if (x.m_upper < m_upper)
		return 1;
	return 0;
}

template <typename T, typename ST>
struct interval_traits {
	typedef ST set_t;
};

template <typename T>
struct interval_traits<T, std::set<Interval<T> > > {
	typedef std::set<Interval<T> > set_t;
	typedef Interval<T> Intvl;

	static inline void reserve(set_t& set, const set_t& a, const set_t& b) {
	}

	static inline void normalize(set_t& set) {
		for (typename set_t::iterator i(set.begin()), e(set.end()); i != e; ) {
			const Intvl& x(*i);
			typename set_t::iterator in(i);
			++in;
			// remove empty subintervals
			if (x.is_empty()) {
				set.erase(i);
				i = in;
				//e = set.end();
				continue;
			}
			if (in == e)
				break;
			const Intvl& y(*in);
			// remove empty subintervals
			if (y.is_empty()) {
				set.erase(in);
				//e = set.end();
				continue;
			}
			// merge adjacent
			if (!x.is_overlap_or_adjacent(y)) {
				++i;
				continue;
			}
			Intvl z(std::min(x.get_lower(), y.get_lower()),
				std::max(x.get_upper(), y.get_upper()));
			set.erase(i);
			set.erase(in);
			std::pair<typename set_t::iterator,bool> ins(set.insert(z));
			i = ins.first;
			//e = set.end();
		}
	}
};

template <typename T>
struct interval_traits<T, std::vector<Interval<T> > > {
	typedef std::vector<Interval<T> > set_t;
	typedef Interval<T> Intvl;

	static inline void reserve(set_t& set, const set_t& a, const set_t& b) {
		set.reserve(a.size() * b.size());
	}

	static inline void normalize(set_t& set) {
		std::sort(set.begin(), set.end());
		for (typename set_t::iterator i(set.begin()), e(set.end()); i != e; ) {
			Intvl& x(*i);
			// remove empty subintervals
			if (x.is_empty()) {
				i = set.erase(i);
				e = set.end();
				continue;
			}
			typename set_t::iterator in(i);
			++in;
			if (in == e)
				break;
			const Intvl& y(*in);
			// remove empty subintervals
			if (y.is_empty()) {
				i = set.erase(in);
				e = set.end();
				--i;
				continue;
			}
			// merge adjacent
			if (!x.is_overlap_or_adjacent(y)) {
				++i;
				continue;
			}
			x = Intvl(std::min(x.get_lower(), y.get_lower()),
				  std::max(x.get_upper(), y.get_upper()));
			i = set.erase(in);
			e = set.end();
			--i;
		}
	}
};

template <typename T, typename ST>
IntervalSet<T,ST>::IntervalSet(void)
{
}

template <typename T, typename ST>
IntervalSet<T,ST>::IntervalSet(const Intvl& x)
{
	add_traits<T,ST>::add(m_set, x);
	normalize();
}

template <typename T, typename ST>
IntervalSet<T,ST>::IntervalSet(const IntervalSet& x)
{
	m_set = x.m_set;
}

template <typename T, typename ST>
bool IntervalSet<T,ST>::is_empty(void) const
{
	return m_set.empty();
}

template <typename T, typename ST>
bool IntervalSet<T,ST>::is_full(void) const
{
	if (m_set.empty())
		return false;
	return m_set.begin()->is_full();
}

template <typename T, typename ST>
bool IntervalSet<T,ST>::is_inside(type_t x) const
{
	for (typename set_t::const_iterator i(m_set.begin()), e(m_set.end()); i != e; ++i)
		if (i->is_inside(x))
			return true;
	return false;
}

template <typename T, typename ST>
void IntervalSet<T,ST>::set_empty(void)
{
	m_set.clear();
}

template <typename T, typename ST>
void IntervalSet<T,ST>::set_full(void)
{
	m_set.clear();
	Intvl x;
	x.set_full();
	add_traits<T,ST>::add(m_set, x);
}

template <typename T, typename ST>
void IntervalSet<T,ST>::swap(IntervalSet& x)
{
	m_set.swap(x.m_set);
}

template <typename T, typename ST>
IntervalSet<T,ST>& IntervalSet<T,ST>::operator&=(type_t x)
{
	return (*this) &= Intvl(x);
}

template <typename T, typename ST>
IntervalSet<T,ST>& IntervalSet<T,ST>::operator|=(type_t x)
{
	return (*this) |= Intvl(x);
}

template <typename T, typename ST>
IntervalSet<T,ST>& IntervalSet<T,ST>::operator^=(type_t x)
{
	return (*this) ^= Intvl(x);
}

template <typename T, typename ST>
IntervalSet<T,ST>& IntervalSet<T,ST>::operator-=(type_t x)
{
	return (*this) -= Intvl(x);
}

template <typename T, typename ST>
IntervalSet<T,ST>& IntervalSet<T,ST>::operator&=(const Intvl& x)
{
	return (*this) &= IntervalSet(x);
}

template <typename T, typename ST>
IntervalSet<T,ST>& IntervalSet<T,ST>::operator|=(const Intvl& x)
{
	if (add_traits<T,ST>::add(m_set, x))
		normalize();
	return *this;
}

template <typename T, typename ST>
IntervalSet<T,ST>& IntervalSet<T,ST>::operator^=(const Intvl& x)
{
	return (*this) ^= IntervalSet(x);
}

template <typename T, typename ST>
IntervalSet<T,ST>& IntervalSet<T,ST>::operator-=(const Intvl& x)
{
	return (*this) &= ~IntervalSet(x);
}

template <typename T, typename ST>
IntervalSet<T,ST>& IntervalSet<T,ST>::operator&=(const IntervalSet& x)
{
	IntervalSet y;
	y.m_set.swap(m_set);
	interval_traits<T,ST>::reserve(m_set, x.m_set, y.m_set);
	for (typename set_t::const_iterator xi(x.m_set.begin()), xe(x.m_set.end()); xi != xe; ++xi) {
		const Intvl& xx(*xi);
		if (xx.is_empty())
			continue;
		for (typename set_t::const_iterator yi(y.m_set.begin()), ye(y.m_set.end()); yi != ye; ++yi) {
			const Intvl& yy(*yi);
			if (yy.is_empty())
				continue;
			if (yy.get_lower() >= xx.get_upper())
				break;
			if (!yy.is_overlap_or_adjacent(xx))
				continue;
			add_traits<T,ST>::add(m_set, Intvl(std::max(xx.get_lower(), yy.get_lower()), std::min(xx.get_upper(), yy.get_upper())));
		}
	}
	normalize();
	return *this;
}

template <typename T, typename ST>
IntervalSet<T,ST>& IntervalSet<T,ST>::operator|=(const IntervalSet& x)
{
	bool norm(false);
	for (typename set_t::const_iterator i(x.m_set.begin()), e(x.m_set.end()); i != e; ++i)
		norm = add_traits<T,ST>::add(m_set, *i) || norm;
	if (norm)
		normalize();
	return *this;
}

template <typename T, typename ST>
IntervalSet<T,ST>& IntervalSet<T,ST>::operator^=(const IntervalSet& x)
{
	IntervalSet z(*this);
	z &= x;
	(*this) |= x;
	return (*this) -= z;
}

template <typename T, typename ST>
IntervalSet<T,ST>& IntervalSet<T,ST>::operator-=(const IntervalSet& x)
{
	return (*this) &= ~x;
}

template <typename T, typename ST>
IntervalSet<T,ST> IntervalSet<T,ST>::operator~() const
{
	IntervalSet z;
	type_t v(std::numeric_limits<type_t>::min());
	for (typename set_t::const_iterator i(m_set.begin()), e(m_set.end()); i != e; ++i) {
		const Intvl& x(*i);
		if (x.is_empty())
			continue;
		if (v < x.get_lower())
			add_traits<T,ST>::add(z.m_set, Intvl(v, x.get_lower()));
		if (v < x.get_upper())
			v = x.get_upper();
	}
	if (v < std::numeric_limits<type_t>::max())
		add_traits<T,ST>::add(z.m_set, Intvl(v, std::numeric_limits<type_t>::max()));
	return z;
}

template <typename T, typename ST>
std::string IntervalSet<T,ST>::to_str(void) const
{
	bool subseq(false);
	std::string r;
	for (typename set_t::const_iterator i(m_set.begin()), e(m_set.end()); i != e; ++i) {
		if (i->is_empty())
			continue;
		if (subseq)
			r += "|";
		subseq = true;
		r += i->to_str();
	}
	if (subseq)
		return r;
	return "[)";
}

template <typename T, typename ST>
void IntervalSet<T,ST>::normalize(void)
{
	interval_traits<T,ST>::normalize(m_set);
}

template <typename T, typename ST>
int IntervalSet<T,ST>::compare(const IntervalSet& x) const
{
	typename set_t::const_iterator i0(m_set.begin()), e0(m_set.end()), i1(x.m_set.begin()), e1(x.m_set.end());
	for (;;) {
		if (i0 == e0)
			return (i1 == e1) ? 0 : -1;
		if (i1 == e1)
			return 1;
		int c(i0->compare(*i1));
		if (c)
			return c;
		++i0;
		++i1;
	}
}

template class Interval<uint16_t>;
template class IntervalSet<uint16_t, std::set<Interval<uint16_t> > >;
template class IntervalSet<uint16_t, std::vector<Interval<uint16_t> > >;
template class Interval<int32_t>;
template class IntervalSet<int32_t, std::set<Interval<int32_t> > >;
template class IntervalSet<int32_t, std::vector<Interval<int32_t> > >;
template class Interval<uint64_t>;
template class IntervalSet<uint64_t, std::set<Interval<uint64_t> > >;
template class IntervalSet<uint64_t, std::vector<Interval<uint64_t> > >;
template class Interval<int64_t>;
template class IntervalSet<int64_t, std::set<Interval<int64_t> > >;
template class IntervalSet<int64_t, std::vector<Interval<int64_t> > >;
