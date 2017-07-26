//
// C++ Interface: opsperf
//
// Description: Aircraft Operations Performance Model
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2014, 2015, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef OPSPERF_H
#define OPSPERF_H

#include "sysdeps.h"

#include <limits>
#include <set>
#include <glibmm.h>

#include "airdata.h"

class OperationsPerformance {
public:
	class Aircraft {
	public:
		class Config {
		public:
			Config(const std::string& phase = "", const std::string& name = "",
			       double vstall = std::numeric_limits<double>::quiet_NaN(),
			       double cd0 = std::numeric_limits<double>::quiet_NaN(),
			       double cd2 = std::numeric_limits<double>::quiet_NaN());

			const std::string& get_phase(void) const { return m_phase; }
			const std::string& get_name(void) const { return m_name; }
			double get_vstall(void) const { return m_vstall; }
			double get_cd0(void) const { return m_cd0; }
			double get_cd2(void) const { return m_cd2; }

			bool operator<(const Config& x) const { return get_phase() < x.get_phase(); }
			bool operator<=(const Config& x) const { return get_phase() <= x.get_phase(); }
			bool operator>(const Config& x) const { return get_phase() > x.get_phase(); }
			bool operator>=(const Config& x) const { return get_phase() >= x.get_phase(); }
			bool operator==(const Config& x) const { return get_phase() == x.get_phase(); }
			bool operator!=(const Config& x) const { return get_phase() != x.get_phase(); }

		protected:
			std::string m_phase;
			std::string m_name;
			double m_vstall;
			double m_cd0;
			double m_cd2;
		};

		class Procedure {
		public:
			Procedure(double clcaslo = std::numeric_limits<double>::quiet_NaN(),
				  double clcashi = std::numeric_limits<double>::quiet_NaN(),
				  double clmc = std::numeric_limits<double>::quiet_NaN(),
				  double crcaslo = std::numeric_limits<double>::quiet_NaN(),
				  double crcashi = std::numeric_limits<double>::quiet_NaN(),
				  double crmc = std::numeric_limits<double>::quiet_NaN(),
				  double dcaslo = std::numeric_limits<double>::quiet_NaN(),
				  double dcashi = std::numeric_limits<double>::quiet_NaN(),
				  double dmc = std::numeric_limits<double>::quiet_NaN());

			double get_climbcaslo(void) const { return m_climbcaslo; }
			double get_climbcashi(void) const { return m_climbcashi; }
			double get_climbmc(void) const { return m_climbmc; }
			double get_cruisecaslo(void) const { return m_cruisecaslo; }
			double get_cruisecashi(void) const { return m_cruisecashi; }
			double get_cruisemc(void) const { return m_cruisemc; }
			double get_descentcaslo(void) const { return m_descentcaslo; }
			double get_descentcashi(void) const { return m_descentcashi; }
			double get_descentmc(void) const { return m_descentmc; }

		protected:
			double m_climbcaslo;
			double m_climbcashi;
			double m_climbmc;
			double m_cruisecaslo;
			double m_cruisecashi;
			double m_cruisemc;
			double m_descentcaslo;
			double m_descentcashi;
			double m_descentmc;
		};

		class ComputeResult : public AirData<float> {
		public:
			ComputeResult(double mass = 0, double thrust = 0, double drag = 0,
				      double ff = 0, double esf = 0, double rocd = 0,
				      double tdc = 0, double cpowred = 0, double grad = 0);

			double get_mass(void) const { return m_mass; }
			double get_thrust(void) const { return m_thrust; }
			double get_drag(void) const { return m_drag; }
			double get_fuelflow(void) const { return m_fuelflow; }
			double get_esf(void) const { return m_esf; }
			double get_rocd(void) const { return m_rocd; }
			double get_tdc(void) const { return m_tdc; }
			double get_cpowred(void) const { return m_cpowred; }
			double get_gradient(void) const { return m_gradient; }

			void set_mass(double x) { m_mass = x; }
			void set_thrust(double x) { m_thrust = x; }
			void set_drag(double x) { m_drag = x; }
			void set_fuelflow(double x) { m_fuelflow = x; }
			void set_esf(double x) { m_esf = x; }
			void set_rocd(double x) { m_rocd = x; }
			void set_tdc(double x) { m_tdc = x; }
			void set_cpowred(double x) { m_cpowred = x; }
			void set_gradient(double x) { m_gradient = x; }

