//
// C++ Interface: aircraft
//
// Description: Aircraft Model
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2012, 2013, 2014, 2015, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef AIRCRAFT_H
#define AIRCRAFT_H

#include "sysdeps.h"

#include "opsperf.h"

#include <limits>
#include <cmath>
#include <set>
#include <glibmm.h>
#include <libxml++/libxml++.h>

class FPlanRoute;
class FPlanWaypoint;
class FPlanAlternate;
class Engine;
template <class T> class DbQueryInterface;
namespace DbBaseElements {
	class Airport;
};

#ifdef HAVE_JSONCPP
namespace Json {
class Value;
};
#endif

class Aircraft {
public:
	typedef enum {
		// length units
		unit_m,
		unit_cm,
		unit_km,
		unit_in,
		unit_ft,
		unit_nmi,
		// mass units
		unit_kg,
		unit_lb,
		// volume units
		unit_liter,
		unit_usgal,
		unit_quart,
		// end
		unit_invalid
	} unit_t;

	typedef enum {
		unitmask_m       = 1 << unit_m,
		unitmask_cm      = 1 << unit_cm,
		unitmask_km      = 1 << unit_km,
		unitmask_in      = 1 << unit_in,
		unitmask_ft      = 1 << unit_ft,
		unitmask_nmi     = 1 << unit_nmi,
		unitmask_kg      = 1 << unit_kg,
		unitmask_lb      = 1 << unit_lb,
		unitmask_liter   = 1 << unit_liter,
		unitmask_usgal   = 1 << unit_usgal,
		unitmask_quart   = 1 << unit_quart,
		unitmask_length  = unitmask_m | unitmask_cm | unitmask_km | unitmask_in | unitmask_ft | unitmask_nmi,
		unitmask_mass    = unitmask_kg | unitmask_lb,
		unitmask_volume  = unitmask_liter | unitmask_usgal | unitmask_quart,
		unitmask_valid   = unitmask_length | unitmask_mass | unitmask_volume,
		unitmask_invalid = 1 << unit_invalid
	} unitmask_t;

	typedef enum {
		propulsion_fixedpitch,
		propulsion_constantspeed,
		propulsion_turboprop, // future extension
		propulsion_turbojet // future extension
	} propulsion_t;

	typedef enum {
		opsrules_auto,
		opsrules_easacat,
		opsrules_easanco,
		opsrules_last = opsrules_easanco
	} opsrules_t;

	class CheckErrors;

protected:
	template <typename T> class Poly1D : public std::vector<T> {
	public:
		class XY {
		public:
			XY(T x = std::numeric_limits<T>::quiet_NaN(), T y = std::numeric_limits<T>::quiet_NaN()) : m_x(x), m_y(y) {}
			T get_x(void) const { return m_x; }
			T get_y(void) const { return m_y; }
			void set_x(T v) { m_x = v; }
			void set_y(T v) { m_y = v; }
			void set_x_nan(void) { m_x = std::numeric_limits<T>::quiet_NaN(); }
			void set_y_nan(void) { m_y = std::numeric_limits<T>::quiet_NaN(); }
			void set_x_max(void) { m_x = std::numeric_limits<T>::max(); }
			void set_y_max(void) { m_y = std::numeric_limits<T>::max(); }
			void set_x_min(void) { m_x = std::numeric_limits<T>::min(); }
			void set_y_min(void) { m_y = std::numeric_limits<T>::min(); }

		protected:
			T m_x;
			T m_y;
		};

		Poly1D(void) : std::vector<T>() {}
		Poly1D(typename Poly1D<T>::size_type n) : std::vector<T>(n) {}
		Poly1D(typename Poly1D<T>::size_type n, const T& t) : std::vector<T>(n, t) {}
		Poly1D(const std::vector<T>& t) : std::vector<T>(t) {}
		template <class InputIterator> Poly1D(InputIterator a, InputIterator b) : std::vector<T>(a, b) {}

		T eval(T x) const;
		T evalderiv(T x) const;
		T evalderiv2(T x) const;
		T inveval(T x, T tol, unsigned int maxiter, T t) const;
		T inveval(T x, T tol, unsigned int maxiter = 16) const;
		T inveval(T x) const;
		T boundedinveval(T x, T blo, T bhi, T tol, unsigned int maxiter = 16) const;
		T boundedinveval(T x, T blo, T bhi) const;

		void eval(XY& x) const;
		void evalderiv(XY& x) const;
		void evalderiv2(XY& x) const;
		void inveval(XY& x, T tol, unsigned int maxiter, T t) const;
		void inveval(XY& x, T tol, unsigned int maxiter = 16) const;
		void inveval(XY& x) const;
		void boundedinveval(XY& x, T blo, T bhi, T tol, unsigned int maxiter = 16) const;
		void boundedinveval(XY& x, T blo, T bhi) const;

		void reduce(void);

		Poly1D<T>& operator+=(T x);
		Poly1D<T>& operator-=(T x);
		Poly1D<T>& operator*=(T x);
		Poly1D<T>& operator/=(T x);
		Poly1D<T>& operator+=(const Poly1D<T>& x);
		Poly1D<T>& operator-=(const Poly1D<T>& x);
		Poly1D<T> operator*(const Poly1D<T>& x) const;

		inline Poly1D<T> operator+(T x) const { Poly1D<T> y(*this); y += x; return y; }
		inline Poly1D<T> operator-(T x) const { Poly1D<T> y(*this); y -= x; return y; }
		inline Poly1D<T> operator*(T x) const { Poly1D<T> y(*this); y *= x; return y; }
		inline Poly1D<T> operator/(T x) const { Poly1D<T> y(*this); y /= x; return y; }
		inline Poly1D<T> operator+(const Poly1D<T>& x) const { Poly1D<T> y(*this); y += x; return y; }
		inline Poly1D<T> operator-(const Poly1D<T>& x) const { Poly1D<T> y(*this); y -= x; return y; }
		inline Poly1D<T>& operator*=(const Poly1D<T>& x) { Poly1D<T> y((*this) * x); this->swap(y); return *this; }

		Poly1D<T> differentiate(void) const;
		Poly1D<T> integrate(void) const;
		Poly1D<T> convert(unit_t ufrom, unit_t uto) const;
		Poly1D<T> simplify(double absacc, double relacc, double invmaxmag) const;

		class MinMax {
		protected:
			struct CmpX {
				bool operator()(const XY& a, const XY& b) const {
					return a.get_x() < b.get_x();
				}
			};

		public:
			MinMax(void) {}
			const XY& get_glbmin(void) const { return m_glbmin; }
			const XY& get_glbmax(void) const { return m_glbmax; }
			typedef std::set<XY,CmpX> local_t;
			const local_t& get_lclmin(void) const { return m_lclmin; }
			const local_t& get_lclmax(void) const { return m_lclmax; }

		protected:
			XY m_glbmin;
			XY m_glbmax;
			local_t m_lclmin;
			local_t m_lclmax;

			friend class Poly1D;
		};

		MinMax get_minmax(T dmin, T dmax) const;

		void load_xml(const xmlpp::Element *el);
		void save_xml(xmlpp::Element *el) const;

		std::ostream& save_octave(std::ostream& os, const Glib::ustring& name) const;

		std::ostream& print(std::ostream& os) const;

		template<class Archive> void hibernate(Archive& ar) {
			uint32_t n(this->size());
			ar.ioleb(n);
			if (ar.is_load())
				this->resize(n);
			for (typename Poly1D<T>::iterator i(this->begin()), e(this->end()); i != e; ++i)
				ar.io(*i);
		}
	};

public:
	class WeightBalance {
	public:
		class Element {
		public:
			class Unit {
			public:
				Unit(const Glib::ustring& name = "", double factor = 1, double offset = 0);

				const Glib::ustring& get_name(void) const { return m_name; }
				double get_factor(void) const { return m_factor; }
				double get_offset(void) const { return m_offset; }
				bool is_fixed(void) const { return get_factor() == 0; }

				void load_xml(const xmlpp::Element *el);
				void save_xml(xmlpp::Element *el) const;

				template<class Archive> void hibernate(Archive& ar) {
					ar.io(m_name);
					ar.io(m_factor);
					ar.io(m_offset);
				}

			protected:
				Glib::ustring m_name;
				double m_factor;
				double m_offset;
			};

			typedef enum {
				flag_none = 0,
				flag_fixed = (1 << 0),
				flag_binary = (1 << 1),
				flag_avgas = (1 << 2),
				flag_jeta = (1 << 3),
				flag_diesel = (1 << 4),
				flag_oil = (1 << 5),
				flag_gear = (1 << 6),
				flag_consumable = (1 << 7),
				flag_fuel = flag_avgas | flag_jeta | flag_diesel,
				flag_all = flag_fixed | flag_binary | flag_fuel | flag_oil | flag_gear | flag_consumable
			} flags_t;

			Element(const Glib::ustring& name = "", double massmin = 0, double massmax = 0, double arm = 0, flags_t flags = flag_none);
			const Glib::ustring& get_name(void) const { return m_name; }
			double get_massmin(void) const { return m_massmin; }
			double get_massmax(void) const { return m_massmax; }
			double get_massdiff(void) const { return m_massmax - m_massmin; }
			double get_arm(double mass) const;
			std::pair<double,double> get_piecelimits(double mass, bool rounddown = false) const;
			double get_moment(double mass) const { return get_arm(mass) * mass; }
			flags_t get_flags(void) const { return m_flags; }

			unsigned int get_nrunits(void) const { return m_units.size(); }
			const Unit& get_unit(unsigned int idx) const { return m_units[idx]; }

			void clear_units(void);
			void add_unit(const Glib::ustring& name = "", double factor = 1, double offset = 0);
			void add_auto_units(unit_t massunit, unit_t fuelunit, unit_t oilunit);

			void load_xml(const xmlpp::Element *el);
			void save_xml(xmlpp::Element *el) const;

			template<class Archive> void hibernate(Archive& ar) {
				uint32_t n(m_units.size());
				ar.ioleb(n);
				if (ar.is_load())
					m_units.resize(n);
				for (units_t::iterator i(m_units.begin()), e(m_units.end()); i != e; ++i)
					i->hibernate(ar);
				if (ar.is_load()) {
					m_arms.clear();
					uint32_t sz(0);
					ar.ioleb(sz);
					for (; sz > 0; --sz) {
						double m, a;
						ar.io(m);
						ar.io(a);
						m_arms[m] = a;
					}
				} else {
					uint32_t sz(m_arms.size());
					ar.ioleb(sz);
					for (arms_t::const_iterator ai(m_arms.begin()), ae(m_arms.end()); ai != ae && sz > 0; ++ai, --sz) {
						double m(ai->first), a(ai->second);
						ar.io(m);
						ar.io(a);
					}
				}
				ar.io(m_name);
				ar.io(m_massmin);
				ar.io(m_massmax);
				ar.iouint8(m_flags);
			}

		protected:
			class FlagName {
			public:
				const char *m_name;
				flags_t m_flag;
			};
			static const FlagName flagnames[];

			typedef std::vector<Unit> units_t;
			units_t m_units;
			typedef std::map<double,double> arms_t;
			arms_t m_arms;
			Glib::ustring m_name;
			double m_massmin;
			double m_massmax;
			flags_t m_flags;
		};

		class ElementValue {
		public:
			ElementValue(double value = 0, unsigned int unit = 0) : m_value(value), m_unit(unit) {}
			double get_value(void) const { return m_value; }
			unsigned int get_unit(void) const { return m_unit; }
			void set_value(double v) { m_value = v; }
			void set_unit(unsigned int u) { m_unit = u; }

			template<class Archive> void hibernate(Archive& ar) {
				ar.io(m_value);
				ar.ioleb(m_unit);
			}

		protected:
			double m_value;
			unsigned int m_unit;
		};

		typedef std::vector<ElementValue> elementvalues_t;

		class Envelope {
		public:
			class Point {
			public:
				Point(double arm = 0, double mass = 0);
				double get_arm(void) const { return m_arm; }
				double get_mass(void) const { return m_mass; }

