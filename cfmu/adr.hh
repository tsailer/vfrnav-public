#ifndef ADR_H
#define ADR_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <limits>
#include <boost/config.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/logic/tribool_io.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/nil_generator.hpp>

#include "sysdeps.h"
#include "aircraft.h"
#include "fplan.h"
#include "dbobj.h"
#include "engine.h"
#include "interval.hh"

class TopoDb30;

namespace ADR {

class Object;
class Database;
template <typename TS> class DependencyGenerator;
class LinkLoader;
class ArchiveReadStream;
class ArchiveReadBuffer;
class ArchiveWriteStream;
class IdentTimeSlice;
class PointIdentTimeSlice;
class ElevPointIdentTimeSlice;
class AirportTimeSlice;
class AirportCollocationTimeSlice;
class OrganisationAuthorityTimeSlice;
class AirTrafficManagementServiceTimeSlice;
class UnitTimeSlice;
class SpecialDateTimeSlice;
class NavaidTimeSlice;
class DesignatedPointTimeSlice;
class AngleIndicationTimeSlice;
class DistanceIndicationTimeSlice;
class SegmentTimeSlice;
class StandardInstrumentTimeSlice;
class StandardInstrumentDepartureTimeSlice;
class StandardInstrumentArrivalTimeSlice;
class DepartureLegTimeSlice;
class ArrivalLegTimeSlice;
class RouteTimeSlice;
class RouteSegmentTimeSlice;
class StandardLevelColumnTimeSlice;
class StandardLevelTableTimeSlice;
class AirspaceTimeSlice;
class FlightRestrictionTimeSlice;
class TimeTableEval;
class TimeTableSpecialDateEval;
class TimeTable;
class TimeTableWeekdayPattern;

typedef uint64_t timetype_t;
typedef std::set<timetype_t> timeset_t;
typedef std::pair<timetype_t,timetype_t> timepair_t;
typedef IntervalSet<timetype_t> timeinterval_t;

class UUID : public boost::uuids::uuid {
public:
	typedef std::set<UUID> set_t;

	UUID(void);
	UUID(const boost::uuids::uuid& uuid);
	UUID(const boost::uuids::uuid& uuid, const std::string& t);
	UUID(uint32_t w0, uint32_t w1, uint32_t w2, uint32_t w3);
	static const UUID niluuid;
	static UUID from_str(const std::string& x);
	static UUID from_str(const UUID& uuid, const std::string& x);
	static UUID from_countryborder(const std::string& x);
	std::string to_str(bool prefix = false) const;
	void dependencies(UUID::set_t& dep) const;
	uint32_t get_word(unsigned int nr) const;
	int compare(const UUID& x) const;

	template<class Archive> void hibernate(Archive& ar) {
		for (iterator i(begin()), e(end()); i != e; ++i)
			ar.iouint8(*i);
	}
};

class Link : public UUID {
public:
	class LinkSet : public std::set<Link> {
	public:
		typedef std::set<Link> base_t;
		LinkSet(const base_t& x = base_t());
		LinkSet(const LinkSet& x);

		std::pair<iterator,bool> insert(const Link& link);
		bool has_unlinked(void) const { return m_unlinked; }

	protected:
		bool m_unlinked;
	};

