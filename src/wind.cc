/*****************************************************************************/

/*
 *      wind.c  --  Wind Triangle related Computations.
 *
 *      Copyright (C) 2012, 2013, 2014, 2017  Thomas Sailer (t.sailer@alumni.ethz.ch)
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

#include "wind.h"
#include "geom.h"
#include <stdexcept>
#include <limits>
#include <iostream>

template <typename T> constexpr typename Wind<T>::float_t Wind<T>::to_rad;
template <typename T> constexpr typename Wind<T>::float_t Wind<T>::from_rad;

template <typename T> Wind<T>::Wind(void)
	: m_tas(std::numeric_limits<float_t>::quiet_NaN()),
	  m_gs(std::numeric_limits<float_t>::quiet_NaN()),
	  m_hdg(std::numeric_limits<float_t>::quiet_NaN()),
	  m_crs(std::numeric_limits<float_t>::quiet_NaN()),
	  m_winddir(std::numeric_limits<float_t>::quiet_NaN()),
	  m_windspeed(std::numeric_limits<float_t>::quiet_NaN())
{
}

template <> inline void Wind<double>::mysincos(float_t x, float_t *sin, float_t *cos)
{
	sincos(x, sin, cos);
}

template <> inline void Wind<float>::mysincos(float_t x, float_t *sin, float_t *cos)
{
	sincosf(x, sin, cos);
}

template <> inline void Wind<long double>::mysincos(float_t x, float_t *sin, float_t *cos)
{
	sincosl(x, sin, cos);
}

template <typename T> void Wind<T>::set_wind(float_t winddir, float_t windspeed)
{
	m_winddir = winddir;
	m_windspeed = windspeed;
}

template <typename T> void Wind<T>::set_wind(float_t winddir0, float_t windspeed0, float_t winddir1, float_t windspeed1)
{
	float_t x0, y0, x1, y1;
	mysincos(winddir0 * to_rad, &x0, &y0);
	mysincos(winddir1 * to_rad, &x1, &y1);
	x0 *= windspeed0;
	y0 *= windspeed0;
	x1 *= windspeed1;
	y1 *= windspeed1;
	x0 += x1;
	y0 += y1;
	x0 *= 0.5;
	y0 *= 0.5;
	m_windspeed = sqrt(x0 * x0 + y0 * y0);
	m_winddir = atan2(x0, y0) * from_rad;
	if (m_winddir < 0)
		m_winddir += 360;
	else if (m_winddir >= 360)
		m_winddir -= 360;
}

template <typename T> void Wind<T>::set_wind(float_t winddir0, float_t windspeed0, float_t winddir1, float_t windspeed1, float_t frac)
{
	if (std::isnan(frac))
		frac = 0.5;
	frac = std::min(std::max(frac, (float_t)0), (float_t)1);
	float_t x0, y0, x1, y1;
	mysincos(winddir0 * to_rad, &x0, &y0);
	mysincos(winddir1 * to_rad, &x1, &y1);
	{
		float_t x(windspeed0 * (1 - frac));
		x0 *= x;
		y0 *= x;
	}
	{
		float_t x(windspeed1 * frac);
		x1 *= x;
		y1 *= x;
	}
	x0 += x1;
	y0 += y1;
	m_windspeed = sqrt(x0 * x0 + y0 * y0);
	m_winddir = atan2(x0, y0) * from_rad;
	if (m_winddir < 0)
		m_winddir += 360;
	else if (m_winddir >= 360)
		m_winddir -= 360;
}

template <typename T> void Wind<T>::set_hdg_tas(float_t hdg, float_t tas)
{
	m_tas = tas;
	m_hdg = hdg;
	float_t x, y;
        mysincos((m_winddir - m_hdg) * to_rad, &x, &y);
	x *= m_windspeed;
	y *= m_windspeed;
	y = m_tas - y;
	m_gs = sqrt(x * x + y * y);
	m_crs = m_hdg - atan2(x, y) * from_rad;
	if (m_crs < 0)
		m_crs += 360;
	else if (m_crs >= 360)
		m_crs -= 360;
}

template <typename T> void Wind<T>::set_crs_gs(float_t crs, float_t gs)
{
	m_gs = gs;
	m_crs = crs;
	float_t x, y;
        mysincos((m_winddir - m_crs) * to_rad, &x, &y);
	x *= m_windspeed;
	y *= m_windspeed;
	y += m_gs;
	m_tas = sqrt(x * x + y * y);
	m_hdg = m_crs + atan2(x, y) * from_rad;
	if (m_hdg < 0)
		m_hdg += 360;
	else if (m_hdg >= 360)
		m_hdg -= 360;
}

template <typename T> void Wind<T>::set_crs_tas(float_t crs, float_t tas)
{
	m_crs = crs;
	m_tas = tas;
	float_t x, y;
        mysincos((m_winddir - m_crs) * to_rad, &x, &y);
	x *= m_windspeed;
	y *= m_windspeed;
	float_t y1(m_tas * m_tas - x * x);
	if (y1 < 0) {
		m_gs = m_hdg = std::numeric_limits<float_t>::quiet_NaN();
		return;
	}
	y1 = sqrt(y1);
	m_gs = y1 - y;
	m_hdg = m_crs + atan2(x, y1) * from_rad;
}

template <typename T> void Wind<T>::set_hdg_tas_crs_gs(float_t hdg, float_t tas, float_t crs, float_t gs)
{
	m_tas = tas;
	m_gs = gs;
	m_hdg = hdg;
	m_crs = crs;
	m_winddir = m_windspeed = std::numeric_limits<float_t>::quiet_NaN();
	if (std::isnan(m_hdg) || std::isnan(m_tas) || std::isnan(m_crs) || std::isnan(m_gs))
                return;
        float_t x, y;
        mysincos((m_crs - m_hdg) * to_rad, &y, &x);
        x *= -m_gs;
        y *= -m_gs;
        x += m_tas;
        if (false)
                std::cerr << "set_hdg_tas_crs_gs: HDG " << hdg << " TAS " << tas << " CRS " << crs << " GS " << gs
                          << " X " << x << " Y " << y << std::endl;
        float_t d(x * x + y * y);
        m_windspeed = sqrt(d);
        if (d <= 0) {
                m_winddir = 0;
                return;
        }
        d = atan2(y, x);
        if (std::isnan(d))
                return;
	d = d * from_rad + hdg;
        while (d >= 360)
                d -= 360;
        while (d < 0)
                d += 360;
        m_winddir = d;
}

template <typename T> void Wind<T>::set_wind_grib_mps(float_t wu, float_t wv)
{
	// convert from m/s to kts
        wu *= (-1e-3f * Point::km_to_nmi * 3600);
        wv *= (-1e-3f * Point::km_to_nmi * 3600);
        set_wind((M_PI - atan2f(wu, wv)) * from_rad, sqrtf(wu * wu + wv * wv));
}

template <typename T> void Wind<T>::set_wind_grib_mps(const gribwind_t& w)
{
	set_wind_grib_mps(w.first, w.second);
}

template class Wind<float>;
template class Wind<double>;
