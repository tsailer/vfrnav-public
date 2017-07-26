#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <limits>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <set>
#include <time.h>
#include "wmm.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/connected_components.hpp>

#include "adrfplan.hh"
#include "icaofpl.h"

using namespace ADR;

FPLWaypoint::FPLWaypoint(void)
	: FPlanWaypoint(), m_wptnr(0), m_flags(0)
{
}

FPLWaypoint::FPLWaypoint(const FPlanWaypoint& w)
	:FPlanWaypoint(w), m_wptnr(0), m_flags(0)
{
}

const UUID& FPLWaypoint::get_ptuuid(void) const
{
	const Object::const_ptr_t& p(get_ptobj());
	if (!p)
		return UUID::niluuid;
	return p->get_uuid();
}

const UUID& FPLWaypoint::get_pathuuid(void) const
{
	const Object::const_ptr_t& p(get_pathobj());
	if (!p)
		return UUID::niluuid;
	return p->get_uuid();
}

void FPLWaypoint::set_from_objects(bool term)
{
	{
		const PointIdentTimeSlice& tspt(get_ptobj()->operator()(get_time_unix()).as_point());
		if (tspt.is_valid()) {
			set_coord(tspt.get_coord());
			if (term) {
				const ElevPointIdentTimeSlice& tselev(tspt.as_elevpoint());
				if (tselev.is_valid() && tselev.is_elev_valid()) {
					set_altitude(tselev.get_elev());
					frob_flags(0, altflag_standard);
				}
			}
			{
				const AirportTimeSlice& ts(tspt.as_airport());
				if (ts.is_valid()) {
					set_icao(ts.get_ident());
					set_name(ts.get_name());
					set_type(type_airport);
				}
			}
			{
				const NavaidTimeSlice& ts(tspt.as_navaid());
				if (ts.is_valid()) {
					set_icao(ts.get_ident());
					set_name(ts.get_name());
					switch (ts.get_type()) {
					case NavaidTimeSlice::type_vor:
						set_type(type_navaid, navaid_vor);
						break;

					case NavaidTimeSlice::type_vor_dme:
						set_type(type_navaid, navaid_vordme);
						break;

					case NavaidTimeSlice::type_vortac:
						set_type(type_navaid, navaid_vortac);
						break;

					case NavaidTimeSlice::type_tacan:
						set_type(type_navaid, navaid_tacan);
						break;

					case NavaidTimeSlice::type_ils_dme:
					case NavaidTimeSlice::type_loc_dme:
					case NavaidTimeSlice::type_dme:
						set_type(type_navaid, navaid_dme);
						break;

					case NavaidTimeSlice::type_ndb:
					case NavaidTimeSlice::type_ndb_mkr:
						set_type(type_navaid, navaid_ndb);
						break;

					case NavaidTimeSlice::type_ndb_dme:
						set_type(type_navaid, navaid_ndbdme);
						break;

					default:
						set_type(type_navaid, navaid_invalid);
						break;
					}
				}
			}
			{
				const DesignatedPointTimeSlice& ts(tspt.as_designatedpoint());
				if (ts.is_valid()) {
					set_icao("");
					set_name(ts.get_ident());
					set_type(type_intersection);
				}
			}
		}
	}
	if (!get_pathobj()) {
		if (is_stay())
			return;
		set_pathname("");
		set_pathcode(pathcode_directto);
		return;
	}
	{
		const TimeSlice& ts(get_pathobj()->operator()(get_time_unix()));
		if (!ts.is_valid()) {
			set_pathname("");
			set_pathcode(pathcode_none);
			return;
		}
		const SegmentTimeSlice& tsseg(ts.as_segment());
		if (!tsseg.is_valid()) {
			{
				const RouteTimeSlice& tsr(ts.as_route());
				if (tsr.is_valid()) {
					set_pathname(tsr.get_ident());
					set_pathcode(pathcode_airway);
					return;
				}
			}
			{
				const StandardInstrumentDepartureTimeSlice& tsr(ts.as_sid());
				if (tsr.is_valid()) {
					set_pathname(tsr.get_ident());
					set_pathcode(pathcode_sid);
					return;
				}
			}
			{
				const StandardInstrumentArrivalTimeSlice& tsr(ts.as_star());
				if (tsr.is_valid()) {
					set_pathname(tsr.get_ident());
					set_pathcode(pathcode_star);
					return;
				}
			}
			set_pathname("");
			set_pathcode(pathcode_none);
			return;
		}
		{
			const IdentTimeSlice& tsr(tsseg.get_route().get_obj()->operator()(get_time_unix()).as_ident());
			if (!tsr.is_valid()) {
				set_pathname("");
				set_pathcode(pathcode_none);
				return;
			}
			set_pathname(tsr.get_ident());
		}
	}
	{
		const RouteSegmentTimeSlice& ts(get_pathobj()->operator()(get_time_unix()).as_routesegment());
		if (ts.is_valid()) {
			set_pathcode(pathcode_airway);
			return;
		}
	}
	{
		const DepartureLegTimeSlice& ts(get_pathobj()->operator()(get_time_unix()).as_departureleg());
		if (ts.is_valid()) {
			set_pathcode(pathcode_sid);
			return;
		}
	}
	{
		const ArrivalLegTimeSlice& ts(get_pathobj()->operator()(get_time_unix()).as_arrivalleg());
		if (ts.is_valid()) {
			set_pathcode(pathcode_star);
			return;
		}
	}
	set_pathname("");
	set_pathcode(pathcode_none);
}

const Glib::ustring& FPLWaypoint::get_icao_or_name(void) const
{
	if (get_icao().empty())
		return get_name();
	return get_icao();
}

std::string FPLWaypoint::get_coordstr(void) const
{
	std::ostringstream oss;
	double c(get_lat() * Point::to_deg_dbl);
	char ch('N');
	if (c < 0) {
		c = -c;
		ch = 'S';
	}
	int ci(floor(c) + 0.01);
	c -= ci;
	c *= 60;
	oss << std::setw(2) << std::setfill('0') << ci;
	ci = floor(c + 0.5) + 0.01;
	oss << std::setw(2) << std::setfill('0') << ci << ch;
	c = get_lon() * Point::to_deg_dbl;
	ch = 'E';
	if (c < 0) {
		c = -c;
		ch = 'W';
	}
	ci = floor(c) + 0.01;
	c -= ci;
	c *= 60;
	oss << std::setw(3) << std::setfill('0') << ci;
	ci = floor(c + 0.5) + 0.01;
	oss << std::setw(2) << std::setfill('0') << ci << ch;
	return oss.str();	
}

bool FPLWaypoint::enforce_pathcode_vfrifr(void)
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

std::string FPLWaypoint::to_str(void) const
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
	oss << ' ' << (is_standard() ? 'F' : 'A');
	if (get_altitude() == std::numeric_limits<int32_t>::min())
		oss << "---";
	else
		oss << std::setfill('0') << std::setw(3) << ((get_altitude() + 50) / 100);
	oss << ' ' << (is_ifr() ? 'I' : 'V') << "FR "
	    << get_type_string() << ' ' << Glib::TimeVal(get_time_unix(), 0).as_iso8601();
	if (get_ptobj())
		oss << " point " << get_ptobj()->get_uuid();
	if (get_pathobj())
		oss << " path " << get_pathobj()->get_uuid();
	if (is_expanded())
		oss << " (E)";
	return oss.str();
}

bool FPLWaypoint::is_path_match(const Object::const_ptr_t& p) const
{
	if (!p)
		return !get_pathobj();
	if (!get_pathobj())
		return false;
	if (p->get_uuid() == get_pathobj()->get_uuid())
		return true;
	const SegmentTimeSlice& seg(get_pathobj()->operator()(get_time_unix()).as_segment());
	if (!seg.is_valid())
		return false;
	return p->get_uuid() == seg.get_route();
}

bool FPLWaypoint::is_path_match(const UUID& uuid) const
{
	if (!get_pathobj())
		return uuid.is_nil();
	if (uuid == get_pathobj()->get_uuid())
		return true;
	const SegmentTimeSlice& seg(get_pathobj()->operator()(get_time_unix()).as_segment());
	if (!seg.is_valid())
		return false;
	return uuid == seg.get_route();
}

FlightPlan::ParseWaypoint::ParseWaypoint(void)
	: FPLWaypoint(), m_course(-1), m_dist(-1)
{
	set_altitude(std::numeric_limits<int32_t>::min());
}

FlightPlan::ParseWaypoint::ParseWaypoint(const FPLWaypoint& wpt)
	: FPLWaypoint(wpt), m_course(-1), m_dist(-1)
{
}

FlightPlan::ParseWaypoint::ParseWaypoint(const FPlanWaypoint& wpt)
	: FPLWaypoint(wpt), m_course(-1), m_dist(-1)
{
}

void FlightPlan::ParseWaypoint::upcaseid(void)
{
	set_icao(upcase(get_icao()));
}

bool FlightPlan::ParseWaypoint::process_speedalt(uint16_t& flags, int32_t& alt, float& speed)
{
	speed = std::numeric_limits<float>::quiet_NaN();
	const Glib::ustring& icao(get_icao());
	Glib::ustring::size_type n(icao.find('/'));
	if (n == Glib::ustring::npos)
		return false;
	Glib::ustring::size_type i(n + 1);
	Glib::ustring::size_type nend(icao.size());
	char ch(icao[i]);
	if (ch == 'N' || ch == 'K' || ch == 'M') {
		bool mach(ch == 'M');
		if (i + 4 > nend || (!mach && i + 5 > nend) || !std::isdigit(icao[i + 1]) ||
		    !std::isdigit(icao[i + 2]) || !std::isdigit(icao[i + 3]) ||
		    (!mach && !std::isdigit(icao[i + 4]))) {
			std::string err("invalid speed: " + icao);
			set_icao(icao.substr(0, n));
			throw std::runtime_error(err);
		}
		int speedn((icao[i + 1] - '0') * 100 + (icao[i + 2] - '0') * 10 + (icao[i + 3] - '0'));
		if (!mach)
			speedn = speedn * 10 + (icao[i + 4] - '0');
		i += mach ? 4 : 5;
		float spd(1);
		if (ch == 'K')
			spd = Point::km_to_nmi;
		else if (mach)
			spd = 330.0 * Point::mps_to_kts; // very approximate!
		spd *= speedn;
		speed = spd;
	}
	ch = icao[i];
	if (ch == 'V') {
		if (i + 3 > nend || icao[i + 1] != 'F' ||  icao[i + 2] != 'R') {
			std::string err("invalid altitude: " + icao);
			set_icao(icao.substr(0, n));
			throw std::runtime_error(err);
		}
		i += 3;
		flags &= ~FPlanWaypoint::altflag_standard;
		alt = std::numeric_limits<int32_t>::min();
	} else if (ch == 'F' || ch == 'A' || ch == 'S' || ch == 'M') {
		bool metric(ch == 'S' || ch == 'M');
		if (i + 4 > nend || (metric && i + 5 > nend) || !std::isdigit(icao[i + 1]) ||
		    !std::isdigit(icao[i + 2]) || !std::isdigit(icao[i + 3]) ||
		    (metric && !std::isdigit(icao[i + 4]))) {
			std::string err("invalid altitude: " + icao);
			set_icao(icao.substr(0, n));
			throw std::runtime_error(err);
		}
		int altn((icao[i + 1] - '0') * 100 + (icao[i + 2] - '0') * 10 + (icao[i + 3] - '0'));
		if (metric)
			altn = altn * 10 + (icao[i + 4] - '0');
		i += metric ? 5 : 4;
		altn *= 100;
		if (metric)
			altn = Point::round<int,float>(altn * Point::m_to_ft);
		alt = altn;
		if (ch == 'F' || ch == 'S')
			flags |= FPlanWaypoint::altflag_standard;
		else
			flags &= ~FPlanWaypoint::altflag_standard;
	}
	if (i != nend) {
		std::string err("invalid altitude/speed: " + icao);
		set_icao(icao.substr(0, n));
		throw std::runtime_error(err);
	}
	set_icao(icao.substr(0, n));
	return true;
}

void FlightPlan::ParseWaypoint::process_crsdist(void)
{
	const Glib::ustring& icao(get_icao());
	Glib::ustring::size_type i(icao.size());
	set_coursedist();
	if (i < 7 || !isdigit(icao[i - 6]) || !isdigit(icao[i - 5]) || !isdigit(icao[i - 4]) ||
	    !isdigit(icao[i - 3]) || !isdigit(icao[i - 2]) || !isdigit(icao[i - 1]))
		return;
	set_coursedist((icao[i - 6] - '0') * 100 + (icao[i - 5] - '0') * 10 + (icao[i - 4] - '0'),
		       (icao[i - 3] - '0') * 100 + (icao[i - 2] - '0') * 10 + (icao[i - 1] - '0'));
	set_icao(icao.substr(0, i-6));
}

Point FlightPlan::ParseWaypoint::parse_coord(const std::string& txt)
{
	Point ptinv;
	ptinv.set_invalid();
	std::string::size_type n(txt.size());
	if (n < 7)
		return ptinv;
	if (!std::isdigit(txt[0]) || !std::isdigit(txt[1]))
		return ptinv;
	Point pt;
	std::string::size_type i(2);
	{
		unsigned int deg((txt[0] - '0') *  10 + (txt[1] - '0')), min(0);
		if (std::isdigit(txt[2]) && std::isdigit(txt[3])) {
			min = (txt[2] - '0') *  10 + (txt[3] - '0');
			i = 4;
		}
		if (txt[i] == 'S')
			pt.set_lat_deg(-(deg + min * (1.0 / 60.0)));
		else if (txt[i] == 'N')
			pt.set_lat_deg(deg + min * (1.0 / 60.0));
		else 
			return ptinv;
		++i;
	}
	if (i + 4 > n)
		return ptinv;
	if (!std::isdigit(txt[i]) || !std::isdigit(txt[i + 1]) || !std::isdigit(txt[i + 2]))
		return ptinv;
	{
		unsigned int deg((txt[i] - '0') *  100 + (txt[i + 1] - '0') *  10 + (txt[i + 2] - '0')), min(0);
		i += 3;
		if (i + 3 <= n && std::isdigit(txt[i]) && std::isdigit(txt[i + 1])) {
			min = (txt[i] - '0') *  10 + (txt[i + 1] - '0');
			i += 2;
		}
		if (txt[i] == 'W')
			pt.set_lon_deg(-(deg + min * (1.0 / 60.0)));
		else if (txt[i] == 'E')
			pt.set_lon_deg(deg + min * (1.0 / 60.0));
		else 
			return ptinv;
		++i;
	}
	if (i != n)
		return ptinv;
	return pt;
}

bool FlightPlan::ParseWaypoint::process_coord(void)
{
	Point pt(parse_coord(get_icao()));
	if (pt.is_invalid())
		return false;
	set_coord(pt);
	set_type(FPlanWaypoint::type_user);
	return true;
}

bool FlightPlan::ParseWaypoint::process_airportcoord(void)
{
	typedef std::vector<std::string> tok_t;
	tok_t tok;
	{
		const Glib::ustring& name(get_name());
		Glib::ustring::const_iterator i(name.begin()), e(name.end());
	        for (;;) {
			while (i != e && std::isspace(*i))
				++i;
			if (i == e)
				break;
			Glib::ustring::const_iterator i2(i);
			++i2;
			while (i2 != e && !std::isspace(*i2))
				++i2;
			tok.push_back(std::string(i, i2));
			i = i2;
		}
	}
	bool ret(false);
	for (tok_t::iterator i(tok.begin()), e(tok.end()); i != e; ++i) {
		Point pt(parse_coord(*i));
		if (pt.is_invalid())
			continue;
		set_coord(pt);
		i = tok.erase(i);
		ret = true;
		break;
	}
	{
		bool subseq(false);
		std::string t;
		for (tok_t::const_iterator i(tok.begin()), e(tok.end()); i != e; ++i) {
			if (subseq)
				t += " ";
			subseq = true;
			t += *i;
		}
		set_name(t);
	}
	return ret;
}

std::string FlightPlan::ParseWaypoint::to_str(void) const
{
	std::ostringstream oss;
	oss << FPLWaypoint::to_str();
	if (is_coursedist())
		oss << ' ' << std::setfill('0') << std::setw(3) << get_course()
		    << std::setfill('0') << std::setw(3) << get_dist();
	if (!empty()) {
		oss << " [";
		for (unsigned int i = 0, n = size(); i < n; ++i) {
			const Path& p(operator[](i));
			oss << ' ' << (unsigned int)p.get_vertex() << ' ' << p.get_dist();
			if (p.empty())
				continue;
			oss << " (";
			for (unsigned int j = 0, k = p.size(); j < k; ++j)
				oss << ' ' << (unsigned int)p[j];
			oss << " )";
		}
		oss << " ]";
       	}
	return oss.str();
}

const bool FlightPlan::ParseState::trace_dbload;

FlightPlan::ParseState::ParseState(Database& db)
	: m_db(db)
{
}

uint64_t FlightPlan::ParseState::get_departuretime(void) const
{
	if (m_wpts.empty())
		return 0;
	return m_wpts[0].get_time_unix();
}

void FlightPlan::ParseState::process_speedalt(void)
{
	wpts_t::size_type n(m_wpts.size());
	int32_t alt(std::numeric_limits<int32_t>::min());
	uint16_t flags(0);
	float speed(0);
	if (n) {
		const ParseWaypoint& wpt(m_wpts[0]);
		alt = wpt.get_altitude();
		flags = wpt.get_flags() & (FPlanWaypoint::altflag_standard |
					   FPlanWaypoint::altflag_ifr);
		speed = wpt.get_tas_kts();
	}
	for (wpts_t::size_type i(0); i < n; ) {
		ParseWaypoint& wpt(m_wpts[i]);
		wpt.upcaseid();
		wpt.set_altitude(alt);
		wpt.frob_flags(flags, FPlanWaypoint::altflag_standard | FPlanWaypoint::altflag_ifr);
		{
			bool ifr(wpt.get_icao() == "IFR");
			if (ifr || (wpt.get_icao() == "VFR")) {
				if (ifr)
					flags |= FPlanWaypoint::altflag_ifr;
				else
					flags &= ~FPlanWaypoint::altflag_ifr;
				if (i) {
					ParseWaypoint& wptp(m_wpts[i - 1]);
					wptp.frob_flags(flags, FPlanWaypoint::altflag_standard | FPlanWaypoint::altflag_ifr);
				}
				m_wpts.erase(m_wpts.begin() + i);
				n = m_wpts.size();
				continue;
			}
		}
		if ((flags & FPlanWaypoint::altflag_ifr) && i && wpt.get_icao() == "DCT") {
			if (i) {
				ParseWaypoint& wptp(m_wpts[i - 1]);
				wptp.set_pathcode(FPlanWaypoint::pathcode_directto);
			}
			m_wpts.erase(m_wpts.begin() + i);
			n = m_wpts.size();
			continue;
		}
		if ((flags & FPlanWaypoint::altflag_ifr) && i && FPlanWaypoint::is_stay(wpt.get_icao())) {
			if (i) {
				ParseWaypoint& wptp(m_wpts[i - 1]);
				wptp.set_pathcode(FPlanWaypoint::pathcode_airway);
				wptp.set_pathname(wpt.get_icao());
			}
			m_wpts.erase(m_wpts.begin() + i);
			n = m_wpts.size();
			continue;
		}
		bool spdset(false);
		try {
			float spd;
			if (wpt.process_speedalt(flags, alt, spd) && !std::isnan(spd)) {
				m_cruisespeeds[alt] = spd;
				speed = spd;
				spdset = true;
			}
		} catch (const std::exception& e) {
			m_errors.push_back(e.what());
		}
		if (!spdset) {
			float spd(wpt.get_tas_kts());
			if (!std::isnan(spd) && spd > 0)
				m_cruisespeeds[std::max(alt, (int32_t)0)] = spd;
		}
		wpt.set_tas_kts(speed);
		wpt.process_crsdist();
		{
			Point pt;
			pt.set_invalid();
			wpt.set_coord(pt);
		}
		wpt.set_type(FPlanWaypoint::type_undefined);
		wpt.process_coord();
		if (!i || i + 1 == n)
			wpt.process_airportcoord();
		++i;
	}
}

