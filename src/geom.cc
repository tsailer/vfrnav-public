/***************************************************************************
 *   Copyright (C) 2007, 2009, 2012, 2013, 2014, 2015, 2016, 2017, 2018    *
 *     by Thomas Sailer t.sailer@alumni.ethz.ch                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "sysdeps.h"

#ifdef BOOST_INT128
#include <boost/multiprecision/cpp_int.hpp>
#endif

#include <limits>
#include <sqlite3x.hpp>
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <endian.h>
#include "geom.h"

#ifdef HAVE_JSONCPP
#include <json/json.h>
#endif

const Point::coord_t Point::lon_min;
const Point::coord_t Point::lon_max;
const Point::coord_t Point::lat_min;
const Point::coord_t Point::lat_max;
const Point Point::invalid = Point(0, 0x80000000);

constexpr float Point::to_deg;
constexpr float Point::to_min;
constexpr float Point::to_sec;
constexpr float Point::to_rad;
constexpr float Point::from_deg;
constexpr float Point::from_min;
constexpr float Point::from_sec;
constexpr float Point::from_rad;

constexpr double Point::to_deg_dbl;
constexpr double Point::to_min_dbl;
constexpr double Point::to_sec_dbl;
constexpr double Point::to_rad_dbl;
constexpr double Point::from_deg_dbl;
constexpr double Point::from_min_dbl;
constexpr double Point::from_sec_dbl;
constexpr double Point::from_rad_dbl;

const int32_t Point::pole_lat;

constexpr float Point::earth_radius;
constexpr float Point::km_to_nmi;
constexpr float Point::m_to_ft;
constexpr float Point::ft_to_m;
constexpr float Point::mps_to_kts;

constexpr double Point::earth_radius_dbl;
constexpr double Point::km_to_nmi_dbl;
constexpr double Point::m_to_ft_dbl;
constexpr double Point::ft_to_m_dbl;
constexpr double Point::mps_to_kts_dbl;

const unsigned int Point::setstr_lat;
const unsigned int Point::setstr_lon;
const unsigned int Point::setstr_excess;
const unsigned int Point::setstr_flags_locator;
const unsigned int Point::setstr_flags_ch1903;

void Point::set_invalid(void)
{
	m_lon = 0;
	m_lat = 0x80000000;
}

int Point::compare(const Point& x) const
{
	if (get_lat() < x.get_lat())
		return -1;
	if (x.get_lat() < get_lat())
		return 1;
	if (get_lon() < x.get_lon())
		return -1;
	if (x.get_lon() < get_lon())
		return 1;
	return 0;
}

Rect Point::get_bbox(void) const
{
	return Rect(*this, *this);
}

/*
 * cf. Joseph O'Rourke, Computational Geometry in C
 */

/*
 * softSurfer's (www.softsurfer.com) variant uses less multiplications,
 * but computes the same value
 * http://geometryalgorithms.com/Archive/algorithm_0103/algorithm_0103.htm
 */

/* twice the signed area of the triangle p0, p1, p2 */

int64_t Point::area2(const Point& p0, const Point& p1, const Point& p2)
{
#if 1
	int64_t p2x = p2.get_lon();
	int64_t p2y = p2.get_lat();
	int64_t p0x = p0.get_lon() - p2x;
	int64_t p0y = p0.get_lat() - p2y;
	int64_t p1x = p1.get_lon() - p2x;
	int64_t p1y = p1.get_lat() - p2y;
	return p0x * (p1y - p0y) - p0y * (p1x - p0x);
#else
	return p0.get_lon() * (int64_t)p1.get_lat() - p0.get_lat() * (int64_t)p1.get_lon() +
			p2.get_lon() * (int64_t)p0.get_lat() - p2.get_lat() * (int64_t)p0.get_lon() +
			p1.get_lon() * (int64_t)p2.get_lat() - p1.get_lat() * (int64_t)p2.get_lon();
#endif
}

inline bool Point::is_between_helper(const Point& p0, const Point& p1) const
{
#if 1
	Rect bbox(p0, p0);
	bbox = bbox.add(p1);
	return bbox.is_inside(*this);
#else
	if (p0.get_lon() != p1.get_lon())
		return ((get_lon() >= p0.get_lon() && get_lon() <= p1.get_lon()) ||
				(get_lon() >= p1.get_lon() && get_lon() <= p0.get_lon()));
	return ((get_lat() >= p0.get_lat() && get_lat() <= p1.get_lat()) ||
			(get_lat() >= p1.get_lat() && get_lat() <= p0.get_lat()));
#endif
}

inline bool Point::is_strictly_between_helper(const Point& p0, const Point& p1) const
{
	Rect bbox(p0, p0);
	bbox = bbox.add(p1);
	return bbox.is_strictly_inside(*this);
}

inline bool Point::intersect_precheck(const Point& l0a, const Point& l0b, const Point& l1a, const Point& l1b)
{
	int32_t xmin = l0a.get_lon();
	int32_t xmax = l0b.get_lon();
	int32_t ymin = l0a.get_lat();
	int32_t ymax = l0b.get_lat();
	if (xmin > xmax)
		std::swap(xmin, xmax);
	if (ymin > ymax)
		std::swap(ymin, ymax);
	if (l1a.get_lon() > xmax && l1b.get_lon() > xmax)
		return false;
	if (l1a.get_lon() < xmin && l1b.get_lon() < xmin)
		return false;
	if (l1a.get_lat() > ymax && l1b.get_lat() > ymax)
		return false;
	if (l1a.get_lat() < ymin && l1b.get_lat() < ymin)
		return false;
	return true;
}

/*
 * "proper" intersection, i.e. not one endpoint collinear with the other line
 */

bool Point::is_proper_intersection(const Point& l0a, const Point& l0b, const Point& l1a, const Point& l1b)
{
	if (!intersect_precheck(l0a, l0b, l1a, l1b))
		return false;
	int64_t abc = area2(l0a, l0b, l1a);
	int64_t abd = area2(l0a, l0b, l1b);
	int64_t cda = area2(l1a, l1b, l0a);
	int64_t cdb = area2(l1a, l1b, l0b);
	if (!abc || !abd || !cda || !cdb)
		return false;
	return ((abc >= 0) ^ (abd >= 0)) && ((cda >= 0) ^ (cdb >= 0));
}

/*
 * intersection, including one endpoint lying on the other line
 */

bool Point::is_intersection(const Point& l0a, const Point& l0b, const Point& l1a, const Point& l1b)
{
	if (!intersect_precheck(l0a, l0b, l1a, l1b))
		return false;
	int64_t abc = area2(l0a, l0b, l1a);
	int64_t abd = area2(l0a, l0b, l1b);
	int64_t cda = area2(l1a, l1b, l0a);
	int64_t cdb = area2(l1a, l1b, l0b);
	if (abc && abd && cda && cdb && ((abc >= 0) ^ (abd >= 0)) && ((cda >= 0) ^ (cdb >= 0)))
		return true;
	if (!abc && l1a.is_between_helper(l0a, l0b))
		return true;
	if (!abd && l1b.is_between_helper(l0a, l0b))
		return true;
	if (!cda && l0a.is_between_helper(l1a, l1b))
		return true;
	if (!cdb && l0b.is_between_helper(l1a, l1b))
		return true;
	return false;
}

/*
 * intersection, excluding one endpoint lying on the other line
 */

bool Point::is_strict_intersection(const Point& l0a, const Point& l0b, const Point& l1a, const Point& l1b)
{
	if (!intersect_precheck(l0a, l0b, l1a, l1b))
		return false;
	int64_t abc = area2(l0a, l0b, l1a);
	int64_t abd = area2(l0a, l0b, l1b);
	int64_t cda = area2(l1a, l1b, l0a);
	int64_t cdb = area2(l1a, l1b, l0b);
	if (abc && abd && cda && cdb && ((abc >= 0) ^ (abd >= 0)) && ((cda >= 0) ^ (cdb >= 0)))
		return true;
	if (!abc && l1a.is_strictly_between_helper(l0a, l0b))
		return true;
	if (!abd && l1b.is_strictly_between_helper(l0a, l0b))
		return true;
	if (!cda && l0a.is_strictly_between_helper(l1a, l1b))
		return true;
	if (!cdb && l0b.is_strictly_between_helper(l1a, l1b))
		return true;
	return false;
}

bool Point::is_between(const Point& p0, const Point& p1) const
{
	if (area2(p0, p1, *this))
		return false;
	return is_between_helper(p0, p1);
}

bool Point::is_strictly_between(const Point& p0, const Point& p1) const
{
	if (area2(p0, p1, *this))
		return false;
	return is_strictly_between_helper(p0, p1);
}

bool Point::is_convex(const Point& vprev, const Point& vnext) const
{
	return vnext.is_left_or_on(vprev, *this);
}

bool Point::is_in_cone(const Point& vprev, const Point& v, const Point& vnext) const
{
	if (v.is_convex(vprev, vnext))
		return vprev.is_strictly_left(v, *this) &&
				vnext.is_strictly_left(*this, v);
	return !(vnext.is_left_or_on(v, *this) &&
			vprev.is_left_or_on(*this, v));
}

/*
 * based on an algorithm by softSurfer (www.softsurfer.com)
 * http://geometryalgorithms.com/Archive/algorithm_0103/algorithm_0103.htm
 */

/*
 * left_of: tests if a point is Left|On|Right of an infinite line.
 *    Input:  three points P0, P1, and P2
 *    Return: >0 for P2 left of the line through P0 and P1
 *	    =0 for P2 on the line
 *	    <0 for P2 right of the line
 *    See: the January 2001 Algorithm "Area of 2D and 3D Triangles and Polygons"
 */

inline bool Point::is_left_of(const Point& p0, const Point& p1) const
{
	int64_t lon = area2(p0, p1, *this);
	if (lon < 0)
		return -1;
	if (lon > 0)
		return 1;
	return 0;
}

bool Point::left_of(const Point& p0, const Point& p1) const
{
	int64_t p0x = p0.get_lon() - get_lon();
	int64_t p0y = p0.get_lat() - get_lat();
	int64_t p1x = p1.get_lon() - get_lon();
	int64_t p1y = p1.get_lat() - get_lat();
	int64_t lon = p0x * (p1y - p0y) - p0y * (p1x - p0x);
	if (lon < 0)
		return -1;
	if (lon > 0)
		return 1;
	return 0;
}

bool Point::in_box(const Point& p0, const Point& p1) const
{
	Point x(*this - p0), y(p1 - p0);
	if (y.get_lon() < 0) {
		if (x.get_lon() < y.get_lon() || x.get_lon() > 0)
			return false;
	} else {
		if (x.get_lon() > y.get_lon() || x.get_lon() < 0)
			return false;
	}
	if (y.get_lat() < 0) {
		if (x.get_lat() < y.get_lat() || x.get_lat() > 0)
			return false;
	} else {
		if (x.get_lat() > y.get_lat() || x.get_lat() < 0)
			return false;
	}
	return true;
}

/* http://mathworld.wolfram.com/Line-LineIntersection.html */

bool Point::intersection(const Point& l0a, const Point& l0b, const Point& l1a, const Point& l1b)
{
	int64_t x1 = l0a.get_lon();
	int64_t y1 = l0a.get_lat();
	int64_t x2 = l0b.get_lon();
	int64_t y2 = l0b.get_lat();
	int64_t x3 = l1a.get_lon();
	int64_t y3 = l1a.get_lat();
	int64_t x4 = l1b.get_lon();
	int64_t y4 = l1b.get_lat();
	int64_t detd = (x1 - x2) * (y3 - y4) - (x3 - x4) * (y1 - y2);
	if (!detd)
		return false;
	int64_t det1 = (x1 * y2) - (x2 * y1);
	int64_t det2 = (x3 * y4) - (x4 * y3);
	if (false)
		std::cerr << "detd " << detd << " det1 " << det1 << " det2 " << det2
			  << " x1-x2 " << (x1 - x2) << " y1-y2 " << (y1 - y2)
			  << " x3-x4 " << (x3 - x4) << " y3-y4 " << (y3 - y4) << std::endl;
#if 1
	set_lon(Point::round<Point::coord_t,double>((det1 * (double)(x3 - x4) - det2 * (double)(x1 - x2)) / detd));
	set_lat(Point::round<Point::coord_t,double>((det1 * (double)(y3 - y4) - det2 * (double)(y1 - y2)) / detd));
#else
	set_lon((det1 * (x3 - x4) - det2 * (x1 - x2) + detd / 2) / detd);
	set_lat((det1 * (y3 - y4) - det2 * (y1 - y2) + detd / 2) / detd);
#endif
	return true;
}

// return the point on an infinite line through la and lb nearest to *this

Point Point::nearest_on_line(const Point& la, const Point& lb) const
{
	// a: vector from point to line
	// n: line normal vector
	// x: nearest point on line
	// x = a - (n^T * a) / (n^T * n) * n
	if (is_invalid()) {
		Point pt;
		pt.set_invalid();
		return pt;
	}
	Point pa(la - *this);
	Point pn(lb - la);
	double c(cos(this->get_lat_rad_dbl()));
	if (c < 1e-10) {
		Point pt;
		pt.set_invalid();
		return pt;
	}
	double cc(c * c);
	double ax(pa.get_lon());
	double ay(pa.get_lat());
	double nx(pn.get_lon());
	double ny(pn.get_lat());
	double t(cc * nx * nx + ny * ny);
	if (fabs(t) < 1e-10) {
		Point pt;
		pt.set_invalid();
		return pt;
	}
	t = (cc * nx * ax + ny * ay) / t;
	Point p(ax - t * nx, ay - t * ny);
	return p + *this;
}

Point Point::spheric_nearest_on_line(const Point& la, const Point& lb) const
{
	// http://stackoverflow.com/questions/1299567/how-to-calculate-distance-from-a-point-to-a-line-segment-on-a-sphere
	CartesianVector3Dd A((CartesianVector3Dd)PolarVector3Dd(la));
	CartesianVector3Dd B((CartesianVector3Dd)PolarVector3Dd(lb));
	CartesianVector3Dd C((CartesianVector3Dd)PolarVector3Dd(*this));
	// G = A x B
	CartesianVector3Dd G(A.crossprod(B));
	// F = C x G
	CartesianVector3Dd F(C.crossprod(G));
	// T = G x F
	PolarVector3Dd T(G.crossprod(F));
	Point pt1(T.getcoord());
	Point pt2(-pt1.get_lon(), -pt1.get_lat());
	return (simple_distance_rel(pt1) < simple_distance_rel(pt2)) ? pt1 : pt2;
}

Point Point::simple_unwrap(const Point& p2) const
{
	float r1 = cosf(((*this) + p2).get_lat_rad() * 0.5);
	Point p(p2 - *this);
	p.set_lon((coord_t)(r1 * p.get_lon()));
	return p;
}

float Point::simple_true_course(const Point& p2) const
{
	Point p(simple_unwrap(p2));
	float v = atan2f(p.get_lon(), p.get_lat()) * (180.0 / M_PI);
	if (v < 0)
		v += 360;
	return v;
}

uint64_t Point::simple_distance_rel( const Point & p2 ) const
{
	Point p(simple_unwrap(p2));
	return p.length2();
}

float Point::simple_distance_km(const Point& p2) const
{
	Point p(simple_unwrap(p2));
	return p.length() * (earth_radius * to_rad);
}

Point Point::simple_course_distance_nmi(float course, float dist) const
{
	float r1 = cosf(get_lat_rad());
	float r2 = course * (M_PI / 180.0);
	float x = sinf(r2);
	float y = cosf(r2);
	r2 = r1 * r1 * x * x + y * y;
	if (r2 < 0.001)
		return *this;
	r2 = dist * (1.0 / km_to_nmi / earth_radius / to_rad) / sqrt(r2);
	x *= r2;
	y *= r2;
	return (*this) + Point((coord_t)x, (coord_t)y);
}

Rect Point::simple_box_km(float centerdist) const
{
	centerdist *= (from_rad / earth_radius);
	float distlon = centerdist;
	if (abs(get_lat()) < pole_lat)
		distlon /= cosf(get_lat_rad());
	Point pdist((int32_t)distlon, (int32_t)centerdist);
	Point sw(*this - pdist), ne(*this + pdist);
	if (sw.get_lat() < -pole_lat)
		sw.set_lat(-pole_lat);
	if (ne.get_lat() > pole_lat)
		ne.set_lat(pole_lat);
	return Rect(sw, ne);
}

Rect Point::simple_box_nmi(float centerdist) const
{
	return simple_box_km(centerdist * (float)(1.0 / km_to_nmi));
}

/*
 * http://en.wikipedia.org/wiki/Web_Mercator
 */

const double Point::webmercator_maxlat_rad_dbl = 2.0 * atan(exp(M_PI)) - M_PI * 0.5;
const double Point::webmercator_maxlat_deg_dbl = 180.0 / M_PI * (2.0 * atan(exp(M_PI)) - M_PI * 0.5);
const Point::coord_t Point::webmercator_maxlat = (1UL << 31) / M_PI * (2.0 * atan(exp(M_PI)) - M_PI * 0.5);

bool Point::is_webmercator_valid(void) const
{
	return get_lat() < webmercator_maxlat && get_lat() > -webmercator_maxlat;
}

int32_t Point::get_lon_webmercator(unsigned int zoomlevel) const
{
	return ((get_lon() + (1 << (31 - 8 - zoomlevel))) >> (31 - 7 - zoomlevel)) + (128 << zoomlevel);
}

int32_t Point::get_lat_webmercator(unsigned int zoomlevel) const
{
	return round<int32_t,double>(get_lat_webmercator_dbl(zoomlevel));
}

void Point::set_lon_webmercator(unsigned int zoomlevel, int32_t x)
{
	set_lon((x - (128 << zoomlevel)) << (31 - 7 - zoomlevel));
}

void Point::set_lat_webmercator(unsigned int zoomlevel, int32_t y)
{
	set_lat_webmercator_dbl(zoomlevel, y);
}

double Point::get_lon_webmercator_dbl(unsigned int zoomlevel) const
{
	return ldexp(get_lon(), zoomlevel + (7 - 31)) + (128 << zoomlevel);
}

double Point::get_lat_webmercator_dbl(unsigned int zoomlevel) const
{
	if (get_lat() > webmercator_maxlat)
		return -1;
	if (get_lat() < -webmercator_maxlat)
		return ldexp(256, zoomlevel) + 1;
	return ldexp((128.0 / M_PI) * (M_PI - log(tan(0.25 * M_PI + to_rad_dbl * 0.5 * get_lat()))), zoomlevel);
}

void Point::set_lon_webmercator_dbl(unsigned int zoomlevel, double x)
{
	set_lon(round<coord_t,double>(ldexp(x - (128 << zoomlevel), 31 - 7 - zoomlevel)));
}

void Point::set_lat_webmercator_dbl(unsigned int zoomlevel, double y)
{
	set_lat(round<coord_t,double>((atan(exp(M_PI - ldexp(y * (M_PI / 128.0), -zoomlevel))) - 0.25 * M_PI) * (2.0 * from_rad_dbl)));
}

/*
 * Spheric Geometry
 * http://de.wikipedia.org/wiki/Orthodrome
 * http://williams.best.vwh.net/avform.htm
 */

#define USE_HAVERSINE

float Point::spheric_true_course(const Point& p2) const
{
	Point pdiff(p2 - *this);
	float lat1 = get_lat_rad(), lat2 = p2.get_lat_rad();
	float sinlat1 = sinf(lat1), coslat1 = cosf(lat1);
	float sinlat2 = sinf(lat2), coslat2 = cosf(lat2);
	float londiff = pdiff.get_lon_rad();
	float crs = atan2f(sinf(londiff) * coslat2, coslat1 * sinlat2 - sinlat1 * coslat2 * cosf(londiff));
	crs *= (180.0 / M_PI);
	if (crs < 0)
		crs += 360;
	return crs;
}

float Point::spheric_distance_km(const Point& p2) const
{
	Point pdiff(p2 - *this);
#if defined(USE_HAVERSINE)
	float slatd = sinf(pdiff.get_lat_rad() * 0.5f);
	float slond = sinf(pdiff.get_lon_rad() * 0.5f);
	return 2.0f * earth_radius * asinf(std::min(1.f, sqrtf(slatd * slatd + cosf(get_lat_rad()) * cosf(p2.get_lat_rad()) * slond * slond)));
#else
	float lat = pdiff.get_lat_rad();
	float lon = pdiff.get_lon_rad() * cosf(get_lat_rad());
	float dist = acosf(cosf(lat) * cosf(lon));
	//double dist = acos(cos(lat) * cos(lon));
	return dist * earth_radius;
#endif
}

Point Point::spheric_course_distance_nmi(float course, float dist) const
{
	//std::cerr << "spheric_course_distance_nmi: crs " << course << " d " << dist << " pt " << (*this) << std::endl;
	dist *= (1.0f / km_to_nmi / earth_radius);
	course *= (M_PI / 180.f);
	float sintc = sinf(course), costc = cosf(course);
	float lat1 = get_lat_rad();
	float sinlat1 = sinf(lat1), coslat1 = cosf(lat1);
	float cosd = cosf(dist), sind = sinf(dist);
	float sinlat = sinlat1 * cosd + coslat1 * sind * costc;
	float lat = asinf(sinlat);
#if 1
	float dlon = atan2f(sintc * sind * coslat1, cosd - sinlat1 * sinlat);
#else
	float dlon = 0;
	float coslat = cosf(lat);
	if (fabsf(coslat) > 1e-10)
		dlon = asinf(sintc * sind / coslat);
#endif
	Point p;
	p.set_lat_rad(lat);
	p.set_lon_rad(dlon);
	p.set_lon(get_lon() + p.get_lon());
	return p;
}

double Point::spheric_true_course_dbl(const Point& p2) const
{
	Point pdiff(p2 - *this);
	double lat1 = get_lat_rad_dbl(), lat2 = p2.get_lat_rad_dbl();
	double sinlat1 = sin(lat1), coslat1 = cos(lat1);
	double sinlat2 = sin(lat2), coslat2 = cos(lat2);
	double londiff = pdiff.get_lon_rad();
	double crs = atan2(sin(londiff) * coslat2, coslat1 * sinlat2 - sinlat1 * coslat2 * cos(londiff));
	crs *= (180.0 / M_PI);
	if (crs < 0)
		crs += 360;
	return crs;
}

double Point::spheric_distance_km_dbl(const Point& p2) const
{
	Point pdiff(p2 - *this);
#if defined(USE_HAVERSINE)
	double slatd = sin(pdiff.get_lat_rad_dbl() * 0.5);
	double slond = sin(pdiff.get_lon_rad_dbl() * 0.5);
	return 2.0 * earth_radius_dbl * asin(std::min(1., sqrt(slatd * slatd + cos(get_lat_rad_dbl()) * cos(p2.get_lat_rad_dbl()) * slond * slond)));
#else
	double lat = pdiff.get_lat_rad_dbl();
	double lon = pdiff.get_lon_rad_dbl() * cos(get_lat_rad_dbl());
	double dist = acos(cos(lat) * cos(lon));
	return dist * earth_radius_dbl;
#endif
}

Point Point::halfway(const Point& pt2) const
{
	Point pd(pt2 - *this);
	pd = Point(pd.get_lon() / 2, pd.get_lat() / 2);
	return *this + pd;
}

std::pair<Point,double> Point::get_gcnav(const Point& p2, double frac) const
{
	double lambda12(p2.get_lon_rad_dbl() - get_lon_rad_dbl());
	double sinlambda12(sin(lambda12));
	double coslambda12(cos(lambda12));
	double alpha1(atan2(sinlambda12, cos(get_lat_rad_dbl()) * tan(p2.get_lat_rad_dbl()) -
			    sin(get_lat_rad_dbl()) * coslambda12));
	double alpha2(atan2(sinlambda12, -cos(p2.get_lat_rad_dbl()) * tan(get_lat_rad_dbl()) +
			    sin(p2.get_lat_rad_dbl()) * coslambda12));
	double x(cos(get_lat_rad_dbl()) * sin(p2.get_lat_rad_dbl()) -
		 sin(get_lat_rad_dbl()) * cos(p2.get_lat_rad_dbl()) * coslambda12);
	double y(cos(p2.get_lat_rad_dbl()) * sinlambda12);
	double sigma12(atan2(sqrt(x * x + y * y),
			     sin(get_lat_rad_dbl()) * sin(p2.get_lat_rad_dbl()) +
			     cos(get_lat_rad_dbl()) * cos(p2.get_lat_rad_dbl()) * coslambda12));
	x = cos(alpha1);
	y = sin(alpha1) * sin(get_lat_rad_dbl());
	double alpha0(atan2(sin(alpha1) * cos(get_lat_rad_dbl()), sqrt(x * x + y * y)));
	x = cos(alpha1);
	double sigma01(0);
	if (fabs(get_lat_rad_dbl()) > 1e-5 || fabs(x) > 1e-5)
		sigma01 = atan2(tan(get_lat_rad_dbl()), x);
	double sigma02(sigma01 + sigma12);
	double lambda01(atan2(sin(alpha0) * sin(sigma01), cos(sigma01)));
	double lambda0(get_lon_rad_dbl() - lambda01);
	double sigma(sigma01 + (sigma02 - sigma01) * frac);
	std::pair<Point,double> r;
	r.first.set_lat_rad_dbl(asin(cos(alpha0) * sin(sigma)));
	r.first.set_lon_rad_dbl(atan2(sin(alpha0) * sin(sigma), cos(sigma)) + lambda0);
	r.second = atan2(tan(alpha0), cos(sigma)) * (180.0 / M_PI);
	return r;
}

float Point::rawcoord_course(const Point& p2) const
{
	Point pdiff(p2 - *this);
	return std::atan2((float)pdiff.get_lat(), (float)pdiff.get_lon()) * (180.0 / M_PI);
}

Point Point::rawcoord_course_distance(float course, float dist) const
{
	course *= (M_PI / 180.f);
	Point pt;
	pt.set_lon(Point::round<float,int32_t>(get_lon() + dist * std::cos(course)));
	pt.set_lat(Point::round<float,int32_t>(get_lat() + dist * std::sin(course)));
	return pt;
}

double Point::rawcoord_true_course_dbl(const Point& p2) const
{
	Point pdiff(p2 - *this);
	return std::atan2((double)pdiff.get_lat(), (double)pdiff.get_lon()) * (180.0 / M_PI);
}

Point Point::rawcoord_course_distance(double course, double dist) const
{
	course *= (M_PI / 180.);
	Point pt;
	pt.set_lon(Point::round<double,int32_t>(get_lon() + dist * std::cos(course)));
	pt.set_lat(Point::round<double,int32_t>(get_lat() + dist * std::sin(course)));
	return pt;
}

