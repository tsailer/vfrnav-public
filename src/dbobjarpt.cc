//
// C++ Implementation: dbobjarpt
//
// Description: Database Objects: Airport
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012, 2013, 2016, 2017
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <limits>

#include "dbobj.h"

#ifdef HAVE_PQXX
#include <pqxx/transactor.hxx>
#include <pqxx/result.hxx>
#include <pqxx/except.hxx>
#endif

const uint16_t DbBaseElements::Airport::Runway::flag_le_usable;
const uint16_t DbBaseElements::Airport::Runway::flag_he_usable;

const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_off;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_pcl;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_sf;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_tdzl;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_cl;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_hirl;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_mirl;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_lirl;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_rail;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_reil;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_alsf2;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_alsf1;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_sals_salsf;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_ssalr;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_mals_malsf_ssals_ssalsf;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_malsr;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_overruncenterline;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_center;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_usconfiguration;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_hongkongcurve;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_centerrow;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_leftsinglerow;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_formernatostandard;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_centerrow2;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_natostandard;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_centerdoublerow;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_portableapproach;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_centerrow3;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_hals;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_2parallelrow;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_leftrow;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_airforceoverrun;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_calvert;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_singlerowcenterline;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_narrowmulticross;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_centerlinehighintensity;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_alternatecenverline;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_cross;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_centerrow4;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_singaporecenterline;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_centerline2crossbars;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_odals;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_vasi;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_tvasi;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_pvasi;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_jumbo;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_tricolor;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_apap;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_papi;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_ols;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_waveoff;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_portable;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_floods;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_lights;
const DbBaseElements::Airport::Runway::light_t DbBaseElements::Airport::Runway::light_lcvasi;

const Glib::ustring& DbBaseElements::Airport::Runway::get_light_string(light_t x)
{
	switch (x) {
	case light_off:
	{
		static const Glib::ustring r("Off");
		return r;
	}

	case light_pcl:
	{
		static const Glib::ustring r("Pilot Controlled Lighting");
		return r;
	}

	case light_sf:
	{
		static const Glib::ustring r("Sequenced Flashing Lights");
		return r;
	}

	case light_tdzl:
	{
		static const Glib::ustring r("Touchdown Zone Lighting");
		return r;
	}

	case light_cl:
	{
		static const Glib::ustring r("Centerline Lighting System");
		return r;
	}

	case light_hirl:
	{
		static const Glib::ustring r("High Intensity Runway Lights");
		return r;
	}

	case light_mirl:
	{
		static const Glib::ustring r("Medium Intensity Runway Lighting System");
		return r;
	}

	case light_lirl:
	{
		static const Glib::ustring r("Low Intensity Runway Lighting System");
		return r;
	}

	case light_rail:
	{
		static const Glib::ustring r("Runway Alignment Lights");
		return r;
	}

	case light_reil:
	{
		static const Glib::ustring r("Runway End Identifier Lights");
		return r;
	}

	case light_alsf2:
	{
		static const Glib::ustring r("ALSF-2");
		return r;
	}

	case light_alsf1:
	{
		static const Glib::ustring r("ALSF-1");
		return r;
	}

	case light_sals_salsf:
	{
		static const Glib::ustring r("SALS or SALSF");
		return r;
	}

	case light_ssalr:
	{
		static const Glib::ustring r("SSALR");
		return r;
	}

	case light_mals_malsf_ssals_ssalsf:
	{
		static const Glib::ustring r("MALS and MALSF or SSALS and SSALF");
		return r;
	}

	case light_malsr:
	{
		static const Glib::ustring r("MALSR");
		return r;
	}

	case light_overruncenterline:
	{
		static const Glib::ustring r("Overrun Centerline");
		return r;
	}

	case light_center:
	{
		static const Glib::ustring r("Centerline and Bar");
		return r;
	}

	case light_usconfiguration:
	{
		static const Glib::ustring r("US Configuration");
		return r;
	}

	case light_hongkongcurve:
	{
		static const Glib::ustring r("Hong Kong Curve");
		return r;
	}

	case light_centerrow:
	{
		static const Glib::ustring r("Center Row");
		return r;
	}

	case light_leftsinglerow:
	{
		static const Glib::ustring r("Left Single Row");
		return r;
	}

	case light_formernatostandard:
	{
		static const Glib::ustring r("Former NATO Standard");
		return r;
	}

	case light_centerrow2:
	{
		static const Glib::ustring r("Center Row (2)");
		return r;
	}

	case light_natostandard:
	{
		static const Glib::ustring r("NATO Standard");
		return r;
	}

	case light_centerdoublerow:
	{
		static const Glib::ustring r("Center and Double Row");
		return r;
	}

	case light_portableapproach:
	{
		static const Glib::ustring r("Portable Approach");
		return r;
	}

	case light_centerrow3:
	{
		static const Glib::ustring r("Center Row (3)");
		return r;
	}

	case light_hals:
	{
		static const Glib::ustring r("Helicopter Approach Lighting System (HALS)");
		return r;
	}

	case light_2parallelrow:
	{
		static const Glib::ustring r("Two Parallel Row");
		return r;
	}

	case light_leftrow:
	{
		static const Glib::ustring r("Left Row (High Intensity)");
		return r;
	}

	case light_airforceoverrun:
	{
		static const Glib::ustring r("Air Force Overrun");
		return r;
	}

	case light_calvert:
	{
		static const Glib::ustring r("Calvert (British)");
		return r;
	}

	case light_singlerowcenterline:
	{
		static const Glib::ustring r("Single Row Centerline");
		return r;
	}

	case light_narrowmulticross:
	{
		static const Glib::ustring r("Narrow Multi-Cross");
		return r;
	}

	case light_centerlinehighintensity:
	{
		static const Glib::ustring r("Centerline High Intensity");
		return r;
	}

	case light_alternatecenverline:
	{
		static const Glib::ustring r("Alternate Centerline and Bar");
		return r;
	}

	case light_cross:
	{
		static const Glib::ustring r("Cross");
		return r;
	}

	case light_centerrow4:
	{
		static const Glib::ustring r("Center Row (4)");
		return r;
	}

	case light_singaporecenterline:
	{
		static const Glib::ustring r("Singapore Centerline");
		return r;
	}

	case light_centerline2crossbars:
	{
		static const Glib::ustring r("Centerline 2 Crossbars");
		return r;
	}

	case light_odals:
	{
		static const Glib::ustring r("Omni-directional Approach Lighting System");
		return r;
	}

	case light_vasi:
	{
		static const Glib::ustring r("Visual Approach Slope Indicator");
		return r;
	}

	case light_tvasi:
	{
		static const Glib::ustring r("T-Visual Approach Slope Indicator");
		return r;
	}

	case light_pvasi:
	{
		static const Glib::ustring r("Pulsating Visual Approach Slope Indicator");
		return r;
	}

	case light_jumbo:
	{
		static const Glib::ustring r("VASI with a TCH to accommodate long bodied or jumbo aircraft");
		return r;
	}

	case light_tricolor:
	{
		static const Glib::ustring r("Tri-color Arrival Approach");
		return r;
	}

	case light_apap:
	{
		static const Glib::ustring r("Alignment of Elements System");
		return r;
	}

	case light_papi:
	{
		static const Glib::ustring r("Precision Approach Path Indicator");
		return r;
	}

	case light_ols:
	{
		static const Glib::ustring r("Optical landing System");
		return r;
	}

	case light_waveoff:
	{
		static const Glib::ustring r("WAVEOFF");
		return r;
	}

	case light_portable:
	{
		static const Glib::ustring r("Portable");
		return r;
	}

	case light_floods:
	{
		static const Glib::ustring r("Floods");
		return r;
	}

	case light_lights:
	{
		static const Glib::ustring r("Lights");
		return r;
	}

	case light_lcvasi:
	{
		static const Glib::ustring r("Low Cost Visual Approach Slope Indicator");
		return r;
	}
	}
	{
		static const Glib::ustring r;
		return r;
	}
}

DbBaseElements::Airport::Runway::Runway(const Glib::ustring & idhe, const Glib::ustring & idle, uint16_t len, uint16_t wid,
					const Glib::ustring & sfc, const Point & hecoord, const Point & lecoord,
					uint16_t hetda, uint16_t helda, uint16_t hedisp, uint16_t hehdg, int16_t heelev,
					uint16_t letda, uint16_t lelda, uint16_t ledisp, uint16_t lehdg, int16_t leelev,
					uint16_t flags)
	: m_ident_he(idhe), m_ident_le(idle), m_length(len), m_width(wid),
	  m_surface(sfc), m_he_coord(hecoord), m_le_coord(lecoord),
	  m_he_tda(hetda), m_he_lda(helda), m_he_disp(hedisp), m_he_hdg(hehdg), m_he_elev(heelev),
	  m_le_tda(letda), m_le_lda(lelda), m_le_disp(ledisp), m_le_hdg(lehdg), m_le_elev(leelev),
	  m_flags(flags)
{
	memset(m_he_lights, 0, sizeof(m_he_lights));
	memset(m_le_lights, 0, sizeof(m_le_lights));
}

DbBaseElements::Airport::Runway::Runway(void)
	: m_ident_he(), m_ident_le(), m_length(0), m_width(0),
	  m_surface(), m_he_coord(), m_le_coord(),
	  m_he_tda(0), m_he_lda(0), m_he_disp(0), m_he_hdg(0), m_he_elev(0),
	  m_le_tda(0), m_le_lda(0), m_le_disp(0), m_le_hdg(0), m_le_elev(0),
	  m_flags(0)
{
	memset(m_he_lights, 0, sizeof(m_he_lights));
	memset(m_le_lights, 0, sizeof(m_le_lights));
}

void DbBaseElements::Airport::Runway::compute_default_hdg(void)
{
	char *cp;
	uint16_t ang = strtoul(m_ident_he.c_str(), &cp, 10);
	if (cp == m_ident_he.c_str())
		ang = 0;
	ang = Point::round<int,float>(ang * (10.f * FPlanLeg::from_deg));
	m_he_hdg = ang;
	m_le_hdg = ang + 0x8000;
}

void DbBaseElements::Airport::Runway::compute_default_coord(const Point & arptcoord)
{
	m_he_coord = arptcoord.spheric_course_distance_nmi((m_he_hdg + 0x8000) * FPlanLeg::to_deg, m_length * (0.5f * Point::ft_to_m * 0.001f * Point::km_to_nmi));
	m_le_coord = arptcoord.spheric_course_distance_nmi((m_le_hdg + 0x8000) * FPlanLeg::to_deg, m_length * (0.5f * Point::ft_to_m * 0.001f * Point::km_to_nmi));
}

bool DbBaseElements::Airport::Runway::is_concrete(void) const
{
	return (m_surface == "ASP") || (m_surface == "ASPH") || (m_surface == "CON") || (m_surface == "PEM");
}

bool DbBaseElements::Airport::Runway::is_water(void) const
{
	return (m_surface == "WTR") || (m_surface == "WATER");
}

void DbBaseElements::Airport::Runway::swap_he_le(void)
{
	std::swap(m_ident_he, m_ident_le);
	std::swap(m_he_coord, m_le_coord);
	std::swap(m_he_tda, m_le_tda);
	std::swap(m_he_lda, m_le_lda);
	std::swap(m_he_disp, m_le_disp);
	std::swap(m_he_hdg, m_le_hdg);
	std::swap(m_he_elev, m_le_elev);
	for (unsigned int i = 0; i < sizeof(m_he_lights)/sizeof(m_he_lights[0]); ++i)
		std::swap(m_he_lights[i], m_le_lights[i]);
	m_flags = (m_flags & ~(flag_he_usable | flag_le_usable))
			| ((m_flags << 1) & flag_he_usable)
			| ((m_flags >> 1) & flag_le_usable);
}

bool DbBaseElements::Airport::Runway::normalize_he_le(void)
{
#if 1
	if (m_ident_le.size() < 2)
		return false;
	if (!Glib::Unicode::isdigit(m_ident_le[0]) || !Glib::Unicode::isdigit(m_ident_le[1]))
		return false;
	unsigned int le((m_ident_le[0] - '0') * 10 + (m_ident_le[1] - '0'));
	if (le < 19)
		return false;
#else
	if (m_he_hdg >= 0x8000)
		return false;
#endif
	swap_he_le();
	return true;
}

DbBaseElements::Airport::FASNavigate DbBaseElements::Airport::Runway::get_he_fake_fas(void) const
{
	FASNavigate fas;
	if (!is_he_usable())
		return fas;
	fas.set_ident("(" + get_ident_he() + ")");
	fas.set_course(get_he_coord(), get_le_coord(), 1000.0 * Point::ft_to_m_dbl * 0.001 * Point::km_to_nmi_dbl,
		       std::numeric_limits<double>::quiet_NaN());
	fas.set_thralt(get_he_elev());
	return fas;
}

DbBaseElements::Airport::FASNavigate DbBaseElements::Airport::Runway::get_le_fake_fas(void) const
{
	FASNavigate fas;
	if (!is_le_usable())
		return fas;
	fas.set_ident("(" + get_ident_le() + ")");
	fas.set_course(get_le_coord(), get_he_coord(), 1000.0 * Point::ft_to_m_dbl * 0.001 * Point::km_to_nmi_dbl,
		       std::numeric_limits<double>::quiet_NaN());
	fas.set_thralt(get_le_elev());
	return fas;
}

DbBaseElements::Airport::Helipad::Helipad(const Glib::ustring & id, uint16_t len, uint16_t wid, const Glib::ustring & sfc, const Point & coord, uint16_t hdg, int16_t elev, uint16_t flags)
	: m_ident(id), m_length(len), m_width(wid), m_surface(sfc), m_coord(coord), m_hdg(hdg), m_elev(elev), m_flags(flags)
{
}

DbBaseElements::Airport::Helipad::Helipad(void)
	: m_ident(), m_length(0), m_width(0), m_surface(), m_coord(), m_hdg(0), m_elev(0), m_flags(0)
{
}

bool DbBaseElements::Airport::Helipad::is_concrete(void) const
{
	return (m_surface == "ASP") || (m_surface == "ASPH") || (m_surface == "CON") || (m_surface == "PEM");
}

DbBaseElements::Airport::Comm::Comm( const Glib::ustring & nm, const Glib::ustring & sect, const Glib::ustring & oprhr, uint32_t f0, uint32_t f1, uint32_t f2, uint32_t f3, uint32_t f4 )
	: m_name(nm), m_sector(sect), m_oprhours(oprhr)
{
	m_freq[0] = f0;
	m_freq[1] = f1;
	m_freq[2] = f2;
	m_freq[3] = f3;
	m_freq[4] = f4;
}

DbBaseElements::Airport::Comm::Comm(void)
	: m_name(), m_sector(), m_oprhours()
{
	m_freq[0] = m_freq[1] = m_freq[2] = m_freq[3] = m_freq[4] = 0;
}

DbBaseElements::Airport::VFRRoute::VFRRoutePoint::VFRRoutePoint( const Glib::ustring & nm, const Point & pt, int16_t al, label_placement_t lp, char pts, pathcode_t pc, altcode_t ac, bool aa )
	: m_name(nm), m_coord(pt), m_alt(al), m_label_placement(lp), m_ptsym(pts), m_pathcode(pc), m_altcode(ac), m_at_airport(aa)
{
}

DbBaseElements::Airport::VFRRoute::VFRRoutePoint::VFRRoutePoint(void)
	: m_name(), m_coord(), m_alt(0), m_label_placement(label_off), m_ptsym(' '), m_pathcode(pathcode_other), m_altcode(altcode_altspecified), m_at_airport(false)
{
}

Glib::ustring DbBaseElements::Airport::VFRRoute::VFRRoutePoint::get_altitude_string(void) const
{
	char buf[16];
	snprintf(buf, sizeof(buf), "%d'", get_altitude());
	return get_altcode_string() + buf;
}

const Glib::ustring& DbBaseElements::Airport::VFRRoute::VFRRoutePoint::get_altcode_string(altcode_t ac)
{
	switch (ac) {
	case altcode_atorabove:
	{
		static const Glib::ustring r(">=");
		return r;
	}

	case altcode_atorbelow:
	{
		static const Glib::ustring r("<=");
		return r;
	}

	case altcode_assigned:
	{
		static const Glib::ustring r(":=");
		return r;
	}

	case altcode_altspecified:
	{
		static const Glib::ustring r("=");
		return r;
	}

	case altcode_recommendedalt:
	{
		static const Glib::ustring r("~");
		return r;
	}

	default:
	{
		static const Glib::ustring r("?""?");
		return r;
	}
	}
}

