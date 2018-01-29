//
// C++ Implementation: fplan
//
// Description:
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012, 2013, 2014, 2015, 2016, 2017
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <libxml++/libxml++.h>
#include <glibmm.h>
#ifdef HAVE_GIOMM
#include <giomm.h>
#endif
#include <unistd.h>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sqlite3x.hpp>
#include <sqlite3.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdexcept>
#include <stdarg.h>
#include <limits>
#include <cstring>
#include <glib.h>
//#include <glib/gstdio.h>

#ifdef HAVE_JSONCPP
#include <json/json.h>
#endif

#include "fplan.h"
#include "baro.h"
#include "aircraft.h"
#include "airdata.h"
#include "icaofpl.h"
#include "wmm.h"
#include "dbobj.h"
#include "wind.h"
#include "nwxweather.h"
#include "grib2.h"
#include "dbobj.h"
#include "engine.h"

FPlanLeg::FPlanLeg(void)
        : m_fromicao(), m_toicao(), m_fromname(), m_toname(), m_fromcoord(), m_tocoord(),
          m_fromalt(0), m_toalt(0), m_fromtruealt(0), m_totruealt(0), m_fromflags(0), m_toflags(0),
          m_fromfreq(0), m_tofreq(0), m_fromtime(0), m_legtime(0), m_winddir(0), m_windspeed(0),
          m_dist(0), m_vdist(0), m_truetrack(0), m_var(0), m_wca(0),
          m_legtype(legtype_invalid), m_future(false),
          m_crosstrack(0), m_disttodest(0), m_vgnd(0), m_legfraction(0),
          m_altdiff(0), m_referencealt(0), m_mapalt(0),
          m_mapcoord(), m_coursetodest(0), m_gpsvalid(false), m_gpsaltvalid(false)
{
}

FPlanLeg::FPlanLeg(const Glib::ustring& fromicao, const Glib::ustring & fromname, const Point & fromcoord,
                   int16_t fromalt, uint16_t fromflags, uint32_t fromtime, uint32_t fromfreq,
                   const Glib::ustring& toicao, const Glib::ustring & toname, const Point & tocoord,
                   int16_t toalt, uint16_t toflags, uint32_t totime, uint32_t tofreq,
		   float winddir, float windspeed, float qnh, float isaoffset,
                   legtype_t legtype, bool future)
        : m_fromicao(fromicao), m_toicao(toicao), m_fromname(fromname), m_toname(toname), m_fromcoord(fromcoord), m_tocoord(tocoord),
          m_fromalt(fromalt), m_toalt(toalt), m_fromtruealt(fromalt), m_totruealt(toalt), m_fromflags(fromflags), m_toflags(toflags),
          m_fromfreq(fromfreq), m_tofreq(tofreq), m_fromtime(fromtime), m_legtime(totime - fromtime), m_winddir(winddir), m_windspeed(windspeed),
          m_dist(0), m_vdist(0), m_truetrack(0), m_var(0), m_wca(0),
          m_legtype(legtype), m_future(future),
          m_crosstrack(0), m_disttodest(0), m_vgnd(0), m_legfraction(0),
          m_altdiff(0), m_referencealt(0), m_mapalt(0),
          m_mapcoord(), m_coursetodest(0), m_gpsvalid(false), m_gpsaltvalid(false)
{
        m_dist = m_fromcoord.spheric_distance_nmi(m_tocoord);
        m_truetrack = (uint16_t)(m_fromcoord.spheric_true_course(m_tocoord) * from_deg);
        WMM wmm;
        if (wmm.compute(get_fromaltitude() * (FPlanWaypoint::ft_to_m * 1e-3), get_fromcoord(), get_fromtime_unix()))
                m_var = (uint16_t)(wmm.get_dec() * from_deg);
	baro_correction(qnh, isaoffset);
}

Glib::ustring FPlanLeg::get_fromicaoname(void) const
{
        Glib::ustring r(m_fromicao);
        if (!m_fromicao.empty() && !m_fromname.empty())
                r += " ";
        r += m_fromname;
        return r;
}

Glib::ustring FPlanLeg::get_toicaoname(void) const
{
        Glib::ustring r(m_toicao);
        if (!m_toicao.empty() && !m_toname.empty())
                r += " ";
        r += m_toname;
        return r;
}

void FPlanLeg::baro_correction(float qnh, float isaoffset)
{
        m_fromtruealt = m_fromalt;
        m_totruealt = m_toalt;
        if (!((m_fromflags | m_toflags) & FPlanWaypoint::altflag_standard))
                return;
	IcaoAtmosphere<float> atmo(qnh, isaoffset + IcaoAtmosphere<float>::std_sealevel_temperature);
        float press, alt;
        if (m_fromflags & FPlanWaypoint::altflag_standard) {
                atmo.std_altitude_to_pressure(&press, 0, get_fromaltitude() * FPlanWaypoint::ft_to_m);
                atmo.pressure_to_altitude(&alt, 0, press);
                m_fromtruealt = (int16_t)(alt * FPlanWaypoint::m_to_ft);
        }
        if (m_toflags & FPlanWaypoint::altflag_standard) {
                atmo.std_altitude_to_pressure(&press, 0, get_toaltitude() * FPlanWaypoint::ft_to_m);
                atmo.pressure_to_altitude(&alt, 0, press);
                m_totruealt = (int16_t)(alt * FPlanWaypoint::m_to_ft);
        }
}

int16_t FPlanLeg::baro_correction(float qnh, float isaoffset, int16_t alt, uint16_t altflags)
{
        if (!(altflags & FPlanWaypoint::altflag_standard))
                return alt;
	IcaoAtmosphere<float> atmo(qnh, isaoffset + IcaoAtmosphere<float>::std_sealevel_temperature);
        float press, alt1;
        atmo.std_altitude_to_pressure(&press, 0, alt * FPlanWaypoint::ft_to_m);
        atmo.pressure_to_altitude(&alt1, 0, press);
        return Point::round<int16_t,float>(alt1 * FPlanWaypoint::m_to_ft);
}

void FPlanLeg::wind_correction(float vcruise_kts)
{
	wind_correction(vcruise_kts, m_winddir, m_windspeed);
}

/*
 * Winddreieck:
 * a = vdist
 * b = vwind
 * c = vcruise
 * alpha = ?
 * beta = WCA
 * gamma = 180 + TT - winddir
 *
 * Sinussatz: vwind/sin(WCA) = vcruise/sin(winddir-TT) = vdist/sin(winddir-TT-WCA)
 * Cosinussatz: vdist^2 = vwind^2 + vcruise^2 - 2*vwind*vcruise*cos(winddir-TT-WCA)
 */

void FPlanLeg::wind_correction(float vcruise_kts, float winddir_deg, float vwind_kts)
{
        uint16_t wdirminustt = (uint16_t)(winddir_deg * from_deg);
        wdirminustt -= m_truetrack;
        m_wca = (uint16_t)(from_rad * asin(sin(wdirminustt * to_rad) * vwind_kts / vcruise_kts));
        m_vdist = sqrt(vwind_kts * vwind_kts + vcruise_kts * vcruise_kts -
                       2.0 * vwind_kts * vcruise_kts * cos(to_rad * (wdirminustt - m_wca)));
        m_vgnd = m_vdist;
        switch (m_legtype) {
                case legtype_offblock_takeoff:
                case legtype_landing_onblock:
                        m_legtime = 5 * 60;
                        break;

                case legtype_normal:
                        m_legtime = (uint32_t)(3600 * m_dist / std::max(m_vdist, 0.1f));
                        break;

                default:
                        m_legtime = 0;
                        break;
        }
}

void FPlanLeg::update( time_t curtime, const Point & coord, bool coordvalid, int16_t alt, bool altvalid, float vgnd )
{
        m_crosstrack = m_disttodest = 0.0;
        m_legfraction = 0;
        m_altdiff = m_mapalt = 0;
        m_coursetodest = m_truetrack;
        m_gpsvalid = m_gpsaltvalid = false;
        CartesianVector3Df cfrom = PolarVector3Df(m_fromcoord, 0);
        CartesianVector3Df cto = PolarVector3Df(m_tocoord, 0);
        m_gpsvalid = coordvalid;
        m_gpsaltvalid = coordvalid && altvalid;
        float t = 0;
        if (m_gpsvalid) {
                m_mapcoord = coord;
                m_vgnd = vgnd;
                if (get_legtype() == legtype_normal) {
                        CartesianVector3Df cm = PolarVector3Df(m_mapcoord, 0);
                        t = cm.nearest_fraction(cfrom, cto);
                        CartesianVector3Df cproj = cfrom + (cto - cfrom) * t;
                        m_crosstrack = (cm - cproj).length();
                        m_coursetodest = (uint16_t)(m_mapcoord.spheric_true_course(m_tocoord) * from_deg);
                        if ((int16_t)(m_coursetodest - m_truetrack) < 0)
                                m_crosstrack = -m_crosstrack;
                }
        } else {
                if (!m_legtime) {
                        t = 0.5;
                } else {
                        t = curtime - get_fromtime_unix();
                        t = std::min(std::max(t / m_legtime, 0.0f), 1.0f);
                }
                m_mapcoord = ((PolarVector3Df)(cfrom + (cto - cfrom) * t)).getcoord();
                m_vgnd = m_vdist;
        }
        m_referencealt = m_fromtruealt + (int16_t)(t * (m_totruealt - m_fromtruealt));
        m_legfraction = (uint16_t)((1U << 15) * t);
        m_mapalt = m_gpsaltvalid ? alt : m_referencealt;
        m_altdiff = m_mapalt - m_referencealt;
        m_disttodest = m_mapcoord.spheric_distance_nmi(m_tocoord);
        WMM wmm;
        if (wmm.compute(get_mapalt() * (FPlanWaypoint::ft_to_m * 1e-3), get_mapcoord(), curtime))
                m_var = (uint16_t)(wmm.get_dec() * from_deg);
}

float FPlanLeg::get_verticalspeedtodest(void) const
{
        return (get_totruealtitude() - get_mapalt()) / std::max(1.0f / 60.0f, 60.0f * get_disttodest() / std::max(get_vgnd(), 1.0f));
}

float FPlanLeg::get_verticalspeed(void) const
{
        return (get_totruealtitude() - get_fromtruealtitude()) / std::max(1.0f / 60.0f, 60.0f * get_dist() / std::max(get_vgnd(), 1.0f));
}

const time_t FPlanWaypoint::unix_minus_palmos_epoch;
const uint16_t FPlanWaypoint::altflag_standard;
const uint16_t FPlanWaypoint::altflag_climb;
const uint16_t FPlanWaypoint::altflag_descent;
const uint16_t FPlanWaypoint::altflag_oat;
const uint16_t FPlanWaypoint::altflag_altvfr;
const uint16_t FPlanWaypoint::altflag_turnpoint;
const uint16_t FPlanWaypoint::altflag_ifr;
const int32_t FPlanWaypoint::invalid_altitude = std::numeric_limits<int32_t>::min();
constexpr float FPlanWaypoint::ft_to_m;
constexpr float FPlanWaypoint::m_to_ft;
constexpr float FPlanWaypoint::to_rad;
constexpr float FPlanWaypoint::to_deg;
constexpr float FPlanWaypoint::from_rad;
constexpr float FPlanWaypoint::from_deg;
const std::string FPlanWaypoint::reader_fields = "ICAO,NAME,LON,LAT,ALT,FREQ,TIME,SAVETIME,FLAGS,PATHNAME,PATHCODE,NOTE,WINDDIR,WINDSPEED,QFF,ISAOFFS,"
	"TRUEALT,TERRAIN,TRUETRACK,TRUEHEADING,DECLINATION,DIST,FUEL,TAS,RPM,MP,TYPE,NAVAID,FLIGHTTIME,MASS,ENGINEPROFILE,BHP,TROPOPAUSE";

FPlanWaypoint::FPlanWaypoint(void)
        : m_icao(), m_name(), m_pathname(), m_note(), m_engineprofile(), m_coord(), m_time(0), m_savetime(0), m_flighttime(0), m_frequency(0), m_alt(0),
	  m_truealt(0), m_terrain(invalid_altitude), m_tropopause(invalid_altitude), m_dist(0), m_mass(0), m_fuel(0), m_bhp(0), m_flags(0), m_winddir(0), m_windspeed(0),
	  m_qff(IcaoAtmosphere<float>::std_sealevel_pressure * 32), m_isaoffset(0), m_tt(0), m_th(0), m_decl(0), m_tas(0), m_rpm(0), m_mp(0),
	  m_pathcode(pathcode_none), m_type(type_undefined), m_navaid(navaid_invalid), m_dirty(false)
{
}

FPlanWaypoint::FPlanWaypoint(sqlite3x::sqlite3_cursor& cursor)
        : m_icao(cursor.getstring(0)), m_name(cursor.getstring(1)), m_pathname(cursor.getstring(9)), m_note(cursor.getstring(11)),
          m_engineprofile(cursor.getstring(30)), m_coord(Point(cursor.getint(2), cursor.getint(3))), m_time(cursor.getint64(6)),
	  m_savetime(cursor.getint64(7)), m_flighttime(cursor.getint(28)), m_frequency(cursor.getint(5)), m_alt(cursor.getint(4)),
	  m_truealt(cursor.getint(16)), m_terrain(cursor.getint(17)), m_tropopause(cursor.getint(32)), m_dist(cursor.getint(21)), m_mass(cursor.getint(29)),
	  m_fuel(cursor.getint(22)), m_bhp(cursor.getint(31)), m_flags(cursor.getint(8)), m_winddir(cursor.getint(12)),
	  m_windspeed(cursor.getint(13)), m_qff(cursor.getint(14)), m_isaoffset(cursor.getint(15)),
	  m_tt(cursor.getint(18)), m_th(cursor.getint(19)), m_decl(cursor.getint(20)), m_tas(cursor.getint(23)),
	  m_rpm(cursor.getint(24)), m_mp(cursor.getint(25)), m_pathcode(static_cast<pathcode_t>(cursor.getint(10))),
	  m_type(static_cast<type_t>(cursor.getint(26))), m_navaid(static_cast<navaid_type_t>(cursor.getint(27))), m_dirty(false)
{
	if (cursor.isnull(17))
		m_terrain = invalid_altitude;
	if (cursor.isnull(26))
		m_type = type_undefined;
	if (cursor.isnull(27))
		m_navaid = navaid_invalid;
	if (cursor.isnull(32))
		m_tropopause = invalid_altitude;
}

Glib::ustring FPlanWaypoint::get_icao_name(void) const
{
        Glib::ustring r(m_icao);
        if (!m_icao.empty() && !m_name.empty())
                r += " ";
        r += m_name;
        return r;
}

const Glib::ustring& FPlanWaypoint::get_icao_or_name(void) const
{
	if (!m_icao.empty())
		return m_icao;
	return m_name;
}

const Glib::ustring& FPlanWaypoint::get_ident(void) const
{
	if (!m_icao.empty())
		return m_icao;
	return m_name;
}

const std::string& FPlanWaypoint::get_pathcode_name(pathcode_t pc)
{
	switch (pc) {
	case pathcode_none:
	{
		static const std::string r("None");
		return r;
	}

	case pathcode_sid:
	{
		static const std::string r("SID");
		return r;
	}

	case pathcode_star:
	{
		static const std::string r("STAR");
		return r;
	}

	case pathcode_approach:
	{
		static const std::string r("Approach");
		return r;
	}

	case pathcode_transition:
	{
		static const std::string r("Transition");
		return r;
	}

	case pathcode_vfrdeparture:
	{
		static const std::string r("VFR Departure");
		return r;
	}

	case pathcode_vfrarrival:
	{
		static const std::string r("VFR Arrival");
		return r;
	}

	case pathcode_vfrtransition:
	{
		static const std::string r("VFR Transition");
		return r;
	}

	case pathcode_airway:
	{
		static const std::string r("Airway");
		return r;
	}

	case pathcode_directto:
	{
		static const std::string r("Direct To");
		return r;
	}

	default:
	{
		static const std::string r("Invalid");
		return r;
	}

	}
}

bool FPlanWaypoint::set_pathcode_name(const std::string& p)
{
	for (pathcode_t pc(pathcode_none); pc <= pathcode_directto; pc = static_cast<FPlanWaypoint::pathcode_t>(pc + 1)) {
		if (get_pathcode_name(pc) != p)
			continue;
		set_pathcode(pc);
		return true;
	}
	char *cp;
	unsigned int n(strtoul(p.c_str(), &cp, 10));
	if (cp != p.c_str() && !*cp && n >= pathcode_none && n <= pathcode_directto) {
		set_pathcode(static_cast<pathcode_t>(n));
		return true;
	}
	return false;
}

bool FPlanWaypoint::is_stay(const Glib::ustring& pn, unsigned int& nr, unsigned int& tm)
{
	if (pn.size() != 10)
		return false;
	if (!((pn[0] == 'S' || pn[0] == 's') && (pn[1] == 'T' || pn[1] == 't') &&
	      (pn[2] == 'A' || pn[2] == 'a') && (pn[3] == 'Y' || pn[3] == 'y') &&
	      (pn[4] >= '1' && pn[4] <= '9') && pn[5] == '/' && std::isdigit(pn[6]) &&
	      std::isdigit(pn[7]) && std::isdigit(pn[8]) && std::isdigit(pn[9])))
		return false;
	nr = pn[4] - '1';
	tm = (((pn[6] - '0') * 10 + pn[7] - '0') * 60 + ((pn[8] - '0') * 10 + pn[9] - '0')) * 60;
	return true;
}

bool FPlanWaypoint::is_stay(const Glib::ustring& pn)
{
	unsigned int nr, tm;
	return is_stay(pn, nr, tm);
}

bool FPlanWaypoint::is_stay(unsigned int& nr, unsigned int& tm) const
{
	if (get_pathcode() != pathcode_airway)
		return false;
	return is_stay(get_pathname(), nr, tm);
}

bool FPlanWaypoint::is_stay(void) const
{
	unsigned int nr, tm;
	return is_stay(nr, tm);
}

void FPlanWaypoint::set_stay(Glib::ustring& pn, unsigned int nr, unsigned int tm)
{
	tm += 30U;
	tm /= 60U;
	std::ostringstream oss;
	oss << "STAY" << (char)('1' + nr) << '/'
	    << std::setw(2) << std::setfill('0') << (tm / 60)
	    << std::setw(2) << std::setfill('0') << (tm % 60);
	pn = oss.str();
}

void FPlanWaypoint::set_stay(unsigned int nr, unsigned int tm)
{
	set_pathcode(pathcode_airway);
	Glib::ustring pn;
	set_stay(pn, nr, tm);
	set_pathname(pn);
}

std::string FPlanWaypoint::get_fpl_altstr(int32_t a, uint16_t f)
{
	if (a == invalid_altitude || a < 0)
		return "VFR";
	std::ostringstream oss;
	oss << ((f & altflag_standard) ? 'F' : 'A') << std::setw(3) << std::setfill('0') << ((a + 50) / 100);
	return oss.str();
}

int32_t FPlanWaypoint::round_altitude(int32_t alt, roundalt_t flags)
{
	if (alt == invalid_altitude)
		return alt;
	int32_t basicspc(500);
	if (!(flags & roundalt_rvsm) && alt >= 29000)
		basicspc *= 2;
	int32_t spc(basicspc * 2);
	if (flags & roundalt_forcevfr)
		alt += basicspc;
	switch (flags & roundalt_modemask) {
	case roundalt_round:
	default:
		alt += basicspc;
		break;

	case roundalt_floor:
		break;

	case roundalt_ceil:
		alt += spc - 1;
		break;
	}
	alt = (alt / spc) * spc;
	if (flags & roundalt_forcevfr)
		alt -= basicspc;
	return alt;
}

int32_t FPlanWaypoint::wpt_round_altitude(int32_t alt, roundalt_t flags) const
{
	if (!(flags & roundalt_rulesmask))
		flags |= is_ifr() ? roundalt_forceifr : roundalt_forcevfr;
	return round_altitude(alt, flags);
}

int FPlanWaypoint::tune_profile(const IntervalSet<int32_t>& alt)
{
	if (!is_altitude_valid())
		return -1;
	if (alt.is_inside(get_altitude()))
		return 0;
	for (IntervalSet<int32_t>::const_reverse_iterator ai(alt.rbegin()), ae(alt.rend()); ai != ae; ++ai) {
		if (ai->get_lower() > get_altitude())
			continue;
		if (ai->get_upper() > get_altitude())
			return 0;
		if (ai->get_upper() <= 500)
			return -1;
		int32_t alt(wpt_round_altitude(ai->get_upper() - 1, roundalt_floor | roundalt_rvsm));
		if (!ai->is_inside(alt))
			continue;
		set_altitude(alt);
		return 1;
	}
	return -1;
}

bool FPlanWaypoint::enforce_pathcode_vfrifr(void)
{
	Glib::ustring pn(get_pathname());
	{
		Glib::ustring::size_type n(pn.size()), i(0);
		while (i < n && std::isspace(pn[i]))
			++i;
		while (n > i) {
			--n;
			if (std::isspace(pn[n]))
				continue;
			++n;
			break;
		}
		pn = pn.substr(i, n - i);
	}
	FPlanWaypoint::pathcode_t pc(get_pathcode());
	if (is_ifr()) {
		pn = pn.uppercase();
		switch (pc) {
		case FPlanWaypoint::pathcode_none:
			pc = FPlanWaypoint::pathcode_airway;
			// fall through

		case FPlanWaypoint::pathcode_airway:
			if (pn.empty() || pn == "DCT")
				pc = FPlanWaypoint::pathcode_directto;
			break;

		default:
			pn.clear();
			pc = FPlanWaypoint::pathcode_directto;
			break;
		}
	} else {
		switch (pc) {
		case FPlanWaypoint::pathcode_vfrdeparture:
		case FPlanWaypoint::pathcode_vfrarrival:
		case FPlanWaypoint::pathcode_vfrtransition:
			break;

		default:
			pn.clear();
			pc = FPlanWaypoint::pathcode_none;
			break;
		}
	}
	bool work(false);
	if (get_pathcode() != pc) {
		set_pathcode(pc);
		work = true;
	}
	if (get_pathname() != pn) {
		set_pathname(pn);
		work = true;
	}
	return work;
}

char FPlanWaypoint::get_ruleschar(uint16_t flags)
{
	return (flags & altflag_ifr) ? 'I' : 'V';
}

char FPlanWaypoint::get_altchar(uint16_t flags)
{
	return (flags & altflag_standard) ? 'F' : 'A';
}

float FPlanWaypoint::get_sltemp_kelvin(void) const
{
	return IcaoAtmosphere<float>::std_sealevel_temperature + get_isaoffset_kelvin();
}

void FPlanWaypoint::set_sltemp_kelvin(float temp)
{
	set_isaoffset_kelvin(temp - IcaoAtmosphere<float>::std_sealevel_temperature);
}

float FPlanWaypoint::get_oat_kelvin(void) const
{
	if (!is_altitude_valid())
		return std::numeric_limits<float>::quiet_NaN();
	IcaoAtmosphere<float> atmo((get_flags() & altflag_standard) ? IcaoAtmosphere<float>::std_sealevel_pressure : get_qff_hpa(), get_sltemp_kelvin());
	float oat(0);
	atmo.altitude_to_pressure(0, &oat, get_altitude() * FPlanWaypoint::ft_to_m);
	return oat;
}

void FPlanWaypoint::set_oat_kelvin(float temp)
{
	IcaoAtmosphere<float> atmo((get_flags() & altflag_standard) ? IcaoAtmosphere<float>::std_sealevel_pressure : get_qff_hpa());
	float oat(0);
	atmo.altitude_to_pressure(0, &oat, get_altitude() * FPlanWaypoint::ft_to_m);
	set_isaoffset_kelvin(temp - oat);
}

float FPlanWaypoint::get_pressure_altitude(void) const
{
	if (get_flags() & altflag_standard)
		return get_altitude();
	IcaoAtmosphere<float> atmo(get_qff_hpa(), get_sltemp_kelvin());
        float p(0), temp(0), tempstd(0), alt(0);
        atmo.altitude_to_pressure(&p, &temp, get_altitude() * FPlanWaypoint::ft_to_m);
        atmo.std_pressure_to_altitude(&alt, &tempstd, p);
        return alt * FPlanWaypoint::m_to_ft;
}

float FPlanWaypoint::get_true_altitude(void) const
{
	if (!(get_flags() & altflag_standard))
		return get_altitude();
	IcaoAtmosphere<float> atmo(get_qff_hpa(), get_sltemp_kelvin());
	float p(0), tempstd(0), temp(0), alt(0);
	atmo.std_altitude_to_pressure(&p, &tempstd, get_altitude() * FPlanWaypoint::ft_to_m);
        atmo.pressure_to_altitude(&alt, &temp, p);
        return alt * FPlanWaypoint::m_to_ft;
}

float FPlanWaypoint::get_density_altitude(void) const
{
	AirData<float> ad;
	ad.set_qnh_tempoffs(get_qff() ? get_qff_hpa() : IcaoAtmosphere<float>::std_sealevel_pressure, get_isaoffset_kelvin());
	if (get_flags() & altflag_standard)
		ad.set_pressure_altitude(get_altitude());
	else
		ad.set_true_altitude(get_altitude());
        return ad.get_density_altitude();
}

int32_t FPlanWaypoint::get_msa(void) const
{
	int32_t a(get_terrain());
	if (a == invalid_altitude)
		return a;
	if (a >= 5000)
		a += 1000;
	a += 1000;
	return a;
}

