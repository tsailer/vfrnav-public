#ifndef ADRROUTE_H
#define ADRROUTE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "adr.hh"

namespace ADR {

class SegmentTimeSlice : public TimeSlice {
public:
	SegmentTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual SegmentTimeSlice& as_segment(void);
	virtual const SegmentTimeSlice& as_segment(void) const;

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

	virtual void get_bbox(Rect& bbox) const { bbox = m_bbox; }
	void set_bbox(const Rect& bbox) { m_bbox = bbox; }

        const Link& get_route(void) const { return m_route; }
	void set_route(const Link& route) { m_route = route; }

        const Link& get_start(void) const { return m_start; }
	void set_start(const Link& start) { m_start = start; }

        const Link& get_end(void) const { return m_end; }
	void set_end(const Link& end) { m_end = end; }

	const AltRange& get_altrange(void) const { return m_altrange; }
	AltRange& get_altrange(void) { return m_altrange; }
	void set_altrange(const AltRange& ar) { m_altrange = ar; }

	AltRange::alt_t get_lower_mode(void) const { return get_altrange().get_lower_mode(); }
	int32_t get_lower_alt(void) const { return get_altrange().get_lower_alt(); }
	AltRange::alt_t get_upper_mode(void) const { return get_altrange().get_upper_mode(); }
	int32_t get_upper_alt(void) const { return get_altrange().get_upper_alt(); }
	const std::string& get_lower_mode_string(void) const { return get_altrange().get_lower_mode_string(); }
	const std::string& get_upper_mode_string(void) const { return get_altrange().get_upper_mode_string(); }
	bool is_lower_valid(void) const { return get_altrange().is_lower_valid(); }
	bool is_upper_valid(void) const { return get_altrange().is_upper_valid(); }
	bool is_alt_valid(void) const { return get_altrange().is_valid(); }
	int32_t get_lower_alt_if_valid(void) const { return get_altrange().get_lower_alt_if_valid(); }
	int32_t get_upper_alt_if_valid(void) const { return get_altrange().get_upper_alt_if_valid(); }

	typedef ElevPointIdentTimeSlice::elev_t elev_t;

	elev_t get_terrainelev(void) const { return m_terrainelev; }
	void set_terrainelev(elev_t e = ElevPointIdentTimeSlice::invalid_elev) { m_terrainelev = e; }
	bool is_terrainelev_valid(void) const { return get_terrainelev() != ElevPointIdentTimeSlice::invalid_elev; }
	elev_t get_corridor5elev(void) const { return m_corridor5elev; }
	void set_corridor5elev(elev_t e = ElevPointIdentTimeSlice::invalid_elev) { m_corridor5elev = e; }
	bool is_corridor5elev_valid(void) const { return get_corridor5elev() != ElevPointIdentTimeSlice::invalid_elev; }

	virtual bool is_forward(void) const { return true; }
	virtual bool is_backward(void) const { return false; }

	virtual bool is_airway(void) const { return true; }
	static bool is_airway_static(void) { return true; }
	virtual bool get(uint64_t tm, AirwaysDb::Airway& el) const;

	virtual bool recompute(void);
	virtual bool recompute(TopoDb30& topodb);

	typedef std::pair<elev_t,elev_t> profile_t;
	static profile_t compute_profile(TopoDb30& topodb, const Point& p0, const Point& p1);
	profile_t compute_profile(TopoDb30& topodb) const;

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
	        TimeSlice::hibernate(ar);
		ar.io(m_bbox);
		ar.io(m_route);
		ar.io(m_start);
		ar.io(m_end);
		m_altrange.hibernate(ar);
		ar.io(m_terrainelev);
		ar.io(m_corridor5elev);
	}

protected:
	Rect m_bbox;
	Link m_route;
	Link m_start;
	Link m_end;
	AltRange m_altrange;
	elev_t m_terrainelev;
	elev_t m_corridor5elev;
};

class StandardInstrumentTimeSlice : public IdentTimeSlice {
public:
	StandardInstrumentTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual timeset_t timediscontinuities(void) const;

	virtual StandardInstrumentTimeSlice& as_sidstar(void);
	virtual const StandardInstrumentTimeSlice& as_sidstar(void) const;

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

	const TimeTable& get_timetable(void) const { return m_timetable; }
        void set_timetable(const TimeTable& tt) { m_timetable = tt; }