PolygonSimple Point::make_circle(float radius, float angleincr, bool rawcoord) const
{
	if (is_invalid() || std::isnan(radius) || std::isnan(angleincr) ||
	    radius <= 0 || fabsf(angleincr) < 0.1)
		return PolygonSimple();
	PolygonSimple ps;
	if (rawcoord) {
		if (angleincr >= 0) {
			for (float i = 0; i > -360.0; i -= angleincr)
				ps.push_back(rawcoord_course_distance(i, radius));
		} else {
			for (float i = 0; i < 360.0; i -= angleincr)
				ps.push_back(rawcoord_course_distance(i, radius));
		}
		return ps;
	}
	// positive angleincr makes the polygon counterclockwise
	if (angleincr >= 0) {
		for (float i = 0; i > -360.0; i -= angleincr)
			ps.push_back(spheric_course_distance_nmi(i, radius));
	} else {
		for (float i = 0; i < 360.0; i -= angleincr)
			ps.push_back(spheric_course_distance_nmi(i, radius));
	}
	return ps;
}

PolygonSimple Point::make_fat_line(const Point& p, float radius, bool roundcaps, float angleincr, bool rawcoord) const
{
	if (is_invalid() || p.is_invalid() || std::isnan(radius) || radius <= 0 ||
	    (roundcaps && (std::isnan(angleincr) || angleincr < 0.1)))
		return PolygonSimple();
	PolygonSimple ps;
	if (rawcoord) {
		float angle(rawcoord_course(p));
		if (roundcaps) {
			for (float i = -90; i > -270; i -= angleincr)
				ps.push_back(rawcoord_course_distance(angle + i, radius));
			ps.push_back(rawcoord_course_distance(angle - 270, radius));
			for (float i = 90; i > -90; i -= angleincr)
				ps.push_back(p.rawcoord_course_distance(angle + i, radius));
			ps.push_back(p.rawcoord_course_distance(angle - 90, radius));
			return ps;
		}
		radius *= sqrtf(2);
		ps.push_back(rawcoord_course_distance(angle - 135, radius));
		ps.push_back(rawcoord_course_distance(angle - 225, radius));
		ps.push_back(p.rawcoord_course_distance(angle + 45, radius));
		ps.push_back(p.rawcoord_course_distance(angle - 45, radius));
		return ps;
	}
	float angle(spheric_true_course(p));
	if (roundcaps) {
		for (float i = -90; i > -270; i -= angleincr)
			ps.push_back(spheric_course_distance_nmi(angle + i, radius));
		ps.push_back(spheric_course_distance_nmi(angle - 270, radius));
		for (float i = 90; i > -90; i -= angleincr)
			ps.push_back(p.spheric_course_distance_nmi(angle + i, radius));
		ps.push_back(p.spheric_course_distance_nmi(angle - 90, radius));
		return ps;
	}
	radius *= sqrtf(2);
	ps.push_back(spheric_course_distance_nmi(angle - 135, radius));
	ps.push_back(spheric_course_distance_nmi(angle - 225, radius));
	ps.push_back(p.spheric_course_distance_nmi(angle + 45, radius));
	ps.push_back(p.spheric_course_distance_nmi(angle - 45, radius));
	return ps;
}

Glib::ustring Point::to_ustring(const std::wstring& str)
{
	GError *error(0);

#if SIZEOF_WCHAR_T == 4
	glong n_bytes(0);
	const Glib::ScopedPtr<char> buf(g_ucs4_to_utf8(reinterpret_cast<const gunichar*>(str.data()), str.size(), 0, &n_bytes, &error));
#elif defined(G_OS_WIN32) && SIZEOF_WCHAR_T == 2
	glong n_bytes(0);
	const Glib::ScopedPtr<char> buf(g_utf16_to_utf8(reinterpret_cast<const gunichar2*>(str.data()), str.size(), 0, &n_bytes, &error));
#else
	gsize n_bytes(0);
	const Glib::ScopedPtr<char> buf(g_convert(reinterpret_cast<const char*>(str.data()), str.size() * sizeof(std::wstring::value_type), "UTF-8", "WCHAR_T", 0, &n_bytes, &error));
#endif
	if (error)
		Glib::Error::throw_exception(error);
	return Glib::ustring(buf.get(), buf.get() + n_bytes);
}

Glib::ustring Point::get_lon_str(void) const
{
	int32_t min(get_lon());
	char sgn((min < 0) ? 'W' : 'E');
	min = round<int32_t,double>(abs(min) * (to_deg_dbl * 6000000));
	int32_t deg(min / 6000000);
	min -= deg * 6000000;
	std::wostringstream oss;
	oss << sgn << std::setw(3) << std::setfill(L'0') << deg << (wchar_t)0xb0
	    << std::fixed << std::setw(8) << std::setfill(L'0') << std::setprecision(5) << (min * 0.00001) << '\'';
	return to_ustring(oss.str());
}

Glib::ustring Point::get_lat_str(void) const
{
	int32_t min(get_lat());
	char sgn((min < 0) ? 'S' : 'N');
	min = round<int32_t,double>(abs(min) * (to_deg_dbl * 6000000));
	int32_t deg(min / 6000000);
	min -= deg * 6000000;
	std::wostringstream oss;
	oss << sgn << std::setw(2) << std::setfill(L'0') << deg << (wchar_t)0xb0
	    << std::fixed << std::setw(8) << std::setfill(L'0') << std::setprecision(5) << (min * 0.00001) << '\'';
	return to_ustring(oss.str());
}

std::string Point::get_lon_str2(void) const
{
	int32_t min(get_lon());
	char sgn((min < 0) ? 'W' : 'E');
	min = round<int32_t,double>(abs(min) * (to_deg_dbl * 6000000));
	int32_t deg(min / 6000000);
	min -= deg * 6000000;
	std::ostringstream oss;
	oss << sgn << std::setw(3) << std::setfill('0') << deg << ' '
	    << std::fixed << std::setw(8) << std::setfill('0') << std::setprecision(5) << (min * 0.00001);
	return oss.str();
}

std::string Point::get_lat_str2(void) const
{
	int32_t min(get_lat());
	char sgn((min < 0) ? 'S' : 'N');
	min = round<int32_t,double>(abs(min) * (to_deg_dbl * 6000000));
	int32_t deg(min / 6000000);
	min -= deg * 6000000;
	std::ostringstream oss;
	oss << sgn << std::setw(2) << std::setfill('0') << deg << ' '
	    << std::fixed << std::setw(8) << std::setfill('0') << std::setprecision(5) << (min * 0.00001);
	return oss.str();
}

std::string Point::get_lon_str3(void) const
{
	int32_t min(get_lon());
	char sgn((min < 0) ? 'W' : 'E');
	min = round<int32_t,double>(abs(min) * (to_deg_dbl * 6000));
	int32_t deg(min / 6000);
	min -= deg * 6000;
	std::ostringstream oss;
	oss << sgn << std::setw(3) << std::setfill('0') << deg << ' '
	    << std::fixed << std::setw(5) << std::setfill('0') << std::setprecision(2) << (min * 0.01);
	return oss.str();
}

std::string Point::get_lat_str3(void) const
{
	int32_t min(get_lat());
	char sgn((min < 0) ? 'S' : 'N');
	min = round<int32_t,double>(abs(min) * (to_deg_dbl * 6000));
	int32_t deg(min / 6000);
	min -= deg * 6000;
	std::ostringstream oss;
	oss << sgn << std::setw(2) << std::setfill('0') << deg << ' '
	    << std::fixed << std::setw(5) << std::setfill('0') << std::setprecision(2) << (min * 0.01);
	return oss.str();
}

std::string Point::get_lon_adexp_str(void) const
{
	int32_t sec(get_lon());
	char sgn((sec < 0) ? 'W' : 'E');
	sec = round<int32_t,double>(abs(sec) * (to_deg_dbl * 60 * 60));
	int32_t min(sec / 60);
	sec -= min * 60;
	int32_t deg(min / 60);
	min -= deg * 60;
	std::ostringstream oss;
	oss << std::setw(3) << std::setfill('0') << deg << std::setw(2) << std::setfill('0') << min
	    << std::setw(2) << std::setfill('0') << sec << sgn;
	return oss.str();
}

std::string Point::get_lat_adexp_str(void) const
{
	int32_t sec(get_lat());
	char sgn((sec < 0) ? 'S' : 'N');
	sec = round<int32_t,double>(abs(sec) * (to_deg_dbl * 60 * 60));
	int32_t min(sec / 60);
	sec -= min * 60;
	int32_t deg(min / 60);
	min -= deg * 60;
	std::ostringstream oss;
	oss << std::setw(2) << std::setfill('0') << deg << std::setw(2) << std::setfill('0') << min
	    << std::setw(2) << std::setfill('0') << sec << sgn;
	return oss.str();
}

std::string Point::get_lon_fpl_str(fplstrmode_t mode) const
{
	int32_t lonsec(get_lon()), lonmin, londeg;
	char lonsgn((lonsec < 0) ? 'W' : 'E');
	lonsec = round<int32_t,double>(abs(lonsec) * (to_deg_dbl * 60 * 60));
	switch (mode & 3) {
	case 0:
		londeg = (lonsec + 30 * 60) / (60 * 60);
		lonsec = lonmin = 0;
		break;

	case 1:
		lonmin = (lonsec + 30) / 60;
		londeg = lonmin / 60;
		lonmin -= londeg * 60;
		lonsec = 0;
		break;

	default:
		lonmin = lonsec / 60;
		lonsec -= lonmin * 60;
		londeg = lonmin / 60;
		lonmin -= londeg * 60;
		break;
	}
	std::ostringstream oss;
	oss << std::setw(3) << std::setfill('0') << londeg;
	if (lonmin || lonsec || (mode & (8 | 4))) {
		oss << std::setw(2) << std::setfill('0') << lonmin;
		if (lonsec || (mode & 4))
			oss << std::setw(2) << std::setfill('0') << lonsec;
	}
	oss << lonsgn;
	return oss.str();
}

std::string Point::get_lat_fpl_str(fplstrmode_t mode) const
{
	int32_t latsec(get_lat()), latmin, latdeg;
	char latsgn((latsec < 0) ? 'S' : 'N');
	latsec = round<int32_t,double>(abs(latsec) * (to_deg_dbl * 60 * 60));
	switch (mode & 3) {
	case 0:
		latdeg = (latsec + 30 * 60) / (60 * 60);
		latsec = latmin = 0;
		break;

	case 1:
		latmin = (latsec + 30) / 60;
		latdeg = latmin / 60;
		latmin -= latdeg * 60;
		latsec = 0;
		break;

	default:
		latmin = latsec / 60;
		latsec -= latmin * 60;
		latdeg = latmin / 60;
		latmin -= latdeg * 60;
		break;
	}
	std::ostringstream oss;
	oss << std::setw(2) << std::setfill('0') << latdeg;
	if (latmin || latsec || (mode & (8 | 4))) {
		oss << std::setw(2) << std::setfill('0') << latmin;
		if (latsec || (mode & 4))
			oss << std::setw(2) << std::setfill('0') << latsec;
	}
	oss << latsgn;
	return oss.str();
}

std::string Point::get_fpl_str(fplstrmode_t mode) const
{
	int32_t latsec(get_lat()), latmin, latdeg;
	char latsgn((latsec < 0) ? 'S' : 'N');
	latsec = round<int32_t,double>(abs(latsec) * (to_deg_dbl * 60 * 60));
	int32_t lonsec(get_lon()), lonmin, londeg;
	char lonsgn((lonsec < 0) ? 'W' : 'E');
	lonsec = round<int32_t,double>(abs(lonsec) * (to_deg_dbl * 60 * 60));
	switch (mode & 3) {
	case 0:
		latdeg = (latsec + 30 * 60) / (60 * 60);
		latsec = latmin = 0;
		londeg = (lonsec + 30 * 60) / (60 * 60);
		lonsec = lonmin = 0;
		break;

	case 1:
		latmin = (latsec + 30) / 60;
		latdeg = latmin / 60;
		latmin -= latdeg * 60;
		latsec = 0;
		lonmin = (lonsec + 30) / 60;
		londeg = lonmin / 60;
		lonmin -= londeg * 60;
		lonsec = 0;
		break;

	default:
		latmin = latsec / 60;
		latsec -= latmin * 60;
		latdeg = latmin / 60;
		latmin -= latdeg * 60;
		lonmin = lonsec / 60;
		lonsec -= lonmin * 60;
		londeg = lonmin / 60;
		lonmin -= londeg * 60;
		break;
	}
	std::ostringstream oss;
	oss << std::setw(2) << std::setfill('0') << latdeg;
	if (latmin || latsec || lonmin || lonsec || (mode & (8 | 4))) {
		oss << std::setw(2) << std::setfill('0') << latmin;
		if (latsec || lonsec || (mode & 4))
			oss << std::setw(2) << std::setfill('0') << latsec;
	}
	oss << latsgn
	    << std::setw(3) << std::setfill('0') << londeg;
	if (lonmin || lonsec || lonmin || lonsec || (mode & (8 | 4))) {
		oss << std::setw(2) << std::setfill('0') << lonmin;
		if (lonsec || lonsec || (mode & 4))
			oss << std::setw(2) << std::setfill('0') << lonsec;
	}
	oss << lonsgn;
	return oss.str();
}

std::ostream& Point::print(std::ostream& os) const
{
	os << '(' << (std::string)get_lon_str() << ',' << (std::string)get_lat_str() << ')';
	return os;
}

bool Point::set_lon_str(const std::string& str)
{
	char hemi;
	double deg, min, sec;
	Point::coord_t c(0);
	bool sok = sscanf(str.c_str(), "%c%lf\302\260%lf'%lf\"", &hemi, &deg, &min, &sec) == 4;
	if (!sok)
		sok = sscanf(str.c_str(), "%c%lf %lf %lf", &hemi, &deg, &min, &sec) == 4;
	if (sok) {
		if (hemi != 'E' && hemi != 'e' && hemi != 'W' && hemi != 'w')
			goto err;
		c = (Point::coord_t)((deg + (1.0 / 60.0) * min + (1.0 / 60.0 / 60.0) * sec) * Point::from_deg);
		if (hemi == 'W' || hemi == 'w')
			c = -c;
		goto ok;
	}
	sok = sscanf(str.c_str(), "%c%lf\302\260%lf'", &hemi, &deg, &min) == 3;
	if (!sok)
		sok = sscanf(str.c_str(), "%c%lf %lf", &hemi, &deg, &min) == 3;
	if (sok) {
		if (hemi != 'E' && hemi != 'e' && hemi != 'W' && hemi != 'w')
			goto err;
		c = (Point::coord_t)((deg + (1.0 / 60.0) * min) * Point::from_deg);
		if (hemi == 'W' || hemi == 'w')
			c = -c;
		goto ok;
	}
err:
	return false;

ok:
	m_lon = c;
	return true;
}

bool Point::set_lat_str(const std::string& str)
{
	char hemi;
	double deg, min, sec;
	Point::coord_t c(0);
	bool sok = sscanf(str.c_str(), "%c%lf\302\260%lf'%lf\"", &hemi, &deg, &min, &sec) == 4;
	if (!sok)
		sok = sscanf(str.c_str(), "%c%lf %lf %lf", &hemi, &deg, &min, &sec) == 4;
	if (sok) {
		if (hemi != 'N' && hemi != 'n' && hemi != 'S' && hemi != 's')
			goto err;
		c = (Point::coord_t)((deg + (1.0 / 60.0) * min + (1.0 / 60.0 / 60.0) * sec) * Point::from_deg);
		if (hemi == 'S' || hemi == 's')
			c = -c;
		goto ok;
	}
	sok = sscanf(str.c_str(), "%c%lf\302\260%lf'", &hemi, &deg, &min) == 3;
	if (!sok)
		sok = sscanf(str.c_str(), "%c%lf %lf", &hemi, &deg, &min) == 3;
	if (sok) {
		if (hemi != 'N' && hemi != 'n' && hemi != 'S' && hemi != 's')
			goto err;
		c = (Point::coord_t)((deg + (1.0 / 60.0) * min) * Point::from_deg);
		if (hemi == 'S' || hemi == 's')
			c = -c;
		goto ok;
	}
err:
	return false;

ok:
	m_lat = c;
	return true;
}

void Point::set_ch1903(float x, float y)
{
	if (y < x)
		std::swap(x, y);
	y = (y - 600000) * 1e-6;
	x = (x - 200000) * 1e-6;
	float lambda = 2.6779094
			+ 4.728982 * y
			+ 0.791484 * y * x
			+ 0.1306 * y * x * x
			- 0.0436 * y * y * y;
	float phi = 16.9023892
			+ 3.238272 * x
			- 0.270978 * y * y
			- 0.002528 * x * x
			- 0.0447 * y * y * x
			- 0.0140 * x * x * x;
	set_lon(round<coord_t,float>(lambda * (100.0 / 36.0 * Point::from_deg)));
	set_lat(round<coord_t,float>(phi * (100.0 / 36.0 * Point::from_deg)));
}

bool Point::get_ch1903(float& x, float& y) const
{
	float phi = (get_lat() - ((int32_t)(169028.66 * Point::from_deg / 60.0 / 60.0))) * (Point::to_deg * 60.0 * 60.0 / 10000.0);
	float lambda = (get_lon() - ((int32_t)(26782.5 * Point::from_deg / 60.0 / 60.0))) * (Point::to_deg * 60.0 * 60.0 / 10000.0);
	float yy = 600072.37
		 + 211455.93 * lambda
		 -  10938.51 * lambda * phi
		 -      0.36 * lambda * phi * phi
		 -     44.54 * lambda * lambda * lambda;
	float xx = 200147.07
		 + 308807.95 * phi
		 +   3745.25 * lambda * lambda
		 +     76.63 * phi * phi
		 -    194.56 * lambda * lambda * phi
		 +    119.79 * lambda * lambda * lambda;
	x = xx;
	y = yy;
	return (xx >= 0) && (xx < 400000) && (yy >= 400000) && (yy < 1000000);
}

void Point::set_ch1903p(double x, double y)
{
	static const double polya1[] = { +4.72973056, +0.7925714, +0.132812, +0.02550, +0.0048 };
	static const double polya3[] = { -0.044270, -0.02550, -0.0096 };
	static const double polya5[] = { +0.00096 };
	static const double polyp0[] = { 0, +3.23864877, -0.0025486, -0.013245, +0.000048 };
	static const double polyp2[] = { -0.27135379, -0.0450442, -0.007553, -0.00146 };
	static const double polyp4[] = { +0.002442, +0.00132 };
	if (y < x)
		std::swap(x, y);
	y = (y - 600000) * 1e-6;
	x = (x - 200000) * 1e-6;
	double p[6];
	p[0] = 0;
	p[1] = polyeval(polya1, sizeof(polya1)/sizeof(polya1[0]), x);
	p[2] = 0;
	p[3] = polyeval(polya3, sizeof(polya3)/sizeof(polya3[0]), x);
	p[4] = 0;
	p[5] = polyeval(polya5, sizeof(polya5)/sizeof(polya5[0]), x);
	double lambda(polyeval(p, 6, y));
	p[0] = polyeval(polyp0, sizeof(polyp0)/sizeof(polyp0[0]), x);
	p[1] = 0;
	p[2] = polyeval(polyp2, sizeof(polyp2)/sizeof(polyp2[0]), x);
	p[3] = 0;
	p[4] = polyeval(polyp4, sizeof(polyp4)/sizeof(polyp4[0]), x);
	p[5] = 0;
	double phi(polyeval(p, 5, y));
	lambda += 2.67825;
	phi += 16.902866;
	set_lon(round<coord_t,double>(lambda * (100.0 / 36.0 * Point::from_deg_dbl)));
	set_lat(round<coord_t,double>(phi * (100.0 / 36.0 * Point::from_deg_dbl)));
}

bool Point::get_ch1903p(double& x, double& y) const
{
	static const double polyy1[] = { +0.2114285339, -0.010939608, -0.000002658, -0.00000853	};
	static const double polyy3[] = { -0.0000442327, +0.000004291, -0.000000309 };
	static const double polyy5[] = { +0.0000000197 };
	static const double polyx0[] = { 0, +0.3087707463, +0.000075028, +0.000120435, 0, +0.00000007 };
	static const double polyx2[] = { +0.0037454089, -0.0001937927, +0.000004340, -0.000000376 };
	static const double polyx4[] = { -0.0000007346, +0.0000001444 };
	double phi(get_lat() * (Point::to_deg_dbl * 60.0 * 60.0 / 10000.0) - 16.902866);
	double lambda(get_lon() * (Point::to_deg * 60.0 * 60.0 / 10000.0) - 2.67825);
	double p[6];
	p[0] = 0;
	p[1] = polyeval(polyy1, sizeof(polyy1)/sizeof(polyy1[0]), phi);
	p[2] = 0;
	p[3] = polyeval(polyy3, sizeof(polyy3)/sizeof(polyy3[0]), phi);
	p[4] = 0;
	p[5] = polyeval(polyy5, sizeof(polyy5)/sizeof(polyy5[0]), phi);
	double yy(polyeval(p, 6, lambda));
	p[0] = polyeval(polyx0, sizeof(polyx0)/sizeof(polyx0[0]), phi);
	p[1] = 0;
	p[2] = polyeval(polyx2, sizeof(polyx2)/sizeof(polyx2[0]), phi);
	p[3] = 0;
	p[4] = polyeval(polyx4, sizeof(polyx4)/sizeof(polyx4[0]), phi);
	p[5] = 0;
	double xx(polyeval(p, 5, lambda));
	xx *= 1e6;
	yy *= 1e6;
	xx += 200000;
	yy += 600000;
	x = xx;
	y = yy;
	return (xx >= 0) && (xx < 400000) && (yy >= 400000) && (yy < 1000000);
}

template<typename T> T Point::polyeval(const T *coeff, unsigned int n, T v)
{
	T vv(1), s(0);
	for (; n; --n, ++coeff, vv *= v)
		s += vv * *coeff;
	return s;
}

template double Point::polyeval<double>(const double *coeff, unsigned int n, double v);
template float Point::polyeval<float>(const float *coeff, unsigned int n, float v);

const unsigned int Point::maidenhead_maxdigits;

const unsigned int Point::maidenhead_max[maidenhead_maxdigits] = {
	18, 10, 24, 10, 24
};

const double Point::maidenhead_factors[maidenhead_maxdigits] = {
	20 * from_deg_dbl,
	2 * from_deg_dbl,
	2.0/24.0 * from_deg_dbl,
	2.0/240.0 * from_deg_dbl,
	2.0/240.0/24.0 * from_deg_dbl
};

std::string Point::get_locator(unsigned int digits) const
{
	digits = std::min(digits >> 1, maidenhead_maxdigits);
	if (!digits)
		return std::string();
	double lon(get_lon()), lat(get_lat());
	lat *= 2;
	{
		double rnd(0.5 * maidenhead_factors[digits-1] + 180 * from_deg_dbl);
		lon += rnd;
		lat += rnd;
	}
	std::string r;
	for (unsigned int i = 0; i < digits; ++i) {
		int londig, latdig;
		{
			double f(1 / maidenhead_factors[i]);
			londig = floor(lon * f);
			latdig = floor(lat * f);
		}
		londig = std::max(std::min(londig, (int)maidenhead_max[i]-1), 0);
		latdig = std::max(std::min(latdig, (int)maidenhead_max[i]-1), 0);
		lon -= londig * maidenhead_factors[i];
		lat -= latdig * maidenhead_factors[i];
		if (i & 1) {
			r += (char)('0' + londig);
			r += (char)('0' + latdig);
			continue;
		}
		r += (char)('A' + londig);
		r += (char)('A' + latdig);
	}
	return r;
}

unsigned int Point::set_str_ch1903(std::string::const_iterator& si, std::string::const_iterator se)
{
	std::string::const_iterator si2(si);
	if (si2 == se || (*si2 != 'C' && *si2 != 'c'))
		return 0;
	++si2;
	if (si2 == se || (*si2 != 'H' && *si2 != 'h'))
		return 0;
	++si2;
	float x[2];
	unsigned int nr(0);
	while (si2 != se) {
		if (std::isspace(*si2)) {
			++si2;
			continue;
		}
		if (*si2 < '0' || *si2 > '9')
			break;
		std::string::const_iterator si3(si2);
		bool dot(true);
		do {
			if (*si3 == '.')
				dot = false;
			++si3;
		} while (si3 != se && ((*si3 >= '0' && *si3 <= '9') || (*si3 == '.' && dot)));
		x[nr++] = Glib::Ascii::strtod(std::string(si2, si3));
		si2 = si3;
		if (nr == 2)
			break;
	}
	if (nr < 2 || x[0] >= 1000000 || x[1] >= 1000000)
		return 0;
	set_ch1903(x[0], x[1]);
	return setstr_lat | setstr_lon;
}

unsigned int Point::set_str_locator(std::string::const_iterator& si, std::string::const_iterator se)
{
	unsigned int buf[2 * maidenhead_maxdigits];
	{
		unsigned int cnt(0);
		std::string::const_iterator si2(si);
		for (; si2 != se && cnt < 2 * maidenhead_maxdigits; ++cnt, ++si2) {
			if (cnt & 2) {
				if (*si2 < '0' || *si2 > '9')
					break;
				buf[cnt] = *si2 - '0';
			} else {
				if (*si2 >= 'a' && *si2 <= 'z')
					buf[cnt] = *si2 - 'a';
				else if (*si2 >= 'A' && *si2 <= 'Z')
					buf[cnt] = *si2 - 'A';
				else
					break;
			}
			if (buf[cnt] >= maidenhead_max[cnt >> 1])
				break;
		}
		if (false)
			std::cerr << "set_str_locator: " << cnt << " digits" << std::endl;
		if (cnt < 6)
			return 0;
		cnt &= ~1;
		for (unsigned int i = 0; i < cnt; ++i)
			++si;
		for (; cnt < 2 * maidenhead_maxdigits; ++cnt)
			buf[cnt] = maidenhead_max[cnt >> 1] >> 1;
	}
	double lon(0.5 * maidenhead_factors[maidenhead_maxdigits-1] - 180 * from_deg_dbl);
	double lat(lon);
	for (unsigned int i = 0; i < maidenhead_maxdigits; ++i) {
		lon += maidenhead_factors[i] * buf[2 * i + 0];
		lat += maidenhead_factors[i] * buf[2 * i + 1];
	}
	lat *= 0.5;
	set_lat(round<coord_t,double>(lat));
	set_lon(round<coord_t,double>(lon));
	return setstr_lat | setstr_lon;
}