const std::string& FPlanWaypoint::get_type_string(type_t typ, navaid_type_t navaid)
{
	switch (typ) {
	case type_airport:
	{
		static const std::string r("ARPT");
		return r;
	}

	case type_navaid:
		switch (navaid) {
		default:
		case navaid_invalid:
		{
			static const std::string r("NAVAID");
			return r;
		}

		case navaid_vor:
		{
			static const std::string r("VOR");
			return r;
		}

		case navaid_vordme:
		{
			static const std::string r("VORDME");
			return r;
		}

		case navaid_dme:
		{
			static const std::string r("DME");
			return r;
		}

		case navaid_ndb:
		{
			static const std::string r("NDB");
			return r;
		}

		case navaid_ndbdme:
		{
			static const std::string r("NDBDME");
			return r;
		}

		case navaid_vortac:
		{
			static const std::string r("VORTAC");
			return r;
		}

		case navaid_tacan:
		{
			static const std::string r("TACAN");
			return r;
		}
		}


	case type_intersection:
	{
		static const std::string r("INT");
		return r;
	}

	case type_mapelement:
	{
		static const std::string r("MAP");
		return r;
	}

	default:
	case type_undefined:
	{
		static const std::string r("UNDEF");
		return r;
	}

	case type_fplwaypoint:
	{
		static const std::string r("FPLWPT");
		return r;
	}

	case type_vfrreportingpt:
	{
		static const std::string r("VFRREPPT");
		return r;
	}

	case type_user:
	{
		static const std::string r("USER");
		return r;
	}

	case type_boc:
	{
		static const std::string r("BOC");
		return r;
	}

	case type_toc:
	{
		static const std::string r("TOC");
		return r;
	}

	case type_bod:
	{
		static const std::string r("BOD");
		return r;
	}

	case type_tod:
	{
		static const std::string r("TOD");
		return r;
	}

	case type_center:
	{
		static const std::string r("CENTER");
		return r;
	}
	}
}

bool FPlanWaypoint::set_type_string(const std::string& x)
{
	std::string xu(Glib::ustring(x).uppercase());
	for (type_t typ = type_airport; typ <= type_user; typ = (type_t)(typ + 1)) {
		if (typ != type_navaid) {
			if (get_type_string(typ) != xu)
				continue;
			set_type(typ);
			return true;
		}
		for (navaid_type_t nav = navaid_invalid; nav <= navaid_tacan; nav = (navaid_type_t)(nav + 1)) {
			if (get_type_string(typ, nav) != xu)
				continue;
			set_type(typ, nav);
			return true;
		}
	}
	return false;
}

void FPlanWaypoint::set_airport(const AirportsDb::Airport& el)
{
	set_type(type_airport);
	set_icao(el.get_icao());
	set_name(el.get_name());
	set_pathcode(FPlanWaypoint::pathcode_none);
	set_pathname("");
	set_coord(el.get_coord());
	set_altitude(el.get_elevation());
	set_flags(get_flags() & ~FPlanWaypoint::altflag_standard);
	set_frequency(0);
	Glib::ustring note("Airport Type: " + el.get_type_string());
	if (el.get_nrrwy() > 0)
		note += "\n\nRunways:";
	for (unsigned int nr(0); nr < el.get_nrrwy(); ++nr) {
		const AirportsDb::Airport::Runway& rwy(el.get_rwy(nr));
		if (rwy.get_flags() & AirportsDb::Airport::Runway::flag_le_usable) {
			std::ostringstream oss;
			oss << "\n  " << std::setw(4) << rwy.get_ident_le() << ' '
			    << std::setw(4) << Conversions::track_str(rwy.get_le_hdg() * FPlanLeg::to_deg) << ' '
			    << std::fixed << std::setw(4) << std::setprecision(0) << (rwy.get_length() * Point::ft_to_m_dbl) << 'x'
			    << std::fixed << std::setprecision(0) << (rwy.get_width() * Point::ft_to_m_dbl) << ' '
			    << std::fixed << std::setw(4) << std::setprecision(0) << (rwy.get_le_tda() * Point::ft_to_m_dbl) << ' '
			    << std::fixed << std::setw(4) << std::setprecision(0) << (rwy.get_le_lda() * Point::ft_to_m_dbl) << ' '
			    << std::setw(4) << Conversions::alt_str(rwy.get_le_elev(), 0) << ' ' << rwy.get_surface();
			note += oss.str();
			Glib::ustring lights;
			for (unsigned int i = 0; i < AirportsDb::Airport::Runway::nr_lights; ++i) {
				AirportsDb::Airport::Runway::light_t l(rwy.get_le_light(i));
				if (l == AirportsDb::Airport::Runway::light_off)
					continue;
				if (!lights.empty())
					lights += "; ";
				lights += AirportsDb::Airport::Runway::get_light_string(l);
			}
			note += " " + lights;
		}
		if (rwy.get_flags() & AirportsDb::Airport::Runway::flag_he_usable) {
			std::ostringstream oss;
			oss << "\n  " << std::setw(4) << rwy.get_ident_he() << ' '
			    << std::setw(4) << Conversions::track_str(rwy.get_he_hdg() * FPlanLeg::to_deg) << ' '
			    << std::fixed << std::setw(4) << std::setprecision(0) << (rwy.get_length() * Point::ft_to_m_dbl) << 'x'
			    << std::fixed << std::setprecision(0) << (rwy.get_width() * Point::ft_to_m_dbl) << ' '
			    << std::fixed << std::setw(4) << std::setprecision(0) << (rwy.get_he_tda() * Point::ft_to_m_dbl) << ' '
			    << std::fixed << std::setw(4) << std::setprecision(0) << (rwy.get_he_lda() * Point::ft_to_m_dbl) << ' '
			    << std::setw(4) << Conversions::alt_str(rwy.get_he_elev(), 0) << ' ' << rwy.get_surface();
			note += oss.str();
			Glib::ustring lights;
			for (unsigned int i = 0; i < AirportsDb::Airport::Runway::nr_lights; ++i) {
				AirportsDb::Airport::Runway::light_t l(rwy.get_he_light(i));
				if (l == AirportsDb::Airport::Runway::light_off)
					continue;
				if (!lights.empty())
					lights += "; ";
				lights += AirportsDb::Airport::Runway::get_light_string(l);
			}
			note += " " + lights;
		}
	}
	if (el.get_nrhelipads() > 0)
		note += "\n\nHelipads:";
	for (unsigned int nr(0); nr < el.get_nrhelipads(); ++nr) {
		const AirportsDb::Airport::Helipad& helipad(el.get_helipad(nr));
		if (!(helipad.get_flags() & AirportsDb::Airport::Helipad::flag_usable))
			continue;
		 std::ostringstream oss;
		 oss << "\n  " << std::setw(4) << helipad.get_ident() << ' '
		     << std::setw(4) << Conversions::track_str(helipad.get_hdg() * FPlanLeg::to_deg) << ' '
		     << std::fixed << std::setw(4) << std::setprecision(0) << (helipad.get_length() * Point::ft_to_m_dbl) << 'x'
		     << std::fixed << std::setprecision(0) << (helipad.get_width() * Point::ft_to_m_dbl) << ' '
		     << std::setw(4) << Conversions::alt_str(helipad.get_elev(), 0);
		 note += oss.str();
	}
	if (el.get_nrcomm() > 0) {
		note += "\n\nComm:";
		unsigned int prio(0);
		Glib::ustring twr("twr");
		twr = twr.casefold();
		for (unsigned int nr(0); nr < el.get_nrcomm(); ++nr) {
			const AirportsDb::Airport::Comm& comm(el.get_comm(nr));
			note += "\n  " + comm.get_name() + " " + Conversions::freq_str(comm[0]) + " " + Conversions::freq_str(comm[1]) + " " +
				Conversions::freq_str(comm[2]) + " " + Conversions::freq_str(comm[3]) + " " + Conversions::freq_str(comm[4]) + " " +
				comm.get_sector() + " " + comm.get_oprhours();
			if (!comm[0])
				continue;
			unsigned int p(1);
			Glib::ustring name(comm.get_name().casefold());
			if (name == twr)
				p = 3;
			else if (name.find(twr) != Glib::ustring::npos)
				p = 2;
			if (p <= prio)
				continue;
			set_frequency(comm[0]);
		}
	}
	if (!el.get_vfrrmk().empty())
		note += "\n\n" + el.get_vfrrmk();
	set_note(note);
}

void FPlanWaypoint::set_navaid(const NavaidsDb::Navaid& el)
{
	set_type(type_navaid, static_cast<navaid_type_t>(el.get_navaid_type()));
	set_flags(get_flags() & ~FPlanWaypoint::altflag_standard);
	set_icao(el.get_icao());
	set_name(el.get_name());
	set_pathcode(FPlanWaypoint::pathcode_none);
	set_pathname("");
	set_coord(el.get_coord());
	set_altitude(el.get_elev());
	set_frequency(el.get_frequency());
	std::ostringstream range;
	range << el.get_range();
	Glib::ustring note(el.get_navaid_typename() + " Range " + range.str() + " ");
	if (el.is_inservice())
		note += "in";
	else
		note += "out of";
	note += " service";
	set_note(note);
}

void FPlanWaypoint::set_waypoint(const WaypointsDb::Waypoint& el)
{
	set_type(type_intersection);
	set_icao(el.get_icao());
	set_name(el.get_name());
	set_pathcode(FPlanWaypoint::pathcode_none);
	set_pathname("");
	set_coord(el.get_coord());
	set_note(el.get_usage_name());
}

void FPlanWaypoint::set_mapelement(const MapelementsDb::Mapelement& el)
{
	set_type(type_mapelement);
	set_name(el.get_name());
	set_coord(el.get_coord());
	set_note(el.get_typename());
	set_pathcode(FPlanWaypoint::pathcode_none);
	set_pathname("");
}

void FPlanWaypoint::set_wpt(const FPlanWaypoint& el)
{
	*this = el;
}

std::string FPlanWaypoint::to_str(void) const
{
	std::ostringstream oss;
	oss << (std::string)get_icao() << '/' << (std::string)get_name() << ' ';
	if (get_pathcode() == FPlanWaypoint::pathcode_none) {
		oss << '-';
	} else {
		oss << FPlanWaypoint::get_pathcode_name(get_pathcode());
		switch (get_pathcode()) {
		case FPlanWaypoint::pathcode_airway:
		case FPlanWaypoint::pathcode_sid:
		case FPlanWaypoint::pathcode_star:
			oss << ' ' << get_pathname();
			break;

		default:
			break;
		}
	}
	if (!get_coord().is_invalid())
		oss << ' ' << get_coord().get_lat_str2() << ' ' << get_coord().get_lon_str2();
	oss << ' ' << get_fpl_altstr();
	if (is_altvfr())
		oss << " [VFR]";
	oss << ' ' << (is_ifr() ? 'I' : 'V') << "FR ";
	if (is_oat())
		oss << "OAT ";
	oss << get_type_string() << ' ' << Glib::TimeVal(get_time_unix(), 0).as_iso8601()
	    << ' ' << std::setfill('0') << std::setw(2) << (get_flighttime() / 3600)
	    << ':' << std::setfill('0') << std::setw(2) << (get_flighttime() / 60 % 60)
	    << ':' << std::setfill('0') << std::setw(2) << (get_flighttime() % 60);
	return oss.str();
}

std::string FPlanWaypoint::to_detailed_str(void) const
{
	std::ostringstream oss;
	oss << get_icao() << '/' << get_name() << ' ';
	if (get_pathcode() == FPlanWaypoint::pathcode_none) {
		oss << '-';
	} else {
		oss << FPlanWaypoint::get_pathcode_name(get_pathcode());
		switch (get_pathcode()) {
		case FPlanWaypoint::pathcode_airway:
		case FPlanWaypoint::pathcode_sid:
		case FPlanWaypoint::pathcode_star:
			oss << ' ' << get_pathname();
			break;

		default:
			break;
		}
	}
	if (!get_coord().is_invalid())
		oss << ' ' << get_coord().get_lat_str2() << ' ' << get_coord().get_lon_str2();
	oss << " D" << std::fixed << std::setprecision(1) << get_dist_nmi()
	    << " T" << std::fixed << std::setprecision(0) << get_truetrack_deg()
	    << ' ' << get_fpl_altstr();
	if (is_altvfr())
		oss << " [VFR]";
	oss << ' ' << (is_ifr() ? 'I' : 'V') << "FR ";
	if (is_oat())
		oss << "OAT ";
	oss << 'M' << std::fixed << std::setprecision(0) << get_mass_kg() << ' '
	    << std::setw(2) << std::setfill('0') << (get_flighttime() / 3600) << ':'
	    << std::setw(2) << std::setfill('0') << ((get_flighttime() / 60) % 60) << ':'
	    << std::setw(2) << std::setfill('0') << (get_flighttime() % 60)
	    << " F" << std::fixed << std::setprecision(1) << get_fuel_usg()
	    << " TAS" << std::fixed << std::setprecision(0) << get_tas_kts()
	    << ' ' << get_truealt() << '\'';
	if (is_terrain_valid())
		oss << " T" << get_terrain();
	if (is_tropopause_valid())
		oss << " TP" << get_tropopause();
	oss << " flags 0x"
	    << std::hex << std::setfill('0') << std::setw(4) << get_flags() << std::dec
	    << ' ' << std::setw(3) << std::setfill('0') << Point::round<int,double>(get_winddir_deg())
	    << '/' << std::setw(2) << std::setfill('0') << Point::round<int,double>(get_windspeed_kts())
	    << " ISA" << std::showpos << std::fixed << std::setprecision(1) << get_isaoffset_kelvin()
	    << " Q" << std::noshowpos << std::fixed << std::setprecision(1) << get_qff_hpa();
	if (get_bhp_raw())
		oss << " BHP" << std::fixed << std::setprecision(1) << get_bhp();
	if (get_rpm())
		oss << " RPM" << get_rpm();
	if (get_mp())
		oss << " MP" << get_mp_inhg();
	if (!get_engineprofile().empty())
		oss << ' ' << (std::string)get_engineprofile();
	return oss.str();
}

#ifdef HAVE_JSONCPP

Json::Value FPlanWaypoint::to_json(void) const
{
	Json::Value root;
	if (get_icao().empty()) {
		root["ident"] = (std::string)get_name();
	} else {
		root["ident"] = (std::string)get_icao();
		root["name"] = (std::string)get_name();
	}
	if (get_pathcode() != pathcode_none) {
		root["pathcode"] = get_pathcode_name();
		if (!get_pathname().empty())
			root["pathname"] = (std::string)get_pathname();
	}
	{
		unsigned int nr, tm;
		if (is_stay(nr, tm)) {
			root["stay"] = nr;
			root["stayduration"] = tm;
		}
	}
	if (!get_note().empty())
		root["note"] = (std::string)get_note();
	root["time"] = (Json::Value::Int64)get_time_unix();
	root["flighttime"] = get_flighttime();
	if (get_frequency())
		root["freq"] = get_frequency();
	if (!get_coord().is_invalid()) {
		root["coordlatdeg"] = get_coord().get_lat_deg_dbl();
		root["coordlondeg"] = get_coord().get_lon_deg_dbl();
	}
	if (is_altitude_valid())
		root["alt"] = get_altitude();
	altflags_to_json(root, get_flags());
	root["winddir"] = get_winddir_deg();
	root["windspeed"] = get_windspeed_kts();
	if (get_qff())
		root["qff"] = get_qff_hpa();
	root["isaoffset"] = get_isaoffset_kelvin();
	if (is_tropopause_valid())
		root["tropopause"] = get_tropopause();
	root["truealt"] = get_truealt();
	if (is_terrain_valid())
		root["terrain"] = get_terrain();
	root["tt"] = get_truetrack_deg();
	root["th"] = get_trueheading_deg();
	root["mt"] = get_magnetictrack_deg();
	root["mh"] = get_magneticheading_deg();
	root["decl"] = get_declination_deg();
	root["dist"] = get_dist_nmi();
	if (get_mass())
		root["mass"] = get_mass_kg();
	root["fuel"] = get_fuel_usg();
	root["tas"] = get_tas_kts();
	if (!get_engineprofile().empty())
		root["engine"] = (std::string)get_engineprofile();
	if (get_bhp_raw())
		root["bhp"] = get_bhp();
	if (get_rpm())
		root["rpm"] = get_rpm();
	if (get_mp())
		root["mp"] = get_mp_inhg();
	if (get_type() != type_undefined)
		root["type"] = get_type_string();
	return root;
}

void FPlanWaypoint::from_json(const Json::Value& root)
{
	m_icao.clear();
	m_name.clear();
	m_pathname.clear();
	m_note.clear();
	m_engineprofile.clear();
	m_coord.set_invalid();
	m_time = 0;
	m_savetime = 0;
	m_flighttime = 0;
	m_frequency = 0;
	m_alt = invalid_altitude;
	m_truealt = 0;
	m_terrain = invalid_altitude;
	m_tropopause = invalid_altitude;
	m_dist = 0;
	m_mass = 0;
	m_fuel = 0;
	m_bhp = 0;
	m_flags = 0;
	m_winddir = 0;
	m_windspeed = 0;
	m_qff = IcaoAtmosphere<float>::std_sealevel_pressure * 32;
	m_isaoffset = 0;
	m_tt = 0;
	m_th = 0;
	m_decl = 0;
	m_tas = 0;
	m_rpm = 0;
	m_mp = 0;
	m_pathcode = pathcode_none;
	m_type = type_undefined;
	m_navaid = navaid_invalid;
	m_dirty = true;

	if (root.isMember("ident")) {
		const Json::Value& x(root["ident"]);
		if (x.isString())
			set_icao(x.asString());
	}
	if (root.isMember("name")) {
		const Json::Value& x(root["name"]);
		if (x.isString())
			set_name(x.asString());
	}
	if (root.isMember("pathcode")) {
		const Json::Value& x(root["pathcode"]);
		if (x.isString())
			set_pathcode_name(x.asString());
	}
	if (root.isMember("pathname")) {
		const Json::Value& x(root["pathname"]);
		if (x.isString())
			set_pathname(x.asString());
	}
	if (root.isMember("stay") && root.isMember("stayduration")) {
		const Json::Value& x(root["stay"]);
		const Json::Value& xd(root["stayduration"]);
		if (x.isIntegral() && xd.isIntegral())
			set_stay(x.asInt(), xd.asInt());
	}
	if (root.isMember("note")) {
		const Json::Value& x(root["note"]);
		if (x.isString())
			set_note(x.asString());
	}
	if (root.isMember("time")) {
		const Json::Value& x(root["time"]);
		if (x.isIntegral())
			set_time_unix(x.asInt());
	}
	if (root.isMember("flighttime")) {
		const Json::Value& x(root["flighttime"]);
		if (x.isIntegral())
			set_flighttime(x.asInt());
	}
	if (root.isMember("freq")) {
		const Json::Value& x(root["freq"]);
		if (x.isIntegral())
			set_frequency(x.asInt());
	}
	if (root.isMember("coordlatdeg") && root.isMember("coordlondeg")) {
		const Json::Value& lat(root["coordlatdeg"]);
		const Json::Value& lon(root["coordlondeg"]);
		if (lat.isNumeric() && lon.isNumeric()) {
			Point pt;
			pt.set_lat_deg_dbl(lat.asDouble());
			pt.set_lon_deg_dbl(lon.asDouble());
			set_coord(pt);
		}
	}
	if (root.isMember("alt")) {
		const Json::Value& x(root["alt"]);
		if (x.isIntegral())
			set_altitude(x.asInt());
	}
	set_flags(altflags_from_json(root));
	if (root.isMember("winddir")) {
		const Json::Value& x(root["winddir"]);
		if (x.isNumeric())
			set_winddir_deg(x.asDouble());
	}
	if (root.isMember("windspeed")) {
		const Json::Value& x(root["windspeed"]);
		if (x.isNumeric())
			set_windspeed_kts(x.asDouble());
	}
	if (root.isMember("qff")) {
		const Json::Value& x(root["qff"]);
		if (x.isNumeric())
			set_qff_hpa(x.asDouble());
	}
	if (root.isMember("isaoffset")) {
		const Json::Value& x(root["isaoffset"]);
		if (x.isNumeric())
			set_isaoffset_kelvin(x.asDouble());
	}
	if (root.isMember("tropopause")) {
		const Json::Value& x(root["tropopause"]);
		if (x.isIntegral())
			set_tropopause(x.asInt());
	}
	if (root.isMember("truealt")) {
		const Json::Value& x(root["truealt"]);
		if (x.isIntegral())
			set_truealt(x.asInt());
	}
	if (root.isMember("terrain")) {
		const Json::Value& x(root["terrain"]);
		if (x.isIntegral())
			set_terrain(x.asInt());
	}
	if (root.isMember("decl")) {
		const Json::Value& x(root["decl"]);
		if (x.isNumeric())
			set_declination_deg(x.asDouble());
	}
	if (root.isMember("mt")) {
		const Json::Value& x(root["mt"]);
		if (x.isNumeric())
			set_truetrack_deg(x.asDouble() + get_declination_deg());
	}
	if (root.isMember("mh")) {
		const Json::Value& x(root["mh"]);
		if (x.isNumeric())
			set_trueheading_deg(x.asDouble() + get_declination_deg());
	}
	if (root.isMember("tt")) {
		const Json::Value& x(root["tt"]);
		if (x.isNumeric())
			set_truetrack_deg(x.asDouble());
	}
	if (root.isMember("th")) {
		const Json::Value& x(root["th"]);
		if (x.isNumeric())
			set_trueheading_deg(x.asDouble());
	}
	if (root.isMember("dist")) {
		const Json::Value& x(root["dist"]);
		if (x.isNumeric())
			set_dist_nmi(x.asDouble());
	}
	if (root.isMember("mass")) {
		const Json::Value& x(root["mass"]);
		if (x.isNumeric())
			set_mass_kg(x.asDouble());
	}
	if (root.isMember("fuel")) {
		const Json::Value& x(root["fuel"]);
		if (x.isNumeric())
			set_fuel_usg(x.asDouble());
	}
	if (root.isMember("tas")) {
		const Json::Value& x(root["tas"]);
		if (x.isNumeric())
			set_tas_kts(x.asDouble());
	}
	if (root.isMember("engine")) {
		const Json::Value& x(root["engine"]);
		if (x.isString())
			set_engineprofile(x.asString());
	}
	if (root.isMember("bhp")) {
		const Json::Value& x(root["bhp"]);
		if (x.isNumeric())
			set_bhp(x.asDouble());
	}
	if (root.isMember("rpm")) {
		const Json::Value& x(root["rpm"]);
		if (x.isIntegral())
			set_rpm(x.asInt());
	}
	if (root.isMember("mp")) {
		const Json::Value& x(root["mp"]);
		if (x.isNumeric())
			set_mp_inhg(x.asDouble());
	}
	if (root.isMember("type")) {
		const Json::Value& x(root["type"]);
		if (x.isString())
			set_type_string(x.asString());
	}

	if (get_type() == type_intersection && m_name.empty())
		m_name.swap(m_icao);
}

Json::Value& FPlanWaypoint::altflags_to_json(Json::Value& root, uint16_t flg)
{
	root["standard"] = !!(flg & altflag_standard);
	root["climb"] = !!(flg & altflag_climb);
	root["descent"] = !!(flg & altflag_descent);
	root["oat"] = !!(flg & altflag_oat);
	root["altvfr"] = !!(flg & altflag_altvfr);
	root["partialstandardroute"] = !!(flg & altflag_partialstandardroute);
	root["standardroute"] = !!(flg & altflag_standardroute);
	root["turnpoint"] = !!(flg & altflag_turnpoint);
	root["ifr"] = !!(flg & altflag_ifr);
	return root;
}

uint16_t FPlanWaypoint::altflags_from_json(const Json::Value& root)
{
	uint16_t flg(0);
	if (root.isMember("standard")) {
		const Json::Value& x(root["standard"]);
		if (x.isBool() && x.asBool())
			flg |= altflag_standard;
	}
	if (root.isMember("climb")) {
		const Json::Value& x(root["climb"]);
		if (x.isBool() && x.asBool())
			flg |= altflag_climb;
	}
	if (root.isMember("descent")) {
		const Json::Value& x(root["descent"]);
		if (x.isBool() && x.asBool())
			flg |= altflag_descent;
	}
	if (root.isMember("oat")) {
		const Json::Value& x(root["oat"]);
		if (x.isBool() && x.asBool())
			flg |= altflag_oat;
	}
	if (root.isMember("altvfr")) {
		const Json::Value& x(root["altvfr"]);
		if (x.isBool() && x.asBool())
			flg |= altflag_altvfr;
	}
	if (root.isMember("partialstandardroute")) {
		const Json::Value& x(root["partialstandardroute"]);
		if (x.isBool() && x.asBool())
			flg |= altflag_partialstandardroute;
	}
	if (root.isMember("standardroute")) {
		const Json::Value& x(root["standardroute"]);
		if (x.isBool() && x.asBool())
			flg |= altflag_standardroute;
	}
	if (root.isMember("turnpoint")) {
		const Json::Value& x(root["turnpoint"]);
		if (x.isBool() && x.asBool())
			flg |= altflag_turnpoint;
	}
	if (root.isMember("ifr")) {
		const Json::Value& x(root["ifr"]);
		if (x.isBool() && x.asBool())
			flg |= altflag_ifr;
	}
	return flg;
}

#endif