		protected:
			double m_mass;     // kg
			double m_thrust;   // N
			double m_drag;     // N
			double m_fuelflow; // kg/min
			double m_esf;      // energy share factor
			double m_rocd;     // ft/min
			double m_tdc;      // kg/min
			double m_cpowred;  // power reduction coefficient
			double m_gradient; // degree
		};

		typedef enum {
			propulsion_invalid,
			propulsion_piston,
			propulsion_turboprop,
			propulsion_jet
		} propulsion_t;

		typedef enum {
			compute_climb,
			compute_cruise,
			compute_descent
		} compute_t;

		Aircraft(const std::string& basename = "", const std::string& desc = "");

		const std::string& get_basename(void) const { return m_basename; }
		const std::string& get_description(void) const { return m_description; }
		const std::string& get_acfttype(void) const { return m_acfttype; }
		const std::string& get_enginetype(void) const { return m_enginetype; }
		unsigned int get_nrengines(void) const { return m_nrengines; }
		propulsion_t get_propulsion(void) const { return m_propulsion; }
		char get_wakecategory(void) const { return m_wakecategory; }

		// Mass
		double get_massref(void) const { return m_massref; }
		double get_massmin(void) const { return m_massmin; }
		double get_massmax(void) const { return m_massmax; }
		double get_massmaxpayload(void) const { return m_massmaxpayload; }
		double get_massgrad(void) const { return m_massgrad; }

		// Envelope
		double get_vmo(void) const { return m_vmo; }
		double get_mmo(void) const { return m_mmo; }
		double get_maxalt(void) const { return m_maxalt; }
		double get_hmax(void) const { return m_hmax; }
		double get_tempgrad(void) const { return m_tempgrad; }

		// Wing
		double get_wingsurface(void) const { return m_wingsurface; }
		double get_clbo(void) const { return m_clbo; }
		double get_k(void) const { return m_k; }
		double get_cm16(void) const { return m_cm16; }

		// Configuration Accessories
		double get_spoilercd2(void) const { return m_spoilercd2; }
		double get_gearcd0(void) const { return m_gearcd0; }
		double get_gearcd2(void) const { return m_gearcd2; }
		double get_brakecd2(void) const { return m_brakecd2; }

		// Thrust and fuel
		double get_thrustclimb(unsigned int idx) const { return (idx < 5) ? m_thrustclimb[idx] : std::numeric_limits<double>::quiet_NaN(); }
		double get_thrustdesclow(void) const { return m_thrustdesclow; }
		double get_thrustdeschigh(void) const { return m_thrustdeschigh; }
		double get_thrustdesclevel(void) const { return m_thrustdesclevel; }
		double get_thrustdescapp(void) const { return m_thrustdescapp; }
		double get_thrustdescldg(void) const { return m_thrustdescldg; }
		double get_thrustdesccas(void) const { return m_thrustdesccas; }
		double get_thrustdescmach(void) const { return m_thrustdescmach; }
		double get_thrustfuel(unsigned int idx) const { return (idx < 2) ? m_thrustfuel[idx] : std::numeric_limits<double>::quiet_NaN(); }
		double get_descfuel(unsigned int idx) const { return (idx < 2) ? m_descfuel[idx] : std::numeric_limits<double>::quiet_NaN(); }
		double get_cruisefuel(void) const { return m_cruisefuel; }

		// Ground
		double get_tol(void) const { return m_tol; }
		double get_ldl(void) const { return m_ldl; }
		double get_span(void) const { return m_span; }
		double get_length(void) const { return m_length; }

		const Procedure& get_proc(unsigned int idx) const { return m_procs[std::min(2U, idx)]; }
		const Config& get_config(const std::string& phase) const;