const Glib::ustring& DbBaseElements::Airport::VFRRoute::VFRRoutePoint::get_pathcode_string(pathcode_t pc)
{
	switch (pc) {
	case pathcode_arrival:
	{
		static const Glib::ustring r("Arrival");
		return r;
	}

	case pathcode_departure:
	{
		static const Glib::ustring r("Departure");
		return r;
	}

	case pathcode_holding:
	{
		static const Glib::ustring r("Holding");
		return r;
	}

	case pathcode_trafficpattern:
	{
		static const Glib::ustring r("Traffic Pattern");
		return r;
	}

	case pathcode_vfrtransition:
	{
		static const Glib::ustring r("VFR Transition");
		return r;
	}

	case pathcode_other:
	{
		static const Glib::ustring r("Other");
		return r;
	}

	case pathcode_star:
	{
		static const Glib::ustring r("STAR");
		return r;
	}

	case pathcode_sid:
	{
		static const Glib::ustring r("SID");
		return r;
	}

	default:
	{
		static const Glib::ustring r("Invalid");
		return r;
	}
	}
}

FPlanWaypoint::pathcode_t DbBaseElements::Airport::VFRRoute::VFRRoutePoint::get_fplan_pathcode(pathcode_t pc)
{
	switch (pc) {
	case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_arrival:
		return FPlanWaypoint::pathcode_vfrarrival;

	case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_departure:
		return FPlanWaypoint::pathcode_vfrdeparture;

	case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_vfrtransition:
		return FPlanWaypoint::pathcode_vfrtransition;

	case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_star:
		return FPlanWaypoint::pathcode_star;

	case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_sid:
		return FPlanWaypoint::pathcode_sid;

	case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_holding:
	case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_trafficpattern:
	case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_other:
	case AirportsDb::Airport::VFRRoute::VFRRoutePoint::pathcode_invalid:
	default:
		return FPlanWaypoint::pathcode_none;
	}
}

DbBaseElements::Airport::VFRRoute::VFRRoute(const Glib::ustring& nm)
	: m_name(nm), m_minalt(std::numeric_limits<int32_t>::min()), m_maxalt(std::numeric_limits<int32_t>::max())
{
}

DbBaseElements::Airport::VFRRoute::VFRRoute(void)
	: m_name(), m_minalt(std::numeric_limits<int32_t>::min()), m_maxalt(std::numeric_limits<int32_t>::max())
{
}

void DbBaseElements::Airport::VFRRoute::reverse(void)
{
	std::reverse(m_pts.begin(), m_pts.end());
	for(std::vector<VFRRoutePoint>::iterator pi(m_pts.begin()), pe(m_pts.end()); pi != pe; ++pi) {
		switch (pi->get_pathcode()) {
		case VFRRoutePoint::pathcode_arrival:
			pi->set_pathcode(VFRRoutePoint::pathcode_departure);
			break;

		case VFRRoutePoint::pathcode_departure:
			pi->set_pathcode(VFRRoutePoint::pathcode_arrival);
			break;

		case VFRRoutePoint::pathcode_star:
			pi->set_pathcode(VFRRoutePoint::pathcode_sid);
			break;

		case VFRRoutePoint::pathcode_sid:
			pi->set_pathcode(VFRRoutePoint::pathcode_star);
			break;

		default:
			break;
		}
	}
}

void DbBaseElements::Airport::VFRRoute::name(void)
{
	if (m_name.empty())
		m_name = "?";
	if (m_pts.empty())
		return;
	if (m_pts.front().get_pathcode() == VFRRoutePoint::pathcode_arrival) {
		m_name = m_pts.front().get_name() + " ARR";
		return;
	}
	if (m_pts.back().get_pathcode() == VFRRoutePoint::pathcode_departure) {
		m_name = m_pts.back().get_name() + " DEP";
		return;
	}
	if (m_pts.front().get_pathcode() == VFRRoutePoint::pathcode_vfrtransition &&
	    m_pts.back().get_pathcode() == VFRRoutePoint::pathcode_vfrtransition) {
		m_name = m_pts.front().get_name() + "-" + m_pts.back().get_name() + " TRANSIT";
		return;
	}
	if (m_pts.front().get_pathcode() == VFRRoutePoint::pathcode_star) {
		m_name = m_pts.front().get_name() + " STAR";
		return;
	}
	if (m_pts.back().get_pathcode() == VFRRoutePoint::pathcode_sid) {
		m_name = m_pts.back().get_name() + " SID";
		return;
	}
}

double DbBaseElements::Airport::VFRRoute::compute_distance(void) const
{
	double dist(0);
	for (unsigned int i = 1, j = m_pts.size(); i < j; ++i)
		dist += m_pts[i-1].get_coord().spheric_distance_nmi_dbl(m_pts[i].get_coord());
	return dist;
}

DbBaseElements::Airport::PolylineNode::PolylineNode(const Point & pt, const Point & pcpt, const Point & ncpt, uint16_t flags, feature_t f1, feature_t f2)
	: m_point(pt), m_prev_controlpoint(pcpt), m_next_controlpoint(ncpt), m_flags(flags)
{
	m_feature[0] = f1;
	m_feature[1] = f2;
}

unsigned int DbBaseElements::Airport::PolylineNode::blobsize(void) const
{
	unsigned int sz(12);
	if (!is_bezier())
		return sz;
	sz += 8;
	if (!(is_bezier_prev() && is_bezier_next()))
		return sz;
	Point pt(m_point - m_prev_controlpoint + m_point);
	if (pt == m_next_controlpoint)
		return sz;
	sz += 8;
	return sz;
}

uint8_t * DbBaseElements::Airport::PolylineNode::blobencode(uint8_t * ptr) const
{
	uint16_t flags(m_flags & ~flag_bezier_splitcontrol);
	Point ptprevmirror(m_point - m_prev_controlpoint + m_point);
	Point ptctrl(m_next_controlpoint);
	if (!is_bezier_next()) {
		ptctrl = ptprevmirror;
	} else if (is_bezier_prev() && ptprevmirror != m_next_controlpoint) {
		flags |= flag_bezier_splitcontrol;
	}
	ptr[0] = m_point.get_lon() >> 0;
	ptr[1] = m_point.get_lon() >> 8;
	ptr[2] = m_point.get_lon() >> 16;
	ptr[3] = m_point.get_lon() >> 24;
	ptr[4] = m_point.get_lat() >> 0;
	ptr[5] = m_point.get_lat() >> 8;
	ptr[6] = m_point.get_lat() >> 16;
	ptr[7] = m_point.get_lat() >> 24;
	ptr += 8;
	ptr[0] = flags >> 0;
	ptr[1] = flags >> 8;
	ptr += 2;
	ptr[0] = m_feature[0];
	ptr[1] = m_feature[1];
	ptr += 2;
	if (is_bezier()) {
		ptr[0] = ptctrl.get_lon() >> 0;
		ptr[1] = ptctrl.get_lon() >> 8;
		ptr[2] = ptctrl.get_lon() >> 16;
		ptr[3] = ptctrl.get_lon() >> 24;
		ptr[4] = ptctrl.get_lat() >> 0;
		ptr[5] = ptctrl.get_lat() >> 8;
		ptr[6] = ptctrl.get_lat() >> 16;
		ptr[7] = ptctrl.get_lat() >> 24;
		ptr += 8;
		if (flags & flag_bezier_splitcontrol) {
			ptr[0] = m_prev_controlpoint.get_lon() >> 0;
			ptr[1] = m_prev_controlpoint.get_lon() >> 8;
			ptr[2] = m_prev_controlpoint.get_lon() >> 16;
			ptr[3] = m_prev_controlpoint.get_lon() >> 24;
			ptr[4] = m_prev_controlpoint.get_lat() >> 0;
			ptr[5] = m_prev_controlpoint.get_lat() >> 8;
			ptr[6] = m_prev_controlpoint.get_lat() >> 16;
			ptr[7] = m_prev_controlpoint.get_lat() >> 24;
			ptr += 8;
		}
	}
	return ptr;
}

const uint8_t * DbBaseElements::Airport::PolylineNode::blobdecode(const uint8_t *ptr, uint8_t const *end)
{
	m_point = Point();
	m_prev_controlpoint = Point();
	m_next_controlpoint = Point();
	m_flags = 0;
	m_feature[0] = feature_off;
	m_feature[1] = feature_off;
	if (ptr + 12 > end)
		return end;
	uint32_t lon = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
	uint32_t lat = ptr[4] | (ptr[5] << 8) | (ptr[6] << 16) | (ptr[7] << 24);
	ptr += 8;
	m_point = Point(lon, lat);
	m_prev_controlpoint = m_point;
	m_next_controlpoint = m_point;
	uint16_t flags = ptr[0] | (ptr[1] << 8);
	m_flags = flags & ~flag_bezier_splitcontrol;
	ptr += 2;
	m_feature[0] = (feature_t)ptr[0];
	m_feature[1] = (feature_t)ptr[1];
	ptr += 2;
	if (!is_bezier())
		return ptr;
	if (ptr + 8 > end) {
		m_flags &= ~(flag_bezier_prev | flag_bezier_next);
		return end;
	}
	lon = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
	lat = ptr[4] | (ptr[5] << 8) | (ptr[6] << 16) | (ptr[7] << 24);
	ptr += 8;
	m_next_controlpoint = Point(lon, lat);
	if ((flags & flag_bezier_splitcontrol) && (ptr + 8 <= end)) {
		lon = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
		lat = ptr[4] | (ptr[5] << 8) | (ptr[6] << 16) | (ptr[7] << 24);
		ptr += 8;
		m_prev_controlpoint = Point(lon, lat);
	} else {
		m_prev_controlpoint = m_point - m_next_controlpoint + m_point;
	}
	return ptr;
}

bool DbBaseElements::Airport::Polyline::is_concrete(void) const
{
	return (m_surface == "ASP") || (m_surface == "ASPH") || (m_surface == "CON") || (m_surface == "PEM");
}

unsigned int DbBaseElements::Airport::Polyline::blobsize_polygon(void) const
{
	unsigned int sz(2);
	for (const_iterator pi(begin()), pe(end()); pi != pe; ++pi)
		sz += pi->blobsize();
	return sz;
}

unsigned int DbBaseElements::Airport::Polyline::blobsize(void) const
{
	unsigned int sz(6 + m_name.bytes() + m_surface.bytes() + blobsize_polygon());
	return sz;
}

uint8_t * DbBaseElements::Airport::Polyline::blobencode_polygon(uint8_t * ptr) const
{
	uint16_t len = size();
	ptr[0] = len >> 0;
	ptr[1] = len >> 8;
	ptr += 2;
	for (const_iterator pi(begin()), pe(end()); pi != pe; ++pi)
		ptr = pi->blobencode(ptr);
	return ptr;
}

uint8_t *DbBaseElements::Airport::Polyline::blobencode(uint8_t *ptr) const
{
	ptr[0] = m_flags >> 0;
	ptr[1] = m_flags >> 8;
	ptr += 2;
	uint16_t len = m_name.bytes();
	ptr[0] = len >> 0;
	ptr[1] = len >> 8;
	ptr += 2;
	if (len) {
		memcpy(ptr, m_name.data(), len);
		ptr += len;
	}
	len = m_surface.bytes();
	ptr[0] = len >> 0;
	ptr[1] = len >> 8;
	ptr += 2;
	if (len) {
		memcpy(ptr, m_surface.data(), len);
		ptr += len;
	}
	return blobencode_polygon(ptr);
}

const uint8_t * DbBaseElements::Airport::Polyline::blobdecode_polygon(const uint8_t * ptr, uint8_t const * end)
{
	clear();
	if (ptr + 2 > end)
		return end;
	uint16_t len = ptr[0] | (ptr[1] << 8);
	ptr += 2;
	for (; len > 0 && ptr < end; --len) {
		push_back(PolylineNode());
		ptr = back().blobdecode(ptr, end);
	}
	return ptr;
}

const uint8_t *DbBaseElements::Airport::Polyline::blobdecode(const uint8_t *ptr, uint8_t const *end)
{
	m_name = "";
	m_surface = "";
	m_flags = 0;
	clear();
	if (ptr + 4 > end)
		return end;
	m_flags = ptr[0] | (ptr[1] << 8);
	ptr += 2;
	uint16_t len = ptr[0] | (ptr[1] << 8);
	ptr += 2;
	if (len && ptr + len <= end) {
		m_name = std::string((const char *)ptr, len);
		ptr += len;
	}
	if (ptr + 2 > end)
		return end;
	len = ptr[0] | (ptr[1] << 8);
	ptr += 2;
	if (len && ptr + len <= end) {
		m_surface = std::string((const char *)ptr, len);
		ptr += len;
	}
	return blobdecode_polygon(ptr, end);
}

void DbBaseElements::Airport::Polyline::bindblob(sqlite3x::sqlite3_command & cmd, int index) const
{
	unsigned int sz(blobsize());
	uint8_t data[sz];
	uint8_t *d = blobencode(data);
	if (false && d - data != sz)
		*(int *)0 = 0;
	cmd.bind(index, data, sz);
}

void DbBaseElements::Airport::Polyline::getblob(sqlite3x::sqlite3_cursor & cursor, int index)
{
	m_name = "";
	m_surface = "";
	m_flags = 0;
	clear();
	if (cursor.isnull(index))
		return;
	int sz;
	uint8_t const *data((uint8_t const *)cursor.getblob(index, sz));
	uint8_t const *end(data + sz);
	data = blobdecode(data, end);
}

void DbBaseElements::Airport::Polyline::bindblob_polygon(sqlite3x::sqlite3_command & cmd, int index) const
{
	unsigned int sz(blobsize_polygon());
	uint8_t data[sz];
	uint8_t *d = blobencode_polygon(data);
	if (false && d - data != sz) {
		for (const_iterator pi(begin()), pe(end()); pi != pe; ++pi) {
			unsigned sz1(pi->blobsize());
			uint8_t data1[32];
			uint8_t *d1(pi->blobencode(data1));
			if (d1 - data1 != sz1) {
				std::cerr << "blobencode error: expected " << sz1 << " actual " << (d1 - data1) << std::endl;
				*(int *)0 = 0;
			}
		}
	}
	cmd.bind(index, data, sz);
}

void DbBaseElements::Airport::Polyline::getblob_polygon(sqlite3x::sqlite3_cursor & cursor, int index)
{
	clear();
	if (cursor.isnull(index))
		return;
	int sz;
	uint8_t const *data((uint8_t const *)cursor.getblob(index, sz));
	uint8_t const *end(data + sz);
	data = blobdecode_polygon(data, end);
}

#ifdef HAVE_PQXX

void DbBaseElements::Airport::Polyline::bindblob_polygon(pqxx::binarystring& blob) const
{
	unsigned int sz(blobsize_polygon());
	uint8_t data[sz];
	uint8_t *d = blobencode_polygon(data);
	if (false && d - data != sz) {
		for (const_iterator pi(begin()), pe(end()); pi != pe; ++pi) {
			unsigned sz1(pi->blobsize());
			uint8_t data1[32];
			uint8_t *d1(pi->blobencode(data1));
			if (d1 - data1 != sz1) {
				std::cerr << "blobencode error: expected " << sz1 << " actual " << (d1 - data1) << std::endl;
				*(int *)0 = 0;
			}
		}
	}
	pqxx::binarystring blob1(data, sz);
	blob.swap(blob1);
}

void DbBaseElements::Airport::Polyline::getblob_polygon(const pqxx::binarystring& blob)
{
	clear();
	uint8_t const *data(blob.data());
	uint8_t const *end(data + blob.size());
	data = blobdecode_polygon(data, end);
}

#endif