DbBaseElements::Airport FPlanWaypoint::find_airport(AirportsDbQueryInterface& db) const
{
	if (get_coord().is_invalid())
		return DbBaseElements::Airport();
	if (get_icao() != "ZZZZ") {
		AirportsDbQueryInterface::elementvector_t ev(db.find_by_icao(get_icao(), 0, AirportsDbQueryInterface::comp_exact, 0, DbBaseElements::Airport::subtables_all));
		AirportsDbQueryInterface::elementvector_t::size_type j(ev.size());
		float m(10.0f);
		Glib::ustring icao(get_icao().casefold());
		for (AirportsDbQueryInterface::elementvector_t::size_type i(0), n(ev.size()); i < n; ++i) {
			const AirportsDb::Airport& arpt(ev[i]);
			if (!arpt.is_valid() || arpt.get_coord().is_invalid() || icao != arpt.get_icao().casefold())
				continue;
			float m0(get_coord().simple_distance_nmi(arpt.get_coord()));
			if (m0 >= m)
				continue;
			m = m0;
			j = i;
		}
		if (j < ev.size())
			return ev[j];
		return DbBaseElements::Airport();
	}
	if (!DbBaseElements::Airport::is_fpl_zzzz(get_icao()))
		return DbBaseElements::Airport();
	AirportsDbQueryInterface::elementvector_t ev(db.find_by_rect(get_coord().simple_box_nmi(3.5f), 0, DbBaseElements::Airport::subtables_all));
	AirportsDbQueryInterface::elementvector_t::size_type j(ev.size());
	float m(3.0f);
	for (AirportsDbQueryInterface::elementvector_t::size_type i(0), n(ev.size()); i < n; ++i) {
		const DbBaseElements::Airport& arpt(ev[i]);
		if (!arpt.is_valid() || arpt.get_coord().is_invalid() ||
		    !DbBaseElements::Airport::is_fpl_zzzz(arpt.get_icao()))
			continue;
		float m0(get_coord().simple_distance_nmi(arpt.get_coord()));
		if (m0 >= m)
			continue;
		m = m0;
		j = i;
	}
	if (j < ev.size())
		return ev[j];
	return DbBaseElements::Airport();
}

DbBaseElements::Mapelement FPlanWaypoint::find_mapelement(MapelementsDbQueryInterface& db, double maxdist, uint64_t minarea,
							  bool prefer_area, const Point& ptanchor) const
{
	const Glib::ustring& nm(get_icao_or_name());
	if (nm.empty())
		return DbBaseElements::Mapelement();
	const Point& ptanch(ptanchor.is_invalid() ? get_coord() : ptanchor);
	if (ptanch.is_invalid())
		return DbBaseElements::Mapelement();
	Glib::ustring nmcf(nm.casefold());
	MapelementsDbQueryInterface::elementvector_t ev(db.find_by_name(nm, 0, MapelementsDbQueryInterface::comp_exact,
									0, DbBaseElements::Mapelement::subtables_all));
	double dist(maxdist);
	uint64_t area(minarea);
	MapelementsDbQueryInterface::elementvector_t::size_type j(ev.size());
	for (MapelementsDbQueryInterface::elementvector_t::size_type i(0), n(ev.size()); i < n; ++i) {
		const DbBaseElements::Mapelement& mel(ev[i]);
		if (!mel.is_valid() || nmcf != mel.get_name().casefold())
			continue;
		switch (mel.get_typecode()) {
                case DbBaseElements::Mapelement::maptyp_lake:
                case DbBaseElements::Mapelement::maptyp_lake_t:
                case DbBaseElements::Mapelement::maptyp_city:
                case DbBaseElements::Mapelement::maptyp_village:
                case DbBaseElements::Mapelement::maptyp_spot:
                case DbBaseElements::Mapelement::maptyp_landmark:
			break;

		default:
			continue;
		}
		uint64_t a(std::abs(mel.get_polygon().area2()));
		if (a <= area)
			continue;
		Point p(mel.get_exterior().centroid());
		if (p.is_invalid()) {
			p = mel.get_labelcoord();
			if (p.is_invalid())
				continue;
		}
		double d(ptanch.spheric_distance_nmi_dbl(p));
		if (d >= dist)
			continue;
		j = i;
		if (prefer_area)
			area = a;
		else
			dist = d;
	}
	if (j >= ev.size())
		return DbBaseElements::Mapelement();
	return ev[j];
}

FPlanAlternate::FPlanAlternate(void)
	: m_cruisealt(invalid_altitude), m_holdalt(invalid_altitude), m_holdtime(0), m_holdfuel(0)
{
}

FPlanAlternate::FPlanAlternate(const FPlanWaypoint& w)
	: FPlanWaypoint(w), m_cruisealt(invalid_altitude), m_holdalt(invalid_altitude), m_holdtime(0), m_holdfuel(0)
{
}

std::string FPlanAlternate::to_str(void) const
{
	std::ostringstream oss;
	oss << FPlanWaypoint::to_str();
	if (is_cruisealtitude_valid())
		oss << " CRUISE F" << std::setw(3) << std::setfill('0') << ((get_cruisealtitude() + 50) / 100);
	oss << " HOLD";
	if (is_holdaltitude_valid())
		oss << " F" << std::setw(3) << std::setfill('0') << ((get_holdaltitude() + 50) / 100);
	oss << ' ' << std::setw(2) << std::setfill('0') << (get_holdtime() / 3600) << ':'
	    << std::setw(2) << std::setfill('0') << ((get_holdtime() / 60) % 60) << ':'
	    << std::setw(2) << std::setfill('0') << (get_holdtime() % 60)
	    << " F" << std::fixed << std::setprecision(1) << get_holdfuel_usg();
	return oss.str();
}

std::string FPlanAlternate::to_detailed_str(void) const
{
	std::ostringstream oss;
	oss << FPlanWaypoint::to_str();
	if (is_cruisealtitude_valid())
		oss << " CRUISE F" << std::setw(3) << std::setfill('0') << ((get_cruisealtitude() + 50) / 100);
	oss << " HOLD";
	if (is_holdaltitude_valid())
		oss << " F" << std::setw(3) << std::setfill('0') << ((get_holdaltitude() + 50) / 100);
	oss << ' ' << std::setw(2) << std::setfill('0') << (get_holdtime() / 3600) << ':'
	    << std::setw(2) << std::setfill('0') << ((get_holdtime() / 60) % 60) << ':'
	    << std::setw(2) << std::setfill('0') << (get_holdtime() % 60)
	    << " F" << std::fixed << std::setprecision(1) << get_holdfuel_usg();
	return oss.str();
}

#ifdef HAVE_JSONCPP

Json::Value FPlanAlternate::to_json(void) const
{
	Json::Value root(FPlanWaypoint::to_json());
	if (is_cruisealtitude_valid())
		root["cruisealt"] = get_cruisealtitude();
	if (is_holdaltitude_valid())
		root["holdalt"] = get_holdaltitude();
	root["holdtime"] = get_holdtime();
	root["holdfuel"] = get_holdfuel_usg();
	root["totaltime"] = get_totaltime();
	root["totalfuel"] = get_totalfuel_usg();
	return root;
}

void FPlanAlternate::from_json(const Json::Value& root)
{
	m_cruisealt = 0;
	m_holdalt = 0;
	m_holdtime = 0;
	m_holdfuel = 0;
	FPlanWaypoint::from_json(root);
	if (root.isMember("cruisealt")) {
		const Json::Value& x(root["cruisealt"]);
		if (x.isIntegral())
			set_cruisealtitude(x.asInt());
	}
	if (root.isMember("holdalt")) {
		const Json::Value& x(root["holdalt"]);
		if (x.isIntegral())
			set_holdaltitude(x.asInt());
	}
	if (root.isMember("holdtime")) {
		const Json::Value& x(root["holdtime"]);
		if (x.isIntegral())
			set_holdtime(x.asInt());
	}
	if (root.isMember("holdfuel")) {
		const Json::Value& x(root["holdfuel"]);
		if (x.isNumeric())
			set_holdfuel_usg(x.asDouble());
	}
}

#endif

FPlanRoute::GFSResult::GFSResult(void)
	: m_minreftime(std::numeric_limits<gint64>::max()), m_maxreftime(std::numeric_limits<gint64>::min()),
	  m_minefftime(std::numeric_limits<gint64>::max()), m_maxefftime(std::numeric_limits<gint64>::min()),
	  m_modified(false), m_partial(false)
{
}

void FPlanRoute::GFSResult::add(const GFSResult& r)
{
	m_minreftime = std::min(m_minreftime, r.m_minreftime);
	m_maxreftime = std::max(m_maxreftime, r.m_maxreftime);
	m_minefftime = std::min(m_minefftime, r.m_minefftime);
	m_maxefftime = std::max(m_maxefftime, r.m_maxefftime);
	m_modified = m_modified || r.m_modified;
	m_partial = m_partial || r.m_partial;
}


FPlanRoute::ClimbDescentProfilePoint::ClimbDescentProfilePoint(void)
	: m_flighttime(0), m_alt(FPlanWaypoint::invalid_altitude), m_dist(0), m_fuel(0)
{
}

#ifdef HAVE_JSONCPP

Json::Value FPlanRoute::ClimbDescentProfilePoint::to_json(void) const
{
	Json::Value root;
	root["flighttime"] = get_flighttime();
	if (is_altitude_valid())
		root["alt"] = get_altitude();
	root["dist"] = get_dist_nmi();
	root["fuel"] = get_fuel_usg();
	return root;
}

void FPlanRoute::ClimbDescentProfilePoint::from_json(const Json::Value& root)
{
	m_flighttime = 0;
	m_alt = FPlanWaypoint::invalid_altitude;
	m_dist = 0;
	m_fuel = 0;
	if (root.isMember("flighttime")) {
		const Json::Value& x(root["flighttime"]);
		if (x.isIntegral())
			set_flighttime(x.asInt());
	}
	if (root.isMember("alt")) {
		const Json::Value& x(root["alt"]);
		if (x.isIntegral())
			set_altitude(x.asInt());
	}
	if (root.isMember("dist")) {
		const Json::Value& x(root["dist"]);
		if (x.isNumeric())
			set_dist_nmi(x.asDouble());
	}
	if (root.isMember("fuel")) {
		const Json::Value& x(root["fuel"]);
		if (x.isNumeric())
			set_fuel_usg(x.asDouble());
	}
}

#endif

void FPlanRoute::ClimbDescentProfile::push_back(const Aircraft::ClimbDescent& cd, int32_t alt)
{
	double t(cd.altitude_to_time(alt));
	ClimbDescentProfilePoint pp;
	pp.set_flighttime(Point::round<int32_t,double>(t));
	pp.set_altitude(alt);
	pp.set_dist_nmi(cd.time_to_distance(t));
	pp.set_fuel_usg(cd.time_to_fuel(t));
	push_back(pp);
}

#ifdef HAVE_JSONCPP

Json::Value FPlanRoute::ClimbDescentProfile::to_json(void) const
{
	Json::Value root(Json::arrayValue);
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		root.append(i->to_json());
	return root;
}

void FPlanRoute::ClimbDescentProfile::from_json(const Json::Value& root)
{
	clear();
	if (!root.isArray())
		return;
	for (Json::ArrayIndex ri = 0; ri < root.size(); ++ri) {
		ClimbDescentProfilePoint pp;
		pp.from_json(root[ri]);
		push_back(pp);
	}
}

#endif

FPlanRoute::LevelChange::LevelChange(type_t t, double dist, int32_t alt, time_t flttime)
	: m_dist(dist), m_flttime(flttime), m_alt(alt), m_type(t)
{
}

FPlanRoute::Profile::Profile(void)
	: m_dist(std::numeric_limits<double>::quiet_NaN())
{
}

double FPlanRoute::Profile::get_distat(unsigned int x) const
{
	distat_t::const_iterator i(m_distat.find(x));
	if (i == m_distat.end())
		return std::numeric_limits<double>::quiet_NaN();
	return i->second;
}

void FPlanRoute::Profile::set_distat(unsigned int x, double d)
{
	if (std::isnan(d)) {
		distat_t::iterator i(m_distat.find(x));
		if (i != m_distat.end())
			m_distat.erase(i);
		return;
	}
	m_distat[x] = d;
}

void FPlanRoute::Profile::remove_waypoint(unsigned int x)
{
	distat_t d;
	for (distat_t::const_iterator i(m_distat.begin()), e(m_distat.end()); i != e; ++i) {
		if (i->first == x)
			continue;
		d[i->first - (i->first > x)] = i->second;
	}
	m_distat.swap(d);
}

unsigned int FPlanRoute::Profile::remove_close_distat(double minsep)
{
	if (std::isnan(minsep))
		return 0;
	unsigned int rm(0);
	for (distat_t::iterator i(m_distat.begin()), e(m_distat.end()); i != e; ) {
		distat_t::iterator i0(i);
		++i;
		double i0s(i0->second), i0m(i0s + minsep);
		if (std::isnan(i0s) || i0s < minsep || (!std::isnan(m_dist) && i0m > m_dist)) {
			m_distat.erase(i0);
			++rm;
			continue;
		}
		if (i == e)
			break;
		if (i0m > i->second) {
			m_distat.erase(i0);
			++rm;
			continue;
		}
	}
	return rm;
}

void FPlanRoute::Profile::clear(void)
{
	std::vector<LevelChange>::clear();
	m_distat.clear();
	m_dist = std::numeric_limits<double>::quiet_NaN();
}

FPlanRoute::FPlanRoute(FPlan & fpp)
        : m_fp(&fpp), m_id(0), m_note(), m_time_offblock(0), m_savetime_offblock(0), m_time_onblock(0), m_savetime_onblock(0), m_curwpt(0), m_wpts(), m_dirty(false)
{
}

FPlanWaypoint& FPlanRoute::operator[](uint32_t nr)
{
        if (nr >= m_wpts.size())
                throw std::runtime_error("Waypoint index out of range");
        return m_wpts[nr];
}

const FPlanWaypoint& FPlanRoute::operator[](uint32_t nr) const
{
        if (nr >= m_wpts.size())
                throw std::runtime_error("Waypoint index out of range");
        return m_wpts[nr];
}

void FPlanRoute::insert_wpt(uint32_t nr, const FPlanWaypoint & wpt)
{
	m_dirty = true;
        if (nr >= m_wpts.size()) {
                m_wpts.push_back(wpt);
                return;
        }
        m_wpts.insert(m_wpts.begin() + nr, wpt);
        if (m_id && m_fp) {
                sqlite3x::sqlite3_transaction tran(m_fp->get_db(), true);
                {
                        sqlite3x::sqlite3_command cmd(m_fp->get_db(), "UPDATE waypoints SET NR = NR + 100000 WHERE PLANID = ? AND NR >= ?;");
                        cmd.bind(1, m_id);
                        cmd.bind(2, (int)nr);
                        cmd.executenonquery();
                }
                {
                        sqlite3x::sqlite3_command cmd(m_fp->get_db(), "UPDATE waypoints SET NR = NR - 99999 WHERE PLANID = ? AND NR >= ?;");
                        cmd.bind(1, m_id);
                        cmd.bind(2, (int)nr);
                        cmd.executenonquery();
                }
                {
                        sqlite3x::sqlite3_command cmd(m_fp->get_db(), "INSERT INTO waypoints (PLANID,NR) VALUES (?,?);");
                        cmd.bind(1, m_id);
                        cmd.bind(2, (int)nr);
                        cmd.executenonquery();
                }
                tran.commit();
        }
}

void FPlanRoute::delete_wpt(uint32_t nr)
{
        if (nr >= m_wpts.size())
                throw std::runtime_error("Waypoint index out of range");
        if (m_id && m_fp) {
                sqlite3x::sqlite3_transaction tran(m_fp->get_db(), true);
                {
                        sqlite3x::sqlite3_command cmd(m_fp->get_db(), "DELETE FROM waypoints WHERE PLANID = ? AND NR = ?;");
                        cmd.bind(1, m_id);
                        cmd.bind(2, (int)nr);
                        cmd.executenonquery();
                }
                {
                        sqlite3x::sqlite3_command cmd(m_fp->get_db(), "UPDATE waypoints SET NR = NR + 99999 WHERE PLANID = ? AND NR > ?;");
                        cmd.bind(1, m_id);
                        cmd.bind(2, (int)nr);
                        cmd.executenonquery();
                }
                {
                        sqlite3x::sqlite3_command cmd(m_fp->get_db(), "UPDATE waypoints SET NR = NR - 100000 WHERE PLANID = ? AND NR > ?;");
                        cmd.bind(1, m_id);
                        cmd.bind(2, (int)nr);
                        cmd.executenonquery();
                }
                tran.commit();
        }
        m_wpts.erase(m_wpts.begin() + nr);
        m_dirty = true;
}

void FPlanRoute::clear_wpt(void)
{
	if (!m_wpts.size())
		return;
	if (m_id && m_fp) {
		sqlite3x::sqlite3_transaction tran(m_fp->get_db(), true);
		{
			sqlite3x::sqlite3_command cmd(m_fp->get_db(), "DELETE FROM waypoints WHERE PLANID = ?;");
			cmd.bind(1, m_id);
			cmd.executenonquery();
		}
		tran.commit();
	}
	m_wpts.clear();
	m_dirty = true;
}

bool FPlanRoute::is_dirty(void) const
{
	if (m_dirty)
		return true;
	for (waypoints_t::const_iterator i(m_wpts.begin()), e(m_wpts.end()); i != e; i++)
		if ((*i).m_dirty)
			return true;
	return false;
}

void FPlanRoute::set_dirty(bool d)
{
	m_dirty = d;
	for (waypoints_t::iterator i(m_wpts.begin()), e(m_wpts.end()); i != e; i++)
		(*i).m_dirty = d;
}

void FPlanRoute::load_fp(id_t nid)
{
	m_id = nid;
	load_fp();
}

void FPlanRoute::load_fp(void)
{
	m_wpts.clear();
	if (!m_fp)
		throw std::runtime_error("load_fp: no database");
	{
		sqlite3x::sqlite3_command cmd(m_fp->get_db(), "SELECT TIMEOFFBLOCK,SAVETIMEOFFBLOCK,TIMEONBLOCK,SAVETIMEONBLOCK,"
					      "CURWPT,NOTE FROM fplan WHERE ID = ?;");
		cmd.bind(1, m_id);
		sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
		if (!cursor.step())
			throw std::runtime_error("load_fp: plan ID not found");
		m_time_offblock = cursor.getint64(0);
		m_savetime_offblock = cursor.getint64(1);
		m_time_onblock = cursor.getint64(2);
		m_savetime_onblock = cursor.getint64(3);
		m_curwpt = cursor.getint(4);
		m_note = cursor.getstring(5);
	}
	{
		sqlite3x::sqlite3_command cmd(m_fp->get_db(), "SELECT " + FPlanWaypoint::reader_fields + " FROM waypoints WHERE PLANID = ? ORDER BY NR ASC;");
		cmd.bind(1, m_id);
		sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
		while (cursor.step())
			m_wpts.push_back(FPlanWaypoint(cursor));
	}
	m_dirty = false;
}

void FPlanRoute::save_fp(void)
{
        if (!is_dirty())
                return;
	if (!m_fp)
		throw std::runtime_error("save_fp: no database");
        if (!m_id) {
                {
                        //sqlite3x::sqlite3_command cmd(m_fp->get_db(), "SELECT ID FROM fplan ORDER BY ID DESC LIMIT 1;");
                        sqlite3x::sqlite3_command cmd(m_fp->get_db(), "SELECT MAX(ID) FROM fplan;");
                        sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
                        if (cursor.step())
                                m_id = cursor.getint(0) + 1;
                        else
                                m_id = 1;
                }
                {
                        sqlite3x::sqlite3_command cmd(m_fp->get_db(), "INSERT INTO fplan (ID) VALUES(?);");
                        cmd.bind(1, m_id);
                        cmd.executenonquery();
                }
        }
        sqlite3x::sqlite3_transaction tran(m_fp->get_db(), true);
        {
                sqlite3x::sqlite3_command cmd(m_fp->get_db(), "DELETE FROM waypoints WHERE PLANID = ? AND (NR < 0 OR NR >= ?);");
                cmd.bind(1, m_id);
                cmd.bind(2, get_nrwpt());
                cmd.executenonquery();
        }
        {
                sqlite3x::sqlite3_command cmd(m_fp->get_db(), "UPDATE fplan SET TIMEOFFBLOCK = ?,"
                                              "SAVETIMEOFFBLOCK = ?, TIMEONBLOCK = ?, SAVETIMEONBLOCK = ?,"
                                              "CURWPT = ?, NOTE = ? WHERE ID = ?;");
                cmd.bind(1, (long long int)m_time_offblock);
                cmd.bind(2, (long long int)m_savetime_offblock);
                cmd.bind(3, (long long int)m_time_onblock);
                cmd.bind(4, (long long int)m_savetime_onblock);
                cmd.bind(5, (int)m_curwpt);
                cmd.bind(6, m_note);
                cmd.bind(7, m_id);
                cmd.executenonquery();
        }
        for (unsigned int nr = 0; nr < get_nrwpt(); nr++) {
                FPlanWaypoint& w(m_wpts[nr]);
                {
                        sqlite3x::sqlite3_command cmd(m_fp->get_db(), "INSERT OR IGNORE INTO waypoints (PLANID,NR) VALUES(?,?);");
                        cmd.bind(1, m_id);
                        cmd.bind(2, (int)nr);
                        cmd.executenonquery();
                }
                {
                        sqlite3x::sqlite3_command cmd(m_fp->get_db(), "UPDATE waypoints SET ICAO  = ?, NAME = ?, "
                                                      "LON = ?, LAT = ?, ALT = ?, FREQ = ?, TIME = ?, SAVETIME = ?, "
                                                      "FLAGS = ?, PATHNAME = ?, PATHCODE = ?, NOTE = ?, "
						      "WINDDIR = ?, WINDSPEED = ?, QFF = ?, ISAOFFS = ?, "
						      "TRUEALT = ?, TRUETRACK = ?, TRUEHEADING = ?, DECLINATION = ?, "
						      "DIST = ?, FUEL = ?, TAS = ?, RPM = ?, MP = ?, TYPE = ?, NAVAID = ?, "
						      "FLIGHTTIME = ?, MASS = ?, ENGINEPROFILE = ?, BHP = ?, TROPOPAUSE = ? "
						      "WHERE PLANID = ? AND NR = ?;");
                        cmd.bind(1, w.get_icao());
                        cmd.bind(2, w.get_name());
                        cmd.bind(3, w.get_lon());
                        cmd.bind(4, w.get_lat());
                        cmd.bind(5, w.get_altitude());
                        cmd.bind(6, (int)w.get_frequency());
                        cmd.bind(7, (long long int)w.get_time());
                        cmd.bind(8, (long long int)w.get_save_time());
                        cmd.bind(9, w.get_flags());
                        cmd.bind(10, w.get_pathname());
                        cmd.bind(11, w.get_pathcode());
                        cmd.bind(12, w.get_note());
			cmd.bind(13, w.get_winddir());
			cmd.bind(14, w.get_windspeed());
			cmd.bind(15, w.get_qff());
			cmd.bind(16, w.get_isaoffset());
			cmd.bind(17, w.get_truealt());
			cmd.bind(18, w.get_truetrack());
			cmd.bind(19, w.get_trueheading());
			cmd.bind(20, w.get_declination());
			cmd.bind(21, (long long int)w.get_dist());
			cmd.bind(22, (long long int)w.get_fuel());
			cmd.bind(23, w.get_tas());
			cmd.bind(24, w.get_rpm());
			cmd.bind(25, w.get_mp());
			cmd.bind(26, w.get_type());
			cmd.bind(27, w.get_navaid());
			cmd.bind(28, (long long int)w.get_flighttime());
  			cmd.bind(29, (long long int)w.get_mass());
			cmd.bind(30, w.get_engineprofile());
			cmd.bind(31, (long long int)w.get_bhp());
			cmd.bind(32, w.get_tropopause());
			cmd.bind(33, m_id);
                        cmd.bind(34, (int)nr);
                        cmd.executenonquery();
                }
        }
        tran.commit();
        set_dirty(false);
}

void FPlanRoute::duplicate_fp(void)
{
        m_id = 0;
        m_dirty = true;
}

void FPlanRoute::new_fp(void)
{
        duplicate_fp();
        m_wpts.clear();
        m_note.clear();
        m_time_offblock = 0;
        m_savetime_offblock = 0;
        m_time_onblock = 0;
        m_savetime_onblock = 0;
        m_curwpt = 0;
        set_dirty(false);
}

void FPlanRoute::delete_fp(void)
{
        if (m_id) {
		if (!m_fp)
			throw std::runtime_error("delete_fp: no database");
                sqlite3x::sqlite3_transaction tran(m_fp->get_db(), true);
                {
                        sqlite3x::sqlite3_command cmd(m_fp->get_db(), "DELETE FROM waypoints WHERE PLANID = ?;");
                        cmd.bind(1, m_id);
                        cmd.executenonquery();
                }
                {
                        sqlite3x::sqlite3_command cmd(m_fp->get_db(), "DELETE FROM fplan WHERE ID = ?;");
                        cmd.bind(1, m_id);
                        cmd.executenonquery();
                }
                tran.commit();
        }
        new_fp();
}

void FPlanRoute::reverse_fp(void)
{
        if (m_wpts.empty())
                return;
        reverse(m_wpts.begin(), m_wpts.end());
        set_dirty(true);
}

bool FPlanRoute::load_first_fp(void)
{
	if (!m_fp)
		throw std::runtime_error("load_first_fp: no database");
        {
                sqlite3x::sqlite3_command cmd(m_fp->get_db(), "SELECT ID FROM fplan ORDER BY ID ASC LIMIT 1;");
                sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
                if (!cursor.step())
                        return false;
		m_id = cursor.getint(0);
        }
        load_fp();
        return true;
}

bool FPlanRoute::load_next_fp(void)
{
	if (!m_fp)
		throw std::runtime_error("load_next_fp: no database");
        {
                sqlite3x::sqlite3_command cmd(m_fp->get_db(), "SELECT ID FROM fplan WHERE ID > ? ORDER BY ID ASC LIMIT 1;");
                cmd.bind(1, m_id);
                sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
                if (!cursor.step())
                        return false;
                m_id = cursor.getint(0);
        }
        load_fp();
        return true;
}

