//
// C++ Implementation: Navigation
//
// Description: Navigation
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include "sensors.h"
#include "fplan.h"
#include "wind.h"
#include "prefs.h"

constexpr double Sensors::track_to_deg;
constexpr double Sensors::deg_to_track;
constexpr double Sensors::track_to_rad;
constexpr double Sensors::rad_to_track;

void Sensors::set_acft_cruise(const Aircraft::Cruise::EngineParams& x)
{
	m_cruise = x;
	get_configfile().set_double("main", "cruiserpm", m_cruise.get_rpm());
	get_configfile().set_double("main", "cruisemp", m_cruise.get_mp());
	get_configfile().set_double("main", "cruisebhp", m_cruise.get_bhp());
	save_config();
	update_sensors_core(change_altitude);
}

void Sensors::set_acft_iasmode(iasmode_t iasmode)
{
	m_iasmode = iasmode;
	get_configfile().set_integer("main", "autoias", (unsigned int)m_iasmode);
	save_config();
}

Aircraft::Cruise::EngineParams Sensors::get_acft_cruise_params(void) const
{
	Aircraft::Cruise::EngineParams r(get_acft_cruise());
	bool constspeed(get_aircraft().is_constantspeed());
	if (get_acft_iasmode() == Sensors::iasmode_bhp_rpm && !constspeed)
		r.set_rpm();
	if (get_acft_iasmode() == Sensors::iasmode_bhp_rpm || !constspeed)
		r.set_mp();
	if (get_acft_iasmode() != Sensors::iasmode_bhp_rpm)
		r.set_bhp();
	return r;
}

void Sensors::nav_fplan_modified(void)
{
	if (!m_route.is_dirty())
		return;
	recompute_wptextra();
	if (m_route.get_nrwpt())
		m_route.save_fp();
	m_sensorschange(change_fplan);
}

void Sensors::nav_fplan_setdeparture(time_t offblock, unsigned int taxitime)
{
	m_route.set_time_offblock_unix(offblock);
	if (m_route.get_nrwpt())
		m_route[0].set_time_unix(offblock + taxitime);
	recompute_wptextra();
	recompute_times(1);
	m_sensorschange(change_fplan);
}

void Sensors::nav_load_fplan(FPlanRoute::id_t id)
{
	close_track();
	m_route.load_fp(id);
	recompute_wptextra();
	m_sensorschange(change_fplan);
}

void Sensors::nav_new_fplan(void)
{
	close_track();
	m_route.new_fp();
}

void Sensors::nav_duplicate_fplan(void)
{
	m_route.duplicate_fp();
}

void Sensors::nav_delete_fplan(void)
{
	close_track();
	m_route.delete_fp();
}

void Sensors::nav_reverse_fplan(void)
{
	m_route.reverse_fp();
	recompute_wptextra();
}

void Sensors::nav_insert_wpt(uint32_t nr, const FPlanWaypoint& wpt)
{
	m_route.insert_wpt(nr, wpt);
	recompute_wptextra();
}

void Sensors::nav_delete_wpt(uint32_t nr)
{
	m_route.delete_wpt(nr);
	recompute_wptextra();
}

void Sensors::nav_set_brg2(const Glib::ustring& dest)
{
	m_navbrg2dest = dest;
	if (m_navbrg2ena)
		m_sensorschange(change_navigation);
}

void Sensors::nav_set_brg2(const Point& pt)
{
	m_navbrg2 = pt;
	if (m_navbrg2ena)
		if (update_navigation() & change_navigation)
			m_sensorschange(change_navigation);
}

void Sensors::nav_set_brg2(bool enable)
{
	if (m_navbrg2ena == enable)
		return;
	m_navbrg2ena = enable;
	update_navigation();
	m_sensorschange(change_navigation);
}

void Sensors::nav_set_hold(double track)
{
	if (std::isnan(track)) {
		nav_set_hold(false);
		return;
	}
	m_navholdtrack = track * deg_to_track;
	if (m_navmode == navmode_fpl)
		m_navmode = navmode_fplhold;
	m_sensorschange(change_navigation);
}

void Sensors::nav_set_hold(bool enable)
{
	switch (m_navmode) {
	case navmode_fpl:
		if (!enable)
			break;
		m_navmode = navmode_fplhold;
		m_sensorschange(change_navigation);
		break;

	case navmode_fplhold:
		if (enable)
			break;
		m_navmode = navmode_fpl;
		m_sensorschange(change_navigation);
		break;

	case navmode_direct:
		if (!enable)
			break;
		m_navmode = navmode_directhold;
		m_sensorschange(change_navigation);
		break;

	case navmode_directhold:
		if (enable)
			break;
		m_navmode = navmode_direct;
		m_sensorschange(change_navigation);
		break;

	default:
		break;
	}
}

