/***************************************************************************
 *   Copyright (C) 2007, 2009, 2012, 2013, 2014, 2015, 2016                *
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

#include <sstream>
#include <iomanip>

#include "geom.h"

#ifdef __AVX__

extern __inline int64_t __attribute__ ((__gnu_inline__, __always_inline__, __artificial__)) vextr_epi64(__m256i x, unsigned int j)
{
	switch (j & 3) {
	case 3:
		return _mm256_extract_epi64(x, 3);

	case 2:
		return _mm256_extract_epi64(x, 2);

	case 1:
		return _mm256_extract_epi64(x, 1);

	default:
		return _mm256_extract_epi64(x, 0);
	}
}

extern __inline int32_t __attribute__ ((__gnu_inline__, __always_inline__, __artificial__)) vextr_epi32(__m256i x, unsigned int j)
{
	switch (j & 7) {
	case 7:
		return _mm256_extract_epi32(x, 7);

	case 6:
		return _mm256_extract_epi32(x, 6);

	case 5:
		return _mm256_extract_epi32(x, 5);

	case 4:
		return _mm256_extract_epi32(x, 4);

	case 3:
		return _mm256_extract_epi32(x, 3);

	case 2:
		return _mm256_extract_epi32(x, 2);

	case 1:
		return _mm256_extract_epi32(x, 1);

	default:
		return _mm256_extract_epi32(x, 0);
	}
}

inline void PolygonSimple::InsideHelper::avx2_horizontal(winding_t& wind, int dir, __m256i vptprevlat, __m256i vptprevlon, __m256i vptlat, __m256i vptlon,
							 uint32_t cond, unsigned int cnt, unsigned int i) const
{
	// dir << 24 is a hack to ensure horizontal lines are treated as part of the polygon in all cases
	DEFINE_INT128
	if (!__builtin_expect(cond, 0))
		return;
	for (unsigned int j = 0; j < cnt; ++j, cond >>= 8) {
		if (!(cond & 0x80))
			continue;
		winding_t::iterator wi0(wind.insert(winding_t::value_type(vextr_epi64(vptprevlon, j), 0)).first);
		wi0->second += dir << 24;
		winding_t::iterator wi1(wind.insert(winding_t::value_type(vextr_epi64(vptlon, j), 0)).first);
		wi1->second -= dir << 24;
		if (debug)
			std::cerr << "InsideHelper: line 0.." << m_end << " horizontal edge " << vextr_epi64(vptprevlon, j)
				  << ',' << vextr_epi64(vptprevlat, j) << "..." << vextr_epi64(vptlon, j) << ','
				  << vextr_epi64(vptlat, j) << " x0 " << wi0->first << " w0 "
				  << (wi0->second - dir) << "->" << wi0->second << " x1 " << wi1->first << " w1 "
				  << (wi1->second + dir) << "->" << wi1->second << " i " << (i + j) << std::endl;
	}
}

inline void PolygonSimple::InsideHelper::avx2_upward(winding_t& wind, int dir, __m256i vptprevlat, __m256i vptprevlon, __m256i vptlat, __m256i vptlon,
					uint32_t cond, unsigned int cnt, unsigned int i) const
{
	DEFINE_INT128
	if (!__builtin_expect(cond, 0))
		return;
	__m256i vptlonmptprevlon = _mm256_sub_epi64(vptlon, vptprevlon);
	__m256i vptprevlatmptlat = _mm256_sub_epi64(vptprevlat, vptlat);
	for (unsigned int j = 0; j < cnt; ++j, cond >>= 8) {
		if (!(cond & 0x80))
			continue;
		int64_t x = vextr_epi64(vptprevlon, j);
		{
			int128_t xx = vextr_epi64(vptprevlat, j);
			xx *= vextr_epi64(vptlonmptprevlon, j);
			int128_t d = vextr_epi64(vptprevlatmptlat, j);
			xx += d / 2;
			xx /= d;
			x += static_cast<int64_t>(xx);
		}
		winding_t::iterator wi(wind.insert(winding_t::value_type(x, 0)).first);
		wi->second += dir;
		if (debug)
			std::cerr << "InsideHelper: line 0.." << m_end << " upward edge " << vextr_epi64(vptprevlon, j)
				  << ',' << vextr_epi64(vptprevlat, j) << "..." << vextr_epi64(vptlon, j) << ','
				  << vextr_epi64(vptlat, j) << " x " << wi->first << " w "
				  << (wi->second - dir) << "->" << wi->second << " i " << (i + j) << std::endl;
	}
}

inline void PolygonSimple::InsideHelper::avx2_downward(winding_t& wind, int dir, __m256i vptprevlat, __m256i vptprevlon, __m256i vptlat, __m256i vptlon,
						       uint32_t cond, unsigned int cnt, unsigned int i) const
{
	DEFINE_INT128
	if (!__builtin_expect(cond, 0))
		return;
	__m256i vptlonmptprevlon = _mm256_sub_epi64(vptlon, vptprevlon);
	__m256i vptprevlatmptlat = _mm256_sub_epi64(vptprevlat, vptlat);
	for (unsigned int j = 0; j < cnt; ++j, cond >>= 8) {
		if (!(cond & 0x80))
			continue;
		int64_t x = vextr_epi64(vptprevlon, j);
		{
			int128_t xx = vextr_epi64(vptprevlat, j);
			xx *= vextr_epi64(vptlonmptprevlon, j);
			int128_t d = vextr_epi64(vptprevlatmptlat, j);
			xx += d / 2;
			xx /= d;
			x += static_cast<int64_t>(xx);
		}
		winding_t::iterator wi(wind.insert(winding_t::value_type(x, 0)).first);
		wi->second -= dir;
		if (debug)
			std::cerr << "InsideHelper: line 0.." << m_end << " downward edge " << vextr_epi64(vptprevlon, j)
				  << ',' << vextr_epi64(vptprevlat, j) << "..." << vextr_epi64(vptlon, j) << ','
				  << vextr_epi64(vptlat, j) << " x " << wi->first << " w "
				  << (wi->second + dir) << "->" << wi->second << " i " << (i + j) << std::endl;
	}
}

#endif

PolygonSimple::InsideHelper::tpoint_t PolygonSimple::InsideHelper::transform(const Point& pt) const
{
#ifdef __AVX__
	if (m_simd != simd_none) {
		__m128i vpt = _mm_set1_epi64(*(const __m64 *)&pt);
		__m128i vporigin = _mm_set1_epi64(*(const __m64 *)&get_origin());
		__m128i vpvec = _mm_set1_epi64(*(const __m64 *)&m_vector);
		__m128i vp = _mm_sub_epi32(vpt, vporigin);
		vp = _mm_shuffle_epi32(vp, 0x50);
		vpvec = _mm_shuffle_epi32(vpvec, 0x50);
		__m128i m0 = _mm_mul_epi32(vp, vpvec);
		__m128i m1 = _mm_mul_epi32(vp, _mm_shuffle_epi32(vpvec, 0x0a));
		return tpoint_t(_mm_extract_epi64(m0, 1) + _mm_extract_epi64(m0, 0),
				_mm_extract_epi64(m1, 1) - _mm_extract_epi64(m1, 0));
	}
#endif
	Point p(pt - get_origin());
	int64_t a(m_vector.get_lon()), b(m_vector.get_lat()), pa(p.get_lon()), pb(p.get_lat());
	return tpoint_t(a * pa + b * pb, a * pb - b * pa);
}

void PolygonSimple::InsideHelper::compute_winding(winding_t& wind, const PolygonSimple& poly, int dir) const
{
	DEFINE_INT128
	/* loop through all edges of the polygon */
	const unsigned int sz(poly.size());
	if (debug)
		std::cerr << "InsideHelper::compute_winding: dir " << dir << " sz " << sz << std::endl;
	if (!sz)
		return;
