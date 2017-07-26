/*****************************************************************************/

/*
 *      baro.cc  --  ICAO/U.S. Standard Atmosphere 1976 computations.
 *
 *      Copyright (C) 2003, 2007, 2012, 2016  Thomas Sailer (t.sailer@alumni.ethz.ch)
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

#include "baro.h"
#include <cmath>
#include <sstream>
#include <stdexcept>

template<typename T> constexpr typename IcaoAtmosphere<T>::float_t IcaoAtmosphere<T>::const_g;                     /* gravity const. */
template<typename T> constexpr typename IcaoAtmosphere<T>::float_t IcaoAtmosphere<T>::const_m;                     /* Molecular Weight of air */
template<typename T> constexpr typename IcaoAtmosphere<T>::float_t IcaoAtmosphere<T>::const_r;                     /* gas const. for air */
template<typename T> constexpr typename IcaoAtmosphere<T>::float_t IcaoAtmosphere<T>::const_l0;                    /* Lapse rate first layer */
template<typename T> constexpr typename IcaoAtmosphere<T>::float_t IcaoAtmosphere<T>::const_gmr = (const_g*const_m/const_r);   /* Hydrostatic constant */
template<typename T> constexpr typename IcaoAtmosphere<T>::float_t IcaoAtmosphere<T>::degc_to_kelvin;
template<typename T> constexpr typename IcaoAtmosphere<T>::float_t IcaoAtmosphere<T>::std_sealevel_temperature;
template<typename T> constexpr typename IcaoAtmosphere<T>::float_t IcaoAtmosphere<T>::std_sealevel_pressure;
template<typename T> constexpr typename IcaoAtmosphere<T>::float_t IcaoAtmosphere<T>::std_sealevel_density = const_m / const_r * std_sealevel_pressure / std_sealevel_temperature;

template<typename T> const unsigned int IcaoAtmosphere<T>::nr_layers;
template<typename T> const typename IcaoAtmosphere<T>::AtmospericLayer IcaoAtmosphere<T>::atmosperic_layers[nr_layers+1] = {
	{     0, -const_l0 },
	{ 11000,    0 },
	{ 20000,  1.0/1000 },
	{ 32000,  2.8/1000 },
	{ 47000,    0 },
	{ 51000, -2.8/1000 },
	{ 71000, -2.0/1000 },
	{ 84852,    0 }
};

template<typename T>
IcaoAtmosphere<T>::IcaoAtmosphere(float_t qnh, float_t temp)
{
	set(qnh, temp);
}

template<typename T>
void IcaoAtmosphere<T>::compute_boundaries(void)
{
	m_bdry[0].rho = pressure_to_density(m_bdry[0].p, m_bdry[0].t);
	for (unsigned int i = 0; i < nr_layers; ++i) {
		if (atmosperic_layers[i].lapserate) {
			m_bdry[i+1].t = m_bdry[i].t + atmosperic_layers[i].lapserate * (atmosperic_layers[i+1].limit - atmosperic_layers[i].limit);
			//m_bdry[i+1].p = m_bdry[i].p * exp(log(m_bdry[i].t / m_bdry[i+1].t) * (const_gmr / 1000) / atmosperic_layers[i].lapserate);
			m_bdry[i+1].p = m_bdry[i].p * pow(m_bdry[i].t / m_bdry[i+1].t, (const_gmr / 1000) / atmosperic_layers[i].lapserate);
		} else {
			m_bdry[i+1].t = m_bdry[i].t;
			m_bdry[i+1].p = m_bdry[i].p * exp((-const_gmr / 1000) * (atmosperic_layers[i+1].limit - atmosperic_layers[i].limit) / m_bdry[i].t);
		}
		m_bdry[i+1].rho = pressure_to_density(m_bdry[i+1].p, m_bdry[i+1].t);
	}
}

template<typename T>
void IcaoAtmosphere<T>::set(float_t qnh, float_t temp)
{
	m_bdry[0].t = temp;
	m_bdry[0].p = qnh;
	compute_boundaries();
}

template<typename T>
void IcaoAtmosphere<T>::altitude_to_pressure(float_t *press, float_t *temp, float_t alt, const atmospheric_boundary b[nr_layers+1])
{
	if (false && alt < 0)
		throw std::runtime_error("IcaoAtmosphere::altitude_to_pressure: altitude negative");
	unsigned int i;
	for (i = 0; i < nr_layers; i++)
		if (alt <= atmosperic_layers[i+1].limit)
			break;
	if (i >= nr_layers) {
		std::ostringstream oss;
		oss << "IcaoAtmosphere::altitude_to_pressure: altitude " << alt << " above model limit (QNH "
		    << b[0].p << " ISAoffs " << (b[0].t - std_sealevel_temperature) << ')';
		throw std::runtime_error(oss.str());
	}
	//std::cerr << "Alt " << alt << " layer " << i << std::endl;
	float_t t = b[i].t + atmosperic_layers[i].lapserate * (alt - atmosperic_layers[i].limit);
	if (temp)
		*temp = t;
	if (!press)
		return;
	if (atmosperic_layers[i].lapserate) {
		*press = b[i].p * pow(b[i].t / t, (const_gmr / 1000) / atmosperic_layers[i].lapserate);
		return;
	}
	*press = b[i].p * exp((-const_gmr / 1000) * (alt - atmosperic_layers[i].limit) / b[i].t);
}