uint32_t FPlanRoute::distance_sum(void) const
{
	uint32_t r(0);
	for (unsigned int i(1), n(m_wpts.size()); i < n; ++i)
		r += m_wpts[i-1].get_dist();
	return r;
}

float FPlanRoute::total_distance_km(void) const
{
        float sum(0);
	for (unsigned int i(0), n(m_wpts.size()); i < n; ) {
		const Point& pt0(m_wpts[i].get_coord());
		if (pt0.is_invalid()) {
			++i;
			continue;
		}
		unsigned int j(i + 1);
		for (; j < n; ++j) {
			const Point& pt1(m_wpts[j].get_coord());
			if (pt1.is_invalid())
				continue;
			sum += pt0.spheric_distance_km(pt1);
			break;
		}
		i = j;
	}
        return sum;
}

double FPlanRoute::total_distance_km_dbl(void) const
{
        double sum(0);
	for (unsigned int i(0), n(m_wpts.size()); i < n; ) {
		const Point& pt0(m_wpts[i].get_coord());
		if (pt0.is_invalid()) {
			++i;
			continue;
		}
		unsigned int j(i + 1);
		for (; j < n; ++j) {
			const Point& pt1(m_wpts[j].get_coord());
			if (pt1.is_invalid())
				continue;
			sum += pt0.spheric_distance_km_dbl(pt1);
			break;
		}
		i = j;
	}
        return sum;
}

float FPlanRoute::gc_distance_km(void) const
{
	if (m_wpts.empty())
		return 0;
	return m_wpts.front().get_coord().spheric_distance_km(m_wpts.back().get_coord());
}

double FPlanRoute::gc_distance_km_dbl(void) const
{
	if (m_wpts.empty())
		return 0;
	return m_wpts.front().get_coord().spheric_distance_km_dbl(m_wpts.back().get_coord());
}

int32_t FPlanRoute::max_altitude(void) const
{
        int32_t a(std::numeric_limits<int32_t>::min());
        for (unsigned int i = m_wpts.size(); i > 0; )
                a = std::max(a, m_wpts[--i].get_altitude());
        return a;
}

int32_t FPlanRoute::max_altitude(uint16_t& flags) const
{
        int32_t a(std::numeric_limits<int32_t>::min());
	uint16_t f(0);
        for (unsigned int i = m_wpts.size(); i > 0; ) {
		const FPlanWaypoint& wpt(m_wpts[--i]);
		int16_t aw(wpt.get_altitude());
		if (aw < a)
			continue;
                a = aw;
		f = wpt.get_flags();
	}
	flags = f;
        return a;
}

int32_t FPlanRoute::initial_altitude(void) const
{
	uint16_t flags(0);
	return initial_altitude(flags);
}

int32_t FPlanRoute::initial_altitude(uint16_t& flags) const
{
	const waypoints_t::const_iterator rdep(m_wpts.begin()), re(m_wpts.end());
	waypoints_t::const_iterator rcur(rdep), rdest(re);
	if (rdest != rcur)
		--rdest;
	waypoints_t::const_iterator rnext(rcur);
	if (rnext != rdest) {
		for (;;) {
			++rnext;
			if (rnext == rdest)
				break;
			if ((!rnext->is_ifr() || rnext->get_pathcode() != FPlanWaypoint::pathcode_sid) &&
			    (rnext->is_ifr() || rnext->get_pathcode() != FPlanWaypoint::pathcode_vfrdeparture))
				break;
		}
	}
	int32_t alt(FPlanWaypoint::invalid_altitude);
	flags = 0;
	if (rnext != rdest && rnext->is_altitude_valid()) {
		alt = rnext->get_altitude();
		flags = rnext->get_flags() & (FPlanWaypoint::altflag_ifr | FPlanWaypoint::altflag_standard | FPlanWaypoint::altflag_altvfr);
	}
	return alt;
}

Rect FPlanRoute::get_bbox(unsigned int wptidx0, unsigned int wptidx1) const
{
	Rect bbox;
	if (wptidx0 >= wptidx1 || wptidx1 > m_wpts.size())
		return bbox;
	bool first(true);
        for (unsigned int i = wptidx0; i < wptidx1; ++i) {
                Point p(m_wpts[i].get_coord());
		if (p.is_invalid())
			continue;
		if (first) {
			bbox = Rect(p, p);
			first = false;
			continue;
		}
		bbox = bbox.add(p);
        }
	return bbox;
}

Rect FPlanRoute::get_bbox(void) const
{
	return get_bbox(0, m_wpts.size());
}

bool FPlanRoute::get_flags(uint16_t& flags_or, uint16_t& flags_and) const
{
	uint16_t f_or(0U), f_and(~0U);
	for (waypoints_t::const_iterator i(m_wpts.begin()), e(m_wpts.end()); i != e; i++) {
		f_or |= i->get_flags();
		f_and &= i->get_flags();
	}
	flags_or = f_or;
	flags_and = f_and;
	return !m_wpts.empty();
}

char FPlanRoute::get_flightrules(void) const
{
	unsigned int i(m_wpts.size());
	if (!i)
		return ' ';
	--i;
	if (!i)
		return m_wpts[0].is_ifr() ? 'I' : 'V';
	uint16_t vfr(0), ifr(0);
	while (i > 0) {
		uint16_t f(m_wpts[--i].get_flags());
		vfr |= ~f;
		ifr |= f;
		if (vfr & ifr & FPlanWaypoint::altflag_ifr)
			return m_wpts[0].is_ifr() ? 'Y' : 'Z';
	}
	return m_wpts[0].is_ifr() ? 'I' : 'V';
}

void FPlanRoute::save_time(uint32_t nr)
{
        if (!nr) {
                save_time_offblock();
                return;
        }
        if (nr > m_wpts.size()) {
                save_time_onblock();
                return;
        }
        m_wpts[nr-1].save_time();
}

void FPlanRoute::restore_time(uint32_t nr)
{
        if (!nr) {
                restore_time_offblock();
                return;
        }
        if (nr > m_wpts.size()) {
                restore_time_onblock();
                return;
        }
        m_wpts[nr-1].restore_time();
}

void FPlanRoute::set_time(uint32_t nr, uint32_t t)
{
        if (!nr) {
                set_time_offblock(t);
                return;
        }
        if (nr > m_wpts.size()) {
                set_time_onblock(t);
                return;
        }
        m_wpts[nr-1].set_time(t);
}

uint32_t FPlanRoute::get_time(uint32_t nr) const
{
        if (!nr)
                return get_time_offblock();
        if (nr > m_wpts.size())
                return get_time_onblock();
        return m_wpts[nr-1].get_time();
}

FPlanLeg FPlanRoute::get_leg(uint32_t nr)
{
        bool future = nr > get_curwpt();
        const FPlanWaypoint *wpt0(0), *wpt1(0);
        if (nr > 0 && nr <= get_nrwpt())
                wpt0 = &(*this)[nr - 1];
        if (nr < get_nrwpt())
                wpt1 = &(*this)[nr];
        if (!wpt0 && !wpt1) {
                if (nr > 0 && get_nrwpt()) {
                        wpt0 = &(*this)[get_nrwpt()-1];
                        return FPlanLeg("", _("On Block"), wpt0->get_coord(), wpt0->get_altitude(), wpt0->get_flags(), wpt0->get_time(), wpt0->get_frequency(),
                                        "", _("On Block"), wpt0->get_coord(), wpt0->get_altitude(), wpt0->get_flags(), wpt0->get_time(), wpt0->get_frequency(),
				        wpt0->get_winddir_deg(), wpt0->get_windspeed_kts(), wpt0->get_qff_hpa(), wpt0->get_isaoffset_kelvin(), FPlanLeg::legtype_invalid, future);
                }
                return FPlanLeg("", _("Off Block"), Point(), 0, 0, 0, 0,
                                "", _("On Block"), Point(), 0, 0, 0, 0,
                                0, 0, 0, 0, FPlanLeg::legtype_invalid, future);
        }
        if (!wpt0)
                return FPlanLeg("", _("Off Block"), wpt1->get_coord(), wpt1->get_altitude(), wpt1->get_flags(), get_time_offblock(), wpt1->get_frequency(),
                                wpt1->get_icao(), wpt1->get_name(), wpt1->get_coord(), wpt1->get_altitude(), wpt1->get_flags(), wpt1->get_time(), wpt1->get_frequency(),
				wpt1->get_winddir_deg(), wpt1->get_windspeed_kts(), wpt1->get_qff_hpa(), wpt1->get_isaoffset_kelvin(), FPlanLeg::legtype_offblock_takeoff, future);
        if (!wpt1)
                return FPlanLeg(wpt0->get_icao(), wpt0->get_name(), wpt0->get_coord(), wpt0->get_altitude(), wpt0->get_flags(), wpt0->get_time(), wpt0->get_frequency(),
                                "", _("On Block"), wpt0->get_coord(), wpt0->get_altitude(), wpt0->get_flags(), wpt0->get_time(), wpt0->get_frequency(),
                                wpt0->get_winddir_deg(), wpt0->get_windspeed_kts(), wpt0->get_qff_hpa(), wpt0->get_isaoffset_kelvin(), FPlanLeg::legtype_landing_onblock, future);
        return FPlanLeg(wpt0->get_icao(), wpt0->get_name(), wpt0->get_coord(), wpt0->get_altitude(), wpt0->get_flags(), wpt0->get_time(), wpt0->get_frequency(),
                        wpt1->get_icao(), wpt1->get_name(), wpt1->get_coord(), wpt1->get_altitude(), wpt1->get_flags(), get_time_onblock(), wpt1->get_frequency(),
			wpt0->get_winddir_deg(), wpt0->get_windspeed_kts(), wpt0->get_qff_hpa(), wpt0->get_isaoffset_kelvin(), FPlanLeg::legtype_normal, future);
}

FPlanLeg FPlanRoute::get_directleg(void)
{
        uint32_t nr(get_nrwpt());
        bool future = false;
        if (!nr)
                return FPlanLeg("", _("Off Block"), Point(), 0, 0, 0, 0,
                                "", _("On Block"), Point(), 0, 0, 0, 0,
                                0, 0, 0, 0, FPlanLeg::legtype_invalid, future);
        const FPlanWaypoint *wpt0(&(*this)[0]), *wpt1(&(*this)[nr - 1]);
        return FPlanLeg(wpt0->get_icao(), wpt0->get_name(), wpt0->get_coord(), wpt0->get_altitude(), wpt0->get_flags(), wpt0->get_time(), wpt0->get_frequency(),
                        wpt1->get_icao(), wpt1->get_name(), wpt1->get_coord(), wpt1->get_altitude(), wpt1->get_flags(), get_time_onblock(), wpt1->get_frequency(),
                        wpt0->get_winddir_deg(), wpt0->get_windspeed_kts(), wpt0->get_qff_hpa(), wpt0->get_isaoffset_kelvin(), FPlanLeg::legtype_normal, future);
}

bool FPlanRoute::has_pathcodes(void) const
{
        for (unsigned int i = 0; i < m_wpts.size(); ++i)
		if (m_wpts[i].get_pathcode() != FPlanWaypoint::pathcode_none)
			return true;
	return false;
}

bool FPlanRoute::enforce_pathcode_vfrdeparr(void)
{
	unsigned int nr(m_wpts.size());
	if (nr <= 2U)
		return false;
	bool work(false);
	unsigned int i(0);
	for (; i < nr - 1; ++i) {
		const FPlanWaypoint& wpt(m_wpts[i]);
		if (wpt.get_icao() != m_wpts[0].get_icao() ||
		    (wpt.get_flags() & FPlanWaypoint::altflag_ifr) ||
		    wpt.get_pathcode() == FPlanWaypoint::pathcode_vfrarrival ||
		    wpt.get_pathcode() == FPlanWaypoint::pathcode_vfrtransition)
			break;
	}
	if (i > 1) {
		work = true;
		while (i > 0) {
			--i;
			m_wpts[i].set_pathcode(FPlanWaypoint::pathcode_vfrdeparture);
		}
	}
	i = nr - 1;
	for (; i > 0; --i) {
		const FPlanWaypoint& wpt(m_wpts[i]);
		if (wpt.get_icao() != m_wpts[nr - 1].get_icao() ||
		    (wpt.get_flags() & FPlanWaypoint::altflag_ifr) ||
		    wpt.get_pathcode() == FPlanWaypoint::pathcode_vfrdeparture ||
		    wpt.get_pathcode() == FPlanWaypoint::pathcode_vfrtransition)
			break;
	}
	if (i < nr - 2) {
		work = true;
		while (i < nr - 1) {
			++i;
			m_wpts[i].set_pathcode(FPlanWaypoint::pathcode_vfrarrival);
		}
	}
	return work;
}

bool FPlanRoute::enforce_pathcode_vfrifr(void)
{
	unsigned int nr(m_wpts.size());
	bool work(false);
	for (unsigned int i = 0; i < nr; ++i) {
		FPlanWaypoint& wpt(m_wpts[i]);
		work = wpt.enforce_pathcode_vfrifr() || work;
	}
	if (nr >= 2) {
		const FPlanWaypoint& wpt1(m_wpts[nr - 1]);
		FPlanWaypoint& wpt0(m_wpts[nr - 2]);
		work = work || ((wpt0.get_flags() ^ wpt1.get_flags()) & FPlanWaypoint::altflag_ifr);
		wpt0.set_flags(wpt0.get_flags() ^ ((wpt0.get_flags() ^ wpt1.get_flags()) & FPlanWaypoint::altflag_ifr));
	}
	if (nr >= 1) {
		FPlanWaypoint& wpt(m_wpts[nr - 1]);
		work = work || wpt.get_pathcode() != FPlanWaypoint::pathcode_none ||
			!wpt.get_pathname().empty();
		wpt.set_pathcode(FPlanWaypoint::pathcode_none);
		wpt.set_pathname("");
	}
	return work;
}

void FPlanRoute::recompute_dist(void)
{
	for (waypoints_t::iterator i(m_wpts.begin()), e(m_wpts.end()); i != e;) {
		waypoints_t::iterator i0(i);
		do {
			FPlanWaypoint& wpt(*i);
			wpt.set_dist(0);
			wpt.set_truetrack(0);
			++i;
		} while (i != e && i->get_coord().is_invalid());
		if (i == e)
			break;
		double tt, dist;
		{
			const FPlanWaypoint& wpto(*i0);
			const FPlanWaypoint& wpt(*i);
			dist = wpto.get_coord().spheric_distance_nmi_dbl(wpt.get_coord());
			tt = wpto.get_coord().spheric_true_course_dbl(wpt.get_coord());
		}
		dist /= i - i0;
		for (; i0 != i; ++i0) {
			FPlanWaypoint& wpto(*i0);
			wpto.set_dist_nmi(dist);
			wpto.set_truetrack_deg(tt);
		}
	}
}

void FPlanRoute::recompute_decl(void)
{
	WMM wmm;
	time_t curtime(time(0));
	double decl(0);
	for (waypoints_t::iterator i(m_wpts.begin()), e(m_wpts.end()); i != e; ++i) {
		FPlanWaypoint& wpt(*i);
		if (!wpt.get_coord().is_invalid()) {
			wmm.compute(wpt.get_truealt() * (FPlanWaypoint::ft_to_m / 1000.0), wpt.get_coord(), curtime);
			decl = wmm.get_dec();
		}
		wpt.set_declination_deg(decl);
	}
}

void FPlanRoute::recompute_time(void)
{
	if (m_wpts.empty())
		return;
	const FPlanWaypoint& wptst((*this)[0]);
	for (waypoints_t::iterator i(m_wpts.begin()), e(m_wpts.end()); i != e; ++i) {
		FPlanWaypoint& wpt(*i);
		wpt.set_time_unix(wpt.get_flighttime() + wptst.get_time_unix());
	}
}

void FPlanRoute::set_winddir(uint16_t dir)
{
	for (waypoints_t::iterator i(m_wpts.begin()), e(m_wpts.end()); i != e; ++i)
		i->set_winddir(dir);
}

void FPlanRoute::set_windspeed(uint16_t speed)
{
	for (waypoints_t::iterator i(m_wpts.begin()), e(m_wpts.end()); i != e; ++i)
		i->set_windspeed(speed);
}

void FPlanRoute::set_qff(uint16_t press)
{
	for (waypoints_t::iterator i(m_wpts.begin()), e(m_wpts.end()); i != e; ++i)
		i->set_qff(press);
}

void FPlanRoute::set_isaoffset(int16_t temp)
{
	for (waypoints_t::iterator i(m_wpts.begin()), e(m_wpts.end()); i != e; ++i)
		i->set_isaoffset(temp);
}

void FPlanRoute::set_declination(uint16_t decl)
{
	for (waypoints_t::iterator i(m_wpts.begin()), e(m_wpts.end()); i != e; ++i)
		i->set_declination(decl);
}

void FPlanRoute::set_rpm(uint16_t rpm)
{
	for (waypoints_t::iterator i(m_wpts.begin()), e(m_wpts.end()); i != e; ++i)
		i->set_rpm(rpm);
}

void FPlanRoute::set_mp(uint16_t mp)
{
	for (waypoints_t::iterator i(m_wpts.begin()), e(m_wpts.end()); i != e; ++i)
		i->set_mp(mp);
}

void FPlanRoute::set_tas(uint16_t tas)
{
	for (waypoints_t::iterator i(m_wpts.begin()), e(m_wpts.end()); i != e; ++i)
		i->set_tas(tas);
}

void FPlanRoute::set_tomass(uint32_t x)
{
	waypoints_t::iterator i(m_wpts.begin()), e(m_wpts.end());
	if (i == e)
		return;
	int32_t dm(-i->get_mass());
	i->set_mass(x);
	dm += i->get_mass();
	for (++i; i != e; ++i)
		i->set_mass(i->get_mass() + dm);
}

FPlanWaypoint FPlanRoute::insert_prepare(uint32_t nr)
{
	FPlanWaypoint wpt;
	{
		time_t t(time(0));
		wpt.set_time_unix(t);
                wpt.set_save_time_unix(t);
	}
	wpt.set_flags(0);
	wpt.set_altitude(0);
        unsigned int nrwpt(get_nrwpt());
	if (!nrwpt)
		return wpt;
	if (nr >= 1 && nr <= nrwpt) {
		const FPlanWaypoint& wpt1((*this)[nr - 1]);
		wpt.set_flags(wpt1.get_flags() & (FPlanWaypoint::altflag_ifr | FPlanWaypoint::altflag_standard));
		wpt.set_altitude(wpt1.get_altitude());
	}
	return wpt;
}

unsigned int FPlanRoute::insert(const AirportsDb::Airport& el, uint32_t nr)
{
	FPlanWaypoint wpt(insert_prepare(nr));
	wpt.set(el);
	insert_wpt(nr, wpt);
	return 1;
}

unsigned int FPlanRoute::insert(unsigned int vfrrtenr, const AirportsDb::Airport& el, uint32_t nr)
{
	if (vfrrtenr >= el.get_nrvfrrte())
		return insert(el, nr);
	const AirportsDb::Airport::VFRRoute& vfrrte(el.get_vfrrte(vfrrtenr));
	FPlanWaypoint wpt(insert_prepare(nr));
	if (!vfrrte.size()) {
		wpt.set(el);
		wpt.set_pathcode(FPlanWaypoint::pathcode_vfrtransition);
		wpt.set_pathname(vfrrte.get_name());
		insert_wpt(nr, wpt);
		return 1;
	}
	nr = std::max(nr, static_cast<uint32_t>(get_nrwpt()));
	for (unsigned int nrr(0); nrr < vfrrte.size(); ++nrr) {
		const AirportsDb::Airport::VFRRoute::VFRRoutePoint& vfrpt(vfrrte[nrr]);
		if (vfrpt.is_at_airport()) {
			wpt.set(el);
		} else {
			wpt.set_icao(el.get_icao());
			wpt.set_name(vfrpt.get_name());
			wpt.set_coord(vfrpt.get_coord());
			wpt.set_altitude(vfrpt.get_altitude());
			wpt.set_frequency(0);
			Glib::ustring note(vfrpt.get_altcode_string());
			if (vfrpt.is_compulsory())
				note += " Compulsory";
			note += "\n" + vfrpt.get_pathcode_string();
			wpt.set_note(note);
		}
		switch (vfrpt.get_pathcode()) {
		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_arrival:
			wpt.set_pathcode(FPlanWaypoint::pathcode_vfrarrival);
			break;

		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_departure:
			wpt.set_pathcode(FPlanWaypoint::pathcode_vfrdeparture);
			break;

		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_vfrtransition:
			wpt.set_pathcode(FPlanWaypoint::pathcode_vfrtransition);
			break;

		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_holding:
		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_trafficpattern:
		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_other:
		case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_invalid:
		default:
			wpt.set_pathcode(FPlanWaypoint::pathcode_none);
			break;
		}
		wpt.set_pathname(vfrrte.get_name());
		insert_wpt(nr, wpt);
		++nr;
	}
	return vfrrte.size();
}

unsigned int FPlanRoute::insert(unsigned int vfrrtenr, unsigned int vfrrteptnr, const AirportsDb::Airport& el, uint32_t nr)
{
	if (vfrrtenr >= el.get_nrvfrrte())
		return insert(el, nr);
	const AirportsDb::Airport::VFRRoute& vfrrte(el.get_vfrrte(vfrrtenr));
	if (vfrrteptnr >= vfrrte.size())
		return insert(vfrrtenr, el, nr);
	FPlanWaypoint wpt(insert_prepare(nr));
	const AirportsDb::Airport::VFRRoute::VFRRoutePoint& vfrpt(vfrrte[vfrrteptnr]);
	if (vfrpt.is_at_airport()) {
		wpt.set(el);
	} else {
		wpt.set_icao(el.get_icao());
		wpt.set_name(vfrpt.get_name());
		wpt.set_coord(vfrpt.get_coord());
		wpt.set_altitude(vfrpt.get_altitude());
		wpt.set_frequency(0);
		Glib::ustring note(vfrpt.get_altcode_string());
		if (vfrpt.is_compulsory())
			note += " Compulsory";
		note += "\n" + vfrpt.get_pathcode_string();
		wpt.set_note(note);
	}
	switch (vfrpt.get_pathcode()) {
	case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_arrival:
		wpt.set_pathcode(FPlanWaypoint::pathcode_vfrarrival);
		break;

	case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_departure:
		wpt.set_pathcode(FPlanWaypoint::pathcode_vfrdeparture);
		break;

	case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_vfrtransition:
		wpt.set_pathcode(FPlanWaypoint::pathcode_vfrtransition);
		break;

	case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_holding:
	case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_trafficpattern:
	case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_other:
	case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_invalid:
	default:
		wpt.set_pathcode(FPlanWaypoint::pathcode_none);
		break;
	}
	wpt.set_pathname(vfrrte.get_name());
	insert_wpt(nr, wpt);
	return 1;
}

unsigned int FPlanRoute::insert(const NavaidsDb::Navaid& el, uint32_t nr)
{
	FPlanWaypoint wpt(insert_prepare(nr));
	wpt.set(el);
	insert_wpt(nr, wpt);
	return 1;
}

unsigned int FPlanRoute::insert(const WaypointsDb::Waypoint& el, uint32_t nr)
{
	FPlanWaypoint wpt(insert_prepare(nr));
	wpt.set(el);
	insert_wpt(nr, wpt);
	return 1;
}

unsigned int FPlanRoute::insert(const MapelementsDb::Mapelement& el, uint32_t nr)
{
	FPlanWaypoint wpt(insert_prepare(nr));
	wpt.set(el);
	insert_wpt(nr, wpt);
	return 1;
}

unsigned int FPlanRoute::insert(const FPlanWaypoint& el, uint32_t nr)
{
	FPlanWaypoint wpt(insert_prepare(nr));
	wpt.set(el);
	insert_wpt(nr, wpt);
	return 1;
}

const std::string& to_str(FPlanRoute::LevelChange::type_t t)
{
	switch (t) {
	case FPlanRoute::LevelChange::type_boc:
	{
		static const std::string r("BOC");
		return r;
	}

	case FPlanRoute::LevelChange::type_toc:
	{
		static const std::string r("TOC");
		return r;
	}

	case FPlanRoute::LevelChange::type_bod:
	{
		static const std::string r("BOD");
		return r;
	}

	case FPlanRoute::LevelChange::type_tod:
	{
		static const std::string r("TOD");
		return r;
	}

	case FPlanRoute::LevelChange::type_invalid:
	default:
	{
		static const std::string r("invalid");
		return r;
	}
	}
}

FPlanRoute::Profile FPlanRoute::recompute(const Aircraft& acft, float qnh, float tempoffs, const Aircraft::Cruise::EngineParams& ep, bool insert_tocbod)
{
	FPlanWaypoint wptcenter;
	wptcenter.unset_altitude();
	return recompute(wptcenter, acft, qnh, tempoffs, ep, insert_tocbod);
}

