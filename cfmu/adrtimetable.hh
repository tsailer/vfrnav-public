#ifndef ADRTIMETABLE_H
#define ADRTIMETABLE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "adr.hh"

namespace ADR {

class TimePattern {
public:
	TimePattern(void);
	static TimePattern create_always(void);
	static TimePattern create_never(void);

	typedef enum {
		operator_set,
		operator_add,
		operator_sub,
		operator_invalid
	} operator_t;

	operator_t get_operator(void) const { return m_operator; }
	void set_operator(operator_t op) { m_operator = op; }

	typedef enum {
		type_hol,
		type_busyfri,
		type_beforehol,
		type_afterhol,
		type_weekday,
		type_invalid
	} type_t;

	type_t get_type(void) const { return m_type; }
	void set_type(type_t t) { m_type = t; }

	uint16_t get_starttime_min(void) const { return m_starttime; }
	void set_starttime_min(uint16_t t) { m_starttime = t; }
	uint16_t get_endtime_min(void) const { return m_endtime; }
	void set_endtime_min(uint16_t t) { m_endtime = t; }

	uint32_t get_starttime(void) const { return get_starttime_min() * 60; }
	uint32_t get_endtime(void) const { return get_endtime_min() * 60; }

	static const uint8_t daymask_sun = (1 << 0);
	static const uint8_t daymask_mon = (1 << 1);
	static const uint8_t daymask_tue = (1 << 2);
	static const uint8_t daymask_wed = (1 << 3);
	static const uint8_t daymask_thu = (1 << 4);
	static const uint8_t daymask_fri = (1 << 5);
	static const uint8_t daymask_sat = (1 << 6);
	static const uint8_t daymask_all = (daymask_sun | daymask_mon | daymask_tue | daymask_wed |
					    daymask_thu | daymask_fri | daymask_sat);

	uint8_t get_daymask(void) const { return m_daymask; }
	void set_daymask(uint8_t m) { m_daymask = m; }
	bool is_wday(int wday) const { return (wday >= 0) && (wday <= 6) && (get_daymask() & (1 << wday)); }

	static const std::string& get_wday_name(int wday);
	static int get_wday_from_name(const std::string& wday);
	static std::string get_daymask_string(uint8_t mask);
	std::string get_daymask_string(void) const { return get_daymask_string(get_daymask()); }

	std::ostream& print(std::ostream& os) const;

	bool is_inside(const TimeTableEval& tte) const;
	bool is_always(void) const;
	bool is_never(void) const;

	template<class Archive> void hibernate(Archive& ar) {
		ar.io(m_starttime);
		ar.io(m_endtime);
		ar.iouint8(m_operator);
		ar.iouint8(m_type);
		ar.io(m_daymask);
	}

protected:
	uint16_t m_starttime;
	uint16_t m_endtime;
	operator_t m_operator;
	type_t m_type;
	uint8_t m_daymask;
};

class TimeTableElement : public std::vector<TimePattern> {
public:
	TimeTableElement(void);
	static TimeTableElement create_always(void);
	static TimeTableElement create_never(void);

	void simplify(void);
	void limit(timetype_t s, timetype_t e);

	timetype_t get_start(void) const { return m_start; }
	void set_start(timetype_t x) { m_start = x; }
	timetype_t get_end(void) const { return m_end; }
	void set_end(timetype_t x) { m_end = x; }

	bool is_exclude(void) const { return m_exclude; }
	void set_exclude(bool e = true) { m_exclude = e; }

	std::ostream& print(std::ostream& os, unsigned int indent) const;

	bool is_inside(const TimeTableEval& tte) const;
	timeset_t timediscontinuities(void) const;

	bool is_always(void) const;
	bool is_never(void) const;

	bool convert(TimeTableWeekdayPattern& x) const;

	template<class Archive> void hibernate(Archive& ar) {
		ar.io(m_start);
		ar.io(m_end);
		ar.iouint8(m_exclude);
		uint32_t n(size());
		ar.ioleb(n);
		if (ar.is_load())
			resize(n);
		for (uint16_t i(0); i < n; ++i)
			this->operator[](i).hibernate(ar);
	}

protected:
	timetype_t m_start;
	timetype_t m_end;
	bool m_exclude;

