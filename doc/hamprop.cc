// NASA 1971 / Hamilton Standard Prop Model

#include <octave/config.h>

#include <iostream>

#include <octave/defun-dld.h>
#include <octave/error.h>
#include <octave/oct-obj.h>
#include <octave/pager.h>
#include <octave/symtab.h>
#include <octave/variables.h>

#include "propeller.h"
#include "proptbl.cc"


Propeller::Propeller(unsigned int nrblades, double diam_ft, double af)
	: m_diam(std::max(diam_ft, 0.1)), m_af(std::max(std::min(af, 200.), 80.)),
	  m_bladeanglemin(0.), m_bladeanglemax(90.), m_nrblades(std::max(std::min(nrblades, 8U), 2U))
{
}

void Propeller::set_blades(unsigned int nrblades)
{
	m_nrblades = std::max(std::min(nrblades, 8U), 2U);
}

void Propeller::set_activityfactor(double af)
{
	m_af = std::max(std::min(af, 200.), 80.);
}

void Propeller::set_diameter(double diam_ft)
{
	m_diam = std::max(diam_ft, 0.1);
}

void Propeller::set_pitch_limits(double min, double max)
{
	if (min > max)
		std::swap(min, max);
	m_bladeanglemin = min;
	m_bladeanglemax = max;
}

void Propeller::set_pitch(double ang)
{
	m_bladeanglemin = m_bladeanglemax = ang;
}

double Propeller::polyeval(const double *poly, unsigned int order, double x)
{
	if (!order || !poly)
		return 0;
	--order;
	double y(poly[order]);
	while (order > 0) {
		--order;
		y = y * x + poly[order];
	}
	return y;
}

double Propeller::poly2eval(const double poly[15], double x0, double x1)
{
	// x0: J; x1: bldang
	double x0_2, x1_2, x0_3, x1_3, y = 0;
	double x[15];
	x[0] = 1;
	x[1] = x0;
	x[2] = x1;
	x[3] = x0_2 = x0 * x0;
	x[4] = x0 * x1;
	x[5] = x1_2 = x1 * x1;
	x[6] = x0_3 = x0_2 * x0;
	x[7] = x0_2 * x1;
	x[8] = x0 * x1_2;
	x[9] = x1_3 = x1_2 * x1;
	x[10] = x0_3 * x0;
	x[11] = x0_3 * x1;
	x[12] = x0_2 * x1_2;
	x[13] = x0 * x1_3;
	x[14] = x1_3 * x1;
	for (unsigned int i = 0; i < 15; ++i)
		y += poly[i] * x[i];
	return y;
}

double Propeller::poly2deriv0(const double poly[15], double x0, double x1)
{
	// x0: J; x1: bldang
	double x0_2, x1_2, y = 0;
	double x[15];
	x[0] = 0;
	x[1] = 1;
	x[2] = 0;
	x[3] = 2 * x0;
	x[4] = x1;
	x[5] = 0;
	x[6] = 3 * (x0_2 = x0 * x0);
	x[7] = 2 * x0 * x1;
	x[8] = x1_2 = x1 * x1;
	x[9] = 0;
	x[10] = 4 * x0_2 * x0;
	x[11] = 3 * x0_2 * x1;
	x[12] = 2 * x0 * x1_2;
	x[13] = x1_2 * x1;
	x[14] = 0;
	for (unsigned int i = 0; i < 15; ++i)
		y += poly[i] * x[i];
	return y;
}

double Propeller::poly2deriv1(const double poly[15], double x0, double x1)
{
	// x0: J; x1: bldang
	double x0_2, x1_2, y = 0;
	double x[15];
	x[0] = 0;
	x[1] = 0;
	x[2] = 1;
	x[3] = 0;
	x[4] = x0;
	x[5] = 2 * x1;
	x[6] = 0;
	x[7] = x0_2 = x0 * x0;
	x[8] = 2 * x0 * x1;
	x[9] = 3 * (x1_2 = x1 * x1);
	x[10] = 0;
	x[11] = x0_2 * x0;
	x[12] = 2 * x0_2 * x1;
	x[13] = 3 * x0 * x1_2;
	x[14] = 4 * x1_2 * x1;
	for (unsigned int i = 0; i < 15; ++i)
		y += poly[i] * x[i];
	return y;
}

