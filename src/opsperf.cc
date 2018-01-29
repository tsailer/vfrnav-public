/*****************************************************************************/

/*
 *      opsperf.cc  --  Aircraft Operations Performance Model.
 *
 *      Copyright (C) 2014, 2015  Thomas Sailer (t.sailer@alumni.ethz.ch)
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

#include "opsperf.h"

#include <iostream>
#include <fstream>

#include "geom.h"

OperationsPerformance::Aircraft::Config::Config(const std::string& phase, const std::string& name,
						double vstall, double cd0, double cd2)
	: m_phase(phase), m_name(name), m_vstall(vstall), m_cd0(cd0), m_cd2(cd2)
{
	trim(m_phase);
	trim(m_name);
}

OperationsPerformance::Aircraft::Procedure::Procedure(double clcaslo, double clcashi, double clmc,
						      double crcaslo, double crcashi, double crmc,
						      double dcaslo, double dcashi, double dmc)
	: m_climbcaslo(clcaslo), m_climbcashi(clcashi), m_climbmc(clmc),
	  m_cruisecaslo(crcaslo), m_cruisecashi(crcashi), m_cruisemc(crmc),
	  m_descentcaslo(dcaslo), m_descentcashi(dcashi), m_descentmc(dmc)
{
}

OperationsPerformance::Aircraft::ComputeResult::ComputeResult(double mass, double thrust, double drag,
							      double ff, double esf, double rocd,
							      double tdc, double cpowred, double grad)
	: m_mass(mass), m_thrust(thrust), m_drag(drag), m_fuelflow(ff), m_esf(esf), m_rocd(rocd), m_tdc(tdc),
	  m_cpowred(cpowred), m_gradient(grad)
{
}

OperationsPerformance::Aircraft::Aircraft(const std::string& basename, const std::string& desc)
	: m_basename(basename), m_description(desc),
	  m_massref(std::numeric_limits<double>::quiet_NaN()),
	  m_massmin(std::numeric_limits<double>::quiet_NaN()),
	  m_massmax(std::numeric_limits<double>::quiet_NaN()),
	  m_massmaxpayload(std::numeric_limits<double>::quiet_NaN()),
	  m_massgrad(std::numeric_limits<double>::quiet_NaN()),
	  m_vmo(std::numeric_limits<double>::quiet_NaN()),
	  m_mmo(std::numeric_limits<double>::quiet_NaN()),
	  m_maxalt(std::numeric_limits<double>::quiet_NaN()),
	  m_hmax(std::numeric_limits<double>::quiet_NaN()),
	  m_tempgrad(std::numeric_limits<double>::quiet_NaN()),
	  m_wingsurface(std::numeric_limits<double>::quiet_NaN()),
	  m_clbo(std::numeric_limits<double>::quiet_NaN()),
	  m_k(std::numeric_limits<double>::quiet_NaN()),
	  m_cm16(std::numeric_limits<double>::quiet_NaN()),
	  m_spoilercd2(std::numeric_limits<double>::quiet_NaN()),
	  m_gearcd0(std::numeric_limits<double>::quiet_NaN()),
	  m_gearcd2(std::numeric_limits<double>::quiet_NaN()),
	  m_brakecd2(std::numeric_limits<double>::quiet_NaN()),
	  m_thrustdesclow(std::numeric_limits<double>::quiet_NaN()),
	  m_thrustdeschigh(std::numeric_limits<double>::quiet_NaN()),
	  m_thrustdesclevel(std::numeric_limits<double>::quiet_NaN()),
	  m_thrustdescapp(std::numeric_limits<double>::quiet_NaN()),
	  m_thrustdescldg(std::numeric_limits<double>::quiet_NaN()),
	  m_thrustdesccas(std::numeric_limits<double>::quiet_NaN()),
	  m_thrustdescmach(std::numeric_limits<double>::quiet_NaN()),
	  m_cruisefuel(std::numeric_limits<double>::quiet_NaN()),
	  m_tol(std::numeric_limits<double>::quiet_NaN()),
	  m_ldl(std::numeric_limits<double>::quiet_NaN()),
	  m_span(std::numeric_limits<double>::quiet_NaN()),
	  m_length(std::numeric_limits<double>::quiet_NaN()),
	  m_nrengines(0), m_propulsion(propulsion_invalid), m_wakecategory(' ')
{
	m_thrustclimb[0] = m_thrustclimb[1] = m_thrustclimb[2] = m_thrustclimb[3] =
		m_thrustclimb[4] = std::numeric_limits<double>::quiet_NaN();
	m_thrustfuel[0] = m_thrustfuel[1] = std::numeric_limits<double>::quiet_NaN();
	m_descfuel[0] = m_descfuel[1] = std::numeric_limits<double>::quiet_NaN();
}

const OperationsPerformance::Aircraft::Config& OperationsPerformance::Aircraft::get_config(const std::string& phase) const
{
	configs_t::const_iterator i(m_configs.find(Config(phase)));
	if (i != m_configs.end())
		return *i;
	static const Config invalid_config;
	return invalid_config;
}

void OperationsPerformance::Aircraft::load(const std::string& dir)
{
	if (get_basename().empty())
		throw std::runtime_error("invalid basename");
	load_opf(dir + "/" + get_basename() + ".OPF");
	load_apf(dir + "/" + get_basename() + ".APF");
}

void OperationsPerformance::Aircraft::load_opf(const std::string& fn)
{
	std::ifstream ifs(fn.c_str());
	if (!ifs.good())
		throw std::runtime_error("Cannot open " + fn);
	for (;;) {
		std::string line;
		getline(ifs, line);
		if (ifs.fail())
			throw std::runtime_error("premature end of " + fn);
		if (line.size() < 60 || line.substr(0, 2) != "CD")
			continue;
		m_wakecategory = line[59];
		m_nrengines = strtoul(line.c_str() + 20, 0, 10);
		if (!m_nrengines)
			throw std::runtime_error("invalid number of engines");
		std::string prop(line, 33, 20);
		trim(prop);
		if (prop == "Piston")
			m_propulsion = propulsion_piston;
		else if (prop == "Turboprop")
			m_propulsion = propulsion_turboprop;
		else if (prop == "Jet")
			m_propulsion = propulsion_jet;
		else
			throw std::runtime_error("invalid propulsion: " + prop);
		getline(ifs, line);
		if (ifs.fail())
			throw std::runtime_error("premature end of " + fn);
		std::string::size_type n1(line.find("with")), n2(line.find("engine"));
		if (n2 == std::string::npos)
			n2 = line.find("eng.");
		if (n1 != std::string::npos && n2 != std::string::npos && n1 > 4 && n2 > n1) {
			m_acfttype = std::string(line, 4, n1 - 4);
			trim(m_acfttype);
			trimfront(m_acfttype);
			m_enginetype = std::string(line, n1 + 5, n2 - n1 - 5);
			trim(m_enginetype);
		}
		break;
	}
	for (;;) {
		std::string line;
		getline(ifs, line);
		if (ifs.fail())
			throw std::runtime_error("premature end of " + fn);
		if (line.size() < 60 || line.substr(0, 2) != "CD")
			continue;
		if (sscanf(line.c_str(), "CD %lf %lf %lf %lf %lf ", &m_massref,
			   &m_massmin, &m_massmax, &m_massmaxpayload, &m_massgrad) < 5)
			throw std::runtime_error("cannot parse masses");
		m_massref *= 1e3;
		m_massmin *= 1e3;
		m_massmax *= 1e3;
		m_massmaxpayload *= 1e3;
		break;
	}
	for (;;) {
		std::string line;
		getline(ifs, line);
		if (ifs.fail())
			throw std::runtime_error("premature end of " + fn);
		if (line.size() < 60 || line.substr(0, 2) != "CD")
			continue;
		if (sscanf(line.c_str(), "CD %lf %lf %lf %lf %lf ", &m_vmo,
			   &m_mmo, &m_maxalt, &m_hmax, &m_tempgrad) < 5)
			throw std::runtime_error("cannot parse flight envelope");
		break;
	}
	unsigned int nrcfg(0);
	for (;;) {
		std::string line;
		getline(ifs, line);
		if (ifs.fail())
			throw std::runtime_error("premature end of " + fn);
		if (line.size() < 60 || line.substr(0, 2) != "CD")
			continue;
		if (sscanf(line.c_str(), "CD %u %lf %lf %lf %lf ", &nrcfg,
			   &m_wingsurface, &m_clbo, &m_k, &m_cm16) < 5)
			throw std::runtime_error("cannot parse wing");
		break;
	}
	for (unsigned int cfgc(0); cfgc < nrcfg;) {
		std::string line;
		getline(ifs, line);
		if (ifs.fail())
			throw std::runtime_error("premature end of " + fn);
		if (line.size() < 60 || line.substr(0, 2) != "CD")
			continue;
		unsigned int n(0);
		char phase[8];
		char cfgname[16];
		double vstall;
		double cd0;
		double cd2;
		if (sscanf(line.c_str(), "CD %u %5c %10c %lf %lf %lf ",
			   &n, phase, cfgname, &vstall, &cd0, &cd2) < 6)
			throw std::runtime_error("cannot parse configuration");
		phase[5] = 0;
		cfgname[10] = 0;
		Config cfg(phase, cfgname, vstall, cd0, cd2);
		std::pair<configs_t::iterator,bool> ins(m_configs.insert(cfg));
		if (!ins.second)
			std::cerr << get_basename() << ": duplicate configuration " << cfg.get_phase() << std::endl;
		++cfgc;
	}
	for (;;) {
		std::string line;
		getline(ifs, line);
		if (ifs.fail())
			throw std::runtime_error("premature end of " + fn);
		if (line.size() < 60 || line.substr(0, 2) != "CD")
			continue;
		unsigned int n(0);
		if (sscanf(line.c_str(), "CD %u RET ", &n) < 1)
			throw std::runtime_error("cannot parse spoiler");
		break;
	}
	for (;;) {
		std::string line;
		getline(ifs, line);
		if (ifs.fail())
			throw std::runtime_error("premature end of " + fn);
		if (line.size() < 60 || line.substr(0, 2) != "CD")
			continue;
		unsigned int n(0);
		if (sscanf(line.c_str(), "CD %u EXT %lf ", &n, &m_spoilercd2) < 2)
			throw std::runtime_error("cannot parse spoiler");
		break;
	}
	for (;;) {
		std::string line;
		getline(ifs, line);
		if (ifs.fail())
			throw std::runtime_error("premature end of " + fn);
		if (line.size() < 60 || line.substr(0, 2) != "CD")
			continue;
		unsigned int n(0);
		if (sscanf(line.c_str(), "CD %u UP ", &n) < 1)
			throw std::runtime_error("cannot parse gear");
		break;
	}
	for (;;) {
		std::string line;
		getline(ifs, line);
		if (ifs.fail())
			throw std::runtime_error("premature end of " + fn);
		if (line.size() < 60 || line.substr(0, 2) != "CD")
			continue;
		unsigned int n(0);
		if (sscanf(line.c_str(), "CD %u DOWN %lf %lf ", &n, &m_gearcd0, &m_gearcd2) < 3)
			throw std::runtime_error("cannot parse gear");
		break;
	}
	for (;;) {
		std::string line;
		getline(ifs, line);
		if (ifs.fail())
			throw std::runtime_error("premature end of " + fn);
		if (line.size() < 60 || line.substr(0, 2) != "CD")
			continue;
		unsigned int n(0);
		if (sscanf(line.c_str(), "CD %u OFF ", &n) < 1)
			throw std::runtime_error("cannot parse brake");
		break;
	}
	for (;;) {
		std::string line;
		getline(ifs, line);
		if (ifs.fail())
			throw std::runtime_error("premature end of " + fn);
		if (line.size() < 60 || line.substr(0, 2) != "CD")
			continue;
		unsigned int n(0);
		if (sscanf(line.c_str(), "CD %u ON %lf ", &n, &m_brakecd2) < 2)
			throw std::runtime_error("cannot parse brake");
		break;
	}
	for (;;) {
		std::string line;
		getline(ifs, line);
		if (ifs.fail())
			throw std::runtime_error("premature end of " + fn);
		if (line.size() < 60 || line.substr(0, 2) != "CD")
			continue;
		if (sscanf(line.c_str(), "CD %lf %lf %lf %lf %lf ", &m_thrustclimb[0],
			   &m_thrustclimb[1], &m_thrustclimb[2], &m_thrustclimb[3],
			   &m_thrustclimb[4]) < 5)
			throw std::runtime_error("cannot parse climb thrust");
		break;
	}
	for (;;) {
		std::string line;
		getline(ifs, line);
		if (ifs.fail())
			throw std::runtime_error("premature end of " + fn);
		if (line.size() < 60 || line.substr(0, 2) != "CD")
			continue;
		if (sscanf(line.c_str(), "CD %lf %lf %lf %lf %lf ", &m_thrustdesclow,
			   &m_thrustdeschigh, &m_thrustdesclevel, &m_thrustdescapp,
			   &m_thrustdescldg) < 5)
			throw std::runtime_error("cannot parse descent thrust");
		break;
	}
	for (;;) {
		std::string line;
		getline(ifs, line);
		if (ifs.fail())
			throw std::runtime_error("premature end of " + fn);
		if (line.size() < 60 || line.substr(0, 2) != "CD")
			continue;
		if (sscanf(line.c_str(), "CD %lf %lf ", &m_thrustdesccas,
			   &m_thrustdescmach) < 2)
			throw std::runtime_error("cannot parse descent airspeed");
		break;
	}
	for (;;) {
		std::string line;
		getline(ifs, line);
		if (ifs.fail())
			throw std::runtime_error("premature end of " + fn);
		if (line.size() < 60 || line.substr(0, 2) != "CD")
			continue;
		if (sscanf(line.c_str(), "CD %lf %lf ", &m_thrustfuel[0],
			   &m_thrustfuel[1]) < 2)
			throw std::runtime_error("cannot parse climb fuel");
		break;
	}
	for (;;) {
		std::string line;
		getline(ifs, line);
		if (ifs.fail())
			throw std::runtime_error("premature end of " + fn);
		if (line.size() < 60 || line.substr(0, 2) != "CD")
			continue;
		if (sscanf(line.c_str(), "CD %lf %lf ", &m_descfuel[0],
			   &m_descfuel[1]) < 2)
			throw std::runtime_error("cannot parse descent fuel");
		break;
	}
	for (;;) {
		std::string line;
		getline(ifs, line);
		if (ifs.fail())
			throw std::runtime_error("premature end of " + fn);
		if (line.size() < 60 || line.substr(0, 2) != "CD")
			continue;
		if (sscanf(line.c_str(), "CD %lf  ", &m_cruisefuel) < 1)
			throw std::runtime_error("cannot parse cruise fuel");
		break;
	}
	for (;;) {
		std::string line;
		getline(ifs, line);
		if (ifs.fail())
			throw std::runtime_error("premature end of " + fn);
		if (line.size() < 60 || line.substr(0, 2) != "CD")
			continue;
		if (sscanf(line.c_str(), "CD %lf %lf %lf %lf ", &m_tol,
			   &m_ldl, &m_span, &m_length) < 4)
			throw std::runtime_error("cannot parse ground");
		break;
	}
	for (;;) {
		std::string line;
		getline(ifs, line);
		if (ifs.fail())
			throw std::runtime_error("premature end of " + fn);
		if (line.substr(0, 2) != "FI")
			continue;
		break;
	}
}

void OperationsPerformance::Aircraft::load_apf(const std::string& fn)
{
	std::ifstream ifs(fn.c_str());
	if (!ifs.good())
		throw std::runtime_error("Cannot open " + fn);
	for (;;) {
		std::string line;
		getline(ifs, line);
		if (ifs.fail())
			throw std::runtime_error("premature end of " + fn);
		if (line.size() < 60 || line.substr(0, 2) != "CD")
			continue;
		break;
	}
	for (unsigned int i = 0; i < 3;) {
		static const char *expmass[3] = { "LO", "AV", "HI" };
		std::string line;
		getline(ifs, line);
		if (ifs.fail())
			throw std::runtime_error("premature end of " + fn);
		if (line.size() < 60 || line.substr(0, 2) != "CD")
			continue;
		char version[14];
		char engines[10];
		char mass[10];
		double clcaslo, clcashi, clmc, crcaslo, crcashi, crmc, dmc, dcashi, dcaslo;
		clcaslo = clcashi = clmc = crcaslo = crcashi = crmc = dmc = dcashi =
			dcaslo = std::numeric_limits<double>::quiet_NaN();
		if (sscanf(line.c_str(), "CD%12c%8c %9s %lf %lf %lf %lf %lf %lf %lf %lf %lf ",
			   version, engines, mass, &clcaslo, &clcashi, &clmc, &crcaslo, &crcashi, &crmc,
			   &dmc, &dcashi, &dcaslo) < 12)
			throw std::runtime_error("cannot parse procedures");
		version[sizeof(version)-1] = 0;
		engines[sizeof(engines)-1] = 0;
		mass[sizeof(mass)-1] = 0;
		if (strcmp(mass, expmass[i]))
			std::cerr << get_basename() << ": unexpected procedure mass name" << std::endl;
		m_procs[i] = Procedure(clcaslo, clcashi, clmc * 0.01, crcaslo, crcashi, crmc * 0.01, dcaslo, dcashi, dmc * 0.01);
		++i;
	}
}

bool OperationsPerformance::Aircraft::compute(ComputeResult& res, compute_t mode) const
{
	static const double C_v_min    = .13000E+01;
	static const double C_v_min_to = .12000E+01;
	static const double V_cl_1     = .50000E+01;
	static const double V_cl_2     = .10000E+02;
	static const double V_cl_3     = .30000E+02;
	static const double V_cl_4     = .60000E+02;
	static const double V_cl_5     = .80000E+02;
	static const double V_cl_6     = .20000E+02;
	static const double V_cl_7     = .30000E+02;
	static const double V_cl_8     = .35000E+02;
	static const double V_des_1    = .50000E+01;
	static const double V_des_2    = .10000E+02;
	static const double V_des_3    = .20000E+02;
	static const double V_des_4    = .50000E+02;
	static const double V_des_5    = .50000E+01;
	static const double V_des_6    = .10000E+02;
	static const double V_des_7    = .20000E+02;

	AirData<float>& ad(res);
	// compute climb TAS
	bool constmach(false);
	unsigned int procidx(1);
	if (res.get_mass() < 0.5 * (get_massref() + get_massmin()))
		procidx = 0;
	else if (res.get_mass() > 0.5 * (get_massref() + get_massmax()))
		procidx = 2;
	const Procedure& proc(get_proc(procidx));
	if (mode == compute_descent) {
		const Config& cfg(get_config("LD"));
		double vstall(cfg.get_vstall() * sqrt(res.get_mass() / get_massref()));
		switch (get_propulsion()) {
		case propulsion_jet:
		case propulsion_turboprop:
			if (ad.get_pressure_altitude() < 1000)
				ad.set_cas(C_v_min * vstall + V_des_1);
			else if (ad.get_pressure_altitude() < 1500)
				ad.set_cas(C_v_min * vstall + V_des_2);
			else if (ad.get_pressure_altitude() < 2000)
				ad.set_cas(C_v_min * vstall + V_des_3);
			else if (ad.get_pressure_altitude() < 3000)
				ad.set_cas(C_v_min * vstall + V_des_4);
			else if (ad.get_pressure_altitude() < 5000)
				ad.set_cas(std::min(220., get_thrustdesccas()));
			else if (ad.get_pressure_altitude() < 10000)
				ad.set_cas(get_thrustdesccas());
			else
				ad.set_cas(0.5 * (proc.get_descentcaslo() + proc.get_descentcashi()));
			break;

		default:
			if (ad.get_pressure_altitude() < 500)
				ad.set_cas(C_v_min * vstall + V_des_5);
			else if (ad.get_pressure_altitude() < 1000)
				ad.set_cas(C_v_min * vstall + V_des_6);
			else if (ad.get_pressure_altitude() < 1500)
				ad.set_cas(C_v_min * vstall + V_des_7);
			else if (ad.get_pressure_altitude() < 10000)
				ad.set_cas(get_thrustdesccas());
			else
				ad.set_cas(0.5 * (proc.get_descentcaslo() + proc.get_descentcashi()));
			break;
		}
		if (ad.get_cas() > 250 && ad.get_pressure_altitude() < 10000)
			ad.set_cas(250);
		constmach = ad.get_mach() > proc.get_descentmc();
		if (constmach)
			ad.set_tas(proc.get_descentmc() * ad.get_lss());
	} else {
		if (mode == compute_climb) {
			const Config& cfg(get_config("TO"));
			double vstall(cfg.get_vstall() * sqrt(res.get_mass() / get_massref()));
			switch (get_propulsion()) {
			case propulsion_jet:
				if (ad.get_pressure_altitude() < 1500)
					ad.set_cas(std::min(C_v_min * vstall + V_cl_1, proc.get_climbcaslo()));
				else if (ad.get_pressure_altitude() < 3000)
					ad.set_cas(std::min(C_v_min * vstall + V_cl_2, proc.get_climbcaslo()));
				else if (ad.get_pressure_altitude() < 4000)
					ad.set_cas(std::min(C_v_min * vstall + V_cl_3, proc.get_climbcaslo()));
				else if (ad.get_pressure_altitude() < 5000)
					ad.set_cas(std::min(C_v_min * vstall + V_cl_4, proc.get_climbcaslo()));
				else if (ad.get_pressure_altitude() < 6000)
					ad.set_cas(std::min(C_v_min * vstall + V_cl_5, proc.get_climbcaslo()));
				else if (ad.get_pressure_altitude() < 10000)
					ad.set_cas(proc.get_climbcaslo());
				else
					ad.set_cas(proc.get_climbcashi());
				break;

			default:
				if (ad.get_pressure_altitude() < 500)
					ad.set_cas(std::min(C_v_min * vstall + V_cl_6, proc.get_climbcaslo()));
				else if (ad.get_pressure_altitude() < 1000)
					ad.set_cas(std::min(C_v_min * vstall + V_cl_7, proc.get_climbcaslo()));
				else if (ad.get_pressure_altitude() < 1500)
					ad.set_cas(std::min(C_v_min * vstall + V_cl_8, proc.get_climbcaslo()));
				else if (ad.get_pressure_altitude() < 10000)
					ad.set_cas(proc.get_climbcaslo());
				else
					ad.set_cas(proc.get_climbcashi());
				break;
			}
			if (ad.get_cas() > 250 && ad.get_pressure_altitude() < 10000)
				ad.set_cas(250);
			constmach = ad.get_mach() > proc.get_climbmc();
			if (constmach)
				ad.set_tas(proc.get_climbmc() * ad.get_lss());
		} else {
			switch (get_propulsion()) {
			case propulsion_jet:
				if (ad.get_pressure_altitude() < 3000)
					ad.set_cas(std::min(170., proc.get_cruisecaslo()));
				else if (ad.get_pressure_altitude() < 6000)
					ad.set_cas(std::min(220., proc.get_cruisecaslo()));
				else if (ad.get_pressure_altitude() < 14000)
					ad.set_cas(std::min(250., proc.get_cruisecaslo()));
				else
					ad.set_cas(proc.get_cruisecashi());
				break;

			default:
				if (ad.get_pressure_altitude() < 3000)
					ad.set_cas(std::min(150., proc.get_cruisecaslo()));
				else if (ad.get_pressure_altitude() < 6000)
					ad.set_cas(std::min(180., proc.get_cruisecaslo()));
				else if (ad.get_pressure_altitude() < 10000)
					ad.set_cas(std::min(250., proc.get_cruisecaslo()));
				else
					ad.set_cas(proc.get_cruisecashi());
				break;
			}
			if (ad.get_cas() > 250 && ad.get_pressure_altitude() < 10000)
				ad.set_cas(250);
			constmach = ad.get_mach() > proc.get_cruisemc();
			if (constmach)
				ad.set_tas(proc.get_cruisemc() * ad.get_lss());
			const Config& cfg(get_config("CR"));
			if (ad.get_mach() > get_mmo())
				ad.set_tas(get_mmo() * ad.get_lss());
			if (ad.get_cas() > get_vmo())
				ad.set_cas(get_vmo());
			if (ad.get_cas() < C_v_min * cfg.get_vstall())
				ad.set_cas(C_v_min * cfg.get_vstall());
		}
	}
	// maximum altitude
	const double oat(ad.get_oat() + IcaoAtmosphere<double>::degc_to_kelvin);
	double TmdTdivT((oat - ad.get_tempoffs()) / oat);
	double maxalt(get_maxalt());
	if (get_hmax() > 0)
		maxalt = std::min(maxalt, get_hmax() + get_tempgrad() * std::max(ad.get_tempoffs() - get_thrustclimb(3), 0.0)
				  + get_massgrad() * (get_massmax() - res.get_mass()));
	// Drag
	std::string cfgname("CR");
	double addcd0(0), addcd2(0);
	if (mode == compute_descent) {
		switch (get_propulsion()) {
		case propulsion_jet:
		case propulsion_turboprop:
			if (ad.get_pressure_altitude() < 1500) {
				cfgname = "LD";
				addcd0 = get_gearcd0();
				addcd2 = get_gearcd2();
			} else if (ad.get_pressure_altitude() < 3000) {
				cfgname = "AP";
			}
			break;

		default:
			if (ad.get_pressure_altitude() < 1000) {
				cfgname = "LD";
				addcd0 = get_gearcd0();
				addcd2 = get_gearcd2();
			} else if (ad.get_pressure_altitude() < 1500) {
				cfgname = "AP";
			}
			break;
		}
	}
	const Config& cfg(get_config(cfgname));
	{
		double rho(IcaoAtmosphere<float>::pressure_to_density(ad.get_p(), oat) * 0.1);
		double rtts(rho * ad.get_tas() * ad.get_tas() * get_wingsurface() * (1.0 / Point::mps_to_kts_dbl / Point::mps_to_kts_dbl));
		double cl((2 * IcaoAtmosphere<double>::const_g) * res.get_mass() / rtts);
		double cd(cfg.get_cd0() + addcd0 + (cfg.get_cd2() + addcd2) * cl * cl);
		res.set_drag(cd * rtts * 0.5);
	}
	double Cred(0);
	if (ad.get_pressure_altitude() < 0.8 * maxalt) {
		switch (get_propulsion()) {
		case propulsion_jet:
			Cred = 0.15;
			break;

		case propulsion_turboprop:
			Cred = 0.25;
			break;

		default:
			break;
		}
	}
	// compute engine thrust
	switch (get_propulsion()) {
	case propulsion_jet:
		res.set_thrust(get_thrustclimb(0) * (1 - ad.get_pressure_altitude() / get_thrustclimb(1) +
						     ad.get_pressure_altitude() * ad.get_pressure_altitude() * get_thrustclimb(2)));
		break;

	case propulsion_turboprop:
		res.set_thrust(get_thrustclimb(0) / ad.get_tas() * (1 - ad.get_pressure_altitude() / get_thrustclimb(1)) +
			       get_thrustclimb(2));
		break;

	case propulsion_piston:
		res.set_thrust(get_thrustclimb(0) * (1 - ad.get_pressure_altitude() / get_thrustclimb(1)) +
			       get_thrustclimb(2) / ad.get_tas());
		break;

	default:
		res.set_thrust(0);
		break;
	}
	if (mode == compute_descent) {
		switch (get_propulsion()) {
		case propulsion_jet:
		case propulsion_turboprop:
			if (ad.get_pressure_altitude() < 1500)
				res.set_thrust(res.get_thrust() * get_thrustdescldg());
			else if (ad.get_pressure_altitude() < 3000)
				res.set_thrust(res.get_thrust() * get_thrustdescapp());
			else if (ad.get_pressure_altitude() <= get_thrustdesclevel())
				res.set_thrust(res.get_thrust() * get_thrustdesclow());
			else
				res.set_thrust(res.get_thrust() * get_thrustdeschigh());
			break;

		default:
			if (ad.get_pressure_altitude() < 1000)
				res.set_thrust(res.get_thrust() * get_thrustdescldg());
			else if (ad.get_pressure_altitude() < 1500)
				res.set_thrust(res.get_thrust() * get_thrustdescapp());
			else if (ad.get_pressure_altitude() <= get_thrustdesclevel())
				res.set_thrust(res.get_thrust() * get_thrustdesclow());
			else
				res.set_thrust(res.get_thrust() * get_thrustdeschigh());
			break;
		}
	} else if (mode == compute_cruise) {
		res.set_thrust(res.get_drag());
	}
	// fuel consumption
	switch (get_propulsion()) {
	case propulsion_jet:
	case propulsion_turboprop:
	{
		double eta;
		if (get_propulsion() == propulsion_jet)
			eta = get_thrustfuel(0) * (1 + ad.get_tas() / get_thrustfuel(1));
		else
			eta = get_thrustfuel(0) * (1 - ad.get_tas() / get_thrustfuel(1)) * ad.get_tas() * 1e-3;
		double ff(res.get_thrust() * eta * 1e-3);
		double ffmin(get_descfuel(0) * (1 - ad.get_pressure_altitude() / get_descfuel(1)));
		res.set_fuelflow(std::max(ff, ffmin));
		break;
	}

	case propulsion_piston:
		res.set_fuelflow((mode == compute_descent) ? get_descfuel(0) : get_thrustfuel(0));
		break;

	default:
		res.set_fuelflow(0);
		break;
	}
	// RoCD
	bool stratosphere(ad.get_pressure_altitude() >= (11000 * Point::m_to_ft_dbl));
	// constants
	// kappa -> AirData<double>::gamma
	// R -> IcaoAtmosphere<double>::const_r / IcaoAtmosphere<double>::const_m * 1000
	// g0 -> IcaoAtmosphere<double>::const_g
	// beta_T,< -> -IcaoAtmosphere<double>::const_l0
	{
		static const double c0(AirData<double>::gamma * IcaoAtmosphere<double>::const_r / IcaoAtmosphere<double>::const_m * 1000 * -IcaoAtmosphere<double>::const_l0
				       * 0.5 / IcaoAtmosphere<double>::const_g);
		double y(1);
		if (!stratosphere)
			y += c0  * ad.get_mach() * ad.get_mach() * TmdTdivT;
		if (constmach) {
			res.set_esf(1.0 / y);
		} else {
			double x(1 + (AirData<double>::gamma - 1) * 0.5 * ad.get_mach() * ad.get_mach());
			res.set_esf(1.0 / (y + pow(x, -1 / (AirData<double>::gamma - 1)) * (pow(x, AirData<double>::gamma / (AirData<double>::gamma - 1)) - 1)));
		}
	}
	res.set_cpowred(1);
	if (mode == compute_climb)
		res.set_cpowred(1 - Cred * (get_massmax() - res.get_mass()) / (get_massmax() - get_massmin()));
	{
		static const double c0(Point::m_to_ft_dbl * 60 / Point::mps_to_kts_dbl / IcaoAtmosphere<double>::const_g);
		res.set_tdc((res.get_thrust() - res.get_drag()) * res.get_cpowred());
		res.set_rocd(c0 * TmdTdivT * res.get_tdc() * ad.get_tas() / res.get_mass() * res.get_esf());
	}
	{
		static const double c0(Point::mps_to_kts_dbl / 60 / Point::m_to_ft_dbl);
		res.set_gradient(atan(c0 * res.get_rocd() / ad.get_tas()) * (180.0 / M_PI));
	}
	return ad.get_pressure_altitude() <= maxalt;
}

OperationsPerformance::Synonym::Synonym(const std::string& icaotype, const std::string& basename, bool native, bool icao)
	: m_icaotype(icaotype), m_basename(basename), m_native(native), m_icao(icao)
{
}

const OperationsPerformance::Aircraft OperationsPerformance::invalidacft("", "INVALID AIRCRAFT");
const OperationsPerformance::Synonym OperationsPerformance::invalidsyn("", "INVALID", false, false);

OperationsPerformance::OperationsPerformance(void)
{
}

void OperationsPerformance::trim(std::string& x)
{
	std::string::size_type n(x.size());
	while (n > 0 && std::isspace(x[n - 1]))
		--n;
	x.resize(n);
}

void OperationsPerformance::trimfront(std::string& x)
{
	std::string::size_type n(0);
	while (n < x.size() && std::isspace(x[n]))
		++n;
	x.erase(0, n);
}

void OperationsPerformance::clear(void)
{
	m_aircraft.clear();
	m_synonyms.clear();
}

void OperationsPerformance::load(const std::string& dir)
{
	if (!load_synnew(dir) &&
	    !load_synlst(dir))
		throw std::runtime_error("Cannot open " + dir + "/SYNONYM.NEW or .LST");
}

bool OperationsPerformance::load_synlst(const std::string& dir)
{
	clear();
	std::ifstream ifs((dir + "/SYNONYM.LST").c_str());
	if (!ifs.good())
		return false;
	const Aircraft *lastacft(0);
	for (;;) {
		std::string line;
		getline(ifs, line);
		if (ifs.fail())
			break;
		while (!line.empty() && line[line.size() - 1] == '\r')
			line.resize(line.size() - 1);
		if (line.empty())
			lastacft = 0;
		if (line.size() >= 53 && line[0] == ' ' && line[2] == ' ') {
			if (line[1] == '-') {
				std::string desc(line, 11, 20);
				std::string basefile(line, 32, 8);
				trim(desc);
				trim(basefile);
				lastacft = 0;
				if (!desc.empty() && !basefile.empty()) {
					std::pair<aircraft_t::iterator,bool> ins(m_aircraft.insert(Aircraft(basefile, desc)));
					if (ins.second) {
						lastacft = &*ins.first;
						try {
							const_cast<Aircraft&>(*ins.first).load(dir);
						} catch (const std::exception& e) {
							if (true)
								std::cerr << "Cannot load aircraft " << lastacft->get_basename()
									  << ": " << e.what() << std::endl;
							m_aircraft.erase(ins.first);
							lastacft = 0;
						}
					}
				}
			}
			if (lastacft) {
				bool native(true);
				std::string::const_iterator si(line.begin() + 53), se(line.end());
				for (;;) {
					while (si != se && std::isspace(*si))
						++si;
					if (si == se)
						break;
					std::string::const_iterator si2(si + 1);
					while (si2 != se && !std::isspace(*si2))
						++si2;
					{
						synonyms_t::value_type v(std::string(si, si2), lastacft->get_basename(), native, false);
						native = false;
						std::pair<synonyms_t::iterator,bool> ins(m_synonyms.insert(v));
						if (true && !ins.second)
							std::cerr << "OperationsPerformance: duplicate synonym " << v.get_icaotype() << std::endl;
					}
					si = si2;
					if (si == se)
						break;
					++si;
				}
			}
		}
		if (false)
			std::cerr << "Line: \"" << line << "\"" << std::endl;
		if (!ifs.good())
			break;
	}
	return true;
}

bool OperationsPerformance::load_synnew(const std::string& dir)
{
	clear();
	std::ifstream ifs((dir + "/SYNONYM.NEW").c_str());
	if (!ifs.good())
		return false;
	for (;;) {
		std::string line;
		getline(ifs, line);
		if (ifs.fail())
			break;
		while (!line.empty() && line[line.size() - 1] == '\r')
			line.resize(line.size() - 1);
		if (line.size() >= 70 && line[0] == 'C' && line[1] == 'D') {
			std::string accode(line, 5, 4);
			std::string mfg(line, 12, 20);
			std::string mdl(line, 32, 25);
			std::string basefile(line, 57, 8);
			bool icao(line[65] == 'Y');
			bool hasmdl(line[3] == '-');
			trim(accode);
			trim(mfg);
			trim(mdl);
			trim(basefile);
			if ((!mfg.empty() || !mdl.empty()) && !basefile.empty()) {
				std::pair<aircraft_t::iterator,bool> ins;
				{
					std::string desc(mfg);
					if (!desc.empty() && !mdl.empty())
						desc += " ";
					desc += mdl;
					ins = m_aircraft.insert(Aircraft(basefile, desc));
				}
				const Aircraft *acft(&*ins.first);
				if (ins.second) {
					try {
						const_cast<Aircraft&>(*ins.first).load(dir);
					} catch (const std::exception& e) {
						if (true)
							std::cerr << "Cannot load aircraft " << acft->get_basename()
								  << ": " << e.what() << std::endl;
						m_aircraft.erase(ins.first);
						acft = 0;
					}
				}
				if (acft && !accode.empty()) {
					synonyms_t::value_type v(accode, acft->get_basename(), hasmdl, icao);
					std::pair<synonyms_t::iterator,bool> ins(m_synonyms.insert(v));
					if (true && !ins.second)
						std::cerr << "OperationsPerformance: duplicate synonym " << v.get_icaotype() << std::endl;
				}
			}
		}
		if (false)
			std::cerr << "Line: \"" << line << "\"" << std::endl;
		if (!ifs.good())
			break;
	}
	return true;
}

OperationsPerformance::stringset_t OperationsPerformance::list_aircraft(void) const
{
	stringset_t r;
	for (synonyms_t::const_iterator i(m_synonyms.begin()), e(m_synonyms.end()); i != e; ++i)
		r.insert(i->get_icaotype());
	return r;
}

const OperationsPerformance::Aircraft& OperationsPerformance::find_aircraft(const std::string& name) const
{
	if (name.empty())
		return  invalidacft;
	const Synonym& syn(find_synonym(name));
	if (!syn.is_valid())
		return  invalidacft;
	return find_basename(syn.get_basename());
}

const OperationsPerformance::Aircraft& OperationsPerformance::find_basename(const std::string& basename) const
{
	if (basename.empty())
		return invalidacft;
	aircraft_t::const_iterator i(m_aircraft.find(Aircraft(basename)));
	if (i != m_aircraft.end())
		return *i;
	return invalidacft;
}

const OperationsPerformance::Synonym& OperationsPerformance::find_synonym(const std::string& name) const
{
	if (name.empty())
		return invalidsyn;
	synonyms_t::const_iterator i(m_synonyms.find(Synonym(name)));
	if (i != m_synonyms.end())
		return *i;
	return invalidsyn;
}

const std::string& to_str(OperationsPerformance::Aircraft::propulsion_t p)
{
	switch (p) {
	case OperationsPerformance::Aircraft::propulsion_piston:
	{
		static const std::string r("piston");
		return r;
	}

	case OperationsPerformance::Aircraft::propulsion_turboprop:
	{
		static const std::string r("turboprop");
		return r;
	}

	case OperationsPerformance::Aircraft::propulsion_jet:
	{
		static const std::string r("jet");
		return r;
	}

	default:
	{
		static const std::string r("invalid");
		return r;
	}
	}
}