				static double area2(const Point& p0, const Point& p1, const Point& p2);
				double area2(const Point& p0, const Point& p1) const { return area2(p0, p1, *this); }

				void load_xml(const xmlpp::Element *el);
				void save_xml(xmlpp::Element *el) const;

				template<class Archive> void hibernate(Archive& ar) {
					ar.io(m_arm);
					ar.io(m_mass);
				}

			protected:
				double m_arm;
				double m_mass;
			};

			Envelope(const Glib::ustring& name = "");

			const Glib::ustring& get_name(void) const { return m_name; }

			unsigned int size(void) const { return m_env.size(); }
			const Point& operator[](unsigned int idx) const { return m_env[idx]; }
			int windingnumber(const Point& pt) const;
			bool is_inside(const Point& pt) const { return !!windingnumber(pt); }
			void get_bounds(double& minarm, double& maxarm, double& minmass, double& maxmass) const;

			void load_xml(const xmlpp::Element *el);
			void save_xml(xmlpp::Element *el) const;

			void add_point(const Point& pt);
			void add_point(double arm = 0, double mass = 0);

		public:
			template<class Archive> void hibernate(Archive& ar) {
				uint32_t n(m_env.size());
				ar.ioleb(n);
				if (ar.is_load())
					m_env.resize(n);
				for (env_t::iterator i(m_env.begin()), e(m_env.end()); i != e; ++i)
					i->hibernate(ar);
				ar.io(m_name);
			}

		protected:
			typedef std::vector<Point> env_t;
			env_t m_env;
			Glib::ustring m_name;
		};

		WeightBalance(void);

		unit_t get_armunit(void) const { return m_armunit; }
		unit_t get_massunit(void) const { return m_massunit; }

		const std::string& get_armunitname(void) const;
		const std::string& get_massunitname(void) const;

		unsigned int get_nrelements(void) const { return m_elements.size(); }
		const Element& get_element(unsigned int idx) const { return m_elements[idx]; }
		void clear_elements(void);
		void add_element(const Element& el);

		unsigned int get_nrenvelopes(void) const { return m_envelopes.size(); }
		const Envelope& get_envelope(unsigned int idx) const { return m_envelopes[idx]; }
		void clear_envelopes(void);
		void add_envelope(const Envelope& el);

		void load_xml(const xmlpp::Element *el);
		void save_xml(xmlpp::Element *el) const;

		double get_useable_fuelmass(void) const;
		double get_total_fuelmass(void) const;

		double get_useable_oilmass(void) const;
		double get_total_oilmass(void) const;

		double get_min_mass(void) const;

		Element::flags_t get_element_flags(void) const;

		bool add_auto_units(unit_t fuelunit, unit_t oilunit);
		void set_units(unit_t armunit = unit_in, unit_t massunit = unit_lb);

		void limit(elementvalues_t& ev) const;

		const std::string& get_remark(void) const { return m_remark; }

		template<class Archive> void hibernate(Archive& ar) {
			uint32_t n(m_elements.size());
			ar.ioleb(n);
			if (ar.is_load())
				m_elements.resize(n);
			for (elements_t::iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i)
				i->hibernate(ar);
			n = m_envelopes.size();
			ar.ioleb(n);
			if (ar.is_load())
				m_envelopes.resize(n);
			for (envelopes_t::iterator i(m_envelopes.begin()), e(m_envelopes.end()); i != e; ++i)
				i->hibernate(ar);
			ar.iouint8(m_armunit);
			ar.iouint8(m_massunit);
			ar.io(m_remark);
		}

	protected:
		typedef std::vector<Element> elements_t;
		elements_t m_elements;
		typedef std::vector<Envelope> envelopes_t;
		envelopes_t m_envelopes;
		std::string m_remark;
		unit_t m_armunit;
		unit_t m_massunit;
	};

	class Distances {
	public:
		class Distance {
		public:
			class Point {
			public:
				Point(double da = 0, double pa = 0, double temp = 0, double gnddist = 0, double obstdist = 0);
				
				void load_xml(const xmlpp::Element *el, double altfactor = 1, double tempfactor = 1,
					      double tempoffset = 273.15, double distfactor = 1);
				void save_xml(xmlpp::Element *el, double altfactor = 1, double tempfactor = 1,
					      double tempoffset = 273.15, double distfactor = 1) const;

				double get_densityalt(void) const;
				double get_gnddist(void) const { return m_gnddist; }
				double get_obstdist(void) const { return m_obstdist; }

				template<class Archive> void hibernate(Archive& ar) {
					ar.io(m_da);
					ar.io(m_pa);
					ar.io(m_temp);
					ar.io(m_gnddist);
					ar.io(m_obstdist);
				}

			protected:
				double m_da;
				double m_pa;
				double m_temp;
				double m_gnddist;
				double m_obstdist;
			};

			Distance(const Glib::ustring& name = "", double vrotate = 0, double mass = 0);

			bool recalculatepoly(bool force = false);
			void add_point_da(double gnddist, double obstdist, double da);
			void add_point_pa(double gnddist, double obstdist, double pa, double temp = std::numeric_limits<double>::quiet_NaN());

			void load_xml(const xmlpp::Element *el, double altfactor = 1, double tempfactor = 1,
				      double tempoffset = 273.15, double distfactor = 1);
			void save_xml(xmlpp::Element *el, double altfactor = 1, double tempfactor = 1,
				      double tempoffset = 273.15, double distfactor = 1) const;

			const Glib::ustring& get_name(void) const { return m_name; }
			double get_vrotate(void) const { return m_vrotate; }
			double get_mass(void) const { return m_mass; }
			const Poly1D<double>& get_gndpoly(void) const { return m_gndpoly; }
			const Poly1D<double>& get_obstpoly(void) const { return m_obstpoly; }

			void set_name(const Glib::ustring& n) { m_name = n; }
			void set_vrotate(double v) { m_vrotate = v; }
			void set_mass(double m) { m_mass = m; }
			Poly1D<double>& get_gndpoly(void) { return m_gndpoly; }
			Poly1D<double>& get_obstpoly(void) { return m_obstpoly; }
			void set_gndpoly(const Poly1D<double>& p) { m_gndpoly = p; }
			void set_obstpoly(const Poly1D<double>& p) { m_obstpoly = p; }

			double calculate_gnddist(double da) const;
			double calculate_obstdist(double da) const;
			void calculate(double& gnddist, double& obstdist, double da, double headwind = 0) const;
			void calculate(double& gnddist, double& obstdist, double da, double headwind, double mass) const;

			template<class Archive> void hibernate(Archive& ar) {
				ar.io(m_name);
				ar.io(m_vrotate);
				ar.io(m_mass);
			        m_gndpoly.hibernate(ar);
				m_obstpoly.hibernate(ar);
				uint32_t n(m_points.size());
				ar.ioleb(n);
				if (ar.is_load())
					m_points.resize(n);
				for (points_t::iterator i(m_points.begin()), e(m_points.end()); i != e; ++i)
					i->hibernate(ar);
			}

		protected:
			Glib::ustring m_name;
			double m_vrotate;
			double m_mass;
			typedef Poly1D<double> poly_t;
			poly_t m_gndpoly;
			poly_t m_obstpoly;
			typedef std::vector<Point> points_t;
			points_t m_points;
		};

		Distances(unit_t distunit = unit_m, double altfactor = 1, double tempfactor = 1, double tempoffset = 273.15, double distfactor = 1);

		unsigned int get_nrtakeoffdist(void) const { return m_takeoffdist.size(); }
		const Distance& get_takeoffdist(unsigned int i) const { return m_takeoffdist[i]; }
		Distance& get_takeoffdist(unsigned int i) { return m_takeoffdist[i]; }

		unsigned int get_nrldgdist(void) const { return m_ldgdist.size(); }
		const Distance& get_ldgdist(unsigned int i) const { return m_ldgdist[i]; }
		Distance& get_ldgdist(unsigned int i) { return m_ldgdist[i]; }

		bool recalculatepoly(bool force = false);

		void clear_takeoffdists(void);
		void add_takeoffdist(const Distance& d);
		void clear_ldgdists(void);
		void add_ldgdist(const Distance& d);

		unit_t get_distunit(void) const { return m_distunit; }

		void load_xml(const xmlpp::Element *el);
		void save_xml(xmlpp::Element *el) const;

		const std::string& get_remark(void) const { return m_remark; }

		template<class Archive> void hibernate(Archive& ar) {
			uint32_t n(m_takeoffdist.size());
			ar.ioleb(n);
			if (ar.is_load())
				m_takeoffdist.resize(n);
			for (distances_t::iterator i(m_takeoffdist.begin()), e(m_takeoffdist.end()); i != e; ++i)
				i->hibernate(ar);
			n = m_ldgdist.size();
			ar.ioleb(n);
			if (ar.is_load())
				m_ldgdist.resize(n);
			for (distances_t::iterator i(m_ldgdist.begin()), e(m_ldgdist.end()); i != e; ++i)
				i->hibernate(ar);
			ar.io(m_altfactor);
			ar.io(m_tempfactor);
			ar.io(m_tempoffset);
			ar.io(m_distfactor);
			ar.iouint8(m_distunit);
			ar.io(m_remark);
		}

	protected:
		typedef std::vector<Distance> distances_t;
		distances_t m_takeoffdist;
		distances_t m_ldgdist;
		std::string m_remark;
		double m_altfactor;
		double m_tempfactor;
		double m_tempoffset;
		double m_distfactor;
		unit_t m_distunit;
	};

	class ClimbDescent {
	public:
		typedef enum {
			mode_rateofclimb,
			mode_timetoaltitude,
			mode_invalid
		} mode_t;

		class Point {
		public:
			Point(double pa = 0, double rate = 0, double fuelflow = 0, double cas = 0);

			void load_xml(const xmlpp::Element *el, mode_t& mode, double altfactor = 1,
				      double ratefactor = 1, double fuelflowfactor = 1, double casfactor = 1,
				      double timefactor = 1, double fuelfactor = 1, double distfactor = 1);
			void save_xml(xmlpp::Element *el, mode_t mode, double altfactor = 1,
				      double ratefactor = 1, double fuelflowfactor = 1, double casfactor = 1,
				      double timefactor = 1, double fuelfactor = 1, double distfactor = 1) const;

			double get_pressurealt(void) const { return m_pa; }

			void set_pressurealt(double x) { m_pa = x; }

			// Rate of Climb Mode
			double get_rate(void) const { return m_rate; }
			double get_fuelflow(void) const { return m_fuelflow; }
			double get_cas(void) const { return m_cas; }

			void set_rate(double x) { m_rate = x; }
			void set_fuelflow(double x) { m_fuelflow = x; }		
			void set_cas(double x) { m_cas = x; }

			void scale_rate(double x) { m_rate *= x; }

			// Param to Altitude Mode
			double get_time(void) const { return m_rate; }
			double get_fuel(void) const { return m_fuelflow; }
			double get_dist(void) const { return m_cas; }

			void set_time(double x) { m_rate = x; }
			void set_fuel(double x) { m_fuelflow = x; }		
			void set_dist(double x) { m_cas = x; }

			std::ostream& print(std::ostream& os, mode_t mode) const;

			template<class Archive> void hibernate(Archive& ar) {
				ar.io(m_pa);
				ar.io(m_rate);
				ar.io(m_fuelflow);
				ar.io(m_cas);
			}

		protected:
			double m_pa;
			double m_rate;
			double m_fuelflow;
			double m_cas;
		};

		ClimbDescent(const std::string& name = "", double mass = std::numeric_limits<double>::quiet_NaN(), double isaoffs = 0);

		void calculate(double& rate, double& fuelflow, double& cas, double pa) const;
		const std::string& get_name(void) const { return m_name; }
		double get_mass(void) const { return m_mass; }
		double get_isaoffset(void) const { return m_isaoffset; }
		double get_ceiling(void) const { return m_ceiling; }
		double get_climbtime(void) const { return m_climbtime; }
		mode_t get_mode(void) const { return m_mode; }

