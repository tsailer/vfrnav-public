/*****************************************************************************/

/*
 *      ssetest.cc  --  Test SSE function implementations.
 *
 *      Copyright (C) 2015  Thomas Sailer (t.sailer@alumni.ethz.ch)
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*****************************************************************************/

#include "geom.h"
#include <iostream>
#include <iomanip>
#include <sys/time.h>
#include <stdlib.h>

typedef std::pair<int64_t,int64_t> tpoint_t;

static tpoint_t transform1(const Point& pt, const Point& porigin, const Point& pvec)
{
	Point p(pt - porigin);
	int64_t a(pvec.get_lon()), b(pvec.get_lat()), pa(p.get_lon()), pb(p.get_lat());
	return tpoint_t(a * pa + b * pb, a * pb - b * pa);
}

#ifdef __AVX__
static tpoint_t transform2(const Point& pt, const Point& porigin, const Point& pvec)
{
	__m128i vpt = _mm_set1_epi64(*(const __m64 *)&pt);
	__m128i vporigin = _mm_set1_epi64(*(const __m64 *)&porigin);
	__m128i vpvec = _mm_set1_epi64(*(const __m64 *)&pvec);
	__m128i vp = _mm_sub_epi32(vpt, vporigin);
	vp = _mm_shuffle_epi32(vp, 0x50);
	vpvec = _mm_shuffle_epi32(vpvec, 0x50);
	__m128i m0 = _mm_mul_epi32(vp, vpvec);
	__m128i m1 = _mm_mul_epi32(_mm_shuffle_epi32(vp, 0x0a), vpvec);
	return tpoint_t(_mm_extract_epi64(m0, 0) + _mm_extract_epi64(m0, 1),
			_mm_extract_epi64(m1, 0) - _mm_extract_epi64(m1, 1));
}
#else
static tpoint_t transform2(const Point& pt, const Point& porigin, const Point& pvec)
{
	return transform1(pt, porigin, pvec);
}
#endif

static void testdirtransform(void)
{
	std::vector<Point> pt, porigin, pvec;
	std::vector<tpoint_t> res1, res2;
	pt.push_back(Point(1, 2));
	porigin.push_back(Point(3, 4));
	pvec.push_back(Point(5, 6));
	for (unsigned int i = 1; i < 262144; ++i) {
		pt.push_back(Point(random(), random()));
		porigin.push_back(Point(random(), random()));
		pvec.push_back(Point(random(), random()));
	}
	std::vector<Point>::size_type n(pt.size());
	res1.resize(n);
	res2.resize(n);
	struct timeval tv1, tv2, tv3;
	gettimeofday(&tv1, 0);
	for (std::vector<Point>::size_type i(0); i < n; ++i)
		res1[i] = transform1(pt[i], porigin[i], pvec[i]);
	gettimeofday(&tv2, 0);
	for (std::vector<Point>::size_type i(0); i < n; ++i)
		res2[i] = transform2(pt[i], porigin[i], pvec[i]);
	gettimeofday(&tv3, 0);
	for (std::vector<Point>::size_type i(0); i < n; ++i) {
		if (res1[i].first == res2[i].first &&
		    res1[i].second == res2[i].second)
			continue;
		std::cerr << "Error: " << pt[i].get_lon() << ',' << pt[i].get_lat() << ' '
			  << porigin[i].get_lon() << ',' << porigin[i].get_lat() << ' '
			  << pvec[i].get_lon() << ',' << pvec[i].get_lat() << " -> "
			  << res1[i].first << ',' << res1[i].second << " != "
			  << res2[i].first << ',' << res2[i].second << std::endl;
	}
	std::cout << "Direct Transform Timings: classical " << ((tv2.tv_sec - tv1.tv_sec) * 1000000 + tv2.tv_usec - tv1.tv_usec)
		  << "us, vector " << ((tv3.tv_sec - tv2.tv_sec) * 1000000 + tv3.tv_usec - tv2.tv_usec) << "us"
		  << std::endl;
}