void Sensors::nav_nearestleg(void)
{
	if (m_route.get_nrwpt() <= 0)
		return;
	unsigned int nrsti(0);
	double nrstdist(m_position.spheric_distance_nmi_dbl(m_route[0].get_coord()));
	for (unsigned int i = 1; i < m_route.get_nrwpt(); ++i) {
		double d(m_position.spheric_distance_nmi_dbl(m_route[i].get_coord()));
		if (d > nrstdist)
			continue;
		nrsti = i;
		nrstdist = d;
	}
	double track(std::numeric_limits<double>::quiet_NaN());
	if (nrsti > 0)
		track = m_route[nrsti - 1].get_coord().spheric_true_course(m_route[nrsti].get_coord());
	nav_directto(m_route[nrsti].get_icao_name(), m_route[nrsti].get_coord(), track, m_route[nrsti].get_altitude(), m_route[nrsti].get_flags(), nrsti + 1);
}

void Sensors::nav_directto(const Glib::ustring& dest, const Point& coord, double track, int32_t alt, uint16_t altflags, uint16_t wptnr)
{
	if (std::isnan(track))
		track = m_position.spheric_true_course(coord);
	m_navcurdest = dest;
	m_navcur = m_navnext = coord;
	m_navcuralt = alt;
	m_navcuraltflags = altflags;
	m_navcurtrack = m_navnexttrack = m_navholdtrack = track * deg_to_track;
	m_navcurwpt = ~0U;
	m_navmode = navmode_direct;
	m_navblock = m_position.simple_box_km(0.02);
	if (wptnr <= m_route.get_nrwpt()) {
		m_navcurwpt = wptnr;
		m_navmode = navmode_fpl;
		++wptnr;
		if (wptnr <= m_route.get_nrwpt()) {
			const FPlanWaypoint& wpt(m_route[wptnr - 1]);
			m_navnext = wpt.get_coord();
			m_navnexttrack = m_navcur.spheric_true_course(m_navnext) * deg_to_track;
		}
		if (m_navcurwpt >= 1) {
			if (m_positionok && !std::isnan(m_groundspeed)) {
				m_navcurdist = m_position.spheric_distance_nmi_dbl(m_navcur);
				FPlanWaypoint& wpt(m_route[m_navcurwpt - 1]);
				wpt.save_time();
				wpt.set_time_unix(time(0) + m_navcurdist / std::max(m_groundspeed, 10.0) * 3600.0);
				recompute_times(m_navcurwpt);
			}
		} else {
			m_route.save_time_offblock();
			m_route.set_time_offblock_unix(time(0));
			if (m_route.get_nrwpt() > 0) {
				FPlanWaypoint& wpt(m_route[0]);
				wpt.save_time();
				wpt.set_time_unix(m_route.get_time_offblock_unix() + 5 * 60);
				recompute_times(1);
			}
		}
	} else if (wptnr == m_route.get_nrwpt() + 1) {
		m_navcurwpt = wptnr;
		m_navmode = navmode_fpl;
		m_route.save_time_onblock();
		m_route.set_time_onblock_unix(time(0));
	}
	m_sensorschange((update_navigation() & change_navigation) | change_fplan);
}

void Sensors::nav_finalapproach(const AirportsDb::Airport::FASNavigate& fas)
{
	if (!fas.is_valid())
		return;
	m_navfas = fas;
	m_navcurdest = fas.get_airport() + " " + fas.get_ident();
	m_navcur = m_navnext = fas.get_fpap();
	m_navcuralt = fas.get_thralt();
	m_navcuraltflags = 0;
	m_navcurtrack = m_navnexttrack = m_navholdtrack = fas.get_course();
	m_navcurwpt = ~0U;
	m_navmode = navmode_finalapproach;
	m_navblock = m_position.simple_box_km(0.02);
	m_sensorschange(update_navigation() & change_navigation);
}

Sensors::fplanwind_t Sensors::get_fplan_wind(void) const
{
	if ((m_navmode == navmode_fpl || m_navmode == navmode_fplhold) &&
	    m_navcurwpt >= 1 && m_navcurwpt <= m_route.get_nrwpt()) {
		const FPlanWaypoint& wpt(m_route[m_navcurwpt - 1]);
		return fplanwind_t(wpt.get_winddir_deg(), wpt.get_windspeed_kts());
	}
	return get_fplan_wind(m_position);
}