unsigned int Point::set_str(const std::string& str, unsigned int flags)
{
	unsigned int r(0);
	{
		static const double factors[3] = {
			Point::from_deg_dbl, Point::from_deg_dbl / 60.0, Point::from_deg_dbl / 60.0 / 60.0
		};
		std::string::const_iterator si(str.begin()), se(str.end());
		unsigned int pre(0), precnt(0), cnt(0);
		double prefrac(0);
		int hemimul(0);
		bool hemilon(false), dot(true);
		while (si != se) {
			if (std::isspace(*si)) {
				++si;
				continue;
			}
			if ((flags & setstr_flags_locator) && !hemimul && !precnt &&
			    ((*si >= 'A' && *si <= 'R') || (*si >= 'a' && *si <= 'r'))) {
				unsigned int r2(set_str_locator(si, se));
				if (r2) {
					r |= r2;
					continue;
				}
			}
			if ((flags & setstr_flags_ch1903) && !hemimul && !precnt && (*si == 'c' || *si == 'C')) {
				unsigned int r2(set_str_ch1903(si, se));
				if (r2) {
					r |= r2;
					continue;
				}
			}
			if (*si >= '0' && *si <= '9' && !hemimul) {
				if (precnt)
					break;
				bool predot(true);
				double fracmul(1);
				do {
					if (*si == '.') {
						++si;
						predot = false;
						continue;
					}
					if (predot) {
						pre *= 10;
						pre += (*si++) - '0';
						++precnt;
						continue;
					}
					fracmul *= 0.1;
					prefrac += fracmul * ((*si++) - '0');
				} while (si != se && ((*si >= '0' && *si <= '9') || (predot && *si == '.')));
				continue;
			}
			if (hemimul && cnt < sizeof(factors)/sizeof(factors[0]) &&
			    ((*si >= '0' && *si <= '9') || (dot && *si == '.'))) {
				{
					unsigned int maxdgt((dot && hemilon) ? 3 : 2);
					std::string::const_iterator si2(si);
					do {
						if (*si2 == '.')
							dot = false;
						else if (maxdgt > 0)
							--maxdgt;
						++si2;
					} while (si2 != se && (((maxdgt || !dot) && *si2 >= '0' && *si2 <= '9') || (dot && *si2 == '.')));
					Point::coord_t c(round<coord_t,double>(Glib::Ascii::strtod(std::string(si, si2)) * factors[cnt]));
					c *= hemimul;
					si = si2;
					if (hemilon) {
						if (cnt)
							c += get_lon();
						set_lon(c);
						r |= setstr_lon;
					} else {
						if (cnt)
							c += get_lat();
						set_lat(c);
						r |= setstr_lat;
					}
				}
				while (si != se && (std::isspace(*si) ||
						    (*si == (gunichar)176 && cnt == 0) ||
						    (*si == '\'' && cnt == 1) ||
						    (*si == '"' && cnt == 2)))
					++si;
				if (false && si != se)
					std::cerr << "Coordinate parsing level " << cnt << " stopped at " << (uint32_t)*si << std::endl;
				++cnt;
				continue;
			}
			if (*si == 'N' || *si == 'n') {
				hemimul = 1;
				hemilon = false;
			} else if (*si == 'S' || *si == 's') {
				hemimul = -1;
				hemilon = false;
			} else if (*si == 'E' || *si == 'e') {
				hemimul = 1;
				hemilon = true;
			} else if (*si == 'W' || *si == 'w') {
				hemimul = -1;
				hemilon = true;
			} else {
				r |= setstr_excess;
				break;
			}
			++si;
			cnt = 0;
			if (precnt == 6U + hemilon || precnt == 4U + hemilon || precnt == 2U + hemilon) {
				Point::coord_t c;
				if (precnt >= 6) {
					unsigned int deg(pre / 10000);
					pre -= deg * 10000;
					unsigned int min(pre / 100);
					pre -= min * 100;
					c = round<coord_t,double>(deg * factors[0] + min * factors[1] + (pre + prefrac) * factors[2]);
				} else if (precnt >= 4) {
					unsigned int deg(pre / 100);
					pre -= deg * 100;
					c = round<coord_t,double>(deg * factors[0] + (pre + prefrac) * factors[1]);
				} else {
					c = round<coord_t,double>((pre + prefrac) * factors[0]);
				}
				c *= hemimul;
				if (hemilon) {
					set_lon(c);
					r |= setstr_lon;
				} else {
					set_lat(c);
					r |= setstr_lat;
				}
				pre = precnt = 0;
				prefrac = 0;
				hemimul = 0;
				hemilon = false;
				dot = true;
				continue;
			}
			if (precnt) {
				r |= setstr_excess;
				break;
			}
			dot = true;
		}
		if (hemimul && !cnt) {
			r |= setstr_excess;
		}
	}
	if (!(r & (setstr_lon | setstr_lat)))
		r &= ~setstr_excess;
	return r;
}

bool Point::is_latitude(const std::string& s)
{
	std::string::size_type n(s.size());
	if (n < 3)
		return false;
	bool fr(s[0] == 'N' || s[0] == 'S');
	bool ba(s[n-1] == 'N' || s[n-1] == 'S');
	if (fr == ba)
		return false;
	std::string::size_type i(fr);
	n -= ba;
	unsigned int predot(0);
	for (; i < n; ++i, ++predot)
		if (!std::isdigit(s[i]))
			break;
	if (predot != 2 && predot != 4 && predot != 6)
		return false;
	if (i >= n)
		return true;
	if (s[i++] != '.')
		return false;
	for (; i < n; ++i)
		if (!std::isdigit(s[i]))
			return false;
	return true;
}

bool Point::is_longitude(const std::string& s)
{
	std::string::size_type n(s.size());
	if (n < 4)
		return false;
	bool fr(s[0] == 'E' || s[0] == 'W');
	bool ba(s[n-1] == 'E' || s[n-1] == 'W');
	if (fr == ba)
		return false;
	std::string::size_type i(fr);
	n -= ba;
	unsigned int predot(0);
	for (; i < n; ++i, ++predot)
		if (!std::isdigit(s[i]))
			break;
	if (predot != 3 && predot != 5 && predot != 7)
		return false;
	if (i >= n)
		return true;
	if (s[i++] != '.')
		return false;
	for (; i < n; ++i)
		if (!std::isdigit(s[i]))
			return false;
	return true;
}

bool Point::is_latlon(const std::string& s)
{
	std::string::size_type n(s.size());
	std::string::size_type lats(n), lons(n);
	for (std::string::size_type i(0); i < n; ++i) {
		if (s[i] == 'N' || s[i] == 'S') {
			if (lats != n)
				return false;
			lats = i;
			continue;
		}
		if (s[i] == 'E' || s[i] == 'W') {
			if (lons != n)
				return false;
			lons = i;
			continue;
		}
		if (std::isdigit(s[i]) || s[i] == '.')
			continue;
		return false;
	}
	if (lats == n || lons == n)
		return false;
	// case 1: hemisphere indicator at beginning
	if (!lats || !lons) {
		if (!is_latitude(s.substr(lats, ((lats < lons) ? lons : n) - lats)))
			return false;
		if (!is_longitude(s.substr(lons, ((lons < lats) ? lats : n) - lons)))
			return false;
		return true;
	}
	// case 2: hemisphere indicator at end
	if (lats + 1 == n || lons + 1 == n) {
		std::string::size_type b;
		b = (lons < lats) ? lons + 1 : 0;
		if (!is_latitude(s.substr(b, lats + 1 - b)))
			return false;
		b = (lats < lons) ? lats + 1 : 0;
		if (!is_longitude(s.substr(b, lons + 1 - b)))
			return false;
		return true;
	}
	return false;
}

bool Point::is_fpl_latlon(std::string::const_iterator i, std::string::const_iterator e)
{
	unsigned int n(0);
	while (i != e && std::isdigit(*i)) {
		++i;
		++n;
	}
	if (n != 2 && n != 4)
		return false;
	if (i == e || (*i != 'N' && *i != 'n' && *i != 'S' && *i != 's'))
		return false;
	++i;
	n = 0;
	while (i != e && std::isdigit(*i)) {
		++i;
		++n;
	}
	if (n != 3 && n != 5)
		return false;
	if (i == e || (*i != 'E' && *i != 'e' && *i != 'W' && *i != 'w'))
		return false;
	++i;
	return i == e;
}

Point Point::get_round_deg(void) const
{
	Point p;
	p.set_lon_deg_dbl(round<int,double>(get_lon_deg_dbl()));
	p.set_lat_deg_dbl(round<int,double>(get_lat_deg_dbl()));
	return p;
}

Point Point::get_round_min(void) const
{
	Point p;
	p.set_lon_min_dbl(round<int,double>(get_lon_min_dbl()));
	p.set_lat_min_dbl(round<int,double>(get_lat_min_dbl()));
	return p;
}

Point Point::get_round_sec(void) const
{
	Point p;
	p.set_lon_sec_dbl(round<int,double>(get_lon_sec_dbl()));
	p.set_lat_sec_dbl(round<int,double>(get_lat_sec_dbl()));
	return p;
}

unsigned int Point::get_wkbsize(void)
{
	return Geometry::get_wkbhdrsize() + get_wkbptsize();
}

uint8_t *Point::wkbencode(uint8_t *cp, uint8_t *ep) const
{
	cp = Geometry::wkbencode(cp, ep, get_lon_deg_dbl());
	cp = Geometry::wkbencode(cp, ep, get_lat_deg_dbl());
	return cp;
}

const uint8_t *Point::wkbdecode(const uint8_t *cp, const uint8_t *ep, uint8_t byteorder)
{
	double lon, lat;
	cp = Geometry::wkbdecode(cp, ep, lon, byteorder);
	cp = Geometry::wkbdecode(cp, ep, lat, byteorder);
	if (!std::isnan(lon) && !std::isnan(lat)) {
		set_lon_deg_dbl(lon);
		set_lat_deg_dbl(lat);
	}
	return cp;
}

uint8_t *Point::to_wkb(uint8_t *cp, uint8_t *ep) const
{
	cp = Geometry::wkbencode(cp, ep, Geometry::type_point);
	cp = wkbencode(cp, ep);
	return cp;
}

std::vector<uint8_t> Point::to_wkb(void) const
{
	std::vector<uint8_t> b(0, get_wkbsize());
	to_wkb(&b[0], &b[b.size()]);
	return b;
}

const uint8_t *Point::from_wkb(const uint8_t *cp, const uint8_t *ep)
{
	set_invalid();
	uint8_t byteorder(1);
	{
		Geometry::type_t typ(Geometry::type_invalid);
		cp = Geometry::wkbdecode(cp, ep, typ, byteorder);
		if (typ != Geometry::type_point)
			throw std::runtime_error("from_wkb: not a point: " + to_str(typ));
	}
	cp = wkbdecode(cp, ep, byteorder);
	return cp;
}

std::ostream& Point::to_wktpt(std::ostream& os) const
{
	return os << get_lon_deg_dbl() << ' ' << get_lat_deg_dbl();
}

std::ostream& Point::to_wkt(std::ostream& os) const
{
	return to_wktpt(os << "POINT(") << ')';
}

#ifdef HAVE_JSONCPP

Json::Value Point::to_json(void) const
{
	Json::Value r;
	r["type"] = "Point";
	Json::Value& c(r["coordinates"]);
	c.append(get_lon_deg_dbl());
	c.append(get_lat_deg_dbl());
	return r;
}

void Point::from_json(const Json::Value& g)
{
	set_invalid();
	if (!g.isMember("type") || !g["type"].isString() || g["type"].asString() != "Point")
		throw std::runtime_error("Point::from_json: invalid type");
	if (!g.isMember("coordinates"))
		throw std::runtime_error("Point::from_json: no coordinates");
	const Json::Value& c(g["coordinates"]);
	if (!c.isArray() || c.size() < 2 || !c[0].isNumeric() || !c[1].isNumeric())
		throw std::runtime_error("Point::from_json: invalid coordinates");
	set_lon_deg_dbl(c[0].asDouble());
	set_lat_deg_dbl(c[1].asDouble());
}

#endif

const Rect Rect::invalid(Point::invalid, Point::invalid);

int Rect::compare(const Rect& x) const
{
	int c(get_southwest().compare(x.get_southwest()));
	if (c)
		return c;
	return get_northeast().compare(x.get_northeast());
}

int64_t Rect::get_east_unwrapped(void) const
{
	int64_t r(get_east());
	if (get_east() < get_west())
		r += 0x100000000LL;
	return r;
}

bool Rect::is_inside(const Point& pt) const
{
	Point p0(pt - get_southwest());
	Point p1(get_northeast() - get_southwest());
	return ((uint32_t)p0.get_lon() <= (uint32_t)p1.get_lon()) &&
		((uint32_t)p0.get_lat() <= (uint32_t)p1.get_lat());
}

bool Rect::is_inside(const Rect& r) const
{
	{
		uint32_t s(get_north() - get_south()), c0(r.get_south() - get_south()), c1(r.get_north() - get_south());
		if (c0 > s || c1 > s)
			return false;
	}
	{
		uint32_t s(get_east() - get_west()), x(r.get_west() - get_west());
		if (x > s)
			return false;
	}
	{
		uint32_t s(get_east() - r.get_west()), x(r.get_east() - r.get_west());
		if (x > s)
			return false;
	}
	return true;
}

bool Rect::is_strictly_inside(const Point& pt) const
{
	Point p0(pt - get_southwest());
	Point p1(get_northeast() - get_southwest());
	return ((uint32_t)p0.get_lon() > 0) &&
		((uint32_t)p0.get_lat() > 0) &&
		((uint32_t)p0.get_lon() < (uint32_t)p1.get_lon()) &&
		((uint32_t)p0.get_lat() < (uint32_t)p1.get_lat());
}

bool Rect::is_lon_inside(const Point& pt) const
{
	Point p0(pt - get_southwest());
	Point p1(get_northeast() - get_southwest());
	return ((uint32_t)p0.get_lon() <= (uint32_t)p1.get_lon());
}

bool Rect::is_lat_inside(const Point& pt) const
{
	Point p0(pt - get_southwest());
	Point p1(get_northeast() - get_southwest());
	return ((uint32_t)p0.get_lat() <= (uint32_t)p1.get_lat());
}

uint64_t Rect::simple_distance_rel(const Point& pt) const
{
	if (is_inside(pt))
		return 0;
	uint64_t r0 = pt.simple_distance_rel(get_southwest());
	uint64_t r1 = pt.simple_distance_rel(get_northeast());
	uint64_t r2 = pt.simple_distance_rel(get_southeast());
	uint64_t r3 = pt.simple_distance_rel(get_northwest());
	uint64_t r = std::min(std::min(r0, r1), std::min(r2, r3));
	if (is_lon_inside(pt)) {
		uint64_t r0 = pt.simple_distance_rel(Point(pt.get_lon(), get_north()));
		uint64_t r1 = pt.simple_distance_rel(Point(pt.get_lon(), get_south()));
		r = std::min(r, std::min(r0, r1));
	}
	if (is_lat_inside(pt)) {
		uint64_t r0 = pt.simple_distance_rel(Point(get_east(), pt.get_lat()));
		uint64_t r1 = pt.simple_distance_rel(Point(get_west(), pt.get_lat()));
		r = std::min(r, std::min(r0, r1));
	}
	return r;
}

float Rect::simple_distance_km(const Point& pt) const
{
	return sqrt(simple_distance_rel(pt)) * (Point::earth_radius * Point::to_rad);
}

float Rect::spheric_distance_km(const Point& pt) const
{
	if (is_inside(pt))
		return 0;
	float r0 = pt.spheric_distance_km(get_southwest());
	float r1 = pt.spheric_distance_km(get_northeast());
	float r2 = pt.spheric_distance_km(get_southeast());
	float r3 = pt.spheric_distance_km(get_northwest());
	float r = std::min(std::min(r0, r1), std::min(r2, r3));
	if (is_lon_inside(pt)) {
		float r0 = pt.spheric_distance_km(Point(pt.get_lon(), get_north()));
		float r1 = pt.spheric_distance_km(Point(pt.get_lon(), get_south()));
		r = std::min(r, std::min(r0, r1));
	}
	if (is_lat_inside(pt)) {
		float r0 = pt.spheric_distance_km(Point(get_east(), pt.get_lat()));
		float r1 = pt.spheric_distance_km(Point(get_west(), pt.get_lat()));
		r = std::min(r, std::min(r0, r1));
	}
	return r;
}

std::pair<bool,bool> Rect::is_overlap_helper(const Rect& r) const
{
	Point p0(r.get_southwest() - get_southwest());
	Point p1(r.get_northeast() - get_southwest());
	Point pdim(get_northeast() - get_southwest());
	return std::pair<bool,bool>((uint32_t)p0.get_lon() <= (uint32_t)pdim.get_lon() ||
				    (uint32_t)p1.get_lon() <= (uint32_t)pdim.get_lon(),
				    (uint32_t)p0.get_lat() <= (uint32_t)pdim.get_lat() ||
				    (uint32_t)p1.get_lat() <= (uint32_t)pdim.get_lat());
}

bool Rect::is_intersect(const Rect& r) const
{
	std::pair<bool,bool> ol0(is_overlap_helper(r));
	std::pair<bool,bool> ol1(r.is_overlap_helper(*this));
	return (ol0.first || ol1.first) && (ol0.second  || ol1.second);
}

bool Rect::is_intersect(const Point& la, const Point& lb) const
{
	return Point::is_intersection(la, lb, get_southwest(), get_southeast()) ||
		Point::is_intersection(la, lb, get_southeast(), get_northeast()) ||
		Point::is_intersection(la, lb, get_northeast(), get_northwest()) ||
		Point::is_intersection(la, lb, get_northwest(), get_southwest());
}

Rect Rect::intersect(const Rect& r) const
{
	Rect ret;
	unsigned int set(0);
	Point tdim(get_northeast() - get_southwest());
	Point rdim(r.get_northeast() - r.get_southwest());
	Point t0(get_southwest() - r.get_southwest());
	Point t1(get_northeast() - r.get_southwest());
	Point r0(r.get_southwest() - get_southwest());
	Point r1(r.get_northeast() - get_southwest());
	// west
	if ((uint32_t)r0.get_lon() <= (uint32_t)tdim.get_lon())
		ret.set_west(r.get_west());
	else if ((uint32_t)t0.get_lon() <= (uint32_t)rdim.get_lon())
		ret.set_west(get_west());
	else
		return invalid;
	// south
	if ((uint32_t)r0.get_lat() <= (uint32_t)tdim.get_lat())
		ret.set_south(r.get_south());
	else if ((uint32_t)t0.get_lat() <= (uint32_t)rdim.get_lat())
		ret.set_south(get_south());
	else
		return invalid;
	// east
	if ((uint32_t)r1.get_lon() <= (uint32_t)tdim.get_lon())
		ret.set_east(r.get_east());
	else if ((uint32_t)t1.get_lon() <= (uint32_t)rdim.get_lon())
		ret.set_east(get_east());
	else
		return invalid;
	// north
	if ((uint32_t)r1.get_lat() <= (uint32_t)tdim.get_lat())
		ret.set_north(r.get_north());
	else if ((uint32_t)t1.get_lat() <= (uint32_t)rdim.get_lat())
		ret.set_north(get_north());
	else
		return invalid;
	return ret;
}

uint64_t Rect::simple_distance_rel(const Rect& r) const
{
	if (is_intersect(r))
		return 0;
	uint64_t r0 = simple_distance_rel(get_southwest());
	uint64_t r1 = simple_distance_rel(get_northeast());
	uint64_t r2 = simple_distance_rel(get_southeast());
	uint64_t r3 = simple_distance_rel(get_northwest());
	return std::min(std::min(r0, r1), std::min(r2, r3));
}

float Rect::simple_distance_km(const Rect& r) const
{
	return sqrt(simple_distance_rel(r)) * (Point::earth_radius * Point::to_rad);
}

float Rect::spheric_distance_km(const Rect& r) const
{
	if (is_intersect(r))
		return 0;
	float r0 = spheric_distance_km(get_southwest());
	float r1 = spheric_distance_km(get_northeast());
	float r2 = spheric_distance_km(get_southeast());
	float r3 = spheric_distance_km(get_northwest());
	return std::min(std::min(r0, r1), std::min(r2, r3));
}

std::ostream & Rect::print(std::ostream& os) const
{
	return os << get_southwest() << '-' << get_northeast();
}

Rect Rect::oversize_nmi(float nmi) const
{
	return oversize_km(nmi * (float)(1.0 / Point::km_to_nmi));
}

Rect Rect::oversize_km(float km) const
{
	Rect r(*this);
	km *= (Point::from_rad / Point::earth_radius);
	uint64_t londiff((uint32_t)(r.get_east() - r.get_west()));
	{
		float distlon = km;
		if (abs(r.get_south()) < Point::pole_lat)
			distlon /= cosf(r.get_southwest().get_lat_rad());
		Point::coord_t dl(distlon);
		r.set_west(r.get_west() - dl);
		londiff += dl;
	}
	{
		float distlon = km;
		if (abs(r.get_north()) < Point::pole_lat)
			distlon /= cosf(r.get_northeast().get_lat_rad());
		Point::coord_t dl(distlon);
		londiff += dl;
	}
	londiff = std::min(londiff, static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()));
	r.set_east(r.get_west() + londiff);
	r.set_north(std::min((Point::coord_t)(r.get_north() + km), Point::pole_lat));
	r.set_south(std::max((Point::coord_t)(r.get_south() - km), -Point::pole_lat));
	return r;
}

Rect Rect::add(const Point& pt) const
{
	if (get_south() > get_north())
		return Rect(pt, pt);
	Point ref(boxcenter());
	Point p0(get_northeast() - ref);
	Point p1(get_southwest() - ref);
	Point p2(pt - ref);
	Rect ret(Point(std::min(p1.get_lon(), p2.get_lon()), std::min(p1.get_lat(), p2.get_lat())) + ref,
		 Point(std::max(p0.get_lon(), p2.get_lon()), std::max(p0.get_lat(), p2.get_lat())) + ref);
	return ret;
}

Rect Rect::add(const Rect& r) const
{
	static const uint64_t maxlen = (uint32_t)-1;
	if (r.get_south() > r.get_north())
		return *this;
	if (get_south() > get_north())
		return r;
	Rect x;
	x.set_north(std::max(get_north(), r.get_north()));
	x.set_south(std::min(get_south(), r.get_south()));
	uint64_t l0 = (uint32_t)(r.get_west() - get_west());
	l0 += (uint32_t)(r.get_east() - r.get_west());
	l0 = std::max(l0, (uint64_t)(uint32_t)(get_east() - get_west()));
	uint64_t l1 = (uint32_t)(get_west() - r.get_west());
	l1 += (uint32_t)(get_east() - get_west());
	l1 = std::max(l1, (uint64_t)(uint32_t)(r.get_east() - r.get_west()));
	if (l0 < l1) {
		x.set_west(get_west());
		x.set_east(std::min(l0, maxlen) + x.get_west());
	} else {
		x.set_west(r.get_west());
		x.set_east(std::min(l1, maxlen) + x.get_west());
	}
	if (false && (!x.is_inside(*this) || !x.is_inside(r)))
		std::cerr << "ERROR: " << *this << " + " << r << " -> " << x << std::endl;
	return x;
}

bool Rect::is_empty(void) const
{
	return get_south() > get_north();
}

void Rect::set_empty(void)
{
	m_southwest = Point(0, 0x80000001);
	m_northeast = Point(0, 0x80000000);
}

int64_t Rect::area2(void) const
{
	/* http://mathworld.wolfram.com/PolygonArea.html */
	/* loop through all edges of the rect, in the order sw se ne nw - sw */
	/* area += (x1 * y2 - x2 * y1) */
	int64_t area = get_west() * (int64_t)get_south() - get_east() * (int64_t)get_south() + // sw se
		get_east() * (int64_t)get_north() - get_east() * (int64_t)get_south() + // se ne
		get_east() * (int64_t)get_north() - get_west() * (int64_t)get_north() + // ne nw
		get_west() * (int64_t)get_south() - get_west() * (int64_t)get_north(); // nw sw
	return area;
}

uint64_t Rect::simple_circumference_rel(void) const
{
	uint64_t d = get_southwest().simple_distance_rel(get_southeast()) +
		get_southeast().simple_distance_rel(get_northeast()) +
		get_northeast().simple_distance_rel(get_northwest()) +
		get_northwest().simple_distance_rel(get_southwest());
	return d;
}

float Rect::simple_circumference_km(void) const
{
	float d = get_southwest().simple_distance_km(get_southeast()) +
		get_southeast().simple_distance_km(get_northeast()) +
		get_northeast().simple_distance_km(get_northwest()) +
		get_northwest().simple_distance_km(get_southwest());
	return d;
}

float Rect::spheric_circumference_km(void) const
{
	float d = get_southwest().spheric_distance_km(get_southeast()) +
		get_southeast().spheric_distance_km(get_northeast()) +
		get_northeast().spheric_distance_km(get_northwest()) +
		get_northwest().spheric_distance_km(get_southwest());
	return d;
}

double Rect::spheric_circumference_km_dbl(void) const
{
	double d = get_southwest().spheric_distance_km_dbl(get_southeast()) +
		get_southeast().spheric_distance_km_dbl(get_northeast()) +
		get_northeast().spheric_distance_km_dbl(get_northwest()) +
		get_northwest().spheric_distance_km_dbl(get_southwest());
	return d;
}

double Rect::get_simple_area_km2_dbl(void) const
{
	Point center(boxcenter());
	if (center.is_invalid())
		return std::numeric_limits<double>::quiet_NaN();
	// area2 delivers the double area
	double a(area2());
	a *= (0.5 * Point::earth_radius_dbl * Point::to_rad_dbl *
	      Point::earth_radius_dbl * Point::to_rad_dbl);
	a *= cos(center.get_lat_rad());
	return a;
}