static void testtransform(void)
{
	std::vector<Point> p0, p1;
	std::vector<std::vector<Point> > pt;
	std::vector<tpoint_t> res1, res2;
	for (unsigned int i = 0; i < 1024; ++i) {
		p0.push_back(Point(random(), random()));
		p1.push_back(Point(random(), random()));
		pt.push_back(std::vector<Point>());
		for (unsigned int j = 0; j < 8192; ++j)
			pt.back().push_back(Point(random(), random()));
	}
	{
		PolygonSimple::InsideHelper h0(p0[0], p1[0], false);
		PolygonSimple::InsideHelper h1(p0[0], p1[0], true);
		if (h0.is_simd() || !h1.is_simd()) {
			std::cerr << "SIMD switching does not work" << std::endl;
			return;
		}
	}
	std::vector<Point>::size_type n(pt.size());
	std::vector<std::vector<Point> >::size_type m(pt.front().size());
	res1.resize(n * m);
	res2.resize(n * m);
	struct timeval tv1, tv2, tv3;
	gettimeofday(&tv1, 0);
	for (std::vector<Point>::size_type i(0); i < n; ++i) {
		PolygonSimple::InsideHelper h(p0[i], p1[i], false);
		tpoint_t *resp(&res1[i * m]);
		const Point *ptp(&pt[i][0]);
		for (std::vector<std::vector<Point> >::size_type j(0); j < m; ++j, ++resp, ++ptp)
			*resp = h.transform(*ptp);
	}
	gettimeofday(&tv2, 0);
	for (std::vector<Point>::size_type i(0); i < n; ++i) {
		PolygonSimple::InsideHelper h(p0[i], p1[i], true);
		tpoint_t *resp(&res2[i * m]);
		const Point *ptp(&pt[i][0]);
		for (std::vector<std::vector<Point> >::size_type j(0); j < m; ++j, ++resp, ++ptp)
			*resp = h.transform(*ptp);
	}
	gettimeofday(&tv3, 0);
	for (std::vector<Point>::size_type i(0); i < n * m; ++i) {
		if (res1[i].first == res2[i].first &&
		    res1[i].second == res2[i].second)
			continue;
		std::cerr << "Error: " << p0[i / m].get_lon() << ',' << p0[i / m].get_lat() << ' '
			  << p1[i / m].get_lon() << ',' << p1[i / m].get_lat() << ' '
			  << pt[i / m][i % m].get_lon() << ',' << pt[i / m][i % m].get_lat() << " -> "
			  << res1[i].first << ',' << res1[i].second << " != "
			  << res2[i].first << ',' << res2[i].second << std::endl;
	}
	std::cout << "Transform Timings: classical " << ((tv2.tv_sec - tv1.tv_sec) * 1000000 + tv2.tv_usec - tv1.tv_usec)
		  << "us, vector " << ((tv3.tv_sec - tv2.tv_sec) * 1000000 + tv3.tv_usec - tv2.tv_usec) << "us"
		  << std::endl;
}

static void testwinding(void)
{
	std::vector<Point> p0, p1;
	std::vector<PolygonSimple> ps;
	std::vector<IntervalSet<int64_t> > res1, res2;
	for (unsigned int i = 0; i < 1024; ++i) {
		p0.push_back(Point(random(), random()));
		p1.push_back(Point(random(), random()));
		ps.push_back(PolygonSimple());
		for (unsigned int j = 0; j < 256; ++j)
			ps.back().push_back(Point(random(), random()));
	}
	{
		PolygonSimple::InsideHelper h0(p0[0], p1[0], false);
		PolygonSimple::InsideHelper h1(p0[0], p1[0], true);
		if (h0.is_simd() || !h1.is_simd()) {
			std::cerr << "SIMD switching does not work" << std::endl;
			return;
		}
	}
	std::vector<Point>::size_type n(ps.size());
	res1.resize(n);
	res2.resize(n);
	struct timeval tv1, tv2, tv3;
	gettimeofday(&tv1, 0);
	for (std::vector<Point>::size_type i(0); i < n; ++i) {
		PolygonSimple::InsideHelper h(p0[i], p1[i], false);
		h |= ps[i];
		res1[i] = h;
	}
	gettimeofday(&tv2, 0);
	for (std::vector<Point>::size_type i(0); i < n; ++i) {
		PolygonSimple::InsideHelper h(p0[i], p1[i], true);
		h |= ps[i];
		res2[i] = h;
	}
	gettimeofday(&tv3, 0);
	for (std::vector<Point>::size_type i(0); i < n; ++i) {
		if (res1[i] == res2[i])
			continue;
		std::cerr << "Error: " << p0[i].get_lon() << ',' << p0[i].get_lat() << ' '
			  << p1[i].get_lon() << ',' << p1[i].get_lat() << ' '
			  << res1[i].to_str() << " != " << res2[i].to_str() << std::endl;
	}
	std::cout << "Winding Timings: classical " << ((tv2.tv_sec - tv1.tv_sec) * 1000000 + tv2.tv_usec - tv1.tv_usec)
		  << "us, vector " << ((tv3.tv_sec - tv2.tv_sec) * 1000000 + tv3.tv_usec - tv2.tv_usec) << "us"
		  << std::endl;
}

int main(int argc, char *argv[])
{
#if !defined(__GNUC__) || !defined(__GNUC_MINOR__) || (__GNUC__ < 4) || (__GNUC__ == 4 && __GNUC_MINOR__ < 8)
	std::cerr << "Compiler too old" << std::endl;
	return 1;
#elif defined(__AVX__)
	if (!__builtin_cpu_supports("avx")) {
		std::cerr << "CPU does not support AVX" << std::endl;
		return 1;
	}
#else
	std::cerr << "Compiler does not support AVX" << std::endl;
	return 1;
#endif
	testdirtransform();
	testtransform();
	testwinding();
	return 0;
}