double Propeller::calc_paf(double J, double af)
{
	if (J <= 0)
		return polyeval(poly_afcp[0], sizeof(poly_afcp[0])/sizeof(poly_afcp[0][0]), af);
	if (J >= 0.5)
		return polyeval(poly_afcp[1], sizeof(poly_afcp[1])/sizeof(poly_afcp[1][0]), af);
	J *= 2;
	double x0(polyeval(poly_afcp[0], sizeof(poly_afcp[0])/sizeof(poly_afcp[0][0]), af));
	double x1(polyeval(poly_afcp[1], sizeof(poly_afcp[1])/sizeof(poly_afcp[1][0]), af));
	return (1 - J) * x0 + J * x1;
}

double Propeller::calc_taf(double J, double af)
{
	if (J <= 0)
		return polyeval(poly_afct[0], sizeof(poly_afct[0])/sizeof(poly_afct[0][0]), af);
	if (J >= 0.5)
		return polyeval(poly_afct[1], sizeof(poly_afct[1])/sizeof(poly_afct[1][0]), af);
	J *= 2;
	double x0(polyeval(poly_afct[0], sizeof(poly_afct[0])/sizeof(poly_afct[0][0]), af));
	double x1(polyeval(poly_afct[1], sizeof(poly_afct[1])/sizeof(poly_afct[1][0]), af));
	return (1 - J) * x0 + J * x1;
}

double Propeller::calc_bldang_from_cpe(double J, double cpe) const
{
	if (true)
		std::cerr << "number of blades: " << m_nrblades << " AF " << m_af << std::endl;
	double bldang(0.5 * (m_bladeanglemin + m_bladeanglemax));
	for (unsigned int i = 0; i < 16; ++i) {
		double epsilon(calc_cpe(J, bldang) - cpe);
		double depsilon(calc_cpe_deriv_bldang(J, bldang));
		if (true)
			std::cerr << "calc_bldang_from_cpe: iter " << i << " J " << J << " CPE " << cpe
				  << " beta " << bldang << " epsilon " << epsilon << " depsilon " << depsilon << std::endl;
		if (depsilon == 0 || fabs(epsilon) < 1e-6)
			break;
		bldang -= epsilon / depsilon;
	}
	return bldang;
}

double Propeller::shp_to_thrust(double& pitch, double& efficiency, double& rpm, double shp, double velocity) const
{
	efficiency = 0;
	double beta(m_bladeanglemin);
	double diam4(m_diam * m_diam);
	diam4 *= diam4;
	double diam5(diam4 * m_diam);
	double rpm0(rpm);
	if (!is_fixed_pitch()) {
		double J(101.4 * velocity / (rpm0 * m_diam));
		double CP(0.5e11 * shp / (rpm0 * rpm0 * rpm0 * diam5));
		double PAF(calc_paf(J));
		double CPE(CP * PAF);
		beta = calc_bldang_from_cpe(J, CPE);
		if (beta < m_bladeanglemin) {
			beta = m_bladeanglemin;
		} else if (beta > m_bladeanglemax) {
			beta = m_bladeanglemax;
		} else {
			pitch = beta;
			double CTE(calc_cte(J, beta));
			double TAF(calc_taf(J));
			double CT(CTE / TAF);
			efficiency = CT / CP * J;
			return 0.661e-6 * CT * rpm0 * rpm0 * diam4;
		}
	}
	// fixed pitch iteration
	pitch = beta;
	double rpmx(rpm0);
	for (unsigned int i = 0; i < 16; ++i) {
		double J(101.4 * velocity / (rpmx * m_diam));
		double CP(0.5e11 * shp / (rpm0 * rpmx * rpmx * diam5));
		double PAF(calc_paf(J));
		double CPE(CP * PAF);
		double epsilon(CPE - calc_cpe(J, beta));
		double depsilon(-2 * CPE / rpmx + calc_cpe_deriv_J(J, beta) * J / rpmx);
		if (true)
			std::cerr << "shp_to_thrust: iter " << i << " rpm " << rpmx << " J " << J << " CPE " << CPE
				  << " beta " << beta << " epsilon " << epsilon << " depsilon " << depsilon << std::endl;
		if (depsilon == 0 || fabs(epsilon) < 1e-6)
			break;
		rpmx -= epsilon / depsilon;
		rpmx = std::max(rpmx, 100.);
	}
	double J(101.4 * velocity / (rpmx * m_diam));
	double CP(0.5e11 * shp / (rpmx * rpmx * rpmx * diam5));
	double CTE(calc_cte(J, beta));
	double TAF(calc_taf(J));
	double CT(CTE / TAF);
	efficiency = CT / CP * J;
	rpm = rpmx;		
	return 0.661e-6 * CT * rpmx * rpmx * diam4;
}

