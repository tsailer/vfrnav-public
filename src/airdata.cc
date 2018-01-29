/*****************************************************************************/

/*
 *      airdata.c  --  Air Data related Computations.
 *
 *      Copyright (C) 2012, 2014, 2016  Thomas Sailer (t.sailer@alumni.ethz.ch)
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

#include "airdata.h"
#include "fplan.h"
#include <cmath>
#include <stdexcept>
#include <limits>

template<typename T> constexpr typename AirData<T>::float_t AirData<T>::gamma;
template<typename T> constexpr typename AirData<T>::float_t AirData<T>::speed_of_sound_0degc;

template<typename T>
AirData<T>::AirData(void)
	: m_pressalt(std::numeric_limits<float_t>::quiet_NaN()),
	  m_truealt(std::numeric_limits<float_t>::quiet_NaN()),
	  m_densityalt(std::numeric_limits<float_t>::quiet_NaN()),
	  m_p(std::numeric_limits<float_t>::quiet_NaN()),
	  m_q(std::numeric_limits<float_t>::quiet_NaN()),
	  m_cas(std::numeric_limits<float_t>::quiet_NaN()),
	  m_eas(std::numeric_limits<float_t>::quiet_NaN()),
	  m_tas(std::numeric_limits<float_t>::quiet_NaN()),
	  m_mach(std::numeric_limits<float_t>::quiet_NaN()),
	  m_ct(1.0),
	  m_temp(std::numeric_limits<float_t>::quiet_NaN()),
	  m_tempstd(std::numeric_limits<float_t>::quiet_NaN())
{
}

template<typename T>
void AirData<T>::set_qnh(float_t qnh)
{
	m_atmosphere.set(qnh, get_temp());
	m_densityalt = std::numeric_limits<float_t>::quiet_NaN();
}

template<typename T>
void AirData<T>::set_qnh_temp(float_t qnh, float_t temp)
{
	m_atmosphere.set(qnh, temp);
	set_pressure_altitude(m_pressalt);
}

template<typename T>
typename AirData<T>::float_t AirData<T>::true_to_pressure_altitude(float_t truealt) const
{
	if (std::isnan(truealt))
		return truealt;
	float_t press, alt;
	m_atmosphere.altitude_to_pressure(&press, 0, truealt * (float_t)Point::ft_to_m_dbl);
	m_atmosphere.std_pressure_to_altitude(&alt, 0, press);
	return alt * (float_t)Point::m_to_ft_dbl;
}

template<typename T>
typename AirData<T>::float_t AirData<T>::pressure_to_true_altitude(float_t baroalt) const
{
	if (std::isnan(baroalt))
		return baroalt;
	float_t press, alt;
	m_atmosphere.std_altitude_to_pressure(&press, 0, baroalt * (float_t)Point::ft_to_m_dbl);
	m_atmosphere.pressure_to_altitude(&alt, 0, press);
	return alt * (float_t)Point::m_to_ft_dbl;
}

template<typename T>
void AirData<T>::set_pressure_altitude(float_t pressalt)
{
	m_densityalt = std::numeric_limits<float_t>::quiet_NaN();
	if (std::isnan(pressalt)) {
		m_pressalt = m_truealt = m_p = std::numeric_limits<float_t>::quiet_NaN();
		return;
	}
	m_pressalt = pressalt;
	m_atmosphere.std_altitude_to_pressure(&m_p, &m_tempstd, m_pressalt * (float_t)Point::ft_to_m_dbl);
	float_t alt;
	m_atmosphere.pressure_to_altitude(&alt, &m_temp, m_p);
	m_truealt = alt * (float_t)Point::m_to_ft_dbl;
	compute_mach_eas_tas();
}

template<typename T>
void AirData<T>::set_true_altitude(float_t truealt)
{
	m_densityalt = std::numeric_limits<float_t>::quiet_NaN();
	if (std::isnan(truealt)) {
		m_truealt = m_pressalt = m_p = std::numeric_limits<float_t>::quiet_NaN();
		return;
	}
	m_truealt = truealt;
	m_atmosphere.altitude_to_pressure(&m_p, &m_temp, m_truealt * (float_t)Point::ft_to_m_dbl);
	float_t alt;
	m_atmosphere.std_pressure_to_altitude(&alt, &m_tempstd, m_p);
	m_pressalt = alt * (float_t)Point::m_to_ft_dbl;
	compute_mach_eas_tas();
}

template<typename T>
typename AirData<T>::float_t AirData<T>::get_density_altitude(void) const
{
	if (!std::isnan(m_densityalt))
		return m_densityalt;
	if (std::isnan(m_p) || std::isnan(m_temp))
		return std::numeric_limits<float_t>::quiet_NaN();
	float_t density(IcaoAtmosphere<float_t>::pressure_to_density(m_p, m_temp));
	m_densityalt = IcaoAtmosphere<float_t>::std_density_to_altitude(density) * (float_t)Point::m_to_ft_dbl;
	if (false)
		std::cerr << "get_density_altitude: TA " << m_truealt << " PA " << m_pressalt << " P " << m_p
			  << " T " << (m_temp - IcaoAtmosphere<T>::degc_to_kelvin)
			  << " Tstd " << (m_tempstd - IcaoAtmosphere<T>::degc_to_kelvin) << " DA " << m_densityalt
			  << " density " << density << " QNH " << m_atmosphere.get_qnh()
			  << " ISA " << (m_atmosphere.get_temp() - IcaoAtmosphere<T>::std_sealevel_temperature) << std::endl;
	return m_densityalt;
}

template<typename T>
void AirData<T>::set_density_altitude(float_t da)
{
	if (std::isnan(da)) {
		m_truealt = m_pressalt = m_p = m_densityalt = std::numeric_limits<float_t>::quiet_NaN();
		return;
	}
	m_densityalt = da;
	set_true_altitude(m_atmosphere.density_to_altitude(IcaoAtmosphere<float_t>::std_altitude_to_density(da * (float_t)Point::ft_to_m_dbl)) * (float_t)Point::m_to_ft_dbl);
}

template<typename T>
void AirData<T>::set_tatprobe_ct(float_t ct)
{
	static constexpr float_t ctmin = 0.5, ctmax = 1.0;
	if (!std::isnan(ct))
		m_ct = std::max(std::min(ct, ctmax), ctmin);
}

template<typename T>
void AirData<T>::set_oat(float_t oat)
{
	if (std::isnan(oat) || std::isnan(m_tempstd)) {
		m_truealt = m_p = std::numeric_limits<float_t>::quiet_NaN();
		compute_mach_eas_tas();
		return;
	}
	set_qnh_tempoffs(get_qnh(), oat - m_tempstd);
}

template<typename T>
typename AirData<T>::float_t AirData<T>::get_tat_sat_ratio(void) const
{
	if (std::isnan(m_mach))
		return std::numeric_limits<float_t>::quiet_NaN();
	static constexpr float_t gammam1d2 = (gamma - 1) / 2.0;
	return 1 + gammam1d2 * m_ct * m_mach * m_mach;
}

template<typename T>
typename AirData<T>::float_t AirData<T>::get_rat(void) const
{
	if (std::isnan(m_temp) || m_temp <= 0)
		return std::numeric_limits<float_t>::quiet_NaN();
	float_t x(get_tat_sat_ratio());
	if (std::isnan(x))
		return std::numeric_limits<float_t>::quiet_NaN();
	return m_temp * x - IcaoAtmosphere<float_t>::degc_to_kelvin;
}

template<typename T>
void AirData<T>::set_rat(float_t rat)
{
	if (!std::isnan(rat)) {
		float_t x(get_tat_sat_ratio());
		if (false)
			std::cerr << "RAT: " << rat << " TAT/SAT ratio " << x << std::endl;
		if (!std::isnan(x)) {
			static constexpr float_t fnull = 0;
			rat += IcaoAtmosphere<float_t>::degc_to_kelvin;
			rat = std::max(rat, fnull);
			set_oat(rat / x);
		}
	}
}

template<typename T>
void AirData<T>::set_cas(float_t cas)
{
	if (std::isnan(cas) || cas < 0) {
		m_q = m_cas = std::numeric_limits<float_t>::quiet_NaN();
	} else {
		m_cas = cas;
		cas *= (1.0 / speed_of_sound_std);
		static constexpr float_t gammam1d2 = (gamma - 1) / 2.0;
		static constexpr float_t gammadgammam1 = gamma / (gamma - 1);
		m_q = IcaoAtmosphere<float_t>::std_sealevel_pressure * (std::pow(((gamma - 1) / 2.0) * cas * cas + 1, gammadgammam1) - 1);
	}
	compute_mach_eas_tas();
	if (false)
		std::cerr << "set_cas: tas " << m_tas << " eas " << m_eas << " mach " << m_mach << " q " << m_q << " cas " << m_cas
			  << " p " << m_p << " T " << m_temp << std::endl;
	if (false)
		std::cerr << "CAT/SSS: " << cas << " q " << get_q() << " p " << get_p() << " mach " << get_mach() << std::endl;
}

template<typename T>
void AirData<T>::set_tas(float_t tas)
{
	if (std::isnan(tas) || tas < 0) {
		m_q = m_cas = m_eas = m_tas = std::numeric_limits<float_t>::quiet_NaN();
	} else {
		m_tas = tas;
		{
			static constexpr float_t c0(std::sqrt(IcaoAtmosphere<float_t>::std_sealevel_temperature / IcaoAtmosphere<float_t>::std_sealevel_pressure));
			m_eas = m_tas * c0 * std::sqrt(m_p / m_temp);
		}
		{
			static constexpr float_t c0(std::sqrt(IcaoAtmosphere<float_t>::degc_to_kelvin) / speed_of_sound_0degc);
			m_mach = m_tas * c0 / std::sqrt(m_temp);
		}
		{
			static constexpr float_t c0((gamma - 1) / 2.0);
			static constexpr float_t c1(gamma / (gamma - 1));
			m_q = m_p * (std::pow(m_mach * m_mach * c0 + 1.f, c1) - 1.f);
		}
		{
			static constexpr float_t c0(std::sqrt(2.0 / (gamma - 1)));
			static constexpr float_t c1(1 / IcaoAtmosphere<float_t>::std_sealevel_pressure);
			static constexpr float_t c2((gamma - 1) / gamma);
			m_cas = speed_of_sound_std * c0 * std::sqrt(std::pow(m_q * c1 + 1, c2) - 1);
		}
		if (false)
			std::cerr << "set_tas: tas " << tas << " eas " << m_eas << " mach " << m_mach << " q " << m_q << " cas " << m_cas
				  << " p " << m_p << " T " << m_temp << std::endl;
	}
	if (false)
		std::cerr << "CAT/SSS: " << m_cas << " q " << get_q() << " p " << get_p() << " mach " << get_mach() << std::endl;
}

template<typename T>
typename AirData<T>::float_t AirData<T>::get_lss_degc(float_t temp)
{
	static constexpr float_t c0(1.0 / IcaoAtmosphere<float_t>::degc_to_kelvin);
	return speed_of_sound_0degc * std::sqrt(1 + temp * c0);
}

template<typename T>
typename AirData<T>::float_t AirData<T>::get_lss_kelvin(float_t temp)
{
	static constexpr float_t c0(speed_of_sound_0degc / std::sqrt(IcaoAtmosphere<float_t>::degc_to_kelvin));
	return std::sqrt(temp) * c0;
}

template<typename T>
void AirData<T>::compute_mach_eas_tas(void)
{
	if (std::isnan(m_p) || std::isnan(m_q) || m_p <= 0) {
		m_mach = m_eas = m_tas = std::numeric_limits<float_t>::quiet_NaN();
		return;
	}
	static constexpr float_t c0(2 / (gamma - 1));
	static constexpr float_t c1((gamma - 1) / gamma);
	m_mach = std::sqrt(c0 * (std::pow(m_q / m_p + 1, c1) - 1));
	m_eas = m_mach * std::sqrt(m_p) * (speed_of_sound_std / std::sqrt(IcaoAtmosphere<float_t>::std_sealevel_pressure));
	if (std::isnan(m_temp) || m_temp <= 0) {
		m_tas = std::numeric_limits<float_t>::quiet_NaN();
	} else {
		m_tas = m_mach * get_lss_kelvin(m_temp);
	}
}

template<typename T>
void AirData<T>::compute_tas(void)
{
	if (std::isnan(m_mach) || std::isnan(m_temp) || m_temp <= 0) {
		m_tas = std::numeric_limits<float_t>::quiet_NaN();
		return;
	}
	m_tas = m_mach * std::sqrt(m_temp) * (speed_of_sound_0degc / std::sqrt(IcaoAtmosphere<float_t>::degc_to_kelvin));
}

template<typename T>
void AirData<T>::compute_cas(void)
{
	static constexpr float_t c0(2 / (gamma - 1));
	static constexpr float_t c1((gamma - 1) / gamma);
	static constexpr float_t c2(1.0 / IcaoAtmosphere<float_t>::std_sealevel_pressure);
	m_cas = speed_of_sound_std * std::sqrt(c0 * (std::pow(m_q * c2 + 1, c1) - 1));
}

template class AirData<float>;
template class AirData<double>;