		double time_to_altitude(double t) const;
		double time_to_distance(double t) const;
		double time_to_fuel(double t) const;
		double time_to_climbrate(double t) const;
		double time_to_tas(double t) const;
		double time_to_fuelflow(double t) const;
		double altitude_to_time(double a) const;
		double distance_to_time(double d) const;

		bool recalculatepoly(bool force = false);
		void recalculateformass(double newmass);
		void recalculateforatmo(double isaoffs = 0, double qnh = IcaoAtmosphere<double>::std_sealevel_pressure);
		ClimbDescent interpolate(double t, const ClimbDescent& cd) const;

		void load_xml(const xmlpp::Element *el, double altfactor, double ratefactor, double fuelflowfactor, double casfactor,
			      double timefactor, double fuelfactor, double distfactor);
		void save_xml(xmlpp::Element *el, double altfactor, double ratefactor, double fuelflowfactor, double casfactor,
			      double timefactor, double fuelfactor, double distfactor) const;
		void add_point(const Point& pt);
		void clear_points(void);
		bool has_points(void) const { return !m_points.empty(); }
		void recalc_points() { recalc_points(get_mode()); }
		void recalc_points(mode_t m);
		void limit_ceiling(double lim);
		void set_ceiling(double ceil);

		double max_point_pa(void) const;

		void set_point_vy(double vy);
		void set_mass(double mass);
		void set_descent_rate(double rate);
		void set_name(const std::string& n) { m_name = n; }

		CheckErrors check(double minmass, double maxmass, bool descent) const;

		std::ostream& print(std::ostream& os, const std::string& indent) const;

		typedef Poly1D<double> poly_t;
		const poly_t& get_ratepoly(void) const { return m_ratepoly; }
		const poly_t& get_fuelflowpoly(void) const { return m_fuelflowpoly; }
		const poly_t& get_caspoly(void) const { return m_caspoly; }
		const poly_t& get_climbaltpoly(void) const { return m_climbaltpoly; }
		const poly_t& get_climbdistpoly(void) const { return m_climbdistpoly; }
		const poly_t& get_climbfuelpoly(void) const { return m_climbfuelpoly; }

		template<class Archive> void hibernate(Archive& ar) {
			m_ratepoly.hibernate(ar);
			m_fuelflowpoly.hibernate(ar);
			m_caspoly.hibernate(ar);
			m_climbaltpoly.hibernate(ar);
			m_climbdistpoly.hibernate(ar);
			m_climbfuelpoly.hibernate(ar);
			uint32_t n(m_points.size());
			ar.ioleb(n);
			if (ar.is_load())
				m_points.resize(n);
			for (points_t::iterator i(m_points.begin()), e(m_points.end()); i != e; ++i)
				i->hibernate(ar);
			ar.io(m_name);
			ar.io(m_mass);
			ar.io(m_isaoffset);
			ar.io(m_ceiling);
			ar.io(m_climbtime);
			ar.iouint8(m_mode);
		}

	protected:
		poly_t m_ratepoly;
		poly_t m_fuelflowpoly;
		poly_t m_caspoly;
		poly_t m_climbaltpoly;
		poly_t m_climbdistpoly;
		poly_t m_climbfuelpoly;
		typedef std::vector<Point> points_t;
		points_t m_points;
		std::string m_name;
		double m_mass;
		double m_isaoffset;
		double m_ceiling;
		double m_climbtime;
		mode_t m_mode;

		bool recalculateratepoly(bool force = false);
		void recalculateclimbpoly(double pastart);
		bool recalculatetoclimbpoly(bool force = false);
		void recalculatetoratepoly(void);
	};

	class Climb {
	public:
		Climb(double altfactor = 1, double ratefactor = 1, double fuelflowfactor = 1, double casfactor = 1,
		      double timefactor = 1, double fuelfactor = 1, double distfactor = 1);

		ClimbDescent calculate(const std::string& name, double mass = std::numeric_limits<double>::quiet_NaN(),
				       double isaoffs = 0, double qnh = IcaoAtmosphere<double>::std_sealevel_pressure) const;
		ClimbDescent calculate(std::string& name, const std::string& dfltname, double mass = std::numeric_limits<double>::quiet_NaN(),
				       double isaoffs = 0, double qnh = IcaoAtmosphere<double>::std_sealevel_pressure) const;
		unsigned int count_curves(void) const;
		std::set<std::string> get_curves(void) const;
		double get_ceiling(void) const;

		void load_xml(const xmlpp::Element *el);
		void save_xml(xmlpp::Element *el) const;

		const std::string& get_remark(void) const { return m_remark; }

		void clear(void);
		void add(const ClimbDescent& cd);

		bool recalculatepoly(bool force = false);

		void set_point_vy(double vy);
		void set_mass(double mass);

		CheckErrors check(double minmass, double maxmass) const { return check(minmass, maxmass, false); }

		std::ostream& print(std::ostream& os, const std::string& indent = "") const;

		template<class Archive> void hibernate(Archive& ar) {
			if (ar.is_load()) {
			        clear();
				uint32_t sz(0);
				ar.ioleb(sz);
				for (; sz > 0; --sz) {
					ClimbDescent cd;
					cd.hibernate(ar);
					add(cd);
				}
			} else {
				uint32_t sz(count_curves());
				ar.ioleb(sz);
				for (curves_t::const_iterator ci(m_curves.begin()), ce(m_curves.end()); ci != ce; ++ci)
					for (massmap_t::const_iterator mi(ci->second.begin()), me(ci->second.end()); mi != me; ++mi)
						for (isamap_t::const_iterator ii(mi->second.begin()), ie(mi->second.end()); ii != ie && sz > 0; ++ii, --sz)
							const_cast<ClimbDescent&>(ii->second).hibernate(ar);
				for (; sz > 0; --sz) {
					ClimbDescent cd;
					cd.hibernate(ar);
				}
			}
			ar.io(m_altfactor);
			ar.io(m_ratefactor);
			ar.io(m_fuelflowfactor);
			ar.io(m_timefactor);
			ar.io(m_fuelfactor);
			ar.io(m_distfactor);
			ar.io(m_remark);
		}

	protected:
		typedef std::map<double, ClimbDescent> isamap_t;
		typedef std::map<double, isamap_t> massmap_t;
		typedef std::map<std::string, massmap_t> curves_t;
		curves_t m_curves;
		std::string m_remark;
		double m_altfactor;
		double m_ratefactor;
		double m_fuelflowfactor;
		double m_casfactor;
		double m_timefactor;
		double m_fuelfactor;
		double m_distfactor;

		ClimbDescent calculate(const massmap_t& ci, double mass, double isaoffs, double qnh) const;
		ClimbDescent calculate(const isamap_t& mi, double isaoffs, double qnh) const;
		const ClimbDescent *find_single_curve(void) const;
		CheckErrors check(double minmass, double maxmass, bool descent) const;
	};

	class Cruise;

	class Descent : public Climb {
	public:
		Descent(double altfactor = 1, double ratefactor = 1, double fuelflowfactor = 1, double casfactor = 1,
			double timefactor = 1, double fuelfactor = 1, double distfactor = 1);

		ClimbDescent calculate(const std::string& name, double mass = std::numeric_limits<double>::quiet_NaN(),
				       double isaoffs = 0, double qnh = IcaoAtmosphere<double>::std_sealevel_pressure) const;
		ClimbDescent calculate(std::string& name, const std::string& dfltname, double mass = std::numeric_limits<double>::quiet_NaN(),
				       double isaoffs = 0, double qnh = IcaoAtmosphere<double>::std_sealevel_pressure) const;

		void load_xml(const xmlpp::Element *el);
		void save_xml(xmlpp::Element *el) const;

		void set_default(propulsion_t prop, const Cruise& cruise, const Climb& climb);

 		CheckErrors check(double minmass, double maxmass) const { return Climb::check(minmass, maxmass, true); }

		template<class Archive> void hibernate(Archive& ar) {
			Climb::hibernate(ar);
		}

	protected:
		ClimbDescent calculate(const massmap_t& ci, double mass, double isaoffs, double qnh) const;
		using Climb::calculate;
		double get_rate(void) const;
	};

	class Cruise {
	protected:
		class PistonPower;

	public:
		class CruiseEngineParams;

		class Point {
		public:
			Point(double pa = 0, double bhp = 0, double tas = 0, double fuelflow = 0);

			void load_xml(const xmlpp::Element *el, PistonPower& pp, double altfactor = 1, double tasfactor = 1, double fuelfactor = 1, double isaoffs = 0);
			void save_xml(xmlpp::Element *el, double altfactor = 1, double tasfactor = 1, double fuelfactor = 1) const;

			double get_pressurealt(void) const { return m_pa; }
			double get_bhp(void) const { return m_bhp; }
			double get_tas(void) const { return m_tas; }
			double get_fuelflow(void) const { return m_fuelflow; }

			int compare(const Point& x) const;
			bool operator==(const Point& x) const { return !compare(x); }
			bool operator!=(const Point& x) const { return !operator==(x); }
			bool operator<(const Point& x) const { return compare(x) < 0; }
			bool operator<=(const Point& x) const { return compare(x) <= 0; }
			bool operator>(const Point& x) const { return compare(x) > 0; }
			bool operator>=(const Point& x) const { return compare(x) >= 0; }

			template<class Archive> void hibernate(Archive& ar) {
				ar.io(m_pa);
				ar.io(m_bhp);
				ar.io(m_tas);
				ar.io(m_fuelflow);
			}

		protected:
			double m_pa;
			double m_bhp;
			double m_tas;
			double m_fuelflow;
		};

		class PointRPMMP : public Point {
		public:
			PointRPMMP(double pa = 0, double rpm = 0, double mp = 0, double bhp = 0, double tas = 0, double fuelflow = 0);

			void load_xml(const xmlpp::Element *el, PistonPower& pp, double altfactor = 1, double tasfactor = 1, double fuelfactor = 1, double isaoffs = 0);
			void save_xml(xmlpp::Element *el, double altfactor = 1, double tasfactor = 1, double fuelfactor = 1) const;

			double get_rpm(void) const { return m_rpm; }
			double get_mp(void) const { return m_mp; }

			template<class Archive> void hibernate(Archive& ar) {
				Point::hibernate(ar);
				ar.io(m_rpm);
				ar.io(m_mp);
			}

		protected:
			double m_rpm;
			double m_mp;
		};

		class Curve : public std::set<Point> {
		public:
			typedef enum {
				flags_none        = 0,
				flags_interpolate = 1 << 0,
				flags_hold        = 1 << 1
			} flags_t;

			Curve(const std::string& name = "", flags_t flags = flags_none, double mass = std::numeric_limits<double>::quiet_NaN(),
			      double isaoffs = 0, double rpm = std::numeric_limits<double>::quiet_NaN());
			void add_point(const Point& pt);
			void clear_points(void);

			const std::string& get_name(void) const { return m_name; }
			void set_name(const std::string& n) { m_name = n; }
			flags_t get_flags(void) const { return m_flags; }
			void set_flags(flags_t f) { m_flags = f; }

			double get_mass(void) const { return m_mass; }
			void set_mass(double mass);
			double get_isaoffset(void) const { return m_isaoffset; }
			double get_rpm(void) const { return m_rpm; }

			std::pair<double,double> get_bhp_range(void) const;
			bool is_bhp_const(void) const;

			void calculate(double& tas, double& fuelflow, double& pa, CruiseEngineParams& ep) const;

			CheckErrors check(double minmass, double maxmass) const;

			int compare(const Curve& x) const;
			bool operator==(const Curve& x) const { return !compare(x); }
			bool operator!=(const Curve& x) const { return !operator==(x); }
			bool operator<(const Curve& x) const { return compare(x) < 0; }
			bool operator<=(const Curve& x) const { return compare(x) <= 0; }
			bool operator>(const Curve& x) const { return compare(x) > 0; }
			bool operator>=(const Curve& x) const { return compare(x) >= 0; }