	void never_if_empty(void);
	void kill_never(void);
	void kill_invalid(void);
	void kill_leadingsub(void);
	void simplify_set(void);
	void simplify_always(void);
};

class TimeTable : public std::vector<TimeTableElement> {
public:
	TimeTable(void);
	static TimeTable create_always(void);
	static TimeTable create_never(void);

	void simplify(void);
	void limit(timetype_t s, timetype_t e);
	void invert(void);
	TimeTable operator~(void) const { TimeTable t(*this); t.invert(); return t; }

	std::ostream& print(std::ostream& os, unsigned int indent) const;

	template<class Archive> void hibernate(Archive& ar) {
		uint32_t n(size());
		ar.ioleb(n);
		if (ar.is_load())
			resize(n);
		for (uint16_t i(0); i < n; ++i)
			this->operator[](i).hibernate(ar);
	}

	bool is_inside(const TimeTableEval& tte) const;
	timeset_t timediscontinuities(void) const;
	timepair_t timebounds(void) const;

	bool is_never(void) const;

	bool convert(TimeTableWeekdayPattern& x) const;

protected:
};

class TimeTableWeekdayPattern {
public:
	TimeTableWeekdayPattern(timetype_t st = 0, timetype_t en = std::numeric_limits<timetype_t>::max());

	timetype_t get_start(void) const { return m_start; }
	void set_start(timetype_t x) { m_start = x; }
	timetype_t get_end(void) const { return m_end; }
	void set_end(timetype_t x) { m_end = x; }

	typedef IntervalSet<uint16_t> daytimeset_t;
	daytimeset_t& operator[](unsigned int idx) { return m_set[idx]; }
	const daytimeset_t& operator[](unsigned int idx) const { return m_set[idx]; }

	void set_never(void);
	void set_h24(void);

	TimeTableWeekdayPattern& invert(void);
	TimeTableWeekdayPattern operator~(void) const { TimeTableWeekdayPattern t(*this); t.invert(); return t; }
	TimeTableWeekdayPattern& operator&=(const TimeTableWeekdayPattern& tt);
	TimeTableWeekdayPattern& operator|=(const TimeTableWeekdayPattern& tt);
	TimeTableWeekdayPattern operator&(const TimeTableWeekdayPattern& tt) const { TimeTableWeekdayPattern t(*this); t &= tt; return t; }
	TimeTableWeekdayPattern operator|(const TimeTableWeekdayPattern& tt) const { TimeTableWeekdayPattern t(*this); t |= tt; return t; }

	TimeTableElement get_timetableelement(void) const;
	TimeTable get_timetable(void) const;

	bool operator==(const TimeTableWeekdayPattern& x) const;
	bool startend_eq(const TimeTableWeekdayPattern& x) const;
	bool daytime_eq(const TimeTableWeekdayPattern& x) const;
	bool operator!=(const TimeTableWeekdayPattern& x) const { return !operator==(x); }

	bool merge_adjacent(const TimeTableWeekdayPattern& x);
	bool merge_sametime(const TimeTableWeekdayPattern& x);

	bool is_never(void) const;
	bool is_h24(void) const;

	std::string to_str(void) const;

protected:
	timetype_t m_start;
	timetype_t m_end;
	daytimeset_t m_set[7];
	static const daytimeset_t fullday;
};

class TimeTableAnd : public std::vector<TimeTable> {
public:
	TimeTableAnd();

	void simplify(void);
	void limit(timetype_t s, timetype_t e);

	std::ostream& print(std::ostream& os, unsigned int indent) const;

	template<class Archive> void hibernate(Archive& ar) {
		uint32_t n(size());
		ar.ioleb(n);
		if (ar.is_load())
			resize(n);
		for (uint16_t i(0); i < n; ++i)
			this->operator[](i).hibernate(ar);
	}

