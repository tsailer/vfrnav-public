#ifndef ADRPOINT_H
#define ADRPOINT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "adr.hh"

namespace ADR {

class NavaidTimeSlice : public ElevPointIdentTimeSlice {
public:
	NavaidTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual NavaidTimeSlice& as_navaid(void);
	virtual const NavaidTimeSlice& as_navaid(void) const;

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

        const std::string& get_name(void) const { return m_name; }
	void set_name(const std::string& name) { m_name = name; }

	typedef enum {
		type_vor,
		type_vor_dme,
		type_vortac,
		type_tacan,
		type_ils,
		type_ils_dme,
		type_loc,
		type_loc_dme,
		type_dme,
		type_ndb,
		type_ndb_dme,
		type_ndb_mkr,
		type_mkr,
		type_invalid
	} type_t;

	type_t get_type(void) const { return m_type; }
	void set_type(type_t t) { m_type = t; }

	static bool is_vor(type_t t);
	bool is_vor(void) const { return is_vor(get_type()); }
	static bool is_dme(type_t t);
	bool is_dme(void) const { return is_dme(get_type()); }
	static bool is_tacan(type_t t);
	bool is_tacan(void) const { return is_tacan(get_type()); }
	static bool is_ils(type_t t);
	bool is_ils(void) const { return is_ils(get_type()); }
	static bool is_loc(type_t t);
	bool is_loc(void) const { return is_loc(get_type()); }
	static bool is_ndb(type_t t);
	bool is_ndb(void) const { return is_ndb(get_type()); }
	static bool is_mkr(type_t t);
	bool is_mkr(void) const { return is_mkr(get_type()); }

	virtual bool is_navaid(void) const { return true; }
	static bool is_navaid_static(void) { return true; }
	virtual bool get(uint64_t tm, NavaidsDb::Navaid& el) const;

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		ElevPointIdentTimeSlice::hibernate(ar);
		ar.io(m_name);
		ar.iouint8(m_type);
	}

protected:
        std::string m_name;
	type_t m_type;
};

class Navaid : public ObjectImpl<NavaidTimeSlice,Object::type_navaid> {
public:
	typedef Glib::RefPtr<Navaid> ptr_t;
	typedef Glib::RefPtr<const Navaid> const_ptr_t;
	typedef ObjectImpl<NavaidTimeSlice,Object::type_navaid> objbase_t;

	Navaid(const UUID& uuid = UUID());

	ptr_t clone_obj(void) const { ptr_t p(new Navaid(get_uuid())); clone_impl(p); return p; }
	virtual Object::ptr_t clone(void) const { return clone_obj(); }
};

class DesignatedPointTimeSlice : public PointIdentTimeSlice {
public:
	DesignatedPointTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual DesignatedPointTimeSlice& as_designatedpoint(void);
	virtual const DesignatedPointTimeSlice& as_designatedpoint(void) const;

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

        const std::string& get_name(void) const { return m_name; }
	void set_name(const std::string& name) { m_name = name; }

	typedef enum {
		type_icao,
		type_terminal,
		type_coord,
		type_adrboundary,
		type_adrreference,
		type_user,
		type_invalid
	} type_t;

	type_t get_type(void) const { return m_type; }
	void set_type(type_t t) { m_type = t; }

	virtual bool is_intersection(void) const { return true; }
	static bool is_intersection_static(void) { return true; }
	virtual bool get(uint64_t tm, WaypointsDb::Waypoint& el) const;

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		PointIdentTimeSlice::hibernate(ar);
		ar.io(m_name);
		ar.iouint8(m_type);
	}

protected:
        std::string m_name;
	type_t m_type;
};

class DesignatedPoint : public ObjectImpl<DesignatedPointTimeSlice,Object::type_designatedpoint> {
public:
	typedef Glib::RefPtr<DesignatedPoint> ptr_t;
	typedef Glib::RefPtr<const DesignatedPoint> const_ptr_t;
	typedef ObjectImpl<DesignatedPointTimeSlice,Object::type_designatedpoint> objbase_t;

	DesignatedPoint(const UUID& uuid = UUID());