			void load_xml(const xmlpp::Element *el, PistonPower& pp, double altfactor = 1, double tasfactor = 1, double fuelfactor = 1);
			void save_xml(xmlpp::Element *el, double altfactor = 1, double tasfactor = 1, double fuelfactor = 1) const;

			template<class Archive> void hibernate(Archive& ar) {
				ar.io(m_name);
				ar.io(m_mass);
				ar.io(m_isaoffset);
				ar.io(m_rpm);
				ar.iouint8(m_flags);
				uint32_t sz(size());
				ar.ioleb(sz);
				if (ar.is_load()) {
					clear();
					for (; sz > 0; --sz) {
						Point p;
						p.hibernate(ar);
						insert(p);
					}
				} else {
					for (iterator i(begin()), e(end()); i != e; ++i)
						const_cast<Point&>(*i).hibernate(ar);
				}
			}

		protected:
			class FlagName {
			public:
				const char *m_name;
				flags_t m_flag;
			};
			static const FlagName flagnames[];

			std::string m_name;
			double m_mass;
			double m_isaoffset;
			double m_rpm;
			flags_t m_flags;
		};

		class CurveRPMMP : public std::set<PointRPMMP> {
		public:
			CurveRPMMP(const std::string& name = "");

			const std::string& get_name(void) const { return m_name; }
			void set_name(const std::string& n) { m_name = n; }

			bool is_mp(void) const;
			std::pair<double,double> get_bhp_range(void) const;
			std::pair<double,double> get_rpm_range(void) const;
			bool is_bhp_const(void) const;
			bool is_rpm_const(void) const;

			void load_xml(const xmlpp::Element *el, PistonPower& pp, double altfactor = 1, double tasfactor = 1, double fuelfactor = 1);
			void save_xml(xmlpp::Element *el, double altfactor = 1, double tasfactor = 1, double fuelfactor = 1) const;

			operator Curve(void) const;

		protected:
			std::string m_name;
		};

		class CruiseEngineParams {
		public:
			CruiseEngineParams(double bhp = std::numeric_limits<double>::quiet_NaN(),
					   double rpm = std::numeric_limits<double>::quiet_NaN(), 
					   double mp = std::numeric_limits<double>::quiet_NaN(),
					   const std::string& name = "", Curve::flags_t flags = Curve::flags_interpolate);
			CruiseEngineParams(const std::string& name, Curve::flags_t flags = Curve::flags_interpolate);

			double get_bhp(void) const { return m_bhp; }
			double get_rpm(void) const { return m_rpm; }
			double get_mp(void) const { return m_mp; }
			const std::string& get_name(void) const { return m_name; }
			std::string& get_name(void) { return m_name; }
			Curve::flags_t get_flags(void) const { return m_flags; }
			bool is_unset(void) const;
			int compare(const CruiseEngineParams& x) const;
			bool operator==(const CruiseEngineParams& x) const { return !compare(x); }
			bool operator!=(const CruiseEngineParams& x) const { return !operator==(x); }
			bool operator<(const CruiseEngineParams& x) const { return compare(x) < 0; }
			bool operator<=(const CruiseEngineParams& x) const { return compare(x) <= 0; }
			bool operator>(const CruiseEngineParams& x) const { return compare(x) > 0; }
			bool operator>=(const CruiseEngineParams& x) const { return compare(x) >= 0; }

			void set_bhp(double x = std::numeric_limits<double>::quiet_NaN()) { m_bhp = x; }
			void set_rpm(double x = std::numeric_limits<double>::quiet_NaN()) { m_rpm = x; }
			void set_mp(double x = std::numeric_limits<double>::quiet_NaN()) { m_mp = x; }
			void set_name(const std::string& n) { m_name = n; }
			void set_flags(Curve::flags_t f) { m_flags = f; }

			std::ostream& print(std::ostream& os) const;

		protected:
			std::string m_name;
			double m_bhp;
			double m_rpm;
			double m_mp;
			Curve::flags_t m_flags;
		};

		class EngineParams : public CruiseEngineParams {
		public:
			EngineParams(double bhp = std::numeric_limits<double>::quiet_NaN(),
				     double rpm = std::numeric_limits<double>::quiet_NaN(), 
				     double mp = std::numeric_limits<double>::quiet_NaN(),
				     const std::string& name = "", Curve::flags_t flags = Curve::flags_interpolate,
				     const std::string& climbname = "", const std::string& descentname = "");
			EngineParams(const std::string& name, Curve::flags_t flags = Curve::flags_interpolate,
				     const std::string& climbname = "", const std::string& descentname = "");

			const std::string& get_climbname(void) const { return m_climbname; }
			std::string& get_climbname(void) { return m_climbname; }
			const std::string& get_descentname(void) const { return m_descentname; }
			std::string& get_descentname(void) { return m_descentname; }
			bool is_unset(void) const;
			int compare(const EngineParams& x) const;
			bool operator==(const EngineParams& x) const { return !compare(x); }
			bool operator!=(const EngineParams& x) const { return !operator==(x); }
			bool operator<(const EngineParams& x) const { return compare(x) < 0; }
			bool operator<=(const EngineParams& x) const { return compare(x) <= 0; }
			bool operator>(const EngineParams& x) const { return compare(x) > 0; }
			bool operator>=(const EngineParams& x) const { return compare(x) >= 0; }

			void set_climbname(const std::string& n) { m_climbname = n; }
			void set_descentname(const std::string& n) { m_descentname = n; }

			std::ostream& print(std::ostream& os) const;

		protected:
			std::string m_climbname;
			std::string m_descentname;
		};

		Cruise(double altfactor = 1, double tasfactor = 1, double fuelfactor = 1, double tempfactor = 1, double bhpfactor = 1);

		void calculate(propulsion_t prop, double& tas, double& fuelflow, double& pa, double& mass, double& isaoffs, CruiseEngineParams& ep) const;

		void load_xml(const xmlpp::Element *el, double maxbhp);
		void save_xml(xmlpp::Element *el, double maxbhp) const;

		const std::string& get_remark(void) const { return m_remark; }

		void add_curve(const Curve& c);
		void clear_curves(void);
		unsigned int count_curves(void) const;
		bool has_hold(void) const;
		std::set<std::string> get_curves(void) const;
		Curve::flags_t get_curve_flags(const std::string& name) const;
		typedef std::vector<Point> points_t;
		typedef std::vector<PointRPMMP> rpmmppoints_t;
		void set_points(const rpmmppoints_t& pts, double maxbhp);
		void build_altmap(propulsion_t prop);
		void set_mass(double mass);
		bool has_fixedpitch(void) const { return m_pistonpower.has_fixedpitch(); }
		bool has_variablepitch(void) const { return m_pistonpower.has_variablepitch(); }

		CheckErrors check(double minmass, double maxmass) const;

		template<class Archive> void hibernate(Archive& ar) {
			if (ar.is_load()) {
			        clear_curves();
				uint32_t sz(0);
				ar.ioleb(sz);
				for (; sz > 0; --sz) {
					Curve c;
					c.hibernate(ar);
					add_curve(c);
				}
			} else {
				uint32_t sz(count_curves());
				ar.ioleb(sz);
				for (curves_t::const_iterator ci(m_curves.begin()), ce(m_curves.end()); ci != ce; ++ci)
					for (massmap_t::const_iterator mi(ci->second.begin()), me(ci->second.end()); mi != me; ++mi)
						for (isamap_t::const_iterator ii(mi->second.begin()), ie(mi->second.end()); ii != ie && sz > 0; ++ii, --sz)
							const_cast<Curve&>(ii->second).hibernate(ar);
				for (; sz > 0; --sz) {
					Curve c;
					c.hibernate(ar);
				}
			}
			m_pistonpower.hibernate(ar);
			ar.io(m_altfactor);
			ar.io(m_tasfactor);
			ar.io(m_fuelfactor);
			ar.io(m_tempfactor);
			ar.io(m_bhpfactor);
			ar.io(m_remark);
		}

	protected:
		class Point1 {
		public:
			Point1(double tas = 0, double fuelflow = 0) : m_tas(tas), m_fuelflow(fuelflow) {}
			double get_tas(void) const { return m_tas; }
			double get_fuelflow(void) const { return m_fuelflow; }

		protected:
			double m_bhp;
			double m_tas;
			double m_fuelflow;
		};

		class PistonPowerBHPRPM {
		public:
			PistonPowerBHPRPM(double bhp, double rpm);
			double get_bhp(void) const { return m_bhp; }
			double get_rpm(void) const { return m_rpm; }

			void save_xml(xmlpp::Element *el, double alt, double isaoffs, double bhpfactor = 1) const;

			int compare(const PistonPowerBHPRPM& x) const;
			bool operator==(const PistonPowerBHPRPM& x) const { return !compare(x); }
			bool operator!=(const PistonPowerBHPRPM& x) const { return !operator==(x); }
			bool operator<(const PistonPowerBHPRPM& x) const { return compare(x) < 0; }
			bool operator<=(const PistonPowerBHPRPM& x) const { return compare(x) <= 0; }
			bool operator>(const PistonPowerBHPRPM& x) const { return compare(x) > 0; }
			bool operator>=(const PistonPowerBHPRPM& x) const { return compare(x) >= 0; }

			template<class Archive> void hibernate(Archive& ar) {
				ar.io(m_bhp);
				ar.io(m_rpm);
			}

		protected:
			double m_bhp;
			double m_rpm;
		};

		class PistonPowerBHPMP {
		public:
			PistonPowerBHPMP(double bhp, double mp);
			double get_bhp(void) const { return m_bhp; }
			double get_mp(void) const { return m_mp; }

			void save_xml(xmlpp::Element *el, double alt, double isaoffs, double rpm, double bhpfactor = 1) const;

			int compare(const PistonPowerBHPMP& x) const;
			bool operator==(const PistonPowerBHPMP& x) const { return !compare(x); }
			bool operator!=(const PistonPowerBHPMP& x) const { return !operator==(x); }
			bool operator<(const PistonPowerBHPMP& x) const { return compare(x) < 0; }
			bool operator<=(const PistonPowerBHPMP& x) const { return compare(x) <= 0; }
			bool operator>(const PistonPowerBHPMP& x) const { return compare(x) > 0; }
			bool operator>=(const PistonPowerBHPMP& x) const { return compare(x) >= 0; }

			template<class Archive> void hibernate(Archive& ar) {
				ar.io(m_bhp);
				ar.io(m_mp);
			}

		protected:
			double m_bhp;
			double m_mp;
		};

		class PistonPowerRPM : public std::set<PistonPowerBHPMP> {
		public:
			PistonPowerRPM(double rpm);
			double get_rpm(void) const { return m_rpm; }
			void calculate(double& bhp, double& mp) const;

			void save_xml(xmlpp::Element *el, double alt, double isaoffs, double bhpfactor = 1) const;

			int compare(const PistonPowerRPM& x) const;
			bool operator==(const PistonPowerRPM& x) const { return !compare(x); }
			bool operator!=(const PistonPowerRPM& x) const { return !operator==(x); }
			bool operator<(const PistonPowerRPM& x) const { return compare(x) < 0; }
			bool operator<=(const PistonPowerRPM& x) const { return compare(x) <= 0; }
			bool operator>(const PistonPowerRPM& x) const { return compare(x) > 0; }
			bool operator>=(const PistonPowerRPM& x) const { return compare(x) >= 0; }

			template<class Archive> void hibernate(Archive& ar) {
				ar.io(m_rpm);
				uint32_t sz(size());
				ar.ioleb(sz);
				if (ar.is_load()) {
					clear();
					for (; sz > 0; --sz) {
						PistonPowerBHPMP pp(0, 0);
						pp.hibernate(ar);
						insert(pp);
					}
				} else {
					for (iterator i(begin()), e(end()); i != e; ++i)
						const_cast<PistonPowerBHPMP&>(*i).hibernate(ar);
				}
			}

		protected:
			double m_rpm;
		};

