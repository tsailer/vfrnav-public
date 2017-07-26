//
// C++ Interface: geom
//
// Description: 
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012, 2013, 2014, 2015, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef GEOM_H
#define GEOM_H

//#include <cstdint>
#include <stdint.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <glibmm.h>
#include <interval.hh>

#ifdef __AVX__
#include <immintrin.h>
#endif

#include "sysdeps.h"
#include "alignedalloc.h"

#ifdef HAVE_PQXX
#include <pqxx/binarystring.hxx>
#endif

#ifdef HAVE_JSONCPP
namespace Json {
class Value;
};
#endif

namespace sqlite3x {
        class sqlite3_cursor;
        class sqlite3_command;
};

class Geometry;
class Rect;
class PolygonSimple;

class Point {
public:
	typedef int32_t coord_t;
	static const coord_t lon_min = 0x80000000;
	static const coord_t lon_max = 0x7FFFFFFF;
	static const coord_t lat_min = 0xC0000000;
	static const coord_t lat_max = 0x40000000;
	static const Point invalid;

	Point(coord_t lo = 0, coord_t la = 0) : lon(lo), lat(la) {}
	coord_t get_lon(void) const { return lon; }
	coord_t get_lat(void) const { return lat; }
	void set_lon(coord_t lo) { lon = lo; }
	void set_lat(coord_t la) { lat = la; }
	bool operator==(const Point &p) const { return lon == p.lon && lat == p.lat; }
	bool operator!=(const Point &p) const { return !operator==(p); }
	Point operator+(const Point &p) const { return Point(lon + (uint32_t)p.lon, lat + (uint32_t)p.lat); }
	Point operator-(const Point &p) const { return Point(lon - (uint32_t)p.lon, lat - (uint32_t)p.lat); }
	Point operator-() const { return Point(-lon, -lat); }
	Point& operator+=(const Point &p) { lon += (uint32_t)p.lon; lat += (uint32_t)p.lat; return *this; }
	Point& operator-=(const Point &p) { lon -= (uint32_t)p.lon; lat -= (uint32_t)p.lat; return *this; }
	bool operator<(const Point &p) const { return lon < p.lon && lat < p.lat; }
	bool operator<=(const Point &p) const { return lon <= p.lon && lat <= p.lat; }
	bool operator>(const Point &p) const { return lon > p.lon && lat > p.lat; }
	bool operator>=(const Point &p) const { return lon >= p.lon && lat >= p.lat; }
	int compare(const Point& x) const;
	uint64_t length2(void) const { return (uint64_t)(get_lon() * (int64_t)get_lon()) + (uint64_t)(get_lat() * (int64_t)get_lat()); }
	double length(void) const { return sqrt(length2()); }
	bool left_of(const Point& p0, const Point& p1) const;
	bool intersection(const Point& l0a, const Point& l0b, const Point& l1a, const Point& l1b);
	Point nearest_on_line(const Point& la, const Point& lb) const;
	Point spheric_nearest_on_line(const Point& la, const Point& lb) const;

	bool is_strictly_left(const Point& p0, const Point& p1) const { return area2(p0, p1, *this) > 0; }
	bool is_left_or_on(const Point& p0, const Point& p1) const { return area2(p0, p1, *this) >= 0; }
	bool is_collinear(const Point& p0, const Point& p1) const { return area2(p0, p1, *this) == 0; }
	bool is_between(const Point& p0, const Point& p1) const;
	bool is_strictly_between(const Point& p0, const Point& p1) const;
	bool is_convex(const Point& vprev, const Point& vnelont) const;
	bool is_in_cone(const Point& vprev, const Point& v, const Point& vnelont) const;
	bool is_left_of(const Point& p0, const Point& p1) const;
	bool in_box(const Point& p0, const Point& p1) const;
	bool is_invalid(void) const { return lat < lat_min || lat > lat_max; }
	void set_invalid(void);
	Rect get_bbox(void) const;

	std::ostream& print(std::ostream& os) const;

	static int64_t area2(const Point& p0, const Point& p1, const Point& p2);
	static bool is_proper_intersection(const Point& l0a, const Point& l0b, const Point& l1a, const Point& l1b);
	static bool is_intersection(const Point& l0a, const Point& l0b, const Point& l1a, const Point& l1b);
	static bool is_strict_intersection(const Point& l0a, const Point& l0b, const Point& l1a, const Point& l1b);

	Point simple_unwrap(const Point& p2) const;

public:
	static constexpr float to_deg = 90.0 / (1UL << 30);
	static constexpr float to_min = 90.0 * 60.0 / (1UL << 30);
	static constexpr float to_sec = 90.0 * 60.0 * 60.0 / (1UL << 30);
	static constexpr float to_rad = M_PI / (1UL << 31);
	static constexpr float from_deg = (1UL << 30) / 90.0;
	static constexpr float from_min = (1UL << 30) / 90.0 / 60.0;
	static constexpr float from_sec = (1UL << 30) / 90.0 / 60.0 / 60.0;
	static constexpr float from_rad = (1UL << 31) / M_PI;

	static constexpr double to_deg_dbl = 90.0 / (1UL << 30);
	static constexpr double to_min_dbl = 90.0 * 60.0 / (1UL << 30);
	static constexpr double to_sec_dbl = 90.0 * 60.0 * 60.0 / (1UL << 30);
	static constexpr double to_rad_dbl = M_PI / (1UL << 31);
	static constexpr double from_deg_dbl = (1UL << 30) / 90.0;
	static constexpr double from_min_dbl = (1UL << 30) / 90.0 / 60.0;
	static constexpr double from_sec_dbl = (1UL << 30) / 90.0 / 60.0 / 60.0;
	static constexpr double from_rad_dbl = (1UL << 31) / M_PI;

	static const int32_t pole_lat = (1UL << 30);

	static constexpr float earth_radius = 6378.155;
	static constexpr float km_to_nmi = 1.0 / 1.852;
	static constexpr float m_to_ft = 1.0 / 0.3048;
	static constexpr float ft_to_m = 0.3048;
	static constexpr float mps_to_kts = 3.6 / 1.852;

	static constexpr double earth_radius_dbl = 6378.155;
	static constexpr double km_to_nmi_dbl = 1.0 / 1.852;
	static constexpr double m_to_ft_dbl = 1.0 / 0.3048;
	static constexpr double ft_to_m_dbl = 0.3048;
	static constexpr double mps_to_kts_dbl = 3.6 / 1.852;

	static const double webmercator_maxlat_rad_dbl;
	static const double webmercator_maxlat_deg_dbl;
	static const coord_t webmercator_maxlat;

	template<typename I, typename F> static I round(F x) { return static_cast<I>((x < 0) ? (x - 0.5) : (x + 0.5)); }

	float get_lon_rad(void) const { return lon * to_rad; }
	float get_lat_rad(void) const { return lat * to_rad; }
	void set_lon_rad(float lo) { lon = round<coord_t,float>(lo * from_rad); }
	void set_lat_rad(float la) { lat = round<coord_t,float>(la * from_rad); }

	double get_lon_rad_dbl(void) const { return lon * to_rad_dbl; }
	double get_lat_rad_dbl(void) const { return lat * to_rad_dbl; }
	void set_lon_rad_dbl(double lo) { lon = round<coord_t,double>(lo * from_rad_dbl); }
	void set_lat_rad_dbl(double la) { lat = round<coord_t,double>(la * from_rad_dbl); }

	float get_lon_deg(void) const { return lon * to_deg; }
	float get_lat_deg(void) const { return lat * to_deg; }
	void set_lon_deg(float lo) { lon = round<coord_t,float>(lo * from_deg); }
	void set_lat_deg(float la) { lat = round<coord_t,float>(la * from_deg); }