const std::string& to_str(BorderPoint::flag_t f)
{
	switch (f) {
	case BorderPoint::flag_begin:
	{
		static const std::string r("begin");
		return r;
	}

	case BorderPoint::flag_end:
	{
		static const std::string r("end");
		return r;
	}

	case BorderPoint::flag_onborder:
	{
		static const std::string r("onborder");
		return r;
	}

	case BorderPoint::flag_inc:
	{
		static const std::string r("inc");
		return r;
	}

	case BorderPoint::flag_dec:
	{
		static const std::string r("dec");
		return r;
	}

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

BorderPoint::BorderPoint(const Point& pt, double dist, flagmask_t flags)
	: m_pt(pt), m_dist(dist), m_flags(flags)
{
}

std::string BorderPoint::get_flags_string(flagmask_t m)
{
	std::string r;
	bool subseq(false);
	for (flag_t f(flag_begin); f <= flag_dec; f = (flag_t)(f + 1)) {
		if (!(m & (1 << f)))
			continue;
		if (subseq)
			r.push_back(',');
		else
			subseq = true;
		r += to_str(f);
	}
	return r;
}

int BorderPoint::get_windingnumber(void) const
{
	int w(0);
	if (get_flags() & flagmask_inc)
		++w;
	if (get_flags() & flagmask_dec)
		--w;
	return w;
}

int BorderPoint::compare(const BorderPoint& x) const
{
	if (get_dist() < x.get_dist())
		return -1;
	if (x.get_dist() < get_dist())
		return 1;
	return 0;
}

Rect MultiPoint::get_bbox(void) const
{
	const_iterator i(begin()), e(end());
	if (i == e) {
		Rect bbox;
		bbox.set_empty();
		return bbox;
	}
	Point ref(*i);
	Point pmin(0, 0), pmax(0, 0);
	for (++i; i != e; ++i) {
		Point p(*i - ref);
		pmin.set_lon(std::min(pmin.get_lon(), p.get_lon()));
		pmin.set_lat(std::min(pmin.get_lat(), p.get_lat()));
		pmax.set_lon(std::max(pmax.get_lon(), p.get_lon()));
		pmax.set_lat(std::max(pmax.get_lat(), p.get_lat()));
	}
	return Rect(pmin + ref, pmax + ref);
}

int MultiPoint::compare(const MultiPoint& x) const
{
	size_type n(size()), xn(x.size());
	if (n < xn)
		return -1;
	if (xn < n)
		return 1;
	for (size_type i(0); i < n; ++i) {
		int c(operator[](i).compare(x[i]));
		if (c)
			return c;
	}
	return 0;
}

std::ostream& MultiPoint::print(std::ostream& os) const
{
	os << '{';
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		i->print(os << ' ');
	return os << " }";
}

unsigned int MultiPoint::get_wkbsize(void) const
{
	unsigned int sz(Geometry::get_wkbhdrsize() + 4);
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		sz += i->get_wkbsize();
	return sz;
}

uint8_t *MultiPoint::wkbencode(uint8_t *cp, uint8_t *ep) const
{
	cp = Geometry::wkbencode(cp, ep, (uint32_t)size());
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		cp = i->to_wkb(cp, ep);
	return cp;
}

const uint8_t *MultiPoint::wkbdecode(const uint8_t *cp, const uint8_t *ep, uint8_t byteorder)
{
	uint32_t num(0);
	cp = Geometry::wkbdecode(cp, ep, num, byteorder);
	reserve(num);
	for (; num; --num) {
		Point pt;
		pt.from_wkb(cp, ep);
		push_back(pt);
	}
	return cp;
}

uint8_t *MultiPoint::to_wkb(uint8_t *cp, uint8_t *ep) const
{
	cp = Geometry::wkbencode(cp, ep, Geometry::type_multipoint);
	return wkbencode(cp, ep);
}

std::vector<uint8_t> MultiPoint::to_wkb(void) const
{
	std::vector<uint8_t> b(0, get_wkbsize());
	to_wkb(&b[0], &b[b.size()]);
	return b;
}

const uint8_t *MultiPoint::from_wkb(const uint8_t *cp, const uint8_t *ep)
{
	clear();
	uint8_t byteorder(1);
	{
		Geometry::type_t typ(Geometry::type_invalid);
		const uint8_t *cp1(Geometry::wkbdecode(cp, ep, typ, byteorder));
		if (typ == Geometry::type_point) {
			Point pt;
			cp = pt.from_wkb(cp, ep);
			push_back(pt);
			return cp;
		}
		cp = cp1;
		if (typ != Geometry::type_multipoint)
			throw std::runtime_error("from_wkb: not a multipoint: " + to_str(typ));
	}
	return wkbdecode(cp, ep, byteorder);
}

std::ostream& MultiPoint::to_wkt(std::ostream& os) const
{
	os << "MULTIPOINT(";
	static const char comma[] = ",";
	const char *del(comma + 1);
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		i->to_wktpt(os << del);
		del = comma;
	}
	return os << ')';
}

#ifdef HAVE_JSONCPP

Json::Value MultiPoint::to_json(void) const
{
	Json::Value r;
	r["type"] = "MultiPoint";
	Json::Value& c(r["coordinates"]);
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		Json::Value p;
		p.append(i->get_lon_deg_dbl());
		p.append(i->get_lat_deg_dbl());
		c.append(p);
	}
	return r;
}

void MultiPoint::from_json(const Json::Value& g)
{
	clear();
	if (!g.isMember("type") || !g["type"].isString() || g["type"].asString() != "MultiPoint")
		throw std::runtime_error("MultiPoint::from_json: invalid type");
	if (!g.isMember("coordinates"))
		throw std::runtime_error("MultiPoint::from_json: no coordinates");
	const Json::Value& c(g["coordinates"]);
	if (!c.isArray())
		throw std::runtime_error("MultiPoint::from_json: invalid coordinates");
	for (Json::ArrayIndex i = 0; i < c.size(); ++i) {
		const Json::Value& cc(c[i]);
		if (!cc.isArray() || cc.size() < 2 || !cc[0].isNumeric() || !cc[1].isNumeric())
			throw std::runtime_error("MultiPoint::from_json: invalid coordinates");
		Point pt;
		pt.set_lon_deg_dbl(cc[0].asDouble());
		pt.set_lat_deg_dbl(cc[1].asDouble());
		push_back(pt);
	}
}

#endif

const std::string LineString::skyvector_url("http://skyvector.com");

bool LineString::is_on_boundary( const Point & pt ) const
{
	for (unsigned int i = 1; i < size(); ++i)
		if (pt.is_between(operator[](i-1), operator[](i)))
			return true;
	return false;
}

bool LineString::is_boundary_point(const Point& pt) const
{
	for (unsigned int i = 0; i < size(); ++i)
		if (pt == operator[](i))
			return true;
	return false;
}

bool LineString::is_intersection(const Point & la, const Point & lb) const
{
	for (unsigned int i = 1; i < size(); ++i)
		if (Point::is_intersection(la, lb, operator[](i-1), operator[](i)))
			return true;
	return false;
}

bool LineString::is_strict_intersection(const Point & la, const Point & lb) const
{
	for (unsigned int i = 1; i < size(); ++i)
		if (Point::is_strict_intersection(la, lb, operator[](i-1), operator[](i)))
			return true;
	return false;
}

bool LineString::is_intersection(const Rect & r) const
{
	return is_intersection(r.get_southwest(), r.get_northwest())
	    || is_intersection(r.get_northwest(), r.get_northeast())
	    || is_intersection(r.get_northeast(), r.get_southeast())
	    || is_intersection(r.get_southeast(), r.get_southwest());
}

bool LineString::is_self_intersecting(void) const
{
	const unsigned int sz(size());
	for (unsigned int i = 1; i < sz; ++i) {
		for (unsigned int j = i + 1; j < sz; ++j) {
			if (Point::is_strict_intersection(operator[](i-1), operator[](i),
							  operator[](j-1), operator[](j)))
				return true;
		}
	}
	return false;
}

bool LineString::PointLatLonSorter::operator()(const Point& a, const Point& b) const
{
	if (a.get_lat() < b.get_lat())
		return true;
	if (b.get_lat() < a.get_lat())
		return false;
	return a.get_lon() < b.get_lon();
}

LineString::pointset_t LineString::get_intersections(const LineString& x) const
{
	pointset_t r;
	const unsigned int sz(size());
	const unsigned int xsz(x.size());
	for (unsigned int i = 0; i < sz; ++i)
		for (unsigned int j = 0; j < xsz; ++j)
			if (operator[](i) == x[j])
				r.insert(x[j]);
	for (unsigned int i = 1; i < sz; ++i) {
		for (unsigned int j = 1; j < xsz; ++j) {
			Point pt;
			if (!pt.intersection(operator[](i-1), operator[](i), x[j-1], x[j]))
				continue;
			Rect r0(Rect(x[j], x[j]).add(x[j-1]));
			Rect r1(Rect(operator[](i), operator[](i)).add(operator[](i-1)));
			if (!r0.is_inside(pt) || !r1.is_inside(pt))
				continue;
			r.insert(pt);
		}
	}
	return r;
}

void LineString::reverse(void)
{
	std::reverse(begin(), end());
}

Rect LineString::get_bbox(void) const
{
	if (!size()) {
		Rect r;
		r.set_empty();
		return r;
	}
	Point::coord_t lat(front().get_lat()), latmin(lat), latmax(lat);
	int64_t lon(front().get_lon()), lonmin(lon), lonmax(lon);
	for (size_type i(1), n(size()); i < n; ++i) {
		Point p(operator[](i) - operator[](i - 1));
		lat += p.get_lat();
		lon += p.get_lon();
		latmin = std::min(latmin, lat);
		latmax = std::max(latmax, lat);
		lonmin = std::min(lonmin, lon);
		lonmax = std::max(lonmax, lon);
	}
	return Rect(Point(lonmin, latmin), Point(lonmax, latmax));
}

bool LineString::operator==(const LineString &p) const
{
	size_type n(size());
	if (n != p.size())
		return false;
	for (size_type i = 0; i < n; ++i)
		if (operator[](i) != p[i])
			return false;
	return true;
}

int LineString::compare(const LineString& x) const
{
	size_type n(size()), xn(x.size());
	if (n < xn)
		return -1;
	if (xn < n)
		return 1;
	for (size_type i = 0; i < n; ++i) {
		int c(operator[](i).compare(x[i]));
		if (c)
			return c;
	}
	return 0;
}

std::ostream& LineString::print(std::ostream& os) const
{
	os << '(';
	bool subseq(false);
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		if (subseq)
			os << ", ";
		subseq = true;
		os << i->get_lon_str2() << ' ' << i->get_lat_str2();
	}
	return os << ')';
}

std::string LineString::to_skyvector(void) const
{
	std::ostringstream url;
	url << skyvector_url << '/';
	if (empty())
		return url.str();
	char paramdelim('?');
	if (!front().is_invalid()) {
		url << paramdelim << "ll=" << front().get_lat_deg_dbl()
		    << ',' << front().get_lon_deg_dbl();
		paramdelim = '&';
	}
	if (true) {
		url << paramdelim << "chart=302";
		paramdelim = '&';
	}
	// flight plan
	url << paramdelim << "plan";
	paramdelim = '=';
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		if (i->is_invalid())
			continue;
		url << paramdelim << "G." << i->get_lat_deg_dbl() << ',' << i->get_lon_deg_dbl();
		paramdelim = ':';
	}
	paramdelim = '&';
	return url.str();
}

unsigned int LineString::get_wkbsize(void) const
{
	return Geometry::get_wkbhdrsize() + 4 + size() * Point::get_wkbptsize();
}

uint8_t *LineString::wkbencode(uint8_t *cp, uint8_t *ep) const
{
	cp = Geometry::wkbencode(cp, ep, (uint32_t)size());
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		cp = i->wkbencode(cp, ep);
	return cp;
}

const uint8_t *LineString::wkbdecode(const uint8_t *cp, const uint8_t *ep, uint8_t byteorder)
{
	uint32_t num;
	cp = Geometry::wkbdecode(cp, ep, num, byteorder);
	reserve(num);
	for (; num; --num) {
		Point pt;
		cp = pt.wkbdecode(cp, ep, byteorder);
		push_back(pt);
	}
	return cp;
}

uint8_t *LineString::to_wkb(uint8_t *cp, uint8_t *ep) const
{
	cp = Geometry::wkbencode(cp, ep, Geometry::type_linestring);
	return wkbencode(cp, ep);
}

std::vector<uint8_t> LineString::to_wkb(void) const
{
	std::vector<uint8_t> b(0, get_wkbsize());
	to_wkb(&b[0], &b[b.size()]);
	return b;
}

const uint8_t *LineString::from_wkb(const uint8_t *cp, const uint8_t *ep)
{
	clear();
	uint8_t byteorder(1);
	{
		Geometry::type_t typ(Geometry::type_invalid);
		cp = Geometry::wkbdecode(cp, ep, typ, byteorder);
		if (typ != Geometry::type_linestring)
			throw std::runtime_error("from_wkb: not a linestring: " + to_str(typ));
	}
	return wkbdecode(cp, ep, byteorder);
}

std::ostream& LineString::to_wktlst(std::ostream& os) const
{
	os << '(';
	static const char comma[] = ",";
	const char *del(comma + 1);
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		i->to_wktpt(os << del);
		del = comma;
	}
	return os << ')';
}

std::ostream& LineString::to_wkt(std::ostream& os) const
{
	return to_wktlst(os << "LINESTRING");
}

#ifdef HAVE_JSONCPP

Json::Value LineString::to_json(void) const
{
	Json::Value r;
	r["type"] = "LineString";
	Json::Value& c(r["coordinates"]);
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		Json::Value p;
		p.append(i->get_lon_deg_dbl());
		p.append(i->get_lat_deg_dbl());
		c.append(p);
	}
	return r;
}

void LineString::from_json(const Json::Value& g)
{
	clear();
	if (!g.isMember("type") || !g["type"].isString() || g["type"].asString() != "LineString")
		throw std::runtime_error("LineString::from_json: invalid type");
	if (!g.isMember("coordinates"))
		throw std::runtime_error("LineString::from_json: no coordinates");
	const Json::Value& c(g["coordinates"]);
	if (!c.isArray())
		throw std::runtime_error("LineString::from_json: invalid coordinates");
	for (Json::ArrayIndex i = 0; i < c.size(); ++i) {
		const Json::Value& cc(c[i]);
		if (!cc.isArray() || cc.size() < 2 || !cc[0].isNumeric() || !cc[1].isNumeric())
			throw std::runtime_error("LineString::from_json: invalid coordinates");
		Point pt;
		pt.set_lon_deg_dbl(cc[0].asDouble());
		pt.set_lat_deg_dbl(cc[1].asDouble());
		push_back(pt);
	}
}

#endif

Rect MultiLineString::get_bbox(void) const
{
	Rect bbox;
	bbox.set_empty();
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		bbox = bbox.add(i->get_bbox());
	return bbox;
}

int MultiLineString::compare(const MultiLineString& x) const
{
	size_type n(size()), xn(x.size());
	if (n < xn)
		return -1;
	if (xn < n)
		return 1;
	for (size_type i = 0; i < n; ++i) {
		int c(operator[](i).compare(x[i]));
		if (c)
			return c;
	}
	return 0;
}

std::ostream& MultiLineString::print(std::ostream& os) const
{
	os << '{';
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		i->print(os << ' ');
	return os << " }";
}

unsigned int MultiLineString::get_wkbsize(void) const
{
	unsigned int sz(Geometry::get_wkbhdrsize() + 4);
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		sz += i->get_wkbsize();
	return sz;
}

uint8_t *MultiLineString::wkbencode(uint8_t *cp, uint8_t *ep) const
{
	cp = Geometry::wkbencode(cp, ep, (uint32_t)size());
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		cp = i->to_wkb(cp, ep);
	return cp;
}

const uint8_t *MultiLineString::wkbdecode(const uint8_t *cp, const uint8_t *ep, uint8_t byteorder)
{
	uint32_t num(0);
	cp = Geometry::wkbdecode(cp, ep, num, byteorder);
	reserve(num);
	for (; num; --num) {
		LineString ls;
		ls.from_wkb(cp, ep);
		push_back(ls);
	}
	return cp;
}

uint8_t *MultiLineString::to_wkb(uint8_t *cp, uint8_t *ep) const
{
	cp = Geometry::wkbencode(cp, ep, Geometry::type_multilinestring);
	return wkbencode(cp, ep);
}

std::vector<uint8_t> MultiLineString::to_wkb(void) const
{
	std::vector<uint8_t> b(0, get_wkbsize());
	to_wkb(&b[0], &b[b.size()]);
	return b;
}

const uint8_t *MultiLineString::from_wkb(const uint8_t *cp, const uint8_t *ep)
{
	clear();
	uint8_t byteorder(1);
	{
		Geometry::type_t typ(Geometry::type_invalid);
		const uint8_t *cp1(Geometry::wkbdecode(cp, ep, typ, byteorder));
		if (typ == Geometry::type_linestring) {
			LineString ls;
			cp = ls.from_wkb(cp, ep);
			push_back(ls);
			return cp;
		}
		cp = cp1;
		if (typ != Geometry::type_multilinestring)
			throw std::runtime_error("from_wkb: not a multilinestring: " + to_str(typ));
	}
	return wkbdecode(cp, ep, byteorder);
}

std::ostream& MultiLineString::to_wkt(std::ostream& os) const
{
	os << "MULTILINESTRING(";
	static const char comma[] = ",";
	const char *del(comma + 1);
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		i->to_wktlst(os << del);
		del = comma;
	}
	return os << ')';
}

#ifdef HAVE_JSONCPP

Json::Value MultiLineString::to_json(void) const
{
	Json::Value r;
	r["type"] = "MultiLineString";
	Json::Value& c(r["coordinates"]);
	for (const_iterator ii(begin()), ee(end()); ii != ee; ++ii) {
		Json::Value pp;
		for (LineString::const_iterator i(ii->begin()), e(ii->end()); i != e; ++i) {
			Json::Value p;
			p.append(i->get_lon_deg_dbl());
			p.append(i->get_lat_deg_dbl());
			pp.append(p);
		}
		c.append(pp);
	}
	return r;
}

void MultiLineString::from_json(const Json::Value& g)
{
	clear();
	if (!g.isMember("type") || !g["type"].isString() || g["type"].asString() != "MultiLineString")
		throw std::runtime_error("MultiLineString::from_json: invalid type");
	if (!g.isMember("coordinates"))
		throw std::runtime_error("MultiLineString::from_json: no coordinates");
	const Json::Value& c(g["coordinates"]);
	if (!c.isArray())
		throw std::runtime_error("MultiLineString::from_json: invalid coordinates");
	for (Json::ArrayIndex i = 0; i < c.size(); ++i) {
		const Json::Value& cc(c[i]);
		if (!cc.isArray())
			throw std::runtime_error("MultiLineString::from_json: invalid coordinates");
		LineString ls;
		for (Json::ArrayIndex ii = 0; ii < cc.size(); ++ii) {
			const Json::Value& ccc(cc[ii]);
			if (!ccc.isArray() || ccc.size() < 2 || !ccc[0].isNumeric() || !ccc[1].isNumeric())
				throw std::runtime_error("MultiLineString::from_json: invalid coordinates");
			Point pt;
			pt.set_lon_deg_dbl(ccc[0].asDouble());
			pt.set_lat_deg_dbl(ccc[1].asDouble());
			ls.push_back(pt);
		}
		push_back(ls);
	}
}

#endif

void PolygonSimple::ScanLine::insert(Point::coord_t lon, int wn)
{
	std::pair<iterator,bool> i(this->insert(value_type(lon, 0)));
	i.first->second += wn;
}

PolygonSimple::ScanLine PolygonSimple::ScanLine::integrate(Point::coord_t lon) const
{
	const_iterator istart(this->lower_bound(lon)), b(this->begin()), e(this->end());
	if (istart == e) {
		istart = b;
		if (istart == e)
			return ScanLine();
	}
	const_iterator icur(istart);
	int wn(0);
	ScanLine sl;
	const Point::coord_t min(std::numeric_limits<Point::coord_t>::min());
	do {
		// compute next iterator (wrap around)
		const_iterator inext(icur);
		++inext;
		if (inext == e)
			inext = b;
		// update winding number
		wn += icur->second;
		sl.insert(value_type(icur->first, wn));
		// check wrap around
		if (wn && inext->first < icur->first && inext->first != min)
			sl.insert(value_type(min, wn));
		// advance to next
		icur = inext;
	} while (icur != istart);
	if (wn) {
		std::ostringstream oss;
		oss << "PolygonSimple::ScanLine::integrate: nonzero winding number integral: " << wn
		    << " lon " << lon;
		for (const_iterator i(b); i != e; ++i)
			oss << " (" << i->first << ',' << i->second << ')';
		throw std::runtime_error(oss.str());
	}
	return sl;
}

PolygonSimple::ScanLine PolygonSimple::ScanLine::combine(const ScanLine& sl2, int factor1, int factor2) const
{
	int wn1(0), wn2(0);
	const_iterator i1(this->begin()), e1(this->end());
	const_iterator i2(sl2.begin()), e2(sl2.end());
	ScanLine sl;
	int wn(0);
	for (;;) {
		Point::coord_t lon;
		{
			bool set1(false), set2(false);
			if (i1 != e1) {
				set1 = true;
				lon = i1->first;
			}
			if (i2 != e2) {
				if (!set1 || i2->first < lon) {
					set2 = true;
					set1 = false;
					lon = i2->first;
				} else if (i2->first == lon) {
					set2 = true;
				}
			}
			if (!(set1 && set2))
				break;
			if (set1) {
				wn1 = i1->second;
				++i1;
			}
			if (set2) {
				wn2 = i2->second;
				++i2;
			}
		}
		int wnnew(wn1 * factor1 + wn2 * factor2);
		if (wnnew != wn) {
			wn = wnnew;
			sl.insert(value_type(lon, wn));
		}
	}
	return sl;
}

const bool PolygonSimple::InsideHelper::debug;
const int64_t PolygonSimple::InsideHelper::bordertolerance;

PolygonSimple::InsideHelper::InsideHelper(void)
	: m_origin(Point::invalid), m_end(-1), m_simd(simd_none)
{
}

PolygonSimple::InsideHelper::InsideHelper(const Point& pt0, const Point& pt1, bool enable_simd)
	: m_origin(pt0), m_end(-1), m_simd(simd_none)
{
#if defined(HAVE_SIMD_X64) && defined(__GNUC__) && defined(__GNUC_MINOR__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))
	if (enable_simd && __builtin_cpu_supports("avx")) {
		m_simd = simd_avx;
		if (!false && __builtin_cpu_supports("avx2"))
			m_simd = simd_avx2;
	}
#endif
	if (get_origin().is_invalid() || pt1.is_invalid()) {
		m_vector.set_invalid();
		return;
	}
	m_vector = pt1 - get_origin();
	{
		int64_t a(m_vector.get_lon()), b(m_vector.get_lat());
		m_end = a * a + b * b;
		if (!m_end)
			m_vector.set_lon(1);
	}
}

Point PolygonSimple::InsideHelper::transform(const tpoint_t& tp) const
{
	double a(m_vector.get_lon()), b(m_vector.get_lat());
	{
		double t(a * a + b * b);
		if (t > 0) {
			t = 1.0 / t;
			a *= t;
			b *= t;
		}
	}
	double pa(a * tp.first - b * tp.second);
	double pb(b * tp.first + a * tp.second);
	Point p(Point::round<Point::coord_t,double>(pa), Point::round<Point::coord_t,double>(pb));
	p += get_origin();
	return p;
}

void PolygonSimple::InsideHelper::compute_winding(winding_t& wind, const PolygonHole& poly, int dir) const
{
	compute_winding(wind, poly.get_exterior(), dir);
	for (unsigned int i(0), n(poly.get_nrinterior()); i < n; ++i)
		compute_winding(wind, poly[i], dir);
}

void PolygonSimple::InsideHelper::compute_winding(winding_t& wind, const MultiPolygonHole& poly, int dir) const
{
	for (MultiPolygonHole::const_iterator phi(poly.begin()), phe(poly.end()); phi != phe; ++phi)
		compute_winding(wind, *phi, dir);
}

PolygonSimple::InsideHelper::interval_t PolygonSimple::InsideHelper::finalize(const winding_t& wind)
{
	int w(0);
	interval_t r;
	interval_t::type_t xmin(0);
	if (false) {
		std::cerr << "InsideHelper::finalize:";
		for (winding_t::const_iterator wi(wind.begin()), we(wind.end()); wi != we; ++wi)
			std::cerr << ' ' << wi->first << ':' << wi->second;
		std::cerr << std::endl;
	}
	for (winding_t::const_iterator wi(wind.begin()), we(wind.end()); wi != we; ++wi) {
		if (!wi->second)
			continue;
		bool wpz(!w);
		w += wi->second;
		bool wz(!w);
		if (wpz && !wz)
			xmin = wi->first;
		if (!wpz && wz) {
			r |= interval_t::Intvl(xmin, wi->first);
			xmin = wi->first;
		}
	}
	if (w) {
		std::ostringstream oss;
		for (winding_t::const_iterator wi(wind.begin()), we(wind.end()); wi != we; ++wi)
			oss << ' ' << wi->first << ':' << wi->second;
		std::cerr << "InsideHelper::finalize: winding number error" << oss.str() << " sum " << w << std::endl;
		set_error();
		if (false)
			throw std::runtime_error("InsideHelper::finalize: invalid winding numbers");
	}
	return r;
}

PolygonSimple::InsideHelper::interval_t PolygonSimple::InsideHelper::compute_winding(const PolygonSimple& poly)
{
	winding_t wind;
	compute_winding(wind, poly, 1);
	return finalize(wind);
}

PolygonSimple::InsideHelper::interval_t PolygonSimple::InsideHelper::compute_winding(const PolygonHole& poly)
{
	winding_t wind;
	compute_winding(wind, poly, 1);
	return finalize(wind);
}

PolygonSimple::InsideHelper::interval_t PolygonSimple::InsideHelper::compute_winding(const MultiPolygonHole& poly)
{
	winding_t wind;
	compute_winding(wind, poly, 1);
	return finalize(wind);
}

PolygonSimple::inside_t PolygonSimple::InsideHelper::get_result(void) const
{
	if (is_error()) {
		if (debug)
			std::cerr << "InsideHelper: marked as error, result " << ::to_str(inside_error) << std::endl;
		return inside_error;
	}
	const interval_t::Intvl ib0(get_startintvl());
	const interval_t::Intvl ib1(get_endintvl());
	const interval_t::Intvl iins(get_insideintvl());
	inside_t r(inside_none);
	if (!(*this & ib0).is_empty())
		r |= inside_point0;
	if (!(*this & ib1).is_empty())
		r |= inside_point1;
	if (!(*this & iins).is_empty())
		r |= ((~*this) & iins).is_empty() ? inside_all : inside_partial;
	if (debug)
		std::cerr << "InsideHelper: interval: " << to_str() << " result " << ::to_str(r) << std::endl;
	return r;
}