		class PistonPowerTemp {
		public:
			PistonPowerTemp(double isaoffs);
			double get_isaoffs(void) const { return m_isaoffs; }
			void calculate(double& bhp, double& rpm, double& mp) const;

			typedef std::set<PistonPowerBHPRPM> fixedpitch_t;
			typedef std::set<PistonPowerRPM> variablepitch_t;
			fixedpitch_t& get_fixedpitch(void) { return m_fixedpitch; }
			const fixedpitch_t& get_fixedpitch(void) const { return m_fixedpitch; }
			variablepitch_t& get_variablepitch(void) { return m_variablepitch; }
			const variablepitch_t& get_variablepitch(void) const { return m_variablepitch; }

			void save_xml(xmlpp::Element *el, double alt, double tempfactor = 1, double bhpfactor = 1) const;

			bool has_fixedpitch(void) const { return !get_fixedpitch().empty(); }
			bool has_variablepitch(void) const { return !get_variablepitch().empty(); }

			int compare(const PistonPowerTemp& x) const;
			bool operator==(const PistonPowerTemp& x) const { return !compare(x); }
			bool operator!=(const PistonPowerTemp& x) const { return !operator==(x); }
			bool operator<(const PistonPowerTemp& x) const { return compare(x) < 0; }
			bool operator<=(const PistonPowerTemp& x) const { return compare(x) <= 0; }
			bool operator>(const PistonPowerTemp& x) const { return compare(x) > 0; }
			bool operator>=(const PistonPowerTemp& x) const { return compare(x) >= 0; }

			template<class Archive> void hibernate(Archive& ar) {
				ar.io(m_isaoffs);
				uint32_t sz(m_fixedpitch.size());
				ar.ioleb(sz);
				if (ar.is_load()) {
					m_fixedpitch.clear();
					for (; sz > 0; --sz) {
						PistonPowerBHPRPM pp(0, 0);
						pp.hibernate(ar);
						m_fixedpitch.insert(pp);
					}
				} else {
					for (fixedpitch_t::iterator i(m_fixedpitch.begin()), e(m_fixedpitch.end()); i != e; ++i)
						const_cast<PistonPowerBHPRPM&>(*i).hibernate(ar);
				}
				sz = m_variablepitch.size();
				ar.ioleb(sz);
				if (ar.is_load()) {
					m_variablepitch.clear();
					for (; sz > 0; --sz) {
						PistonPowerRPM pp(0);
						pp.hibernate(ar);
						m_variablepitch.insert(pp);
					}
				} else {
					for (variablepitch_t::iterator i(m_variablepitch.begin()), e(m_variablepitch.end()); i != e; ++i)
						const_cast<PistonPowerRPM&>(*i).hibernate(ar);
				}
			}

		protected:
			fixedpitch_t m_fixedpitch;
			variablepitch_t m_variablepitch;
			double m_isaoffs;
		};

		class PistonPowerPA : public std::set<PistonPowerTemp> {
		public:
			PistonPowerPA(double pa);
			double get_pa(void) const { return m_pa; }
			void calculate(double& isaoffs, double& bhp, double& rpm, double& mp) const;
			void save_xml(xmlpp::Element *el, double altfactor = 1, double tempfactor = 1, double bhpfactor = 1) const;

			bool has_fixedpitch(void) const;
			bool has_variablepitch(void) const;

			int compare(const PistonPowerPA& x) const;
			bool operator==(const PistonPowerPA& x) const { return !compare(x); }
			bool operator!=(const PistonPowerPA& x) const { return !operator==(x); }
			bool operator<(const PistonPowerPA& x) const { return compare(x) < 0; }
			bool operator<=(const PistonPowerPA& x) const { return compare(x) <= 0; }
			bool operator>(const PistonPowerPA& x) const { return compare(x) > 0; }
			bool operator>=(const PistonPowerPA& x) const { return compare(x) >= 0; }

			template<class Archive> void hibernate(Archive& ar) {
				ar.io(m_pa);
				uint32_t sz(size());
				ar.ioleb(sz);
				if (ar.is_load()) {
					clear();
					for (; sz > 0; --sz) {
						PistonPowerTemp pp(0);
						pp.hibernate(ar);
						insert(pp);
					}
				} else {
					for (iterator i(begin()), e(end()); i != e; ++i)
						const_cast<PistonPowerTemp&>(*i).hibernate(ar);
				}
			}

		protected:
			double m_pa;
		};

		class PistonPower : public std::set<PistonPowerPA> {
		public:
			PistonPower(void);
			void add(double pa, double isaoffs, double bhp, double rpm, double mp = std::numeric_limits<double>::quiet_NaN());
			void calculate(double& pa, double& isaoffs, double& bhp, double& rpm, double& mp) const;
			void load_xml(const xmlpp::Element *el, double altfactor = 1, double tempfactor = 1, double bhpfactor = 1);
			void save_xml(xmlpp::Element *el, double altfactor = 1, double tempfactor = 1, double bhpfactor = 1) const;
			void set_default(void);

			bool has_fixedpitch(void) const;
			bool has_variablepitch(void) const;

			template<class Archive> void hibernate(Archive& ar) {
				uint32_t sz(size());
				ar.ioleb(sz);
				if (ar.is_load()) {
					clear();
					for (; sz > 0; --sz) {
						PistonPowerPA pp(0);
						pp.hibernate(ar);
						insert(pp);
					}
				} else {
					for (iterator i(begin()), e(end()); i != e; ++i)
						const_cast<PistonPowerPA&>(*i).hibernate(ar);
				}
			}
		};

		typedef std::map<double,Point1> bhpbhpmap_t;
		typedef std::map<double,bhpbhpmap_t> bhpaltmap_t;
		typedef std::map<double,bhpaltmap_t> bhpisamap_t;
		typedef std::map<double,bhpisamap_t> bhpmap_t;
		bhpmap_t m_bhpmap;
		typedef std::map<double, Curve> isamap_t;
		typedef std::map<double, isamap_t> massmap_t;
		typedef std::map<std::string, massmap_t> curves_t;
		curves_t m_curves;
		PistonPower m_pistonpower;
		std::string m_remark;
		double m_altfactor;
		double m_tasfactor;
		double m_fuelfactor;
		double m_tempfactor;
		double m_bhpfactor;

		void calculate(double& mass, double& isaoffs, double& pa, double& bhp, double& tas, double& ff) const;
		void calculate(bhpmap_t::const_iterator it, double& isaoffs, double& pa, double& bhp, double& tas, double& ff) const;
		void calculate(bhpisamap_t::const_iterator it, double& pa, double& bhp, double& tas, double& ff) const;
		void calculate(bhpaltmap_t::const_iterator it, double& bhp, double& tas, double& ff) const;
		
		Curve::flags_t calculate(curves_t::const_iterator it, double& tas, double& fuelflow, double& pa, double& mass, double& isaoffs, CruiseEngineParams& ep) const;
		Curve::flags_t calculate(massmap_t::const_iterator it, double& tas, double& fuelflow, double& pa, double& isaoffs, CruiseEngineParams& ep) const;
		std::pair<double,double> get_bhp_range(curves_t::const_iterator it) const;
		std::pair<double,double> get_bhp_range(massmap_t::const_iterator it) const;
	};

	class OtherInfo {
	public:
		OtherInfo(const Glib::ustring& category = "", const Glib::ustring& text = "");
		const Glib::ustring& get_category(void) const { return m_category; }
		const Glib::ustring& get_text(void) const { return m_text; }
		bool operator<(const OtherInfo& x) const { return m_category < x.m_category; }
		bool is_valid(void) const { return !m_category.empty(); }
		static bool is_category_valid(const Glib::ustring& cat);
		bool is_category_valid(void) const { return is_category_valid(get_category()); }
		static std::string get_valid_text(const std::string& txt);
		static Glib::ustring get_valid_text(const Glib::ustring& txt);
		Glib::ustring get_valid_text(void) const { return get_valid_text(get_text()); }

		template<class Archive> void hibernate(Archive& ar) {
			ar.io(m_category);
			ar.io(m_text);
		}

	protected:
		Glib::ustring m_category;
		Glib::ustring m_text;
	};

	class CheckError {
	public:
		class MessageOStream {
		public:
			MessageOStream(CheckError& ce) : m_ce(ce) {}
			MessageOStream(const MessageOStream& mos) : m_ce(mos.m_ce) {}
			~MessageOStream() { m_ce.append_message(m_oss.str()); }
			operator std::ostream&(void) { return m_oss; }
			std::ostream& get_ostream(void) { return m_oss; }
			template<typename T> inline std::ostream& operator<<(T x) { return m_oss << x; }

		protected:
			CheckError& m_ce;
			std::ostringstream m_oss;
		};
		
		typedef enum {
			type_climb,
			type_descent,
			type_cruise,
			type_unknown
		} type_t;

		typedef enum {
			severity_warning,
			severity_error
		} severity_t;

		CheckError(type_t typ = type_unknown, severity_t sev = severity_error,
			   const std::string& n = "", double mass = std::numeric_limits<double>::quiet_NaN(),
			   double isaoffs = std::numeric_limits<double>::quiet_NaN(), const std::string& msg = "");
		CheckError(const ClimbDescent& cd, bool descent, severity_t sev = severity_error,
			   double tmin = std::numeric_limits<double>::quiet_NaN(),
			   double tmax = std::numeric_limits<double>::quiet_NaN());
		CheckError(const Cruise::Curve& c, severity_t sev = severity_error);

		const std::string& get_name(void) const { return m_name; }
		void set_name(const std::string& n) { m_name = n; }
		const std::string& get_mode(void) const { return m_mode; }
		void set_mode(const std::string& n) { m_mode = n; }
		const std::string& get_message(void) const { return m_message; }
		void set_message(const std::string& m) { m_message = m; }
		void append_message(const std::string& m) { m_message += m; }
		MessageOStream get_messageostream(void) { return MessageOStream(*this); }
		double get_mass(void) const { return m_mass; }
		void set_mass(double m = std::numeric_limits<double>::quiet_NaN()) { m_mass = m; }
		double get_isaoffset(void) const { return m_isaoffset; }
		void set_isaoffset(double i = std::numeric_limits<double>::quiet_NaN()) { m_isaoffset = i; }
		typedef std::pair<double,double> timeinterval_t;
		const timeinterval_t& get_timeinterval(void) const { return m_timeinterval; }
		double get_mintime(void) const { return m_timeinterval.first; }
		double get_maxtime(void) const { return m_timeinterval.second; }
		void set_timeinterval(const timeinterval_t& iv) { m_timeinterval = iv; }
		void set_mintime(double t) { m_timeinterval.first = t; }
		void set_maxtime(double t) { m_timeinterval.second = t; }
		type_t get_type(void) const { return m_type; }
		void set_type(type_t t = type_unknown) { m_type = t; }
		severity_t get_severity(void) const { return m_severity; }
		void set_severity(severity_t s = severity_error) { m_severity = s; }

		std::string to_str(void) const;
#ifdef HAVE_JSONCPP
		Json::Value to_json(void) const;
#endif

	protected:
		std::string m_name;
		std::string m_mode;
		std::string m_message;
		double m_mass;
		double m_isaoffset;
		timeinterval_t m_timeinterval;
		type_t m_type;
		severity_t m_severity;
	};

	class CheckErrors : public std::vector<CheckError> {
	public:
		void set_name(const std::string& n);
		void set_mode(const std::string& m);
		void set_mass(double m = std::numeric_limits<double>::quiet_NaN());
		void set_isaoffset(double i = std::numeric_limits<double>::quiet_NaN());
		void set_type(CheckError::type_t t = CheckError::type_unknown);
		void set_severity(CheckError::severity_t s = CheckError::severity_error);

