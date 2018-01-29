#include "sysdeps.h"

#include <cmath>
#include <limits>
#include <string.h>
#include "wmm.h"

constexpr float WMM::rTd;
const int WMM::maxord;
constexpr float WMM::pi;
constexpr float WMM::dtr;
constexpr float WMM::a;
constexpr float WMM::b;
constexpr float WMM::re;
constexpr float WMM::a2;
constexpr float WMM::b2;
constexpr float WMM::c2;
constexpr float WMM::a4;
constexpr float WMM::b4;
constexpr float WMM::c4;
const WMM::WMMModel::Fm WMM::WMMModel::fm;
const WMM::WMMModel::Fn WMM::WMMModel::fn;

WMM::WMM( void )
        : dec(std::numeric_limits<float>::quiet_NaN()), dip(std::numeric_limits<float>::quiet_NaN()),
          ti(std::numeric_limits<float>::quiet_NaN()), gv(std::numeric_limits<float>::quiet_NaN()),
          otime(std::numeric_limits<float>::quiet_NaN()), oalt(std::numeric_limits<float>::quiet_NaN()),
          olat(std::numeric_limits<float>::quiet_NaN()), olon(std::numeric_limits<float>::quiet_NaN()),
          ok(false)
{
}

WMM::WMM( float alt, float glat, float glon, float time )
        : dec(std::numeric_limits<float>::quiet_NaN()), dip(std::numeric_limits<float>::quiet_NaN()),
          ti(std::numeric_limits<float>::quiet_NaN()), gv(std::numeric_limits<float>::quiet_NaN()),
          otime(std::numeric_limits<float>::quiet_NaN()), oalt(std::numeric_limits<float>::quiet_NaN()),
          olat(std::numeric_limits<float>::quiet_NaN()), olon(std::numeric_limits<float>::quiet_NaN()),
          ok(false)
{
        compute(alt, glat, glon, time);
}

void WMM::invalidate(void)
{
	dec = dip = ti = gv = otime = oalt = olat = olon = std::numeric_limits<float>::quiet_NaN();
	ok = false;
}

void WMM::invalidate_flag(void)
{
	ok = false;
}

const WMM::WMMModel & WMM::select_model( float time ) const
{
        static const WMMModel *models[] = {
                &WMM2000,
                &WMM2005,
                &WMM2010,
                &WMM2015
        };
        static const int nrmodels = sizeof(models) / sizeof(models[0]);
        for (int i = 1; i < nrmodels; i++)
                if (time < models[i]->epoch)
                        return *models[i-1];
        return *models[nrmodels-1];
}

