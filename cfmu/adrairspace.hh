#ifndef ADRAIRSPACE_H
#define ADRAIRSPACE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "adr.hh"
#include "adrtimetable.hh"

namespace ADR {

class AirspaceTimeSlice : public IdentTimeSlice {
public:
	class Trace {
	public:
		static const unsigned int nocomponent = ~0U;

		typedef enum {
			reason_inside,
			reason_intersect,
			reason_outside,
			reason_outsidebbox,
			reason_nointersect,
			reason_border,
			reason_outsidetime,
			reason_outsidetimeslice,
			reason_altrange
		} reason_t;

		Trace(const Link& aspc = Link(), unsigned int comp = nocomponent, reason_t reason = reason_inside)
			: m_airspace(aspc), m_component(comp), m_reason(reason) {}

		const Link& get_airspace(void) const { return m_airspace; }
		unsigned int get_component(void) const { return m_component; }
		reason_t get_reason(void) const { return m_reason; }
		operator bool(void) const { return get_reason() == reason_inside || get_reason() == reason_intersect; }

	protected:
		Link m_airspace;
		unsigned int m_component;
		reason_t m_reason;
	};

	class Component {
	public:
		class PointLink {
		public:
			PointLink(const Link& l = Link(), uint16_t poly = 0, uint16_t ring = 0, uint32_t index = 0);
			const Link& get_link(void) const { return m_link; }
			void set_link(const Link& l) { m_link = l; }
			uint16_t get_poly(void) const { return m_poly; }
			void set_poly(uint16_t p = 0) { m_poly = p; }
			uint16_t get_ring(void) const { return m_ring; }
			void set_ring(uint16_t r) { m_ring = r; }
			uint16_t get_index(void) const { return m_index; }
			void set_index(uint16_t i) { m_index = i; }

			static const uint16_t exterior;

			bool is_exterior(void) const { return get_ring() == exterior; }
			void set_exterior(void) { set_ring(exterior); }

			template<class Archive> void hibernate(Archive& ar) {
				ar.io(m_link);
				ar.io(m_poly);
				ar.io(m_ring);
				ar.io(m_index);
			}

		protected:
			Link m_link;
			uint16_t m_poly;
			uint16_t m_ring;
			uint16_t m_index;
		};

		Component(void);

		typedef enum {
			operator_base,
			operator_union,
			operator_invalid
		} operator_t;

		operator_t get_operator(void) const { return (operator_t)(m_flags & 0x03); }
		void set_operator(operator_t t) { m_flags ^= (m_flags ^ t) & 0x03; }

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

		bool is_full_geometry(void) const { return !!(m_flags & 0x10); }
		void set_full_geometry(bool g = true) { if (g) m_flags |= 0x10; else m_flags &= ~0x10; }

		typedef ElevPointIdentTimeSlice::elev_t elev_t;

		elev_t get_gndelevmin(void) const { return m_gndelevmin; }
		void set_gndelevmin(elev_t e = ElevPointIdentTimeSlice::invalid_elev) { m_gndelevmin = e; }
		bool is_gndelevmin_valid(void) const { return get_gndelevmin() != ElevPointIdentTimeSlice::invalid_elev; }
		elev_t get_gndelevmax(void) const { return m_gndelevmax; }
		void set_gndelevmax(elev_t e = ElevPointIdentTimeSlice::invalid_elev) { m_gndelevmax = e; }
		bool is_gndelevmax_valid(void) const { return get_gndelevmax() != ElevPointIdentTimeSlice::invalid_elev; }

		const Link& get_airspace(void) const { return m_airspace; }
		void set_airspace(const Link& airspace) { m_airspace = airspace; }

		const MultiPolygonHole& get_poly(void) const { return m_poly; }
		MultiPolygonHole& get_poly(void) { return m_poly; }
		void set_poly(const MultiPolygonHole& p) { m_poly = p; }
		bool is_poly(void) const { return !m_poly.empty(); }