		iterator add(const CheckErrors& ce);
		CheckError::MessageOStream add(const CheckError& ce);
		CheckError::MessageOStream add(CheckError::type_t typ = CheckError::type_unknown, CheckError::severity_t sev = CheckError::severity_error,
					       const std::string& n = "", double mass = std::numeric_limits<double>::quiet_NaN(),
					       double isaoffs = std::numeric_limits<double>::quiet_NaN(), const std::string& msg = "") {
			return add(CheckError(typ, sev, n, mass, isaoffs, msg));
		}
		CheckError::MessageOStream add(const ClimbDescent& cd, bool descent, CheckError::severity_t sev = CheckError::severity_error,
					       double tmin = std::numeric_limits<double>::quiet_NaN(),
					       double tmax = std::numeric_limits<double>::quiet_NaN()) {
			return add(CheckError(cd, descent, sev, tmin, tmax));
		}
		CheckError::MessageOStream add(const Cruise::Curve& c, CheckError::severity_t sev = CheckError::severity_error) {
			return add(CheckError(c, sev));
		}

		std::ostream& print(std::ostream& os, const std::string& indent = "") const;
#ifdef HAVE_JSONCPP
		Json::Value to_json(void) const;
#endif
	};

protected:
	typedef std::set<OtherInfo> otherinfo_t;

public:
	typedef enum {
		pbn_a1 = 1 << 0,
		pbn_b1 = 1 << 1,
		pbn_b2 = 1 << 2,
		pbn_b3 = 1 << 3,
		pbn_b4 = 1 << 4,
		pbn_b5 = 1 << 5,
		pbn_b6 = 1 << 6,
		pbn_c1 = 1 << 7,
		pbn_c2 = 1 << 8,
		pbn_c3 = 1 << 9,
		pbn_c4 = 1 << 10,
		pbn_d1 = 1 << 11,
		pbn_d2 = 1 << 12,
		pbn_d3 = 1 << 13,
		pbn_d4 = 1 << 14,
		pbn_l1 = 1 << 15,
		pbn_o1 = 1 << 16,
		pbn_o2 = 1 << 17,
		pbn_o3 = 1 << 18,
		pbn_o4 = 1 << 19,
		pbn_s1 = 1 << 20,
		pbn_s2 = 1 << 21,
		pbn_t1 = 1 << 22,
		pbn_t2 = 1 << 23,
		pbn_rnav = pbn_b1 | pbn_b2 | pbn_b3 | pbn_b4 | pbn_b5, // B1 B2 B3 B4 B5
		pbn_gnss = pbn_b1 | pbn_b2 | pbn_c1 | pbn_c2 | pbn_d1 | pbn_d2 | pbn_o1 | pbn_o2, // B1 B2 C1 C2 D1 D2 O1 O2
		pbn_dmedme = pbn_b1 | pbn_b3 | pbn_c1 | pbn_c3 | pbn_d1 | pbn_d3 | pbn_o1 | pbn_o3, // B1 B3 C1 C3 D1 D3 O1 O3
		pbn_vordme = pbn_b1 | pbn_b4, // B1 B4
		pbn_dmedmeiru = pbn_b1 | pbn_b5 | pbn_c1 | pbn_c4 | pbn_d1 | pbn_d4 | pbn_o1 | pbn_o4, //  B1 B5 C1 C4 D1 D4 O1 O4
		pbn_loran = pbn_b5, // B5
		pbn_dme = pbn_dmedme | pbn_vordme | pbn_dmedmeiru,
		pbn_none = 0,
		pbn_all = pbn_a1 | pbn_b1 | pbn_b2 | pbn_b3 | pbn_b4 | pbn_b5 | pbn_b6
		| pbn_c1 | pbn_c2 | pbn_c3 | pbn_c4
		| pbn_d1 | pbn_d2 | pbn_d3 | pbn_d4
		| pbn_l1
		| pbn_o1 | pbn_o2 | pbn_o3 | pbn_o4
		| pbn_s1 | pbn_s2 | pbn_t1 | pbn_t2
	} pbn_t;

	typedef enum {
		gnssflags_none    = 0,
		gnssflags_gps     = 1 << 0,
		gnssflags_sbas    = 1 << 1,
		gnssflags_glonass = 1 << 2,
		gnssflags_galileo = 1 << 3,
		gnssflags_qzss    = 1 << 4,
		gnssflags_beidou  = 1 << 5,
		gnssflags_baroaid = 1 << 6
	} gnssflags_t;

	Aircraft(void);

	WeightBalance& get_wb(void) { return m_wb; }
	const WeightBalance& get_wb(void) const { return m_wb; }

	Distances& get_dist(void) { return m_dist; }
	const Distances& get_dist(void) const { return m_dist; }

	Climb& get_climb(void) { return m_climb; }
	const Climb& get_climb(void) const { return m_climb; }

	Descent& get_descent(void) { return m_descent; }
	const Descent& get_descent(void) const { return m_descent; }

	Cruise& get_cruise(void) { return m_cruise; }
	const Cruise& get_cruise(void) const { return m_cruise; }

	typedef otherinfo_t::const_iterator otherinfo_const_iterator_t;
	otherinfo_const_iterator_t otherinfo_begin(void) const { return m_otherinfo.begin(); }
	otherinfo_const_iterator_t otherinfo_end(void) const { return m_otherinfo.end(); }
	void otherinfo_add(const OtherInfo& oi);
	void otherinfo_add(const Glib::ustring& category, const Glib::ustring& text = "") { otherinfo_add(OtherInfo(category, text)); }
	const OtherInfo& otherinfo_find(const Glib::ustring& category) const;

	const Glib::ustring& get_callsign(void) const { return m_callsign; }
 	const Glib::ustring& get_manufacturer(void) const { return m_manufacturer; }
 	const Glib::ustring& get_model(void) const { return m_model; }
 	const Glib::ustring& get_year(void) const { return m_year; }
	Glib::ustring get_description(void) const;
	static std::string get_aircrafttypeclass(const Glib::ustring& acfttype);
	std::string get_aircrafttypeclass(void) const;
	const Glib::ustring& get_icaotype(void) const { return m_icaotype; }
	const Glib::ustring& get_equipment(void) const { return m_equipment; }
	const Glib::ustring& get_transponder(void) const { return m_transponder; }
	pbn_t get_pbn(void) const { return m_pbn; }
	static Glib::ustring get_pbn_string(pbn_t pbn);
	Glib::ustring get_pbn_string(void) const { return get_pbn_string(m_pbn); }
	gnssflags_t get_gnssflags(void) const { return m_gnssflags; }
	static Glib::ustring get_gnssflags_string(gnssflags_t gnssflags);
	Glib::ustring get_gnssflags_string(void) const { return get_gnssflags_string(m_gnssflags); }
	const Glib::ustring& get_colormarking(void) const { return m_colormarking; }
	const Glib::ustring& get_emergencyradio(void) const { return m_emergencyradio; }
	const Glib::ustring& get_survival(void) const { return m_survival; }
	const Glib::ustring& get_lifejackets(void) const { return m_lifejackets; }
	const Glib::ustring& get_dinghies(void) const { return m_dinghies; }
	const Glib::ustring& get_picname(void) const { return m_picname; }
	const Glib::ustring& get_crewcontact(void) const { return m_crewcontact; }
	const Glib::ustring& get_homebase(void) const { return m_homebase; }
	char get_wakecategory(void) const;
	double get_mrm(void) const { return m_mrm; }
	double get_mtom(void) const { return m_mtom; }
	double get_mlm(void) const { return m_mlm; }
	double get_mzfm(void) const { return m_mlm; }
	double get_mrm_kg(void) const;
	double get_mtom_kg(void) const;
	double get_mlm_kg(void) const;
	double get_mzfm_kg(void) const;
	double get_vr0(void) const { return m_vr0; }
	double get_vr1(void) const { return m_vr1; }
	double get_vx0(void) const { return m_vx0; }
	double get_vx1(void) const { return m_vx1; }
	double get_vy(void) const { return m_vy; }
	double get_vs0(void) const { return m_vs0; }
	double get_vs1(void) const { return m_vs1; }
	double get_vno(void) const { return m_vno; }
	double get_vne(void) const { return m_vne; }
	double get_vref0(void) const { return m_vref0; }
	double get_vref1(void) const { return m_vref1; }
	double get_va(void) const { return m_va; }
	double get_vfe(void) const { return m_vfe; }
	double get_vgearext(void) const { return m_vgearext; }
	double get_vgearret(void) const { return m_vgearret; }
	double get_vbestglide(void) const { return m_vbestglide; }
	double get_glideslope(void) const { return m_glideslope; }
	double get_vdescent(void) const { return m_vdescent; }
	double get_fuelmass(void) const { return m_fuelmass; }
	unit_t get_fuelunit(void) const { return m_fuelunit; }
	double get_taxifuel(void) const { return m_taxifuel; }
	double get_taxifuelflow(void) const { return m_taxifuelflow; }
	double get_maxbhp(void) const { return m_maxbhp; }
	double get_contingencyfuel(void) const { return m_contingencyfuel; }
	const std::string& get_remark(void) const { return m_remark; }
	propulsion_t get_propulsion(void) const { return m_propulsion; }
	bool is_constantspeed(void) const { return get_propulsion() == propulsion_constantspeed; }
	bool is_freecirculation(void) const { return m_freecirculation; }

	void set_callsign(const Glib::ustring& x) { m_callsign = x; }
 	void set_manufacturer(const Glib::ustring& x) { m_manufacturer = x; }
 	void set_model(const Glib::ustring& x) { m_model = x; }
 	void set_year(const Glib::ustring& x) { m_year = x; }
	void set_icaotype(const Glib::ustring& x) { m_icaotype = x; }
	void set_equipment(const Glib::ustring& x) { m_equipment = x; }
	void set_transponder(const Glib::ustring& x) { m_transponder = x; }
	void set_pbn(pbn_t x) { m_pbn = x; }
	void set_pbn(const Glib::ustring& x) { m_pbn = parse_pbn(x); }
	void set_gnssflags(gnssflags_t x) { m_gnssflags = x; }
	void set_gnssflags(const Glib::ustring& x) { m_gnssflags = parse_gnssflags(x); }
	void set_colormarking(const Glib::ustring& x) { m_colormarking = x; }
	void set_emergencyradio(const Glib::ustring& x) { m_emergencyradio = x; }
	void set_survival(const Glib::ustring& x) { m_survival = x; }
	void set_lifejackets(const Glib::ustring& x) { m_lifejackets = x; }
	void set_dinghies(const Glib::ustring& x) { m_dinghies = x; }
	void set_picname(const Glib::ustring& x) { m_picname = x; }
	void set_crewcontact(const Glib::ustring& x) { m_crewcontact = x; }
	void set_homebase(const Glib::ustring& x) { m_homebase = x; }
	void set_mrm(double x) { m_mrm = x; }
	void set_mtom(double x) { m_mtom = x; }
	void set_mlm(double x) { m_mlm = x; }
	void set_mzfm(double x) { m_mzfm = x; }
	void set_vr0(double x) { m_vr0 = x; }
	void set_vr1(double x) { m_vr1 = x; }
	void set_vx0(double x) { m_vx0 = x; }
	void set_vx1(double x) { m_vx1 = x; }
	void set_vy(double x) { m_vy = x; }
	void set_vs0(double x) { m_vs0 = x; }
	void set_vs1(double x) { m_vs1 = x; }
	void set_vno(double x) { m_vno = x; }
	void set_vne(double x) { m_vne = x; }
	void set_vref0(double x) { m_vref0 = x; }
	void set_vref1(double x) { m_vref1 = x; }
	void set_va(double x) { m_va = x; }
	void set_vfe(double x) { m_vfe = x; }
	void set_vgearext(double x) { m_vgearext = x; }
	void set_vgearret(double x) { m_vgearret = x; }
	void set_vbestglide(double x) { m_vbestglide = x; }
	void set_glideslope(double x) { m_glideslope = x; }
	void set_vdescent(double x) { m_vdescent = x; }
	void set_fuelmass(double x) { m_fuelmass = x; }
	void set_taxifuel(double x) { m_taxifuel = x; }
	void set_taxifuelflow(double x) { m_taxifuelflow = x; }
	void set_maxbhp(double x) { m_maxbhp = x; }
	void set_contingencyfuel(double x) { m_contingencyfuel = x; }
	void set_remark(const std::string& x) { m_remark = x; }
	void set_propulsion(propulsion_t p) { m_propulsion = p; }
	void set_freecirculation(bool x) { m_freecirculation = x; }