	const Link& get_airport(void) const { return m_airport; }
	void set_airport(const Link& a) { m_airport = a; }

	const std::string& get_instruction(void) const { return m_instruction; }
	void get_instruction(const std::string& t) { m_instruction = t; }

	typedef std::set<Link> connpoints_t;
	const connpoints_t& get_connpoints(void) const { return m_connpoints; }
	connpoints_t& get_connpoints(void) { return m_connpoints; }
	void set_connpoints(const connpoints_t& cp) { m_connpoints = cp; }

	typedef enum {
		status_usable,
		status_atcdiscr,
		status_invalid
	} status_t;

	status_t get_status(void) const { return m_status; }
	void set_status(status_t s) { m_status = s; }

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		IdentTimeSlice::hibernate(ar);
		m_timetable.hibernate(ar);
		ar.io(m_airport);
		ar.io(m_connpoints);
		ar.io(m_instruction);
		ar.iouint8(m_status);
	}

protected:
	TimeTable m_timetable;
	Link m_airport;
	connpoints_t m_connpoints;
	std::string m_instruction;
	status_t m_status;
};

class StandardInstrumentDepartureTimeSlice : public StandardInstrumentTimeSlice {
public:
	StandardInstrumentDepartureTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual StandardInstrumentDepartureTimeSlice& as_sid(void);
	virtual const StandardInstrumentDepartureTimeSlice& as_sid(void) const;

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

        const std::string& get_instruction(void) const { return m_instruction; }
	void set_instruction(const std::string& x) { m_instruction = x; }

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		StandardInstrumentTimeSlice::hibernate(ar);
		ar.io(m_instruction);
	}

protected:
	std::string m_instruction;
};

class StandardInstrumentDeparture : public ObjectImpl<StandardInstrumentDepartureTimeSlice,Object::type_sid> {
public:
	typedef Glib::RefPtr<StandardInstrumentDeparture> ptr_t;
	typedef Glib::RefPtr<const StandardInstrumentDeparture> const_ptr_t;
	typedef ObjectImpl<StandardInstrumentDepartureTimeSlice,Object::type_sid> objbase_t;

	StandardInstrumentDeparture(const UUID& uuid = UUID());

	ptr_t clone_obj(void) const { ptr_t p(new StandardInstrumentDeparture(get_uuid())); clone_impl(p); return p; }
	virtual Object::ptr_t clone(void) const { return clone_obj(); }
};

class StandardInstrumentArrivalTimeSlice : public StandardInstrumentTimeSlice {
public:
	StandardInstrumentArrivalTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual StandardInstrumentArrivalTimeSlice& as_star(void);
	virtual const StandardInstrumentArrivalTimeSlice& as_star(void) const;

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

        const Link& get_iaf(void) const { return m_iaf; }
	void set_iaf(const Link& iaf) { m_iaf = iaf; }

        const std::string& get_instruction(void) const { return m_instruction; }
	void set_instruction(const std::string& x) { m_instruction = x; }

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		StandardInstrumentTimeSlice::hibernate(ar);
		ar.io(m_iaf);
		ar.io(m_instruction);
	}

protected:
        Link m_iaf;
	std::string m_instruction;
};

class StandardInstrumentArrival : public ObjectImpl<StandardInstrumentArrivalTimeSlice,Object::type_star> {
public:
	typedef Glib::RefPtr<StandardInstrumentArrival> ptr_t;
	typedef Glib::RefPtr<const StandardInstrumentArrival> const_ptr_t;
	typedef ObjectImpl<StandardInstrumentArrivalTimeSlice,Object::type_star> objbase_t;

	StandardInstrumentArrival(const UUID& uuid = UUID());

	ptr_t clone_obj(void) const { ptr_t p(new StandardInstrumentArrival(get_uuid())); clone_impl(p); return p; }
	virtual Object::ptr_t clone(void) const { return clone_obj(); }
};

class DepartureLegTimeSlice : public SegmentTimeSlice {
public:
	DepartureLegTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual DepartureLegTimeSlice& as_departureleg(void);
	virtual const DepartureLegTimeSlice& as_departureleg(void) const;

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

	template<class Archive> void hibernate(Archive& ar) {
		SegmentTimeSlice::hibernate(ar);
	}

protected:
};