FPlanRoute::Profile FPlanRoute::recompute(FPlanWaypoint& wptcenter, const Aircraft& acft, float qnh, float tempoffs,
					  const Aircraft::Cruise::EngineParams& ep, bool insert_tocbod)
{
	m_climbprofile.clear();
	m_descentprofile.clear();
	if (!get_nrwpt())
		return Profile();
	// ensure that there are no reserved waypoint types
	for (unsigned int i(0), n(get_nrwpt()); i < n; ++i) {
		FPlanWaypoint& wpt((*this)[i]);
		if (wpt.get_type() < FPlanWaypoint::type_generated_start || wpt.get_type() > FPlanWaypoint::type_generated_end)
			continue;
		wpt.set_type(FPlanWaypoint::type_user);
	}
	// set up airdata
	AirData<float> ad;
	if (!std::isnan(qnh) && !std::isnan(tempoffs))
		ad.set_qnh_tempoffs(qnh, tempoffs);
	if (false)
		std::cerr << "recompute: qnh " << qnh << " tempoffs " << tempoffs
			  << " ep " << ep.get_rpm() << ' ' << ep.get_mp() << ' ' << ep.get_bhp() << std::endl;
        WMM wmm;
        time_t curtime;
	double mass((*this)[0].get_mass_kg());
	double massmin(Aircraft::convert(acft.get_wb().get_massunit(), Aircraft::unit_kg, acft.get_wb().get_min_mass()));
	if (std::isnan(massmin) || massmin <= 0)
		massmin = 0;
	if (std::isnan(mass) || mass < massmin)
		mass = massmin;
	{
		double mmax(acft.get_mtom_kg());
		if (!std::isnan(mmax) && mass > mmax)
			mass = mmax;
	}
	{
		FPlanWaypoint& wpt((*this)[0]);
		curtime = wpt.get_time_unix();
		// use current time if start time is before 2000-1-1 00:00 (before start of WMM validity)
		if (curtime < 946684800)
			curtime = time(0);
		if (std::isnan(qnh) || std::isnan(tempoffs))
			ad.set_qnh_tempoffs(std::isnan(qnh) ? wpt.get_qff_hpa() : qnh, std::isnan(tempoffs) ? wpt.get_isaoffset_kelvin() : tempoffs);
		wpt.frob_flags(0, FPlanWaypoint::altflag_climb | FPlanWaypoint::altflag_descent);
		wpt.set_tas(0);
		wpt.set_flighttime(0);
		wpt.set_fuel(0);
		wpt.set_rpm(0);
		wpt.set_mp(0);
		wpt.set_mass_kg(mass);
		if (wpt.is_standard())
			ad.set_pressure_altitude(wpt.get_altitude());
		else
			ad.set_true_altitude(wpt.get_altitude());
		wpt.set_truealt(ad.get_true_altitude());
		Aircraft::Cruise::EngineParams ep1(ep);
		Aircraft::ClimbDescent climb(acft.calculate_climb(ep1, Aircraft::convert(Aircraft::unit_kg, acft.get_wb().get_massunit(), mass), wpt.get_isaoffset_kelvin(), wpt.get_qff_hpa()));
		{
			double tm(climb.altitude_to_time(ad.get_pressure_altitude()));
			wpt.set_tas(climb.time_to_tas(tm));
		}
		wmm.compute(ad.get_true_altitude() * (FPlanWaypoint::ft_to_m / 1000.0), wpt.get_coord(), curtime);
		wpt.set_declination_deg(wmm.get_dec());
		{
			double da(ad.get_density_altitude());
			double t(climb.altitude_to_time(da));
			wpt.set_tas_kts(climb.time_to_tas(t));
		}
	}
	{
		FPlanWaypoint& wpt((*this)[get_nrwpt() - 1]);
		wpt.set_tas(0);
		wpt.set_truetrack(0);
		wpt.set_trueheading(0);
		wpt.set_dist(0);
	}
	Profile p;
	if (get_nrwpt() == 2 && !(*this)[0].get_coord().is_invalid() && !(*this)[1].get_coord().is_invalid()) {
		FPlanWaypoint wptc((*this)[0]);
		wptc.set_icao("");
		wptc.set_name("*CENTER*");
		wptc.set_type(FPlanWaypoint::type_center);
		const FPlanWaypoint& wptp((*this)[0]);
		const FPlanWaypoint& wptn((*this)[1]);
		wptc.set_coord(wptp.get_coord().halfway(wptn.get_coord()));
		{
			Wind<float> wnd;
			wnd.set_wind(wptp.get_winddir_rad(), wptp.get_windspeed_kts(),
				     wptn.get_winddir_rad(), wptn.get_windspeed_kts());
			wptc.set_winddir_rad(wnd.get_winddir());
			wptc.set_windspeed_kts(wnd.get_windspeed());
		}
		wptc.set_qff_hpa(0.5 * (wptp.get_qff_hpa() + wptn.get_qff_hpa()));
		wptc.set_isaoffset_kelvin(0.5 * (wptp.get_isaoffset_kelvin() + wptn.get_isaoffset_kelvin()));
		wptc.set_declination_rad(wptp.get_declination_rad() + 0.5 * (wptn.get_declination_rad() - wptp.get_declination_rad()));
		int32_t alt(wptcenter.get_altitude());
		uint16_t flags(wptcenter.get_flags());
		if (alt != FPlanWaypoint::invalid_altitude && alt != 0) {
			wptc.frob_flags(flags & FPlanWaypoint::altflag_standard, FPlanWaypoint::altflag_standard);
			wptc.set_altitude(alt);
		} else if ((alt = IcaoFlightPlan::get_route_pogo_alt(wptp.get_icao(), wptn.get_icao())) > 0) {
			wptc.frob_flags(FPlanWaypoint::altflag_standard, FPlanWaypoint::altflag_standard);
			wptc.set_altitude(alt);
		} else {
			double dist(wptp.get_coord().spheric_distance_nmi_dbl(wptn.get_coord()));
			alt = wptn.wpt_round_altitude(std::max(wptp.get_altitude(), wptn.get_altitude()) + 1000, FPlanWaypoint::roundalt_ceil | FPlanWaypoint::roundalt_rvsm);
			double bestm(std::numeric_limits<double>::max());
			double mass1(Aircraft::convert(Aircraft::unit_kg, acft.get_wb().get_massunit(), mass));
			double qnh1(qnh), tempoffs1(tempoffs), mass2(mass1), qnh2(qnh), tempoffs2(tempoffs), mass3(mass1), qnh3(qnh), tempoffs3(tempoffs);
			Aircraft::Cruise::EngineParams ep1(ep), ep2(ep);
			if (std::isnan(qnh1))
				qnh1 = wptp.get_qff() ? wptp.get_qff_hpa() : IcaoAtmosphere<float>::std_sealevel_pressure;
			if (std::isnan(tempoffs1))
				tempoffs1 = wptp.get_isaoffset_kelvin();
			if (std::isnan(qnh2))
				qnh2 = wptn.get_qff() ? wptn.get_qff_hpa() : IcaoAtmosphere<float>::std_sealevel_pressure;
			if (std::isnan(tempoffs2))
				tempoffs2 = wptn.get_isaoffset_kelvin();
			if (std::isnan(qnh3))
				qnh3 = wptc.get_qff() ? wptc.get_qff_hpa() : IcaoAtmosphere<float>::std_sealevel_pressure;
			if (std::isnan(tempoffs3))
				tempoffs3 = wptc.get_isaoffset_kelvin();
			Aircraft::ClimbDescent climb(acft.calculate_climb(ep1, mass1, tempoffs1, qnh1));
			Aircraft::ClimbDescent desc(acft.calculate_descent(ep2, mass2, tempoffs2, qnh2));
			for (int32_t alt1(alt); alt1 <= climb.get_ceiling(); alt1 += 1000) {
				double tm(0), dst(0), fuel(0);
				{
					double t1(climb.altitude_to_time(wptp.get_altitude()));
					double t2(climb.altitude_to_time(alt1));
					tm = t2 - t1;
					dst = climb.time_to_distance(t2) - climb.time_to_distance(t1);
					fuel = climb.time_to_fuel(t2) - climb.time_to_fuel(t1);
				}
				{
					double t1(desc.altitude_to_time(wptn.get_altitude()));
					double t2(desc.altitude_to_time(alt1));
					tm += t2 - t1;
					dst += desc.time_to_distance(t2) - desc.time_to_distance(t1);
					fuel += desc.time_to_fuel(t2) - desc.time_to_fuel(t1);
				}
				if (dst > dist)
					break;
				Aircraft::Cruise::EngineParams ep3(ep);
				double tas3(0), ff3(0), pa3(alt1), mass33(mass3), tempoffs33(tempoffs3), qnh33(qnh3);
				acft.calculate_cruise(tas3, ff3, pa3, mass33, tempoffs33, qnh33, ep3);
				if (std::isnan(tas3) || tas3 < 1 || std::isnan(ff3))
					continue;
				double t3((dist - dst) / tas3);
				tm += 3600.0 * t3;
				fuel += ff3 * t3;
				double m((acft.get_propulsion() == Aircraft::propulsion_turboprop ||
					  acft.get_propulsion() == Aircraft::propulsion_turbojet) ? fuel : tm);
				if (m > bestm)
					continue;
				alt = alt1;
				bestm = m;
			}
			wptc.set_altitude(alt);
			wptc.frob_flags(FPlanWaypoint::altflag_standard, FPlanWaypoint::altflag_standard);
		}
		insert_wpt(1, wptc);
	}
	// move backwards, record maximum altitude in set_truealt that results in a reasonable descent to destination
	{
		AirData<float> ad;
		if (!std::isnan(qnh) && !std::isnan(tempoffs))
			ad.set_qnh_tempoffs(qnh, tempoffs);
		{
			FPlanWaypoint& wpt((*this)[get_nrwpt() - 1]);
			if (std::isnan(qnh) || std::isnan(tempoffs))
				ad.set_qnh_tempoffs(std::isnan(qnh) ? (wpt.get_qff() ? wpt.get_qff_hpa() : IcaoAtmosphere<float>::std_sealevel_pressure) : qnh,
						    std::isnan(tempoffs) ? wpt.get_isaoffset_kelvin() : tempoffs);
			ad.set_true_altitude(wpt.get_altitude());
			wpt.set_truealt(ad.get_true_altitude());
			// add a "circuit altitude" as buffer
			ad.set_true_altitude(wpt.get_altitude() + 1000.0);
		}
		for (unsigned int n(get_nrwpt()), i(n - 1); i;) {
			unsigned int i0(i);
			for (;;) {
				--i;
				if (!i)
					break;
				const FPlanWaypoint& wpt((*this)[i]);
				if (!wpt.get_coord().is_invalid())
					break;
			}
			if (!i)
				break;
			unsigned int in(i);
			for (;;) {
				--in;
				if (!in)
					break;
				const FPlanWaypoint& wpt((*this)[in]);
				if (!wpt.get_coord().is_invalid())
					break;
			}
			FPlanWaypoint *wpt(&(*this)[i]);
			FPlanWaypoint *wpto(&(*this)[i0]);
			FPlanWaypoint *wptn(&(*this)[in]);
			Wind<double> wind;
			wind.set_wind(wpto->get_winddir_deg(), wpto->get_windspeed_kts(), wpt->get_winddir_deg(), wpt->get_windspeed_kts());
			double prevpa(ad.get_pressure_altitude());
			double prevta(ad.get_true_altitude());
			if (std::isnan(qnh) || std::isnan(tempoffs))
				ad.set_qnh_tempoffs(std::isnan(qnh) ? (wpt->get_qff() ? wpt->get_qff_hpa() : IcaoAtmosphere<float>::std_sealevel_pressure) : qnh,
						    std::isnan(tempoffs) ? wpt->get_isaoffset_kelvin() : tempoffs);
			double legdist(wpt->get_coord().spheric_distance_nmi_dbl(wpto->get_coord()));
			double dist(legdist);
			double tt(wpt->get_coord().spheric_true_course_dbl(wpto->get_coord()));
			Aircraft::Cruise::EngineParams ep1(ep);
			double qnh1(qnh), isaoffs1(tempoffs), mass1(massmin);
			if (std::isnan(qnh1))
				qnh1 = wpto->get_qff_hpa();
			if (std::isnan(isaoffs1))
				isaoffs1 = wpto->get_isaoffset_kelvin();
			mass1 = Aircraft::convert(Aircraft::unit_kg, acft.get_wb().get_massunit(), mass1);
			Aircraft::ClimbDescent descent(acft.calculate_descent(ep1, mass1, isaoffs1, qnh1));
			double prevt(descent.altitude_to_time(prevpa));
			double prevd(descent.time_to_distance(prevt));
			// check for wind
			double speedfactor(1);
			double tas(descent.time_to_tas(prevt));
			if (tas < 1) {
				std::ostringstream oss;
				oss << "FPlanRoute::recompute: invalid aircraft model (descent max alt i "
				    << i << " i0 " << i0 << " pa " << prevpa << " t " << prevt << " d " << prevd
				    << " fuel " << descent.time_to_fuel(prevt) << " alt " << descent.time_to_altitude(prevt)
				    << " sf " << speedfactor << " tas " << tas
				    << " mass " << mass1 << " isaoffs " << isaoffs1 << " qnh " << qnh1;
				descent.get_ratepoly().print(oss << " ratepoly = [") << "]";
				descent.get_fuelflowpoly().print(oss << " fuelflowpoly = [") << "]";
				descent.get_caspoly().print(oss << " caspoly = [") << "]";
				descent.get_climbaltpoly().print(oss << " climbaltpoly = [") << "]";
				descent.get_climbdistpoly().print(oss << " climbdistpoly = [") << "]";
				descent.get_climbfuelpoly().print(oss << " climbfuelpoly = [") << "]";
				oss << ')';
				throw std::runtime_error(oss.str());
			}
			if (wind.get_windspeed() > 0) {
				wind.set_crs_tas(tt, tas);
				if (!std::isnan(wind.get_gs()) && !std::isnan(wind.get_hdg()) && wind.get_gs() > 0) {
					double th(wind.get_hdg());
					speedfactor = wind.get_gs() / tas;
					if (false)
						std::cerr << "FPlanRoute::recompute: maxalt: windcorr: TT " << tt << " TH " << th
							  << " TAS " << tas << " GS " << wind.get_gs()
							  << " Wind " << wind.get_winddir() << '/' << wind.get_windspeed() << std::endl;
				}
			}
			dist /= speedfactor;
			double nextd(prevd + dist);
			double nextt(prevt), nextpa(prevpa), nextta(prevta);
			if (prevpa < descent.get_ceiling()) {
				nextt = std::min(std::max(descent.distance_to_time(nextd), prevt), descent.get_climbtime());
				nextpa = std::max(descent.time_to_altitude(nextt), prevpa);
				ad.set_pressure_altitude(nextpa);
				nextta = ad.get_true_altitude();
			}
			// check terrain
			int32_t terrain(0);
			if (wpt->is_terrain_valid())
				terrain = wpt->get_terrain();
			if (wptn->is_terrain_valid())
				terrain = std::max(terrain, wptn->get_terrain());
			if (terrain >= 5000)
				terrain += 1000;
			terrain += 1000;
			if (false)
				std::cerr << "FPlanRoute::recompute: maxalt i " << i << " i0 " << i0 << " pa " << prevpa << '/' << nextpa
					  << " ta " << prevta << '/' << nextta << " t " << prevt << '/' << nextt
					  << " d " << prevd << '/' << nextd << " terrain " << terrain
					  << " alt " << descent.time_to_altitude(prevt) << '/' << descent.time_to_altitude(nextt)
					  << " sf " << speedfactor << " tas " << tas << " dist " << dist << std::endl;
			nextta = std::max(nextta, (double)terrain);
			wpt->set_truealt(nextta);
			for (;;) {
				--i0;
				if (i == i0)
					break;
				FPlanWaypoint *wpt(&(*this)[i0]);
				wpt->set_truealt(nextta);
			}
		}
	}
	double cumdist(0);
	typedef enum {
		mode_cruise,
		mode_climb,
		mode_descent
	} mode_t;
	mode_t mode(mode_climb);
	for (unsigned int i(0), n(get_nrwpt()); i < n;) {
		unsigned int i0(i);
		for (;;) {
			++i;
			if (i >= n)
				break;
			const FPlanWaypoint& wpt((*this)[i]);
			if (!wpt.get_coord().is_invalid())
				break;
		}
		if (i >= n)
			break;
		FPlanWaypoint *wpt(&(*this)[i]);
		FPlanWaypoint *wpto(&(*this)[i0]);
		if (false)
			std::cerr << "FPlanRoute::recompute: [" << i << '/' << n << "]: alt " << wpt->get_altitude()
				  << " descent " << wpt->get_truealt() << std::endl;
		double prevpa(ad.get_pressure_altitude());
		double prevta(ad.get_true_altitude());
		if (std::isnan(qnh) || std::isnan(tempoffs))
			ad.set_qnh_tempoffs(std::isnan(qnh) ? (wpt->get_qff() ? wpt->get_qff_hpa() : IcaoAtmosphere<float>::std_sealevel_pressure) : qnh,
					    std::isnan(tempoffs) ? wpt->get_isaoffset_kelvin() : tempoffs);
		wpt->frob_flags(0, FPlanWaypoint::altflag_climb | FPlanWaypoint::altflag_descent);
		wpt->set_engineprofile("");
		wpt->set_bhp_raw(0);
		wpt->set_tas(0);
		wpt->set_rpm(0);
		wpt->set_mp(0);
		double legdist(wpto->get_coord().spheric_distance_nmi_dbl(wpt->get_coord()));
		double dist(legdist);
		double tt(wpto->get_coord().spheric_true_course_dbl(wpt->get_coord()));
		wpto->set_dist_nmi(dist);
		wpto->set_truetrack_deg(tt);
		if (wpt->get_type() != FPlanWaypoint::type_center &&
		    ((wpto->get_pathcode() == FPlanWaypoint::pathcode_sid && wpt->get_pathcode() != FPlanWaypoint::pathcode_sid) ||
		     (wpto->get_pathcode() != FPlanWaypoint::pathcode_star && wpt->get_pathcode() == FPlanWaypoint::pathcode_star))) {
			p.set_distat(i, cumdist + dist);
		}
		if (wpt->is_standard())
			ad.set_pressure_altitude(wpt->get_altitude());
		else
			ad.set_true_altitude(wpt->get_altitude());
		bool latedescent(ad.get_true_altitude() > wpt->get_truealt());
		if (latedescent) {
			if (false)
				std::cerr << "FPlanRoute::recompute: wpt " << i << ' ' << wpt->get_icao_name()
					  << " alt " << ad.get_true_altitude() << " descent alt " << wpt->get_truealt() << std::endl;
			ad.set_true_altitude(wpt->get_truealt());
		}
		double nextpa(ad.get_pressure_altitude());
		double ltime(wpto->get_flighttime()), fuel(wpto->get_fuel_usg());
		double th(tt);
		Wind<double> wind;
		wind.set_wind(wpto->get_winddir_deg(), wpto->get_windspeed_kts(), wpt->get_winddir_deg(), wpt->get_windspeed_kts());
		unsigned int staynr(0), staytime(0);
		bool stay(wpto->is_stay(staynr, staytime));
		mode_t newmode(mode_cruise);
		if (!stay) {
			if (nextpa - prevpa > 200)
				newmode = mode_climb;
			if (prevpa - nextpa > 200)
				newmode = mode_descent;
		}
		if (false)
			std::cerr << "recompute: i " << i << " i0 " << i0 << " pa " << prevpa << '/' << nextpa << std::endl;
		// check for climb segment
		if (newmode == mode_climb) {
			Aircraft::Cruise::EngineParams ep1(ep);
			double qnh1(qnh), isaoffs1(tempoffs), mass1(mass);
			if (std::isnan(qnh1))
				qnh1 = wpto->get_qff_hpa();
			if (std::isnan(isaoffs1))
				isaoffs1 = wpto->get_isaoffset_kelvin();
			{
				double m(acft.convert_fuel(acft.get_fuelunit(), Aircraft::unit_kg, fuel));
				if (!std::isnan(m))
					mass1 -= m;
			}
			if (mass1 < massmin)
				mass1 = massmin;
			mass1 = Aircraft::convert(Aircraft::unit_kg, acft.get_wb().get_massunit(), mass1);
			Aircraft::ClimbDescent climb(acft.calculate_climb(ep1, mass1, isaoffs1, qnh1));
			if (false)
				climb.print(std::cerr << "FPlanRoute::recompute: climb i " << i << std::endl, "  ") << std::endl;
			wpto->frob_flags(FPlanWaypoint::altflag_climb, FPlanWaypoint::altflag_climb);
			wpto->set_engineprofile(ep1.get_climbname());
			double prevt(climb.altitude_to_time(prevpa));
			double nextt(climb.altitude_to_time(nextpa));
			double prevd(climb.time_to_distance(prevt));
			double nextd(climb.time_to_distance(nextt));
			double climbd(nextd - prevd);
			// check for wind
			double speedfactor(1);
			double tas(climb.time_to_tas(prevt));
			if (tas < 1 || climbd < 0) {
				std::ostringstream oss;
				oss << "FPlanRoute::recompute: invalid aircraft model (climb i "
				    << i << " i0 " << i0 << " pa " << prevpa << '/' << nextpa
				    << " t " << prevt << '/' << nextt << " d " << prevd << '/' << nextd
				    << " fuel " << climb.time_to_fuel(prevt) << '/' << climb.time_to_fuel(nextt)
				    << " alt " << climb.time_to_altitude(prevt) << '/' << climb.time_to_altitude(nextt)
				    << " sf " << speedfactor << " tas " << tas << " climbd " << climbd
				    << " mass " << mass1 << " isaoffs " << isaoffs1 << " qnh " << qnh1;
				climb.get_ratepoly().print(oss << " ratepoly = [") << "]";
				climb.get_fuelflowpoly().print(oss << " fuelflowpoly = [") << "]";
				climb.get_caspoly().print(oss << " caspoly = [") << "]";
				climb.get_climbaltpoly().print(oss << " climbaltpoly = [") << "]";
				climb.get_climbdistpoly().print(oss << " climbdistpoly = [") << "]";
				climb.get_climbfuelpoly().print(oss << " climbfuelpoly = [") << "]";
				oss << ')';
				throw std::runtime_error(oss.str());
			}
			if (wind.get_windspeed() > 0) {
				wind.set_crs_tas(tt, tas);
				if (std::isnan(wind.get_gs()) || std::isnan(wind.get_hdg()) || wind.get_gs() <= 0) {
					climbd = 0;
				} else {
					th = wind.get_hdg();
					speedfactor = wind.get_gs() / tas;
					climbd *= speedfactor;
					if (false)
						std::cerr << "FPlanRoute::recompute: climb: windcorr: TT " << tt << " TH " << th
							  << " TAS " << tas << " GS " << wind.get_gs()
							  << " Wind " << wind.get_winddir() << '/' << wind.get_windspeed() << std::endl;
				}
			}
			if (false)
				std::cerr << "recompute: climb i " << i << " i0 " << i0 << " pa " << prevpa << '/' << nextpa
					  << " t " << prevt << '/' << nextt << " d " << prevd << '/' << nextd
					  << " fuel " << climb.time_to_fuel(prevt) << '/' << climb.time_to_fuel(nextt)
					  << " alt " << climb.time_to_altitude(prevt) << '/' << climb.time_to_altitude(nextt)
					  << " sf " << speedfactor << " tas " << tas << " climbd " << climbd << std::endl;
			if (newmode != mode) {
				if (mode == mode_climb)
					p.push_back(LevelChange(LevelChange::type_toc, cumdist, wpto->get_altitude(), wpto->get_flighttime()));
				if (mode == mode_descent)
					p.push_back(LevelChange(LevelChange::type_bod, cumdist, wpto->get_altitude(), wpto->get_flighttime()));
				if (newmode == mode_climb)
					p.push_back(LevelChange(LevelChange::type_boc, cumdist, wpto->get_altitude(), wpto->get_flighttime()));
				if (newmode == mode_descent)
					p.push_back(LevelChange(LevelChange::type_tod, cumdist, wpto->get_altitude(), wpto->get_flighttime()));
				mode = newmode;
			}
			wpto->set_tas_kts(tas);
			if (climbd <= dist + 1.0) {
				climbd = std::min(dist, climbd);
				cumdist += climbd;
				dist -= climbd;
				ltime += nextt - prevt;
				p.push_back(LevelChange(LevelChange::type_toc, cumdist, wpt->get_altitude(), ltime));
				newmode = mode = mode_cruise;
				if (dist <= 1.0) {
					dist = 0;
				} else if (!stay) {
					// check POGO plans
					FPlanWaypoint wptx(*wpto);
					wptx.set_icao("");
					wptx.set_name("*TOC*");
					wptx.set_type(FPlanWaypoint::type_toc);
					wptx.set_coord(wpto->get_coord().get_gcnav(wpt->get_coord(), climbd / legdist).first);
					wptx.set_altitude(wpt->get_altitude());
					wptx.frob_flags(((wptx.get_flags() ^ wpt->get_flags()) & FPlanWaypoint::altflag_standard),
							FPlanWaypoint::altflag_climb | FPlanWaypoint::altflag_descent);
					wptx.set_frequency(0);
					wptx.set_tas(0);
					wptx.set_rpm(0);
					wptx.set_mp(0);
					wpto->set_dist_nmi(climbd);
					wptx.set_dist_nmi(legdist - climbd);
					wptx.set_winddir(wpt->get_winddir());
					wptx.set_windspeed(wpt->get_windspeed());
					wptx.set_isaoffset(wpt->get_isaoffset());
					insert_wpt(i, wptx);
					++n;
					wpt = &(*this)[i];
					wpto = &(*this)[i0];
					dist = 0;
				} else {
					wind.set_wind(wpt->get_winddir_deg(), wpt->get_windspeed_kts());
				}
			} else {
				// climb not finished at waypoint
				nextt = climb.distance_to_time(prevd + dist / speedfactor);
				cumdist += dist;
				if (false)
					std::cerr << "climb calc: prevd " << prevd << " legdist " << legdist << " dist " << dist
						  << " climbd " << climbd << " prevt " << prevt << " nextt " << nextt
						  << " prevpa " << prevpa << " nextpa " << nextpa << std::endl;
				dist = 0;
				ltime += nextt - prevt;
				ad.set_pressure_altitude(climb.time_to_altitude(nextt));
			}
			fuel += climb.time_to_fuel(nextt) -
				climb.time_to_fuel(prevt);
		}
		// check for descent segment
		if (newmode == mode_descent) {
			Aircraft::Cruise::EngineParams ep1(ep);
			double qnh1(qnh), isaoffs1(tempoffs), mass1(mass);
			if (std::isnan(qnh1))
				qnh1 = wpto->get_qff_hpa();
			if (std::isnan(isaoffs1))
				isaoffs1 = wpto->get_isaoffset_kelvin();
			{
				double m(acft.convert_fuel(acft.get_fuelunit(), Aircraft::unit_kg, fuel));
				if (!std::isnan(m))
					mass1 -= m;
			}
			if (mass1 < massmin)
				mass1 = massmin;
			mass1 = Aircraft::convert(Aircraft::unit_kg, acft.get_wb().get_massunit(), mass1);
			Aircraft::ClimbDescent descent(acft.calculate_descent(ep1, mass1, isaoffs1, qnh1));
			double prevt(descent.altitude_to_time(prevpa));
			double nextt(descent.altitude_to_time(nextpa));
			double prevd(descent.time_to_distance(prevt));
			double nextd(descent.time_to_distance(nextt));
			double descentd(prevd - nextd);
			// check for wind
			double speedfactor(1);
			double tas(descent.time_to_tas(prevt));
			if (tas < 1) {
				std::ostringstream oss;
				oss << "FPlanRoute::recompute: invalid aircraft model (descent i "
				    << i << " i0 " << i0 << " pa " << prevpa << '/' << nextpa
				    << " t " << prevt << '/' << nextt << " d " << prevd << '/' << nextd
				    << " fuel " << descent.time_to_fuel(prevt) << '/' << descent.time_to_fuel(nextt)
				    << " alt " << descent.time_to_altitude(prevt) << '/' << descent.time_to_altitude(nextt)
				    << " sf " << speedfactor << " tas " << tas << " descentd " << descentd
				    << " mass " << mass1 << " isaoffs " << isaoffs1 << " qnh " << qnh1;
				descent.get_ratepoly().print(oss << " ratepoly = [") << "]";
				descent.get_fuelflowpoly().print(oss << " fuelflowpoly = [") << "]";
				descent.get_caspoly().print(oss << " caspoly = [") << "]";
				descent.get_climbaltpoly().print(oss << " climbaltpoly = [") << "]";
				descent.get_climbdistpoly().print(oss << " climbdistpoly = [") << "]";
				descent.get_climbfuelpoly().print(oss << " climbfuelpoly = [") << "]";
				oss << ')';
				throw std::runtime_error(oss.str());
			}
			if (wind.get_windspeed() > 0) {
				wind.set_crs_tas(tt, tas);
				if (std::isnan(wind.get_gs()) || std::isnan(wind.get_hdg()) || wind.get_gs() <= 0) {
					descentd = 0;
				} else {
					th = wind.get_hdg();
					speedfactor = wind.get_gs() / tas;
					descentd *= speedfactor;
					if (false)
						std::cerr << "FPlanRoute::recompute: descent: windcorr: TT " << tt << " TH " << th
							  << " TAS " << tas << " GS " << wind.get_gs()
							  << " Wind " << wind.get_winddir() << '/' << wind.get_windspeed() << std::endl;
				}
			}
			if (false)
				std::cerr << "recompute: descent i " << i << " i0 " << i0 << " pa " << prevpa << '/' << nextpa
					  << " t " << prevt << '/' << nextt << " d " << prevd << '/' << nextd
					  << " fuel " << descent.time_to_fuel(prevt) << '/' << descent.time_to_fuel(nextt)
					  << " alt " << descent.time_to_altitude(prevt) << '/' << descent.time_to_altitude(nextt)
					  << " sf " << speedfactor << " tas " << tas << " descentd " << descentd << std::endl;
			if ((latedescent || i + 1 >= n) && descentd <= dist - 1.0 && wpto->get_type() != FPlanWaypoint::type_tod) {
				// check POGO
				newmode = mode_cruise;
				nextpa = prevpa;
				dist -= descentd;
				FPlanWaypoint wptx(*wpto);
				wptx.set_icao("");
				wptx.set_name("*TOD*");
				wptx.set_type(FPlanWaypoint::type_tod);
				wptx.set_coord(wpto->get_coord().get_gcnav(wpt->get_coord(), dist / legdist).first);
				wptx.set_altitude(wpto->get_altitude());
				wptx.frob_flags(((wptx.get_flags() ^ wpto->get_flags()) & FPlanWaypoint::altflag_standard),
						FPlanWaypoint::altflag_climb | FPlanWaypoint::altflag_descent);
				wptx.set_frequency(0);
				wptx.set_tas(0);
				wptx.set_rpm(0);
				wptx.set_mp(0);
				wpto->set_dist_nmi(dist);
				wptx.set_dist_nmi(legdist - dist);
				insert_wpt(i, wptx);
				++n;
				wpt = &(*this)[i];
				wpto = &(*this)[i0];
				if (wpt->is_standard())
					ad.set_pressure_altitude(wpt->get_altitude());
				else
					ad.set_true_altitude(wpt->get_altitude());
				if (ad.get_true_altitude() > wpt->get_truealt()) {
					if (false)
						std::cerr << "FPlanRoute::recompute: wpt " << i << ' ' << wpt->get_icao_name()
							  << " alt " << ad.get_true_altitude() << " descent alt " << wpt->get_truealt() << std::endl;
					ad.set_true_altitude(wpt->get_truealt());
				}
				wind.set_wind(wpto->get_winddir_deg(), wpto->get_windspeed_kts());
			} else {
				wpto->frob_flags(FPlanWaypoint::altflag_descent, FPlanWaypoint::altflag_descent);
				wpto->set_engineprofile(ep1.get_descentname());
				if (newmode != mode) {
					if (mode == mode_climb)
						p.push_back(LevelChange(LevelChange::type_toc, cumdist, wpto->get_altitude(), wpto->get_flighttime()));
					if (mode == mode_descent)
						p.push_back(LevelChange(LevelChange::type_bod, cumdist, wpto->get_altitude(), wpto->get_flighttime()));
					if (newmode == mode_climb)
						p.push_back(LevelChange(LevelChange::type_boc, cumdist, wpto->get_altitude(), wpto->get_flighttime()));
					if (newmode == mode_descent)
						p.push_back(LevelChange(LevelChange::type_tod, cumdist, wpto->get_altitude(), wpto->get_flighttime()));
					mode = newmode;
				}
				wpto->set_tas_kts(tas);
				if (descentd <= dist + 1.0 || wpto->get_type() == FPlanWaypoint::type_tod) {
					descentd = std::min(dist, descentd);
					cumdist += descentd;
					dist -= descentd;
					ltime += prevt - nextt;
					p.push_back(LevelChange(LevelChange::type_bod, cumdist, wpt->get_altitude(), ltime));
					newmode = mode = mode_cruise;
					if (dist <= 1.0) {
						dist = 0;
					} else if (!stay) {
						// check POGO
						FPlanWaypoint wptx(*wpto);
						wptx.set_icao("");
						wptx.set_name("*BOD*");
						wptx.set_type(FPlanWaypoint::type_bod);
						wptx.set_coord(wpto->get_coord().get_gcnav(wpt->get_coord(), descentd / legdist).first);
						wptx.set_altitude(wpt->get_altitude());
						wptx.frob_flags(((wptx.get_flags() ^ wpt->get_flags()) & FPlanWaypoint::altflag_standard),
								FPlanWaypoint::altflag_climb | FPlanWaypoint::altflag_descent);
						wptx.set_frequency(0);
						wptx.set_tas(0);
						wptx.set_rpm(0);
						wptx.set_mp(0);
						wpto->set_dist_nmi(descentd);
						wptx.set_dist_nmi(legdist - descentd);
						insert_wpt(i, wptx);
						++n;
						wpt = &(*this)[i];
						wpto = &(*this)[i0];
						dist = 0;
					} else {
						wind.set_wind(wpt->get_winddir_deg(), wpt->get_windspeed_kts());
					}
				} else {
					// descent not finished at waypoint
					nextt = descent.distance_to_time(prevd - dist / speedfactor);
					cumdist += dist;
					dist = 0;
					ltime += prevt - nextt;
					ad.set_pressure_altitude(descent.time_to_altitude(nextt));
				}
				fuel += descent.time_to_fuel(prevt) -
					descent.time_to_fuel(nextt);
			}
		}
		if (newmode != mode) {
			if (mode == mode_climb)
				p.push_back(LevelChange(LevelChange::type_toc, cumdist, wpto->get_altitude(), wpto->get_flighttime()));
			if (mode == mode_descent)
				p.push_back(LevelChange(LevelChange::type_bod, cumdist, wpto->get_altitude(), wpto->get_flighttime()));
			if (newmode == mode_climb)
				p.push_back(LevelChange(LevelChange::type_boc, cumdist, wpto->get_altitude(), wpto->get_flighttime()));
			if (newmode == mode_descent)
				p.push_back(LevelChange(LevelChange::type_tod, cumdist, wpto->get_altitude(), wpto->get_flighttime()));
			mode = newmode;
		}
		// check for cruise segment
		if (mode == mode_cruise && (stay || dist > 0)) {
			Aircraft::Cruise::EngineParams ep1(ep);
			double tas(0), fuelflow(0);
			if (ep1.is_unset()) {
				ep1.set_bhp(std::numeric_limits<double>::quiet_NaN());
				ep1.set_rpm(wpt->get_rpm() ? wpt->get_rpm() : std::numeric_limits<double>::quiet_NaN());
				ep1.set_mp(wpt->get_mp() ? wpt->get_mp_inhg() : std::numeric_limits<double>::quiet_NaN());
			}
			double pa(nextpa), mass1(mass), qnh1(qnh), isaoffs1(tempoffs);
			{
				double m(acft.convert_fuel(acft.get_fuelunit(), Aircraft::unit_kg, fuel));
				if (!std::isnan(m))
					mass1 -= m;
			}
			if (mass1 < massmin)
				mass1 = massmin;
			mass1 = Aircraft::convert(Aircraft::unit_kg, acft.get_wb().get_massunit(), mass1);
			if (std::isnan(qnh1))
				qnh1 = wpto->get_qff_hpa();
			if (std::isnan(isaoffs1))
				isaoffs1 = wpto->get_isaoffset_kelvin();
			if (false)
				ep1.print(std::cerr << "Cruise: before: i " << i << " TAS " << tas << " FF " << fuelflow << " PA " << pa << " mass " << mass1
					  << " isaoffs " << isaoffs1 << " qnh " << qnh1 << " ep ") << " wpt rpm " << wpt->get_rpm() << " mp " << wpt->get_mp() << std::endl;
			acft.calculate_cruise(tas, fuelflow, pa, mass1, isaoffs1, qnh1, ep1);
			if (false)
				ep1.print(std::cerr << "Cruise: after: i " << i << " TAS " << tas << " FF " << fuelflow << " PA " << pa << " mass " << mass1
					  << " isaoffs " << isaoffs1 << " qnh " << qnh1 << " ep ") << std::endl;
			if (tas < 1) {
				std::ostringstream oss;
				oss << "FPlanRoute::recompute: invalid aircraft model (cruise i "
				    << i << " i0 " << i0 << " pa " << pa << " fuel " << fuelflow << " tas " << tas
				    << " mass " << mass1 << " isaoffs " << isaoffs1 << " qnh " << qnh1;
				oss << ')';
				throw std::runtime_error(oss.str());
			}
			if (!wpto->is_climb() && !wpto->is_descent()) {
				wpto->set_tas_kts(tas);
				wpto->set_engineprofile(ep1.get_name());
			}
			if (stay) {
				ltime += staytime;
				fuel +=  staytime * (1.0 / 3600.0) * fuelflow;
			} else {
				double gs(tas);
				if (wind.get_windspeed() > 0) {
					wind.set_crs_tas(tt, tas);
					if (std::isnan(wind.get_gs()) || std::isnan(wind.get_hdg()) || wind.get_gs() <= 0) {
						gs = 1;
					} else {
						if (!wpto->is_climb())
							th = wind.get_hdg();
						gs = wind.get_gs();
					}
					if (false)
						std::cerr << "FPlanRoute::recompute: cruise: windcorr: i " << i << " TT " << tt << " TH " << th
							  << " dist " << dist << " TAS " << tas << " GS " << gs
							  << " Wind " << wind.get_winddir() << '/' << wind.get_windspeed() << std::endl;
				}
				double ct(dist / gs);
				ltime += ct * 3600.0;
				fuel += ct * fuelflow;
			}
			if (!std::isnan(ep1.get_rpm()) && ep1.get_rpm() > 0)
				wpto->set_rpm(ep1.get_rpm());
			if (!std::isnan(ep1.get_mp()) && ep1.get_mp() > 0)
				wpto->set_mp_inhg(ep1.get_mp());
			if (!std::isnan(ep1.get_bhp()) && ep1.get_bhp() > 0)
				wpto->set_bhp(ep1.get_bhp());
		}
		wpt->set_flighttime(ltime);
		wpt->set_fuel_usg(fuel);
		{
			double m(mass);
			m -= acft.convert_fuel(acft.get_fuelunit(), Aircraft::unit_kg, fuel);
			if (std::isnan(m))
				m = massmin;
			wpt->set_mass_kg(m);
		}
		{
			double nextta(ad.get_true_altitude());
			wpt->set_truealt(nextta);
			wmm.compute(nextta * (FPlanWaypoint::ft_to_m / 1000.0), wpt->get_coord(), curtime);
			wpt->set_declination_deg(wmm.get_dec());
		}
		wpto->set_trueheading_deg(th);
		if (i - i0 > 1) {
			double tinc(i - i0);
			tinc = 1.0 / tinc;
			double t0(tinc);
			wpto->set_dist_nmi(wpto->get_dist_nmi() * tinc);
			for (++i0; i0 < i; ++i0, t0 += tinc) {
				FPlanWaypoint& wptx((*this)[i0]);
				wptx.set_flighttime(wpto->get_flighttime() + (wpt->get_flighttime() - wpto->get_flighttime()) * t0);
				wptx.set_fuel_usg(wpto->get_fuel_usg() + (wpt->get_fuel_usg() - wpto->get_fuel_usg()) * t0);
				wptx.set_declination(wpto->get_declination());
				wptx.set_truealt(wpto->get_truealt() + (wpt->get_truealt() - wpto->get_truealt()) * t0);
				wptx.set_truetrack(wpto->get_truetrack());
				wptx.set_trueheading(wpto->get_trueheading());
				wptx.set_dist(wpto->get_dist());
				wptx.set_tas(wpto->get_tas());
				wptx.set_rpm(wpto->get_rpm());
				wptx.set_mp(wpto->get_mp());
				wptx.frob_flags(wpto->get_flags() & (FPlanWaypoint::altflag_climb | FPlanWaypoint::altflag_descent),
						FPlanWaypoint::altflag_climb | FPlanWaypoint::altflag_descent);
				{
					double m(mass);
					m -= acft.convert_fuel(acft.get_fuelunit(), Aircraft::unit_kg, wptx.get_fuel_usg());
					if (std::isnan(m))
						m = massmin;
					wptx.set_mass_kg(m);
				}
			}
		}
		cumdist += dist;
	}
	p.set_dist(cumdist);
	// IFPS currently does not support profile info for non-pure IFR/GAT flights
	{
		uint16_t flags_or, flags_and;
		if (get_flags(flags_or, flags_and) && (flags_and & FPlanWaypoint::altflag_ifr) && !(flags_or & FPlanWaypoint::altflag_oat))
			p.remove_close_distat(1.0);
		else
			p.clear();
	}
	// check masses
	{
		unsigned int n(get_nrwpt());
		if (n) {
			uint32_t dm((*this)[n-1].get_mass());
			uint32_t mm(Point::round<uint32_t,float>(massmin * 1024.0));
			if ((uint32_t)(-dm) < (uint32_t)std::numeric_limits<int32_t>::max() || dm < mm) {
				dm = mm - dm;
				for (unsigned int i(0); i < n; ++i) {
					FPlanWaypoint& wpt((*this)[i]);
				        wpt.set_mass(wpt.get_mass() + dm);
				}
			}
		}
	}
	// remove generated waypoints
	for (unsigned int i(0), n(get_nrwpt()); i < n; ) {
		FPlanWaypoint& wpt((*this)[i]);
		if (wpt.get_type() < FPlanWaypoint::type_generated_start || wpt.get_type() > FPlanWaypoint::type_generated_end ||
		    (insert_tocbod && wpt.get_type() >= FPlanWaypoint::type_boc && wpt.get_type() <= FPlanWaypoint::type_tod)) {
			++i;
			continue;
		}
		if (wpt.get_type() == FPlanWaypoint::type_center)
			wptcenter = wpt;
		if (i) {
			FPlanWaypoint& wpto((*this)[i - 1]);
			wpto.set_dist(wpto.get_dist() + wpt.get_dist());
		}
		delete_wpt(i);
		n = get_nrwpt();
		p.remove_waypoint(i);
	}
	// compute climb and descent profile
	if (get_nrwpt()) {
		const FPlanWaypoint& wptc((*this)[0]);
		const FPlanWaypoint& wptd((*this)[get_nrwpt()-1]);
		double mass1(Aircraft::convert(Aircraft::unit_kg, acft.get_wb().get_massunit(), wptc.get_mass_kg()));
		double mass2(Aircraft::convert(Aircraft::unit_kg, acft.get_wb().get_massunit(), wptd.get_mass_kg()));
		double qnh1(qnh), tempoffs1(tempoffs), qnh2(qnh), tempoffs2(tempoffs);
		Aircraft::Cruise::EngineParams ep1(ep), ep2(ep);
		if (std::isnan(qnh1))
			qnh1 = wptc.get_qff() ? wptc.get_qff_hpa() : IcaoAtmosphere<float>::std_sealevel_pressure;
		if (std::isnan(tempoffs1))
			tempoffs1 = wptc.get_isaoffset_kelvin();
		if (std::isnan(qnh2))
			qnh2 = wptd.get_qff() ? wptd.get_qff_hpa() : IcaoAtmosphere<float>::std_sealevel_pressure;
		if (std::isnan(tempoffs2))
			tempoffs2 = wptd.get_isaoffset_kelvin();
		Aircraft::ClimbDescent climb(acft.calculate_climb(ep1, mass1, tempoffs1, qnh1));
		Aircraft::ClimbDescent desc(acft.calculate_descent(ep2, mass2, tempoffs2, qnh2));
		static const int32_t altsteps = 1000;
		int32_t alt((max_altitude() + altsteps - 1) / altsteps * altsteps);
		int32_t alt1(std::min(alt, Point::round<int32_t,double>(climb.get_ceiling())));
		int32_t alt2(std::min(alt, Point::round<int32_t,double>(desc.get_ceiling())));
		for (alt = altsteps; alt <= alt1 || alt <= alt2; alt += altsteps) {
			if (alt <= alt1)
				m_climbprofile.push_back(climb, alt);
			if (alt <= alt2)
				m_descentprofile.push_back(desc, alt);
		}
	}
	if (false) {
		for (unsigned int i(0), n(get_nrwpt()); i < n; ++i) {
			const FPlanWaypoint& wpt((*this)[i]);
			std::ostringstream oss;
			oss << "FPL[" << i << "]: " << wpt.to_detailed_str();
			std::cerr << oss.str() << std::endl;
		}
	}
	return p;
}