	void load_xml(const xmlpp::Element *el);
	void save_xml(xmlpp::Element *el) const;

	bool load_file(const Glib::ustring& filename);
	void save_file(const Glib::ustring& filename) const;

	bool load_string(const Glib::ustring& data);
	Glib::ustring save_string(void) const;

	bool load_opsperf(const OperationsPerformance::Aircraft& acft);

	static pbn_t parse_pbn(const Glib::ustring& x);
	static void pbn_fix_equipment(Glib::ustring& equipment, pbn_t pbn);
	static void pbn_fix_equipment(std::string& equipment, pbn_t pbn);
	void pbn_fix_equipment(void) { pbn_fix_equipment(m_equipment, m_pbn); }
	static gnssflags_t parse_gnssflags(const Glib::ustring& x);

	double get_useable_fuelmass(void) const { return m_wb.get_useable_fuelmass(); }
	double get_total_fuelmass(void) const { return m_wb.get_total_fuelmass(); }

	double get_useable_fuel(void) const;
	double get_total_fuel(void) const;

	double get_useable_oilmass(void) const { return m_wb.get_useable_oilmass(); }
	double get_total_oilmass(void) const { return m_wb.get_total_oilmass(); }

	static unit_t parse_unit(const std::string& s);
	static double convert(unit_t fromunit, unit_t tounit, double val = 1.0,
			      WeightBalance::Element::flags_t flags = WeightBalance::Element::flag_none);
	double convert_fuel(unit_t fromunit, unit_t tounit, double val = 1.0) const;
	bool add_auto_units(void);
	const std::string& get_fuelname(void) const;

	ClimbDescent calculate_climb(const std::string& name, double mass = std::numeric_limits<double>::quiet_NaN(),
				     double isaoffs = 0, double qnh = IcaoAtmosphere<double>::std_sealevel_pressure) const;
	ClimbDescent calculate_descent(const std::string& name, double mass = std::numeric_limits<double>::quiet_NaN(),
				       double isaoffs = 0, double qnh = IcaoAtmosphere<double>::std_sealevel_pressure) const;
	ClimbDescent calculate_climb(Cruise::EngineParams& ep, double mass = std::numeric_limits<double>::quiet_NaN(),
				     double isaoffs = 0, double qnh = IcaoAtmosphere<double>::std_sealevel_pressure) const;
	ClimbDescent calculate_descent(Cruise::EngineParams& ep, double mass = std::numeric_limits<double>::quiet_NaN(),
				       double isaoffs = 0, double qnh = IcaoAtmosphere<double>::std_sealevel_pressure) const;
	void calculate_cruise(double& tas, double& fuelflow, double& pa, double& mass, double& isaoffs, double& qnh, Cruise::CruiseEngineParams& ep) const;

	opsrules_t get_opsrules(void) const { return m_opsrules; }
	void set_opsrules(opsrules_t r) { m_opsrules = r; }

	static time_t get_opsrules_holdtime(opsrules_t r, propulsion_t p);
	time_t get_opsrules_holdtime(void) const { return get_opsrules_holdtime(get_opsrules(), get_propulsion()); }
	static bool is_opsrules_holdcruisealt(opsrules_t r);
	bool is_opsrules_holdcruisealt(void) const { return is_opsrules_holdcruisealt(get_opsrules()); }

	CheckErrors check(void) const;

	class Endurance {
	public:
		Endurance(void);
		const Cruise::CruiseEngineParams get_engineparams(void) const { return m_ep; }
		double get_dist(void) const { return m_dist; }
		double get_fuel(void) const { return m_fuel; }
		double get_tas(void) const { return m_tas; }
		double get_fuelflow(void) const { return m_fuelflow; }
		double get_pa(void) const { return m_pa; }
		double get_tkoffelev(void) const { return m_tkoffelev; }
		time_t get_endurance(void) const { return m_endurance; }

	protected:
		Cruise::CruiseEngineParams m_ep;
		double m_dist;
		double m_fuel;
		double m_tas;
		double m_fuelflow;
		double m_pa;
		double m_tkoffelev;
		time_t m_endurance;
	};

	Endurance compute_endurance(double pa,
				    const Cruise::CruiseEngineParams& ep = Cruise::CruiseEngineParams(),
				    double tkoffelev = std::numeric_limits<double>::quiet_NaN());

	void navfplan(const Glib::ustring& templatefile, Engine& engine, const FPlanRoute& fplan,
		      const Cruise::EngineParams& epcruise = Cruise::EngineParams(),
		      const WeightBalance::elementvalues_t& wbev = WeightBalance::elementvalues_t()) const;

	void navfplan_gnumeric(const std::string& outfile, Engine& engine, const FPlanRoute& fplan,
			       const std::vector<FPlanAlternate>& altn,
			       const Cruise::EngineParams& epcruise = Cruise::EngineParams(),
			       double fuel_on_board = std::numeric_limits<double>::quiet_NaN(),
			       const WeightBalance::elementvalues_t& wbev = WeightBalance::elementvalues_t(),
			       const std::string& templatefile = PACKAGE_DATA_DIR "/navlog.xml",
			       gint64 gfsminreftime = -1, gint64 gfsmaxreftime = -1,
			       gint64 gfsminefftime = -1, gint64 gfsmaxefftime = -1,
			       bool hidewbpage = false, unit_t fuelunit = unit_invalid, unit_t massunit = unit_invalid,
			       const std::string& atcfpl = "", const std::string& atcfplc = "",
			       const std::string& atcfplcdct = "", const std::string& atcfplcall = "") const;

	static std::ostream& navfplan_latex_defpathfrom(std::ostream& os);
	static std::ostream& navfplan_latex_defpathto(std::ostream& os);
	static std::ostream& navfplan_latex_defpathmid(std::ostream& os);

	std::ostream& navfplan_latex(std::ostream& os, Engine& engine, const FPlanRoute& fplan,
				     const std::vector<FPlanAlternate>& altn,
				     const std::vector<FPlanRoute>& altnfpl,
				     const Cruise::EngineParams& epcruise = Cruise::EngineParams(),
				     gint64 gfsminreftime = -1, gint64 gfsmaxreftime = -1,
				     gint64 gfsminefftime = -1, gint64 gfsmaxefftime = -1,
				     double tofuel = std::numeric_limits<double>::quiet_NaN(),
				     unit_t fuelunit = unit_invalid, const std::string& atcfplc = "",
				     const std::string& servicename = "www.autorouter.eu",
				     const std::string& servicelink = "http://www.autorouter.eu") const;

	std::ostream& navfplan_lualatex(std::ostream& os, Engine& engine, const FPlanRoute& fplan,
					const std::vector<FPlanAlternate>& altn,
					const std::vector<FPlanRoute>& altnfpl,
					const Cruise::EngineParams& epcruise = Cruise::EngineParams(),
					gint64 gfsminreftime = -1, gint64 gfsmaxreftime = -1,
					gint64 gfsminefftime = -1, gint64 gfsmaxefftime = -1,
					double tofuel = std::numeric_limits<double>::quiet_NaN(),
					unit_t fuelunit = unit_invalid, const std::string& atcfplc = "",
					const std::string& servicename = "www.autorouter.eu",
					const std::string& servicelink = "http://www.autorouter.eu") const;

	std::ostream& massbalance_latex(std::ostream& os, const FPlanRoute& fplan,
					const std::vector<FPlanAlternate>& altn,
					const Cruise::EngineParams& epcruise = Cruise::EngineParams(),
					double fuel_on_board = std::numeric_limits<double>::quiet_NaN(),
					const WeightBalance::elementvalues_t& wbev = WeightBalance::elementvalues_t(),
					unit_t fuelunit = unit_invalid, unit_t massunit = unit_invalid,
					double *tomass = (double *)0) const;

	std::ostream& distances_latex(std::ostream& os, double tomass, const FPlanRoute& fplan,
				      const std::vector<FPlanAlternate>& altn,
				      const std::vector<FPlanAlternate>& taltn,
				      const std::vector<FPlanAlternate>& raltn) const;

	std::ostream& distances_latex(std::ostream& os, DbQueryInterface<DbBaseElements::Airport>& arptdb,
				      double tomass, const FPlanRoute& fplan,
				      const std::vector<FPlanAlternate>& altn,
				      const std::vector<FPlanAlternate>& taltn,
				      const std::vector<FPlanAlternate>& raltn) const;

	std::ostream& climb_latex(std::ostream& os, double tomass, double isaoffs, double qnh, unit_t fu, const FPlanWaypoint& wpt) const;
	std::ostream& descent_latex(std::ostream& os, double dmass, double isaoffs, double qnh, unit_t fu, const FPlanWaypoint& wpt) const;

	WeightBalance::elementvalues_t prepare_wb(const WeightBalance::elementvalues_t& wbev = WeightBalance::elementvalues_t(),
						  double fuel_on_board = std::numeric_limits<double>::quiet_NaN(),
						  unit_t fuelunit = unit_invalid, unit_t massunit = unit_invalid, unit_t oilunit = unit_invalid) const;

	void compute_masses(double *rampmass, double *tomass, double *tofuel, double fuel_on_board = std::numeric_limits<double>::quiet_NaN(),
			    time_t taxitime = 0, const WeightBalance::elementvalues_t& wbev = WeightBalance::elementvalues_t()) const;

	static std::string to_ascii(const Glib::ustring& x);
	static std::string to_luastring(const std::string& x);

	template<class Archive> void hibernate(Archive& ar) {
		m_wb.hibernate(ar);
		m_dist.hibernate(ar);
		m_climb.hibernate(ar);
		m_descent.hibernate(ar);
		m_cruise.hibernate(ar);
		uint32_t n(m_otherinfo.size());
		ar.ioleb(n);
		if (ar.is_load()) {
			m_otherinfo.clear();
			for (; n; --n) {
				OtherInfo o;
				o.hibernate(ar);
				m_otherinfo.insert(o);
			}
		} else {
			for (otherinfo_t::iterator i(m_otherinfo.begin()), e(m_otherinfo.end()); i != e; ++i)
				const_cast<OtherInfo&>(*i).hibernate(ar);
		}
		ar.io(m_callsign);
		ar.io(m_manufacturer);
		ar.io(m_model);
		ar.io(m_year);
		ar.io(m_icaotype);
		ar.io(m_equipment);
		ar.io(m_transponder);
		ar.io(m_colormarking);
		ar.io(m_emergencyradio);
		ar.io(m_survival);
		ar.io(m_lifejackets);
		ar.io(m_dinghies);
		ar.io(m_picname);
		ar.io(m_crewcontact);
		ar.io(m_homebase);
		ar.io(m_mrm);
		ar.io(m_mtom);
		ar.io(m_mlm);
		ar.io(m_mzfm);
		ar.io(m_vr0);
		ar.io(m_vr1);
		ar.io(m_vx0);
		ar.io(m_vx1);
		ar.io(m_vy);
		ar.io(m_vs0);
		ar.io(m_vs1);
		ar.io(m_vno);
		ar.io(m_vne);
		ar.io(m_vref0);
		ar.io(m_vref1);
		ar.io(m_va);
		ar.io(m_vfe);
		ar.io(m_vgearext);
		ar.io(m_vgearret);
		ar.io(m_vbestglide);
		ar.io(m_glideslope);
		ar.io(m_vdescent);
		ar.io(m_fuelmass);
		ar.io(m_taxifuel);
		ar.io(m_taxifuelflow);
		ar.io(m_maxbhp);
		ar.iouint8(m_pbn);
		ar.iouint8(m_gnssflags);
		ar.iouint8(m_fuelunit);
		ar.iouint8(m_opsrules);
		ar.iouint8(m_propulsion);
		ar.iouint8(m_freecirculation);
		ar.io(m_remark);
		if (ar.is_load())
			get_cruise().build_altmap(get_propulsion());
	}