void FlightPlan::ParseState::graph_add(const Object::ptr_t& p)
{
	if (!p)
		return;
	p->link(m_db, ~0U);
	unsigned int added(m_graph.add(get_departuretime(), p));
	if (trace_dbload)
		p->print(std::cerr) << std::endl;
	switch (p->get_type()) {
	case Object::type_sid:
	case Object::type_star:
	case Object::type_route:
		if (!added)
			return;
		break;

	default:
		return;
	}
	uint64_t deptime(get_departuretime());
	graph_add(m_db.find_dependson(p->get_uuid(), Database::loadmode_link, deptime, deptime + 1,
				      Object::type_first, Object::type_last, 0));
}

void FlightPlan::ParseState::graph_add(const Database::findresults_t& r)
{
	for (Database::findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri)
		graph_add(ri->get_obj());
}

void FlightPlan::ParseState::process_dblookup(bool termarpt)
{
	bool lastwpt(false);
	wpts_t::size_type n(m_wpts.size());
	uint64_t deptime(get_departuretime());
	for (wpts_t::size_type i(0); i < n; ) {
		ParseWaypoint& wpt(m_wpts[i]);
		if (wpt.get_ptobj() && !wpt.get_ptobj()->get_uuid().is_nil()) {
			Object::ptr_t p(Object::ptr_t::cast_const(m_db.load(wpt.get_ptobj()->get_uuid())));
			wpt.set_ptobj(p);
			if (p) {
				p->link(m_db, ~0U);
				m_graph.add(get_departuretime(), p);
				if (trace_dbload)
					p->print(std::cerr) << std::endl;
				std::pair<Graph::vertex_descriptor,bool> u(m_graph.find_vertex(p));
				if (u.second) {
					wpt.add(ParseWaypoint::Path(u.first));
					++i;
					lastwpt = wpt.get_pathcode() == FPlanWaypoint::pathcode_none;
					continue;
				}
			}
		}
		if ((!i || i + 1 >= n) && !wpt.get_coord().is_invalid()) {
			Rect bbox(wpt.get_coord(), wpt.get_coord());
			bbox = bbox.oversize_nmi(2.);
			Database::findresults_t r(m_db.find_by_bbox(bbox, Database::loadmode_link, deptime, deptime + 1,
								    Object::type_airport, Object::type_airportend, 0));
			Object::ptr_t p;
			double dist(std::numeric_limits<double>::max());
			for (Database::findresults_t::const_iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
				Object::ptr_t p1(ri->get_obj());
				if (!p1)
					continue;
				const AirportTimeSlice& ts(p1->operator()(get_departuretime()).as_airport());
				if (!ts.is_valid())
					continue;
				double d(wpt.get_coord().spheric_distance_nmi_dbl(ts.get_coord()));
				if (d > dist)
					continue;
				dist = d;
				p = p1;
			}
			if (p && dist <= 1.) {
				p->link(m_db, ~0U);
				m_graph.add(get_departuretime(), p);
				if (trace_dbload)
					p->print(std::cerr) << std::endl;
				std::pair<Graph::vertex_descriptor,bool> u(m_graph.find_vertex(p));
				if (u.second) {
					wpt.add(ParseWaypoint::Path(u.first));
					++i;
					lastwpt = wpt.get_pathcode() == FPlanWaypoint::pathcode_none;
					continue;
				}
			}
		}
		{
			// check if ident is already present in graph
			Graph::findresult_t r(m_graph.find_ident(wpt.get_icao()));
			if (r.first == r.second) {
			        graph_add(m_db.find_by_ident(wpt.get_icao(), Database::comp_exact, 0, Database::loadmode_link, deptime, deptime + 1,
							     Object::type_first, Object::type_last, 0));
				r = m_graph.find_ident(wpt.get_icao());
			}
			if (false) {
				std::cerr << "graph find ident: " << wpt.get_icao() << std::endl;
				for (Graph::objectset_t::const_iterator ri(r.first); ri != r.second; ++ri)
					if (*ri)
						(*ri)->print(std::cerr);
				std::cerr << std::endl;
			}
			// check if airway
			if (lastwpt && wpt.is_ifr() && i && (i + 1 < n)) {
				Graph::objectset_t::const_iterator ri(r.first);
				for (; ri != r.second; ++ri) {
					const Object::const_ptr_t& p(*ri);
					if (!p)
						continue;
					FPlanWaypoint::pathcode_t pc(FPlanWaypoint::pathcode_none);
					switch (p->get_type()) {
					case Object::type_sid:
						pc = FPlanWaypoint::pathcode_sid;
						break;

					case Object::type_star:
						pc = FPlanWaypoint::pathcode_star;
						break;

					case Object::type_route:
						pc = FPlanWaypoint::pathcode_airway;
						break;

					default:
						continue;
					}
					const IdentTimeSlice& ts(p->operator()(deptime).as_ident());
					if (!ts.is_valid())
						continue;
					ParseWaypoint& wptp(m_wpts[i - 1]);
					wptp.set_pathcode(pc);
					wptp.set_pathname(ts.get_ident());
					break;
				}
				if (ri != r.second) {
					m_wpts.erase(m_wpts.begin() + i);
					n = m_wpts.size();
					lastwpt = false;
					continue;
				}
			}
			// find waypoint
			bool maybearpt(!i || (i + 1 >= n));
			bool mustbearbt(maybearpt && termarpt);
			maybearpt = maybearpt || !wpt.is_ifr();
			maybearpt = maybearpt && !AirportsDb::Airport::is_fpl_zzzz(wpt.get_icao());
			bool found(false);
			for (; r.first != r.second; ++r.first) {
				const Object::const_ptr_t& p(*r.first);
				if (!p)
					continue;
				{
					Object::type_t t(p->get_type());
					if (t >= Object::type_airport && t <= Object::type_airportend) {
						if (!maybearpt)
							continue;
					} else if (t == Object::type_navaid || t == Object::type_designatedpoint) {
						if (mustbearbt)
							continue;
					} else {
						continue;
					}
				}
				std::pair<Graph::vertex_descriptor,bool> u(m_graph.find_vertex(p));
				if (!u.second)
					continue;
				wpt.add(ParseWaypoint::Path(u.first));
				found = true;
			}
			if (found) {
				lastwpt = wpt.get_pathcode() == FPlanWaypoint::pathcode_none;
				++i;
				continue;
			}
		}
		lastwpt = wpt.get_pathcode() == FPlanWaypoint::pathcode_none;
		++i;
		if (!wpt.is_ifr())
			continue;
		m_errors.push_back("unknown identifier " + wpt.get_icao());
	}
	for (wpts_t::size_type i(0); i < n; ++i) {
		ParseWaypoint& wpt(m_wpts[i]);
		if (!wpt.is_coursedist())
			continue;
		// convert magnetic to true
		double course(wpt.get_course());
		WMM wmm;
		wmm.compute(wpt.get_altitude() * (FPlanWaypoint::ft_to_m / 1000.0), wpt.get_coord(), time(0));
		if (wmm)
			course += wmm.get_dec();
		if (wpt.empty()) {
			if (wpt.get_coord().is_invalid()) {
				std::ostringstream oss;
				oss << wpt.get_icao()
				    << std::setfill('0') << std::setw(3) << wpt.get_course()
				    << std::setfill('0') << std::setw(3) << wpt.get_dist();
				wpt.set_icao(oss.str());
				wpt.set_coursedist();
				continue;
			}
			wpt.set_coord(wpt.get_coord().spheric_course_distance_nmi(course, wpt.get_dist()));
                        wpt.set_icao(wpt.get_coord().get_fpl_str());
			wpt.set_type(FPlanWaypoint::type_user);
			wpt.set_coursedist();
			continue;
		}
		typedef std::vector<ParseWaypoint::Path> paths_t;
		paths_t paths;
		for (unsigned int j = 0, k = wpt.size(); j < k; ++j) {
			ParseWaypoint::Path& p(wpt[j]);
			const GraphVertex& v(m_graph[p.get_vertex()]);
			DesignatedPointTimeSlice ts(0, std::numeric_limits<uint64_t>::max());
			ts.set_type(DesignatedPointTimeSlice::type_user);
			ts.set_name("flight plan course/dist point");
			{
			       	std::ostringstream oss;
				oss << v.get_ident()
				    << std::setfill('0') << std::setw(3) << wpt.get_course()
				    << std::setfill('0') << std::setw(3) << wpt.get_dist();
				ts.set_ident(oss.str());
			}
			ts.set_coord(v.get_coord().spheric_course_distance_nmi(course, wpt.get_dist()));
			UUID uuid(UUID::from_str(v.get_object()->get_uuid(), ts.get_ident()));
			DesignatedPoint::ptr_t po(new DesignatedPoint(uuid));
			po->add_timeslice(ts);
			m_graph.add(deptime, po);
			std::pair<Graph::vertex_descriptor,bool> u(m_graph.find_vertex(po));
			if (!u.second)
				continue;
			paths.push_back(ParseWaypoint::Path(u.first));
		}
		if (paths.empty()) {
			wpt.set_coursedist();
			m_errors.push_back("Cannot handle course/distance for " + wpt.get_icao());
			continue;
		}
		{
			std::ostringstream oss;
			oss << wpt.get_icao()
			    << std::setfill('0') << std::setw(3) << wpt.get_course()
			    << std::setfill('0') << std::setw(3) << wpt.get_dist();
			wpt.set_icao(oss.str());
			wpt.set_coursedist();
		}
		wpt.clear();
		for (paths_t::const_iterator i(paths.begin()), e(paths.end()); i != e; ++i)
			wpt.add(*i);
	}
}