	double get_lon_deg_dbl(void) const { return lon * to_deg_dbl; }
	double get_lat_deg_dbl(void) const { return lat * to_deg_dbl; }
	void set_lon_deg_dbl(double lo) { lon = round<coord_t,double>(lo * from_deg_dbl); }
	void set_lat_deg_dbl(double la) { lat = round<coord_t,double>(la * from_deg_dbl); }

	float get_lon_min(void) const { return lon * to_min; }
	float get_lat_min(void) const { return lat * to_min; }
	void set_lon_min(float lo) { lon = round<coord_t,float>(lo * from_min); }
	void set_lat_min(float la) { lat = round<coord_t,float>(la * from_min); }

	double get_lon_min_dbl(void) const { return lon * to_min_dbl; }
	double get_lat_min_dbl(void) const { return lat * to_min_dbl; }
	void set_lon_min_dbl(double lo) { lon = round<coord_t,double>(lo * from_min_dbl); }
	void set_lat_min_dbl(double la) { lat = round<coord_t,double>(la * from_min_dbl); }

	float get_lon_sec(void) const { return lon * to_sec; }
	float get_lat_sec(void) const { return lat * to_sec; }
	void set_lon_sec(float lo) { lon = round<coord_t,float>(lo * from_sec); }
	void set_lat_sec(float la) { lat = round<coord_t,float>(la * from_sec); }

	double get_lon_sec_dbl(void) const { return lon * to_sec_dbl; }
	double get_lat_sec_dbl(void) const { return lat * to_sec_dbl; }
	void set_lon_sec_dbl(double lo) { lon = round<coord_t,double>(lo * from_sec_dbl); }
	void set_lat_sec_dbl(double la) { lat = round<coord_t,double>(la * from_sec_dbl); }

	bool is_webmercator_valid(void) const;
	int32_t get_lon_webmercator(unsigned int zoomlevel) const;
	int32_t get_lat_webmercator(unsigned int zoomlevel) const;
	void set_lon_webmercator(unsigned int zoomlevel, int32_t x);
	void set_lat_webmercator(unsigned int zoomlevel, int32_t y);
	double get_lon_webmercator_dbl(unsigned int zoomlevel) const;
	double get_lat_webmercator_dbl(unsigned int zoomlevel) const;
	void set_lon_webmercator_dbl(unsigned int zoomlevel, double x);
	void set_lat_webmercator_dbl(unsigned int zoomlevel, double y);

	float simple_true_course(const Point& p2) const;
	uint64_t simple_distance_rel(const Point& p2) const;
	float simple_distance_km(const Point& p2) const;
	float simple_distance_nmi(const Point& p2) const { return simple_distance_km(p2) * km_to_nmi; }
	Point simple_course_distance_nmi(float course, float dist) const;
	Rect simple_box_km(float centerdist) const;
	Rect simple_box_nmi(float centerdist) const;

	float spheric_true_course(const Point& p2) const;
	float spheric_distance_km(const Point& p2) const;
	float spheric_distance_nmi(const Point& p2) const { return spheric_distance_km(p2) * km_to_nmi; }
	Point spheric_course_distance_nmi(float course, float dist) const;

	double spheric_true_course_dbl(const Point& p2) const;
	double spheric_distance_km_dbl(const Point& p2) const;
	double spheric_distance_nmi_dbl(const Point& p2) const { return spheric_distance_km_dbl(p2) * km_to_nmi_dbl; }

	Point halfway(const Point& pt2) const;

	std::pair<Point,double> get_gcnav(const Point& p2, double frac) const;
	PolygonSimple make_circle(float radius = 5, float angleincr = 15, bool rawcoord = false) const;
	PolygonSimple make_fat_line(const Point& p, float radius = 5, bool roundcaps = true, float angleincr = 15, bool rawcoord = false) const;

	static Glib::ustring to_ustring(const std::wstring& str);

	Glib::ustring get_lon_str(void) const;
	Glib::ustring get_lat_str(void) const;
	std::string get_lon_str2(void) const;
	std::string get_lat_str2(void) const;
	std::string get_lon_str3(void) const;
	std::string get_lat_str3(void) const;
	std::string get_lon_adexp_str(void) const;
	std::string get_lat_adexp_str(void) const;
	typedef enum {
		fplstrmode_deg          = 0,
		fplstrmode_degmin       = 1 | 8,
		fplstrmode_degoptmin    = 1,
		fplstrmode_degminsec    = 2 | 8 | 4,
		fplstrmode_degminoptsec = 2 | 8,
		fplstrmode_degoptminsec = 2
	} fplstrmode_t;
	std::string get_lon_fpl_str(fplstrmode_t mode = fplstrmode_degmin) const;
	std::string get_lat_fpl_str(fplstrmode_t mode = fplstrmode_degmin) const;
	std::string get_fpl_str(fplstrmode_t mode = fplstrmode_degmin) const;

	bool set_lon_str(const std::string& str);
	bool set_lat_str(const std::string& str);
	static const unsigned int setstr_lat    = 1 << 0;
	static const unsigned int setstr_lon    = 1 << 1;
	static const unsigned int setstr_excess = 1 << 2;
	static const unsigned int setstr_flags_locator    = 1 << 0;
	static const unsigned int setstr_flags_ch1903     = 1 << 1;
	unsigned int set_str(const std::string& str, unsigned int flags = setstr_flags_locator | setstr_flags_ch1903);

	static bool is_latitude(const std::string& s);
	static bool is_longitude(const std::string& s);
	static bool is_latlon(const std::string& s);
	static bool is_fpl_latlon(std::string::const_iterator i, std::string::const_iterator e);
	static bool is_fpl_latlon(const std::string& s) { return is_fpl_latlon(s.begin(), s.end()); }

	void set_ch1903(float x, float y);
	bool get_ch1903(float& x, float& y) const;
	void set_ch1903p(double x, double y);
	bool get_ch1903p(double& x, double& y) const;
	template<typename T> static T polyeval(const T *coeff, unsigned int n, T v);
	std::string get_locator(unsigned int digits = 10) const;

	Point get_round_deg(void) const;
	Point get_round_min(void) const;
	Point get_round_sec(void) const;

	// WKT / WKB
	static unsigned int get_wkbptsize(void) { return 16; }
	static unsigned int get_wkbsize(void);
	uint8_t *wkbencode(uint8_t *cp, uint8_t *ep) const;
	const uint8_t *wkbdecode(const uint8_t *cp, const uint8_t *ep, uint8_t byteorder);
	uint8_t *to_wkb(uint8_t *cp, uint8_t *ep) const;
	std::vector<uint8_t> to_wkb(void) const;
	const uint8_t *from_wkb(const uint8_t *cp, const uint8_t *ep);
	void from_wkb(const std::vector<uint8_t>& b) { from_wkb(&b[0], &b[b.size()]); }
	std::ostream& to_wktpt(std::ostream& os) const;
	std::ostream& to_wkt(std::ostream& os) const;

	// GeoJSON
#ifdef HAVE_JSONCPP
	Json::Value to_json(void) const;
	void from_json(const Json::Value& g);
#endif

	template<class Archive> void hibernate_binary(Archive& ar) {
		ar.io(lat);
		ar.io(lon);
	}

protected:
	static const unsigned int maidenhead_maxdigits = 5;
	static const unsigned int maidenhead_max[maidenhead_maxdigits];
	static const double maidenhead_factors[maidenhead_maxdigits];

	coord_t lon, lat;

	bool is_between_helper(const Point& p0, const Point& p1) const;
	bool is_strictly_between_helper(const Point& p0, const Point& p1) const;

	static bool intersect_precheck(const Point& l0a, const Point& l0b, const Point& l1a, const Point& l1b);
	unsigned int set_str_ch1903(std::string::const_iterator& si, std::string::const_iterator se);
	unsigned int set_str_locator(std::string::const_iterator& si, std::string::const_iterator se);