Sensors::fplanwind_t Sensors::get_fplan_wind(const Point& coord) const
{
	fplanwind_t r(0, 0);
	uint64_t mindist(std::numeric_limits<uint64_t>::max());
	for (unsigned int i = 0; i < m_route.get_nrwpt(); ++i) {
		const FPlanWaypoint& wpt(m_route[i]);
		uint64_t dist(coord.simple_distance_rel(wpt.get_coord()));
		if (dist > mindist)
			continue;
		mindist = dist;
		r.first = wpt.get_winddir_deg();
		r.second = wpt.get_windspeed_kts();
	}
	return r;
}

void Sensors::recompute_wptextra(void)
{
	m_route.recompute(get_aircraft(), get_altimeter_qnh(), get_altimeter_tempoffs(), get_acft_cruise_params());
}

void Sensors::recompute_times(unsigned int wptnr)
{
	if (true) {
		recompute_wptextra();
		for (; wptnr < m_route.get_nrwpt(); ++wptnr) {
			FPlanWaypoint& wpt(m_route[wptnr]);
			if (!wptnr) {
				wpt.set_time_unix(time(0));
				continue;
			}
			const FPlanWaypoint& wpto(m_route[wptnr - 1]);
			wpt.set_time_unix(wpto.get_time_unix() + wpt.get_flighttime() - wpto.get_flighttime());
		}
		if (wptnr == m_route.get_nrwpt() && m_route.get_nrwpt()) {
			FPlanWaypoint& wpt(m_route[m_route.get_nrwpt() - 1]);
			m_route.set_time_onblock_unix(wpt.get_time_unix() + 5U * 60U);
		}
		return;
	}
	double smul(m_groundspeed);
	if (std::isnan(smul) || smul < m_tkoffldgspeed) {
		if (m_engine)
			smul = m_engine->get_prefs().vcruise;
		else
			smul = 100;
	}
	smul = std::max(smul, 10.0);
	smul = 3600 / smul;
	for (; wptnr < m_route.get_nrwpt(); ++wptnr) {
		FPlanWaypoint& wpt(m_route[wptnr]);
		if (!wptnr) {
			wpt.set_time_unix(time(0));
			continue;
		}
		const FPlanWaypoint& wpto(m_route[wptnr - 1]);
		double dist(wpto.get_coord().spheric_distance_nmi_dbl(wpt.get_coord()));
		wpt.set_time_unix(wpto.get_time_unix() + smul * dist);
		if (false)
			std::cerr << "recompute_times[" << wptnr << "] " << wpt.get_icao_name() << " prevtime " << wpto.get_time_unix()
				  << " dist " << dist << " mul " << smul << std::endl;
	}
	if (wptnr == m_route.get_nrwpt() && m_route.get_nrwpt()) {
		FPlanWaypoint& wpt(m_route[m_route.get_nrwpt() - 1]);
		m_route.set_time_onblock_unix(wpt.get_time_unix() + 5 * 60);
	}
}

void Sensors::nav_off(void)
{
	if (m_navmode == navmode_off)
		return;
	m_navcurdist = m_navcurxtk = m_navroc = m_navtimetowpt = std::numeric_limits<double>::quiet_NaN();
	m_navcurtrackerr = m_navnexttrackerr = 0;
	m_sensorschange(change_navigation | change_fplan);
}

double Sensors::nav_get_track_true(void) const
{
	if (!nav_is_on())
		return std::numeric_limits<double>::quiet_NaN();
	return m_navcurtrack * track_to_deg;
}

double Sensors::nav_get_track_mag(void) const
{
	double t(nav_get_track_true());
	if (std::isnan(t))
		return t;
	t -= get_declination();
	if (t < 0)
		t += 360;
	else if (t >= 360)
		t -= 360;
	return t;
}

time_t Sensors::nav_get_flighttime(void) const
{
	if (m_navflighttimerunning)
		return time(0) - m_navflighttime;
	return m_navflighttime;
}

void Sensors::close_track(void)
{
        if (!m_track.is_valid())
		return;
	if (m_engine) {
		m_track.set_modtime(time(0));
		m_engine->get_tracks_db().save(m_track);
        }
        m_track = TracksDb::Track();
}

