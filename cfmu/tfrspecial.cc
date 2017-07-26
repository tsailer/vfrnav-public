#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iomanip>

#include "tfr.hh"
#include "icaofpl.h"

bool TrafficFlowRestrictions::check_fplan_specialcase_pogo(void)
{
	if (m_route.size() != 2)
		return false;
	if (!m_route.front().is_ifr() || !m_route.back().is_ifr())
		return false;
	if (m_route.front().get_pathcode() != FPlanWaypoint::pathcode_directto)
		return false;
	if (!IcaoFlightPlan::is_route_pogo(m_route.front().get_icao(), m_route.back().get_icao()))
		return false;
	return true;
}

bool TrafficFlowRestrictions::check_fplan_specialcase(void)
{
	return check_fplan_specialcase_pogo();
}