PolygonSimple::borderpoints_t PolygonSimple::InsideHelper::get_borderpoints(void) const
{
	borderpoints_t b;
	if (is_error())
		return b;
	for (interval_t::const_iterator i(begin()), e(end()); i != e; ++i) {
		if (i->get_lower() > m_end || i->get_upper() <= 0)
			continue;
		int64_t x(i->get_lower());
		BorderPoint::flagmask_t f(BorderPoint::flagmask_inc);
		if (x < 0)
			x = 0;
		else
			f |= BorderPoint::flagmask_onborder;
		Point pt(transform(tpoint_t(x, 0)));
		if (!x) {
			f |= BorderPoint::flagmask_begin;
			pt = get_origin();
		}
		if (x == m_end) {
			f |= BorderPoint::flagmask_end;
			pt = get_origin() + m_vector;
		}
		b.push_back(BorderPoint(pt, pt.spheric_distance_nmi_dbl(get_origin()), f));
		x = i->get_upper() - 1;
		f = BorderPoint::flagmask_dec;
		if (x > m_end)
			x = m_end;
		else
			f |= BorderPoint::flagmask_onborder;
		pt = transform(tpoint_t(x, 0));
		if (!x) {
			f |= BorderPoint::flagmask_begin;
			pt = get_origin();
		}
		if (x == m_end) {
			f |= BorderPoint::flagmask_end;
			pt = get_origin() + m_vector;
		}
		b.push_back(BorderPoint(pt, pt.spheric_distance_nmi_dbl(get_origin()), f));
	}
	return b;
}

void PolygonSimple::InsideHelper::limit(int64_t min, int64_t max)
{
	{
		int64_t x(max + 1);
		if (x > max)
			max = x;
	}
	if (max <= min) {
		set_empty();
		return;
	}
	*this &= Intvl(min, max);
}

void PolygonSimple::InsideHelper::clear(void)
{
	set_empty();
}

void PolygonSimple::InsideHelper::invert(void)
{
	*this = operator~();
}

std::string PolygonSimple::InsideHelper::to_str(void) const
{
	if (get_end() <= 0)
		return "error";
	bool subseq(false);
	std::ostringstream r;
	r << std::fixed << std::setprecision(3);
	double emul(1.0 / get_end());
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		if (i->is_empty())
			continue;
		if (subseq)
			r << '|';
		subseq = true;
		r << '[' << (i->get_lower() * emul) << ',' << (i->get_upper() * emul) << ')';
	}
	if (subseq)
		return r.str();
	return "[)";
}

std::string to_str(PolygonSimple::inside_t i)
{
	std::string r;
	switch (i & PolygonSimple::inside_mask) {
	case PolygonSimple::inside_none:
		r = "none";
		break;

	case PolygonSimple::inside_partial:
		r = "partial";
		break;

	case PolygonSimple::inside_all:
		r = "all";
		break;

	case PolygonSimple::inside_error:
		r = "error";
		break;

	default:
		r = "?";
		break;
	}
	{
		char c('(');
		if (i & PolygonSimple::inside_point0) {
			r += c;
			r += "point0";
			c = ',';
		}
		if (i & PolygonSimple::inside_point1) {
			r += c;
			r += "point1";
			c = ',';
		}
		if (c != '(')
			r += ')';
	}
	return r;
}

Rect::operator PolygonSimple(void) const
{
	PolygonSimple p;
	p.push_back(get_southwest());
	p.push_back(get_southeast());
	p.push_back(get_northeast());
	p.push_back(get_northwest());
	return p;
}

std::ostream& operator<<(std::ostream& os, const Point& p)
{
	return p.print(os);
}

std::ostream & operator<<(std::ostream & os, const Rect & r)
{
	return r.print(os);
}

PolygonSimple::reference PolygonSimple::operator[](size_type x)
{
	size_type y(size());
	if (x >= y && y > 0)
		x %= y;
	return base_t::operator[](x);
}

PolygonSimple::const_reference PolygonSimple::operator[](size_type x) const
{
	size_type y(size());
	if (x >= y && y > 0)
		x %= y;
	return base_t::operator[](x);
}

bool PolygonSimple::is_on_boundary( const Point & pt ) const
{
	unsigned int j = size() - 1;
	for (unsigned int i = 0; i < size(); j = i, ++i)
		if (pt.is_between(operator[](i), operator[](j)))
			return true;
	return false;
}

bool PolygonSimple::is_intersection(const Point & la, const Point & lb) const
{
	unsigned int j = size() - 1;
	for (unsigned int i = 0; i < size(); j = i, ++i)
		if (Point::is_intersection(la, lb, operator[](i), operator[](j)))
			return true;
	return false;
}

bool PolygonSimple::is_strict_intersection(const Point & la, const Point & lb) const
{
	unsigned int j = size() - 1;
	for (unsigned int i = 0; i < size(); j = i, ++i)
		if (Point::is_strict_intersection(la, lb, operator[](i), operator[](j)))
			return true;
	return false;
}

bool PolygonSimple::is_intersection(const Rect & r) const
{
	return is_intersection(r.get_southwest(), r.get_northwest())
	    || is_intersection(r.get_northwest(), r.get_northeast())
	    || is_intersection(r.get_northeast(), r.get_southeast())
	    || is_intersection(r.get_southeast(), r.get_southwest());
}

bool PolygonSimple::is_self_intersecting(void) const
{
	const unsigned int sz(size());
	for (unsigned int i = 0, j = sz - 1; i < sz; j = i, ++i) {
		for (unsigned int k = i + 1, l = i; k < sz; l = k, ++k) {
			if (Point::is_strict_intersection(operator[](i), operator[](j),
							  operator[](k), operator[](l)))
				return true;
		}
	}
	return false;
}

PolygonSimple::pointset_t PolygonSimple::get_intersections(const LineString& x) const
{
	pointset_t r;
	const unsigned int sz(size());
	const unsigned int xsz(x.size());
	for (unsigned int i = 0; i < sz; ++i)
		for (unsigned int j = 0; j < xsz; ++j)
			if (operator[](i) == x[j])
				r.insert(x[j]);
      	for (unsigned int i = 0, j = sz - 1; i < sz; j = i, ++i) {
		for (unsigned int k = 0, l = xsz - 1; k < xsz; l = k, ++k) {
			Point pt;
			if (!pt.intersection(operator[](j), operator[](i), x[l], x[k]))
				continue;
			Rect r0(Rect(x[k], x[k]).add(x[l]));
			Rect r1(Rect(operator[](i), operator[](i)).add(operator[](j)));
			if (!r0.is_inside(pt) || !r1.is_inside(pt))
				continue;
			r.insert(pt);
		}
	}
	return r;
}

bool PolygonSimple::is_all_points_inside(const Rect& r) const
{
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		if (!r.is_inside(*i))
			return false;
	return true;
}

bool PolygonSimple::is_any_point_inside(const Rect& r) const
{
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		if (r.is_inside(*i))
			return true;
	return false;
}

bool PolygonSimple::is_overlap(const Rect& r) const
{
	if (empty())
		return false;
	{
		Rect bbox(get_bbox());
		if (!r.is_intersect(bbox))
			return false;
	}
	if (is_any_point_inside(r))
		return true;
	if (windingnumber(r.get_northeast()) ||
	    windingnumber(r.get_southeast()) ||
	    windingnumber(r.get_northwest()) ||
	    windingnumber(r.get_southwest()))
		return true;
	if (is_intersection(r))
		return true;
	return false;
}

int PolygonSimple::windingnumber(const Point & pt) const
{
	int wn = 0; /* the winding number counter */

	/* loop through all edges of the polygon */
	const unsigned int sz(size());
	unsigned int j = sz - 1;
	for (unsigned int i = 0; i < sz; j = i, ++i) {
		const Point& pti(operator[](i));
		const Point& ptj(operator[](j));
		int64_t lon = Point::area2(ptj, pti, pt);
		/* check if pt lies on edge from V[j] to V[i] */
		if (!lon && pt.is_between_helper(ptj, pti))
			return std::numeric_limits<int>::max();
		/* edge from V[j] to V[i] */
		if (ptj.get_lat() <= pt.get_lat()) {
			if (pti.get_lat() > pt.get_lat())
				/* an upward crossing */
				if (lon > 0)
					/* P left of edge: have a valid up intersect */
					wn++;
		} else {
			if (pti.get_lat() <= pt.get_lat())
				/* a downward crossing */
				if (lon < 0)
					/* P right of edge: have a valid down intersect */
					wn--;
		}
	}
	return wn;
}

PolygonSimple::ScanLine PolygonSimple::scanline(Point::coord_t lat) const
{
	const unsigned int sz(size());
	if (!sz)
		return ScanLine();
	ScanLine sl;
	Rect bbox(operator[](0), operator[](0));
	unsigned int j = sz - 1;
	for (unsigned int i = 0; i < sz; j = i, ++i) {
		const Point& pti(operator[](i));
		const Point& ptj(operator[](j));
		bbox = bbox.add(pti);
		if (false)
			std::cerr << "scanline: lat " << lat << " edge " << ptj.get_lon() << ',' << ptj.get_lat()
				  << ' ' << pti.get_lon() << ',' << pti.get_lat() << std::endl;
		if (pti.get_lat() == lat && ptj.get_lat() == lat) {
			// left->right: +1
			// right->left: -1
			int wn(1 | -(ptj.get_lon() >= pti.get_lon()));
			sl.insert(ptj.get_lon(), wn);
			sl.insert(pti.get_lon(), -wn);
			if (false)
				std::cerr << "scanline: insert " << ptj.get_lon() << ',' << wn << ' '
					  << pti.get_lon() << ',' << -wn << std::endl;
			continue;
		}
		int wn(0);
		if (ptj.get_lat() <= lat) {
			if (pti.get_lat() > lat)
				// upward edge: +1
				wn = 1;
		} else {
			if (pti.get_lat() <= lat)
				// downward edge: -1
				wn = -1;
		}
		if (!wn)
			continue;
		Point ptdiff(pti - ptj);
		Point::coord_t lon(ptj.get_lon() + (ptdiff.get_lon() * (lat - ptj.get_lat()) + ptdiff.get_lat() / 2) / ptdiff.get_lat());
		sl.insert(lon, wn);
		if (false)
			std::cerr << "scanline: insert " << lon << ',' << wn << std::endl;
	}
	try {
		return sl.integrate(bbox.get_west());
	} catch (const std::runtime_error& e) {
		std::ostringstream oss;
		oss << e.what() << " lat " << lat << " poly";
		for (unsigned int i = 0; i < sz; ++i) {
			const Point& pti(operator[](i));
			oss << ' ' << pti.get_lon() << ',' << pti.get_lat();
		}
		throw std::runtime_error(oss.str());
	}
}

PolygonSimple::inside_t PolygonSimple::get_inside(const Point& pt0, const Point& pt1) const
{
	PolygonSimple::InsideHelper state(pt0, pt1);
	state = *this;
	return state.get_result();
}

PolygonSimple::borderpoints_t PolygonSimple::get_borderpoints(const Point& pt0, const Point& pt1) const
{
	PolygonSimple::InsideHelper state(pt0, pt1);
	state = *this;
	return state.get_borderpoints();
}

/*
 * negative, if polygon is clockwise oriented
 */
int64_t PolygonSimple::area2(void) const
{
	int64_t area = 0;

	/* http://mathworld.wolfram.com/PolygonArea.html */
	/* loop through all edges of the polygon */
	const unsigned int n(size());
	for (unsigned int i = 0, j = n - 1; i < n; j = i, i++) {
		/* edge from V[j] to V[i]  (j = (i + 1) mod size) */
		Point pt1(operator[](j) - operator[](0));
		Point pt2(operator[](i) - operator[](0));
		int64_t x1 = pt1.get_lon();
		int64_t y1 = pt1.get_lat();
		int64_t x2 = pt2.get_lon();
		int64_t y2 = pt2.get_lat();
		area += (x1 * y2 - x2 * y1);
	}
	return area;
}

Point PolygonSimple::centroid(void) const
{
	/* http://en.wikipedia.org/wiki/Centroid */
	/* loop through all edges of the polygon */
	const size_type n(size());
	if (!n)
		return Point::invalid;
	if (n == 1)
		return operator[](0);
	if (n == 2)
		return operator[](0).halfway(operator[](1));
	int64_t area = 0;
	double cx = 0, cy = 0;
	for (size_type i = 0, j = n - 1; i < n; j = i, i++) {
		/* edge from V[j] to V[i]  (j = (i + 1) mod size) */
		Point pt1(operator[](j) - operator[](0));
		Point pt2(operator[](i) - operator[](0));
		int64_t x1 = pt1.get_lon();
		int64_t y1 = pt1.get_lat();
		int64_t x2 = pt2.get_lon();
		int64_t y2 = pt2.get_lat();
		int64_t m = x1 * y2 - x2 * y1;
		cx += (x1 + x2) * (double)m;
		cy += (y1 + y2) * (double)m;
		area += m;
	}
	if (!area)
		return Point::invalid;
	area *= 3;
	return operator[](0) + Point(cx / area, cy / area);
}

void PolygonSimple::make_counterclockwise(void)
{
	if (area2() >= 0)
		return;
	reverse();
}

void PolygonSimple::make_clockwise(void)
{
	if (area2() <= 0)
		return;
	reverse();
}

unsigned int PolygonSimple::next(unsigned int it) const
{
	if (size() < 2U)
		return 0U;
	++it;
	if (it >= size())
		it = 0;
	return it;
}

unsigned int PolygonSimple::prev(unsigned int it) const
{
	if (size() < 2U)
		return 0U;
	if (!it)
		it = size();
	--it;
	return it;
}

void PolygonSimple::remove_redundant_polypoints(void)
{
	size_type sz = size();
	if (sz >= 2) {
		if (operator[](0) == operator[](sz-1)) {
			sz--;
			resize(sz);
		}
	}
	if (sz < 3) {
		clear();
		return;
	}
	bool first = true;
	for (size_type i = 0; i < sz;) {
		size_type j = (i + 1) % sz;
		size_type k = (j + 1) % sz;
		if (!operator[](j).is_between(operator[](i), operator[](k))) {
			++i;
			continue;
		}
#if 0
		if (first) {
			std::cerr << "remove_redundant_polypoints: poly:";
			for (unsigned int m = 0; m < sz; m++)
				operator[](m).print(std::cerr << ' ');
			std::cerr << std::endl;
		}
		first = false;
		operator[](j).print(std::cerr << "remove_redundant_polypoints: removing: ");
		operator[](i).print(std::cerr << " between: ");
		operator[](k).print(std::cerr << ' ') << std::endl;
#endif
		erase(begin() + j);
		--sz;
		if (sz < 3)
			break;
	}
	if (sz < 3)
		clear();
}

void PolygonSimple::simplify_outside(const Rect& bbox)
{
	size_type n(size());
	const Point ps(bbox.get_northeast() - bbox.get_southwest());
	for (size_type i0(0); i0 < n; ) {
		size_type i1(i0 + 1);
		if (i1 >= n)
			i1 = 0;
		size_type i2(i1 + 1);
		if (i2 >= n)
			i2 = 0;
		Point p0(operator[](i0) - bbox.get_southwest());
		Point p1(operator[](i1) - bbox.get_southwest());
		Point p2(operator[](i2) - bbox.get_southwest());
		// check if all 3 points are on the same side of the rectangle; if so remove the center one
		if ((p0.get_lon() < 0 &&
		     p1.get_lon() < 0 &&
		     p2.get_lon() < 0) ||
		    (p0.get_lat() < 0 &&
		     p1.get_lat() < 0 &&
		     p2.get_lat() < 0) ||
		    (p0.get_lon() > ps.get_lon() &&
		     p1.get_lon() > ps.get_lon() &&
		     p2.get_lon() > ps.get_lon()) ||
		    (p0.get_lat() > ps.get_lat() &&
		     p1.get_lat() > ps.get_lat() &&
		     p2.get_lat() > ps.get_lat())) {
			erase(begin() + i1);
			if (i1 < i0)
				--i0;
			--n;
			continue;
		}
		++i0;
	}
}

uint64_t PolygonSimple::simple_circumference_rel(void) const
{
	uint64_t d(0);
	for (size_type i = 0, j = size() - 1, k = size(); i < k; j = i, ++i)
		d += operator[](i).simple_distance_rel(operator[](j));
	return d;
}

float PolygonSimple::simple_circumference_km(void) const
{
	float d(0);
	for (size_type i = 0, j = size() - 1, k = size(); i < k; j = i, ++i)
		d += operator[](i).simple_distance_km(operator[](j));
	return d;
}

float PolygonSimple::spheric_circumference_km(void) const
{
	double d(0);
	for (size_type i = 0, j = size() - 1, k = size(); i < k; j = i, ++i)
		d += operator[](i).spheric_distance_km(operator[](j));
	return d;
}

double PolygonSimple::spheric_circumference_km_dbl(void) const
{
	double d(0);
	for (size_type i = 0, j = size() - 1, k = size(); i < k; j = i, ++i)
		d += operator[](i).spheric_distance_km_dbl(operator[](j));
	return d;
}

double PolygonSimple::get_simple_area_km2_dbl(void) const
{
	Point center(get_bbox().boxcenter());
	if (center.is_invalid())
		return std::numeric_limits<double>::quiet_NaN();
	// area2 delivers the double area
	double a(area2());
	a *= (0.5 * Point::earth_radius_dbl * Point::to_rad_dbl *
	      Point::earth_radius_dbl * Point::to_rad_dbl);
	a *= cos(center.get_lat_rad());
	return a;
}

PolygonSimple PolygonSimple::line_width_rectangular(const Point& l0, const Point& l1, double width_nmi)
{
	double crs(0);
	if (l0 != l1)
		crs = l0.spheric_true_course_dbl(l1);
	width_nmi *= 0.5;
	PolygonSimple r;
	r.push_back(l0.spheric_course_distance_nmi(crs + 135, width_nmi));
	r.push_back(l0.spheric_course_distance_nmi(crs + 225, width_nmi));
	r.push_back(l1.spheric_course_distance_nmi(crs + 315, width_nmi));
	r.push_back(l1.spheric_course_distance_nmi(crs + 45, width_nmi));
	if (true && (!r.windingnumber(l0) || !r.windingnumber(l1)))
		throw std::runtime_error("line_width_rectangular points not inside box");
	return r;
}

void PolygonSimple::snap_bits(unsigned int nrbits)
{
	if (!nrbits)
		return;
	Point::coord_t mask(~((1U << nrbits) - 1U));
	Point::coord_t rnd((1U << nrbits) >> 1);
	for (iterator i(begin()), e(end()); i != e; ++i) {
		Point& pt(*i);
		pt.set_lon((pt.get_lon() + rnd) & mask);
		pt.set_lat((pt.get_lat() + rnd) & mask);
	}
	remove_redundant_polypoints();
}

void PolygonSimple::randomize_bits(unsigned int nrbits)
{
	if (!nrbits)
		return;
	Point::coord_t rmask((1U << nrbits) - 1U);
	Point::coord_t mask(~rmask);
	for (iterator i(begin()), e(end()); i != e; ++i) {
		Point& pt(*i);
		pt.set_lon((pt.get_lon() & mask) | (rand() & rmask));
		pt.set_lat((pt.get_lat() & mask) | (rand() & rmask));
	}
	remove_redundant_polypoints();
}

unsigned int PolygonSimple::zap_leastimportant(Point& pt)
{
	pt.set_invalid();
	size_type sz(size());
	if (sz < 4) {
		clear();
		return 0;
	}
	size_type idx(0);
	int64_t area(std::numeric_limits<int64_t>::max());
	for (size_type i = 0; i < sz; ++i) {
		size_type j = (i + 1) % sz;
		size_type k = (j + 1) % sz;
		int64_t a(Point::area2(operator[](i), operator[](j), operator[](k)));
		if (a > area)
			continue;
		area = a;
		idx = j;
	}
	pt = operator[](idx);
	erase(begin() + idx);
	return idx;
}

std::ostream& PolygonSimple::print_nrvertices(std::ostream& os, bool prarea) const
{
	os << size();
	if (prarea)
		os << " [" << area2() << ']';
	return os;
}

std::string PolygonSimple::to_skyvector(void) const
{
	std::ostringstream url;
	url << skyvector_url << '/';
	if (empty())
		return url.str();
	char paramdelim('?');
	if (!front().is_invalid()) {
		url << paramdelim << "ll=" << front().get_lat_deg_dbl()
		    << ',' << front().get_lon_deg_dbl();
		paramdelim = '&';
	}
	if (true) {
		url << paramdelim << "chart=302";
		paramdelim = '&';
	}
	// flight plan
	url << paramdelim << "plan";
	paramdelim = '=';
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		if (i->is_invalid())
			continue;
		url << paramdelim << "G." << i->get_lat_deg_dbl() << ',' << i->get_lon_deg_dbl();
		paramdelim = ':';
	}
	if (!front().is_invalid()) {
		url << paramdelim << "G." << front().get_lat_deg_dbl() << ',' << front().get_lon_deg_dbl();
		paramdelim = ':';
	}
	paramdelim = '&';
	return url.str();
}

void PolygonSimple::make_fat_lines_helper(MultiPolygonHole& ph, float radius, bool roundcaps, float angleincr, bool rawcoord) const
{
	MultiPolygonHole ph2(make_fat_lines(radius, roundcaps, angleincr, rawcoord));
	ph.insert(ph.end(), ph2.begin(), ph2.end());
}

MultiPolygonHole PolygonSimple::make_fat_lines(float radius, bool roundcaps, float angleincr, bool rawcoord) const
{
	std::vector<MultiPolygonHole> p;
	for (unsigned int i = 0, j = size() - 1, k = size(); i < k; j = i, ++i) {
		PolygonSimple ps(operator[](i).make_fat_line(operator[](j), radius, roundcaps, angleincr, rawcoord));
		ps.make_counterclockwise();
		if (!(i & 1)) {
			p.push_back(MultiPolygonHole());
			p.back().push_back(PolygonHole(ps));
			continue;
		}
		MultiPolygonHole ph2;
		ph2.push_back(PolygonHole(ps));
		p.back().geos_union(ph2);
	}
	while (p.size() > 1) {
		std::vector<MultiPolygonHole> p2;
		bool n(true);
		for (std::vector<MultiPolygonHole>::const_iterator pi(p.begin()), pe(p.end()); pi != pe; ++pi, n = !n) {
			if (n) {
				p2.push_back(*pi);
				continue;
			}
			p2.back().geos_union(*pi);
		}
		p.swap(p2);
	}
	if (p.empty())
		return MultiPolygonHole();
	return p.front();
}

void PolygonSimple::bindblob(sqlite3x::sqlite3_command & cmd, int index) const
{
	unsigned int sz(size() * 8);
	if (!sz) {
		cmd.bind(index);
		return;
	}
	uint8_t data[sz];
	for (size_type i(0); i < size(); i++) {
		Point pt(operator[](i));
		data[8*i+0] = pt.get_lon() >> 0;
		data[8*i+1] = pt.get_lon() >> 8;
		data[8*i+2] = pt.get_lon() >> 16;
		data[8*i+3] = pt.get_lon() >> 24;
		data[8*i+4] = pt.get_lat() >> 0;
		data[8*i+5] = pt.get_lat() >> 8;
		data[8*i+6] = pt.get_lat() >> 16;
		data[8*i+7] = pt.get_lat() >> 24;
	}
	cmd.bind(index, data, sz);
}

void PolygonSimple::getblob(sqlite3x::sqlite3_cursor & cursor, int index)
{
	clear();
	if (cursor.isnull(index))
		return;
	int sz;
	uint8_t const *data((uint8_t const *)cursor.getblob(index, sz));
	if (sz <= 7)
		return;
	sz >>= 3;
	reserve(sz);
	for (size_type i(0); i < (size_type)sz; i++) {
		uint32_t lon, lat;
		lon = data[8*i+0] | (data[8*i+1] << 8) | (data[8*i+2] << 16) | (data[8*i+3] << 24);
		lat = data[8*i+4] | (data[8*i+5] << 8) | (data[8*i+6] << 16) | (data[8*i+7] << 24);
		push_back(Point(lon, lat));
	}
}

#ifdef HAVE_PQXX

pqxx::binarystring PolygonSimple::bindblob(void) const
{
	unsigned int sz(size() * 8);
	if (!sz)
		return pqxx::binarystring(0, 0);
	uint8_t data[sz];
	for (size_type i(0); i < size(); i++) {
		Point pt(operator[](i));
		data[8*i+0] = pt.get_lon() >> 0;
		data[8*i+1] = pt.get_lon() >> 8;
		data[8*i+2] = pt.get_lon() >> 16;
		data[8*i+3] = pt.get_lon() >> 24;
		data[8*i+4] = pt.get_lat() >> 0;
		data[8*i+5] = pt.get_lat() >> 8;
		data[8*i+6] = pt.get_lat() >> 16;
		data[8*i+7] = pt.get_lat() >> 24;
	}
	return pqxx::binarystring(data, sz);
}

void PolygonSimple::getblob(const pqxx::binarystring& blob)
{
	clear();
	if (blob.size() <= 7)
		return;
	size_type sz(blob.size() >> 3);
	uint8_t const *data(blob.data());
	reserve(sz);
	for (size_type i(0); i < sz; i++) {
		uint32_t lon, lat;
		lon = data[8*i+0] | (data[8*i+1] << 8) | (data[8*i+2] << 16) | (data[8*i+3] << 24);
		lat = data[8*i+4] | (data[8*i+5] << 8) | (data[8*i+6] << 16) | (data[8*i+7] << 24);
		push_back(Point(lon, lat));
	}
}

#endif

unsigned int PolygonSimple::get_wkbpolysize(void) const
{
	return 4 + (empty() ? 0 : ((size() + 1) * Point::get_wkbptsize()));
}

unsigned int PolygonSimple::get_wkbsize(void) const
{
	return Geometry::get_wkbhdrsize() + 4 + get_wkbpolysize();
}

uint8_t *PolygonSimple::wkbencode(uint8_t *cp, uint8_t *ep) const
{
	if (empty())
		return Geometry::wkbencode(cp, ep, (uint32_t)0);
	cp = Geometry::wkbencode(cp, ep, (uint32_t)(size() + 1));
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		cp = i->wkbencode(cp, ep);
	return front().wkbencode(cp, ep);
}

const uint8_t *PolygonSimple::wkbdecode(const uint8_t *cp, const uint8_t *ep, uint8_t byteorder)
{
	clear();
	uint32_t num;
	cp = Geometry::wkbdecode(cp, ep, num, byteorder);
	reserve(num);
	for (; num; --num) {
		Point pt;
		cp = pt.wkbdecode(cp, ep, byteorder);
		push_back(pt);
	}
	if (size() >= 2 && front() == back())
		resize(size() - 1);
	return cp;
}