	static constexpr double avgas_density = 0.721;
	static constexpr double jeta_density = 0.804;
	static constexpr double diesel_density = 0.82;
	static constexpr double oil_density = 0.860;
	static constexpr double quart_to_liter = 0.94635295;
	static constexpr double usgal_to_liter = 3.7854118;
	static constexpr double lb_to_kg = 1. / 2.2046226;
	static constexpr double kg_to_lb = 2.2046226;

protected:

	typedef char aircraft_type_class_t[10];
	static const aircraft_type_class_t aircraft_type_class_db[];

	class AircraftTypeClassDbCompare {
	public:
		bool operator()(const char *a, const char *b) const { return strncmp(a, b, 4) < 0; }
	};

	WeightBalance m_wb;
	Distances m_dist;
	Climb m_climb;
	Descent m_descent;
	Cruise m_cruise;
	otherinfo_t m_otherinfo;
	// some basic data
	Glib::ustring m_callsign;
	Glib::ustring m_manufacturer;
 	Glib::ustring m_model;
 	Glib::ustring m_year;
	Glib::ustring m_icaotype;
	Glib::ustring m_equipment;
	Glib::ustring m_transponder;
	Glib::ustring m_colormarking;
	Glib::ustring m_emergencyradio;
	Glib::ustring m_survival;
	Glib::ustring m_lifejackets;
	Glib::ustring m_dinghies;
	Glib::ustring m_picname;
	Glib::ustring m_crewcontact;
	Glib::ustring m_homebase;
	std::string m_remark;
	double m_mrm;
	double m_mtom;
	double m_mlm;
	double m_mzfm;
	double m_vr0;
	double m_vr1;
	double m_vx0;
	double m_vx1;
	double m_vy;
	double m_vs0;
	double m_vs1;
	double m_vno;
	double m_vne;
	double m_vref0;
	double m_vref1;
	double m_va;
	double m_vfe;
	double m_vgearext;
	double m_vgearret;
	double m_vbestglide;
	double m_glideslope;
	double m_vdescent;
	double m_fuelmass;
	double m_taxifuel;
	double m_taxifuelflow;
	double m_maxbhp;
	double m_contingencyfuel;
	pbn_t m_pbn;
	gnssflags_t m_gnssflags;
	unit_t m_fuelunit;
	propulsion_t m_propulsion;
	opsrules_t m_opsrules;
	bool m_freecirculation;

	class NavFPlan;
	class EnduranceW;
	class GnmCellSpec;
	class GnmCellSpecs;

	static std::string get_text(const xmlpp::Node *n);
	void load_xml_fs(const xmlpp::Element *el);
	void check_aircraft_type_class(void);

	std::ostream& distances_latex(std::ostream& os, double tomass,
				      const std::vector<FPlanWaypoint>& alt,
				      const std::vector<DbBaseElements::Airport>& arpt) const;

	class UnicodeMapping {
	public:
		gunichar ch;
		const char *txt;
	};

	static const UnicodeMapping unicodemapping[];

	class UnicodeMappingCompare {
	public:
		bool operator()(gunichar a, gunichar b) const { return a < b; }
		bool operator()(const UnicodeMapping& a, gunichar b) const { return operator()(a.ch, b); }
		bool operator()(gunichar a, const UnicodeMapping& b) const { return operator()(a, b.ch); }
		bool operator()(const UnicodeMapping *a, gunichar b) const {
			if (!a)
				return true;
			return operator()(a->ch, b);
		}
		bool operator()(gunichar a, const UnicodeMapping *b) const {
			if (!b)
				return false;
			return operator()(a, b->ch);
		}
		bool operator()(const UnicodeMapping *a, const UnicodeMapping *b) const {
			if (!b)
				return false;
			if (!a)
				return true;
			return operator()(a->ch, b->ch);
		}
	};

	static bool to_ascii_iconv(std::string& r, const Glib::ustring& x);
	std::ostream& climbdescent_latex(const ClimbDescent& cd, bool rev, std::ostream& os, double xmass, double isaoffs, double qnh, unit_t fu, const FPlanWaypoint& wpt) const;
	std::ostream& write_lua_point(std::ostream& os, const std::string& ident, Aircraft::unit_t fuelunit,
				      const FPlanWaypoint& wpt, time_t deptime, double totalfuel, double tofuel, double cumdist, double remdist) const;
	std::ostream& write_lua_path(std::ostream& os, const std::string& ident, Aircraft::unit_t fuelunit,
				     const FPlanWaypoint& wpt, const FPlanWaypoint& wptn) const;
};

inline Aircraft::WeightBalance::Element::flags_t operator|(Aircraft::WeightBalance::Element::flags_t x, Aircraft::WeightBalance::Element::flags_t y) { return (Aircraft::WeightBalance::Element::flags_t)((unsigned int)x | (unsigned int)y); }
inline Aircraft::WeightBalance::Element::flags_t operator&(Aircraft::WeightBalance::Element::flags_t x, Aircraft::WeightBalance::Element::flags_t y) { return (Aircraft::WeightBalance::Element::flags_t)((unsigned int)x & (unsigned int)y); }
inline Aircraft::WeightBalance::Element::flags_t operator^(Aircraft::WeightBalance::Element::flags_t x, Aircraft::WeightBalance::Element::flags_t y) { return (Aircraft::WeightBalance::Element::flags_t)((unsigned int)x ^ (unsigned int)y); }
inline Aircraft::WeightBalance::Element::flags_t operator~(Aircraft::WeightBalance::Element::flags_t x) { return (Aircraft::WeightBalance::Element::flags_t)~(unsigned int)x; }
inline Aircraft::WeightBalance::Element::flags_t& operator|=(Aircraft::WeightBalance::Element::flags_t& x, Aircraft::WeightBalance::Element::flags_t y) { x = x | y; return x; }
inline Aircraft::WeightBalance::Element::flags_t& operator&=(Aircraft::WeightBalance::Element::flags_t& x, Aircraft::WeightBalance::Element::flags_t y) { x = x & y; return x; }
inline Aircraft::WeightBalance::Element::flags_t& operator^=(Aircraft::WeightBalance::Element::flags_t& x, Aircraft::WeightBalance::Element::flags_t y) { x = x ^ y; return x; }

inline Aircraft::pbn_t operator|(Aircraft::pbn_t x, Aircraft::pbn_t y) { return (Aircraft::pbn_t)((unsigned int)x | (unsigned int)y); }
inline Aircraft::pbn_t operator&(Aircraft::pbn_t x, Aircraft::pbn_t y) { return (Aircraft::pbn_t)((unsigned int)x & (unsigned int)y); }
inline Aircraft::pbn_t operator^(Aircraft::pbn_t x, Aircraft::pbn_t y) { return (Aircraft::pbn_t)((unsigned int)x ^ (unsigned int)y); }
inline Aircraft::pbn_t operator~(Aircraft::pbn_t x) { return (Aircraft::pbn_t)~(unsigned int)x; }
inline Aircraft::pbn_t& operator|=(Aircraft::pbn_t& x, Aircraft::pbn_t y) { x = x | y; return x; }
inline Aircraft::pbn_t& operator&=(Aircraft::pbn_t& x, Aircraft::pbn_t y) { x = x & y; return x; }
inline Aircraft::pbn_t& operator^=(Aircraft::pbn_t& x, Aircraft::pbn_t y) { x = x ^ y; return x; }

inline Aircraft::gnssflags_t operator|(Aircraft::gnssflags_t x, Aircraft::gnssflags_t y) { return (Aircraft::gnssflags_t)((unsigned int)x | (unsigned int)y); }
inline Aircraft::gnssflags_t operator&(Aircraft::gnssflags_t x, Aircraft::gnssflags_t y) { return (Aircraft::gnssflags_t)((unsigned int)x & (unsigned int)y); }
inline Aircraft::gnssflags_t operator^(Aircraft::gnssflags_t x, Aircraft::gnssflags_t y) { return (Aircraft::gnssflags_t)((unsigned int)x ^ (unsigned int)y); }
inline Aircraft::gnssflags_t operator~(Aircraft::gnssflags_t x) { return (Aircraft::gnssflags_t)~(unsigned int)x; }
inline Aircraft::gnssflags_t& operator|=(Aircraft::gnssflags_t& x, Aircraft::gnssflags_t y) { x = x | y; return x; }
inline Aircraft::gnssflags_t& operator&=(Aircraft::gnssflags_t& x, Aircraft::gnssflags_t y) { x = x & y; return x; }
inline Aircraft::gnssflags_t& operator^=(Aircraft::gnssflags_t& x, Aircraft::gnssflags_t y) { x = x ^ y; return x; }

inline Aircraft::unitmask_t operator|(Aircraft::unitmask_t x, Aircraft::unitmask_t y) { return (Aircraft::unitmask_t)((unsigned int)x | (unsigned int)y); }
inline Aircraft::unitmask_t operator&(Aircraft::unitmask_t x, Aircraft::unitmask_t y) { return (Aircraft::unitmask_t)((unsigned int)x & (unsigned int)y); }
inline Aircraft::unitmask_t operator^(Aircraft::unitmask_t x, Aircraft::unitmask_t y) { return (Aircraft::unitmask_t)((unsigned int)x ^ (unsigned int)y); }
inline Aircraft::unitmask_t operator~(Aircraft::unitmask_t x) { return (Aircraft::unitmask_t)~(unsigned int)x; }
inline Aircraft::unitmask_t& operator|=(Aircraft::unitmask_t& x, Aircraft::unitmask_t y) { x = x | y; return x; }
inline Aircraft::unitmask_t& operator&=(Aircraft::unitmask_t& x, Aircraft::unitmask_t y) { x = x & y; return x; }
inline Aircraft::unitmask_t& operator^=(Aircraft::unitmask_t& x, Aircraft::unitmask_t y) { x = x ^ y; return x; }

inline Aircraft::Cruise::Curve::flags_t operator|(Aircraft::Cruise::Curve::flags_t x, Aircraft::Cruise::Curve::flags_t y) { return (Aircraft::Cruise::Curve::flags_t)((unsigned int)x | (unsigned int)y); }
inline Aircraft::Cruise::Curve::flags_t operator&(Aircraft::Cruise::Curve::flags_t x, Aircraft::Cruise::Curve::flags_t y) { return (Aircraft::Cruise::Curve::flags_t)((unsigned int)x & (unsigned int)y); }
inline Aircraft::Cruise::Curve::flags_t operator^(Aircraft::Cruise::Curve::flags_t x, Aircraft::Cruise::Curve::flags_t y) { return (Aircraft::Cruise::Curve::flags_t)((unsigned int)x ^ (unsigned int)y); }
inline Aircraft::Cruise::Curve::flags_t operator~(Aircraft::Cruise::Curve::flags_t x) { return (Aircraft::Cruise::Curve::flags_t)~(unsigned int)x; }
inline Aircraft::Cruise::Curve::flags_t& operator|=(Aircraft::Cruise::Curve::flags_t& x, Aircraft::Cruise::Curve::flags_t y) { x = x | y; return x; }
inline Aircraft::Cruise::Curve::flags_t& operator&=(Aircraft::Cruise::Curve::flags_t& x, Aircraft::Cruise::Curve::flags_t y) { x = x & y; return x; }
inline Aircraft::Cruise::Curve::flags_t& operator^=(Aircraft::Cruise::Curve::flags_t& x, Aircraft::Cruise::Curve::flags_t y) { x = x ^ y; return x; }

const std::string& to_str(Aircraft::ClimbDescent::mode_t m);
const std::string& to_str(Aircraft::CheckError::type_t t);
const std::string& to_str(Aircraft::CheckError::severity_t s);
const std::string& to_str(Aircraft::propulsion_t prop);
const std::string& to_str(Aircraft::opsrules_t r);
const std::string& to_str(Aircraft::unit_t u, bool lng = false);
std::string to_str(Aircraft::unitmask_t u, bool lng = false);

#endif /* AIRCRAFT_H */