void FlightPlan::ParseState::process_airways(bool expand, bool termarpt)
{
	uint64_t deptime(get_departuretime());
	wpts_t::size_type n(m_wpts.size());
	// forward pass
	wpts_t::size_type previ(n);
	for (wpts_t::size_type i(0); i < n; ++i) {
		ParseWaypoint& wpt(m_wpts[i]);
		wpt.set_wptnr(i);
		if ((wpt.get_pathcode() == FPlanWaypoint::pathcode_airway ||
		     wpt.get_pathcode() == FPlanWaypoint::pathcode_sid ||
		     wpt.get_pathcode() == FPlanWaypoint::pathcode_star) &&
		    (wpt.empty() || i + 1 >= n || m_wpts[i + 1].empty()) && !wpt.is_stay()) {
			m_errors.push_back("Airway segment " + wpt.get_icao() + " " + wpt.get_pathname() +
					   " " + m_wpts[i + 1].get_icao() + " has unknown endpoint(s)");
			wpt.set_pathcode(FPlanWaypoint::pathcode_none);
			wpt.set_pathname("");
		}
		if (wpt.empty()) {
			if (!wpt.get_coord().is_invalid())
				previ = i;
			continue;
		}
		if (previ >= n) {
			for (unsigned int j = 0, k = wpt.size(); j < k; ++j) {
				ParseWaypoint::Path& p(wpt[j]);
				p.clear();
				p.set_dist(0);
			}
			previ = i;
			continue;
		}
		const ParseWaypoint& wptp(m_wpts[previ]);
		if (wptp.empty()) {
			for (unsigned int j = 0, k = wpt.size(); j < k; ++j) {
				ParseWaypoint::Path& p(wpt[j]);
				const GraphVertex& v(m_graph[p.get_vertex()]);
				p.clear();
				p.set_dist(wptp.get_coord().spheric_distance_nmi_dbl(v.get_coord()));
			}
			previ = i;
			continue;
		}
		if (previ + 1 != i || ((wptp.get_pathcode() != FPlanWaypoint::pathcode_airway || wptp.is_stay()) &&
				       wptp.get_pathcode() != FPlanWaypoint::pathcode_sid &&
				       wptp.get_pathcode() != FPlanWaypoint::pathcode_star)) {
			for (unsigned int j = 0, k = wpt.size(); j < k; ++j) {
				ParseWaypoint::Path& p(wpt[j]);
				const GraphVertex& v(m_graph[p.get_vertex()]);
				p.clear();
				p.set_dist(std::numeric_limits<double>::max());
				for (unsigned int l = 0, m = wptp.size(); l < m; ++l) {
					const ParseWaypoint::Path& pp(wptp[l]);
					const GraphVertex& vv(m_graph[pp.get_vertex()]);
					double dist(vv.get_coord().spheric_distance_nmi_dbl(v.get_coord()));
					dist += pp.get_dist();
					if (dist > p.get_dist())
						continue;
					p.clear();
					p.add(pp.get_vertex());
					p.set_dist(dist);
				}
			}
			previ = i;
			continue;
		}
		previ = i;
		// try first without helper edges
		{
			Graph::edge_iterator e0, e1;
			for (tie(e0, e1) = boost::edges(m_graph); e0 != e1; ++e0) {
				GraphEdge& ee(m_graph[*e0]);
				ee.clear_solution();
				if (ee.get_route_ident() != wptp.get_pathname())
					continue;
				ee.set_solution(1);
			}
		}
		// dijkstra
		for (unsigned int j = 0, k = wpt.size(); j < k; ++j) {
			ParseWaypoint::Path& p(wpt[j]);
			p.set_dist(std::numeric_limits<double>::max());
			p.clear();
		}
		bool needhelper(true);
		for (unsigned int j = 0, k = wptp.size(); j < k; ++j) {
			const ParseWaypoint::Path& p(wptp[j]);
			Graph::SolutionEdgeFilter filter(m_graph);
			typedef boost::filtered_graph<Graph, Graph::SolutionEdgeFilter> fg_t;
			fg_t fg(m_graph, filter);
			std::vector<Graph::vertex_descriptor> predecessors(boost::num_vertices(m_graph), 0);
			std::vector<double> distances(boost::num_vertices(m_graph), 0);
			dijkstra_shortest_paths(fg, p.get_vertex(), boost::weight_map(boost::get(&GraphEdge::m_dist, m_graph)).
						predecessor_map(&predecessors[0]).distance_map(&distances[0]));
			for (unsigned int l = 0, m = wpt.size(); l < m; ++l) {
				ParseWaypoint::Path& pp(wpt[l]);
				Graph::vertex_descriptor u(pp.get_vertex());
				if (u == predecessors[u])
					continue;
				double dist(p.get_dist() + distances[u]);
				if (dist > pp.get_dist())
					continue;
				needhelper = false;
				pp.clear();
				pp.set_dist(dist);
				for (;;) {
					Graph::vertex_descriptor u1(predecessors[u]);
					if (u1 == u)
						break;
					pp.add(u1);
					u = u1;
				}
			}
		}
		// helper edges
		if (needhelper) {
			bool found(false);
			Graph::findresult_t awys(m_graph.find_ident(wptp.get_pathname()));
			for (; awys.first != awys.second; ++awys.first) {
				const Object::const_ptr_t& pawy(*awys.first);
				if (!pawy)
					continue;
				switch (pawy->get_type()) {
				case Object::type_sid:
				case Object::type_star:
				case Object::type_route:
					break;

				default:
					continue;
				}
				typedef std::set<Graph::vertex_descriptor> vertex_descriptor_set_t;
				vertex_descriptor_set_t awyv;
				{
					Graph::findresult_t r(m_graph.find_dependson(pawy));
					for (; r.first != r.second; ++r.first) {
						const Object::const_ptr_t& p(*r.first);
						const SegmentTimeSlice& ts(p->operator()(deptime).as_segment());
						if (!ts.is_valid())
							continue;
						if (false) {
							std::cerr << "Awy " << pawy->operator()(deptime).as_ident().get_ident()
								  << ' ' << pawy->get_uuid() << " adding segment " << p->get_uuid()
								  << std::endl;
						}
						std::pair<Graph::vertex_descriptor,bool> u(m_graph.find_vertex(ts.get_start().get_obj()));
						if (u.second)
							awyv.insert(u.first);
						u = m_graph.find_vertex(ts.get_end().get_obj());
						if (u.second)
							awyv.insert(u.first);
					}
				}
				if (awyv.empty())
					continue;
				found = true;
				if (false) {
					for (vertex_descriptor_set_t::const_iterator i(awyv.begin()), e(awyv.end()); i != e; ++i) {
						const GraphVertex& v1(m_graph[*i]);
						std::cerr << "Airway " << wptp.get_pathname() << ' ' << v1.get_ident()
							  << '(' << (unsigned int)*i << ')';
						Graph::out_edge_iterator e0, e1;
						for (boost::tie(e0, e1) = boost::out_edges(*i, m_graph); e0 != e1; ++e0) {
							const GraphEdge& ee(m_graph[*e0]);
							if (ee.get_route_ident() != wptp.get_pathname())
								continue;
							std::cerr << ' ' << (unsigned int)boost::target(*e0, m_graph);
						}
						std::cerr << std::endl;
					}
				}
				for (unsigned int j = 0, k = wptp.size(); j < k; ++j) {
					const ParseWaypoint::Path& p(wptp[j]);
					if (awyv.find(p.get_vertex()) != awyv.end())
						continue;
					const GraphVertex& v0(m_graph[p.get_vertex()]);
					for (vertex_descriptor_set_t::const_iterator i(awyv.begin()), e(awyv.end()); i != e; ++i) {
						const GraphVertex& v1(m_graph[*i]);
						GraphEdge edge(0, v0.get_coord().spheric_distance_nmi(v1.get_coord()),
							       v0.get_coord().spheric_true_course(v1.get_coord()));
						edge.set_solution(2);
						boost::add_edge(p.get_vertex(), *i, edge, m_graph);
						if (false) {
							const GraphVertex& v1(m_graph[p.get_vertex()]);
							const GraphVertex& v2(m_graph[*i]);
							std::cerr << "DCT edge " << v1.get_ident() << '(' << p.get_vertex()
								  << ") " << v2.get_ident() << '(' << *i << ')' << std::endl;
						}
					}
				}
				for (unsigned int j = 0, k = wpt.size(); j < k; ++j) {
					ParseWaypoint::Path& p(wpt[j]);
					if (awyv.find(p.get_vertex()) != awyv.end())
						continue;
					const GraphVertex& v0(m_graph[p.get_vertex()]);
					for (vertex_descriptor_set_t::const_iterator i(awyv.begin()), e(awyv.end()); i != e; ++i) {
						const GraphVertex& v1(m_graph[*i]);
						GraphEdge edge(0, v1.get_coord().spheric_distance_nmi(v0.get_coord()),
							       v1.get_coord().spheric_true_course(v0.get_coord()));
						edge.set_solution(2);
						boost::add_edge(*i, p.get_vertex(), edge, m_graph);
						if (false) {
							const GraphVertex& v1(m_graph[*i]);
							const GraphVertex& v2(m_graph[p.get_vertex()]);
							std::cerr << "DCT edge " << v1.get_ident() << '(' << *i
								  << ") " << v2.get_ident() << '(' << p.get_vertex() << ')' << std::endl;
						}
					}
				}
			}
			if (!found) {
				m_errors.push_back("airway " + wptp.get_pathname() + " has no terminals");
				continue;
			}
			// dijkstra
			for (unsigned int j = 0, k = wptp.size(); j < k; ++j) {
				const ParseWaypoint::Path& p(wptp[j]);
				Graph::SolutionEdgeFilter filter(m_graph);
				typedef boost::filtered_graph<Graph, Graph::SolutionEdgeFilter> fg_t;
				fg_t fg(m_graph, filter);
				std::vector<Graph::vertex_descriptor> predecessors(boost::num_vertices(m_graph), 0);
				std::vector<double> distances(boost::num_vertices(m_graph), 0);
				dijkstra_shortest_paths(fg, p.get_vertex(), boost::weight_map(boost::get(&GraphEdge::m_dist, m_graph)).
							predecessor_map(&predecessors[0]).distance_map(&distances[0]));
				for (unsigned int l = 0, m = wpt.size(); l < m; ++l) {
					ParseWaypoint::Path& pp(wpt[l]);
					Graph::vertex_descriptor u(pp.get_vertex());
					if (u == predecessors[u])
						continue;
					double dist(p.get_dist() + distances[u]);
					if (dist > pp.get_dist())
						continue;
					pp.clear();
					pp.set_dist(dist);
					for (;;) {
						Graph::vertex_descriptor u1(predecessors[u]);
						if (u1 == u)
							break;
						pp.add(u1);
						u = u1;
					}
				}
			}
			// kill helper DCT edges
			for (;;) {
				bool done(true);
				Graph::edge_iterator e0, e1;
				for (tie(e0, e1) = boost::edges(m_graph); e0 != e1;) {
					Graph::edge_descriptor e(*e0);
					++e0;
					GraphEdge& ee(m_graph[e]);
					if (!ee.is_solution(2))
						continue;
					boost::remove_edge(e, m_graph);
					if (true) {
						done = false;
						break;
					}
				}
				if (done)
					break;
			}
		}
	}
	if (n > 0) {
		ParseWaypoint& wpt(m_wpts[n - 1]);
		wpt.set_pathcode(FPlanWaypoint::pathcode_none);
	}
	if (false)
		print(std::cerr << "Flight Plan after airways forward processing" << std::endl);
	// backward pass
	previ = n;
	for (wpts_t::size_type i(n); i > 1; ) {
		--i;
		ParseWaypoint& wpt(m_wpts[i]);
		if (wpt.empty()) {
			if (!wpt.get_coord().is_invalid())
				previ = i;
			continue;
		}
		if (wpt.size() > 1) {
			unsigned int bidx(wpt.size());
			double bdist(std::numeric_limits<double>::max());
			for (unsigned int j = 0, k = wpt.size(); j < k; ++j) {
				const ParseWaypoint::Path& p(wpt[j]);
				double dist(p.get_dist());
				if (previ < n) {
					const GraphVertex& v(m_graph[p.get_vertex()]);
					dist += v.get_coord().spheric_distance_nmi_dbl(m_wpts[previ].get_coord());
				}
				if (dist > bdist && bidx < wpt.size())
					continue;
				bdist = dist;
				bidx = j;
			}
			ParseWaypoint::Path p(wpt[bidx]);
			wpt.clear();
			wpt.add(p);
		}
		const ParseWaypoint::Path& p(wpt[0]);
		const GraphVertex& v(m_graph[p.get_vertex()]);
		wpt.set_ptobj(v.get_object());
		ParseWaypoint& wptp(m_wpts[i - 1]);
		if (i > 0 && !p.empty()) {
			Graph::vertex_descriptor u(p[p.size()-1]);
			unsigned int n(wptp.size()), b(n);
			for (unsigned int j = 0; j < n; ++j) {
				if (wptp[j].get_vertex() != u)
					continue;
				b = j;
				break;
			}
			if (b < n) {
				ParseWaypoint::Path pp(wptp[b]);
				wptp.clear();
				wptp.add(pp);
			}
		}
		std::vector<ParseWaypoint> wpts;
		for (unsigned int j(1), k(p.size()); j < k; ++j) {
			const GraphVertex& v(m_graph[p[j - 1]]);
			ParseWaypoint w(wptp);
			w.frob_flags(wpt.get_flags() & FPlanWaypoint::altflag_standard, FPlanWaypoint::altflag_standard);
			w.set_altitude(wpt.get_altitude());
			w.set_icao(v.get_ident());
			w.set_coord(v.get_coord());
			w.set_ptobj(v.get_object());
			w.set_expanded(true);
			w.set_pathcode(wptp.get_pathcode());
			w.set_pathname(wptp.get_pathname());
			w.clear();
			w.add(ParseWaypoint::Path(p[j - 1]));
			wpts.push_back(w);
		}
		m_wpts.insert(m_wpts.begin() + i, wpts.rbegin(), wpts.rend());
	}
	if (!m_wpts[0].empty()) {
		ParseWaypoint& wpt(m_wpts[0]);
		const ParseWaypoint::Path& p(wpt[0]);
		const GraphVertex& v(m_graph[p.get_vertex()]);
		wpt.set_ptobj(v.get_object());
	}
	if (false)
		print(std::cerr << "Flight Plan after airways backward processing" << std::endl);
	// expand pathcode_none in IFR segments
	n = m_wpts.size();
	for (wpts_t::size_type i(1); i < n; ++i) {
		ParseWaypoint& wpt(m_wpts[i - 1]);
		if (!wpt.is_ifr() || wpt.get_pathcode() != FPlanWaypoint::pathcode_none)
			continue;
		const ParseWaypoint& wptn(m_wpts[i]);
		if (wpt.empty() || wptn.empty() || !wpt.get_ptobj() || !wptn.get_ptobj()) {
			wpt.set_pathcode(FPlanWaypoint::pathcode_none);
			wpt.set_pathname("");
			continue;
		}
		if (i <= 1) {
			// SID
			// add all SIDs of the departure airport to the graph
			graph_add(m_db.find_dependson(wpt.get_ptobj()->get_uuid(), Database::loadmode_link, deptime, deptime + 1,
						      Object::type_sid, Object::type_sid, 0));
			Graph::findresult_t deps(m_graph.find_dependson(Object::ptr_t::cast_const(wpt.get_ptobj())));
			Object::const_ptr_t proute;
			double droute(std::numeric_limits<double>::max());
			typedef std::vector<Graph::vertex_descriptor> addv_t;
			addv_t addv;
			for (Graph::objectset_t::const_iterator di(deps.first), de(deps.second); di != de; ++di) {
				Object::const_ptr_t p(*di);
				if (!p)
					continue;
				const StandardInstrumentDepartureTimeSlice& ts(p->operator()(deptime).as_sid());
				if (!ts.is_valid())
					continue;
				if (ts.get_airport() != wpt.get_ptobj()->get_uuid())
					continue;
				if (ts.get_connpoints().find(wptn.get_ptobj()->get_uuid()) == ts.get_connpoints().end())
					continue;
				Graph::UUIDEdgeFilter filter(m_graph, p->get_uuid(), GraphEdge::matchcomp_segment);
				typedef boost::filtered_graph<Graph, Graph::UUIDEdgeFilter> fg_t;
				fg_t fg(m_graph, filter);
				std::vector<Graph::vertex_descriptor> predecessors(boost::num_vertices(m_graph), 0);
				std::vector<double> distances(boost::num_vertices(m_graph), 0);
				dijkstra_shortest_paths(fg, wpt[0].get_vertex(), boost::weight_map(boost::get(&GraphEdge::m_dist, m_graph)).
							predecessor_map(&predecessors[0]).distance_map(&distances[0]));
				Graph::vertex_descriptor v(wptn[0].get_vertex());
				double d(distances[v]);
				if (d >= droute)
					continue;
				addv_t addv1;
				for (;;) {
					Graph::vertex_descriptor u(predecessors[v]);
					if (u == v)
						break;
					addv1.insert(addv1.begin(), u);
					v = u;
				}
				if (addv1.empty() || addv1.front() != wpt[0].get_vertex())
					continue;
				// check altitude of last segment
				std::pair<Graph::edge_descriptor,bool> edge(m_graph.find_edge(addv1.back(), wptn[0].get_vertex(), p));
				if (!edge.second)
					continue;
	        		const SegmentTimeSlice& tsseg(m_graph[edge.first].get_timeslice());
				if (!tsseg.is_valid() || !tsseg.get_altrange().is_inside(wptn.get_altitude()))
					continue;
				addv1.erase(addv1.begin());
				droute = d;
				proute = p;
				addv.swap(addv1);
			}
			if (!proute) {
				wpt.set_pathcode(FPlanWaypoint::pathcode_none);
				wpt.set_pathname("");
				continue;
			}
			const StandardInstrumentDepartureTimeSlice& ts(proute->operator()(deptime).as_sid());
			wpt.set_pathcode(FPlanWaypoint::pathcode_sid);
			wpt.set_pathname(ts.get_ident());
			for (addv_t::const_iterator ai(addv.begin()), ae(addv.end()); ai != ae; ++ai) {
				const GraphVertex v(m_graph[*ai]);
				ParseWaypoint w(m_wpts[i - 1]);
				w.frob_flags(m_wpts[i].get_flags() & FPlanWaypoint::altflag_standard, FPlanWaypoint::altflag_standard);
				w.set_icao(v.get_ident());
				w.set_coord(v.get_coord());
				w.set_ptobj(v.get_object());
				w.set_expanded(true);
				w.clear();
				w.add(ParseWaypoint::Path(*ai));
				m_wpts.insert(m_wpts.begin() + i, w);
				++i;
				++n;
			}
			continue;
		}
		typedef std::vector<Graph::vertex_descriptor> addv_t;
		if (i + 1 >= n) {
			// STAR
			// add all STARs of the destination airport to the graph
			graph_add(m_db.find_dependson(wptn.get_ptobj()->get_uuid(), Database::loadmode_link, deptime, deptime + 1,
						      Object::type_star, Object::type_star, 0));
			Graph::findresult_t deps(m_graph.find_dependson(Object::ptr_t::cast_const(wptn.get_ptobj())));
			Object::const_ptr_t proute;
			double droute(std::numeric_limits<double>::max());
			addv_t addv;
			for (Graph::objectset_t::const_iterator di(deps.first), de(deps.second); di != de; ++di) {
				Object::const_ptr_t p(*di);
				if (!p)
					continue;
				const StandardInstrumentArrivalTimeSlice& ts(p->operator()(deptime).as_star());
				if (!ts.is_valid())
					continue;
				if (ts.get_airport() != wptn.get_ptobj()->get_uuid())
					continue;
				if (ts.get_connpoints().find(wpt.get_ptobj()->get_uuid()) == ts.get_connpoints().end())
					continue;
				Graph::UUIDEdgeFilter filter(m_graph, p->get_uuid(), GraphEdge::matchcomp_segment);
				typedef boost::filtered_graph<Graph, Graph::UUIDEdgeFilter> fg_t;
				fg_t fg(m_graph, filter);
				std::vector<Graph::vertex_descriptor> predecessors(boost::num_vertices(m_graph), 0);
				std::vector<double> distances(boost::num_vertices(m_graph), 0);
				dijkstra_shortest_paths(fg, wpt[0].get_vertex(), boost::weight_map(boost::get(&GraphEdge::m_dist, m_graph)).
							predecessor_map(&predecessors[0]).distance_map(&distances[0]));
				Graph::vertex_descriptor v(wptn[0].get_vertex());
				double d(distances[v]);
				if (d >= droute)
					continue;
				addv_t addv1;
				for (;;) {
					Graph::vertex_descriptor u(predecessors[v]);
					if (u == v)
						break;
					addv1.insert(addv1.begin(), v);
					v = u;
				}
				if (addv1.empty() || addv1.back() != wptn[0].get_vertex())
					continue;
				// check altitude of last segment
				std::pair<Graph::edge_descriptor,bool> edge(m_graph.find_edge(wpt[0].get_vertex(), addv1.front(), p));
				if (!edge.second)
					continue;
	        		const SegmentTimeSlice& tsseg(m_graph[edge.first].get_timeslice());
				if (!tsseg.is_valid() || !tsseg.get_altrange().is_inside(wpt.get_altitude()))
					continue;
				addv1.erase(addv1.end() - 1);
				droute = d;
				proute = p;
				addv.swap(addv1);
			}
			if (!proute) {
				wpt.set_pathcode(FPlanWaypoint::pathcode_none);
				wpt.set_pathname("");
				continue;
			}
			const StandardInstrumentArrivalTimeSlice& ts(proute->operator()(deptime).as_star());
			wpt.set_pathcode(FPlanWaypoint::pathcode_star);
			wpt.set_pathname(ts.get_ident());
			for (addv_t::const_iterator ai(addv.begin()), ae(addv.end()); ai != ae; ++ai) {
				const GraphVertex v(m_graph[*ai]);
				ParseWaypoint w(m_wpts[i - 1]);
				w.set_icao(v.get_ident());
				w.set_coord(v.get_coord());
				w.set_ptobj(v.get_object());
				w.set_expanded(true);
				w.clear();
				w.add(ParseWaypoint::Path(*ai));
				m_wpts.insert(m_wpts.begin() + i, w);
				++i;
				++n;
			}
			continue;
		}
		// add airway segments within the bbox
		double ddct;
		{
			const GraphVertex v0(m_graph[wpt[0].get_vertex()]);
			const GraphVertex v1(m_graph[wptn[0].get_vertex()]);
			Rect bbox(v0.get_coord(), v0.get_coord());
			bbox = bbox.add(v1.get_coord());
			bbox = bbox.oversize_nmi(100.f);
			graph_add(m_db.find_by_bbox(bbox, Database::loadmode_link, deptime, deptime + 1, Object::type_routesegment, Object::type_routesegment, 0));
			ddct = v0.get_coord().spheric_distance_nmi_dbl(v1.get_coord());
		}
		int32_t alt0(std::numeric_limits<int32_t>::max());
		int32_t alt1(std::numeric_limits<int32_t>::min());
		if (wpt.get_altitude() != std::numeric_limits<int32_t>::min()) {
			alt0 = std::min(alt0, wpt.get_altitude());
			alt1 = std::max(alt1, wpt.get_altitude());
		}
		if (wptn.get_altitude() != std::numeric_limits<int32_t>::min()) {
			alt0 = std::min(alt0, wptn.get_altitude());
			alt1 = std::max(alt1, wptn.get_altitude());
		}
		Graph::AirwayAltEdgeFilter filter(m_graph, alt0, alt1);
		typedef boost::filtered_graph<Graph, Graph::AirwayAltEdgeFilter> fg_t;
		fg_t fg(m_graph, filter);
		std::vector<Graph::vertex_descriptor> predecessors(boost::num_vertices(m_graph), 0);
		std::vector<double> distances(boost::num_vertices(m_graph), 0);
		dijkstra_shortest_paths(fg, wpt[0].get_vertex(), boost::weight_map(boost::get(&GraphEdge::m_dist, m_graph)).
					predecessor_map(&predecessors[0]).distance_map(&distances[0]));
		Graph::vertex_descriptor v(wptn[0].get_vertex());
		double d(distances[v]);
		addv_t addv;
		for (;;) {
			Graph::vertex_descriptor u(predecessors[v]);
			if (u == v)
				break;
			addv.insert(addv.begin(), v);
			v = u;
		}
		if (d > 1.1 * ddct || addv.empty() || addv.back() != wptn[0].get_vertex())
			continue;
		addv.erase(addv.end() - 1);
		wpt.set_pathcode(FPlanWaypoint::pathcode_airway);
		wpt.set_pathname("");
		for (addv_t::const_iterator ai(addv.begin()), ae(addv.end()); ai != ae; ++ai) {
			const GraphVertex v(m_graph[*ai]);
			ParseWaypoint w(m_wpts[i - 1]);
			w.set_icao(v.get_ident());
			w.set_coord(v.get_coord());
			w.set_ptobj(v.get_object());
			w.set_expanded(true);
			w.clear();
			w.add(ParseWaypoint::Path(*ai));
			m_wpts.insert(m_wpts.begin() + i, w);
			++i;
			++n;
		}
	}
	if (false)
		print(std::cerr << "Flight Plan after airways pathcode_none removal processing" << std::endl);
	// verify airways
	n = m_wpts.size();
	for (wpts_t::size_type i(1); i < n; ++i) {
		ParseWaypoint& wpt(m_wpts[i - 1]);
		const ParseWaypoint& wptn(m_wpts[i]);
		if (!wpt.is_ifr())
			continue;
		if (wpt.get_pathcode() == FPlanWaypoint::pathcode_directto || wpt.is_stay()) {
			if (wpt.get_altitude() != std::numeric_limits<int32_t>::min())
				continue;
			wpt.set_altitude(wptn.get_altitude());
			continue;
		}
		if (wpt.get_pathcode() != FPlanWaypoint::pathcode_airway &&
		    wpt.get_pathcode() != FPlanWaypoint::pathcode_sid &&
		    wpt.get_pathcode() != FPlanWaypoint::pathcode_star)
			continue;
		if (wpt.empty() || wptn.empty()) {
			wpt.set_pathcode(FPlanWaypoint::pathcode_none);
			wpt.set_pathname("");
			continue;
		}
		bool ok(false);
		Graph::vertex_descriptor u(wpt[0].get_vertex()), v(wptn[0].get_vertex());
		Graph::out_edge_iterator e0, e1;
		for (boost::tie(e0, e1) = boost::out_edges(u, m_graph); e0 != e1; ++e0) {
			if (boost::target(*e0, m_graph) != v)
				continue;
			const GraphEdge& ee(m_graph[*e0]);
			if (wpt.get_pathname().empty()) {
				int32_t alt0(std::numeric_limits<int32_t>::max());
				int32_t alt1(std::numeric_limits<int32_t>::min());
				if (i > 1 && wpt.get_altitude() != std::numeric_limits<int32_t>::min()) {
					alt0 = std::min(alt0, wpt.get_altitude());
					alt1 = std::max(alt1, wpt.get_altitude());
				}
				if (i + 1 < n && wptn.get_altitude() != std::numeric_limits<int32_t>::min()) {
					alt0 = std::min(alt0, wptn.get_altitude());
					alt1 = std::max(alt1, wptn.get_altitude());
				}
				if (!ee.is_inside(alt0, alt1))
					continue;
			} else {
				if (ee.get_route_ident() != wpt.get_pathname())
					continue;
			}
			ok = true;
			wpt.set_pathobj(ee.get_object());
			if (i > 1 && wpt.get_pathcode() == FPlanWaypoint::pathcode_sid) {
				const ParseWaypoint& wptp(m_wpts[i-2]);
				if (wptp.get_pathcode() == FPlanWaypoint::pathcode_sid) {
					const DepartureLegTimeSlice& tsdl0(wptp.get_pathobj()->operator()(wptp.get_time_unix()).as_departureleg());
					const DepartureLegTimeSlice& tsdl1(wpt.get_pathobj()->operator()(wpt.get_time_unix()).as_departureleg());
					if (tsdl0.is_valid() && tsdl1.is_valid()) {
						AltRange ar(tsdl0.get_altrange());
						ar.intersect(tsdl1.get_altrange());
						if (ar.is_upper_valid() && wpt.get_altitude() > ar.get_upper_alt())
							wpt.set_altitude(ar.get_upper_alt());
						else if (ar.is_lower_valid() && wpt.get_altitude() < ar.get_lower_alt())
							wpt.set_altitude(ar.get_lower_alt());
					}
				}
				break;
			}
			if (i > 1 && wpt.get_pathcode() == FPlanWaypoint::pathcode_star) {
				const ParseWaypoint& wptp(m_wpts[i-2]);
				if (wptp.get_pathcode() == FPlanWaypoint::pathcode_star) {
					const ArrivalLegTimeSlice& tsal0(wptp.get_pathobj()->operator()(wptp.get_time_unix()).as_arrivalleg());
					const ArrivalLegTimeSlice& tsal1(wpt.get_pathobj()->operator()(wpt.get_time_unix()).as_arrivalleg());
					if (tsal0.is_valid() && tsal1.is_valid()) {
						AltRange ar(tsal0.get_altrange());
						ar.intersect(tsal1.get_altrange());
						if (ar.is_upper_valid() && wpt.get_altitude() > ar.get_upper_alt())
							wpt.set_altitude(ar.get_upper_alt());
						else if (ar.is_lower_valid() && wpt.get_altitude() < ar.get_lower_alt())
							wpt.set_altitude(ar.get_lower_alt());
					}
				}
				break;
			}
			if (wpt.get_altitude() != std::numeric_limits<int32_t>::min())
				break;
			IntervalSet<int32_t> r(ee.get_altrange());
			if (r.is_empty())
				break;
			int32_t alt(wptn.get_altitude());
			if (wptn.get_altitude() == std::numeric_limits<int32_t>::min() || !r.is_inside(alt))
				alt = r.begin()->get_lower();
			wpt.set_altitude(alt);
			break;
		}
		if (ok)
			continue;
		wpt.set_pathcode(FPlanWaypoint::pathcode_none);
		wpt.set_pathname("");
	}
	// kill expanded segments
	if (!expand && n >= 2) {
		for (wpts_t::size_type i(n - 1); i > 1; --i) {
			const ParseWaypoint& wptp(m_wpts[i - 1]);
			ParseWaypoint& wpt(m_wpts[i]);
			if (!wpt.is_expanded() ||
			    (wptp.get_pathcode() != FPlanWaypoint::pathcode_airway &&
			     wptp.get_pathcode() != FPlanWaypoint::pathcode_sid &&
			     wptp.get_pathcode() != FPlanWaypoint::pathcode_star) ||
			    (wpt.get_pathcode() != FPlanWaypoint::pathcode_airway &&
			     wpt.get_pathcode() != FPlanWaypoint::pathcode_sid &&
			     wpt.get_pathcode() != FPlanWaypoint::pathcode_star) ||
			    wptp.get_pathobj()->get_uuid() != wpt.get_pathobj()->get_uuid())
				continue;
			m_wpts.erase(m_wpts.begin() + i);
		}
	}
	// set names etc from objects
	n = m_wpts.size();
	for (wpts_t::size_type i(0); i < n; ++i) {
		ParseWaypoint& wpt(m_wpts[i]);
		wpt.set_from_objects(termarpt && (!i || (i + 1) >= n));
	}
	if (!m_wpts.empty())
		m_wpts.back().set_pathcode(FPlanWaypoint::pathcode_none);
	// calculate SID/STAR segment altitudes
	for (wpts_t::size_type i(1); i < n; ++i) {
		const ParseWaypoint& wpt0(m_wpts[i - 1]);
		ParseWaypoint& wpt1(m_wpts[i]);
		if (!(wpt0.get_pathcode() == FPlanWaypoint::pathcode_sid &&
		      wpt1.get_pathcode() == FPlanWaypoint::pathcode_sid) &&
		    !(wpt0.get_pathcode() == FPlanWaypoint::pathcode_star &&
		      wpt1.get_pathcode() == FPlanWaypoint::pathcode_star))
			continue;
		const SegmentTimeSlice& seg0(wpt0.get_pathobj()->operator()(deptime).as_segment());
		const SegmentTimeSlice& seg1(wpt1.get_pathobj()->operator()(deptime).as_segment());
		int32_t alt(0);
		if (seg0.is_valid() && seg0.get_altrange().is_lower_valid())
			alt = seg0.get_altrange().get_lower_alt();
		if (seg1.is_valid() && seg1.get_altrange().is_lower_valid() && seg1.get_altrange().get_lower_alt() > alt)
			alt = seg1.get_altrange().get_lower_alt();
		if (alt > 0)
			wpt1.set_altitude(alt);
	}
}