template<typename T>
void IcaoAtmosphere<T>::pressure_to_altitude(float_t *alt, float_t *temp, float_t press, const atmospheric_boundary b[nr_layers+1])
{
	if (/*press > b[0].p ||*/ press <= 0)
		throw std::runtime_error("IcaoAtmosphere::pressure_to_altitude: pressure below zero");
	unsigned int i;
	for (i = 0; i < nr_layers; i++)
		if (press > b[i+1].p)
			break;
	if (i >= nr_layers)
		throw std::runtime_error("IcaoAtmosphere::pressure_to_altitude: pressure lower than model limit");
	//std::cerr << "Pressure " << press << " layer " << i << std::endl;
	float_t a;
	if (atmosperic_layers[i].lapserate) {
		a = (b[i].t / pow(press / b[i].p, atmosperic_layers[i].lapserate * (1000 / const_gmr)) - b[i].t);
		a /= atmosperic_layers[i].lapserate;
		a += atmosperic_layers[i].limit;
	} else {
		a = log(press / b[i].p) * b[i].t * (-1000 / const_gmr) + atmosperic_layers[i].limit;
	}
	if (alt)
		*alt = a;
	if (temp)
		*temp = b[i].t + atmosperic_layers[i].lapserate * (a - atmosperic_layers[i].limit);
}

template<typename T>
typename IcaoAtmosphere<T>::float_t IcaoAtmosphere<T>::altitude_to_density(float_t alt, const atmospheric_boundary b[nr_layers+1])
{
	if (false && alt < 0)
		throw std::runtime_error("IcaoAtmosphere::altitude_to_density: altitude negative");
	unsigned int i;
	for (i = 0; i < nr_layers; i++)
		if (alt <= atmosperic_layers[i+1].limit)
			break;
	if (i >= nr_layers)
		throw std::runtime_error("IcaoAtmosphere::altitude_to_density: altitude above model limit");
	if (atmosperic_layers[i].lapserate)
		return b[i].p * pow(b[i].t, (const_gmr / 1000) / atmosperic_layers[i].lapserate) * (const_m / const_r)
			* pow(b[i].t + atmosperic_layers[i].lapserate * (alt - atmosperic_layers[i].limit),
			      (-const_gmr / 1000) / atmosperic_layers[i].lapserate - 1);
	return	b[i].p / b[i].t * (const_m / const_r) * exp((-const_gmr / 1000) / b[i].t * (alt - atmosperic_layers[i].limit));
}

template<typename T>
typename IcaoAtmosphere<T>::float_t IcaoAtmosphere<T>::density_to_altitude(float_t density, const atmospheric_boundary b[nr_layers+1])
{
	if (/*density > b[0].rho ||*/ density <= 0)
		throw std::runtime_error("IcaoAtmosphere::density_to_altitude: density below zero");
	unsigned int i;
	for (i = 0; i < nr_layers; i++)
		if (density > b[i+1].rho)
			break;
	if (i >= nr_layers)
		throw std::runtime_error("IcaoAtmosphere::density_to_altitude: density lower than model limit");
	if (atmosperic_layers[i].lapserate) {
		float_t x(b[i].p * pow(b[i].t, (const_gmr / 1000) / atmosperic_layers[i].lapserate) * (const_m / const_r) / density);
		x = exp(log(x) / ((const_gmr / 1000) / atmosperic_layers[i].lapserate + 1));
		x -= b[i].t;
		x /= atmosperic_layers[i].lapserate;
		x += atmosperic_layers[i].limit;
		return x;
	}
	return (-1000 / const_gmr) * b[i].t * log(b[i].t / b[i].p * (const_r / const_m) * density) + atmosperic_layers[i].limit;
}

template<typename T>
typename IcaoAtmosphere<T>::float_t IcaoAtmosphere<T>::pressure_to_density(float_t pressure, float_t temp)
{
	static constexpr float_t mintemp = 0.01;
	return (const_m / const_r) * pressure / std::max(temp, mintemp);
}

template<typename T>
typename IcaoAtmosphere<T>::float_t IcaoAtmosphere<T>::density_to_pressure(float_t density, float_t temp)
{
	return density * temp * (const_r / const_m);
}

template class IcaoAtmosphere<float>;
template class IcaoAtmosphere<double>;
