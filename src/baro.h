//
// C++ Interface: baro
//
// Description:
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2012, 2015, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef BARO_H
#define BARO_H

#include "sysdeps.h"

template<typename T>
class IcaoAtmosphere {
public:
	typedef T float_t;
	static constexpr float_t const_g   = 9.80665;                     /* gravity const. */
	static constexpr float_t const_m   = 28.9644;                     /* Molecular Weight of air */
	static constexpr float_t const_r   = 8.31432;                     /* gas const. for air */
	static constexpr float_t const_l0  = 6.5/1000;                    /* Lapse rate first layer */
	static const float_t const_gmr;                                   /* Hydrostatic constant */
	static constexpr float_t degc_to_kelvin = 273.15;
	static constexpr float_t std_sealevel_temperature = 15 + 273.15/*degc_to_kelvin*/;
	static constexpr float_t std_sealevel_pressure = 1013.25;
	static const float_t std_sealevel_density;

	IcaoAtmosphere(float_t qnh = std_sealevel_pressure, float_t temp = std_sealevel_temperature);
	void set(float_t qnh = std_sealevel_pressure, float_t temp = std_sealevel_temperature);
	float_t get_qnh(void) const { return m_bdry[0].p; }
	float_t get_temp(void) const { return m_bdry[0].t; }
	void altitude_to_pressure(float_t *press, float_t *temp, float_t alt) const { altitude_to_pressure(press, temp, alt, m_bdry); }
	void pressure_to_altitude(float_t *alt, float_t *temp, float_t press) const { pressure_to_altitude(alt, temp, press, m_bdry); }
	float_t altitude_to_density(float_t alt) const { return altitude_to_density(alt, m_bdry); }
	float_t density_to_altitude(float_t density) const { return density_to_altitude(density, m_bdry); }
	static inline void std_altitude_to_pressure(float_t *press, float_t *temp, float_t alt) { altitude_to_pressure(press, temp, alt, stdbdry); }
	static inline void std_pressure_to_altitude(float_t *alt, float_t *temp, float_t press) { pressure_to_altitude(alt, temp, press, stdbdry); }
	static inline float_t std_altitude_to_density(float_t alt) { return altitude_to_density(alt, stdbdry); }
	static inline float_t std_density_to_altitude(float_t density) { return density_to_altitude(density, stdbdry); }
	static float_t pressure_to_density(float_t pressure, float_t temp);
	static float_t density_to_pressure(float_t density, float_t temp);

protected:
	struct AtmospericLayer {
		float_t limit;
		float_t lapserate;
	};

	struct atmospheric_boundary {
		float_t t;
		float_t p;
		float_t rho;
	};

	//static const unsigned int nr_layers = sizeof(atmosperic_layers)/sizeof(atmosperic_layers[0])-1;
	static const unsigned int nr_layers = 7;
	static const AtmospericLayer atmosperic_layers[nr_layers+1];
	static const atmospheric_boundary stdbdry[nr_layers+1];
	atmospheric_boundary m_bdry[nr_layers+1];

	static void altitude_to_pressure(float_t *press, float_t *temp, float_t alt, const atmospheric_boundary b[nr_layers+1]);
	static void pressure_to_altitude(float_t *alt, float_t *temp, float_t press, const atmospheric_boundary b[nr_layers+1]);
	static float_t altitude_to_density(float_t alt, const atmospheric_boundary b[nr_layers+1]);
	static float_t density_to_altitude(float_t density, const atmospheric_boundary b[nr_layers+1]);
	void compute_boundaries(void);
};

#endif /* BARO_H */