	bool is_inside(const TimeTableEval& tte) const;
	timeset_t timediscontinuities(void) const;
	timepair_t timebounds(void) const;

	bool is_never(void) const;

	bool convert(TimeTableWeekdayPattern& x) const;

protected:
};

class TimeTableOr : public std::vector<TimeTableAnd> {
public:
	TimeTableOr();
	static TimeTableOr create_always(void);
	static TimeTableOr create_never(void);

	void simplify(bool merge_adj = true);
	void limit(timetype_t s, timetype_t e);
	void split(const timeset_t& s);
	bool is_always(void) const;
	bool is_never(void) const;

	bool convert(TimeTableWeekdayPattern& x) const;

	TimeTableOr& invert(void);
	TimeTableOr operator~(void) const { TimeTableOr t(*this); t.invert(); return t; }
	TimeTableOr& operator&=(const TimeTableOr& tt);
	TimeTableOr& operator|=(const TimeTableOr& tt);
	TimeTableOr operator&(const TimeTableOr& tt) const { TimeTableOr t(*this); t &= tt; return t; }
	TimeTableOr operator|(const TimeTableOr& tt) const { TimeTableOr t(*this); t |= tt; return t; }
	TimeTableOr& operator&=(const TimeTable& tt);
	TimeTableOr& operator|=(const TimeTable& tt);
	TimeTableOr operator&(const TimeTable& tt) const { TimeTableOr t(*this); t &= tt; return t; }
	TimeTableOr operator|(const TimeTable& tt) const { TimeTableOr t(*this); t |= tt; return t; }

	std::ostream& print(std::ostream& os, unsigned int indent) const;

	template<class Archive> void hibernate(Archive& ar) {
		uint32_t n(size());
		ar.ioleb(n);
		if (ar.is_load())
			resize(n);
		for (uint16_t i(0); i < n; ++i)
			this->operator[](i).hibernate(ar);
	}

	bool is_inside(const TimeTableEval& tte) const;
	timeset_t timediscontinuities(void) const;
};

class TimeTableEval {
public:
	TimeTableEval(timetype_t tm, const Point& pt, const TimeTableSpecialDateEval& ttsde);
	timetype_t get_time(void) const { return m_time; }
	const Point& get_point(void) const { return m_pt; }
	int get_sec(void) const { return m_tm.tm_sec; }
	int get_min(void) const { return m_tm.tm_min; }
	int get_hour(void) const { return m_tm.tm_hour; }
	int get_mday(void) const { return m_tm.tm_mday; }
	int get_mon(void) const { return m_tm.tm_mon + 1; }
	int get_year(void) const { return m_tm.tm_year + 1900; }
	int get_wday(void) const { return m_tm.tm_wday; }
	unsigned int get_daytime(void) const { return (get_hour() * 60 + get_min()) * 60 + get_sec(); }
	bool is_busyfri(void) const;
	bool is_hol(void) const;
	bool is_beforehol(void) const;
	bool is_afterhol(void) const;
	std::string to_str(void) const;
	const TimeTableSpecialDateEval& get_specialdateeval(void) const { return m_specialeval; }

protected:
	const TimeTableSpecialDateEval& m_specialeval;
	struct tm m_tm;
	Point m_pt;
	timetype_t m_time;
	mutable uint8_t m_special;
};

class PointPair {
public:
	PointPair(const Link& pt0 = Link(), const Link& pt1 = Link());
	const Link& get_point(unsigned int index) const { return m_pt[!!index]; }
	Link& get_point(unsigned int index) { return m_pt[!!index]; }
	const UUID& get_uuid(unsigned int index) const;
	std::pair<double,double> dist(void) const;
	void swapdir(void);
	Rect get_bbox(void) const;
	int compare(const PointPair& x) const;
	bool operator==(const PointPair& x) const { return compare(x) == 0; }
	bool operator!=(const PointPair& x) const { return compare(x) != 0; }
	bool operator<(const PointPair& x) const { return compare(x) < 0; }
	bool operator<=(const PointPair& x) const { return compare(x) <= 0; }
	bool operator>(const PointPair& x) const { return compare(x) > 0; }
	bool operator>=(const PointPair& x) const { return compare(x) >= 0; }

