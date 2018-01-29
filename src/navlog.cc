/*****************************************************************************/

/*
 *      navlog.cc  --  Navigation Log Creation.
 *
 *      Copyright (C) 2012, 2013, 2014, 2015, 2016, 2017
 *          Thomas Sailer (t.sailer@alumni.ethz.ch)
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*****************************************************************************/

#include "sitename.h"
#include "aircraft.h"
#include "fplan.h"
#include "airdata.h"
#include "engine.h"
#include "dbobj.h"
#include "baro.h"
#include "wmm.h"
#include "wind.h"
#include "icaofpl.h"
#include "metgraph.h"

#include <cmath>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <ext/stdio_filebuf.h>

#ifdef __WIN32__
#include <windows.h>
#endif

class Aircraft::NavFPlan {
public:
	NavFPlan(const Aircraft& acft, Engine& engine, const Cruise::EngineParams& epcruise);
	~NavFPlan();
	void begin(const WeightBalance::elementvalues_t& wbev = WeightBalance::elementvalues_t());
	void fpl(const FPlanRoute& fpl);
	void profile(const FPlanRoute& fpl);
	void end(const Glib::ustring& templatefile);

protected:
	class Profile {
	  public:
		Profile(Engine& engine);
		std::ostream& profile(std::ostream& os, const FPlanRoute& fpl, double corridor_width = 5);

	  protected:
		TopoDb30::Profile m_profile;
		Glib::RefPtr<Engine::ElevationProfileResult> m_topoquery;
		Glib::Cond m_cond;
		Glib::Mutex m_mutex;
		Engine& m_engine;

		void async_cancel(void);
		void async_clear(void);
		void async_connectdone(void);
		void async_done(void);
		void async_waitdone(void);
	};


	const Aircraft& m_aircraft;
	Engine& m_engine;
	__gnu_cxx::stdio_filebuf<char> *m_tempfile;
	std::string m_tempfilename;
	Cruise::EngineParams m_cruise;
};

Aircraft::NavFPlan::Profile::Profile(Engine& engine)
	: m_engine(engine)
{
}

std::ostream& Aircraft::NavFPlan::Profile::profile(std::ostream& os, const FPlanRoute& fpl, double corridor_width)
{
	double dist(0);
	unsigned int nrwpt(fpl.get_nrwpt());
	for (unsigned int i = 1; i < nrwpt;) {
		const FPlanWaypoint& wpt0(fpl[i - 1]);
		const FPlanWaypoint& wpt1(fpl[i]);
		++i;
		bool lastleg(i >= nrwpt);
		async_cancel();
		async_clear();
		m_topoquery = m_engine.async_elevation_profile(wpt0.get_coord(), wpt1.get_coord(), corridor_width);
		async_connectdone();
		async_waitdone();
		unsigned int profsize(m_profile.size());
		for (unsigned int j = 0; j < profsize;) {
			const TopoDb30::ProfilePoint& pp(m_profile[j]);
			TopoDb30::elev_t el(pp.get_elev()), maxel(pp.get_maxelev());
			if (el == TopoDb30::nodata) {
				++j;
				continue;
			}
			if (el == TopoDb30::ocean)
				el = 0;
			if (maxel == TopoDb30::nodata)
				maxel = el;
			else if (maxel == TopoDb30::ocean)
				maxel = 0;
			os << "VPROF|";
			++j;
			if (j == 1)
				os << wpt0.get_icao_name() << '|' << wpt0.get_true_altitude();
			else if (lastleg && j >= profsize)
				os << wpt1.get_icao_name() << '|' << wpt1.get_true_altitude();
			else
				os << '|';
			os << '|' << (dist + pp.get_dist()) << '|' << el << '|' << maxel << std::endl;
		}
		dist += wpt0.get_coord().spheric_distance_nmi_dbl(wpt1.get_coord());
	}
	return os;
}

void Aircraft::NavFPlan::Profile::async_cancel(void)
{
        Engine::ElevationProfileResult::cancel(m_topoquery);
}

void Aircraft::NavFPlan::Profile::async_clear(void)
{
	m_topoquery = Glib::RefPtr<Engine::ElevationProfileResult>();
	m_profile.clear();
}

void Aircraft::NavFPlan::Profile::async_connectdone(void)
{
	if (m_topoquery)
		m_topoquery->connect(sigc::mem_fun(*this, &Profile::async_done));
}

void Aircraft::NavFPlan::Profile::async_done(void)
{
        Glib::Mutex::Lock lock(m_mutex);
        m_cond.broadcast();
}

void Aircraft::NavFPlan::Profile::async_waitdone(void)
{
        Glib::Mutex::Lock lock(m_mutex);
        for (;;) {
		if (m_topoquery && m_topoquery->is_done()) {
			m_profile.clear();
			if (!m_topoquery->is_error())
				m_profile = m_topoquery->get_result();
			m_topoquery = Glib::RefPtr<Engine::ElevationProfileResult>();
		}
		if (!m_topoquery)
			break;
		m_cond.wait(m_mutex);
	}
}

Aircraft::NavFPlan::NavFPlan(const Aircraft& acft, Engine& engine, const Cruise::EngineParams& epcruise)
	: m_aircraft(acft), m_engine(engine), m_tempfile(0), m_cruise(epcruise)
{
}

Aircraft::NavFPlan::~NavFPlan()
{
	if (m_tempfile) {
		m_tempfile->close();
		delete m_tempfile;
	}
}

void Aircraft::NavFPlan::begin(const WeightBalance::elementvalues_t& wbev)
{
	if (m_tempfile) {
		m_tempfile->close();
		delete m_tempfile;
		m_tempfile = 0;
	}
	int fd(Glib::file_open_tmp(m_tempfilename, "vfrnav_navlog_"));
	m_tempfile = new __gnu_cxx::stdio_filebuf<char>(fd, std::ios::out);
	std::ostream os(m_tempfile);
	os << "ACFT|" << m_aircraft.get_callsign()
	   << '|' << m_aircraft.get_manufacturer()
	   << '|' << m_aircraft.get_model()
	   << '|' << m_aircraft.get_year()
	   << '|' << m_aircraft.get_description()
	   << '|' << m_aircraft.get_icaotype()
	   << '|' << m_aircraft.get_equipment_string()
	   << '|' << m_aircraft.get_transponder()
	   << '|' << m_aircraft.get_pbn()
	   << '|' << m_aircraft.get_colormarking()
	   << '|' << m_aircraft.get_wakecategory() << std::endl
	   << "ACFTD|" << m_aircraft.get_mtom_kg()
	   << '|' << m_aircraft.get_vr0()
	   << '|' << m_aircraft.get_vr1()
	   << '|' << m_aircraft.get_vx0()
	   << '|' << m_aircraft.get_vx1()
	   << '|' << m_aircraft.get_vy()
	   << '|' << m_aircraft.get_vs0()
	   << '|' << m_aircraft.get_vs1()
	   << '|' << m_aircraft.get_vno()
	   << '|' << m_aircraft.get_vne()
	   << '|' << m_aircraft.get_vref0()
	   << '|' << m_aircraft.get_vref1()
	   << '|' << m_aircraft.get_va()
	   << '|' << m_aircraft.get_vfe()
	   << '|' << m_aircraft.get_vgearext()
	   << '|' << m_aircraft.get_vgearret()
	   << '|' << m_aircraft.get_fuelmass()
	   << '|' << m_aircraft.get_maxbhp()
	   << '|' << m_aircraft.get_contingencyfuel() << std::endl;
	{
		const Aircraft::WeightBalance& wb(m_aircraft.get_wb());
		for (unsigned int i = 0; i < wb.get_nrelements(); ++i) {
			const Aircraft::WeightBalance::Element& el(wb.get_element(i));
			double mass(0);
			if (el.get_flags() & WeightBalance::Element::flag_fixed) {
				if (i < wbev.size() && wbev[i].get_unit() < el.get_nrunits()) {
					const WeightBalance::Element::Unit& u(el.get_unit(wbev[i].get_unit()));
					mass = std::min(std::max(wbev[i].get_value() * u.get_factor() + u.get_offset(), el.get_massmin()), el.get_massmax());
				} else {
					mass = el.get_massmin();
				}
			} else if (i < wbev.size() && !(el.get_flags() & WeightBalance::Element::flag_binary) && wbev[i].get_unit() < el.get_nrunits()) {
				const WeightBalance::Element::Unit& u(el.get_unit(wbev[i].get_unit()));
				mass = wbev[i].get_value() * u.get_factor() + u.get_offset();
			}
			os << "WBEL|" << el.get_name();
			if (i >= wbev.size())
				os << "|0|0";
			else
				os << '|' << wbev[i].get_value() << '|' << wbev[i].get_unit();
			os << '|' << el.get_massmin()
			   << '|' << el.get_massmax()
			   << '|' << el.get_arm(mass) << '|';
			if (el.get_flags() & Aircraft::WeightBalance::Element::flag_fixed)
				os << 'F';
			if (el.get_flags() & Aircraft::WeightBalance::Element::flag_binary)
				os << 'B';
			if (el.get_flags() & Aircraft::WeightBalance::Element::flag_avgas)
				os << 'A';
			if (el.get_flags() & Aircraft::WeightBalance::Element::flag_jeta)
				os << 'J';
			if (el.get_flags() & Aircraft::WeightBalance::Element::flag_oil)
				os << 'O';
			if (el.get_flags() & Aircraft::WeightBalance::Element::flag_gear)
				os << 'G';
			if (el.get_flags() & Aircraft::WeightBalance::Element::flag_consumable)
				os << 'C';
			for (unsigned int j = 0; j < el.get_nrunits(); ++j) {
				const Aircraft::WeightBalance::Element::Unit& u(el.get_unit(j));
				os << '|' << u.get_name()
				   << '|' << u.get_factor()
				   << '|' << u.get_offset();
			}
			os << std::endl;
		}
		for (unsigned int i = 0; i < wb.get_nrenvelopes(); ++i) {
			const Aircraft::WeightBalance::Envelope& env(wb.get_envelope(i));
			os << "WBENV|" << env.get_name();
			for (unsigned int j = 0; j < env.size(); ++j) {
				const Aircraft::WeightBalance::Envelope::Point& pt(env[j]);
				os << '|' << pt.get_mass() << '|' << pt.get_arm();
			}
			os << std::endl;
		}
	}
	{
		const Aircraft::Distances& dists(m_aircraft.get_dist());
		for (unsigned int i = 0; i < dists.get_nrtakeoffdist(); ++i) {
			const Aircraft::Distances::Distance& dist(dists.get_takeoffdist(i));
			os << "TKOFFDIST|" << dist.get_name()
			   << '|' << dist.get_vrotate()
			   << '|' << dist.get_mass() << '|';
			{
				Aircraft::Poly1D<double> p(dist.get_gndpoly().convert(dists.get_distunit(), Aircraft::unit_m));
				bool subseq(false);
				for (Aircraft::Poly1D<double>::const_iterator pi(p.begin()), pe(p.end()); pi != pe; ++pi) {
					if (subseq)
						os << ',';
					subseq = true;
					os << *pi;
				}
			}
			os << '|';
			{
				Aircraft::Poly1D<double> p(dist.get_obstpoly().convert(dists.get_distunit(), Aircraft::unit_m));
				bool subseq(false);
				for (Aircraft::Poly1D<double>::const_iterator pi(p.begin()), pe(p.end()); pi != pe; ++pi) {
					if (subseq)
						os << ',';
					subseq = true;
					os << *pi;
				}
			}
			os << std::endl;
		}
		for (unsigned int i = 0; i < dists.get_nrldgdist(); ++i) {
			const Aircraft::Distances::Distance& dist(dists.get_ldgdist(i));
			os << "LDGDIST|" << dist.get_name()
			   << '|' << dist.get_vrotate()
			   << '|' << dist.get_mass() << '|';
			{
				Aircraft::Poly1D<double> p(dist.get_gndpoly().convert(dists.get_distunit(), Aircraft::unit_m));
				bool subseq(false);
				for (Aircraft::Poly1D<double>::const_iterator pi(p.begin()), pe(p.end()); pi != pe; ++pi) {
					if (subseq)
						os << ',';
					subseq = true;
					os << *pi;
				}
			}
			os << '|';
			{
				Aircraft::Poly1D<double> p(dist.get_obstpoly().convert(dists.get_distunit(), Aircraft::unit_m));
				bool subseq(false);
				for (Aircraft::Poly1D<double>::const_iterator pi(p.begin()), pe(p.end()); pi != pe; ++pi) {
					if (subseq)
						os << ',';
					subseq = true;
					os << *pi;
				}
			}
			os << std::endl;
		}
	}
}

void Aircraft::NavFPlan::fpl(const FPlanRoute& fpl)
{
	if (!m_tempfile || fpl.get_nrwpt() < 1)
		return;
	std::ostream os(m_tempfile);
	{
		IcaoFlightPlan icaofpl(m_engine);
		icaofpl.populate(fpl, IcaoFlightPlan::awymode_keep);
		icaofpl.set_aircraft(m_aircraft, m_cruise);
		os << "FPLH|" << fpl.get_time_offblock_unix()
		   << '|' << fpl.get_time_onblock_unix()
		   << '|' << icaofpl.get_fpl() << std::endl;
	}
	for (unsigned int i = 0; i < 2; ++i) {
		const FPlanWaypoint& wpt(fpl[(fpl.get_nrwpt() - 1) & -i]);
		{
			Cruise::EngineParams ep(m_cruise);
			double tas(0), fuelflow(0), pa(wpt.get_pressure_altitude() + 1500.0);
			double mass(Aircraft::convert(Aircraft::unit_kg, m_aircraft.get_wb().get_massunit(), wpt.get_mass_kg())), isaoffs(wpt.get_isaoffset_kelvin()), qnh(wpt.get_qff_hpa());
			m_aircraft.calculate_cruise(tas, fuelflow, pa, mass, isaoffs, qnh, ep);
			os << (i ? "FPLDESTHLD|" : "FPLDEPHLD|") << wpt.get_density_altitude()
			   << '|' << fuelflow << '|' << tas << '|' << ep.get_bhp() << std::endl;
		}
		IcaoFlightPlan::FindCoord fc(m_engine);
		if (!fc.find(wpt.get_icao(), wpt.get_name(), IcaoFlightPlan::FindCoord::flag_subtables | IcaoFlightPlan::FindCoord::flag_airport, wpt.get_coord()))
			continue;
		AirportsDb::Airport arpt;
		if (!fc.get_airport(arpt))
			continue;
		if (!arpt.is_valid())
			continue;
		for (unsigned int j = 0; j < arpt.get_nrrwy(); ++j) {
			const AirportsDb::Airport::Runway& rwy(arpt.get_rwy(j));
			os << (i ? "FPLDESTRWY" : "FPLDEPRWY")
			   << '|' << rwy.get_ident_he()
			   << '|' << rwy.get_ident_le()
			   << '|' << rwy.get_length()
			   << '|' << rwy.get_width()
			   << '|' << rwy.get_surface()
			   << '|' << std::fixed << std::setprecision(9) << rwy.get_he_coord().get_lat_deg_dbl()
			   << '|' << std::fixed << std::setprecision(9) << rwy.get_he_coord().get_lon_deg_dbl()
			   << '|' << std::fixed << std::setprecision(9) << rwy.get_le_coord().get_lat_deg_dbl()
			   << '|' << std::fixed << std::setprecision(9) << rwy.get_le_coord().get_lon_deg_dbl()
			   << '|' << rwy.get_he_tda()
			   << '|' << rwy.get_he_lda()
			   << '|' << rwy.get_he_disp()
			   << '|' << rwy.get_he_hdg()
			   << '|' << rwy.get_he_elev()
			   << '|' << rwy.get_le_tda()
			   << '|' << rwy.get_le_lda()
			   << '|' << rwy.get_le_disp()
			   << '|' << rwy.get_le_hdg()
			   << '|' << rwy.get_le_elev()
			   << '|' << (int)rwy.is_he_usable()
			   << '|' << (int)rwy.is_le_usable();
			for (unsigned int k = 0; k < 8; ++k)
				os << '|' << AirportsDb::Airport::Runway::get_light_string(rwy.get_he_light(k));
			for (unsigned int k = 0; k < 8; ++k)
				os << '|' << AirportsDb::Airport::Runway::get_light_string(rwy.get_le_light(k));
			os << std::endl;
		}
		for (unsigned int j = 0; j < arpt.get_nrhelipads(); ++j) {
			const AirportsDb::Airport::Helipad& hp(arpt.get_helipad(j));
			os << (i ? "FPLDESTHP" : "FPLDEPHP")
			   << '|' << hp.get_ident()
			   << '|' << hp.get_length()
			   << '|' << hp.get_width()
			   << '|' << hp.get_surface()
			   << '|' << std::fixed << std::setprecision(9) << hp.get_coord().get_lat_deg_dbl()
			   << '|' << std::fixed << std::setprecision(9) << hp.get_coord().get_lon_deg_dbl()
			   << '|' << hp.get_hdg()
			   << '|' << hp.get_elev()
			   << '|' << (int)hp.is_usable() << std::endl;
		}
		for (unsigned int j = 0; j < arpt.get_nrcomm(); ++j) {
			const AirportsDb::Airport::Comm& comm(arpt.get_comm(j));
			std::string sector(comm.get_sector());
			std::string oprhours(comm.get_oprhours());
			for (std::string::iterator si(sector.begin()), se(sector.end()); si != se; ++si)
				if (*si == '\n' || *si == '\r')
					*si = ' ';
			for (std::string::iterator si(oprhours.begin()), se(oprhours.end()); si != se; ++si)
				if (*si == '\n' || *si == '\r')
					*si = ' ';
			os << (i ? "FPLDESTCOMM" : "FPLDEPCOMM")
			   << '|' << comm.get_name()
			   << '|' << sector
			   << '|' << oprhours;
			for (unsigned int k = 0; k < 5; ++k)
				os << '|' << comm[k];
			os << std::endl;
		}
	}
        for (unsigned int i = 0; i < fpl.get_nrwpt(); ++i) {
		const FPlanWaypoint& wpt(fpl[i]);
		os << "FPLL|" << wpt.get_icao() << '|' << wpt.get_name() << '|' << wpt.get_pathname()
		   << '|' << wpt.get_pathcode_name() << '|';
		if (!i)
			os << (wpt.get_time_unix() - fpl.get_time_offblock_unix());
		else
			os << (wpt.get_flighttime() - fpl[i - 1].get_flighttime());
		os << '|' << wpt.get_frequency()
		   << '|' << std::fixed << std::setprecision(9) << wpt.get_coord().get_lat_deg_dbl()
		   << '|' << std::fixed << std::setprecision(9) << wpt.get_coord().get_lon_deg_dbl()
		   << '|' << wpt.get_altitude()
		   << '|' << ((wpt.get_flags() & FPlanWaypoint::altflag_standard) ? "STD" : "QNH")
		   << '|' << ((wpt.get_flags() & FPlanWaypoint::altflag_ifr) ? "IFR" : "VFR")
		   << '|' << std::fixed << std::setprecision(9) << wpt.get_truealt()
		   << '|' << std::fixed << std::setprecision(9) << wpt.get_truetrack_deg()
		   << '|' << std::fixed << std::setprecision(9) << wpt.get_trueheading_deg()
		   << '|' << std::fixed << std::setprecision(9) << wpt.get_declination_deg()
		   << '|' << std::fixed << std::setprecision(9) << wpt.get_magnetictrack_deg()
		   << '|' << std::fixed << std::setprecision(9) << wpt.get_magneticheading_deg()
		   << '|' << std::fixed << std::setprecision(9) << wpt.get_dist_nmi()
		   << '|';
		if (i)
			os << std::fixed << std::setprecision(9) << (wpt.get_fuel_usg() - fpl[i - 1].get_fuel_usg());
		else
			os << '0';
		double gs(wpt.get_tas_kts());
		if (wpt.get_windspeed() > 0) {
			Wind<double> wind;
			wind.set_wind(wpt.get_winddir_deg(), wpt.get_windspeed_kts());
			wind.set_crs_tas(wpt.get_truetrack_deg(), wpt.get_tas_kts());
			if (std::isnan(wind.get_gs()) || std::isnan(wind.get_hdg()) || wind.get_gs() <= 0)
				gs = 1;
			else
				gs = wind.get_gs();
		}
		os << '|' << std::fixed << std::setprecision(9) << wpt.get_tas_kts()
		   << '|' << std::fixed << std::setprecision(9) << gs
		   << '|' << std::fixed << std::setprecision(9) << wpt.get_rpm()
		   << '|' << std::fixed << std::setprecision(9) << wpt.get_mp_inhg()
		   << '|' << std::fixed << std::setprecision(9) << wpt.get_winddir_deg()
		   << '|' << std::fixed << std::setprecision(9) << wpt.get_windspeed_kts()
		   << '|' << std::fixed << std::setprecision(9) << wpt.get_qff_hpa()
		   << '|' << std::fixed << std::setprecision(9) << wpt.get_isaoffset_kelvin()
		   << '|' << std::fixed << std::setprecision(9) << wpt.get_oat_kelvin()
		   << '|' << std::fixed << std::setprecision(9) << wpt.get_pressure_altitude()
		   << '|' << std::fixed << std::setprecision(9) << wpt.get_density_altitude()
		   << std::endl;
	}
}

void Aircraft::NavFPlan::profile(const FPlanRoute& fpl)
{
	if (!m_tempfile || fpl.get_nrwpt() < 2)
		return;
	std::ostream os(m_tempfile);
	Profile p(m_engine);
	p.profile(os, fpl);
}

void Aircraft::NavFPlan::end(const Glib::ustring& templatefile)
{
	if (m_tempfile) {
		m_tempfile->close();
		delete m_tempfile;
		m_tempfile = 0;
	}
	std::vector<std::string> argv;
	std::vector<std::string> env(Glib::listenv());
	for (std::vector<std::string>::iterator i(env.begin()), e(env.end()); i != e; ++i)
		*i += "=" + Glib::getenv(*i);
	{
		bool needfile(true);
		for (std::vector<std::string>::iterator i(env.begin()), e(env.end()); i != e; ++i) {
			if (*i == "VFRNAV_FPLFILE") {
				*i += "=" + m_tempfilename;
				needfile = false;
				continue;
			}
			*i += "=" + Glib::getenv(*i);
		}
		if (needfile)
			env.push_back("VFRNAV_FPLFILE=" + m_tempfilename);
	}
	if (false) {
		std::cerr << "Environment:" << std::endl;
		for (std::vector<std::string>::const_iterator i(env.begin()), e(env.end()); i != e; ++i)
			std::cerr << *i << std::endl;
	}
#ifdef __WIN32__
	{
		std::string version;
		std::string path;
		static const char * const regkeys[] = {
			"Software\\LibreOffice\\LibreOffice",
			"Software\\OpenOffice.org\\OpenOffice.org",
			0
		};
		for (const char * const *regkey = regkeys; *regkey; ++regkey) {
			HKEY hKey;
			if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, *regkey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
				if (false)
					std::cerr << "Enumerating keys in HKLM\\" << *regkey << std::endl;
				for (DWORD dwIndex = 0;; ++dwIndex) {
					TCHAR lpName[1024];
					DWORD lpcbName(sizeof(lpName));
					FILETIME fTime;
					if (RegEnumKeyEx(hKey, dwIndex, lpName, &lpcbName, NULL, NULL, NULL, &fTime) != ERROR_SUCCESS)
						break;
					std::string ver1(lpName);
					if (false)
						std::cerr << "Subkey " << dwIndex << ": " << ver1 << std::endl;
					if (!path.empty() && ver1 < version)
						continue;
					//TCHAR lpData[1024];
					BYTE lpData[1024];
					DWORD cbData(sizeof(lpData));
					//LONG err(RegGetValue(hKey, lpName, "Path", RRF_RT_REG_SZ, 0, &lpData, &cbData));
					HKEY hKey2;
					LONG err(RegOpenKeyEx(hKey, lpName, 0, KEY_READ, &hKey2));
					if (err != ERROR_SUCCESS) {
						if (true)
							std::cerr << "Cannot open key HKLM\\" << *regkey << "\\" << lpName
								  << " Error " << g_win32_error_message(err) << " (" << err << ')' << std::endl;
						continue;
					}
					DWORD lpType;
					err = RegQueryValueEx(hKey2, "Path", 0, &lpType, lpData, &cbData);
					if (cbData < sizeof(lpData))
						cbData[lpData] = 0;
					RegCloseKey(hKey2);
					if (err != ERROR_SUCCESS) {
						if (true)
							std::cerr << "Cannot get key HKLM\\" << *regkey << "\\" << lpName
								  << " value: Error " << g_win32_error_message(err) << " (" << err << ')' << std::endl;
						continue;
					}
					version = ver1;
					path = (char *)lpData;

				}
				RegCloseKey(hKey);
			}
		}
		if (!path.empty()) {
			std::cerr << "Found libreoffice V" << version << ": " << path << std::endl;
			argv.push_back(path);
		} else {
			std::cerr << "Libreoffice not found; trying soffice.exe" << std::endl;
			argv.push_back("soffice.exe");
		}
	}
#else
	argv.push_back("soffice");
#endif
	argv.push_back("--calc");
	argv.push_back("--norestore");
	argv.push_back("--nologo");
	argv.push_back("-n");
	argv.push_back(templatefile);
	argv.push_back("macro://./Standard.FPLImport.Main()");
	Glib::spawn_async_with_pipes(Glib::get_home_dir(), argv, env, Glib::SPAWN_SEARCH_PATH);
}

void Aircraft::navfplan(const Glib::ustring& templatefile, Engine& engine, const FPlanRoute& fplan,
			const Cruise::EngineParams& epcruise, const WeightBalance::elementvalues_t& wbev) const
{
	NavFPlan n(*this, engine, epcruise);
	n.begin(wbev);
	n.fpl(fplan);
	n.profile(fplan);
	n.end(templatefile);
}

class Aircraft::GnmCellSpec {
public:
	GnmCellSpec(const std::string& name = "", const std::string& value = "A1");
	const std::string& get_name(void) const { return m_name; }
	unsigned int get_row(void) const { return m_row; }
	unsigned int get_col(void) const { return m_col; }
	std::string get_rowstr(unsigned int wptnr = 0, unsigned int wptrows = 1, unsigned int pgbrkheader = 0,
			       unsigned int pgbrkinterval = 0, unsigned int pgbrkoffset = 0) const;
	std::string get_colstr(void) const;
	std::string get_colrow(bool absolute = false) const;
	static bool is_fplanrow(const std::string& name);
	bool is_fplanrow(void) const { return is_fplanrow(get_name()); }
	int compare(const GnmCellSpec& x) const { return get_name().compare(x.get_name()); }
	bool operator==(const GnmCellSpec& x) const { return compare(x) == 0; }
	bool operator!=(const GnmCellSpec& x) const { return compare(x) != 0; }
	bool operator<(const GnmCellSpec& x) const { return compare(x) < 0; }
	bool operator<=(const GnmCellSpec& x) const { return compare(x) <= 0; }
	bool operator>(const GnmCellSpec& x) const { return compare(x) > 0; }
	bool operator>=(const GnmCellSpec& x) const { return compare(x) >= 0; }

protected:
	std::string m_name;
	unsigned int m_row;
	unsigned int m_col;
};

Aircraft::GnmCellSpec::GnmCellSpec(const std::string& name, const std::string& value)
	: m_name(name), m_row(0), m_col(0)
{
	std::string::size_type n(value.rfind('!'));
	if (n == std::string::npos)
		n = 0;
	else
		++n;
	unsigned int rowdigits(0), coldigits(0);
	for (; n < value.size(); ++n) {
		char ch(value[n]);
		if (ch == '$')
			continue;
		if (ch >= '0' && ch <= '9') {
			m_row *= 10;
			m_row += ch - '0';
			++rowdigits;
			continue;
		}
		if (ch >= 'A' && ch <= 'Z') {
			m_col *= 26;
			m_col += ch - 'A';
			++coldigits;
			continue;
		}
		std::ostringstream oss;
		oss << "invalid row/col character: " << ch;
		throw std::runtime_error(oss.str());
	}
	if (!rowdigits)
		throw std::runtime_error(value + " has no row spec");
	if (!coldigits)
		throw std::runtime_error(value + " has no col spec");
	if (!m_row)
		throw std::runtime_error("row should start at one");
	--m_row;
	{
		unsigned int x(1), a(0);
		for (unsigned int i(0); i < coldigits; ++i) {
			a += x;
			x *= 26;
		}
		m_col += a - 1;
	}
}

std::string Aircraft::GnmCellSpec::get_rowstr(unsigned int wptnr, unsigned int wptrows, unsigned int pgbrkheader,
					      unsigned int pgbrkinterval, unsigned int pgbrkoffset) const
{
	std::ostringstream oss;
	unsigned int hdr(0);
	if (pgbrkinterval)
		hdr = (wptnr + pgbrkoffset) / pgbrkinterval;
	oss << (get_row() + wptnr * wptrows + hdr * pgbrkheader);
	return oss.str();
}

std::string Aircraft::GnmCellSpec::get_colstr(void) const
{
	std::ostringstream oss;
	oss << get_col();
	return oss.str();
}

std::string Aircraft::GnmCellSpec::get_colrow(bool absolute) const
{
	std::ostringstream oss;
	if (absolute)
		oss << '$';
	{
		unsigned int n(get_col() + 1);
		std::string c;
		for (;;) {
			if (n <= 26) {
				c = (char)('A' + n - 1) + c;
				break;
			}
			unsigned int nn(n);
			n /= 26;
			nn -= n * 26;
			c = (char)('A' + nn) + c;
		}
		oss << c;
	}
	if (absolute)
		oss << '$';
	oss << (get_row() + 1);
	return oss.str();
}

bool Aircraft::GnmCellSpec::is_fplanrow(const std::string& name)
{
	return name == "WPTICAO" || name == "WPTNAME" || name == "WPTICAONAME" || name == "WPTIDENT" ||
		name == "WPTPATHNAME" || name == "WPTPATHCODE" || name == "WPTNOTE" || name == "WPTTIME" || name == "WPTFLTTIME" ||
		name == "WPTFREQ" || name == "WPTLAT" || name == "WPTLON" || name == "WPTALT" || name == "WPTALTSTD" ||
		name == "WPTFRULES" || name == "WPTCLIMB" || name == "WPTDESCENT" || name == "WPTTURNPOINT" ||
		name == "WPTWINDDIR" || name == "WPTWINDSPEED" || name == "WPTQFF" || name == "WPTISAOFFS" ||
		name == "WPTSLT" || name == "WPTOAT" || name == "WPTPA" || name == "WPTTA" || name == "WPTDA" || name == "WPTTRUEALT" ||
		name == "WPTTT" || name == "WPTTH" || name == "WPTMT" || name == "WPTMH" || name == "WPTDECL" ||
		name == "WPTDIST" || name == "WPTFUEL" || name == "WPTTAS" || name == "WPTCRUISETAS" || name == "WPTAVGTAS" ||
		name == "WPTGS" || name == "WPTCRUISEGS" || name == "WPTAVGGS" ||
		name == "WPTRPM" || name == "WPTMP" || name == "WPTTYPE";
}

class Aircraft::GnmCellSpecs : public std::set<GnmCellSpec> {
public:
	typedef std::set<GnmCellSpec> base_t;
	using base_t::find;
	GnmCellSpecs(void);
	GnmCellSpecs(const xmlpp::Node *sheet);
	const_iterator find(const std::string& name) const { return base_t::find(GnmCellSpec(name)); }
	const_iterator find(const std::string& pfx, const std::string& name) const { return find(pfx + name); }
	const_iterator find(const char *name) const { if (!name) return end(); return find(std::string(name)); }
	const_iterator find(const std::string& pfx, const char *name) const { if (!name) return end(); return find(pfx, std::string(name)); }
	void parse(const xmlpp::Node *sheet);
	bool computefplrows(void);
	unsigned int get_fplfirstrow(void) const { return m_fplfirstrow; }
	unsigned int get_fplrows(void) const { return m_fplrows; }
	unsigned int get_pgbrkheader(void) const { return m_pgbrkheader; }
	unsigned int get_pgbrkinterval(void) const { return m_pgbrkinterval; }
	unsigned int get_pgbrkoffset(void) const { return m_pgbrkoffset; }
	unsigned int get_pagelen(void) const { return m_pagelen; }

	std::ostream& print(std::ostream& os) const;
	unsigned int get_rowadjlinelen(void) const { return m_rowadjlinelen; }
	int get_rowadjlineoffs(void) const { return m_rowadjlineoffs; }
	double get_rowadjlineheight(void) const { return m_rowadjlineheight; }
	double get_rowadjlineheightoffs(void) const { return m_rowadjlineheightoffs; }
	bool is_rowadj(void) const {
		return !std::isnan(get_rowadjlineheight()) && !std::isnan(get_rowadjlineheightoffs()) &&
			get_rowadjlinelen() > 0;
	}

protected:
	unsigned int m_fplfirstrow;
	unsigned int m_fplrows;
	unsigned int m_pgbrkheader;
	unsigned int m_pgbrkinterval;
	unsigned int m_pgbrkoffset;
	unsigned int m_pagelen;
	unsigned int m_rowadjlinelen;
	int m_rowadjlineoffs;
	double m_rowadjlineheight;
	double m_rowadjlineheightoffs;
};

Aircraft::GnmCellSpecs::GnmCellSpecs(void)
	: m_fplfirstrow(0), m_fplrows(0), m_pgbrkheader(0), m_pgbrkinterval(0), m_pgbrkoffset(0),
	  m_pagelen(100), m_rowadjlinelen(0), m_rowadjlineoffs(0),
	  m_rowadjlineheight(std::numeric_limits<double>::quiet_NaN()),
	  m_rowadjlineheightoffs(std::numeric_limits<double>::quiet_NaN())
{
}

Aircraft::GnmCellSpecs::GnmCellSpecs(const xmlpp::Node *sheet)
	: m_fplfirstrow(0), m_fplrows(0), m_pgbrkheader(0), m_pgbrkinterval(0), m_pgbrkoffset(0),
	  m_pagelen(100), m_rowadjlinelen(0), m_rowadjlineoffs(0),
	  m_rowadjlineheight(std::numeric_limits<double>::quiet_NaN()),
	  m_rowadjlineheightoffs(std::numeric_limits<double>::quiet_NaN())
{
	parse(sheet);
	computefplrows();
}

void Aircraft::GnmCellSpecs::parse(const xmlpp::Node *sheet)
{
	if (!sheet)
		return;
	xmlpp::Node::NodeList nlnames(sheet->get_children("Names"));
	for (xmlpp::Node::NodeList::const_iterator ni(nlnames.begin()), ne(nlnames.end()); ni != ne; ++ni) {
		xmlpp::Node::NodeList nlname((*ni)->get_children("Name"));
		for (xmlpp::Node::NodeList::const_iterator ni(nlname.begin()), ne(nlname.end()); ni != ne; ++ni) {
			std::string name, value;
			xmlpp::Node::NodeList nll((*ni)->get_children("name"));
			for (xmlpp::Node::NodeList::const_iterator ni(nll.begin()), ne(nll.end()); ni != ne; ++ni) {
				const xmlpp::TextNode* txt(static_cast<xmlpp::Element *>(*ni)->get_child_text());
				if (!txt)
					continue;
				name = txt->get_content();
			}
			nll = (*ni)->get_children("value");
			for (xmlpp::Node::NodeList::const_iterator ni(nll.begin()), ne(nll.end()); ni != ne; ++ni) {
				const xmlpp::TextNode* txt(static_cast<const xmlpp::Element *>(*ni)->get_child_text());
				if (!txt)
					continue;
				value = txt->get_content();
			}
			if (name == "RowAdjust_LINEHEIGHT") {
				m_rowadjlineheight = strtod(value.c_str(), 0);
				continue;
			}
			if (name == "RowAdjust_LINEHEIGHTOFFSET") {
				m_rowadjlineheightoffs = strtod(value.c_str(), 0);
				continue;
			}
			if (name == "RowAdjust_LINELEN") {
				m_rowadjlinelen = strtoul(value.c_str(), 0, 0);
				continue;
			}
			if (name == "RowAdjust_LINEOFFS") {
				m_rowadjlineoffs = strtol(value.c_str(), 0, 0);
				continue;
			}
			if (name == "PageBreak_HEADER") {
				m_pgbrkheader = strtoul(value.c_str(), 0, 0);
				continue;
			}
			if (name == "PageBreak_INTERVAL") {
				m_pgbrkinterval = strtoul(value.c_str(), 0, 0);
				continue;
			}
			if (name == "PageBreak_OFFSET") {
				m_pgbrkoffset = strtoul(value.c_str(), 0, 0);
				continue;
			}
			if (name == "Page_LENGTH") {
				m_pagelen = strtoul(value.c_str(), 0, 0);
				continue;
			}
			if (name.compare(0, 5, "Cell_") || value.empty())
				continue;
			try {
				std::pair<iterator,bool> ins(insert(GnmCellSpec(name.substr(5), value)));
				if (!ins.second)
					std::cerr << "Gnumeric Name Error: " << name << '=' << value << ": duplicate name" << std::endl;
			} catch (const std::runtime_error& e) {
				std::cerr << "Gnumeric Name Error: " << name << '=' << value << ": " << e.what() << std::endl;
			}
		}
	}
}