void FlightPlan::ParseState::process_time(const std::string& eets)
{
	typedef std::map<std::string,unsigned int> eet_t;
	eet_t eet;
	for (std::string::const_iterator i(eets.begin()), e(eets.end()); i != e; ) {
		if (std::isspace(*i)) {
			++i;
			continue;
		}
		typedef enum {
			mode_ident,
			mode_time,
			mode_invalid
		} mode_t;
		mode_t mode(mode_ident);
		std::string ident;
		unsigned int tm(0);
		unsigned int tmc(0);
		std::string::const_iterator j(i);
		++j;
		ident += std::toupper(*i);
		if (!std::isalpha(*i))
			mode = mode_invalid;
		while (j != e && !std::isspace(*j)) {
			switch (mode) {
			case mode_ident:
				if (std::isalpha(*j)) {
					ident += std::toupper(*j);
					break;
				}
				mode = mode_time;
				/* fall through */
			case mode_time:
				if (!std::isdigit(*j)) {
					mode = mode_invalid;
					break;
				}
				tm = tm * 10 + (*j - '0');
				++tmc;
				break;

			default:
				break;
			}
			++j;
		}
		std::string el(i, j);
		i = j;
		if (ident.size() < 2 || tmc != 4)
			continue;
		tm = (tm / 100) * 60 + (tm % 100);
		eet[ident] = tm;
	}
	if (false) {
		for (cruisespeeds_t::const_iterator i(m_cruisespeeds.begin()), e(m_cruisespeeds.end()); i != e; ++i)
			std::cerr << "process_time: alt " << i->first << " speed " << i->second << std::endl;
		if (m_cruisespeeds.empty())
			std::cerr << "process_time: cruise speed table empty" << std::endl;
	}
	wpts_t::size_type n(m_wpts.size()), previ(n);
	if (!m_wpts[0].get_coord().is_invalid())
		previ = 0;
	for (wpts_t::size_type i(1); i < n; ++i) {
		ParseWaypoint& wpt(m_wpts[i]);
		{
			eet_t::const_iterator j(eet.find(wpt.get_icao_or_name()));
			if (j != eet.end()) {
				wpt.set_time_unix(m_wpts[0].get_time_unix() + j->second * 60);
				wpt.set_flighttime(j->second * 60);
				if (wpt.get_coord().is_invalid())
					continue;
				previ = i;
				continue;
			}
		}
		if (previ >= n) {
			wpt.set_time_unix(m_wpts[i - 1].get_time_unix());
			wpt.set_flighttime(m_wpts[i - 1].get_flighttime());
			if (wpt.get_coord().is_invalid())
				continue;
			previ = i;
			continue;
		}
		const ParseWaypoint& wptp(m_wpts[previ]);
		wpt.set_time_unix(wptp.get_time_unix());
		wpt.set_flighttime(wptp.get_flighttime());
		if (wpt.get_coord().is_invalid())
			continue;
		previ = i;
		if (m_cruisespeeds.empty())
			continue;
		double dist(wptp.get_coord().spheric_distance_nmi_dbl(wpt.get_coord()));
		cruisespeeds_t::const_iterator b(m_cruisespeeds.begin());
		for (cruisespeeds_t::const_iterator i(b), e(m_cruisespeeds.end()); i != e; ++i) {
			if (abs(i->first - wptp.get_altitude()) < abs(b->first - wptp.get_altitude()))
				b = i;
		}
		if (b->second <= 0)
			continue;
		time_t dt(Point::round<time_t,double>(dist / b->second * 3600));
		if (false)
			std::cerr << "process_time: alt " << b->first << " speed " << b->second
				  << " dist " << dist << " deltat " << dt << std::endl;
		wpt.set_time_unix(wpt.get_time_unix() + dt);
		wpt.set_flighttime(wpt.get_flighttime() + dt);
	}
}

Rect FlightPlan::ParseState::get_bbox(void) const
{
	Rect bbox(Point::invalid, Point::invalid);
	bool first(true);
	wpts_t::size_type n(m_wpts.size());
	for (wpts_t::size_type i(0); i < n; ++i) {
		const ParseWaypoint& wpt(m_wpts[i]);
		if (wpt.get_coord().is_invalid())
			continue;
		if (first) {
			bbox = Rect(wpt.get_coord(), wpt.get_coord());
			first = false;
			continue;
		}
		bbox = bbox.add(wpt.get_coord());
	}
	return bbox;
}

void FlightPlan::ParseState::process_dblookupcoord(void)
{
	uint64_t deptime(get_departuretime());
	wpts_t::size_type n(m_wpts.size());
	graph_add(m_db.find_by_bbox(get_bbox(), Database::loadmode_link, deptime, deptime + 1,
				    Object::type_first, Object::type_last, 0));
	for (wpts_t::size_type i(0); i < n; ++i) {
		ParseWaypoint& wpt(m_wpts[i]);
		if (wpt.get_coord().is_invalid())
			continue;
		{
			Graph::findresult_t r(m_graph.find_ident(wpt.get_icao_or_name()));
			double bdist(std::numeric_limits<double>::max());
			std::pair<Graph::vertex_descriptor,bool> bu;
			bu.second = false;
			for (; r.first != r.second; ++r.first) {
				const Object::const_ptr_t& p(*r.first);
				if (!p)
					continue;
				std::pair<Graph::vertex_descriptor,bool> u(m_graph.find_vertex(p));
				if (!u.second)
					continue;
				const GraphVertex& v(m_graph[u.first]);
				double d(wpt.get_coord().spheric_distance_nmi_dbl(v.get_coord()));
				if (d > bdist)
					continue;
				bdist = d;
				bu = u;
			}
			if (bu.second && bdist <= 1.0) {
				wpt.add(ParseWaypoint::Path(bu.first));
				continue;
			}
		}
		if (!wpt.is_ifr())
			continue;
		m_errors.push_back("unknown identifier " + wpt.get_icao_or_name());
	}
}

std::ostream& FlightPlan::ParseState::print(std::ostream& os) const
{
	for (wpts_t::size_type i = 0, n = size(); i < n; ++i) {
		const ParseWaypoint& wpt(operator[](i));
		os << '[' << i << "] " << wpt.to_str() << std::endl;
	}
	return os;
}

const bool FlightPlan::trace_parsestate;

FlightPlan::FlightPlan(void)
	: m_aircraftid("ZZZZZ"), m_aircrafttype("P28R"), m_equipment("SDFGLOR"), m_transponder("S"),
	  m_departuretime(0), m_totaleet(0), m_endurance(0), m_pbn(Aircraft::pbn_b2), m_number(1),
	  m_personsonboard(0), m_defaultalt(5000), m_flighttype('G'), m_wakecategory('L')
{
	time(&m_departuretime);
	m_departuretime += 60 * 60;
	m_cruisespeed = "N0120";
}

void FlightPlan::clear(void)
{
	base_t::clear();
	m_otherinfo.clear();
	m_aircraftid = "ZZZZZ";
	m_aircrafttype = "P28R";
	m_equipment = "SDFGLOR";
	m_transponder = "S";
	m_cruisespeed.clear();
	m_alternate1.clear();
	m_alternate2.clear();
	m_colourmarkings.clear();
	m_remarks.clear();
	m_picname.clear();
	m_emergencyradio.clear();
	m_survival.clear();
	m_lifejackets.clear();
	m_dinghies.clear();
	m_cruisespeeds.clear();
	m_departuretime = 0;
	m_totaleet = 0;
	m_endurance = 0;
	m_pbn = Aircraft::pbn_b2;
	m_number = 1;
	m_personsonboard = 0;
	m_defaultalt = 5000;
	m_flighttype = 'G';
	m_wakecategory = 'L';

	time(&m_departuretime);
	m_departuretime += 60 * 60;
 	m_cruisespeed = "N0120";
}

void FlightPlan::trimleft(std::string::const_iterator& txti, std::string::const_iterator txte)
{
	std::string::const_iterator txti2(txti);
	for (; txti2 != txte && isspace(*txti2); ++txti2);
	txti = txti2;
}

void FlightPlan::trimright(std::string::const_iterator txti, std::string::const_iterator& txte)
{
	std::string::const_iterator txte2(txte);
	while (txti != txte2) {
		std::string::const_iterator txte3(txte2);
		--txte3;
		if (!isspace(*txte3))
			break;
		txte2 = txte3;
	}
	txte = txte2;
}

void FlightPlan::trim(std::string::const_iterator& txti, std::string::const_iterator& txte)
{
	trimleft(txti, txte);
	trimright(txti, txte);
}

std::string FlightPlan::upcase(const std::string& txt)
{
	Glib::ustring x(txt);
	return x.uppercase();
}

bool FlightPlan::parsetxt(std::string& txt, unsigned int len, std::string::const_iterator& txti, std::string::const_iterator txte, bool slashsep)
{
	trimleft(txti, txte);
	txt.clear();
	while (txti != txte) {
		if (isspace(*txti) || (slashsep && *txti == '/') || *txti == '-' || *txti == '(' || *txti == ')')
			break;
		txt.push_back(*txti);
		++txti;
		if (len && txt.size() >= len)
			break;
	}
	trimleft(txti, txte);
	return len ? txt.size() == len : !txt.empty();
}

bool FlightPlan::parsenum(unsigned int& num, unsigned int digits, std::string::const_iterator& txti, std::string::const_iterator txte)
{
	trimleft(txti, txte);
	num = 0;
	unsigned int d = 0;
	while (txti != txte) {
		if (!isdigit(*txti))
			break;
		num *= 10;
		num += (*txti) - '0';
		++d;
		++txti;
		if (digits && d >= digits)
			break;
	}
	trimleft(txti, txte);
	return d && (!digits || d == digits);
}

bool FlightPlan::parsetime(time_t& t, std::string::const_iterator& txti, std::string::const_iterator txte)
{
	t = 0;
	unsigned int n;
	if (!parsenum(n, 4, txti, txte))
		return false;
	t = ((n / 100) * 60 + (n % 100)) * 60;
	return true;
}

bool FlightPlan::parsespeed(std::string& spdstr, float& spd, std::string::const_iterator& txti, std::string::const_iterator txte)
{
	spd = 0;
	spdstr.clear();
	trimleft(txti, txte);
	if (txti == txte)
		return false;
	char ch(*txti);
	++txti;
	float factor(1);
	unsigned int len(4);
	switch (ch) {
	case 'N':
		break;

	case 'K':
		factor = Point::km_to_nmi;
		break;

	case 'M':
		factor = 330.0 * Point::mps_to_kts; // very approximate!
		len = 3;
		break;

	default:
		return false;
	}
	unsigned int num;
	if (!parsenum(num, len, txti, txte))
		return false;
	spd = num * factor;
	std::ostringstream oss;
	oss << ch << std::setw(len) << std::setfill('0') << num;
	spdstr = oss.str();
	return true;
}

bool FlightPlan::parsealt(int32_t& alt, unsigned int& flags, std::string::const_iterator& txti, std::string::const_iterator txte)
{
	alt = std::numeric_limits<int32_t>::min();
	flags &= ~FPlanWaypoint::altflag_standard;
	trimleft(txti, txte);
	if (txti == txte)
		return false;
	char ch(*txti);
	++txti;
	if (ch == 'V') {
		std::string s;
		if (!parsetxt(s, 2, txti, txte))
			return false;
		if (s != "FR")
			return false;
		flags |= 1 << 16;
		return true;
	}
	if (ch == 'F')
		flags |= FPlanWaypoint::altflag_standard;
	else if (ch != 'A')
		return false;
	unsigned int num;
	if (!parsenum(num, 3, txti, txte))
		return false;
	alt = num * 100;
	return true;
}

void FlightPlan::otherinfo_clear(const std::string& category)
{
	otherinfo_t::iterator i(m_otherinfo.find(Aircraft::OtherInfo(category)));
	if (i != m_otherinfo.end())
		m_otherinfo.erase(i);
}

const Glib::ustring& FlightPlan::otherinfo_get(const std::string& category)
{
	static const Glib::ustring empty;
	otherinfo_t::iterator i(m_otherinfo.find(Aircraft::OtherInfo(category)));
	if (i == m_otherinfo.end())
		return empty;
	return i->get_text();
}

void FlightPlan::otherinfo_add(const std::string& category, std::string::const_iterator texti, std::string::const_iterator texte)
{
	if (category.empty())
		return;
	trim(texti, texte);
	if (texti == texte)
		return;
	std::pair<otherinfo_t::iterator,bool> ins(m_otherinfo.insert(Aircraft::OtherInfo(category, std::string(texti, texte))));
	if (ins.second)
		return;
	Aircraft::OtherInfo oi(category, ins.first->get_text() + " " + std::string(texti, texte));
	m_otherinfo.erase(ins.first);
	m_otherinfo.insert(oi);
}

