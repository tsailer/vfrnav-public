//
// C++ Interface: wind
//
// Description: Wind triangle related routines
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2012, 2013, 2014, 2015
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef WIND_H
#define WIND_H

#include "sysdeps.h"

#include <cmath>

template <typename T> class Wind {
  public:
	typedef T float_t;
	static constexpr float_t to_rad = M_PI / 180.0;
	static constexpr float_t from_rad = 180.0 / M_PI;

	Wind(void);
	float_t get_tas(void) const { return m_tas; }
	float_t get_gs(void) const { return m_gs; }
	float_t get_hdg(void) const { return m_hdg; }
	float_t get_crs(void) const { return m_crs; }
	float_t get_winddir(void) const { return m_winddir; }
	float_t get_windspeed(void) const { return m_windspeed; }
	void set_wind(float_t winddir, float_t windspeed);
	void set_wind(float_t winddir0, float_t windspeed0, float_t winddir1, float_t windspeed1);
	void set_wind(float_t winddir0, float_t windspeed0, float_t winddir1, float_t windspeed1, float_t frac);
	void set_hdg_tas(float_t hdg, float_t tas);
	void set_crs_gs(float_t crs, float_t gs);
	void set_crs_tas(float_t crs, float_t tas);
	void set_hdg_tas_crs_gs(float_t hdg, float_t tas, float_t crs, float_t gs);

  protected:
	float_t m_tas;
	float_t m_gs;
	float_t m_hdg;
	float_t m_crs;
	float_t m_winddir;
	float_t m_windspeed;

	static inline void mysincos(float_t x, float_t *sin, float_t *cos);
};

#endif /* WIND_H */