	float rawcoord_course(const Point& p2) const;
	Point rawcoord_course_distance(float course, float dist) const;

	double rawcoord_true_course_dbl(const Point& p2) const;
	Point rawcoord_course_distance(double course, double dist) const;

	friend class PolygonSimple;
};

class Rect {
public:
	static const Rect invalid;
	Rect(void) : southwest(Point(0, -Point::pole_lat)), northeast(Point(-1, Point::pole_lat)) {}
	Rect(const Point& sw, const Point& ne) : southwest(sw), northeast(ne) {}
	Point get_southwest(void) const { return southwest; }
	Point get_northeast(void) const { return northeast; }
	Point get_southeast(void) const { return Point(northeast.get_lon(), southwest.get_lat()); }
	Point get_northwest(void) const { return Point(southwest.get_lon(), northeast.get_lat()); }
	void set_southwest(const Point& p) { southwest = p; }
	void set_northeast(const Point& p) { northeast = p; }
	void set_southeast(const Point& p) { southwest.set_lat(p.get_lat()); northeast.set_lon(p.get_lon()); }
	void set_northwest(const Point& p) { northeast.set_lat(p.get_lat()); southwest.set_lon(p.get_lon()); }
	Point::coord_t get_north(void) const { return northeast.get_lat(); }
	Point::coord_t get_south(void) const { return southwest.get_lat(); }
	Point::coord_t get_east(void) const { return northeast.get_lon(); }
	int64_t get_east_unwrapped(void) const;
	Point::coord_t get_west(void) const { return southwest.get_lon(); }
	void set_north(Point::coord_t c) { northeast.set_lat(c); }
	void set_south(Point::coord_t c) { southwest.set_lat(c); }
	void set_east(Point::coord_t c) { northeast.set_lon(c); }
	void set_west(Point::coord_t c) { southwest.set_lon(c); }
	int compare(const Rect& x) const;
	bool operator==(const Rect &r) const { return get_southwest() == r.get_southwest() && get_northeast() == r.get_northeast(); }
	bool operator!=(const Rect &r) const { return !operator==(r); }
	Rect& operator+=(const Point &p) { southwest += p; northeast += p; return *this; }
	Rect& operator-=(const Point &p) { southwest -= p; northeast -= p; return *this; }
	Rect operator+(const Point &p) { Rect r(*this); r += p; return r; }
	Rect operator-(const Point &p) { Rect r(*this); r -= p; return r; }
	bool is_invalid(void) const { return get_northeast().is_invalid() || get_southwest().is_invalid(); }
	void set_invalid(void) { *this = invalid; }
	bool is_inside(const Point& pt) const;
	bool is_inside(const Rect& r) const;
	bool is_strictly_inside(const Point& pt) const;
	bool is_strictly_inside(const Rect& r) const { return is_strictly_inside(r.get_southwest()) && is_strictly_inside(r.get_northeast()); }
	bool is_lon_inside(const Point& pt) const;
	bool is_lat_inside(const Point& pt) const;
	uint64_t simple_distance_rel(const Point& pt) const;
	float simple_distance_km(const Point& pt) const;
	float simple_distance_nmi(const Point& pt) const { return simple_distance_km(pt) * Point::km_to_nmi; }
	float spheric_distance_km(const Point& pt) const;
	float spheric_distance_nmi(const Point& pt) const { return spheric_distance_km(pt) * Point::km_to_nmi; }
	bool is_intersect(const Rect& r) const;
	bool is_intersect(const Point& la, const Point& lb) const;
	Rect intersect(const Rect& r) const;
	uint64_t simple_distance_rel(const Rect& r) const;
	float simple_distance_km(const Rect& r) const;
	float simple_distance_nmi(const Rect& r) const { return simple_distance_km(r) * Point::km_to_nmi; }
	float spheric_distance_km(const Rect& r) const;
	float spheric_distance_nmi(const Rect& r) const { return spheric_distance_km(r) * Point::km_to_nmi; }
	Point boxcenter(void) const { return southwest.halfway(northeast); }
	Rect oversize_nmi(float nmi) const;
	Rect oversize_km(float km) const;
	Rect add(const Point& pt) const;
	Rect add(const Rect& r) const;
	int64_t area2(void) const;
	uint64_t simple_circumference_rel(void) const;
	float simple_circumference_km(void) const;
	float simple_circumference_nmi(void) const { return simple_circumference_km() * Point::km_to_nmi; }
	float spheric_circumference_km(void) const;
	float spheric_circumference_nmi(void) const { return spheric_circumference_km() * Point::km_to_nmi; }
	double spheric_circumference_km_dbl(void) const;
	double spheric_circumference_nmi_dbl(void) const { return spheric_circumference_km_dbl() * Point::km_to_nmi_dbl; }
	double get_simple_area_km2_dbl(void) const;
	double get_simple_area_nmi2_dbl(void) const { return get_simple_area_km2_dbl() * (Point::km_to_nmi_dbl * Point::km_to_nmi_dbl); }
	operator PolygonSimple(void) const;

	bool is_empty(void) const;
	void set_empty(void);

	std::ostream& print(std::ostream& os) const;

	template<class Archive> void hibernate_binary(Archive& ar) {
		southwest.hibernate_binary(ar);
		northeast.hibernate_binary(ar);
	}

protected:
	Point southwest;
	Point northeast;

	std::pair<bool,bool> is_overlap_helper(const Rect& r) const;
};

class PolygonHole;
class MultiPolygonHole;

class BorderPoint {
public:
	typedef enum {
		flag_begin,
		flag_end,
		flag_onborder,
		flag_inc,
		flag_dec
	} flag_t;

	typedef enum {
		flagmask_begin    = 1 << flag_begin,
		flagmask_end      = 1 << flag_end,
		flagmask_onborder = 1 << flag_onborder,
		flagmask_inc      = 1 << flag_inc,
		flagmask_dec      = 1 << flag_dec,
		flagmask_none     = 0,
		flagmask_all      = flagmask_begin | flagmask_end | flagmask_onborder | flagmask_inc | flagmask_dec
	} flagmask_t;

	typedef std::vector<BorderPoint> borderpoints_t;

	BorderPoint(const Point& pt = Point::invalid, double dist = std::numeric_limits<double>::quiet_NaN(), flagmask_t flags = flagmask_none);
	const Point& get_point(void) const { return m_pt; }
	double get_dist(void) const { return m_dist; }
	flagmask_t get_flags(void) const { return m_flags; }
	static std::string get_flags_string(flagmask_t m);
	std::string get_flags_string(void) const { return get_flags_string(get_flags()); }
	int get_windingnumber(void) const;

	int compare(const BorderPoint& x) const;
	bool operator==(const BorderPoint& x) const { return compare(x) == 0; }
	bool operator!=(const BorderPoint& x) const { return compare(x) != 0; }
	bool operator<(const BorderPoint& x) const { return compare(x) < 0; }
	bool operator<=(const BorderPoint& x) const { return compare(x) <= 0; }
	bool operator>(const BorderPoint& x) const { return compare(x) > 0; }
	bool operator>=(const BorderPoint& x) const { return compare(x) >= 0; }

protected:
	Point m_pt;
	double m_dist;
	flagmask_t m_flags;
};

class MultiPoint : public std::vector<Point> {
protected:
	typedef std::vector<Point> base_t;
	std::ostream& print(std::ostream& os) const;

public:
	Rect get_bbox(void) const;
	int compare(const MultiPoint& x) const;

	// WKT / WKB
	unsigned int get_wkbsize(void) const;
	uint8_t *wkbencode(uint8_t *cp, uint8_t *ep) const;
	const uint8_t *wkbdecode(const uint8_t *cp, const uint8_t *ep, uint8_t byteorder);
	uint8_t *to_wkb(uint8_t *cp, uint8_t *ep) const;
	std::vector<uint8_t> to_wkb(void) const;
	const uint8_t *from_wkb(const uint8_t *cp, const uint8_t *ep);
	void from_wkb(const std::vector<uint8_t>& b) { from_wkb(&b[0], &b[b.size()]); }
	std::ostream& to_wkt(std::ostream& os) const;