void FlightPlan::otherinfo_replace(const std::string& category, std::string::const_iterator texti, std::string::const_iterator texte)
{
	if (category.empty())
		return;
	trim(texti, texte);
	Aircraft::OtherInfo oi(category, std::string(texti, texte));
	otherinfo_t::iterator i(m_otherinfo.find(oi));
	if (i != m_otherinfo.end())
		m_otherinfo.erase(i);
	if (!oi.get_text().empty())
		m_otherinfo.insert(oi);	
}

void FlightPlan::set_departuretime(time_t x)
{
	time_t tdiff(x - m_departuretime);
	m_departuretime = x;
	for (iterator i(begin()), e(end()); i != e; ++i)
		i->set_time_unix(i->get_time_unix() + tdiff);
	otherinfo_clear("DOF");
	if (x < 24*60*60)
		return;
	struct tm tm;
        if (!gmtime_r(&x, &tm))
                return;
	std::ostringstream oss;
	oss << std::setw(2) << std::setfill('0') << (tm.tm_year % 100)
	    << std::setw(2) << std::setfill('0') << (tm.tm_mon + 1)
	    << std::setw(2) << std::setfill('0') << tm.tm_mday;
	otherinfo_add("DOF", oss.str());
}

FlightPlan::parseresult_t FlightPlan::parse(Database& db, std::string::const_iterator txti, std::string::const_iterator txte, bool do_expand_airways)
{
        errors_t errors;
	clear();
	{
		trimleft(txti, txte);
		if (txti != txte && *txti == '-') {
			++txti;
			trimleft(txti, txte);
		}
		if (txti == txte || *txti != '(') {
			errors.push_back("opening parenthesis not found");
			return std::make_pair(txti, errors);
		}
		++txti;
		std::string s;
		if (!parsetxt(s, 3, txti, txte) || s != "FPL") {
			errors.push_back("flight plan does not start with FPL");
			return std::make_pair(txti, errors);
		}
		if (txti == txte || *txti != '-') {
			errors.push_back("dash expected after FPL");
			return std::make_pair(txti, errors);
		}
		++txti;
	}
	{
		std::string s;
		if (!parsetxt(s, 0, txti, txte) || s.size() < 1 || s.size() > 7) {
			errors.push_back("invalid callsign");
			return std::make_pair(txti, errors);
		}
		set_aircraftid(s);
		if (txti == txte || *txti != '-') {
			errors.push_back("dash expected after callsign");
			return std::make_pair(txti, errors);
		}
		++txti;
		trim(txti, txte);
	}
	if (txti == txte || (*txti != 'V' && *txti != 'I' && *txti != 'Z' && *txti != 'Y')) {
		errors.push_back("invalid flight rules");
		return std::make_pair(txti, errors);
	}
	uint16_t departureflags(0);
	if (*txti == 'I' || *txti == 'Y')
		departureflags |= FPlanWaypoint::altflag_ifr;
	++txti;
	trim(txti, txte);
	if (txti == txte || !isalpha(*txti)) {
		errors.push_back("invalid flight type");
		return std::make_pair(txti, errors);
	}
	set_flighttype(*txti);
	++txti;
	trim(txti, txte);
	if (txti == txte || *txti != '-') {
		errors.push_back("dash expected aircraft type");
		return std::make_pair(txti, errors);
	}
	++txti;
	{
		unsigned int num;
		set_number(1);
		if (parsenum(num, 0, txti, txte))
			set_number(num);
	}
	{
		std::string s;
		if (!parsetxt(s, 0, txti, txte) || s.size() < 2 || s.size() > 4) {
			errors.push_back("invalid aircraft type");
			return std::make_pair(txti, errors);
		}
		set_aircrafttype(s);
	}
	if (txti == txte || *txti != '/') {
		errors.push_back("slash expected");
		return std::make_pair(txti, errors);
	}
	++txti;
	trim(txti, txte);
	if (txti == txte || !isalpha(*txti)) {
		errors.push_back("invalid wake turbulence category");
		return std::make_pair(txti, errors);
	}
	set_wakecategory(*txti);
	++txti;
	trim(txti, txte);
	if (txti == txte || *txti != '-') {
		errors.push_back("dash expected before equipment");
		return std::make_pair(txti, errors);
	}
	++txti;
	{
		std::string s;
		if (!parsetxt(s, 0, txti, txte)) {
			errors.push_back("invalid equipment");
			return std::make_pair(txti, errors);
		}
		set_equipment(s);
	}
	if (txti == txte || *txti != '/') {
		errors.push_back("slash expected");
		return std::make_pair(txti, errors);
	}
	++txti;
	trim(txti, txte);
	{
		std::string s;
		if (txti == txte || !parsetxt(s, 0, txti, txte) || s.size() < 1) {
			errors.push_back("invalid transponder");
			return std::make_pair(txti, errors);
		}
		set_transponder(s);
	}
	trim(txti, txte);
	if (txti == txte || *txti != '-') {
		errors.push_back("dash expected before departure");
		return std::make_pair(txti, errors);
	}
	++txti;
	ParseState state(db);
	{
		std::string s;
		if (!parsetxt(s, 4, txti, txte)) {
			errors.push_back("invalid departure");
			return std::make_pair(txti, errors);
		}
		ParseWaypoint w;
		w.set_icao(s);
		w.set_type(FPlanWaypoint::type_airport);
		w.set_coord(Point::invalid);
		state.add(w);
	}
	{
		time_t t;
		if (!parsetime(t, txti, txte)) {
			errors.push_back("invalid off-block");
			return std::make_pair(txti, errors);
		}
		m_departuretime = t;
		state[0].set_time_unix(t + 5 * 60);
	}
	if (txti == txte || *txti != '-') {
		errors.push_back("dash expected before speed");
		return std::make_pair(txti, errors);
	}
	++txti;
	// parse airspeed and altitude
	{
		std::string spdstr;
		float spd;
		if (!parsespeed(spdstr, spd, txti, txte)) {
			std::string::const_iterator txti1(txti);
			std::string txt;
			if (parsetxt(txt, 0, txti1, txte))
				txt = " " + txt;
			errors.push_back("invalid speed" + txt);
			return std::make_pair(txti, errors);
		}
		set_cruisespeed(spdstr);
		int32_t alt(std::numeric_limits<int32_t>::min());
		unsigned int altflags(0);
		if (!parsealt(alt, altflags, txti, txte)) {
			std::string::const_iterator txti1(txti);
			std::string txt;
			if (parsetxt(txt, 0, txti1, txte))
				txt = " " + txt;
			errors.push_back("invalid altitude" + txt);
			return std::make_pair(txti, errors);
		}
		departureflags &= ~FPlanWaypoint::altflag_standard;
		departureflags |= altflags & FPlanWaypoint::altflag_standard;
		state[0].set_altitude(alt);
		state[0].set_flags(departureflags);
		state[0].set_tas_kts(spd);
		state.add_cruisespeed(std::min(alt, (int32_t)0), spd);
	}
	// parse route
	while (txti != txte && *txti != '-' && *txti != ')') {
		std::string txt;
		if (!parsetxt(txt, 0, txti, txte, false)) {
			errors.push_back("invalid route element " + txt);
			return std::make_pair(txti, errors);
		}
		ParseWaypoint w;
		w.set_icao(txt);
		w.set_type(FPlanWaypoint::type_undefined);
		w.set_coord(Point::invalid);
		state.add(w);
	}
	if (txti == txte || *txti != '-') {
		errors.push_back("dash expected before destination");
		return std::make_pair(txti, errors);
	}
	++txti;
	{
		std::string s;
		if (!parsetxt(s, 4, txti, txte)) {
			errors.push_back("invalid destination");
			return std::make_pair(txti, errors);
		}
		ParseWaypoint w;
		w.set_icao(s);
		w.set_type(FPlanWaypoint::type_airport);
		w.set_coord(Point::invalid);
		state.add(w);
	}
	{
		time_t t;;
		if (!parsetime(t, txti, txte)) {
			errors.push_back("invalid total eet");
			return std::make_pair(txti, errors);
		}
		set_totaleet(t);
	}
	if (txti != txte && std::isalpha(*txti)) {
		std::string s;
		if (!parsetxt(s, 4, txti, txte)) {
			errors.push_back("invalid first alternate");
			return std::make_pair(txti, errors);
		}
		set_alternate1(s);
	}
	if (txti != txte && std::isalpha(*txti)) {
		std::string s;
		if (!parsetxt(s, 4, txti, txte)) {
			errors.push_back("invalid second alternate");
			return std::make_pair(txti, errors);
		}
		set_alternate2(s);
	}
	if (txti != txte && *txti == '-') {
		++txti;
		std::string cat;
		while (txti != txte && *txti != '-' && *txti != ')') {
			std::string s;
			if (!parsetxt(s, 0, txti, txte)) {
				errors.push_back("invalid other information");
				return std::make_pair(txti, errors);
			}
			if (txti != txte && *txti == '/') {
				cat.swap(s);
				++txti;
				continue;
			}
			if (cat.empty()) {
				errors.push_back("invalid other information");
				return std::make_pair(txti, errors);
			}
			if (cat == "PBN")
				set_pbn(s);
			else
				otherinfo_add(cat, s);
		}
		if (txti != txte && *txti == '-') {
			++txti;
			std::string cat;
			while (txti != txte && *txti != '-' && *txti != ')') {
				std::string s;
				if (!parsetxt(s, 0, txti, txte)) {
					errors.push_back("invalid other2 information");
					return std::make_pair(txti, errors);
				}
				if (txti != txte && *txti == '/') {
					cat.swap(s);
					++txti;
					if (cat == "E") {
						time_t t;
						if (!parsetime(t, txti, txte)) {
							errors.push_back("invalid endurance");
							return std::make_pair(txti, errors);
						}
						set_endurance(t);
						continue;
					}
					if (cat == "P") {
						unsigned int num;
						set_personsonboard_tbn();
						if (parsenum(num, 3, txti, txte)) {
							set_personsonboard(num);
							continue;
						}
						if (!parsetxt(s, 3, txti, txte) || s != "TBN") {
							errors.push_back("invalid number of persons on board");
							return std::make_pair(txti, errors);
						}
						continue;
					}
					continue;
				}
				if (cat.empty()) {
					errors.push_back("invalid other2 information");
					return std::make_pair(txti, errors);
				}
				if (cat == "A") {
					if (get_colourmarkings().empty())
						set_colourmarkings(s);
					else
						set_colourmarkings(get_colourmarkings() + " " + s);
					continue;
				}
				if (cat == "N") {
					if (get_remarks().empty())
						set_remarks(s);
					else
						set_remarks(get_remarks() + " " + s);
					continue;
				}
				if (cat == "C") {
					if (get_picname().empty())
						set_picname(s);
					else
						set_picname(get_picname() + " " + s);
					continue;
				}
				if (cat == "R") {
					if (get_emergencyradio().empty())
						set_emergencyradio(s);
					else
						set_emergencyradio(get_emergencyradio() + " " + s);
					continue;
				}
				if (cat == "S") {
					if (get_survival().empty())
						set_survival(s);
					else
						set_survival(get_survival() + " " + s);
					continue;
				}
				if (cat == "J") {
					if (get_lifejackets().empty())
						set_lifejackets(s);
					else
						set_lifejackets(get_lifejackets() + " " + s);
					continue;
				}
				if (cat == "D") {
					if (get_dinghies().empty())
						set_dinghies(s);
					else
						set_dinghies(get_dinghies() + " " + s);
					continue;
				}
			}
		}
	}
	{
		const std::string& dof(otherinfo_get("DOF"));
		bool ok(dof.size() == 6);
		unsigned int dofn(0);
		if (ok) {
			char *cp;
			dofn = strtoul(dof.c_str(), &cp, 10);
			ok = !*cp;
		}
		if (ok) {
			struct tm tm;
			if (gmtime_r(&m_departuretime, &tm)) {
				tm.tm_mday = dofn % 100U;
				tm.tm_mon = (dofn / 100U) % 100U - 1U;
				tm.tm_year = dofn / 10000U + 100U;
				time_t t = timegm(&tm);
				if (t != -1) {
					set_departuretime(t);
					state[0].set_time_unix(t + 5 * 60);
				}
			}
		}
	}
	{
		if (m_departuretime < 24*60*60) {
			time_t t;
			time(&t);
			struct tm tm;
			time_t y(t);
			t += 30*60;
			for (unsigned int iter(0); iter < 4; ++iter) {
				if (!gmtime_r(&y, &tm))
					break;
				tm.tm_min = m_departuretime / 60;
				tm.tm_sec = m_departuretime - tm.tm_min * 60;
				tm.tm_hour = tm.tm_min / 60;
				tm.tm_min -= tm.tm_hour * 60;
				y = timegm(&tm);
				if (y == -1)
					break;
				if (y >= t) {
					set_departuretime(y);
					state[0].set_time_unix(y + 5 * 60);
					break;
				}
				y += 24*60*60;
			}
		}
	}
	for (ParseState::wpts_t::size_type i(1), n(state.size()); i < n; ++i)
		state[i].set_time_unix(state[0].get_time_unix());
	state[0].set_name(otherinfo_get("DEP"));
	state[state.size() - 1].set_name(otherinfo_get("DEST"));
	if (trace_parsestate)
		state.print(std::cerr << "Flight Plan after Parsing" << std::endl);
	state.process_speedalt();
	if (trace_parsestate)
		state.print(std::cerr << "Flight Plan after Speed/Alt processing" << std::endl);
	state.process_dblookup(true);
	if (trace_parsestate)
		state.print(std::cerr << "Flight Plan after DB lookup" << std::endl);
	state.process_airways(do_expand_airways, true);
	state.process_time(otherinfo_get("EET"));
	if (trace_parsestate)
		state.print(std::cerr << "Flight Plan after airways processing" << std::endl);
	{
		ParseState::wpts_t::size_type n(state.size());
		if (n < 2) {
			errors.push_back("no waypoints after processing");
			return std::make_pair(txti, errors);
		}
		for (ParseState::wpts_t::size_type i(0); i < n; ++i) {
			push_back(state[i]);
			if (back().get_altitude() == std::numeric_limits<int32_t>::min())
				back().set_altitude(m_defaultalt);
		}
		recompute_dist();
		recompute_decl();
	}
	if (AirportsDb::Airport::is_fpl_zzzz(front().get_icao())) {
		front().set_icao("ZZZZ");
		if (!front().get_coord().is_invalid() && otherinfo_get("DEP").empty())
			otherinfo_add("DEP", front().get_coord().get_fpl_str());
	} else if (!front().get_coord().is_invalid()) {
		otherinfo_clear("DEP");
	}
	if (AirportsDb::Airport::is_fpl_zzzz(back().get_icao())) {
		back().set_icao("ZZZZ");
		if (!back().get_coord().is_invalid() && otherinfo_get("DEST").empty())
			otherinfo_add("DEST", back().get_coord().get_fpl_str());
	} else if (!back().get_coord().is_invalid()) {
		otherinfo_clear("DEST");
	}
	{
		typedef std::set<unsigned int> staynr_t;
		staynr_t staynr;
		for (const_iterator ri(begin()), re(end()); ri != re; ++ri) {
			unsigned int nr, tm;
			if (!ri->is_stay(nr, tm))
				continue;
			if (staynr.find(nr) != staynr.end()) {
				std::ostringstream oss;
				oss << "duplicate STAY" << nr;
				errors.push_back(oss.str());
			}
			staynr.insert(nr);
			if (ri == begin() || (ri + 1) == re || (ri + 2) == re)
				errors.push_back("STAY cannot be used for airport connection");
		}
		if (!staynr.empty() && (*staynr.rbegin() - *staynr.begin()) != (staynr.size() - 1))
			errors.push_back("multiple STAY but not consecutive");
		for (staynr_t::const_iterator si(staynr.begin()), se(staynr.end()); si != se; ++si) {
			std::ostringstream oss;
			oss << "STAYINFO" << (*si + 1);
			if (!otherinfo_get(oss.str()).empty())
				continue;
			errors.push_back(oss.str() + " missing");
		}
	}
	m_cruisespeeds = state.get_cruisespeeds();
	normalize_pogo();
	errors.insert(errors.end(), state.get_errors().begin(), state.get_errors().end());
	return std::make_pair(txti, errors);
}

FlightPlan::parseresult_t FlightPlan::parse_route(Database& db, std::vector<FPlanWaypoint>& wpts, std::string::const_iterator txti, std::string::const_iterator txte, bool expand_airways)
{
	bool hasstart(!wpts.empty());
	ParseState state(db);
	if (hasstart)
		state.add(wpts.back());
	errors_t errors;
	while (txti != txte) {
		trimleft(txti, txte);
		if (txti == txte)
			break;
		if (*txti == '-') {
			++txti;
			continue;
		}
		std::string txt;
		if (!parsetxt(txt, 0, txti, txte, false)) {
			errors.push_back("invalid route element " + txt);
			break;
		}
		ParseWaypoint w;
		w.set_icao(txt);
		w.set_type(FPlanWaypoint::type_undefined);
		w.set_coord(Point::invalid);
		w.set_time_unix(time(0));
		state.add(w);
	}
	if (trace_parsestate)
		state.print(std::cerr << "Flight Plan after Parsing" << std::endl);
	state.process_speedalt();
	if (trace_parsestate)
		state.print(std::cerr << "Flight Plan after Speed/Alt processing" << std::endl);
	state.process_dblookup(false);
	if (trace_parsestate)
		state.print(std::cerr << "Flight Plan after DB lookup" << std::endl);
	state.process_airways(expand_airways, false);
	state.process_time();
	if (trace_parsestate)
		state.print(std::cerr << "Flight Plan after airways processing" << std::endl);
	for (ParseState::wpts_t::size_type i(hasstart), n(state.size()); i < n; ++i) {
		wpts.push_back(state[i]);
		if (wpts.back().get_altitude() == std::numeric_limits<int32_t>::min())
			wpts.back().set_altitude(m_defaultalt);
	}
	recompute_dist();
	recompute_decl();
	errors.insert(errors.end(), state.get_errors().begin(), state.get_errors().end());
	return std::make_pair(txti, errors);
}