		bool is_valid(void) const { return !get_basename().empty(); }
		bool operator<(const Aircraft& x) const { return get_basename() < x.get_basename(); }
		bool operator<=(const Aircraft& x) const { return get_basename() <= x.get_basename(); }
		bool operator>(const Aircraft& x) const { return get_basename() > x.get_basename(); }
		bool operator>=(const Aircraft& x) const { return get_basename() >= x.get_basename(); }
		bool operator==(const Aircraft& x) const { return get_basename() == x.get_basename(); }
		bool operator!=(const Aircraft& x) const { return get_basename() != x.get_basename(); }

		void load(const std::string& dir = PACKAGE_DATA_DIR "/opsperf");

		bool compute(ComputeResult& res, compute_t mode) const;

	protected:
		typedef std::set<Config> configs_t;
		configs_t m_configs;
		Procedure m_procs[3];
		std::string m_basename;
		std::string m_description;
		std::string m_acfttype;
		std::string m_enginetype;
		double m_massref;
		double m_massmin;
		double m_massmax;
		double m_massmaxpayload;
		double m_massgrad;
		double m_vmo;
		double m_mmo;
		double m_maxalt;
		double m_hmax;
		double m_tempgrad;
		double m_wingsurface;
		double m_clbo;
		double m_k;
		double m_cm16;
		double m_spoilercd2;
		double m_gearcd0;
		double m_gearcd2;
		double m_brakecd2;
		double m_thrustclimb[5];
		double m_thrustdesclow;
		double m_thrustdeschigh;
		double m_thrustdesclevel;
		double m_thrustdescapp;
		double m_thrustdescldg;
		double m_thrustdesccas;
		double m_thrustdescmach;
		double m_thrustfuel[2];
		double m_descfuel[2];
		double m_cruisefuel;
		double m_tol;
		double m_ldl;
		double m_span;
		double m_length;
		unsigned int m_nrengines;
		propulsion_t m_propulsion;
		char m_wakecategory;

		void load_opf(const std::string& fn);
		void load_apf(const std::string& fn);
	};

	class Synonym {
	public:
		Synonym(const std::string& icaotype = "", const std::string& basename = "", bool native = false, bool icao = false);
		const std::string& get_icaotype(void) const { return m_icaotype; }
		const std::string& get_basename(void) const { return m_basename; }
		bool is_native(void) const { return m_native; }
		bool is_icao(void) const { return m_icao; }
		bool is_valid(void) const { return !get_basename().empty(); }

		bool operator<(const Synonym& x) const { return get_icaotype() < x.get_icaotype(); }
		bool operator<=(const Synonym& x) const { return get_icaotype() <= x.get_icaotype(); }
		bool operator>(const Synonym& x) const { return get_icaotype() > x.get_icaotype(); }
		bool operator>=(const Synonym& x) const { return get_icaotype() >= x.get_icaotype(); }
		bool operator==(const Synonym& x) const { return get_icaotype() == x.get_icaotype(); }
		bool operator!=(const Synonym& x) const { return get_icaotype() != x.get_icaotype(); }

	protected:
		std::string m_icaotype;
		std::string m_basename;
		bool m_native;
		bool m_icao;
	};

	OperationsPerformance(void);
	void clear(void);
	void load(const std::string& dir = PACKAGE_DATA_DIR "/opsperf");
	typedef std::set<std::string> stringset_t;
	stringset_t list_aircraft(void) const;
	const Aircraft& find_aircraft(const std::string& name) const;
	const Aircraft& find_basename(const std::string& basename) const;
	const Synonym& find_synonym(const std::string& name) const;

protected:
	static const Aircraft invalidacft;
	static const Synonym invalidsyn;
	typedef std::set<Aircraft> aircraft_t;
	aircraft_t m_aircraft;
	typedef std::set<Synonym> synonyms_t;
	synonyms_t m_synonyms;

	static void trim(std::string& x);
	static void trimfront(std::string& x);
	bool load_synlst(const std::string& dir = PACKAGE_DATA_DIR "/opsperf");
	bool load_synnew(const std::string& dir = PACKAGE_DATA_DIR "/opsperf");
};

const std::string& to_str(OperationsPerformance::Aircraft::propulsion_t p);
inline std::ostream& operator<<(std::ostream& os, OperationsPerformance::Aircraft::propulsion_t p) { return os << to_str(p); }


#endif /* OPSPERF_H */