	Link(const UUID& uuid = UUID(), const Glib::RefPtr<Object>& obj = Glib::RefPtr<Object>());
	Link(const Glib::RefPtr<Object>& obj);
	const Glib::RefPtr<Object>& get_obj(void) const { return m_obj; }
	const Glib::RefPtr<const Object>& get_const_obj(void) const { return reinterpret_cast<const Glib::RefPtr<const Object>&>(get_obj()); }
	void set_obj(const Glib::RefPtr<Object>& obj) { m_obj = obj; }
	void load(Database& db);
	using UUID::dependencies;
	void dependencies(LinkSet& dep) const;

protected:
	Glib::RefPtr<Object> m_obj;
};

class TimeSlice {
public:
	TimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	timetype_t get_starttime(void) const { return m_time.first; }
	timetype_t get_endtime(void) const { return m_time.second; }
	const timepair_t& get_timeinterval(void) const { return m_time; }
	void set_starttime(timetype_t t) { m_time.first = t; }
	void set_endtime(timetype_t t) { m_time.second = t; }
	bool is_valid(void) const { return get_starttime() < get_endtime(); }
	bool is_inside(timetype_t tm) const { return get_starttime() <= tm && tm < get_endtime(); }
	bool is_overlap(timetype_t tm0, timetype_t tm1) const;
	bool is_overlap(const TimeSlice& ts) const { return is_overlap(ts.get_starttime(), ts.get_endtime()); }
	bool is_overlap(const timepair_t& tm) const { return is_overlap(tm.first, tm.second); }
	timetype_t get_overlap(timetype_t tm0, timetype_t tm1) const;
	timetype_t get_overlap(const TimeSlice& ts) const { return get_overlap(ts.get_starttime(), ts.get_endtime()); }
	timetype_t get_overlap(const timepair_t& tm) const { return get_overlap(tm.first, tm.second); }

	virtual void get_bbox(Rect& bbox) const;

	virtual bool is_airport(void) const { return false; }
	virtual bool is_navaid(void) const { return false; }
	virtual bool is_intersection(void) const { return false; }
	virtual bool is_airway(void) const { return false; }
	virtual bool is_airspace(void) const { return false; }

	static bool is_airport_static(void) { return false; }
	static bool is_navaid_static(void) { return false; }
	static bool is_intersection_static(void) { return false; }
	static bool is_airway_static(void) { return false; }
	static bool is_airspace_static(void) { return false; }

	virtual bool get(timetype_t tm, AirportsDb::Airport& el) const { return false; }
	virtual bool get(timetype_t tm, NavaidsDb::Navaid& el) const { return false; }
	virtual bool get(timetype_t tm, WaypointsDb::Waypoint& el) const { return false; }
	virtual bool get(timetype_t tm, AirwaysDb::Airway& el) const { return false; }
	virtual bool get(timetype_t tm, AirspacesDb::Airspace& el) const { return false; }

	virtual void clone(void) {}
	virtual bool recompute(void) { return false; }
	virtual bool recompute(TopoDb30& topodb) { return recompute(); }
	virtual bool recompute(Database& db) { return false; }

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;
	virtual timeset_t timediscontinuities(void) const;