	// GeoJSON
#ifdef HAVE_JSONCPP
	Json::Value to_json(void) const;
	void from_json(const Json::Value& g);
#endif

	template<class Archive> void hibernate_binary(Archive& ar) {
		uint32_t sz(size());
		ar.io(sz);
		if (ar.is_load())
			resize(sz);
		for (iterator i(begin()), e(end()); i != e; ++i)
			i->hibernate_binary(ar);
	}
};

class LineString : public std::vector<Point, aligned_allocator<Point> > {
protected:
	typedef std::vector<Point, aligned_allocator<Point> > base_t;

	class PointLatLonSorter {
	public:
		bool operator()(const Point& a, const Point& b) const;
	};

public:
	static const std::string skyvector_url;

	bool is_on_boundary(const Point& pt) const;
	bool is_boundary_point(const Point& pt) const;
	bool is_intersection(const Point& la, const Point& lb) const;
	bool is_intersection(const Rect& r) const;
	bool is_strict_intersection(const Point& la, const Point& lb) const;
	bool is_self_intersecting(void) const;
	typedef std::set<Point,PointLatLonSorter> pointset_t;
	pointset_t get_intersections(const LineString& x) const;
	void reverse(void);
	Rect get_bbox(void) const;
	bool operator==(const LineString &p) const;
	bool operator!=(const LineString &p) const { return !operator==(p); }
	int compare(const LineString& x) const;
	std::ostream& print(std::ostream& os) const;
	std::string to_skyvector(void) const;

	// WKT / WKB
	unsigned int get_wkbsize(void) const;
	uint8_t *wkbencode(uint8_t *cp, uint8_t *ep) const;
	const uint8_t *wkbdecode(const uint8_t *cp, const uint8_t *ep, uint8_t byteorder);
	uint8_t *to_wkb(uint8_t *cp, uint8_t *ep) const;
	std::vector<uint8_t> to_wkb(void) const;
	const uint8_t *from_wkb(const uint8_t *cp, const uint8_t *ep);
	void from_wkb(const std::vector<uint8_t>& b) { from_wkb(&b[0], &b[b.size()]); }
	std::ostream& to_wktlst(std::ostream& os) const;
	std::ostream& to_wkt(std::ostream& os) const;

	// GeoJSON
#ifdef HAVE_JSONCPP
	Json::Value to_json(void) const;
	void from_json(const Json::Value& g);
#endif

	template<class Archive> void hibernate_binary(Archive& ar) {
		uint32_t sz(size());
		ar.io(sz);
		if (ar.is_load())
			resize(sz);
		for (iterator i(begin()), e(end()); i != e; ++i)
			i->hibernate_binary(ar);
	}
};

class MultiLineString : public std::vector<LineString> {
protected:
	typedef std::vector<LineString> base_t;

public:
	Rect get_bbox(void) const;
	int compare(const MultiLineString& x) const;
	std::ostream& print(std::ostream& os) const;

	// WKT / WKB
	unsigned int get_wkbsize(void) const;
	uint8_t *wkbencode(uint8_t *cp, uint8_t *ep) const;
	const uint8_t *wkbdecode(const uint8_t *cp, const uint8_t *ep, uint8_t byteorder);
	uint8_t *to_wkb(uint8_t *cp, uint8_t *ep) const;
	std::vector<uint8_t> to_wkb(void) const;
	const uint8_t *from_wkb(const uint8_t *cp, const uint8_t *ep);
	void from_wkb(const std::vector<uint8_t>& b) { from_wkb(&b[0], &b[b.size()]); }
	std::ostream& to_wkt(std::ostream& os) const;

	// GeoJSON
#ifdef HAVE_JSONCPP
	Json::Value to_json(void) const;
	void from_json(const Json::Value& g);
#endif

	template<class Archive> void hibernate_binary(Archive& ar) {
		uint32_t sz(size());
		ar.io(sz);
		if (ar.is_load())
			resize(sz);
		for (iterator i(begin()), e(end()); i != e; ++i)
			i->hibernate_binary(ar);
	}
};

class PolygonSimple : public LineString {
public:
	typedef LineString base_t;

	class ScanLine : public std::map<Point::coord_t,int> {
	protected:
		typedef std::map<Point::coord_t,int> base_t;

	public:
		using base_t::insert;
		void insert(Point::coord_t lon, int wn);
		ScanLine integrate(Point::coord_t lon) const;
		ScanLine combine(const ScanLine& sl2, int factor1 = 1, int factor2 = 1) const;
	};

	typedef enum {
		inside_none      = 0x00,
		inside_partial   = 0x01,
		inside_all       = 0x02,
		inside_error     = 0x03,
		inside_mask      = 0x03,
		inside_point0    = 0x04,
		inside_point1    = 0x08
	} inside_t;

	typedef BorderPoint::borderpoints_t borderpoints_t;

	class InsideHelper : public IntervalSet<int64_t> {
	protected:
		typedef enum {
			simd_none,
			simd_avx,
			simd_avx2
		} simd_t;
	public:
		typedef IntervalSet<int64_t> interval_t;
		static const bool debug = false;

		InsideHelper(const Point& pt0, const Point& pt1, bool enable_simd = true);
		const Point& get_origin(void) const { return m_origin; }
		const Point& get_vector(void) const { return m_vector; }
		int64_t get_end(void) const { return m_end; }
		typedef std::pair<int64_t,int64_t> tpoint_t;
		tpoint_t transform(const Point& pt) const;
		Point transform(const tpoint_t& tp) const;
		bool is_error(void) const { return m_end < 0; }
		void set_error(void) { m_end = -1; }
		bool is_simd(void) const { return m_simd != simd_none; }
		inside_t get_result(void) const;
		borderpoints_t get_borderpoints(void) const;
		void clear(void);
		using interval_t::operator|=;
		using interval_t::operator&=;
		using interval_t::operator-=;
		using interval_t::operator=;
		InsideHelper& operator|=(const PolygonSimple& poly) { *this |= compute_winding(poly); return *this; }
		InsideHelper& operator&=(const PolygonSimple& poly) { *this &= compute_winding(poly); return *this; }
		InsideHelper& operator-=(const PolygonSimple& poly) { *this -= compute_winding(poly); return *this; }
		InsideHelper& operator|=(const PolygonHole& poly) { *this |= compute_winding(poly); return *this; }
		InsideHelper& operator&=(const PolygonHole& poly) { *this &= compute_winding(poly); return *this; }
		InsideHelper& operator-=(const PolygonHole& poly) { *this -= compute_winding(poly); return *this; }
		InsideHelper& operator|=(const MultiPolygonHole& poly) { *this |= compute_winding(poly); return *this; }
		InsideHelper& operator&=(const MultiPolygonHole& poly) { *this &= compute_winding(poly); return *this; }
		InsideHelper& operator-=(const MultiPolygonHole& poly) { *this -= compute_winding(poly); return *this; }
		InsideHelper& operator=(const PolygonSimple& poly) { *this = compute_winding(poly); return *this; }
		InsideHelper& operator=(const PolygonHole& poly) { *this = compute_winding(poly); return *this; }
		InsideHelper& operator=(const MultiPolygonHole& poly) { *this = compute_winding(poly); return *this; }

	protected:
		Point m_origin;
		Point m_vector;
		int64_t m_end;
		simd_t m_simd;

