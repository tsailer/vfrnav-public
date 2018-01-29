//
// C++ Interface: SunriseSunset
//
// Description:
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SUNRISESUNSET_H
#define SUNRISESUNSET_H

#include "geom.h"

class SunriseSunset {
        private:
                /* A macro to compute the number of days elapsed since 2000 Jan 0.0 */
                /* (which is equal to 1999 Dec 31, 0h UT)                           */

                static long days_since_2000_Jan_0(int y, int m, int d) {
                        return (367L*(y)-((7*((y)+(((m)+9)/12)))/4)+((275*(m))/9)+(d)-730530L);
                }

                static constexpr double RADEG  = ( 180.0 / M_PI );
                static constexpr double DEGRAD = ( M_PI / 180.0 );
                static constexpr double INV360 = ( 1.0 / 360.0 );


                /* The trigonometric functions in degrees */

                static double sind(double x) { return sin((x)*DEGRAD); }
                static double cosd(double x) { return cos((x)*DEGRAD); }
                static double tand(double x) { return tan((x)*DEGRAD); }

                static double atand(double x) { return (RADEG*atan(x)); }
                static double asind(double x) { return (RADEG*asin(x)); }
                static double acosd(double x) { return (RADEG*acos(x)); }
                static double atan2d(double y, double x) { return (RADEG*atan2(y,x)); }


                /* Function prototypes */

                static double __daylen__(int year, int month, int day, double lon, double lat, double altit, bool upper_limb);

                static int __sunriset__(int year, int month, int day, double lon, double lat, double altit, bool upper_limb, double& rise, double& set);

                static void sunpos(double d, double& lon, double& r);

                static void sun_RA_dec(double d, double& RA, double& dec, double& r);

                static double revolution( double x );

                static double rev180( double x );

                static double GMST0( double d );

        public:
                /* Following are some macros around the "workhorse" function __daylen__ */
                /* They mainly fill in the desired values for the reference altitude    */
                /* below the horizon, and also selects whether this altitude should     */
                /* refer to the Sun's center or its upper limb.                         */


                /* This macro computes the length of the day, from sunrise to sunset. */
                /* Sunrise/set is considered to occur when the Sun's upper limb is    */
                /* 35 arc minutes below the horizon (this accounts for the refraction */
                /* of the Earth's atmosphere).                                        */
                static double day_length(int year, int month, int day, double lon, double lat) {
                        return __daylen__(year, month, day, lon, lat, -35.0/60.0, true);
                }

                static double day_length(int year, int month, int day, const Point& pt) {
                        return day_length(year, month, day, pt.get_lon_deg(), pt.get_lat_deg());
                }

                /* This macro computes the length of the day, including civil twilight. */
                /* Civil twilight starts/ends when the Sun's center is 6 degrees below  */
                /* the horizon.                                                         */
                static double day_civil_twilight_length(int year, int month, int day, double lon, double lat) {
                        return __daylen__(year, month, day, lon, lat, -6.0, false);
                }

                static double day_civil_twilight_length(int year, int month, int day, const Point& pt) {
                        return day_civil_twilight_length(year, month, day, pt.get_lon_deg(), pt.get_lat_deg());
                }

                /* This macro computes the length of the day, incl. nautical twilight.  */
                /* Nautical twilight starts/ends when the Sun's center is 12 degrees    */
                /* below the horizon.                                                   */
                static double day_nautical_twilight_length(int year, int month, int day, double lon, double lat) {
                        return __daylen__(year, month, day, lon, lat, -12.0, false);
                }

                static double day_nautical_twilight_length(int year, int month, int day, const Point& pt) {
                        return day_nautical_twilight_length(year, month, day, pt.get_lon_deg(), pt.get_lat_deg());
                }

                /* This macro computes the length of the day, incl. astronomical twilight. */
                /* Astronomical twilight starts/ends when the Sun's center is 18 degrees   */
                /* below the horizon.                                                      */
                static double day_astronomical_twilight_length(int year, int month, int day, double lon, double lat) {
                        return __daylen__(year, month, day, lon, lat, -18.0, false);
                }

                static double day_astronomical_twilight_length(int year, int month, int day, const Point& pt) {
                        return day_astronomical_twilight_length(year, month, day, pt.get_lon_deg(), pt.get_lat_deg());
                }


                /* This macro computes times for sunrise/sunset.                      */
                /* Sunrise/set is considered to occur when the Sun's upper limb is    */
                /* 35 arc minutes below the horizon (this accounts for the refraction */
                /* of the Earth's atmosphere).                                        */
                static int sun_rise_set(int year, int month, int day, double lon, double lat, double& rise, double& set) {
                        return __sunriset__(year, month, day, lon, lat, -35.0/60.0, true, rise, set);
                }

                static int sun_rise_set(int year, int month, int day, const Point& pt, double& rise, double& set) {
                        return sun_rise_set(year, month, day, pt.get_lon_deg(), pt.get_lat_deg(), rise, set);
                }

                /* This macro computes the start and end times of civil twilight.       */
                /* Civil twilight starts/ends when the Sun's center is 6 degrees below  */
                /* the horizon.                                                         */
                static int civil_twilight(int year, int month, int day, double lon, double lat, double& start, double& end) {
                        return __sunriset__(year, month, day, lon, lat, -6.0, false, start, end);
                }

                static int civil_twilight(int year, int month, int day, const Point& pt, double& start, double& end) {
                        return civil_twilight(year, month, day, pt.get_lon_deg(), pt.get_lat_deg(), start, end);
                }

                /* This macro computes the start and end times of nautical twilight.    */
                /* Nautical twilight starts/ends when the Sun's center is 12 degrees    */
                /* below the horizon.                                                   */
                static int nautical_twilight(int year, int month, int day, double lon, double lat, double& start, double& end) {
                        return __sunriset__(year, month, day, lon, lat, -12.0, false, start, end);
                }

                static int nautical_twilight(int year, int month, int day, const Point& pt, double& start, double& end) {
                        return nautical_twilight(year, month, day, pt.get_lon_deg(), pt.get_lat_deg(), start, end);
                }

                /* This macro computes the start and end times of astronomical twilight.   */
                /* Astronomical twilight starts/ends when the Sun's center is 18 degrees   */
                /* below the horizon.                                                      */
                static int astronomical_twilight(int year, int month, int day, double lon, double lat, double& start, double& end) {
                        return __sunriset__(year, month, day, lon, lat, -18.0, false, start, end);
                }

                static int astronomical_twilight(int year, int month, int day, const Point& pt, double& start, double& end) {
                        return astronomical_twilight(year, month, day, pt.get_lon_deg(), pt.get_lat_deg(), start, end);
                }
};

#endif /* SUNRISESUNSET_H */