	virtual IdentTimeSlice& as_ident(void);
	virtual const IdentTimeSlice& as_ident(void) const;
	virtual PointIdentTimeSlice& as_point(void);
	virtual const PointIdentTimeSlice& as_point(void) const;
	virtual ElevPointIdentTimeSlice& as_elevpoint(void);
	virtual const ElevPointIdentTimeSlice& as_elevpoint(void) const;
	virtual AirportTimeSlice& as_airport(void);
	virtual const AirportTimeSlice& as_airport(void) const;
	virtual AirportCollocationTimeSlice& as_airportcollocation(void);
	virtual const AirportCollocationTimeSlice& as_airportcollocation(void) const;
	virtual OrganisationAuthorityTimeSlice& as_organisationauthority(void);
	virtual const OrganisationAuthorityTimeSlice& as_organisationauthority(void) const;
	virtual AirTrafficManagementServiceTimeSlice& as_airtrafficmanagementservice(void);
	virtual const AirTrafficManagementServiceTimeSlice& as_airtrafficmanagementservice(void) const;
	virtual UnitTimeSlice& as_unit(void);
	virtual const UnitTimeSlice& as_unit(void) const;
	virtual SpecialDateTimeSlice& as_specialdate(void);
	virtual const SpecialDateTimeSlice& as_specialdate(void) const;
	virtual NavaidTimeSlice& as_navaid(void);
	virtual const NavaidTimeSlice& as_navaid(void) const;
	virtual DesignatedPointTimeSlice& as_designatedpoint(void);
	virtual const DesignatedPointTimeSlice& as_designatedpoint(void) const;
	virtual AngleIndicationTimeSlice& as_angleindication(void);
	virtual const AngleIndicationTimeSlice& as_angleindication(void) const;
	virtual DistanceIndicationTimeSlice& as_distanceindication(void);
	virtual const DistanceIndicationTimeSlice& as_distanceindication(void) const;
	virtual SegmentTimeSlice& as_segment(void);
	virtual const SegmentTimeSlice& as_segment(void) const;
	virtual StandardInstrumentTimeSlice& as_sidstar(void);
	virtual const StandardInstrumentTimeSlice& as_sidstar(void) const;
	virtual StandardInstrumentDepartureTimeSlice& as_sid(void);
	virtual const StandardInstrumentDepartureTimeSlice& as_sid(void) const;
	virtual StandardInstrumentArrivalTimeSlice& as_star(void);
	virtual const StandardInstrumentArrivalTimeSlice& as_star(void) const;
	virtual DepartureLegTimeSlice& as_departureleg(void);
	virtual const DepartureLegTimeSlice& as_departureleg(void) const;
	virtual ArrivalLegTimeSlice& as_arrivalleg(void);
	virtual const ArrivalLegTimeSlice& as_arrivalleg(void) const;
	virtual RouteTimeSlice& as_route(void);
	virtual const RouteTimeSlice& as_route(void) const;
	virtual RouteSegmentTimeSlice& as_routesegment(void);
	virtual const RouteSegmentTimeSlice& as_routesegment(void) const;
	virtual StandardLevelColumnTimeSlice& as_standardlevelcolumn(void);
	virtual const StandardLevelColumnTimeSlice& as_standardlevelcolumn(void) const;
	virtual StandardLevelTableTimeSlice& as_standardleveltable(void);
	virtual const StandardLevelTableTimeSlice& as_standardleveltable(void) const;
	virtual AirspaceTimeSlice& as_airspace(void);
	virtual const AirspaceTimeSlice& as_airspace(void) const;
	virtual FlightRestrictionTimeSlice& as_flightrestriction(void);
	virtual const FlightRestrictionTimeSlice& as_flightrestriction(void) const;

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		ar.iouint64(m_time.first);
		ar.iouint64(m_time.second);
	}

protected:
	timepair_t m_time;

	void constraintimediscontinuities(timeset_t& tdisc) const;
};

class Object {
public:
	typedef Glib::RefPtr<Object> ptr_t;
	typedef Glib::RefPtr<const Object> const_ptr_t;

	Object(const UUID& uuid = UUID());
	virtual ~Object();

	void reference(void) const;
	void unreference(void) const;
	gint get_refcount(void) const { return m_refcount; }
	ptr_t get_ptr(void) { reference(); return ptr_t(this); }
	const_ptr_t get_ptr(void) const { reference(); return const_ptr_t(this); }
	template<class T> typename T::ptr_t as(void) { return T::ptr_t::cast_dynamic(get_ptr()); }
	template<class T> typename T::const_ptr_t as(void) const { return T::ptr_const_t::cast_dynamic(get_ptr()); }
	template<class T> bool is(void) const { return !!dynamic_cast<T *>(this); }
	template<class T> static typename T::ptr_t as(const ptr_t& p) { return T::ptr_const_t::cast_dynamic(p); }
	template<class T> static typename T::const_ptr_t as(const const_ptr_t& p) { return T::ptr_const_t::cast_dynamic(p); }
	virtual ptr_t clone(void) const = 0;
	virtual void link(Database& db, unsigned int depth = ~0U) = 0;
	virtual void recompute(void);
	virtual void recompute(TopoDb30& topodb);
	virtual void recompute(Database& db);

	const UUID& get_uuid(void) const { return m_uuid; }

	virtual void dependencies(UUID::set_t& dep) const = 0;
	virtual void dependencies(Link::LinkSet& dep) const = 0;
	virtual bool is_unlinked(void) const;
	virtual timeset_t timediscontinuities(void) const;