uint8_t *PolygonSimple::to_wkb(uint8_t *cp, uint8_t *ep) const
{
	cp = Geometry::wkbencode(cp, ep, Geometry::type_polygon);
	cp = Geometry::wkbencode(cp, ep, (uint32_t)1);
	cp = wkbencode(cp, ep);
	return cp;
}

std::vector<uint8_t> PolygonSimple::to_wkb(void) const
{
	std::vector<uint8_t> b(0, get_wkbsize());
	to_wkb(&b[0], &b[b.size()]);
	return b;
}

const uint8_t *PolygonSimple::from_wkb(const uint8_t *cp, const uint8_t *ep)
{
	clear();
	uint8_t byteorder(1);
	{
		Geometry::type_t typ(Geometry::type_invalid);
		cp = Geometry::wkbdecode(cp, ep, typ, byteorder);
		if (typ != Geometry::type_polygon)
			throw std::runtime_error("from_wkb: not a polygon: " + to_str(typ));
	}
	uint32_t num(0);
	cp = Geometry::wkbdecode(cp, ep, num, byteorder);
	if (num > 1)
		throw std::runtime_error("from_wkb: not a simple polygon");
	if (num)
		cp = wkbdecode(cp, ep, byteorder);
	return cp;
}

std::ostream& PolygonSimple::to_wktpoly(std::ostream& os) const
{
	os << '(';
	static const char comma[] = ",";
	const char *del(comma + 1);
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		i->to_wktpt(os << del);
		del = comma;
	}
	if (!empty()) {
		front().to_wktpt(os << del);
		del = comma;
	}
	return os << ')';
}

std::ostream& PolygonSimple::to_wkt(std::ostream& os) const
{
	return to_wktpoly(os << "POLYGON(") << ')';
}

#ifdef HAVE_JSONCPP

Json::Value PolygonSimple::to_json(void) const
{
	Json::Value r;
	r["type"] = "Polygon";
	Json::Value& c(r["coordinates"]);
	Json::Value ri;
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		Json::Value p;
		p.append(i->get_lon_deg_dbl());
		p.append(i->get_lat_deg_dbl());
		ri.append(p);
	}
	c.append(ri);
	return r;
}

void PolygonSimple::from_json(const Json::Value& g)
{
	clear();
	if (!g.isMember("type") || !g["type"].isString() || g["type"].asString() != "Polygon")
		throw std::runtime_error("PolygonSimple::from_json: invalid type");
	if (!g.isMember("coordinates"))
		throw std::runtime_error("PolygonSimple::from_json: no coordinates");
	const Json::Value& c(g["coordinates"]);
	if (!c.isArray() || c.size() != 1)
		throw std::runtime_error("PolygonSimple::from_json: invalid coordinates");
	{
		const Json::Value& cc(c[0]);
		if (!cc.isArray())
			throw std::runtime_error("PolygonSimple::from_json: invalid coordinates");
		for (Json::ArrayIndex ii = 0; ii < cc.size(); ++ii) {
			const Json::Value& ccc(cc[ii]);
			if (!ccc.isArray() || ccc.size() < 2 || !ccc[0].isNumeric() || !ccc[1].isNumeric())
				throw std::runtime_error("PolygonSimple::from_json: invalid coordinates");
			Point pt;
			pt.set_lon_deg_dbl(ccc[0].asDouble());
			pt.set_lat_deg_dbl(ccc[1].asDouble());
			push_back(pt);
		}
	}
}

#endif

