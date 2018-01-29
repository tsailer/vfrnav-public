//
// C++ Interface: airdata
//
// Description: AirData related routines
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2012, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef AIRDATA_H
#define AIRDATA_H

#include "baro.h"

template<typename T>
class AirData {
public:
	typedef T float_t;
	static constexpr float_t gamma = 1.402;
	static constexpr float_t speed_of_sound_0degc = 331.3 * 3.6 / 1.852 /*Point::mps_to_kts*/;
	static const float_t speed_of_sound_std;

	// units of measurement:
	// altitude: ft
	// temperature: degrees centigrade
	// pressure: hPa
	// speed: kts

	AirData(void);

	void set_qnh(float_t qnh = IcaoAtmosphere<float_t>::std_sealevel_pressure);
	void set_qnh_temp(float_t qnh = IcaoAtmosphere<float_t>::std_sealevel_pressure, float_t temp = IcaoAtmosphere<float_t>::std_sealevel_temperature);
	void set_qnh_tempoffs(float_t qnh = IcaoAtmosphere<float_t>::std_sealevel_pressure, float_t tempoffs = 0) { set_qnh_temp(qnh, tempoffs + IcaoAtmosphere<float_t>::std_sealevel_temperature); }
	float_t get_qnh(void) const { return m_atmosphere.get_qnh(); }
	float_t get_temp(void) const { return m_atmosphere.get_temp(); }
	float_t get_tempoffs(void) const { return get_temp() - IcaoAtmosphere<float_t>::std_sealevel_temperature; }

	const IcaoAtmosphere<float_t>& get_atmosphere(void) const { return m_atmosphere; }

	float_t true_to_pressure_altitude(float_t truealt) const;
	float_t pressure_to_true_altitude(float_t baroalt) const;

	static void std_altitude_to_pressure(float_t *press, float_t *temp, float_t alt) { IcaoAtmosphere<float_t>::std_altitude_to_pressure(press, temp, alt); }
	static void std_pressure_to_altitude(float_t *alt, float_t *temp, float_t press) { IcaoAtmosphere<float_t>::std_pressure_to_altitude(alt, temp, press); }
	void altitude_to_pressure(float_t *press, float_t *temp, float_t alt) const { m_atmosphere.altitude_to_pressure(press, temp, alt); }
	void pressure_to_altitude(float_t *alt, float_t *temp, float_t press) const { m_atmosphere.pressure_to_altitude(alt, temp, press); }

	float_t get_pressure_altitude(void) const { return m_pressalt; }
	void set_pressure_altitude(float_t pressalt);
	float_t get_true_altitude(void) const { return m_truealt; }
	void set_true_altitude(float_t truealt);
	float_t get_density_altitude(void) const;
	void set_density_altitude(float_t da);

	float_t get_tatprobe_ct(void) const { return m_ct; }
	void set_tatprobe_ct(float_t ct = 1);

	float_t get_oat(void) const { return m_temp - IcaoAtmosphere<float_t>::degc_to_kelvin; }
	void set_oat(float_t oat);
	float_t get_rat(void) const;
	void set_rat(float_t oat);

	float_t get_cas(void) const { return m_cas; }
	float_t get_eas(void) const { return m_eas; }
	float_t get_tas(void) const { return m_tas; }
	float_t get_mach(void) const { return m_mach; }
	void set_cas(float_t cas);
	void set_tas(float_t tas);

	float_t get_p(void) const { return m_p; }
	float_t get_q(void) const { return m_q; }
	static float_t get_lss_degc(float_t temp);
	static float_t get_lss_kelvin(float_t temp);
	float_t get_lss(void) const { return get_lss_degc(get_oat()); }

protected:
	IcaoAtmosphere<float_t> m_atmosphere;
	float_t m_pressalt;
	float_t m_truealt;
	mutable float_t m_densityalt;
	float_t m_p;
	float_t m_q;
	float_t m_cas;
	float_t m_eas;
	float_t m_tas;
	float_t m_mach;
	float_t m_ct;
	float_t m_temp;
	float_t m_tempstd;

	float_t get_tat_sat_ratio(void) const;
	void compute_mach_eas_tas(void);
	void compute_tas(void);
	void compute_cas(void);
};

#endif /* AIRDATA_H */