bool WMM::compute( const WMMModel & model, float alt, float glat, float glon, float time )
{
        int n, m, D3, D4;
        float dt, rlon, rlat, srlon, srlat, crlon, crlat, srlat2, crlat2;
        float q, q1, q2, ct, st, r2, r, d, ca, sa;
        float aor, ar, br, bt, bp, bpp, par, temp1, temp2, parp, bx, by, bz, bh;
        float sp[13], cp[13], pp[13], snorm_work[169];
        float tc[13][13], dp[13][13];
        float *p = snorm_work;

        /* INITIALIZE CONSTANTS */
        sp[0] = 0.0;
        cp[0] = *p = pp[0] = 1.0;
        dp[0][0] = 0.0;
        memcpy(snorm_work, model.snorm, sizeof(snorm_work));

        ok = true;
        dt = time - model.epoch;
        if (/*otime < 0.0 && */(dt < 0.0 || dt > 5.0)) {
                ok = false;
        }

        //pi = 3.14159265359;
        //dtr = pi/180.0;
        rlon = glon*dtr;
        rlat = glat*dtr;
        srlon = sin(rlon);
        srlat = sin(rlat);
        crlon = cos(rlon);
        crlat = cos(rlat);
        srlat2 = srlat*srlat;
        crlat2 = crlat*crlat;
        sp[1] = srlon;
        cp[1] = crlon;

        /* CONVERT FROM GEODETIC COORDS. TO SPHERICAL COORDS. */
        //if (alt != oalt || glat != olat)
        {
                q = sqrt(a2-c2*srlat2);
                q1 = alt*q;
                q2 = ((q1+a2)/(q1+b2))*((q1+a2)/(q1+b2));
                ct = srlat/sqrt(q2*crlat2+srlat2);
                st = sqrt(1.0-(ct*ct));
                r2 = (alt*alt)+2.0*q1+(a4-c4*srlat2)/(q*q);
                r = sqrt(r2);
                d = sqrt(a2*crlat2+b2*srlat2);
                ca = (alt+d)/r;
                sa = c2*crlat*srlat/(r*d);
        }
        //if (glon != olon)
        {
                for (m=2; m<=maxord; m++) {
                        sp[m] = sp[1]*cp[m-1]+cp[1]*sp[m-1];
                        cp[m] = cp[1]*cp[m-1]-sp[1]*sp[m-1];
                }
        }
        aor = re/r;
        ar = aor*aor;
        br = bt = bp = bpp = 0.0;
        for (n=1; n<=maxord; n++) {
                ar = ar*aor;
                for (m=0,D3=1,D4=(n+m+D3)/D3; D4>0; D4--,m+=D3) {
/*
                        COMPUTE UNNORMALIZED ASSOCIATED LEGENDRE POLYNOMIALS
                        AND DERIVATIVES VIA RECURSION RELATIONS
*/
                        //if (alt != oalt || glat != olat)
                        {
                                if (n == m) {
                                        *(p+n+m*13) = st**(p+n-1+(m-1)*13);
                                        dp[m][n] = st*dp[m-1][n-1]+ct**(p+n-1+(m-1)*13);
                                        goto S50;
                                }
                                if (n == 1 && m == 0) {
                                        *(p+n+m*13) = ct**(p+n-1+m*13);
                                        dp[m][n] = ct*dp[m][n-1]-st**(p+n-1+m*13);
                                        goto S50;
                                }
                                if (n > 1 && n != m) {
                                        if (m > n-2) *(p+n-2+m*13) = 0.0;
                                        if (m > n-2) dp[m][n-2] = 0.0;
                                        *(p+n+m*13) = ct**(p+n-1+m*13)-model.k[m][n]**(p+n-2+m*13);
                                        dp[m][n] = ct*dp[m][n-1] - st**(p+n-1+m*13)-model.k[m][n]*dp[m][n-2];
                                }
                        }
S50:
/*
                        TIME ADJUST THE GAUSS COEFFICIENTS
*/
                        //if (time != otime)
                        {
                                tc[m][n] = model.c[m][n]+dt*model.cd[m][n];
                                if (m != 0) tc[n][m-1] = model.c[n][m-1]+dt*model.cd[n][m-1];
                        }
/*
    ACCUMULATE TERMS OF THE SPHERICAL HARMONIC EXPANSIONS
*/
                        par = ar**(p+n+m*13);
                        if (m == 0) {
                                temp1 = tc[m][n]*cp[m];
                                temp2 = tc[m][n]*sp[m];
                        } else {
                                temp1 = tc[m][n]*cp[m]+tc[n][m-1]*sp[m];
                                temp2 = tc[m][n]*sp[m]-tc[n][m-1]*cp[m];
                        }
                        bt = bt-ar*temp1*dp[m][n];
                        bp += (model.fm[m]*temp2*par);
                        br += (model.fn[n]*temp1*par);
/*
                        SPECIAL CASE:  NORTH/SOUTH GEOGRAPHIC POLES
*/
                        if (st == 0.0 && m == 1) {
                                if (n == 1) pp[n] = pp[n-1];
                                else pp[n] = ct*pp[n-1]-model.k[m][n]*pp[n-2];
                                parp = ar*pp[n];
                                bpp += (model.fm[m]*temp2*parp);
                        }
                }
        }
        if (st == 0.0) bp = bpp;
        else bp /= st;
/*
        ROTATE MAGNETIC VECTOR COMPONENTS FROM SPHERICAL TO
        GEODETIC COORDINATES
*/
        bx = -bt*ca-br*sa;
        by = bp;
        bz = bt*sa-br*ca;
/*
        COMPUTE DECLINATION (DEC), INCLINATION (DIP) AND
        TOTAL INTENSITY (TI)
*/
        bh = sqrt((bx*bx)+(by*by));
        ti = sqrt((bh*bh)+(bz*bz));
        dec = atan2(by,bx)/dtr;
        dip = atan2(bz,bh)/dtr;
/*
        COMPUTE MAGNETIC GRID VARIATION IF THE CURRENT
        GEODETIC POSITION IS IN THE ARCTIC OR ANTARCTIC
        (I.E. GLAT > +55 DEGREES OR GLAT < -55 DEGREES)

        OTHERWISE, SET MAGNETIC GRID VARIATION TO -999.0
*/
        gv = -999.0;
        if (fabs(glat) >= 55.) {
                if (glat > 0.0 && glon >= 0.0) gv = dec-glon;
                if (glat > 0.0 && glon < 0.0) gv = dec+fabs(glon);
                if (glat < 0.0 && glon >= 0.0) gv = dec+glon;
                if (glat < 0.0 && glon < 0.0) gv = dec-fabs(glon);
                if (gv > +180.0) gv -= 360.0;
                if (gv < -180.0) gv += 360.0;
        }
        otime = time;
        oalt = alt;
        olat = glat;
        olon = glon;
        return ok;
}

float WMM::timet_to_time( time_t t )
{
        struct tm tm;
        memset(&tm, 0, sizeof(tm));
        gmtime_r(&t, &tm);
        int32_t f = tm.tm_yday * (24*60*60) + tm.tm_hour * (60 * 60) + tm.tm_min * 60 + tm.tm_sec;
        //return (tm.tm_year + 1900) + f * (1.0 / 365.2425 / 24 / 60 / 60);
        struct tm tm1;
        memset(&tm1, 0, sizeof(tm1));
        tm1.tm_year = tm.tm_year;
        time_t t1 = mktime(&tm1);
        tm1.tm_year++;
        time_t t2 = mktime(&tm1);
        return (tm.tm_year + 1900) + f * ((1.0 / 24.0 / 60.0 / 60.0) / (t2 - t1));
}

bool WMM::compute( float alt, const Point & pt, time_t time )
{
        return compute(alt, pt.get_lat_deg(), pt.get_lon_deg(), timet_to_time(time));
}