bool Aircraft::GnmCellSpecs::computefplrows(void)
{
	m_fplfirstrow = m_fplrows = 0;
	unsigned int firstrow(std::numeric_limits<unsigned int>::max());
	unsigned int lastrow(std::numeric_limits<unsigned int>::min());
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		if (!i->is_fplanrow())
			continue;
		firstrow = std::min(firstrow, i->get_row());
		lastrow = std::max(lastrow, i->get_row());
	}
	if (firstrow > lastrow)
		return false;
	m_fplfirstrow = firstrow;
	m_fplrows = lastrow - firstrow + 1;
	return true;
}

std::ostream& Aircraft::GnmCellSpecs::print(std::ostream& os) const
{
	for (const_iterator i(begin()), e(end()); i != e; ++i) {
		os << "Cell: " << i->get_name() << " R" << i->get_row() << " C" << i->get_col()
		   << ' ' << i->get_colrow();
		if (i->is_fplanrow())
			os << " WPT";
		os << std::endl;
	}
	return os;
}

Aircraft::WeightBalance::elementvalues_t Aircraft::prepare_wb(const WeightBalance::elementvalues_t& wbev, double fuel_on_board,
							      unit_t fuelunit, unit_t massunit, unit_t oilunit) const
{
	const WeightBalance& wb(get_wb());
	WeightBalance::elementvalues_t wbv(wbev);
	wbv.resize(wb.get_nrelements());
	// set units
	for (WeightBalance::elementvalues_t::size_type i(0), n(wbv.size()); i < n; ++i) {
		const WeightBalance::Element& el(wb.get_element(i));
		if (el.get_flags() & WeightBalance::Element::flag_gear) {
			if (i < wbev.size() && wbv[i].get_unit() < el.get_nrunits())
				continue;
			for (unsigned int u(0), n(el.get_nrunits()); u < n; ++u) {
				const WeightBalance::Element::Unit& elu(el.get_unit(u));
				if (elu.get_name() != "Retracted")
					continue;
				wbv[i].set_unit(u);
				wbv[i].set_value(0);
				break;
			}
			if (wbv[i].get_unit() < el.get_nrunits())
				continue;
			wbv[i].set_unit(0);
			wbv[i].set_value(0);
			continue;
		}
		unit_t unit(massunit);
		if (el.get_flags() & WeightBalance::Element::flag_fuel)
			unit = fuelunit;
		else if (el.get_flags() & WeightBalance::Element::flag_oil)
			unit = oilunit;
		if (unit == unit_invalid) {
			if (wbv[i].get_unit() < el.get_nrunits())
				continue;
			wbv[i].set_unit(0);
			continue;
		}
		double mass(0);
		if (wbv[i].get_unit() < el.get_nrunits()) {
			const WeightBalance::Element::Unit& elu(el.get_unit(wbv[i].get_unit()));
			mass = wbv[i].get_value() * elu.get_factor() + elu.get_offset();
		}
		double desiredfactorinv(1.0 / convert(unit, wb.get_massunit(), 1.0, el.get_flags()));
		if (std::isnan(desiredfactorinv))
			continue;
		double best(std::numeric_limits<double>::max());
		for (unsigned int u(0), n(el.get_nrunits()); u < n; ++u) {
			const WeightBalance::Element::Unit& elu(el.get_unit(u));
			if (elu.get_offset() != 0.0)
				continue;
			double x(desiredfactorinv * elu.get_factor());
			if (x < 1)
				x = 1.0 / x;
			if (std::isnan(x) || x < 1 || x > best)
				continue;
			best = x;
			wbv[i].set_unit(u);
		}
		if (wbv[i].get_unit() >= el.get_nrunits()) {
			wbv[i].set_unit(0);
			if (!el.get_nrunits())
				continue;
		}
		const WeightBalance::Element::Unit& elu(el.get_unit(wbv[i].get_unit()));
		double val((mass - elu.get_offset()) / elu.get_factor());
		if (std::isnan(val))
			val = 0;
		wbv[i].set_value(val);
	}
	// apply fuel
	if (std::isnan(fuel_on_board) || fuel_on_board < 0)
		return wbv;
	if (fuelunit == unit_invalid)
		fuelunit = get_fuelunit();
	if (massunit == unit_invalid)
		massunit = get_wb().get_massunit();
	if (oilunit == unit_invalid)
		oilunit = unit_quart;
	for (WeightBalance::elementvalues_t::size_type i(0), n(wbv.size()); i < n; ++i) {
		const WeightBalance::Element& el(wb.get_element(i));
		if (!(el.get_flags() & WeightBalance::Element::flag_fuel))
			continue;
		WeightBalance::ElementValue& wbve(wbv[i]);
		if (wbve.get_unit() >= el.get_nrunits()) {
			wbve.set_unit(0);
			if (!el.get_nrunits()) {
				wbve.set_value(0);
				continue;
			}
		}
		const WeightBalance::Element::Unit& elu(el.get_unit(wbve.get_unit()));
		if (std::isnan(elu.get_factor()) || elu.get_factor() <= 0 || std::isnan(elu.get_offset())) {
			wbve.set_value(0);
			continue;
		}
		double mass(convert(fuelunit, get_wb().get_massunit(), fuel_on_board, el.get_flags()));
		if (std::isnan(mass) || mass < 0)
			mass = 0;
		else if (mass > el.get_massdiff())
			mass = el.get_massdiff();
		fuel_on_board -= convert(get_wb().get_massunit(), fuelunit, mass, el.get_flags());
		wbve.set_value((mass + el.get_massmin() - elu.get_offset()) / elu.get_factor());
	}
	return wbv;
}

