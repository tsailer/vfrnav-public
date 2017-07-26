#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <iomanip>

#include "cfmuautoroute.hh"
#include "icaofpl.h"

bool CFMUAutoroute::start_ifr_specialcase(void)
{
	return start_ifr_specialcase_pogo();
}

bool CFMUAutoroute::start_ifr_specialcase_pogo(void)
{
	if (!get_departure_ifr() || !get_destination_ifr())
		return false;
	if (!IcaoFlightPlan::is_route_pogo(get_departure().get_icao(), get_destination().get_icao()))
		return false;
	while (m_route.get_nrwpt())
		m_route.delete_wpt(0);
	m_routetime = 0;
	m_routefuel = 0;
	m_route.insert(get_departure(), ~0U);
	m_route.insert(get_destination(), ~0U);
	m_route[0].set_pathcode(FPlanWaypoint::pathcode_directto);
	m_route[0].set_flags(m_route[0].get_flags() | FPlanWaypoint::altflag_ifr);
	m_route[1].set_flags(m_route[1].get_flags() | FPlanWaypoint::altflag_ifr);
	updatefpl();
	m_done = true;
	m_signal_log(log_fplproposal, get_simplefpl());
	stop(statusmask_none);
	return true;
}