DbBaseElements::Airport::Polyline::operator PolygonHole(void) const
{
	static const int bdiv = 6;
	static const int ndiv = 1 << bdiv;
	PolygonHole ph;
	if (empty())
		return ph;
	PolygonSimple p;
	bool first = true;
	const PolylineNode *pnfirst = 0;
	for (unsigned int i = 0;; ++i) {
		const PolylineNode& pn1(operator[](i));
		if (!pnfirst) {
			pnfirst = &pn1;
			p.push_back(pn1.get_point());
			continue;
		}
		const PolylineNode& pn0(operator[](i - 1));
		if (pn0.is_bezier_next() || pn1.is_bezier_prev()) {
			Point p0(pn0.get_point());
			Point p1(pn0.get_next_controlpoint() - p0);
			Point p2(pn1.get_prev_controlpoint() - p0);
			Point p3(pn1.get_point() - p0);
			for (int t = 1; t < ndiv; ++t) {
				int tm = ndiv - t;
				int m1 = 3 * t * tm * tm;
				int m2 = 3 * t * t * tm;
				int m3 = t * t * t;
				int64_t lon(m1 * (int64_t)p1.get_lon() + m2 * (int64_t)p2.get_lon() + m3 * (int64_t)p3.get_lon());
				int64_t lat(m1 * (int64_t)p1.get_lat() + m2 * (int64_t)p2.get_lat() + m3 * (int64_t)p3.get_lat());
				p.push_back(Point(lon >> (3 * bdiv), lat >> (3 * bdiv)) + p0);
			}
		}
		p.push_back(pn1.get_point());
		if (pn1.is_closepath() && (pn0.is_bezier_next() || pn1.is_bezier_prev())) {
			Point p0(pn1.get_point());
			Point p1(pn1.get_next_controlpoint() - p0);
			Point p2(pnfirst->get_prev_controlpoint() - p0);
			Point p3(pnfirst->get_point() - p0);
			for (int t = 1; t < ndiv; ++t) {
				int tm = ndiv - t;
				int m1 = 3 * t * tm * tm;
				int m2 = 3 * t * t * tm;
				int m3 = t * t * t;
				int64_t lon(m1 * (int64_t)p1.get_lon() + m2 * (int64_t)p2.get_lon() + m3 * (int64_t)p3.get_lon());
				int64_t lat(m1 * (int64_t)p1.get_lat() + m2 * (int64_t)p2.get_lat() + m3 * (int64_t)p3.get_lat());
				p.push_back(Point(lon >> (3 * bdiv), lat >> (3 * bdiv)) + p0);
			}
		}
		if (pn1.is_closepath() || pn1.is_endpath()) {
			pnfirst = 0;
			if (first)
				ph.set_exterior(p);
			else
				ph.add_interior(p);
			first = false;
			p.clear();
		}
	}
}

Rect DbBaseElements::Airport::Polyline::get_bbox(void) const
{
	if (empty())
		return Rect();
	Point poffs(begin()->get_point());
	Point sw(0, 0), ne(0, 0);
	for (const_iterator pi(begin()), pe(end()); pi != pe; ++pi) {
		Point p(pi->get_point() - poffs);
		sw = Point(std::min(sw.get_lon(), p.get_lon()), std::min(sw.get_lat(), p.get_lat()));
		ne = Point(std::max(ne.get_lon(), p.get_lon()), std::max(ne.get_lat(), p.get_lat()));
		if (!pi->is_bezier())
			continue;
		// FIXME: this is pessimistic, should actually find max of bezier curve
		p = pi->get_point() - poffs;
		sw = Point(std::min(sw.get_lon(), p.get_lon()), std::min(sw.get_lat(), p.get_lat()));
		ne = Point(std::max(ne.get_lon(), p.get_lon()), std::max(ne.get_lat(), p.get_lat()));
	}
	return Rect(sw + poffs, ne + poffs);
}

DbBaseElements::Airport::PointFeature::PointFeature(feature_t ft, const Point & pt, uint16_t hdg, int16_t elev, unsigned int subtype, int attr1, int attr2, const Glib::ustring & name, const Glib::ustring & rwyident)
	: m_feature(ft), m_coord(pt), m_hdg(hdg), m_elev(elev), m_subtype(subtype), m_attr1(attr1), m_attr2(attr2), m_name(name), m_rwyident(rwyident)
{
}

constexpr double DbBaseElements::Airport::FASNavigate::to_deg;
constexpr double DbBaseElements::Airport::FASNavigate::from_deg;
constexpr double DbBaseElements::Airport::FASNavigate::to_rad;
constexpr double DbBaseElements::Airport::FASNavigate::from_rad;
const uint32_t DbBaseElements::Airport::FASNavigate::standard_glidepath;
const uint32_t DbBaseElements::Airport::FASNavigate::standard_glidepathdefl;
const uint32_t DbBaseElements::Airport::FASNavigate::standard_coursedefl;

DbBaseElements::Airport::FASNavigate::FASNavigate(void)
	: m_course(0), m_coursedefl(standard_coursedefl), m_glidepath(standard_glidepath), m_glidepathdefl(standard_glidepathdefl),
	  m_thralt(std::numeric_limits<double>::quiet_NaN())
{
	m_ftp.set_invalid();
	m_fpap.set_invalid();
}

void DbBaseElements::Airport::FASNavigate::set_course(const Point& ftp, const Point& fpap, double fpapoffset_nmi, double cwidth_meter)
{
	m_ftp.set_invalid();
	m_fpap.set_invalid();
	m_thralt = std::numeric_limits<double>::quiet_NaN();
	if (ftp.is_invalid() || fpap.is_invalid() || std::isnan(fpapoffset_nmi))
		return;
	double dist(ftp.spheric_distance_nmi_dbl(fpap));
	if (dist < 0.01)
		return;
	dist += fpapoffset_nmi;
	if (dist < 0.01)
		return;
	double crs(ftp.spheric_true_course_dbl(fpap));
	m_ftp = ftp;
	m_fpap = fpap.spheric_course_distance_nmi(crs, fpapoffset_nmi);
	m_course = Point::round<uint32_t,double>(crs * from_deg);
	if (!std::isnan(cwidth_meter)) {
		dist *= 1000.0 / Point::km_to_nmi_dbl;
		m_coursedefl = Point::round<uint32_t,double>(atan2(cwidth_meter, dist) * from_rad);
	}
}

void DbBaseElements::Airport::FASNavigate::navigate(const Point& pos, double truealt, double& loc, double& gs, bool& fromflag)
{
	loc = gs = std::numeric_limits<double>::quiet_NaN();
	fromflag = false;
	if (!is_valid() || pos.is_invalid())
		return;
	double dist(m_ftp.spheric_distance_nmi_dbl(pos));
	uint32_t crs(Point::round<uint32_t,double>(pos.spheric_true_course_dbl(m_fpap) * from_deg));
	int32_t crsdiff(crs - m_course);
	bool flg(abs(crsdiff) > (1 << 30));
	fromflag = flg;
	if (flg)
		crsdiff = (1 << 31) - crsdiff;
	loc = crsdiff / (double)m_coursedefl;
	if (std::isnan(truealt) || std::isnan(m_thralt))
		return;
	dist *= 1000.0 / Point::km_to_nmi_dbl * Point::m_to_ft_dbl;
	gs = (atan2(truealt - m_thralt, dist) - m_glidepath * to_rad) / (m_glidepathdefl * to_rad);
}

constexpr double DbBaseElements::Airport::MinimalFAS::from_coord;
constexpr double DbBaseElements::Airport::MinimalFAS::to_coord;

DbBaseElements::Airport::MinimalFAS::MinimalFAS()
	: m_operationtype(0), m_runway(0), m_route(0), m_refpathdatasel(0), m_ftpheight(0),  m_approachtch(0),
	  m_glidepathangle(0), m_coursewidth(0), m_dlengthoffset(0), m_hal(0), m_val(0)
{
	memset(m_refpathident, 0, sizeof(m_refpathident));
	m_ftp.set_invalid();
	m_fpap.set_invalid();
}

char DbBaseElements::Airport::MinimalFAS::get_runwayletter_char(void) const
{
	static const char rwyletter[16] = {
		' ', 'R', 'C', 'L',
		'?', '?', '?', '?',
		'?', '?', '?', '?',
		'?', '?', '?', '?'
	};
	return rwyletter[get_runwayletter()];
}

void DbBaseElements::Airport::MinimalFAS::set_runwayletter_char(char l)
{
	switch (l) {
	case ' ':
		set_runwayletter(0);
		return;

	case 'R':
	case 'r':
		set_runwayletter(1);
		return;

	case 'C':
	case 'c':
		set_runwayletter(2);
		return;

	case 'L':
	case 'l':
		set_runwayletter(3);
		return;

	default:
		set_runwayletter(15);
		return;
	}
}

char DbBaseElements::Airport::MinimalFAS::get_routeindicator_char(void) const
{
	uint8_t ri(get_routeindicator());
	if (!ri)
		return ' ';
	return ('A' & 0xC0) | ri;
}

void DbBaseElements::Airport::MinimalFAS::set_routeindicator_char(char ri)
{
	if (std::isalpha(ri)) {
		set_routeindicator(ri);
		return;
	}
	set_routeindicator(0);
}

Glib::ustring DbBaseElements::Airport::MinimalFAS::get_referencepathident(void) const
{
	Glib::ustring r;
	for (unsigned int i = sizeof(m_refpathident)/sizeof(m_refpathident[0]); i > 0;) {
		--i;
		if (!m_refpathident[i])
			break;
		if (m_refpathident[i] < 0x20)
			r.push_back((char)(m_refpathident[i] | ('A' & 0xC0)));
		else
			r.push_back((char)(m_refpathident[i]));
	}
	return r;
}

void DbBaseElements::Airport::MinimalFAS::set_referencepathident(const Glib::ustring& rid)
{
	Glib::ustring id(rid.uppercase());
	unsigned int i(sizeof(m_refpathident)/sizeof(m_refpathident[0]));
	for (Glib::ustring::const_iterator ii(id.begin()), ie(id.end()); ii != ie; ++ii) {
		char ch(*ii);
		if (ch == ' ')
			break;
		if (i == 0)
			break;
		m_refpathident[--i] = ch & 0x3F;
	}
	while (i > 0)
		m_refpathident[--i] = 0;
}


double DbBaseElements::Airport::MinimalFAS::get_approach_tch_meter(void) const
{
	if (m_approachtch & 0x8000)
		return (m_approachtch & 0x7FFF) * 0.05;
	return m_approachtch * (0.1 * Point::ft_to_m_dbl);
}

void DbBaseElements::Airport::MinimalFAS::set_approach_tch_meter(double atch)
{
	m_approachtch = Point::round<uint16_t,double>(std::min(std::max(atch * 20.0, 0.0), 32767.0)) | 0x8000;
}

double DbBaseElements::Airport::MinimalFAS::get_approach_tch_ft(void) const
{
	if (m_approachtch & 0x8000)
		return (m_approachtch & 0x7FFF) * (0.05 * Point::m_to_ft_dbl);
	return m_approachtch * 0.1;
}

void DbBaseElements::Airport::MinimalFAS::set_approach_tch_ft(double atch)
{
	m_approachtch = Point::round<uint16_t,double>(std::min(std::max(atch * 10.0, 0.0), 32767.0)) & ~0x8000;
}

double DbBaseElements::Airport::MinimalFAS::get_dlengthoffset_meter(void) const
{
	uint8_t dl(get_dlengthoffset());
	if (dl == 0xFF)
		return std::numeric_limits<double>::quiet_NaN();
	return dl * 8.0;
}

void DbBaseElements::Airport::MinimalFAS::set_dlengthoffset_meter(double l)
{
	if (std::isnan(l)) {
		set_dlengthoffset(0xFF);
		return;
	}
	set_dlengthoffset(Point::round<uint8_t,double>(l * 0.125));
}

DbBaseElements::Airport::FASNavigate DbBaseElements::Airport::MinimalFAS::navigate(void) const
{
	FASNavigate fas;
	if (!is_valid())
		return fas;
	fas.set_ident(get_referencepathident());
	fas.set_course(get_ftp(), get_fpap(), 1000.0 * Point::ft_to_m_dbl * 0.001 * Point::km_to_nmi_dbl,
		       get_coursewidth_meter());
	fas.set_thralt(get_ftp_height_meter() * Point::m_to_ft_dbl + get_approach_tch_ft());
	fas.set_glidepath_deg(get_glidepathangle_deg());
	return fas;
}

DbBaseElements::Airport::FinalApproachSegment::FinalApproachSegment()
{
	memset(m_airportid, 0, sizeof(m_airportid));
}

Glib::ustring DbBaseElements::Airport::FinalApproachSegment::get_airportid(void) const
{
	Glib::ustring r;
	for (unsigned int i = sizeof(m_airportid)/sizeof(m_airportid[0]); i > 0;) {
		--i;
		if (!m_airportid[i])
			break;
		if (m_airportid[i] < 0x20)
			r.push_back((char)(m_airportid[i] | ('A' & 0xC0)));
		else
			r.push_back((char)(m_airportid[i]));
	}
	return r;
}

void DbBaseElements::Airport::FinalApproachSegment::set_airportid(const Glib::ustring& arptid)
{
	Glib::ustring id(arptid.uppercase());
	unsigned int i(sizeof(m_airportid)/sizeof(m_airportid[0]));
	for (Glib::ustring::const_iterator ii(id.begin()), ie(id.end()); ii != ie; ++ii) {
		char ch(*ii);
		if (ch == ' ')
			break;
		if (i == 0)
			break;
		m_airportid[--i] = ch & 0x3F;
	}
	while (i > 0)
		m_airportid[--i] = 0;
}

const uint32_t DbBaseElements::Airport::FinalApproachSegment::crc_poly;
const uint32_t DbBaseElements::Airport::FinalApproachSegment::crc_rev_poly;

uint32_t DbBaseElements::Airport::FinalApproachSegment::crc(const uint8_t *data, unsigned int length, uint32_t initial)
{
	for (; length > 0; --length, ++data) {
		uint8_t b(initial ^ *data);
		initial >>= 8;
		initial ^= crc_table[b];
	}
	return initial;
}

bool DbBaseElements::Airport::FinalApproachSegment::decode(const fas_datablock_t data)
{
	if (crc(data, sizeof(fas_datablock_t)/sizeof(data[0])))
		return false;
	m_operationtype = data[0];
	for (unsigned int i = 0; i < sizeof(m_airportid)/sizeof(m_airportid[0]); ++i)
		m_airportid[i] = data[i + 1];
	m_runway = data[5];
	m_route = data[6];
	m_refpathdatasel = data[7];
	for (unsigned int i = 0; i < sizeof(m_refpathident)/sizeof(m_refpathident[0]); ++i)
		m_refpathident[i] = data[i + 8];
	{
		int32_t lat(data[12] | (((uint32_t)data[13]) << 8) | (((uint32_t)data[14]) << 16) | (((uint32_t)data[15]) << 24));
		int32_t lon(data[16] | (((uint32_t)data[17]) << 8) | (((uint32_t)data[18]) << 16) | (((uint32_t)data[19]) << 24));
		m_ftp = Point(Point::round<Point::coord_t,double>(lon * to_coord),
			      Point::round<Point::coord_t,double>(lat * to_coord));
	}
	m_ftpheight = data[20] | (((uint16_t)data[21]) << 8);
	{
		int32_t lat(data[22] | (((uint32_t)data[23]) << 8) | (((uint32_t)data[24]) << 16));
		int32_t lon(data[25] | (((uint32_t)data[26]) << 8) | (((uint32_t)data[27]) << 16));
		lat |= -(lat & (1 << 23));
		lon |= -(lon & (1 << 23));
		m_fpap = m_ftp + Point(Point::round<Point::coord_t,double>(lon * to_coord),
				       Point::round<Point::coord_t,double>(lat * to_coord));
	}
	m_approachtch = data[28] | (((uint16_t)data[29]) << 8);
	m_glidepathangle = data[30] | (((uint16_t)data[31]) << 8);
	m_coursewidth = data[32];
	m_dlengthoffset = data[33];
	m_hal = data[34];
	m_val = data[35];
	return true;
}