const std::vector<FPlanAlternate>::size_type FPlanRoute::FuelCalc::invalid_altn;

FPlanRoute::FuelCalc::FuelCalc(void)
	: m_taxifuel(std::numeric_limits<double>::quiet_NaN()),
	  m_taxifuelflow(std::numeric_limits<double>::quiet_NaN()),
	  m_tripfuel(std::numeric_limits<double>::quiet_NaN()),
	  m_contfuel(std::numeric_limits<double>::quiet_NaN()),
	  m_contfuelpercent(std::numeric_limits<double>::quiet_NaN()),
	  m_contfuelflow(std::numeric_limits<double>::quiet_NaN()),
	  m_altnfuel(std::numeric_limits<double>::quiet_NaN()),
	  m_holdfuel(std::numeric_limits<double>::quiet_NaN()),
	  m_holdfuelflow(std::numeric_limits<double>::quiet_NaN()),
	  m_reqdfuel(std::numeric_limits<double>::quiet_NaN()),
	  m_taxitime(0),
	  m_holdtime(0),
	  m_altnalt(FPlanWaypoint::invalid_altitude),
	  m_holdalt(FPlanWaypoint::invalid_altitude),
	  m_altnidx(invalid_altn)
{
}

FPlanRoute::FuelCalc FPlanRoute::calculate_fuel(const Aircraft& acft, const std::vector<FPlanAlternate>& altn) const
{
	FuelCalc fc;
	// find preferred alternate
	bool noaltn(altn.empty() || (get_nrwpt() && altn.size() == 1 && !(*this)[get_nrwpt()-1].get_coord().is_invalid() &&
				     (*this)[get_nrwpt()-1].get_coord() == altn.front().get_coord()));
	fc.m_altnidx = 0U;
	if (!noaltn) {
		time_t ft(0);
		for (std::vector<FPlanAlternate>::size_type i(0), n(altn.size()); i < n; ++i) {
			const FPlanAlternate& a(altn[i]);
			if (a.get_coord().is_invalid() || a.get_flighttime() < ft)
				continue;
			ft = a.get_flighttime();
			fc.m_altnidx = i;
		}
	}
	// fuel calculation
	fc.m_taxifuel = acft.get_taxifuel();
	if (get_nrwpt()) {
		fc.m_taxitime = (*this)[0].get_time_unix() - get_time_offblock_unix();
		fc.m_taxifuelflow = acft.get_taxifuelflow();
		double tf(fc.m_taxitime * fc.m_taxifuelflow * (1.0 / 3600.0));
		if (!std::isnan(tf) && tf >= 0) {
			if (std::isnan(fc.m_taxifuel))
				fc.m_taxifuel = 0;
			fc.m_taxifuel += tf;
		}
	}
	fc.m_reqdfuel = fc.m_taxifuel;
	if (std::isnan(fc.m_reqdfuel))
		fc.m_reqdfuel = 0;
	if (get_nrwpt() >= 2) {
		fc.m_tripfuel = (*this)[get_nrwpt()-1].get_fuel_usg();
		if (!std::isnan(fc.m_tripfuel) && fc.m_tripfuel > 0)
			fc.m_reqdfuel += fc.m_tripfuel;
		fc.m_contfuelpercent = acft.get_contingencyfuel();
		if (std::isnan(fc.m_contfuelpercent))
			fc.m_contfuelpercent = 5;
		fc.m_contfuel = 1e-2 * fc.m_contfuelpercent * fc.m_tripfuel;
		double mincontfuel(std::numeric_limits<double>::quiet_NaN());
		double mincontfuelflow(std::numeric_limits<double>::quiet_NaN());
		if (!altn.empty() && altn.front().get_holdtime() > 0) {
			mincontfuelflow = 3600.0 * altn.front().get_holdfuel_usg() / altn.front().get_holdtime();
			mincontfuel = mincontfuelflow * (5.0 / 60.0);
		}
		if (!std::isnan(mincontfuel) && (std::isnan(fc.m_contfuel) || fc.m_contfuel < mincontfuel)) {
			fc.m_contfuel = mincontfuel;
			fc.m_contfuelflow = mincontfuelflow;
			fc.m_contfuelpercent = std::numeric_limits<double>::quiet_NaN();
		}
		if (!std::isnan(fc.m_contfuel) && fc.m_contfuel > 0)
			fc.m_reqdfuel += fc.m_contfuel;
		if (!noaltn && fc.m_altnidx < altn.size()) {
			const FPlanAlternate& a(altn[fc.m_altnidx]);
			if (a.is_cruisealtitude_valid())
				fc.m_altnalt = a.get_cruisealtitude();
			fc.m_altnfuel = a.get_fuel_usg();
			if (get_nrwpt() >= 2)
				fc.m_altnfuel -= (*this)[get_nrwpt()-1].get_fuel_usg();
			fc.m_altnfuel = std::max(fc.m_altnfuel, 0.);
			if (!std::isnan(fc.m_altnfuel) && fc.m_altnfuel > 0)
				fc.m_reqdfuel += fc.m_altnfuel;
		}
		if (fc.m_altnidx < altn.size()) {
			const FPlanAlternate& a(altn[fc.m_altnidx]);
			if (a.is_holdaltitude_valid())
				fc.m_holdalt = a.get_holdaltitude();
			fc.m_holdfuel = a.get_holdfuel_usg();
			if (!std::isnan(fc.m_holdfuel) && fc.m_holdfuel > 0)
				fc.m_reqdfuel += fc.m_holdfuel;
			fc.m_holdtime = a.get_holdtime();
			if (a.get_holdtime() > 0)
				fc.m_holdfuelflow = fc.m_holdfuel * 3600.0 / a.get_holdtime();
		}
	}
	if (noaltn)
		fc.m_altnidx = FuelCalc::invalid_altn;
	return fc;
}