FlightPlan::errors_t FlightPlan::reparse(Database& db, bool do_expand_airways)
{
        errors_t errors;
	ParseState state(db);
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		state.add(*i);
	base_t::clear();
	for (ParseState::wpts_t::size_type i(1), n(state.size()); i < n; ++i)
		state[i].set_time_unix(state[0].get_time_unix());
	state.process_dblookup(true);
	if (trace_parsestate)
		state.print(std::cerr << "Flight Plan after DB lookup" << std::endl);
	state.process_airways(do_expand_airways, true);
	state.process_time(otherinfo_get("EET"));
	if (trace_parsestate)
		state.print(std::cerr << "Flight Plan after airways processing" << std::endl);
	{
		ParseState::wpts_t::size_type n(state.size());
		if (n < 2) {
			errors.push_back("no waypoints after processing");
			return errors;
		}
		for (ParseState::wpts_t::size_type i(0); i < n; ++i) {
			push_back(state[i]);
			if (back().get_altitude() == std::numeric_limits<int32_t>::min())
				back().set_altitude(m_defaultalt);
		}
		recompute_dist();
		recompute_decl();
	}
	if (AirportsDb::Airport::is_fpl_zzzz(front().get_icao())) {
		front().set_icao("ZZZZ");
		if (!front().get_coord().is_invalid() && otherinfo_get("DEP").empty())
			otherinfo_add("DEP", front().get_coord().get_fpl_str());
	} else if (!front().get_coord().is_invalid()) {
		otherinfo_clear("DEP");
	}
	if (AirportsDb::Airport::is_fpl_zzzz(back().get_icao())) {
		back().set_icao("ZZZZ");
		if (!back().get_coord().is_invalid() && otherinfo_get("DEST").empty())
			otherinfo_add("DEST", back().get_coord().get_fpl_str());
	} else if (!back().get_coord().is_invalid()) {
		otherinfo_clear("DEST");
	}
	m_cruisespeeds = state.get_cruisespeeds();
	normalize_pogo();
	errors.insert(errors.end(), state.get_errors().begin(), state.get_errors().end());
	return errors;
}

Rect FlightPlan::get_bbox(void) const
{
	Rect bbox(Point::invalid, Point::invalid);	
	bool first(true);
	for (const_iterator ri(begin()), re(end()); ri != re; ++ri) {
		if (ri->get_coord().is_invalid())
			continue;
		if (first) {
			bbox = Rect(ri->get_coord(), ri->get_coord());
			first = false;
			continue;
		}
      		bbox = bbox.add(ri->get_coord());
	}
	return bbox;
}

char FlightPlan::get_flightrules(void) const
{
	const_iterator ri(begin()), re(end());
	if (ri == re)
		return 'Z';
	unsigned int ftype(ri->is_ifr());
	for (++ri; ri != re; ++ri) {
		unsigned int ft(ri->is_ifr());
		ft ^= ftype;
		ft &= 1;
		ftype |= ft << 1;
		if (ft)
			break;
	}
	static const char frules[4] = { 'V', 'I', 'Z', 'Y' };
	return frules[ftype];	
}

FlightPlan::nameset_t FlightPlan::intersect(const nameset_t& s1, const nameset_t& s2)
{
	nameset_t r;
	std::set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(), std::inserter(r, r.begin()));
	return r;
}

FlightPlan::errors_t FlightPlan::populate(Database& db, const FPlanRoute& route, bool expand_airways)
{
	clear();
	uint16_t nrwpt(route.get_nrwpt());
	if (!nrwpt)
		return errors_t();
	ParseState state(db);
	{
		uint16_t nrwpt(route.get_nrwpt());
		for (unsigned int i(0); i < nrwpt; ++i) {
			push_back(route[i]);
			if (back().get_icao().empty())
				back().set_name(upcase(back().get_name()));
			else
				back().set_icao(upcase(back().get_icao()));
		}
	}
	set_departuretime(route.get_time_offblock_unix());
	m_totaleet = route[nrwpt - 1].get_time_unix() - route[0].get_time_unix();
	m_defaultalt = std::max(0, (int)std::max(front().get_altitude(), back().get_altitude()));
	if (m_defaultalt >= 5000)
		m_defaultalt += 1000;
	m_defaultalt += 1999;
	m_defaultalt /= 1000;
	m_defaultalt *= 1000;
	state.process_dblookupcoord();
	if (trace_parsestate)
		state.print(std::cerr << "Flight Plan after DB lookup" << std::endl);
	state.process_airways(expand_airways, true);
	if (trace_parsestate)
		state.print(std::cerr << "Flight Plan after airways processing" << std::endl);
	errors_t errors;
	{
		ParseState::wpts_t::size_type n(state.size());
		if (n < 2) {
			errors.push_back("no waypoints after processing");
			return errors;
		}
		for (ParseState::wpts_t::size_type i(0); i < n; ++i) {
			push_back(state[i]);
			if (back().get_altitude() == std::numeric_limits<int32_t>::min())
				back().set_altitude(m_defaultalt);
		}
	}
	otherinfo_clear("DEP");
	if (AirportsDb::Airport::is_fpl_zzzz(front().get_icao())) {
		front().set_icao("ZZZZ");
		otherinfo_add("DEP", upcase(front().get_name()) + " " + front().get_coord().get_fpl_str());
		if (false)
			std::cerr << "DEP: " << otherinfo_get("DEP") << std::endl;
	}
	otherinfo_clear("DEST");
	if (AirportsDb::Airport::is_fpl_zzzz(back().get_icao())) {
		back().set_icao("ZZZZ");
		otherinfo_add("DEST", upcase(back().get_name()) + " " + back().get_coord().get_fpl_str());
		if (false)
			std::cerr << "DEST: " << otherinfo_get("DEST") << std::endl;
	}
	m_cruisespeeds = state.get_cruisespeeds();
	enforce_pathcode_vfrifr();
	normalize_pogo();
	errors.insert(errors.end(), state.get_errors().begin(), state.get_errors().end());
	{
		bool ifr(false);
		for (const_iterator i(begin()), e(end()); i != e && !ifr; ++i)
			ifr = ifr || i->is_ifr();
		if (ifr) {
			static const char *ifpsra = /*"IFPS RTE AMDT ACPT"*/ "IFPSRA";
			const std::string& rmk(otherinfo_get("RMK"));
			if (rmk.find(ifpsra) == std::string::npos)
				otherinfo_add("RMK", ifpsra);
		}
	}
	add_eet();
	if (false) {
		std::cerr << "Other Info:";
                for (otherinfo_t::const_iterator oi(m_otherinfo.begin()), oe(m_otherinfo.end()); oi != oe; ++oi)
			std::cerr << ' ' << oi->get_category() << '/' << oi->get_text();
		std::cerr << std::endl;
	}
	return errors;
}

FlightPlan::errors_t FlightPlan::expand_composite(const Graph& g)
{
	typedef std::vector<bool> sidstar_t;
	sidstar_t isid(size(), false), istar(size(), false);
	errors_t errors;
	for (iterator i(begin()), e(end()); i != e;) {
		const FPLWaypoint& wpt0(*i);
		++i;
		if (i == e)
			break;
		const IdentTimeSlice& ts(wpt0.get_pathobj()->operator()(wpt0.get_time_unix()).as_ident());
		if (!ts.is_valid())
			continue;
		const FPLWaypoint& wpt1(*i);
		bool issid(ts.as_sid().is_valid());
		bool isstar(ts.as_star().is_valid());
		if (!(issid || isstar || ts.as_route().is_valid())) {
			errors.push_back("Path " + wpt0.get_ident() + "--" + wpt0.get_pathname() +
					 "->" + wpt1.get_ident() + " is neither Airway, SID nor STAR");
			continue;
		}
		Graph::vertex_descriptor u, v;
		{
			bool ok;
			boost::tie(u, ok) = g.find_vertex(wpt0.get_ptobj());
			if (!ok) {
				errors.push_back("Cannot find vertex for " + wpt0.get_ident());
				continue;
			}
			boost::tie(v, ok) = g.find_vertex(wpt1.get_ptobj());
			if (!ok) {
				errors.push_back("Cannot find vertex for " + wpt1.get_ident());
				continue;
			}
		}
		if (u == v) {
			errors.push_back("Same vertex for consecutive waypoints " + wpt0.get_ident() + "," + wpt1.get_ident());
			continue;
		}
		Graph::UUIDEdgeFilter filter(g, wpt0.get_pathobj()->get_uuid(), GraphEdge::matchcomp_segment);
		typedef boost::filtered_graph<Graph, Graph::UUIDEdgeFilter> fg_t;
		fg_t fg(g, filter);
		std::vector<Graph::vertex_descriptor> predecessors(boost::num_vertices(g), 0);
		dijkstra_shortest_paths(fg, u, boost::weight_map(boost::get(&GraphEdge::m_dist, g)).
					predecessor_map(&predecessors[0]));
		if (predecessors[v] == v) {
			std::ostringstream oss;
			oss << "Cannot find path for " << wpt0.get_ident() << "--" << wpt0.get_pathname() << "->" << wpt1.get_ident();
			if (true)
				oss << " (" << wpt0.get_ptobj()->get_uuid() << ' ' << wpt0.get_pathobj()->get_uuid() << ' '
				    << wpt1.get_ptobj()->get_uuid() << ')';
			errors.push_back(oss.str());
			continue;
		}
		double totdist(wpt0.get_dist_nmi());
		time_t tottime(wpt1.get_time_unix() - wpt0.get_time_unix());
		Graph::vertex_descriptor v1(v);
		for (;;) {
			Graph::vertex_descriptor u1(predecessors[v1]);
			if (u1 == v1)
				break;
			Graph::edge_descriptor e;
			{
				fg_t::out_edge_iterator e0, e1;
				for(boost::tie(e0, e1) = boost::out_edges(u1, fg); e0 != e1; ++e0) {
					if (boost::target(*e0, fg) != v1)
						continue;
					e = *e0;
					break;
				}
				if (e0 == e1) {
					errors.push_back("Error finding path for " + wpt0.get_ident() + "--" +
							 wpt0.get_pathname() + "->" + wpt1.get_ident());
					break;
				}
			}
			const GraphEdge& ee(g[e]);
			if (u1 == u) {
				FPLWaypoint& wpt(*(i-1));
				wpt.set_pathobj(ee.get_object());
				wpt.set_dist_nmi(wpt.get_coord().spheric_distance_nmi_dbl(i->get_coord()));
				v1 = u1;
				continue;
			}
			{
				size_type j(i - begin());
				isid.insert(isid.begin() + j, issid);
				istar.insert(istar.begin() + j, isstar);
			}
			if (isstar) {
				i = insert(i, *(i-1));
				i->frob_flags(FPlanWaypoint::altflag_standard, FPlanWaypoint::altflag_standard);
			} else {
				i = insert(i, *i);
				i->set_wptnr((i-1)->get_wptnr());
			}
			FPLWaypoint& wpt(*i);
			wpt.set_pathobj(ee.get_object());
			wpt.set_ptobj(g[u1].get_object());
			wpt.set_from_objects(false);
			double dist(i->get_coord().spheric_distance_nmi_dbl((i+1)->get_coord()));
			wpt.set_dist_nmi(dist);
			if (totdist > 0) {
				time_t tdiff(tottime * dist / totdist);
				if (isstar)
					wpt.set_time_unix(wpt.get_time_unix() + tdiff);
				else
					wpt.set_time_unix(wpt.get_time_unix() - tdiff);
			}
			wpt.set_expanded(true);
			v1 = u1;
		}
		e = end();
	}
	for (iterator i(begin()), e(end()); i != e; ++i) {
		bool issid, isstar;
		{
			size_type j(i - begin());
			issid = isid[j];
			isstar = istar[j];
		}
		if (!issid && !isstar)
			continue;
		if (i == begin())
			continue;
		FPLWaypoint& wpt(*i);
		const SegmentTimeSlice& tsn(wpt.get_pathobj()->operator()(wpt.get_time_unix()).as_segment());
		if (!tsn.is_valid())
			continue;
		const FPLWaypoint& wptp(*(i - 1));
		const SegmentTimeSlice& tsp(wptp.get_pathobj()->operator()(wpt.get_time_unix()).as_segment());
		if (!tsp.is_valid())
			continue;
		AltRange ar(tsp.get_altrange());
		ar.intersect(tsn.get_altrange());
		if (ar.get_lower_mode() <= AltRange::alt_height && ar.get_lower_alt() > wpt.get_altitude())
			continue;
		if (ar.get_upper_mode() <= AltRange::alt_height && ar.get_upper_alt() < wpt.get_altitude())
			wpt.set_altitude(ar.get_upper_alt());
	}
	return errors;
}

bool FlightPlan::has_pathcodes(void) const
{
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		if (i->get_pathcode() != FPlanWaypoint::pathcode_none)
			return true;
	return false;
}

bool FlightPlan::enforce_pathcode_vfrdeparr(void)
{
	unsigned int nr(size());
	if (nr < 1U)
		return false;
	bool work(false);
	if (!front().is_ifr()) {
		unsigned int i(1);
		for (; i + 1U < nr; ++i) {
			const FPLWaypoint& wpt(operator[](i));
			if (wpt.get_icao() != front().get_icao() || wpt.is_ifr() ||
			    wpt.get_pathcode() == FPlanWaypoint::pathcode_vfrarrival ||
			    wpt.get_pathcode() == FPlanWaypoint::pathcode_vfrtransition)
				break;
		}
		if (i > 0) {
			work = true;
			do {
				--i;
				operator[](i).set_pathcode(FPlanWaypoint::pathcode_vfrdeparture);
			} while (i > 0);
		}
	}
	if (!back().is_ifr()) {
		unsigned int i(nr);
		if (i > 0) {
			--i;
			while (i > 0) {
				--i;
				const FPLWaypoint& wpt(operator[](i));
				if (wpt.get_icao() != back().get_icao() || wpt.is_ifr() ||
				    wpt.get_pathcode() == FPlanWaypoint::pathcode_vfrdeparture ||
				    wpt.get_pathcode() == FPlanWaypoint::pathcode_vfrtransition)
					break;
			}
		}
		if (i < nr - 1) {
			work = true;
			do {
				++i;
				operator[](i).set_pathcode(FPlanWaypoint::pathcode_vfrarrival);
			} while (i < nr - 1);
		}
	}
	{
		FPLWaypoint& wpt(operator[](nr - 1));
		work = work || wpt.get_pathcode() != FPlanWaypoint::pathcode_none ||
			!wpt.get_pathname().empty();
		wpt.set_pathcode(FPlanWaypoint::pathcode_none);
		wpt.set_pathname("");
	}
	return work;
}

bool FlightPlan::enforce_pathcode_vfrifr(void)
{
	unsigned int nr(size());
	bool work(false);
	for (unsigned int i = 0; i < nr;) {
		FPLWaypoint& wpt(operator[](i));
		++i;
		if (i >= nr) {
			work = work || wpt.get_pathcode() != FPlanWaypoint::pathcode_none ||
				!wpt.get_pathname().empty();
			wpt.set_pathcode(FPlanWaypoint::pathcode_none);
			wpt.set_pathname("");
			break;
		}
		work = wpt.enforce_pathcode_vfrifr() || work;
	}
	return work;
}

bool FlightPlan::is_route_pogo(void) const
{
	if (empty())
		return false;
	return IcaoFlightPlan::is_route_pogo(front().get_icao(), back().get_icao());
}

void FlightPlan::disable_none(void)
{
	for (iterator ri(begin()), re(end()); ri != re; ++ri)
		ri->set_disabled(false);
}

void FlightPlan::disable_unnecessary(bool keep_turnpoints, bool include_dct, float maxdev)
{
	for (iterator ri(begin()), re(end()); ri != re; ++ri)
		ri->set_disabled(false);
	for (unsigned int nr = 1; nr + 3U < size(); ) {
		const FPLWaypoint& wpt0(operator[](nr));
		if (wpt0.get_pathcode() == FPlanWaypoint::pathcode_sid || wpt0.get_pathcode() == FPlanWaypoint::pathcode_star) {
			++nr;
			continue;
		}
		if (!wpt0.is_ifr()) {
			++nr;
			continue;
		}
		unsigned int nr1(nr);
		for (; nr1 + 3U < size(); ++nr1) {
			const FPLWaypoint& wpt2(operator[](nr1 + 2));
			FPLWaypoint& wpt1(operator[](nr1 + 1));
			if (!wpt1.is_ifr() || !wpt2.is_ifr())
				break;
			if (((wpt1.get_flags() ^ wpt2.get_flags()) & FPlanWaypoint::altflag_standard) ||
			    (wpt1.get_altitude() != wpt2.get_altitude()))
				break;
			if ((wpt0.get_pathcode() != FPlanWaypoint::pathcode_airway || wpt1.get_pathcode() != FPlanWaypoint::pathcode_airway ||
			     wpt0.get_pathname().empty() || wpt0.get_pathname() != wpt1.get_pathname()) &&
			    (!include_dct || wpt0.get_pathcode() != FPlanWaypoint::pathcode_directto ||
			     wpt1.get_pathcode() != FPlanWaypoint::pathcode_directto))
				break;
			if (keep_turnpoints) {
				bool ok(true);
				for (unsigned int i = nr; i <= nr1; ) {
					++i;
					const FPLWaypoint& wptx(operator[](i));
					Point pt(wptx.get_coord().spheric_nearest_on_line(wpt0.get_coord(), wpt2.get_coord()));
					if (pt.spheric_distance_nmi(wptx.get_coord()) <= maxdev)
						continue;
					ok = false;
					break;
				}
				if (!ok)
					break;
			}
			wpt1.set_disabled(true);
		}
		if (nr1 == nr) {
			++nr;
			continue;
		}
		nr = nr1 + 2;
	}
}

void FlightPlan::fix_max_dct_distance(double dct_limit)
{
	// check DCT limit
	for (iterator ri(end()), rb(begin()); ri != rb; ) {
		--ri;
		if (ri == rb)
			break;
		FPLWaypoint& wpt2(*ri);
		iterator ri1(ri);
		--ri1;
		FPLWaypoint& wpt1(*ri1);
		// VFR or DCT - check for too long segments (limit at 50nmi)
		if (!wpt1.is_ifr() || wpt1.get_pathcode() == FPlanWaypoint::pathcode_directto) {
			double dist(wpt1.get_coord().spheric_distance_nmi_dbl(wpt2.get_coord()));
			if (dist <= dct_limit)
				continue;
	                float crs(wpt1.get_coord().spheric_true_course(wpt2.get_coord()));
			unsigned int nseg(ceil(dist / dct_limit) + 0.1);
			if (nseg > 1)
				dist /= nseg;
			while (nseg > 1) {
				--nseg;
				FPLWaypoint w(wpt1);
				w.set_coord(wpt1.get_coord().spheric_course_distance_nmi(crs, dist * nseg));
				w.set_name("");
				w.set_icao("");
				ri = insert(ri, w);
				rb = begin();
			}
			continue;
		}
	}
}

