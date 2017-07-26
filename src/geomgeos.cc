/***************************************************************************
 *   Copyright (C) 2012, 2013, 2014, 2015 by Thomas Sailer                 *
 *   t.sailer@alumni.ethz.ch                                               *
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

#include <limits>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include "geom.h"

#if defined(HAVE_CLIPPER)

#include <polyclipping/clipper.hpp>

#if defined(HAVE_CLIPPER_PATH)

ClipperLib::Path to_clipper(const PolygonSimple& p, Point::coord_t lonoffs)
{
	ClipperLib::Path cp(p.size(), ClipperLib::IntPoint(0, 0));
	if (p.empty())
		return cp;
	for (unsigned int i = 0, sz = p.size(); i < sz; ++i) {
		const Point& pp(p[i]);
		int64_t x((uint32_t)(pp.get_lon() - lonoffs)), y(pp.get_lat());
		x <<= 8;
		y <<= 8;
		cp[i] = ClipperLib::IntPoint(x, y);
	}
	return cp;
}

void to_clipper(ClipperLib::Paths& cp, const PolygonHole& p, Point::coord_t lonoffs)
{
	{
		ClipperLib::Path cp1(to_clipper(p.get_exterior(), lonoffs));
		if (cp1.empty())
			return;
		cp.push_back(cp1);
	}
	for (unsigned int i = 0; i < p.get_nrinterior(); ++i) {
		ClipperLib::Path cp1(to_clipper(p[i], lonoffs));
		if (cp1.empty())
			return;
		cp.push_back(cp1);
	}
}

void to_clipper(ClipperLib::Paths& cp, const MultiPolygonHole& p, Point::coord_t lonoffs)
{
	for (MultiPolygonHole::const_iterator i(p.begin()), e(p.end()); i != e; ++i)
		to_clipper(cp, *i, lonoffs);
}

PolygonSimple from_clipper(const ClipperLib::Path& cp, Point::coord_t lonoffs)
{
	PolygonSimple p;
	p.resize(cp.size(), Point(0, 0));
	for (unsigned int i = 0, sz = cp.size(); i < sz; ++i) {
		const ClipperLib::IntPoint& cpp(cp[i]);
		Point pt(((cpp.X + 128) >> 8) + lonoffs, (cpp.Y + 128) >> 8);
		p[i] = pt;
	}
	return p;
}

#ifdef HAVE_CLIPPER_EXPOLYGON

PolygonHole from_clipper(const ClipperLib::ExPolygon& cp, Point::coord_t lonoffs)
{
	PolygonHole p(from_clipper(cp.outer, lonoffs));
	for (ClipperLib::Paths::const_iterator i(cp.holes.begin()), e(cp.holes.end()); i != e; ++i)
		p.add_interior(from_clipper(*i, lonoffs));
	p.normalize();
	return p;
}

MultiPolygonHole from_clipper(const ClipperLib::ExPolygons& cp, Point::coord_t lonoffs)
{
	MultiPolygonHole p;
	for (ClipperLib::ExPolygons::const_iterator i(cp.begin()), e(cp.end()); i != e; ++i)
		p.push_back(from_clipper(*i, lonoffs));
	return p;
}

typedef ClipperLib::ExPolygons ClipperLibSolution;

#endif

#ifdef HAVE_CLIPPER_POLYTREE

void from_clipper(MultiPolygonHole& ph, const ClipperLib::PolyNode& cp, Point::coord_t lonoffs)
{
        if (!cp.Contour.size())
                return;
        if (cp.IsHole())
                throw std::runtime_error("from_clipper: outer polygon is a hole!");
	MultiPolygonHole::size_type idx(ph.size());
        ph.push_back(PolygonHole());
	ph[idx].set_exterior(from_clipper(cp.Contour, lonoffs));
        for (std::vector<ClipperLib::PolyNode *>::const_iterator chi(cp.Childs.begin()), che(cp.Childs.end()); chi != che; ++chi) {
                const ClipperLib::PolyNode& cpc(**chi);
                if (!cpc.IsHole())
                        throw std::runtime_error("from_clipper: outer polygon is a child of an outer polygon");
		ph[idx].add_interior(from_clipper(cpc.Contour, lonoffs));
                for (std::vector<ClipperLib::PolyNode *>::const_iterator cchi(cpc.Childs.begin()), cche(cpc.Childs.end()); cchi != cche; ++cchi) {
                        const ClipperLib::PolyNode& cpcc(**cchi);
                        from_clipper(ph, cpcc, lonoffs);
                }
        }
        ph[idx].normalize();
}

MultiPolygonHole from_clipper(const ClipperLib::PolyTree& cp, Point::coord_t lonoffs)
{
        MultiPolygonHole p;
        if (cp.Contour.size())
                throw std::runtime_error("from_clipper: PolyTree has a contour!");
        for (std::vector<ClipperLib::PolyNode *>::const_iterator chi(cp.Childs.begin()), che(cp.Childs.end()); chi != che; ++chi) {
                const ClipperLib::PolyNode& cpc(**chi);
                from_clipper(p, cpc, lonoffs);
        }
        return p;
}

typedef ClipperLib::PolyTree ClipperLibSolution;

#endif
#endif

#if defined(HAVE_CLIPPER_POLYGON)

ClipperLib::Polygon to_clipper(const PolygonSimple& p, Point::coord_t lonoffs)
{
	ClipperLib::Polygon cp(p.size(), ClipperLib::IntPoint(0, 0));
	if (p.empty())
		return cp;
	for (unsigned int i = 0, sz = p.size(); i < sz; ++i) {
		const Point& pp(p[i]);
		int64_t x((uint32_t)(pp.get_lon() - lonoffs)), y(pp.get_lat());
		x <<= 8;
		y <<= 8;
		cp[i] = ClipperLib::IntPoint(x, y);
	}
	return cp;
}

void to_clipper(ClipperLib::Polygons& cp, const PolygonHole& p, Point::coord_t lonoffs)
{
	{
		ClipperLib::Polygon cp1(to_clipper(p.get_exterior(), lonoffs));
		if (cp1.empty())
			return;
		cp.push_back(cp1);
	}
	for (unsigned int i = 0; i < p.get_nrinterior(); ++i) {
		ClipperLib::Polygon cp1(to_clipper(p[i], lonoffs));
		if (cp1.empty())
			return;
		cp.push_back(cp1);
	}
}

void to_clipper(ClipperLib::Polygons& cp, const MultiPolygonHole& p, Point::coord_t lonoffs)
{
	for (MultiPolygonHole::const_iterator i(p.begin()), e(p.end()); i != e; ++i)
		to_clipper(cp, *i, lonoffs);
}

PolygonSimple from_clipper(const ClipperLib::Polygon& cp, Point::coord_t lonoffs)
{
	PolygonSimple p;
	p.resize(cp.size(), Point(0, 0));
	for (unsigned int i = 0, sz = cp.size(); i < sz; ++i) {
		const ClipperLib::IntPoint& cpp(cp[i]);
		Point pt(((cpp.X + 128) >> 8) + lonoffs, (cpp.Y + 128) >> 8);
		p[i] = pt;
	}
	return p;
}

#ifdef HAVE_CLIPPER_EXPOLYGON

PolygonHole from_clipper(const ClipperLib::ExPolygon& cp, Point::coord_t lonoffs)
{
	PolygonHole p(from_clipper(cp.outer, lonoffs));
	for (ClipperLib::Polygons::const_iterator i(cp.holes.begin()), e(cp.holes.end()); i != e; ++i)
		p.add_interior(from_clipper(*i, lonoffs));
	p.normalize();
	return p;
}

MultiPolygonHole from_clipper(const ClipperLib::ExPolygons& cp, Point::coord_t lonoffs)
{
	MultiPolygonHole p;
	for (ClipperLib::ExPolygons::const_iterator i(cp.begin()), e(cp.end()); i != e; ++i)
		p.push_back(from_clipper(*i, lonoffs));
	return p;
}

typedef ClipperLib::ExPolygons ClipperLibSolution;

#endif

#ifdef HAVE_CLIPPER_POLYTREE

void from_clipper(MultiPolygonHole& ph, const ClipperLib::PolyNode& cp, Point::coord_t lonoffs)
{
        if (!cp.Contour.size())
                return;
        if (cp.IsHole())
                throw std::runtime_error("from_clipper: outer polygon is a hole!");
	MultiPolygonHole::size_type idx(ph.size());
        ph.push_back(PolygonHole());
	ph[idx].set_exterior(from_clipper(cp.Contour, lonoffs));
        for (std::vector<ClipperLib::PolyNode *>::const_iterator chi(cp.Childs.begin()), che(cp.Childs.end()); chi != che; ++chi) {
                const ClipperLib::PolyNode& cpc(**chi);
                if (!cpc.IsHole())
                        throw std::runtime_error("from_clipper: outer polygon is a child of an outer polygon");
		ph[idx].add_interior(from_clipper(cpc.Contour,lon offs));
                for (std::vector<ClipperLib::PolyNode *>::const_iterator cchi(cpc.Childs.begin()), cche(cpc.Childs.end()); cchi != cche; ++cchi) {
                        const ClipperLib::PolyNode& cpcc(**cchi);
                        from_clipper(ph, cpcc, lonoffs);
                }
        }
        ph[idx].normalize();
}

MultiPolygonHole from_clipper(const ClipperLib::PolyTree& cp, Point::coord_t lonoffs)
{
        MultiPolygonHole p;
        if (cp.Contour.size())
                throw std::runtime_error("from_clipper: PolyTree has a contour!");
        for (std::vector<ClipperLib::PolyNode *>::const_iterator chi(cp.Childs.begin()), che(cp.Childs.end()); chi != che; ++chi) {
                const ClipperLib::PolyNode& cpc(**chi);
                from_clipper(p, cpc, lonoffs);
        }
        return p;
}

typedef ClipperLib::PolyTree ClipperLibSolution;

#endif
#endif

bool PolygonSimple::geos_is_valid(void) const
{
	return true;
}

void PolygonSimple::geos_make_valid(void)
{
}

MultiPolygonHole PolygonSimple::geos_simplify(void)
{
	return geos_simplify(get_bbox().get_west());
}

MultiPolygonHole PolygonSimple::geos_simplify(Point::coord_t lonoffs)
{
	ClipperLib::Clipper c;
#if defined(HAVE_CLIPPER_PATH)
	c.AddPath(to_clipper(*this, lonoffs), ClipperLib::ptSubject, true);
#else
	c.AddPolygon(to_clipper(*this, lonoffs), ClipperLib::ptSubject);
#endif
	ClipperLibSolution sol;
	c.Execute(ClipperLib::ctUnion, sol, ClipperLib::pftNonZero, ClipperLib::pftNonZero); 
	return from_clipper(sol, lonoffs);
}

bool PolygonHole::geos_is_valid(void) const
{
	return true;
}

bool MultiPolygonHole::geos_is_valid(void) const
{
	return true;
}

void MultiPolygonHole::geos_make_valid(void)
{
}

void MultiPolygonHole::geos_union(const MultiPolygonHole& m)
{
	Point::coord_t lonoffs;
	ClipperLib::Clipper c;
	{
		MultiPolygonHole mp1(*this);
		MultiPolygonHole mp2(m);
		mp1.normalize();
		mp2.normalize();
		lonoffs = mp1.get_bbox().add(mp2.get_bbox()).get_west();
#if defined(HAVE_CLIPPER_PATH)
		{
			ClipperLib::Paths cp;
			to_clipper(cp, mp1, lonoffs);
			c.AddPaths(cp, ClipperLib::ptSubject, true);
		}
		{
			ClipperLib::Paths cp;
			to_clipper(cp, mp2, lonoffs);
			c.AddPaths(cp, ClipperLib::ptSubject, true);
		}
#else
		{
			ClipperLib::Polygons cp;
			to_clipper(cp, mp1, lonoffs);
			c.AddPolygons(cp, ClipperLib::ptSubject);
		}
		{
			ClipperLib::Polygons cp;
			to_clipper(cp, mp2, lonoffs);
			c.AddPolygons(cp, ClipperLib::ptSubject);
		}
#endif
	}
	ClipperLibSolution sol;
	c.Execute(ClipperLib::ctUnion, sol, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
	MultiPolygonHole mp(from_clipper(sol, lonoffs));
	swap(mp);
}


void MultiPolygonHole::geos_union(const MultiPolygonHole& m, Point::coord_t lonoffs)
{
	ClipperLib::Clipper c;
	{
		MultiPolygonHole mp1(*this);
		MultiPolygonHole mp2(m);
		mp1.normalize();
		mp2.normalize();
#if defined(HAVE_CLIPPER_PATH)
		{
			ClipperLib::Paths cp;
			to_clipper(cp, mp1, lonoffs);
			c.AddPaths(cp, ClipperLib::ptSubject, true);
		}
		{
			ClipperLib::Paths cp;
			to_clipper(cp, mp2, lonoffs);
			c.AddPaths(cp, ClipperLib::ptSubject, true);
		}
#else
		{
			ClipperLib::Polygons cp;
			to_clipper(cp, mp1, lonoffs);
			c.AddPolygons(cp, ClipperLib::ptSubject);
		}
		{
			ClipperLib::Polygons cp;
			to_clipper(cp, mp2, lonoffs);
			c.AddPolygons(cp, ClipperLib::ptSubject);
		}
#endif
	}
	ClipperLibSolution sol;
	c.Execute(ClipperLib::ctUnion, sol, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
	MultiPolygonHole mp(from_clipper(sol, lonoffs));
	swap(mp);
}

void MultiPolygonHole::geos_subtract(const MultiPolygonHole& m)
{
	Point::coord_t lonoffs;
	ClipperLib::Clipper c;
	{
		MultiPolygonHole mp1(*this);
		MultiPolygonHole mp2(m);
		mp1.normalize();
		mp2.normalize();
		lonoffs = mp1.get_bbox().add(mp2.get_bbox()).get_west();
#if defined(HAVE_CLIPPER_PATH)
		{
			ClipperLib::Paths cp;
			to_clipper(cp, mp1, lonoffs);
			c.AddPaths(cp, ClipperLib::ptSubject, true);
		}
		{
			ClipperLib::Paths cp;
			to_clipper(cp, mp2, lonoffs);
			c.AddPaths(cp, ClipperLib::ptClip, true);
		}
#else
		{
			ClipperLib::Polygons cp;
			to_clipper(cp, mp1, lonoffs);
			c.AddPolygons(cp, ClipperLib::ptSubject);
		}
		{
			ClipperLib::Polygons cp;
			to_clipper(cp, mp2, lonoffs);
			c.AddPolygons(cp, ClipperLib::ptClip);
		}
#endif
	}
	ClipperLibSolution sol;
	c.Execute(ClipperLib::ctDifference, sol, ClipperLib::pftNonZero, ClipperLib::pftNonZero); 
	MultiPolygonHole mp(from_clipper(sol, lonoffs));
	swap(mp);
}

void MultiPolygonHole::geos_subtract(const MultiPolygonHole& m, Point::coord_t lonoffs)
{
	ClipperLib::Clipper c;
	{
		MultiPolygonHole mp1(*this);
		MultiPolygonHole mp2(m);
		mp1.normalize();
		mp2.normalize();
#if defined(HAVE_CLIPPER_PATH)
		{
			ClipperLib::Paths cp;
			to_clipper(cp, mp1, lonoffs);
			c.AddPaths(cp, ClipperLib::ptSubject, true);
		}
		{
			ClipperLib::Paths cp;
			to_clipper(cp, mp2, lonoffs);
			c.AddPaths(cp, ClipperLib::ptClip, true);
		}
#else
		{
			ClipperLib::Polygons cp;
			to_clipper(cp, mp1, lonoffs);
			c.AddPolygons(cp, ClipperLib::ptSubject);
		}
		{
			ClipperLib::Polygons cp;
			to_clipper(cp, mp2, lonoffs);
			c.AddPolygons(cp, ClipperLib::ptClip);
		}
#endif
	}
	ClipperLibSolution sol;
	c.Execute(ClipperLib::ctDifference, sol, ClipperLib::pftNonZero, ClipperLib::pftNonZero); 
	MultiPolygonHole mp(from_clipper(sol, lonoffs));
	swap(mp);
}

void MultiPolygonHole::geos_intersect(const MultiPolygonHole& m)
{
	Point::coord_t lonoffs;
	ClipperLib::Clipper c;
	{
		MultiPolygonHole mp1(*this);
		MultiPolygonHole mp2(m);
		mp1.normalize();
		mp2.normalize();
		lonoffs = mp1.get_bbox().add(mp2.get_bbox()).get_west();
#if defined(HAVE_CLIPPER_PATH)
		{
			ClipperLib::Paths cp;
			to_clipper(cp, mp1, lonoffs);
			c.AddPaths(cp, ClipperLib::ptSubject, true);
		}
		{
			ClipperLib::Paths cp;
			to_clipper(cp, mp2, lonoffs);
			c.AddPaths(cp, ClipperLib::ptClip, true);
		}
#else
		{
			ClipperLib::Polygons cp;
			to_clipper(cp, mp1, lonoffs);
			c.AddPolygons(cp, ClipperLib::ptSubject);
		}
		{
			ClipperLib::Polygons cp;
			to_clipper(cp, mp2, lonoffs);
			c.AddPolygons(cp, ClipperLib::ptClip);
		}
#endif
	}
	ClipperLibSolution sol;
	c.Execute(ClipperLib::ctIntersection, sol, ClipperLib::pftNonZero, ClipperLib::pftNonZero); 
	MultiPolygonHole mp(from_clipper(sol, lonoffs));
	swap(mp);
}

void MultiPolygonHole::geos_intersect(const MultiPolygonHole& m, Point::coord_t lonoffs)
{
	ClipperLib::Clipper c;
	{
		MultiPolygonHole mp1(*this);
		MultiPolygonHole mp2(m);
		mp1.normalize();
		mp2.normalize();
#if defined(HAVE_CLIPPER_PATH)
		{
			ClipperLib::Paths cp;
			to_clipper(cp, mp1, lonoffs);
			c.AddPaths(cp, ClipperLib::ptSubject, true);
		}
		{
			ClipperLib::Paths cp;
			to_clipper(cp, mp2, lonoffs);
			c.AddPaths(cp, ClipperLib::ptClip, true);
		}
#else
		{
			ClipperLib::Polygons cp;
			to_clipper(cp, mp1, lonoffs);
			c.AddPolygons(cp, ClipperLib::ptSubject);
		}
		{
			ClipperLib::Polygons cp;
			to_clipper(cp, mp2, lonoffs);
			c.AddPolygons(cp, ClipperLib::ptClip);
		}
#endif
	}
	ClipperLibSolution sol;
	c.Execute(ClipperLib::ctIntersection, sol, ClipperLib::pftNonZero, ClipperLib::pftNonZero); 
	MultiPolygonHole mp(from_clipper(sol, lonoffs));
	swap(mp);
}

#elif defined(HAVE_GEOS)

#include <geos.h>

geos::geom::LinearRing *to_geos(geos::geom::GeometryFactory& f, const PolygonSimple& p, ::Point& offs)
{
	std::unique_ptr<CoordinateSequence> coord(f.getCoordinateSequenceFactory()->create((size_t)0, (size_t)2));
	for (PolygonSimple::const_iterator i(p.begin()), e(p.end()); i != e; ++i) {
		if (offs.is_invalid())
			offs = *i;
		::Point pt(*i - offs);
		coord->add(geos::geom::Coordinate(pt.get_lon_rad_dbl(), pt.get_lat_rad_dbl()), false);
	}
	if (!coord->isEmpty())
		coord->add(coord->front());
	if (false)
		std::cerr << "to_geos: poly size " << p.size() << " coord size " << coord->getSize() << " offs " << offs << std::endl;
	if (true) {
		if (coord->getSize() < 4 && !coord->isEmpty()) {
			std::cerr << "coord: " << coord->toString() << std::endl << "poly:";
			for (PolygonSimple::const_iterator i(p.begin()), e(p.end()); i != e; ++i)
				std::cerr << ' ' << *i;
			std::cerr << std::endl;
			if (p.size() == 3)
				std::cerr << "area2: " << ::Point::area2(p[0], p[2], p[1]) << " is"
					  << (p[1].is_between(p[0], p[2]) ? "" : " NOT") << " between" << std::endl;
		}
	}
	std::unique_ptr<geos::geom::LinearRing> ring(f.createLinearRing(coord.release()));
	ring->normalize();
	return ring.release();
}

PolygonSimple from_geos(const geos::geom::LineString *ring, const ::Point& offs)
{
	PolygonSimple p;
	if (!ring)
		return p;
	unsigned int nrpt(ring->getNumPoints());
	if (!nrpt)
		return p;
	for (unsigned int i = 0; i < nrpt; ++i) {
		const geos::geom::Point *pt(ring->getPointN(i));
		::Point pt1;
		pt1.set_lon_rad_dbl(pt->getX());
		pt1.set_lat_rad_dbl(pt->getY());
		pt1 += offs;
		p.push_back(pt1);
	}
	if (p.empty())
		return p;
	if (p.front() == p.back())
		p.resize(p.size() - 1);
	return p;
}


geos::geom::Polygon *to_geos(geos::geom::GeometryFactory& f, const PolygonHole& p, ::Point& offs)
{
	std::unique_ptr<geos::geom::LinearRing> outer(to_geos(f, p.get_exterior(), offs));
	std::unique_ptr<std::vector<geos::geom::Geometry *> > inner(new std::vector<geos::geom::Geometry *>());
	try {
		for (unsigned int i = 0; i < p.get_nrinterior(); ++i)
			inner->push_back(to_geos(f, p[i], offs));
	} catch (...) {
		for (std::vector<geos::geom::Geometry *>::iterator i(inner->begin()), e(inner->end()); i != e; ++i)
			delete *i;
		throw;
	}
 	std::unique_ptr<geos::geom::Polygon> poly(f.createPolygon(outer.release(), inner.release()));
	poly->normalize();
	return poly.release();
}

PolygonHole from_geos(const geos::geom::Polygon *poly, const ::Point& offs)
{
	PolygonHole p;
	if (!poly)
		return p;
	p.set_exterior(from_geos(poly->getExteriorRing(), offs));
	for (unsigned int i = 0; i < poly->getNumInteriorRing(); ++i) {
		PolygonSimple ps(from_geos(poly->getInteriorRingN(i), offs));
		if (ps.empty())
			continue;
		p.add_interior(ps);
	}
	return p;
}

geos::geom::MultiPolygon *to_geos(geos::geom::GeometryFactory& f, const MultiPolygonHole& p, ::Point& offs)
{
	std::unique_ptr<std::vector<geos::geom::Geometry *> > polys(new std::vector<geos::geom::Geometry *>());
	try {
		for (MultiPolygonHole::const_iterator pi(p.begin()), pe(p.end()); pi != pe; ++pi)
			polys->push_back(to_geos(f, *pi, offs));
	} catch (...) {
		for (std::vector<geos::geom::Geometry *>::iterator i(polys->begin()), e(polys->end()); i != e; ++i)
			delete *i;
		throw;
	}
	std::unique_ptr<geos::geom::MultiPolygon> pp(f.createMultiPolygon(polys.release()));
	return pp.release();
}

MultiPolygonHole from_geos_any(const geos::geom::Geometry *g, const ::Point& offs);

MultiPolygonHole from_geos(const geos::geom::GeometryCollection *gc, const ::Point& offs)
{
	MultiPolygonHole mp;
	if (!gc)
		return mp;
 	for (unsigned int i = 0; i < gc->getNumGeometries(); ++i) {
		MultiPolygonHole mp1(from_geos_any(gc->getGeometryN(i), offs));
		mp.insert(mp.end(), mp1.begin(), mp1.end());
	}
	return mp;
}

MultiPolygonHole from_geos_any(const geos::geom::Geometry *g, const ::Point& offs)
{
	if (!g)
		return MultiPolygonHole();
	{
		const geos::geom::GeometryCollection *g1(dynamic_cast<const geos::geom::GeometryCollection *>(g));
		if (g1)
			return from_geos(g1, offs);
	}
	{
		const geos::geom::Polygon *g1(dynamic_cast<const geos::geom::Polygon *>(g));
		if (g1) {
			PolygonHole ph(from_geos(g1, offs));
			MultiPolygonHole mp;
			mp.push_back(ph);
			return mp;
		}
	}
	{
		const geos::geom::LineString *g1(dynamic_cast<const geos::geom::LineString *>(g));
		if (g1) {
			PolygonHole ph(from_geos(g1, offs));
			MultiPolygonHole mp;
			mp.push_back(ph);
			return mp;
		}
	}
	throw std::runtime_error("from_geos: cannot handle " + g->getGeometryType());
}

bool PolygonSimple::geos_is_valid(void) const
{
	geos::geom::GeometryFactory f;
	::Point offs;
	offs.set_invalid();
	geos::geom::LinearRing *ring(to_geos(f, *this, offs));
	bool valid(ring->isValid());
	f.destroyGeometry(ring);
	return valid;
}

bool PolygonSimple::geos_make_valid_helper(void)
{
	if (size() < 3) {
		clear();
		return true;
	}
	geos::geom::GeometryFactory f;
	::Point offs;
	offs.set_invalid();
	geos::geom::LinearRing *ring(to_geos(f, *this, offs));
	geos::geom::Point *stpt(ring->getStartPoint());
	if (ring->isEmpty() || !stpt) {
		std::cerr << "PolygonSimple::geos_make_valid_helper: ring collapsed" << std::endl;
		f.destroyGeometry(ring);
		return false;
	}
	geos::geom::Point *pt(f.createPoint(stpt->getCoordinates()));
	geos::geom::Geometry *g;
	try {
		g = ring->Union(pt);
	} catch (...) {
		if (false)
			std::cerr << "PolygonSimple::geos_make_valid: union exception" << std::endl;
		f.destroyGeometry(pt);
		f.destroyGeometry(ring);
		return false;
	}
	f.destroyGeometry(pt);
	f.destroyGeometry(ring);
	MultiPolygonHole mp(from_geos_any(g, offs));
	f.destroyGeometry(g);
	if (mp.empty()) {
		std::cerr << "PolygonSimple::geos_make_valid: result is empty" << std::endl;
		clear();
		return false;
	}
	if (mp.size() > 1)
		std::cerr << "PolygonSimple::geos_make_valid: result has multiple polygons" << std::endl;
	if (mp[0].get_nrinterior())
		std::cerr << "PolygonSimple::geos_make_valid: result has holes" << std::endl;
	swap(mp[0].get_exterior());
	return true;
}

void PolygonSimple::geos_make_valid(void)
{
	remove_redundant_polypoints();
	if (size() < 3) {
		clear();
		return;
	}
	PolygonSimple ps(*this);
	while (ps.size() >= 3) {
		for (unsigned int roundbits = 0; roundbits < 12; ++roundbits) {
			bool dornd(!roundbits);
			do {
				PolygonSimple psnew;
				int64_t areadiff(std::numeric_limits<int64_t>::max());
				for (unsigned int delpoint = 0; delpoint <= size(); ++delpoint) {
					PolygonSimple ps1(ps);
					int64_t curareadiff(0);
					if (delpoint) {
						unsigned int sz(ps1.size());
						unsigned int i((delpoint + sz - 2) % sz);
						unsigned int j(delpoint - 1);
						unsigned int k(delpoint % sz);
						curareadiff = abs(::Point::area2(ps1[i], ps1[j], ps1[k]));
						ps1.erase(ps1.begin() + (delpoint - 1));
					}
					if (dornd)
						ps1.randomize_bits(roundbits);
					else
						ps1.snap_bits(roundbits);
					if (ps1.geos_make_valid_helper() && curareadiff <= areadiff) {
						if (delpoint || roundbits || ps.size() < size())
							std::cerr << "PolygonSimple::geos_make_valid: succeeded, roundbits "
								  << roundbits << (dornd ? ", random" : "") << ", delpt " << delpoint
								  << " Area " << ps.area2() << " diff " << (ps1.area2() - ps.area2())
								  << " ptdiff " << curareadiff << std::endl;
						areadiff = curareadiff;
						psnew.swap(ps1);
					}
				}
				if (!psnew.empty()) {
					this->swap(psnew);
					return;
				}
				dornd = !dornd;
			} while (dornd);
		}
		::Point pt;
		ps.zap_leastimportant(pt);
		std::cerr << "PolygonSimple::geos_make_valid: zapping " << pt << " for good: points "
			  << ps.size() << '/' << size() << " area " << ps.area2() << '/' << area2() << std::endl;
	}
	std::cerr << "PolygonSimple::geos_make_valid: Warning: cannot make valid, zapping" << std::endl;
	clear();
}

MultiPolygonHole PolygonSimple::geos_simplify(void)
{
	MultiPolygonHole mp;
	mp.push_back(*this);
	return mp;
}

bool PolygonHole::geos_is_valid(void) const
{
	geos::geom::GeometryFactory f;
	::Point offs;
	offs.set_invalid();
	geos::geom::Polygon *poly(to_geos(f, *this, offs));
	bool valid(poly->isValid());
	f.destroyGeometry(poly);
	return valid;
}

bool MultiPolygonHole::geos_is_valid(void) const
{
	try {
		geos::geom::GeometryFactory f;
		::Point offs;
		offs.set_invalid();
		geos::geom::MultiPolygon *poly(to_geos(f, *this, offs));
		bool valid(poly->isValid());
		f.destroyGeometry(poly);
		return valid;
	} catch (geos::util::GEOSException& e) {
		std::cerr << "geos_is_valid: exception " << e.what() << std::endl;
		throw;
	}
}

void MultiPolygonHole::geos_make_valid(void)
{
	geos::geom::GeometryFactory f;
	::Point offs;
	offs.set_invalid();
	geos::geom::MultiPolygon *poly(to_geos(f, *this, offs));
	geos::geom::Geometry *g(poly->buffer(0));
	MultiPolygonHole ph(from_geos_any(g, offs));
	f.destroyGeometry(poly);
	f.destroyGeometry(g);
	ph.remove_redundant_polypoints();
	this->swap(ph);
}

void MultiPolygonHole::geos_union(const MultiPolygonHole& m)
{
	for (unsigned int roundbits = 0; roundbits < 16; ++roundbits) {
		bool dornd(!roundbits);
		do {
			MultiPolygonHole m1(*this);
			MultiPolygonHole m2(m);
			if (dornd) {
				m1.randomize_bits(roundbits);
				m2.randomize_bits(roundbits);
			} else {
				m1.snap_bits(roundbits);
				m2.snap_bits(roundbits);
			}
			dornd = !dornd;
			geos::geom::GeometryFactory f;
			::Point offs;
			offs.set_invalid();
			geos::geom::MultiPolygon *p1(to_geos(f, m1, offs));
			geos::geom::MultiPolygon *p2(to_geos(f, m2, offs));
			geos::geom::Geometry *g;
			try {
				g = p1->Union(p2);
			} catch (...) {
				std::cerr << "MultiPolygonHole::geos_union: exception, roundbits " << roundbits << (dornd ? ", random" : "") << std::endl;
				f.destroyGeometry(p1);
				f.destroyGeometry(p2);
				continue;
			}
			f.destroyGeometry(p1);
			f.destroyGeometry(p2);
			MultiPolygonHole ph(from_geos_any(g, offs));
			f.destroyGeometry(g);
			ph.remove_redundant_polypoints();
			this->swap(ph);
			return;
		} while (dornd);
	}
	std::cerr << "MultiPolygonHole::geos_union: Warning: robustness error, zapping" << std::endl;
	clear();
}

void MultiPolygonHole::geos_subtract(const MultiPolygonHole& m)
{
	for (unsigned int roundbits = 0; roundbits < 16; ++roundbits) {
		bool dornd(!roundbits);
		do {
			MultiPolygonHole m1(*this);
			MultiPolygonHole m2(m);
			if (dornd) {
				m1.randomize_bits(roundbits);
				m2.randomize_bits(roundbits);
			} else {
				m1.snap_bits(roundbits);
				m2.snap_bits(roundbits);
			}
			dornd = !dornd;
			geos::geom::GeometryFactory f;
			::Point offs;
			offs.set_invalid();
			geos::geom::MultiPolygon *p1(to_geos(f, m1, offs));
			geos::geom::MultiPolygon *p2(to_geos(f, m2, offs));
			geos::geom::Geometry *g;
			try {
				g = p1->difference(p2);
			} catch (...) {
				std::cerr << "MultiPolygonHole::geos_subtract: exception, roundbits " << roundbits << (dornd ? ", random" : "") << std::endl;
				f.destroyGeometry(p1);
				f.destroyGeometry(p2);
				continue;
			}
			f.destroyGeometry(p1);
			f.destroyGeometry(p2);
			MultiPolygonHole ph(from_geos_any(g, offs));
			f.destroyGeometry(g);
			ph.remove_redundant_polypoints();
			this->swap(ph);
			return;
		} while (dornd);
	}
	std::cerr << "MultiPolygonHole::geos_subtract: Warning: robustness error, zapping" << std::endl;
	clear();
}

void MultiPolygonHole::geos_intersect(const MultiPolygonHole& m)
{
	for (unsigned int roundbits = 0; roundbits < 16; ++roundbits) {
		bool dornd(!roundbits);
		do {
			MultiPolygonHole m1(*this);
			MultiPolygonHole m2(m);
			if (dornd) {
				m1.randomize_bits(roundbits);
				m2.randomize_bits(roundbits);
			} else {
				m1.snap_bits(roundbits);
				m2.snap_bits(roundbits);
			}
			dornd = !dornd;
			geos::geom::GeometryFactory f;
			::Point offs;
			offs.set_invalid();
			geos::geom::MultiPolygon *p1(to_geos(f, m1, offs));
			geos::geom::MultiPolygon *p2(to_geos(f, m2, offs));
			geos::geom::Geometry *g;
			try {
				g = p1->intersection(p2);
			} catch (...) {
				std::cerr << "MultiPolygonHole::geos_intersect: exception, roundbits " << roundbits << (dornd ? ", random" : "") << std::endl;
				f.destroyGeometry(p1);
				f.destroyGeometry(p2);
				continue;
			}
			f.destroyGeometry(p1);
			f.destroyGeometry(p2);
			MultiPolygonHole ph(from_geos_any(g, offs));
			f.destroyGeometry(g);
			ph.remove_redundant_polypoints();
			this->swap(ph);
			return;
		} while (dornd);
	}
	std::cerr << "MultiPolygonHole::geos_intersect: Warning: robustness error, zapping" << std::endl;
	clear();
}

#else

bool PolygonSimple::geos_is_valid(void) const
{
	return true;
}

void PolygonSimple::geos_make_valid(void)
{
}

MultiPolygonHole PolygonSimple::geos_simplify(void)
{
	MultiPolygonHole mp;
	mp.push_back(*this);
	return mp;
}

bool PolygonHole::geos_is_valid(void) const
{
	return true;
}

bool MultiPolygonHole::geos_is_valid(void) const
{
	return true;
}

void MultiPolygonHole::geos_make_valid(void)
{
}

void MultiPolygonHole::geos_union(const MultiPolygonHole& m)
{
}

void MultiPolygonHole::geos_subtract(const MultiPolygonHole& m)
{
}

void MultiPolygonHole::geos_intersect(const MultiPolygonHole& m)
{
}

#endif