		typedef std::vector<PointLink> pointlink_t;
		const pointlink_t& get_pointlink(void) const { return m_pointlink; }
		pointlink_t& get_pointlink(void) { return m_pointlink; }
		void set_pointlink(const pointlink_t& p) { m_pointlink = p; }
		bool is_pointlink(const UUID& uuid = UUID()) const;

		MultiPolygonHole get_full_poly(const TimeSlice& ts) const;
		MultiPolygonHole get_full_poly(uint64_t tm) const;

		bool is_inside(const Point& pt, const timepair_t& tint, int32_t alt = AltRange::altignore, const AltRange& altrange = AltRange(), const UUID& uuid = UUID()) const;
		bool is_inside(const TimeTableEval& tte, int32_t alt = AltRange::altignore, const AltRange& altrange = AltRange(), const UUID& uuid = UUID()) const;
		bool is_intersect(const TimeTableEval& tte, const Point& pt1, int32_t alt = AltRange::altignore, const AltRange& altrange = AltRange()) const;
		bool is_intersect(const TimeTableEval& tte, const Point& pt1, int32_t alt0, int32_t alt1, const AltRange& altrange = AltRange()) const;
		IntervalSet<int32_t> get_point_altitudes(const TimeTableEval& tte, const AltRange& altrange = AltRange(), const UUID& uuid = UUID()) const;
		IntervalSet<int32_t> get_intersect_altitudes(const TimeTableEval& tte, const Point& pt1, const AltRange& altrange = AltRange()) const;
		IntervalSet<int32_t> get_point_intersect_altitudes(const TimeTableEval& tte, const Point& pt1, const AltRange& altrange = AltRange(), const UUID& uuid0 = UUID(), const UUID& uuid1 = UUID()) const;
		std::vector<Trace> trace_inside(const TimeTableEval& tte, int32_t alt, const AltRange& altrange, const UUID& uuid, const Object::ptr_t& aspc, unsigned int comp) const;
		std::vector<Trace> trace_intersect(const TimeTableEval& tte, const Point& pt1, int32_t alt, const AltRange& altrange, const Object::ptr_t& aspc, unsigned int comp) const;

		bool is_altitude_overlap(int32_t alt0, int32_t alt1, const timepair_t& tm, const AltRange& altrange = AltRange()) const;

		bool recompute(const TimeSlice& ts);
		bool recompute(const TimeSlice& ts, TopoDb30& topodb);

		std::ostream& print(std::ostream& os, const TimeSlice& ts) const;

		template<class Archive> void hibernate(Archive& ar) {
			ar.io(m_airspace);
			ar.io(m_flags);
			m_altrange.hibernate(ar);
			m_poly.hibernate_binary(ar);
			ar.io(m_gndelevmin);
			ar.io(m_gndelevmax);
			uint32_t sz(m_pointlink.size());
			ar.ioleb(sz);
			if (ar.is_load())
				m_pointlink.resize(sz);
			for (pointlink_t::iterator i(m_pointlink.begin()), e(m_pointlink.end()); i != e; ++i)
				i->hibernate(ar);
		}

	protected:
		Link m_airspace;
		MultiPolygonHole m_poly;
		pointlink_t m_pointlink;
		AltRange m_altrange;
		elev_t m_gndelevmin;
		elev_t m_gndelevmax;
		uint8_t m_flags;
	};

	AirspaceTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual AirspaceTimeSlice& as_airspace(void);
	virtual const AirspaceTimeSlice& as_airspace(void) const;

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

	const TimeTable& get_timetable(void) const { return m_timetable; }
	void set_timetable(const TimeTable& tt) { m_timetable = tt; }

	virtual void get_bbox(Rect& bbox) const { bbox = m_bbox; }
	void set_bbox(const Rect& bbox) { m_bbox = bbox; }

	typedef enum {
		type_atz,
		type_cba,
		type_cta,
		type_cta_p,
		type_ctr,
		type_d,
		type_d_other,
		type_fir,
		type_fir_p,
		type_nas,
		type_oca,
		type_adr_auag,
		type_adr_fab,
		type_p,
		type_part,
		type_r,
		type_sector,
		type_sector_c,
		type_tma,
		type_tra,
		type_tsa,
		type_uir,
		type_uta,
		type_border,
		type_invalid
	} type_t;