bool PolygonHole::is_on_boundary(const Point & pt) const
{
	if (m_exterior.is_on_boundary(pt))
		return true;
	for (interior_t::const_iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
		if (ii->is_on_boundary(pt))
			return true;
	return false;
}

bool PolygonHole::is_boundary_point(const Point& pt) const
{
	if (m_exterior.is_boundary_point(pt))
		return true;
	for (interior_t::const_iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
		if (ii->is_boundary_point(pt))
			return true;
	return false;
}

bool PolygonHole::is_intersection(const Point & la, const Point & lb) const
{
	if (m_exterior.is_intersection(la, lb))
		return true;
	for (interior_t::const_iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
		if (ii->is_intersection(la, lb))
			return true;
	return false;
}

bool PolygonHole::is_strict_intersection(const Point & la, const Point & lb) const
{
	if (m_exterior.is_strict_intersection(la, lb))
		return true;
	for (interior_t::const_iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
		if (ii->is_strict_intersection(la, lb))
			return true;
	return false;
}

bool PolygonHole::is_intersection(const Rect & r) const
{
	if (m_exterior.is_intersection(r))
		return true;
	for (interior_t::const_iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
		if (ii->is_intersection(r))
			return true;
	return false;
}

bool PolygonHole::is_all_points_inside(const Rect& r) const
{
	if (!m_exterior.is_all_points_inside(r))
		return false;
	for (interior_t::const_iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
		if (!ii->is_all_points_inside(r))
			return false;
	return true;
}

bool PolygonHole::is_any_point_inside(const Rect& r) const
{
	if (m_exterior.is_any_point_inside(r))
		return true;
	for (interior_t::const_iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
		if (ii->is_any_point_inside(r))
			return true;
	return false;
}

bool PolygonHole::is_overlap(const Rect& r) const
{
	if (m_exterior.empty())
		return false;
	{
		Rect bbox(get_bbox());
		if (!r.is_intersect(bbox))
			return false;
	}
	if (is_any_point_inside(r))
		return true;
	if (windingnumber(r.get_northeast()) ||
	    windingnumber(r.get_southeast()) ||
	    windingnumber(r.get_northwest()) ||
	    windingnumber(r.get_southwest()))
		return true;
	if (is_intersection(r))
		return true;
	return false;
}

int PolygonHole::windingnumber(const Point & pt) const
{
#if 1
	int wn(m_exterior.windingnumber(pt));
	for (interior_t::const_iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
		wn += ii->windingnumber(pt);
	return wn;
#else
	int wn(m_exterior.windingnumber(pt));
	if (!wn)
		return wn;
	for (interior_t::const_iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
		if (ii->windingnumber(pt))
			return 0;
	return wn;
#endif
}

PolygonHole::ScanLine PolygonHole::scanline(Point::coord_t lat) const
{
	ScanLine sl(m_exterior.scanline(lat));
	for (interior_t::const_iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
		sl = sl.combine(ii->scanline(lat));
	return sl;
}

PolygonHole::inside_t PolygonHole::get_inside(const Point& pt0, const Point& pt1) const
{
	PolygonSimple::InsideHelper state(pt0, pt1);
	state = *this;
	return state.get_result();
}

PolygonHole::borderpoints_t PolygonHole::get_borderpoints(const Point& pt0, const Point& pt1) const
{
	PolygonSimple::InsideHelper state(pt0, pt1);
	state = *this;
	return state.get_borderpoints();
}

int64_t PolygonHole::area2(void) const
{
	int64_t r(m_exterior.area2());
	for (interior_t::const_iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
		r += ii->area2();
	return r;
}

double PolygonHole::get_simple_area_km2_dbl(void) const
{
	double r(m_exterior.get_simple_area_km2_dbl());
	for (interior_t::const_iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
		r += ii->get_simple_area_km2_dbl();
	return r;
}

void PolygonHole::reverse(void)
{
	m_exterior.reverse();
	for (interior_t::iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
		ii->reverse();
}

void PolygonHole::remove_redundant_polypoints(void)
{
	m_exterior.remove_redundant_polypoints();
	for (interior_t::iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
		ii->remove_redundant_polypoints();
}

void PolygonHole::simplify_outside(const Rect& bbox)
{
	m_exterior.simplify_outside(bbox);
	if (m_exterior.empty()) {
		m_interior.clear();
		return;
	}
	for (interior_t::iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
		ii->simplify_outside(bbox);
}

bool PolygonHole::operator==(const PolygonHole &p) const
{
	unsigned int n(get_nrinterior());
	if (n != p.get_nrinterior())
		return false;
	if (get_exterior() != p.get_exterior())
		return false;
	for (unsigned int i = 0; i < n; ++i)
		if (operator[](i) != p[i])
			return false;
	return true;
}

int PolygonHole::compare(const PolygonHole& x) const
{
	unsigned int n(get_nrinterior()), xn(x.get_nrinterior());
	if (n < xn)
		return -1;
	if (xn < n)
		return 1;
	{
		int c(get_exterior().compare(x.get_exterior()));
		if (c)
			return c;
	}
	for (unsigned int i = 0; i < n; ++i) {
		int c(operator[](i).compare(x[i]));
		if (c)
			return c;
	}
	return 0;
}

void PolygonHole::normalize(void)
{
	bool es(m_exterior.area2() < 0);
	for (interior_t::iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii) {
		bool is(ii->area2() < 0);
		if (is == es)
			ii->reverse();
	}
}

void PolygonHole::normalize_boostgeom(void)
{
	m_exterior.make_counterclockwise();
	for (interior_t::iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
		ii->make_clockwise();
}

void PolygonHole::snap_bits(unsigned int nrbits)
{
	m_exterior.snap_bits(nrbits);
	if (m_exterior.empty())
		m_interior.clear();
	for (interior_t::iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
		ii->snap_bits(nrbits);
}

void PolygonHole::randomize_bits(unsigned int nrbits)
{
	m_exterior.randomize_bits(nrbits);
	if (m_exterior.empty())
		m_interior.clear();
	for (interior_t::iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
		ii->randomize_bits(nrbits);
}

unsigned int PolygonHole::zap_holes(uint64_t abslim, double rellim)
{
	if (m_interior.empty())
		return 0;
	if (rellim > 0) {
		uint64_t a(Point::round<int64_t,double>(std::abs(m_exterior.area2()) * rellim));
		abslim = std::max(abslim, a);
	}
	unsigned int nrerase(0);
	for (interior_t::iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie;) {
		if (std::abs(ii->area2()) >= abslim) {
			++ii;
			continue;
		}
		ii = m_interior.erase(ii);
		ie = m_interior.end();
		++nrerase;
	}
	if (true)
		std::cerr << "PolygonHole::zap_holes: removed " << nrerase << " holes, area limit " << abslim << std::endl;
	return nrerase;
}

std::ostream& PolygonHole::print_nrvertices(std::ostream& os, bool prarea) const
{
	m_exterior.print_nrvertices(os, prarea);
	if (m_interior.empty())
		return os;
	os << " (";
	bool subseq(false);
	for (interior_t::const_iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii) {
		if (subseq)
			os << ' ';
		subseq = true;
		ii->print_nrvertices(os, prarea);
	}
	return os << ')';
}

std::ostream& PolygonHole::print(std::ostream& os) const
{
	m_exterior.print(os);
	if (m_interior.empty())
		return os;
	os << " [";
	bool subseq(false);
	for (interior_t::const_iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii) {
		if (subseq)
			os << ", ";
		subseq = true;
		ii->print(os);
	}
	return os << ']';
}

void PolygonHole::make_fat_lines_helper(MultiPolygonHole& ph, float radius, bool roundcaps, float angleincr, bool rawcoord) const
{
	if (!false) {
		m_exterior.make_fat_lines_helper(ph, radius, roundcaps, angleincr, rawcoord);
		for (interior_t::const_iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
			ii->make_fat_lines_helper(ph, radius, roundcaps, angleincr, rawcoord);
	} else {
		{
			MultiPolygonHole ph2(m_exterior.make_fat_lines(radius, roundcaps, angleincr, rawcoord));
			ph.insert(ph.end(), ph2.begin(), ph2.end());
		}
		for (interior_t::const_iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii) {
			MultiPolygonHole ph2(ii->make_fat_lines(radius, roundcaps, angleincr, rawcoord));
			ph.insert(ph.end(), ph2.begin(), ph2.end());
		}
	}
}

MultiPolygonHole PolygonHole::make_fat_lines(float radius, bool roundcaps , float angleincr, bool rawcoord) const
{
	MultiPolygonHole ph;
	make_fat_lines_helper(ph, radius, roundcaps, angleincr, rawcoord);
	ph.geos_union(MultiPolygonHole());
	return ph;
}

unsigned int PolygonHole::bloblength(const PolygonSimple& p)
{
	return p.size() * 8 + 4;
}

uint8_t * PolygonHole::blobencode(const PolygonSimple& p, uint8_t * data)
{
	unsigned int sz(p.size());
	data[0] = sz;
	data[1] = sz >> 8;
	data[2] = sz >> 16;
	data[3] = sz >> 24;
	data += 4;
	for (unsigned int i = 0; i < sz; ++i) {
		Point pt(p[i]);
		data[0] = pt.get_lon() >> 0;
		data[1] = pt.get_lon() >> 8;
		data[2] = pt.get_lon() >> 16;
		data[3] = pt.get_lon() >> 24;
		data[4] = pt.get_lat() >> 0;
		data[5] = pt.get_lat() >> 8;
		data[6] = pt.get_lat() >> 16;
		data[7] = pt.get_lat() >> 24;
		data += 8;
	}
	return data;
}

uint8_t const *PolygonHole::blobdecode(PolygonSimple& p, uint8_t const *data, uint8_t const *end)
{
	p.clear();
	if (data + 4 > end)
		return end;
	uint32_t sz(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
	data += 4;
	if (sz >= 0x01000000 || data + 8 * sz > end)
		return end;
	p.reserve(sz);
	for (unsigned int i = 0; i < sz; ++i) {
		uint32_t lon, lat;
		lon = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
		lat = data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24);
		data += 8;
		p.push_back(Point(lon, lat));
	}
	return data;
}

void PolygonHole::getblob(sqlite3x::sqlite3_cursor & cursor, int index)
{
	clear();
	if (cursor.isnull(index))
		return;
	int sz;
	uint8_t const *data((uint8_t const *)cursor.getblob(index, sz));
	uint8_t const *end(data + sz);
	data = blobdecode(m_exterior, data, end);
	while (data < end) {
		m_interior.push_back(PolygonSimple());
		data = blobdecode(m_interior.back(), data, end);
	}
}

void PolygonHole::bindblob(sqlite3x::sqlite3_command & cmd, int index) const
{
	unsigned int sz(bloblength(m_exterior));
	for (interior_t::const_iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
		sz += bloblength(*ii);
	if (sz <= 4) {
		cmd.bind(index);
		return;
	}
	if (sz <= 262144) {
		uint8_t data[sz];
		uint8_t *d = blobencode(m_exterior, data);
		for (interior_t::const_iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
			d = blobencode(*ii, d);
		cmd.bind(index, data, sz);
		return;
	}
	uint8_t *data(new uint8_t[sz]);
	uint8_t *d = blobencode(m_exterior, data);
	for (interior_t::const_iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
		d = blobencode(*ii, d);
	cmd.bind(index, data, sz);
	delete[] data;
}

#ifdef HAVE_PQXX

pqxx::binarystring PolygonHole::bindblob(void) const
{
	unsigned int sz(bloblength(m_exterior));
	for (interior_t::const_iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
		sz += bloblength(*ii);
	if (sz <= 4)
		return pqxx::binarystring(0, 0);
	if (sz <= 262144) {
		uint8_t data[sz];
		uint8_t *d = blobencode(m_exterior, data);
		for (interior_t::const_iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
			d = blobencode(*ii, d);
		return pqxx::binarystring(data, sz);
	}
	uint8_t *data(new uint8_t[sz]);
	uint8_t *d = blobencode(m_exterior, data);
	for (interior_t::const_iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
		d = blobencode(*ii, d);
	pqxx::binarystring blob(data, sz);
	delete[] data;
	return blob;
}

void PolygonHole::getblob(const pqxx::binarystring& blob)
{
	clear();
	uint8_t const *data((uint8_t const *)blob.data());
	uint8_t const *end(data + blob.size());
	if (data == end)
		return;
	data = blobdecode(m_exterior, data, end);
	while (data < end) {
		m_interior.push_back(PolygonSimple());
		data = blobdecode(m_interior.back(), data, end);
	}
}

#endif

unsigned int PolygonHole::get_wkbsize(void) const
{
	unsigned int sz(Geometry::get_wkbhdrsize() + 4 + m_exterior.get_wkbpolysize());
	for (interior_t::const_iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
		sz += ii->get_wkbpolysize();
	return sz;
}

uint8_t *PolygonHole::wkbencode(uint8_t *cp, uint8_t *ep) const
{
	cp = Geometry::wkbencode(cp, ep, (uint32_t)(1 + m_interior.size()));
	cp = m_exterior.wkbencode(cp, ep);
	for (interior_t::const_iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
		cp = ii->wkbencode(cp, ep);
	return cp;
}

const uint8_t *PolygonHole::wkbdecode(const uint8_t *cp, const uint8_t *ep, uint8_t byteorder)
{
	uint32_t num(0);
	cp = Geometry::wkbdecode(cp, ep, num, byteorder);
	if (num) {
		cp = m_exterior.wkbdecode(cp, ep, byteorder);
		--num;
		m_interior.reserve(num);
		for (; num; --num) {
			PolygonSimple ps;
			ps.wkbdecode(cp, ep, byteorder);
			m_interior.push_back(ps);
		}
	}
	return cp;
}

uint8_t *PolygonHole::to_wkb(uint8_t *cp, uint8_t *ep) const
{
	cp = Geometry::wkbencode(cp, ep, Geometry::type_polygon);
	return wkbencode(cp, ep);
}

std::vector<uint8_t> PolygonHole::to_wkb(void) const
{
	std::vector<uint8_t> b(0, get_wkbsize());
	to_wkb(&b[0], &b[b.size()]);
	return b;
}

const uint8_t *PolygonHole::from_wkb(const uint8_t *cp, const uint8_t *ep)
{
	m_exterior.clear();
	m_interior.clear();
	uint8_t byteorder(1);
	{
		Geometry::type_t typ(Geometry::type_invalid);
		cp = Geometry::wkbdecode(cp, ep, typ, byteorder);
		if (typ != Geometry::type_polygon)
			throw std::runtime_error("from_wkb: not a polygon: " + to_str(typ));
	}
	return wkbdecode(cp, ep, byteorder);
}

std::ostream& PolygonHole::to_wktpoly(std::ostream& os) const
{
	m_exterior.to_wktpoly(os << '(');
	for (interior_t::const_iterator ii(m_interior.begin()), ie(m_interior.end()); ii != ie; ++ii)
		ii->to_wktpoly(os << ',');
	return os << ')';
}

std::ostream& PolygonHole::to_wkt(std::ostream& os) const
{
	return to_wktpoly(os << "POLYGON");
}

#ifdef HAVE_JSONCPP

Json::Value PolygonHole::to_json(void) const
{
	Json::Value r;
	r["type"] = "Polygon";
	Json::Value& c(r["coordinates"]);
	{
		Json::Value r;
		for (PolygonSimple::const_iterator i(get_exterior().begin()), e(get_exterior().end()); i != e; ++i) {
			Json::Value p;
			p.append(i->get_lon_deg_dbl());
			p.append(i->get_lat_deg_dbl());
			r.append(p);
		}
		c.append(r);
	}
	for (unsigned int ii(0), nn(get_nrinterior()); ii < nn; ++ii) {
		Json::Value r;
		for (PolygonSimple::const_iterator i(operator[](ii).begin()), e(operator[](ii).end()); i != e; ++i) {
			Json::Value p;
			p.append(i->get_lon_deg_dbl());
			p.append(i->get_lat_deg_dbl());
			r.append(p);
		}
		c.append(r);
	}
	return r;
}

void PolygonHole::from_json(const Json::Value& g)
{
	clear();
	if (!g.isMember("type") || !g["type"].isString() || g["type"].asString() != "Polygon")
		throw std::runtime_error("PolygonHole::from_json: invalid type");
	if (!g.isMember("coordinates"))
		throw std::runtime_error("PolygonHole::from_json: no coordinates");
	const Json::Value& c(g["coordinates"]);
	if (!c.isArray() || c.size() < 1)
		throw std::runtime_error("PolygonHole::from_json: invalid coordinates");
	for (Json::ArrayIndex i = 0; i < c.size(); ++i) {
		const Json::Value& cc(c[i]);
		if (!cc.isArray())
			throw std::runtime_error("PolygonHole::from_json: invalid coordinates");
		PolygonSimple ps;
		for (Json::ArrayIndex ii = 0; ii < cc.size(); ++ii) {
			const Json::Value& ccc(cc[ii]);
			if (!ccc.isArray() || ccc.size() < 2 || !ccc[0].isNumeric() || !ccc[1].isNumeric())
				throw std::runtime_error("PolygonHole::from_json: invalid coordinates");
			Point pt;
			pt.set_lon_deg_dbl(ccc[0].asDouble());
			pt.set_lat_deg_dbl(ccc[1].asDouble());
			ps.push_back(pt);
		}
		if (!i)
			set_exterior(ps);
		else
			add_interior(ps);
	}
}

#endif

MultiPolygonHole::MultiPolygonHole()
{
}

MultiPolygonHole::MultiPolygonHole(const PolygonHole& p)
{
	push_back(p);
}

bool MultiPolygonHole::is_on_boundary(const Point& pt) const
{
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		if (i->is_on_boundary(pt))
			return true;
	return false;
}

bool MultiPolygonHole::is_boundary_point(const Point& pt) const
{
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		if (i->is_boundary_point(pt))
			return true;
	return false;
}

bool MultiPolygonHole::is_intersection(const Point& la, const Point& lb) const
{
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		if (i->is_intersection(la, lb))
			return true;
	return false;
}

bool MultiPolygonHole::is_strict_intersection(const Point& la, const Point& lb) const
{
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		if (i->is_strict_intersection(la, lb))
			return true;
	return false;
}

bool MultiPolygonHole::is_intersection(const Rect& r) const
{
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		if (i->is_intersection(r))
			return true;
	return false;
}

bool MultiPolygonHole::is_all_points_inside(const Rect& r) const
{
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		if (!i->is_all_points_inside(r))
			return false;
	return true;
}

bool MultiPolygonHole::is_any_point_inside(const Rect& r) const
{
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		if (i->is_any_point_inside(r))
			return true;
	return false;
}

bool MultiPolygonHole::is_overlap(const Rect& r) const
{
	if (empty())
		return false;
	{
		Rect bbox(get_bbox());
		if (!r.is_intersect(bbox))
			return false;
	}
	if (is_any_point_inside(r))
		return true;
	if (windingnumber(r.get_northeast()) ||
	    windingnumber(r.get_southeast()) ||
	    windingnumber(r.get_northwest()) ||
	    windingnumber(r.get_southwest()))
		return true;
	if (is_intersection(r))
		return true;
	return false;
}

int MultiPolygonHole::windingnumber(const Point& pt) const
{
	int wn(0);
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		wn += i->windingnumber(pt);
	return wn;
}

MultiPolygonHole::ScanLine MultiPolygonHole::scanline(Point::coord_t lat) const
{
	ScanLine sl;
	for (const_iterator i(begin()), e(end()); i != e; ++i)
	       sl = sl.combine(i->scanline(lat));
	return sl;
}

MultiPolygonHole::inside_t MultiPolygonHole::get_inside(const Point& pt0, const Point& pt1) const
{
	PolygonSimple::InsideHelper state(pt0, pt1);
	state = *this;
	return state.get_result();
}

MultiPolygonHole::borderpoints_t MultiPolygonHole::get_borderpoints(const Point& pt0, const Point& pt1) const
{
	PolygonSimple::InsideHelper state(pt0, pt1);
	state = *this;
	return state.get_borderpoints();
}

int64_t MultiPolygonHole::area2(void) const
{
	int64_t a(0);
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		a += i->area2();
	return a;
}

void MultiPolygonHole::remove_redundant_polypoints(void)
{
	for (iterator i(begin()); i != end();) {
		i->remove_redundant_polypoints();
		if (i->get_exterior().size() < 3) {
			i = erase(i);
			continue;
		}
		++i;
	}
}

void MultiPolygonHole::simplify_outside(const Rect& bbox)
{
	for (iterator i(begin()); i != end();) {
		i->simplify_outside(bbox);
		if (i->get_exterior().size() < 3) {
			i = erase(i);
			continue;
		}
		++i;
	}
}

void MultiPolygonHole::snap_bits(unsigned int nrbits)
{
	for (iterator i(begin()); i != end();) {
		i->snap_bits(nrbits);
		if (i->get_exterior().size() < 3) {
			i = erase(i);
			continue;
		}
		++i;
	}
}

void MultiPolygonHole::randomize_bits(unsigned int nrbits)
{
	for (iterator i(begin()); i != end();) {
		i->randomize_bits(nrbits);
		if (i->get_exterior().size() < 3) {
			i = erase(i);
			continue;
		}
		++i;
	}
}

unsigned int MultiPolygonHole::zap_holes(uint64_t abslim, double rellim)
{
	unsigned int r(0);
	for (iterator i(begin()), e(end()); i != e; ++i)
		r += i->zap_holes(abslim, rellim);
	return r;
}

unsigned int MultiPolygonHole::zap_polys(uint64_t abslim, double rellim)
{
	if (empty())
		return 0;
	if (rellim > 0) {
		uint64_t a(Point::round<int64_t,double>(std::abs(area2()) * rellim));
		abslim = std::max(abslim, a);
	}
	unsigned int nrerase(0);
	for (iterator i(begin()), e(end()); i != e;) {
		if (std::abs(i->area2()) >= abslim) {
			++i;
			continue;
		}
		i = erase(i);
		e = end();
		++nrerase;
	}
	if (true)
		std::cerr << "MultiPolygonHole::zap_polys: removed " << nrerase << " polygons, area limit " << abslim << std::endl;
	return nrerase;
}

std::ostream& MultiPolygonHole::print_nrvertices(std::ostream& os, bool prarea) const
{
	os << '{';
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		i->print_nrvertices(os << ' ', prarea);
	return os << " }";
}

std::ostream& MultiPolygonHole::print(std::ostream& os) const
{
	os << '{';
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		i->print(os << ' ');
	return os << " }";
}

Rect MultiPolygonHole::get_bbox(void) const
{
	Rect bbox;
	bbox.set_empty();
	const_iterator i(begin()), e(end());
	if (i == e)
		return bbox;
	bbox = i->get_bbox();
	for (++i; i != e; ++i)
		bbox = bbox.add(i->get_bbox());
	return bbox;
}

bool MultiPolygonHole::operator==(const MultiPolygonHole &p) const
{
	size_type n(size());
	if (n != p.size())
		return false;
	for (size_type i(0); i < n; ++i)
		if (operator[](i) != p[i])
			return false;
	return true;
}

int MultiPolygonHole::compare(const MultiPolygonHole& x) const
{
	size_type n(size()), xn(x.size());
	if (n < xn)
		return -1;
	if (xn < n)
		return 1;
	for (size_type i = 0; i < n; ++i) {
		int c(operator[](i).compare(x[i]));
		if (c)
			return c;
	}
	return 0;
}

void MultiPolygonHole::make_fat_lines_helper(MultiPolygonHole& ph, float radius, bool roundcaps, float angleincr, bool rawcoord) const
{
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		i->make_fat_lines_helper(ph, radius, roundcaps, angleincr, rawcoord);
}

MultiPolygonHole MultiPolygonHole::make_fat_lines(float radius, bool roundcaps , float angleincr, bool rawcoord) const
{
	MultiPolygonHole ph;
	make_fat_lines_helper(ph, radius, roundcaps, angleincr, rawcoord);
	ph.geos_union(MultiPolygonHole());
	return ph;
}

void MultiPolygonHole::getblob(sqlite3x::sqlite3_cursor& cursor, int index)
{
	clear();
	if (cursor.isnull(index))
		return;
	int sz;
	uint8_t const *data((uint8_t const *)cursor.getblob(index, sz));
	uint8_t const *end(data + sz);
	while (data < end) {
		push_back(PolygonHole());
		data = blobdecode(back(), data, end);
	}
}

void MultiPolygonHole::bindblob(sqlite3x::sqlite3_command& cmd, int index) const
{
	unsigned int sz(0);
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		sz += bloblength(*i);
	if (sz <= 4) {
		cmd.bind(index);
		return;
	}
	if (sz <= 262144) {
		uint8_t data[sz];
		uint8_t *d = data;
		for (const_iterator i(begin()), e(end()); i != e; ++i)
			d = blobencode(*i, d);
		cmd.bind(index, data, sz);
		return;
	}
	uint8_t *data(new uint8_t[sz]);
	uint8_t *d = data;
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		d = blobencode(*i, d);
	cmd.bind(index, data, sz);
	delete[] data;
}

#ifdef HAVE_PQXX

pqxx::binarystring MultiPolygonHole::bindblob(void) const
{
	unsigned int sz(0);
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		sz += bloblength(*i);
	if (sz <= 4)
		return pqxx::binarystring(0, 0);
	if (sz <= 262144) {
		uint8_t data[sz];
		uint8_t *d = data;
		for (const_iterator i(begin()), e(end()); i != e; ++i)
			d = blobencode(*i, d);
		return pqxx::binarystring(data, sz);
	}
	uint8_t *data(new uint8_t[sz]);
	uint8_t *d = data;
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		d = blobencode(*i, d);
	pqxx::binarystring blob(data, sz);
	delete[] data;
	return blob;
}

void MultiPolygonHole::getblob(const pqxx::binarystring& blob)
{
	clear();
	uint8_t const *data((uint8_t const *)blob.data());
	uint8_t const *end(data + blob.size());
	while (data < end) {
		push_back(PolygonHole());
		data = blobdecode(back(), data, end);
	}
}

void MultiPolygonHole::from_json(const Json::Value& g)
{
	clear();
	if (!g.isMember("type") || !g["type"].isString() || g["type"].asString() != "Polygon")
		throw std::runtime_error("MultiPolygonHole::from_json: invalid type");
	if (!g.isMember("coordinates"))
		throw std::runtime_error("MultiPolygonHole::from_json: no coordinates");
	const Json::Value& c(g["coordinates"]);
	if (!c.isArray() || c.size() < 1)
		throw std::runtime_error("MultiPolygonHole::from_json: invalid coordinates");
	for (Json::ArrayIndex i = 0; i < c.size(); ++i) {
		const Json::Value& cc(c[i]);
		if (!cc.isArray())
			throw std::runtime_error("MultiPolygonHole::from_json: invalid coordinates");
		PolygonHole ph;
		for (Json::ArrayIndex ii = 0; ii < cc.size(); ++ii) {
			const Json::Value& ccc(cc[ii]);
			if (!ccc.isArray())
				throw std::runtime_error("MultiPolygonHole::from_json: invalid coordinates");
			PolygonSimple ps;
			for (Json::ArrayIndex iii = 0; iii < ccc.size(); ++iii) {
				const Json::Value& cccc(ccc[iii]);
				if (!cccc.isArray() || cccc.size() < 2 || !cccc[0].isNumeric() || !cccc[1].isNumeric())
					throw std::runtime_error("PolygonHole::from_json: invalid coordinates");
				Point pt;
				pt.set_lon_deg_dbl(cccc[0].asDouble());
				pt.set_lat_deg_dbl(cccc[1].asDouble());
				ps.push_back(pt);
			}
			if (!i)
				ph.set_exterior(ps);
			else
				ph.add_interior(ps);
		}
		push_back(ph);
	}
}

#endif

void MultiPolygonHole::normalize(void)
{
	for (iterator i(begin()), e(end()); i != e; ++i)
		i->normalize();
}

void MultiPolygonHole::normalize_boostgeom(void)
{
	for (iterator i(begin()), e(end()); i != e; ++i)
		i->normalize_boostgeom();
}

double MultiPolygonHole::get_simple_area_km2_dbl(void) const
{
	double a(0);
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		a += i->get_simple_area_km2_dbl();
	return a;
}

unsigned int MultiPolygonHole::bloblength(const PolygonHole& p)
{
	unsigned int len(2 + PolygonHole::bloblength(p.get_exterior()));
	for (unsigned int n(p.get_nrinterior()), i(0); i < n; ++i)
		len += PolygonHole::bloblength(p[i]);
	return len;
}

uint8_t *MultiPolygonHole::blobencode(const PolygonHole& p, uint8_t *data)
{
	unsigned int n(p.get_nrinterior());
	data[0] = n;
	data[1] = n >> 8;
	data += 2;
	data = PolygonHole::blobencode(p.get_exterior(), data);
	for (unsigned int i = 0; i < n; ++i)
		data = PolygonHole::blobencode(p[i], data);
	return data;
}

uint8_t const *MultiPolygonHole::blobdecode(PolygonHole& p, uint8_t const *data, uint8_t const *end)
{
	p.clear();
	if (data + 2 > end)
		return end;
	unsigned int n(data[0] | (data[1] << 8));
     	data += 2;
	{
		PolygonSimple ps;
		data = PolygonHole::blobdecode(ps, data, end);
		p.set_exterior(ps);
	}
	for (; n > 0; --n) {
		PolygonSimple ps;
		data = PolygonHole::blobdecode(ps, data, end);
		p.add_interior(ps);
	}
	return data;
}

bool MultiPolygonHole::is_intersect_fat_line(const Point& pa, const Point& pb, float radius) const
{
	if (pa.is_invalid() || pb.is_invalid() || empty())
		return false;
	MultiPolygonHole p1(PolygonHole(pa.make_fat_line(pb, radius, true, 15)));
	p1.geos_intersect(*this);
	return !p1.empty() && p1.area2();
}

bool MultiPolygonHole::is_intersect_circle(const Point& pa, float radius) const
{
	if (pa.is_invalid() || empty())
		return false;
	MultiPolygonHole p1(PolygonHole(pa.make_circle(radius, 15)));
	p1.geos_intersect(*this);
	return !p1.empty() && p1.area2();
}

unsigned int MultiPolygonHole::get_wkbsize(void) const
{
	unsigned int sz(Geometry::get_wkbhdrsize() + 4);
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		sz += i->get_wkbsize();
	return sz;
}

uint8_t *MultiPolygonHole::wkbencode(uint8_t *cp, uint8_t *ep) const
{
	cp = Geometry::wkbencode(cp, ep, (uint32_t)size());
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		cp = i->to_wkb(cp, ep);
	return cp;
}

const uint8_t *MultiPolygonHole::wkbdecode(const uint8_t *cp, const uint8_t *ep, uint8_t byteorder)
{
	uint32_t num(0);
	cp = Geometry::wkbdecode(cp, ep, num, byteorder);
	reserve(num);
	for (; num; --num) {
		PolygonHole ph;
		ph.from_wkb(cp, ep);
		push_back(ph);
	}
	return cp;
}

uint8_t *MultiPolygonHole::to_wkb(uint8_t *cp, uint8_t *ep) const
{
	cp = Geometry::wkbencode(cp, ep, Geometry::type_multipolygon);
	return wkbencode(cp, ep);
}

std::vector<uint8_t> MultiPolygonHole::to_wkb(void) const
{
	std::vector<uint8_t> b(0, get_wkbsize());
	to_wkb(&b[0], &b[b.size()]);
	return b;
}

const uint8_t *MultiPolygonHole::from_wkb(const uint8_t *cp, const uint8_t *ep)
{
	clear();
	uint8_t byteorder(1);
	{
		Geometry::type_t typ(Geometry::type_invalid);
		const uint8_t *cp1(Geometry::wkbdecode(cp, ep, typ, byteorder));
		if (typ == Geometry::type_polygon) {
			PolygonHole ph;
			cp = ph.from_wkb(cp, ep);
			push_back(ph);
			return cp;
		}
		cp = cp1;
		if (typ != Geometry::type_multipolygon)
			throw std::runtime_error("from_wkb: not a multipolygon: " + to_str(typ));
	}
	return wkbdecode(cp, ep, byteorder);
}

std::ostream& MultiPolygonHole::to_wkt(std::ostream& os) const
{
	os << "MULTIPOLYGON(";
	static const char comma[] = ",";
	const char *del(comma + 1);
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		i->to_wktpoly(os << del);
		del = comma;
	}
	return os << ')';
}

#ifdef HAVE_JSONCPP
Json::Value MultiPolygonHole::to_json(void) const
{
	Json::Value r;
	r["type"] = "MultiPolygon";
	Json::Value& c(r["coordinates"]);
	for (const_iterator pi(begin()), pe(end()); pi != pe; ++pi) {
		Json::Value p;
		{
			Json::Value ri;
			for (PolygonSimple::const_iterator i(pi->get_exterior().begin()), e(pi->get_exterior().end()); i != e; ++i) {
				Json::Value p;
				p.append(i->get_lon_deg_dbl());
				p.append(i->get_lat_deg_dbl());
				ri.append(p);
			}
			p.append(ri);
		}
		for (unsigned int ii(0), nn(pi->get_nrinterior()); ii < nn; ++ii) {
			Json::Value ri;
			for (PolygonSimple::const_iterator i(pi->operator[](ii).begin()), e(pi->operator[](ii).end()); i != e; ++i) {
				Json::Value p;
				p.append(i->get_lon_deg_dbl());
				p.append(i->get_lat_deg_dbl());
				ri.append(p);
			}
			p.append(ri);
		}
		c.append(p);
	}
	return r;
}

#endif

const Point3D::alt_t Point3D::invalid_alt = std::numeric_limits<alt_t>::min();

void Point3D::set_invalid(void)
{
	Point::set_invalid();
	unset_alt();
}

std::ostream& Point3D::print(std::ostream& os) const
{
	os << '(' << (std::string)get_lon_str() << ',' << (std::string)get_lat_str()<< ',';
	if (is_alt_valid())
		os << get_alt();
	else
		os << "--";
	os << ')';
	return os;
}

std::ostream& Point3D::to_wktpt(std::ostream& os) const
{
	os << get_lon_deg_dbl() << ' ' << get_lat_deg_dbl();
	if (is_alt_valid())
		os << ' ' << get_alt();
	return os;
}

std::ostream& Point3D::to_wkt(std::ostream& os) const
{
	return to_wktpt(os << "POINT(") << ')';
}

#ifdef HAVE_JSONCPP

Json::Value Point3D::to_json(void) const
{
	Json::Value r;
	r["type"] = "Point";
	Json::Value& c(r["coordinates"]);
	c.append(get_lon_deg_dbl());
	c.append(get_lat_deg_dbl());
	if (is_alt_valid())
		c.append((double)get_alt());
	return r;
}

void Point3D::from_json(const Json::Value& g)
{
	set_invalid();
	if (!g.isMember("type") || !g["type"].isString() || g["type"].asString() != "Point")
		throw std::runtime_error("Point3D::from_json: invalid type");
	if (!g.isMember("coordinates"))
		throw std::runtime_error("Point3D::from_json: no coordinates");
	const Json::Value& c(g["coordinates"]);
	if (!c.isArray() || c.size() < 2 || !c[0].isNumeric() || !c[1].isNumeric())
		throw std::runtime_error("Point3D::from_json: invalid coordinates");
	set_lon_deg_dbl(c[0].asDouble());
	set_lat_deg_dbl(c[1].asDouble());
	if (c.size() >= 3 && c[2].isNumeric())
		set_alt(round<alt_t,double>(c[2].asDouble()));
}

#endif

LineString3D::LineString3D(void)
{
}

LineString3D::LineString3D(const LineString& ls)
{
	insert(end(), ls.begin(), ls.end());
}

Rect LineString3D::get_bbox(void) const
{
	if (!size()) {
		Rect r;
		r.set_empty();
		return r;
	}
	Point::coord_t lat(front().get_lat()), latmin(lat), latmax(lat);
	int64_t lon(front().get_lon()), lonmin(lon), lonmax(lon);
	for (size_type i(1), n(size()); i < n; ++i) {
		Point p(operator[](i) - operator[](i - 1));
		lat += p.get_lat();
		lon += p.get_lon();
		latmin = std::min(latmin, lat);
		latmax = std::max(latmax, lat);
		lonmin = std::min(lonmin, lon);
		lonmax = std::max(lonmax, lon);
	}
	return Rect(Point(lonmin, latmin), Point(lonmax, latmax));
}

LineString3D::operator LineString(void) const
{
	LineString ls;
	ls.reserve(size());
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		ls.push_back(*i);
	return ls;
}

std::ostream& LineString3D::print(std::ostream& os) const
{
	os << '(';
	bool subseq(false);
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		if (subseq)
			os << ", ";
		subseq = true;
		os << i->get_lon_str2() << ' ' << i->get_lat_str2() << ' ';
		if (i->is_alt_valid())
			os << i->get_alt();
		else
			os << "--";
	}
	return os << ')';
}

std::ostream& LineString3D::to_wktlst(std::ostream& os) const
{
	os << '(';
	static const char comma[] = ",";
	const char *del(comma + 1);
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		i->to_wktpt(os << del);
		del = comma;
	}
	return os << ')';
}

std::ostream& LineString3D::to_wkt(std::ostream& os) const
{
	return to_wktlst(os << "LINESTRING");
}

#ifdef HAVE_JSONCPP

Json::Value LineString3D::to_json(void) const
{
	Json::Value r;
	r["type"] = "LineString";
	Json::Value& c(r["coordinates"]);
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		Json::Value p;
		p.append(i->get_lon_deg_dbl());
		p.append(i->get_lat_deg_dbl());
		if (i->is_alt_valid())
			p.append((double)i->get_alt());
		c.append(p);
	}
	return r;
}

void LineString3D::from_json(const Json::Value& g)
{
	clear();
	if (!g.isMember("type") || !g["type"].isString() || g["type"].asString() != "LineString")
		throw std::runtime_error("LineString3D::from_json: invalid type");
	if (!g.isMember("coordinates"))
		throw std::runtime_error("LineString3D::from_json: no coordinates");
	const Json::Value& c(g["coordinates"]);
	if (!c.isArray())
		throw std::runtime_error("LineString3D::from_json: invalid coordinates");
	for (Json::ArrayIndex i = 0; i < c.size(); ++i) {
		const Json::Value& cc(c[i]);
		if (!cc.isArray() || cc.size() < 2 || !cc[0].isNumeric() || !cc[1].isNumeric())
			throw std::runtime_error("LineString3D::from_json: invalid coordinates");
		Point3D pt;
		pt.set_lon_deg_dbl(cc[0].asDouble());
		pt.set_lat_deg_dbl(cc[1].asDouble());
		if (cc.size() >= 3 && cc[2].isNumeric())
			pt.set_alt(Point3D::round<Point3D::alt_t,double>(cc[2].asDouble()));
		push_back(pt);
	}
}

#endif

PolygonSimple3D::PolygonSimple3D(void)
{
}

PolygonSimple3D::PolygonSimple3D(const PolygonSimple& ps)
{
	insert(end(), ps.begin(), ps.end());
}

PolygonSimple3D::reference PolygonSimple3D::operator[](size_type x)
{
	size_type y(size());
	if (x >= y && y > 0)
		x %= y;
	return base_t::operator[](x);
}

PolygonSimple3D::const_reference PolygonSimple3D::operator[](size_type x) const
{
	size_type y(size());
	if (x >= y && y > 0)
		x %= y;
	return base_t::operator[](x);
}

PolygonSimple3D::operator PolygonSimple(void) const
{
	PolygonSimple ps;
	ps.reserve(size());
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		ps.push_back(*i);
	return ps;
}

std::ostream& PolygonSimple3D::to_wktpoly(std::ostream& os) const
{
	os << '(';
	static const char comma[] = ",";
	const char *del(comma + 1);
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		i->to_wktpt(os << del);
		del = comma;
	}
	if (!empty()) {
		front().to_wktpt(os << del);
		del = comma;
	}
	return os << ')';
}

std::ostream& PolygonSimple3D::to_wkt(std::ostream& os) const
{
	return to_wktpoly(os << "POLYGON(") << ')';
}

#ifdef HAVE_JSONCPP

Json::Value PolygonSimple3D::to_json(void) const
{
	Json::Value r;
	r["type"] = "Polygon";
	Json::Value& c(r["coordinates"]);
	Json::Value ri;
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		Json::Value p;
		p.append(i->get_lon_deg_dbl());
		p.append(i->get_lat_deg_dbl());
		if (i->is_alt_valid())
			p.append((double)i->get_alt());
		ri.append(p);
	}
	c.append(ri);
	return r;
}

void PolygonSimple3D::from_json(const Json::Value& g)
{
	clear();
	if (!g.isMember("type") || !g["type"].isString() || g["type"].asString() != "Polygon")
		throw std::runtime_error("PolygonSimple::from_json: invalid type");
	if (!g.isMember("coordinates"))
		throw std::runtime_error("PolygonSimple::from_json: no coordinates");
	const Json::Value& c(g["coordinates"]);
	if (!c.isArray() || c.size() != 1)
		throw std::runtime_error("PolygonSimple::from_json: invalid coordinates");
	{
		const Json::Value& cc(c[0]);
		if (!cc.isArray())
			throw std::runtime_error("PolygonSimple::from_json: invalid coordinates");
		for (Json::ArrayIndex ii = 0; ii < cc.size(); ++ii) {
			const Json::Value& ccc(cc[ii]);
			if (!ccc.isArray() || ccc.size() < 2 || !ccc[0].isNumeric() || !ccc[1].isNumeric())
				throw std::runtime_error("PolygonSimple::from_json: invalid coordinates");
			Point3D pt;
			pt.set_lon_deg_dbl(ccc[0].asDouble());
			pt.set_lat_deg_dbl(ccc[1].asDouble());
			if (ccc.size() >= 3 && ccc[2].isNumeric())
				pt.set_alt(Point3D::round<Point3D::alt_t,double>(ccc[2].asDouble()));
			push_back(pt);
		}
	}
}

#endif

const IntervalBoundingBox::int_t::type_t IntervalBoundingBox::lmin(std::numeric_limits<Point::coord_t>::min());
const IntervalBoundingBox::int_t::type_t IntervalBoundingBox::lmax(std::numeric_limits<Point::coord_t>::max() + lone);
const IntervalBoundingBox::int_t::type_t IntervalBoundingBox::lone;
const IntervalBoundingBox::int_t::type_t IntervalBoundingBox::ltotal;

IntervalBoundingBox::IntervalBoundingBox(void)
	: m_lonset(int_t::Intvl(lmin, lmax)),
	  m_latmin(std::numeric_limits<Point::coord_t>::max()),
	  m_latmax(std::numeric_limits<Point::coord_t>::min())
{
}

IntervalBoundingBox::operator Rect(void) const
{
	Rect r;
	r.set_empty();
	if (m_lonset.is_empty())
		return r;
	int_t ls(m_lonset);
	ls |= int_t::Intvl(ls.begin()->get_lower() + ltotal, ls.begin()->get_upper() + ltotal);
	int_t::type_t best(0);
	for (int_t::const_iterator i(ls.begin()), e(ls.end()); i != e; ++i) {
		int_t::type_t w(i->get_upper() - i->get_lower());
		if (w < best)
			continue;
		r.set_west(i->get_lower() + 1);
		r.set_east(r.get_west() + w);
		best = w;
	}
	r.set_north(m_latmax);
	r.set_south(m_latmin);
	return r;
}

void IntervalBoundingBox::add(const Rect& r)
{
	if (r.is_empty())
		return;
	m_latmin = std::min(m_latmin, r.get_south());
	m_latmax = std::max(m_latmax, r.get_north());
	if (r.get_east() < r.get_west()) {
		m_lonset -= int_t::Intvl(r.get_west(), lmax);
		m_lonset -= int_t::Intvl(lmin, r.get_east() + lone);
	} else {
		m_lonset -= int_t::Intvl(r.get_west(), r.get_east() + lone);
	}
}

void IntervalBoundingBox::add(const Point& p)
{
	m_latmin = std::min(m_latmin, p.get_lat());
	m_latmax = std::max(m_latmax, p.get_lat());
	m_lonset -= int_t::Intvl(p.get_lon(), p.get_lon() + lone);
}

void IntervalBoundingBox::add(const Point& p0, const Point& p1)
{
	m_latmin = std::min(m_latmin, p0.get_lat());
	m_latmax = std::max(m_latmax, p0.get_lat());
	m_latmin = std::min(m_latmin, p1.get_lat());
	m_latmax = std::max(m_latmax, p1.get_lat());
	Point::coord_t lon0(p0.get_lon());
	Point::coord_t lon1(p1.get_lon());
	{
		Point::coord_t londiff(lon1 - lon0);
		if (londiff < 0)
			std::swap(lon0, lon1);
	}
	if (lon1 < lon0) {
		m_lonset -= int_t::Intvl(lon0, lmax);
		m_lonset -= int_t::Intvl(lmin, lon1 + lone);
	} else {
		m_lonset -= int_t::Intvl(lon0, lon1 + lone);
	}
}

void IntervalBoundingBox::add(const PolygonSimple& p)
{
	PolygonSimple::size_type n(p.size());
	if (!n)
		return;
	for (PolygonSimple::size_type j(n-1), i(0); i < n; j = i, ++i)
		add(p[j], p[i]);
}

void IntervalBoundingBox::add(const PolygonHole& p)
{
	add(p.get_exterior());
}

void IntervalBoundingBox::add(const MultiPolygonHole& p)
{
	for (MultiPolygonHole::const_iterator i(p.begin()), e(p.end()); i != e; ++i)
		add(*i);
}

template <typename T> class Geometry::Var : public Geometry {
public:
	Var(const T& x);
	virtual type_t get_type(void) const { return type; }
	virtual operator const T& (void) const { return m_payload; }

	virtual Rect get_bbox(void) const;
	virtual int64_t area2(void) const;
	virtual Geometry::ptr_t flatten(void) const;

	virtual unsigned int get_wkbsize(void) const;
	virtual uint8_t *wkbencode(uint8_t *cp, uint8_t *ep) const;
	virtual const uint8_t *wkbdecode(const uint8_t *cp, const uint8_t *ep, uint8_t byteorder);
	virtual uint8_t *to_wkb(uint8_t *cp, uint8_t *ep) const;
	virtual const uint8_t *from_wkb(const uint8_t *cp, const uint8_t *ep);
	virtual std::ostream& to_wkt(std::ostream& os) const;

#ifdef HAVE_JSONCPP
	virtual Json::Value to_json(void) const;
#endif

protected:
	static const type_t type;
	T m_payload;
};

template <typename T> Geometry::Var<T>::Var(const T& x)
	: Geometry(), m_payload(x)
{
}

template <typename T> Rect Geometry::Var<T>::get_bbox(void) const
{
	return m_payload.get_bbox();
}

template <typename T> int64_t Geometry::Var<T>::area2(void) const
{
	return m_payload.area2();
}

template <> int64_t Geometry::Var<Point>::area2(void) const
{
	return 0;
}

template <> int64_t Geometry::Var<LineString>::area2(void) const
{
	return 0;
}

template <> int64_t Geometry::Var<MultiPoint>::area2(void) const
{
	return 0;
}

template <> int64_t Geometry::Var<MultiLineString>::area2(void) const
{
	return 0;
}

template <typename T> Geometry::ptr_t Geometry::Var<T>::flatten(void) const
{
	return ptr_t();
}

template <> Geometry::ptr_t Geometry::Var<GeometryCollection>::flatten(void) const
{
	return m_payload.flatten();
}

template <typename T> unsigned int Geometry::Var<T>::get_wkbsize(void) const
{
	return m_payload.get_wkbsize();
}

template <typename T> uint8_t *Geometry::Var<T>::wkbencode(uint8_t *cp, uint8_t *ep) const
{
	return m_payload.wkbencode(cp, ep);
}

template <typename T> const uint8_t *Geometry::Var<T>::wkbdecode(const uint8_t *cp, const uint8_t *ep, uint8_t byteorder)
{
	return m_payload.wkbdecode(cp, ep, byteorder);
}

template <typename T> uint8_t *Geometry::Var<T>::to_wkb(uint8_t *cp, uint8_t *ep) const
{
	return m_payload.to_wkb(cp, ep);
}

template <typename T> const uint8_t *Geometry::Var<T>::from_wkb(const uint8_t *cp, const uint8_t *ep)
{
	return m_payload.from_wkb(cp, ep);
}

template <typename T> std::ostream& Geometry::Var<T>::to_wkt(std::ostream& os) const
{
	return m_payload.to_wkt(os);
}

#ifdef HAVE_JSONCPP

template <typename T> Json::Value Geometry::Var<T>::to_json(void) const
{
	return m_payload.to_json();
}

#endif

template <> const Geometry::type_t Geometry::Var<Point>::type = type_point;
template <> const Geometry::type_t Geometry::Var<LineString>::type = type_linestring;
template <> const Geometry::type_t Geometry::Var<PolygonHole>::type = type_polygon;
template <> const Geometry::type_t Geometry::Var<MultiPoint>::type = type_multipoint;
template <> const Geometry::type_t Geometry::Var<MultiLineString>::type = type_multilinestring;
template <> const Geometry::type_t Geometry::Var<MultiPolygonHole>::type = type_multipolygon;
template <> const Geometry::type_t Geometry::Var<GeometryCollection>::type = type_geometrycollection;

template class Geometry::Var<Point>;
template class Geometry::Var<LineString>;
template class Geometry::Var<PolygonHole>;
template class Geometry::Var<MultiPoint>;
template class Geometry::Var<MultiLineString>;
template class Geometry::Var<MultiPolygonHole>;
template class Geometry::Var<GeometryCollection>;

const std::string& to_str(Geometry::type_t t)
{
	switch (t) {
	case Geometry::type_point:
	{
		static const std::string r("POINT");
		return r;
	}

	case Geometry::type_linestring:
	{
		static const std::string r("LINESTRING");
		return r;
	}

	case Geometry::type_polygon:
	{
		static const std::string r("POLYGON");
		return r;
	}

	case Geometry::type_multipoint:
	{
		static const std::string r("MULTIPOINT");
		return r;
	}

	case Geometry::type_multilinestring:
	{
		static const std::string r("MULTILINESTRING");
		return r;
	}

	case Geometry::type_multipolygon:
	{
		static const std::string r("MULTIPOLYGON");
		return r;
	}

	case Geometry::type_geometrycollection:
	{
		static const std::string r("GEOMETRYCOLLECTION");
		return r;
	}

	default:
	{
		static const std::string r("INVALID");
		return r;
	}
	}
}

Geometry::Geometry(void)
	: m_refcount(1)
{
}

Geometry::~Geometry()
{
}

void Geometry::reference(void) const
{
	g_atomic_int_inc(&m_refcount);
}

void Geometry::unreference(void) const
{
	if (!g_atomic_int_dec_and_test(&m_refcount))
		return;
	delete this;
}

Glib::RefPtr<Geometry> Geometry::create(const Point& x)
{
	return Glib::RefPtr<Geometry>(new Var<Point>(x));
}

Glib::RefPtr<Geometry> Geometry::create(const LineString& x)
{
	return Glib::RefPtr<Geometry>(new Var<LineString>(x));
}

Glib::RefPtr<Geometry> Geometry::create(const PolygonSimple& x)
{
	PolygonHole ph;
	ph.set_exterior(x);
	return Glib::RefPtr<Geometry>(new Var<PolygonHole>(ph));
}

Glib::RefPtr<Geometry> Geometry::create(const PolygonHole& x)
{
	return Glib::RefPtr<Geometry>(new Var<PolygonHole>(x));
}

Glib::RefPtr<Geometry> Geometry::create(const MultiPoint& x)
{
	return Glib::RefPtr<Geometry>(new Var<MultiPoint>(x));
}

Glib::RefPtr<Geometry> Geometry::create(const MultiLineString& x)
{
	return Glib::RefPtr<Geometry>(new Var<MultiLineString>(x));
}

Glib::RefPtr<Geometry> Geometry::create(const MultiPolygonHole& x)
{
	return Glib::RefPtr<Geometry>(new Var<MultiPolygonHole>(x));
}

Glib::RefPtr<Geometry> Geometry::create(const GeometryCollection& x)
{
	return Glib::RefPtr<Geometry>(new Var<GeometryCollection>(x));
}

#ifdef HAVE_JSONCPP

Glib::RefPtr<Geometry> Geometry::create(const Json::Value& g)
{
	if (!g.isMember("type") || !g["type"].isString())
		throw std::runtime_error("GeometryCollection::from_json: invalid type");
	std::string t(g["type"].asString());
	if (t == "Point") {
		Point x;
		x.from_json(g);
		return create(x);
	}
	if (t == "LineString") {
		LineString x;
		x.from_json(g);
		return create(x);
	}
	if (t == "Polygon") {
		PolygonHole x;
		x.from_json(g);
		return create(x);
	}
	if (t == "MultiPoint") {
		MultiPoint x;
		x.from_json(g);
		return create(x);
	}
	if (t == "MultiLineString") {
		MultiLineString x;
		x.from_json(g);
		return create(x);
	}
	if (t == "MultiPolygon") {
		MultiPolygonHole x;
		x.from_json(g);
		return create(x);
	}
	if (t == "GeometryCollection") {
		GeometryCollection x;
		x.from_json(g);
		return create(x);
	}
	throw std::runtime_error("GeometryCollection::from_json: invalid type " + t);
}

Json::Value Geometry::to_json(void) const
{
	return Json::Value();
}

#endif

Glib::RefPtr<Geometry> Geometry::create_from_wkb(const uint8_t * &cpout, const uint8_t *cp, const uint8_t *ep)
{
	if (cp + 1 > ep) {
		cpout = cp;
		return Glib::RefPtr<Geometry>();
	}
	return create_from_wkb(cpout, cp + 1, ep, *cp);
}

Glib::RefPtr<Geometry> Geometry::create_from_wkb(const uint8_t * &cpout, const uint8_t *cp, const uint8_t *ep, uint8_t byteorder)
{
	if (cp + 4 > ep) {
		cpout = cp;
		return Glib::RefPtr<Geometry>();
	}
	type_t typ = type_invalid;
	{
		uint32_t x(*cp++);
		x |= ((uint32_t)(*cp++)) << 8;
		x |= ((uint32_t)(*cp++)) << 16;
		x |= ((uint32_t)(*cp++)) << 24;
		if (x <= type_last)
			typ = (type_t)x;
	}
       	return create_from_wkb(cpout, cp, ep, typ, byteorder);
}

Glib::RefPtr<Geometry> Geometry::create_from_wkb(const uint8_t * &cpout, const uint8_t *cp, const uint8_t *ep, type_t typ, uint8_t byteorder)
{
	switch (typ) {
	case type_point:
	{
		Point x;
		cp = x.wkbdecode(cp, ep, byteorder);
		cpout = cp;
		return create(x);
	}

	case type_linestring:
	{
		LineString x;
		cp = x.wkbdecode(cp, ep, byteorder);
		cpout = cp;
		return create(x);
	}

	case type_polygon:
	{
		PolygonHole x;
		cp = x.wkbdecode(cp, ep, byteorder);
		cpout = cp;
		return create(x);
	}

	case type_multipoint:
	{
		MultiPoint x;
		cp = x.wkbdecode(cp, ep, byteorder);
		cpout = cp;
		return create(x);
	}

	case type_multilinestring:
	{
		MultiLineString x;
		cp = x.wkbdecode(cp, ep, byteorder);
		cpout = cp;
		return create(x);
	}

	case type_multipolygon:
	{
		MultiPolygonHole x;
		cp = x.wkbdecode(cp, ep, byteorder);
		cpout = cp;
		return create(x);
	}

	case type_geometrycollection:
	{
		GeometryCollection x;
		cp = x.wkbdecode(cp, ep, byteorder);
		cpout = cp;
		return create(x);
	}

	default:
		cpout = cp;
		return Glib::RefPtr<Geometry>();
	}
}

Geometry::operator const Point& (void) const
{
	throw std::runtime_error("Geometry not Point");
}

Geometry::operator const LineString& (void) const
{
	throw std::runtime_error("Geometry not LineString");
}

Geometry::operator const PolygonHole& (void) const
{
	throw std::runtime_error("Geometry not PolygonHole");
}

Geometry::operator const MultiPoint& (void) const
{
	throw std::runtime_error("Geometry not MultiPoint");
}

Geometry::operator const MultiLineString& (void) const
{
	throw std::runtime_error("Geometry not MultiLineString");
}

Geometry::operator const MultiPolygonHole& (void) const
{
	throw std::runtime_error("Geometry not MultiPolygonHole");
}

Geometry::operator const GeometryCollection& (void) const
{
	throw std::runtime_error("Geometry not GeometryCollection");
}

uint8_t *Geometry::wkbencode(uint8_t *cp, uint8_t *ep, type_t typ)
{
	if (cp + 5 > ep)
		throw std::runtime_error("wkbencode: overflow");
	*cp++ = 1;
	*cp++ = typ;
	*cp++ = typ >> 8;
	*cp++ = typ >> 16;
	*cp++ = typ >> 24;
	return cp;
}

uint8_t *Geometry::wkbencode(uint8_t *cp, uint8_t *ep, uint8_t x)
{
	if (cp >= ep)
		throw std::runtime_error("wkbencode: overflow");
	*cp++ = x;
	return cp;
}

uint8_t *Geometry::wkbencode(uint8_t *cp, uint8_t *ep, uint32_t x)
{
	if (cp + 4 > ep)
		throw std::runtime_error("wkbencode: overflow");
	*cp++ = x;
	*cp++ = x >> 8;
	*cp++ = x >> 16;
	*cp++ = x >> 24;
	return cp;
}

uint8_t *Geometry::wkbencode(uint8_t *cp, uint8_t *ep, double x)
{
	if (cp + 8 > ep)
		throw std::runtime_error("wkbencode: overflow");
	union {
		double d;
		uint8_t b[8];
	} u;
	u.d = x;
	if (__BYTE_ORDER == __LITTLE_ENDIAN) {
		*cp++ = u.b[0];
		*cp++ = u.b[1];
		*cp++ = u.b[2];
		*cp++ = u.b[3];
		*cp++ = u.b[4];
		*cp++ = u.b[5];
		*cp++ = u.b[6];
		*cp++ = u.b[7];
	} else {
		*cp++ = u.b[7];
		*cp++ = u.b[6];
		*cp++ = u.b[5];
		*cp++ = u.b[4];
		*cp++ = u.b[3];
		*cp++ = u.b[2];
		*cp++ = u.b[1];
		*cp++ = u.b[0];
	}
	return cp;
}

const uint8_t *Geometry::wkbdecode(const uint8_t *cp, const uint8_t *ep, type_t& typ, uint8_t& byteorder)
{
	byteorder = 1;
	typ = type_invalid;
	if (cp + 5 > ep)
		return cp;
	byteorder = *cp++;
	uint32_t x(*cp++);
	x |= ((uint32_t)(*cp++)) << 8;
	x |= ((uint32_t)(*cp++)) << 16;
	x |= ((uint32_t)(*cp++)) << 24;
	if (x <= type_last)
		typ = (type_t)x;
	return cp;
}

const uint8_t *Geometry::wkbdecode(const uint8_t *cp, const uint8_t *ep, uint8_t& x, uint8_t byteorder)
{
	x = 0;
	if (cp >= ep)
		throw std::runtime_error("wkbdecode: underflow");
	x = *cp++;
	return cp;
}

const uint8_t *Geometry::wkbdecode(const uint8_t *cp, const uint8_t *ep, uint32_t& x, uint8_t byteorder)
{
	x = 0;
	if (cp + 4 > ep)
		throw std::runtime_error("wkbdecode: underflow");
	if ((!byteorder && __BYTE_ORDER != __LITTLE_ENDIAN) ||
	    (byteorder && __BYTE_ORDER == __LITTLE_ENDIAN)) {
		uint32_t xx(*cp++);
		xx |= ((uint32_t)(*cp++)) << 8;
		xx |= ((uint32_t)(*cp++)) << 16;
		xx |= ((uint32_t)(*cp++)) << 24;
		x = xx;
	} else {
		uint32_t xx(((uint32_t)(*cp++)) << 24);
		xx |= ((uint32_t)(*cp++)) << 16;
		xx |= ((uint32_t)(*cp++)) << 8;
		xx |= *cp++;
		x = xx;
	}
	return cp;
}

const uint8_t *Geometry::wkbdecode(const uint8_t *cp, const uint8_t *ep, double& x, uint8_t byteorder)
{
	x = std::numeric_limits<double>::quiet_NaN();
	if (cp + 8 > ep)
		throw std::runtime_error("wkbdecode: underflow");
	union {
		double d;
		uint8_t b[8];
	} u;
	if ((!byteorder && __BYTE_ORDER != __LITTLE_ENDIAN) ||
	    (byteorder && __BYTE_ORDER == __LITTLE_ENDIAN)) {
		u.b[0] = *cp++;
		u.b[1] = *cp++;
		u.b[2] = *cp++;
		u.b[3] = *cp++;
		u.b[4] = *cp++;
		u.b[5] = *cp++;
		u.b[6] = *cp++;
		u.b[7] = *cp++;
	} else {
		u.b[7] = *cp++;
		u.b[6] = *cp++;
		u.b[5] = *cp++;
		u.b[4] = *cp++;
		u.b[3] = *cp++;
		u.b[2] = *cp++;
		u.b[1] = *cp++;
		u.b[0] = *cp++;
	}
	x = u.d;
	return cp;
}

GeometryCollection::GeometryCollection(void)
{
}

GeometryCollection::~GeometryCollection()
{
}

GeometryCollection::GeometryCollection(const GeometryCollection& g)
	: m_el(g.m_el)
{
}

GeometryCollection::GeometryCollection(const Geometry::ptr_t& p)
{
	if (!p)
		return;
	m_el.push_back(p);
}

GeometryCollection::GeometryCollection(const Point& pt)
{
	m_el.push_back(Geometry::create(pt));
}

GeometryCollection::GeometryCollection(const LineString& ls)
{
	m_el.push_back(Geometry::create(ls));
}

GeometryCollection::GeometryCollection(const PolygonSimple& ps)
{
	m_el.push_back(Geometry::create(ps));
}

GeometryCollection::GeometryCollection(const PolygonHole& ph)
{
	m_el.push_back(Geometry::create(ph));
}

GeometryCollection::GeometryCollection(const MultiPoint& mpt)
{
	m_el.push_back(Geometry::create(mpt));
}

GeometryCollection::GeometryCollection(const MultiLineString& mls)
{
	m_el.push_back(Geometry::create(mls));
}

GeometryCollection::GeometryCollection(const MultiPolygonHole& mph)
{
	m_el.push_back(Geometry::create(mph));
}

GeometryCollection& GeometryCollection::operator=(const Geometry::ptr_t& p)
{
	m_el.clear();
	if (!p)
		return *this;
	m_el.push_back(p);
	return *this;
}

GeometryCollection& GeometryCollection::operator=(const GeometryCollection& g)
{
	m_el = g.m_el;
	return *this;
}

GeometryCollection& GeometryCollection::operator=(const Point& pt)
{
	m_el.clear();
	m_el.push_back(Geometry::create(pt));
	return *this;
}

GeometryCollection& GeometryCollection::operator=(const LineString& ls)
{
	m_el.clear();
	m_el.push_back(Geometry::create(ls));
	return *this;
}

GeometryCollection& GeometryCollection::operator=(const PolygonSimple& ps)
{
	m_el.clear();
	m_el.push_back(Geometry::create(ps));
	return *this;
}

GeometryCollection& GeometryCollection::operator=(const PolygonHole& ph)
{
	m_el.clear();
	m_el.push_back(Geometry::create(ph));
	return *this;
}

GeometryCollection& GeometryCollection::operator=(const MultiPoint& mpt)
{
	m_el.clear();
	m_el.push_back(Geometry::create(mpt));
	return *this;
}

GeometryCollection& GeometryCollection::operator=(const MultiLineString& mls)
{
	m_el.clear();
	m_el.push_back(Geometry::create(mls));
	return *this;
}

GeometryCollection& GeometryCollection::operator=(const MultiPolygonHole& mph)
{
	m_el.clear();
	m_el.push_back(Geometry::create(mph));
	return *this;
}

GeometryCollection& GeometryCollection::append(const Geometry::ptr_t& p)
{
	if (!p)
		return *this;
	m_el.push_back(p);
	return *this;
}

GeometryCollection& GeometryCollection::append(const GeometryCollection& g)
{
	m_el.insert(m_el.end(), g.m_el.begin(), g.m_el.end());
	return *this;
}

GeometryCollection& GeometryCollection::append(const Point& pt)
{
	m_el.push_back(Geometry::create(pt));
	return *this;
}

GeometryCollection& GeometryCollection::append(const LineString& ls)
{
	m_el.push_back(Geometry::create(ls));
	return *this;
}

GeometryCollection& GeometryCollection::append(const PolygonSimple& ps)
{
	m_el.push_back(Geometry::create(ps));
	return *this;
}

GeometryCollection& GeometryCollection::append(const PolygonHole& ph)
{
	m_el.push_back(Geometry::create(ph));
	return *this;
}

GeometryCollection& GeometryCollection::append(const MultiPoint& pt)
{
	m_el.push_back(Geometry::create(pt));
	return *this;
}

GeometryCollection& GeometryCollection::append(const MultiLineString& ls)
{
	m_el.push_back(Geometry::create(ls));
	return *this;
}

GeometryCollection& GeometryCollection::append(const MultiPolygonHole& ph)
{
	m_el.push_back(Geometry::create(ph));
	return *this;
}

const Geometry::ptr_t& GeometryCollection::operator[](std::size_t i) const
{
	static const Geometry::ptr_t empty;
	if (i >= size())
		return empty;
	return m_el[i];
}

Rect GeometryCollection::get_bbox(void) const
{
	Rect bbox;
	bbox.set_empty();
	for (el_t::const_iterator i(m_el.begin()), e(m_el.end()); i != e; ++i) {
		if (!*i)
			continue;
		bbox = bbox.add((*i)->get_bbox());
	}
	return bbox;
}

int64_t GeometryCollection::area2(void) const
{
	int64_t area(0);
	for (el_t::const_iterator i(m_el.begin()), e(m_el.end()); i != e; ++i) {
		if (!*i)
			continue;
		area += (*i)->area2();
	}
	return area;
}

Geometry::ptr_t GeometryCollection::flatten(void) const
{
	GeometryCollection gn;
	bool chg(false);
	for (el_t::const_iterator i(m_el.begin()), e(m_el.end()); i != e; ++i) {
		if (!*i) {
			chg = true;
			continue;
		}
		Geometry::ptr_t p(*i);
		{
			Geometry::ptr_t p1(p->flatten());
			if (p1) {
				p.swap(p1);
				chg = true;
			}
		}
		if (p->get_type() != Geometry::type_geometrycollection) {
			gn.m_el.push_back(p);
			continue;
		}
		chg = true;
		const GeometryCollection& g(p->operator const GeometryCollection& ());
		for (std::size_t i(0), n(g.size()); i < n; ++i) {
			Geometry::ptr_t p(g[i]);
			if (!p)
				continue;
			gn.m_el.push_back(p);
		}
	}
	if (gn.m_el.size() == 1)
		return gn.m_el.front();
	if (!chg)
		return Geometry::ptr_t();
	return Geometry::create(gn);
}

unsigned int GeometryCollection::get_wkbsize(void) const
{
	unsigned int sz(Geometry::get_wkbhdrsize() + 4);
	for (el_t::const_iterator i(m_el.begin()), e(m_el.end()); i != e; ++i)
		sz += (*i)->get_wkbsize() - (Geometry::get_wkbhdrsize() - 4);
	return sz;
}

uint8_t *GeometryCollection::wkbencode(uint8_t *cp, uint8_t *ep) const
{
	cp = Geometry::wkbencode(cp, ep, (uint32_t)m_el.size());
	for (el_t::const_iterator i(m_el.begin()), e(m_el.end()); i != e; ++i) {
		{
			Geometry::type_t typ((*i)->get_type());
			*cp++ = typ;
			*cp++ = typ >> 8;
			*cp++ = typ >> 16;
			*cp++ = typ >> 24;
		}
		cp = (*i)->wkbencode(cp, ep);
	}
	return cp;
}

const uint8_t *GeometryCollection::wkbdecode(const uint8_t *cp, const uint8_t *ep, uint8_t byteorder)
{
	uint32_t num(0);
	cp = Geometry::wkbdecode(cp, ep, num, byteorder);
	m_el.clear();
	m_el.reserve(num);
	for (; num; --num) {
		Geometry::ptr_t p(Geometry::create_from_wkb(cp, cp, ep, byteorder));
		if (!p)
			break;
		m_el.push_back(p);
	}
	return cp;
}

uint8_t *GeometryCollection::to_wkb(uint8_t *cp, uint8_t *ep) const
{
	cp = Geometry::wkbencode(cp, ep, Geometry::type_geometrycollection);
	return wkbencode(cp, ep);
}

std::vector<uint8_t> GeometryCollection::to_wkb(void) const
{
	std::vector<uint8_t> b(0, get_wkbsize());
	to_wkb(&b[0], &b[b.size()]);
	return b;
}

const uint8_t *GeometryCollection::from_wkb(const uint8_t *cp, const uint8_t *ep)
{
	m_el.clear();
	uint8_t byteorder(1);
	{
		Geometry::type_t typ(Geometry::type_invalid);
		cp = Geometry::wkbdecode(cp, ep, typ, byteorder);
		if (typ != Geometry::type_geometrycollection)
			throw std::runtime_error("from_wkb: not a geometrycollection: " + to_str(typ));
	}
	return wkbdecode(cp, ep, byteorder);
}

std::ostream& GeometryCollection::to_wkt(std::ostream& os) const
{
	os << "GEOMETRYCOLLECTION";
	char sep('(');
	for (el_t::const_iterator i(m_el.begin()), e(m_el.end()); i != e; ++i) {
		if (!*i)
			continue;
		(*i)->to_wkt(os << sep);
		sep = ',';
	}
	if (sep == '(')
		os << sep;
	return os << ')';
}

#ifdef HAVE_JSONCPP

Json::Value GeometryCollection::to_json(void) const
{
	Json::Value r;
	r["type"] = "GeometryCollection";
	Json::Value& g(r["geometries"]);
	g = Json::Value(Json::arrayValue);
	for (el_t::const_iterator i(m_el.begin()), e(m_el.end()); i != e; ++i) {
		if (!*i)
			continue;
		g.append((*i)->to_json());
	}
	return r;
}

void GeometryCollection::from_json(const Json::Value& g)
{
	m_el.clear();
	if (!g.isMember("type") || !g["type"].isString() || g["type"].asString() != "GeometryCollection")
		throw std::runtime_error("GeometryCollection::from_json: invalid type");
	if (!g.isMember("geometries"))
		throw std::runtime_error("GeometryCollection::from_json: no geometries");
	const Json::Value& gg(g["geometries"]);
	if (!gg.isArray())
		throw std::runtime_error("GeometryCollection::from_json: invalid geometries");
	for (Json::ArrayIndex i = 0; i < gg.size(); ++i)
		m_el.push_back(Geometry::create(gg[i]));
}

#endif

template <typename T> constexpr typename CartesianVector3D<T>::float_t CartesianVector3D<T>::from_deg;

template <typename T>
CartesianVector3D<T>& CartesianVector3D<T>::operator+=(const CartesianVector3D<T>& v)
{
	m_v[0] += v.m_v[0];
	m_v[1] += v.m_v[1];
	m_v[2] += v.m_v[2];
	return *this;
}

template <typename T>
CartesianVector3D<T>& CartesianVector3D<T>::operator-=(const CartesianVector3D<T>& v)
{
	m_v[0] -= v.m_v[0];
	m_v[1] -= v.m_v[1];
	m_v[2] -= v.m_v[2];
	return *this;
}

template <typename T>
CartesianVector3D<T>& CartesianVector3D<T>::operator*=(float_t v)
{
	m_v[0] *= v;
	m_v[1] *= v;
	m_v[2] *= v;
	return *this;
}

template <typename T>
typename CartesianVector3D<T>::float_t CartesianVector3D<T>::scalarprod(const CartesianVector3D<T>& v) const
{
	return m_v[0] * v.m_v[0] + m_v[1] * v.m_v[1] + m_v[2] * v.m_v[2];
}

template <typename T>
CartesianVector3D<T> CartesianVector3D<T>::crossprod(const CartesianVector3D<T>& v) const
{
	return CartesianVector3D<T>(m_v[1] * v.m_v[2] - m_v[2] * v.m_v[1],
				    m_v[2] * v.m_v[0] - m_v[0] * v.m_v[2],
				    m_v[0] * v.m_v[1] - m_v[1] * v.m_v[0]);
}

template <typename T>
typename CartesianVector3D<T>::float_t CartesianVector3D<T>::length2(void) const
{
	return m_v[0] * m_v[0] + m_v[1] * m_v[1] + m_v[2] * m_v[2];
}

template <typename T>
typename CartesianVector3D<T>::float_t CartesianVector3D<T>::nearest_fraction(const CartesianVector3D<T>& v0, const CartesianVector3D<T>& v1)
{
	CartesianVector3D<T> n(v1 - v0);
	float_t t = n.length2();
	if (t < (float_t)1e-6)
		return 0.5;
	t = n.scalarprod((*this) - v0) / t;
	return std::min(std::max(t, (float_t)0.0), (float_t)1.0);
}

template <typename T>
void CartesianVector3D<T>::rotate_xz(float_t ang)
{
	float_t c = cos(ang), s = sin(ang);
	float_t t = m_v[0] * c - m_v[2] * s;
	m_v[2] = m_v[0] * s + m_v[2] * c;
	m_v[0] = t;
}

template <typename T>
void CartesianVector3D<T>::rotate_xy(float_t ang)
{
	float_t c = cos(ang), s = sin(ang);
	float_t t = m_v[0] * c - m_v[1] * s;
	m_v[1] = m_v[0] * s + m_v[1] * c;
	m_v[0] = t;
}

template <typename T> constexpr typename PolarVector3D<T>::float_t PolarVector3D<T>::earth_radius;
template <typename T> constexpr typename PolarVector3D<T>::float_t PolarVector3D<T>::km_to_nmi;
template <typename T> constexpr typename PolarVector3D<T>::float_t PolarVector3D<T>::m_to_ft;
template <typename T> constexpr typename PolarVector3D<T>::float_t PolarVector3D<T>::ft_to_m;
template <typename T> constexpr typename PolarVector3D<T>::float_t PolarVector3D<T>::from_deg;
template <typename T> constexpr typename PolarVector3D<T>::float_t PolarVector3D<T>::to_deg;

template <typename T>
PolarVector3D<T>::PolarVector3D(const Point& pt, int32_t altft)
	: m_lat(pt.get_lat_rad()), m_lon(pt.get_lon_rad()), m_r(altft * ft_to_m + earth_radius)
{
}

template <typename T>
Point PolarVector3D<T>::getcoord(void) const
{
	return Point((int32_t)(m_lon * Point::from_rad), (int32_t)(m_lat * Point::from_rad));
}

template <typename T>
CartesianVector3D<T>::operator PolarVector3D<T>(void) const
{
	float_t r = length();
	float_t lon = atan2(m_v[1], m_v[0]);
	float_t lat = 0;
	if (r >= 0.1)
		lat = asin(m_v[2] / r);
	return PolarVector3D<T>(lat, lon, r);
}

template <typename T>
PolarVector3D<T>::operator CartesianVector3D<T>(void) const
{
	float_t r = m_r;
	float_t z = r * sin(m_lat);
	r *= cos(m_lat);
	float_t x = r * cos(m_lon);
	float_t y = r * sin(m_lon);
	return CartesianVector3D<T>(x, y, z);
}

template class CartesianVector3D<float>;
template class PolarVector3D<float>;
template class CartesianVector3D<double>;
template class PolarVector3D<double>;