class DepartureLeg : public ObjectImpl<DepartureLegTimeSlice,Object::type_departureleg> {
public:
	typedef Glib::RefPtr<DepartureLeg> ptr_t;
	typedef Glib::RefPtr<const DepartureLeg> const_ptr_t;
	typedef ObjectImpl<DepartureLegTimeSlice,Object::type_departureleg> objbase_t;

	DepartureLeg(const UUID& uuid = UUID());

	ptr_t clone_obj(void) const { ptr_t p(new DepartureLeg(get_uuid())); clone_impl(p); return p; }
	virtual Object::ptr_t clone(void) const { return clone_obj(); }
};

class ArrivalLegTimeSlice : public SegmentTimeSlice {
public:
	ArrivalLegTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual ArrivalLegTimeSlice& as_arrivalleg(void);
	virtual const ArrivalLegTimeSlice& as_arrivalleg(void) const;

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

	template<class Archive> void hibernate(Archive& ar) {
		SegmentTimeSlice::hibernate(ar);
	}

protected:
};

class ArrivalLeg : public ObjectImpl<ArrivalLegTimeSlice,Object::type_arrivalleg> {
public:
	typedef Glib::RefPtr<ArrivalLeg> ptr_t;
	typedef Glib::RefPtr<const ArrivalLeg> const_ptr_t;
	typedef ObjectImpl<ArrivalLegTimeSlice,Object::type_arrivalleg> objbase_t;

	ArrivalLeg(const UUID& uuid = UUID());

	ptr_t clone_obj(void) const { ptr_t p(new ArrivalLeg(get_uuid())); clone_impl(p); return p; }
	virtual Object::ptr_t clone(void) const { return clone_obj(); }
};

class RouteTimeSlice : public IdentTimeSlice {
public:
	RouteTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual RouteTimeSlice& as_route(void);
	virtual const RouteTimeSlice& as_route(void) const;

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
	        IdentTimeSlice::hibernate(ar);
	}
};

class Route : public ObjectImpl<RouteTimeSlice,Object::type_route> {
public:
	typedef Glib::RefPtr<Route> ptr_t;
	typedef Glib::RefPtr<const Route> const_ptr_t;
	typedef ObjectImpl<RouteTimeSlice,Object::type_route> objbase_t;

	Route(const UUID& uuid = UUID());

	ptr_t clone_obj(void) const { ptr_t p(new Route(get_uuid())); clone_impl(p); return p; }
	virtual Object::ptr_t clone(void) const { return clone_obj(); }
};

class RouteSegmentTimeSlice : public SegmentTimeSlice {
public:
	class Availability {
	public:
		Availability(void);

		const Link& get_levels(void) const { return m_levels; }
		void set_levels(const Link& l) { m_levels = l; }

		const TimeTable& get_timetable(void) const { return m_timetable; }
		void set_timetable(const TimeTable& t) { m_timetable = t; }

		const AltRange& get_altrange(void) const { return m_altrange; }
		AltRange& get_altrange(void) { return m_altrange; }
		void set_altrange(const AltRange& ar) { m_altrange = ar; }

		AltRange::alt_t get_lower_mode(void) const { return get_altrange().get_lower_mode(); }
		int32_t get_lower_alt(void) const { return get_altrange().get_lower_alt(); }
		AltRange::alt_t get_upper_mode(void) const { return get_altrange().get_upper_mode(); }
		int32_t get_upper_alt(void) const { return get_altrange().get_upper_alt(); }
		const std::string& get_lower_mode_string(void) const { return get_altrange().get_lower_mode_string(); }
		const std::string& get_upper_mode_string(void) const { return get_altrange().get_upper_mode_string(); }
		bool is_lower_valid(void) const { return get_altrange().is_lower_valid(); }
		bool is_upper_valid(void) const { return get_altrange().is_upper_valid(); }
		bool is_valid(void) const { return get_altrange().is_valid(); }
		int32_t get_lower_alt_if_valid(void) const { return get_altrange().get_lower_alt_if_valid(); }
		int32_t get_upper_alt_if_valid(void) const { return get_altrange().get_upper_alt_if_valid(); }

		typedef enum {
			status_closed      = 0x00,
			status_open        = 0x01,
			status_conditional = 0x02,
			status_invalid     = 0x03
		} status_t;