void unint(unsigned int n, const double *xa, const double *ya, double x, double& y, unsigned int& l)
{
	l = 0;
	y = std::numeric_limits<double>::quiet_NaN();
	if (!xa || !ya || n < 4) {
		l = 3;
		return;
	}
	if (x <= xa[0]) {
		y = ya[0];
		if (x < xa[0])
			l = 1;
		return;
	}
	unsigned int i;
	for (i = 1; i < n; ++i) {
		if (x == xa[i]) {
			y = ya[0];
			return;
		}
		if (xa[i] > x)
			break;
	}
	if (i >= n) {
		l = 2;
		y = ya[n - 1];
		return;
	}
	unsigned int jx1(0);
	double ra(1);
	if (i >= n - 1) {
		jx1 = n - 4;
		ra = 0;
	} else if (i > 1) {
		jx1 = i - 2;
		ra = (xa[i] - x) / (xa[i] - xa[i-1]);
	}
	double rb(1 - ra);
	double d[4], p[5];
	for (unsigned int i = 0; i < 2; ++i) {
		p[i] = xa[jx1 + i + 1] - xa[jx1 + i];
		d[i] = x - xa[jx1 + i];
	}
	d[3] = x - xa[jx1 + 3];
	p[3] = p[0] + p[1];
	p[4] = p[1] + p[2];
	y = ya[jx1] * ra/p[0] * d[1]/p[3] * d[2] +
		ya[jx1 + 1] * (-ra/p[0] * d[0]/p[1] * d[2] + rb/p[1] * d[2]/p[4] * d[3]) +
		ya[jx1 + 2] * (ra/p[1] * d[0]/p[3] * d[1] - rb/p[1] * d[1]/p[2] * d[3]) +
		ya[jx1 + 3] * rb/p[4] * d[1]/p[2] * d[2];
}