		typedef std::map<int64_t,int32_t> winding_t;
		void compute_winding(winding_t& wind, const PolygonSimple& poly, int dir) const;
		void compute_winding(winding_t& wind, const PolygonHole& poly, int dir) const;
		void compute_winding(winding_t& wind, const MultiPolygonHole& poly, int dir) const;
		interval_t finalize(const winding_t& wind);
		interval_t compute_winding(const PolygonSimple& poly);
		interval_t compute_winding(const PolygonHole& poly);
		interval_t compute_winding(const MultiPolygonHole& poly);

#ifdef __AVX__
		inline void avx2_horizontal(winding_t& wind, int dir, __m256i vptprevlat, __m256i vptprevlon, __m256i vptlat, __m256i vptlon,
					    uint32_t cond, unsigned int cnt, unsigned int i) const __attribute__((always_inline));
		inline void avx2_upward(winding_t& wind, int dir, __m256i vptprevlat, __m256i vptprevlon, __m256i vptlat, __m256i vptlon,
					uint32_t cond, unsigned int cnt, unsigned int i) const __attribute__((always_inline));
		inline void avx2_downward(winding_t& wind, int dir, __m256i vptprevlat, __m256i vptprevlon, __m256i vptlat, __m256i vptlon,
					  uint32_t cond, unsigned int cnt, unsigned int i) const __attribute__((always_inline));
#endif		
	};

	reference operator[](size_type x);
	const_reference operator[](size_type x) const;
	bool is_on_boundary(const Point& pt) const;
	bool is_intersection(const Point& la, const Point& lb) const;
	bool is_intersection(const Rect& r) const;
	bool is_strict_intersection(const Point& la, const Point& lb) const;
	bool is_self_intersecting(void) const;
	pointset_t get_intersections(const LineString& x) const;
	bool is_all_points_inside(const Rect& r) const;
	bool is_any_point_inside(const Rect& r) const;
	bool is_overlap(const Rect& r) const;
	int windingnumber(const Point& pt) const;
	ScanLine scanline(Point::coord_t lat) const;
	inside_t get_inside(const Point& pt0, const Point& pt1) const;
	borderpoints_t get_borderpoints(const Point& pt0, const Point& pt1) const;
	int64_t area2(void) const;
	Point centroid(void) const;
	bool is_counterclockwise(void) const { return area2() >= 0; }
	void make_counterclockwise(void);
	void make_clockwise(void);
	unsigned int next(unsigned int it) const;
	unsigned int prev(unsigned int it) const;
	void remove_redundant_polypoints(void);
	void simplify_outside(const Rect& bbox);
	uint64_t simple_circumference_rel(void) const;
	float simple_circumference_km(void) const;
	float simple_circumference_nmi(void) const { return simple_circumference_km() * Point::km_to_nmi; }
	float spheric_circumference_km(void) const;
	float spheric_circumference_nmi(void) const { return spheric_circumference_km() * Point::km_to_nmi; }
	double spheric_circumference_km_dbl(void) const;
	double spheric_circumference_nmi_dbl(void) const { return spheric_circumference_km_dbl() * Point::km_to_nmi_dbl; }
	// get_simple_area is signed depending on the turn direction
	double get_simple_area_km2_dbl(void) const;
	double get_simple_area_nmi2_dbl(void) const { return get_simple_area_km2_dbl() * (Point::km_to_nmi_dbl * Point::km_to_nmi_dbl); }
	static PolygonSimple line_width_rectangular(const Point& l0, const Point& l1, double width_nmi);
	void make_fat_lines_helper(MultiPolygonHole& ph, float radius = 5, bool roundcaps = true, float angleincr = 15, bool rawcoord = false) const;
	MultiPolygonHole make_fat_lines(float radius = 5, bool roundcaps = true, float angleincr = 15, bool rawcoord = false) const;
	void bindblob(sqlite3x::sqlite3_command& cmd, int index) const;
	void getblob(sqlite3x::sqlite3_cursor& cursor, int index);
#ifdef HAVE_PQXX
	pqxx::binarystring bindblob(void) const;
	void getblob(const pqxx::binarystring& blob);
#endif
	void snap_bits(unsigned int nrbits);
	void randomize_bits(unsigned int nrbits);
	unsigned int zap_leastimportant(Point& pt);
	unsigned int zap_leastimportant(void) { Point pt; return zap_leastimportant(pt); }
	std::ostream& print_nrvertices(std::ostream& os, bool prarea = false) const;
	std::string to_skyvector(void) const;
	bool geos_is_valid(void) const;
	void geos_make_valid(void);
	MultiPolygonHole geos_simplify(void);
	MultiPolygonHole geos_simplify(Point::coord_t lonoffs);

	// WKT / WKB
	unsigned int get_wkbpolysize(void) const;
	unsigned int get_wkbsize(void) const;
	uint8_t *wkbencode(uint8_t *cp, uint8_t *ep) const;
	const uint8_t *wkbdecode(const uint8_t *cp, const uint8_t *ep, uint8_t byteorder);
	uint8_t *to_wkb(uint8_t *cp, uint8_t *ep) const;
	std::vector<uint8_t> to_wkb(void) const;
	const uint8_t *from_wkb(const uint8_t *cp, const uint8_t *ep);
	void from_wkb(const std::vector<uint8_t>& b) { from_wkb(&b[0], &b[b.size()]); }
	std::ostream& to_wktpoly(std::ostream& os) const;
	std::ostream& to_wkt(std::ostream& os) const;

	// GeoJSON
#ifdef HAVE_JSONCPP
	Json::Value to_json(void) const;
	void from_json(const Json::Value& g);
#endif

protected:
	bool geos_make_valid_helper(void);
};

class PolygonHole {
public:
	typedef PolygonSimple::ScanLine ScanLine;
	typedef BorderPoint::borderpoints_t borderpoints_t;
	typedef PolygonSimple::inside_t inside_t;

	PolygonHole(const PolygonSimple& extr = PolygonSimple(), const std::vector<PolygonSimple>& intr = std::vector<PolygonSimple>()) : m_exterior(extr), m_interior(intr) {}
	void clear(void) { m_exterior.clear(); m_interior.clear(); }
	PolygonSimple& get_exterior(void) { return m_exterior; }
	const PolygonSimple& get_exterior(void) const { return m_exterior; }
	void set_exterior(const PolygonSimple& extr) { m_exterior = extr; }
	unsigned int get_nrinterior(void) const { return m_interior.size(); }
	PolygonSimple& operator[](unsigned int x) { return m_interior[x]; }
	const PolygonSimple& operator[](unsigned int x) const { return m_interior[x]; }
	void add_interior(const PolygonSimple& intr) { m_interior.push_back(intr); }
	void insert_interior(const PolygonSimple& intr, int index) { m_interior.insert(m_interior.begin() + index, intr); }
	void erase_interior(int index) { m_interior.erase(m_interior.begin() + index); }
	void clear_interior(void) { m_interior.clear(); }
	bool is_on_boundary(const Point& pt) const;
	bool is_boundary_point(const Point& pt) const;
	bool is_intersection(const Point& la, const Point& lb) const;
	bool is_intersection(const Rect& r) const;
	bool is_strict_intersection(const Point& la, const Point& lb) const;
	bool is_all_points_inside(const Rect& r) const;
	bool is_any_point_inside(const Rect& r) const;
	bool is_overlap(const Rect& r) const;
	int windingnumber(const Point& pt) const;
	ScanLine scanline(Point::coord_t lat) const;
	inside_t get_inside(const Point& pt0, const Point& pt1) const;
	borderpoints_t get_borderpoints(const Point& pt0, const Point& pt1) const;
	int64_t area2(void) const;
	void remove_redundant_polypoints(void);
	void simplify_outside(const Rect& bbox);
	void reverse(void);
	Rect get_bbox(void) const { return m_exterior.get_bbox(); }
	bool operator==(const PolygonHole &p) const;
	bool operator!=(const PolygonHole &p) const { return !operator==(p); }
	int compare(const PolygonHole& x) const;
	void make_fat_lines_helper(MultiPolygonHole& ph, float radius = 5, bool roundcaps = true, float angleincr = 15, bool rawcoord = false) const;
	MultiPolygonHole make_fat_lines(float radius = 5, bool roundcaps = true, float angleincr = 15, bool rawcoord = false) const;
	void bindblob(sqlite3x::sqlite3_command& cmd, int index) const;
	void getblob(sqlite3x::sqlite3_cursor& cursor, int index);
#ifdef HAVE_PQXX
	pqxx::binarystring bindblob(void) const;
	void getblob(const pqxx::binarystring& blob);
#endif
	void normalize(void);
	void normalize_boostgeom(void);
	// get_simple_area is signed depending on the turn direction
	double get_simple_area_km2_dbl(void) const;
	double get_simple_area_nmi2_dbl(void) const { return get_simple_area_km2_dbl() * (Point::km_to_nmi_dbl * Point::km_to_nmi_dbl); }
	void snap_bits(unsigned int nrbits);
	void randomize_bits(unsigned int nrbits);
	unsigned int zap_holes(uint64_t abslim = 0, double rellim = 0);
	std::ostream& print_nrvertices(std::ostream& os, bool prarea = false) const;
	std::ostream& print(std::ostream& os) const;
	bool geos_is_valid(void) const;