		status_t get_status(void) const { return (status_t)((m_flags & 0x03) >> 0); }
		void set_status(status_t s) { m_flags ^= (m_flags ^ (s << 0)) & 0x03; }

		unsigned int get_cdr(void) const { return ((m_flags & 0x0C) >> 2); }
		void set_cdr(unsigned int c) { m_flags ^= (m_flags ^ (c << 2)) & 0x0C; }

		bool is_forward(void) const { return !(m_flags & 0x10); }
		void set_forward(void) { m_flags &= ~0x10; }
		bool is_backward(void) const { return !!(m_flags & 0x10); }
		void set_backward(void) { m_flags |= 0x10; }

		std::ostream& print(std::ostream& os, const TimeSlice& ts) const;

		template<class Archive> void hibernate(Archive& ar) {
			ar.io(m_levels);
			m_timetable.hibernate(ar);
			m_altrange.hibernate(ar);
			ar.io(m_flags);
		}

	protected:
		Link m_levels;
		TimeTable m_timetable;
		AltRange m_altrange;
		uint8_t m_flags;
	};

	class Level {
	public:
		Level(void);

		const TimeTable& get_timetable(void) const { return m_timetable; }
		void set_timetable(const TimeTable& t) { m_timetable = t; }

		const AltRange& get_altrange(void) const { return m_altrange; }
		AltRange& get_altrange(void) { return m_altrange; }
		void set_altrange(const AltRange& ar) { m_altrange = ar; }

		std::ostream& print(std::ostream& os, const TimeSlice& ts) const;

		template<class Archive> void hibernate(Archive& ar) {
			m_timetable.hibernate(ar);
			m_altrange.hibernate(ar);
		}

	protected:
		TimeTable m_timetable;
		AltRange m_altrange;
	};

	RouteSegmentTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual timeset_t timediscontinuities(void) const;

	virtual RouteSegmentTimeSlice& as_routesegment(void);
	virtual const RouteSegmentTimeSlice& as_routesegment(void) const;

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

	virtual bool is_forward(void) const;
	virtual bool is_backward(void) const;

	typedef std::vector<Availability> availability_t;
	const availability_t& get_availability(void) const { return m_availability; }
	availability_t& get_availability(void) { return m_availability; }
	void set_availability(const availability_t& a) { m_availability = a; }

	typedef std::vector<Level> levels_t;
	const levels_t& get_levels(void) const { return m_levels; }
	levels_t& get_levels(void) { return m_levels; }
	void set_levels(const levels_t& a) { m_levels = a; }

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		SegmentTimeSlice::hibernate(ar);
		uint32_t sz(m_availability.size());
		ar.ioleb(sz);
		if (ar.is_load())
			m_availability.resize(sz);
		for (availability_t::iterator i(m_availability.begin()), e(m_availability.end()); i != e; ++i)
			i->hibernate(ar);
		sz = m_levels.size();
		ar.ioleb(sz);
		if (ar.is_load())
			m_levels.resize(sz);
		for (levels_t::iterator i(m_levels.begin()), e(m_levels.end()); i != e; ++i)
			i->hibernate(ar);
	}

protected:
	availability_t m_availability;
	levels_t m_levels;
};

class RouteSegment : public ObjectImpl<RouteSegmentTimeSlice,Object::type_routesegment> {
public:
	typedef Glib::RefPtr<RouteSegment> ptr_t;
	typedef Glib::RefPtr<const RouteSegment> const_ptr_t;
	typedef ObjectImpl<RouteSegmentTimeSlice,Object::type_routesegment> objbase_t;

	RouteSegment(const UUID& uuid = UUID());

	ptr_t clone_obj(void) const { ptr_t p(new RouteSegment(get_uuid())); clone_impl(p); return p; }
	virtual Object::ptr_t clone(void) const { return clone_obj(); }
};

class StandardLevelColumnTimeSlice : public TimeSlice {
public:
	StandardLevelColumnTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual StandardLevelColumnTimeSlice& as_standardlevelcolumn(void);
	virtual const StandardLevelColumnTimeSlice& as_standardlevelcolumn(void) const;

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

        const Link& get_leveltable(void) const { return m_leveltable; }
	void set_leveltable(const Link& leveltable) { m_leveltable = leveltable; }

