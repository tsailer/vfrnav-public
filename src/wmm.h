//
// C++ Interface: wmm
//
// Description: 
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2010
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef WMM_H
#define WMM_H

#include <math.h>
#include <string>

#include <geom.h>

class WMM {
        private:
                static constexpr float rTd = M_PI / 180.0;

        public:
                WMM(float alt, float glat, float glon, float time);
                WMM(void);
                /** \brief get declination
                 *
                 * Returns the declination (or magnetic variation) D (the horizontal angle between true north
                 * and the field vector, measured positive eastwards)
                 * @return declination in degrees
                 */
                float get_dec(void) const { return dec; }
                /** \brief get inclination
                 *
                 * Returns the inclination (or dip) I (the angle between the horizontal plane and the
                 * field vector, measured positive downwards)
                 * @return inclination in degrees
                 */
                float get_dip(void) const { return dip; }
                /** \brief get total intensity
                 *
                 * Returns the total intensity
                 * @return total intensity in nT (nano Tesla)
                 */
                float get_ti(void) const { return ti; }
                /** \brief get grid variation
                 *
                 * Returns the grid variation
                 * @return grid variation in degrees
                 */
                float get_gv(void) const { return gv; }
                /** \brief get last computation time
                 *
                 * Returns the last computation time
                 * @return current (fractional) year (i.e. 2006.0 means 1.1.2006 00:00z)
                 */
                float get_time(void) const { return otime; }
                /** \brief get last computation geodetic altitude
                 *
                 * Returns the last computation geodetic altitude
                 * @return geodetic altitude in km (kilometers)
                 */
                float get_alt(void) const { return oalt; }
                /** \brief get last computation latitude
                 *
                 * Returns the last computation latitude
                 * @return latitude in degrees
                 */
                float get_lat(void) const { return olat; }
                /** \brief get last computation longitude
                 *
                 * Returns the last computation longitude
                 * @return longitude in degrees
                 */
                float get_lon(void) const { return olon; }
                /** \brief get last computation success
                 *
                 * Returns whether the last computation succeeded
                 * @return true if success
                 */
                operator bool(void) const { return ok; }
                /** \brief get northerly intensity
                 *
                 * Returns the X (northerly intensity) B vector component
                 * @return northerly intensity in nT (nano Tesla)
                 */
                float get_x(void) const { return get_ti() * cos(get_dec() * rTd) * cos(get_dip() * rTd); }
                /** \brief get easterly intensity
                 *
                 * Returns the Y (easterly intensity) B vector component
                 * @return easterly intensity in nT (nano Tesla)
                 */
                float get_y(void) const { return get_ti() * sin(get_dec() * rTd) * cos(get_dip() * rTd); }
                /** \brief get vertical intensity
                 *
                 * Returns the Z (vertical intensity, positive downwards) B vector component
                 * @return vertical intensity (positive downwards) in nT (nano Tesla)
                 */
                float get_z(void) const { return get_ti() * sin(get_dip() * rTd); }
                /** \brief get horizontal intensity
                 *
                 * Returns the H (horizontal intensity) B vector component
                 * @return horizontal intensity in nT (nano Tesla)
                 */
                float get_h(void) const { return get_ti() * cos(get_dip() * rTd); }

        private:
                static const int maxord = 12;
                static constexpr float pi = 3.141592741e+00;
                static constexpr float dtr = 1.745329238e-02;
                static constexpr float a = 6.378137207e+03;
                static constexpr float b = 6.356752441e+03;
                static constexpr float re = 6.371200195e+03;
                static constexpr float a2 = 4.068063600e+07;
                static constexpr float b2 = 4.040830000e+07;
                static constexpr float c2 = 2.723360000e+05;
                static constexpr float a4 = 1.654914116e+15;
                static constexpr float b4 = 1.632830736e+15;
                static constexpr float c4 = 2.208337966e+13;

                class WMMModel {
                        private:
                                class Fm {
                                        public:
					        Fm(void) {}
                                                float operator[](int x) const { return x; }
                                };

                                class Fn {
                                        public:
					        Fn(void) {}
                                                float operator[](int x) const { return x + 1; }
                                };

                        public:
                                const std::string name;
                                const std::string cofdate;
                                const float epoch;
                                const float c[maxord+1][maxord+1];
                                const float cd[maxord+1][maxord+1];
                                const float snorm[(maxord+1)*(maxord+1)];
                                static const Fm fm;
                                static const Fn fn;
                                const float k[maxord+1][maxord+1];
                };

                static const WMMModel WMM2000;
                static const WMMModel WMM2005;
                static const WMMModel WMM2010;
                static const WMMModel WMM2015;

                float dec;
                float dip;
                float ti;
                float gv;
                float otime;
                float oalt;
                float olat;
                float olon;
                bool ok;

                /** \brief return model coefficients
                 * 
                 * Return the model coefficients closes to the selected time
                 * @param time current (fractional) year (i.e. 2006.0 means 1.1.2006 00:00z)
                 * @return model coefficient reference
                 */
                const WMMModel& select_model(float time) const;
                /** \brief compute magnetic field
                 *
                 * Compute Magnetic Field
                 * @param model the WMM coefficients to use
                 * @param alt geodetic altitude in km (kilometers)
                 * @param glat latitude in degrees
                 * @param glon longitude in degrees
                 * @param time current (fractional) year (i.e. 2006.0 means 1.1.2006 00:00z)
                 * @return true if the computation succeeded (time is within the model timespan)
                 */
                bool compute(const WMMModel& model, float alt, float glat, float glon, float time);
                /** \brief Convert Unix time to fractional year
                 *
                 * Convert Unix time to fractional year
                 * @param t Unix time
                 * @return fractional year
                 */
                static float timet_to_time(time_t t);

        public:
                /** \brief compute magnetic field
                 *
                 * Compute Magnetic Field
                 * @param alt geodetic altitude in km (kilometers)
                 * @param glat latitude in degrees
                 * @param glon longitude in degrees
                 * @param time current (fractional) year (i.e. 2006.0 means 1.1.2006 00:00z)
                 * @return true if the computation succeeded (time is within the model timespan)
                 */
                bool compute(float alt, float glat, float glon, float time) { return compute(select_model(time), alt, glat, glon, time); }
                /** \brief compute magnetic field
                 *
                 * Compute Magnetic Field
                 * @param alt geodetic altitude in km (kilometers)
                 * @param pt coordinates
                 * @param time current Unix time
                 * @return true if the computation succeeded (time is within the model timespan)
                 */
                bool compute(float alt, const Point& pt, time_t time);

                /** \brief invalidate previous computation
                 *
                 * Invalidate the previous computation, so that subsequent get calls return NaN
                 */
		void invalidate(void);

                /** \brief make operator bool return false
                 *
                 * make operator bool return false
                 */
		void invalidate_flag(void);
};

#endif /* WMM_H */