	// needed for boost adaptation
	typedef std::vector<PolygonSimple> interior_t;
	interior_t& get_interior(void) { return m_interior; }
	const interior_t& get_interior(void) const { return m_interior; }

	// WKT / WKB
	unsigned int get_wkbsize(void) const;
	uint8_t *wkbencode(uint8_t *cp, uint8_t *ep) const;
	const uint8_t *wkbdecode(const uint8_t *cp, const uint8_t *ep, uint8_t byteorder);
	uint8_t *to_wkb(uint8_t *cp, uint8_t *ep) const;
	std::vector<uint8_t> to_wkb(void) const;
	const uint8_t *from_wkb(const uint8_t *cp, const uint8_t *ep);
	void from_wkb(const std::vector<uint8_t>& b) { from_wkb(&b[0], &b[b.size()]); }
	std::ostream& to_wktpoly(std::ostream& os) const;
	std::ostream& to_wkt(std::ostream& os) const;

	// GeoJSON
#ifdef HAVE_JSONCPP
	Json::Value to_json(void) const;
	void from_json(const Json::Value& g);
#endif

	template<class Archive> void hibernate_binary(Archive& ar) {
		m_exterior.hibernate_binary(ar);
		uint32_t sz(m_interior.size());
		ar.io(sz);
		if (ar.is_load())
			m_interior.resize(sz);
		for (interior_t::iterator i(m_interior.begin()), e(m_interior.end()); i != e; ++i)
			i->hibernate_binary(ar);
	}

protected:
	PolygonSimple m_exterior;
	interior_t m_interior;

	friend class MultiPolygonHole;
	static unsigned int bloblength(const PolygonSimple& p);
	static uint8_t *blobencode(const PolygonSimple& p, uint8_t *data);
	static uint8_t const *blobdecode(PolygonSimple& p, uint8_t const *data, uint8_t const *end);
};

class MultiPolygonHole : public std::vector<PolygonHole> {
public:
	typedef PolygonSimple::ScanLine ScanLine;
	typedef BorderPoint::borderpoints_t borderpoints_t;
	typedef PolygonSimple::inside_t inside_t;

	MultiPolygonHole();
	MultiPolygonHole(const PolygonHole& p);
	bool is_on_boundary(const Point& pt) const;
	bool is_boundary_point(const Point& pt) const;
	bool is_intersection(const Point& la, const Point& lb) const;
	bool is_intersection(const Rect& r) const;
	bool is_strict_intersection(const Point& la, const Point& lb) const;
	bool is_all_points_inside(const Rect& r) const;
	bool is_any_point_inside(const Rect& r) const;
	bool is_overlap(const Rect& r) const;
	int windingnumber(const Point& pt) const;
	ScanLine scanline(Point::coord_t lat) const;
	inside_t get_inside(const Point& pt0, const Point& pt1) const;
	borderpoints_t get_borderpoints(const Point& pt0, const Point& pt1) const;
	int64_t area2(void) const;
	void remove_redundant_polypoints(void);
	void simplify_outside(const Rect& bbox);
	Rect get_bbox(void) const;
	bool operator==(const MultiPolygonHole &p) const;
	bool operator!=(const MultiPolygonHole &p) const { return !operator==(p); }
	int compare(const MultiPolygonHole& x) const;
	void make_fat_lines_helper(MultiPolygonHole& ph, float radius = 5, bool roundcaps = true, float angleincr = 15, bool rawcoord = false) const;
	MultiPolygonHole make_fat_lines(float radius = 5, bool roundcaps = true, float angleincr = 15, bool rawcoord = false) const;
	void bindblob(sqlite3x::sqlite3_command& cmd, int index) const;
	void getblob(sqlite3x::sqlite3_cursor& cursor, int index);
#ifdef HAVE_PQXX
	pqxx::binarystring bindblob(void) const;
	void getblob(const pqxx::binarystring& blob);
#endif
	void normalize(void);
	void normalize_boostgeom(void);
	// get_simple_area is signed depending on the turn direction
	double get_simple_area_km2_dbl(void) const;
	double get_simple_area_nmi2_dbl(void) const { return get_simple_area_km2_dbl() * (Point::km_to_nmi_dbl * Point::km_to_nmi_dbl); }
	void snap_bits(unsigned int nrbits);
	void randomize_bits(unsigned int nrbits);
	unsigned int zap_holes(uint64_t abslim = 0, double rellim = 0);
	unsigned int zap_polys(uint64_t abslim = 0, double rellim = 0);
	std::ostream& print_nrvertices(std::ostream& os, bool prarea = false) const;
	std::ostream& print(std::ostream& os) const;
	bool geos_is_valid(void) const;
	void geos_make_valid(void);
	void geos_union(const MultiPolygonHole& m);
	void geos_union(const MultiPolygonHole& m, Point::coord_t lonoffs);
	void geos_subtract(const MultiPolygonHole& m);
	void geos_subtract(const MultiPolygonHole& m, Point::coord_t lonoffs);
	void geos_intersect(const MultiPolygonHole& m);
	void geos_intersect(const MultiPolygonHole& m, Point::coord_t lonoffs);
	bool is_intersect_fat_line(const Point& pa, const Point& pb, float radius) const;
	bool is_intersect_circle(const Point& pa, float radius) const;

	// WKT / WKB
	unsigned int get_wkbsize(void) const;
	uint8_t *wkbencode(uint8_t *cp, uint8_t *ep) const;
	const uint8_t *wkbdecode(const uint8_t *cp, const uint8_t *ep, uint8_t byteorder);
	uint8_t *to_wkb(uint8_t *cp, uint8_t *ep) const;
	std::vector<uint8_t> to_wkb(void) const;
	const uint8_t *from_wkb(const uint8_t *cp, const uint8_t *ep);
	void from_wkb(const std::vector<uint8_t>& b) { from_wkb(&b[0], &b[b.size()]); }
	std::ostream& to_wkt(std::ostream& os) const;

	// GeoJSON
#ifdef HAVE_JSONCPP
	Json::Value to_json(void) const;
	void from_json(const Json::Value& g);
#endif

	template<class Archive> void hibernate_binary(Archive& ar) {
		uint32_t sz(size());
		ar.io(sz);
		if (ar.is_load())
			resize(sz);
		for (iterator i(begin()), e(end()); i != e; ++i)
			i->hibernate_binary(ar);
	}

protected:
	static unsigned int bloblength(const PolygonHole& p);
	static uint8_t *blobencode(const PolygonHole& p, uint8_t *data);
	static uint8_t const *blobdecode(PolygonHole& p, uint8_t const *data, uint8_t const *end);
};