	type_t get_type(void) const { return m_type; }
	void set_type(type_t t) { m_type = t; }

	typedef enum {
		localtype_none,
		localtype_mra,
		localtype_mta
	} localtype_t;

	localtype_t get_localtype(void) const { return (localtype_t)(m_flags & 0x03); }
	void set_localtype(localtype_t t) { m_flags ^= (m_flags ^ t) & 0x03; }

	bool is_icao(void) const { return !!(m_flags & 0x04); }
	void set_icao(bool icao = true) { if (icao) m_flags |= 0x04; else m_flags &= ~0x04; }

	bool is_flexibleuse(void) const { return !!(m_flags & 0x08); }
	void set_flexibleuse(bool fu = true) { if (fu) m_flags |= 0x08; else m_flags &= ~0x08; }

	bool is_level(unsigned int l) const { return !!(m_flags & 0x70 & (0x08 << l)); }
	void set_level(unsigned int l, bool x = true) { m_flags ^= (m_flags ^ -!!x) & 0x70 & (0x08 << l); }

	typedef Component::elev_t elev_t;

	elev_t get_gndelevmin(void) const;
	bool is_gndelevmin_valid(void) const { return get_gndelevmin() != ElevPointIdentTimeSlice::invalid_elev; }
	elev_t get_gndelevmax(void) const;
	bool is_gndelevmax_valid(void) const { return get_gndelevmax() != ElevPointIdentTimeSlice::invalid_elev; }

	typedef std::vector<Component> components_t;
	const components_t& get_components(void) const { return m_components; }
	components_t& get_components(void) { return m_components; }
	void set_components(const components_t& c) { m_components = c; }

	MultiPolygonHole get_full_poly(void) const;
	MultiPolygonHole get_full_poly(uint64_t tm) const;

	static char get_airspace_bdryclass(type_t t);
	char get_airspace_bdryclass(void) const { return get_airspace_bdryclass(get_type()); }

	virtual bool is_airspace(void) const { return true; }
	static bool is_airspace_static(void) { return true; }
	virtual bool get(uint64_t tm, AirspacesDb::Airspace& el) const;

	using IdentTimeSlice::is_inside;
	bool is_inside(const Point& pt, int32_t alt = AltRange::altignore, const AltRange& altrange = AltRange(), const UUID& uuid = UUID()) const;
	bool is_inside(const TimeTableEval& tte, int32_t alt = AltRange::altignore, const AltRange& altrange = AltRange(), const UUID& uuid = UUID()) const;
	bool is_intersect(const TimeTableEval& tte, const Point& pt1, int32_t alt = AltRange::altignore, const AltRange& altrange = AltRange()) const;
	bool is_intersect(const TimeTableEval& tte, const Point& pt1, int32_t alt0, int32_t alt1, const AltRange& altrange = AltRange()) const;
	IntervalSet<int32_t> get_point_altitudes(const TimeTableEval& tte, const AltRange& altrange = AltRange(), const UUID& uuid = UUID()) const;
	IntervalSet<int32_t> get_intersect_altitudes(const TimeTableEval& tte, const Point& pt1, const AltRange& altrange = AltRange()) const;
	IntervalSet<int32_t> get_point_intersect_altitudes(const TimeTableEval& tte, const Point& pt1, const AltRange& altrange = AltRange(), const UUID& uuid0 = UUID(), const UUID& uuid1 = UUID()) const;
	std::vector<Trace> trace_inside(const TimeTableEval& tte, int32_t alt, const AltRange& altrange, const UUID& uuid, const Object::ptr_t& aspc) const;
	std::vector<Trace> trace_intersect(const TimeTableEval& tte, const Point& pt1, int32_t alt, const AltRange& altrange, const Object::ptr_t& aspc) const;

