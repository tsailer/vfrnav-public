/***************************************************************************
 *   Copyright (C) 2007, 2009, 2012, 2013, 2014, 2015, 2018                *
 *     by Thomas Sailer                                                    *
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

#ifdef BOOST_INT128
#include <boost/multiprecision/cpp_int.hpp>
#endif

#include "geom.h"

PolygonSimple::InsideHelper::tpoint_t PolygonSimple::InsideHelper::transform(const Point& pt) const
{
	Point p(pt - get_origin());
	int64_t a(m_vector.get_lon()), b(m_vector.get_lat()), pa(p.get_lon()), pb(p.get_lat());
	return tpoint_t(a * pa + b * pb, a * pb - b * pa);
}

void PolygonSimple::InsideHelper::compute_winding(winding_t& wind, const PolygonSimple& poly, int dir) const
{
	DEFINE_INT128
        /* loop through all edges of the polygon */
        const unsigned int sz(poly.size());
        unsigned int j = sz - 1;
	tpoint_t pti, ptj(transform(poly[j]));
	if (debug)
		std::cerr << "InsideHelper::compute_winding: dir " << dir << " sz " << sz << std::endl;
        for (unsigned int i = 0; i < sz; j = i, ptj = pti, ++i) {
		pti = transform(poly[i]);
		/* edge from V[j] to V[i] */
		if (ptj.second <= 0) {
			if (pti.second > 0) {
				/* an upward crossing */
				int64_t x = ptj.first;
				{
					int128_t xx = ptj.second;
					xx *= (pti.first - ptj.first);
					int128_t d = (ptj.second - pti.second);
					xx += d / 2;
					xx /= d;
					x += static_cast<int64_t>(xx);
				}
				winding_t::iterator wi(wind.insert(winding_t::value_type(x, 0)).first);
				wi->second += dir;
				if (debug)
					std::cerr << "InsideHelper: line 0.." << m_end << " upward edge " << ptj.first << ',' << ptj.second
						  << "..." << pti.first << ',' << pti.second << " x " << wi->first << " w "
						  << (wi->second - dir) << "->" << wi->second << std::endl;
			} else if (!pti.second && !ptj.second) {
				/* a horizontal crossing on line */
				winding_t::iterator wi0(wind.insert(winding_t::value_type(ptj.first, 0)).first);
				wi0->second += dir;
				winding_t::iterator wi1(wind.insert(winding_t::value_type(pti.first, 0)).first);
				wi1->second -= dir;
				if (debug)
					std::cerr << "InsideHelper: line 0.." << m_end << " horizontal edge " << ptj.first << ',' << ptj.second
						  << "..." << pti.first << ',' << pti.second << " x0 " << wi0->first << " w0 "
						  << (wi0->second - dir) << "->" << wi0->second << " x1 " << wi1->first << " w1 "
						  << (wi1->second + dir) << "->" << wi1->second << std::endl;
			}
		} else {
			if (pti.second <= 0) {
				/* a downward crossing */
				int64_t x = ptj.first;
				{
					int128_t xx = ptj.second;
					xx *= (pti.first - ptj.first);
					int128_t d = (ptj.second - pti.second);
					xx += d / 2;
					xx /= d;
					x += static_cast<int64_t>(xx);
				}
				winding_t::iterator wi(wind.insert(winding_t::value_type(x, 0)).first);
				wi->second -= dir;
				if (debug)
					std::cerr << "InsideHelper: line 0.." << m_end << " downward edge " << ptj.first << ',' << ptj.second
						  << "..." << pti.first << ',' << pti.second << " x " << wi->first << " w "
						  << (wi->second + dir) << "->" << wi->second << std::endl;
			}
		}
	}
}

void PolygonSimple::InsideHelper::limit(int32_t fsmin, int32_t fsmax, int32_t min, int32_t max, bool allowswap)
{
	DEFINE_INT128
	int32_t fsdiff(fsmax - fsmin);
	if (!fsdiff)
		return;
	int128_t x(min - fsmin);
	int128_t y(max - fsmin);
	if (fsdiff < 0) {
		x = -x;
		y = -y;
		fsdiff = -fsdiff;
	}
	x *= m_end;
	y *= m_end;
	if (allowswap && x > y)
		std::swap(x, y);
	y += fsdiff - 1;
	x /= fsdiff;
	y /= fsdiff;
	limit(static_cast<int64_t>(x), static_cast<int64_t>(y));
}