class IntervalBoundingBox {
public:
	IntervalBoundingBox(void);
	operator Rect(void) const;
	void add(const Rect& r);
	void add(const Point& p);
	void add(const Point& p0, const Point& p1);
	void add(const PolygonSimple& p);
	void add(const PolygonHole& p);
	void add(const MultiPolygonHole& p);

protected:
	typedef IntervalSet<int64_t> int_t;
	static const int_t::type_t lmin;
	static const int_t::type_t lmax;
	static const int_t::type_t lone = 1;
	static const int_t::type_t ltotal = lone << 32;
	int_t m_lonset;
	Point::coord_t m_latmin;
	Point::coord_t m_latmax;
};

class GeometryCollection;

class Geometry {
public:
	typedef enum {
		type_invalid            = 0,
		type_point              = 1,
		type_linestring         = 2,
		type_polygon            = 3,
		type_multipoint         = 4,
		type_multilinestring    = 5,
		type_multipolygon       = 6,
		type_geometrycollection = 7,
		type_last               = type_geometrycollection
	} type_t;

	typedef Glib::RefPtr<Geometry> ptr_t;

	void reference(void) const;
	void unreference(void) const;

	static Glib::RefPtr<Geometry> create(const Point& x);
	static Glib::RefPtr<Geometry> create(const LineString& x);
	static Glib::RefPtr<Geometry> create(const PolygonSimple& x);
	static Glib::RefPtr<Geometry> create(const PolygonHole& x);
	static Glib::RefPtr<Geometry> create(const MultiPoint& x);
	static Glib::RefPtr<Geometry> create(const MultiLineString& x);
	static Glib::RefPtr<Geometry> create(const MultiPolygonHole& x);
	static Glib::RefPtr<Geometry> create(const GeometryCollection& x);

	virtual type_t get_type(void) const { return type_invalid; }
	virtual operator const Point& (void) const;
	virtual operator const LineString& (void) const;
	virtual operator const PolygonHole& (void) const;
	virtual operator const MultiPoint& (void) const;
	virtual operator const MultiLineString& (void) const;
	virtual operator const MultiPolygonHole& (void) const;
	virtual operator const GeometryCollection& (void) const;

	virtual Rect get_bbox(void) const { Rect bbox; bbox.set_empty(); return bbox; }
	virtual int64_t area2(void) const { return 0; }
	virtual Geometry::ptr_t flatten(void) const { return ptr_t(); }

	// WKT / WKB helper
	static unsigned int get_wkbhdrsize(void) { return 5; }
	static uint8_t *wkbencode(uint8_t *cp, uint8_t *ep, type_t typ);
	static uint8_t *wkbencode(uint8_t *cp, uint8_t *ep, uint8_t x);
	static uint8_t *wkbencode(uint8_t *cp, uint8_t *ep, uint32_t x);
	static uint8_t *wkbencode(uint8_t *cp, uint8_t *ep, double x);
	static const uint8_t *wkbdecode(const uint8_t *cp, const uint8_t *ep, type_t& typ, uint8_t& byteorder);
	static const uint8_t *wkbdecode(const uint8_t *cp, const uint8_t *ep, uint8_t& x, uint8_t byteorder);
	static const uint8_t *wkbdecode(const uint8_t *cp, const uint8_t *ep, uint32_t& x, uint8_t byteorder);
	static const uint8_t *wkbdecode(const uint8_t *cp, const uint8_t *ep, double& x, uint8_t byteorder);

	static Glib::RefPtr<Geometry> create_from_wkb(const uint8_t * &cpout, const uint8_t *cp, const uint8_t *ep);
	static Glib::RefPtr<Geometry> create_from_wkb(const uint8_t * &cpout, const uint8_t *cp, const uint8_t *ep, uint8_t byteorder);
	static Glib::RefPtr<Geometry> create_from_wkb(const uint8_t * &cpout, const uint8_t *cp, const uint8_t *ep, type_t typ, uint8_t byteorder);

	// WKT / WKB
	virtual unsigned int get_wkbsize(void) const { return 0; }
	virtual uint8_t *wkbencode(uint8_t *cp, uint8_t *ep) const { return cp; }
	virtual const uint8_t *wkbdecode(const uint8_t *cp, const uint8_t *ep, uint8_t byteorder) { return cp; }
	virtual uint8_t *to_wkb(uint8_t *cp, uint8_t *ep) const { return cp; }
	virtual const uint8_t *from_wkb(const uint8_t *cp, const uint8_t *ep) { return cp; }
	virtual std::ostream& to_wkt(std::ostream& os) const { return os; }

	// GeoJSON
#ifdef HAVE_JSONCPP
	virtual Json::Value to_json(void) const;
	static Glib::RefPtr<Geometry> create(const Json::Value& g);
#endif

protected:
	mutable gint m_refcount;

	Geometry(void);
	virtual ~Geometry();
	template <typename T> class Var;
};

class GeometryCollection {
public:
	GeometryCollection(void);
	~GeometryCollection();
	GeometryCollection(const Geometry::ptr_t& p);
	GeometryCollection(const GeometryCollection& g);
	GeometryCollection(const Point& pt);
	GeometryCollection(const LineString& ls);
	GeometryCollection(const PolygonSimple& ph);
	GeometryCollection(const PolygonHole& ph);
	GeometryCollection(const MultiPoint& pt);
	GeometryCollection(const MultiLineString& ls);
	GeometryCollection(const MultiPolygonHole& ph);
	GeometryCollection& operator=(const Geometry::ptr_t& p);
	GeometryCollection& operator=(const GeometryCollection& g);
	GeometryCollection& operator=(const Point& pt);
	GeometryCollection& operator=(const LineString& ls);
	GeometryCollection& operator=(const PolygonSimple& ph);
	GeometryCollection& operator=(const PolygonHole& ph);
	GeometryCollection& operator=(const MultiPoint& pt);
	GeometryCollection& operator=(const MultiLineString& ls);
	GeometryCollection& operator=(const MultiPolygonHole& ph);
	GeometryCollection& append(const Geometry::ptr_t& p);
	GeometryCollection& append(const GeometryCollection& g);
	GeometryCollection& append(const Point& pt);
	GeometryCollection& append(const LineString& ls);
	GeometryCollection& append(const PolygonSimple& ph);
	GeometryCollection& append(const PolygonHole& ph);
	GeometryCollection& append(const MultiPoint& pt);
	GeometryCollection& append(const MultiLineString& ls);
	GeometryCollection& append(const MultiPolygonHole& ph);
	std::size_t size(void) const { return m_el.size(); }
	const Geometry::ptr_t& operator[](std::size_t i) const;
	Rect get_bbox(void) const;
	int64_t area2(void) const;
	Geometry::ptr_t flatten(void) const;

	// WKT / WKB
	unsigned int get_wkbsize(void) const;
	uint8_t *wkbencode(uint8_t *cp, uint8_t *ep) const;
	const uint8_t *wkbdecode(const uint8_t *cp, const uint8_t *ep, uint8_t byteorder);
	uint8_t *to_wkb(uint8_t *cp, uint8_t *ep) const;
	std::vector<uint8_t> to_wkb(void) const;
	const uint8_t *from_wkb(const uint8_t *cp, const uint8_t *ep);
	void from_wkb(const std::vector<uint8_t>& b) { from_wkb(&b[0], &b[b.size()]); }
	std::ostream& to_wkt(std::ostream& os) const;

	// GeoJSON
#ifdef HAVE_JSONCPP
	Json::Value to_json(void) const;
	void from_json(const Json::Value& g);
#endif

protected:
	typedef std::vector<Geometry::ptr_t> el_t;
	el_t m_el;
};