#ifdef __AVX__
	if (m_simd == simd_avx2) {
		// https://software.intel.com/sites/landingpage/IntrinsicsGuide/#techs=AVX,AVX2
		__m256i vorigin = _mm256_broadcastq_epi64(_mm_set1_epi64(*(const __m64 *)&m_origin));
		__m256i vvectorlon = _mm256_broadcastq_epi64(_mm_set1_epi64(*(const __m64 *)&m_vector));
		__m256i vvectorlat = _mm256_srli_epi64(vvectorlon, 32);

		// preparation; transformed last element (pt[j]) in lane 3 of vptprevlon/lat
		__m256i vptprevlon, vptprevlat;
		{
			__m256i vplon = _mm256_set1_epi64x(*(const uint64_t *)&poly.PolygonSimple::base_t::operator[](sz - 1));
			//_mm_prefetch(&poly.PolygonSimple::base_t::operator[](0), _MM_HINT_T0);
			vplon = _mm256_permute4x64_epi64(vplon, 0);
			vplon = _mm256_sub_epi32(vplon, vorigin);
			__m256i vplat = _mm256_srli_epi64(vplon, 32);
			vptprevlon = _mm256_add_epi64(_mm256_mul_epi32(vvectorlon, vplon), _mm256_mul_epi32(vvectorlat, vplat));
			vptprevlat = _mm256_sub_epi64(_mm256_mul_epi32(vvectorlon, vplat), _mm256_mul_epi32(vvectorlat, vplon));
		}

		if (false) {
			std::ostringstream oss;
			oss << std::hex;
			for (unsigned int j = 0; j < 4; ++j)
				oss << "vorigin[" << j << "]: 0x" << std::setfill('0') << std::setw(16) << vextr_epi64(vorigin, j) << std::endl
				    << "vvectorlon[" << j << "]: 0x" << std::setfill('0') << std::setw(16) << vextr_epi64(vvectorlon, j) << std::endl
				    << "vvectorlat[" << j << "]: 0x" << std::setfill('0') << std::setw(16) << vextr_epi64(vvectorlat, j) << std::endl
				    << "vptprevlon[" << j << "]: 0x" << std::setfill('0') << std::setw(16) << vextr_epi64(vptprevlon, j) << std::endl
				    << "vptprevlat[" << j << "]: 0x" << std::setfill('0') << std::setw(16) << vextr_epi64(vptprevlat, j) << std::endl;
			tpoint_t ptj(transform(poly.PolygonSimple::base_t::operator[](sz - 1)));
			oss << "origin: lat 0x" << std::setfill('0') << std::setw(8) << m_origin.get_lat()
			    << " lon 0x" << std::setfill('0') << std::setw(8) << m_origin.get_lon() << std::endl
			    << "vector: lat 0x" << std::setfill('0') << std::setw(8) << m_vector.get_lat()
			    << " lon 0x" << std::setfill('0') << std::setw(8) << m_vector.get_lon() << std::endl
			    << "lastpoint: lat 0x" << std::setfill('0') << std::setw(16) << ptj.second
			    << " lon 0x" << std::setfill('0') << std::setw(16) << ptj.first << std::endl;
			std::cerr << oss.str();
		}

		// tranform first 4 elements
		__m256i vptlon, vptlat;
		{
			__m256i vplon = _mm256_load_si256((const __m256i *)&poly.PolygonSimple::base_t::operator[](0));
			//_mm_prefetch(&poly.PolygonSimple::base_t::operator[](4), _MM_HINT_T0);
			vplon = _mm256_sub_epi32(vplon, vorigin);
			__m256i vplat = _mm256_srli_epi64(vplon, 32);
			vptlon = _mm256_add_epi64(_mm256_mul_epi32(vvectorlon, vplon), _mm256_mul_epi32(vvectorlat, vplat));
			vptlat = _mm256_sub_epi64(_mm256_mul_epi32(vvectorlon, vplat), _mm256_mul_epi32(vvectorlat, vplon));
		}
		// vptprevxxx = vptxxx[2:0] vptprevxxx[3]
		vptprevlon = _mm256_blend_epi32(_mm256_permute4x64_epi64(vptlon, 0x90), _mm256_permute4x64_epi64(vptprevlon, 0xff), 0x03);
		vptprevlat = _mm256_blend_epi32(_mm256_permute4x64_epi64(vptlat, 0x90), _mm256_permute4x64_epi64(vptprevlat, 0xff), 0x03);
		unsigned int i = 0;
		for (unsigned int iend = (sz - 1) & ~3U; i != iend; i += 4) {
			// tranform 4 elements
			__m256i vptnextlon, vptnextlat;

			// transform next
			{
				__m256i vplon = _mm256_load_si256((const __m256i *)&poly.PolygonSimple::base_t::operator[](i + 4));
				//_mm_prefetch(&poly.PolygonSimple::base_t::operator[](i + 8), _MM_HINT_T0);
				vplon = _mm256_sub_epi32(vplon, vorigin);
				__m256i vplat = _mm256_srli_epi64(vplon, 32);
				vptnextlon = _mm256_add_epi64(_mm256_mul_epi32(vvectorlon, vplon), _mm256_mul_epi32(vvectorlat, vplat));
				vptnextlat = _mm256_sub_epi64(_mm256_mul_epi32(vvectorlon, vplat), _mm256_mul_epi32(vvectorlat, vplon));
			}

			__m256i vzero = _mm256_setzero_si256();

			if (false) {
				for (unsigned int j = 0; j < 4; ++j)
					std::cerr << "pt[" << (i + j) << "]: " << vextr_epi64(vptlat, j) << ',' << vextr_epi64(vptlon, j)
						  << " (prev " << vextr_epi64(vptprevlat, j) << ',' << vextr_epi64(vptprevlon, j) << ')' << std::endl;
				std::ostringstream oss;
				oss << std::hex << "horizcrossing: 0x" << std::setw(8) << std::setfill('0')
				    << _mm256_movemask_epi8(_mm256_and_si256(_mm256_cmpeq_epi64(vptlat, vzero), _mm256_cmpeq_epi64(vptprevlat, vzero)))
				    << std::endl << "upward crossing: 0x" << std::setw(8) << std::setfill('0')
				    << _mm256_movemask_epi8(_mm256_andnot_si256(_mm256_cmpgt_epi64(vptprevlat, vzero), _mm256_cmpgt_epi64(vptlat, vzero)))
				    << std::endl << "downward crossing: 0x" << std::setw(8) << std::setfill('0')
				    << _mm256_movemask_epi8(_mm256_andnot_si256(_mm256_cmpgt_epi64(vptlat, vzero), _mm256_cmpgt_epi64(vptprevlat, vzero)))
				    << std::endl;
				std::cerr << oss.str();
			}
			uint32_t condh, condu, condd;
			{
				__m256i vl = _mm256_cmpgt_epi64(vptlat, vzero);
				__m256i vpl = _mm256_cmpgt_epi64(vptprevlat, vzero);
				condh = _mm256_movemask_epi8(_mm256_and_si256(_mm256_cmpeq_epi64(vptlat, vzero), _mm256_cmpeq_epi64(vptprevlat, vzero)));
				condu = _mm256_movemask_epi8(_mm256_andnot_si256(vpl, vl));
				condd = _mm256_movemask_epi8(_mm256_andnot_si256(vl, vpl));
			}
			avx2_horizontal(wind, dir, vptprevlat, vptprevlon, vptlat, vptlon, condh, 4, i);
			avx2_upward(wind, dir, vptprevlat, vptprevlon, vptlat, vptlon, condu, 4, i);
			avx2_downward(wind, dir, vptprevlat, vptprevlon, vptlat, vptlon, condd, 4, i);
			vptprevlon = vptlon;
			vptprevlat = vptlat;
			vptlon = vptnextlon;
			vptlat = vptnextlat;
			// vptprevxxx = vptxxx[2:0] vptprevxxx[3]
			vptprevlon = _mm256_blend_epi32(_mm256_permute4x64_epi64(vptlon, 0x90), _mm256_permute4x64_epi64(vptprevlon, 0xff), 0x03);
			vptprevlat = _mm256_blend_epi32(_mm256_permute4x64_epi64(vptlat, 0x90), _mm256_permute4x64_epi64(vptprevlat, 0xff), 0x03);
		}
		// last iteration
		{
			static const uint32_t masks[4] = {
				0x00000080, 0x00008080, 0x00808080, 0x80808080
			};

			unsigned int cnt = sz - i;
			uint32_t mask = masks[cnt - 1];

			__m256i vzero = _mm256_setzero_si256();

			if (false) {
				for (unsigned int j = 0; j < cnt; ++j)
					std::cerr << "pt[" << (i + j) << "]: " << vextr_epi64(vptlat, j) << ',' << vextr_epi64(vptlon, j)
						  << " (prev " << vextr_epi64(vptprevlat, j) << ',' << vextr_epi64(vptprevlon, j) << ')' << std::endl;
				std::ostringstream oss;
				oss << std::hex << "horizcrossing: 0x" << std::setw(8) << std::setfill('0')
				    << _mm256_movemask_epi8(_mm256_and_si256(_mm256_cmpeq_epi64(vptlat, vzero), _mm256_cmpeq_epi64(vptprevlat, vzero)))
				    << std::endl << "upward crossing: 0x" << std::setw(8) << std::setfill('0')
				    << _mm256_movemask_epi8(_mm256_andnot_si256(_mm256_cmpgt_epi64(vptprevlat, vzero), _mm256_cmpgt_epi64(vptlat, vzero)))
				    << std::endl << "downward crossing: 0x" << std::setw(8) << std::setfill('0')
				    << _mm256_movemask_epi8(_mm256_andnot_si256(_mm256_cmpgt_epi64(vptlat, vzero), _mm256_cmpgt_epi64(vptprevlat, vzero)))
				    << std::endl;
				std::cerr << oss.str();
			}
			uint32_t condh, condu, condd;
			{
				__m256i vl = _mm256_cmpgt_epi64(vptlat, vzero);
				__m256i vpl = _mm256_cmpgt_epi64(vptprevlat, vzero);
				condh = _mm256_movemask_epi8(_mm256_and_si256(_mm256_cmpeq_epi64(vptlat, vzero), _mm256_cmpeq_epi64(vptprevlat, vzero)));
				condu = _mm256_movemask_epi8(_mm256_andnot_si256(vpl, vl));
				condd = _mm256_movemask_epi8(_mm256_andnot_si256(vl, vpl));
			}
			condh &= mask;
			condu &= mask;
			condd &= mask;
			avx2_horizontal(wind, dir, vptprevlat, vptprevlon, vptlat, vptlon, condh, cnt, i);
			avx2_upward(wind, dir, vptprevlat, vptprevlon, vptlat, vptlon, condu, cnt, i);
			avx2_downward(wind, dir, vptprevlat, vptprevlon, vptlat, vptlon, condd, cnt, i);
		}
		return;
	}
	unsigned int j = sz - 1;
	if (m_simd == simd_avx) {
		__m128i vorigin = _mm_set1_epi64(*(const __m64 *)&m_origin);
		__m128i vvector = _mm_set1_epi64(*(const __m64 *)&m_vector);
		vvector = _mm_shuffle_epi32(vvector, 0x50);
		__m128i vvectorswapped = _mm_shuffle_epi32(vvector, 0x0a);
		tpoint_t pti, ptj;
		{
			__m128i v0 = _mm_set1_epi64(*(const __m64 *)&poly.PolygonSimple::base_t::operator[](j));
			v0 = _mm_sub_epi32(v0, vorigin);
			v0 = _mm_shuffle_epi32(v0, 0x50);
			__m128i m0 = _mm_mul_epi32(v0, vvector);
			__m128i m1 = _mm_mul_epi32(v0, vvectorswapped);
			ptj.first = _mm_extract_epi64(m0, 1) + _mm_extract_epi64(m0, 0);
			ptj.second = _mm_extract_epi64(m1, 1) - _mm_extract_epi64(m1, 0);
		}
		for (unsigned int i = 0; i < sz; j = i, ptj = pti, ++i) {
			{
				__m128i v0 = _mm_set1_epi64(*(const __m64 *)&poly.PolygonSimple::base_t::operator[](i));
				v0 = _mm_sub_epi32(v0, vorigin);
				v0 = _mm_shuffle_epi32(v0, 0x50);
				__m128i m0 = _mm_mul_epi32(v0, vvector);
				__m128i m1 = _mm_mul_epi32(v0, vvectorswapped);
				pti.first = _mm_extract_epi64(m0, 1) + _mm_extract_epi64(m0, 0);
				pti.second = _mm_extract_epi64(m1, 1) - _mm_extract_epi64(m1, 0);
			}
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
							  << (wi->second - dir) << "->" << wi->second << " i " << i << std::endl;
				} else if (!pti.second && !ptj.second) {
					/* a horizontal crossing on line */
					winding_t::iterator wi0(wind.insert(winding_t::value_type(ptj.first, 0)).first);
					wi0->second += dir << 24;
					winding_t::iterator wi1(wind.insert(winding_t::value_type(pti.first, 0)).first);
					wi1->second -= dir << 24;
					if (debug)
						std::cerr << "InsideHelper: line 0.." << m_end << " horizontal edge " << ptj.first << ',' << ptj.second
							  << "..." << pti.first << ',' << pti.second << " x0 " << wi0->first << " w0 "
							  << (wi0->second - dir) << "->" << wi0->second << " x1 " << wi1->first << " w1 "
							  << (wi1->second + dir) << "->" << wi1->second << " i " << i << std::endl;
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
							  << (wi->second + dir) << "->" << wi->second << " i " << i << std::endl;
				}
			}
		}
		return;
	}