	typedef enum {
		type_first                        = 0x00,
		type_nongeographicstart           = 0x00,
		type_nongeographicend             = 0x3F,
		type_pointstart                   = 0x40,
		type_pointend                     = 0x7F,
		type_linestart                    = 0x80,
		type_lineend                      = 0xBF,
		type_areastart                    = 0xC0,
		type_areaend                      = 0xFF,
		type_airportcollocation           = type_first,
		type_organisationauthority,
		type_airtrafficmanagementservice,
		type_unit,
		type_specialdate,
		type_angleindication,
		type_distanceindication,
		type_sid,
		type_star,
		type_route,
		type_standardlevelcolumn,
		type_standardleveltable,
		type_flightrestriction,
		type_invalid                      = 0x3F,
		type_airport                      = 0x40, // encodes flags in lower 4 bits
		type_airportend                   = 0x4F,
		type_navaid,
		type_designatedpoint,
		type_departureleg                 = 0x80,
		type_arrivalleg,
		type_routesegment,
		type_airspace                     = 0xC0,
		type_last                         = 0xFF
	} type_t;

	virtual type_t get_type(void) const = 0;
	static const std::string& get_type_name(type_t t);
	const std::string& get_type_name(void) const { return get_type_name(get_type()); }
	static const std::string& get_long_type_name(type_t t);
	const std::string& get_long_type_name(void) const { return get_long_type_name(get_type()); }
	static ptr_t create(type_t t = type_invalid, const UUID& uuid = UUID());
	static ptr_t create(ArchiveReadStream& ar, const UUID& uuid = UUID());
	static ptr_t create(ArchiveReadBuffer& ar, const UUID& uuid = UUID());
	virtual void load(ArchiveReadStream& ar) = 0;
	virtual void load(ArchiveReadBuffer& ar) = 0;
	virtual void save(ArchiveWriteStream& ar) const = 0;

	bool empty(void) const;
	unsigned int size(void) const;
	TimeSlice& operator[](unsigned int idx);
	const TimeSlice& operator[](unsigned int idx) const;
	// find timeslice containing given time
	TimeSlice& operator()(timetype_t tm);
	const TimeSlice& operator()(timetype_t tm) const;
	// find best overlap
	TimeSlice& operator()(timetype_t tm0, timetype_t tm1);
	const TimeSlice& operator()(timetype_t tm0, timetype_t tm1) const;
	TimeSlice& operator()(const TimeSlice& ts);
	const TimeSlice& operator()(const TimeSlice& ts) const;

	timepair_t get_timebounds(void) const;
	timeinterval_t get_timeintervals(void) const;
	bool has_overlap(timetype_t tm0, timetype_t tm1) const;
	bool has_overlap(const TimeSlice& ts) const { return has_overlap(ts.get_starttime(), ts.get_endtime()); }
	bool has_overlap(const timepair_t& tm) const { return has_overlap(tm.first, tm.second); }
	bool has_nonoverlap(timetype_t tm0, timetype_t tm1) const;
	bool has_nonoverlap(const TimeSlice& ts) const { return has_nonoverlap(ts.get_starttime(), ts.get_endtime()); }
	bool has_nonoverlap(const timepair_t& tm) const { return has_nonoverlap(tm.first, tm.second); }
	virtual ptr_t simplify_time(timetype_t tm0, timetype_t tm1) const = 0;
	ptr_t simplify_time(const TimeSlice& ts) const { return simplify_time(ts.get_starttime(), ts.get_endtime()); }
	ptr_t simplify_time(const timepair_t& tm) const { return simplify_time(tm.first, tm.second); }

	timetype_t get_modified(void) const { return m_modified; }
	void set_modified(timetype_t m);
	void set_modified(void);

	bool is_dirty(void) const { return m_dirty; }
	void set_dirty(bool d = true) const { m_dirty = d; }
	void unset_dirty(void) const { set_dirty(false); }

	virtual void get_bbox(Rect& bbox) const;

