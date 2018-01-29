#ifndef INTERVAL_H
#define INTERVAL_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <limits>
#include <string>
#include <set>
#include <vector>
//#include <cstdint>
#include <stdint.h>

template <typename T>
class Interval {
public:
	typedef T type_t;

	Interval(void);
	Interval(type_t val);
	Interval(type_t val0, type_t val1);

	type_t get_lower(void) const { return m_lower; }
	type_t get_upper(void) const { return m_upper; }
	bool is_empty(void) const;
	bool is_full(void) const;
	bool is_inside(type_t x) const;
	bool is_overlap(const Interval& x) const;
	bool is_adjacent(const Interval& x) const;
	bool is_overlap_or_adjacent(const Interval& x) const;
	void set_lower(type_t val) { m_lower = val; }
	void set_upper(type_t val) { m_upper = val; }
	void set_empty(void);
	void set_full(void);
	std::string to_str(void) const;
	int compare(const Interval& x) const;
	bool operator==(const Interval& x) const { return compare(x) == 0; }
	bool operator!=(const Interval& x) const { return compare(x) != 0; }
	bool operator<(const Interval& x) const { return compare(x) < 0; }
	bool operator<=(const Interval& x) const { return compare(x) <= 0; }
	bool operator>(const Interval& x) const { return compare(x) > 0; }
	bool operator>=(const Interval& x) const { return compare(x) >= 0; }
	Interval& operator&=(const Interval& x) { *this = *this & x; return *this; }
	Interval operator&(const Interval& x) const;

	template<class Archive> void hibernate(Archive& ar) {
		ar.io(m_lower);
		ar.io(m_upper);
	}

protected:
	type_t m_lower;
	type_t m_upper;
};

template <typename T, typename ST = std::vector<Interval<T> > >
class IntervalSet {
public:
	typedef T type_t;
	typedef Interval<T> Intvl;

	IntervalSet(void);
	IntervalSet(const Intvl& x);
	IntervalSet(const IntervalSet& x);
	bool is_empty(void) const;
	bool is_full(void) const;
	bool is_inside(type_t x) const;
	void set_empty(void);
	void set_full(void);
	void swap(IntervalSet& x);
	IntervalSet& operator&=(type_t x);
	IntervalSet& operator|=(type_t x);
	IntervalSet& operator^=(type_t x);
	IntervalSet& operator-=(type_t x);
	IntervalSet& operator&=(const Intvl& x);
	IntervalSet& operator|=(const Intvl& x);
	IntervalSet& operator^=(const Intvl& x);
	IntervalSet& operator-=(const Intvl& x);
	IntervalSet& operator&=(const IntervalSet& x);
	IntervalSet& operator|=(const IntervalSet& x);
	IntervalSet& operator^=(const IntervalSet& x);
	IntervalSet& operator-=(const IntervalSet& x);
	IntervalSet operator~() const;
	IntervalSet operator&(type_t x) const { IntervalSet y(*this); y &= x; return y; }
	IntervalSet operator|(type_t x) const { IntervalSet y(*this); y |= x; return y; }
	IntervalSet operator^(type_t x) const { IntervalSet y(*this); y ^= x; return y; }
	IntervalSet operator-(type_t x) const { IntervalSet y(*this); y -= x; return y; }
	IntervalSet operator&(const Intvl& x) const { IntervalSet y(*this); y &= x; return y; }
	IntervalSet operator|(const Intvl& x) const { IntervalSet y(*this); y |= x; return y; }
	IntervalSet operator^(const Intvl& x) const { IntervalSet y(*this); y ^= x; return y; }
	IntervalSet operator-(const Intvl& x) const { IntervalSet y(*this); y -= x; return y; }
	IntervalSet operator&(const IntervalSet& x) const { IntervalSet y(*this); y &= x; return y; }
	IntervalSet operator|(const IntervalSet& x) const { IntervalSet y(*this); y |= x; return y; }
	IntervalSet operator^(const IntervalSet& x) const { IntervalSet y(*this); y ^= x; return y; }
	IntervalSet operator-(const IntervalSet& x) const { IntervalSet y(*this); y -= x; return y; }
	std::string to_str(void) const;
	int compare(const IntervalSet& x) const;
	bool operator==(const IntervalSet& x) const { return compare(x) == 0; }
	bool operator!=(const IntervalSet& x) const { return compare(x) != 0; }
	bool operator<(const IntervalSet& x) const { return compare(x) < 0; }
	bool operator<=(const IntervalSet& x) const { return compare(x) <= 0; }
	bool operator>(const IntervalSet& x) const { return compare(x) > 0; }
	bool operator>=(const IntervalSet& x) const { return compare(x) >= 0; }

	template<class Archive> void hibernate(Archive& ar) {
		uint32_t sz(m_set.size());
		ar.ioleb(sz);
		if (ar.is_load()) {
			m_set.clear();
			for (; sz; --sz) {
				Intvl x;
				x.hibernate(ar);
				add_traits<T,ST>::add(m_set, x);
			}
		} else {
			for (typename set_t::iterator i(m_set.begin()), e(m_set.end()); i != e && sz > 0; ++i, --sz)
				const_cast<Intvl&>(*i).hibernate(ar);
		}
	}

protected:
	typedef ST set_t;

	template <typename TT, typename TST>
	struct add_traits {
		typedef TST set_t;
	};

	template <typename TT>
	struct add_traits<TT, std::set<Interval<TT> > > {
		typedef std::set<Interval<TT> > set_t;
		typedef Interval<TT> Intvl;

		static inline bool add(set_t& set, const Interval<TT>& iv) {
			if (iv.is_empty())
				return false;
			return set.insert(iv).second;
		}
	};

	template <typename TT>
	struct add_traits<TT, std::vector<Interval<TT> > > {
		typedef std::vector<Interval<TT> > set_t;
		typedef Interval<TT> Intvl;

		static inline bool add(set_t& set, const Interval<TT>& iv) {
			if (iv.is_empty())
				return false;
			set.push_back(iv);
			return true;
		}
	};

public:
	typedef typename set_t::const_iterator const_iterator;
	const_iterator begin(void) const { return m_set.begin(); }
	const_iterator end(void) const { return m_set.end(); }
	typedef typename set_t::const_reverse_iterator const_reverse_iterator;
	const_reverse_iterator rbegin(void) const { return m_set.rbegin(); }
	const_reverse_iterator rend(void) const { return m_set.rend(); }

protected:
	set_t m_set;

	void normalize(void);
};

template <typename T> inline IntervalSet<T> operator&(const IntervalSet<T>& x, const IntervalSet<T>& y)
{
	IntervalSet<T> r(x);
	r &= y;
	return r;
}

template <typename T> inline IntervalSet<T> operator|(const IntervalSet<T>& x, const IntervalSet<T>& y)
{
	IntervalSet<T> r(x);
	r |= y;
	return r;
}

template <typename T> inline IntervalSet<T> operator^(const IntervalSet<T>& x, const IntervalSet<T>& y)
{
	IntervalSet<T> r(x);
	r ^= y;
	return r;
}

template <typename T> inline IntervalSet<T> operator-(const IntervalSet<T>& x, const IntervalSet<T>& y)
{
	IntervalSet<T> r(x);
	r -= y;
	return r;
}

#endif /* INTERVAL_H */