Sensors::change_t Sensors::update_navigation(change_t changemask)
{
	if (changemask & change_position) {
		changemask |= change_navigation;
		if (!m_navbrg2ena || !m_positionok) {
			m_navbrg2tracktrue = m_navbrg2trackmag = m_navbrg2dist = std::numeric_limits<double>::quiet_NaN();
		} else {
			m_navbrg2tracktrue = m_position.spheric_true_course(m_navbrg2);
			m_navbrg2trackmag = m_navbrg2tracktrue - get_declination();
			if (m_navbrg2trackmag < 0)
				m_navbrg2trackmag += 360;
			else if (m_navbrg2trackmag >= 360)
				m_navbrg2trackmag -= 360;
			m_navbrg2dist = m_position.spheric_distance_nmi_dbl(m_navbrg2);
		}
		if (m_navmode == navmode_off || !m_positionok) {
			m_navcurdist = m_navcurxtk = m_navroc = m_navtimetowpt = std::numeric_limits<double>::quiet_NaN();
			m_navcurtrackerr = m_navnexttrackerr = 0;
		} else {
			if ((m_navmode == navmode_fpl || m_navmode == navmode_fplhold) && m_navcurwpt < 1) {
				if (!m_navblock.is_inside(m_position)) {
					changemask |= change_fplan;
					m_navlastwpttime = time(0);
					m_route.set_time_offblock_unix(m_navlastwpttime);
					m_navcurwpt = 1;
					if (m_route.get_nrwpt() > 0) {
						FPlanWaypoint& wpt(m_route[0]);
						wpt.save_time();
						wpt.set_time_unix(m_route.get_time_offblock_unix() + 5 * 60);
						recompute_times(1);
						m_navcurdest = wpt.get_icao_name();
						m_navcur = m_navnext = wpt.get_coord();
						m_navcuralt = wpt.get_altitude();
						m_navcuraltflags = wpt.get_flags();
						m_navcurtrack = m_navnexttrack = m_navholdtrack = 0;
						if (m_route.get_nrwpt() >= 2) {
							const FPlanWaypoint& wpt1(m_route[1]);
							m_navnext = wpt1.get_coord();
							m_navnexttrack = m_navcur.spheric_true_course(m_navnext) * deg_to_track;
						}
						// open track
						m_track = TracksDb::Track();
						m_track.make_valid();
						m_track.set_sourceid(NewWaypointWindow::make_sourceid());
						{
							const FPlanWaypoint& wpt0(m_route[0]);
							const FPlanWaypoint& wpt1(m_route[m_route.get_nrwpt()-1]);
							m_track.set_fromicao(wpt0.get_icao());
							m_track.set_fromname(wpt0.get_name());
							m_track.set_toicao(wpt1.get_icao());
							m_track.set_toname(wpt1.get_name());
						}
						m_track.set_notes(m_route.get_note());
						m_track.set_offblocktime(m_navlastwpttime);
						m_track.set_takeofftime(m_navlastwpttime);
						m_track.set_landingtime(m_navlastwpttime);
						m_track.set_onblocktime(m_navlastwpttime);
						m_track.set_modtime(m_navlastwpttime);
					}
				}
			} else if ((m_navmode == navmode_fpl || (m_navmode == navmode_fplhold && m_navcurwpt <= 1)) && m_navcurwpt < m_route.get_nrwpt()) {
				bool nextwpt(false);
				time_t deltat(0);
				if (m_navcurwpt <= 1) {
					nextwpt = !std::isnan(m_groundspeed) && m_groundspeed > m_tkoffldgspeed;
					if (nextwpt) {
						m_navflighttime = time(0);
						m_navflighttimerunning = true;
						if (m_track.is_valid()) {
							m_track.set_takeofftime(m_navflighttime);
							m_track.set_landingtime(m_navflighttime);
							m_track.set_onblocktime(m_navflighttime);
						}
					}
				} else {
					m_navcurdist = m_position.spheric_distance_nmi_dbl(m_navcur);
					uint32_t trkcur(m_position.spheric_true_course(m_navcur) * deg_to_track);
					m_navcurtrackerr = m_navcurtrack - trkcur;
					// try to fit a circle segment into triangle m_position - m_navcur - m_navnext
					// determine distance from m_navcur to advance such that a smooth standard rate turn
					// will exactly intercept the new track
					int32_t alpha(m_navnexttrack - trkcur + (1 << 31));
					alpha = std::max(abs(alpha), (int32_t)(2*deg_to_track));
					double stdturnradius(m_groundspeed);
					if (std::isnan(stdturnradius))
						stdturnradius = 0;
					stdturnradius = std::max(stdturnradius, 10.0);
					stdturnradius *= (1.0 / 60.0 / M_PI);
					double distadv(stdturnradius);
					{
						// do not divide by tangent to avoid numeric problems
						double angle(alpha * (0.5 * track_to_rad));
						distadv *= cos(angle) / sin(angle);
					}
					nextwpt = m_navcurdist <= distadv;
					if (nextwpt) {
						// correct for time required to fly half the arc;
						// the time stored for the current waypoint is the current time
						// plus time for half the arc
						double dt(alpha * (track_to_rad / M_PI));
						dt = 1 - dt;
						dt *= 60;
						if (!std::isnan(m_groundspeed) && m_groundspeed > 10)
							dt = std::min(dt,  m_position.spheric_distance_nmi_dbl(m_navnext) / m_groundspeed * (3600 * 0.5));
						deltat = Point::round<time_t,double>(dt);
					}
				}
				if (nextwpt) {
					// advance waypoint
					changemask |= change_fplan;
					m_navlastwpttime = time(0);
					m_route[m_navcurwpt - 1].save_time();
					m_route[m_navcurwpt - 1].set_time_unix(m_navlastwpttime + deltat);
					m_navcuralt = m_route[m_navcurwpt].get_altitude();
					m_navcuraltflags = m_route[m_navcurwpt].get_flags();
					m_navcurdest = m_route[m_navcurwpt].get_icao_name();
					m_navcurtrack = m_navholdtrack = m_navnexttrack;
					m_navcur = m_navnext;
					++m_navcurwpt;
					if (m_navcurwpt < m_route.get_nrwpt()) {
						m_navnext = m_route[m_navcurwpt].get_coord();
						m_navnexttrack = m_navcur.spheric_true_course(m_navnext) * deg_to_track;
					}
					recompute_times(m_navcurwpt - 1);
				}
			} else if ((m_navmode == navmode_fpl || m_navmode == navmode_fplhold) && m_navcurwpt > m_route.get_nrwpt()) {
				if (!m_navblock.is_inside(m_position)) {
					changemask |= change_fplan;
					m_navlastwpttime = time(0);
					m_route.set_time_onblock_unix(m_navlastwpttime);
					m_navblock = m_position.simple_box_km(0.02);
					if (m_track.is_valid())
						m_track.set_onblocktime(m_navlastwpttime);
				}
			} else if ((m_navmode == navmode_fpl || m_navmode == navmode_fplhold) && !std::isnan(m_groundspeed) &&
				   m_groundspeed < m_tkoffldgspeed && m_route.get_nrwpt() > 0) {
				changemask |= change_fplan;
				m_route[m_navcurwpt - 1].save_time();
				m_route[m_navcurwpt - 1].set_time_unix(time(0));
				m_navlastwpttime = time(0);
				m_route.save_time_onblock();
				m_route.set_time_onblock_unix(m_navlastwpttime);
				m_navblock = m_position.simple_box_km(0.02);
				++m_navcurwpt;
				if (m_navflighttimerunning)
					m_navflighttime = time(0) - m_navflighttime;
				m_navflighttimerunning = false;
				if (m_track.is_valid()) {
					m_track.set_landingtime(m_navlastwpttime);
					m_track.set_onblocktime(m_navlastwpttime);
				}
			} else if ((m_navmode == navmode_fpl || m_navmode == navmode_fplhold ||
				    m_navmode == navmode_direct || m_navmode == navmode_directhold) && 
				   (abs(m_navcurtrackerr) >= (1 << 30))) {
				m_navcurtrack = m_navholdtrack;
				if (m_navmode == navmode_direct) {
					m_navmode = navmode_off;
				} else if ((m_navmode == navmode_fpl || m_navmode == navmode_fplhold) && m_navcurwpt < m_route.get_nrwpt()) {
					changemask |= change_fplan;
					m_navlastwpttime = time(0);
					m_route[m_navcurwpt - 1].save_time();
					m_route[m_navcurwpt - 1].set_time_unix(m_navlastwpttime);
					recompute_times(m_navcurwpt);
				}
			}
			if (m_navmode == navmode_finalapproach) {
				m_navcurdist = m_position.spheric_distance_nmi_dbl(m_navfas.get_ftp());
				double dist(m_position.spheric_distance_nmi_dbl(m_navfas.get_fpap()));
				m_navcurtrackerr = m_navcurtrack - (m_position.spheric_true_course(m_navfas.get_fpap()) * deg_to_track);
				m_navcurxtk = sin(m_navcurtrackerr * track_to_rad) * dist;
				dist *= fabs(cos(m_navcurtrackerr * track_to_rad));
				double maxxtk(dist * tan(m_navfas.get_coursedefl_rad()));
				m_navmaxxtk = std::min(std::max(maxxtk, 0.001), 1.);
				double baroalt, truealt;
				get_altitude(baroalt, truealt);
				dist = m_navcurdist * (1000.0 / Point::km_to_nmi_dbl * Point::m_to_ft_dbl);
				m_navgs = (m_navfas.get_glidepath_rad() - atan2(truealt - m_navfas.get_thralt(), dist)) / m_navfas.get_glidepathdefl_rad();
			} else {
				m_navcurdist = m_position.spheric_distance_nmi_dbl(m_navcur);
				m_navcurtrackerr = m_navcurtrack - (m_position.spheric_true_course(m_navcur) * deg_to_track);
				m_navcurxtk = sin(m_navcurtrackerr * track_to_rad) * m_navcurdist;
				m_navmaxxtk = 1.0;
				m_navgs = std::numeric_limits<double>::quiet_NaN();
			}
			if (std::isnan(m_groundspeed) || m_groundspeed < 1.0) {
				m_navroc = m_navtimetowpt = std::numeric_limits<double>::quiet_NaN();
			} else {
				m_navtimetowpt = m_navcurdist / m_groundspeed * 3600.0;
				double baroalt, truealt;
				get_altitude(baroalt, truealt);
				double altdiff(m_navcuralt - ((m_navcuraltflags & FPlanWaypoint::altflag_standard) ? baroalt : truealt));
				if (std::isnan(altdiff) || m_navtimetowpt < 10 || m_navcuralt == std::numeric_limits<int32_t>::min()) {
					m_navroc = std::numeric_limits<double>::quiet_NaN();
				} else {
					m_navroc = altdiff * 60.0 / m_navtimetowpt;
				}
			}
		}
		// Update Track
		if (m_positionok && m_engine && m_track.is_valid()) {
			bool doadd(!m_track.size());
			if (!doadd) {
				Glib::TimeVal tv(m_track[m_track.size()].get_timeval());
				tv -= m_positiontime;
				tv.add_milliseconds(1500);
				doadd = tv.negative();
			}
			if (doadd) {
				TracksDb::Track::TrackPoint tp;
				tp.set_timeval(m_positiontime);
				tp.set_coord(m_position);
				double alt(m_airdata.get_true_altitude());
				if (std::isnan(alt))
					tp.set_alt_invalid();
				else
					tp.set_alt((TracksDb::Track::TrackPoint::alt_t)alt);
				m_track.append(tp);
				if (m_track.get_unsaved_points() >= 60)
					m_engine->get_tracks_db().save_appendlog(m_track);
			}
		}
	}
	{
		Glib::TimeVal tv;
		tv.assign_current_time();
		std::swap(m_fueltime, tv);
		tv -= m_fueltime;
		m_fuel -= (1.0 / 3600.0) * m_fuelflow * tv.as_double();
	}
	if (changemask & (change_altitude | change_course)) {
		double pa(m_airdata.get_pressure_altitude()), tas(0), fuelflow(0), mass(m_aircraft.get_mtom()), isaoffs(m_airdata.get_tempoffs());
		Aircraft::Cruise::EngineParams ep(get_acft_cruise_params());
	        m_aircraft.get_cruise().calculate(m_aircraft.get_propulsion(), tas, fuelflow, pa, mass, isaoffs, ep);
		if (!std::isnan(m_groundspeed) && m_groundspeed > m_tkoffldgspeed) {
			if (m_altrate > 150) {
				// FIXME: mass
				Aircraft::ClimbDescent c(m_aircraft.calculate_climb("", mass, isaoffs));
				double t(c.altitude_to_time(pa));
				tas = c.time_to_tas(t);
				fuelflow = c.time_to_fuelflow(t);
				ep.set_bhp();
			}
			if (std::isnan(fuelflow))
				fuelflow = 0;
			m_fuelflow = fuelflow;
			if (m_iasmode != iasmode_manual && !std::isnan(tas)) {
				m_airdata.set_tas(tas);
				changemask |= change_airdata;
			}
			m_bhp = ep.get_bhp();
		}
	}
	return changemask;
}