void Aircraft::navfplan_gnumeric(const std::string& outfile, Engine& engine, const FPlanRoute& fplan,
				 const std::vector<FPlanAlternate>& altn,
				 const Cruise::EngineParams& epcruise, double fuel_on_board, /* in "native" units */
				 const WeightBalance::elementvalues_t& wbev, const std::string& templatefile,
				 gint64 gfsminreftime, gint64 gfsmaxreftime,
				 gint64 gfsminefftime, gint64 gfsmaxefftime,
				 bool hidewbpage, unit_t fuelunit, unit_t massunit,
				 const std::string& atcfpl, const std::string& atcfplc,
				 const std::string& atcfplcdct, const std::string& atcfplcall) const
{
	if (fuelunit == unit_invalid)
		fuelunit = get_fuelunit();
	if (massunit == unit_invalid)
		massunit = get_wb().get_massunit();
	xmlpp::DomParser p(templatefile);
	if (!p) {
		std::ostringstream oss;
		oss << "cannot open template file " << templatefile;
		throw std::runtime_error(oss.str());
	}
	xmlpp::Document *doc(p.get_document());
	xmlpp::Element *root(doc->get_root_node());
	if (root->get_name() != "Workbook") {
		std::ostringstream oss;
		oss << "invalid template file " << templatefile << " (root node " << root->get_name() << ')';
		throw std::runtime_error(oss.str());
	}
	// fuel scaling value
	// wbmassunit * fuelscale_mass -> fuelunit
	// acftfuelunit * fuelscale -> fuelunit
	double fuelscale(convert_fuel(get_fuelunit(), fuelunit));
	double fuelscale_mass(convert_fuel(get_wb().get_massunit(), fuelunit));
	WeightBalance::elementvalues_t wbv(prepare_wb(wbev, fuel_on_board, fuelunit, massunit, unit_invalid));
	xmlpp::Node::NodeList nlsheets(root->get_children("Sheets"));
	for (xmlpp::Node::NodeList::const_iterator ni(nlsheets.begin()), ne(nlsheets.end()); ni != ne; ++ni) {
		xmlpp::Node::NodeList nlsheet((*ni)->get_children("Sheet"));
		unsigned int sheetnr(0);
		for (xmlpp::Node::NodeList::const_iterator ni(nlsheet.begin()), ne(nlsheet.end()); ni != ne; ++ni, ++sheetnr) {
			double armmin(std::numeric_limits<double>::max());
			double armmax(std::numeric_limits<double>::min());
			double massmin(std::numeric_limits<double>::max());
			double massmax(std::numeric_limits<double>::min());
			unsigned int atcfpllen(0);
			unsigned int atcfplrow(~0U);
			GnmCellSpecs cellspecs(*ni);
			if (!false)
				cellspecs.print(std::cerr);
			// hide page 1 if the model does not have w&b info and takeoff/landing distances
			if (sheetnr == 1 && (hidewbpage || (!get_wb().get_nrelements() && !get_wb().get_nrenvelopes() &&
							    !get_dist().get_nrtakeoffdist() && !get_dist().get_nrldgdist()))) {
				xmlpp::Attribute *attr(static_cast<xmlpp::Element *>(*ni)->get_attribute("Visibility"));
				if (attr)
					attr->set_value("GNM_SHEET_VISIBILITY_HIDDEN");
			}
			xmlpp::Node::NodeList nlcells((*ni)->get_children("Cells"));
			for (xmlpp::Node::NodeList::const_iterator ni(nlcells.begin()), ne(nlcells.end()); ni != ne; ++ni) {
				switch (sheetnr) {
				case 0:
				{
					GnmCellSpecs::const_iterator csi;
					{
						IcaoFlightPlan::awymode_t awym(IcaoFlightPlan::awymode_keep);
						bool ok(false);
						std::string fpl;
						if ((ok = (csi = cellspecs.find("ATCFPL")) != cellspecs.end())) {
							awym = IcaoFlightPlan::awymode_keep;
							fpl = atcfpl;
						} else if ((ok = (csi = cellspecs.find("ATCFPLC")) != cellspecs.end())) {
							awym = IcaoFlightPlan::awymode_collapse;
							fpl = atcfplc;
						} else if ((ok = (csi = cellspecs.find("ATCFPLCDCT")) != cellspecs.end())) {
							awym = IcaoFlightPlan::awymode_collapse_dct;
							fpl = atcfplcdct;
						} else if ((ok = (csi = cellspecs.find("ATCFPLCALL")) != cellspecs.end())) {
							awym = IcaoFlightPlan::awymode_collapse_all;
							fpl = atcfplcall;
						}
						if (ok) {
							if (fpl.empty()) {
								IcaoFlightPlan icaofpl(engine);
								icaofpl.populate(fplan, awym);
								icaofpl.set_aircraft(*this, epcruise);
								if (altn.size() >= 1)
									icaofpl.set_alternate1(altn[0].get_icao());
								if (altn.size() >= 2)
									icaofpl.set_alternate2(altn[1].get_icao());
								 fpl = icaofpl.get_fpl();
							}
							atcfpllen = fpl.size();
							atcfplrow = csi->get_row();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr());
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "60");
							el->set_child_text(fpl);
						}
					}
					if ((csi = cellspecs.find("FPLID")) != cellspecs.end()) {
						std::ostringstream oss;
						oss << fplan.get_id();
						xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
						el->set_attribute("Row", csi->get_rowstr());
						el->set_attribute("Col", csi->get_colstr());
						el->set_attribute("ValueType", "40");
						el->set_child_text(oss.str());
					}
					if ((csi = cellspecs.find("FPLNOTE")) != cellspecs.end()) {
						xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
						el->set_attribute("Row", csi->get_rowstr());
						el->set_attribute("Col", csi->get_colstr());
						el->set_attribute("ValueType", "60");
						el->set_child_text(fplan.get_note());
					}
					if ((csi = cellspecs.find("NRWPT")) != cellspecs.end()) {
						std::ostringstream oss;
						oss << fplan.get_nrwpt();
						xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
						el->set_attribute("Row", csi->get_rowstr());
						el->set_attribute("Col", csi->get_colstr());
						el->set_attribute("ValueType", "40");
						el->set_child_text(oss.str());
					}
					if ((csi = cellspecs.find("OFFBLOCK")) != cellspecs.end()) {
						std::ostringstream oss;
						oss << fplan.get_time_offblock_unix();
						xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
						el->set_attribute("Row", csi->get_rowstr());
						el->set_attribute("Col", csi->get_colstr());
						el->set_attribute("ValueType", "40");
						el->set_child_text(oss.str());
					}
					if ((csi = cellspecs.find("ONBLOCK")) != cellspecs.end()) {
						std::ostringstream oss;
						oss << fplan.get_time_onblock_unix();
						xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
						el->set_attribute("Row", csi->get_rowstr());
						el->set_attribute("Col", csi->get_colstr());
						el->set_attribute("ValueType", "40");
						el->set_child_text(oss.str());
					}
					if ((csi = cellspecs.find("TOTDIST")) != cellspecs.end()) {
						std::ostringstream oss;
						oss << fplan.total_distance_nmi_dbl();
						xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
						el->set_attribute("Row", csi->get_rowstr());
						el->set_attribute("Col", csi->get_colstr());
						el->set_attribute("ValueType", "40");
						el->set_child_text(oss.str());
					}
					if ((csi = cellspecs.find("GCDIST")) != cellspecs.end()) {
						std::ostringstream oss;
						oss << fplan.gc_distance_nmi_dbl();
						xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
						el->set_attribute("Row", csi->get_rowstr());
						el->set_attribute("Col", csi->get_colstr());
						el->set_attribute("ValueType", "40");
						el->set_child_text(oss.str());
					}
					if ((csi = cellspecs.find("MAXALT")) != cellspecs.end()) {
						std::ostringstream oss;
						oss << fplan.max_altitude();
						xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
						el->set_attribute("Row", csi->get_rowstr());
						el->set_attribute("Col", csi->get_colstr());
						el->set_attribute("ValueType", "40");
						el->set_child_text(oss.str());
					}
					if ((csi = cellspecs.find("FRULES")) != cellspecs.end()) {
						std::ostringstream oss;
						oss << fplan.get_flightrules();
						xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
						el->set_attribute("Row", csi->get_rowstr());
						el->set_attribute("Col", csi->get_colstr());
						el->set_attribute("ValueType", "60");
						el->set_child_text(oss.str());
					}
					if ((csi = cellspecs.find("GFSMINREFTIME")) != cellspecs.end()) {
						std::ostringstream oss;
						oss << gfsminreftime;
						xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
						el->set_attribute("Row", csi->get_rowstr());
						el->set_attribute("Col", csi->get_colstr());
						el->set_attribute("ValueType", "40");
						el->set_child_text(oss.str());
					}
					if ((csi = cellspecs.find("GFSMAXREFTIME")) != cellspecs.end()) {
						std::ostringstream oss;
						oss << gfsmaxreftime;
						xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
						el->set_attribute("Row", csi->get_rowstr());
						el->set_attribute("Col", csi->get_colstr());
						el->set_attribute("ValueType", "40");
						el->set_child_text(oss.str());
					}
					if ((csi = cellspecs.find("GFSMINEFFTIME")) != cellspecs.end()) {
						std::ostringstream oss;
						oss << gfsminefftime;
						xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
						el->set_attribute("Row", csi->get_rowstr());
						el->set_attribute("Col", csi->get_colstr());
						el->set_attribute("ValueType", "40");
						el->set_child_text(oss.str());
					}
					if ((csi = cellspecs.find("GFSMAXEFFTIME")) != cellspecs.end()) {
						std::ostringstream oss;
						oss << gfsmaxefftime;
						xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
						el->set_attribute("Row", csi->get_rowstr());
						el->set_attribute("Col", csi->get_colstr());
						el->set_attribute("ValueType", "40");
						el->set_child_text(oss.str());
					}
					if ((csi = cellspecs.find("FUELNAME")) != cellspecs.end()) {
						xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
						el->set_attribute("Row", csi->get_rowstr());
						el->set_attribute("Col", csi->get_colstr());
						el->set_attribute("ValueType", "60");
						el->set_child_text(get_fuelname());
					}
					if ((csi = cellspecs.find("FUELUNITNAME")) != cellspecs.end()) {
						xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
						el->set_attribute("Row", csi->get_rowstr());
						el->set_attribute("Col", csi->get_colstr());
						el->set_attribute("ValueType", "60");
						el->set_child_text(to_str(fuelunit, true));
					}
					double cumdist(0);
					for (unsigned int i(0), n(fplan.get_nrwpt()); i < n + 2; ++i) {
						unsigned int wptidx(i);
						if (wptidx >= n)
							wptidx = 0;
						if (i == n + 1)
							wptidx = n - 1;
						const FPlanWaypoint& wpt(fplan[wptidx]);
						if (i >= n)
							wptidx = 0;
						std::string pfx("WPT");
						if (i == n)
							pfx = "DEP";
						else if (i == n + 1)
							pfx = "DEST";
						if ((csi = cellspecs.find(pfx, "ICAO")) != cellspecs.end()) {
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "60");
							el->set_child_text(wpt.get_icao());
						}
						if ((csi = cellspecs.find(pfx, "NAME")) != cellspecs.end()) {
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "60");
							el->set_child_text(wpt.get_name());
						}
						if ((csi = cellspecs.find(pfx, "ICAONAME")) != cellspecs.end()) {
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "60");
							el->set_child_text(wpt.get_icao_name());
						}
						if ((csi = cellspecs.find(pfx, "IDENT")) != cellspecs.end()) {
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "60");
							el->set_child_text(wpt.get_ident());
						}
						if ((csi = cellspecs.find(pfx, "PATHNAME")) != cellspecs.end()) {
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "60");
							el->set_child_text(wpt.get_pathname());
						}
						if ((csi = cellspecs.find(pfx, "PATHCODE")) != cellspecs.end()) {
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "60");
							el->set_child_text(wpt.get_pathcode_name());
						}
						if ((csi = cellspecs.find(pfx, "NOTE")) != cellspecs.end()) {
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "60");
							el->set_child_text(wpt.get_note());
						}
						if ((csi = cellspecs.find(pfx, "TIME")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << wpt.get_time_unix();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "FLTTIME")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << wpt.get_flighttime();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "FREQ")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << wpt.get_frequency();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "LAT")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << wpt.get_coord().get_lat_deg_dbl();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "LON")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << wpt.get_coord().get_lon_deg_dbl();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "ALT")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << wpt.get_altitude();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "ALTSTD")) != cellspecs.end()) {
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "60");
							el->set_child_text(wpt.is_standard() ? "STD" : "QNH");
						}
						if ((csi = cellspecs.find(pfx, "FRULES")) != cellspecs.end()) {
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "60");
							el->set_child_text(wpt.is_ifr() ? "IFR" : "VFR");
						}
						if ((csi = cellspecs.find(pfx, "CLIMB")) != cellspecs.end()) {
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(wpt.is_climb() ? "1" : "0");
						}
						if ((csi = cellspecs.find(pfx, "DESCENT")) != cellspecs.end()) {
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(wpt.is_descent() ? "1" : "0");
						}
						if ((csi = cellspecs.find(pfx, "TURNPOINT")) != cellspecs.end()) {
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(wpt.is_turnpoint() ? "1" : "0");
						}
						if ((csi = cellspecs.find(pfx, "WINDDIR")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << wpt.get_winddir_deg();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "WINDSPEED")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << wpt.get_windspeed_kts();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "QFF")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << wpt.get_qff_hpa();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "ISAOFFS")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << wpt.get_isaoffset_kelvin();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "SLT")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << wpt.get_sltemp_kelvin();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "OAT")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << wpt.get_oat_kelvin();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "PA")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << wpt.get_pressure_altitude();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "TA")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << wpt.get_true_altitude();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "DA")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << wpt.get_density_altitude();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "TRUEALT")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << wpt.get_truealt();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "TT")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << wpt.get_truetrack_deg();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "TH")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << wpt.get_trueheading_deg();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "MT")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << wpt.get_magnetictrack_deg();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "MH")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << wpt.get_magneticheading_deg();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "DECL")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << wpt.get_declination_deg();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "DIST")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << wpt.get_dist_nmi();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if (i < n && (csi = cellspecs.find(pfx, "CUMDIST")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << cumdist;
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "FUEL")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << (fuelscale * wpt.get_fuel_usg());
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "TAS")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << wpt.get_tas_kts();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "CRUISETAS")) != cellspecs.end()) {
							std::ostringstream oss;
							bool val(!wpt.is_climb() && !wpt.is_descent());
							if (val)
								oss << wpt.get_tas_kts();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", val ? "40" : "60");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "AVGTAS")) != cellspecs.end()) {
							std::ostringstream oss;
							bool val(!wpt.is_climb() && !wpt.is_descent());
							if (val) {
								oss << wpt.get_tas_kts();
							} else if (i + 1 < n && wpt.get_dist() > 0) {
								const FPlanWaypoint& wptn(fplan[i + 1]);
								if (wptn.get_flighttime() > wpt.get_flighttime()) {
									Wind<double> wind;
									wind.set_wind(wpt.get_winddir_deg(), wpt.get_windspeed_kts(), wptn.get_winddir_deg(), wptn.get_windspeed_kts());
									wind.set_crs_gs(wpt.get_truetrack_deg(),
											wpt.get_dist_nmi() * 3600 / (wptn.get_flighttime() - wpt.get_flighttime()));
									if (false)
										std::cerr << "AVGTAS: Wind " << wind.get_winddir() << '/' << wind.get_windspeed()
											  << " TAS " << wind.get_tas() << " GS " << wind.get_gs()
											  << " HDG " << wind.get_hdg() << " CRS " << wind.get_crs() << std::endl;
									if (!std::isnan(wind.get_tas())) {
										oss << wind.get_tas();
										val = true;
									}
								}
							}
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", val ? "40" : "60");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "GS")) != cellspecs.end()) {
							std::ostringstream oss;
							double gs(wpt.get_tas_kts());
							if (wpt.get_windspeed() > 0) {
								Wind<double> wind;
								wind.set_wind(wpt.get_winddir_deg(), wpt.get_windspeed_kts());
								wind.set_crs_tas(wpt.get_truetrack_deg(), wpt.get_tas_kts());
								if (std::isnan(wind.get_gs()) || std::isnan(wind.get_hdg()) || wind.get_gs() <= 0)
									gs = 1;
								else
									gs = wind.get_gs();
							}
							oss << gs;
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "CRUISEGS")) != cellspecs.end()) {
							std::ostringstream oss;
							bool val(!wpt.is_climb() && !wpt.is_descent());
							if (val) {
								double gs(wpt.get_tas_kts());
								if (wpt.get_windspeed() > 0) {
									Wind<double> wind;
									wind.set_wind(wpt.get_winddir_deg(), wpt.get_windspeed_kts());
									wind.set_crs_tas(wpt.get_truetrack_deg(), wpt.get_tas_kts());
									if (std::isnan(wind.get_gs()) || std::isnan(wind.get_hdg()) || wind.get_gs() <= 0)
										gs = 1;
									else
										gs = wind.get_gs();
								}
								oss << gs;
							}
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", val ? "40" : "60");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "AVGGS")) != cellspecs.end()) {
							std::ostringstream oss;
							bool val(!wpt.is_climb() && !wpt.is_descent());
							if (val) {
								double gs(wpt.get_tas_kts());
								if (wpt.get_windspeed() > 0) {
									Wind<double> wind;
									wind.set_wind(wpt.get_winddir_deg(), wpt.get_windspeed_kts());
									wind.set_crs_tas(wpt.get_truetrack_deg(), wpt.get_tas_kts());
									if (std::isnan(wind.get_gs()) || std::isnan(wind.get_hdg()) || wind.get_gs() <= 0)
										val = false;
									else
										oss << wind.get_gs();
								} else {
									oss << gs;
								}
							} else if (i + 1 < n && wpt.get_dist() > 0) {
								const FPlanWaypoint& wptn(fplan[i + 1]);
								if (wptn.get_flighttime() > wpt.get_flighttime()) {
									double gs(wpt.get_dist_nmi() * 3600 / (wptn.get_flighttime() - wpt.get_flighttime()));
									oss << gs;
									val = true;
								}
							}
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", val ? "40" : "60");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "RPM")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << wpt.get_rpm();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "MP")) != cellspecs.end()) {
							std::ostringstream oss;
							oss << wpt.get_mp_inhg();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if ((csi = cellspecs.find(pfx, "TYPE")) != cellspecs.end()) {
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr(wptidx, cellspecs.get_fplrows(),
												 cellspecs.get_pgbrkheader(),
												 cellspecs.get_pgbrkinterval(),
												 cellspecs.get_pgbrkoffset()));
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "60");
							el->set_child_text(wpt.get_type_string());
						}
						cumdist += wpt.get_dist_nmi();
					}
					break;
				}

				case 1:
				{
					GnmCellSpecs::const_iterator csi;
					if ((csi = cellspecs.find("CONTFUEL")) != cellspecs.end()) {
						double contfuel(get_contingencyfuel());
						if (std::isnan(contfuel))
							contfuel = 5;
						std::ostringstream oss;
						oss << (contfuel * 1e-2);
						xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
						el->set_attribute("Row", csi->get_rowstr());
						el->set_attribute("Col", csi->get_colstr());
						el->set_attribute("ValueType", "40");
						el->set_child_text(oss.str());
					}
					if ((csi = cellspecs.find("FUELMASS")) != cellspecs.end()) {
						std::ostringstream oss;
						oss << fuelscale_mass;
						xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
						el->set_attribute("Row", csi->get_rowstr());
						el->set_attribute("Col", csi->get_colstr());
						el->set_attribute("ValueType", "40");
						el->set_child_text(oss.str());
					}
					// taxi fuel
					if (!std::isnan(get_taxifuel()) && (csi = cellspecs.find("TAXIFUEL")) != cellspecs.end()) {
						double tfuel(get_taxifuel());
						if (fplan.get_nrwpt()) {
							double tf(fplan[0].get_time_unix() - fplan.get_time_offblock_unix());
							tf *= get_taxifuelflow() * (1.0 / 3600.0);
							if (!std::isnan(tf))
								tfuel += tf;
						}
						std::ostringstream oss;
						oss << tfuel;
						xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
						el->set_attribute("Row", csi->get_rowstr());
						el->set_attribute("Col", csi->get_colstr());
						el->set_attribute("ValueType", "40");
						el->set_child_text(oss.str());
					}
					// alternates
					{
						if ((csi = cellspecs.find("ALTFUELFLOW")) != cellspecs.end()) {
							double fuelflow(0);
							if (fplan.get_nrwpt() > 0) {
								uint32_t fuel(0);
								for (std::vector<FPlanAlternate>::const_iterator i(altn.begin()), e(altn.end()); i != e; ++i) {
									if (i->get_totalfuel() < fuel)
										continue;
									const FPlanWaypoint& wpte(fplan[fplan.get_nrwpt() - 1]);
									time_t tt(i->get_flighttime() - wpte.get_flighttime());
									double ff(i->get_fuel_usg() - wpte.get_fuel_usg());
									if (ff <= 0 || tt <= 0)
										continue;
									fuel = i->get_totalfuel();
									fuelflow = ff / tt * 3600.0;
								}
							}
							std::ostringstream oss;
							oss << fuelflow * fuelscale;
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", csi->get_rowstr());
							el->set_attribute("Col", csi->get_colstr());
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						for (std::vector<FPlanAlternate>::size_type i(0), n(std::min(altn.size(), (std::vector<FPlanAlternate>::size_type)2)); i < n; ++i) {
							std::ostringstream col;
							col << (6 + i);
							const FPlanAlternate& alt(altn[i]);
							{
								xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
								el->set_attribute("Row", "2");
								el->set_attribute("Col", col.str().c_str());
								el->set_attribute("ValueType", "60");
								el->set_child_text(alt.get_icao());
							}
							{
								std::ostringstream oss;
								oss << alt.get_dist_nmi();
								xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
								el->set_attribute("Row", "3");
								el->set_attribute("Col", col.str().c_str());
								el->set_attribute("ValueType", "40");
								el->set_child_text(oss.str());
							}
							if (fplan.get_nrwpt() > 0) {
								const FPlanWaypoint& wpte(fplan[fplan.get_nrwpt() - 1]);
								std::ostringstream oss;
								oss << (alt.get_flighttime() - wpte.get_flighttime()) * (1.0 / 24 / 60 / 60);
								xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
								el->set_attribute("Row", "4");
								el->set_attribute("Col", col.str().c_str());
								el->set_attribute("ValueType", "40");
								el->set_child_text(oss.str());
							}
							{
						        	double fuelflow(0);
								if (alt.get_holdtime() > 0)
									fuelflow = alt.get_holdfuel_usg() / alt.get_holdtime() * 3600.0;
								std::ostringstream oss;
								oss << fuelflow * fuelscale;
								xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
								el->set_attribute("Row", "6");
								el->set_attribute("Col", col.str().c_str());
								el->set_attribute("ValueType", "40");
								el->set_child_text(oss.str());
							}
						}
					}
					std::string loadedfuelformula;
					bool fuelarmset(false);
					double kggain(std::numeric_limits<double>::quiet_NaN()), kgoffs(std::numeric_limits<double>::quiet_NaN());
					const WeightBalance& wb(get_wb());
					for (unsigned int i = 0; i < wb.get_nrelements(); ++i) {
						const WeightBalance::Element& wel(wb.get_element(i));
						double mass(0);
						if (wel.get_flags() & WeightBalance::Element::flag_fixed) {
							if (i < wbv.size() && wbv[i].get_unit() < wel.get_nrunits()) {
								const WeightBalance::Element::Unit& u(wel.get_unit(wbv[i].get_unit()));
								mass = std::min(std::max(wbv[i].get_value() * u.get_factor() + u.get_offset(), wel.get_massmin()), wel.get_massmax());
							} else {
								mass = wel.get_massmin();
							}
						} else {
							if (i < wbv.size()) {
								if (wel.get_flags() & WeightBalance::Element::flag_binary)
									wbv[i].set_value(0);
								if (wbv[i].get_unit() < wel.get_nrunits()) {
									const WeightBalance::Element::Unit& u(wel.get_unit(wbv[i].get_unit()));
									mass = wbv[i].get_value() * u.get_factor() + u.get_offset();
								}
							}
						}
						std::ostringstream row;
						row << (15 + i);
						{
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", row.str());
							el->set_attribute("Col", "0");
							el->set_attribute("ValueType", "60");
							el->set_child_text(wel.get_name());
						}
						{
							std::ostringstream oss;
							bool number(true);
							if (wel.get_flags() & WeightBalance::Element::flag_fixed)
								oss << wel.get_massmin();
							else if (wel.get_flags() & WeightBalance::Element::flag_binary)
								number = false;
							else if (i >= wbv.size())
								number = false;
							else
								oss << wbv[i].get_value();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", row.str());
							el->set_attribute("Col", "1");
							el->set_attribute("ValueType", number ? "40" : "60");
							el->set_child_text(oss.str());
						}
						{
							std::string unit;
							if (i < wbv.size() && wbv[i].get_unit() < wel.get_nrunits())
								unit = wel.get_unit(wbv[i].get_unit()).get_name();
							if (unit.empty() && wel.get_flags() & WeightBalance::Element::flag_gear) {
								for (unsigned int j(0); j < wel.get_nrunits(); ++j) {
									if (wel.get_unit(j).get_name() == "Retracted")
										unit = wel.get_unit(j).get_name();
								}
							}
							if (unit.empty() && wel.get_nrunits())
								unit = wel.get_unit(0).get_name();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", row.str());
							el->set_attribute("Col", "2");
							el->set_attribute("ValueType", "60");
							el->set_child_text(unit);
						}
						{
							std::ostringstream oss;
							oss << wel.get_arm(mass);
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", row.str());
							el->set_attribute("Col", "3");
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						{
							std::ostringstream oss;
							oss << wel.get_massmin();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", row.str());
							el->set_attribute("Col", "5");
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						{
							std::ostringstream oss;
							oss << wel.get_massmax();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", row.str());
							el->set_attribute("Col", "6");
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if (wel.get_flags() & WeightBalance::Element::flag_fuel) {
							{
								std::ostringstream oss;
								oss << "=F" << (16 + i);
								xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
								el->set_attribute("Row", row.str());
								el->set_attribute("Col", "23");
								el->set_child_text(oss.str());
							}
							{
								std::ostringstream oss;
								oss << "+E" << (16 + i) << "*Y4";
								loadedfuelformula += oss.str();
							}
							if (!fuelarmset) {
								std::ostringstream oss;
								oss << "=D" << (16 + i);
								xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
								el->set_attribute("Row", "3");
								el->set_attribute("Col", "23");
								el->set_child_text(oss.str());
								fuelarmset = true;
							}
						} else {
							{
								std::ostringstream oss;
								oss << "=E" << (16 + i);
								xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
								el->set_attribute("Row", row.str());
								el->set_attribute("Col", "23");
								el->set_child_text(oss.str());
							}
						}
						std::string gainexpr("1");
						std::string offsexpr("0");
						for (unsigned int j = 0; j < wel.get_nrunits(); ++j) {
							const WeightBalance::Element::Unit& u(wel.get_unit(j));
							{
								std::ostringstream oss;
								oss << (52 + j);
						        	xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
								el->set_attribute("Row", row.str());
								el->set_attribute("Col", oss.str());
								el->set_attribute("ValueType", "60");
								el->set_child_text(u.get_name());
							}
							{
								std::ostringstream oss;
								oss << "IF(C" << (16 + i) << "=\"" << u.get_name() << "\","
								    << u.get_factor() << ',' << gainexpr << ')';
								gainexpr = oss.str();
							}
							{
								std::ostringstream oss;
								oss << "IF(C" << (16 + i) << "=\"" << u.get_name() << "\","
								    << u.get_offset() << ',' << offsexpr << ')';
								offsexpr = oss.str();
							}
							if (u.get_name() == "kg" && std::isnan(kggain)) {
								kggain = u.get_factor();
								kgoffs = u.get_offset();
							}
						}
						if (wel.get_nrunits() == 1) {
							std::ostringstream oss0, oss1;
							oss0 << wel.get_unit(0).get_factor();
							gainexpr = oss0.str();
							oss0 << wel.get_unit(0).get_offset();
							offsexpr = oss1.str();
						}
						if (!gainexpr.empty() && gainexpr[0] == 'I') {
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", row.str());
							el->set_attribute("Col", "25");
							el->set_child_text("=" + gainexpr);
						} else {
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", row.str());
							el->set_attribute("Col", "25");
							el->set_attribute("ValueType", "40");
							el->set_child_text(gainexpr);
						}
						if (!offsexpr.empty() && offsexpr[0] == 'I') {
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", row.str());
							el->set_attribute("Col", "26");
							el->set_child_text("=" + offsexpr);
						} else {
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", row.str());
							el->set_attribute("Col", "26");
							el->set_attribute("ValueType", "40");
							el->set_child_text(offsexpr);
						}
					}
					if (!loadedfuelformula.empty()) {
						xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
						el->set_attribute("Row", "10");
						el->set_attribute("Col", "3");
						el->set_child_text("=" + loadedfuelformula.substr(1));
					}
					{
						double mrm(get_mrm());
						double mtom(get_mtom());
						double mlm(get_mlm());
						if (std::isnan(mtom))
							mtom = mlm;
						if (std::isnan(mrm))
							mrm = mtom;
						if (std::isnan(mtom))
							mtom = mrm;
						if (std::isnan(mlm))
							mlm = mtom;
						if (!std::isnan(mrm)) {
							std::ostringstream oss;
							oss << mrm;
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", "43");
							el->set_attribute("Col", "23");
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if (!std::isnan(mtom)) {
							std::ostringstream oss;
							oss << mtom;
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", "42");
							el->set_attribute("Col", "23");
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
						if (!std::isnan(mlm)) {
							std::ostringstream oss;
							oss << mlm;
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", "41");
							el->set_attribute("Col", "23");
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
					}
					for (unsigned int i = 0; i < wb.get_nrenvelopes(); ++i) {
						const WeightBalance::Envelope& env(wb.get_envelope(i));
						std::ostringstream row0, row1;
						row0 << (50 + 2 * i);
						row1 << (51 + 2 * i);
						{
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", row0.str());
							el->set_attribute("Col", "0");
							el->set_attribute("ValueType", "60");
							el->set_child_text(env.get_name());
						}
						{
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", row1.str());
							el->set_attribute("Col", "0");
							el->set_attribute("ValueType", "60");
							el->set_child_text(env.get_name());
						}
						if (!env.size())
							continue;
						for (unsigned int j = 0; j <= env.size(); ++j) {
							const WeightBalance::Envelope::Point& pt(env[(j >= env.size()) ? 0 : j]);
							armmin = std::min(armmin, pt.get_arm());
							armmax = std::max(armmax, pt.get_arm());
							massmin = std::min(massmin, pt.get_mass());
							massmax = std::max(massmax, pt.get_mass());
							std::ostringstream col;
							col << (1 + j);
							{
								std::ostringstream oss;
								oss << pt.get_mass();
								xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
								el->set_attribute("Row", row0.str());
								el->set_attribute("Col", col.str());
								el->set_attribute("ValueType", "40");
								el->set_child_text(oss.str());
							}
							{
								std::ostringstream oss;
								oss << pt.get_arm();
								xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
								el->set_attribute("Row", row1.str());
								el->set_attribute("Col", col.str());
								el->set_attribute("ValueType", "40");
								el->set_child_text(oss.str());
							}
						}
					}
					{
						unsigned int rowcnt(100);
						const Distances& dists(get_dist());
						for (unsigned int i = 0; i < dists.get_nrtakeoffdist(); ++i, ++rowcnt) {
							const Distances::Distance& dist(dists.get_takeoffdist(i));
							std::ostringstream row;
							row << rowcnt;
							{
								xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
								el->set_attribute("Row", row.str());
								el->set_attribute("Col", "0");
								el->set_attribute("ValueType", "60");
								el->set_child_text("Takeoff");
							}
							{
								xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
								el->set_attribute("Row", row.str());
								el->set_attribute("Col", "1");
								el->set_attribute("ValueType", "60");
								el->set_child_text(dist.get_name());
							}
							{
								xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
								el->set_attribute("Row", row.str());
								el->set_attribute("Col", "2");
								el->set_child_text("=X101");
							}
							{
								std::ostringstream oss;
								if (std::isnan(kggain))
									oss << "=AA" << (rowcnt + 1);
								else
									oss << "=(E43-" << kgoffs << ")*" << (1.0 / kggain);
								xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
								el->set_attribute("Row", row.str());
								el->set_attribute("Col", "4");
								el->set_child_text(oss.str());
							}
							{
								std::ostringstream oss;
								oss << dist.get_vrotate();
								xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
								el->set_attribute("Row", row.str());
								el->set_attribute("Col", "25");
								el->set_attribute("ValueType", "40");
								el->set_child_text(oss.str());
							}
							{
								std::ostringstream oss;
								oss << dist.get_mass();
								xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
								el->set_attribute("Row", row.str());
								el->set_attribute("Col", "26");
								el->set_attribute("ValueType", "40");
								el->set_child_text(oss.str());
							}
							{
								Poly1D<double> p(dist.get_gndpoly().convert(dists.get_distunit(), Aircraft::unit_m));
								unsigned int colcnt(30);
								for (Poly1D<double>::const_iterator pi(p.begin()), pe(p.end()); pi != pe; ++pi, ++colcnt) {
									std::ostringstream col, oss;
									col << colcnt;
									oss << *pi;
									xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
									el->set_attribute("Row", row.str());
									el->set_attribute("Col", col.str());
									el->set_attribute("ValueType", "40");
									el->set_child_text(oss.str());
								}
							}
							{
								Poly1D<double> p(dist.get_obstpoly().convert(dists.get_distunit(), Aircraft::unit_m));
								unsigned int colcnt(40);
								for (Poly1D<double>::const_iterator pi(p.begin()), pe(p.end()); pi != pe; ++pi, ++colcnt) {
									std::ostringstream col, oss;
									col << colcnt;
									oss << *pi;
									xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
									el->set_attribute("Row", row.str());
									el->set_attribute("Col", col.str());
									el->set_attribute("ValueType", "40");
									el->set_child_text(oss.str());
								}
							}
						}
						for (unsigned int i = 0; i < dists.get_nrldgdist(); ++i, ++rowcnt) {
							const Distances::Distance& dist(dists.get_ldgdist(i));
							std::ostringstream row;
							row << rowcnt;
							{
								xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
								el->set_attribute("Row", row.str());
								el->set_attribute("Col", "0");
								el->set_attribute("ValueType", "60");
								el->set_child_text("Landing");
							}
							{
								xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
								el->set_attribute("Row", row.str());
								el->set_attribute("Col", "1");
								el->set_attribute("ValueType", "60");
								el->set_child_text(dist.get_name());
							}
							{
								xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
								el->set_attribute("Row", row.str());
								el->set_attribute("Col", "2");
								el->set_child_text("=Y101");
							}
							{
								std::ostringstream oss;
								if (std::isnan(kggain))
									oss << "=AA" << (rowcnt + 1);
								else
									oss << "=(E42-" << kgoffs << ")*" << (1.0 / kggain);
								xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
								el->set_attribute("Row", row.str());
								el->set_attribute("Col", "4");
								el->set_child_text(oss.str());
							}
							{
								std::ostringstream oss;
								oss << dist.get_vrotate();
								xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
								el->set_attribute("Row", row.str());
								el->set_attribute("Col", "25");
								el->set_attribute("ValueType", "40");
								el->set_child_text(oss.str());
							}
							{
								std::ostringstream oss;
								oss << dist.get_mass();
								xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
								el->set_attribute("Row", row.str());
								el->set_attribute("Col", "26");
								el->set_attribute("ValueType", "40");
								el->set_child_text(oss.str());
							}
							{
								Poly1D<double> p(dist.get_gndpoly().convert(dists.get_distunit(), Aircraft::unit_m));
								unsigned int colcnt(30);
								for (Poly1D<double>::const_iterator pi(p.begin()), pe(p.end()); pi != pe; ++pi, ++colcnt) {
									std::ostringstream col, oss;
									col << colcnt;
									oss << *pi;
									xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
									el->set_attribute("Row", row.str());
									el->set_attribute("Col", col.str());
									el->set_attribute("ValueType", "40");
									el->set_child_text(oss.str());
								}
							}
							{
								Poly1D<double> p(dist.get_obstpoly().convert(dists.get_distunit(), Aircraft::unit_m));
								unsigned int colcnt(40);
								for (Poly1D<double>::const_iterator pi(p.begin()), pe(p.end()); pi != pe; ++pi, ++colcnt) {
									std::ostringstream col, oss;
									col << colcnt;
									oss << *pi;
									xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
									el->set_attribute("Row", row.str());
									el->set_attribute("Col", col.str());
									el->set_attribute("ValueType", "40");
									el->set_child_text(oss.str());
								}
							}
						}
					}
					{
						const FPlanWaypoint& wpt(fplan[0]);
						{
							std::ostringstream oss;
							oss << wpt.get_density_altitude();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", "100");
							el->set_attribute("Col", "23");
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
					}
					{
						const FPlanWaypoint& wpt(fplan[fplan.get_nrwpt() - 1]);
						{
							std::ostringstream oss;
							oss << wpt.get_density_altitude();
							xmlpp::Element *el((*ni)->add_child("Cell", root->get_namespace_prefix()));
							el->set_attribute("Row", "100");
							el->set_attribute("Col", "24");
							el->set_attribute("ValueType", "40");
							el->set_child_text(oss.str());
						}
					}
					break;
				}

				default:
					break;
				}
			}
			xmlpp::Node::NodeList nlrows((*ni)->get_children("Rows"));
			for (xmlpp::Node::NodeList::const_iterator ni(nlrows.begin()), ne(nlrows.end()); ni != ne; ++ni) {
				switch (sheetnr) {
				case 0:
				{
					{
						unsigned int firstrow(cellspecs.get_fplfirstrow() + cellspecs.get_fplrows() * fplan.get_nrwpt());
						if (fplan.get_nrwpt() && cellspecs.get_pgbrkinterval())
							firstrow += ((fplan.get_nrwpt() - 1 + cellspecs.get_pgbrkoffset())
								     / cellspecs.get_pgbrkinterval()) * cellspecs.get_pgbrkheader();
						unsigned int lastrow(cellspecs.get_fplfirstrow() + cellspecs.get_fplrows() * cellspecs.get_pagelen());
						if (cellspecs.get_pgbrkinterval())
							lastrow += ((cellspecs.get_pagelen() - 1 + cellspecs.get_pgbrkoffset())
								    / cellspecs.get_pgbrkinterval()) * cellspecs.get_pgbrkheader();
						std::ostringstream nr, count;
						nr << firstrow;
						count << (lastrow-firstrow);
						xmlpp::Element *el((*ni)->add_child("RowInfo", root->get_namespace_prefix()));
						el->set_attribute("No", nr.str());
						el->set_attribute("Count", count.str());
						el->set_attribute("Unit", "12.1");
						el->set_attribute("Hidden", "1");
					}
					if (atcfpllen && atcfplrow != ~0U && cellspecs.is_rowadj() &&
					    (cellspecs.get_rowadjlineoffs() >= 0 || atcfpllen > (unsigned int)-cellspecs.get_rowadjlineoffs())) {
						unsigned int nrlines(atcfpllen + cellspecs.get_rowadjlineoffs() + cellspecs.get_rowadjlinelen() - 1U);
						nrlines /= cellspecs.get_rowadjlinelen();
						std::ostringstream nr, unit;
						nr << atcfplrow;
						unit << (cellspecs.get_rowadjlineheight() * nrlines + cellspecs.get_rowadjlineheightoffs());
						xmlpp::Element *el((*ni)->add_child("RowInfo", root->get_namespace_prefix()));
						el->set_attribute("No", nr.str());
						el->set_attribute("Unit", unit.str());
					}
					break;
				}

				case 1:
				{
					{
						const WeightBalance& wb(get_wb());
						{
							std::ostringstream nr, count;
							nr << (15 + wb.get_nrelements());
							count << (25 - wb.get_nrelements());
							xmlpp::Element *el((*ni)->add_child("RowInfo", root->get_namespace_prefix()));
							el->set_attribute("No", nr.str());
							el->set_attribute("Count", count.str());
							el->set_attribute("Unit", "12.1");
							el->set_attribute("Hidden", "1");
						}
						{
							std::ostringstream nr, count;
							if (false) {
								nr << (50 + 2 * wb.get_nrenvelopes());
								count << (46 - 2 * wb.get_nrenvelopes());
							} else {
								nr << 48;
								count << 48;
							}
							xmlpp::Element *el((*ni)->add_child("RowInfo", root->get_namespace_prefix()));
							el->set_attribute("No", nr.str());
							el->set_attribute("Count", count.str());
							el->set_attribute("Unit", "12.1");
							el->set_attribute("Hidden", "1");
						}
					}
					{
						const Distances& dists(get_dist());
						unsigned int unr(dists.get_nrtakeoffdist() + dists.get_nrldgdist());
						unsigned int ucount(10 - unr);
						if (!unr) {
							unr -= 3;
							ucount += 3;
						}
						unr += 100;
						std::ostringstream nr, count;
						nr << unr;
						count << ucount;
						xmlpp::Element *el((*ni)->add_child("RowInfo", root->get_namespace_prefix()));
						el->set_attribute("No", nr.str());
						el->set_attribute("Count", count.str());
						el->set_attribute("Unit", "12.1");
						el->set_attribute("Hidden", "1");
					}
					break;
				}

				default:
					break;
				}
			}
			xmlpp::Node::NodeList nlprintinfo((*ni)->get_children("PrintInformation"));
			for (xmlpp::Node::NodeList::const_iterator ni(nlprintinfo.begin()), ne(nlprintinfo.end()); ni != ne; ++ni) {
				xmlpp::Node::NodeList nlhpgbrk((*ni)->get_children("hPageBreaks"));
				for (xmlpp::Node::NodeList::const_iterator ni(nlhpgbrk.begin()), ne(nlhpgbrk.end()); ni != ne; ++ni) {
					switch (sheetnr) {
					case 0:
					{
						if (!cellspecs.get_pgbrkinterval())
							break;
						{
							xmlpp::Node::NodeList children((*ni)->get_children());
							for (xmlpp::Node::NodeList::const_iterator nni(children.begin()), nne(children.end()); nni != nne; ++nni)
								(*ni)->remove_child(*nni);
						}
						unsigned int count(0);
						for (unsigned int i(1), n(fplan.get_nrwpt()); i < n; ++i) {
							if (i > cellspecs.get_pagelen())
								break;
							unsigned int row;
							{
								unsigned int x(i - 1 + cellspecs.get_pgbrkoffset());
								row = x / cellspecs.get_pgbrkinterval();
								x -= row * cellspecs.get_pgbrkinterval();
								if (x)
									continue;
							}
							row *= cellspecs.get_pgbrkheader();
							row += cellspecs.get_fplfirstrow() + cellspecs.get_fplrows() * (i - 1);
							row -= cellspecs.get_pgbrkheader();
							std::ostringstream oss;
							oss << row;
							xmlpp::Element *el((*ni)->add_child("break", root->get_namespace_prefix()));
							el->set_attribute("pos", oss.str());
							el->set_attribute("type", "manual");
							++count;
						}
						std::ostringstream oss;
						oss << count;
						static_cast<xmlpp::Element *>(*ni)->set_attribute("count", oss.str());
						break;
					}

					default:
						break;
					}
				}
			}
			xmlpp::Node::NodeList nlobjs((*ni)->get_children("Objects"));
			for (xmlpp::Node::NodeList::const_iterator ni(nlobjs.begin()), ne(nlobjs.end()); ni != ne; ++ni) {
				xmlpp::Node::NodeList nlobjgraphs((*ni)->get_children("SheetObjectGraph"));
				for (xmlpp::Node::NodeList::const_iterator ni(nlobjgraphs.begin()), ne(nlobjgraphs.end()); ni != ne; ++ni) {
					xmlpp::Node::NodeList nlgogobjgraph((*ni)->get_children("GogObject"));
					for (xmlpp::Node::NodeList::const_iterator ni(nlgogobjgraph.begin()), ne(nlgogobjgraph.end()); ni != ne; ++ni) {
						        xmlpp::Attribute *attr;
							if (!(attr = static_cast<xmlpp::Element *>(*ni)->get_attribute("type")))
								continue;
							if (attr->get_value() != "GogGraph")
								continue;
							xmlpp::Node::NodeList nlgogobjchart((*ni)->get_children("GogObject"));
							for (xmlpp::Node::NodeList::const_iterator ni(nlgogobjchart.begin()), ne(nlgogobjchart.end()); ni != ne; ++ni) {
								xmlpp::Attribute *attr;
								if (!(attr = static_cast<xmlpp::Element *>(*ni)->get_attribute("role")))
									continue;
								if (attr->get_value() != "Chart")
									continue;
								xmlpp::Node::NodeList nlgogobjax((*ni)->get_children("GogObject"));
								for (xmlpp::Node::NodeList::const_iterator ni(nlgogobjax.begin()), ne(nlgogobjax.end()); ni != ne; ++ni) {
									xmlpp::Attribute *attr;
									if (!(attr = static_cast<xmlpp::Element *>(*ni)->get_attribute("role")))
										continue;
									bool xaxis(attr->get_value() == "X-Axis");
									bool yaxis(attr->get_value() == "Y-Axis");
									if (!xaxis && !yaxis)
										continue;
									xmlpp::Node::NodeList nld((*ni)->get_children("data"));
									for (xmlpp::Node::NodeList::const_iterator ni(nld.begin()), ne(nld.end()); ni != ne; ++ni) {
										switch (sheetnr) {
										case 1:
											if (xaxis && !std::isnan(armmin) && !std::isnan(armmax) && armmin < armmax) {
												double scale(1);
												{
													double d(armmax - armmin);
													for (unsigned int i(0); i < 12; ++i) {
														double s(((i % 3) == 1) ? 2.5 : 2);
														d /= s;
														if (d < 20)
															break;
														scale *= s;
													}
												}
												{
													std::ostringstream oss;
													oss << (scale * std::floor((armmin - 0.1 * (armmax - armmin)) / scale));
													xmlpp::Element *el((*ni)->add_child("dimension"));
													el->set_attribute("id", "0");
													el->set_attribute("type", "GnmGODataScalar");
													el->set_child_text(oss.str());
												}
												{
													std::ostringstream oss;
													oss << (scale * std::ceil((armmax + 0.1 * (armmax - armmin)) / scale));
													xmlpp::Element *el((*ni)->add_child("dimension"));
													el->set_attribute("id", "1");
													el->set_attribute("type", "GnmGODataScalar");
													el->set_child_text(oss.str());
												}
											}
											if (yaxis && !std::isnan(massmin) && !std::isnan(massmax) && massmin < massmax) {
												double scale(1);
												{
													double d(massmax - massmin);
													for (unsigned int i(0); i < 12; ++i) {
														double s(((i % 3) == 1) ? 2.5 : 2);
														d /= s;
														if (d < 20)
															break;
														scale *= s;
													}
												}
												{
													std::ostringstream oss;
													oss << (scale * std::floor((massmin - 0.1 * (massmax - massmin)) / scale));
													xmlpp::Element *el((*ni)->add_child("dimension"));
													el->set_attribute("id", "0");
													el->set_attribute("type", "GnmGODataScalar");
													el->set_child_text(oss.str());
												}
												{
													std::ostringstream oss;
													oss << (scale * std::ceil((massmax + 0.1 * (massmax - massmin)) / scale));
													xmlpp::Element *el((*ni)->add_child("dimension"));
													el->set_attribute("id", "1");
													el->set_attribute("type", "GnmGODataScalar");
													el->set_child_text(oss.str());
												}
											}
											break;

										default:
											break;
										}
									}
								}
							}
					}
				}
			}
		}
	}
	doc->write_to_file_formatted(outfile);
}

namespace {

class PLogParam {
public:
	PLogParam(const Aircraft& acft, Aircraft::unit_t fuelunit);
	void set_gfs(gint64 gfsminreftime, gint64 gfsmaxreftime, gint64 gfsminefftime, gint64 gfsmaxefftime);
	void set_atcfpl(const std::string& atcfplc, Engine& engine, const FPlanRoute& fplan, const std::vector<FPlanAlternate>& altn,
			const Aircraft::Cruise::EngineParams& epcruise);
	void clear_engine(void);
	void set_engine(const FPlanWaypoint& wpt);
	void set_service(const std::string& servicename, const std::string& servicelink);
	void set_depdest(const FPlanWaypoint& dep, const FPlanWaypoint& dest, double tofuel, double totdist, double gcdist);
	void set_alt(const FPlanWaypoint& dest, const FPlanWaypoint& alt, double tofuel, double totdist, double gcdist);
	void set_firstline(const FPlanWaypoint& wpt, const FPlanWaypoint& wptn, time_t deptime, double totdist, double totfuel, double tofuel);
	void set_lastline(const FPlanWaypoint& wptp, const FPlanWaypoint& wpt, time_t deptime, double cumdist, double totdist, double totfuel, double tofuel);
	void set_line(const FPlanWaypoint& wptp, const FPlanWaypoint& wpt, const FPlanWaypoint& wptn, time_t deptime, double cumdist, double totdist, double totfuel, double tofuel);
	std::ostream& write_header(std::ostream& os) { return write_param(os, par_begin, par_headerenddest); }
	std::ostream& write_altheader(std::ostream& os) { return write_param(os, par_headerbeginalt, par_headeraltenroutetime); }
	std::ostream& write_line(std::ostream& os) { return write_param(os, par_linebeginwptp, par_lineendpathn); }

protected:
	typedef enum {
		par_begin,
		par_headergfsminreftime = par_begin,
		par_headergfsmaxreftime,
		par_headergfsminefftime,
		par_headergfsmaxefftime,
		par_headerfuelunit,
		par_headerfuelname,
		par_headeratcfpl,
		par_headerservice,
		par_headergcdist,
		par_headerenroutetime,
	        par_headerengbhp,
		par_headerengrpmmp,
		par_headerbegindep,
		par_headerdepicao = par_headerbegindep,
		par_headerdepname,
		par_headerdepicaoname,
		par_headerdepident,
		par_headerdeptype,
		par_headerdeptime,
		par_headerdepflighttime,
		par_headerdepfrequency,
		par_headerdeplat,
		par_headerdeplon,
		par_headerdepfplalt,
		par_headerdepalt,
		par_headerdepdensityalt,
		par_headerdeptruealt,
		par_headerdeppressurealt,
		par_headerdepprofalt,
		par_headerdepclimb,
		par_headerdepdescent,
		par_headerdepstdroute,
		par_headerdepturnpt,
		par_headerdepwinddir,
		par_headerdepwindspeed,
		par_headerdepqff,
		par_headerdepisaoffs,
		par_headerdepoat,
		par_headerdepdist,
		par_headerdepremdist,
		par_headerdepmass,
		par_headerdepfuel,
		par_headerdepremfuel,
		par_headerdeptotremfuel,
		par_headerenddep = par_headerdeptotremfuel,
		par_headerbegindest,
		par_headerdesticao = par_headerbegindest,
		par_headerdestname,
		par_headerdesticaoname,
		par_headerdestident,
		par_headerdesttype,
		par_headerdesttime,
		par_headerdestflighttime,
		par_headerdestfrequency,
		par_headerdestlat,
		par_headerdestlon,
		par_headerdestfplalt,
		par_headerdestalt,
		par_headerdestdensityalt,
		par_headerdesttruealt,
		par_headerdestpressurealt,
		par_headerdestprofalt,
		par_headerdestclimb,
		par_headerdestdescent,
		par_headerdeststdroute,
		par_headerdestturnpt,
		par_headerdestwinddir,
		par_headerdestwindspeed,
		par_headerdestqff,
		par_headerdestisaoffs,
		par_headerdestoat,
		par_headerdestdist,
		par_headerdestremdist,
		par_headerdestmass,
		par_headerdestfuel,
		par_headerdestremfuel,
		par_headerdesttotremfuel,
		par_headerenddest = par_headerdesttotremfuel,
		par_headerbeginalt,
		par_headeralticao = par_headerbeginalt,
		par_headeraltname,
		par_headeralticaoname,
		par_headeraltident,
		par_headeralttype,
		par_headeralttime,
		par_headeraltflighttime,
		par_headeraltfrequency,
		par_headeraltlat,
		par_headeraltlon,
		par_headeraltfplalt,
		par_headeraltalt,
		par_headeraltdensityalt,
		par_headeralttruealt,
		par_headeraltpressurealt,
		par_headeraltprofalt,
		par_headeraltclimb,
		par_headeraltdescent,
		par_headeraltstdroute,
		par_headeraltturnpt,
		par_headeraltwinddir,
		par_headeraltwindspeed,
		par_headeraltqff,
		par_headeraltisaoffs,
		par_headeraltoat,
		par_headeraltdist,
		par_headeraltremdist,
		par_headeraltmass,
		par_headeraltfuel,
		par_headeraltremfuel,
		par_headeralttotremfuel,
		par_headerendalt = par_headeralttotremfuel,
		par_headeraltgcdist,
		par_headeraltenroutetime,
		par_linebeginwptp,
		par_linewptpicao = par_linebeginwptp,
		par_linewptpname,
		par_linewptpicaoname,
		par_linewptpident,
		par_linewptptype,
		par_linewptptime,
		par_linewptpflighttime,
		par_linewptpfrequency,
		par_linewptplat,
		par_linewptplon,
		par_linewptpfplalt,
		par_linewptpalt,
		par_linewptpdensityalt,
		par_linewptptruealt,
		par_linewptppressurealt,
		par_linewptpprofalt,
		par_linewptpclimb,
		par_linewptpdescent,
		par_linewptpstdroute,
		par_linewptpturnpt,
		par_linewptpwinddir,
		par_linewptpwindspeed,
		par_linewptpqff,
		par_linewptpisaoffs,
		par_linewptpoat,
		par_linewptpdist,
		par_linewptpremdist,
		par_linewptpmass,
		par_linewptpfuel,
		par_linewptpremfuel,
		par_linewptptotremfuel,
		par_lineendwptp = par_linewptptotremfuel,
		par_linebeginwpt,
		par_linewpticao = par_linebeginwpt,
		par_linewptname,
		par_linewpticaoname,
		par_linewptident,
		par_linewpttype,
		par_linewpttime,
		par_linewptflighttime,
		par_linewptfrequency,
		par_linewptlat,
		par_linewptlon,
		par_linewptfplalt,
		par_linewptalt,
		par_linewptdensityalt,
		par_linewpttruealt,
		par_linewptpressurealt,
		par_linewptprofalt,
		par_linewptclimb,
		par_linewptdescent,
		par_linewptstdroute,
		par_linewptturnpt,
		par_linewptwinddir,
		par_linewptwindspeed,
		par_linewptqff,
		par_linewptisaoffs,
		par_linewptoat,
		par_linewptdist,
		par_linewptremdist,
		par_linewptmass,
		par_linewptfuel,
		par_linewptremfuel,
		par_linewpttotremfuel,
		par_lineendwpt = par_linewpttotremfuel,
		par_linebeginwptn,
		par_linewptnicao = par_linebeginwptn,
		par_linewptnname,
		par_linewptnicaoname,
		par_linewptnident,
		par_linewptntype,
		par_linewptntime,
		par_linewptnflighttime,
		par_linewptnfrequency,
		par_linewptnlat,
		par_linewptnlon,
		par_linewptnfplalt,
		par_linewptnalt,
		par_linewptndensityalt,
		par_linewptntruealt,
		par_linewptnpressurealt,
		par_linewptnprofalt,
		par_linewptnclimb,
		par_linewptndescent,
		par_linewptnstdroute,
		par_linewptnturnpt,
		par_linewptnwinddir,
		par_linewptnwindspeed,
		par_linewptnqff,
		par_linewptnisaoffs,
		par_linewptnoat,
		par_linewptndist,
		par_linewptnremdist,
		par_linewptnmass,
		par_linewptnfuel,
		par_linewptnremfuel,
		par_linewptntotremfuel,
		par_lineendwptn = par_linewptntotremfuel,
		par_linebeginpathp,
		par_linepathppathcode = par_linebeginpathp,
		par_linepathppathname,
		par_linepathpfrule,
		par_linepathpstay,
		par_linepathptime,
		par_linepathpdist,
		par_linepathptt,
		par_linepathpth,
		par_linepathpmt,
		par_linepathpmh,
		par_linepathpdecl,
		par_linepathpfuel,
		par_linepathptas,
		par_linepathpgs,
		par_linepathpeng,
		par_linepathpbhp,
		par_linepathprpm,
		par_linepathpmp,
		par_linepathpterrain,
		par_linepathpmsa,
		par_lineendpathp = par_linepathpmsa,
		par_linebeginpathn,
		par_linepathnpathcode = par_linebeginpathn,
		par_linepathnpathname,
		par_linepathnfrule,
		par_linepathnstay,
		par_linepathntime,
		par_linepathndist,
		par_linepathntt,
		par_linepathnth,
		par_linepathnmt,
		par_linepathnmh,
		par_linepathndecl,
		par_linepathnfuel,
		par_linepathntas,
		par_linepathngs,
		par_linepathneng,
		par_linepathnbhp,
		par_linepathnrpm,
		par_linepathnmp,
		par_linepathnterrain,
		par_linepathnmsa,
		par_lineendpathn = par_linepathnmsa,
		par_nrparam
	} par_t;
	static const char * const param_names[par_nrparam];
	std::string m_param[par_nrparam];
	const Aircraft& m_acft;
	Aircraft::unit_t m_fuelunit;

	void set_point(const FPlanWaypoint& wpt, par_t offs, time_t deptime, double totalfuel, double tofuel,
		       double cumdist = std::numeric_limits<double>::quiet_NaN(),
		       double remdist = std::numeric_limits<double>::quiet_NaN());
	void set_path(const FPlanWaypoint& wpt, const FPlanWaypoint& wptn, par_t offs);
	void clear_param(par_t from, par_t to);
	std::ostream& write_param(std::ostream& os, par_t from, par_t to);
};

const char * const PLogParam::param_names[par_nrparam] = {
	"headergfsminreftime",
	"headergfsmaxreftime",
	"headergfsminefftime",
	"headergfsmaxefftime",
	"headerfuelunit",
	"headerfuelname",
	"headeratcfpl",
	"headerservice",
	"headergcdist",
	"headerenroutetime",
	"headerengbhp",
	"headerengrpmmp",
	"headerdepicao",
	"headerdepname",
	"headerdepicaoname",
	"headerdepident",
	"headerdeptype",
	"headerdeptime",
	"headerdepflighttime",
	"headerdepfrequency",
	"headerdeplat",
	"headerdeplon",
	"headerdepfplalt",
	"headerdepalt",
	"headerdepdensityalt",
	"headerdeptruealt",
	"headerdeppressurealt",
	"headerdepprofalt",
	"headerdepclimb",
	"headerdepdescent",
	"headerdepstdroute",
	"headerdepturnpt",
	"headerdepwinddir",
	"headerdepwindspeed",
	"headerdepqff",
	"headerdepisaoffs",
	"headerdepoat",
	"headerdepdist",
	"headerdepremdist",
	"headerdepmass",
	"headerdepfuel",
	"headerdepremfuel",
	"headerdeptotremfuel",
	"headerdesticao",
	"headerdestname",
	"headerdesticaoname",
	"headerdestident",
	"headerdesttype",
	"headerdesttime",
	"headerdestflighttime",
	"headerdestfrequency",
	"headerdestlat",
	"headerdestlon",
	"headerdestfplalt",
	"headerdestalt",
	"headerdestdensityalt",
	"headerdesttruealt",
	"headerdestpressurealt",
	"headerdestprofalt",
	"headerdestclimb",
	"headerdestdescent",
	"headerdeststdroute",
	"headerdestturnpt",
	"headerdestwinddir",
	"headerdestwindspeed",
	"headerdestqff",
	"headerdestisaoffs",
	"headerdestoat",
	"headerdestdist",
	"headerdestremdist",
	"headerdestmass",
	"headerdestfuel",
	"headerdestremfuel",
	"headerdesttotremfuel",
	"headeralticao",
	"headeraltname",
	"headeralticaoname",
	"headeraltident",
	"headeralttype",
	"headeralttime",
	"headeraltflighttime",
	"headeraltfrequency",
	"headeraltlat",
	"headeraltlon",
	"headeraltfplalt",
	"headeraltalt",
	"headeraltdensityalt",
	"headeralttruealt",
	"headeraltpressurealt",
	"headeraltprofalt",
	"headeraltclimb",
	"headeraltdescent",
	"headeraltstdroute",
	"headeraltturnpt",
	"headeraltwinddir",
	"headeraltwindspeed",
	"headeraltqff",
	"headeraltisaoffs",
	"headeraltoat",
	"headeraltdist",
	"headeraltremdist",
	"headeraltmass",
	"headeraltfuel",
	"headeraltremfuel",
	"headeralttotremfuel",
	"headeraltgcdist",
	"headeraltenroutetime",
	"linewptpicao",
	"linewptpname",
	"linewptpicaoname",
	"linewptpident",
	"linewptptype",
	"linewptptime",
	"linewptpflighttime",
	"linewptpfrequency",
	"linewptplat",
	"linewptplon",
	"linewptpfplalt",
	"linewptpalt",
	"linewptpdensityalt",
	"linewptptruealt",
	"linewptppressurealt",
	"linewptpprofalt",
	"linewptpclimb",
	"linewptpdescent",
	"linewptpstdroute",
	"linewptpturnpt",
	"linewptpwinddir",
	"linewptpwindspeed",
	"linewptpqff",
	"linewptpisaoffs",
	"linewptpoat",
	"linewptpdist",
	"linewptpremdist",
	"linewptpmass",
	"linewptpfuel",
	"linewptpremfuel",
	"linewptptotremfuel",
	"linewpticao",
	"linewptname",
	"linewpticaoname",
	"linewptident",
	"linewpttype",
	"linewpttime",
	"linewptflighttime",
	"linewptfrequency",
	"linewptlat",
	"linewptlon",
	"linewptfplalt",
	"linewptalt",
	"linewptdensityalt",
	"linewpttruealt",
	"linewptpressurealt",
	"linewptprofalt",
	"linewptclimb",
	"linewptdescent",
	"linewptstdroute",
	"linewptturnpt",
	"linewptwinddir",
	"linewptwindspeed",
	"linewptqff",
	"linewptisaoffs",
	"linewptoat",
	"linewptdist",
	"linewptremdist",
	"linewptmass",
	"linewptfuel",
	"linewptremfuel",
	"linewpttotremfuel",
	"linewptnicao",
	"linewptnname",
	"linewptnicaoname",
	"linewptnident",
	"linewptntype",
	"linewptntime",
	"linewptnflighttime",
	"linewptnfrequency",
	"linewptnlat",
	"linewptnlon",
	"linewptnfplalt",
	"linewptnalt",
	"linewptndensityalt",
	"linewptntruealt",
	"linewptnpressurealt",
	"linewptnprofalt",
	"linewptnclimb",
	"linewptndescent",
	"linewptnstdroute",
	"linewptnturnpt",
	"linewptnwinddir",
	"linewptnwindspeed",
	"linewptnqff",
	"linewptnisaoffs",
	"linewptnoat",
	"linewptndist",
	"linewptnremdist",
	"linewptnmass",
	"linewptnfuel",
	"linewptnremfuel",
	"linewptntotremfuel",
	"linepathppathcode",
	"linepathppathname",
	"linepathpfrule",
	"linepathpstay",
	"linepathptime",
	"linepathpdist",
	"linepathptt",
	"linepathpth",
	"linepathpmt",
	"linepathpmh",
	"linepathpdecl",
	"linepathpfuel",
	"linepathptas",
	"linepathpgs",
	"linepathpeng",
	"linepathpbhp",
	"linepathprpm",
	"linepathpmp",
	"linepathpterrain",
	"linepathpmsa",
	"linepathnpathcode",
	"linepathnpathname",
	"linepathnfrule",
	"linepathnstay",
	"linepathntime",
	"linepathndist",
	"linepathntt",
	"linepathnth",
	"linepathnmt",
	"linepathnmh",
	"linepathndecl",
	"linepathnfuel",
	"linepathntas",
	"linepathngs",
	"linepathneng",
	"linepathnbhp",
	"linepathnrpm",
	"linepathnmp",
	"linepathnterrain",
	"linepathnmsa"
};

PLogParam::PLogParam(const Aircraft& acft, Aircraft::unit_t fuelunit)
	: m_acft(acft), m_fuelunit(fuelunit)
{
	m_param[par_headerfuelunit] = to_str(m_fuelunit, true);
	m_param[par_headerfuelname] = m_acft.get_fuelname();
}

void PLogParam::set_gfs(gint64 gfsminreftime, gint64 gfsmaxreftime, gint64 gfsminefftime, gint64 gfsmaxefftime)
{
	m_param[par_headergfsminreftime] = "";
	m_param[par_headergfsmaxreftime] = "";
	m_param[par_headergfsminefftime] = "";
	m_param[par_headergfsmaxefftime] = "";
	if (gfsminreftime >= 0 && gfsminreftime < std::numeric_limits<int64_t>::max())
		m_param[par_headergfsminreftime] = Glib::TimeVal(gfsminreftime, 0).as_iso8601();
	if (gfsmaxreftime >= 0 && gfsmaxreftime < std::numeric_limits<int64_t>::max())
		m_param[par_headergfsmaxreftime] = Glib::TimeVal(gfsmaxreftime, 0).as_iso8601();
	if (gfsminefftime >= 0 && gfsminefftime < std::numeric_limits<int64_t>::max())
		m_param[par_headergfsminefftime] = Glib::TimeVal(gfsminefftime, 0).as_iso8601();
	if (gfsmaxefftime >= 0 && gfsmaxefftime < std::numeric_limits<int64_t>::max())
		m_param[par_headergfsmaxefftime] = Glib::TimeVal(gfsmaxefftime, 0).as_iso8601();
}

void PLogParam::set_atcfpl(const std::string& atcfplc, Engine& engine, const FPlanRoute& fplan,
			   const std::vector<FPlanAlternate>& altn, const Aircraft::Cruise::EngineParams& epcruise)
{
	std::string fpl(atcfplc);
	if (fpl.empty()) {
		IcaoFlightPlan icaofpl(engine);
		icaofpl.populate(fplan, IcaoFlightPlan::awymode_collapse);
		icaofpl.set_aircraft(m_acft, epcruise);
		if (altn.size() >= 1)
			icaofpl.set_alternate1(altn[0].get_icao());
		if (altn.size() >= 2)
			icaofpl.set_alternate2(altn[1].get_icao());
		fpl = icaofpl.get_fpl();
	}
	for (std::string::size_type i(0), n(fpl.size()); i < n; ++i) {
		if (fpl[i] != '-')
			continue;
		fpl.replace(i,  1, "--");
		++i;
		n = fpl.size();
	}
	std::ostringstream oss;
	oss << "\\nohyphens{" << METARTAFChart::latex_string_meta(fpl) << '}';
	m_param[par_headeratcfpl] = oss.str();
}

void PLogParam::clear_engine(void)
{
	m_param[par_headerengbhp] = "N";
	m_param[par_headerengrpmmp] = "N";
}

void PLogParam::set_engine(const FPlanWaypoint& wpt)
{
	if (wpt.get_bhp_raw() > 0)
		m_param[par_headerengbhp] = "Y";
	if (wpt.get_rpm() > 0 || wpt.get_mp() > 0)
		m_param[par_headerengrpmmp] = "Y";
}

void PLogParam::set_service(const std::string& servicename, const std::string& servicelink)
{
	std::ostringstream oss;
	oss << "\\href{" << servicelink << "}{" << METARTAFChart::latex_string_meta(servicename) << "}";
	m_param[par_headerservice] = oss.str();
}

void PLogParam::set_depdest(const FPlanWaypoint& dep, const FPlanWaypoint& dest, double tofuel, double totdist, double gcdist)
{
	set_point(dep, par_headerbegindep, dep.get_time_unix(), dest.get_fuel_usg(), tofuel, 0, totdist);
	set_point(dest, par_headerbegindest, dep.get_time_unix(), dest.get_fuel_usg(), tofuel, totdist, 0);
	{
		std::ostringstream oss;
		if (!std::isnan(gcdist))
			oss << Conversions::dist_str(gcdist);
		m_param[par_headergcdist] = oss.str();
	}
	{
		unsigned int tm(dest.get_flighttime());
		tm /= 60;
		std::ostringstream oss;
		oss << std::setw(2) << std::setfill('0') << (tm / 60) << ':'
		    << std::setw(2) << std::setfill('0') << (tm % 60);
		m_param[par_headerenroutetime] = oss.str();
	}
}

void PLogParam::set_alt(const FPlanWaypoint& dest, const FPlanWaypoint& alt, double tofuel, double totdist, double gcdist)
{
	set_point(alt, par_headerbeginalt, dest.get_time_unix(), alt.get_fuel_usg() - dest.get_fuel_usg(), tofuel, totdist, 0);
	{
		std::ostringstream oss;
		if (!std::isnan(gcdist))
			oss << Conversions::dist_str(gcdist);
		m_param[par_headeraltgcdist] = oss.str();
	}
	{
		unsigned int tm(alt.get_flighttime());
		tm /= 60;
		std::ostringstream oss;
		oss << std::setw(2) << std::setfill('0') << (tm / 60) << ':'
		    << std::setw(2) << std::setfill('0') << (tm % 60);
		m_param[par_headeraltenroutetime] = oss.str();
	}
}

void PLogParam::set_point(const FPlanWaypoint& wpt, par_t offs, time_t deptime, double totalfuel, double tofuel, double cumdist, double remdist)
{
	m_param[offs + (par_headerdepicao - par_headerbegindep)] = METARTAFChart::latex_string_meta(wpt.get_icao());
	m_param[offs + (par_headerdepname - par_headerbegindep)] = METARTAFChart::latex_string_meta(wpt.get_name());
	m_param[offs + (par_headerdepicaoname - par_headerbegindep)] = METARTAFChart::latex_string_meta(wpt.get_icao_name());
	m_param[offs + (par_headerdepident - par_headerbegindep)] = METARTAFChart::latex_string_meta(wpt.get_ident());
	m_param[offs + (par_headerdeptype - par_headerbegindep)] = wpt.get_type_string();
	m_param[offs + (par_headerdeptime - par_headerbegindep)] = Glib::TimeVal(deptime + wpt.get_flighttime(), 0).as_iso8601();
	{
		unsigned int tm(wpt.get_flighttime());
		tm += 30;
		tm /= 60;
		std::ostringstream oss;
		oss << std::setw(2) << std::setfill('0') << (tm / 60) << ':'
		    << std::setw(2) << std::setfill('0') << (tm % 60);
		m_param[offs + (par_headerdepflighttime - par_headerbegindep)] = oss.str();
	}
	m_param[offs + (par_headerdepfrequency - par_headerbegindep)] = Conversions::freq_str(wpt.get_frequency());
	if (wpt.get_coord().is_invalid()) {
		m_param[offs + (par_headerdeplat - par_headerbegindep)] = "";
		m_param[offs + (par_headerdeplon - par_headerbegindep)] = "";
	} else {
		m_param[offs + (par_headerdeplat - par_headerbegindep)] = wpt.get_coord().get_lat_str3();
		m_param[offs + (par_headerdeplon - par_headerbegindep)] = wpt.get_coord().get_lon_str3();
	}
	{
		std::ostringstream oss;
		if (wpt.is_altitude_valid()) {
			if (wpt.is_standard()) {
				oss << 'F' << std::setw(3) << std::setfill('0') << ((wpt.get_altitude() + 50) / 100);
			} else {
				oss << wpt.get_altitude();
			}
		}
		m_param[offs + (par_headerdepfplalt - par_headerbegindep)] = oss.str();
	}
	{
		std::ostringstream oss;
		oss << wpt.get_altitude();
		m_param[offs + (par_headerdepalt - par_headerbegindep)] = oss.str();
	}
	{
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(0) << wpt.get_density_altitude();
		m_param[offs + (par_headerdepdensityalt - par_headerbegindep)] = oss.str();
	}
	{
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(0) << wpt.get_true_altitude();
		m_param[offs + (par_headerdeptruealt - par_headerbegindep)] = oss.str();
	}
	{
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(0) << wpt.get_pressure_altitude();
		m_param[offs + (par_headerdeppressurealt - par_headerbegindep)] = oss.str();
	}
	{
		std::ostringstream oss;
		oss << wpt.get_truealt();
		m_param[offs + (par_headerdepprofalt - par_headerbegindep)] = oss.str();
	}
	m_param[offs + (par_headerdepclimb - par_headerbegindep)] = wpt.is_climb() ? "Y" : "N";
	m_param[offs + (par_headerdepdescent - par_headerbegindep)] = wpt.is_descent() ? "Y" : "N";
	m_param[offs + (par_headerdepstdroute - par_headerbegindep)] = wpt.is_standard() ? "Y" : (wpt.is_partialstandardroute() ? "P" : "N");
	m_param[offs + (par_headerdepturnpt - par_headerbegindep)] = wpt.is_turnpoint() ? "Y" : "N";
	{
		std::ostringstream oss;
		oss << std::setw(3) << std::setfill('0') << Point::round<int,double>(wpt.get_winddir_deg());
		m_param[offs + (par_headerdepwinddir - par_headerbegindep)] = oss.str();
	}
	{
		std::ostringstream oss;
		oss << std::setw(2) << std::setfill('0') << Point::round<int,double>(wpt.get_windspeed_kts());
		m_param[offs + (par_headerdepwindspeed - par_headerbegindep)] = oss.str();
	}
	{
		std::ostringstream oss;
		oss << Point::round<int,double>(wpt.get_qff_hpa());
		m_param[offs + (par_headerdepqff - par_headerbegindep)] = oss.str();
	}
	{
		std::ostringstream oss;
		oss << Point::round<int,double>(wpt.get_isaoffset_kelvin());
		m_param[offs + (par_headerdepisaoffs - par_headerbegindep)] = oss.str();
	}
	{
		std::ostringstream oss;
		oss << Point::round<int,double>(wpt.get_oat_kelvin() - IcaoAtmosphere<double>::degc_to_kelvin);
		m_param[offs + (par_headerdepoat - par_headerbegindep)] = oss.str();
	}
	{
		std::ostringstream oss;
		if (!std::isnan(cumdist))
			oss << Conversions::dist_str(cumdist);
		m_param[offs + (par_headerdepdist - par_headerbegindep)] = oss.str();
	}
	{
		std::ostringstream oss;
		if (!std::isnan(remdist))
			oss << Conversions::dist_str(remdist);
		m_param[offs + (par_headerdepremdist - par_headerbegindep)] = oss.str();
	}
	{
		std::ostringstream oss;
		oss << Point::round<int,double>(wpt.get_mass_kg());
		m_param[offs + (par_headerdepmass - par_headerbegindep)] = oss.str();
	}
	{
		std::ostringstream oss;
		unsigned int prec(m_fuelunit == Aircraft::unit_usgal);
		oss << std::fixed << std::setprecision(prec) << m_acft.convert_fuel(m_acft.get_fuelunit(), m_fuelunit, wpt.get_fuel_usg());
		m_param[offs + (par_headerdepfuel - par_headerbegindep)] = oss.str();
	}
	{
		std::ostringstream oss;
		unsigned int prec(m_fuelunit == Aircraft::unit_usgal);
		oss << std::fixed << std::setprecision(prec) << m_acft.convert_fuel(m_acft.get_fuelunit(), m_fuelunit, totalfuel - wpt.get_fuel_usg());
		m_param[offs + (par_headerdepremfuel - par_headerbegindep)] = oss.str();
	}
	{
		std::ostringstream oss;
		unsigned int prec(m_fuelunit == Aircraft::unit_usgal);
		oss << std::fixed << std::setprecision(prec) << m_acft.convert_fuel(m_acft.get_fuelunit(), m_fuelunit, tofuel - wpt.get_fuel_usg());
		m_param[offs + (par_headerdeptotremfuel - par_headerbegindep)] = oss.str();
	}
}

void PLogParam::set_path(const FPlanWaypoint& wpt, const FPlanWaypoint& wptn, par_t offs)
{
	m_param[offs + (par_linepathppathcode - par_linebeginpathp)] = wpt.get_pathcode_name();
	m_param[offs + (par_linepathppathname - par_linebeginpathp)] = wpt.get_pathname();
	m_param[offs + (par_linepathpfrule - par_linebeginpathp)] = wpt.is_ifr() ? "I" : "V";
	{
		std::ostringstream oss;
		unsigned int nr, tm;
		if (wpt.is_stay(nr, tm)) {
			tm /= 60;
			oss << std::setw(2) << std::setfill('0') << (tm / 60) << ':'
			    << std::setw(2) << std::setfill('0') << (tm % 60);
		}
		m_param[offs + (par_linepathpstay - par_linebeginpathp)] = oss.str();
       	}
	{
		unsigned int tm(((wptn.get_flighttime() + 30) / 60) - ((wpt.get_flighttime() + 30) / 60));
		std::ostringstream oss;
		oss << std::setw(2) << std::setfill('0') << (tm / 60) << ':'
		    << std::setw(2) << std::setfill('0') << (tm % 60);
		m_param[offs + (par_linepathptime - par_linebeginpathp)] = oss.str();
	}
	{
		std::ostringstream oss;
		if (!std::isnan(wpt.get_dist_nmi()))
			oss << Conversions::dist_str(wpt.get_dist_nmi());
		m_param[offs + (par_linepathpdist - par_linebeginpathp)] = oss.str();
	}
	{
		std::ostringstream oss;
		oss << Point::round<int,double>(wpt.get_truetrack_deg());
		m_param[offs + (par_linepathptt - par_linebeginpathp)] = oss.str();
	}
	{
		std::ostringstream oss;
		oss << Point::round<int,double>(wpt.get_trueheading_deg());
		m_param[offs + (par_linepathpth - par_linebeginpathp)] = oss.str();
	}
	{
		std::ostringstream oss;
		oss << Point::round<int,double>(wpt.get_magnetictrack_deg());
		m_param[offs + (par_linepathpmt - par_linebeginpathp)] = oss.str();
	}
	{
		std::ostringstream oss;
		oss << Point::round<int,double>(wpt.get_magneticheading_deg());
		m_param[offs + (par_linepathpmh - par_linebeginpathp)] = oss.str();
	}
	{
		std::ostringstream oss;
		oss << Point::round<int,double>(wpt.get_declination_deg());
		m_param[offs + (par_linepathpdecl - par_linebeginpathp)] = oss.str();
	}
	{
		std::ostringstream oss;
		unsigned int prec(m_fuelunit == Aircraft::unit_usgal);
		oss << std::fixed << std::setprecision(prec) << m_acft.convert_fuel(m_acft.get_fuelunit(), m_fuelunit, wptn.get_fuel_usg() - wpt.get_fuel_usg());
		m_param[offs + (par_linepathpfuel - par_linebeginpathp)] = oss.str();
	}
	{
		std::ostringstream osstas;
		std::ostringstream ossgs;
		bool val(!wpt.is_climb() && !wpt.is_descent());
		if (val) {
			osstas << Point::round<int,double>(wpt.get_tas_kts());
			double gs(wpt.get_tas_kts());
			if (wpt.get_windspeed() > 0) {
				Wind<double> wind;
				wind.set_wind(wpt.get_winddir_deg(), wpt.get_windspeed_kts());
				wind.set_crs_tas(wpt.get_truetrack_deg(), wpt.get_tas_kts());
				if (std::isnan(wind.get_gs()) || std::isnan(wind.get_hdg()) || wind.get_gs() <= 0)
					val = false;
				else
					ossgs << Point::round<int,double>(wind.get_gs());
			} else {
				ossgs << Point::round<int,double>(gs);
			}
		} else if (wpt.get_dist() > 0) {
			if (wptn.get_flighttime() > wpt.get_flighttime()) {
				double gs(wpt.get_dist_nmi() * 3600 / (wptn.get_flighttime() - wpt.get_flighttime()));
				ossgs << '(' << Point::round<int,double>(gs) << ')';
				Wind<double> wind;
				wind.set_wind(wpt.get_winddir_deg(), wpt.get_windspeed_kts(), wptn.get_winddir_deg(), wptn.get_windspeed_kts());
				wind.set_crs_gs(wpt.get_truetrack_deg(), gs);
				if (!std::isnan(wind.get_tas())) {
					osstas << '(' << Point::round<int,double>(wind.get_tas()) << ')';
					val = true;
				}
			}
		}
		m_param[offs + (par_linepathptas - par_linebeginpathp)] = osstas.str();
		m_param[offs + (par_linepathpgs - par_linebeginpathp)] = ossgs.str();
	}
	m_param[offs + (par_linepathpeng - par_linebeginpathp)] = METARTAFChart::latex_string_meta(wpt.get_engineprofile());
	{
		std::ostringstream ossbhp;
		std::ostringstream ossrpm;
		std::ostringstream ossmp;
		if (wpt.get_bhp_raw() > 0)
			ossbhp << std::fixed << std::setprecision(1) << wpt.get_bhp();
		if (wpt.get_rpm() > 0) {
			ossrpm << wpt.get_rpm();
			if (wpt.get_mp() > 0)
				ossmp << std::fixed << std::setprecision(1) << wpt.get_mp_inhg();
		}
		m_param[offs + (par_linepathpbhp - par_linebeginpathp)] = ossbhp.str();
		m_param[offs + (par_linepathprpm - par_linebeginpathp)] = ossrpm.str();
		m_param[offs + (par_linepathpmp - par_linebeginpathp)] = ossmp.str();
	}
	{
		std::ostringstream ossterrain;
		std::ostringstream ossmsa;
		if (wpt.is_terrain_valid()) {
			ossterrain << wpt.get_terrain();
			ossmsa << wpt.get_msa();
		}
		m_param[offs + (par_linepathpterrain - par_linebeginpathp)] = ossterrain.str();
		m_param[offs + (par_linepathpmsa - par_linebeginpathp)] = ossmsa.str();
	}
}

void PLogParam::set_firstline(const FPlanWaypoint& wpt, const FPlanWaypoint& wptn, time_t deptime, double totdist, double totfuel, double tofuel)
{
	clear_param(par_linebeginwptp, par_lineendwptp);
	set_point(wpt, par_linebeginwpt, deptime, totfuel, tofuel, 0, totdist);
	{
		double dist(wpt.get_dist_nmi());
		set_point(wptn, par_linebeginwptn, deptime, totfuel, tofuel, dist, totdist - dist);
	}
	clear_param(par_linebeginpathp, par_lineendpathp);
	set_path(wpt, wptn, par_linebeginpathn);
}

void PLogParam::set_lastline(const FPlanWaypoint& wptp, const FPlanWaypoint& wpt, time_t deptime, double cumdist, double totdist, double totfuel, double tofuel)
{
	{
		double dist(cumdist - wptp.get_dist_nmi());
		set_point(wptp, par_linebeginwptp, deptime, totfuel, tofuel, dist, totdist - dist);
	}
	set_point(wpt, par_linebeginwpt, deptime, totfuel, tofuel, cumdist, totdist - cumdist);
	clear_param(par_linebeginwptn, par_lineendwptn);
	set_path(wptp, wpt, par_linebeginpathp);
	clear_param(par_linebeginpathn, par_lineendpathn);
}

void PLogParam::set_line(const FPlanWaypoint& wptp, const FPlanWaypoint& wpt, const FPlanWaypoint& wptn, time_t deptime, double cumdist, double totdist, double totfuel, double tofuel)
{
	{
		double dist(cumdist - wptp.get_dist_nmi());
		set_point(wptp, par_linebeginwptp, deptime, totfuel, tofuel, dist, totdist - dist);
	}
	set_point(wpt, par_linebeginwpt, deptime, totfuel, tofuel, cumdist, totdist - cumdist);
	{
		double dist(cumdist + wpt.get_dist_nmi());
		set_point(wptn, par_linebeginwptn, deptime, totfuel, tofuel, dist, totdist - dist);
	}
	set_path(wptp, wpt, par_linebeginpathp);
	set_path(wpt, wptn, par_linebeginpathn);
}

void PLogParam::clear_param(par_t from, par_t to)
{
	for (; from <= to; from = (par_t)(from + 1))
		m_param[from].clear();
}

std::ostream& PLogParam::write_param(std::ostream& os, par_t from, par_t to)
{
	for (; from <= to; from = (par_t)(from + 1))
		os << "\\def\\plog" << param_names[from] << '{' << m_param[from] << "}%" << std::endl;
	return os;
}

};

std::ostream& Aircraft::navfplan_latex_defpathfrom(std::ostream& os)
{
	static const char code1[] =
		"\\newlength{\\plogcolwaypointwidth}\n"
		"\\newlength{\\plogcolaltwidth}\n"
		"\\newlength{\\plogcollatlonwidth}\n"
		"\\newlength{\\plogcolmagwidth}\n"
		"\\newlength{\\plogcoltruewidth}\n"
		"\\newlength{\\plogcoldistwidth}\n"
		"\\newlength{\\plogcolfuelwidth}\n"
		"\\newlength{\\plogcolgswidth}\n"
		"\\newlength{\\plogcoletewidth}\n"
		"\\newlength{\\plogcoletawidth}\n"
		"\\newlength{\\plogcoltaswidth}\n"
		"\\newlength{\\plogcolatmowidth}\n"
		"\n"
		"\\setlength{\\plogcolwaypointwidth}{\\linewidth}\n"
		"\\settowidth{\\plogcolaltwidth}{Altitude}\n"
		"\\settowidth{\\plogcollatlonwidth}{\\narrowfont{}W888 88.88}\n"
		"\\settowidth{\\plogcolmagwidth}{888}\n"
		"\\settowidth{\\plogcoltruewidth}{888}\n"
		"\\settowidth{\\plogcoldistwidth}{Dist R}\n"
		"\\settowidth{\\plogcolfuelwidth}{Fuel R}\n"
		"\\settowidth{\\plogcolgswidth}{(888)}\n"
		"\\settowidth{\\plogcoletewidth}{88:88}\n"
		"\\settowidth{\\plogcoletawidth}{88:88}\n"
		"\\settowidth{\\plogcoltaswidth}{\\narrowfont{}8888/88.8}\n"
		"\\settowidth{\\plogcolatmowidth}{8888/-88}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcolaltwidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcollatlonwidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcolmagwidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcoltruewidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcoldistwidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcolfuelwidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcolgswidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcoletewidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcoletawidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcoltaswidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcolatmowidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-24.0\\tabcolsep}\n"
		"\n"
		"\\addtolength{\\plogcolwaypointwidth}{-11.0\\arrayrulewidth}\n"
		"\n"
		"\\newlength{\\plogtablefullwidth}\n"
		"\\setlength{\\plogtablefullwidth}{\\plogcolwaypointwidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcolaltwidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcollatlonwidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcolmagwidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcoltruewidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcoldistwidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcolfuelwidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcolgswidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcoletewidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcoletawidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcoltaswidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcolatmowidth}\n"
		"\\addtolength{\\plogtablefullwidth}{22.0\\tabcolsep}\n"
		"\\addtolength{\\plogtablefullwidth}{11.0\\arrayrulewidth}\n"
		"\n"
		"\\newcounter{plogzebracount}\n"
		"\\newcommand{\\plogrowrestartcol}{\\setcounter{plogzebracount}{0}\\gdef\\plogzebracol{1.0}}\n"
		"\\plogrowrestartcol\n"
		"\\newcommand{\\plogzebranext}{\\stepcounter{plogzebracount}\\ifthenelse{\\isodd{\\theplogzebracount}}{\\gdef\\plogzebracol{0.9}}{\\gdef\\plogzebracol{1.0}}}\n"
		"\\newcommand{\\plogzebrarowcol}{\\rowcolor[gray]{\\plogzebracol}}\n"
		"\\global\\def\\ploglineend{}\n"
		"\n"
		"\\newenvironment{plogenv}{%\n"
		"\\global\\def\\ploglineend{}%\n"
		"\\noindent\\begin{longtable}{|p{\\plogcolwaypointwidth}|p{\\plogcolaltwidth}|p{\\plogcollatlonwidth}|p{\\plogcolmagwidth}|p{\\plogcoltruewidth}|p{\\plogcoldistwidth}|p{\\plogcolfuelwidth}|p{\\plogcolgswidth}|p{\\plogcoletewidth}|p{\\plogcoletawidth}|p{\\plogcoltaswidth}|p{\\plogcolatmowidth}|}\n"
		"  \\hline\n"
		"  \\rowcolor[gray]{0.8}\\multicolumn{12}{|p{\\plogtablefullwidth}|}{{\\bf Navigation Log} \\plogheaderdepicaoname $\\rightarrow$ \\plogheaderdesticaoname\\hfill\\href{";
	static const char code2[] =
		"}{";
	static const char code3[] =
		"}} \\\\\n"
		"  \\hline\n"
		"  \\rowcolor[gray]{1.0}\\multicolumn{12}{|l|}{Dep \\StrMid{\\plogheaderdeptime}{1}{10}{} \\StrMid{\\plogheaderdeptime}{12}{16}Z DA \\plogheaderdepdensityalt{}ft Arr \\StrMid{\\plogheaderdesttime}{1}{10}{} \\StrMid{\\plogheaderdesttime}{12}{16}Z DA \\plogheaderdestdensityalt{}ft Enroute \\plogheaderenroutetime} \\\\\n"
		"  \\hline\n"
		"  \\rowcolor[gray]{0.9}\\multicolumn{12}{|p{\\plogtablefullwidth}|}{%\n"
		"    \\StrMid{\\plogheadergfsminreftime}{1}{10}[\\ploggfsminreftimedate]\\StrMid{\\plogheadergfsminreftime}{12}{13}[\\ploggfsminreftimehour]%\n"
		"    \\StrMid{\\plogheadergfsmaxreftime}{1}{10}[\\ploggfsmaxreftimedate]\\StrMid{\\plogheadergfsmaxreftime}{12}{13}[\\ploggfsmaxreftimehour]%\n"
		"    \\StrMid{\\plogheadergfsminefftime}{1}{10}[\\ploggfsminefftimedate]\\StrMid{\\plogheadergfsminefftime}{12}{13}[\\ploggfsminefftimehour]%\n"
		"    \\StrMid{\\plogheadergfsmaxefftime}{1}{10}[\\ploggfsmaxefftimedate]\\StrMid{\\plogheadergfsmaxefftime}{12}{13}[\\ploggfsmaxefftimehour]%\n"
		"    \\ifthenelse{\\equal{\\plogheadergfsminreftime}{}\\AND\\equal{\\plogheadergfsminefftime}{}}{}{WX: GFS RefTime \\ploggfsminreftimedate{} \\ploggfsminreftimehour{}Z\\ifthenelse{\\equal{\\plogheadergfsminreftime}{\\plogheadergfsmaxreftime}}{}{-\\ifthenelse{\\equal{\\ploggfsminreftimedate}{\\ploggfsmaxreftimedate}}{}{\\ploggfsmaxreftimedate }\\ploggfsmaxreftimehour{}Z} EffTime \\ploggfsminefftimedate{} \\ploggfsminefftimehour{}Z\\ifthenelse{\\equal{\\plogheadergfsminefftime}{\\plogheadergfsmaxefftime}}{}{-\\ifthenelse{\\equal{\\ploggfsminefftimedate}{\\ploggfsmaxefftimedate}}{}{\\ploggfsmaxefftimedate }\\ploggfsmaxefftimehour{}Z}}} \\\\\n"
		"  \\hline\n"
		"  \\rowcolor[gray]{1.0}\\multicolumn{12}{|l|}{{\\bf ATIS}} \\\\[12ex]\n"
		"  \\hline\n"
		"  \\rowcolor[gray]{0.9}\\multicolumn{12}{|l|}{{\\bf ATC Flightplan}} \\\\*\n"
		"  \\rowcolor[gray]{0.9}\\multicolumn{12}{|p{\\plogtablefullwidth}|}{\\plogheaderatcfpl} \\\\\n"
		"  \\hline\n"
		"  \\rowcolor[gray]{1.0}\\multicolumn{12}{|l|}{{\\bf Clearance}} \\\\[12ex]\n"
		"  \\hline\n"
		"  \\rowcolor[gray]{0.8}Waypoint & Altitude & Latitude & MT & TT & Dist L & Fuel L & GS & ETE & ETA & TAS & Wind \\\\*%\n"
		"  \\rowcolor[gray]{0.8}\\multicolumn{1}{|>{\\plogrowrestartcol}l|}{Route} & Freq & Longitude & MH & TH & Dist R & Fuel R & Terr & ATE & ATA & "
		"\\ifthenelse{\\equal{\\plogheaderengrpmmp}{Y}}{\\narrowfont{}RPM/MP}{\\ifthenelse{\\equal{\\plogheaderengbhp}{Y}}{BHP}{Engine}} & QFF/T \\\\\\hline%\n"
		"  \\endfirsthead%\n"
		"  \\hline\n"
		"  \\rowcolor[gray]{0.8}Waypoint & Altitude & Latitude & MT & TT & Dist L & Fuel L & GS & ETE & ETA & TAS & Wind \\\\*%\n"
		"  \\rowcolor[gray]{0.8}\\multicolumn{1}{|>{\\plogrowrestartcol}l|}{Route} & Freq & Longitude & MH & TH & Dist R & Fuel R & Terr & ATE & ATA & "
		"\\ifthenelse{\\equal{\\plogheaderengrpmmp}{Y}}{\\narrowfont{}RPM/MP}{\\ifthenelse{\\equal{\\plogheaderengbhp}{Y}}{BHP}{Engine}} & QFF/T \\\\\\hline%\n"
		"  \\endhead%\n"
		"  \\endfoot%\n"
		"  \\endlastfoot\\plogzebrarowcol}{\\ploglineend\\end{longtable}}\n"
		"\n"
		"\\def\\plogheader{\\begin{plogenv}}\n"
		"\\def\\plogfooter{\\end{plogenv}}\n"
		"\\def\\plogfirstline{%\n"
		"  \\global\\edef\\plogtempa{\\noexpand\\truncate[]{\\plogcolwaypointwidth}{\\ploglinewpticaoname} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinewptfplalt} &%\n"
		"    {\\noexpand\\narrowfont\\ploglinewptlat} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathnmt} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathntt} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathndist} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathnfuel} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathngs} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathntime} &%\n"
		"    &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathntas} &%\n"
		"    \\ploglinewptwinddir/\\ploglinewptwindspeed}%\n"
		"  \\global\\edef\\plogtempb{\\noexpand\\ifthenelse{\\noexpand\\equal{\\ploglinepathnfrule}{V}}{VFR}{\\noexpand\\ifthenelse{\\noexpand\\equal{\\ploglinepathnpathname}{}}{DCT}{\\ploglinepathnpathname}} &%\n"
		"    \\ploglinewptfrequency &%\n"
		"    {\\noexpand\\narrowfont\\ploglinewptlon} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathnmh} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathnth} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinewptremdist} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinewptremfuel} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\noexpand\\narrowfont\\ploglinepathnterrain} &%\n"
		"    &%\n"
		"    &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\noexpand\\ifthenelse{\\noexpand\\equal{\\ploglinepathnrpm/\\ploglinepathnmp}{/}}{\\noexpand\\ifthenelse{\\noexpand\\equal{\\ploglinepathnbhp}{}}{\\noexpand\\truncate[]{\\plogcoltaswidth}{\\noexpand\\narrowfont\\ploglinepathneng}}{\\ploglinepathnbhp}}{\\noexpand\\narrowfont\\ploglinepathnrpm/\\ploglinepathnmp}} &%\n"
		"    \\ploglinewptqff/\\ploglinewptoat}%\n"
		"  \\ploglineend\\plogtempa\\\\*\\plogzebrarowcol\n"
		"  \\plogtempb%\n"
		"  \\global\\def\\ploglineend{\\\\\\hline\\plogzebrarowcol\\plogzebranext}%\n"
		"}\n"
		"\\def\\ploglastline{%\n"
		"  \\global\\edef\\plogtempa{\\noexpand\\truncate[]{\\plogcolwaypointwidth}{\\ploglinewpticaoname} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinewptfplalt} &%\n"
		"    {\\noexpand\\narrowfont\\ploglinewptlat} &%\n"
		"    &%\n"
		"    &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\plogheaderdestdist} &%\n"
		"    \\noexpand\\multicolumn{1}{l|}{\\noexpand\\narrowfont\\plogheaderfuelname} &%\n"
		"    &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\plogheaderenroutetime} &%\n"
		"    &%\n"
		"    &%\n"
		"    \\ploglinewptwinddir/\\ploglinewptwindspeed}%\n"
		"  \\global\\edef\\plogtempb{&%\n"
		"    \\ploglinewptfrequency &%\n"
		"    {\\noexpand\\narrowfont\\ploglinewptlon} &%\n"
		"    &%\n"
		"    &%\n"
		"    &%\n"
		"    \\noexpand\\multicolumn{1}{l|}{\\plogheaderfuelunit} &%\n"
		"    &%\n"
		"    &%\n"
		"    &%\n"
		"    &%\n"
		"    \\ploglinewptqff/\\ploglinewptoat}%\n"
		"  \\ploglineend\\plogtempa\\\\*\\plogzebrarowcol\n"
		"  \\plogtempb%\n"
		"  \\global\\def\\ploglineend{\\\\\\hline\\plogzebrarowcol}%\n"
		"}\n"
		"\\let\\plogline\\plogfirstline\n"
		"\\def\\plogaltheader{%\n"
		"  \\global\\edef\\plogtemp{\\plogheaderdesticaoname $\\rightarrow$ \\plogheaderalticaoname}%\n"
		"  \\ploglineend\\rowcolor[gray]{0.8}\\multicolumn{12}{|l|}{{{\\bf Alternate} \\plogtemp}}%\n"
		"  \\global\\def\\ploglineend{\\\\\\hline\\plogzebrarowcol}%\n"
		"}\n"
		"\n";
	return os << code1 << SiteName::sitesecureurl << code2 << SiteName::siteurlnoproto << code3;
}

std::ostream& Aircraft::navfplan_latex_defpathto(std::ostream& os)
{
	static const char code1[] =
		"\\newlength{\\plogcolwaypointwidth}\n"
		"\\newlength{\\plogcolaltwidth}\n"
		"\\newlength{\\plogcollatlonwidth}\n"
		"\\newlength{\\plogcolmagwidth}\n"
		"\\newlength{\\plogcoltruewidth}\n"
		"\\newlength{\\plogcoldistwidth}\n"
		"\\newlength{\\plogcolfuelwidth}\n"
		"\\newlength{\\plogcolgswidth}\n"
		"\\newlength{\\plogcoletewidth}\n"
		"\\newlength{\\plogcoletawidth}\n"
		"\\newlength{\\plogcoltaswidth}\n"
		"\\newlength{\\plogcolatmowidth}\n"
		"\n"
		"\\setlength{\\plogcolwaypointwidth}{\\linewidth}\n"
		"\\settowidth{\\plogcolaltwidth}{Altitude}\n"
		"\\settowidth{\\plogcollatlonwidth}{\\narrowfont{}W888 88.88}\n"
		"\\settowidth{\\plogcolmagwidth}{888}\n"
		"\\settowidth{\\plogcoltruewidth}{888}\n"
		"\\settowidth{\\plogcoldistwidth}{Dist R}\n"
		"\\settowidth{\\plogcolfuelwidth}{Fuel R}\n"
		"\\settowidth{\\plogcolgswidth}{(888)}\n"
		"\\settowidth{\\plogcoletewidth}{88:88}\n"
		"\\settowidth{\\plogcoletawidth}{88:88}\n"
		"\\settowidth{\\plogcoltaswidth}{\\narrowfont{}8888/88.8}\n"
		"\\settowidth{\\plogcolatmowidth}{8888/-88}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcolaltwidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcollatlonwidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcolmagwidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcoltruewidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcoldistwidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcolfuelwidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcolgswidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcoletewidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcoletawidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcoltaswidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcolatmowidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-24.0\\tabcolsep}\n"
		"\n"
		"\\addtolength{\\plogcolwaypointwidth}{-11.0\\arrayrulewidth}\n"
		"\n"
		"\\newlength{\\plogtablefullwidth}\n"
		"\\setlength{\\plogtablefullwidth}{\\plogcolwaypointwidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcolaltwidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcollatlonwidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcolmagwidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcoltruewidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcoldistwidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcolfuelwidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcolgswidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcoletewidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcoletawidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcoltaswidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcolatmowidth}\n"
		"\\addtolength{\\plogtablefullwidth}{22.0\\tabcolsep}\n"
		"\\addtolength{\\plogtablefullwidth}{11.0\\arrayrulewidth}\n"
		"\n"
		"\\newcounter{plogzebracount}\n"
		"\\newcommand{\\plogrowrestartcol}{\\setcounter{plogzebracount}{0}\\gdef\\plogzebracol{1.0}}\n"
		"\\plogrowrestartcol\n"
		"\\newcommand{\\plogzebranext}{\\stepcounter{plogzebracount}\\ifthenelse{\\isodd{\\theplogzebracount}}{\\gdef\\plogzebracol{0.9}}{\\gdef\\plogzebracol{1.0}}}\n"
		"\\newcommand{\\plogzebrarowcol}{\\rowcolor[gray]{\\plogzebracol}}\n"
		"\\global\\def\\ploglineend{}\n"
		"\n"
		"\\newenvironment{plogenv}{%\n"
		"\\global\\def\\ploglineend{}%\n"
		"\\noindent\\begin{longtable}{|p{\\plogcolwaypointwidth}|p{\\plogcolaltwidth}|p{\\plogcollatlonwidth}|p{\\plogcolmagwidth}|p{\\plogcoltruewidth}|p{\\plogcoldistwidth}|p{\\plogcolfuelwidth}|p{\\plogcolgswidth}|p{\\plogcoletewidth}|p{\\plogcoletawidth}|p{\\plogcoltaswidth}|p{\\plogcolatmowidth}|}\n"
		"  \\hline\n"
		"  \\rowcolor[gray]{0.8}\\multicolumn{12}{|p{\\plogtablefullwidth}|}{{\\bf Navigation Log} \\plogheaderdepicaoname $\\rightarrow$ \\plogheaderdesticaoname\\hfill\\href{";
	static const char code2[] =
		"}{";
	static const char code3[] =
		"}} \\\\\n"
		"  \\hline\n"
		"  \\rowcolor[gray]{1.0}\\multicolumn{12}{|l|}{Dep \\StrMid{\\plogheaderdeptime}{1}{10}{} \\StrMid{\\plogheaderdeptime}{12}{16}Z DA \\plogheaderdepdensityalt{}ft Arr \\StrMid{\\plogheaderdesttime}{1}{10}{} \\StrMid{\\plogheaderdesttime}{12}{16}Z DA \\plogheaderdestdensityalt{}ft Enroute \\plogheaderenroutetime} \\\\\n"
		"  \\hline\n"
		"  \\rowcolor[gray]{0.9}\\multicolumn{12}{|p{\\plogtablefullwidth}|}{%\n"
		"    \\StrMid{\\plogheadergfsminreftime}{1}{10}[\\ploggfsminreftimedate]\\StrMid{\\plogheadergfsminreftime}{12}{13}[\\ploggfsminreftimehour]%\n"
		"    \\StrMid{\\plogheadergfsmaxreftime}{1}{10}[\\ploggfsmaxreftimedate]\\StrMid{\\plogheadergfsmaxreftime}{12}{13}[\\ploggfsmaxreftimehour]%\n"
		"    \\StrMid{\\plogheadergfsminefftime}{1}{10}[\\ploggfsminefftimedate]\\StrMid{\\plogheadergfsminefftime}{12}{13}[\\ploggfsminefftimehour]%\n"
		"    \\StrMid{\\plogheadergfsmaxefftime}{1}{10}[\\ploggfsmaxefftimedate]\\StrMid{\\plogheadergfsmaxefftime}{12}{13}[\\ploggfsmaxefftimehour]%\n"
		"    \\ifthenelse{\\equal{\\plogheadergfsminreftime}{}\\AND\\equal{\\plogheadergfsminefftime}{}}{}{WX: GFS RefTime \\ploggfsminreftimedate{} \\ploggfsminreftimehour{}Z\\ifthenelse{\\equal{\\plogheadergfsminreftime}{\\plogheadergfsmaxreftime}}{}{-\\ifthenelse{\\equal{\\ploggfsminreftimedate}{\\ploggfsmaxreftimedate}}{}{\\ploggfsmaxreftimedate }\\ploggfsmaxreftimehour{}Z} EffTime \\ploggfsminefftimedate{} \\ploggfsminefftimehour{}Z\\ifthenelse{\\equal{\\plogheadergfsminefftime}{\\plogheadergfsmaxefftime}}{}{-\\ifthenelse{\\equal{\\ploggfsminefftimedate}{\\ploggfsmaxefftimedate}}{}{\\ploggfsmaxefftimedate }\\ploggfsmaxefftimehour{}Z}}} \\\\\n"
		"  \\hline\n"
		"  \\rowcolor[gray]{1.0}\\multicolumn{12}{|l|}{{\\bf ATIS}} \\\\[12ex]\n"
		"  \\hline\n"
		"  \\rowcolor[gray]{0.9}\\multicolumn{12}{|l|}{{\\bf ATC Flightplan}} \\\\*\n"
		"  \\rowcolor[gray]{0.9}\\multicolumn{12}{|p{\\plogtablefullwidth}|}{\\plogheaderatcfpl} \\\\\n"
		"  \\hline\n"
		"  \\rowcolor[gray]{1.0}\\multicolumn{12}{|l|}{{\\bf Clearance}} \\\\[12ex]\n"
		"  \\hline\n"
		"  \\rowcolor[gray]{0.8}Waypoint & Altitude & Latitude & MT & TT & Dist L & Fuel L & GS & ETE & ETA & TAS & Wind \\\\*%\n"
		"  \\rowcolor[gray]{0.8}\\multicolumn{1}{|>{\\plogrowrestartcol}l|}{Route} & Freq & Longitude & MH & TH & Dist R & Fuel R & Terr & ATE & ATA & "
		"\\ifthenelse{\\equal{\\plogheaderengrpmmp}{Y}}{\\narrowfont{}RPM/MP}{\\ifthenelse{\\equal{\\plogheaderengbhp}{Y}}{BHP}{Engine}} & QFF/T \\\\\\hline%\n"
		"  \\endfirsthead%\n"
		"  \\hline\n"
		"  \\rowcolor[gray]{0.8}Waypoint & Altitude & Latitude & MT & TT & Dist L & Fuel L & GS & ETE & ETA & TAS & Wind \\\\*%\n"
		"  \\rowcolor[gray]{0.8}\\multicolumn{1}{|>{\\plogrowrestartcol}l|}{Route} & Freq & Longitude & MH & TH & Dist R & Fuel R & Terr & ATE & ATA & "
		"\\ifthenelse{\\equal{\\plogheaderengrpmmp}{Y}}{\\narrowfont{}RPM/MP}{\\ifthenelse{\\equal{\\plogheaderengbhp}{Y}}{BHP}{Engine}} & QFF/T \\\\\\hline%\n"
		"  \\endhead%\n"
		"  \\endfoot%\n"
		"  \\endlastfoot\\plogzebrarowcol}{\\ploglineend\\end{longtable}}\n"
		"\n"
		"\\def\\plogheader{\\begin{plogenv}}\n"
		"\\def\\plogfooter{\\end{plogenv}}\n"
		"\\def\\plogfirstline{%\n"
		"  \\global\\edef\\plogtempa{\\noexpand\\truncate[]{\\plogcolwaypointwidth}{\\ploglinewpticaoname} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinewptfplalt} &%\n"
		"    {\\noexpand\\narrowfont\\ploglinewptlat} &%\n"
		"    &%\n"
		"    &%\n"
		"    \\noexpand\\multicolumn{2}{r|}{\\plogheaderfuelname{} \\plogheaderfuelunit} &%\n"
		"    &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\plogheaderenroutetime} &%\n"
		"    &%\n"
		"    &%\n"
		"    \\ploglinewptwinddir/\\ploglinewptwindspeed}%\n"
		"  \\global\\edef\\plogtempb{&%\n"
		"    \\ploglinewptfrequency &%\n"
		"    {\\noexpand\\narrowfont\\ploglinewptlon} &%\n"
		"    &%\n"
		"    &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\plogheaderdestdist} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\plogheaderdepremfuel} &%\n"
		"    &%\n"
		"    &%\n"
		"    &%\n"
		"    &%\n"
		"    \\ploglinewptqff/\\ploglinewptoat}%\n"
		"  \\ploglineend\\plogtempa\\\\*\\plogzebrarowcol\n"
		"  \\plogtempb%\n"
		"  \\global\\def\\ploglineend{\\\\\\hline\\plogzebrarowcol\\plogzebranext}%\n"
		"}\n"
		"\\def\\ploglastline{%\n"
		"  \\global\\edef\\plogtempa{\\noexpand\\truncate[]{\\plogcolwaypointwidth}{\\ploglinewpticaoname} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinewptfplalt} &%\n"
		"    {\\noexpand\\narrowfont\\ploglinewptlat} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathpmt} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathptt} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathpdist} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathpfuel} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathpgs} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathptime} &%\n"
		"    &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathptas} &%\n"
		"    \\ploglinewptwinddir/\\ploglinewptwindspeed}%\n"
		"  \\global\\edef\\plogtempb{\\noexpand\\ifthenelse{\\noexpand\\equal{\\ploglinepathpfrule}{V}}{VFR}{\\noexpand\\ifthenelse{\\noexpand\\equal{\\ploglinepathppathname}{}}{DCT}{\\ploglinepathppathname}} &%\n"
		"    \\ploglinewptfrequency &%\n"
		"    {\\noexpand\\narrowfont{}\\ploglinewptlon} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathpmh} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathpth} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinewptremdist} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinewptremfuel} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\noexpand\\narrowfont\\ploglinepathpterrain} &%\n"
		"    &%\n"
		"    &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\noexpand\\ifthenelse{\\noexpand\\equal{\\ploglinepathprpm/\\ploglinepathpmp}{/}}{\\noexpand\\ifthenelse{\\noexpand\\equal{\\ploglinepathpbhp}{}}{\\noexpand\\truncate[]{\\plogcoltaswidth}{\\noexpand\\narrowfont\\ploglinepathpeng}}{\\ploglinepathpbhp}}{\\noexpand\\narrowfont\\ploglinepathprpm/\\ploglinepathpmp}} &%\n"
		"    \\ploglinewptqff/\\ploglinewptoat}%\n"
		"  \\ploglineend\\plogtempa\\\\*\\plogzebrarowcol\n"
		"  \\plogtempb%\n"
		"  \\global\\def\\ploglineend{\\\\\\hline}%\n"
		"}\n"
		"\\def\\plogline{\\ploglastline%\n"
		"  \\expandafter\\def\\expandafter\\ploglineend\\expandafter{\\ploglineend\\plogzebrarowcol\\plogzebranext}%\n"
		"}\n"
		"\\def\\plogaltheader{%\n"
		"  \\global\\edef\\plogtemp{\\plogheaderdesticaoname $\\rightarrow$ \\plogheaderalticaoname}%\n"
		"  \\ploglineend\\rowcolor[gray]{0.8}\\multicolumn{12}{|l|}{{{\\bf Alternate} \\plogtemp}}%\n"
		"  \\global\\def\\ploglineend{\\\\\\hline\\plogzebrarowcol}%\n"
		"}\n"
		"\n";
	return os << code1 << SiteName::sitesecureurl << code2 << SiteName::siteurlnoproto << code3;
}

std::ostream& Aircraft::navfplan_latex_defpathmid(std::ostream& os)
{
	static const char code1[] =
		"\\newlength{\\plogcolwaypointwidth}\n"
		"\\newlength{\\plogcolaltwidth}\n"
		"\\newlength{\\plogcollatlonwidth}\n"
		"\\newlength{\\plogcolmagwidth}\n"
		"\\newlength{\\plogcoltruewidth}\n"
		"\\newlength{\\plogcoldistfuelwidth}\n"
		"\\newlength{\\plogcolgswidth}\n"
		"\\newlength{\\plogcoletewidth}\n"
		"\\newlength{\\plogcoletawidth}\n"
		"\\newlength{\\plogcoltaswidth}\n"
		"\\newlength{\\plogcolatmowidth}\n"
		"\n"
		"\\setlength{\\plogcolwaypointwidth}{\\linewidth}\n"
		"\\settowidth{\\plogcolaltwidth}{Altitude}\n"
		"\\settowidth{\\plogcollatlonwidth}{\\narrowfont{}W888 88.88}\n"
		"\\settowidth{\\plogcolmagwidth}{888}\n"
		"\\settowidth{\\plogcoltruewidth}{888}\n"
		"\\settowidth{\\plogcoldistfuelwidth}{1888.8}\n"
		"\\settowidth{\\plogcolgswidth}{(888)}\n"
		"\\settowidth{\\plogcoletewidth}{88:88}\n"
		"\\settowidth{\\plogcoletawidth}{88:88}\n"
		"\\settowidth{\\plogcoltaswidth}{\\narrowfont{}8888/88.8}\n"
		"\\settowidth{\\plogcolatmowidth}{8888/-88}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcolaltwidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcollatlonwidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-2.0\\plogcoldistfuelwidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcoletawidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcoletewidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcolmagwidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcoltruewidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcolgswidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcoltaswidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-\\plogcolatmowidth}\n"
		"\\addtolength{\\plogcolwaypointwidth}{-24.0\\tabcolsep}\n"
		"\n"
		"\\addtolength{\\plogcolwaypointwidth}{-11.0\\arrayrulewidth}\n"
		"\n"
		"\\newlength{\\plogtablefullwidth}\n"
		"\\setlength{\\plogtablefullwidth}{\\plogcolwaypointwidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcolaltwidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcollatlonwidth}\n"
		"\\addtolength{\\plogtablefullwidth}{2.0\\plogcoldistfuelwidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcoletawidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcoletewidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcolmagwidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcoltruewidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcolgswidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcoltaswidth}\n"
		"\\addtolength{\\plogtablefullwidth}{\\plogcolatmowidth}\n"
		"\\addtolength{\\plogtablefullwidth}{22.0\\tabcolsep}\n"
		"\\addtolength{\\plogtablefullwidth}{11.0\\arrayrulewidth}\n"
		"\n"
		"\\newcounter{plogzebracount}\n"
		"\\newcommand{\\plogrowrestartcol}{\\setcounter{plogzebracount}{0}\\gdef\\plogzebracol{1.0}}\n"
		"\\plogrowrestartcol\n"
		"\\newcommand{\\plogzebranext}{\\stepcounter{plogzebracount}\\ifthenelse{\\isodd{\\theplogzebracount}}{\\gdef\\plogzebracol{0.9}}{\\gdef\\plogzebracol{1.0}}}\n"
		"\\newcommand{\\plogzebrarowcol}{\\rowcolor[gray]{\\plogzebracol}}\n"
		"\\global\\def\\ploglineend{}\n"
		"\n"
		"\\newenvironment{plogenv}{%\n"
		"\\global\\def\\ploglineend{}%\n"
		"\\noindent\\begin{longtable}{|p{\\plogcolwaypointwidth}|p{\\plogcolaltwidth}|p{\\plogcollatlonwidth}|p{\\plogcoldistfuelwidth}|p{\\plogcoletawidth}|p{\\plogcoletewidth}|p{\\plogcolmagwidth}|p{\\plogcoltruewidth}|p{\\plogcoldistfuelwidth}|p{\\plogcolgswidth}|p{\\plogcoltaswidth}|p{\\plogcolatmowidth}|}\n"
		"  \\hline\n"
		"  \\rowcolor[gray]{0.8}\\multicolumn{12}{|p{\\plogtablefullwidth}|}{{\\bf Navigation Log} \\plogheaderdepicaoname $\\rightarrow$ \\plogheaderdesticaoname\\hfill\\href{";
	static const char code2[] =
		"}{";
	static const char code3[] =
		"}} \\\\\n"
		"  \\hline\n"
		"  \\rowcolor[gray]{1.0}\\multicolumn{12}{|l|}{Dep \\StrMid{\\plogheaderdeptime}{1}{10}{} \\StrMid{\\plogheaderdeptime}{12}{16}Z DA \\plogheaderdepdensityalt{}ft Arr \\StrMid{\\plogheaderdesttime}{1}{10}{} \\StrMid{\\plogheaderdesttime}{12}{16}Z DA \\plogheaderdestdensityalt{}ft Enroute \\plogheaderenroutetime} \\\\\n"
		"  \\hline\n"
		"  \\rowcolor[gray]{0.9}\\multicolumn{12}{|p{\\plogtablefullwidth}|}{%\n"
		"    \\StrMid{\\plogheadergfsminreftime}{1}{10}[\\ploggfsminreftimedate]\\StrMid{\\plogheadergfsminreftime}{12}{13}[\\ploggfsminreftimehour]%\n"
		"    \\StrMid{\\plogheadergfsmaxreftime}{1}{10}[\\ploggfsmaxreftimedate]\\StrMid{\\plogheadergfsmaxreftime}{12}{13}[\\ploggfsmaxreftimehour]%\n"
		"    \\StrMid{\\plogheadergfsminefftime}{1}{10}[\\ploggfsminefftimedate]\\StrMid{\\plogheadergfsminefftime}{12}{13}[\\ploggfsminefftimehour]%\n"
		"    \\StrMid{\\plogheadergfsmaxefftime}{1}{10}[\\ploggfsmaxefftimedate]\\StrMid{\\plogheadergfsmaxefftime}{12}{13}[\\ploggfsmaxefftimehour]%\n"
		"    \\ifthenelse{\\equal{\\plogheadergfsminreftime}{}\\AND\\equal{\\plogheadergfsminefftime}{}}{}{WX: GFS RefTime \\ploggfsminreftimedate{} \\ploggfsminreftimehour{}Z\\ifthenelse{\\equal{\\plogheadergfsminreftime}{\\plogheadergfsmaxreftime}}{}{-\\ifthenelse{\\equal{\\ploggfsminreftimedate}{\\ploggfsmaxreftimedate}}{}{\\ploggfsmaxreftimedate }\\ploggfsmaxreftimehour{}Z} EffTime \\ploggfsminefftimedate{} \\ploggfsminefftimehour{}Z\\ifthenelse{\\equal{\\plogheadergfsminefftime}{\\plogheadergfsmaxefftime}}{}{-\\ifthenelse{\\equal{\\ploggfsminefftimedate}{\\ploggfsmaxefftimedate}}{}{\\ploggfsmaxefftimedate }\\ploggfsmaxefftimehour{}Z}}} \\\\\n"
		"  \\hline\n"
		"  \\rowcolor[gray]{1.0}\\multicolumn{12}{|l|}{{\\bf ATIS}} \\\\[12ex]\n"
		"  \\hline\n"
		"  \\rowcolor[gray]{0.9}\\multicolumn{12}{|l|}{{\\bf ATC Flightplan}} \\\\*\n"
		"  \\rowcolor[gray]{0.9}\\multicolumn{12}{|p{\\plogtablefullwidth}|}{\\plogheaderatcfpl} \\\\\n"
		"  \\hline\n"
		"  \\rowcolor[gray]{1.0}\\multicolumn{12}{|l|}{{\\bf Clearance}} \\\\[12ex]\n"
		"  \\hline\n"
		"  \\rowcolor[gray]{0.8}Waypoint & Altitude & Latitude & Dist & ETA & ETE & MT & TT & Dist & GS & TAS & Wind \\\\*%\n"
		"  \\rowcolor[gray]{0.8}\\multicolumn{1}{|>{\\plogrowrestartcol}l|}{Route} & Freq & Longitude & Fuel & ATA & ATE & MH & TH & Fuel & Terr & \\ifthenelse{\\equal{\\plogheaderengrpmmp}{Y}}{\\narrowfont{}RPM/MP}{\\ifthenelse{\\equal{\\plogheaderengbhp}{Y}}{BHP}{Engine}} & QFF/T \\\\\\hline%\n"
		"  \\endfirsthead%\n"
		"  \\hline\n"
		"  \\rowcolor[gray]{0.8}Waypoint & Altitude & Latitude & Dist & ETA & ETE & MT & TT & Dist & GS & TAS & Wind \\\\*%\n"
		"  \\rowcolor[gray]{0.8}\\multicolumn{1}{|>{\\plogrowrestartcol}l|}{Route} & Freq & Longitude & Fuel & ATA & ATE & MH & TH & Fuel & Terr & \\ifthenelse{\\equal{\\plogheaderengrpmmp}{Y}}{\\narrowfont{}RPM/MP}{\\ifthenelse{\\equal{\\plogheaderengbhp}{Y}}{BHP}{Engine}} & QFF/T \\\\\\hline%\n"
		"  \\endhead%\n"
		"  \\endfoot%\n"
		"  \\endlastfoot\\plogzebrarowcol}{\\ploglineend\\end{longtable}}\n"
		"\n"
		"\\def\\plogheader{\\begin{plogenv}}\n"
		"\\def\\plogfooter{\\end{plogenv}}\n"
		"\\def\\plogfirstline{%\n"
		"  \\global\\edef\\plogtempa{\\noexpand\\truncate[]{\\plogcolwaypointwidth}{\\ploglinewpticaoname} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinewptfplalt} &%\n"
		"    {\\noexpand\\narrowfont\\ploglinewptlat} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\plogheaderdestdist} &%\n"
		"    &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\plogheaderenroutetime} &%\n"
		"    &%\n"
		"    &%\n"
		"    \\plogheaderfuelname &%\n"
		"    &%\n"
		"    &%\n"
		"    \\ploglinewptwinddir/\\ploglinewptwindspeed}%\n"
		"  \\global\\edef\\plogtempb{&%\n"
		"    \\ploglinewptfrequency &%\n"
		"    {\\noexpand\\narrowfont\\ploglinewptlon} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\plogheaderdepremfuel} &%\n"
		"    &%\n"
		"    &%\n"
		"    &%\n"
		"    &%\n"
		"    \\plogheaderfuelunit &%\n"
		"    &%\n"
		"    &%\n"
		"    \\ploglinewptqff/\\ploglinewptoat}%\n"
		"  \\ploglineend\\plogtempa\\\\*\\plogzebrarowcol\n"
		"  \\plogtempb%\n"
		"  \\global\\def\\ploglineend{\\\\\\hline\\plogzebrarowcol\\plogzebranext}%\n"
		"}\n"
		"\\def\\ploglastline{%\n"
		"  \\global\\edef\\plogtempa{\\noexpand\\truncate[]{\\plogcolwaypointwidth}{\\ploglinewpticaoname} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinewptfplalt} &%\n"
		"    {\\noexpand\\narrowfont\\ploglinewptlat} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinewptremdist} &%\n"
		"    &}%\n"
		"  \\global\\edef\\plogtempb{\\noexpand\\multicolumn{1}{r|}{\\ploglinepathptime} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathpmt} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathptt} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathpdist} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathpgs} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathptas} &%\n"
		"    \\ploglinewptwinddir/\\ploglinewptwindspeed}%\n"
		"  \\global\\edef\\plogtempc{\\noexpand\\ifthenelse{\\noexpand\\equal{\\ploglinepathpfrule}{V}}{VFR}{\\noexpand\\ifthenelse{\\noexpand\\equal{\\ploglinepathppathname}{}}{DCT}{\\ploglinepathppathname}} &%\n"
		"    \\ploglinewptfrequency &%\n"
		"    {\\noexpand\\narrowfont{}\\ploglinewptlon} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinewptremfuel} &%\n"
		"    &}%\n"
		"  \\global\\edef\\plogtempd{ &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathpmh} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathpth} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\ploglinepathpfuel} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\noexpand\\narrowfont\\ploglinepathpterrain} &%\n"
		"    \\noexpand\\multicolumn{1}{r|}{\\noexpand\\ifthenelse{\\noexpand\\equal{\\ploglinepathprpm/\\ploglinepathpmp}{/}}{\\noexpand\\ifthenelse{\\noexpand\\equal{\\ploglinepathpbhp}{}}{\\noexpand\\truncate[]{\\plogcoltaswidth}{\\noexpand\\narrowfont\\ploglinepathpeng}}{\\ploglinepathpbhp}}{\\noexpand\\narrowfont\\ploglinepathprpm/\\ploglinepathpmp}} &%\n"
		"    \\ploglinewptqff/\\ploglinewptoat}%\n"
		"  \\ploglineend\\plogtempa\\plogtempb\\\\*\\plogzebrarowcol\n"
		"  \\plogtempc\\plogtempd%\n"
		"  \\global\\def\\ploglineend{\\\\\\hline}%\n"
		"}\n"
		"\\def\\plogline{\\ploglastline%\n"
		"  \\expandafter\\def\\expandafter\\ploglineend\\expandafter{\\ploglineend\\plogzebrarowcol\\plogzebranext}%\n"
		"}\n"
		"\\def\\plogaltheader{%\n"
		"  \\global\\edef\\plogtemp{\\plogheaderdesticaoname $\\rightarrow$ \\plogheaderalticaoname}%\n"
		"  \\ploglineend\\rowcolor[gray]{0.8}\\multicolumn{12}{|l|}{{{\\bf Alternate} \\plogtemp}}%\n"
		"  \\global\\def\\ploglineend{\\\\\\hline\\plogzebrarowcol}%\n"
		"}\n"
		"\n";
	return os << code1 << SiteName::sitesecureurl << code2 << SiteName::siteurlnoproto << code3;
}

std::ostream& Aircraft::navfplan_latex(std::ostream& os, Engine& engine, const FPlanRoute& fplan,
				       const std::vector<FPlanAlternate>& altn,
				       const std::vector<FPlanRoute>& altnfpl,
				       const Cruise::EngineParams& epcruise,
				       gint64 gfsminreftime, gint64 gfsmaxreftime,
				       gint64 gfsminefftime, gint64 gfsmaxefftime,
				       double tofuel, unit_t fuelunit, const std::string& atcfplc,
				       const std::string& servicename,
				       const std::string& servicelink) const
{
	if (fuelunit == unit_invalid)
		fuelunit = get_fuelunit();
	// dump header variables
	PLogParam par(*this, fuelunit);
	par.set_gfs(gfsminreftime, gfsmaxreftime, gfsminefftime, gfsmaxefftime);
	par.set_atcfpl(atcfplc, engine, fplan, altn, epcruise);
	par.set_service(servicename, servicelink);
	par.clear_engine();
	for (unsigned int i(0), n(fplan.get_nrwpt()); i < n; ++i)
		par.set_engine(fplan[i]);
	double totdist(0);
	if (std::isnan(tofuel) || tofuel < 0)
		tofuel = 0;
	if (fplan.get_nrwpt() > 0) {
		totdist = fplan.total_distance_nmi_dbl();
		par.set_depdest(fplan[0], fplan[fplan.get_nrwpt() - 1], tofuel, totdist, fplan.gc_distance_nmi_dbl());
		if (fplan.get_nrwpt() > 1)
			par.set_firstline(fplan[0], fplan[1], fplan[0].get_time_unix(), totdist, fplan[fplan.get_nrwpt() - 1].get_fuel_usg(), tofuel);
		if (!std::isnan(fplan[fplan.get_nrwpt() - 1].get_fuel_usg()))
			tofuel = std::max(tofuel, (double)fplan[fplan.get_nrwpt() - 1].get_fuel_usg());
	}
	par.write_header(os);
	par.write_line(os);
	os << "\\plogheader%" << std::endl;
	if (fplan.get_nrwpt() > 1) {
		os << "\\plogfirstline%" << std::endl;
		double cumdist(0);
		unsigned int n(fplan.get_nrwpt());
		for (unsigned int i(2); i < n; ++i) {
			cumdist += fplan[i - 2].get_dist_nmi();
			par.set_line(fplan[i - 2], fplan[i - 1], fplan[i], fplan[0].get_time_unix(), cumdist, totdist, fplan[n - 1].get_fuel_usg(), tofuel);
			par.write_line(os);
			os << "\\plogline%" << std::endl;
		}
		if (n >= 2) {
			cumdist += fplan[n - 2].get_dist_nmi();
			par.set_lastline(fplan[n - 2], fplan[n - 1], fplan[0].get_time_unix(), totdist, totdist, fplan[n - 1].get_fuel_usg(), tofuel);
			par.write_line(os);
			os << "\\ploglastline%" << std::endl;
		}
	}
	if (fplan.get_nrwpt() > 0) {
		Point ptdest(fplan[fplan.get_nrwpt()-1].get_coord());
		if (!ptdest.is_invalid()) {
			for (std::vector<FPlanAlternate>::const_iterator ai(altn.begin()), ae(altn.end()); ai != ae; ++ai) {
				std::vector<FPlanRoute>::const_iterator afi(altnfpl.begin()), afe(altnfpl.end());
				{
					Point ptalt(ai->get_coord());
					if (ptalt.is_invalid())
						continue;
					for (; afi != afe; ++afi)
						if (afi->get_nrwpt() > 1 && !(*afi)[0].get_coord().is_invalid() &&
						    ptdest.simple_distance_nmi((*afi)[0].get_coord()) < 1 &&
						    !(*afi)[afi->get_nrwpt()-1].get_coord().is_invalid() &&
						    ptalt.simple_distance_nmi((*afi)[afi->get_nrwpt()-1].get_coord()) < 1)
							break;
					if (afi == afe)
						continue;
				}
				double totdist(afi->total_distance_nmi_dbl());
				double cumdist(0);
				unsigned int n(afi->get_nrwpt());
				double tofuel1(tofuel - fplan[fplan.get_nrwpt() - 1].get_fuel_usg());
				par.set_alt((*afi)[0], (*afi)[afi->get_nrwpt() - 1], tofuel1, totdist, afi->gc_distance_nmi_dbl());
				par.write_altheader(os);
				os << "\\plogaltheader%"<< std::endl;
				par.set_firstline((*afi)[0], (*afi)[1], (*afi)[0].get_time_unix(), totdist, (*afi)[afi->get_nrwpt() - 1].get_fuel_usg(), tofuel1);
				par.write_line(os);
				os << "\\plogfirstline%" << std::endl;
				for (unsigned int i(2); i < n; ++i) {
					cumdist += (*afi)[i - 2].get_dist_nmi();
					par.set_line((*afi)[i - 2], (*afi)[i - 1], (*afi)[i], (*afi)[0].get_time_unix(), cumdist, totdist, (*afi)[n - 1].get_fuel_usg(), tofuel1);
					par.write_line(os);
					os << "\\plogline%" << std::endl;
				}
				if (n >= 2) {
					cumdist += (*afi)[n - 2].get_dist_nmi();
					par.set_lastline((*afi)[n - 2], (*afi)[n - 1], (*afi)[0].get_time_unix(), totdist, totdist, (*afi)[n - 1].get_fuel_usg(), tofuel1);
					par.write_line(os);
					os << "\\ploglastline%" << std::endl;
				}
			}
		}
	}
	return os << "\\plogfooter%" << std::endl;
}

std::string Aircraft::to_luastring(const std::string& x)
{
	static const char escapes[] = "abtnvfr";
	static const char hexdigits[] = "0123456789abcdef";
	std::string r;
	r.reserve(x.size() + 2);
	r.push_back('"');
	for (std::string::const_iterator i(x.begin()), e(x.end()); i != e; ++i) {
		switch (*i) {
		case 7 ... 13:
			r.push_back('\\');
			r.push_back(escapes[*i - 7]);
			break;

		case -128 ... 6:
		case 14 ... 31:
		case 127 ... 255:
			r.push_back('\\');
			r.push_back('x');
			r.push_back(hexdigits[(*i >> 4) & 15]);
			r.push_back(hexdigits[*i & 15]);
			break;

		case '"':
		case '\\':
			r.push_back('\\');
			// fall through
		default:
			r.push_back(*i);
			break;
		}
	}
	r.push_back('"');
	return r;
}

std::string Aircraft::to_luastring(double x)
{
	if (std::isnan(x))
		return "nil";
	std::ostringstream oss;
	oss << x;
	return oss.str();
}

std::string Aircraft::to_luastring(bool x)
{
	if (x)
		return "true";
	return "false";
}

std::ostream& Aircraft::write_lua_point(std::ostream& os, const std::string& ident, Aircraft::unit_t fuelunit,
					const FPlanWaypoint& wpt, time_t deptime, double totalfuel, double tofuel, double cumdist, double remdist) const
{
	os << ident << "icao = " << to_luastring(wpt.get_icao()) << ',' << std::endl
	   << ident << "name = " << to_luastring(wpt.get_name()) << ',' << std::endl
	   << ident << "icaoname = " << to_luastring(wpt.get_icao_name()) << ',' << std::endl
	   << ident << "ident = " << to_luastring(wpt.get_ident()) << ',' << std::endl
	   << ident << "type = " << to_luastring(wpt.get_type_string()) << ',' << std::endl
	   << ident << "time = " << (deptime + wpt.get_flighttime()) << ',' << std::endl
	   << ident << "timestr = " << to_luastring(Glib::TimeVal(deptime + wpt.get_flighttime(), 0).as_iso8601()) << ',' << std::endl;
	{
		unsigned int tm(wpt.get_flighttime());
		tm += 30;
		tm /= 60;
		os << ident << "flighttime = " << wpt.get_flighttime() << ',' << std::endl
		   << ident << "flighttimestr = \""
		   << std::setw(2) << std::setfill('0') << (tm / 60) << ':'
		   << std::setw(2) << std::setfill('0') << (tm % 60) << "\"," << std::endl;
	}
	os << ident << "freq = " << wpt.get_frequency() << ',' << std::endl
	   << ident << "freqstr = " << to_luastring(Conversions::freq_str(wpt.get_frequency())) << ',' << std::endl
	   << ident << "lat = " << wpt.get_coord().get_lat_deg_dbl() << ',' << std::endl
	   << ident << "lon = " << wpt.get_coord().get_lon_deg_dbl() << ',' << std::endl;
	if (wpt.get_coord().is_invalid())
		os << ident << "latstr = \"\"," << std::endl
		   << ident << "lonstr = \"\"," << std::endl;
	else
		os << ident << "latstr = " << to_luastring(wpt.get_coord().get_lat_str3()) << ',' << std::endl
		   << ident << "lonstr = " << to_luastring(wpt.get_coord().get_lon_str3()) << ',' << std::endl;
	if (wpt.is_altitude_valid())
		os << ident << "alt = " << wpt.get_altitude() << ',' << std::endl
		   << ident << "altstr = \"" << (wpt.is_standard() ? 'F' : 'A')
		   << std::setw(3) << std::setfill('0') << ((wpt.get_altitude() + 50) / 100) << "\"," << std::endl;
	os << ident << "densityalt = " << wpt.get_density_altitude() << ',' << std::endl
	   << ident << "truealt = " << wpt.get_true_altitude() << ',' << std::endl
	   << ident << "pressurealt = " << wpt.get_pressure_altitude() << ',' << std::endl
	   << ident << "ta = " << wpt.get_truealt() << ',' << std::endl
	   << ident << "altstd = " << (wpt.is_standard() ? "true" : "false") << ',' << std::endl
	   << ident << "climb = " << (wpt.is_climb() ? "true" : "false") << ',' << std::endl
	   << ident << "descent = " << (wpt.is_descent() ? "true" : "false") << ',' << std::endl
	   << ident << "stdroute = " << (wpt.is_standard() ? '2' : (wpt.is_partialstandardroute() ? '1' : '0')) << ',' << std::endl
	   << ident << "turnpt = " << (wpt.is_turnpoint() ? "true" : "false") << ',' << std::endl
	   << ident << "winddir = " << wpt.get_winddir_deg() << ',' << std::endl
	   << ident << "winddirstr = \"" << std::setw(3) << std::setfill('0') << Point::round<int,double>(wpt.get_winddir_deg()) << "\"," << std::endl
	   << ident << "windspeed = " << wpt.get_windspeed_kts() << ',' << std::endl
	   << ident << "windspeedstr = \"" << std::setw(2) << std::setfill('0') << Point::round<int,double>(wpt.get_windspeed_kts()) << "\"," << std::endl
	   << ident << "qff = " << wpt.get_qff_hpa() << ',' << std::endl
	   << ident << "qffstr = \"" << Point::round<int,double>(wpt.get_qff_hpa()) << "\"," << std::endl
	   << ident << "isaoffs = " << wpt.get_isaoffset_kelvin() << ',' << std::endl
	   << ident << "isaoffsstr = \"" << Point::round<int,double>(wpt.get_isaoffset_kelvin()) << "\"," << std::endl
	   << ident << "oat = " << (wpt.get_oat_kelvin() - IcaoAtmosphere<double>::degc_to_kelvin) << ',' << std::endl
	   << ident << "oatstr = \"" << Point::round<int,double>(wpt.get_oat_kelvin() - IcaoAtmosphere<double>::degc_to_kelvin) << "\"," << std::endl;
	if (wpt.is_tropopause_valid())
		os << ident << "tropopause = " << wpt.get_tropopause() << ',' << std::endl;
	if (!std::isnan(cumdist))
		os << ident << "cumdist = " << cumdist << ',' << std::endl
		   << ident << "cumdiststr = " << to_luastring(Conversions::dist_str(cumdist)) << ',' << std::endl;
	if (!std::isnan(remdist))
		os << ident << "remdist = " << remdist << ',' << std::endl
		   << ident << "remdiststr = " << to_luastring(Conversions::dist_str(remdist)) << ',' << std::endl;
	os << ident << "mass = " << wpt.get_mass_kg() << ',' << std::endl
	   << ident << "massstr = \"" << Point::round<int,double>(wpt.get_mass_kg()) << "\",";
	{
		std::ostringstream oss;
		unsigned int prec(fuelunit == Aircraft::unit_usgal);
		double f(convert_fuel(get_fuelunit(), fuelunit, wpt.get_fuel_usg()));
		oss << std::fixed << std::setprecision(prec) << f;
		os << std::endl << ident << "fuel = " << f << ','
		   << std::endl << ident << "fuelstr = " << to_luastring(oss.str()) << ',';
	}
	if (!std::isnan(totalfuel)) {
		std::ostringstream oss;
		unsigned int prec(fuelunit == Aircraft::unit_usgal);
		double f(convert_fuel(get_fuelunit(), fuelunit, totalfuel - wpt.get_fuel_usg()));
		oss << std::fixed << std::setprecision(prec) << f;
		os << std::endl << ident << "remfuel = " << f << ','
		   << std::endl << ident << "remfuelstr = " << to_luastring(oss.str()) << ',';
	}
	if (!std::isnan(tofuel)) {
		std::ostringstream oss;
		unsigned int prec(fuelunit == Aircraft::unit_usgal);
		double f(convert_fuel(get_fuelunit(), fuelunit, tofuel - wpt.get_fuel_usg()));
		oss << std::fixed << std::setprecision(prec) << f;
		os << std::endl << ident << "totremfuel = " << f << ','
		   << std::endl << ident << "totremfuelstr = " << to_luastring(oss.str());
	}
	return os;
}

std::ostream& Aircraft::write_lua_alternate(std::ostream& os, const std::string& ident, Aircraft::unit_t fuelunit,
					    const FPlanAlternate& alt, time_t deptime, double totalfuel, double tofuel, double cumdist, double remdist) const
{
	write_lua_point(os, ident, fuelunit, alt, deptime, totalfuel, tofuel, cumdist, remdist);
	if (alt.is_cruisealtitude_valid())
		os << std::endl << ident << "cruisealt = " << alt.get_cruisealtitude() << ',';
	if (alt.is_holdaltitude_valid())
		os << std::endl << ident << "holdalt = " << alt.get_holdaltitude() << ',';
	return os << std::endl << ident << "holdtime = " << alt.get_holdtime() << ','
		  << std::endl << ident << "holdfuel = " << convert_fuel(get_fuelunit(), fuelunit, alt.get_holdfuel_usg()) << ',';
}

std::ostream& Aircraft::write_lua_path(std::ostream& os, const std::string& ident, Aircraft::unit_t fuelunit,
				       const FPlanWaypoint& wpt, const FPlanWaypoint& wptn) const
{
	os << ident << "pathcode = " << to_luastring(wpt.get_pathcode_name()) << ',' << std::endl
	   << ident << "pathname = " << to_luastring(wpt.get_pathname()) << ',' << std::endl
	   << ident << "ifr = " << (wpt.is_ifr() ? "true" : "false") << ',' << std::endl;
	{
		unsigned int nr, tm;
		if (wpt.is_stay(nr, tm)) {
			os << ident << "staynr = " << nr << ',' << std::endl
			   << ident << "stay = " << tm << ',' << std::endl
			   << ident << "staystr = \"";
			tm /= 60;
			os << std::setw(2) << std::setfill('0') << (tm / 60) << ':'
			   << std::setw(2) << std::setfill('0') << (tm % 60) << "\"," << std::endl;
		}
	}
	{
		unsigned int tm(((wptn.get_flighttime() + 30) / 60) - ((wpt.get_flighttime() + 30) / 60));
		os << ident << "pathtime = " << (wptn.get_flighttime() - wpt.get_flighttime()) << ',' << std::endl
		   << ident << "pathtimestr = \"" << std::setw(2) << std::setfill('0') << (tm / 60) << ':'
		   << std::setw(2) << std::setfill('0') << (tm % 60) << "\"," << std::endl;
	}
	if (!std::isnan(wpt.get_dist_nmi()))
		os << ident << "dist = " << wpt.get_dist_nmi() << ',' << std::endl
		   << ident << "diststr = " << to_luastring(Conversions::dist_str(wpt.get_dist_nmi())) << ',' << std::endl;
	os << ident << "tt = " << wpt.get_truetrack_deg() << ',' << std::endl
	   << ident << "ttstr = \"" << std::setw(3) << std::setfill('0') << Point::round<int,double>(wpt.get_truetrack_deg()) << "\"," << std::endl
	   << ident << "th = " << wpt.get_trueheading_deg() << ',' << std::endl
	   << ident << "thstr = \"" << std::setw(3) << std::setfill('0') << Point::round<int,double>(wpt.get_trueheading_deg()) << "\"," << std::endl
	   << ident << "mt = " << wpt.get_magnetictrack_deg() << ',' << std::endl
	   << ident << "mtstr = \"" << std::setw(3) << std::setfill('0') << Point::round<int,double>(wpt.get_magnetictrack_deg()) << "\"," << std::endl
	   << ident << "mh = " << wpt.get_magneticheading_deg() << ',' << std::endl
	   << ident << "mhstr = \"" << std::setw(3) << std::setfill('0') << Point::round<int,double>(wpt.get_magneticheading_deg()) << "\"," << std::endl
	   << ident << "decl = " << wpt.get_declination_deg() << ',' << std::endl
	   << ident << "declstr = \"" << std::setw(3) << std::setfill('0') << Point::round<int,double>(wpt.get_declination_deg()) << "\"," << std::endl;
	{
		std::ostringstream oss;
		unsigned int prec(fuelunit == Aircraft::unit_usgal);
		double f(convert_fuel(get_fuelunit(), fuelunit, wptn.get_fuel_usg() - wpt.get_fuel_usg()));
		oss << std::fixed << std::setprecision(prec) << f;
		os << ident << "pathfuel = " << f << ',' << std::endl
		   << ident << "pathfuelstr = " << to_luastring(oss.str()) << ',' << std::endl;
	}
	{
		bool val(!wpt.is_climb() && !wpt.is_descent());
		if (val) {
			os << ident << "tas = " << wpt.get_tas_kts() << ',' << std::endl
			   << ident << "tasstr = \"" << Point::round<int,double>(wpt.get_tas_kts()) << "\"," << std::endl;
			double gs(wpt.get_tas_kts());
			if (wpt.get_windspeed() > 0) {
				Wind<double> wind;
				wind.set_wind(wpt.get_winddir_deg(), wpt.get_windspeed_kts());
				wind.set_crs_tas(wpt.get_truetrack_deg(), wpt.get_tas_kts());
				if (std::isnan(wind.get_gs()) || std::isnan(wind.get_hdg()) || wind.get_gs() <= 0)
					val = false;
				else
					os << ident << "gs = " << wind.get_gs() << ',' << std::endl
					   << ident << "gsstr = \"" << Point::round<int,double>(wind.get_gs()) << "\"," << std::endl;
			} else {
				os << ident << "gs = " << gs << ',' << std::endl
				   << ident << "gsstr = \"" << Point::round<int,double>(gs) << "\"," << std::endl;
			}
		} else if (wpt.get_dist() > 0) {
			if (wptn.get_flighttime() > wpt.get_flighttime()) {
				double gs(wpt.get_dist_nmi() * 3600 / (wptn.get_flighttime() - wpt.get_flighttime()));
				os << ident << "gs = " << gs << ',' << std::endl
				   << ident << "gsstr = \"(" << Point::round<int,double>(gs) << ")\"," << std::endl;
				Wind<double> wind;
				wind.set_wind(wpt.get_winddir_deg(), wpt.get_windspeed_kts(), wptn.get_winddir_deg(), wptn.get_windspeed_kts());
				wind.set_crs_gs(wpt.get_truetrack_deg(), gs);
				if (!std::isnan(wind.get_tas())) {
					os << ident << "tas = " << wind.get_tas() << ',' << std::endl
					   << ident << "tasstr = \"(" << Point::round<int,double>(wind.get_tas()) << ")\"," << std::endl;
        				val = true;
				}
			}
		}
	}
	{
		if (wpt.get_bhp_raw() > 0) {
			std::ostringstream ossbhp;
			ossbhp << std::fixed << std::setprecision(1) << wpt.get_bhp();
			os << ident << "bhp = " << wpt.get_bhp() << ',' << std::endl
			   << ident << "bhpstr = " << to_luastring(ossbhp.str()) << ',' << std::endl;
		}
		if (wpt.get_rpm() > 0) {
			std::ostringstream ossrpm;
			ossrpm << wpt.get_rpm();
			os << ident << "rpm = " << wpt.get_rpm() << ',' << std::endl
			   << ident << "rpmstr = " << to_luastring(ossrpm.str()) << ',' << std::endl;
			if (wpt.get_mp() > 0) {
				std::ostringstream ossmp;
				ossmp << std::fixed << std::setprecision(1) << wpt.get_mp_inhg();
				os << ident << "mp = " << wpt.get_mp() << ',' << std::endl
				   << ident << "mpstr = " << to_luastring(ossmp.str()) << ',' << std::endl;
			}
		}
	}
	if (wpt.is_terrain_valid())
		os << ident << "terrain = " << wpt.get_terrain() << ',' << std::endl
		   << ident << "msa = " << wpt.get_msa() << ',' << std::endl;
	os << ident << "engineprofile = " << to_luastring(wpt.get_engineprofile());
	return os;
}

std::ostream& Aircraft::navfplan_lualatex(std::ostream& os, Engine& engine, const FPlanRoute& fplan,
					  const std::vector<FPlanAlternate>& altn,
					  const std::vector<FPlanRoute>& altnfpl,
					  const Cruise::EngineParams& epcruise,
					  gint64 gfsminreftime, gint64 gfsmaxreftime,
					  gint64 gfsminefftime, gint64 gfsmaxefftime,
					  double tofuel, unit_t fuelunit, const std::string& atcfplc,
					  const std::string& servicename,
					  const std::string& servicelink) const
{
	if (fuelunit == unit_invalid)
		fuelunit = get_fuelunit();
	// dump header variables
	os << "  gfs = ";
	{
		char delim = '{';
		if (gfsminreftime >= 0 && gfsminreftime < std::numeric_limits<int64_t>::max()) {
			os << delim << " minreftime = " << gfsminreftime
			   << ", minreftimestr = " << to_luastring(Glib::TimeVal(gfsminreftime, 0).as_iso8601());
			delim = ',';
		}
		if (gfsmaxreftime >= 0 && gfsmaxreftime < std::numeric_limits<int64_t>::max()) {
			os << delim << " maxreftime = " << gfsmaxreftime
			   << ", maxreftimestr = " << to_luastring(Glib::TimeVal(gfsmaxreftime, 0).as_iso8601());
			delim = ',';
		}
		if (gfsminefftime >= 0 && gfsminefftime < std::numeric_limits<int64_t>::max()) {
			os << delim << " minefftime = " << gfsminefftime
			   << ", minefftimestr = " << to_luastring(Glib::TimeVal(gfsminefftime, 0).as_iso8601());
			delim = ',';
		}
		if (gfsmaxefftime >= 0 && gfsmaxefftime < std::numeric_limits<int64_t>::max()) {
			os << delim << " maxefftime = " << gfsmaxefftime
			   << ", maxefftimestr = " << to_luastring(Glib::TimeVal(gfsmaxefftime, 0).as_iso8601());
			delim = ',';
		}
		if (delim != ',')
			os << delim;
		os << " }" << std::endl;
	}
	os << "  service = { name = " << to_luastring(servicename) << ", link = " << to_luastring(servicelink) << " }" << std::endl;
	{
		std::string fpl(atcfplc);
		if (fpl.empty()) {
			IcaoFlightPlan icaofpl(engine);
			icaofpl.populate(fplan, IcaoFlightPlan::awymode_collapse);
			icaofpl.set_aircraft(*this, epcruise);
			if (altn.size() >= 1)
				icaofpl.set_alternate1(altn[0].get_icao());
			if (altn.size() >= 2)
				icaofpl.set_alternate2(altn[1].get_icao());
			fpl = icaofpl.get_fpl();
		}
		os << "  atcfpl = " << to_luastring(fpl) << std::endl;
	}
	{
		bool engbhp(false), engrpmmp(false);
		for (unsigned int i(0), n(fplan.get_nrwpt()); i < n; ++i) {
			const FPlanWaypoint& wpt(fplan[i]);
			if (wpt.get_bhp_raw() > 0)
				engbhp = true;
			if (wpt.get_rpm() > 0 || wpt.get_mp() > 0)
				engrpmmp = true;
		}
		os << "  has_eng_bhp = " << (engbhp ? "true" : "false") << std::endl
		   << "  has_eng_rpmmp = " << (engrpmmp ? "true" : "false") << std::endl;
	}
	bool noaltn(altn.empty() || (fplan.get_nrwpt() && altn.size() == 1 && !fplan[fplan.get_nrwpt()-1].get_coord().is_invalid() &&
				     fplan[fplan.get_nrwpt()-1].get_coord() == altn.front().get_coord()));
	if (fplan.get_nrwpt() > 0) {
		if (std::isnan(tofuel) || tofuel < 0)
			tofuel = 0;
		if (!std::isnan(fplan[fplan.get_nrwpt() - 1].get_fuel_usg()))
			tofuel = std::max(tofuel, (double)fplan[fplan.get_nrwpt() - 1].get_fuel_usg());
		double totdist(fplan.total_distance_nmi_dbl());
		double gcdist(fplan.gc_distance_nmi_dbl());
		os << "  fplan = {" << std::endl
		   << "    tofuel = " << tofuel << ',' << std::endl
		   << "    totdist = " << totdist << ',' << std::endl
		   << "    gcdist = " << gcdist;
		if (!std::isnan(totdist))
			os << ',' << std::endl << "    totdiststr = " << to_luastring(Conversions::dist_str(totdist));
		if (!std::isnan(gcdist))
			os << ',' << std::endl << "    gcdiststr = " << to_luastring(Conversions::dist_str(gcdist));
		{
			unsigned int tm(fplan[fplan.get_nrwpt() - 1].get_flighttime());
			os << ',' << std::endl << "    enrtime = " << tm;
			tm /= 60;
			os << ',' << std::endl << "    enrtimestr = \""
			   << std::setw(2) << std::setfill('0') << (tm / 60) << ':'
			   << std::setw(2) << std::setfill('0') << (tm % 60) << "\"";
		}
		os << ',' << std::endl << "    fuelunit = " << to_luastring(to_str(fuelunit, true))
		   << ',' << std::endl << "    fuelname = " << to_luastring(get_fuelname())
		   << ',' << std::endl << "    taxitime = " << (fplan[0].get_time_unix() - fplan.get_time_offblock_unix());
		double cumdist(0);
		unsigned int n(fplan.get_nrwpt());
		for (unsigned int i(0); i < n; ++i) {
			write_lua_point(os << ", {" << std::endl, "      ", fuelunit, fplan[i], fplan[0].get_time_unix(),
					fplan[n - 1].get_fuel_usg(), tofuel, cumdist, totdist - cumdist);
			cumdist += fplan[i].get_dist_nmi();
			if (i + 1 < n)
				write_lua_path(os << ',' << std::endl, "      ", fuelunit, fplan[i], fplan[i + 1]);
			os << " }";
		}
		os << ", alt = {";
		Point ptdest(fplan[fplan.get_nrwpt()-1].get_coord());
		if (!ptdest.is_invalid() && !noaltn) {
			bool subseq(false);
			for (std::vector<FPlanAlternate>::const_iterator ai(altn.begin()), ae(altn.end()); ai != ae; ++ai) {
				if (subseq)
					os << ',';
				subseq = true;
				os << std::endl << "      {" << std::endl;
				if (ai->is_cruisealtitude_valid())
					os << "        cruisealt = " << ai->get_cruisealtitude() << ',' << std::endl;
				if (ai->is_holdaltitude_valid())
					os << "        holdalt = " << ai->get_holdaltitude() << ',' << std::endl;
				os << "        holdtime = " << ai->get_holdtime() << ',' << std::endl
				   << "        holdfuel = " << convert_fuel(get_fuelunit(), fuelunit, ai->get_holdfuel_usg()) << ',' << std::endl;
				std::vector<FPlanRoute>::const_iterator afi(altnfpl.begin()), afe(altnfpl.end());
				{
					Point ptalt(ai->get_coord());
					if (!ptalt.is_invalid()) {
						for (; afi != afe; ++afi)
							if (afi->get_nrwpt() > 1 && !(*afi)[0].get_coord().is_invalid() &&
							    ptdest.simple_distance_nmi((*afi)[0].get_coord()) < 1 &&
							    !(*afi)[afi->get_nrwpt()-1].get_coord().is_invalid() &&
							    ptalt.simple_distance_nmi((*afi)[afi->get_nrwpt()-1].get_coord()) < 1)
								break;
					}
					if (afi == afe) {
						double tofuel1(tofuel - fplan[fplan.get_nrwpt() - 1].get_fuel_usg());
						write_lua_point(os, "        ", fuelunit, *ai, fplan[fplan.get_nrwpt() - 1].get_time_unix(),
								ai->get_fuel_usg() - fplan[fplan.get_nrwpt() - 1].get_fuel_usg(), tofuel1, 0, totdist);
						os << " }";
						continue;
					}
				}
				double totdist(afi->total_distance_nmi_dbl());
				double cumdist(0);
				unsigned int n(afi->get_nrwpt());
				double tofuel1(tofuel - fplan[fplan.get_nrwpt() - 1].get_fuel_usg());
				write_lua_point(os, "        ", fuelunit, (*afi)[afi->get_nrwpt() - 1], (*afi)[0].get_time_unix(),
						(*afi)[afi->get_nrwpt() - 1].get_fuel_usg() - (*afi)[0].get_fuel_usg(), tofuel1, 0, totdist);
				{
					double gcdist(afi->gc_distance_nmi_dbl());
					os << ',' << std::endl << "        totdist = " << totdist
					   << ',' << std::endl << "        gcdist = " << gcdist
					   << ',' << std::endl << "        gcdiststr = " << to_luastring(Conversions::dist_str(gcdist));
				}
				{
					unsigned int tm((*afi)[afi->get_nrwpt() - 1].get_flighttime());
					os << ',' << std::endl << "        enrtime = " << tm;
					tm /= 60;
					os << ',' << std::endl << "        enrtimestr = \""
					   << std::setw(2) << std::setfill('0') << (tm / 60) << ':'
					   << std::setw(2) << std::setfill('0') << (tm % 60) << "\"";
				}
				os << ',' << std::endl << "        fuelunit = " << to_luastring(to_str(fuelunit, true))
				   << ',' << std::endl << "        fuelname = " << to_luastring(get_fuelname());
				for (unsigned int i(0); i < n; ++i) {
					write_lua_point(os << ", {" << std::endl, "          ", fuelunit, (*afi)[i], (*afi)[0].get_time_unix(),
							(*afi)[n - 1].get_fuel_usg(), tofuel, cumdist, totdist - cumdist);
					cumdist += (*afi)[i].get_dist_nmi();
					if (i + 1 < n)
						write_lua_path(os << ',' << std::endl, "          ", fuelunit, (*afi)[i], (*afi)[i + 1]);
					os << " }";
				}
				os << " }";
			}
		}
		os << " } }" << std::endl;
	}
	return os;
}

std::ostream& Aircraft::massbalance_latex(std::ostream& os, const FPlanRoute& fplan,
					  const std::vector<FPlanAlternate>& altn,
					  const Cruise::EngineParams& epcruise,
					  double fuel_on_board, const WeightBalance::elementvalues_t& wbev,
					  unit_t fuelunit, unit_t massunit, double *tomass) const
{
	static const char table_header[] = "\\noindent\\begin{longtable}{|l|r|l|r|r|r|r|r|}\n"
		"  \\hline\n"
		"  \\rowcolor[gray]{0.8}Name & Value & Unit & Arm & Mass & Mass Min & Mass Max & Moment \\\\\n"
		"  \\hline\n"
		"  \\endhead\n";
	static const char table_footer[] = "\\end{longtable}\n\n";
	if (fuelunit == unit_invalid)
		fuelunit = get_fuelunit();
	if (massunit == unit_invalid)
		massunit = get_wb().get_massunit();
	WeightBalance::elementvalues_t wbv(prepare_wb(wbev, fuel_on_board, fuelunit, massunit, unit_invalid));
	const WeightBalance& wb(get_wb());
	// convert fuel stations to preferred fuel units
	FPlanRoute::FuelCalc fc(fplan.calculate_fuel(*this, altn));
	os << "\\ifneedsmassbalance\n\\subsection{Mass \\& Balance}\n\n"
	   << table_header;
	typedef enum {
		mass_zf,
		mass_althld,
		mass_alt,
		mass_ldg,
		mass_to,
		mass_ramp,
		mass_size
	} mass_t;
	double moments[mass_size], masses[mass_size];
	masses[mass_ramp] = masses[mass_zf] = 0;
	moments[mass_ramp] = moments[mass_zf] = 0;
	typedef std::pair<double,double> massmoment_t;
	typedef std::vector<massmoment_t> massmoments_t;
	typedef std::vector<massmoments_t> consumables_t;
	consumables_t consumables;
	typedef std::vector<unsigned int> consumableindex_t;
	consumableindex_t consumableindex;
	for (unsigned int i = 0; i < wb.get_nrelements(); ++i) {
		consumableindex.push_back(~0U);
		const WeightBalance::Element& wel(wb.get_element(i));
		if (i < wbv.size()) {
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(1) << wbv[i].get_value();
			os << "  % station " << i << " unit " << wbv[i].get_unit() << " value " << oss.str() << "\n";
		}
		os << "  ";
		if (i & 1)
			os << "\\rowcolor[gray]{0.9}";
		os << (std::string)METARTAFChart::latex_string_meta(wel.get_name()) << " & ";
		double mass(0);
		if (wel.get_flags() & WeightBalance::Element::flag_fixed) {
			if (i < wbv.size() && wbv[i].get_unit() < wel.get_nrunits()) {
				const WeightBalance::Element::Unit& u(wel.get_unit(wbv[i].get_unit()));
				mass = std::min(std::max(wbv[i].get_value() * u.get_factor() + u.get_offset(), wel.get_massmin()), wel.get_massmax());

			} else {
				mass = wel.get_massmin();
			}
			os << " & ";
		} else {
			if (i < wbv.size()) {
				if (wel.get_flags() & WeightBalance::Element::flag_binary)
					wbv[i].set_value(0);
				if (wbv[i].get_unit() < wel.get_nrunits()) {
					const WeightBalance::Element::Unit& u(wel.get_unit(wbv[i].get_unit()));
					mass = wbv[i].get_value() * u.get_factor() + u.get_offset();
				}
				std::ostringstream oss;
				oss << std::fixed << std::setprecision(1) << wbv[i].get_value();
				os << oss.str();
			}
			os << " & ";
		}
		if (wel.get_flags() & WeightBalance::Element::flag_consumable &&
		    !(wel.get_flags() & WeightBalance::Element::flag_fuel) &&
		    mass > wel.get_massmin()) {
			consumableindex[i] = consumables.size();
			consumables.push_back(massmoments_t());
			consumables.back().push_back(massmoment_t(0, 0));
			double moment(wel.get_moment(mass));
			double massmin(wel.get_massmin());
			double mass1(mass);
			for (;;) {
				std::pair<double,double> lim(wel.get_piecelimits(mass1, true));
				if (std::isnan(lim.first) || lim.first <= massmin) {
					consumables.back().push_back(massmoment_t(massmin - mass, wel.get_moment(massmin) - moment));
					break;
				}
				consumables.back().push_back(massmoment_t(lim.first - mass, wel.get_moment(lim.first) - moment));
				if (lim.first >= mass1)
					break;
				mass1 = lim.first;
			}
		}
		{
			std::string unit;
			if (i < wbv.size() && wbv[i].get_unit() < wel.get_nrunits())
				unit = wel.get_unit(wbv[i].get_unit()).get_name();
			else if (wel.get_nrunits())
				unit = wel.get_unit(0).get_name();
			os << unit;
		}
		os << " & ";
		{
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(2) << wel.get_arm(mass);
			os << oss.str();
		}
		os << " & ";
		{
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(1) << mass;
			if (mass < wel.get_massmin() || mass > wel.get_massmax())
				os << "{\\bf " << oss.str() << '}';
			else
				os << oss.str();
		}
		os << " & ";
		{
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(1) << wel.get_massmin();
			os << oss.str();
		}
		os << " & ";
		{
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(1) << wel.get_massmax();
			os << oss.str();
		}
		os << " & ";
		{
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(1) << wel.get_moment(mass);
			os << oss.str();
		}
		os << " \\\\\n  \\hline\n";
		if (wel.get_flags() & WeightBalance::Element::flag_fuel) {
			moments[mass_zf] += wel.get_moment(wel.get_massmin());
			masses[mass_zf] += wel.get_massmin();
		} else if (i < wbv.size()) {
			moments[mass_zf] += wel.get_moment(mass);
			masses[mass_zf] += mass;
		}
		if (i < wbv.size()) {
			moments[mass_ramp] += wel.get_moment(mass);
			masses[mass_ramp] += mass;
		}
	}
	os << "  \\hline\n";
	// compute maximum masses
	typedef enum {
		maxmass_zf,
		maxmass_ldg,
		maxmass_to,
		maxmass_ramp,
		maxmass_size
	} maxmass_t;
	double maxmasses[maxmass_size] = { get_mzfm(), get_mlm(), get_mtom(), get_mrm() };
	for (maxmass_t m(maxmass_ldg); m <= maxmass_ramp; m = (maxmass_t)(m + 1))
		if (std::isnan(maxmasses[m]))
			maxmasses[m] = maxmasses[m - 1];
	for (maxmass_t m(maxmass_ramp); m >= maxmass_ldg; m = (maxmass_t)(m - 1))
		if (std::isnan(maxmasses[m - 1]))
			maxmasses[m - 1] = maxmasses[m];
	// compute fuel consumed masses & moments
	typedef std::pair<double,double> massarm_t;
	typedef std::pair<massarm_t,unsigned int> graphpt_t;
	typedef std::vector<graphpt_t> graph_t;
	graph_t graph;
	graph.push_back(graphpt_t(massarm_t(masses[mass_ramp], moments[mass_ramp] / masses[mass_ramp]), 8 | mass_ramp));
	{
		WeightBalance::elementvalues_t wbv1(wbv);
		for (mass_t m = mass_ramp; m > mass_zf; ) {
			double mmoment(moments[m]), mmass(masses[m]);
			m = (mass_t)(m - 1);
			double loosefuel(std::numeric_limits<double>::quiet_NaN());
			switch (m) {
			case mass_to:
				loosefuel = fc.get_taxifuel();
				break;

			case mass_ldg:
				loosefuel = fc.get_tripfuel();
				break;

			case mass_alt:
				loosefuel = fc.get_altnfuel();
				break;

			case mass_althld:
				loosefuel = fc.get_holdfuel();
				break;

			case mass_zf:
				loosefuel = std::numeric_limits<double>::max();
				break;

			default:
				break;
			}
			if (!std::isnan(loosefuel) && loosefuel > 0) {
				for (WeightBalance::elementvalues_t::size_type i(wbv1.size()); i > 0; ) {
					--i;
					const WeightBalance::Element& wel(wb.get_element(i));
					if (!(wel.get_flags() & WeightBalance::Element::flag_fuel))
						continue;
					if (wbv1[i].get_unit() >= wel.get_nrunits())
						continue;
					const WeightBalance::Element::Unit& u(wel.get_unit(wbv1[i].get_unit()));
					double mass(wbv1[i].get_value() * u.get_factor() + u.get_offset());
					if (mass <= wel.get_massmin())
						continue;
					double massdiff(mass - wel.get_massmin());
					double fueldiff(convert(wb.get_massunit(), get_fuelunit(), massdiff, wel.get_flags()));
					if (fueldiff > loosefuel) {
						fueldiff = loosefuel;
						massdiff = convert(get_fuelunit(), wb.get_massunit(), fueldiff, wel.get_flags());
					}
					loosefuel -= fueldiff;
				        for (;;) {
						if (std::isnan(massdiff))
							break;
						std::pair<double,double> lim(wel.get_piecelimits(mass, true));
						double massdiff1(mass - lim.first);
						if (std::isnan(massdiff1)) {
							massdiff1 = mass;
							if (std::isnan(massdiff1))
								break;
						}
						if (false)
							std::cerr << "mass " << mass << " lim " << lim.first << ',' << lim.second
								  << " massdiff " << massdiff << " massdiff1 " << massdiff1
								  << " arm " << wel.get_arm(mass) << " name " << wel.get_name() << std::endl;
						if (massdiff1 > massdiff)
							massdiff1 = massdiff;
						mmass -= massdiff1;
						mmoment -= wel.get_moment(mass);
						mass -= massdiff1;
						mmoment += wel.get_moment(mass);
						massdiff -= massdiff1;
						if (massdiff <= 0)
							break;
						graph.push_back(graphpt_t(massarm_t(mmass, mmass > 0 ? mmoment / mmass : std::numeric_limits<double>::quiet_NaN()), m));
					}
					wbv1[i].set_value((mass - u.get_offset()) / u.get_factor());
					if (loosefuel <= 0)
						break;
					graph.push_back(graphpt_t(massarm_t(mmass, mmass > 0 ? mmoment / mmass : std::numeric_limits<double>::quiet_NaN()), m));
				}
			}
			graph.push_back(graphpt_t(massarm_t(mmass, mmass > 0 ? mmoment / mmass : std::numeric_limits<double>::quiet_NaN()), 8 | m));
			if (m != mass_zf) {
				moments[m] = mmoment;
				masses[m] = mmass;
			}
		}
	}
	static const maxmass_t masslimits[] = {
		maxmass_zf,   // mass_zf
		maxmass_ldg,  // mass_althld
		maxmass_ldg,  // mass_alt
		maxmass_ldg,  // mass_ldg
		maxmass_to,   // mass_to
		maxmass_ramp  // mass_ramp
	};
	static const char * const massnames[] = {
		"Zero Fuel",  // mass_zf
		"Hold",       // mass_althld
		"Alternate",  // mass_alt
		"Landing",    // mass_ldg
		"Take Off",   // mass_to
		"Ramp"        // mass_ramp
	};
	{
		unsigned int i(wb.get_nrelements());
		for (mass_t m(mass_zf); m <= mass_ramp; m = (mass_t)(m + 1)) {
			os << "  ";
			double mass(masses[m]);
			double moment(moments[m]);
			double arm(moment / mass);
			double masslim(maxmasses[masslimits[m]]);
			bool overmass(!std::isnan(masslim) && mass > masslim * 1.000001);
			bool outsideenv(!!wb.get_nrenvelopes());
			for (unsigned int j = 0; j < wb.get_nrenvelopes(); ++j) {
				const WeightBalance::Envelope& env(wb.get_envelope(j));
				if (!env.is_inside(WeightBalance::Envelope::Point(arm, mass)))
					continue;
				outsideenv = false;
				break;
			}
			if (overmass || outsideenv)
				os << "\\rowcolor[rgb]{1.0,0.5,0.5}";
			else if (i & 1)
				os << "\\rowcolor[gray]{0.9}";
			os << massnames[m] << " & & & ";
			if (!std::isnan(arm)) {
				std::ostringstream oss;
				oss << std::fixed << std::setprecision(2) << arm;
				if (outsideenv)
					os << "{\\bf " << oss.str() << '}';
				else
					os << oss.str();
			}
			os << " & ";
			if (!std::isnan(mass)) {
				std::ostringstream oss;
				oss << std::fixed << std::setprecision(1) << mass;
				if (overmass || outsideenv)
					os << "{\\bf " << oss.str() << '}';
				else
					os << oss.str();
			}
			os << " & & ";
			if (!std::isnan(masslim)) {
				std::ostringstream oss;
				oss << std::fixed << std::setprecision(1) << masslim;
				if (overmass)
					os << "{\\bf " << oss.str() << '}';
				else
					os << oss.str();
			}
			os << " & ";
			if (!std::isnan(moment)) {
				std::ostringstream oss;
				oss << std::fixed << std::setprecision(1) << moment;
				if (outsideenv)
					os << "{\\bf " << oss.str() << '}';
				else
					os << oss.str();
			}
			os << "\\\\\n  \\hline\n";
			++i;
		}
	}
	os << table_footer;
	if (true && consumables.size()) {
		for (unsigned int ci = 1, cn = std::min((unsigned int)consumables.size(), 4U), cm = 1 << cn; ci < cm; ++ci) {
			double masscorr(0), momentcorr(0);
			os << table_header;
			for (unsigned int i = 0; i < wb.get_nrelements(); ++i) {
				const WeightBalance::Element& wel(wb.get_element(i));
				if (i < wbv.size()) {
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(1) << wbv[i].get_value();
					os << "  % station " << i << " unit " << wbv[i].get_unit() << " value " << oss.str() << "\n";
				}
				os << "  ";
				if (i & 1)
					os << "\\rowcolor[gray]{0.9}";
				os << (std::string)METARTAFChart::latex_string_meta(wel.get_name()) << " & ";
				double mass(0);
				if (wel.get_flags() & WeightBalance::Element::flag_fixed) {
					if (i < wbv.size() && wbv[i].get_unit() < wel.get_nrunits()) {
						const WeightBalance::Element::Unit& u(wel.get_unit(wbv[i].get_unit()));
						mass = std::min(std::max(wbv[i].get_value() * u.get_factor() + u.get_offset(), wel.get_massmin()), wel.get_massmax());

					} else {
						mass = wel.get_massmin();
					}
					os << " & ";
				} else {
					if (i < wbv.size()) {
						if (wel.get_flags() & WeightBalance::Element::flag_binary)
							wbv[i].set_value(0);
						double value(wbv[i].get_value());
						if (wbv[i].get_unit() < wel.get_nrunits()) {
							const WeightBalance::Element::Unit& u(wel.get_unit(wbv[i].get_unit()));
							mass = value * u.get_factor() + u.get_offset();
							if (ci & (1 << consumableindex[i])) {
								mass += consumables[consumableindex[i]].back().first;
								if (u.get_factor() > 0)
									value = (mass - u.get_offset()) / u.get_factor();
								else
									value = 0;
							}
						}
						std::ostringstream oss;
						oss << std::fixed << std::setprecision(1) << value;
						os << oss.str();
					}
					os << " & ";
				}
				{
					std::string unit;
					if (i < wbv.size() && wbv[i].get_unit() < wel.get_nrunits())
						unit = wel.get_unit(wbv[i].get_unit()).get_name();
					else if (wel.get_nrunits())
						unit = wel.get_unit(0).get_name();
					os << unit;
				}
				os << " & ";
				{
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(2) << wel.get_arm(mass);
					os << oss.str();
				}
				os << " & ";
				{
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(1) << mass;
					if (mass < wel.get_massmin() || mass > wel.get_massmax())
						os << "{\\bf " << oss.str() << '}';
					else
						os << oss.str();
				}
				os << " & ";
				{
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(1) << wel.get_massmin();
					os << oss.str();
				}
				os << " & ";
				{
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(1) << wel.get_massmax();
					os << oss.str();
				}
				os << " & ";
				{
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(1) << wel.get_moment(mass);
					os << oss.str();
				}
				os << " \\\\\n  \\hline\n";
				if (ci & (1 << consumableindex[i])) {
					const massarm_t& ma(consumables[consumableindex[i]].back());
					masscorr += ma.first;
					momentcorr += ma.second;
				}
        		}
			os << "  \\hline\n";
			unsigned int i(wb.get_nrelements());
			for (mass_t m(mass_zf); m <= mass_ramp; m = (mass_t)(m + 1)) {
				os << "  ";
				double mass(masses[m] + masscorr);
				double moment(moments[m] + momentcorr);
				double arm(moment / mass);
				double masslim(maxmasses[masslimits[m]]);
				bool overmass(!std::isnan(masslim) && mass > masslim * 1.000001);
				bool outsideenv(!!wb.get_nrenvelopes());
				for (unsigned int j = 0; j < wb.get_nrenvelopes(); ++j) {
					const WeightBalance::Envelope& env(wb.get_envelope(j));
					if (!env.is_inside(WeightBalance::Envelope::Point(arm, mass)))
						continue;
					outsideenv = false;
					break;
				}
				if (overmass || outsideenv)
					os << "\\rowcolor[rgb]{1.0,0.5,0.5}";
				else if (i & 1)
					os << "\\rowcolor[gray]{0.9}";
				os << massnames[m] << " & & & ";
				if (!std::isnan(arm)) {
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(2) << arm;
					if (outsideenv)
						os << "{\\bf " << oss.str() << '}';
					else
						os << oss.str();
				}
				os << " & ";
				if (!std::isnan(mass)) {
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(1) << mass;
					if (overmass || outsideenv)
						os << "{\\bf " << oss.str() << '}';
					else
						os << oss.str();
				}
				os << " & & ";
				if (!std::isnan(masslim)) {
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(1) << masslim;
					if (overmass)
						os << "{\\bf " << oss.str() << '}';
					else
						os << oss.str();
				}
				os << " & ";
				if (!std::isnan(moment)) {
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(1) << moment;
					if (outsideenv)
						os << "{\\bf " << oss.str() << '}';
					else
						os << oss.str();
				}
				os << "\\\\\n  \\hline\n";
				++i;
			}
			os << table_footer;
		}
	}
	// Mass&Balance Graph
	double massmin(std::numeric_limits<double>::max()), massmax(std::numeric_limits<double>::min());
	double armmin(std::numeric_limits<double>::max()), armmax(std::numeric_limits<double>::min());
	bool envok(false);
	for (unsigned int i = 0; i < wb.get_nrenvelopes(); ++i) {
		const WeightBalance::Envelope& env(wb.get_envelope(i));
		envok = envok || (env.size() >= 3);
		for (unsigned int j = 0; j < env.size(); ++j) {
			const WeightBalance::Envelope::Point& pt(env[j]);
			massmin = std::min(massmin, pt.get_mass());
			massmax = std::max(massmax, pt.get_mass());
			armmin = std::min(armmin, pt.get_arm());
			armmax = std::max(armmax, pt.get_arm());
		}
	}
	envok = envok && !std::isnan(armmin) && !std::isnan(armmax) && !std::isnan(massmin) && !std::isnan(massmax) &&
		armmin < armmax && massmin < massmax;
	for (unsigned int ci = 0, cn = std::min((unsigned int)consumables.size(), 4U), cm = 1 << cn; ci < cm; ++ci) {
		double masscorr(0), momentcorr(0);
		for (unsigned int cj = 0; cj < cn; ++cj) {
			if (ci & (1U << cj)) {
				const massarm_t& ma(consumables[cj].back());
				masscorr += ma.first;
				momentcorr += ma.second;
			}
		}
		for (graph_t::const_iterator gi(graph.begin()), ge(graph.end()); gi != ge; ++gi) {
			double mass(gi->first.first), arm(gi->first.second);
			if (ci) {
				double moment(mass * arm);
				mass += masscorr;
				moment += momentcorr;
				if (fabs(mass) < 1e-6)
					continue;
				arm = moment / mass;
			}
			massmin = std::min(massmin, mass);
			massmax = std::max(massmax, mass);
			armmin = std::min(armmin, arm);
			armmax = std::max(armmax, arm);
		}
	}
	if (!std::isnan(armmin) && !std::isnan(armmax) && !std::isnan(massmin) && !std::isnan(massmax) &&
	    armmin < armmax && massmin < massmax && envok) {
		double scalearm(1);
		{
			double d(armmax - armmin);
			for (unsigned int i(0); i < 15; ++i) {
				double s(((i % 3) == 1) ? 0.4 : 0.5);
				d *= s;
				if (d < 5)
					break;
				scalearm *= s;
			}
		}
		double scalemass(1);
		{
			double d(massmax - massmin);
			for (unsigned int i(0); i < 15; ++i) {
				double s(((i % 3) == 1) ? 0.4 : 0.5);
				d *= s;
				if (d < 5)
					break;
				scalemass *= s;
			}
		}
		int xmin(std::floor((armmin - 0.05 * (armmax - armmin)) * scalearm));
		int xmax(std::ceil((armmax + 0.05 * (armmax - armmin)) * scalearm));
		int ymin(std::floor((massmin - 0.05 * (massmax - massmin)) * scalemass));
		int ymax(std::ceil((massmax + 0.05 * (massmax - massmin)) * scalemass));
		{
			std::ostringstream oss;
			oss << std::fixed << (17.0 / std::max(xmax - xmin, ymax - ymin));
			os << "\n\\begin{center}\\begin{tikzpicture}[scale=" << oss.str() << "]\n";
		}
		if (false) {
			std::ostringstream oss;
			oss << std::fixed << "  \\path[clip] (" << ((armmin - 0.1 * (armmax - armmin)) * scalearm - xmin)
			    << ',' << ((massmin - 0.1 * (massmax - massmin)) * scalemass - ymin) << ") rectangle ("
			    << ((armmax + 0.1 * (armmax - armmin)) * scalearm - xmin) << ','
			    << ((massmax + 0.1 * (massmax - massmin)) * scalemass - ymin) << ");\n";
			os << oss.str();
		}
		os << "  \\draw[step=1,help lines] (0,0) grid (" << (xmax - xmin) << ',' << (ymax - ymin) << ");\n";
		for (int x(xmin); x < xmax; ++x)
			os << "  \\path[draw,thin,dotted] (" << (x - xmin) << ".5,0) -- (" << (x - xmin) << ".5," << (ymax - ymin) << ");\n";
		for (int y(ymin); y < ymax; ++y)
			os << "  \\path[draw,thin,dotted] (0," << (y - ymin) << ".5) -- (" << (xmax - xmin) << ',' << (y - ymin) << ".5);\n";
		for (int x(xmin); x <= xmax; ++x) {
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(scalearm > 1) << (x / scalearm);
			os << "  \\path[draw] " << METARTAFChart::pgfcoord(x - xmin, +0.1) << " -- " << METARTAFChart::pgfcoord(x - xmin, -0.1)
			   << " node[anchor=north] {" << oss.str() << "};\n";
		}
		for (int y(ymin); y <= ymax; ++y) {
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(scalemass > 1) << (y / scalemass);
			os << "  \\path[draw] " << METARTAFChart::pgfcoord(+0.1, y - ymin) << " -- " << METARTAFChart::pgfcoord(-0.1, y - ymin)
			   << " node[anchor=east] {" << oss.str() << "};\n";
		}
		os << "  \\node[anchor=north] at " << METARTAFChart::pgfcoord(xmax - 0.5 - xmin, -0.1) << " {Arm};\n"
		   << "  \\node[anchor=east] at " << METARTAFChart::pgfcoord(-0.1, ymax - 0.5 - ymin) << " {Mass};\n";
		for (unsigned int i = 0; i < wb.get_nrenvelopes(); ++i) {
			const WeightBalance::Envelope& env(wb.get_envelope(i));
			if (!env.size())
				continue;
			for (unsigned int j = 0; j < env.size(); ++j) {
				const WeightBalance::Envelope::Point& pt(env[j]);
				if (!j)
					os << "  \\path[draw,thick," << ((i & 1) ? "green" : "blue") << "]";
				else
					os << " --";
				os << ' ' << METARTAFChart::pgfcoord(pt.get_arm() * scalearm - xmin, pt.get_mass() * scalemass - ymin);
			}
			os << " -- cycle;\n";
		}
		for (unsigned int ci = 0, cn = std::min((unsigned int)consumables.size(), 4U), cm = 1 << cn; ci < cm; ++ci) {
			double masscorr(0), momentcorr(0);
			for (unsigned int cj = 0; cj < cn; ++cj) {
				if (ci & (1U << cj)) {
					const massarm_t& ma(consumables[cj].back());
					masscorr += ma.first;
					momentcorr += ma.second;
				}
			}
			bool first(true);
			unsigned int dashmode(0);
			for (graph_t::size_type i(0), n(graph.size()); i < n; ++i) {
				if (std::isnan(graph[i].first.first) ||
				    std::isnan(graph[i].first.second))
					break;
				unsigned int newdashmode(1);
				if ((mass_t)(graph[i].second & 7) <= mass_zf)
					newdashmode = 3;
				else if ((mass_t)(graph[i].second & 7) <= mass_alt)
					newdashmode = 2;
				if (newdashmode != dashmode) {
					if (dashmode)
						os << ";\n";
					dashmode = newdashmode;
					os << "  \\path[draw,thick,red" << ((dashmode == 2) ? ",dashed" : (dashmode == 3) ? ",dotted" : "") << ']';
					if (i > 0) {
						double arm(graph[i-1].first.second), mass(graph[i-1].first.first);
						if (ci) {
							double moment(arm * mass);
							mass += masscorr;
							moment += momentcorr;
							arm = moment / mass;
						}
						os << ' ' << METARTAFChart::pgfcoord(arm * scalearm - xmin, mass * scalemass - ymin) << " --";
					}
				} else {
					os << " --";
				}
				{
					double arm(graph[i].first.second), mass(graph[i].first.first);
					if (ci) {
						double moment(arm * mass);
						mass += masscorr;
						moment += momentcorr;
						arm = moment / mass;
					}
					os << ' ' << METARTAFChart::pgfcoord(arm * scalearm - xmin, mass * scalemass - ymin);
				}
				if (graph[i].second & 8)
					os << " node [fill=red,inner sep=2pt]{}";
			}
			if (dashmode)
				os << ";\n";
		}
		if (!consumables.empty()) {
			unsigned int cn = std::min((unsigned int)consumables.size(), 4U);
			for (graph_t::size_type i(0), n(graph.size()); i < n; ++i) {
				double mass(graph[i].first.first), moment(mass * graph[i].first.second);
				for (unsigned int ci = 0; ci < cn; ++ci) {
					const massmoments_t& mm(consumables[ci]);
					if (mm.size() < 2)
						continue;
					os << "  \\path[draw,thin,red,dotted]";
					for (unsigned int mi(0), mn(mm.size()); mi < mn; ++mi) {
						double mass1(mass + mm[mi].first), moment1(moment + mm[mi].second);
						double arm1(moment1 / mass1);
						os << (mi ? " -- " : " ") << METARTAFChart::pgfcoord(arm1 * scalearm - xmin, mass1 * scalemass - ymin);
					}
					os << ";\n";
				}
			}
		}
		os << "\\end{tikzpicture}\\end{center}\n\n";
	}
	os << "\\fi\n\n";
	// fuel planning
	{
		os << "\\ifneedsfuelplan\n"
		   << "\\subsection{Fuel Planning}\n\n"
		   << "\\newsavebox{\\fueltable}\\savebox{\\fueltable}{\\begin{tabular}{|l|r|r|r|}\n"
		   << "  \\hline\n"
		   << "  \\rowcolor[gray]{0.8}Name & Fuelflow & Amount & Fuel \\\\\n"
		   << "  \\hline\n";
		bool grayinv(true);
		unsigned int prec(fuelunit == unit_usgal);
		if (!std::isnan(fc.get_taxifuel())) {
			os << "  Taxi Fuel & ";
			std::ostringstream oss;
			if (!std::isnan(fc.get_taxifuelflow()))
				oss << std::fixed << std::setprecision(prec) << convert_fuel(get_fuelunit(), fuelunit, fc.get_taxifuelflow());
			oss << " & ";
			if (fc.get_taxitime())
				oss << std::setw(2) << std::setfill('0') << (fc.get_taxitime() / 3600)
				    << ':' << std::setw(2) << std::setfill('0') << ((fc.get_taxitime() / 60) % 60);
			oss << " & ";
			oss << std::fixed << std::setprecision(prec) << convert_fuel(get_fuelunit(), fuelunit, fc.get_taxifuel());
			os << oss.str() << " \\\\\n  \\hline\n";
			grayinv = false;
		}
		if (!std::isnan(fc.get_tripfuel())) {
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(prec) << convert_fuel(get_fuelunit(), fuelunit, fc.get_tripfuel());
			os << "  " << (grayinv ? "" : "\\rowcolor[gray]{0.9}") << "Trip Fuel & & & " << oss.str() << " \\\\\n  \\hline\n";
			grayinv = !grayinv;
		}
		if (!std::isnan(fc.get_contfuel())) {
			std::ostringstream oss2, oss3, oss4;
			if (!std::isnan(fc.get_contfuelflow()) && fc.get_contfuelflow() > 0) {
				time_t ct(fc.get_contfuel() / fc.get_contfuelflow() * 3600.0);
				oss3 << std::setw(2) << std::setfill('0') << (ct / 3600)
				     << ':' << std::setw(2) << std::setfill('0') << ((ct / 60) % 60);;
				oss4 << std::fixed << std::setprecision(prec) << convert_fuel(get_fuelunit(), fuelunit, fc.get_contfuelflow());
			} else if (!std::isnan(fc.get_contfuelpercent())) {
				oss3 << std::fixed << std::setprecision(0) << fc.get_contfuelpercent() << "\\%";
			}
			oss2 << std::fixed << std::setprecision(prec) << convert_fuel(get_fuelunit(), fuelunit, fc.get_contfuel());
			os << "  " << (grayinv ? "" : "\\rowcolor[gray]{0.9}") << "Contingency & " << oss4.str()
			   << " & " << oss3.str() << " & " << oss2.str() << " \\\\\n  \\hline\n";
			grayinv = !grayinv;
		}
		if (!std::isnan(fc.get_altnfuel())) {
			os << "  " << (grayinv ? "" : "\\rowcolor[gray]{0.9}") << "Alternate Fuel";
			grayinv = !grayinv;
			if (fc.is_altnalt_valid()) {
				std::ostringstream oss;
				oss << " F" << std::setw(3) << std::setfill('0') << ((fc.get_altnalt() + 50) / 100);
				os << oss.str();
			}
        		std::ostringstream oss;
			oss << std::fixed << std::setprecision(prec) << convert_fuel(get_fuelunit(), fuelunit, fc.get_altnfuel());
			os << " & & & " << oss.str() << " \\\\\n  \\hline\n";
		}
		if (!std::isnan(fc.get_holdfuel())) {
			os << "  " << (grayinv ? "" : "\\rowcolor[gray]{0.9}") << "Final Reserve";
			grayinv = !grayinv;
			if (fc.is_holdalt_valid()) {
				std::ostringstream oss;
				oss << " F" << std::setw(3) << std::setfill('0') << ((fc.get_holdalt() + 50) / 100);
				os << oss.str();
			}
			std::ostringstream oss;
			if (!std::isnan(fc.get_holdfuelflow()))
				oss << std::fixed << std::setprecision(prec) << convert_fuel(get_fuelunit(), fuelunit, fc.get_holdfuelflow());
			oss << " & " << std::setw(2) << std::setfill('0') << (fc.get_holdtime() / 3600)
			    << ':' << std::setw(2) << std::setfill('0') << ((fc.get_holdtime() / 60) % 60)
			    << " & " << std::fixed << std::setprecision(prec) << convert_fuel(get_fuelunit(), fuelunit, fc.get_holdfuel());
			os << " & " << oss.str() << " \\\\\n  \\hline\n";
		}
		os << "  " << (grayinv ? "" : "\\rowcolor[gray]{0.9}") << "Required Fuel & & & ";
		{
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(prec) << convert_fuel(get_fuelunit(), fuelunit, fc.get_reqdfuel());
			os << oss.str() << ' ';
		}
		double loadedfuel(0);
		for (WeightBalance::elementvalues_t::size_type i(wbv.size()); i > 0; ) {
			--i;
			const WeightBalance::Element& wel(wb.get_element(i));
			if (!(wel.get_flags() & WeightBalance::Element::flag_fuel))
				continue;
			if (wbv[i].get_unit() >= wel.get_nrunits())
				continue;
			const WeightBalance::Element::Unit& u(wel.get_unit(wbv[i].get_unit()));
			double mass(wbv[i].get_value() * u.get_factor() + u.get_offset());
			if (mass < wel.get_massmin())
				continue;
			loadedfuel += convert(wb.get_massunit(), get_fuelunit(), mass - wel.get_massmin(), wel.get_flags());
		}
		os << "\\\\\n  \\hline\n  " << (grayinv ? "\\rowcolor[gray]{0.9}" : "") << "Extra Fuel & & & ";
		{
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(prec) << convert_fuel(get_fuelunit(), fuelunit, loadedfuel - fc.get_reqdfuel());
			os << oss.str() << ' ';
		}
		os << "\\\\\n  \\hline\n  " << (grayinv ? "" : "\\rowcolor[gray]{0.9}") << "Block Fuel & & & ";
		{
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(prec) << convert_fuel(get_fuelunit(), fuelunit, loadedfuel);
			os << oss.str() << ' ';
		}
	}
	os << "\\\\\n  \\hline\n\\end{tabular}}\n\\newsavebox{\\altntable}\\savebox{\\altntable}{";
	if (!fc.is_noaltn() && !altn.empty()) {
		int mode(-1);
		for (std::vector<FPlanAlternate>::const_iterator ai(altn.begin()), ae(altn.end()); ai != ae; ++ai) {
			if (ai->get_coord().is_invalid())
				continue;
			if (mode < 0) {
				os << "\\begin{tabular}{|l|r|r|r|r|}\n"
				   << "  \\hline\n"
				   << "  \\rowcolor[gray]{0.8}Destination Alternate & Dist & Time & Fuel & Hold FF \\\\\n"
				   << "  \\hline\n";
				mode = 0;
			}
			os << "  ";
			if (mode)
				os << "\\rowcolor[gray]{0.9}";
			mode = !mode;
			os << "\\truncate{4.5cm}{" << METARTAFChart::latex_string_meta(ai->get_icao_name()) << "} & ";
			{
				std::ostringstream oss;
				oss << std::fixed << std::setprecision(1) << ai->get_dist_nmi();
				os << oss.str();
			}
			os << " & ";
			{
				time_t t(ai->get_flighttime());
				if (fplan.get_nrwpt() >= 2)
					t -= fplan[fplan.get_nrwpt()-1].get_flighttime();
				if (t < 0)
					t = 0;
				t /= 60;
				std::ostringstream oss;
				oss << std::setw(2) << std::setfill('0') << (t / 60) << ':'
				    << std::setw(2) << std::setfill('0') << (t % 60);
				os << oss.str();
			}
			os << " & ";
			{
				double f(ai->get_fuel_usg());
				if (fplan.get_nrwpt() >= 2)
					f -= fplan[fplan.get_nrwpt()-1].get_fuel_usg();
				if (f < 0)
					f = 0;
				std::ostringstream oss;
				oss << std::fixed << std::setprecision(1) << convert_fuel(get_fuelunit(), fuelunit, f);
				os << oss.str();
			}
			os << " & ";
			if (ai->get_holdtime() > 0) {
				std::ostringstream oss;
				oss << std::fixed << std::setprecision(1) << convert_fuel(get_fuelunit(), fuelunit, ai->get_holdfuel_usg() / ai->get_holdtime() * 3600);
				os << oss.str();
			}
			os << "\\\\\n  \\hline\n";
		}
		if (mode >= 0)
			os << "\\end{tabular}";
	}
	os << "}\n"
	   << "\\newlength{\\fueltablewidth}\n"
	   << "\\settowidth{\\fueltablewidth}{\\usebox{\\fueltable}}\n"
	   << "\\newlength{\\fueltableheight}\n"
	   << "\\settoheight{\\fueltableheight}{\\usebox{\\fueltable}}\n"
	   << "\\newlength{\\altntablewidth}\n"
	   << "\\settowidth{\\altntablewidth}{\\usebox{\\altntable}}\n"
	   << "\\noindent\\begin{minipage}[t]{\\fueltablewidth}\\vspace{0pt}\\usebox{\\fueltable}\\end{minipage}\\hfill%\n"
	   << "\\begin{minipage}[t]{\\altntablewidth}\\vspace{0pt}\\usebox{\\altntable}\\end{minipage}\n"
	   << "\\fi\n";
	if (tomass)
		*tomass = masses[mass_to];
	return os;
}

std::ostream& Aircraft::massbalance_lualatex(std::ostream& os, const FPlanRoute& fplan,
					     const std::vector<FPlanAlternate>& altn,
					     const Cruise::EngineParams& epcruise,
					     double fuel_on_board, const WeightBalance::elementvalues_t& wbev,
					     unit_t fuelunit, unit_t massunit, double *tomass) const
{
	if (fuelunit == unit_invalid)
		fuelunit = get_fuelunit();
	if (massunit == unit_invalid)
		massunit = get_wb().get_massunit();
	WeightBalance::elementvalues_t wbv(prepare_wb(wbev, fuel_on_board, fuelunit, massunit, unit_invalid));
	const WeightBalance& wb(get_wb());
	// convert fuel stations to preferred fuel units
	FPlanRoute::FuelCalc fc(fplan.calculate_fuel(*this, altn));
	bool noaltn(fc.is_noaltn());
	std::vector<FPlanAlternate>::size_type altnidx(fc.get_altnidx());
	if (noaltn)
		altnidx = 0;
	os << "  massbalance = {" << std::endl
	   << "    fuelunit = " << to_luastring(to_str(fuelunit, true)) << ',' << std::endl
	   << "    massunit = " << to_luastring(to_str(massunit, true)) << ',' << std::endl
	   << "    stations = {" << std::endl;
	typedef enum {
		mass_zf,
		mass_althld,
		mass_alt,
		mass_ldg,
		mass_to,
		mass_ramp,
		mass_size
	} mass_t;
	double moments[mass_size], masses[mass_size];
	masses[mass_ramp] = masses[mass_zf] = 0;
	moments[mass_ramp] = moments[mass_zf] = 0;
	typedef std::pair<double,double> massmoment_t;
	typedef std::vector<massmoment_t> massmoments_t;
	typedef std::vector<massmoments_t> consumables_t;
	consumables_t consumables;
	typedef std::vector<unsigned int> consumableindex_t;
	consumableindex_t consumableindex;
	for (unsigned int i = 0; i < wb.get_nrelements(); ++i) {
		consumableindex.push_back(~0U);
		const WeightBalance::Element& wel(wb.get_element(i));
		if (i)
			os << ',' << std::endl;
		os << "      { name = " << to_luastring(wel.get_name());
		double mass(0);
		if (wel.get_flags() & WeightBalance::Element::flag_fixed) {
			if (i < wbv.size() && wbv[i].get_unit() < wel.get_nrunits()) {
				const WeightBalance::Element::Unit& u(wel.get_unit(wbv[i].get_unit()));
				mass = std::min(std::max(wbv[i].get_value() * u.get_factor() + u.get_offset(), wel.get_massmin()), wel.get_massmax());

			} else {
				mass = wel.get_massmin();
			}
		} else {
			if (i < wbv.size()) {
				if (wel.get_flags() & WeightBalance::Element::flag_binary)
					wbv[i].set_value(0);
				if (wbv[i].get_unit() < wel.get_nrunits()) {
					const WeightBalance::Element::Unit& u(wel.get_unit(wbv[i].get_unit()));
					mass = wbv[i].get_value() * u.get_factor() + u.get_offset();
				}
				os << ", massraw = " << wbv[i].get_value();
			}
		}
		os << ", mass = " << mass << ", massmin = " << wel.get_massmin() << ", massmax = " << wel.get_massmax();
		if (wel.get_flags() & WeightBalance::Element::flag_consumable &&
		    !(wel.get_flags() & WeightBalance::Element::flag_fuel) &&
		    mass > wel.get_massmin()) {
			consumableindex[i] = consumables.size();
			consumables.push_back(massmoments_t());
			consumables.back().push_back(massmoment_t(0, 0));
			double moment(wel.get_moment(mass));
			double massmin(wel.get_massmin());
			double mass1(mass);
			for (;;) {
				std::pair<double,double> lim(wel.get_piecelimits(mass1, true));
				if (std::isnan(lim.first) || lim.first <= massmin) {
					consumables.back().push_back(massmoment_t(massmin - mass, wel.get_moment(massmin) - moment));
					break;
				}
				consumables.back().push_back(massmoment_t(lim.first - mass, wel.get_moment(lim.first) - moment));
				if (lim.first >= mass1)
					break;
				mass1 = lim.first;
			}
		}
		{
			std::string unit;
			if (i < wbv.size() && wbv[i].get_unit() < wel.get_nrunits())
				unit = wel.get_unit(wbv[i].get_unit()).get_name();
			else if (wel.get_nrunits())
				unit = wel.get_unit(0).get_name();
			os << ", unit = " << to_luastring(unit);
		}
		os << ", arm = " << wel.get_arm(mass)
		   << ", moment = " << wel.get_moment(mass)
		   << ", fixed = " << to_luastring(!!(wel.get_flags() & WeightBalance::Element::flag_fixed))
		   << ", binary = " << to_luastring(!!(wel.get_flags() & WeightBalance::Element::flag_binary))
		   << ", avgas = " << to_luastring(!!(wel.get_flags() & WeightBalance::Element::flag_avgas))
		   << ", jeta = " << to_luastring(!!(wel.get_flags() & WeightBalance::Element::flag_jeta))
		   << ", diesel = " << to_luastring(!!(wel.get_flags() & WeightBalance::Element::flag_diesel))
		   << ", oil = " << to_luastring(!!(wel.get_flags() & WeightBalance::Element::flag_oil))
		   << ", gear = " << to_luastring(!!(wel.get_flags() & WeightBalance::Element::flag_gear))
		   << ", consumable = " << to_luastring(!!(wel.get_flags() & WeightBalance::Element::flag_consumable))
		   << ", fuel = " << to_luastring(!!(wel.get_flags() & WeightBalance::Element::flag_fuel));
		if (wel.get_flags() & WeightBalance::Element::flag_fuel) {
			moments[mass_zf] += wel.get_moment(wel.get_massmin());
			masses[mass_zf] += wel.get_massmin();
		} else if (i < wbv.size()) {
			moments[mass_zf] += wel.get_moment(mass);
			masses[mass_zf] += mass;
		}
		if (i < wbv.size()) {
			moments[mass_ramp] += wel.get_moment(mass);
			masses[mass_ramp] += mass;
		}
		os << " }";
	}
	os << " }," << std::endl
	   << "    envelopes = {" << std::endl;
	for (unsigned int i(0); i < wb.get_nrenvelopes(); ++i) {
		const WeightBalance::Envelope& env(wb.get_envelope(i));
		if (i)
			os << ',' << std::endl;
		os << "      { name = " << to_luastring(env.get_name());
		for (unsigned int j(0); j < env.size(); ++j)
			os << ", { " << env[j].get_mass() << ',' << env[j].get_arm() << " }";
		os << " }";
	}
	os << " }," << std::endl;
	// compute maximum masses
	typedef enum {
		maxmass_zf,
		maxmass_ldg,
		maxmass_to,
		maxmass_ramp,
		maxmass_size
	} maxmass_t;
	double maxmasses[maxmass_size] = { get_mzfm(), get_mlm(), get_mtom(), get_mrm() };
	for (maxmass_t m(maxmass_ldg); m <= maxmass_ramp; m = (maxmass_t)(m + 1))
		if (std::isnan(maxmasses[m]))
			maxmasses[m] = maxmasses[m - 1];
	for (maxmass_t m(maxmass_ramp); m >= maxmass_ldg; m = (maxmass_t)(m - 1))
		if (std::isnan(maxmasses[m - 1]))
			maxmasses[m - 1] = maxmasses[m];
	// compute fuel consumed masses & moments
	typedef std::pair<double,double> massarm_t;
	typedef std::pair<massarm_t,unsigned int> graphpt_t;
	typedef std::vector<graphpt_t> graph_t;
	graph_t graph;
	graph.push_back(graphpt_t(massarm_t(masses[mass_ramp], moments[mass_ramp] / masses[mass_ramp]), 8 | mass_ramp));
	{
		WeightBalance::elementvalues_t wbv1(wbv);
		for (mass_t m = mass_ramp; m > mass_zf; ) {
			double mmoment(moments[m]), mmass(masses[m]);
			m = (mass_t)(m - 1);
			double loosefuel(std::numeric_limits<double>::quiet_NaN());
			switch (m) {
			case mass_to:
				loosefuel = fc.get_taxifuel();
				break;

			case mass_ldg:
				loosefuel = fc.get_tripfuel();
				break;

			case mass_alt:
				loosefuel = fc.get_altnfuel();
				break;

			case mass_althld:
				loosefuel = fc.get_holdfuel();
				break;

			case mass_zf:
				loosefuel = std::numeric_limits<double>::max();
				break;

			default:
				break;
			}
			if (!std::isnan(loosefuel) && loosefuel > 0) {
				for (WeightBalance::elementvalues_t::size_type i(wbv1.size()); i > 0; ) {
					--i;
					const WeightBalance::Element& wel(wb.get_element(i));
					if (!(wel.get_flags() & WeightBalance::Element::flag_fuel))
						continue;
					if (wbv1[i].get_unit() >= wel.get_nrunits())
						continue;
					const WeightBalance::Element::Unit& u(wel.get_unit(wbv1[i].get_unit()));
					double mass(wbv1[i].get_value() * u.get_factor() + u.get_offset());
					if (mass <= wel.get_massmin())
						continue;
					double massdiff(mass - wel.get_massmin());
					double fueldiff(convert(wb.get_massunit(), get_fuelunit(), massdiff, wel.get_flags()));
					if (fueldiff > loosefuel) {
						fueldiff = loosefuel;
						massdiff = convert(get_fuelunit(), wb.get_massunit(), fueldiff, wel.get_flags());
					}
					loosefuel -= fueldiff;
				        for (;;) {
						if (std::isnan(massdiff))
							break;
						std::pair<double,double> lim(wel.get_piecelimits(mass, true));
						double massdiff1(mass - lim.first);
						if (std::isnan(massdiff1)) {
							massdiff1 = mass;
							if (std::isnan(massdiff1))
								break;
						}
						if (false)
							std::cerr << "mass " << mass << " lim " << lim.first << ',' << lim.second
								  << " massdiff " << massdiff << " massdiff1 " << massdiff1
								  << " arm " << wel.get_arm(mass) << " name " << wel.get_name() << std::endl;
						if (massdiff1 > massdiff)
							massdiff1 = massdiff;
						mmass -= massdiff1;
						mmoment -= wel.get_moment(mass);
						mass -= massdiff1;
						mmoment += wel.get_moment(mass);
						massdiff -= massdiff1;
						if (massdiff <= 0)
							break;
						graph.push_back(graphpt_t(massarm_t(mmass, mmass > 0 ? mmoment / mmass : std::numeric_limits<double>::quiet_NaN()), m));
					}
					wbv1[i].set_value((mass - u.get_offset()) / u.get_factor());
					if (loosefuel <= 0)
						break;
					graph.push_back(graphpt_t(massarm_t(mmass, mmass > 0 ? mmoment / mmass : std::numeric_limits<double>::quiet_NaN()), m));
				}
			}
			graph.push_back(graphpt_t(massarm_t(mmass, mmass > 0 ? mmoment / mmass : std::numeric_limits<double>::quiet_NaN()), 8 | m));
			if (m != mass_zf) {
				moments[m] = mmoment;
				masses[m] = mmass;
			}
		}
	}
	static const maxmass_t masslimits[] = {
		maxmass_zf,   // mass_zf
		maxmass_ldg,  // mass_althld
		maxmass_ldg,  // mass_alt
		maxmass_ldg,  // mass_ldg
		maxmass_to,   // mass_to
		maxmass_ramp  // mass_ramp
	};
	static const char * const massnames[] = {
		"Zero Fuel",  // mass_zf
		"Hold",       // mass_althld
		"Alternate",  // mass_alt
		"Landing",    // mass_ldg
		"Take Off",   // mass_to
		"Ramp"        // mass_ramp
	};
	os << "    masses = {" << std::endl;
	for (mass_t m(mass_zf); m <= mass_ramp; m = (mass_t)(m + 1)) {
		if (m != mass_zf)
			os << ',' << std::endl;
		double mass(masses[m]);
		double moment(moments[m]);
		double arm(moment / mass);
		double masslim(maxmasses[masslimits[m]]);
		os << "      { name = " << to_luastring((std::string)massnames[m]);
		if (!std::isnan(mass))
			os << ", mass = " << mass;
		if (!std::isnan(moment))
			os << ", moment = " << moment;
		if (!std::isnan(arm))
			os << ", arm = " << arm;
		if (!std::isnan(masslim))
			os << ", masslim = " << masslim;
		bool overmass(!std::isnan(masslim) && mass > masslim * 1.000001);
		bool outsideenv(!!wb.get_nrenvelopes());
		for (unsigned int j = 0; j < wb.get_nrenvelopes(); ++j) {
			const WeightBalance::Envelope& env(wb.get_envelope(j));
			if (!env.is_inside(WeightBalance::Envelope::Point(arm, mass)))
				continue;
			outsideenv = false;
			break;
		}
		os << ", overmass = " << to_luastring(overmass)
		   << ", outsideenv = " << to_luastring(outsideenv)
		   << " }";
	}
	os << " }," << std::endl;
	// FIXME
	if (false && consumables.size()) {
		for (unsigned int ci = 1, cn = std::min((unsigned int)consumables.size(), 4U), cm = 1 << cn; ci < cm; ++ci) {
			double masscorr(0), momentcorr(0);
			for (unsigned int i = 0; i < wb.get_nrelements(); ++i) {
				const WeightBalance::Element& wel(wb.get_element(i));
				if (i < wbv.size()) {
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(1) << wbv[i].get_value();
					os << "  % station " << i << " unit " << wbv[i].get_unit() << " value " << oss.str() << "\n";
				}
				os << "  ";
				if (i & 1)
					os << "\\rowcolor[gray]{0.9}";
				os << (std::string)METARTAFChart::latex_string_meta(wel.get_name()) << " & ";
				double mass(0);
				if (wel.get_flags() & WeightBalance::Element::flag_fixed) {
					if (i < wbv.size() && wbv[i].get_unit() < wel.get_nrunits()) {
						const WeightBalance::Element::Unit& u(wel.get_unit(wbv[i].get_unit()));
						mass = std::min(std::max(wbv[i].get_value() * u.get_factor() + u.get_offset(), wel.get_massmin()), wel.get_massmax());
					} else {
						mass = wel.get_massmin();
					}
					os << " & ";
				} else {
					if (i < wbv.size()) {
						if (wel.get_flags() & WeightBalance::Element::flag_binary)
							wbv[i].set_value(0);
						double value(wbv[i].get_value());
						if (wbv[i].get_unit() < wel.get_nrunits()) {
							const WeightBalance::Element::Unit& u(wel.get_unit(wbv[i].get_unit()));
							mass = value * u.get_factor() + u.get_offset();
							if (ci & (1 << consumableindex[i])) {
								mass += consumables[consumableindex[i]].back().first;
								if (u.get_factor() > 0)
									value = (mass - u.get_offset()) / u.get_factor();
								else
									value = 0;
							}
						}
						std::ostringstream oss;
						oss << std::fixed << std::setprecision(1) << value;
						os << oss.str();
					}
					os << " & ";
				}
				{
					std::string unit;
					if (i < wbv.size() && wbv[i].get_unit() < wel.get_nrunits())
						unit = wel.get_unit(wbv[i].get_unit()).get_name();
					else if (wel.get_nrunits())
						unit = wel.get_unit(0).get_name();
					os << unit;
				}
				os << " & ";
				{
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(2) << wel.get_arm(mass);
					os << oss.str();
				}
				os << " & ";
				{
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(1) << mass;
					if (mass < wel.get_massmin() || mass > wel.get_massmax())
						os << "{\\bf " << oss.str() << '}';
					else
						os << oss.str();
				}
				os << " & ";
				{
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(1) << wel.get_massmin();
					os << oss.str();
				}
				os << " & ";
				{
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(1) << wel.get_massmax();
					os << oss.str();
				}
				os << " & ";
				{
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(1) << wel.get_moment(mass);
					os << oss.str();
				}
				os << " \\\\\n  \\hline\n";
				if (ci & (1 << consumableindex[i])) {
					const massarm_t& ma(consumables[consumableindex[i]].back());
					masscorr += ma.first;
					momentcorr += ma.second;
				}
        		}
			os << "  \\hline\n";
			unsigned int i(wb.get_nrelements());
			for (mass_t m(mass_zf); m <= mass_ramp; m = (mass_t)(m + 1)) {
				os << "  ";
				double mass(masses[m] + masscorr);
				double moment(moments[m] + momentcorr);
				double arm(moment / mass);
				double masslim(maxmasses[masslimits[m]]);
				bool overmass(!std::isnan(masslim) && mass > masslim * 1.000001);
				bool outsideenv(!!wb.get_nrenvelopes());
				for (unsigned int j = 0; j < wb.get_nrenvelopes(); ++j) {
					const WeightBalance::Envelope& env(wb.get_envelope(j));
					if (!env.is_inside(WeightBalance::Envelope::Point(arm, mass)))
						continue;
					outsideenv = false;
					break;
				}
				if (overmass || outsideenv)
					os << "\\rowcolor[rgb]{1.0,0.5,0.5}";
				else if (i & 1)
					os << "\\rowcolor[gray]{0.9}";
				os << massnames[m] << " & & & ";
				if (!std::isnan(arm)) {
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(2) << arm;
					if (outsideenv)
						os << "{\\bf " << oss.str() << '}';
					else
						os << oss.str();
				}
				os << " & ";
				if (!std::isnan(mass)) {
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(1) << mass;
					if (overmass || outsideenv)
						os << "{\\bf " << oss.str() << '}';
					else
						os << oss.str();
				}
				os << " & & ";
				if (!std::isnan(masslim)) {
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(1) << masslim;
					if (overmass)
						os << "{\\bf " << oss.str() << '}';
					else
						os << oss.str();
				}
				os << " & ";
				if (!std::isnan(moment)) {
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(1) << moment;
					if (outsideenv)
						os << "{\\bf " << oss.str() << '}';
					else
						os << oss.str();
				}
				os << "\\\\\n  \\hline\n";
				++i;
			}
		}
	}
	// Mass&Balance Graph
	os << "    graphs = {";
	{
		bool subseq(false);
		for (unsigned int ci = 0, cn = std::min((unsigned int)consumables.size(), 4U), cm = 1 << cn; ci < cm; ++ci) {
			double masscorr(0), momentcorr(0);
			for (unsigned int cj = 0; cj < cn; ++cj) {
				if (ci & (1U << cj)) {
					const massarm_t& ma(consumables[cj].back());
					masscorr += ma.first;
					momentcorr += ma.second;
				}
			}
			if (subseq)
				os << ',';
			os << std::endl << "      {";
			subseq = false;
			for (graph_t::size_type i(0), n(graph.size()); i < n; ++i) {
				if (std::isnan(graph[i].first.first) ||
				    std::isnan(graph[i].first.second))
					break;
				double arm(graph[i].first.second), mass(graph[i].first.first);
				if (ci) {
					double moment(arm * mass);
					mass += masscorr;
					moment += momentcorr;
					arm = moment / mass;
				}
				if (subseq)
					os << ',';
				os << " { " << mass << ", " << arm << ", " << (int)(graph[i].second & 7) << ", " << (!!(graph[i].second & 8)) << ", {";
				if (!consumables.empty()) {
					subseq = false;
					unsigned int cn = std::min((unsigned int)consumables.size(), 4U);
					for (unsigned int ci = 0; ci < cn; ++ci) {
						const massmoments_t& mm(consumables[ci]);
						if (mm.size() < 2)
							continue;
						for (unsigned int mi(0), mn(mm.size()); mi < mn; ++mi) {
							double mass1(mass + mm[mi].first), moment1(mass * arm + mm[mi].second);
							double arm1(moment1 / mass1);
							if (subseq)
								os << ',';
							subseq = true;
							os << " { " << mass1 << ", " << arm1 << " }";
						}
					}
				}
				os << " } }";
				subseq = true;
			}
			os << " }";
			subseq = true;
		}
	}
	os << " } }" << std::endl;
	// fuel planning
	os << "  fuelplan = {";
	if (fc.get_taxitime() > 0)
		os << std::endl << "    taxitime = " << fc.get_taxitime() << ',';
	if (!std::isnan(get_taxifuel()))
		os << std::endl << "    taxifuelbase = " << convert_fuel(get_fuelunit(), fuelunit, get_taxifuel()) << ',';
	if (!std::isnan(fc.get_taxifuelflow()))
		os << std::endl << "    taxifuelflow = " << convert_fuel(get_fuelunit(), fuelunit, fc.get_taxifuelflow()) << ',';
	if (!std::isnan(fc.get_taxifuel()))
		os << std::endl << "    taxifuel = " << convert_fuel(get_fuelunit(), fuelunit, fc.get_taxifuel()) << ',';
	if (!std::isnan(fc.get_tripfuel()))
		os << std::endl << "    tripfuel = " << convert_fuel(get_fuelunit(), fuelunit, fc.get_tripfuel()) << ',';
	if (!std::isnan(fc.get_contfuel())) {
		os << std::endl << "    contingencyfuel = " << convert_fuel(get_fuelunit(), fuelunit, fc.get_contfuel()) << ',';
		if (!std::isnan(fc.get_contfuelflow()) && fc.get_contfuelflow() > 0 && fc.get_contfuel() > 0)
			os << std::endl << "    contingencytime = " << Point::round<double,int>(fc.get_contfuel() / fc.get_contfuelflow() * 3600.0) << ',';
	}
	if (!std::isnan(fc.get_contfuelpercent()))
		os << std::endl << "    contingencypercent = " << convert_fuel(get_fuelunit(), fuelunit, fc.get_contfuelpercent()) << ',';
	if (!std::isnan(fc.get_contfuelflow()))
		os << std::endl << "    contingencyfuelflow = " << convert_fuel(get_fuelunit(), fuelunit, fc.get_contfuelflow()) << ',';
	if (!noaltn && altnidx < altn.size()) {
		const FPlanAlternate& a(altn[altnidx]);
		os << std::endl << "    alternateindex = " << (altnidx + 1) << ','
		   << std::endl << "    alternateident = " << to_luastring(a.get_ident()) << ',';
	}
	if (fc.is_altnalt_valid())
		os << std::endl << "    alternatecruisealt = " << fc.get_altnalt() << ',';
	if (!std::isnan(fc.get_altnfuel()))
		os << std::endl << "    alternatefuel = " << convert_fuel(get_fuelunit(), fuelunit, fc.get_altnfuel()) << ',';
	if (fc.is_holdalt_valid())
		os << std::endl << "    finalreservealt = " << fc.get_holdalt() << ',';
	if (!std::isnan(fc.get_holdfuelflow()))
		os << std::endl << "    finalreservefuelflow = " << convert_fuel(get_fuelunit(), fuelunit, fc.get_holdfuelflow()) << ',';
	os << std::endl << "    finalreservetime = " << fc.get_holdtime() << ',';
	if (!std::isnan(fc.get_holdfuel()))
		os << std::endl << "    finalreservefuel = " << convert_fuel(get_fuelunit(), fuelunit, fc.get_holdfuel()) << ',';
	if (!std::isnan(fc.get_reqdfuel()))
		os << std::endl << "    requiredfuel = " << convert_fuel(get_fuelunit(), fuelunit, fc.get_reqdfuel()) << ',';
	{
		double loadedfuel(0);
		for (WeightBalance::elementvalues_t::size_type i(wbv.size()); i > 0; ) {
			--i;
			const WeightBalance::Element& wel(wb.get_element(i));
			if (!(wel.get_flags() & WeightBalance::Element::flag_fuel))
				continue;
			if (wbv[i].get_unit() >= wel.get_nrunits())
				continue;
			const WeightBalance::Element::Unit& u(wel.get_unit(wbv[i].get_unit()));
			double mass(wbv[i].get_value() * u.get_factor() + u.get_offset());
			if (mass < wel.get_massmin())
				continue;
			loadedfuel += convert(wb.get_massunit(), get_fuelunit(), mass - wel.get_massmin(), wel.get_flags());
		}
		os << std::endl << "    blockfuel = " << convert_fuel(get_fuelunit(), fuelunit, loadedfuel);
	}
	if (!noaltn && !altn.empty()) {
		for (std::vector<FPlanAlternate>::const_iterator ai(altn.begin()), ae(altn.end()); ai != ae; ++ai) {
			if (ai->get_coord().is_invalid())
				continue;
			os << ',' << std::endl << "    { ";
			time_t deptime(0);
			time_t t(ai->get_flighttime());
			double f(ai->get_fuel_usg());
			if (fplan.get_nrwpt() >= 2) {
				t -= fplan[fplan.get_nrwpt()-1].get_flighttime();
				f -= fplan[fplan.get_nrwpt()-1].get_fuel_usg();
				deptime = fplan[fplan.get_nrwpt() - 1].get_time_unix();
			}
			if (t < 0)
				t = 0;
			if (f < 0)
				f = 0;
			write_lua_alternate(os, "      ", fuelunit, *ai, deptime, f,
					    std::numeric_limits<double>::quiet_NaN(), ai->get_dist_nmi(), std::numeric_limits<double>::quiet_NaN());
			os << std::endl << "      alternatetime = " << t
			   << ',' << std::endl << "      alternatefuel = " << convert_fuel(get_fuelunit(), fuelunit, f);
			if (ai->get_holdtime() > 0)
				os << ',' << std::endl << "      holdfuelflow = "
				   << convert_fuel(get_fuelunit(), fuelunit, ai->get_holdfuel_usg() / ai->get_holdtime() * 3600);
			os << " }";
		}
	}
	os << " }";
	if (tomass)
		*tomass = masses[mass_to];
	return os << std::endl;
}

std::ostream& Aircraft::distances_latex(std::ostream& os, double tomass,
					const FPlanRoute& fplan,
					const std::vector<FPlanAlternate>& altn,
					const std::vector<FPlanAlternate>& taltn,
					const std::vector<FPlanAlternate>& raltn) const
{
	std::vector<FPlanWaypoint> alt;
	if (fplan.get_nrwpt()) {
		alt.push_back(fplan[0]);
		if (fplan.get_nrwpt() >= 2)
			alt.push_back(fplan[fplan.get_nrwpt() - 1]);
	}
	alt.insert(alt.end(), taltn.begin(), taltn.end());
	alt.insert(alt.end(), raltn.begin(), raltn.end());
	if (!fplan.get_nrwpt() || altn.size() != 1 || fplan[fplan.get_nrwpt()-1].get_coord().is_invalid() ||
	    fplan[fplan.get_nrwpt()-1].get_coord() != altn.front().get_coord())
		alt.insert(alt.end(), altn.begin(), altn.end());
	if (alt.empty())
		return os;
	return distances_latex(os, tomass, alt, std::vector<DbBaseElements::Airport>());
}

std::ostream& Aircraft::distances_latex(std::ostream& os, AirportsDbQueryInterface& arptdb,
					double tomass, const FPlanRoute& fplan,
					const std::vector<FPlanAlternate>& altn,
					const std::vector<FPlanAlternate>& taltn,
					const std::vector<FPlanAlternate>& raltn) const
{
	std::vector<FPlanWaypoint> alt;
	if (fplan.get_nrwpt()) {
		alt.push_back(fplan[0]);
		if (fplan.get_nrwpt() >= 2)
			alt.push_back(fplan[fplan.get_nrwpt() - 1]);
	}
	alt.insert(alt.end(), taltn.begin(), taltn.end());
	alt.insert(alt.end(), raltn.begin(), raltn.end());
	if (!fplan.get_nrwpt() || altn.size() != 1 || fplan[fplan.get_nrwpt()-1].get_coord().is_invalid() ||
	    fplan[fplan.get_nrwpt()-1].get_coord() != altn.front().get_coord())
		alt.insert(alt.end(), altn.begin(), altn.end());
	if (alt.empty())
		return os;
       	std::vector<AirportsDb::Airport> arpt(alt.size());
	for (std::vector<FPlanWaypoint>::size_type i(0), n(alt.size()); i < n; ++i) {
		if (alt[i].get_coord().is_invalid())
			continue;
		Rect bbox(alt[i].get_coord(), alt[i].get_coord());
		bbox = bbox.oversize_nmi(0.5f);
		if (alt[i].get_icao().size() >= 4 && alt[i].get_icao() != "ZZZZ") {
			double bdist(std::numeric_limits<double>::max());
			AirportsDb::elementvector_t ev(arptdb.find_by_icao(alt[i].get_icao(), 0, AirportsDb::comp_exact, 0, AirportsDb::Airport::subtables_all));
			for (AirportsDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
				if (evi->get_coord().is_invalid() || !bbox.is_inside(evi->get_coord()) ||
				    evi->get_icao() != alt[i].get_icao())
					continue;
				double d(alt[i].get_coord().spheric_distance_nmi_dbl(evi->get_coord()));
				if (d > bdist)
					continue;
				bdist = d;
				arpt[i] = *evi;
			}
			continue;
		}
		double bdist(std::numeric_limits<double>::max());
		AirportsDb::elementvector_t ev(arptdb.find_by_rect(bbox, 0, AirportsDb::Airport::subtables_all));
		for (AirportsDb::elementvector_t::const_iterator evi(ev.begin()), eve(ev.end()); evi != eve; ++evi) {
			if (evi->get_coord().is_invalid() || !bbox.is_inside(evi->get_coord()))
				continue;
			double d(alt[i].get_coord().spheric_distance_nmi_dbl(evi->get_coord()));
			if (d > bdist)
				continue;
			bdist = d;
			arpt[i] = *evi;
		}
	}
	return distances_latex(os, tomass, alt, arpt);
}

std::ostream& Aircraft::distances_latex(std::ostream& os, double tomass,
					const std::vector<FPlanWaypoint>& alt,
					const std::vector<DbBaseElements::Airport>& arpt) const
{
	static const double headwind(0);
	static const double softfield[2] = { 0, 0.2 };
	int mode(-1);
	const Distances& dists(get_dist());
	Aircraft::unit_t distunit(dists.get_distunit());
	for (std::vector<FPlanWaypoint>::size_type i(0), n(alt.size() + !alt.empty()); i < n; ++i) {
		std::vector<FPlanWaypoint>::size_type ai(std::max(i, (std::vector<FPlanWaypoint>::size_type)1) - 1);
		bool softfld[2] = { true, true };
		if (i) {
			softfld[1] = false;
		} else if (arpt.size() > ai && arpt[ai].is_valid() && arpt[ai].get_nrrwy()) {
			softfld[0] = softfld[1] = false;
			for (unsigned int j(0), m(arpt[ai].get_nrrwy()); j < m; ++j) {
				const DbBaseElements::Airport::Runway& rwy(arpt[ai].get_rwy(j));
				if (!rwy.is_he_usable() && !rwy.is_le_usable())
					continue;
				softfld[!(rwy.is_concrete() || rwy.is_water())] = true;
			}
			if (!softfld[0] && !softfld[1])
				softfld[0] = softfld[1] = true;
		}
		for (unsigned int j(0), m(i ? dists.get_nrldgdist() : dists.get_nrtakeoffdist()); j < m; ++j) {
			const Distances::Distance& dist(i ? dists.get_ldgdist(j) : dists.get_takeoffdist(j));
			double mass(tomass - convert_fuel(get_fuelunit(), get_wb().get_massunit(), alt[ai].get_fuel_usg()));
			double da(alt[ai].get_density_altitude()), gnddist(0), obstdist(0);
			dist.calculate(gnddist, obstdist, da, headwind, mass);
			gnddist = Aircraft::convert(Aircraft::unit_m, distunit, gnddist);
			obstdist = Aircraft::convert(Aircraft::unit_m, distunit, obstdist);
			if (mode < 0) {
				os << "\\subsection{Distances}\n\n"
				   << "\\noindent\\begin{tabular}{|l|l|l|r|r|r|r|r|r|}\n"
				   << "  \\hline\n"
				   << "  \\rowcolor[gray]{0.8} &  &  & DA & Headwind & Mass & Soft & Ground & Obstacle \\\\\n"
				   << "  \\rowcolor[gray]{0.8}Waypoint & Dist & Condition & (ft) & (kts) & ("
				   << METARTAFChart::latex_string_meta(get_wb().get_massunitname())
				   << ") & Field & Roll (" << to_str(distunit) << ") & (" << to_str(distunit) << ") \\\\\n"
				   << "  \\hline\n";
				mode = 0;
			}
			for (unsigned int k(0); k < 2; ++k) {
				if (!softfld[k])
					continue;
				os << "  ";
				if (mode)
					os << "\\rowcolor[gray]{0.9}";
				mode = !mode;
				os << METARTAFChart::latex_string_meta(alt[ai].get_icao_name()) << " & "
				   << (i ? "Ldg" : "T/O") << " & "
				   << METARTAFChart::latex_string_meta(dist.get_name()) << " & ";
				{
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(0) << da;
					os << oss.str();
				}
				os << " & ";
				{
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(0) << headwind;
					os << oss.str();
				}
				os << " & ";
				{
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(0) << mass;
					os << oss.str();
				}
				os << " & ";
				if (i && !k) {
					os << "N/A";
				} else {
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(0) << (softfield[k] * 100) << "\\%";
					os << oss.str();
				}
				os << " & ";
				double distadd(gnddist * softfield[k]);
				double gnddist1(gnddist + distadd);
				double obstdist1(obstdist + distadd);
				if (!std::isnan(gnddist1)) {
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(0) << gnddist1;
					os << oss.str();
				}
				os << " & ";
				if (!std::isnan(obstdist1)) {
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(0) << obstdist1;
					os << oss.str();
				}
				os << "\\\\\n  \\hline\n";
			}
		}
	}
	if (mode >= 0)
		os << "\\end{tabular}\n\n";
	return os;
}

std::ostream& Aircraft::climbdescent_latex(const ClimbDescent& cd, cdltxmode_t cdltxmode, std::ostream& os, double xmass, double isaoffs, double qnh,
					   unit_t fuelunit, const FPlanWaypoint& wpt) const
{
	if (!((1 << fuelunit) & (unitmask_mass | unitmask_volume))) {
		fuelunit = get_fuelunit();
		if (!((1 << fuelunit) & (unitmask_mass | unitmask_volume)))
			fuelunit = unit_lb;
	}
	double pagnd(wpt.get_pressure_altitude());
	double agnd(wpt.get_true_altitude());
	double tgnd(cd.altitude_to_time(pagnd));
	double dgnd(cd.time_to_distance(tgnd));
	double fgnd(cd.time_to_fuel(tgnd));
	if (false)
		std::cerr << (cdltxmode == cdltxmode_climb ? "Climb" :
			      cdltxmode == cdltxmode_descent ? "Descent" :
			      cdltxmode == cdltxmode_glide ? "Glide" : "??")
			  << ": ground: pa " << pagnd << " a " << agnd << " t " << tgnd << " d " << dgnd << " f " << fgnd
			  << " qff " << wpt.get_qff_hpa() << " tempoffs " << wpt.get_isaoffset_kelvin() << std::endl;
	if (false)
		cd.print(std::cerr << "climbdescent:", "  ");
	AirData<float> ad;
	ad.set_qnh_tempoffs(wpt.get_qff_hpa(), wpt.get_isaoffset_kelvin());
	int mode(-1);
	for (double alt = 1e3 * ceil(agnd * 1e-3); alt <= 600000; alt += 1000) {
		ad.set_true_altitude(alt);
		double pa(ad.get_pressure_altitude());
		if (pa <= pagnd)
			continue;
		if (pa > cd.get_ceiling() + 500)
			break;
		double t(cd.altitude_to_time(pa));
		double d(cd.time_to_distance(t));
		double f(cd.time_to_fuel(t));
                double cr(cd.time_to_climbrate(t));
                double tas(cd.time_to_tas(t));
                double ff(cd.time_to_fuelflow(t));
		if (false)
			std::cerr << (cdltxmode == cdltxmode_climb ? "Climb" :
				      cdltxmode == cdltxmode_descent ? "Descent" :
				      cdltxmode == cdltxmode_glide ? "Glide" : "??")
				  << ": alt " << alt << " pa " << pa << " t " << t << " d " << d << " f " << f
				  << " cr " << cr << " tas " << tas << " ff " << ff << std::endl;
		t -= tgnd;
		d -= dgnd;
		f -= fgnd;
		if (t <= 0 || d < 0.1)
			continue;
		double cgrad((100 * Point::ft_to_m_dbl * 1e-3 * Point::km_to_nmi_dbl) * (alt - agnd) / d);
		if (mode < 0) {
			os << "\\subsection{" << (cdltxmode == cdltxmode_climb ? "Climb" :
						  cdltxmode == cdltxmode_descent ? "Descent" :
						  cdltxmode == cdltxmode_glide ? "Glide" : "??") << "}\n\n"
			   << "\\noindent{}DA " << Glib::ustring::format(std::fixed, std::setprecision(0), wpt.get_density_altitude())
			   << "ft, QFF " << Glib::ustring::format(std::fixed, std::setprecision(1), wpt.get_qff_hpa())
			   << "hPa, ISA" << Glib::ustring::format(std::fixed, std::setprecision(0), std::showpos, wpt.get_isaoffset_kelvin())
			   << "$^\\circ$C, " << (cdltxmode == cdltxmode_climb ? "TO " :
						 cdltxmode == cdltxmode_descent ? "Ldg " : "")
			   << "Mass " << Glib::ustring::format(std::fixed, std::setprecision(0), xmass)
			   << get_wb().get_massunitname();
			if (get_wb().get_massunit() != unit_kg)
				os << " (" << Glib::ustring::format(std::fixed, std::setprecision(0), convert(get_wb().get_massunit(), unit_kg, xmass)) << "kg)";
			os << "\n\n\\noindent\\begin{longtable}{|r|r|r|r|r|r|" << (cdltxmode != cdltxmode_glide ? "r|r|" : "") << "r|r|}\n"
			   << "  \\hline\n"
			   << "  \\rowcolor[gray]{0.8}True Alt & PA & DA & Rate & Track & Time & " << (cdltxmode != cdltxmode_glide ? "Fuel & FF & " : "") << "TAS & Gradient \\\\\n"
			   << "  \\rowcolor[gray]{0.8}ft & FL & ft & ft/min & nmi & & ";
			if (cdltxmode != cdltxmode_glide)
				os << to_str(fuelunit) << " & " << to_str(fuelunit) << "/h & ";
			os << "kts & \\% \\endhead\n"
			   << "  \\hline\n";
			mode = 0;
		}
		os << "  ";
		if (mode)
			os << "\\rowcolor[gray]{0.9}";
		mode = !mode;
		{
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(0) << ad.get_true_altitude();
			os << oss.str();
		}
		os << " & ";
		{
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(0) << (ad.get_pressure_altitude() * 1e-2);
			os << oss.str();
		}
		os << " & ";
		{
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(0) << ad.get_density_altitude();
			os << oss.str();
		}
		os << " & ";
		{
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(0) << cr;
			os << oss.str();
		}
		os << " & ";
		{
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(1) << d;
			os << oss.str();
		}
		os << " & ";
		{
			int tm(Point::round<int,double>(t));
			if (tm < 0)
				tm = 0;
			std::ostringstream oss;
			oss << std::setw(2) << std::setfill('0') << (tm / 60) << ':' << std::setw(2) << std::setfill('0') << (tm % 60);
			os << oss.str();
		}
		if (cdltxmode != cdltxmode_glide) {
			os << " & ";
			{
				unsigned int prec(fuelunit == unit_usgal);
				std::ostringstream oss;
				oss << std::fixed << std::setprecision(prec) << convert_fuel(get_fuelunit(), fuelunit, f);
				os << oss.str();
			}
			os << " & ";
			{
				unsigned int prec(fuelunit == unit_usgal);
				std::ostringstream oss;
				oss << std::fixed << std::setprecision(prec) << convert_fuel(get_fuelunit(), fuelunit, ff);
				os << oss.str();
			}
		}
		os << " & ";
		{
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(0) << tas;
			os << oss.str();
		}
		os << " & ";
		{
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(2) << cgrad << "\\%";
			os << oss.str();
		}
		os << "\\\\\n  \\hline\n";
	}
	if (mode >= 0)
		os << "\\end{longtable}\n\n";
	else
		os << "\\subsection{" << (cdltxmode == cdltxmode_climb ? "Climb" :
					  cdltxmode == cdltxmode_descent ? "Descent" :
					  cdltxmode == cdltxmode_glide ? "Glide" : "??") << "}\n\n";
	// climb performance graph
	const int xdiv = 10;
	const int ydiv = 8;
	double altscale, distscale, timescale, fuelscale, casscale, casmin;
	int ymax(ydiv);
	{
		double maxalt(cd.time_to_altitude(cd.get_climbtime()));
		if (std::isnan(maxalt))
			maxalt = 0;
		if (!std::isnan(cd.get_ceiling()))
			maxalt = std::min(maxalt, cd.get_ceiling());
		double t(cd.altitude_to_time(maxalt));
		if (!std::isnan(cd.get_climbtime()))
			t = std::min(t, cd.get_climbtime());
		altscale = pow(10, std::floor(log10(maxalt / ydiv)));
		if (altscale * ydiv < maxalt) {
			altscale *= 2;
			if (altscale * ydiv < maxalt) {
				altscale *= 2;
				if (altscale * ydiv < maxalt) {
					altscale *= 1.25;
					if (altscale * ydiv < maxalt)
						altscale *= 2;
				}
			}
		}
		timescale = 60 * pow(10, std::floor(log10(t * (1.0 / 60) / xdiv)));
		if (timescale * xdiv < t) {
			timescale *= 2;
			if (timescale * xdiv < t) {
				timescale *= 2;
				if (timescale * xdiv < t) {
					timescale *= 1.25;
					if (timescale * xdiv < t)
						timescale *= 2;
				}
			}
		}
		double maxdist(cd.time_to_distance(t));
		distscale = pow(10, std::floor(log10(maxdist / xdiv)));
		if (distscale * xdiv < maxdist) {
			distscale *= 2;
			if (distscale * xdiv < maxdist) {
				distscale *= 2;
				if (distscale * xdiv < maxdist) {
					distscale *= 1.25;
					if (distscale * xdiv < maxdist)
						distscale *= 2;
				}
			}
		}
		if (cdltxmode == cdltxmode_glide) {
			fuelscale = std::numeric_limits<double>::quiet_NaN();
		} else {
			double maxfuel(cd.time_to_fuel(t));
			fuelscale = pow(10, std::floor(log10(maxfuel / xdiv)));
			if (fuelscale * xdiv < maxfuel) {
				fuelscale *= 2;
				if (fuelscale * xdiv < maxfuel) {
					fuelscale *= 2;
					if (fuelscale * xdiv < maxfuel) {
					fuelscale *= 1.25;
					if (fuelscale * xdiv < maxfuel)
						fuelscale *= 2;
					}
				}
			}
		}
		double maxcas(0), mincas(std::numeric_limits<double>::max());
		{
			AirData<float> ad;
			ad.set_qnh_temp(); // standard atmosphere
			for (double t1 = 0, tinc = t * 0.01; t1 <= t; ) {
				ad.set_pressure_altitude(cd.time_to_altitude(t1));
				ad.set_tas(cd.time_to_tas(t1));
				double cas(ad.get_cas());
				if (false)
					std::cerr << "climbdescent: t " << t1 << " alt " << cd.time_to_altitude(t1)
						  << " tas " << cd.time_to_tas(t1) << " cas " << cas << std::endl;
				if (std::isnan(cas))
					continue;
				mincas = std::min(mincas, cas);
				maxcas = std::max(maxcas, cas);
				double t2(t1 + tinc);
				if (t2 <= t1)
					break;
				t1 = t2;
			}
		}
		double casdiff(std::max(maxcas - mincas, 10.));
		double casmean(0.5 * (mincas + maxcas));
		casscale = pow(10, std::floor(log10(casdiff / xdiv)));
		if (casscale * xdiv < casdiff) {
			casscale *= 2;
			if (casscale * xdiv < casdiff) {
				casscale *= 2;
				if (casscale * xdiv < casdiff) {
					casscale *= 1.25;
					if (casscale * xdiv < casdiff)
						casscale *= 2;
				}
			}
		}
		if (false)
			std::cerr << "climbdescent: name " << cd.get_name() << " mass " << cd.get_mass() << " isa " << cd.get_isaoffset()
				  << " ceil " << cd.get_ceiling() << " clbtime " << cd.get_climbtime() << " mode " << cd.get_mode()
				  << " maxalt " << maxalt << " t " << t <<  " cas: min " << mincas << " max " << maxcas
				  << " diff " << casdiff << " mean " << casmean << " scale " << casscale
				  << " min2 " << ((Point::round<int,double>(casmean / casscale) - (xdiv >> 1)) * casscale)
				  << std::endl;
		casmin = (Point::round<int,double>(casmean / casscale) - (xdiv >> 1)) * casscale;
		ymax = std::min(ydiv, Point::round<int,double>(std::ceil(maxalt * 1.05 / altscale)));
	}
	if (std::isnan(distscale) && std::isnan(timescale) && std::isnan(fuelscale) && (std::isnan(casscale) || std::isnan(casmin)))
		return os;
	if (ymax <= 0)
		return os;
	os << "\n\\begin{center}\\begin{tikzpicture}[scale=1.5]\n";
	if (false)
		os << "  \\path[clip] (0,0) rectangle (" << xdiv << ',' << ymax << ");\n";
	os << "  \\draw[step=1,help lines] (0,0) grid (" << xdiv << ',' << ymax << ");\n";
	for (int x(0); x < xdiv; ++x)
		os << "  \\path[draw,thin,dotted] (" << x << ".5,0) -- (" << x << ".5," << ymax << ");\n";
	for (int y(0); y < ymax; ++y)
		os << "  \\path[draw,thin,dotted] (0," << y << ".5) -- (" << xdiv << ',' << y << ".5);\n";
	os << "  \\draw[step=1,thin,dotted] (0.5,0.5) grid (" << (xdiv - 1) << ".5," << (ymax-1) << ".5);\n";
	for (int x(0); x <= xdiv; ++x) {
		std::ostringstream oss;
		oss << "\\begin{tabular}{c}";
		if (!std::isnan(distscale))
			oss << "\\textcolor{red}{"
			    << std::fixed << std::setprecision(0) << (x * distscale)
			    << "}\\\\";
		if (!std::isnan(timescale))
			oss << "\\textcolor{blue}{"
			    << std::fixed << std::setprecision(0) << (x * timescale * (1.0 / 60))
			    << "}\\\\";
		if (!std::isnan(fuelscale))
			oss << "\\textcolor{magenta}{"
			    << std::fixed << std::setprecision(fuelunit == unit_usgal) << convert_fuel(get_fuelunit(), fuelunit, x * fuelscale)
			    << "}\\\\";
		if (!std::isnan(casscale) && !std::isnan(casmin))
			oss << "\\textcolor{green}{"
			    << std::fixed << std::setprecision(0) << (cdltxmode == cdltxmode_climb ? (x * casscale + casmin) : ((xdiv - x) * casscale + casmin))
			    << "}";
		oss << "\\end{tabular}";
		int xx(cdltxmode == cdltxmode_climb ? x : (xdiv - x));
		os << "  \\path[draw] " << METARTAFChart::pgfcoord(xx, 0.1) << " -- " << METARTAFChart::pgfcoord(xx, -0.1)
		   << " node[anchor=north] {" << oss.str() << "};\n";
	}
	for (int y(0); y <= ymax; ++y) {
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(0) << (y * altscale);
		os << "  \\path[draw] " << METARTAFChart::pgfcoord(0.1, y) << " -- " << METARTAFChart::pgfcoord(-0.1, y)
		   << " node[anchor=east] {" << oss.str() << "};\n";
	}
	os << "  \\node[anchor=north] at " << METARTAFChart::pgfcoord(xdiv - 0.5, -0.1)
	   << " {\\begin{tabular}{c}";
	if (!std::isnan(distscale))
		os << "\\textcolor{red}{Dist}\\\\";
	if (!std::isnan(timescale))
		os << "\\textcolor{blue}{Time}\\\\";
	if (!std::isnan(fuelscale))
		os << "\\textcolor{magenta}{Fuel}\\\\";
	if (!std::isnan(casscale) && !std::isnan(casmin))
		os << "\\textcolor{green}{CAS}";
	os << "\\end{tabular}};\n"
	   << "  \\node[anchor=east] at " << METARTAFChart::pgfcoord(-0.1, ymax - 0.5) << " {Alt};\n";
	{
		std::string pdist, ptime, pfuel, pcas;
		AirData<float> ad;
		ad.set_qnh_temp(); // standard atmosphere
		bool ndist(false), ntime(false), nfuel(cdltxmode == cdltxmode_glide), ncas(false);
		for (double t(0), tinc(cd.get_climbtime() * 0.01), tmax(cd.get_climbtime()); t <= tmax; t += tinc) {
			double alt(cd.time_to_altitude(t) / altscale);
			double dist(cd.time_to_distance(t) / distscale);
			double fuel(cd.time_to_fuel(t) / fuelscale);
			double tim(t / timescale);
			ad.set_pressure_altitude(cd.time_to_altitude(t));
			ad.set_tas(cd.time_to_tas(t));
			double cas((ad.get_cas() - casmin) / casscale);
			if (false)
				std::cerr << "climbdescent: t " << t << " alt " << cd.time_to_altitude(t)
					  << " tas " << cd.time_to_tas(t) << " cas " << ad.get_cas()
					  << " scaled " << cas << std::endl;
			if (cdltxmode != cdltxmode_climb) {
				dist = xdiv - dist;
				tim = xdiv - tim;
				fuel = xdiv - fuel;
			}
			pdist += " -- " + METARTAFChart::pgfcoord(dist, alt);
			ptime += " -- " + METARTAFChart::pgfcoord(tim, alt);
			pfuel += " -- " + METARTAFChart::pgfcoord(fuel, alt);
			pcas += " -- " + METARTAFChart::pgfcoord(cas, alt);
			bool nalt(std::isnan(alt));
			ndist = ndist || std::isnan(dist) || nalt;
			ntime = ntime || std::isnan(tim) || nalt;
			nfuel = nfuel || std::isnan(fuel) || nalt;
			ncas = ncas || std::isnan(cas) || nalt;
		}
		if (!ndist)
			os << "  \\path[draw,thick,red] " << pdist.substr(4) << ";\n";
		if (!ntime)
			os << "  \\path[draw,thick,blue] " << ptime.substr(4) << ";\n";
		if (!nfuel)
			os << "  \\path[draw,thick,magenta] " << pfuel.substr(4) << ";\n";
		if (!ncas)
			os << "  \\path[draw,thick,green] " << pcas.substr(4) << ";\n";
	}
	os << "\\end{tikzpicture}\\end{center}\n\n";
	return os;
}

std::ostream& Aircraft::climb_latex(std::ostream& os, double tomass, double isaoffs, double qnh, unit_t fuelunit, const FPlanWaypoint& wpt) const
{
	if (false)
		tomass = get_mtom();
	ClimbDescent climb(calculate_climb("", tomass, isaoffs, qnh));
	if (false)
		get_climb().print(std::cerr << "Climb" << std::endl, "  ");
	if (false)
		climb.print(std::cerr << "Climb Profile for mass " << tomass << " isaoffs " << isaoffs << " qnh " << qnh << std::endl, "  ");
	return climbdescent_latex(climb, cdltxmode_climb, os, tomass, isaoffs, qnh, fuelunit, wpt);
}

std::ostream& Aircraft::descent_latex(std::ostream& os, double dmass, double isaoffs, double qnh, unit_t fuelunit, const FPlanWaypoint& wpt) const
{
	if (false)
		dmass = get_mtom();
	ClimbDescent descent(calculate_descent("", dmass, isaoffs, qnh));
	if (false)
		get_descent().print(std::cerr << "Descent" << std::endl, "  ");
	if (false)
		descent.print(std::cerr << "Descent Profile for mass " << dmass << " isaoffs " << isaoffs << " qnh " << qnh << std::endl, "  ");
	return climbdescent_latex(descent, cdltxmode_descent, os, dmass, isaoffs, qnh, fuelunit, wpt);
}

std::ostream& Aircraft::glide_latex(std::ostream& os, double dmass, double isaoffs, double qnh, unit_t fuelunit, const FPlanWaypoint& wpt) const
{
	if (false)
		dmass = get_mtom();
	ClimbDescent glide(calculate_glide("", dmass, isaoffs, qnh));
	if (false)
		glide.print(std::cerr << "Glide Profile for mass " << dmass << " isaoffs " << isaoffs << " qnh " << qnh << std::endl, "  ");
	return climbdescent_latex(glide, cdltxmode_glide, os, dmass, isaoffs, qnh, fuelunit, wpt);
}

void Aircraft::compute_masses(double *rampmass, double *tomass, double *tofuel, double fuel_on_board, time_t taxitime,
			      const WeightBalance::elementvalues_t& wbev) const
{
	if (rampmass)
		*rampmass = std::numeric_limits<double>::quiet_NaN();
	if (tomass)
		*tomass = std::numeric_limits<double>::quiet_NaN();
	if (tofuel)
		*tofuel = std::numeric_limits<double>::quiet_NaN();
	const WeightBalance& wb(get_wb());
	WeightBalance::elementvalues_t wbv(prepare_wb(wbev, fuel_on_board, get_fuelunit(), wb.get_massunit(), unit_invalid));
	double totmass(0), tofuelmass(0);
	for (unsigned int i = 0; i < wb.get_nrelements(); ++i) {
		const WeightBalance::Element& wel(wb.get_element(i));
		if (false) {
			double mass(0);
			if (wel.get_flags() & WeightBalance::Element::flag_fixed) {
				if (i < wbv.size() && wbv[i].get_unit() < wel.get_nrunits()) {
					const WeightBalance::Element::Unit& u(wel.get_unit(wbv[i].get_unit()));
					mass = std::min(std::max(wbv[i].get_value() * u.get_factor() + u.get_offset(), wel.get_massmin()), wel.get_massmax());
				} else {
					mass = wel.get_massmin();
				}
			} else {
				if (i < wbv.size()) {
					if (wel.get_flags() & WeightBalance::Element::flag_binary)
						wbv[i].set_value(0);
					if (wbv[i].get_unit() < wel.get_nrunits()) {
						const WeightBalance::Element::Unit& u(wel.get_unit(wbv[i].get_unit()));
						mass = wbv[i].get_value() * u.get_factor() + u.get_offset();
					}
				}
			}
			std::cerr << "compute_masses: " << i << ": " << wel.get_name() << " arm " << wel.get_arm(mass)
				  << " mass min " << wel.get_massmin() << " max " << wel.get_massmax();
			if (i < wbv.size()) {
				std::cerr << " in " << wbv[i].get_value();
				if (wbv[i].get_unit() < wel.get_nrunits()) {
					const WeightBalance::Element::Unit& u(wel.get_unit(wbv[i].get_unit()));
					std::cerr << " unit " << u.get_name() << " f " << u.get_factor() << " o " << u.get_offset()
						  << " out " << std::min(std::max(wbv[i].get_value() * u.get_factor() + u.get_offset(), wel.get_massmin()), wel.get_massmax());
				}
			}
			std::cerr << std::endl;
		}
		double mass(0);
		if (wel.get_flags() & WeightBalance::Element::flag_fixed) {
			if (i < wbv.size() && wbv[i].get_unit() < wel.get_nrunits()) {
				const WeightBalance::Element::Unit& u(wel.get_unit(wbv[i].get_unit()));
				mass = std::min(std::max(wbv[i].get_value() * u.get_factor() + u.get_offset(), wel.get_massmin()), wel.get_massmax());
			} else {
				mass = wel.get_massmin();
			}
		} else {
			if (i < wbv.size()) {
				if (wel.get_flags() & WeightBalance::Element::flag_binary)
					wbv[i].set_value(0);
				if (wbv[i].get_unit() < wel.get_nrunits()) {
					const WeightBalance::Element::Unit& u(wel.get_unit(wbv[i].get_unit()));
					mass = wbv[i].get_value() * u.get_factor() + u.get_offset();
				}
			}
		}
		if (i < wbv.size()) {
			totmass += mass;
			if (wel.get_flags() & WeightBalance::Element::flag_fuel)
				tofuelmass += mass;
		}
	}
	if (rampmass)
		*rampmass = totmass;
	if (false)
		std::cerr << "TO fuel mass before taxi: " << tofuelmass << " taxitime: " << taxitime << std::endl;
	{
		double tfmass(convert_fuel(get_fuelunit(), wb.get_massunit(), get_taxifuel()));
		if (!std::isnan(tfmass) && tfmass > 0) {
			totmass -= tfmass;
			tofuelmass -= tfmass;
		}
	}
	if (taxitime > 0) {
		double tfmass(convert_fuel(get_fuelunit(), wb.get_massunit(), get_taxifuelflow()) * taxitime * (1.0 / 3600.0));
		if (!std::isnan(tfmass) && tfmass > 0) {
			totmass -= tfmass;
			tofuelmass -= tfmass;
		}
	}
	if (false)
		std::cerr << "TO fuel mass after taxi: " << tofuelmass << std::endl;
	if (totmass < 0)
		totmass = 0;
	if (tomass)
		*tomass = totmass;
	tofuelmass = std::max(tofuelmass, 0.);
	if (tofuel)
		*tofuel = convert_fuel(get_wb().get_massunit(), get_fuelunit(), tofuelmass);
}