	ptr_t clone_obj(void) const { ptr_t p(new DesignatedPoint(get_uuid())); clone_impl(p); return p; }
	virtual Object::ptr_t clone(void) const { return clone_obj(); }
};

class IndicationTimeSlice : public TimeSlice {
public:
	IndicationTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

	virtual void get_bbox(Rect& bbox) const { bbox = m_bbox; }
	void set_bbox(const Rect& bbox) { m_bbox = bbox; }

	const Link& get_navaid(void) const { return m_navaid; }
        void set_navaid(const Link& l) { m_navaid = l; }
	const Link& get_fix(void) const { return m_fix; }
	void set_fix(const Link& l) { m_fix = l; }

	virtual bool recompute(void);

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		TimeSlice::hibernate(ar);
		ar.io(m_bbox);
		ar.io(m_navaid);
		ar.io(m_fix);
	}

protected:
	Rect m_bbox;
        Link m_navaid;
	Link m_fix;
};

class AngleIndicationTimeSlice : public IndicationTimeSlice {
public:
	AngleIndicationTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual AngleIndicationTimeSlice& as_angleindication(void);
	virtual const AngleIndicationTimeSlice& as_angleindication(void) const;

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

	Point::coord_t get_angle(void) const { return m_angle; }
	void set_angle(Point::coord_t a) { m_angle = a; }

	double get_angle_deg(void) const { return get_angle() * Point::to_deg_dbl; }
	void set_angle_deg(double a) { set_angle(a * Point::from_deg_dbl); }

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		IndicationTimeSlice::hibernate(ar);
		ar.io(m_angle);
	}

protected:
	Point::coord_t m_angle;
};

class AngleIndication : public ObjectImpl<AngleIndicationTimeSlice,Object::type_angleindication> {
public:
	typedef Glib::RefPtr<AngleIndication> ptr_t;
	typedef Glib::RefPtr<const AngleIndication> const_ptr_t;
	typedef ObjectImpl<AngleIndicationTimeSlice,Object::type_angleindication> objbase_t;

	AngleIndication(const UUID& uuid = UUID());

	ptr_t clone_obj(void) const { ptr_t p(new AngleIndication(get_uuid())); clone_impl(p); return p; }
	virtual Object::ptr_t clone(void) const { return clone_obj(); }
};

class DistanceIndicationTimeSlice : public IndicationTimeSlice {
public:
	DistanceIndicationTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual DistanceIndicationTimeSlice& as_distanceindication(void);
	virtual const DistanceIndicationTimeSlice& as_distanceindication(void) const;

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

	int32_t get_dist(void) const { return m_dist; }
	void set_dist(int32_t d) { m_dist = d; }

	double get_dist_nmi(void) const { return ldexp(get_dist(), -16); }
	void set_dist_nmi(double d) { set_dist(ldexp(d, 16)); }

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		IndicationTimeSlice::hibernate(ar);
		ar.io(m_dist);
	}

protected:
        int32_t m_dist;
};

class DistanceIndication : public ObjectImpl<DistanceIndicationTimeSlice,Object::type_distanceindication> {
public:
	typedef Glib::RefPtr<DistanceIndication> ptr_t;
	typedef Glib::RefPtr<const DistanceIndication> const_ptr_t;
	typedef ObjectImpl<DistanceIndicationTimeSlice,Object::type_distanceindication> objbase_t;

	DistanceIndication(const UUID& uuid = UUID());

	ptr_t clone_obj(void) const { ptr_t p(new DistanceIndication(get_uuid())); clone_impl(p); return p; }
	virtual Object::ptr_t clone(void) const { return clone_obj(); }
};

};

const std::string& to_str(ADR::NavaidTimeSlice::type_t t);
inline std::ostream& operator<<(std::ostream& os, ADR::NavaidTimeSlice::type_t t) { return os << to_str(t); }
const std::string& to_str(ADR::DesignatedPointTimeSlice::type_t t);
inline std::ostream& operator<<(std::ostream& os, ADR::DesignatedPointTimeSlice::type_t t) { return os << to_str(t); }

#endif /* ADRPOINT_H */