	virtual bool is_airport(void) const { return false; }
	virtual bool is_navaid(void) const { return false; }
	virtual bool is_intersection(void) const { return false; }
	virtual bool is_airway(void) const { return false; }
	virtual bool is_airspace(void) const { return false; }

	bool get(timetype_t tm, AirportsDb::Airport& el) const { return (*this)(tm).get(tm, el); }
	bool get(timetype_t tm, NavaidsDb::Navaid& el) const { return (*this)(tm).get(tm, el); }
	bool get(timetype_t tm, WaypointsDb::Waypoint& el) const { return (*this)(tm).get(tm, el); }
	bool get(timetype_t tm, AirwaysDb::Airway& el) const { return (*this)(tm).get(tm, el); }
	bool get(timetype_t tm, AirspacesDb::Airspace& el) const { return (*this)(tm).get(tm, el); }

	bool set(timetype_t tm, FPlanWaypoint& el) const;
	unsigned int insert(timetype_t tm, FPlanRoute& route, uint32_t nr = ~0U) const;

	virtual void clean_timeslices(timetype_t cutoff = 0) = 0;

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		if (!ar.is_dbblob()) {
			ar.io(m_uuid);
			ar.iouint64(m_modified);
		}
	}

	static const TimeSlice invalid_timeslice;
	static const IdentTimeSlice invalid_identtimeslice;
	static const PointIdentTimeSlice invalid_pointidenttimeslice;
	static const ElevPointIdentTimeSlice invalid_elevpointidenttimeslice;
	static const SegmentTimeSlice invalid_segmenttimeslice;
	static const StandardInstrumentTimeSlice invalid_standardinstrumenttimeslice;

protected:
	UUID m_uuid;
	timetype_t m_modified;
	mutable gint m_refcount;
	mutable bool m_dirty;

	virtual bool ts_empty(void) const { return true; }
	virtual unsigned int ts_size(void) const { return 0; }
	virtual TimeSlice& ts_get(unsigned int idx);
	virtual const TimeSlice& ts_get(unsigned int idx) const;
};

template<class TS = TimeSlice, int TYP = Object::type_invalid> class ObjectImpl : public Object {
public:
	typedef TS TimeSliceImpl;
	typedef Glib::RefPtr<ObjectImpl> ptr_t;
	typedef Glib::RefPtr<const ObjectImpl> const_ptr_t;
	typedef ObjectImpl objtype_t;
	typedef Object objbase_t;

	ObjectImpl(const UUID& uuid = UUID());

	ptr_t clone_obj(void) const { ptr_t p(new ObjectImpl(get_uuid())); clone_impl(p); return p; }
	virtual Object::ptr_t clone(void) const { return clone_obj(); }

	virtual void link(Database& db, unsigned int depth = ~0U);

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

	static type_t get_static_type(void) { return (type_t)TYP; }
	virtual type_t get_type(void) const { return (type_t)TYP; }

	virtual bool is_airport(void) const { return TimeSliceImpl::is_airport_static(); }
	virtual bool is_navaid(void) const { return TimeSliceImpl::is_navaid_static(); }
	virtual bool is_intersection(void) const { return TimeSliceImpl::is_intersection_static(); }
	virtual bool is_airway(void) const { return TimeSliceImpl::is_airway_static(); }
	virtual bool is_airspace(void) const { return TimeSliceImpl::is_airspace_static(); }

	virtual void load(ArchiveReadStream& ar);
	virtual void load(ArchiveReadBuffer& ar);
	virtual void save(ArchiveWriteStream& ar) const;

	virtual void clean_timeslices(timetype_t cutoff = 0);
	void add_timeslice(const TimeSliceImpl& ts);
	virtual Object::ptr_t simplify_time(timetype_t tm0, timetype_t tm1) const;

	template<class Archive> void hibernate(Archive& ar) {
		Object::hibernate(ar);
		uint32_t sz(size());
		ar.ioleb(sz);
		if (ar.is_load())
			m_timeslice.resize(sz);
		for (typename timeslice_t::iterator i(m_timeslice.begin()), e(m_timeslice.end()); i != e; ++i)
			i->hibernate(ar);
	}

	static const TimeSliceImpl invalid_timeslice;

protected:
	class SortTimeSlice;

	typedef std::vector<TimeSliceImpl> timeslice_t;
	timeslice_t m_timeslice;

	virtual bool ts_empty(void) const { return m_timeslice.empty(); }
	virtual unsigned int ts_size(void) const { return m_timeslice.size(); }
	virtual TimeSlice& ts_get(unsigned int idx);
	virtual const TimeSlice& ts_get(unsigned int idx) const;
	void clone_impl(const ptr_t& x) const;
};