namespace {
	void nwxweather_cb(const NWXWeather::windvector_t& winds, FPlanRoute& route, const sigc::slot<void,bool>& cb)
	{
		bool fplmodif(false);
		for (unsigned int i = 0; i < winds.size(); ++i) {
			const NWXWeather::WindsAloft& wind(winds[i]);
			if (true) {
				std::cerr << "Coord " << wind.get_coord().get_lat_str2() << ' ' << wind.get_coord().get_lon_str2() << " Alt ";
				if (wind.get_flags() & FPlanWaypoint::altflag_standard)
					std::cerr << "F" << wind.get_altitude();
				else
					std::cerr << wind.get_altitude();
				{
					struct tm utm;
					char buf[64];
					time_t t(wind.get_time());
					strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", gmtime_r(&t, &utm));
					std::cerr << ' ' << buf;
				}
				std::cerr << ' ' << wind.get_winddir() << '/' << wind.get_windspeed()
					  << ' ' << wind.get_qff() << ' ' << wind.get_temp() << std::endl;
			}
			if (i >= route.get_nrwpt())
				continue;
			FPlanWaypoint& wpt(route[i]);
			{
				float d(wpt.get_coord().simple_distance_nmi(wind.get_coord()));
				if (d > 10) {
					std::cerr << "nwxweather: skipping " << wpt.get_icao_name() << " distance " << d << "nmi" << std::endl;
					continue;
				}
			}
			wpt.set_winddir_deg(wind.get_winddir());
			wpt.set_windspeed_kts(wind.get_windspeed());
			wpt.set_qff_hpa(wind.get_qff());
			wpt.set_oat_kelvin(wind.get_temp());
			fplmodif = true;
		}
		cb(fplmodif);
	}
};

void FPlanRoute::nwxweather(NWXWeather& nwx, const sigc::slot<void,bool>& cb)
{
        nwx.abort();
        nwx.winds_aloft(*this, sigc::bind(sigc::bind(sigc::ptr_fun(&nwxweather_cb), cb), *this));
}

FPlanRoute::GFSResult FPlanRoute::gfs(GRIB2& grib2)
{
	std::vector<FPlanAlternate> altn;
	return gfs(grib2, altn);
}

FPlanRoute::GFSResult FPlanRoute::gfs(GRIB2& grib2, std::vector<FPlanAlternate>& altn)
{
	Rect bbox(get_bbox());
	for (std::vector<FPlanAlternate>::const_iterator ai(altn.begin()), ae(altn.end()); ai != ae; ++ai) {
		if (ai->get_coord().is_invalid())
			continue;
		bbox = bbox.add(ai->get_coord());
	}
	bbox = bbox.oversize_nmi(100.f);
	GFSResult r;
	Glib::RefPtr<GRIB2::LayerInterpolateResult> windu, windv, prmsl, temperature, tropopause;
	for (unsigned int wptnr(0), nrwpt(get_nrwpt()), wptsz(get_nrwpt() + altn.size()); wptnr < wptsz; ++wptnr) {
		FPlanWaypoint& wpt((wptnr >= nrwpt) ? altn[wptnr - nrwpt] : (*this)[wptnr]);
		if (wpt.get_coord().is_invalid())
			continue;
		float press(0);
		IcaoAtmosphere<float>::std_altitude_to_pressure(&press, 0, wpt.get_altitude() * Point::ft_to_m);
		press *= 100;
		if (!windu || press < windu->get_minsurface1value() || press > windu->get_maxsurface1value() ||
		    wpt.get_time_unix() < windu->get_minefftime() || wpt.get_time_unix() > windu->get_maxefftime()) {
			{
				GRIB2::layerlist_t ll(grib2.find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_momentum_ugrd),
									wpt.get_time_unix(), GRIB2::surface_isobaric_surface, press));
				windu = GRIB2::interpolate_results(bbox, ll, wpt.get_time_unix(), press);
			}
			{
				GRIB2::layerlist_t ll(grib2.find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_momentum_vgrd),
									wpt.get_time_unix(), GRIB2::surface_isobaric_surface, press));
				windv = GRIB2::interpolate_results(bbox, ll, wpt.get_time_unix(), press);
			}
		}
		if (!temperature || press < temperature->get_minsurface1value() || press > temperature->get_maxsurface1value() ||
		    wpt.get_time_unix() < temperature->get_minefftime() || wpt.get_time_unix() > temperature->get_maxefftime()) {
			GRIB2::layerlist_t ll(grib2.find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_temperature_tmp),
								wpt.get_time_unix(), GRIB2::surface_isobaric_surface, press));
			temperature = GRIB2::interpolate_results(bbox, ll, wpt.get_time_unix(), press);
		}
		if (!prmsl ||
		    wpt.get_time_unix() < prmsl->get_minefftime() || wpt.get_time_unix() > prmsl->get_maxefftime()) {
			GRIB2::layerlist_t ll(grib2.find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_prmsl),
								wpt.get_time_unix()));
			prmsl = GRIB2::interpolate_results(bbox, ll, wpt.get_time_unix());
		}
		if (!tropopause ||
		    wpt.get_time_unix() < tropopause->get_minefftime() || wpt.get_time_unix() > tropopause->get_maxefftime()) {
			GRIB2::layerlist_t ll(grib2.find_layers(GRIB2::find_parameter(GRIB2::param_meteorology_mass_hgt),
								wpt.get_time_unix(), GRIB2::surface_tropopause, 0));
			tropopause = GRIB2::interpolate_results(bbox, ll, wpt.get_time_unix());
		}
		if (windu && windv) {
			if (wpt.get_time_unix() >= windu->get_minefftime() && wpt.get_time_unix() <= windu->get_maxefftime() &&
			    wpt.get_time_unix() >= windv->get_minefftime() && wpt.get_time_unix() <= windv->get_maxefftime()) {
				float wu(windu->operator()(wpt.get_coord(), wpt.get_time_unix(), press));
				float wv(windv->operator()(wpt.get_coord(), wpt.get_time_unix(), press));
				if (!std::isnan(wu) && !std::isnan(wv)) {
					{
						std::pair<float,float> w(windu->get_layer()->get_grid()->transform_axes(wu, wv));
						wu = w.first;
						wv = w.second;
					}
					// convert from m/s to kts
					wu *= (-1e-3f * Point::km_to_nmi * 3600);
					wv *= (-1e-3f * Point::km_to_nmi * 3600);
					wpt.set_windspeed_kts(sqrtf(wu * wu + wv * wv));
					wpt.set_winddir_rad(M_PI - atan2f(wu, wv));
					r.set_modified();
					r.add_efftime(windu->get_minefftime());
					r.add_efftime(windu->get_maxefftime());
					r.add_reftime(windu->get_minreftime());
					r.add_reftime(windu->get_maxreftime());
					r.add_efftime(windv->get_minefftime());
					r.add_efftime(windv->get_maxefftime());
					r.add_reftime(windv->get_minreftime());
					r.add_reftime(windv->get_maxreftime());
				} else {
					r.set_partial();
				}
			} else {
				r.set_partial();
			}
		}
		if (temperature) {
			if (wpt.get_time_unix() >= temperature->get_minefftime() && wpt.get_time_unix() <= temperature->get_maxefftime()) {
				double tk(temperature->operator()(wpt.get_coord(), wpt.get_time_unix(), press));
				if (!std::isnan(tk) && tk >= 100) {
					wpt.set_oat_kelvin(tk);
					r.set_modified();
					r.add_efftime(temperature->get_minefftime());
					r.add_efftime(temperature->get_maxefftime());
					r.add_reftime(temperature->get_minreftime());
					r.add_reftime(temperature->get_maxreftime());
				} else {
					r.set_partial();
				}
			} else {
				r.set_partial();
			}
		}
		if (prmsl) {
			if (wpt.get_time_unix() >= prmsl->get_minefftime() && wpt.get_time_unix() <= prmsl->get_maxefftime()) {
				double hpa(prmsl->operator()(wpt.get_coord(), wpt.get_time_unix(), press) * 0.01f);
				if (!std::isnan(hpa) && hpa >= 100) {
					wpt.set_qff_hpa(hpa);
					r.set_modified();
					r.add_efftime(prmsl->get_minefftime());
					r.add_efftime(prmsl->get_maxefftime());
					r.add_reftime(prmsl->get_minreftime());
					r.add_reftime(prmsl->get_maxreftime());
				} else {
					r.set_partial();
				}
			} else {
				r.set_partial();
			}
		}
		if (tropopause) {
			if (wpt.get_time_unix() >= tropopause->get_minefftime() && wpt.get_time_unix() <= tropopause->get_maxefftime()) {
				double tp(tropopause->operator()(wpt.get_coord(), wpt.get_time_unix(), 0) * Point::m_to_ft);
				if (!std::isnan(tp) && tp >= 0) {
					wpt.set_tropopause(tp);
					r.set_modified();
					r.add_efftime(tropopause->get_minefftime());
					r.add_efftime(tropopause->get_maxefftime());
					r.add_reftime(tropopause->get_minreftime());
					r.add_reftime(tropopause->get_maxreftime());
				} else {
					r.set_partial();
				}
			} else {
				r.set_partial();
			}
		}
	}
	if (get_nrwpt() && (*this)[0].get_coord().is_invalid()) {
		for (unsigned int wptnr(1), nrwpt(get_nrwpt()); wptnr < nrwpt; ++wptnr) {
			const FPlanWaypoint& wpt((*this)[wptnr]);
			if (wpt.get_coord().is_invalid())
				continue;
			while (wptnr > 0) {
				--wptnr;
				FPlanWaypoint& wptx((*this)[wptnr]);
				wptx.set_qff(wpt.get_qff());
				wptx.set_isaoffset(wpt.get_isaoffset());
				wptx.set_windspeed(wpt.get_windspeed());
				wptx.set_winddir(wpt.get_winddir());
				wptx.set_tropopause(wpt.get_tropopause());
			}
			break;
		}
	}
	for (unsigned int wptnr(0), nrwpt(get_nrwpt()); wptnr < nrwpt; ++wptnr) {
		const FPlanWaypoint& wpta((*this)[wptnr]);
		if (wpta.get_coord().is_invalid())
			continue;
		uint32_t totdist(wpta.get_dist());
		uint32_t dist(totdist);
		unsigned int wptnre(wptnr + 1);
		for (; wptnre < nrwpt; ++wptnre) {
			const FPlanWaypoint& wptb((*this)[wptnre]);
			if (!wptb.get_coord().is_invalid())
				break;
			totdist += wptb.get_dist();
		}
		if (wptnre - wptnr <= 1U)
			continue;
		if (wptnre >= nrwpt) {
			for (;;) {
				++wptnr;
				if (wptnr >= nrwpt)
					break;
				FPlanWaypoint& wptx((*this)[wptnr]);
				wptx.set_qff(wpta.get_qff());
				wptx.set_isaoffset(wpta.get_isaoffset());
				wptx.set_windspeed(wpta.get_windspeed());
				wptx.set_winddir(wpta.get_winddir());
				wptx.set_tropopause(wpta.get_tropopause());
			}
			break;
		}
		const FPlanWaypoint& wptb((*this)[wptnre]);
		for (++wptnr; wptnr < wptnre; ++wptnr) {
			FPlanWaypoint& wptx((*this)[wptnr]);
			double frac(dist);
			if (totdist)
				dist /= totdist;
			frac = std::min(std::max(frac, 0.), 1.);
			double fraca(1 - frac);
			wptx.set_qff_hpa(wpta.get_qff_hpa() * fraca + wptb.get_qff_hpa() * frac);
			wptx.set_isaoffset_kelvin(wpta.get_isaoffset_kelvin() * fraca + wptb.get_isaoffset_kelvin() * frac);
			Wind<double> wnd;
			wnd.set_wind(wpta.get_winddir_deg(), wpta.get_windspeed_kts(), wptb.get_winddir_deg(), wptb.get_windspeed_kts(), frac);
			wptx.set_windspeed_kts(wnd.get_windspeed());
			wptx.set_winddir_deg(wnd.get_winddir());
			dist += wptx.get_dist();
		}
	}
	if (get_nrwpt()) {
		const FPlanWaypoint& wpta((*this)[get_nrwpt() - 1]);
		for (std::vector<FPlanAlternate>::iterator ai(altn.begin()), ae(altn.end()); ai != ae; ++ai) {
			FPlanAlternate& wptx(*ai);
			if (!wptx.get_coord().is_invalid())
				continue;
			wptx.set_qff(wpta.get_qff());
			wptx.set_isaoffset(wpta.get_isaoffset());
			wptx.set_windspeed(wpta.get_windspeed());
			wptx.set_winddir(wpta.get_winddir());
			wptx.set_tropopause(wpta.get_tropopause());
		}
	}
	return r;
}

void FPlanRoute::turnpoints(bool include_dct, float maxdev)
{
	for (waypoints_t::iterator i(m_wpts.begin()), e(m_wpts.end()); i != e; ++i)
		i->frob_flags(FPlanWaypoint::altflag_turnpoint, FPlanWaypoint::altflag_turnpoint);
	for (unsigned int nr = 1U; nr + 2U < get_nrwpt(); ) {
		unsigned int nr1(nr);
		for (; nr1 + 2U < get_nrwpt(); ++nr1) {
			const FPlanWaypoint& wpt2((*this)[nr1 + 2U]);
			const FPlanWaypoint& wpt1((*this)[nr1 + 1U]);
			const FPlanWaypoint& wpt0((*this)[nr]);
			if (!(wpt0.get_flags() & wpt1.get_flags() & FPlanWaypoint::altflag_ifr) ||
			    ((wpt0.get_flags() ^ wpt1.get_flags()) & FPlanWaypoint::altflag_standard) ||
			    (wpt0.get_altitude() != wpt1.get_altitude()))
				break;
			if (((wpt0.get_pathcode() != FPlanWaypoint::pathcode_airway &&
			      wpt0.get_pathcode() != FPlanWaypoint::pathcode_sid &&
			      wpt0.get_pathcode() != FPlanWaypoint::pathcode_star) ||
			     wpt1.get_pathcode() != wpt0.get_pathcode() ||
			     wpt0.get_pathname().empty() || wpt0.get_pathname() != wpt1.get_pathname()) &&
			    (!include_dct || wpt0.get_pathcode() != FPlanWaypoint::pathcode_directto ||
			     wpt1.get_pathcode() != FPlanWaypoint::pathcode_directto))
				break;
			if (maxdev < std::numeric_limits<float>::max()) {
				bool ok(true);
				for (unsigned int i = nr; i <= nr1; ) {
					++i;
					const FPlanWaypoint& wptx((*this)[i]);
					Point pt(wptx.get_coord().spheric_nearest_on_line(wpt0.get_coord(), wpt2.get_coord()));
					if (pt.spheric_distance_nmi(wptx.get_coord()) <= maxdev)
						continue;
					ok = false;
					break;
				}
				if (!ok)
					break;
			}
		}
		++nr;
		for (; nr <= nr1; ++nr)
			(*this)[nr].frob_flags(0, FPlanWaypoint::altflag_turnpoint);
	}
}

void FPlanRoute::fix_invalid_altitudes(unsigned int nr)
{
	if (nr >= m_wpts.size())
		return;
	FPlanWaypoint& wpt(m_wpts[nr]);
	if (wpt.is_altitude_valid())
		return;
	if (nr) {
		const FPlanWaypoint& wptp(m_wpts[nr-1]);
		if (wpt.is_ifr() || wptp.is_ifr())
			wpt.set_altitude(10000);
		else if (wptp.get_magnetictrack() < 0x8000)
			wpt.set_altitude(5500);
		else
			wpt.set_altitude(4500);
		return;
	}
	if (wpt.is_ifr())
		wpt.set_altitude(10000);
	else if (wpt.get_magnetictrack() < 0x8000)
		wpt.set_altitude(5500);
	else
		wpt.set_altitude(4500);
}

void FPlanRoute::fix_invalid_altitudes(unsigned int nr, TopoDb30& topodb, AirspacesDbQueryInterface& aspcdb)
{
	if (nr >= m_wpts.size())
		return;
	FPlanWaypoint& wpt(m_wpts[nr]);
	if (wpt.is_altitude_valid())
		return;
	if (wpt.get_coord().is_invalid()) {
		fix_invalid_altitudes(nr);
		return;
	}
	int32_t base(500);
	int32_t terrain(0);
	uint16_t magcrs(wpt.get_magnetictrack());
	if (wpt.is_ifr()) {
		base = 0;
		terrain = 10000;
	}
	if (nr) {
		const FPlanWaypoint& wptp(m_wpts[nr-1]);
		if (wptp.is_ifr()) {
			base = 0;
			terrain = 10000;
		}
		magcrs = wptp.get_magnetictrack();
	}
	bool italy_portugal(false);
	bool newzealand(false);
	bool england(false);
	{
		{
			AirspacesDb::elementvector_t ev(aspcdb.find_by_icao("LI", 0, AirspacesDb::comp_exact, 0, AirspacesDb::element_t::subtables_all));
			for (AirspacesDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				if (evi->get_icao() != "LI" || evi->get_typecode() != AirspacesDb::Airspace::typecode_ead ||
				    evi->get_bdryclass() != AirspacesDb::Airspace::bdryclass_ead_nas)
					continue;
				if (!evi->get_bbox().is_inside(wpt.get_coord()))
					continue;
				if (!evi->get_polygon().windingnumber(wpt.get_coord()))
					continue;
				italy_portugal = true;
				break;
			}
		}
		if (!italy_portugal) {
			AirspacesDb::elementvector_t ev(aspcdb.find_by_icao("LPPC", 0, AirspacesDb::comp_exact, 0, AirspacesDb::element_t::subtables_all));
			for (AirspacesDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				if (evi->get_icao() != "LPPC" || evi->get_typecode() != AirspacesDb::Airspace::typecode_ead ||
				    evi->get_bdryclass() != AirspacesDb::Airspace::bdryclass_ead_fir)
					continue;
				if (!evi->get_bbox().is_inside(wpt.get_coord()))
					continue;
				if (!evi->get_polygon().windingnumber(wpt.get_coord()))
					continue;
				italy_portugal = true;
				break;
			}
		}
		{
        		AirspacesDb::elementvector_t ev(aspcdb.find_by_icao("NZ", 0, AirspacesDb::comp_exact, 0, AirspacesDb::element_t::subtables_all));
			for (AirspacesDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				if (evi->get_icao() != "NZ" || evi->get_typecode() != AirspacesDb::Airspace::typecode_ead ||
				    evi->get_bdryclass() != AirspacesDb::Airspace::bdryclass_ead_nas)
					continue;
				if (!evi->get_bbox().is_inside(wpt.get_coord()))
					continue;
				if (!evi->get_polygon().windingnumber(wpt.get_coord()))
					continue;
			        newzealand = true;
				break;
			}
		}
		{
			// London FIR
			AirspacesDb::elementvector_t ev(aspcdb.find_by_icao("EGTT", 0, AirspacesDb::comp_exact, 0, AirspacesDb::element_t::subtables_all));
			for (AirspacesDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				if (evi->get_icao() != "EGTT" || evi->get_typecode() != AirspacesDb::Airspace::typecode_ead ||
				    evi->get_bdryclass() != AirspacesDb::Airspace::bdryclass_ead_fir)
					continue;
				if (!evi->get_bbox().is_inside(wpt.get_coord()))
					continue;
				if (!evi->get_polygon().windingnumber(wpt.get_coord()))
					continue;
			        england = true;
				break;
			}
		}
		if (!england) {
			// Scottish FIR
			AirspacesDb::elementvector_t ev(aspcdb.find_by_icao("EGPX", 0, AirspacesDb::comp_exact, 0, AirspacesDb::element_t::subtables_all));
			for (AirspacesDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				if (evi->get_icao() != "EGPX" || evi->get_typecode() != AirspacesDb::Airspace::typecode_ead ||
				    evi->get_bdryclass() != AirspacesDb::Airspace::bdryclass_ead_fir)
					continue;
				if (!evi->get_bbox().is_inside(wpt.get_coord()))
					continue;
				if (!evi->get_polygon().windingnumber(wpt.get_coord()))
					continue;
			        england = true;
				break;
			}
		}
		if (nr) {
			TopoDb30::Profile profile(topodb.get_profile(m_wpts[nr-1].get_coord(), wpt.get_coord(), 5.0f));
			TopoDb30::elev_t maxelev(profile.get_maxelev());
			if (maxelev != TopoDb30::nodata)
				terrain = std::max(terrain, Point::round<int32_t,double>(Point::m_to_ft_dbl * maxelev));
		}
		if (nr + 1 < m_wpts.size()) {
			TopoDb30::Profile profile(topodb.get_profile(wpt.get_coord(), m_wpts[nr+1].get_coord(), 5.0f));
			TopoDb30::elev_t maxelev(profile.get_maxelev());
			if (maxelev != TopoDb30::nodata)
				terrain = std::max(terrain, Point::round<int32_t,double>(Point::m_to_ft_dbl * maxelev));
		}
	}
	if (england) {
		// quadrantal rules - not binding for VFR, but apply nevertheless
		base = 0;
		if (magcrs < 0x8000) {
			base += 1000;
			if (magcrs >= 0x4000)
				base += 500;
		} else {
			if (magcrs >= 0xc000)
				base += 500;
		}
	} else {
		if (italy_portugal) {
			if (magcrs >= 0x4000 && magcrs < 0xc000)
				base += 1000;
		} else if (newzealand) {
			if (magcrs < 0x4000 || magcrs >= 0xc000)
				base += 1000;
		} else {
			if (magcrs < 0x8000)
				base += 1000;
		}
	}
	{
		int32_t minalt(terrain);
		if (minalt >= 5000)
			minalt += 1000;
		minalt += 1000;
		int32_t alt = ((minalt + 1000 - base) / 2000) * 2000 + base;
		alt = std::max(std::min(alt, (int32_t)32000), (int32_t)0);
		wpt.set_altitude(alt);
	}
}