void FlightPlan::erase_unnecessary_airway(bool keep_turnpoints, bool include_dct)
{
	for (unsigned int nr = 1; nr + 3U < size(); ) {
		unsigned int nr1(nr);
		for (; nr1 + 3U < size(); ++nr1) {
			const FPLWaypoint& wpt2(operator[](nr1 + 2));
			const FPLWaypoint& wpt1(operator[](nr1 + 1));
			const FPLWaypoint& wpt0(operator[](nr));
			if (!(wpt0.get_flags() & wpt1.get_flags() & FPlanWaypoint::altflag_ifr) ||
			    ((wpt0.get_flags() ^ wpt1.get_flags()) & FPlanWaypoint::altflag_standard) ||
			    (wpt0.get_altitude() != wpt1.get_altitude()))
				break;
			if ((wpt0.get_pathcode() != FPlanWaypoint::pathcode_airway || wpt1.get_pathcode() != FPlanWaypoint::pathcode_airway ||
			     wpt0.get_pathname().empty() || wpt0.get_pathname() != wpt1.get_pathname()) &&
			    (!include_dct || wpt0.get_pathcode() != FPlanWaypoint::pathcode_directto || wpt1.get_pathcode() != FPlanWaypoint::pathcode_directto))
				break;
			if (keep_turnpoints) {
				static const float maxdev(0.5);
				bool ok(true);
				for (unsigned int i = nr; i <= nr1; ) {
					++i;
					const FPLWaypoint& wptx(operator[](i));
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
		if (nr1 == nr) {
			++nr;
			continue;
		}
		while (nr1 > nr) {
			remove_eet(operator[](nr1).get_icao_or_name());
			erase(begin() + nr1);
			--nr1;
		}
	}
}

void FlightPlan::add_eet(void)
{
	bool clr(true);
	for (const_iterator ri(begin()), re(end()); ri != re; ++ri) {
		if (ri->is_disabled())
			continue;
		const std::string& ident(ri->get_icao_or_name());
		if (Engine::AirwayGraphResult::Vertex::is_ident_numeric(ident) ||
		    ident.size() < 2)
			continue;
		if (clr)
			otherinfo_clear("EET");
		clr = false;
		time_t t(ri->get_time_unix() - get_departuretime());
		std::ostringstream oss;
		if (!ident.empty())
			oss << upcase(ident);
		else
			oss << ri->get_coordstr();
		oss << std::setw(2) << std::setfill('0') << (t / 3600)
		    << std::setw(2) << std::setfill('0') << ((t / 60) % 60);
		otherinfo_add("EET", oss.str());
	}
}

std::vector<std::string> FlightPlan::tokenize(const std::string& s)
{
	std::vector<std::string> r;
	for (std::string::const_iterator si(s.begin()), se(s.end()); si != se; ) {
		if (std::isspace(*si)) {
			++si;
			continue;
		}
		std::string::const_iterator si2(si);
		++si2;
		while (si2 != se && !std::isspace(*si2))
			++si2;
		r.push_back(std::string(si, si2));
		si = si2;
	}
	return r;
}

std::string FlightPlan::untokenize(const std::vector<std::string>& s)
{
	bool space(false);
	std::string r;
	for (std::vector<std::string>::const_iterator si(s.begin()), se(s.end()); si != se; ++si) {
		if (space)
			r.push_back(' ');
		else
			space = true;
		r += *si;		
	}
	return r;
}

bool FlightPlan::EETSort::operator()(const std::string& a, const std::string& b) const
{
	if (a.size() < 4)
		return !(b.size() < 4);
	if (b.size() < 4)
		return false;
	return a.substr(a.size() - 4, 4) < b.substr(b.size() - 4, 4);
}

void FlightPlan::add_eet(const std::string& ident, time_t flttime)
{
	std::vector<std::string> eet(tokenize(otherinfo_get("EET")));
	std::string identu(Glib::ustring(ident).uppercase());
	for (std::vector<std::string>::iterator i(eet.begin()), e(eet.end()); i != e; ++i) {
		if (i->size() != identu.size() + 4 || i->compare(0, identu.size(), identu))
			continue;
		i = eet.erase(i);
		e = eet.end();
	}
	{
		std::ostringstream oss;
		oss << identu << std::setw(2) << std::setfill('0') << (flttime / 3600)
		    << std::setw(2) << std::setfill('0') << ((flttime / 60) % 60);
		eet.push_back(oss.str());
	}
	std::sort(eet.begin(), eet.end(), EETSort());
	otherinfo_clear("EET");
	otherinfo_add("EET", untokenize(eet));
}

void FlightPlan::remove_eet(const std::string& ident)
{
	std::vector<std::string> eet(tokenize(otherinfo_get("EET")));
	std::string identu(Glib::ustring(ident).uppercase());
	bool modif(false);
	for (std::vector<std::string>::iterator i(eet.begin()), e(eet.end()); i != e;) {
		if (i->size() != identu.size() + 4 || i->compare(0, identu.size(), identu)) {
			++i;
			continue;
		}
		i = eet.erase(i);
		e = eet.end();
		modif = true;
	}
	if (modif) {
		otherinfo_clear("EET");
		otherinfo_add("EET", untokenize(eet));
	}
}

void FlightPlan::normalize_pogo(void)
{
	static const std::string remark("RMK");
	static const std::string pogo("POGO");
	if (empty())
		return;
	std::vector<std::string> tok(tokenize(otherinfo_get(remark)));
	if (front().is_ifr() && back().is_ifr() && is_route_pogo()) {
		for (std::vector<std::string>::const_iterator i(tok.begin()), e(tok.end()); i != e; ++i)
			if (Glib::ustring(*i).uppercase() == pogo)
				return;
		otherinfo_add(remark, pogo);
		return;
	}
	bool chg(false);
	for (std::vector<std::string>::iterator i(tok.begin()), e(tok.end()); i != e; ++i) {
		if (Glib::ustring(*i).uppercase() != pogo)
			continue;
		i = tok.erase(i);
		e = tok.end();
		chg = true;
	}
	if (!chg)
		return;
	otherinfo_replace(remark, untokenize(tok));
}

std::string FlightPlan::get_fpl(void)
{
	// defaults
	static const std::string fpl_time = "1000";

	if (get_aircraftid().size() < 1 || get_aircraftid().size() > 7)
		return "Error: invalid aircraft ID: " + get_aircraftid();
	if (get_aircrafttype().size() < 2 || get_aircrafttype().size() > 4)
		return "Error: invalid aircraft type: " + get_aircrafttype();
	if (size() < 2)
		return "Error: invalid route";
	if (front().get_icao().size() != 4)
		return "Error: invalid departure: " + front().get_icao();
	if (back().get_icao().size() != 4)
		return "Error: invalid destination: " + back().get_icao();
	std::ostringstream fpl;
	fpl << "-(FPL-" << get_aircraftid() << '-' << get_flightrules() << get_flighttype()
	    << " -" << std::max(get_number(), 1U) << get_aircrafttype() << '/' << get_wakecategory()
	    << " -" << get_equipment() << '/' << get_transponder() << " -" << front().get_icao();
	{
		time_t t(get_departuretime());
		struct tm tm;
		if (gmtime_r(&t, &tm))
			fpl << std::setw(2) << std::setfill('0') << tm.tm_hour
			    << std::setw(2) << std::setfill('0') << tm.tm_min;
		else
			fpl << fpl_time;
	}
	fpl << " -" << get_item15() << " -" << back().get_icao();
	{
		time_t t(get_totaleet());
		struct tm tm;
		if (gmtime_r(&t, &tm))
			fpl << std::setw(2) << std::setfill('0') << tm.tm_hour
			    << std::setw(2) << std::setfill('0') << tm.tm_min;
		else
			fpl << "0000";
	}
	if (!get_alternate1().empty())
		fpl << ' ' << get_alternate1();
	if (!get_alternate2().empty())
		fpl << ' ' << get_alternate2();
	fpl << " -";
	{
		bool pbndone(get_pbn() == Aircraft::pbn_none);
		for (otherinfo_t::const_iterator oi(m_otherinfo.begin()), oe(m_otherinfo.end()); oi != oe; ++oi) {
			if (oi->get_category() == "EET") {
				std::vector<std::string> t(tokenize(oi->get_text()));
				for (const_iterator ri(begin()), re(end()); ri != re; ++ri) {
					const FPLWaypoint& wpt(*ri);
					if (!wpt.is_disabled())
						continue;
					const Glib::ustring& ident(wpt.get_icao_or_name());
					for (std::vector<std::string>::iterator i(t.begin()), e(t.end()); i != e; ) {
						if (i->size() != ident.size() || i->compare(0, ident.size(), ident)) {
							++i;
							continue;
						}
						i = t.erase(i);
						e = t.end();
					}
				}
				if (!t.empty())
					fpl << "EET/" << untokenize(t) << ' ';
				continue;
			}
			fpl << oi->get_category() << '/' << oi->get_text() << ' ';
			if (!pbndone && oi->get_category() == "PBN") {
				fpl << get_pbn_string() << ' ';
				pbndone = true;
			}
		}
		if (!pbndone)
			fpl << "PBN/" << get_pbn_string() << ' ';
	}
	fpl << '-';
	{
		bool subseq(false);
		if (get_endurance()) {
			time_t t(get_endurance());
			struct tm tm;
			if (gmtime_r(&t, &tm)) {
				fpl << "E/" << std::setw(2) << std::setfill('0') << tm.tm_hour
				    << std::setw(2) << std::setfill('0') << tm.tm_min;
				subseq = true;
			}
		}
		if (get_personsonboard()) {
			if (subseq)
				fpl << ' ';
			fpl << "P/";
			if (get_personsonboard() == ~0U)
				fpl << "TBN";
			else
				fpl << std::setw(3) << std::setfill('0') << get_personsonboard();
			subseq = true;
		}
		if (!get_emergencyradio().empty()) {
			if (subseq)
				fpl << ' ';
			fpl << "R/" << get_emergencyradio();
			subseq = true;
		}
		if (!get_survival().empty()) {
			if (subseq)
				fpl << ' ';
			fpl << "S/" << get_survival();
			subseq = true;
		}
		if (!get_lifejackets().empty()) {
			if (subseq)
				fpl << ' ';
			fpl << "J/" << get_lifejackets();
			subseq = true;
		}
		if (!get_dinghies().empty()) {
			if (subseq)
				fpl << ' ';
			fpl << "D/" << get_dinghies();
			subseq = true;
		}
		if (!get_colourmarkings().empty()) {
			if (subseq)
				fpl << ' ';
			fpl << "A/" << get_colourmarkings();
			subseq = true;
		}
		if (!get_remarks().empty()) {
			if (subseq)
				fpl << ' ';
			fpl << "N/" << get_remarks();
			subseq = true;
		}
		if (!get_picname().empty()) {
			if (subseq)
				fpl << ' ';
			fpl << "C/" << get_picname();
			subseq = true;
		}
	}
	fpl << ')';
	return fpl.str();
}

std::string FlightPlan::get_item15(void)
{
	std::ostringstream fpl;
	uint16_t prevflags(front().get_flags());
	int16_t prevalt(m_defaultalt);
	if (size() > 2) {
		prevflags ^= (prevflags ^ operator[](1).get_flags()) & FPlanWaypoint::altflag_standard;
		prevalt = operator[](1).get_altitude();
	}
	if (get_cruisespeed().empty()) {
		int alt(prevalt);
		int cs(Point::round<int,float>(get_cruisespeed(alt)));
		fpl << 'N' << std::setw(4) << std::setfill('0') << cs;
	} else {
		fpl << get_cruisespeed();
	}
	if (!(prevflags & FPlanWaypoint::altflag_ifr)) {
		fpl << "VFR";
	} else {
		fpl << ((prevflags & FPlanWaypoint::altflag_standard) ? 'F' : 'A')
		    << std::setw(3) << std::setfill('0') << ((prevalt + 50) / 100);
	}
	if (size() <= 2 && front().is_ifr() && back().is_ifr()) {
		fpl << " DCT";
	} else {
		const_iterator rb(begin()), re(end());
		const_iterator ri(rb);
		++rb;
		--re;
		if (ri->is_ifr() && ri->get_pathcode() == FPlanWaypoint::pathcode_sid) {
			fpl << ' ' << ri->get_pathname();
			++ri;
			while (ri != re && ri->is_ifr() && ri->get_pathcode() == FPlanWaypoint::pathcode_sid)
				++ri;
		} else if (!ri->is_ifr() && ri->get_pathcode() == FPlanWaypoint::pathcode_vfrdeparture) {
			do {
				++ri;
			} while (ri != re && !ri->is_ifr() && ri->get_pathcode() == FPlanWaypoint::pathcode_vfrdeparture);
		} else {
			++ri;
		}
		while (ri != re) {
			const FPLWaypoint& wpt(*ri);
			const Glib::ustring& ident(wpt.get_icao_or_name());
			// skip intermediate waypoints with non-flightplannable names
			if ((ident.empty() || Engine::AirwayGraphResult::Vertex::is_ident_numeric(ident) || ident.size() < 2) &&
			    wpt.get_pathcode() == FPlanWaypoint::pathcode_airway && ri != rb && ri != re) {
				const_iterator ri1(ri);
				--ri1;
				const FPLWaypoint& wptprev(*ri1);
				if (wptprev.get_pathcode() == FPlanWaypoint::pathcode_airway &&
				    wpt.get_pathname() == wptprev.get_pathname() &&
				    (wpt.get_flags() & wptprev.get_flags() & FPlanWaypoint::altflag_ifr)) {
					++ri;
					continue;
				}
			}
			for (;;) {
				++ri;
				if (ri == re)
					break;
				const FPLWaypoint& wpt(*ri);
				if (!wpt.is_disabled())
					break;
			}
			fpl << ' ';
			if (!ident.empty())
				fpl << upcase(ident);
			else
				fpl << wpt.get_coordstr();
			{
				// change of level or speed and/or change of rules
				bool lvlchg(prevalt != wpt.get_altitude() ||
					    ((prevflags ^ wpt.get_flags()) & FPlanWaypoint::altflag_standard) ||
					    ((wpt.get_flags() & ~prevflags) & FPlanWaypoint::altflag_ifr));
				uint16_t rulechg((prevflags ^ wpt.get_flags()) & FPlanWaypoint::altflag_ifr);
				prevalt = wpt.get_altitude();
				prevflags = wpt.get_flags();
				if (ri != re &&
				    ri->get_pathcode() != FPlanWaypoint::pathcode_star &&
				    ri->get_pathcode() != FPlanWaypoint::pathcode_vfrarrival) {
					const FPlanWaypoint& wptnext(*ri);
					uint16_t flagsnext(wptnext.get_flags());
					int altnext(wptnext.get_altitude());
					if ((prevflags ^ flagsnext) & FPlanWaypoint::altflag_standard ||
					    prevalt != altnext || (rulechg & flagsnext)) {
						fpl << '/';
						if (get_cruisespeed().empty()) {
							int alt(altnext);
							int cs(Point::round<int,float>(get_cruisespeed(alt)));
							fpl << 'N' << std::setw(4) << std::setfill('0') << cs;
						} else {
							fpl << get_cruisespeed();
						}
						fpl << ((flagsnext & FPlanWaypoint::altflag_standard) ? 'F' : 'A')
						    << std::setw(3) << std::setfill('0') << ((altnext + 50) / 100);
					}
				}
				if (rulechg)
					fpl << ' ' << ((prevflags & FPlanWaypoint::altflag_ifr) ? 'I' : 'V') << "FR";
			}
			if (ri == re)
				break;
			switch (wpt.get_pathcode()) {
			case FPlanWaypoint::pathcode_airway:
			case FPlanWaypoint::pathcode_star:
				fpl << ' ' << wpt.get_pathname();
				break;

			case FPlanWaypoint::pathcode_directto:
				if (ri == re)
					break;
				fpl << " DCT";
				break;

			default:
				break;
			}
			if (ri == re ||
			    wpt.get_pathcode() == FPlanWaypoint::pathcode_star ||
			    wpt.get_pathcode() == FPlanWaypoint::pathcode_vfrarrival)
				break;
		}
	}
	if ((prevflags ^ back().get_flags()) & FPlanWaypoint::altflag_ifr)
		fpl << ' ' << (back().is_ifr() ? 'I' : 'V') << "FR";
	return fpl.str();
}

void FlightPlan::recompute_eet(void)
{
	if (m_cruisespeeds.empty())
		return;
	time_t eet(0);
	for (iterator ri(begin()), re(end()); ri != re;) {
		iterator ri0(ri);
		++ri;
		if (ri == re)
			break;
		int alt(ri0->get_altitude() + ri->get_altitude());
		alt >>= 1;
		float tas(get_cruisespeed(alt));
		if (std::isnan(tas) || tas <= 0)
			return;
		float d(ri0->get_coord().spheric_distance_nmi(ri->get_coord()));
		float t(d / tas * 3600.0);
		eet += t;
	}
	set_totaleet(eet);
}

float FlightPlan::get_cruisespeed(int& alt) const
{
	if (m_cruisespeeds.empty())
		return std::numeric_limits<float>::quiet_NaN();
	cruisespeeds_t::const_iterator i1(m_cruisespeeds.lower_bound(alt));
	if (i1 == m_cruisespeeds.end()) {
		--i1;
		alt = i1->first;
		return i1->second;
	}
	if (i1->first == alt)
		return i1->second;
	if (i1 == m_cruisespeeds.begin()) {
		alt = i1->first;
		return i1->second;
	}
	cruisespeeds_t::const_iterator i0(i1);
	--i0;
	return (i0->second * (i1->first - alt) + i1->second * (alt - i0->first)) / static_cast<float>(i1->first - i0->first);
}

void FlightPlan::compute_cruisespeeds(double& avgtas, double& avgff, const Aircraft& acft, const Aircraft::Cruise::EngineParams& ep)
{
	m_cruisespeeds.clear();
	double avgff1(0), avgtas1(0);
	unsigned int avgnr(0);
	for (double pa = 0; pa < 60000; pa += 500) {
		Aircraft::Cruise::EngineParams ep1(ep);
		double pa1(pa), tas(0), fuelflow(0), mass(acft.get_mtom()), isaoffs(0), qnh(IcaoAtmosphere<double>::std_sealevel_pressure);
		acft.calculate_cruise(tas, fuelflow, pa1, mass, isaoffs, qnh, ep1);
		if (pa1 < pa - 100 || tas <= 0)
			break;
		m_cruisespeeds.insert(std::make_pair(static_cast<int>(pa1), static_cast<float>(tas)));
		if (pa < 5000) {
			avgff1 = avgtas1 = 0;
			avgnr = 0;
		}
		avgff1 += fuelflow;
		avgtas1 += tas;
		++avgnr;
	}
	if (avgnr) {
		avgff1 /= avgnr;
		avgtas1 /= avgnr;
	} else {
		avgff1 = avgtas1 = std::numeric_limits<double>::quiet_NaN();
	}
	if (&avgtas)
		avgtas = avgtas1;
	if (&avgff)
		avgff = avgff1;
	if (true)
		return;
	for (cruisespeeds_t::const_iterator i(m_cruisespeeds.begin()), e(m_cruisespeeds.end()); i != e; ++i)
		std::cerr << "compute_cruisespeeds: DA " << i->first << " TAS " << i->second << std::endl;
}

void FlightPlan::set_aircraft(const Aircraft& acft, double rpm, double mp, double bhp)
{
	set_aircraftid(acft.get_callsign());
	set_aircrafttype(acft.get_icaotype());
	set_wakecategory(acft.get_wakecategory());
	set_equipment(acft.get_equipment());
	set_transponder(acft.get_transponder());
	set_pbn(acft.get_pbn());
	set_emergencyradio(acft.get_emergencyradio());
	set_survival(acft.get_survival());
	set_lifejackets(acft.get_lifejackets());
	set_dinghies(acft.get_dinghies());
	set_picname(acft.get_picname());
	for (Aircraft::otherinfo_const_iterator_t oii(acft.otherinfo_begin()), oie(acft.otherinfo_end()); oii != oie; ++oii)
		otherinfo_add(*oii);
	if (!acft.get_crewcontact().empty())
		otherinfo_add("RMK", "CREW CONTACT " + acft.get_crewcontact());
	set_cruisespeed("");
	{
		double avgff(std::numeric_limits<double>::quiet_NaN());
		double avgtas(std::numeric_limits<double>::quiet_NaN());
		compute_cruisespeeds(avgtas, avgff, acft, Aircraft::Cruise::EngineParams(bhp, rpm, mp));
		if (!std::isnan(avgtas) && !std::isnan(avgff) && avgff > 0) {
			// compute total fuel content
			double fuel(acft.get_useable_fuel());
			if (false)
				std::cerr << "Useable fuel volume: " << fuel << " average cruise fuel flow: " << avgff << std::endl;
			if (!std::isnan(fuel) && fuel > 0) {
				fuel /= avgff;
				m_endurance = fuel * 3600.0;
			}
		}
	}
	set_colourmarkings(acft.get_colormarking());
}

float FlightPlan::total_distance_km(void) const
{
        float sum(0);
	for (size_type i(0), n(size()); i < n; ) {
		const Point& pt0((*this)[i].get_coord());
		if (pt0.is_invalid()) {
			++i;
			continue;
		}
		unsigned int j(i + 1);
		for (; j < n; ++j) {
			const Point& pt1((*this)[j].get_coord());
			if (pt1.is_invalid())
				continue;
			sum += pt0.spheric_distance_km(pt1);
			break;
		}
		i = j;
	}
        return sum;
}

double FlightPlan::total_distance_km_dbl(void) const
{
        double sum(0);
	for (size_type i(0), n(size()); i < n; ) {
		const Point& pt0((*this)[i].get_coord());
		if (pt0.is_invalid()) {
			++i;
			continue;
		}
		unsigned int j(i + 1);
		for (; j < n; ++j) {
			const Point& pt1((*this)[j].get_coord());
			if (pt1.is_invalid())
				continue;
			sum += pt0.spheric_distance_km_dbl(pt1);
			break;
		}
		i = j;
	}
	return sum;
}

float FlightPlan::gc_distance_km(void) const
{
	if (empty())
		return 0;
	return front().get_coord().spheric_distance_km(back().get_coord());
}

double FlightPlan::gc_distance_km_dbl(void) const
{
	if (empty())
		return 0;
	return front().get_coord().spheric_distance_km_dbl(back().get_coord());
}

int32_t FlightPlan::max_altitude(void) const
{
        int32_t a(std::numeric_limits<int32_t>::min());
        for (size_type i = size(); i > 0; )
                a = std::max(a, (*this)[--i].get_altitude());
        return a;
}

int32_t FlightPlan::max_altitude(uint16_t& flags) const
{
        int32_t a(std::numeric_limits<int32_t>::min());
	uint16_t f(0);
        for (size_type i = size(); i > 0; ) {
		const FPlanWaypoint& wpt((*this)[--i]);
		int16_t aw(wpt.get_altitude());
		if (aw < a)
			continue;
                a = aw;
		f = wpt.get_flags();
	}
	flags = f;
        return a;
}

void FlightPlan::recompute_dist(void)
{
	for (iterator i(begin()), e(end()); i != e;) {
		FPlanWaypoint& wpto(*i);
		++i;
		if (i == e) {
			wpto.set_dist(0);
			wpto.set_truetrack(0);
			break;
		}
		const FPlanWaypoint& wpt(*i);
		wpto.set_dist_nmi(wpto.get_coord().spheric_distance_nmi_dbl(wpt.get_coord()));
		wpto.set_truetrack_deg(wpto.get_coord().spheric_true_course_dbl(wpt.get_coord()));
	}
}

void FlightPlan::recompute_decl(void)
{
	WMM wmm;
	time_t curtime(time(0));
	for (iterator i(begin()), e(end()); i != e; ++i) {
		FPlanWaypoint& wpt(*i);
		wmm.compute(wpt.get_truealt() * (FPlanWaypoint::ft_to_m / 1000.0), wpt.get_coord(), curtime);
		wpt.set_declination(wmm.get_dec());
	}
}

void FlightPlan::set_winddir(uint16_t dir)
{
	for (iterator i(begin()), e(end()); i != e; ++i)
		i->set_winddir(dir);
}

void FlightPlan::set_windspeed(uint16_t speed)
{
	for (iterator i(begin()), e(end()); i != e; ++i)
		i->set_windspeed(speed);
}

void FlightPlan::set_qff(uint16_t press)
{
	for (iterator i(begin()), e(end()); i != e; ++i)
		i->set_qff(press);
}

void FlightPlan::set_isaoffset(int16_t temp)
{
	for (iterator i(begin()), e(end()); i != e; ++i)
		i->set_isaoffset(temp);
}

void FlightPlan::set_declination(uint16_t decl)
{
	for (iterator i(begin()), e(end()); i != e; ++i)
		i->set_declination(decl);
}

void FlightPlan::set_rpm(uint16_t rpm)
{
	for (iterator i(begin()), e(end()); i != e; ++i)
		i->set_rpm(rpm);
}

void FlightPlan::set_mp(uint16_t mp)
{
	for (iterator i(begin()), e(end()); i != e; ++i)
		i->set_mp(mp);
}

void FlightPlan::set_tas(uint16_t tas)
{
	for (iterator i(begin()), e(end()); i != e; ++i)
		i->set_tas(tas);
}

void FlightPlan::recompute(const Aircraft& acft, float qnh, float tempoffs, double bhp, double rpm, double mp)
{
	FPlanRoute route(*(FPlan *)0);
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		route.insert_wpt(~0, *i);
	route.recompute(acft, qnh, tempoffs, Aircraft::Cruise::EngineParams(bhp, rpm, mp));
	for (size_type i(0), n(size()); i < n; ++i) {
		const FPlanWaypoint& wpto(route[i]);
		FPLWaypoint& wpt((*this)[i]);
		wpt.frob_flags(wpto.get_flags() & (FPlanWaypoint::altflag_climb | FPlanWaypoint::altflag_descent),
			       (FPlanWaypoint::altflag_climb | FPlanWaypoint::altflag_descent));
		wpt.set_flighttime(wpto.get_flighttime());
		wpt.set_fuel(wpto.get_fuel());
		wpt.set_rpm(wpto.get_rpm());
		wpt.set_mp(wpto.get_mp());
		wpt.set_truealt(wpto.get_truealt());
		wpt.set_tas(wpto.get_tas());
		wpt.set_truetrack(wpto.get_truetrack());
		wpt.set_trueheading(wpto.get_trueheading());
		wpt.set_declination(wpto.get_declination());
		wpt.set_dist(wpto.get_dist());
	}
}

void FlightPlan::turnpoints(bool include_dct, float maxdev)
{
	for (iterator i(begin()), e(end()); i != e; ++i)
		i->frob_flags(FPlanWaypoint::altflag_turnpoint, FPlanWaypoint::altflag_turnpoint);
	for (size_type nr = 1U; nr + 3U < size(); ) {
		size_type nr1(nr);
		for (; nr1 + 3U < size(); ++nr1) {
			const FPLWaypoint& wpt2((*this)[nr1 + 2U]);
			const FPLWaypoint& wpt1((*this)[nr1 + 1U]);
			const FPLWaypoint& wpt0((*this)[nr]);
			if (!(wpt0.get_flags() & wpt1.get_flags() & FPlanWaypoint::altflag_ifr) ||
			    ((wpt0.get_flags() ^ wpt1.get_flags()) & FPlanWaypoint::altflag_standard) ||
			    (wpt0.get_altitude() != wpt1.get_altitude()))
				break;
			if ((wpt0.get_pathcode() != FPlanWaypoint::pathcode_airway ||
			     wpt1.get_pathcode() != FPlanWaypoint::pathcode_airway ||
			     wpt0.get_pathname().empty() || wpt0.get_pathname() 
			     != wpt1.get_pathname()) &&
			    (!include_dct || wpt0.get_pathcode() != FPlanWaypoint::pathcode_directto ||
			     wpt1.get_pathcode() != FPlanWaypoint::pathcode_directto))
				break;
			if (maxdev < std::numeric_limits<float>::max()) {
				bool ok(true);
				for (size_type i = nr; i <= nr1; ) {
					++i;
					const FPLWaypoint& wptx((*this)[i]);
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
		if (nr1 == nr) {
			++nr;
			continue;
		}
		for (; nr <= nr1; ++nr)
			(*this)[nr].frob_flags(0, FPlanWaypoint::altflag_turnpoint);
	}
}

FPlanRoute::GFSResult FlightPlan::gfs(GRIB2& grib2)
{
	FPlanRoute route(*(FPlan *)0);
	for (const_iterator i(begin()), e(end()); i != e; ++i)
		route.insert_wpt(~0, *i);
	FPlanRoute::GFSResult r(route.gfs(grib2));
	for (size_type i(0), n(size()); i < n; ++i) {
		const FPlanWaypoint& wpto(route[i]);
		FPLWaypoint& wpt((*this)[i]);
		wpt.frob_flags(wpto.get_flags() & (FPlanWaypoint::altflag_climb | FPlanWaypoint::altflag_descent),
			       (FPlanWaypoint::altflag_climb | FPlanWaypoint::altflag_descent));
		wpt.set_windspeed(wpto.get_windspeed());
		wpt.set_winddir(wpto.get_winddir());
		wpt.set_isaoffset(wpto.get_isaoffset());
		wpt.set_qff(wpto.get_qff());
	}
	return r;
}

void FlightPlan::fix_invalid_altitudes(unsigned int nr)
{
	if (nr >= size())
		return;
	FPLWaypoint& wpt(operator[](nr));
	if (wpt.is_altitude_valid())
		return;
	if (nr) {
		const FPLWaypoint& wptp(operator[](nr-1));
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

void FlightPlan::fix_invalid_altitudes(unsigned int nr, TopoDb30& topodb, Database& db)
{
	if (nr >= size())
		return;
	FPLWaypoint& wpt(operator[](nr));
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
		const FPLWaypoint& wptp(operator[](nr-1));
		if (wptp.is_ifr()) {
			base = 0;
			terrain = 10000;
		}
		magcrs = wptp.get_magnetictrack();
	}
	if (nr) {
		TopoDb30::Profile profile(topodb.get_profile(operator[](nr-1).get_coord(), wpt.get_coord(), 5.0f));
		TopoDb30::elev_t maxelev(profile.get_maxelev());
		if (maxelev != TopoDb30::nodata)				
			terrain = std::max(terrain, Point::round<int32_t,double>(Point::m_to_ft_dbl * maxelev));
	}
	if (nr + 1 < size()) {
		TopoDb30::Profile profile(topodb.get_profile(wpt.get_coord(), operator[](nr+1).get_coord(), 5.0f));
		TopoDb30::elev_t maxelev(profile.get_maxelev());
		if (maxelev != TopoDb30::nodata)				
			terrain = std::max(terrain, Point::round<int32_t,double>(Point::m_to_ft_dbl * maxelev));
	}
	if (magcrs < 0x8000)
		base += 1000;
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

void FlightPlan::fix_invalid_altitudes(void)
{
	for (size_type i(0), n(size()); i < n; ++i)
		fix_invalid_altitudes(i);
}

void FlightPlan::fix_invalid_altitudes(TopoDb30& topodb, Database& db)
{
	for (size_type i(0), n(size()); i < n; ++i)
		fix_invalid_altitudes(i, topodb, db);
}

void FlightPlan::clamp_msl(void)
{
	for (iterator wi(begin()), we(end()); wi != we; ++wi)
		if (wi->is_altitude_valid() && wi->get_altitude() < 0)
			wi->set_altitude(0);
}

void FlightPlan::compute_terrain(TopoDb30& topodb, double corridor_nmi, bool roundcaps)
{
	for (size_type i(0), n(size()); i < n; ) {
		operator[](i).unset_terrain();
		if (operator[](i).get_coord().is_invalid()) {
			++i;
			continue;
		}
		size_type j(i);
		while (j + 1U < n) {
			++j;
			if (!operator[](j).get_coord().is_invalid())
				break;
		}
		if (i == j || operator[](j).get_coord().is_invalid()) {
			while (i < j) {
				++i;
				operator[](i).unset_terrain();
			}
			++i;
			continue;
		}
		TopoDb30::elev_t e(TopoDb30::nodata);
		if (roundcaps || true) {
			TopoDb30::minmax_elev_t mme(topodb.get_minmax_elev(operator[](i).get_coord().make_fat_line(operator[](j).get_coord(), corridor_nmi, roundcaps)));
			if (mme.first != TopoDb30::nodata && mme.second != TopoDb30::nodata && mme.first <= mme.second)
				e = mme.second;
		} else {
			TopoDb30::Profile prof(topodb.get_profile(operator[](i).get_coord(), operator[](j).get_coord(), corridor_nmi));
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
				operator[](i).unset_terrain();
			}
			continue;
		}
		int32_t t(Point::round<int32_t,double>(e * Point::m_to_ft_dbl));
		while (i < j) {
			operator[](i).set_terrain(t);
			++i;
		}
	}
}

void FlightPlan::add_fir_eet(Database& db, const TimeTableSpecialDateEval& ttsde)
{
	Database::findresults_t r(db.find_all(Database::loadmode_obj, get_departuretime(), get_departuretime(),
					      Object::type_airspace, Object::type_airspace, 0));
	Rect bbox(get_bbox());
	typedef std::map<std::string,time_t> eet_t;
	eet_t eet;
	for (Database::findresults_t::iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
		const AirspaceTimeSlice& ts(ri->get_obj()->operator()(get_departuretime()).as_airspace());
		if (!ts.is_valid() ||
		    (ts.get_type() != AirspaceTimeSlice::type_fir &&
		     ts.get_type() != AirspaceTimeSlice::type_uir))
			continue;
		{
			Rect bbox1;
			ts.get_bbox(bbox1);
			if (!bbox.is_intersect(bbox1)) {
				remove_eet(ts.get_ident());
				continue;
			}
		}
		ri->get_obj()->link(db, ~0U);
		for (size_type i0(0), n(size()); i0 < n; ) {
			const FPLWaypoint& wpt0(operator[](i0));
			if (wpt0.get_coord().is_invalid()) {
				++i0;
				continue;
			}
			size_type i1(i0 + 1);
			for (; i1 < n; ++i1)
				if (!operator[](i1).get_coord().is_invalid())
					break;
			if (i1 >= n)
				break;
			const FPLWaypoint& wpt1(operator[](i1));
			TimeTableEval tte0(get_departuretime() + wpt0.get_flighttime(), wpt0.get_coord(), ttsde);
			TimeTableEval tte1(get_departuretime() + wpt1.get_flighttime(), wpt1.get_coord(), ttsde);
			if (ts.is_inside(tte0, wpt0.get_altitude()) ||
			    !(ts.is_inside(tte1, wpt1.get_altitude()) ||
			      ts.is_intersect(tte0, wpt1.get_coord(), wpt1.get_altitude()))) {
				i0 = i1;
				continue;
			}
			std::pair<eet_t::iterator, bool> ins(eet.insert(eet_t::value_type(ts.get_ident(), wpt1.get_flighttime())));
			if (ins.first->second > wpt1.get_flighttime())
				ins.first->second = wpt1.get_flighttime();
			i0 = i1;
		}
	}
	for (eet_t::const_iterator i(eet.begin()), e(eet.end()); i != e; ++i)
		add_eet(i->first, i->second);
}

void FlightPlan::remove_fir_eet(Database& db)
{
	Database::findresults_t r(db.find_all(Database::loadmode_obj, get_departuretime(), get_departuretime(),
					      Object::type_airspace, Object::type_airspace, 0));
	for (Database::findresults_t::iterator ri(r.begin()), re(r.end()); ri != re; ++ri) {
		const AirspaceTimeSlice& ts(ri->get_obj()->operator()(get_departuretime()).as_airspace());
		if (!ts.is_valid() ||
		    (ts.get_type() != AirspaceTimeSlice::type_fir &&
		     ts.get_type() != AirspaceTimeSlice::type_uir))
			continue;
		remove_eet(ts.get_ident());
	}
}


#if 0

(FPL-OEABC-VG
-DV20/L-S/C
-LOWI0800
-N0090VFR WHISKEY
-LOWI0001
-STS/EET0200 , RMK/THIS IS A REMARK DOF/051102
-E/0500 P/003 R/E J/F
A/RED WHITE
C/HUBER)

(FPL-OEABC-VG
-C150/L-OV/C
-LOGG1220
-N0080VFR LH MK VM
-LOWK0130 LOWG
-DOF/050719)

(FPL-OEABC-IN
-A321/M-SEHRWY/S
-LOWW0915
-N0458F340 OSPEN UM984 BZO/N0458F350 UM984 NEDED/N0456F370 UM984
DIVKO UN852 VERSO UL129 LUNIK
-LEPA0157 LEIB
-EET/LIMM0030 LFFF0106 LECB0135 SEL/APHL OPR/TESTAIR DOF/050719
RVR/75 ORGN/LOWWZPZX)

(FPL-OEABC-IS
-B772/H-SEGHIJRWXY/S
-LOWW0913
-N0480F350 ABLOM UL725 ALAMU UY33 NARKA UB499 REBLA UL601 CND UM747
SOBLO/N0490F350 B143 VEKAT UM747 DUKAN B449 DURDY/N0490F370 B449 RJ
A240 AFGAN W615A BUROT B143D ABDAN N644 DI A466 ASARI/N0490F370
A466E DPN L759S AGG L759 KKJ/N0490F370 L759 IBUDA/N0490F370 L759
LIBDI/N0490F370 L759 NISUN/N0490F370 L759 MIPAK/N0490F370 L759 PUT
R325 VIH A464 VBA
-WMKK1046 WSSS
-EET/UKFV0122 URRV0153 UGGG0212 UBBA0244 UTAK0311 UTAA0332 OAKX0426
OPLR0504 VIDF0540 VABF0639 VECF0700 VYYF0838 VOMF0842 VYYF0903
VTBB0925 WMFC1003 SEL/AEBQ OPR/ABC AIR PER/C DAT/SV RALT/VECC
VTSP RMK/AGCS DOF/050719 ORGN/LOWWZPZX)

(FPL-OEABC-IS
-CRJ2/M-SERWY/S
-LOWS0550
-N0450F340 TRAUN L856 MANAL UL856 DERAK UM982 TINIL
-LFPG0120 LFPO
-DOF/050719 RVR/200 ORGN/RPL)

#endif