class IdentTimeSlice : public TimeSlice {
public:
	IdentTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual IdentTimeSlice& as_ident(void);
	virtual const IdentTimeSlice& as_ident(void) const;

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

	const std::string& get_ident(void) const { return m_ident; }
	void set_ident(const std::string& id) { m_ident = id; }

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		TimeSlice::hibernate(ar);
		ar.io(m_ident);
	}

protected:
	std::string m_ident;
};

class PointIdentTimeSlice : public IdentTimeSlice {
public:
	PointIdentTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual PointIdentTimeSlice& as_point(void);
	virtual const PointIdentTimeSlice& as_point(void) const;

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

	const Point& get_coord(void) const { return m_coord; }
	void set_coord(const Point& pt) { m_coord = pt; }

	virtual void get_bbox(Rect& bbox) const;

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		IdentTimeSlice::hibernate(ar);
		ar.io(m_coord);
	}

protected:
        Point m_coord;
};

class ElevPointIdentTimeSlice : public PointIdentTimeSlice {
public:
	ElevPointIdentTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

	virtual ElevPointIdentTimeSlice& as_elevpoint(void);
	virtual const ElevPointIdentTimeSlice& as_elevpoint(void) const;

	virtual void dependencies(UUID::set_t& dep) const;
	virtual void dependencies(Link::LinkSet& dep) const;

	typedef int32_t elev_t;

	static const elev_t invalid_elev;

        elev_t get_elev(void) const { return m_elev; }
	void set_elev(elev_t elev = invalid_elev) { m_elev = elev; }
	bool is_elev_valid(void) const { return m_elev != invalid_elev; }

	virtual bool recompute(TopoDb30& topodb);

	virtual std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate(Archive& ar) {
		PointIdentTimeSlice::hibernate(ar);
		ar.io(m_elev);
	}

protected:
        elev_t m_elev;
};

class AltRange {
public:
	typedef enum {
		alt_first,
		alt_qnh = alt_first,
		alt_std,
		alt_height,
		alt_floor,
		alt_ceiling,
		alt_invalid
	} alt_t;

	// for floor and ceiling, alt is an additional constraint
	// eg. if lower mode is floor, the altitude must be above
	// the airspace floor and above lower alt.

	static const int32_t altmax;
	static const int32_t altignore;
	static const int32_t altinvalid;

	AltRange(int32_t lwr = 0, alt_t lwrmode = alt_invalid,
		 int32_t upr = std::numeric_limits<int32_t>::max(), alt_t uprmode = alt_invalid);
	alt_t get_lower_mode(void) const { return (alt_t)(m_mode & 0x0f); }
	int32_t get_lower_alt(void) const { return m_lwralt; }
	alt_t get_upper_mode(void) const { return (alt_t)((m_mode >> 4) & 0x0f); }
	int32_t get_upper_alt(void) const { return m_upralt; }
	void set_lower_mode(alt_t x) { m_mode ^= (m_mode ^ x) & 0x0f; }
	void set_lower_alt(int32_t x) { m_lwralt = x; }
	void set_upper_mode(alt_t x) { m_mode ^= (m_mode ^ (x << 4)) & 0xf0; }
	void set_upper_alt(int32_t x) { m_upralt = x; }
	static const std::string& get_mode_string(alt_t x);
	const std::string& get_lower_mode_string(void) const { return get_mode_string(get_lower_mode()); }
	const std::string& get_upper_mode_string(void) const { return get_mode_string(get_upper_mode()); }
	void set_lower_mode(const std::string& x);
	void set_upper_mode(const std::string& x);
	bool is_lower_valid(void) const;
	bool is_upper_valid(void) const;
	bool is_valid(void) const;
	int32_t get_lower_alt_if_valid(void) const;
	int32_t get_upper_alt_if_valid(void) const;
	bool operator==(const AltRange& x) const;
	bool operator!=(const AltRange& x) const { return !(*this == x); }
	bool is_inside(int32_t alt) const;
	bool is_inside(int32_t alt0, int32_t alt1) const;
	bool is_overlap(int32_t alt0, int32_t alt1) const;
	bool is_empty(void) const;
	void merge(const AltRange& r);
	void intersect(const AltRange& r);
	IntervalSet<int32_t> get_interval(bool invisfull = false) const;