void DbBaseElements::Airport::FinalApproachSegment::encode(fas_datablock_t data)
{
	data[0] = m_operationtype;
	for (unsigned int i = 0; i < sizeof(m_airportid)/sizeof(m_airportid[0]); ++i)
		data[i + 1] = m_airportid[i];
	data[5] = m_runway;
	data[6] = m_route;
	data[7] = m_refpathdatasel;
	for (unsigned int i = 0; i < sizeof(m_refpathident)/sizeof(m_refpathident[0]); ++i)
		data[i + 8] = m_refpathident[i];
	{
		int32_t lat(Point::round<int32_t,double>(m_ftp.get_lat() * from_coord));
		int32_t lon(Point::round<int32_t,double>(m_ftp.get_lon() * from_coord));
		data[12] = lat;
		data[13] = lat >> 8;
		data[14] = lat >> 16;
		data[15] = lat >> 24;
		data[16] = lon;
		data[17] = lon >> 8;
		data[18] = lon >> 16;
		data[19] = lon >> 24;
	}
	data[20] = m_ftpheight;
	data[21] = m_ftpheight >> 8;
	{
		Point pt(m_fpap - m_ftp);
		int32_t lat(Point::round<int32_t,double>(pt.get_lat() * from_coord));
		int32_t lon(Point::round<int32_t,double>(pt.get_lon() * from_coord));
		data[22] = lat;
		data[23] = lat >> 8;
		data[24] = lat >> 16;
		data[25] = lon;
		data[26] = lon >> 8;
		data[27] = lon >> 16;
	}
	data[28] = m_approachtch;
	data[29] = m_approachtch >> 8;
	data[30] = m_glidepathangle;
	data[31] = m_glidepathangle >> 8;
	data[32] = m_coursewidth;
	data[33] = m_dlengthoffset;
	data[34] = m_hal;
	data[35] = m_val;
	// append CRC
	uint32_t c(crc(data, sizeof(fas_datablock_t)/sizeof(data[0])-4));
	for (unsigned int i = sizeof(fas_datablock_t)/sizeof(data[0]) - 4; i < sizeof(fas_datablock_t)/sizeof(data[0]); ++i, c >>= 8)
		data[i] = c;
}

DbBaseElements::Airport::FASNavigate DbBaseElements::Airport::FinalApproachSegment::navigate(void) const
{
	FASNavigate fas(MinimalFAS::navigate());
	fas.set_airport(get_airportid());
	return fas;
}

const unsigned int DbBaseElements::Airport::subtables_none;
const unsigned int DbBaseElements::Airport::subtables_runways;
const unsigned int DbBaseElements::Airport::subtables_helipads;
const unsigned int DbBaseElements::Airport::subtables_comms;
const unsigned int DbBaseElements::Airport::subtables_vfrroutes;
const unsigned int DbBaseElements::Airport::subtables_linefeatures;
const unsigned int DbBaseElements::Airport::subtables_pointfeatures;
const unsigned int DbBaseElements::Airport::subtables_fas;
const unsigned int DbBaseElements::Airport::subtables_all;

const uint8_t DbBaseElements::Airport::flightrules_arr_vfr;
const uint8_t DbBaseElements::Airport::flightrules_arr_ifr;
const uint8_t DbBaseElements::Airport::flightrules_dep_vfr;
const uint8_t DbBaseElements::Airport::flightrules_dep_ifr;

const char *DbBaseElements::Airport::db_query_string = "airports.ID AS ID,0 AS \"TABLE\",airports.SRCID AS SRCID,airports.ICAO,airports.NAME,airports.AUTHORITY,airports.VFRRMK,airports.LAT,airports.LON,airports.SWLAT,airports.SWLON,airports.NELAT,airports.NELON,airports.ELEVATION,airports.TYPECODE,airports.FRULES,airports.LABELPLACEMENT,airports.MODIFIED";
const char *DbBaseElements::Airport::db_aux_query_string = "aux.airports.ID AS ID,1 AS \"TABLE\",aux.airports.SRCID AS SRCID,aux.airports.ICAO,aux.airports.NAME,aux.airports.AUTHORITY,aux.airports.VFRRMK,aux.airports.LAT,aux.airports.LON,aux.airports.SWLAT,aux.airports.SWLON,aux.airports.NELAT,aux.airports.NELON,aux.airports.ELEVATION,aux.airports.TYPECODE,aux.airports.FRULES,aux.airports.LABELPLACEMENT,aux.airports.MODIFIED";
const char *DbBaseElements::Airport::db_text_fields[] = { "SRCID", "ICAO", "NAME", "VFRRMK", 0 };
const char *DbBaseElements::Airport::db_time_fields[] = { "MODIFIED", 0 };

DbBaseElements::Airport::Airport(void)
	: m_rwy(), m_comm(), m_vfrrte(), m_vfrrmk(),
	  m_bbox(), m_elev(0), m_typecode(0), m_flightrules(0), m_subtables(subtables_all)
{
}

void DbBaseElements::Airport::load_subtables(sqlite3x::sqlite3_connection & db, unsigned int loadsubtables)
{
	if (false)
		std::cerr << "Airport::load_subtables: loadsubtables 0x" << std::hex << loadsubtables << " m_subtables 0x" << std::hex << m_subtables << std::endl;
	loadsubtables &= subtables_all & ~m_subtables;
	if (!loadsubtables)
		return;
	std::string tablepfx;
	switch (m_table) {
		case 0:
			break;
		case 1:
			tablepfx = "aux.";
			break;
		default:
			throw std::runtime_error("invalid table ID");
	}
	if (false)
		std::cerr << "Airport::load_subtables ID " << m_id << " table " << (int)m_table << ' ' << tablepfx << "airportrunways" << std::endl;
	if (loadsubtables & subtables_runways) {
		m_rwy.clear();
		sqlite3x::sqlite3_command cmd(db, "SELECT IDENTHE,IDENTLE,LENGTH,WIDTH,SURFACE,HELAT,HELON,LELAT,LELON,HETDA,HELDA,HEDISP,HEHDG,HEELEV,LETDA,LELDA,LEDISP,LEHDG,LEELEV,FLAGS,HELIGHT0,HELIGHT1,HELIGHT2,HELIGHT3,HELIGHT4,HELIGHT5,HELIGHT6,HELIGHT7,LELIGHT0,LELIGHT1,LELIGHT2,LELIGHT3,LELIGHT4,LELIGHT5,LELIGHT6,LELIGHT7 FROM " + tablepfx + "airportrunways WHERE ARPTID=? ORDER BY ID;");
		cmd.bind(1, (long long int)m_id);
		sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
		while (cursor.step()) {
			Runway rwy(cursor.getstring(0), cursor.getstring(1), cursor.getint(2), cursor.getint(3), cursor.getstring(4),
				   Point(cursor.getint(6), cursor.getint(5)), Point(cursor.getint(8), cursor.getint(7)),
					 cursor.getint(9), cursor.getint(10), cursor.getint(11), cursor.getint(12), cursor.getint(13),
							 cursor.getint(14), cursor.getint(15), cursor.getint(16), cursor.getint(17), cursor.getint(18), cursor.getint(19));
			for (unsigned int i = 0; i < 8; ++i) {
				rwy.set_he_light(i, cursor.getint(20 + i));
				rwy.set_le_light(i, cursor.getint(28 + i));
			}
			add_rwy(rwy);
		}
		m_subtables |= subtables_runways;
	}
	if (loadsubtables & subtables_helipads) {
		m_helipad.clear();
		sqlite3x::sqlite3_command cmd(db, "SELECT IDENT,LENGTH,WIDTH,SURFACE,LAT,LON,HDG,ELEV,FLAGS FROM " + tablepfx + "airporthelipads WHERE ARPTID=? ORDER BY ID;");
		cmd.bind(1, (long long int)m_id);
		sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
		while (cursor.step()) {
			Helipad hp(cursor.getstring(0), cursor.getint(1), cursor.getint(2), cursor.getstring(3),
				    Point(cursor.getint(5), cursor.getint(4)), cursor.getint(6), cursor.getint(7), cursor.getint(8));
			add_helipad(hp);
		}
		m_subtables |= subtables_helipads;
	}
	if (loadsubtables & subtables_comms) {
		m_comm.clear();
		sqlite3x::sqlite3_command cmd(db, "SELECT NAME,SECTOR,OPRHOURS,FREQ0,FREQ1,FREQ2,FREQ3,FREQ4 FROM " + tablepfx + "airportcomms WHERE ARPTID=? ORDER BY ID;");
		cmd.bind(1, (long long int)m_id);
		sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
		while (cursor.step()) {
			add_comm(Comm(cursor.getstring(0), cursor.getstring(1), cursor.getstring(2), cursor.getint(3), cursor.getint(4), cursor.getint(5), cursor.getint(6), cursor.getint(7)));
		}
		m_subtables |= subtables_comms;
	}
	if (loadsubtables & subtables_vfrroutes) {
		m_vfrrte.clear();
		typedef std::map<int,unsigned int> idmap_t;
		idmap_t idmap;
		{
			sqlite3x::sqlite3_command cmd(db, "SELECT ID,NAME,MINALT,MAXALT FROM " + tablepfx + "airportvfrroutes WHERE ARPTID=? ORDER BY ID;");
			cmd.bind(1, (long long int)m_id);
			sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
			while (cursor.step()) {
				VFRRoute rte(cursor.getstring(1));
				if (!cursor.isnull(2))
					rte.set_minalt(cursor.getint(2));
				if (!cursor.isnull(3))
					rte.set_maxalt(cursor.getint(3));
				idmap[cursor.getint(0)] = get_nrvfrrte();
				add_vfrrte(rte);
			}
		}
		{
			sqlite3x::sqlite3_command cmd(db, "SELECT ROUTEID,NAME,LAT,LON,ALT,ALTCODE,PATHCODE,SYM,ATAIRPORT,LABELPLACEMENT FROM " + tablepfx + "airportvfrroutepoints WHERE ARPTID=? ORDER BY ROUTEID,ID;");
			cmd.bind(1, (long long int)m_id);
			sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
			while (cursor.step()) {
				VFRRoute::VFRRoutePoint
					rtept(cursor.getstring(1), Point(cursor.getint(3), cursor.getint(2)),
					      cursor.getint(4), (label_placement_t)cursor.getint(9), cursor.getint(7),
					      (VFRRoute::VFRRoutePoint::pathcode_t)cursor.getint(6),
					      (VFRRoute::VFRRoutePoint::altcode_t)cursor.getint(5), cursor.getint(8));
				idmap_t::const_iterator i(idmap.find(cursor.getint(0)));
				if (i == idmap.end() || i->second >= get_nrvfrrte())
					throw std::runtime_error("airports db: inconsistent airportvfrroutes/airportvfrroutepoints");
				VFRRoute& rte(get_vfrrte(i->second));
				rte.add_point(rtept);
			}
		}
		m_subtables |= subtables_vfrroutes;
	}
	if (loadsubtables & subtables_linefeatures) {
		m_linefeature.clear();
		sqlite3x::sqlite3_command cmd(db, "SELECT NAME,SURFACE,FLAGS,POLY FROM " + tablepfx + "airportlinefeatures WHERE ARPTID=? ORDER BY ID;");
		cmd.bind(1, (long long int)m_id);
		sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
		while (cursor.step()) {
			Polyline pl;
			pl.set_name(cursor.getstring(0));
			pl.set_surface(cursor.getstring(1));
			pl.set_flags(cursor.getint(2));
			pl.getblob_polygon(cursor, 3);
			add_linefeature(pl);
		}
		m_subtables |= subtables_linefeatures;
	}
	if (loadsubtables & subtables_pointfeatures) {
		m_pointfeature.clear();
		sqlite3x::sqlite3_command cmd(db, "SELECT FEATURE,LAT,LON,HDG,ELEV,SUBTYPE,ATTR1,ATTR2,NAME,RWYIDENT FROM " + tablepfx + "airportpointfeatures WHERE ARPTID=? ORDER BY ID;");
		cmd.bind(1, (long long int)m_id);
		sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
		while (cursor.step()) {
			PointFeature pf((PointFeature::feature_t)cursor.getint(0), Point(cursor.getint(2), cursor.getint(1)),
					cursor.getint(3), cursor.getint(4), cursor.getint(5), cursor.getint(6), cursor.getint(7),
					cursor.getstring(8), cursor.getstring(9));
			add_pointfeature(pf);
		}
		m_subtables |= subtables_pointfeatures;
	}
	if (loadsubtables & subtables_fas) {
		m_fas.clear();
		sqlite3x::sqlite3_command cmd(db, "SELECT REFPATHID,FTPLAT,FTPLON,FPAPLAT,FPAPLON,OPTYP,RWY,RTE,REFPATHDS,FTPHEIGHT,APCHTCH,GPA,CWIDTH,DLENOFFS,HAL,VAL FROM " + tablepfx + "airportfas WHERE ARPTID=? ORDER BY ID;");
		cmd.bind(1, (long long int)m_id);
		sqlite3x::sqlite3_cursor cursor(cmd.executecursor());
		while (cursor.step()) {
			MinimalFAS fas;
			fas.set_referencepathident(cursor.getstring(0));
			fas.set_ftp(Point(cursor.getint(2), cursor.getint(1)));
			fas.set_fpap(Point(cursor.getint(4), cursor.getint(3)));
			fas.set_optyp(cursor.getint(5));
			fas.set_rwy(cursor.getint(6));
			fas.set_rte(cursor.getint(7));
			fas.set_referencepathdataselector(cursor.getint(8));
			fas.set_ftp_height(cursor.getint(9));
			fas.set_approach_tch(cursor.getint(10));
			fas.set_glidepathangle(cursor.getint(11));
			fas.set_coursewidth(cursor.getint(12));
			fas.set_dlengthoffset(cursor.getint(13));
			fas.set_hal(cursor.getint(14));
			fas.set_val(cursor.getint(15));
			add_finalapproachsegment(fas);
		}
		m_subtables |= subtables_fas;
	}
}

void DbBaseElements::Airport::load( sqlite3x::sqlite3_cursor & cursor, sqlite3x::sqlite3_connection & db, unsigned int loadsubtables )
{
	m_id = 0;
	m_table = table_main;
	m_typecode = 0;
	m_subtables = subtables_none;
	if (!cursor.step())
		return;
	m_id = cursor.getint(0);
	m_table = (table_t)cursor.getint(1);
	m_sourceid = cursor.getstring(2);
	m_icao = cursor.getstring(3);
	m_name = cursor.getstring(4);
	m_authority = cursor.getstring(5);
	m_vfrrmk = cursor.getstring(6);
	m_coord = Point(cursor.getint(8), cursor.getint(7));
	m_bbox = Rect(Point(cursor.getint(10), cursor.getint(9)), Point(cursor.getint(12), cursor.getint(11)));
	m_elev = cursor.getint(13);
	m_typecode = cursor.getint(14);
	m_flightrules = cursor.getint(15);
	m_label_placement = (label_placement_t)cursor.getint(16);
	m_modtime = cursor.getint(17);
	if (!loadsubtables)
		return;
	load_subtables(db, loadsubtables);
}

void DbBaseElements::Airport::update_index(sqlite3x::sqlite3_connection & db, bool rtree, bool aux_rtree)
{
	if (m_table != table_main || !m_id)
		return;
	sqlite3x::sqlite3_transaction tran(db);
	{
		sqlite3x::sqlite3_command cmd(db, "UPDATE airports SET TILE=?2 WHERE ID=?1;");
		cmd.bind(1, (long long int)m_id);
		cmd.bind(2, (int)TileNumber::to_tilenumber(m_bbox));
		cmd.executenonquery();
	}
	if (rtree) {
		sqlite3x::sqlite3_command cmd(db, "UPDATE airports_rtree SET SWLAT=?,NELAT=?,SWLON=?,NELON=? WHERE ID=?;");
		cmd.bind(1, m_bbox.get_south());
		cmd.bind(2, m_bbox.get_north());
		cmd.bind(3, m_bbox.get_west());
		cmd.bind(4, (long long int)m_bbox.get_east_unwrapped());
		cmd.bind(5, (long long int)m_id);
		cmd.executenonquery();
	}
	tran.commit();
}