template <typename T>
class PolarVector3D;

template <typename T>
class CartesianVector3D {
public:
	typedef T float_t;
	static constexpr float_t from_deg     = M_PI / 180.0;

	CartesianVector3D(float_t x = 0.0, float_t y = 0.0, float_t z = 0.0) { m_v[0] = x; m_v[1] = y; m_v[2] = z; }
	float_t getx(void) const { return m_v[0]; }
	float_t gety(void) const { return m_v[1]; }
	float_t getz(void) const { return m_v[2]; }
	void setx(float_t v) { m_v[0] = v; }
	void sety(float_t v) { m_v[1] = v; }
	void setz(float_t v) { m_v[2] = v; }
	CartesianVector3D<T>& operator+=(const CartesianVector3D<T>& v);
	CartesianVector3D<T>& operator-=(const CartesianVector3D<T>& v);
	CartesianVector3D<T>& operator*=(float_t v);
	CartesianVector3D<T> operator+(const CartesianVector3D<T>& v) const { CartesianVector3D<T> pv(*this); pv += v; return pv; }
	CartesianVector3D<T> operator-(const CartesianVector3D<T>& v) const { CartesianVector3D<T> pv(*this); pv -= v; return pv; }
	CartesianVector3D<T> operator-(void) const { CartesianVector3D<T> pv; pv -= *this; return pv; }
	CartesianVector3D<T> operator*(float_t v) const { CartesianVector3D<T> pv(*this); pv *= v; return pv; }
	float_t scalarprod(const CartesianVector3D<T>& v) const;
	CartesianVector3D<T> crossprod(const CartesianVector3D<T>& v) const;
	float_t length2(void) const;
	float_t length(void) const { return sqrt(length2()); }
	float_t nearest_fraction(const CartesianVector3D<T>& v0, const CartesianVector3D<T>& v1);
	void rotate_xz(float_t ang);
	void rotate_xy(float_t ang);
	operator PolarVector3D<T>(void) const;

	template<class Archive> void hibernate_binary(Archive& ar) {
		for (int i = 0; i < 3; ++i)
			ar.io(m_v[i]);
	}

protected:
	float_t m_v[3];
};

template <typename T>
class PolarVector3D {
public:
	typedef T float_t;
	static constexpr float_t earth_radius = 6378.155 * 1000.0; /* earth_radius in m */
	static constexpr float_t km_to_nmi    = 0.5399568;
	static constexpr float_t m_to_ft      = 3.2808399;
	static constexpr float_t ft_to_m      = 0.3048;
	static constexpr float_t from_deg     = M_PI / 180.0;
	static constexpr float_t to_deg       = 180.0 / M_PI;

	PolarVector3D(float_t lat = 0, float_t lon = 0, float_t r = 0) : m_lat(lat), m_lon(lon), m_r(r) {}
	PolarVector3D(const Point& pt, int16_t altft = 0);
	float_t getlat(void) const { return m_lat; }
	float_t getlatdeg(void) const { return m_lat * to_deg; }
	float_t getlon(void) const { return m_lon; }
	float_t getlondeg(void) const { return m_lon * to_deg; }
	Point getcoord(void) const;
	float_t getalt(void) const { return m_r - earth_radius; }
	float_t getaltft(void) const { return (m_r - earth_radius) * m_to_ft; }
	float_t getr(void) const { return m_r; }
	void setlat(float_t v) { m_lat = v; }
	void setlatdeg(float_t v) { m_lat = v * from_deg; }
	void setlon(float_t v) { m_lon = v; }
	void setlondeg(float_t v) { m_lon = v * from_deg; }
	void setcoord(const Point& pt) { m_lon = pt.get_lon_rad(); m_lat = pt.get_lat_rad(); }
	void setr(float_t v) { m_r = v; }
	void setalt(float_t v) { m_r = v + earth_radius; }
	void setaltft(float_t v) { m_r = v * ft_to_m + earth_radius; }
	PolarVector3D<T>& operator*=(float_t s) { m_r *= s; return *this; }
	PolarVector3D<T> operator*(float_t s) const { PolarVector3D<T> pv(*this); pv *= s; return pv; }
	operator CartesianVector3D<T>(void) const;

	template<class Archive> void hibernate_binary(Archive& ar) {
		ar.io(m_lat);
		ar.io(m_lon);
		ar.io(m_r);
	}

protected:
	float_t m_lat, m_lon, m_r;
};

typedef CartesianVector3D<float> CartesianVector3Df;
typedef PolarVector3D<float> PolarVector3Df;
typedef CartesianVector3D<double> CartesianVector3Dd;
typedef PolarVector3D<double> PolarVector3Dd;

std::ostream& operator<<(std::ostream& os, const Point& p);
std::ostream& operator<<(std::ostream& os, const Rect& r);

const std::string& to_str(BorderPoint::flag_t f);
inline std::ostream& operator<<(std::ostream& os, BorderPoint::flag_t f) { return os << to_str(f); }
std::string to_str(PolygonSimple::inside_t i);
inline std::ostream& operator<<(std::ostream& os, PolygonSimple::inside_t i) { return os << to_str(i); }

inline PolygonSimple::inside_t operator|(PolygonSimple::inside_t x, PolygonSimple::inside_t y) { return (PolygonSimple::inside_t)((unsigned int)x | (unsigned int)y); }
inline PolygonSimple::inside_t operator&(PolygonSimple::inside_t x, PolygonSimple::inside_t y) { return (PolygonSimple::inside_t)((unsigned int)x & (unsigned int)y); }
inline PolygonSimple::inside_t operator^(PolygonSimple::inside_t x, PolygonSimple::inside_t y) { return (PolygonSimple::inside_t)((unsigned int)x ^ (unsigned int)y); }
inline PolygonSimple::inside_t operator~(PolygonSimple::inside_t x){ return (PolygonSimple::inside_t)~(unsigned int)x; }
inline PolygonSimple::inside_t& operator|=(PolygonSimple::inside_t& x, PolygonSimple::inside_t y) { x = x | y; return x; }
inline PolygonSimple::inside_t& operator&=(PolygonSimple::inside_t& x, PolygonSimple::inside_t y) { x = x & y; return x; }
inline PolygonSimple::inside_t& operator^=(PolygonSimple::inside_t& x, PolygonSimple::inside_t y) { x = x ^ y; return x; }

inline BorderPoint::flagmask_t operator|(BorderPoint::flagmask_t x, BorderPoint::flagmask_t y) { return (BorderPoint::flagmask_t)((unsigned int)x | (unsigned int)y); }
inline BorderPoint::flagmask_t operator&(BorderPoint::flagmask_t x, BorderPoint::flagmask_t y) { return (BorderPoint::flagmask_t)((unsigned int)x & (unsigned int)y); }
inline BorderPoint::flagmask_t operator^(BorderPoint::flagmask_t x, BorderPoint::flagmask_t y) { return (BorderPoint::flagmask_t)((unsigned int)x ^ (unsigned int)y); }
inline BorderPoint::flagmask_t operator~(BorderPoint::flagmask_t x){ return (BorderPoint::flagmask_t)~(unsigned int)x; }
inline BorderPoint::flagmask_t& operator|=(BorderPoint::flagmask_t& x, BorderPoint::flagmask_t y) { x = x | y; return x; }
inline BorderPoint::flagmask_t& operator&=(BorderPoint::flagmask_t& x, BorderPoint::flagmask_t y) { x = x & y; return x; }
inline BorderPoint::flagmask_t& operator^=(BorderPoint::flagmask_t& x, BorderPoint::flagmask_t y) { x = x ^ y; return x; }

const std::string& to_str(Geometry::type_t t);
inline std::ostream& operator<<(std::ostream& os, Geometry::type_t t) { return os << to_str(t); }

#endif /* GEOM_H */