DEFUN_DLD (hamprop, args, ,
  "[thrust_shp rpm pitch efficiency ] = hamprop(shp_thrust,v,rpm,nrblades,diam,af,pitchlim,mode)\n\
\n\
Compute Thrust from Shaft Horse Power, or Vice Versa.")
{
        // The list of values to return.  See the declaration in oct-obj.h

        octave_value_list retval;

        // This stream is normally connected to the pager.

        //octave_stdout << "Hello, world!\n";

        // The arguments to this function are available in args.

        int nargin = args.length();

        if (nargin != 8) {
                print_usage();
                return retval;
        }

	dim_vector dv(1, 1);
	{
		bool dvunset(true);
		for (unsigned int i = 0; i < 6; ++i) {
			if (args(i).is_scalar_type())
				continue;
			if (!args(i).is_matrix_type()) {
				std::ostringstream oss;
				oss << "hamprop: invalid argument type (argument " << (i + 1) << ')';
				error(oss.str().c_str());
				return retval;
			}
			if (dvunset) {
				dv = args(i).dims();
				dvunset = false;
				continue;
			}
			if (args(i).dims() != dv) {
				std::ostringstream oss;
				oss << "hamprop: matrix dimensions must match (argument " << (i + 1) << ')';
				error(oss.str().c_str());
				return retval;
			}
		}
	}
	Propeller prop;
	if (args(6).is_scalar_type()) {
		prop.set_pitch(args(6).double_value());
	} else if (args(6).is_matrix_type()) {
		
		switch (args(6).dims().numel()) {
		default:
			error("hamprop: pitch limits must have one or two elements");
			return retval;

		case 1:
			prop.set_pitch(args(6).array_value()(0));
			break;

		case 2:
			prop.set_pitch_limits(args(6).array_value()(0), args(6).array_value()(1));
			break;
		}
	} else {
		error("hamprop: pitch limit error");
	}
	if (!args(7).is_string()) {
		error("hamprop: mode must be string");
		return retval;
	}
	typedef enum {
		mode_thrust,
		mode_shp
	} mode_t;
	mode_t mode(mode_thrust);
	{
		std::string s(args(7).string_value());
		if (s == "thrust") {
			mode = mode_thrust;
		} else if (s == "shp") {
			mode = mode_shp;
		} else {
			error("hamprop: mode must be \"thrust\" or \"shp\"");
			return retval;
		}
	}
	NDArray resval(dv);
	NDArray rpm(dv);
	NDArray pitch(dv);
	NDArray efficiency(dv);
	int numel(dv.numel());
	for (int i = 0; i < numel; ++i) {
		{
			unsigned int nrblades(2);
			if (args(3).is_scalar_type()) {
				nrblades = args(3).uint_value();
			} else if (args(3).is_matrix_type()) {
				nrblades = args(3).uint32_array_value()(i);
			} else {
				error("hamprop: nrblades");
			}
			prop.set_blades(nrblades);
		}
		{
			double diam(6.3333);
			if (args(4).is_scalar_type()) {
				diam = args(4).double_value();
			} else if (args(4).is_matrix_type()) {
				diam = args(4).array_value()(i);
			} else {
				error("hamprop: diameter");
			}
			prop.set_diameter(diam);
		}
		{
			double af(150);
			if (args(5).is_scalar_type()) {
				af = args(5).double_value();
			} else if (args(5).is_matrix_type()) {
				af = args(5).array_value()(i);
			} else {
				error("hamprop: activity_factor");
			}
			prop.set_activityfactor(af);
		}
		double shp_thrust(0), v(100), rpm1(2700);
		if (args(0).is_scalar_type()) {
			shp_thrust = args(0).double_value();
		} else if (args(0).is_matrix_type()) {
			shp_thrust = args(0).array_value()(i);
		} else {
			error("hamprop: shp_thrust");
		}
		if (args(1).is_scalar_type()) {
			v = args(1).double_value();
		} else if (args(1).is_matrix_type()) {
			v = args(1).array_value()(i);
		} else {
			error("hamprop: velocity");
		}
		if (args(2).is_scalar_type()) {
			rpm1 = args(2).double_value();
		} else if (args(2).is_matrix_type()) {
			rpm1 = args(2).array_value()(i);
		} else {
			error("hamprop: rpm");
		}
		switch (mode) {
		case mode_thrust:
		{
			double pitch1(0);
			double efficiency1(0);
			resval(i) = prop.shp_to_thrust(pitch1, efficiency1, rpm1, shp_thrust, v);
			rpm(i) = rpm1;
			pitch(i) = pitch1;
			efficiency(i) = efficiency1;
			break;
		}

		default:
		{
			double pitch1(0);
			double efficiency1(0);
			resval(i) = prop.shp_to_thrust(pitch1, efficiency1, rpm1, shp_thrust, v);
			rpm(i) = rpm1;
			pitch(i) = pitch1;
			efficiency(i) = efficiency1;
			break;
		}
		}
	}
	retval(0) = resval;
	retval(1) = rpm;
	retval(2) = pitch;
	retval(3) = efficiency;
	return retval;
}