void DbBaseElements::Airport::save(sqlite3x::sqlite3_connection & db, bool rtree, bool aux_rtree)
{
	sqlite3x::sqlite3_transaction tran(db);
	if (m_table == table_aux) {
		sqlite3x::sqlite3_command cmd(db, "INSERT OR REPLACE INTO airports_deleted (SRCID) VALUES (?);");
		cmd.bind(1, m_sourceid);
		cmd.executenonquery();
		m_table = table_main;
		m_id = 0;
	}
	if (!m_id) {
		m_id = 1;
		{
			sqlite3x::sqlite3_command cmd(db, "SELECT MAX(ID) FROM airports;");
			sqlite3x::sqlite3_cursor cursor = cmd.executecursor();
			if (cursor.step())
				m_id = cursor.getint(0) + 1;
		}
		{
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO airports (ID,SRCID) VALUES(?,?);");
			cmd.bind(1, (long long int)m_id);
			cmd.bind(2, m_sourceid);
			cmd.executenonquery();
		}
		if (rtree) {
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO airports_rtree (ID,SWLAT,NELAT,SWLON,NELON) VALUES(?,?,?,?,?);");
			cmd.bind(1, (long long int)m_id);
			cmd.bind(2, m_bbox.get_south());
			cmd.bind(3, m_bbox.get_north());
			cmd.bind(4, m_bbox.get_west());
			cmd.bind(5, (long long int)m_bbox.get_east_unwrapped());
			cmd.executenonquery();
		}
	}
	// Update Airport Record
	{
		sqlite3x::sqlite3_command cmd(db, "UPDATE airports SET SRCID=?,ICAO=?,NAME=?,AUTHORITY=?,VFRRMK=?,LAT=?,LON=?,SWLAT=?,SWLON=?,NELAT=?,NELON=?,ELEVATION=?,TYPECODE=?,FRULES=?,LABELPLACEMENT=?,MODIFIED=?,TILE=? WHERE ID=?;");
		cmd.bind(1, m_sourceid);
		cmd.bind(2, m_icao);
		cmd.bind(3, m_name);
		cmd.bind(4, m_authority);
		cmd.bind(5, m_vfrrmk);
		cmd.bind(6, m_coord.get_lat());
		cmd.bind(7, m_coord.get_lon());
		cmd.bind(8, m_bbox.get_south());
		cmd.bind(9, m_bbox.get_west());
		cmd.bind(10, m_bbox.get_north());
		cmd.bind(11, (long long int)m_bbox.get_east_unwrapped());
		cmd.bind(12, m_elev);
		cmd.bind(13, m_typecode);
		cmd.bind(14, m_flightrules);
		cmd.bind(15, m_label_placement);
		cmd.bind(16, (long long int)m_modtime);
		cmd.bind(17, (int)TileNumber::to_tilenumber(m_bbox));
		cmd.bind(18, (long long int)m_id);
		cmd.executenonquery();
	}
	if (m_subtables & subtables_runways) {
		// Delete existing Runways
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airportrunways WHERE ARPTID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		// Write Runways
		for (unsigned int i = 0; i < get_nrrwy(); i++) {
			const Runway& rwy(get_rwy(i));
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO airportrunways (ARPTID,ID,IDENTHE,IDENTLE,LENGTH,WIDTH,SURFACE,HELAT,HELON,LELAT,LELON,HETDA,HELDA,HEDISP,HEHDG,HEELEV,LETDA,LELDA,LEDISP,LEHDG,LEELEV,FLAGS,HELIGHT0,HELIGHT1,HELIGHT2,HELIGHT3,HELIGHT4,HELIGHT5,HELIGHT6,HELIGHT7,LELIGHT0,LELIGHT1,LELIGHT2,LELIGHT3,LELIGHT4,LELIGHT5,LELIGHT6,LELIGHT7) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);");
			cmd.bind(1, (long long int)m_id);
			cmd.bind(2, (int)i);
			cmd.bind(3, rwy.get_ident_he());
			cmd.bind(4, rwy.get_ident_le());
			cmd.bind(5, rwy.get_length());
			cmd.bind(6, rwy.get_width());
			cmd.bind(7, rwy.get_surface());
			cmd.bind(8, rwy.get_he_coord().get_lat());
			cmd.bind(9, rwy.get_he_coord().get_lon());
			cmd.bind(10, rwy.get_le_coord().get_lat());
			cmd.bind(11, rwy.get_le_coord().get_lon());
			cmd.bind(12, rwy.get_he_tda());
			cmd.bind(13, rwy.get_he_lda());
			cmd.bind(14, rwy.get_he_disp());
			cmd.bind(15, rwy.get_he_hdg());
			cmd.bind(16, rwy.get_he_elev());
			cmd.bind(17, rwy.get_le_tda());
			cmd.bind(18, rwy.get_le_lda());
			cmd.bind(19, rwy.get_le_disp());
			cmd.bind(20, rwy.get_le_hdg());
			cmd.bind(21, rwy.get_le_elev());
			cmd.bind(22, (long long int)rwy.get_flags());
			for (unsigned int i = 0; i < 8; ++i) {
				cmd.bind(23 + i, rwy.get_he_light(i));
				cmd.bind(31 + i, rwy.get_le_light(i));
			}
			cmd.executenonquery();
		}
	}
	if (m_subtables & subtables_helipads) {
		// Delete existing Helipads
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airporthelipads WHERE ARPTID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		// Write Helipads
		for (unsigned int i = 0; i < get_nrhelipads(); i++) {
			const Helipad& hp(get_helipad(i));
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO airporthelipads (ARPTID,ID,IDENT,LENGTH,WIDTH,SURFACE,LAT,LON,HDG,ELEV,FLAGS) VALUES(?,?,?,?,?,?,?,?,?,?,?);");
			cmd.bind(1, (long long int)m_id);
			cmd.bind(2, (int)i);
			cmd.bind(3, hp.get_ident());
			cmd.bind(4, hp.get_length());
			cmd.bind(5, hp.get_width());
			cmd.bind(6, hp.get_surface());
			cmd.bind(7, hp.get_coord().get_lat());
			cmd.bind(8, hp.get_coord().get_lon());
			cmd.bind(9, hp.get_hdg());
			cmd.bind(10, hp.get_elev());
			cmd.bind(11, (long long int)hp.get_flags());
			cmd.executenonquery();
		}
	}
	if (m_subtables & subtables_comms) {
		// Delete existing Comms
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airportcomms WHERE ARPTID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		// Write Comms
		for (unsigned int i = 0; i < get_nrcomm(); i++) {
			const Comm& comm(get_comm(i));
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO airportcomms (ARPTID,ID,NAME,SECTOR,OPRHOURS,FREQ0,FREQ1,FREQ2,FREQ3,FREQ4) VALUES(?,?,?,?,?,?,?,?,?,?);");
			cmd.bind(1, (long long int)m_id);
			cmd.bind(2, (int)i);
			cmd.bind(3, comm.get_name());
			cmd.bind(4, comm.get_sector());
			cmd.bind(5, comm.get_oprhours());
			for (unsigned int j = 0; j < 5; j++)
				cmd.bind(6+j, (long long int)comm[j]);
			cmd.executenonquery();
		}
	}
	if (m_subtables & subtables_vfrroutes) {
		// Delete existing VFR Routes and Points
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airportvfrroutes WHERE ARPTID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airportvfrroutepoints WHERE ARPTID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		// Write VFR Routes
		for (unsigned int i = 0; i < get_nrvfrrte(); i++) {
			const VFRRoute& rte(get_vfrrte(i));
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO airportvfrroutes (ARPTID,ID,NAME,MINALT,MAXALT) VALUES(?,?,?,?,?);");
			cmd.bind(1, (long long int)m_id);
			cmd.bind(2, (int)i);
			cmd.bind(3, rte.get_name());
			cmd.bind(4, rte.get_minalt());
			cmd.bind(5, rte.get_maxalt());
			cmd.executenonquery();
			for (unsigned int j = 0; j < rte.size(); j++) {
				const VFRRoute::VFRRoutePoint& rtept(rte[j]);
				sqlite3x::sqlite3_command cmd(db, "INSERT INTO airportvfrroutepoints (ARPTID,ROUTEID,ID,NAME,LAT,LON,ALT,ALTCODE,PATHCODE,SYM,ATAIRPORT,LABELPLACEMENT) VALUES(?,?,?,?,?,?,?,?,?,?,?,?);");
				cmd.bind(1, (long long int)m_id);
				cmd.bind(2, (int)i);
				cmd.bind(3, (int)j);
				cmd.bind(4, rtept.get_name());
				cmd.bind(5, rtept.get_coord().get_lat());
				cmd.bind(6, rtept.get_coord().get_lon());
				cmd.bind(7, rtept.get_altitude());
				cmd.bind(8, rtept.get_altcode());
				cmd.bind(9, rtept.get_pathcode());
				cmd.bind(10, rtept.get_ptsym());
				cmd.bind(11, rtept.is_at_airport());
				cmd.bind(12, rtept.get_label_placement());
				cmd.executenonquery();
			}
		}
	}
	if (m_subtables & subtables_linefeatures) {
		// Delete existing Line Features
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airportlinefeatures WHERE ARPTID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		// Write Line Features
		for (unsigned int i = 0; i < get_nrlinefeatures(); i++) {
			const Polyline& pl(get_linefeature(i));
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO airportlinefeatures (ARPTID,ID,NAME,SURFACE,FLAGS,POLY) VALUES(?,?,?,?,?,?);");
			cmd.bind(1, (long long int)m_id);
			cmd.bind(2, (int)i);
			cmd.bind(3, pl.get_name());
			cmd.bind(4, pl.get_surface());
			cmd.bind(5, (long long int)pl.get_flags());
			pl.bindblob_polygon(cmd, 6);
			cmd.executenonquery();
		}
	}
	if (m_subtables & subtables_pointfeatures) {
		// Delete existing Point Features
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airportpointfeatures WHERE ARPTID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		// Write Point Features
		for (unsigned int i = 0; i < get_nrpointfeatures(); i++) {
			const PointFeature& pf(get_pointfeature(i));
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO airportpointfeatures (ARPTID,ID,FEATURE,LAT,LON,HDG,ELEV,SUBTYPE,ATTR1,ATTR2,NAME,RWYIDENT) VALUES(?,?,?,?,?,?,?,?,?,?,?,?);");
			cmd.bind(1, (long long int)m_id);
			cmd.bind(2, (int)i);
			cmd.bind(3, (int)pf.get_feature());
			cmd.bind(4, pf.get_coord().get_lat());
			cmd.bind(5, pf.get_coord().get_lon());
			cmd.bind(6, pf.get_hdg());
			cmd.bind(7, pf.get_elev());
			cmd.bind(8, (long long int)pf.get_subtype());
			cmd.bind(9, pf.get_attr1());
			cmd.bind(10, pf.get_attr2());
			cmd.bind(11, pf.get_name());
			cmd.bind(12, pf.get_rwyident());
			cmd.executenonquery();
		}
	}
	if (m_subtables & subtables_fas) {
		// Delete existing Final Approach Segments
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airportfas WHERE ARPTID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		// Write Point Features
		for (unsigned int i = 0; i < get_nrfinalapproachsegments(); i++) {
			const MinimalFAS& fas(get_finalapproachsegment(i));
			sqlite3x::sqlite3_command cmd(db, "INSERT INTO airportfas (ARPTID,ID,REFPATHID,FTPLAT,FTPLON,FPAPLAT,FPAPLON,OPTYP,RWY,RTE,REFPATHDS,FTPHEIGHT,APCHTCH,GPA,CWIDTH,DLENOFFS,HAL,VAL) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);");
			cmd.bind(1, (long long int)m_id);
			cmd.bind(2, (int)i);
			cmd.bind(3, fas.get_referencepathident());
			cmd.bind(4, fas.get_ftp().get_lat());
			cmd.bind(5, fas.get_ftp().get_lon());
			cmd.bind(6, fas.get_fpap().get_lat());
			cmd.bind(7, fas.get_fpap().get_lon());
			cmd.bind(8, fas.get_optyp());
			cmd.bind(9, fas.get_rwy());
			cmd.bind(10, fas.get_rte());
			cmd.bind(11, fas.get_referencepathdataselector());
			cmd.bind(12, fas.get_ftp_height());
			cmd.bind(13, fas.get_approach_tch());
			cmd.bind(14, fas.get_glidepathangle());
			cmd.bind(15, fas.get_coursewidth());
			cmd.bind(16, fas.get_dlengthoffset());
			cmd.bind(17, fas.get_hal());
			cmd.bind(18, fas.get_val());
			cmd.executenonquery();
		}
	}
	if (rtree) {
		sqlite3x::sqlite3_command cmd(db, "UPDATE airports_rtree SET SWLAT=?,NELAT=?,SWLON=?,NELON=? WHERE ID=?;");
		cmd.bind(1, m_bbox.get_south());
		cmd.bind(2, m_bbox.get_north());
		cmd.bind(3, m_bbox.get_west());
		cmd.bind(4, (long long int)m_bbox.get_east_unwrapped());
		cmd.bind(5, (long long int)m_id);
		cmd.executenonquery();
	}
	tran.commit();
}