#endif
	tpoint_t pti, ptj(transform(poly.PolygonSimple::base_t::operator[](j)));
        for (unsigned int i = 0; i < sz; j = i, ptj = pti, ++i) {
		pti = transform(poly.PolygonSimple::base_t::operator[](i));
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
						  << (wi->second - dir) << "->" << wi->second << " i " << i << std::endl;
			} else if (!pti.second && !ptj.second) {
				/* a horizontal crossing on line */
				winding_t::iterator wi0(wind.insert(winding_t::value_type(ptj.first, 0)).first);
				wi0->second += dir << 24;
				winding_t::iterator wi1(wind.insert(winding_t::value_type(pti.first, 0)).first);
				wi1->second -= dir << 24;
				if (debug)
					std::cerr << "InsideHelper: line 0.." << m_end << " horizontal edge " << ptj.first << ',' << ptj.second
						  << "..." << pti.first << ',' << pti.second << " x0 " << wi0->first << " w0 "
						  << (wi0->second - dir) << "->" << wi0->second << " x1 " << wi1->first << " w1 "
						  << (wi1->second + dir) << "->" << wi1->second << " i " << i << std::endl;
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
						  << (wi->second + dir) << "->" << wi->second << " i " << i << std::endl;
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
	limit(x, y);
}