	std::string to_str(void) const;
	std::string to_shortstr(void) const;

	template<class Archive> void hibernate(Archive& ar) {
		ar.io(m_lwralt);
		ar.io(m_upralt);
		ar.io(m_mode);
	}

protected:
	int32_t m_lwralt;
	int32_t m_upralt;
	uint8_t m_mode;
};

class BidirAltRange {
public:
	typedef IntervalSet<int32_t> set_t;
	typedef set_t::type_t type_t;
	BidirAltRange(const set_t& set0 = set_t(), const set_t& set1 = set_t());

	bool is_inside(unsigned int index, type_t alt) const;
	bool is_empty(void) const;
	const set_t& operator[](unsigned int idx) const { return m_set[!!idx]; }
	set_t& operator[](unsigned int idx) { return m_set[!!idx]; }
	void set_empty(void);
	void set_full(void);
	void set_interval(type_t alt0, type_t alt1);
	void set(const AltRange& ar);
	void invert(void);
	void swapdir(void);
	BidirAltRange& operator&=(const BidirAltRange& x);
	BidirAltRange& operator|=(const BidirAltRange& x);
	BidirAltRange& operator^=(const BidirAltRange& x);
	BidirAltRange& operator-=(const BidirAltRange& x);
	BidirAltRange operator~(void) const;
	BidirAltRange operator&(const BidirAltRange& x) const { BidirAltRange y(*this); y &= x; return y; }
	BidirAltRange operator|(const BidirAltRange& x) const { BidirAltRange y(*this); y |= x; return y; }
	BidirAltRange operator^(const BidirAltRange& x) const { BidirAltRange y(*this); y ^= x; return y; }
	BidirAltRange operator-(const BidirAltRange& x) const { BidirAltRange y(*this); y -= x; return y; }
	int compare(const BidirAltRange& x) const;
	bool operator==(const BidirAltRange& x) const { return compare(x) == 0; }
	bool operator!=(const BidirAltRange& x) const { return compare(x) != 0; }
	bool operator<(const BidirAltRange& x) const { return compare(x) < 0; }
	bool operator<=(const BidirAltRange& x) const { return compare(x) <= 0; }
	bool operator>(const BidirAltRange& x) const { return compare(x) > 0; }
	bool operator>=(const BidirAltRange& x) const { return compare(x) >= 0; }

	std::ostream& print(std::ostream& os, unsigned int indent) const;

	template<class Archive> void hibernate(Archive& ar) {
		m_set[0].hibernate(ar);
		m_set[1].hibernate(ar);
	}

protected:
	set_t m_set[2];
};

};

inline const std::string& to_str(ADR::Object::type_t t) { return ADR::Object::get_type_name(t); }
inline std::ostream& operator<<(std::ostream& os, ADR::Object::type_t t) { return os << to_str(t); }
inline const std::string& to_str(ADR::AltRange::alt_t x) { return ADR::AltRange::get_mode_string(x); }
inline std::ostream& operator<<(std::ostream& os, ADR::AltRange::alt_t x) { return os << to_str(x); }

#endif /* ADR_H */