void DbBaseElements::Airport::erase(sqlite3x::sqlite3_connection & db, bool rtree, bool aux_rtree)
{
	if (!is_valid())
		return;
	sqlite3x::sqlite3_transaction tran(db);
	if (m_table == table_aux) {
		sqlite3x::sqlite3_command cmd(db, "INSERT OR REPLACE INTO airports_deleted (SRCID) VALUES (?);");
		cmd.bind(1, m_sourceid);
		cmd.executenonquery();
		m_table = table_main;
	} else {
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airports WHERE ID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airportrunways WHERE ARPTID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airporthelipads WHERE ARPTID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airportcomms WHERE ARPTID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airportvfrroutes WHERE ARPTID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airportvfrroutepoints WHERE ARPTID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airportlinefeatures WHERE ARPTID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airportpointfeatures WHERE ARPTID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		{
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airportfas WHERE ARPTID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
		if (rtree) {
			sqlite3x::sqlite3_command cmd(db, "DELETE FROM airports_rtree WHERE ID=?;");
			cmd.bind(1, (long long int)m_id);
			cmd.executenonquery();
		}
	}
	tran.commit();
	m_id = 0;
}

#ifdef HAVE_PQXX

void DbBaseElements::Airport::load(const pqxx::tuple& cursor, pqxx::basic_transaction& w, unsigned int loadsubtables)
{
	m_id = 0;
	m_table = table_main;
	m_typecode = 0;
	m_subtables = subtables_none;
	if (cursor.empty())
		return;
	m_id = cursor[0].as<int64_t>(0);
	m_table = (table_t)cursor[1].as<int>(0);
	m_sourceid = cursor[2].as<std::string>("");
	m_icao = cursor[3].as<std::string>("");
	m_name = cursor[4].as<std::string>("");
	m_authority = cursor[5].as<std::string>("");
	m_vfrrmk = cursor[6].as<std::string>("");
	m_coord = Point(cursor[8].as<Point::coord_t>(0), cursor[7].as<Point::coord_t>(0x80000000));
	m_bbox = Rect(Point(cursor[10].as<Point::coord_t>(0), cursor[9].as<Point::coord_t>(0x80000000)),
		      Point(cursor[12].as<int64_t>(0), cursor[11].as<Point::coord_t>(0x80000000)));
	m_elev = cursor[13].as<int16_t>(0);
	m_typecode = cursor[14].as<int>(0);
	m_flightrules = cursor[15].as<unsigned int>(0);
	m_label_placement = (label_placement_t)cursor[16].as<int>((int)label_off);
	m_modtime = cursor[17].as<time_t>(0);
	if (!loadsubtables)
		return;
	load_subtables(w, loadsubtables);
}

void DbBaseElements::Airport::load_subtables(pqxx::basic_transaction& w, unsigned int loadsubtables)
{
	if (false)
		std::cerr << "Airport::load_subtables: loadsubtables 0x" << std::hex << loadsubtables << " m_subtables 0x" << std::hex << m_subtables << std::endl;
	loadsubtables &= subtables_all & ~m_subtables;
	if (!loadsubtables)
		return;
	if (loadsubtables & subtables_runways) {
		m_rwy.clear();
		pqxx::result r(w.exec("SELECT IDENTHE,IDENTLE,LENGTH,WIDTH,SURFACE,HELAT,HELON,LELAT,LELON,"
				      "HETDA,HELDA,HEDISP,HEHDG,HEELEV,LETDA,LELDA,LEDISP,LEHDG,LEELEV,FLAGS,"
				      "HELIGHT0,HELIGHT1,HELIGHT2,HELIGHT3,HELIGHT4,HELIGHT5,HELIGHT6,HELIGHT7,"
				      "LELIGHT0,LELIGHT1,LELIGHT2,LELIGHT3,LELIGHT4,LELIGHT5,LELIGHT6,LELIGHT7 "
				      "FROM aviationdb.airportrunways WHERE ARPTID=" + w.quote(m_id) + " ORDER BY ID;"));
		for (pqxx::result::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
			Runway rwy((*ri)[0].as<std::string>(""), (*ri)[1].as<std::string>(""),
				   (*ri)[2].as<uint16_t>(0), (*ri)[3].as<uint16_t>(0), (*ri)[4].as<std::string>(""),
				   Point((*ri)[6].as<Point::coord_t>(0), (*ri)[5].as<Point::coord_t>(0x80000000)),
				   Point((*ri)[8].as<Point::coord_t>(0), (*ri)[7].as<Point::coord_t>(0x80000000)),
				   (*ri)[9].as<uint16_t>(0), (*ri)[10].as<uint16_t>(0), (*ri)[11].as<uint16_t>(0),
				   (*ri)[12].as<uint16_t>(0), (*ri)[13].as<int16_t>(0),
				   (*ri)[14].as<uint16_t>(0), (*ri)[15].as<uint16_t>(0), (*ri)[16].as<uint16_t>(0),
				   (*ri)[17].as<uint16_t>(0), (*ri)[18].as<int16_t>(0), (*ri)[19].as<unsigned int>(0));
			for (unsigned int i = 0; i < 8; ++i) {
				rwy.set_he_light(i, (*ri)[20 + i].as<unsigned int>(0));
				rwy.set_le_light(i, (*ri)[28 + i].as<unsigned int>(0));
			}
			add_rwy(rwy);
		}
		m_subtables |= subtables_runways;
	}
	if (loadsubtables & subtables_helipads) {
		m_helipad.clear();
		pqxx::result r(w.exec("SELECT IDENT,LENGTH,WIDTH,SURFACE,LAT,LON,HDG,ELEV,FLAGS "
				      "FROM aviationdb.airporthelipads WHERE ARPTID=" + w.quote(m_id) + " ORDER BY ID;"));
		for (pqxx::result::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
			Helipad hp((*ri)[0].as<std::string>(""), (*ri)[1].as<unsigned int>(0), (*ri)[2].as<unsigned int>(0), (*ri)[3].as<std::string>(""),
				   Point((*ri)[5].as<Point::coord_t>(0), (*ri)[4].as<Point::coord_t>(0x80000000)),
				   (*ri)[6].as<unsigned int>(0), (*ri)[7].as<int32_t>(0), (*ri)[8].as<unsigned int>(0));
			add_helipad(hp);
		}
		m_subtables |= subtables_helipads;
	}
	if (loadsubtables & subtables_comms) {
		m_comm.clear();
		pqxx::result r(w.exec("SELECT NAME,SECTOR,OPRHOURS,FREQ0,FREQ1,FREQ2,FREQ3,FREQ4 "
				      "FROM aviationdb.airportcomms WHERE ARPTID=" + w.quote(m_id) + " ORDER BY ID;"));
		for (pqxx::result::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
			add_comm(Comm((*ri)[0].as<std::string>(""), (*ri)[1].as<std::string>(""), (*ri)[2].as<std::string>(""),
				      (*ri)[3].as<unsigned int>(0), (*ri)[4].as<unsigned int>(0), (*ri)[5].as<unsigned int>(0),
				      (*ri)[6].as<unsigned int>(0), (*ri)[7].as<unsigned int>(0)));
		}
		m_subtables |= subtables_comms;
	}
	if (loadsubtables & subtables_vfrroutes) {
		m_vfrrte.clear();
		typedef std::map<int,unsigned int> idmap_t;
		idmap_t idmap;
		{
			pqxx::result r(w.exec("SELECT ID,NAME,MINALT,MAXALT "
					      "FROM aviationdb.airportvfrroutes WHERE ARPTID=" + w.quote(m_id) + " ORDER BY ID;"));
			for (pqxx::result::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
				VFRRoute rte((*ri)[1].as<std::string>(""));
				if (!(*ri)[2].is_null())
					rte.set_minalt((*ri)[2].as<int32_t>());
				if (!(*ri)[3].is_null())
					rte.set_maxalt((*ri)[3].as<int32_t>());
				idmap[(*ri)[0].as<int>(0)] = get_nrvfrrte();
				add_vfrrte(rte);
			}
		}
		{
			pqxx::result r(w.exec("SELECT ROUTEID,NAME,LAT,LON,ALT,ALTCODE,PATHCODE,SYM,ATAIRPORT,LABELPLACEMENT "
					      "FROM aviationdb.airportvfrroutepoints WHERE ARPTID=" + w.quote(m_id) + " ORDER BY ROUTEID,ID;"));
			for (pqxx::result::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
				VFRRoute::VFRRoutePoint rtept((*ri)[1].as<std::string>(""),
							      Point((*ri)[3].as<Point::coord_t>(0), (*ri)[2].as<Point::coord_t>(0x80000000)),
							      (*ri)[4].as<int16_t>(0), (label_placement_t)(*ri)[9].as<int>((int)label_off),
							      (char)(*ri)[7].as<int>(0),
							      (VFRRoute::VFRRoutePoint::pathcode_t)(*ri)[6].as<int>((int)VFRRoute::VFRRoutePoint::pathcode_invalid),
							      (VFRRoute::VFRRoutePoint::altcode_t)(*ri)[5].as<int>((int)VFRRoute::VFRRoutePoint::altcode_invalid),
							      (*ri)[8].as<bool>(0));
				idmap_t::const_iterator i(idmap.find((*ri)[0].as<int>(0)));
				if (i == idmap.end() || i->second >= get_nrvfrrte())
					throw std::runtime_error("airports db: inconsistent airportvfrroutes/airportvfrroutepoints");
				VFRRoute& rte(get_vfrrte(i->second));
				rte.add_point(rtept);
			}
		}
		m_subtables |= subtables_vfrroutes;
	}
	if (loadsubtables & subtables_linefeatures) {
		m_linefeature.clear();
	        pqxx::result r(w.exec("SELECT NAME,SURFACE,FLAGS,POLY "
				      "FROM aviationdb.airportlinefeatures WHERE ARPTID=" + w.quote(m_id) + " ORDER BY ID;"));
		for (pqxx::result::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
			Polyline pl;
			pl.set_name((*ri)[0].as<std::string>(""));
			pl.set_surface((*ri)[1].as<std::string>(""));
			pl.set_flags((*ri)[2].as<unsigned int>(0));
			pl.getblob_polygon(pqxx::binarystring((*ri)[3]));
			add_linefeature(pl);
		}
		m_subtables |= subtables_linefeatures;
	}
	if (loadsubtables & subtables_pointfeatures) {
		m_pointfeature.clear();
		pqxx::result r(w.exec("SELECT FEATURE,LAT,LON,HDG,ELEV,SUBTYPE,ATTR1,ATTR2,NAME,RWYIDENT "
				      "FROM aviationdb.airportpointfeatures WHERE ARPTID=" + w.quote(m_id) + " ORDER BY ID;"));
		for (pqxx::result::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
			PointFeature pf((PointFeature::feature_t)(*ri)[0].as<int>((int)PointFeature::feature_invalid),
					Point((*ri)[2].as<Point::coord_t>(0), (*ri)[1].as<Point::coord_t>(0x80000000)),
					(*ri)[3].as<unsigned int>(0), (*ri)[4].as<int32_t>(0), (*ri)[5].as<unsigned int>(0),
					(*ri)[6].as<int>(0), (*ri)[7].as<int>(0),
					(*ri)[8].as<std::string>(""), (*ri)[9].as<std::string>(""));
			add_pointfeature(pf);
		}
		m_subtables |= subtables_pointfeatures;
	}
	if (loadsubtables & subtables_fas) {
		m_fas.clear();
		pqxx::result r(w.exec("SELECT REFPATHID,FTPLAT,FTPLON,FPAPLAT,FPAPLON,OPTYP,RWY,RTE,REFPATHDS,FTPHEIGHT,APCHTCH,GPA,CWIDTH,DLENOFFS,HAL,VAL "
				      "FROM aviationdb.airportfas WHERE ARPTID=" + w.quote(m_id) + " ORDER BY ID;"));
		for (pqxx::result::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
			MinimalFAS fas;
			fas.set_referencepathident((*ri)[0].as<std::string>(""));
			fas.set_ftp(Point((*ri)[2].as<Point::coord_t>(0), (*ri)[1].as<Point::coord_t>(0x80000000)));
			fas.set_fpap(Point((*ri)[4].as<Point::coord_t>(0), (*ri)[3].as<Point::coord_t>(0x80000000)));
			fas.set_optyp((*ri)[5].as<unsigned int>(0));
			fas.set_rwy((*ri)[6].as<unsigned int>(0));
			fas.set_rte((*ri)[7].as<unsigned int>(0));
			fas.set_referencepathdataselector((*ri)[8].as<unsigned int>(0));
			fas.set_ftp_height((*ri)[9].as<unsigned int>(0));
			fas.set_approach_tch((*ri)[10].as<unsigned int>(0));
			fas.set_glidepathangle((*ri)[11].as<unsigned int>(0));
			fas.set_coursewidth((*ri)[12].as<unsigned int>(0));
			fas.set_dlengthoffset((*ri)[13].as<unsigned int>(0));
			fas.set_hal((*ri)[14].as<unsigned int>(0));
			fas.set_val((*ri)[15].as<unsigned int>(0));
			add_finalapproachsegment(fas);
		}
		m_subtables |= subtables_fas;
	}
}

void DbBaseElements::Airport::save(pqxx::connection_base& conn, bool rtree)
{
	pqxx::work w(conn);
	if (!m_id) {
		pqxx::result r(w.exec("INSERT INTO aviationdb.airports (ID,SRCID) SELECT COALESCE(MAX(ID),0)+1," + w.quote((std::string)m_sourceid) + " FROM aviationdb.airports RETURNING ID;"));
		m_id = r.front()[0].as<int64_t>();
		if (rtree)
			w.exec("INSERT INTO aviationdb.airports_rtree (ID,SWLAT,NELAT,SWLON,NELON) VALUES(" + w.quote(m_id) + "," + w.quote(m_bbox.get_south()) +
			       "," + w.quote(m_bbox.get_north()) + "," + w.quote(m_bbox.get_west()) + "," + w.quote((long long int)m_bbox.get_east_unwrapped()) + ");");
	}
	// Update Airport Record
	w.exec("UPDATE aviationdb.airports SET SRCID=" + w.quote((std::string)m_sourceid) + ",ICAO=" + w.quote((std::string)m_icao) +
	       ",NAME=" + w.quote((std::string)m_name) + ",AUTHORITY=" + w.quote((std::string)m_authority) +
	       ",VFRRMK=" + w.quote((std::string)m_vfrrmk) + ",LAT=" + w.quote(m_coord.get_lat()) + ",LON=" + w.quote(m_coord.get_lon()) +
	       ",SWLAT=" + w.quote(m_bbox.get_south()) + ",SWLON=" + w.quote(m_bbox.get_west()) +
	       ",NELAT=" + w.quote(m_bbox.get_north()) + ",NELON=" + w.quote(m_bbox.get_east_unwrapped()) +
	       ",ELEVATION=" + w.quote(m_elev) + ",TYPECODE=" + w.quote((int)m_typecode) + ",FRULES=" + w.quote((unsigned int)m_flightrules) +
	       ",LABELPLACEMENT=" + w.quote((int)m_label_placement) + ",MODIFIED=" + w.quote(m_modtime) +
	       ",TILE=" + w.quote(TileNumber::to_tilenumber(m_bbox)) + " WHERE ID=" + w.quote(m_id) + ";");
	if (m_subtables & subtables_runways) {
		// Delete existing Runways
		w.exec("DELETE FROM aviationdb.airportrunways WHERE ARPTID=" + w.quote(m_id) + ";");
		// Write Runways
		for (unsigned int i = 0; i < get_nrrwy(); i++) {
			const Runway& rwy(get_rwy(i));
			w.exec("INSERT INTO aviationdb.airportrunways (ARPTID,ID,IDENTHE,IDENTLE,LENGTH,WIDTH,SURFACE,"
			       "HELAT,HELON,LELAT,LELON,HETDA,HELDA,HEDISP,HEHDG,HEELEV,LETDA,LELDA,LEDISP,LEHDG,LEELEV,FLAGS,"
			       "HELIGHT0,HELIGHT1,HELIGHT2,HELIGHT3,HELIGHT4,HELIGHT5,HELIGHT6,HELIGHT7,"
			       "LELIGHT0,LELIGHT1,LELIGHT2,LELIGHT3,LELIGHT4,LELIGHT5,LELIGHT6,LELIGHT7) VALUES(" +
			       w.quote(m_id) + "," + w.quote(i) + "," +
			       w.quote((std::string)rwy.get_ident_he()) + "," + w.quote((std::string)rwy.get_ident_le()) + "," +
			       w.quote(rwy.get_length()) + "," + w.quote(rwy.get_width()) + "," +
			       w.quote((std::string)rwy.get_surface()) + "," +
			       w.quote(rwy.get_he_coord().get_lat()) + "," + w.quote(rwy.get_he_coord().get_lon()) + "," +
			       w.quote(rwy.get_le_coord().get_lat()) + "," + w.quote(rwy.get_le_coord().get_lon()) + "," +
			       w.quote(rwy.get_he_tda()) + "," + w.quote(rwy.get_he_lda()) + "," + w.quote(rwy.get_he_disp()) + "," +
			       w.quote(rwy.get_he_hdg()) + "," + w.quote(rwy.get_he_elev()) + "," +
			       w.quote(rwy.get_le_tda()) + "," + w.quote(rwy.get_le_lda()) + "," + w.quote(rwy.get_le_disp()) + "," +
			       w.quote(rwy.get_le_hdg()) + "," + w.quote(rwy.get_le_elev()) + "," + w.quote(rwy.get_flags()) + "," +
			       w.quote((unsigned int)rwy.get_he_light(0)) + "," + w.quote((unsigned int)rwy.get_he_light(1)) + "," +
			       w.quote((unsigned int)rwy.get_he_light(2)) + "," + w.quote((unsigned int)rwy.get_he_light(3)) + "," +
			       w.quote((unsigned int)rwy.get_he_light(4)) + "," + w.quote((unsigned int)rwy.get_he_light(5)) + "," +
			       w.quote((unsigned int)rwy.get_he_light(6)) + "," + w.quote((unsigned int)rwy.get_he_light(7)) + "," +
			       w.quote((unsigned int)rwy.get_le_light(0)) + "," + w.quote((unsigned int)rwy.get_le_light(1)) + "," +
			       w.quote((unsigned int)rwy.get_le_light(2)) + "," + w.quote((unsigned int)rwy.get_le_light(3)) + "," +
			       w.quote((unsigned int)rwy.get_le_light(4)) + "," + w.quote((unsigned int)rwy.get_le_light(5)) + "," +
			       w.quote((unsigned int)rwy.get_le_light(6)) + "," + w.quote((unsigned int)rwy.get_le_light(7)) + ");");
		}
	}
	if (m_subtables & subtables_helipads) {
		// Delete existing Helipads
	        w.exec("DELETE FROM aviationdb.airporthelipads WHERE ARPTID=" + w.quote(m_id) + ";");
		// Write Helipads
		for (unsigned int i = 0; i < get_nrhelipads(); i++) {
			const Helipad& hp(get_helipad(i));
			w.exec("INSERT INTO aviationdb.airporthelipads (ARPTID,ID,IDENT,LENGTH,WIDTH,SURFACE,LAT,LON,HDG,ELEV,FLAGS) VALUES(" +
			       w.quote(m_id) + "," + w.quote(i) + "," + w.quote((std::string)hp.get_ident()) + "," +
			       w.quote(hp.get_length()) + "," + w.quote(hp.get_width()) + "," +	w.quote((std::string)hp.get_surface()) + "," +
			       w.quote(hp.get_coord().get_lat()) + "," + w.quote(hp.get_coord().get_lon()) + "," +
			       w.quote(hp.get_hdg()) + "," + w.quote(hp.get_elev()) + "," + w.quote(hp.get_flags()) + ");");
		}
	}
	if (m_subtables & subtables_comms) {
		// Delete existing Comms
		w.exec("DELETE FROM aviationdb.airportcomms WHERE ARPTID=" + w.quote(m_id) + ";");
		// Write Comms
		for (unsigned int i = 0; i < get_nrcomm(); i++) {
			const Comm& comm(get_comm(i));
			w.exec("INSERT INTO aviationdb.airportcomms (ARPTID,ID,NAME,SECTOR,OPRHOURS,FREQ0,FREQ1,FREQ2,FREQ3,FREQ4) VALUES(" +
			       w.quote(m_id) + "," + w.quote(i) + "," + w.quote((std::string)comm.get_name()) + "," +
			       w.quote((std::string)comm.get_sector()) + "," + w.quote((std::string)comm.get_oprhours()) + "," +
			       w.quote(comm[0]) + "," + w.quote(comm[1]) + "," + w.quote(comm[2]) + "," +
			       w.quote(comm[3]) + "," + w.quote(comm[4]) + ");");
		}
	}
	if (m_subtables & subtables_vfrroutes) {
		// Delete existing VFR Routes and Points
		w.exec("DELETE FROM aviationdb.airportvfrroutes WHERE ARPTID=" + w.quote(m_id) + ";");
		w.exec("DELETE FROM aviationdb.airportvfrroutepoints WHERE ARPTID=" + w.quote(m_id) + ";");
		// Write VFR Routes
		for (unsigned int i = 0; i < get_nrvfrrte(); i++) {
			const VFRRoute& rte(get_vfrrte(i));
			w.exec("INSERT INTO aviationdb.airportvfrroutes (ARPTID,ID,NAME,MINALT,MAXALT) VALUES(" +
			       w.quote(m_id) + "," + w.quote(i) + "," +	w.quote((std::string)rte.get_name()) + "," +
			       w.quote(rte.get_minalt()) + "," + w.quote(rte.get_maxalt()) + ");");
        		for (unsigned int j = 0; j < rte.size(); j++) {
				const VFRRoute::VFRRoutePoint& rtept(rte[j]);
				w.exec("INSERT INTO aviationdb.airportvfrroutepoints (ARPTID,ROUTEID,ID,NAME,LAT,LON,ALT,ALTCODE,PATHCODE,SYM,ATAIRPORT,LABELPLACEMENT) VALUES(" +
				       w.quote(m_id) + "," + w.quote(i) + "," +	w.quote(j) + "," + w.quote((std::string)rtept.get_name()) + "," +
				       w.quote(rtept.get_coord().get_lat()) + "," + w.quote(rtept.get_coord().get_lon()) + "," +
				       w.quote(rtept.get_altitude()) + "," + w.quote((int)rtept.get_altcode()) + "," +
				       w.quote((int)rtept.get_pathcode()) + "," + w.quote((int)rtept.get_ptsym()) + "," +
				       w.quote(rtept.is_at_airport()) + "," + w.quote((int)rtept.get_label_placement()) + ");");
			}
		}
	}
	if (m_subtables & subtables_linefeatures) {
		// Delete existing Line Features
		w.exec("DELETE FROM aviationdb.airportlinefeatures WHERE ARPTID=" + w.quote(m_id) + ";");
		// Write Line Features
		for (unsigned int i = 0; i < get_nrlinefeatures(); i++) {
			const Polyline& pl(get_linefeature(i));
			pqxx::binarystring blob(0, 0);
			pl.bindblob_polygon(blob);
			w.exec("INSERT INTO aviationdb.airportlinefeatures (ARPTID,ID,NAME,SURFACE,FLAGS,POLY) VALUES(" +
			       w.quote(m_id) + "," + w.quote(i) + "," + w.quote((std::string)pl.get_name()) + "," +
			       w.quote((std::string)pl.get_surface()) + "," + w.quote(pl.get_flags()) + "," +
			       w.quote(blob) + ");");
		}
	}
	if (m_subtables & subtables_pointfeatures) {
		// Delete existing Point Features
		w.exec("DELETE FROM aviationdb.airportpointfeatures WHERE ARPTID=" + w.quote(m_id) + ";");
		// Write Point Features
		for (unsigned int i = 0; i < get_nrpointfeatures(); i++) {
			const PointFeature& pf(get_pointfeature(i));
			w.exec("INSERT INTO aviationdb.airportpointfeatures (ARPTID,ID,FEATURE,LAT,LON,HDG,ELEV,SUBTYPE,ATTR1,ATTR2,NAME,RWYIDENT) VALUES(" +
			       w.quote(m_id) + "," + w.quote(i) + "," + w.quote((int)pf.get_feature()) + "," +
			       w.quote(pf.get_coord().get_lat()) + "," + w.quote(pf.get_coord().get_lon()) + "," +
			       w.quote(pf.get_hdg()) + "," + w.quote(pf.get_elev()) + "," + w.quote(pf.get_subtype()) + "," +
			       w.quote(pf.get_attr1()) + "," + w.quote(pf.get_attr2()) + "," + w.quote((std::string)pf.get_name()) + "," +
			       w.quote((std::string)pf.get_rwyident()) + ");");
		}
	}
	if (m_subtables & subtables_fas) {
		// Delete existing Final Approach Segments
		w.exec("DELETE FROM aviationdb.airportfas WHERE ARPTID=" + w.quote(m_id) + ";");
		// Write Point Features
		for (unsigned int i = 0; i < get_nrfinalapproachsegments(); i++) {
			const MinimalFAS& fas(get_finalapproachsegment(i));
			w.exec("INSERT INTO aviationdb.airportfas (ARPTID,ID,REFPATHID,FTPLAT,FTPLON,FPAPLAT,FPAPLON,OPTYP,RWY,RTE,REFPATHDS,FTPHEIGHT,APCHTCH,GPA,CWIDTH,DLENOFFS,HAL,VAL) VALUES(" +
			       w.quote(m_id) + "," + w.quote(i) + "," + w.quote((std::string)fas.get_referencepathident()) + "," +
			       w.quote(fas.get_ftp().get_lat()) + "," + w.quote(fas.get_ftp().get_lon()) + "," +
			       w.quote(fas.get_fpap().get_lat()) + "," + w.quote(fas.get_fpap().get_lon()) + "," +
			       w.quote((int)fas.get_optyp()) + "," + w.quote((int)fas.get_rwy()) + "," + w.quote((int)fas.get_rte()) + "," +
			       w.quote((int)fas.get_referencepathdataselector()) + "," + w.quote(fas.get_ftp_height()) + "," +
			       w.quote(fas.get_approach_tch()) + "," + w.quote(fas.get_glidepathangle()) + "," +
			       w.quote((int)fas.get_coursewidth()) + "," + w.quote((int)fas.get_dlengthoffset()) + "," +
			       w.quote((int)fas.get_hal()) + "," + w.quote((int)fas.get_val()) + ");");
		}
	}
	if (rtree)
		w.exec("UPDATE aviationdb.airports_rtree SET SWLAT=" + w.quote(m_bbox.get_south()) + ",NELAT=" + w.quote(m_bbox.get_north()) +
		       ",SWLON=" + w.quote(m_bbox.get_west()) + ",NELON=" + w.quote((long long int)m_bbox.get_east_unwrapped()) + " WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
}

void DbBaseElements::Airport::erase(pqxx::connection_base& conn, bool rtree)
{
	if (!is_valid())
		return;
	pqxx::work w(conn);
	w.exec("DELETE FROM aviationdb.airports WHERE ID=" + w.quote(m_id) + ";");
	w.exec("DELETE FROM aviationdb.airportrunways WHERE ARPTID=" + w.quote(m_id) + ";");
	w.exec("DELETE FROM aviationdb.airporthelipads WHERE ARPTID=" + w.quote(m_id) + ";");
	w.exec("DELETE FROM aviationdb.airportcomms WHERE ARPTID=" + w.quote(m_id) + ";");
	w.exec("DELETE FROM aviationdb.airportvfrroutes WHERE ARPTID=" + w.quote(m_id) + ";");
	w.exec("DELETE FROM aviationdb.airportvfrroutepoints WHERE ARPTID=" + w.quote(m_id) + ";");
	w.exec("DELETE FROM aviationdb.airportlinefeatures WHERE ARPTID=" + w.quote(m_id) + ";");
	w.exec("DELETE FROM aviationdb.airportpointfeatures WHERE ARPTID=" + w.quote(m_id) + ";");
	w.exec("DELETE FROM aviationdb.airportfas WHERE ARPTID=" + w.quote(m_id) + ";");
	if (rtree)
		w.exec("DELETE FROM aviationdb.airports_rtree WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
	m_id = 0;
}

void DbBaseElements::Airport::update_index(pqxx::connection_base& conn, bool rtree)
{
	if (!m_id)
		return;
	pqxx::work w(conn);
	w.exec("UPDATE aviationdb.airports SET TILE=" + w.quote(TileNumber::to_tilenumber(m_bbox)) + " WHERE ID=" + w.quote(m_id) + ";");
	if (rtree)
		w.exec("UPDATE aviationdb.airports_rtree SET SWLAT=" + w.quote(m_bbox.get_south()) + ",NELAT=" + w.quote(m_bbox.get_north()) +
		       ",SWLON=" + w.quote(m_bbox.get_west()) + ",NELON=" + w.quote((long long int)m_bbox.get_east_unwrapped()) + " WHERE ID=" + w.quote(m_id) + ";");
	w.commit();
}

#endif

const Glib::ustring& DbBaseElements::Airport::get_type_string( char t )
{
	switch (t) {
	case 'A':
	{
		static const Glib::ustring r("CIV");
		return r;
	}

	case 'B':
	{
		static const Glib::ustring r("CIV/MIL");
		return r;
	}

	case 'C':
	{
		static const Glib::ustring r("MIL");
		return r;
	}

	case 'D':
	default:
	{
		static const Glib::ustring r("other");
		return r;
	}
	}
}

void DbBaseElements::Airport::recompute_bbox(void)
{
	Point sw(0, 0), ne(0, 0);
	for (unsigned int i = 0; i < get_nrvfrrte(); i++) {
		const VFRRoute& rt(get_vfrrte(i));
		for (unsigned int j = 0; j < rt.size(); j++) {
			const VFRRoute::VFRRoutePoint& rtp(rt[j]);
			Point p(rtp.get_coord() - get_coord());
			sw = Point(std::min(sw.get_lon(), p.get_lon()), std::min(sw.get_lat(), p.get_lat()));
			ne = Point(std::max(ne.get_lon(), p.get_lon()), std::max(ne.get_lat(), p.get_lat()));
		}
	}
	for (unsigned int i = 0; i < get_nrrwy(); i++) {
		const Runway& rwy(get_rwy(i));
		{
			Point p(rwy.get_he_coord() - get_coord());
			sw = Point(std::min(sw.get_lon(), p.get_lon()), std::min(sw.get_lat(), p.get_lat()));
			ne = Point(std::max(ne.get_lon(), p.get_lon()), std::max(ne.get_lat(), p.get_lat()));
		}
		{
			Point p(rwy.get_le_coord() - get_coord());
			sw = Point(std::min(sw.get_lon(), p.get_lon()), std::min(sw.get_lat(), p.get_lat()));
			ne = Point(std::max(ne.get_lon(), p.get_lon()), std::max(ne.get_lat(), p.get_lat()));
		}
	}
	for (unsigned int i = 0; i < get_nrhelipads(); i++) {
		const Helipad& hp(get_helipad(i));
		Point p(hp.get_coord() - get_coord());
		sw = Point(std::min(sw.get_lon(), p.get_lon()), std::min(sw.get_lat(), p.get_lat()));
		ne = Point(std::max(ne.get_lon(), p.get_lon()), std::max(ne.get_lat(), p.get_lat()));
	}
	for (unsigned int i = 0; i < get_nrpointfeatures(); i++) {
		const PointFeature& pf(get_pointfeature(i));
		Point p(pf.get_coord() - get_coord());
		sw = Point(std::min(sw.get_lon(), p.get_lon()), std::min(sw.get_lat(), p.get_lat()));
		ne = Point(std::max(ne.get_lon(), p.get_lon()), std::max(ne.get_lat(), p.get_lat()));
	}
	for (unsigned int i = 0; i < get_nrlinefeatures(); i++) {
		const Polyline& pl(get_linefeature(i));
		if (pl.empty())
			continue;
		Rect bb(pl.get_bbox());
		sw = Point(std::min(sw.get_lon(), bb.get_west() - get_coord().get_lon()), std::min(sw.get_lat(), bb.get_south() - get_coord().get_lat()));
		ne = Point(std::max(ne.get_lon(), bb.get_east() - get_coord().get_lon()), std::max(ne.get_lat(), bb.get_north() - get_coord().get_lat()));
	}
	m_bbox = Rect(sw + get_coord(), ne + get_coord());
}

DbBaseElements::Airport::FinalApproachSegment DbBaseElements::Airport::operator()(const MinimalFAS& fas) const
{
	FinalApproachSegment f;
	*(MinimalFAS *)&f = fas;
	f.set_airportid(get_icao());
	return f;
}

bool DbBaseElements::Airport::is_fpl_zzzz(const Glib::ustring& icao)
{
	if (icao.size() != 4)
		return true;
	bool zzzz(true);
	for (Glib::ustring::const_iterator i(icao.begin()), e(icao.end()); i != e; ++i) {
		if (!std::isalpha(*i))
			return true;
		zzzz = zzzz && (*i == 'Z' || *i == 'z');
	}
	return zzzz;
}

bool DbBaseElements::Airport::is_fpl_afil(const Glib::ustring& icao)
{
	if (icao.size() != 4)
		return false;
	return (icao[0] == 'A' || icao[0] == 'a') &&
		(icao[1] == 'F' || icao[1] == 'f') &&
		(icao[2] == 'I' || icao[2] == 'i') &&
		(icao[3] == 'L' || icao[3] == 'l');
}

bool DbBaseElements::Airport::is_fpl_zzzz_or_afil(const Glib::ustring& icao)
{
	return is_fpl_zzzz(icao) || is_fpl_afil(icao);
}

template<> void DbBase<DbBaseElements::Airport>::drop_tables(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS airports;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS airportrunways;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS airporthelipads;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS airportcomms;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS airportvfrroutes;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS airportvfrroutepoints;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS airportlinefeatures;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS airportpointfeatures;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP TABLE IF EXISTS airports_deleted;"); cmd.executenonquery(); }
}

template<> void DbBase<DbBaseElements::Airport>::create_tables(void)
{
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS airports (ID INTEGER PRIMARY KEY NOT NULL, "
					      "SRCID TEXT UNIQUE NOT NULL,"
					      "ICAO TEXT,"
					      "NAME TEXT,"
					      "AUTHORITY TEXT,"
					      "VFRRMK TEXT,"
					      "LAT INTEGER,"
					      "LON INTEGER,"
					      "SWLAT INTEGER,"
					      "SWLON INTEGER,"
					      "NELAT INTEGER,"
					      "NELON INTEGER,"
					      "ELEVATION INTEGER,"
					      "TYPECODE CHAR,"
					      "FRULES INTEGER,"
					      "LABELPLACEMENT CHAR,"
					      "MODIFIED INTEGER,"
					      "TILE INTEGER);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS airportrunways (ID INTEGER NOT NULL, "
					      "ARPTID INTEGER NOT NULL,"
					      "IDENTHE TEXT,"
					      "IDENTLE TEXT,"
					      "LENGTH INTEGER,"
					      "WIDTH INTEGER,"
					      "SURFACE TEXT,"
					      "HELAT INTEGER,"
					      "HELON INTEGER,"
					      "LELAT INTEGER,"
					      "LELON INTEGER,"
					      "HETDA INTEGER,"
					      "HELDA INTEGER,"
					      "HEDISP INTEGER,"
					      "HEHDG INTEGER,"
					      "HEELEV INTEGER,"
					      "LETDA INTEGER,"
					      "LELDA INTEGER,"
					      "LEDISP INTEGER,"
					      "LEHDG INTEGER,"
					      "LEELEV INTEGER,"
					      "FLAGS INTEGER,"
					      "HELIGHT0 INTEGER,"
					      "HELIGHT1 INTEGER,"
					      "HELIGHT2 INTEGER,"
					      "HELIGHT3 INTEGER,"
					      "HELIGHT4 INTEGER,"
					      "HELIGHT5 INTEGER,"
					      "HELIGHT6 INTEGER,"
					      "HELIGHT7 INTEGER,"
					      "LELIGHT0 INTEGER,"
					      "LELIGHT1 INTEGER,"
					      "LELIGHT2 INTEGER,"
					      "LELIGHT3 INTEGER,"
					      "LELIGHT4 INTEGER,"
					      "LELIGHT5 INTEGER,"
					      "LELIGHT6 INTEGER,"
					      "LELIGHT7 INTEGER);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS airporthelipads (ID INTEGER NOT NULL, "
					      "ARPTID INTEGER NOT NULL,"
					      "IDENT TEXT,"
					      "LENGTH INTEGER,"
					      "WIDTH INTEGER,"
					      "SURFACE TEXT,"
					      "LAT INTEGER,"
					      "LON INTEGER,"
					      "HDG INTEGER,"
					      "ELEV INTEGER,"
					      "FLAGS INTEGER);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS airportcomms (ID INTEGER NOT NULL, "
					      "ARPTID INTEGER NOT NULL,"
					      "NAME TEXT,"
					      "SECTOR TEXT,"
					      "OPRHOURS TEXT,"
					      "FREQ0 INTEGER,"
					      "FREQ1 INTEGER,"
					      "FREQ2 INTEGER,"
					      "FREQ3 INTEGER,"
					      "FREQ4 INTEGER);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS airportvfrroutes (ID INTEGER NOT NULL, "
					      "ARPTID INTEGER NOT NULL,"
					      "NAME TEXT,"
					      "MINALT INTEGER,"
					      "MAXALT INTEGER);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS airportvfrroutepoints (ID INTEGER NOT NULL, "
					      "ARPTID INTEGER NOT NULL,"
					      "ROUTEID INTEGER NOT NULL,"
					      "NAME TEXT,"
					      "LAT INTEGER,"
					      "LON INTEGER,"
					      "ALT INTEGER,"
					      "ALTCODE CHAR,"
					      "PATHCODE CHAR,"
					      "SYM CHAR,"
					      "ATAIRPORT CHAR,"
					      "LABELPLACEMENT CHAR);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS airportlinefeatures (ID INTEGER NOT NULL, "
					      "ARPTID INTEGER NOT NULL,"
					      "NAME TEXT,"
					      "SURFACE TEXT,"
					      "FLAGS INTEGER,"
					      "POLY BLOB);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS airportpointfeatures (ID INTEGER NOT NULL, "
					      "ARPTID INTEGER NOT NULL,"
					      "FEATURE INTEGER,"
					      "LAT INTEGER,"
					      "LON INTEGER,"
					      "HDG INTEGER,"
					      "ELEV INTEGER,"
					      "SUBTYPE INTEGER,"
					      "ATTR1 INTEGER,"
					      "ATTR2 INTEGER,"
					      "NAME TEXT,"
					      "RWYIDENT TEXT);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS airportfas (ID INTEGER NOT NULL, "
					      "ARPTID INTEGER NOT NULL,"
					      "REFPATHID TEXT,"
					      "FTPLAT INTEGER,"
					      "FTPLON INTEGER,"
					      "FPAPLAT INTEGER,"
					      "FPAPLON INTEGER,"
					      "OPTYP INTEGER,"
					      "RWY INTEGER,"
					      "RTE INTEGER,"
					      "REFPATHDS INTEGER,"
					      "FTPHEIGHT INTEGER,"
					      "APCHTCH INTEGER,"
					      "GPA INTEGER,"
					      "CWIDTH INTEGER,"
					      "DLENOFFS INTEGER,"
					      "HAL INTEGER,"
					      "VAL INTEGER);");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS airports_deleted(SRCID TEXT PRIMARY KEY NOT NULL);");
		cmd.executenonquery();
	}
}

template<> void DbBase<DbBaseElements::Airport>::drop_indices(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airports_srcid;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airports_icao;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airports_name;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airports_latlon;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airports_bbox;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airports_modified;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airportrunways_id;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airporthelipads_id;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airportcomms_id;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airportvfrroutes_id;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airportvfrroutepoints_id;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airportvfrroutepoints_name;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airportvfrroutepoints_latlon;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airports_tile;"); cmd.executenonquery(); }
	// old indices
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airports_lat;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airports_lon;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airports_swlat;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airports_swlon;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airports_nelat;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airports_nelon;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airportvfrroutepoints_lat;"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "DROP INDEX IF EXISTS airportvfrroutepoints_lon;"); cmd.executenonquery(); }
}

template<> void DbBase<DbBaseElements::Airport>::create_indices(void)
{
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airports_srcid ON airports(SRCID);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airports_icao ON airports(ICAO COLLATE NOCASE);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airports_name ON airports(NAME COLLATE NOCASE);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airports_latlon ON airports(LAT,LON);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airports_bbox ON airports(SWLAT,NELAT,SWLON,NELON);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airports_modified ON airports(MODIFIED);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airportrunways_id ON airportrunways(ARPTID,ID);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airporthelipads_id ON airporthelipads(ARPTID,ID);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airportcomms_id ON airportcomms(ARPTID,ID);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airportvfrroutes_id ON airportvfrroutes(ARPTID,ID);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airportvfrroutepoints_id ON airportvfrroutepoints(ARPTID,ROUTEID,ID);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airportvfrroutepoints_name ON airportvfrroutepoints(NAME COLLATE NOCASE);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airportvfrroutepoints_latlon ON airportvfrroutepoints(LAT,LON);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airports_tile ON airports(TILE);"); cmd.executenonquery(); }
	// old indices
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airports_swlat ON airports(SWLAT);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airports_swlon ON airports(SWLON);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airports_nelat ON airports(NELAT);"); cmd.executenonquery(); }
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS airports_nelon ON airports(NELON);"); cmd.executenonquery(); }
}

template<> const char *DbBase<DbBaseElements::Airport>::main_table_name = "airports";
template<> const char *DbBase<DbBaseElements::Airport>::database_file_name = "airports.db";
template<> const char *DbBase<DbBaseElements::Airport>::order_field = "SRCID";
template<> const char *DbBase<DbBaseElements::Airport>::delete_field = "SRCID";
template<> const bool DbBase<DbBaseElements::Airport>::area_data = true;

#ifdef HAVE_PQXX

template<> void PGDbBase<DbBaseElements::Airport>::drop_tables(void)
{
	pqxx::work w(m_conn);
	w.exec("DROP TABLE IF EXISTS aviationdb.airports;");
	w.exec("DROP TABLE IF EXISTS aviationdb.airportrunways;");
	w.exec("DROP TABLE IF EXISTS aviationdb.airporthelipads;");
	w.exec("DROP TABLE IF EXISTS aviationdb.airportcomms;");
	w.exec("DROP TABLE IF EXISTS aviationdb.airportvfrroutes;");
	w.exec("DROP TABLE IF EXISTS aviationdb.airportvfrroutepoints;");
	w.exec("DROP TABLE IF EXISTS aviationdb.airportlinefeatures;");
	w.exec("DROP TABLE IF EXISTS aviationdb.airportpointfeatures;");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Airport>::create_tables(void)
{
	create_common_tables();
	pqxx::work w(m_conn);
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.airports ("
	       "VFRRMK TEXT,"
	       "SWLAT INTEGER,"
	       "SWLON INTEGER,"
	       "NELAT INTEGER,"
	       "NELON BIGINT,"
	       "ELEVATION INTEGER,"
	       "TYPECODE SMALLINT,"
	       "FRULES SMALLINT,"
	       "TILE INTEGER,"
	       "PRIMARY KEY (ID)"
	       ") INHERITS (aviationdb.labelsourcecoordicaonameauthoritybase);");
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.airportrunways ("
	       "ID INTEGER NOT NULL,"
	       "ARPTID BIGINT NOT NULL REFERENCES aviationdb.airports (ID) ON DELETE CASCADE,"
	       "IDENTHE TEXT,"
	       "IDENTLE TEXT,"
	       "LENGTH INTEGER,"
	       "WIDTH INTEGER,"
	       "SURFACE TEXT,"
	       "HELAT INTEGER,"
	       "HELON INTEGER,"
	       "LELAT INTEGER,"
	       "LELON INTEGER,"
	       "HETDA INTEGER,"
	       "HELDA INTEGER,"
	       "HEDISP INTEGER,"
	       "HEHDG INTEGER,"
	       "HEELEV INTEGER,"
	       "LETDA INTEGER,"
	       "LELDA INTEGER,"
	       "LEDISP INTEGER,"
	       "LEHDG INTEGER,"
	       "LEELEV INTEGER,"
	       "FLAGS INTEGER,"
	       "HELIGHT0 SMALLINT,"
	       "HELIGHT1 SMALLINT,"
	       "HELIGHT2 SMALLINT,"
	       "HELIGHT3 SMALLINT,"
	       "HELIGHT4 SMALLINT,"
	       "HELIGHT5 SMALLINT,"
	       "HELIGHT6 SMALLINT,"
	       "HELIGHT7 SMALLINT,"
	       "LELIGHT0 SMALLINT,"
	       "LELIGHT1 SMALLINT,"
	       "LELIGHT2 SMALLINT,"
	       "LELIGHT3 SMALLINT,"
	       "LELIGHT4 SMALLINT,"
	       "LELIGHT5 SMALLINT,"
	       "LELIGHT6 SMALLINT,"
	       "LELIGHT7 SMALLINT,"
	       "PRIMARY KEY (ARPTID,ID)"
	       ");");
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.airporthelipads ("
	       "ID INTEGER NOT NULL,"
	       "ARPTID BIGINT NOT NULL REFERENCES aviationdb.airports (ID) ON DELETE CASCADE,"
	       "IDENT TEXT,"
	       "LENGTH INTEGER,"
	       "WIDTH INTEGER,"
	       "SURFACE TEXT,"
	       "LAT INTEGER,"
	       "LON INTEGER,"
	       "HDG INTEGER,"
	       "ELEV INTEGER,"
	       "FLAGS INTEGER,"
	       "PRIMARY KEY (ARPTID,ID)"
	       ");");
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.airportcomms ("
	       "ID INTEGER NOT NULL,"
	       "ARPTID BIGINT NOT NULL REFERENCES aviationdb.airports (ID) ON DELETE CASCADE,"
	       "NAME TEXT,"
	       "SECTOR TEXT,"
	       "OPRHOURS TEXT,"
	       "FREQ0 BIGINT,"
	       "FREQ1 BIGINT,"
	       "FREQ2 BIGINT,"
	       "FREQ3 BIGINT,"
	       "FREQ4 BIGINT,"
	       "PRIMARY KEY (ARPTID,ID)"
	       ");");
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.airportvfrroutes ("
	       "ID INTEGER NOT NULL,"
	       "ARPTID BIGINT NOT NULL REFERENCES aviationdb.airports (ID) ON DELETE CASCADE,"
	       "NAME TEXT,"
	       "MINALT INTEGER,"
	       "MAXALT INTEGER,"
	       "PRIMARY KEY (ARPTID,ID)"
	       ");");
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.airportvfrroutepoints ("
	       "ID INTEGER NOT NULL,"
	       "ARPTID BIGINT NOT NULL,"
	       "ROUTEID INTEGER NOT NULL,"
	       "NAME TEXT,"
	       "LAT INTEGER,"
	       "LON INTEGER,"
	       "ALT INTEGER,"
	       "ALTCODE SMALLINT,"
	       "PATHCODE SMALLINT,"
	       "SYM SMALLINT,"
	       "ATAIRPORT BOOL,"
	       "LABELPLACEMENT INTEGER,"
	       "FOREIGN KEY (ARPTID, ROUTEID) REFERENCES aviationdb.airportvfrroutes (ARPTID, ID) ON DELETE CASCADE,"
	       "PRIMARY KEY (ARPTID,ROUTEID,ID)"
	       ");");
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.airportlinefeatures ("
	       "ID INTEGER NOT NULL,"
	       "ARPTID BIGINT NOT NULL REFERENCES aviationdb.airports (ID) ON DELETE CASCADE,"
	       "NAME TEXT,"
	       "SURFACE TEXT,"
	       "FLAGS INTEGER,"
	       "POLY BYTEA,"
	       "PRIMARY KEY (ARPTID,ID)"
	       ");");
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.airportpointfeatures ("
	       "ID INTEGER NOT NULL,"
	       "ARPTID BIGINT NOT NULL REFERENCES aviationdb.airports (ID) ON DELETE CASCADE,"
	       "FEATURE SMALLINT,"
	       "LAT INTEGER,"
	       "LON INTEGER,"
	       "HDG INTEGER,"
	       "ELEV INTEGER,"
	       "SUBTYPE INTEGER,"
	       "ATTR1 INTEGER,"
	       "ATTR2 INTEGER,"
	       "NAME TEXT,"
	       "RWYIDENT TEXT,"
	       "PRIMARY KEY (ARPTID,ID)"
	       ");");
	w.exec("CREATE TABLE IF NOT EXISTS aviationdb.airportfas ("
	       "ID INTEGER NOT NULL,"
	       "ARPTID BIGINT NOT NULL REFERENCES aviationdb.airports (ID) ON DELETE CASCADE,"
	       "REFPATHID TEXT,"
	       "FTPLAT INTEGER,"
	       "FTPLON INTEGER,"
	       "FPAPLAT INTEGER,"
	       "FPAPLON INTEGER,"
	       "OPTYP SMALLINT,"
	       "RWY SMALLINT,"
	       "RTE SMALLINT,"
	       "REFPATHDS SMALLINT,"
	       "FTPHEIGHT SMALLINT,"
	       "APCHTCH INTEGER,"
	       "GPA INTEGER,"
	       "CWIDTH SMALLINT,"
	       "DLENOFFS SMALLINT,"
	       "HAL SMALLINT,"
	       "VAL SMALLINT,"
	       "PRIMARY KEY (ARPTID,ID)"
	       ");");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Airport>::drop_indices(void)
{
	pqxx::work w(m_conn);
	w.exec("DROP INDEX IF EXISTS aviationdb.airports_srcid;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airports_icao;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airports_name;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airports_latlon;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airports_bbox;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airports_modified;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airportrunways_id;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airporthelipads_id;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airportcomms_id;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airportvfrroutes_id;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airportvfrroutepoints_id;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airportvfrroutepoints_name;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airportvfrroutepoints_latlon;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airports_tile;");
	// old indices
	w.exec("DROP INDEX IF EXISTS aviationdb.airports_lat;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airports_lon;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airports_swlat;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airports_swlon;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airports_nelat;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airports_nelon;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airportvfrroutepoints_lat;");
	w.exec("DROP INDEX IF EXISTS aviationdb.airportvfrroutepoints_lon;");
	w.commit();
}

template<> void PGDbBase<DbBaseElements::Airport>::create_indices(void)
{
	pqxx::work w(m_conn);
	w.exec("CREATE INDEX IF NOT EXISTS airports_srcid ON aviationdb.airports(SRCID);");
	w.exec("CREATE INDEX IF NOT EXISTS airports_icao ON aviationdb.airports((ICAO::ext.CITEXT) COLLATE \"C\");");
	w.exec("CREATE INDEX IF NOT EXISTS airports_name ON aviationdb.airports((NAME::ext.CITEXT) COLLATE \"C\");");
	w.exec("CREATE INDEX IF NOT EXISTS airports_latlon ON aviationdb.airports(LAT,LON);");
	w.exec("CREATE INDEX IF NOT EXISTS airports_bbox ON aviationdb.airports(SWLAT,NELAT,SWLON,NELON);");
	w.exec("CREATE INDEX IF NOT EXISTS airports_modified ON aviationdb.airports(MODIFIED);");
	w.exec("CREATE INDEX IF NOT EXISTS airportrunways_id ON aviationdb.airportrunways(ARPTID,ID);");
	w.exec("CREATE INDEX IF NOT EXISTS airporthelipads_id ON aviationdb.airporthelipads(ARPTID,ID);");
	w.exec("CREATE INDEX IF NOT EXISTS airportcomms_id ON aviationdb.airportcomms(ARPTID,ID);");
	w.exec("CREATE INDEX IF NOT EXISTS airportvfrroutes_id ON aviationdb.airportvfrroutes(ARPTID,ID);");
	w.exec("CREATE INDEX IF NOT EXISTS airportvfrroutepoints_id ON aviationdb.airportvfrroutepoints(ARPTID,ROUTEID,ID);");
	w.exec("CREATE INDEX IF NOT EXISTS airportvfrroutepoints_name ON aviationdb.airportvfrroutepoints((NAME::ext.CITEXT) COLLATE \"C\");");
	w.exec("CREATE INDEX IF NOT EXISTS airportvfrroutepoints_latlon ON aviationdb.airportvfrroutepoints(LAT,LON);");
	w.exec("CREATE INDEX IF NOT EXISTS airports_tile ON aviationdb.airports(TILE);");
	// old indices
	w.exec("CREATE INDEX IF NOT EXISTS airports_swlat ON aviationdb.airports(SWLAT);");
	w.exec("CREATE INDEX IF NOT EXISTS airports_swlon ON aviationdb.airports(SWLON);");
	w.exec("CREATE INDEX IF NOT EXISTS airports_nelat ON aviationdb.airports(NELAT);");
	w.exec("CREATE INDEX IF NOT EXISTS airports_nelon ON aviationdb.airports(NELON);");
	w.commit();
}

template<> const char *PGDbBase<DbBaseElements::Airport>::main_table_name = "aviationdb.airports";
template<> const char *PGDbBase<DbBaseElements::Airport>::order_field = "SRCID";
template<> const char *PGDbBase<DbBaseElements::Airport>::delete_field = "SRCID";
template<> const bool PGDbBase<DbBaseElements::Airport>::area_data = true;

#endif