	typedef enum {
		series_even,
		series_odd,
		series_unidirectional,
		series_invalid
	} series_t;

	series_t get_series(void) const { return m_series; }
	void set_series(series_t s) { m_series = s; }

	static bool is_even(series_t s) { return s == series_even; }
	bool is_even(void) const { return is_even(get_series()); }
	static bool is_odd(series_t s) { return s == series_odd; }
	bool is_odd(void) const { return is_odd(get_series()); }
	static bool is_unidirectional(series_t s) { return s == series_unidirectional; }
	bool is_unidirectional(void) const { return is_unidirectional(get_series()); }

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		TimeSlice::hibernate(ar);
		ar.io(m_leveltable);
		ar.iouint8(m_series);
	}

protected:
        Link m_leveltable;
	series_t m_series;
};

class StandardLevelColumn : public ObjectImpl<StandardLevelColumnTimeSlice,Object::type_standardlevelcolumn> {
public:
	typedef Glib::RefPtr<StandardLevelColumn> ptr_t;
	typedef Glib::RefPtr<const StandardLevelColumn> const_ptr_t;
	typedef ObjectImpl<StandardLevelColumnTimeSlice,Object::type_standardlevelcolumn> objbase_t;

	StandardLevelColumn(const UUID& uuid = UUID());

	ptr_t clone_obj(void) const { ptr_t p(new StandardLevelColumn(get_uuid())); clone_impl(p); return p; }
	virtual Object::ptr_t clone(void) const { return clone_obj(); }
};

class StandardLevelTableTimeSlice : public IdentTimeSlice {
public:
	StandardLevelTableTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual StandardLevelTableTimeSlice& as_standardleveltable(void);
	virtual const StandardLevelTableTimeSlice& as_standardleveltable(void) const;

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

        bool is_standardicao(void) const { return m_standardicao; }
	void set_standardicao(bool standardicao) { m_standardicao = !!standardicao; }

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		IdentTimeSlice::hibernate(ar);
		ar.iouint8(m_standardicao);
	}

protected:
        bool m_standardicao;
};

class StandardLevelTable : public ObjectImpl<StandardLevelTableTimeSlice,Object::type_standardleveltable> {
public:
	typedef Glib::RefPtr<StandardLevelTable> ptr_t;
	typedef Glib::RefPtr<const StandardLevelTable> const_ptr_t;
	typedef ObjectImpl<StandardLevelTableTimeSlice,Object::type_standardleveltable> objbase_t;

	StandardLevelTable(const UUID& uuid = UUID());

	ptr_t clone_obj(void) const { ptr_t p(new StandardLevelTable(get_uuid())); clone_impl(p); return p; }
	virtual Object::ptr_t clone(void) const { return clone_obj(); }
};

class RouteSegmentLink : public Link {
public:
        RouteSegmentLink(const UUID& uuid = UUID(), const Glib::RefPtr<Object>& obj = Glib::RefPtr<Object>(), bool backward = false);
        RouteSegmentLink(const Glib::RefPtr<Object>& obj, bool backward = false);
	void set_backward(bool b) { m_backward = b; }
	void set_forward(bool f) { set_backward(!f); }
	bool is_backward(void) const { return m_backward; }
	bool is_forward(void) const { return !is_backward(); }
	int compare(const RouteSegmentLink& x) const;

	template<class Archive> void hibernate(Archive& ar) {
		ar.io(static_cast<Link&>(*this));
		ar.iouint8(m_backward);
	}

protected:
	bool m_backward;
};

};

const std::string& to_str(ADR::StandardLevelColumnTimeSlice::series_t s);
inline std::ostream& operator<<(std::ostream& os, ADR::StandardLevelColumnTimeSlice::series_t s) { return os << to_str(s); }
const std::string& to_str(ADR::StandardInstrumentTimeSlice::status_t s);
inline std::ostream& operator<<(std::ostream& os, ADR::StandardInstrumentTimeSlice::status_t s) { return os << to_str(s); }
const std::string& to_str(ADR::RouteSegmentTimeSlice::Availability::status_t s);
inline std::ostream& operator<<(std::ostream& os, ADR::RouteSegmentTimeSlice::Availability::status_t s) { return os << to_str(s); }

#endif /* ADRROUTE_H */