	bool is_altitude_overlap(int32_t alt0, int32_t alt1, const timepair_t& tm, const AltRange& altrange = AltRange()) const;

	virtual bool recompute(void);
	virtual bool recompute(TopoDb30& topodb);

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		IdentTimeSlice::hibernate(ar);
		m_timetable.hibernate(ar);
		ar.io(m_bbox);
		ar.iouint8(m_type);
		ar.iouint8(m_flags);
		uint32_t sz(m_components.size());
		ar.ioleb(sz);
		if (ar.is_load())
			m_components.resize(sz);
		for (components_t::iterator i(m_components.begin()), e(m_components.end()); i != e; ++i)
			i->hibernate(ar);
	}

protected:
	components_t m_components;
	TimeTable m_timetable;
	Rect m_bbox;
	type_t m_type;
	uint8_t m_flags;
};

class Airspace : public ObjectImpl<AirspaceTimeSlice,Object::type_airspace> {
public:
	typedef Glib::RefPtr<Airspace> ptr_t;
	typedef Glib::RefPtr<const Airspace> const_ptr_t;
	typedef ObjectImpl<AirspaceTimeSlice,Object::type_airspace> objbase_t;

	Airspace(const UUID& uuid = UUID());

	ptr_t clone_obj(void) const { ptr_t p(new Airspace(get_uuid())); clone_impl(p); return p; }
	virtual Object::ptr_t clone(void) const { return clone_obj(); }

	bool is_inside(const TimeTableEval& tte, int32_t alt = AltRange::altignore, const AltRange& altrange = AltRange(), const UUID& uuid = UUID()) const;
	bool is_intersect(const TimeTableEval& tte, const Point& pt1, int32_t alt = AltRange::altignore, const AltRange& altrange = AltRange()) const;
	bool is_intersect(const TimeTableEval& tte, const Point& pt1, int32_t alt0, int32_t alt1, const AltRange& altrange = AltRange()) const;
	IntervalSet<int32_t> get_point_altitudes(const TimeTableEval& tte, const AltRange& altrange = AltRange(), const UUID& uuid = UUID()) const;
	IntervalSet<int32_t> get_intersect_altitudes(const TimeTableEval& tte, const Point& pt1, const AltRange& altrange = AltRange()) const;
	IntervalSet<int32_t> get_point_intersect_altitudes(const TimeTableEval& tte, const Point& pt1, const AltRange& altrange = AltRange(), const UUID& uuid0 = UUID(), const UUID& uuid1 = UUID()) const;
	std::vector<AirspaceTimeSlice::Trace> trace_inside(const TimeTableEval& tte, int32_t alt = AltRange::altignore, const AltRange& altrange = AltRange(), const UUID& uuid = UUID()) const;
	std::vector<AirspaceTimeSlice::Trace> trace_intersect(const TimeTableEval& tte, const Point& pt1, int32_t alt = AltRange::altignore, const AltRange& altrange = AltRange()) const;

	bool is_altitude_overlap(int32_t alt0, int32_t alt1, const timepair_t& tm, const AltRange& altrange = AltRange()) const;
};

};

const std::string& to_str(ADR::AirspaceTimeSlice::type_t t);
inline std::ostream& operator<<(std::ostream& os, ADR::AirspaceTimeSlice::type_t t) { return os << to_str(t); }
const std::string& to_str(ADR::AirspaceTimeSlice::localtype_t t);
inline std::ostream& operator<<(std::ostream& os, ADR::AirspaceTimeSlice::localtype_t t) { return os << to_str(t); }
const std::string& to_str(ADR::AirspaceTimeSlice::Component::operator_t o);
inline std::ostream& operator<<(std::ostream& os, ADR::AirspaceTimeSlice::Component::operator_t o) { return os << to_str(o); }
const std::string& to_str(ADR::AirspaceTimeSlice::Trace::reason_t r);
inline std::ostream& operator<<(std::ostream& os, ADR::AirspaceTimeSlice::Trace::reason_t r) { return os << to_str(r); }

#endif /* ADRAIRSPACE_H */