	std::ostream& print(std::ostream& os, unsigned int indent, timetype_t tm) const;

	void link(Database& db, unsigned int depth = 0);

	template<class Archive> void hibernate(Archive& ar) {
		ar.io(m_pt[0]);
		ar.io(m_pt[1]);
	}

protected:
	Link m_pt[2];
};

class DctLeg : public PointPair {
public:
	class Alt {
	public:
		Alt(const BidirAltRange& ar = BidirAltRange(), const TimeTableOr& tt = TimeTableOr());
		BidirAltRange& get_altrange(void) { return m_altrange; }
		const BidirAltRange& get_altrange(void) const { return m_altrange; }
		TimeTableOr& get_timetable(void) { return m_timetable; }
		const TimeTableOr& get_timetable(void) const { return m_timetable; }
		int compare(const Alt& x) const { return m_altrange.compare(x.m_altrange); }
		bool operator==(const Alt& x) const { return compare(x) == 0; }
		bool operator!=(const Alt& x) const { return compare(x) != 0; }
		bool operator<(const Alt& x) const { return compare(x) < 0; }
		bool operator<=(const Alt& x) const { return compare(x) <= 0; }
		bool operator>(const Alt& x) const { return compare(x) > 0; }
		bool operator>=(const Alt& x) const { return compare(x) >= 0; }
		void swapdir(void);
		bool is_empty(void) const { return m_altrange.is_empty(); }
		void simplify(void) { m_timetable.simplify(); }

		std::ostream& print(std::ostream& os, unsigned int indent) const;

		template<class Archive> void hibernate(Archive& ar) {
			m_altrange.hibernate(ar);
			m_timetable.hibernate(ar);
		}

	protected:
		BidirAltRange m_altrange;
		TimeTableOr m_timetable;
	};

	DctLeg(const Link& pt0 = Link(), const Link& pt1 = Link());
	typedef std::set<Alt> altset_t;
	static void add(altset_t& as, const Alt& a);
	void add(const Alt& a) { add(m_altset, a); }
	static void clear_empty(altset_t& as);
	void clear_empty(void) { clear_empty(m_altset); }
	BidirAltRange get_altrange(const TimeTableEval& tte) const;
	const altset_t& get_altset(void) const { return m_altset; }
	altset_t& get_altset(void) { return m_altset; }
	void swapdir(void);
	static void simplify(altset_t& as);
	void simplify(void) { simplify(m_altset); }
	static void fix_overlap(altset_t& as);
	void fix_overlap(void) { fix_overlap(m_altset); }
	bool is_empty(void) const { return m_altset.empty(); }

	std::ostream& print(std::ostream& os, unsigned int indent, timetype_t tm) const;

	void load(ArchiveReadStream& ar);
	void load(ArchiveReadBuffer& ar);
	void save(ArchiveWriteStream& ar) const;
	void link(Database& db, unsigned int depth = 0);

	template<class Archive> void hibernate(Archive& ar) {
		if (!ar.is_dbblob()) {
			PointPair::hibernate(ar);
		}
		uint32_t sz(m_altset.size());
		ar.ioleb(sz);
		if (ar.is_load()) {
			m_altset.clear();
			for (; sz; --sz) {
				Alt x;
				x.hibernate(ar);
				m_altset.insert(x);
			}
		} else {
			for (altset_t::iterator i(m_altset.begin()), e(m_altset.end()); i != e && sz > 0; ++i, --sz)
				const_cast<Alt&>(*i).hibernate(ar);
		}
	}

protected:
	altset_t m_altset;
};

};

const std::string& to_str(ADR::TimePattern::operator_t o);
inline std::ostream& operator<<(std::ostream& os, ADR::TimePattern::operator_t o) { return os << to_str(o); }
const std::string& to_str(ADR::TimePattern::type_t t);
inline std::ostream& operator<<(std::ostream& os, ADR::TimePattern::type_t t) { return os << to_str(t); }

#endif /* ADRTIMETABLE_H */
