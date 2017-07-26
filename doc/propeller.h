//
// C++ Interface: propeller
//
// Description: 
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef PROPELLER_H
#define PROPELLER_H

class Propeller {
  public:
	Propeller(unsigned int nrblades = 2, double diam_ft = 6.3333, double af = 150);
	void set_blades(unsigned int nrblades = 2);
	unsigned int get_blades(void) const { return m_nrblades; }
	void set_activityfactor(double af = 150);
        double get_activityfactor(void) const { return m_af; }
	void set_diameter(double diam_ft = 6.3333);
	double get_diameter(void) const { return m_diam; }
	void set_pitch_limits(double min = 14, double max = 29);
	void set_pitch(double ang = 20);
	double get_min_pitch(void) const { return m_bladeanglemin; }
	double get_max_pitch(void) const { return m_bladeanglemax; }
	bool is_fixed_pitch(void) const { return m_bladeanglemin == m_bladeanglemax; }

	// pitch, efficiency are pure outputs, rpm is in/out, shp and velocity are pure inputs
	// thrust output needs to be corrected by rho/rho0
	// shp input needs to be corrected by rho0/rho
	double shp_to_thrust(double& pitch, double& efficiency, double& rpm, double shp, double velocity) const;

  protected:
	double m_diam;
	double m_af;
	double m_bladeanglemin;
	double m_bladeanglemax;
	unsigned int m_nrblades;

	static double polyeval(const double *poly, unsigned int order, double x);
	static double poly2eval(const double poly[15], double x0, double x1);
	static double poly2deriv0(const double poly[15], double x0, double x1);
	static double poly2deriv1(const double poly[15], double x0, double x1);
	double calc_paf(double J) const { return calc_paf(J, m_af); }
	double calc_taf(double J) const { return calc_taf(J, m_af); }
	static double calc_paf(double J, double af);
	static double calc_taf(double J, double af);
	double calc_cpe(double J, double bldang) const { return poly2eval(poly_bldangcp[m_nrblades - 2], J, bldang); }
	double calc_cte(double J, double bldang) const { return poly2eval(poly_bldangct[m_nrblades - 2], J, bldang); }
	double calc_cpe_deriv_J(double J, double bldang) const { return poly2deriv0(poly_bldangcp[m_nrblades - 2], J, bldang); }
	double calc_cte_deriv_J(double J, double bldang) const { return poly2deriv0(poly_bldangct[m_nrblades - 2], J, bldang); }
	double calc_cpe_deriv_bldang(double J, double bldang) const { return poly2deriv1(poly_bldangcp[m_nrblades - 2], J, bldang); }
	double calc_cte_deriv_bldang(double J, double bldang) const { return poly2deriv1(poly_bldangct[m_nrblades - 2], J, bldang); }
	double calc_bldang_from_cpe(double J, double cpe) const;

	static const double poly_afcp[2][6];
	static const double poly_afct[2][6];
	static const double poly_cpstal[7][4];
	static const double poly_ctstal[7][4];
	static const double poly_bldangcp[7][15];
	static const double poly_bldangct[7][15];
	static const double poly_pbl[7][13];
};

#endif /* PROPELLER_H */