void FPlanRoute::fix_invalid_altitudes(unsigned int nr, Engine& eng)
{
	if (nr >= m_wpts.size())
		return;
	FPlanWaypoint& wpt(m_wpts[nr]);
	if (wpt.is_altitude_valid())
		return;
	if (wpt.get_coord().is_invalid()) {
		fix_invalid_altitudes(nr);
		return;
	}
	int32_t base(500);
	int32_t terrain(0);
	uint16_t magcrs(wpt.get_magnetictrack());
	if (wpt.is_ifr()) {
		base = 0;
		terrain = 10000;
	}
	if (nr) {
		const FPlanWaypoint& wptp(m_wpts[nr-1]);
		if (wptp.is_ifr()) {
			base = 0;
			terrain = 10000;
		}
		magcrs = wptp.get_magnetictrack();
	}
	bool italy_portugal(false);
	bool newzealand(false);
	bool england(false);
	{
		Glib::RefPtr<Engine::AirspaceResult> qli(eng.async_airspace_find_by_icao("LI", ~0, 0, AirspacesDb::comp_exact, AirspacesDb::element_t::subtables_all));
		Glib::RefPtr<Engine::AirspaceResult> qlppc(eng.async_airspace_find_by_icao("LPPC", ~0, 0, AirspacesDb::comp_exact, AirspacesDb::element_t::subtables_all));
		Glib::RefPtr<Engine::AirspaceResult> qnz(eng.async_airspace_find_by_icao("NZ", ~0, 0, AirspacesDb::comp_exact, AirspacesDb::element_t::subtables_all));
		Glib::RefPtr<Engine::AirspaceResult> qegtt(eng.async_airspace_find_by_icao("EGTT", ~0, 0, AirspacesDb::comp_exact, AirspacesDb::element_t::subtables_all));
		Glib::RefPtr<Engine::AirspaceResult> qegpx(eng.async_airspace_find_by_icao("EGPX", ~0, 0, AirspacesDb::comp_exact, AirspacesDb::element_t::subtables_all));
		Glib::RefPtr<Engine::ElevationProfileResult> qprof0, qprof1;
		if (nr)
			qprof0 = eng.async_elevation_profile(m_wpts[nr-1].get_coord(), wpt.get_coord());
		if (nr + 1 < m_wpts.size())
			qprof1 = eng.async_elevation_profile(wpt.get_coord(), m_wpts[nr+1].get_coord());
		Glib::Mutex mutex;
		Glib::Cond cond;
		if (qli)
			qli->connect(sigc::mem_fun(cond, &Glib::Cond::broadcast));
		if (qlppc)
			qlppc->connect(sigc::mem_fun(cond, &Glib::Cond::broadcast));
		if (qnz)
			qnz->connect(sigc::mem_fun(cond, &Glib::Cond::broadcast));
		if (qegtt)
			qegtt->connect(sigc::mem_fun(cond, &Glib::Cond::broadcast));
		if (qegpx)
			qegpx->connect(sigc::mem_fun(cond, &Glib::Cond::broadcast));
		if (qprof0)
			qprof0->connect(sigc::mem_fun(cond, &Glib::Cond::broadcast));
		if (qprof1)
			qprof1->connect(sigc::mem_fun(cond, &Glib::Cond::broadcast));
		{
			Glib::Mutex::Lock lock(mutex);
			for (;;) {
				if (qli) {
					bool ok(qli->is_done());
					if (ok && !italy_portugal) {
						const AirspacesDb::elementvector_t& ev(qli->get_result());
						for (AirspacesDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
							if (evi->get_icao() != "LI" || evi->get_typecode() != AirspacesDb::Airspace::typecode_ead ||
							    evi->get_bdryclass() != AirspacesDb::Airspace::bdryclass_ead_nas)
								continue;
							if (!evi->get_bbox().is_inside(wpt.get_coord()))
								continue;
							if (!evi->get_polygon().windingnumber(wpt.get_coord()))
								continue;
							italy_portugal = true;
							break;
						}
					}
					if (ok || qli->is_error()) {
						Glib::RefPtr<Engine::AirspaceResult> q(qli);
						qli.reset();
						Engine::AirspaceResult::cancel(q);
					}
				}
				if (qlppc) {
					bool ok(qlppc->is_done());
					if (ok && !italy_portugal) {
						const AirspacesDb::elementvector_t& ev(qlppc->get_result());
						for (AirspacesDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
							if (evi->get_icao() != "LPPC" || evi->get_typecode() != AirspacesDb::Airspace::typecode_ead ||
							    evi->get_bdryclass() != AirspacesDb::Airspace::bdryclass_ead_fir)
								continue;
							if (!evi->get_bbox().is_inside(wpt.get_coord()))
								continue;
							if (!evi->get_polygon().windingnumber(wpt.get_coord()))
								continue;
							italy_portugal = true;
							break;
						}
					}
					if (ok || qlppc->is_error()) {
						Glib::RefPtr<Engine::AirspaceResult> q(qlppc);
						qlppc.reset();
						Engine::AirspaceResult::cancel(q);
					}
				}
				if (qnz) {
					bool ok(qnz->is_done());
					if (ok) {
						const AirspacesDb::elementvector_t& ev(qnz->get_result());
						for (AirspacesDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
							if (evi->get_icao() != "NZ" || evi->get_typecode() != AirspacesDb::Airspace::typecode_ead ||
							    evi->get_bdryclass() != AirspacesDb::Airspace::bdryclass_ead_nas)
								continue;
							if (!evi->get_bbox().is_inside(wpt.get_coord()))
								continue;
							if (!evi->get_polygon().windingnumber(wpt.get_coord()))
								continue;
							newzealand = true;
							break;
						}
					}
					if (ok || qnz->is_error()) {
						Glib::RefPtr<Engine::AirspaceResult> q(qnz);
						qnz.reset();
						Engine::AirspaceResult::cancel(q);
					}
				}
				if (qegtt) {
					bool ok(qegtt->is_done());
					if (ok && !england) {
						const AirspacesDb::elementvector_t& ev(qegtt->get_result());
						for (AirspacesDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
							if (evi->get_icao() != "EGTT" || evi->get_typecode() != AirspacesDb::Airspace::typecode_ead ||
							    evi->get_bdryclass() != AirspacesDb::Airspace::bdryclass_ead_fir)
								continue;
							if (!evi->get_bbox().is_inside(wpt.get_coord()))
								continue;
							if (!evi->get_polygon().windingnumber(wpt.get_coord()))
								continue;
							england = true;
							break;
						}
					}
					if (ok || qegtt->is_error()) {
						Glib::RefPtr<Engine::AirspaceResult> q(qegtt);
						qegtt.reset();
						Engine::AirspaceResult::cancel(q);
					}
				}
				if (qegpx) {
					bool ok(qegpx->is_done());
					if (ok && !england) {
						const AirspacesDb::elementvector_t& ev(qegpx->get_result());
						for (AirspacesDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
							if (evi->get_icao() != "EGPX" || evi->get_typecode() != AirspacesDb::Airspace::typecode_ead ||
							    evi->get_bdryclass() != AirspacesDb::Airspace::bdryclass_ead_fir)
								continue;
							if (!evi->get_bbox().is_inside(wpt.get_coord()))
								continue;
							if (!evi->get_polygon().windingnumber(wpt.get_coord()))
								continue;
							england = true;
							break;
						}
					}
					if (ok || qegpx->is_error()) {
						Glib::RefPtr<Engine::AirspaceResult> q(qegpx);
						qegpx.reset();
						Engine::AirspaceResult::cancel(q);
					}
				}
				if (qprof0) {
					bool ok(qprof0->is_done());
					if (ok && !england) {
						const TopoDb30::Profile& profile(qprof0->get_result());
						TopoDb30::elev_t maxelev(profile.get_maxelev());
						if (maxelev != TopoDb30::nodata)
							terrain = std::max(terrain, Point::round<int32_t,double>(Point::m_to_ft_dbl * maxelev));
					}
					if (ok || qprof0->is_error()) {
						Glib::RefPtr<Engine::ElevationProfileResult> q(qprof0);
						qprof0.reset();
						Engine::AirspaceResult::cancel(q);
					}
				}
				if (qprof1) {
					bool ok(qprof1->is_done());
					if (ok && !england) {
						const TopoDb30::Profile& profile(qprof1->get_result());
						TopoDb30::elev_t maxelev(profile.get_maxelev());
						if (maxelev != TopoDb30::nodata)
							terrain = std::max(terrain, Point::round<int32_t,double>(Point::m_to_ft_dbl * maxelev));
					}
					if (ok || qprof1->is_error()) {
						Glib::RefPtr<Engine::ElevationProfileResult> q(qprof1);
						qprof1.reset();
						Engine::AirspaceResult::cancel(q);
					}
				}
				if (!qli && !qlppc && !qnz && !qegtt && !qegpx && !qprof0 && !qprof1)
					break;
				cond.wait(mutex);
			}
		}
	}
	if (england) {
		// quadrantal rules - not binding for VFR, but apply nevertheless
		base = 0;
		if (magcrs < 0x8000) {
			base += 1000;
			if (magcrs >= 0x4000)
				base += 500;
		} else {
			if (magcrs >= 0xc000)
				base += 500;
		}
	} else {
		if (italy_portugal) {
			if (magcrs >= 0x4000 && magcrs < 0xc000)
				base += 1000;
		} else if (newzealand) {
			if (magcrs < 0x4000 || magcrs >= 0xc000)
				base += 1000;
		} else {
			if (magcrs < 0x8000)
				base += 1000;
		}
	}
	{
		int32_t minalt(terrain);
		if (minalt >= 5000)
			minalt += 1000;
		minalt += 1000;
		int32_t alt = ((minalt + 1000 - base) / 2000) * 2000 + base;
		alt = std::max(std::min(alt, (int32_t)32000), (int32_t)0);
		wpt.set_altitude(alt);
	}
}

void FPlanRoute::fix_invalid_altitudes(void)
{
	for (waypoints_t::size_type i(0), n(m_wpts.size()); i < n; ++i)
		fix_invalid_altitudes(i);
}

void FPlanRoute::fix_invalid_altitudes(TopoDb30& topodb, AirspacesDbQueryInterface& aspc)
{
	for (waypoints_t::size_type i(0), n(m_wpts.size()); i < n; ++i)
		fix_invalid_altitudes(i, topodb, aspc);
}

void FPlanRoute::fix_invalid_altitudes(Engine& eng)
{
	for (waypoints_t::size_type i(0), n(m_wpts.size()); i < n; ++i)
		fix_invalid_altitudes(i, eng);
}

void FPlanRoute::compute_terrain(TopoDb30& topodb, double corridor_nmi, bool roundcaps)
{
	for (waypoints_t::size_type i(0), n(m_wpts.size()); i < n; ) {
		m_wpts[i].unset_terrain();
		if (m_wpts[i].get_coord().is_invalid()) {
			++i;
			continue;
		}
		waypoints_t::size_type j(i);
		while (j + 1U < n) {
			++j;
			if (!m_wpts[j].get_coord().is_invalid())
				break;
		}
		if (i == j || m_wpts[j].get_coord().is_invalid()) {
			while (i < j) {
				++i;
				m_wpts[i].unset_terrain();
			}
			++i;
			continue;
		}
		TopoDb30::elev_t e(TopoDb30::nodata);
		if (roundcaps || true) {
			TopoDb30::minmax_elev_t mme(topodb.get_minmax_elev(m_wpts[i].get_coord().make_fat_line(m_wpts[j].get_coord(), corridor_nmi, roundcaps)));
			if (mme.first != TopoDb30::nodata && mme.second != TopoDb30::nodata && mme.first <= mme.second)
				e = mme.second;
		} else {
			TopoDb30::Profile prof(topodb.get_profile(m_wpts[i].get_coord(), m_wpts[j].get_coord(), corridor_nmi));
			TopoDb30::elev_t ee(prof.get_maxelev());
			if (!prof.empty() && ee != TopoDb30::nodata) {
				if (ee == TopoDb30::ocean)
					ee = 0;
				e = ee;
			}
		}
		if (e == TopoDb30::nodata) {
			while (i < j) {
				++i;
				m_wpts[i].unset_terrain();
			}
			continue;
		}
		int32_t t(Point::round<int32_t,double>(e * Point::m_to_ft_dbl));
		while (i < j) {
			m_wpts[i].set_terrain(t);
			++i;
		}
	}
}

void FPlanRoute::delete_nocoord_waypoints(void)
{
	for (waypoints_t::iterator i(m_wpts.begin()), e(m_wpts.end()); i != e; ) {
		if (i->get_coord().is_invalid()) {
			i = m_wpts.erase(i);
			e = m_wpts.end();
			continue;
		}
		++i;
	}
}

void FPlanRoute::delete_colocated_waypoints(void)
{
	for (waypoints_t::iterator i(m_wpts.begin()), e(m_wpts.end()); i != e; ) {
		waypoints_t::iterator ip(i);
		++i;
		if (i == e)
			break;
		if (ip->get_coord() != i->get_coord())
			continue;
		waypoints_t::iterator in(i);
		++in;
		if (in == e) {
			i = m_wpts.erase(ip);
		} else {
			i = m_wpts.erase(i);
			--i;
		}
		e = m_wpts.end();
	}
}

void FPlanRoute::delete_sametime_waypoints(void)
{
	for (waypoints_t::iterator i(m_wpts.begin()), e(m_wpts.end()); i != e; ) {
		waypoints_t::iterator ip(i);
		++i;
		if (i == e)
			break;
		if (ip->get_flighttime() < i->get_flighttime())
			continue;
		waypoints_t::iterator in(i);
		++in;
		if (in == e) {
			i = m_wpts.erase(ip);
		} else {
			i = m_wpts.erase(i);
			--i;
		}
		e = m_wpts.end();
	}
}

std::string FPlanRoute::get_garminpilot(void) const
{
	std::ostringstream fpl;
	fpl << "garminpilot://flightplan?etd=" << get_time_offblock_unix();
	if (!m_wpts.empty()) {
		static const char *routedelim[3] = { "+", ".", "&route=" };
		unsigned int rdelim(2);
		for (waypoints_t::const_iterator wi(m_wpts.begin()), we(m_wpts.end()); wi != we;) {
			const FPlanWaypoint& wpt(*wi);
			++wi;
			switch (wpt.get_type()) {
			case FPlanWaypoint::type_airport:
				if (AirportsDb::Airport::is_fpl_zzzz(wpt.get_icao()))
					goto coord;
				// fall through

			case FPlanWaypoint::type_navaid:
				fpl << routedelim[rdelim] << wpt.get_icao();
				rdelim = 0;
				break;

			case FPlanWaypoint::type_intersection:
				fpl << routedelim[rdelim] << wpt.get_name();
				rdelim = 0;
				break;

			default:
			case FPlanWaypoint::type_mapelement:
			case FPlanWaypoint::type_undefined:
			case FPlanWaypoint::type_fplwaypoint:
			case FPlanWaypoint::type_vfrreportingpt:
			case FPlanWaypoint::type_user:
			coord:
				fpl << routedelim[rdelim] << wpt.get_coord().get_lat_deg_dbl() << '/' << wpt.get_coord().get_lon_deg_dbl();
				rdelim = 0;
				break;

			case FPlanWaypoint::type_generated_start ... FPlanWaypoint::type_generated_end:
				break;
			}
			switch (wpt.get_pathcode()) {
			default:
				break;

			case FPlanWaypoint::pathcode_airway:
			case FPlanWaypoint::pathcode_sid:
			case FPlanWaypoint::pathcode_star:
			{
				if (rdelim || wpt.get_pathname().empty())
					break;
				waypoints_t::const_iterator wie(we);
				for (waypoints_t::const_iterator win(wi); win != we; ++win) {
					const FPlanWaypoint& wpt2(*win);
					switch (wpt2.get_type()) {
					case FPlanWaypoint::type_airport:
						if (wpt.get_pathcode() == FPlanWaypoint::pathcode_star)
							wie = win;
						break;

					case FPlanWaypoint::type_navaid:
					case FPlanWaypoint::type_intersection:
						if (wpt.get_pathcode() != FPlanWaypoint::pathcode_star)
							wie = win;
						break;

					default:
						break;
					}
					if (wpt2.get_pathcode() != wpt.get_pathcode() ||
					    wpt2.get_pathname() != wpt.get_pathname())
						break;
				}
				if (wie != we) {
					wi = wie;
        				switch (wpt.get_pathcode()) {
					default:
						break;

					case FPlanWaypoint::pathcode_airway:
						fpl << routedelim[rdelim] << wpt.get_pathname();
						rdelim = 0;
						break;

					case FPlanWaypoint::pathcode_sid:
						fpl << routedelim[rdelim] << wpt.get_pathname();
						rdelim = 1;
						break;

					case FPlanWaypoint::pathcode_star:
						fpl << routedelim[1] << wpt.get_pathname();
						break;
					}
				}
				break;
			}
			}
		}
		// check for altitude
		int32_t alt(max_altitude());
		if (alt != FPlanWaypoint::invalid_altitude)
			fpl << "&altitude=" << alt;
		// speed
		double dist(total_distance_nmi_dbl());
		time_t flttime(m_wpts.back().get_flighttime());
		if (!std::isnan(dist) && dist > 0 && flttime > 0)
			fpl << "&speed=" << Point::round<int32_t,double>(dist * 3600.0 / flttime);
	}
	return fpl.str();
}

Glib::ustring FPlan::get_userdbpath(void)
{
#ifdef __MINGW32__
	Glib::ustring dbname(Glib::build_filename(Glib::get_user_config_dir(), "vfrnav"));
#else
        Glib::ustring dbname;
	if (!GLIB_CHECK_VERSION(2, 36, 0)) {
		bool found(false);
		std::string x(Glib::getenv("HOME", found));
		if (found)
			dbname = x;
	}
	if (dbname.empty())
		dbname = Glib::get_home_dir();
	dbname = Glib::build_filename(dbname, USERDBPATH);
#endif
#if defined(HAVE_GIOMM) && 0
        {
                Glib::RefPtr<Gio::File> dir(Gio::File::create_for_path(dbname));
                try {
#ifdef HAVE_GIOMM_MAKE_DIRECTORY_WITH_PARENTS
                        if (dir->make_directory_with_parents())
#else
                        if (dir->make_directory())
#endif
                                std::cerr << "vfrnav directory (" << dbname << ") created" << std::endl;
                        else
                                throw std::runtime_error("vfrnav directory creation failed");
                } catch (const Gio::Error& e) {
                        if (e.code() != Gio::Error::EXISTS)
                                throw;
                        std::cerr << "vfrnav directory (" << dbname << ") already exists" << std::endl;
                }
        }
#else
        if (g_mkdir_with_parents(dbname.c_str(), 0755)) {
                throw std::runtime_error("vfrnav directory (" + dbname +") creation failed");
        }
#endif
        return dbname;
}

FPlan::FPlan(void)
{
        Glib::ustring dbname(Glib::build_filename(get_userdbpath(), "fplan.db"));
        db.open(dbname);
        if (sqlite3_create_function(db.db(), "simpledist", 4, /*SQLITE_UTF8*/SQLITE_ANY, 0, &dbfunc_simpledist, 0, 0) != SQLITE_OK)
                throw std::runtime_error("FPlanDb: cannot add user function simpledist");
        init_database();
}

FPlan::FPlan(sqlite3 *dbh)
{
        db.take(dbh);
        if (sqlite3_create_function(db.db(), "simpledist", 4, /*SQLITE_UTF8*/SQLITE_ANY, 0, &dbfunc_simpledist, 0, 0) != SQLITE_OK)
                throw std::runtime_error("FPlanDb: cannot add user function simpledist");
        init_database();
}

FPlan::~FPlan() throw()
{
}

void FPlan::dbfunc_simpledist(sqlite3_context *ctxt, int, sqlite3_value **values)
{
        sqlite3_result_double(ctxt, Point(sqlite3_value_int(values[0]), sqlite3_value_int(values[1]))
                        .spheric_distance_km(Point(sqlite3_value_int(values[2]), sqlite3_value_int(values[3]))));
}

void FPlan::init_database(void)
{
        {
                sqlite3x::sqlite3_command cmd(db, "CREATE TABLE IF NOT EXISTS fplan (ID INTEGER PRIMARY KEY NOT NULL, "
                                              "TIMEOFFBLOCK INTEGER,"
                                              "SAVETIMEOFFBLOCK INTEGER,"
                                              "TIMEONBLOCK INTEGER,"
                                              "SAVETIMEONBLOCK INTEGER,"
                                              "CURWPT INTEGER,"
                                              "NOTE TEXT);");
                cmd.executenonquery();
        }
	{
		static const struct field {
			const char *fldname;
			const char *fldtype;
		} fields[] = {
			{ "ICAO", "TEXT" },
			{ "NAME", "TEXT" },
			{ "LON", "INTEGER" },
			{ "LAT", "INTEGER" },
			{ "ALT", "INTEGER" },
			{ "FREQ", "INTEGER" },
			{ "TIME", "INTEGER" },
			{ "SAVETIME", "INTEGER" },
			{ "FLAGS", "INTEGER" },
			{ "PATHNAME", "TEXT" },
			{ "PATHCODE", "INTEGER" },
			{ "NOTE", "TEXT" },
			{ "WINDDIR", "INTEGER" },
			{ "WINDSPEED", "INTEGER" },
			{ "QFF", "INTEGER" },
			{ "ISAOFFS", "INTEGER" },
			{ "TRUEALT", "INTEGER" },
			{ "TERRAIN", "INTEGER" },
			{ "TRUETRACK", "INTEGER" },
			{ "TRUEHEADING", "INTEGER" },
			{ "DECLINATION", "INTEGER" },
			{ "DIST", "INTEGER" },
			{ "FUEL", "INTEGER" },
			{ "TAS", "INTEGER" },
			{ "RPM", "INTEGER" },
			{ "MP", "INTEGER" },
			{ "TYPE", "INTEGER" },
			{ "NAVAID", "INTEGER" },
			{ "FLIGHTTIME", "INTEGER" },
			{ "MASS", "INTEGER" },
			{ "ENGINEPROFILE", "TEXT" },
			{ "BHP", "INTEGER" },
			{ "TROPOPAUSE", "INTEGER" },
			{ 0, 0 }
		};
		{
			std::string flds;
			for (const struct field *f(fields); f->fldname && f->fldtype; ++f)
				flds += std::string(f->fldname) + " " + f->fldtype + ",";
			sqlite3x::sqlite3_command cmd(db, "CREATE TABLE IF NOT EXISTS waypoints (PLANID INTEGER NOT NULL "
						      "CONSTRAINT fk_planid REFERENCES fplan(ID) ON DELETE CASCADE,"
						      "NR INTEGER NOT NULL," + flds +
						      "PRIMARY KEY(PLANID,NR));");
			cmd.executenonquery();
		}
		// Auto Add new columns if they do not yet exist
		{
			sqlite3x::sqlite3_command cmd(db, "SELECT sql FROM sqlite_master WHERE type = \"table\" AND name = \"waypoints\";");
			sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
			if (!cursor.step())
				throw std::runtime_error("FPlan::init_database: unable to determine schema");
			std::string sql(cursor.getstring(0));
			unsigned int fldmask(0);
			for (std::string::const_iterator si(sql.begin()), se(sql.end()); si != se;) {
				if (!std::isalnum(*si)) {
					++si;
					continue;
				}
				std::string::const_iterator si2(si);
				++si2;
				while (si2 != se && std::isalnum(*si2))
					++si2;
				Glib::ustring tok(si, si2);
				si = si2;
				tok = tok.casefold();
				for (unsigned int i = 0; fields[i].fldname && fields[i].fldtype; ++i) {
					if (tok != Glib::ustring(fields[i].fldname).casefold())
						continue;
					fldmask |= 1U << i;
					break;
				}
			}
			for (unsigned int i = 0; fields[i].fldname && fields[i].fldtype; ++i) {
				if (fldmask & (1U << i))
					continue;
				sqlite3x::sqlite3_command cmd(db, std::string("ALTER TABLE waypoints ADD COLUMN ") + fields[i].fldname +
							      " " + fields[i].fldtype + ";");
				cmd.executenonquery();
			}
		}
	}
        { sqlite3x::sqlite3_command cmd(db, "CREATE INDEX IF NOT EXISTS wptplanindex ON waypoints (PLANID);"); cmd.executenonquery(); }
        { sqlite3x::sqlite3_command cmd(db, "CREATE INDEX IF NOT EXISTS wptplanidnr ON waypoints (PLANID, NR);"); cmd.executenonquery(); }
        { sqlite3x::sqlite3_command cmd(db, "CREATE INDEX IF NOT EXISTS wptlon ON waypoints (LON);"); cmd.executenonquery(); }
        { sqlite3x::sqlite3_command cmd(db, "CREATE INDEX IF NOT EXISTS wptlat ON waypoints (LAT);"); cmd.executenonquery(); }
        { sqlite3x::sqlite3_command cmd(db, "CREATE INDEX IF NOT EXISTS wptname ON waypoints (NAME);"); cmd.executenonquery(); }
        { sqlite3x::sqlite3_command cmd(db, "CREATE INDEX IF NOT EXISTS wpticao ON waypoints (ICAO);"); cmd.executenonquery(); }
}

void FPlan::delete_database(void)
{
        { sqlite3x::sqlite3_command cmd(db, "DROP TABLE IF EXISTS fplan;"); cmd.executenonquery(); }
        { sqlite3x::sqlite3_command cmd(db, "DROP TABLE IF EXISTS waypoints;"); cmd.executenonquery(); }
        { sqlite3x::sqlite3_command cmd(db, "DROP INDEX IF EXISTS wptplanindex;"); cmd.executenonquery(); }
        { sqlite3x::sqlite3_command cmd(db, "DROP INDEX IF EXISTS wptplanidnr;"); cmd.executenonquery(); }
        { sqlite3x::sqlite3_command cmd(db, "DROP INDEX IF EXISTS wptlon;"); cmd.executenonquery(); }
        { sqlite3x::sqlite3_command cmd(db, "DROP INDEX IF EXISTS wptlat;"); cmd.executenonquery(); }
        { sqlite3x::sqlite3_command cmd(db, "DROP INDEX IF EXISTS wptname;"); cmd.executenonquery(); }
        { sqlite3x::sqlite3_command cmd(db, "DROP INDEX IF EXISTS wpticao;"); cmd.executenonquery(); }
        init_database();
}

std::string FPlan::escape_string(const std::string& s)
{
        std::string r;
        r.reserve(s.size());
        for (std::string::const_iterator i1(s.begin()), ie(s.end()); i1 != ie; i1++) {
                std::string::value_type c(*i1);
                if (c == '_' || c == '%')
                        r.push_back('%');
                r.push_back(c);
        }
        return r;
}

std::vector<FPlanWaypoint> FPlan::find_by_name(const Glib::ustring & nm, unsigned int limit, unsigned int skip, comp_t comp)
{
        std::vector<FPlanWaypoint> wpts;
        {
                sqlite3x::sqlite3_command cmd(db, "SELECT " + FPlanWaypoint::reader_fields + " FROM waypoints WHERE NAME LIKE ? ORDER BY NAME ASC LIMIT ? OFFSET ?;");
                Glib::ustring pattern(escape_string(nm));
                switch (comp) {
                        case comp_contains:
                                pattern = '%' + pattern;
                                /* fall through */

                        case comp_startswith:
                                pattern += '%';
                                break;

                        case comp_exact:
                        default:
                                break;
                }
                cmd.bind(1, pattern);
                cmd.bind(2, (int)limit);
                cmd.bind(3, (int)skip);
                sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
                while (cursor.step())
                        wpts.push_back(FPlanWaypoint(cursor));
        }
        return wpts;
}

std::vector<FPlanWaypoint> FPlan::find_by_icao(const Glib::ustring & ic, unsigned int limit, unsigned int skip, comp_t comp)
{
        std::vector<FPlanWaypoint> wpts;
        {
                sqlite3x::sqlite3_command cmd(db, "SELECT " + FPlanWaypoint::reader_fields + " FROM waypoints WHERE ICAO LIKE ? ORDER BY ICAO ASC LIMIT ? OFFSET ?;");
                Glib::ustring pattern(escape_string(ic));
                switch (comp) {
                        case comp_contains:
                                pattern = '%' + pattern;
                                /* fall through */

                        case comp_startswith:
                                pattern += '%';
                                break;

                        case comp_exact:
                        default:
                                break;
                }
                cmd.bind(1, pattern);
                cmd.bind(2, (int)limit);
                cmd.bind(3, (int)skip);
                sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
                while (cursor.step())
                        wpts.push_back(FPlanWaypoint(cursor));
        }
        return wpts;
}

std::vector<FPlanWaypoint> FPlan::find_nearest(const Point& pt, unsigned int limit, unsigned int skip, const Rect& rect)
{
        std::vector<FPlanWaypoint> wpts;
        {
                std::string rectexpr("LAT >= ?6 AND LAT <= ?8 AND LON >= ?5 AND LON <= ?7");
                if (rect.get_northeast().get_lon() < rect.get_southwest().get_lon())
                        std::string rectexpr("LAT >= ?6 AND LAT <= ?8 AND (LON <= ?5 OR LON >= ?7)");
                sqlite3x::sqlite3_command cmd(db, "SELECT " + FPlanWaypoint::reader_fields + " FROM waypoints WHERE " + rectexpr + " ORDER BY simpledist(LON,LAT,?1,?2) ASC LIMIT ?3 OFFSET ?4;");
                cmd.bind(1, pt.get_lon());
                cmd.bind(2, pt.get_lat());
                cmd.bind(3, (int)limit);
                cmd.bind(4, (int)skip);
                cmd.bind(5, rect.get_southwest().get_lon());
                cmd.bind(6, rect.get_southwest().get_lat());
                cmd.bind(7, rect.get_northeast().get_lon());
                cmd.bind(8, rect.get_northeast().get_lat());
                sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
                while (cursor.step())
                        wpts.push_back(FPlanWaypoint(cursor));
        }
        return wpts;
}
